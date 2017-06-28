#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EmonLib.h>
#include <NodeMcuFile.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

void sendData(double power_A, String hostname_A, String apiCode_A);
void handleRoot();
void handleNotFound();
void sendPage();

const char* host = "esp8266-power-webupdate";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

NodeMcuFile f;

#define OLED_RESET 14
Adafruit_SSD1306 display(OLED_RESET);

#define EMONCMS_PORT 80

ESP8266WebServer httpServer(EMONCMS_PORT);
ESP8266HTTPUpdateServer httpUpdater;

String readString;

String httpAddress;
String httpAddrFile = "httpAddr";

String apiCode;
String apiCodeFile = "apiCode";

String voltage;
String voltageFile = "voltage";

int ledPin = 16;

EnergyMonitor emon;

//counter for loop
unsigned long g_loopCounter = 0;

char g_sendBuffer[500] = "";
char g_rcvBuffer[500] = "";

char hostnameBuff[50] = "";
char apiCodeBuff[50] = "";

char wifiStatus[20] = "init";
          
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)

  emon.current(0, 30);  // Current: input pin, calibration.

  
  // init done

  //////////////////////////////////////////////////////////////
  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,20);
  
  display.println("Starting...");
  
  display.display();
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("AutoConnectAP")) 
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 

  //if you get here you have connected to the WiFi
  display.println("Connected");

  display.print(WiFi.localIP());

  display.println();

  httpServer.on ( "/", handleRoot );
  httpServer.onNotFound ( handleNotFound );

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.begin();

  MDNS.addService("http", "tcp", EMONCMS_PORT);
  
  display.println("Server started");

  display.display();

  //File system
  f.start();

  //Open HTTP Address
  f.readFile(httpAddrFile, httpAddress);
  
  //Open API code
  f.readFile(apiCodeFile, apiCode);

  //Open voltage
  f.readFile(voltageFile, voltage);
}

void loop() 
{
  httpServer.handleClient();

  double irms = 0.0;
  double power = 0.0;
  
  //increment counter
  g_loopCounter ++;

  if((g_loopCounter % 20000) == 0)
  {
    double irms = emon.calcIrms(1480);  // Calculate Irms only

    double power = irms * (double)voltage.toInt();
  
    display.clearDisplay();
    
    display.setTextColor(WHITE);

    display.setCursor(0,0);
    display.setTextSize(1);
    display.println(wifiStatus);
    
    display.setCursor(0,20);
  
    display.setTextSize(2);
    display.print("");
    display.print(power);
    display.println(" W");
  
    display.setTextSize(1);
    display.println();
    display.print("I = ");
    display.print(irms);
    display.println(" A");
   
    display.display();

    if((g_loopCounter % 2000000) == 0)
    {
      sendData(power, httpAddress, apiCode);

      g_loopCounter = 0;
    }
  }
}

void sendData(double power_A, String hostname_A, String apiCode_A)
{
  WiFiClient emoncmsClient;

  strcpy(hostnameBuff, hostname_A.c_str());
  strcpy(apiCodeBuff, apiCode_A.c_str());

  Serial.println(hostname_A);
  Serial.println(hostnameBuff);

  Serial.println(apiCode_A);
  Serial.println(apiCodeBuff);

  if(emoncmsClient.connect(hostnameBuff, 80))
  {
    Serial.println("Connection OK");

    sprintf(g_sendBuffer, "GET /emoncms/input/post.json?apikey=%s&node=1&csv=power:%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", apiCodeBuff, (int)power_A, hostnameBuff);

    Serial.println(g_sendBuffer);

    emoncmsClient.write((uint8*)g_sendBuffer, strlen(g_sendBuffer));

    int len = emoncmsClient.read((uint8*)g_rcvBuffer, sizeof(g_rcvBuffer));

    if(len > 0)
    {
      Serial.println("Received");
      Serial.println((char*)g_rcvBuffer);
    }

    emoncmsClient.stop();

    sprintf(wifiStatus, "Connection OK");
  }
  else
  {
    Serial.println("Connection Error");

    sprintf(wifiStatus, "Connection Fail");
  }
}

void handleRoot() 
{  
  if(httpServer.hasArg("HTTP_ADDR") || httpServer.hasArg("CODE") || httpServer.hasArg("VOLTAGE")) 
  {
    if(httpServer.hasArg("HTTP_ADDR"))
    {
      httpAddress = httpServer.arg("HTTP_ADDR");
      Serial.print("Http Addr:");
      Serial.println(httpAddress);
      f.saveFile(httpAddrFile, httpAddress);
    }
    if(httpServer.hasArg("CODE"))
    {
      apiCode = httpServer.arg("CODE");
      Serial.print("API code:");
      Serial.println(apiCode);
      f.saveFile(apiCodeFile, apiCode);
    }
    if(httpServer.hasArg("VOLTAGE"))
    {
      voltage = httpServer.arg("VOLTAGE");
      Serial.print("Voltage:");
      Serial.println(voltage);
      f.saveFile(voltageFile, voltage);
    }
    sendPage();
  }
  else 
  {
    sendPage();
  }
}

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += ( httpServer.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";

  for ( uint8_t i = 0; i < httpServer.args(); i++ ) 
  {
    message += " " + httpServer.argName ( i ) + ": " + httpServer.arg ( i ) + "\n";
  }

  httpServer.send ( 404, "text/plain", message );
}

void sendPage()
{
  char temp[1000];
  
  snprintf ( temp, 1000,

    "<html>\
      <head>\
        <title>IOT Energy Webserver</title>\
        <style>\
          body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
        </style>\
        </head>\
        <body>\
          <h1>IOT Energy Webserver</h1>\
          <BR>\
          <FORM ACTION=\"http://%s\" method=get >\
          Http address: %s\
          <BR>\
          <INPUT TYPE=TEXT NAME=\"HTTP_ADDR\" VALUE=\"%s\" SIZE=\"25\" MAXLENGTH=\"50\"><BR>\
          <BR>\
          Write API: %s\
          <BR>\
          <INPUT TYPE=TEXT NAME=\"CODE\" VALUE=\"%s\" SIZE=\"30\" MAXLENGTH=\"50\"><BR>\
          <BR>\
          Voltage: %s\
          <BR>\
          <INPUT TYPE=TEXT NAME=\"VOLTAGE\" VALUE=\"%s\" SIZE=\"10\" MAXLENGTH=\"50\"><BR>\
          <BR>\
          <INPUT TYPE=SUBMIT NAME=\"submit\" VALUE=\"Apply\">\
          <BR>\
          <A HREF=\"javascript:window.location.href = 'http://%s'\">Click to refresh the page</A>\
          </FORM>\
          <BR>\
        </body>\
      </html>",
  
      WiFi.localIP().toString().c_str(),
      httpAddress.c_str(),
      httpAddress.c_str(),
      apiCode.c_str(),
      apiCode.c_str(),
      voltage.c_str(),
      voltage.c_str(),
      WiFi.localIP().toString().c_str()
    );
//  Serial.println(temp);
  httpServer.send ( 200, "text/html", temp );
}

