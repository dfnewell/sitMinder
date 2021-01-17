#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;             // your network SSID (name)
char pass[] = SECRET_PASS;           // your network key

const char* PARAM_INPUT_1 = "input1";

/*// Web text
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    input1: <input type="text" name="input1">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input2: <input type="text" name="input2">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input3: <input type="text" name="input3">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
// end web text */

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

WiFiServer server(80);

// IR Proximity Sensor and Piezo Constants
const int piezo=13;
const int irLED = 12;
const int irSensor = 14;
int dutyCycle = 255;
int sensorValue;

//SevenSeg Display Param (ACTIVE LOW)
const int power = 25;
const int topLeft = 26;
const int top = 33;
const int topRight = 32;
const int center = 27;
const int bottomLeft = 18;
const int bottom = 19;
const int bottomRight = 23;
const int dp = 34;


void setup() {
//Set up WiFi server, OLED display, Timer, 7 segment display, IR senesor  
  // WiFi Initialize
  Serial.begin(115200);
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  server.begin();
  
  // OLED Initialize
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // IR Proximity Sensor Initialize
  pinMode(13,OUTPUT);
  ledcSetup(0,2500,8);
  ledcAttachPin(piezo,0);

  // SevenSegDisplay initialize
  pinMode(power,OUTPUT);
  pinMode(topLeft,OUTPUT);
  pinMode(topRight,OUTPUT);
  pinMode(top,OUTPUT);
  pinMode(center,OUTPUT);
  pinMode(bottomLeft,OUTPUT);
  pinMode(bottom,OUTPUT);
  pinMode(bottomRight,OUTPUT);
  pinMode(dp,OUTPUT);
  digitalWrite(power,HIGH);
  sevenSegDisplay(0);
 }

int value = 0;

void loop() {
// First initialize website, and provide prompt on OLED

WiFiClient client = server.available();     // listen for incoming clients
initialdisplay();
  if (client) {                             // if you get a client,
startTimerDisplay();                        // Tell the user to press the button to start the timer
                                            // in a perfect world I could set this up to receive some kind of input from the client
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (c == '\n') {                    // if the byte is a newline character

          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println();

            client.print("Click <a href=\"/H\">here</a> to begin the activity timer!<br>");
          
            client.println();
            break;
          } else {    
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was 
        if (currentLine.endsWith("GET /H")) {
          startTimer();               // starts the timer!
        }
      }
    }
 // close the connection:
    client.stop();
  }
}

void startTimer(void) {
  uint32_t timer = millis();           // set up timer using millis()   
  while(true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,4);
    display.println(F("Time Started:"));
    display.setTextSize(2);
    display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
    int timeRemaining = (millis() - timer)/1000;
    display.println(timeRemaining);
    display.display();
  if (millis() - timer > 30000)        // use 30 seconds for demo
  {
    digitalWrite(irLED,HIGH);
    timer = millis();                   // once 30 seconds elapses, reset timer before entering while
    while (millis() - timer < 30000)   // this is technically the point you're supposed to get out of your chair
    {                                  // include some kind of update to the website?
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,4);
      display.println(F("Time Remaining:"));
      display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
      timeRemaining = (30000 - (millis() - timer))/1000;
      display.println(timeRemaining);
      display.display();
      sensorValue = analogRead(irSensor);
      if (sensorValue < 1300)           // Using rudimentary IR proximity sensor, check if they left their chair
        {annoyingSound();              // If they didn't, play an annoying sound at them so they leave
        getUpMessage();}               // Also send them a message!
      if (timeRemaining > 24)      // Show a count down on seven segment display.
        sevenSegDisplay(5);            // In a full-functioning system, it would display the minutes remaining
      else if (timeRemaining > 18) // this is roughly at 10x speed
        sevenSegDisplay(4);
      else if (timeRemaining > 12)
        sevenSegDisplay(3);
      else if (timeRemaining > 6)
        sevenSegDisplay(2);
      else if (timeRemaining > 0)
        sevenSegDisplay(1);
      else
        sevenSegDisplay(0);       
    }
   break;}
  }
}

void initialdisplay(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
  display.setCursor(0,4);
  display.println(F(" WELCOME"));
  display.setTextSize(1);
  display.println(F("  Access Server at: "));
  
  display.println(WiFi.localIP());
  display.display();
}

void startTimerDisplay(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,4);
  display.println(F("Press the button"));
  display.println(F("to get started"));
  display.display();
}

void getUpMessage(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("GET UP AND MOVE!"));
  display.display();
}

void sevenSegDisplay(int number) //function to control sevenSegDisplay
{
switch(number){
case 1: 
  digitalWrite(topRight,LOW);
  digitalWrite(bottomRight,LOW);
  digitalWrite(top,HIGH);
  digitalWrite(topLeft,HIGH);
  digitalWrite(center,HIGH);
  digitalWrite(bottomLeft,HIGH);
  digitalWrite(bottom,HIGH);
  break;
case 2:  
  digitalWrite(top,LOW);
  digitalWrite(topRight,LOW);
  digitalWrite(center,LOW);
  digitalWrite(bottomLeft,LOW);
  digitalWrite(bottom,LOW);
  digitalWrite(topLeft,HIGH);
  digitalWrite(bottomRight,HIGH);  
  break;
case 3: 
  digitalWrite(top,LOW);
  digitalWrite(topRight,LOW);
  digitalWrite(center,LOW);
  digitalWrite(bottomRight,LOW);
  digitalWrite(bottom,LOW);
  digitalWrite(topLeft,HIGH);
  digitalWrite(bottomLeft,HIGH);
  break;
case 4: 
  digitalWrite(topLeft,LOW);
  digitalWrite(center,LOW);
  digitalWrite(topRight,LOW);
  digitalWrite(bottomRight,LOW);
  digitalWrite(top,HIGH);
  digitalWrite(bottomLeft,HIGH);
  digitalWrite(bottom,HIGH);
  break;
case 5:
  digitalWrite(top,LOW);
  digitalWrite(topLeft,LOW);
  digitalWrite(center,LOW);
  digitalWrite(bottomRight,LOW);
  digitalWrite(bottom,LOW);
  digitalWrite(topRight,HIGH);
  digitalWrite(bottomLeft,HIGH);
  break;
default:
  digitalWrite(topLeft,HIGH);
  digitalWrite(top,HIGH);
  digitalWrite(topRight,HIGH);
  digitalWrite(center,HIGH);
  digitalWrite(bottomLeft,HIGH);
  digitalWrite(bottom,HIGH);
  digitalWrite(bottomRight,HIGH);
}
}

void annoyingSound(void) {
for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++)
{
  ledcWrite(0,dutyCycle);
  delay(15); 
}

//decrease the volume or pitch?
for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
{
  ledcWrite(0,dutyCycle);
  delay(15); 
}
}
