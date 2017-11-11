#include "ESPNetworkManager.h"


EspNetworkManager::EspNetworkManager(String chipId){
moduleID=chipId;
ssid=("ESP"+chipId).c_str();
}
void EspNetworkManager::begin(){
  Serial.println("<<<<<<NETMAN STARTED>>>>>>");
  	Serial.println("NETMAN>>EPROM VARIABLES:");
  	Serial.println(" -"+ReadEeprom(1));
  	Serial.println(" -"+ReadEeprom(2));
  	Serial.println(" -"+ReadEeprom(3));
  	Serial.println(" -"+ReadEeprom(4));
    Serial.println(" -"+ReadEeprom(5));
    Serial.println(" -"+ReadEeprom(6));
    server =  ESP8266WebServer(80);
    local_IP= IPAddress(192,168,4,22);
    gateway=IPAddress (192, 168, 4, 9);
    subnet=IPAddress (255, 255, 255, 0);
  	//BrokerAddress = "0";
  	Module_Description = "No Description";
  //  BrokerPassword="noname";
    //BrokerUsername="nopass";

  	initializeAP();
  	Serial.println("NETMAN>>AccessPoint Initialization completed");
  	initializeWebServer();
}

boolean EspNetworkManager::runManager(){
  while(!managerFinished){
    server.handleClient();
  }
  return true;
}

void EspNetworkManager::initializeAP(){
	WiFi.mode(WIFI_AP);
	WiFi.disconnect();
  WiFi.mode(WIFI_AP);
	delay(100);
	WiFi.softAPConfig(local_IP, gateway, subnet);
	if (WiFi.softAP(ssid))
	{
    Serial.print("NETMAN>>AccessPoint started at: ");
		Serial.println(WiFi.softAPIP());
    Serial.println(ssid);
	}else
	{
		Serial.println("NETMAN>>AcessPoint failed to start");
	}
	delay(100);
}

void EspNetworkManager::initializeWebServer(){
	server.on("/", [this]() {
		String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
		page += FPSTR(HTML_MAIN_SCRIPT);
		page += FPSTR(HTML_MAIN_BODY_1);
		page += moduleID;
		page += FPSTR(HTML_MAIN_BODY_2);
		page += FPSTR(HTML_END_SIGN);
		Serial.println("Root accesed");
		IPAddress ip = WiFi.softAPIP();
		String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
		server.send(200, "text/html",page );
	});
	server.on("/options", [this]() {
		int argnumber = server.args();
		if (argnumber > 0) {
      Serial.println("NETMAN>>Options Saved");
			Serial.println("NETMAN>>Number of properties: " + String(argnumber));
      for (int i = 0; i < argnumber; i++) {
				Serial.println("NETMAN>> - " + server.argName(i) + " = " + server.arg(i));
			}
			Module_Description = server.arg(0);
			BrokerAddress = server.arg(1);
      BrokerUsername = server.arg(2);
      BrokerPassword = server.arg(3);
			server.send(200, "text/html", "Options Saved");
			ClearEeprom(2);
			delay(10);
			WriteEeprom(3);
			WriteEeprom(4);
      WriteEeprom(5);
      WriteEeprom(6);
		}else{
			String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
			page += FPSTR(HTML_OPTIONS_SCRIPT_N_BODY);
			page += FPSTR(HTML_END_SIGN);
			Serial.println("NETMAN>>Options accesed");
			server.send(200, "text/html", page);
		}

	});
	server.on("/wifi", [this]() {
		int argnumber = server.args();
		if (argnumber > 0) {
      Serial.println("NETMAN>>Wifi Settings Saved. " + String(argnumber));
      Serial.println("NETMAN>>Number of properties: " + String(argnumber));
			for (int i = 0; i < argnumber; i++) {
				Serial.println(" - " + server.argName(i) + " = " + server.arg(i));
			}
			WIFI_SSID = server.arg(0);
			WIFI_PASS = server.arg(1);

			server.send(200, "text/html", "WiFi Saved");
			ClearEeprom(1);
			delay(10);
			WriteEeprom(1);
			WriteEeprom(2);
			delay(10);
			managerFinished= true;
		}else
		{
			String page = FPSTR(HTML_GENERAL_HEADER_STYLE);
			page += FPSTR(HTML_SSID_SCRIPT);
			page += FPSTR(HTML_SSID_BODY_1);
      int n = WiFi.scanNetworks();
			if (n == 0)
			{
				page += "<li><a href=\"#\" id=\"0\">No Network Found</a></li>";
			}
			else
			{
				String mssid;
				String EncryptType;
				String signal;
				for (int i = 0; i<n; i++)
				{
					mssid = WiFi.SSID(i);
					EncryptType = ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*Secure");
					signal = String(WiFi.RSSI(i));
					page +="<li><a id=\""+mssid+"\" class=\"listItem\">"+mssid + " (" + signal + "dB) " + EncryptType + "</a></li>";
				}
				page += "</ul>";
			}
			page += FPSTR(HTML_END_SIGN);
			Serial.println("Wifi accesed");

			server.send(200, "text/html", page);
		}
	});
	server.onNotFound( [this]() {
		Serial.println("Page not Found");
		server.send(404, "text/plain", "FileNotFound");
	});
	server.begin();
	Serial.println("NETMAN>>WebServer Started.");
}

//1= read ssid
String EspNetworkManager::ReadSSID(){
   return  ReadEeprom(1);
}
String EspNetworkManager::ReadPASSWORD(){
   return  ReadEeprom(2);
}
String EspNetworkManager::ReadDescription(){
   return  ReadEeprom(3);
}
String EspNetworkManager::ReadBrokerAddress(){
   return  ReadEeprom(4);
}
String EspNetworkManager::ReadBrokerUsername(){
   return  ReadEeprom(5);
}
String EspNetworkManager::ReadBrokerPassword(){
   return  ReadEeprom(6);
}
//2= read pass
//3=read description
//4= read security level
String EspNetworkManager::ReadEeprom (int c){
  int addr_pass = 32; //pass adress = 0x20 (16bytes)
	int addr_desc = 48;// desc address= 0x30 (64btyes)
  int addr_brok = 112;//security level addr = 0x70 (30byte)
  int addr_bruser=142;//security level addr = 0x70 (25byte)
  int addr_brpass=167;//security level addr = 0x70 (40byte)
	String returnString;
	if (c == 1)
	{
		String esid = "";

		for (int i = 0; i < addr_pass - 1; ++i)
		{
			esid += char(EEPROM.read(i));
		}
		esid.trim();
		returnString= esid;
	}
	else if (c == 2)
	{
		String pass = "";
		for (int i = addr_pass; i < addr_desc - 1; ++i)
		{
			pass += char(EEPROM.read(i));
		}
		//pass.trim();
		returnString = pass;
	}
	else if (c == 3)
	{
		String desc = "";
		for (int i = addr_desc; i < addr_brok - 1; ++i)
		{
			desc += char(EEPROM.read(i));
		}
		desc.trim();
		returnString = desc;
	}
	else if (c == 4)
	{
		String brok = "";
		for (int i = addr_brok; i < addr_bruser-1; ++i)
		{
			brok += char(EEPROM.read(i));
		}
		//brok.trim();
		returnString = brok;
	}else if (c == 5)
	{
		String brUser = "";
		for (int i = addr_bruser; i < addr_brpass-1; ++i)
		{
			brUser += char(EEPROM.read(i));
		}
		brUser.trim();
		returnString = brUser;
	}else if (c == 6)
	{
		String brPass = "";
		for (int i = addr_brpass; i < addr_brpass + 20; ++i)
		{
        brPass += char(EEPROM.read(i));
		}
		brPass.trim();
		returnString = brPass;
	}
	EEPROM.commit();
	return returnString;
}

void EspNetworkManager::ClearEeprom(int c){
  if (c == 1)
  	{
  		for (int i = 0; i < 48; ++i) { EEPROM.write(i, 0); }
  	}
  	if (c == 2)
  	{
  		for (int i = 48; i < 168; ++i) { EEPROM.write(i, 0); }
  	}
  	EEPROM.commit();
}

void EspNetworkManager::WriteEeprom (int c){
  int addr_pass = 32; //pass adress = 0x20 (16bytes)
  int addr_desc = 48;// desc address= 0x30 (64btyes)
  int addr_brok = 112;//security level addr = 0x70 (30byte)
  int addr_bruser=142;//security level addr = 0x70 (25byte)
  int addr_brpass=167;//security level addr = 0x70 (40byte)
	if (c==1)
	{
		for (int i = 0; i <WIFI_SSID.length(); ++i)
		{

			EEPROM.write(i, WIFI_SSID[i]);

		}
    Serial.println("NETMAN>>Wrote EEPROMM: SSID:"+ WIFI_SSID);
	}
	if (c==2)
	{
    for (int i = 0; i <WIFI_PASS.length(); ++i)
		{
			EEPROM.write(addr_pass + i, WIFI_PASS[i]);
		}
    Serial.println("NETMAN>>Wrote EEPROMM: PASS:"+WIFI_PASS);
	}
	if (c==3)
	{
		for (int i = 0; i <Module_Description.length(); ++i)
		{
			EEPROM.write(addr_desc + i, Module_Description[i]);

		}
    Serial.print("NETMAN>>Wrote EEPROMM: Description:"+Module_Description);
	}
	if (c==4)
	{
		for (int i = 0; i <BrokerAddress.length(); ++i)
		{
			EEPROM.write(addr_brok + i, BrokerAddress[i]);
		}
    Serial.print("NETMAN>>Wrote EEPROMM: Broker:");
	}
  if (c==5)
	{
		for (int i = 0; i <BrokerUsername.length(); ++i)
		{
			EEPROM.write(addr_bruser + i, BrokerUsername[i]);
		}
    Serial.print("NETMAN>>Wrote EEPROMM: BrokerUsername:");
	}
  if (c==6)
	{
		for (int i = 0; i <BrokerPassword.length(); ++i)
		{
			EEPROM.write(addr_brpass + i, BrokerPassword[i]);
		}
    Serial.print("NETMAN>>Wrote EEPROMM: BrokerPass:");
	}
	EEPROM.commit();
}
