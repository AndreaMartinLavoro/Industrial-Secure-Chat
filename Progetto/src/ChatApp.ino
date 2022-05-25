//To upload the Firmware
//Run activities ctrl+alt+t
//PlatformIO Upload
//PlatformIO Upload Filesystem Image
#include <ESP8266WiFi.h>
#include "./DNSServer.h"
#include <ESP8266WebServer.h>
#include <FS.h>

//NETWORK//
const String messagesFile = "/mem.txt";
const String chatFile = "/app.html";
const char *wifiName = "ESPChat";
const char *pwd = "Chat2022@!";
const byte DNS_PORT = 53;
IPAddress apIP(10, 10, 10, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

//BATTERY//
int battery;
float batteryVolt;
String batteryResult;

//FILE//
String chatHtml;

void setup()
{
    //-->PIN<--//
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  ////-->UNDER VOLTAGE PROTECTION<--//
  //if (GetVolt() < 3.5){
  //  UsrAlertLed(8);
  //  ESP.deepSleep(0);  
  //}

  //-->FILES<--//
  SPIFFS.begin();
  chatHtml = fileRead(chatFile);
  
  UsrAlertLed(4);

  //-->NETWORK<--//
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiName, pwd, 1, true, 7);
  dnsServer.start(DNS_PORT, "*", apIP);
  webServer.begin();
  
  //-->HANDLERS<--//
  setupAppHandlers();
}

void loop()
{
  dnsServer.processNextRequest();
  webServer.handleClient();
}

///////////////////////
//-->PAGES HANDLE<--//
///////////////////////
void setupAppHandlers()
{
  webServer.onNotFound([]()
                       { ShowChatPage(); });
  webServer.on("10.10.10.1", ShowChatPage);
  webServer.on("/sendMessage", HandleSendMessage);
  webServer.on("/readMessages", ShowMessages);
  webServer.on("/clearMessages", HandleClearMessages);
  webServer.on("/status", ShowStatus); // Display the battery level
  webServer.on("/powerOff", PowerOff); // Warning this function let the device go to DeepSleep
}

///////////////////////
//-->PAGES METHODS<--//
///////////////////////
void ShowChatPage()
{
  webServer.send(200, "text/html", chatHtml);
}
void HandleSendMessage()
{
  if (webServer.hasArg("message"))
  {
    String message = webServer.arg("message");
    fileWrite(messagesFile, message + "\n", "a+");
    webServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/plain", "Message Sent");
  }
}
void ShowMessages()
{
  String messages = fileRead(messagesFile);
  webServer.sendHeader("Access-Control-Allow-Methods", "GET");
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "text/plain", messages);
}

void HandleClearMessages()
{
  SPIFFS.remove(messagesFile);
  webServer.sendHeader("Access-Control-Allow-Methods", "GET");
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "text/plain", "File Deleted");
}

void ShowStatus()
{
  webServer.sendHeader("Access-Control-Allow-Methods", "GET");
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  batteryResult = (battery == 1024 ? ">3.3" : (battery < 100 ? "Usb" : String(GetVolt()))) + " Volt";
  webServer.send(200, "text/plain", batteryResult);
  UsrAlertLed(5);
}
void PowerOff()
{
  webServer.send(200, "text/plain", "Goodbye");
  UsrAlertLed(5);
  ESP.deepSleep(0);
}

//////////////////////////
//-->HARDWARE METHODS<--//
//////////////////////////
void UsrAlertLed(int cicle)
{
  for (int i = 0; i < cicle; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}
float GetVolt(){
  battery = analogRead(A0);
  return battery * (4.08 / 967); // 967 => 4.08
}

///////////////////////
//-->FILES METHODS<--//
///////////////////////
String fileRead(String name)
{
  String contents;
  int i;
  File file = SPIFFS.open(name, "a+");
  for (i = 0; i < file.size(); i++)
  {
    contents += (char)file.read();
  }
  file.close();
  return contents;
}

void fileWrite(String name, String content, String mode)
{
  File file = SPIFFS.open(name.c_str(), mode.c_str());
  file.write((uint8_t *)content.c_str(), content.length());
  file.close();
}