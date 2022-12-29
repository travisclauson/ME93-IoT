#include "arduino_secrets.h"
#define VERBOSE false
#include <WiFiNINA.h>
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE);     //u8g2 constructor

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
String apikey = "YLj9sqfORMOV3WUwbrqCt8vXjC5WIr520QpVeuRbbE";     // your API Key from SystemLink
int status = WL_IDLEraspi_STATUS;
char server[] = "api.systemlinkcloud.com";    
WiFiSSLClient client;
String SL_Time = "";

int pipeStartLocX = 60;
int pipeStartLocY = 10;
int pipeCurrY = 10;
int pipeCurrX = 10;
int nextLineDist = 8;       //should be same as font size
int heightOfPipe;       //from top of screen
int linesBetweenPipes = 6;  //gap size
int bottomPipe;         //lines from top of screen
int vertLines = 128 / nextLineDist;
int joyStickX;
int joyStickY;
int lastJump = 100; //frames since last jump //set to a high number until first jump
const int jumpSize = 8; //inital increase in height,                                        changed from 8
int currJump = -2;   //
int score = 0;
int nameCounter = 0;
bool joyMove = false;
bool joyRelease = true;
bool gameEnd;
bool intro = true;
String userName[5] = {"Jess", "Travis", "Briana", "Anica", "Barrett"};
String currentName;
String pipeSides = "|  |";
String pipeEndBottom = "  _";
String pipeEndTop = "|_|";
String bird = "(,)>";
const int birdX = 20;
int birdY = 80;
int pipeSpeed = 3;                                                                            //changed from 2

void setup(){
    u8g2.begin();
    Serial.begin(9600);
    if (VERBOSE) while (!Serial); // waits for serial connection, if verbose is true
    if (!StartWiFi()) {  
        while (true); // don't continue if wifi is not connected
  }
}

void loop(){
    heightOfPipe = random(5);
    bottomPipe = heightOfPipe+linesBetweenPipes + 2;
    pipeCurrY = pipeStartLocY;
    pipeCurrX = pipeStartLocX;
    joyStickX = analogRead(A5);
    u8g2.clearBuffer();
    introScreen();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    runGame();
    if (gameEnd) {
        endGame();
    }
    Serial.println("Score: " + String(score));
}

void introScreen() {
    while (intro) {
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.setCursor(0,15);
        u8g2.print("CRAPPY");
        u8g2.setCursor(0,30);
        u8g2.print("BIRD");
        u8g2.setCursor(0,55);
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.print("Click");
        u8g2.setCursor(0,65);
        u8g2.print("joystick");
        u8g2.setCursor(0,75);
        u8g2.print("to start");
        u8g2.sendBuffer();
        joyStickX = analogRead(A5);
        if (joyStickX > 1000) {
            delay(500);
            joyStickX = analogRead(A5);
            while (joyStickX < 1000) {
                u8g2.setFont(u8g2_font_ncenB08_tr);
                if (joyStickX > 750 and joyStickX < 1000) {
                    nameCounter++;
                    if (nameCounter > 4) {
                        nameCounter = 0;
                    }
                }
                if (joyStickX < 300) {
                    nameCounter--;
                    if (nameCounter < 0) {
                        nameCounter = 4;
                    }
                }
                u8g2.clearBuffer();
                u8g2.setCursor(0,10);
                u8g2.print("Scroll to");
                u8g2.setCursor(0,22);
                u8g2.print("side and");
                u8g2.setCursor(0,34);
                u8g2.print("click to");
                u8g2.setCursor(0,46);
                u8g2.print("select user:");
                u8g2.setCursor(0,70);
                u8g2.print("Username: ");
                u8g2.setCursor(0,85);
                u8g2.setFont(u8g2_font_ncenB10_tr);
                currentName = userName[nameCounter];
                u8g2.print(currentName);
                u8g2.sendBuffer();
                delay(100);
                joyStickX = analogRead(A5);
            }
            countdown();
            intro = false;
        }
    }
}

void runGame() {
    while (pipeCurrX >= -10 and !gameEnd) {
        u8g2.clearBuffer();
        joyMove = moveCheck();
        if (joyMove and joyRelease) { //checks if joystick is moved, but not held
            lastJump = 0;
            currJump = jumpSize;
            joyRelease = false;
        }
        joyMove = moveCheck();
        if (!joyMove) { //resets joyMove when joyStick released
            joyRelease = true;
        }
        moveBird();
        lastJump++;
        printTopPipe();
        pipeCurrY += linesBetweenPipes*nextLineDist;
        printBotPipe();
        u8g2.setCursor(birdX, birdY);
        u8g2.print(bird);
        u8g2.sendBuffer();
        collisionCheck();
        pipeCurrX -= pipeSpeed;
        pipeCurrY = pipeStartLocY;
        scoreCount();
    }
}

void countdown() {
    for (int i = 3; i > 0; i--) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB24_tr);
        u8g2.setCursor(20,90);
        u8g2.print(i);
        u8g2.sendBuffer();
        delay(950);
    }
}

void printTopPipe() {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    for (int i = 0; i <= heightOfPipe; i++) {
        u8g2.setCursor(pipeCurrX,pipeCurrY);
        u8g2.print(pipeSides);
        pipeCurrY += nextLineDist;
    }
    u8g2.setCursor(pipeCurrX,pipeCurrY);
    u8g2.print(pipeEndTop);
}

void printBotPipe() {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(pipeCurrX,pipeCurrY);
    u8g2.print(pipeEndBottom);
    pipeCurrY += nextLineDist;
    for (int i = 0; i <= vertLines-linesBetweenPipes-heightOfPipe; i++) {
            u8g2.setCursor(pipeCurrX,pipeCurrY);
            u8g2.print(pipeSides);
            pipeCurrY += nextLineDist;
        }
}

bool moveCheck() {
    joyStickX = analogRead(A5);
    joyStickY = analogRead(A6);
    if (joyStickX > 750 or joyStickX < 300 or joyStickY > 750 or joyStickY < 300) {
        return true;
    }
    else {
        return false;
    }
}

void moveBird() {
    if (lastJump <= 5) { //bird will be moving down after jumpSize+1 frames
        birdY -= currJump;
        currJump -= 2;
    }
    else {
        birdY += 2;
    }
}

void scoreCount() {
    if (birdX == pipeCurrX + 8) {
        score++;
    }
}

void collisionCheck() {
    if ((birdX + 6) >= pipeCurrX and (birdX + 6) <= (pipeCurrX + 6)) {
      Serial.println("bird y: ");
      Serial.println(birdY);
      Serial.println("top pipe");
      Serial.println((heightOfPipe+2)*nextLineDist);
      Serial.println("bottom pipe");
      Serial.println(bottomPipe*nextLineDist);
      
        if (birdY > (bottomPipe)*nextLineDist or (birdY - 6) < (heightOfPipe+2)*nextLineDist)  {
            gameEnd = true;
        }
    }
}

void endGame() {
    delay(5000);
    u8g2.clearBuffer();
    u8g2.setCursor(0,10);
    u8g2.print(userName[nameCounter]);
    u8g2.setCursor(0,25);
    u8g2.print("You lose!");
    u8g2.setCursor(0,42);
    u8g2.print("Score:");
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(0,60);
    u8g2.print(score);
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0,85);
    u8g2.print("Scan");
    u8g2.setCursor(0,95);
    u8g2.print("Thingmark");
    u8g2.setCursor(0, 105);
    u8g2.print("to view");
    u8g2.setCursor(0,115);
    u8g2.print("high scores.");
    u8g2.sendBuffer();
    PUT_SystemLink("current score", BuildString(String(score)));
    PUT_SystemLink("current name", BuildString(currentName));
    delay(3000);
    u8g2.clearBuffer();
    u8g2.setCursor(0,20);
    u8g2.print("Click to");
    u8g2.setCursor(0,30);
    u8g2.print("play again");
    u8g2.sendBuffer();
    joyStickX = analogRead(A5);
    while (joyStickX < 1000) {
        joyStickX = analogRead(A5);
    }
    intro = true;
    gameEnd = false;
    score = 0;
    birdY = 80;
    delay(500);
}

void PUT_SystemLink (String tag, String PostData) {
  Serial.print("\nTrying to connect for PUT...");
  bool success = client.connect(server, 443);
  if (success) {
    Serial.println("connected to server, sending " + PostData);
    client.println("PUT /nitag/v2/tags/" + tag + "/values/current HTTP/1.1");
    client.println("Host: api.systemlinkcloud.com");
    client.println("Content-Type:application/json");
    client.println("Accept:application/json");
    client.println("x-ni-api-key:" + apikey);
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(PostData.length());
    client.println();
    client.println(PostData);
    client.println();
    String response = GetReply();  
    Serial.println(PutStringValue(response));
  }
}

String GetReply() {
  String response = "";
  int counter = 0;
  while (client.connected() & (counter <100)) {  // connects for 1 sec if no data or loss of connection
    if (client.available()) {
      char c= client.read();
      if (VERBOSE) Serial.print(c);
      response += c;
      counter = 0;   // rezero counter
    }
    else {
      counter += 1;
      delay(10);
    }
  }
  if (VERBOSE) Serial.println();
  else Serial.println(response.substring(0,response.indexOf("\n")));
  if (VERBOSE) Serial.println("disconnecting from server.");
  client.stop();
  return response;
}

// converts string value to format readable by SystemLink:
String BuildString(String package) {
  return "{\"value\":{\"type\":\"STRING\", \"value\":\"" + package + "\"}}";
}

// converts string value to format readable by SystemLink
String BuildInt16(int package) {
  return "{\"value\":{\"type\":\"INT\", \"value\":\"" + String(package) + "\"}}";
}

String GetStringValue (String replyString)  {
  String json = replyString.substring(replyString.lastIndexOf("{"),replyString.lastIndexOf("}"));
  if (VERBOSE) Serial.println ("full reply: " + json);
  json = json.substring((json.lastIndexOf(":")+2),json.indexOf("}")-1);  // used to be +1, 0
  return json;
}

String GetStringValueURL (String replyString)  {
  String json = replyString.substring(replyString.lastIndexOf("{"),replyString.lastIndexOf("}"));
  if (VERBOSE) Serial.println ("full reply: " + json);
  json = json.substring((json.lastIndexOf(":")-4),json.indexOf("}")-1);  // used to be +1, 0
  return json;
}

String PutStringValue (String replyString)  {
  return replyString.substring(replyString.lastIndexOf("\n"));
}

bool StartWiFi() {
    // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    return 0;
  }
    // check for updated WiFi firmware:
  String fv = WiFi.firmwareVersion();
  if (VERBOSE) Serial.println("Firmware: "+fv);
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    //delay(10000);
  }
  Serial.println("Connected to wifi");
  if (VERBOSE) printWiFiStatus();
  return true;
}

// MAC: A4:CF:12:23:3D:9C or A4:CF:12:23:57:A4
void printWiFiStatus() {
  byte mac[6];
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  for (int i = 5; i<0; i--) {mac[i]=0;}
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  for (int i = 5; i>=0; i--) {
    Serial.print(mac[i],HEX);
    if (i!=0) Serial.print(":");
  }
  Serial.println();
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  if (VERBOSE) Serial.print("signal strength (RSSI):");
  if (VERBOSE) Serial.print(rssi);
  if (VERBOSE) Serial.println(" dBm");
}

String NextInfo(String searchText) {
  String temp = SL_Time.substring(0,SL_Time.indexOf(searchText) + searchText.length());
  SL_Time = SL_Time.substring(SL_Time.indexOf(searchText) + searchText.length());
  SL_Time.trim();
  return temp;
}