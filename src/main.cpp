#include "ESPNetworkManager.h"
#include "Free_Fonts.h"
#include "SPI.h"
#include "TFT_eSPI.h" //Μια TFT SPI βιβλιοθήκη γραφικών για τον ESP8266 (Bodmer)
#include <Arduino.h>
#include <DNSServer.h>
#include <PubSubClient.h> //Βιβλιοθήκη για δημιουργία πελάτη Mqtt (Nick O'Leary)
/** Αρχικοποίηση μεταβλητών */
//Ο μικροελεγκτής ελέγχει 2 συσκευές άρα θα δημιουργήσουμε 2 ModuleID
String ModuleID = String(ESP.getChipId());
String ModuleID2 = (ModuleID + "2");
//Η συσκευές δέχονται δεδομένα οπότε θα είναι είσοδοι
String mcu_type = "input";
//Θέματα στα οποία δέχεται και στέλνει Mqtt Μηνύματα ο Μικροελεγκτής
String myTopic = ("/" + mcu_type + "/" + ModuleID + "/" + "data");
String myTopic2 = ("/" + mcu_type + "/" + ModuleID2 + "/" + "data");
String settingsTopic = ("/" + mcu_type + "/" + ModuleID + "/" + "settings");
String settingsTopic2 = ("/" + mcu_type + "/" + ModuleID2 + "/" + "settings");
String willTopic = ("/" + mcu_type + "/" + ModuleID + "/" + "lastwill");
String willTopic2 = ("/" + mcu_type + "/" + ModuleID2 + "/" + "lastwill");
//Τρόπος ελέγχου των συσκευών(ControlType). Χρησιμοποιείτε
//για την σωστή δημιουργία εργαλείων ελέγχου στις συσκευές Android
String controlType = "motorcontrol";
String controlType2 = "lcdtext";

//Αρχικοποίηση μεταβλητών πελάτη Mqtt
const char *micro_mqtt_broker;
const char *mqtt_username;
const char *mqtt_password;
int retries = 0;
WiFiClient sclient;
PubSubClient mqtt_client(sclient);

//Αρχικοποίηση βιβλιοθήκης που αναλαμβάνει την "κατάσταση
//ρύθμισης" του Esp8266
EspNetworkManager netmanager(ModuleID);
String Username;
String Password;

//Αρχικοποίηση μεταβλητών σύνδεσης Wi-Fi
String WiFiSSID;
String WiFiPassword;

//Αρχικοποίηση ακροδεκτών ελέγχου κινητήρα
#define IN1 15
#define IN2 10
#define IN3 4
#define IN4 5
//Αρχικοποίηση μεταβλητών ελέγχου κινητήρα
int Steps = 0;
boolean Direction = true;
unsigned long last_time;
unsigned long currentMillis;
int steps_left = 128;

// Αρχικοποίηση μεταβλητών οθόνης TFT ILI9341
TFT_eSPI tft = TFT_eSPI();
unsigned long drawTime = 0;
int xpos = 0;
int ypos = 40;
int count = 2;

//Δηλώσεις Μεθόδων
void ExplicitRunNetworkManager(
    void); //Μέθοδος που καλεί την βιβλιοθήκη EspNetworkManager
void onMessageReceived(
    char *, byte *,
    unsigned int); // Callback που καλείτε όταν ληφθεί μήνυμα MQTT
boolean ConnectToNetwork(void); //Μέθοδος σύνδεσης σε τοπικό δίκτυο Wi-Fi
void MqttReconnect(void); //Μέθοδος σύνδεσης - επανασύνδεσης σε μεσίτη MQTT
void SetDirection(void);  //Μέθοδος που ελέγχει την φορά περιστροφής του κινητήρα
void stepper(int);        //Μέθοδος που ενεργοποιεί τα κατάλληλα πηνία του κινητήρα
void doStep(void);        //Μέθοδος που καλείτε για να εκτελέσει 1 βήμα ο κινητήρας

void setup() {
  EEPROM.begin(512);
  //Οι ακροδέκτες ελέγχου του κινητήρα δηλώνονται ως έξοδοι
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  Serial.begin(115200); //Σειριακή επικοινωνία για debugging με baudrate 115200
  Serial.println("");
  delay(
      800); //Αναμονή για να σιγουρευτούμε ότι η EEPROM και η σειριακή ξεκίνησαν
  /**Ρύθμιση λειτουργίας της οθόνης ILI9341*/
  tft.begin();
  tft.setRotation(1);           //Οριζόντια προβολή
  tft.setTextColor(TFT_YELLOW); //Χρώμα γραμμάτων
  tft.setCursor(xpos, ypos); //Σημείο έναρξης στην οθόνη
  tft.setFreeFont(FSB9); //Γραμματοσειρά (Μέσω αρχείων βιβλιοθήκης)
  int xpos = 0;
  int ypos = 40;
  tft.fillScreen(TFT_BLACK); // Μαύρο Background
  tft.setCursor(xpos,
                ypos);  //Παράλειψη πρώτης σειράς για να χωρέσει η γραμματοσειρά
  tft.println("Hello"); //Εντολή προβολής κειμένου
                        /**Αξιολόγηση αν χρειάζεται η "Λειτουργία ρυθμίσεων". Αν μπορει να συνδεθεί σε
                         * τοπικό
                         * δίκτυο ,τότε  δεν χρειάζεται.*/
  if (!ConnectToNetwork()) {
    //Αν δεν μπορέσεις να συνδεθείς στο δίκτυο ξεκίνα την "Λειτουργία Ρυθμίσεων"
    //μέσω την βιβλιοθήκης EspNetworkManager
    ExplicitRunNetworkManager();
  }
  delay(1000);
  mqtt_client.setServer("educ.chem.auth.gr", 1883);
  mqtt_client.setCallback(onMessageReceived);
}

/*Κεντρική λούπα προγράμματος*/
void loop() {
  //Πάντα προσπαθεί  να συνδεθεί στο μεσίτη, επαναλαμβάνει αν δεν τα καταφέρει.
  if (!mqtt_client.connected()) {
    MqttReconnect();
  }
  mqtt_client.loop(); //Μέθοδος που καλείτε για να λειτουργήσει το callback των
                      //μηνυμάτων MQTT
}

void MqttReconnect() // Προσπάθησε να συνδεθείς μέχρι να τα καταφέρεις
{
  Serial.print("Connecting to Push Service...");
  // Συνδέσου στον server που ορίσαμε με username: androiduse, password:and3125
  //Οι μεταβλητές αυτές διαβάζονται από την βιβλιοθήκη EspNetworkManager η οποία
  //τις ανακαλέι απο την ROM
  if (!mqtt_client.connect(ModuleID.c_str(),
                           netmanager.ReadBrokerUsername().c_str(),
                           netmanager.ReadBrokerPassword().c_str())) {
    delay(1000);
    retries++;
    if (retries <= 4) //Μετά από 4 προσπάθειες σταμάτησε να προσπαθείς. Ο
                      //Μεσίτης δεν υπάρχει η χάθηκε η σύνδεση στο δίκτυο
    {
      Serial.print("Failed to connect, mqtt_error_code=");
      Serial.print(mqtt_client.state()); //Αποσφαλμάτωση αποτυχίας σύνδεσης
      Serial.println(" Retrying in 2 seconds");
      retries++;
    } else {
      retries = 0;
      ExplicitRunNetworkManager(); //Εκκίνηση της "Λειτουργίας ρυθμίσεων"
    }
    return;
  } else /*Επιτυχής Σύνδεση*/
  {
    delay(500);
    Serial.println("Connected Sucessfully");
    //Εγγραφή στα απαραίτητα θέματα
    mqtt_client.subscribe("/android/synchronize"); //Θέμα συγχρονισμού Android
    mqtt_client.subscribe(myTopic.c_str()); //Θέμα λήψης εντολών κινητήρα
    mqtt_client.subscribe(myTopic2.c_str()); //Θέμα λήψης εντολών οθόνης
    mqtt_client.subscribe(settingsTopic.c_str()); //θέμα ρυθμίσεων μΕ
    mqtt_client.subscribe(settingsTopic2.c_str()); //Θέμα ρυθμίσεων μΕ
  }
}

void ExplicitRunNetworkManager() {
  netmanager.begin(); //Αρχικοποίηση EspNetworkManager
  if (!netmanager.runManager()) {
    Serial.println("Manager Failed");
  } else //Η διεργασίες της βιβλιοθήκης EspNetworkManager εκτελέστηκα επιτυχώς ο
  //χρήστης καταχώρησε κωδικούς και ονόματα
  {
    Serial.println("Credentials Received");
    Serial.println(
        netmanager.ReadSSID()); //Δειξε το ονομα δικτυου που διάλεξε ο χρήστης
    Serial.println(netmanager.ReadPASSWORD()); //Δείξει τον κωδικο δικτυου
  }
}

void onMessageReceived(char *topic, byte *payload,
                       unsigned int length) /*Λήφθηκε Μήνυμα Mqtt*/
{
  //Δείξε το Mqtt θέμα μέσω σειριακής
  Serial.print("Inoming Message [Topic: ");
  Serial.print(topic);
  Serial.print("] Message: ");
  String androidID;
  for (int i = 0; i < length;
       i++) //Μετατροπή μηνύματος σε String και προβολή του
  {
    Serial.print((char)payload[i]);
    androidID += (char)payload[i];
  }
  Serial.println();

  if (strcmp(topic, "/android/synchronize") ==
      0) /*Μια συσκευή Android ζήτησε συγχρονισμό */
  {
    Serial.println("Android asking for mC ID");
    //Το μήνυμα περιέχει το ID της συσκευής android. Ετσι μπορούμε να βρούμε το
    //θέμα(newAndroidTopic)
    //οπού πρέπει να στείλουμε τις πληροφορίες αυτής της συσκευής.
    String newAndroidTopic = ("/android/" + androidID + "/newmodules");
    //Δημιούργησε τα string που περιέχουν όλες τις πληροφορίες αυτής της
    //συσκευής.
    //Κάθε μεταβλητή χωρίζεται με τον χαρακτήρα "&" ώστε να μπορει να διαβαστεί
    //από την συσκευή Android
    String myInfo = (ModuleID + "&" + mcu_type + "&" + controlType + "&" +
                     "Motor"); //Πληροφορίες Κινητήρα
    String myInfo2 = (ModuleID2 + "&" + mcu_type + "&" + controlType2 + "&" +
                      "ILI9341 Display"); //Πληροφορίες οθόνης
    //Αποστολή πληροφοριών στης συσκευή που τις ζήτησε
    mqtt_client.publish((newAndroidTopic.c_str()), (myInfo.c_str()));
    delay(4);
    mqtt_client.publish((newAndroidTopic.c_str()), (myInfo2.c_str()));
    delay(2);
  } else if (strcmp(topic, myTopic.c_str()) ==
             0) /*Μια συσκευή ζήτησε κίνηση του κινητήρα */
  {
    if (payload[0] == 0x30) // ASCI 0 για δεξιόστροφη κίνηση
    {
      Direction = 0;
      doStep();
    } else if (payload[0] == 0x31) // ASCI 1 για αριστερόστροφη κίνηση
    {
      Direction = 1;
      doStep();
    }
  } else if (strcmp(topic, myTopic2.c_str()) ==
             0) /*Μια συσκευή ζήτησε γραφή κειμένου στην οθόνη */
  {
    String message;
    for (int i = 0; i < length; i++) //Μετατροπή byte σε String
    {
      message += (char)payload[i];
    }
    if (count >= 3) // H οθόνη εμφανίζει μόνο 3 μηνύματα την φορά, για το 4ο
    // μήνυμα η οθόνη θα σβήσει τα προηγούμενα 3
    {
      tft.fillScreen(TFT_BLACK); //Σβήσιμο οθόνης
      tft.setCursor(xpos, ypos); //Γράψιμο στην πρώτη γραμμή
      count = 0;
    }
    tft.println(); // 1η σειρά= κενό από το προηγούμενο μήνυμα
    tft.print("Topic: ");
    tft.print(topic); // 2η σειρά= Θέμα μηνύματος
    tft.println();
    tft.print(message); // 3η σειρά= Μήνυμα κειμένου
    count++;
  }
}

boolean ConnectToNetwork() {
  WiFi.mode(WIFI_STA); //Ο μΕ γίνεται Wi-Fi Station
  delay(10);
  Serial.println("WIFI>>:Attempting Connection to ROM Saved Network");
  String netssid = netmanager.ReadSSID(); //Ανάγνωση SSID δικτύου από την ROM
  Serial.println("WIFI>>Network: " + netssid);
  String netpass =
      netmanager.ReadPASSWORD(); //Ανάγνωση Κωδικού δικτύου από την ROM
  WiFi.begin(netssid.c_str(), netpass.c_str()); // Σύνδεση
  int attemps = 0;
  while (WiFi.status() != WL_CONNECTED) //Έλεγχος αν είναι συνδεδεμένο
  {
    delay(500);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECT_FAILED ||
        attemps > 20) //Η σύνδεση θεωρείται αποτυχημένη μετα από 20 προσπάθειες
    {
      Serial.println("");
      Serial.println("WIFI>>:Connection Failed. Starting Network Manager.");
      delay(10);
      return false;
    }
    attemps++;
  } //Επιτύχής σύνδεση
  Serial.println("");
  Serial.println("WIFI>>:Connected");
  Serial.print("WIFI>>:IPAddress: ");
  Serial.println(WiFi.localIP());
  return true;
}

void doStep() //Πραγματικό βήμα
{
  while (steps_left > 0) {
    currentMillis = micros();
    if (currentMillis - last_time >= 1000) //Εναλλαγή ανά 1000 μικρόsecond
    {
      stepper(1); // steper(βήμα)
      last_time = micros();
      steps_left--;
    }
  }
  steps_left = 128; // 128 επαναλήψεις εναλλαγής(128x2αλλαγές πηνίων =256
  // αλλαγές πηνίων για να ολοκληρωθεί 1 dostep )
}

void stepper(int xw) //Το xw ορίζει πόσες αλλαγές πηνίων θα έχουμε
{
  for (int x = 0; x < xw; x++) //Για xw=1 θα γίνουν 2 αλλαγές πηνίων
  {
    switch (Steps) {
    case 0:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    case 1:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, HIGH);
      break;
    case 2:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case 3:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case 4:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 5:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 6:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 7:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    default:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    }
    SetDirection();
  }
}

void SetDirection() //η φορά με την οποία καλούνται οι 8 καταστάσεις των πηνίων
                    //ορίζει την φορά του κινητήρα
{
  if (Direction == 1) //(case 0 - case7) δεξιόστροφα
  {
    Steps++;
  }
  if (Direction == 0) //(case 7 - case 0)αριστερόστροφα
  {
    Steps--;
  }
  if (Steps > 7) {
    Steps = 0;
  }
  if (Steps < 0) {
    Steps = 7;
  }
}
#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup !
#endif
