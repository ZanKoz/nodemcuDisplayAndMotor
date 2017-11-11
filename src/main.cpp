#include <Arduino.h>
#include "ESPNetworkManager.h"
#include <PubSubClient.h>
#include <DNSServer.h>
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include "SPI.h"
#include "TFT_eSPI.h"
//Module Vars
String ModuleID = String(ESP.getChipId());
String ModuleID2 = (ModuleID + "2");
String mcu_type = "input";
String myTopic = ("/" + mcu_type + "/" + ModuleID+ "/" +"data");
String myTopic2 = ("/" + mcu_type + "/" + ModuleID2+ "/" +"data");
String settingsTopic= ("/" + mcu_type + "/" + ModuleID+ "/" +"settings");
String settingsTopic2= ("/" + mcu_type + "/" + ModuleID2+ "/" +"settings");
String willTopic=("/" + mcu_type + "/" + ModuleID+ "/" +"lastwill");
String willTopic2=("/" + mcu_type + "/" + ModuleID2+ "/" +"lastwill");
String controlType="motorcontrol";
String controlType2="lcdtext";
String myStatus = "00";

//Mqtt Client Setup
const char* micro_mqtt_broker;
const char* mqtt_username ;
const char* mqtt_password ;
int retries=0;
WiFiClient sclient;
PubSubClient mqtt_client(sclient);
//NetworkManager
EspNetworkManager netmanager(ModuleID);
//Wifi Connection
String WiFiSSID;
String WiFiPassword;
//motor
#define IN1  15
#define IN2  10
#define IN3  4
#define IN4  5
int Steps = 0;
boolean Direction = true;// gre
unsigned long last_time;
unsigned long currentMillis ;
int steps_left=128;
// TFT
TFT_eSPI tft = TFT_eSPI();
unsigned long drawTime = 0;
int xpos =  0;
int ypos = 40;
int count=2;
//Functions
void  ExplicitRunNetworkManager(void);
void onMessageReceived(char*, byte*, unsigned int);
boolean ConnectToNetwork(void);
void MqttReconnect(void);
void SetDirection(void);
void stepper(int);
void doStep(void);
  String Username;
    String Password;



void setup() {
  EEPROM.begin(512);
pinMode(IN1, OUTPUT); // Initialize the RELAY_1 pin as an output
pinMode(IN2, OUTPUT);
pinMode(IN3, OUTPUT);
pinMode(IN4, OUTPUT);// Initialize the RELAY_2 pin as an output
  Serial.begin(115200);
  delay(800);
//  Serial.println("");
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(xpos, ypos);
  tft.setFreeFont(FSB9);
  int xpos =  0;
  int ypos = 40;
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(xpos, ypos);
  tft.println("hello");
 Serial.println("");
  if (!ConnectToNetwork()) {
    ExplicitRunNetworkManager();
  }
  delay (1000);
  mqtt_client.setServer("educ.chem.auth.gr", 1883);
  mqtt_client.setCallback(onMessageReceived);
}

void loop() {
  if (!mqtt_client.connected()) {
    MqttReconnect();
  }
  mqtt_client.loop();
}

void MqttReconnect() {
	// Προσπάθησε να συνδεθείς μέχρι να τα καταφερεις
  Serial.print("Connecting to Push Service...");

		// Συνδέσου στον server που ορίσαμε με username:androiduse,password:and3125
		if (!mqtt_client.connect(ModuleID.c_str(),netmanager.ReadBrokerUsername().c_str(),netmanager.ReadBrokerPassword().c_str())) {
      delay(1000);
      retries++;
      if (retries<=4) {
  			Serial.print("Failed to connect, mqtt_rc=");
  			Serial.print(mqtt_client.state());
  			Serial.println(" Retrying in 2 seconds");
        retries++;
      }else{
        retries=0;
        ExplicitRunNetworkManager();
      }
      return;
    }else{
      delay(500);
      Serial.println("Connected Sucessfully");
      //TODO PubSub Here
      mqtt_client.subscribe("/android/synchronize");
      mqtt_client.subscribe(myTopic.c_str());
      mqtt_client.subscribe(myTopic2.c_str());
      mqtt_client.subscribe(settingsTopic.c_str());
      mqtt_client.subscribe(settingsTopic2.c_str());
    }
}

void  ExplicitRunNetworkManager(){
  netmanager.begin();
  if (!netmanager.runManager()) {
    Serial.println("Manager Failed");
  }else{
    Serial.println("Credentials Received");
    Serial.println(netmanager.ReadSSID());
    Serial.println(netmanager.ReadPASSWORD());
  }
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
	//TODO: Add critical notification kai web setup signal

  Serial.print("Inoming Message [Topic: ");
  Serial.print(topic);
  Serial.print("] Message: ");
  String androidID;
  //Ξ£Ο„ΞΏ ΞµΞΉΟƒΞ±ΞΉΟ�Ο‡Ο�ΞΌΞµΞ½ΞΏ ΞΌΟ�Ξ½Ξ·ΞΌΞ±  Ξ­Ο‡ΞµΞΉ ΟƒΟ„ΞµΞ―Ξ»ΞµΞΉ Ο„ΞΏ Ξ΄ΞΉΞΊΟ� Ο„ΞΏΟ… ID, ΟƒΟ„ΞΏ ΞΏΟ€ΞΏΞ―ΞΏ Ξ±ΞΊΞΏΟ…ΞµΞΉ Ξ³ΞΉΞ± Ξ½ΞµΞΏΟ…Ο‚ ΞΌC: /{AndroidID}/new
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    androidID += (char)payload[i];
  }
  Serial.println();
  // Ξ‘Ξ½ Ξ­Ο�ΞΈΞµΞΉ ΞΌΟ�Ξ½Ξ·ΞΌΞ± ΟƒΟ„ΞΏ ΞΈΞ­ΞΌΞ± /syn Ο„ΞΏΟ„Ξµ ΞΊΞ¬Ο€ΞΏΞΉΞΏΟ‚ ΞΈΞ­Ξ»ΞµΞΉ Ξ½Ξ± ΞΌΞ¬ΞΈΞµΞΉ Ο„ΞΏ Ξ΄ΞΉΞΊΞΏ ΞΌΞ±Ο‚ ID
  if (strcmp(topic, "/android/synchronize") == 0)
  {
    Serial.println("Android asking for mC ID");
    //Ξ£Ο„ΞµΞ―Ξ»Ξµ ΟƒΟ„ΞΏ ΞΈΞ­ΞΌΞ± ΞΏΟ€ΞΏΟ… Ξ±ΞΊΞΏΟ�ΞµΞΉ Ο„ΞΉΟ‚ Ξ½Ξ­ΞΏΟ…Ο‚ ΞΌC Ο„ΞΉΟ‚ Ο€Ξ»Ξ·Ο�ΞΏΟ†ΞΏΟ�Ξ―ΞµΟ‚ ΞΌΞ±Ο‚
    String newAndroidTopic = ("/android/" + androidID + "/newmodules");
    String myInfo = (ModuleID + "&" + mcu_type+"&"+controlType+ "&" + "Motor");
    String myInfo2=(ModuleID2+"&" + mcu_type+"&"+controlType2 + "&" + "ILI9341 Display");
    mqtt_client.publish((newAndroidTopic.c_str()), (myInfo.c_str()));
    delay(2);
    delay(2);
    mqtt_client.publish((newAndroidTopic.c_str()), (myInfo2.c_str()));
    delay(2);
    //Ξ¤Ο‰Ο�Ξ± Ο€ΞΏΟ… ΞΎΞ­Ο�ΞµΞΉ Ο„ΞΏ ID ΞΌΞ±Ο‚, ΞΌΟ€ΞΏΟ�ΞµΞ― Ξ½Ξ± Ξ±ΞΊΞΏΟ�ΞµΞΉ Ο„Ξ·Ξ½ Ξ­ΞΎΞΏΞ΄Ο� Ξ”ΞµΞ΄ΞΏΞΌΞ­Ξ½Ο‰Ξ½ ΞΌΞ±Ο‚, ΟƒΟ„ΞΏ ΞΈΞ­ΞΌΞ±: /out/{ModuleID}
  }
  else if(strcmp(topic,myTopic.c_str())==0)
 {
   if(payload[0]==0x30)// asci0
   {
     //Εντολη Απενεργοποίησης
     Direction=0;
     doStep();
   }
   else if(payload[0]==0x31)//asci 1
   {
     Direction=1;
     doStep();
     //Εντολη Ενεργοποιησης
   }
 }else if(strcmp(topic,myTopic2.c_str())==0){
   String message;
   for (int i = 0; i < length; i++) {
     message+= (char)payload[i];
   }
   if (count>=3) {
       tft.fillScreen(TFT_BLACK);
       tft.setCursor(xpos, ypos);
       count =0;
   }
   tft.println();
   tft.print("Topic: ");
   tft.print(topic);
   tft.println();
   tft.print(message);
   count++;
 }
}

boolean ConnectToNetwork(){
  WiFi.mode(WIFI_STA);
	delay(10);
	Serial.println("WIFI>>:Attempting Connection to ROM Saved Network");
	String netssid = netmanager.ReadSSID();
	Serial.println("WIFI>>Network: "+netssid);
	String netpass = netmanager.ReadPASSWORD();
	WiFi.begin(netssid.c_str(),netpass.c_str());
	int attemps = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		if (WiFi.status() == WL_CONNECT_FAILED || attemps>21)
		{
			Serial.println("");
			Serial.println("WIFI>>:Connection Failed. Starting Network Manager.");
      //TODO Remove this delay at release
      delay(10);
			return false;
		}
		attemps++;
	}
	Serial.println("");
	Serial.println("WIFI>>:Connected");
	Serial.print("WIFI>>:IPAddress: ");
	Serial.println(WiFi.localIP());
  return true;
}


void doStep(){
  while(steps_left>0){
  currentMillis = micros();
    if(currentMillis-last_time>=1000){
     stepper(1);
     //time=time+micros()-last_time;
     last_time=micros();
     steps_left--;
  }
 }
 steps_left=128;
}

void stepper(int xw){
  for (int x=0;x<xw;x++)
  {
    switch(Steps){
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

void SetDirection(){
if(Direction==1){ Steps++;}
if(Direction==0){ Steps--; }
if(Steps>7){Steps=0;}
if(Steps<0){Steps=7; }
}


#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_FONT2
//ERROR_Please_enable_LOAD_FONT2_in_User_Setup!
#endif

#ifndef LOAD_FONT4
//ERROR_Please_enable_LOAD_FONT4_in_User_Setup!
#endif

#ifndef LOAD_FONT6
//ERROR_Please_enable_LOAD_FONT6_in_User_Setup!
#endif

#ifndef LOAD_FONT7
//ERROR_Please_enable_LOAD_FONT7_in_User_Setup!
#endif

#ifndef LOAD_FONT8
//ERROR_Please_enable_LOAD_FONT8_in_User_Setup!
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
