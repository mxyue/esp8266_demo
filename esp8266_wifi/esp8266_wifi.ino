#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

const char* APssid = "nodemcu";
const char* APpwd = "12345678";

ESP8266WebServer server(80);

bool needRestart = false;
int wifiConnectTimeout = 15;

char ssidfile[] = "/ssid.txt";
char pwdfile[] = "/password.txt";
char htmlhomefile[] = "/html/index.html";
char htmlnotfoundfile[] = "/html/not-found.html";
char htmlsuccessfile[] = "/html/success.html";


String nullStr = "";
String getContent(char filepath[]){
  bool exist = SPIFFS.exists(filepath);
  if (!exist) {
    Serial.println("file not present");
    return nullStr;
  }
  File f = SPIFFS.open(filepath, "r");
  if (!f) {
    Serial.printf("%s file open failed \n", filepath);
    return nullStr;
  }
  String content = f.readString();
  f.close();
  Serial.printf("[%s]-->size(%d)\n", filepath, f.size());
  return content;
}

void setContent(char filepath[], String content){
  File f = SPIFFS.open(filepath, "w");
  f.print(content);
  f.close();
}

void connectWifi(){
  String ssid = getContent(ssidfile);
  String password = getContent(pwdfile);
  if(ssid.length() < 1 || password.length() < 1){
    return;
  }
  const char *cssid = ssid.c_str();
  const char *cpwd = password.c_str();
  Serial.printf("ssid: %s, pwd: %s \n", cssid, cpwd);
  WiFi.begin(cssid, cpwd);
  Serial.print("Connecting WIFI.");
  int connectTime  = 0;
  while (WiFi.status() != WL_CONNECTED && connectTime < wifiConnectTimeout) {
    if(WiFi.status() == WL_NO_SSID_AVAIL){
      Serial.println("\nwifi not found");
      WiFi.disconnect();
      return;
    }else if(WiFi.status() == WL_CONNECT_FAILED){
      Serial.println("\npassword err");
      WiFi.disconnect();
      return;
    }
    delay(1000);
    connectTime ++;
    Serial.print(".");
  }
  Serial.print("\nLocal IP address: ");
  Serial.println(WiFi.localIP());
}

void setSoftAP(){
  // Serial.print("Setting soft-AP configuration ... ");
  // Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(APssid, APpwd) ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void setWifiPWD(String ssid, String pwd){
  setContent(ssidfile, ssid);
  setContent(pwdfile, pwd);
}

// ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓--handler--↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
String homeContent;
void homeController(){
  server.send(200, "text/html", homeContent);
}

// handler
String successContent;
void successController(){
  server.send(200, "text/html", successContent);
}

// handler wifi
void wifisetController(){
  String ssid = server.arg("ssid");
  String pwd = server.arg("password");
  Serial.printf("----ssid: %s ;pwd: %s \n",ssid.c_str(), pwd.c_str() );
  setWifiPWD(ssid, pwd);
  ssid.clear();
  pwd.clear();
  server.sendHeader("Location", String("/success"), true);
  server.send( 302, "text/plain", "");
}

// restart handler
void restartController(){
  server.sendHeader("Location", String("/success"), true);
  server.send( 302, "text/plain", "");
  Serial.println("restart");
  needRestart = true;
}

// 404 handler
String notFoundContent;
void handleNotFound(){
  Serial.println("not found page------>");
  server.send(404, "text/html", notFoundContent);
}
// ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑--handler--↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑--

void setHtmlContent(){
  homeContent = getContent(htmlhomefile);
  successContent = getContent(htmlsuccessfile);
  notFoundContent = getContent(htmlnotfoundfile);
}

void router(){
  setHtmlContent();
  server.on("", HTTP_GET, homeController);
  server.on("/", HTTP_GET, homeController);
  server.on("/wifisets", HTTP_POST, wifisetController);
  server.on("/success", HTTP_GET, successController);
  server.on("/restart", HTTP_GET, restartController);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");  
}

//---------------main--------------------------
void setup() {
  Serial.begin(115200);
  bool fsok = SPIFFS.begin();
  if (fsok) {
    Serial.println("FS ok ");
  }
  //开启热点
  setSoftAP();
  //连接WiFi
  connectWifi();
  router();
}

void loop() {
  if(needRestart){
    needRestart = false;
    Serial.println("start restart");
    ESP.restart();
  }
  server.handleClient();
}
