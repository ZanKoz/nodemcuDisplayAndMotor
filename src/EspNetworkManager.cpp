#include "ESPNetworkManager.h"

EspNetworkManager::EspNetworkManager(
    String chipId) { /*Αρχικοποίηση Κλάσης EspNetworkManager */
  moduleID = chipId; // Depricated
  ssid = ("ESP" + chipId).c_str(); //Κατασκευή Ονόματος Wi-Fi δικτύου από τον
                                   //Σειριακό Κωδικό του ESP8266
}
void EspNetworkManager::begin() { /*Αρχικοποίηση Μεταβλητών AcessPoint και
                                     Webserver */

  Serial.println("<<<<<<NETMAN STARTED>>>>>>"); //Προβολή των ήδη αποθηκευμένων
                                                //μεταβλητών(Για αποσφαλμάτωση)
  Serial.println("NETMAN>>EPROM VARIABLES:");
  Serial.println(" -" + ReadEeprom(1));
  Serial.println(" -" + ReadEeprom(2));
  Serial.println(" -" + ReadEeprom(3));
  Serial.println(" -" + ReadEeprom(4));
  Serial.println(" -" + ReadEeprom(5));
  Serial.println(" -" + ReadEeprom(6));
  ESP8266WebServer server(80); //Αρχικοποίηση WebServer που ακούει στην θύρα 80
  local_IP = IPAddress(192, 168, 4,
                       22); // IP προσπέλασης WebServer (Αρχική σελίδα HTML)
  gateway = IPAddress(192, 168, 4, 9); //Προεπιλεγμένη πύλη τοπικού δικτύου
  subnet = IPAddress(255, 255, 255, 0); //Διεύθυνση υποδικτύου
  Module_Description = "No Description"; //Αρχικοποίηση περιγραφής συσκευής
  initializeAP(); //Έναρξη AcessPoint
  Serial.println("NETMAN>>AccessPoint Initialization completed");
  initializeWebServer();
}

boolean EspNetworkManager::runManager() {
  while (!managerFinished) { //Αν δεν δοθεί εντολή τέλους μέσω της μεταβλητής
                             //manager finished
    server.handleClient();   //Εξυπηρέτησε τους πελάτες που συνδέονται στον
                             //WebServer
  }
  return true;
}

void EspNetworkManager::initializeAP() { /*Έναρξη λειτουργίας AcessPoint*/
                                         //Οι 3 παρακάτω εντολές απαιτούνται για την σωστή ρύθμισης της συσκευής ως
                                         //AccessPoint
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAPConfig(
      local_IP, gateway,
      subnet); //καταχώρηση παραμέτρων για την δημιουργία AccessPoint
  if (WiFi.softAP(ssid)) //Εντολή έναρξης AccessPoint. True=επιτυχής έναρξη.
                         //False=έναρξη απέτυχε
  {
    Serial.print("NETMAN>>AccessPoint started at: ");
    Serial.println(WiFi.softAPIP()); //Προβολή διεύθυνσης πύλης
    Serial.println(ssid); //Προβολή ονόματος δικτύου
  } else {
    Serial.println("NETMAN>>AcessPoint failed to start");
  }
  delay(100);
}

void EspNetworkManager::initializeWebServer() { /* Η μέθοδος αναλαμβάνει τα
                                                   γεγονότα δρομολόγησης του
                                                   WebServer */
  server.on("/", [this]() { //Δρομολόγηση στην Αρχική Σελίδα
    //Η μεταβλητή "page" κατασκευάζει την HTML σελίδα που απαιτείται
    String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
    page += FPSTR(HTML_MAIN_SCRIPT);
    page += FPSTR(HTML_MAIN_BODY_1);
    page += moduleID; // πρόσθεσε στον τίτλο τον κωδικό του μΕ
    page += FPSTR(HTML_MAIN_BODY_2);
    page += FPSTR(HTML_END_SIGN);
    Serial.println("Root accesed");
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) +
                   '.' + String(ip[3]);
    server.send(200, "text/html",
                page); //Αποστολή της σελίδας HTML στον browser του χρήστη
  });
  server.on("/options", [this]() { //Δρομολόγηση στην σελίδα ρυθμίσεων MQTT
    int argnumber = server.args(); //Αριθμός των url μεταβλητών που
                                   //χρησιμοποιήθηκαν στην κλήση της σελίδας
    if (argnumber > 0) { //Αν υπάρχουν μεταβλητές τότε αυτές προήλθαν από την
                         //φόρμα Javascript
      //Άρα πατήθηκε αποθήκευση στην σελίδα
      Serial.println("NETMAN>>Options Saved");
      Serial.println("NETMAN>>Number of properties: " + String(argnumber));
      for (int i = 0; i < argnumber; i++) { //Εμφάνηση των μεταβλητών
        Serial.println("NETMAN>> - " + server.argName(i) + " = " +
                       server.arg(i));
      }
      Module_Description =
          server.arg(0); //Μεταβλητή Περιγραφής από τις μεταβλητές URL
      BrokerAddress =
          server.arg(1); //Μεταβλητή Διεύθυνση μεσίτη από τις μεταβλητές URL
      BrokerUsername =
          server.arg(2); //Μεταβλητή όνομα χρήστη από τις μεταβλητές URL
      BrokerPassword = server.arg(3); //Μεταβλητή κωδικός από τις μεταβλητές URL
      server.send(200, "text/html", "Options Saved");
      ClearEeprom(2); //Εκκαθάριση των προηγουμένων μεταβλητών απο την ROM
      delay(10);
      //Αποθήκευση των μεταβλητών στην ROM
      WriteEeprom(3);
      WriteEeprom(4);
      WriteEeprom(5);
      WriteEeprom(6);
    } else { //Δεν υπάρχουν μεταβλητές άρα ο χρήστης επισκέπτετε για πρώτη φορά
             //την σελίδα
      String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
      page += FPSTR(HTML_OPTIONS_SCRIPT_N_BODY);
      page += FPSTR(HTML_END_SIGN);
      Serial.println("NETMAN>>Options accesed");
      server.send(200, "text/html", page); //Αποστολή της σελίδας
    }

  });
  server.on("/wifi", [this]() { //Δρομολόγηση στην σελίδα επιλογή Wi-Fi
    int argnumber = server.args();
    if (argnumber > 0) { //Αν υπάρχουν μεταβλητές τότε αυτές προήλθαν από την
                         //φόρμα JavaScript
      Serial.println("NETMAN>>Wifi Settings Saved. " + String(argnumber));
      Serial.println("NETMAN>>Number of properties: " + String(argnumber));
      for (int i = 0; i < argnumber; i++) {
        Serial.println(" - " + server.argName(i) + " = " + server.arg(i));
      }
      WIFI_SSID = server.arg(0); //Μεταβλητή SSID από τις μεταβλητές URL
      WIFI_PASS = server.arg(1); //Μεταβλητή Password από τις μεταβλητές URL

      server.send(200, "text/html", "WiFi Saved");
      ClearEeprom(1);
      delay(10);
      //Αποθήκευση μεταβλητών στην ROM
      WriteEeprom(1);
      WriteEeprom(2);
      delay(10);
      managerFinished =
          true; /*Τερματισμός υπηρεσίας ρυθμίσεων. Έξοδος στο κυρίως πρόγραμμα*/
    } else      //Δεν υπάρχουν μεταβλητές άρα ο χρήστης επισκέπτετε για πρώτη φορά
                //την σελίδα
    {
      String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
      page += FPSTR(HTML_SSID_SCRIPT);
      page += FPSTR(HTML_SSID_BODY_1);
      int n = WiFi.scanNetworks(); //Αναζήτηση δικτύων κοντά στον esp8266
      if (n == 0) { //Αν δεν υπάρχουν δίκτυα εμφάνισε κατάλληλο μήνυμα στην HTML
                    //σελιδα
        page += "<li><a href=\"#\" id=\"0\">No Network Found</a></li>";
      } else {
        String mssid;
        String EncryptType;
        String signal;
        for (int i = 0; i < n; i++) { //Για κάθε στοιχείο στην λίστα δικτύων
          mssid = WiFi.SSID(i); //καταχώρηση του ονόματος δικτύου
          EncryptType = ((WiFi.encryptionType(i) == ENC_TYPE_NONE)
                             ? " "
                             : "*Secure"); //καταχώρηση της μορφής κλειδώματος
          signal = String(WiFi.RSSI(i)); //καταχώρηση του σήματος δικτύου
                                         //Προσθήκη των πληροφοριών σε μια καινούργια γραμμή HTML σαν ενα
          //καινούργιο στοιχείο λίστας
          page += "<li><a id=\"" + mssid + "\" class=\"listItem\">" + mssid +
                  " (" + signal + "dB) " + EncryptType + "</a></li>";
        }
        page += "</ul>";
      }
      page += FPSTR(HTML_END_SIGN);
      Serial.println("Wifi accesed");
      server.send(200, "text/html", page);
    }
  });
  server.onNotFound([this]() { //Δρομολόγηση σε άγνωστη σελίδα
    Serial.println("Page not Found");
    server.send(404, "text/plain",
                "FileNotFound"); //Αποστολή κωδικού 404 not found
  });
  server.begin(); //Εκκίνηση του Web Server με τα παραπάνω γεγονότα
  Serial.println("NETMAN>>Webserver Started.");
}

//Βοηθητικές μέθοδοι για την λήψη δεδομένων από την ROM
String EspNetworkManager::ReadSSID() { return ReadEeprom(1); }
String EspNetworkManager::ReadPASSWORD() { return ReadEeprom(2); }
String EspNetworkManager::ReadDescription() { return ReadEeprom(3); }
String EspNetworkManager::ReadBrokerAddress() { return ReadEeprom(4); }
String EspNetworkManager::ReadBrokerUsername() { return ReadEeprom(5); }
String EspNetworkManager::ReadBrokerPassword() { return ReadEeprom(6); }

String EspNetworkManager::ReadEeprom(int c) { /*Διάβασμα περιοχής της EEPROM*/
  int addr_pass =
      32; //Διεύθυνση ‘Κωδικού δικτύου’ = 0x20 (Μέγιστος χώρος=16bytes)
  int addr_desc =
      48; // Διεύθυνση ‘περιγραφής συσκευής’= 0x30 (Μέγιστος χώρος=64btyes)
  int addr_brok =
      112; //Διεύθυνση ‘διεύθυνσης μεσίτη’ = 0x70 (Μέγιστος χώρος=30byte)
  int addr_bruser =
      142; //Διεύθυνση ‘ονόματος χρήστη μεσίτη’ = 0x70 (Μέγιστος χώρος=25byte)
  int addr_brpass =
      167; //Διεύθυνση ‘κωδικού μεσίτη’ = 0x70 (Μέγιστος χώρος=40byte)
  String returnString;
  if (c == 1) //Διεύθυνση Ονόματος Δικτύου = 0x00 (Μέγιστος χώρος=32bytes)
  {
    String esid = "";
    for (int i = 0; i < addr_pass - 1; ++i) {
      esid += char(EEPROM.read(i));
    }
    esid.trim();
    returnString = esid;
  } else if (c == 2) {
    String pass = "";
    for (int i = addr_pass; i < addr_desc - 1; ++i) {
      pass += char(EEPROM.read(i));
    }
    // pass.trim();
    returnString = pass;
  } else if (c == 3) {
    String desc = "";
    for (int i = addr_desc; i < addr_brok - 1; ++i) {
      desc += char(EEPROM.read(i));
    }
    desc.trim();
    returnString = desc;
  } else if (c == 4) {
    String brok = "";
    for (int i = addr_brok; i < addr_bruser - 1; ++i) {
      brok += char(EEPROM.read(i));
    }
    returnString = brok;
  } else if (c == 5) {
    String brUser = "";
    for (int i = addr_bruser; i < addr_brpass - 1; ++i) {
      brUser += char(EEPROM.read(i));
    }
    brUser.trim();
    returnString = brUser;
  } else if (c == 6) {
    String brPass = "";
    for (int i = addr_brpass; i < addr_brpass + 20; ++i) {
      brPass += char(EEPROM.read(i));
    }
    brPass.trim();
    returnString = brPass;
  }
  EEPROM.commit();
  return returnString;
}

void EspNetworkManager::ClearEeprom(int c) { //Σβήσιμο περιοχής στην ROM
  if (c == 1) //Σβήσιμο περιοχής στοιχείων Wifi
  {
    for (int i = 0; i < 48; ++i) {
      EEPROM.write(i, 0);
    }
  }
  if (c == 2) //Σβήσιμο περιοχής ΜQTT ρυθμίσεων
  {
    for (int i = 48; i < 168; ++i) {
      EEPROM.write(i, 0);
    }
  }
  EEPROM.commit();
}

void EspNetworkManager::WriteEeprom(int c) { //Εγγραφή μεταβλητών στην EEPROM
  int addr_pass =
      32; //Διεύθυνση Κωδικού δικτύου = 0x20 (Μέγιστος χώρος=16bytes)
  int addr_desc =
      48; // Διεύθυνση περιγραφής συσκευής= 0x30 (Μέγιστος χώρος=64btyes)
  int addr_brok =
      112; //Διεύθυνση διεύθυνσης μεσίτη = 0x70 (Μέγιστος χώρος=30byte)
  int addr_bruser =
      142; //Διεύθυνση ονόματος χρήστη μεσίτη = 0x70 (Μέγιστος χώρος=25byte)
  int addr_brpass =
      167; //Διεύθυνση κωδικού μεσίτη = 0x70 (Μέγιστος χώρος=40byte)
  if (c == 1) {
    for (int i = 0; i < WIFI_SSID.length(); ++i) {

      EEPROM.write(i, WIFI_SSID[i]);
    }
    Serial.println("NETMAN>>Wrote EEPROMM: SSID:" + WIFI_SSID);
  }
  if (c == 2) {
    for (int i = 0; i < WIFI_PASS.length(); ++i) {
      EEPROM.write(addr_pass + i, WIFI_PASS[i]);
    }
    Serial.println("NETMAN>>Wrote EEPROMM: PASS:" + WIFI_PASS);
  }
  if (c == 3) {
    for (int i = 0; i < Module_Description.length(); ++i) {
      EEPROM.write(addr_desc + i, Module_Description[i]);
    }
    Serial.print("NETMAN>>Wrote EEPROMM: Description:" + Module_Description);
  }
  if (c == 4) {
    for (int i = 0; i < BrokerAddress.length(); ++i) {
      EEPROM.write(addr_brok + i, BrokerAddress[i]);
    }
    Serial.print("NETMAN>>Wrote EEPROMM: Broker:");
  }
  if (c == 5) {
    for (int i = 0; i < BrokerUsername.length(); ++i) {
      EEPROM.write(addr_bruser + i, BrokerUsername[i]);
    }
    Serial.print("NETMAN>>Wrote EEPROMM: BrokerUsername:");
  }
  if (c == 6) {
    for (int i = 0; i < BrokerPassword.length(); ++i) {
      EEPROM.write(addr_brpass + i, BrokerPassword[i]);
    }
    Serial.print("NETMAN>>Wrote EEPROMM: BrokerPass:");
  }
  EEPROM.commit();
}
