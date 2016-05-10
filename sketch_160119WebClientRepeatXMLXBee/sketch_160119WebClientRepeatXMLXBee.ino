/*
 using this XML parse library https://github.com/adamvr/arduino-sketches/tree/master/libraries/TinyXML
 weather data from http://www.yr.no/place/Japan/Kanagawa/Yokohama/forecast_hour_by_hour.xml

 modified 5 Nov 2015
  by Tadanori Matsui
 */

#include <SPI.h>
#include <Ethernet.h>
#include <inttypes.h>
#include <TinyXML.h>
#include<LiquidCrystal.h>
#define LCD_COLS 16

// assign a MAC address for the ethernet controller.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// initialize the library instance:
EthernetClient client;
char server[] = "www.yr.no";

//initialization of update time
unsigned long lastConnectionTime = 0;             // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1200L * 1000L; // delay between updates, in milliseconds // the "L" is needed to use long type numbers

//initialization of XML parser
TinyXML xml;
uint8_t buffer[150];
uint16_t buflen = 150;

//initialization of LCD
LiquidCrystal lcd(9, 8, 7, 5, 4, 3, 2);
String lcdStr = "";
int lcdStrStartPosition = 0;

//xml parser
boolean getDataFlag = false;
int firstOneCounter = 0;

//xbee interval
long xbeePastMillis = millis();
long xbeeInterval = 1000;
long xbeeDataIndicator = 0;

void setup() {
  // start serial port:
  Serial.begin(9600);
  Serial.println(F("Catch Weather info on XML and Repeat Request and draw on LCD"));
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  xml.init((uint8_t*)&buffer, buflen, &XML_callback);

  Ethernet.begin(mac);
  delay(1000);
  Serial.print(F("My IP address: "));
  Serial.println(Ethernet.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("My IP address: "));
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print(Ethernet.localIP());

  delay(3000);

  //first data get
  httpRequest();

  delay(1000);

  if (client.available()) {
    while (true)
    {
      Serial.println(F("in the loop"));
      if (client.available())
      {
        char c = client.read();
        if (c == '<')
        {
          c = client.read();
          if (c == '?')
          {
            xml.processChar('<');
            xml.processChar('?');
            break;
          }
        }
      }
    }
  }

  Serial.println(F("Waiting for response"));

  while (client.available() > 0)
  {
    if (client.available())
    {
      char c = client.read();
      xml.processChar(c);
      //Serial.print(c);
    }
  }

  Serial.println(F("Finished reading"));

  lcd.begin(LCD_COLS, 2);
}

void loop() {
  //xbee data send, if I can use 2 or more hardware serial (for ex. fio) it could be better
  if (millis() - xbeePastMillis > xbeeInterval) {
    xbeePastMillis = millis();
    if (lcdStr.equals("No data!No data!")) { //if no data random command send
      String tempRandomCommand = "ND ";
      char tempF[3];
      sprintf(tempF, "%03d", random(0, 180));
      tempRandomCommand += "f";
      tempRandomCommand += tempF;
      tempRandomCommand += "\n";

      char tempG[3];
      sprintf(tempG, "%03d", random(0, 180));
      tempRandomCommand += "g";
      tempRandomCommand += tempG;
      tempRandomCommand += "\n";

      switch (random(0, 5)) {
        case 0:
          tempRandomCommand += "a";
          tempRandomCommand += 0;
          break;
        case 1:
          tempRandomCommand += "b";
          tempRandomCommand += 0;
          break;
        case 2:
          tempRandomCommand += "c";
          tempRandomCommand += 0;
          break;
        case 3:
          tempRandomCommand += "d";
          tempRandomCommand += 0;
          break;
        case 4:
          tempRandomCommand += "e";
          tempRandomCommand += 0;
          break;
        default:
          break;
      }
      Serial.println(tempRandomCommand);
    }
    else {
      Serial.print(lcdStr.substring(xbeeDataIndicator * 8, (xbeeDataIndicator + 1) * 8));
      xbeeDataIndicator++;
      if (xbeeDataIndicator > lcdStr.length() / 8) xbeeDataIndicator = 0;

      String tempRandomCommand = "_";
      switch (random(0, 2)) {
        case 0:
          char tempF[3];
          sprintf(tempF, "%03d", random(0, 180));
          tempRandomCommand += "f";
          tempRandomCommand += tempF;
          break;
        case 1:
          char tempG[3];
          sprintf(tempG, "%03d", random(0, 180));
          tempRandomCommand += "g";
          tempRandomCommand += tempG;
          break;
        default:
          break;
      }
      tempRandomCommand += "a0";
      Serial.println(tempRandomCommand);
    }
  }

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {

    httpRequest();

    delay(1000);

    if (client.available()) {
      while (true)
      {
        Serial.println(F("in the loop"));
        if (client.available())
        {
          char c = client.read();
          if (c == '<')
          {
            c = client.read();
            if (c == '?')
            {
              xml.processChar('<');
              xml.processChar('?');
              break;
            }
          }
        }
      }

      Serial.println(F("Waiting for response"));
    }

    while (client.available() > 0)
    {
      if (client.available())
      {
        char c = client.read();
        xml.processChar(c);
        //Serial.print(c);
      }
    }
    Serial.println(F("Finished reading"));

    xbeeDataIndicator = 0;
  }

  if (lcdStr.equals("")) lcdStr = "No data!No data!";
  //lcd drawing
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.setCursor(i, 0);
    int tempPos = i + lcdStrStartPosition;
    if (tempPos >= lcdStr.length()) tempPos = tempPos - lcdStr.length();
    lcd.print(lcdStr.charAt(tempPos));

    lcd.setCursor(i, 1);
    int tempPos1 = i + lcdStrStartPosition / 2;
    if (tempPos1 >= lcdStr.length()) tempPos1 = tempPos1 - lcdStr.length();
    lcd.print(lcdStr.charAt(tempPos1));
  }
  lcdStrStartPosition++;
  if (lcdStrStartPosition >= lcdStr.length() - 1) lcdStrStartPosition = 0;
  delay(250);
}

// this method makes a HTTP connection to the server:
void httpRequest() {
  client.stop();  // close any connection before send a new request.  // This will free the socket on the WiFi shield

  if (client.connect(server, 80)) {  // if there's a successful connection:
    lcdStr = "";
    getDataFlag = false;
    firstOneCounter = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("get weather from"));
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print(server);
    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("My IP address: "));
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print(Ethernet.localIP());
    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("get weather from"));
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print(server);
    delay(1000);

    Serial.println(F("connecting..."));
    // send the HTTP PUT request:
    client.println(F("GET /place/Japan/Kanagawa/Yokohama/forecast_hour_by_hour.xml HTTP/1.0"));
    client.println(F("Host: www.yr.no"));
    client.println(F("User-Agent: arduino-ethernet"));
    client.println(F("Connection: close"));
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println(F("connection failed"));
  }
}


void XML_callback( uint8_t statusflags, char* tagName,  uint16_t tagNameLen,  char* data,  uint16_t dataLen )
{
  if (statusflags & STATUS_START_TAG)
  {
    if ( tagNameLen )
    {
      if (String(tagName).equals(F("/weatherdata/forecast/tabular/time"))) {
        if (firstOneCounter == 0) {
          Serial.println(F("---------- flag ON ----------"));
          Serial.print(F("Start tag "));
          Serial.println(tagName);
          getDataFlag = true;
        }
      }
      else if (String(tagName).equals(F("/weatherdata/location"))) {
        if (firstOneCounter == 0) {
          Serial.println(F("---------- flag ON ----------"));
          Serial.print(F("Start tag "));
          Serial.println(tagName);
          getDataFlag = true;
        }
      }
    }
  }
  else if  (statusflags & STATUS_END_TAG)
  {
    if (String(tagName).equals(F("/weatherdata/forecast/tabular/time"))) {
      if (firstOneCounter == 0) {
        Serial.print(F("End tag "));
        Serial.println(tagName);
        Serial.println(F("---------- flag OFF ----------"));
        Serial.print(F("lcdStr = "));
        Serial.println(lcdStr);
        getDataFlag = false;
      }
      firstOneCounter++;
    }
    else if (String(tagName).equals(F("/weatherdata/location"))) {
      if (firstOneCounter == 0) {
        Serial.print(F("End tag "));
        Serial.println(tagName);
        Serial.println(F("---------- flag OFF ----------"));
        Serial.print(F("lcdStr = "));
        Serial.println(lcdStr);
        getDataFlag = false;
      }
    }
  }
  else if  (statusflags & STATUS_TAG_TEXT)
  {
    if (getDataFlag) {
      Serial.print("Tag:");
      Serial.print(tagName);
      Serial.print(" text:");
      Serial.println(data);
      if (String(tagName).equals(F("/weatherdata/location/name"))) {
        lcdStr += " ";
        lcdStr += data;
      }
    }
  }
  else if  (statusflags & STATUS_ATTR_TEXT)
  {
    if (getDataFlag) {
      Serial.print(F("Attribute:"));
      Serial.print(tagName);
      Serial.print(F(" text:"));
      Serial.println(data);
      if (String(tagName).equals(F("from"))) {
        lcdStr += F(" from");
        lcdStr += data;
      } else if (String(tagName).equals(F("to"))) {
        lcdStr += F(" to");
        lcdStr += data;
      } else if (String(tagName).equals(F("name"))) {
        lcdStr += F(" ");
        lcdStr += data;
      } else if (String(tagName).equals(F("value"))) {
        lcdStr += F(" ");
        lcdStr += data;
      } else if (String(tagName).equals(F("unit"))) {
        lcdStr += F(" ");
        lcdStr += data;
        lcdStr += F(":");
      } else if (String(tagName).equals(F("mps"))) {
        lcdStr += F(" mps:");
        lcdStr += data;
      }
    }
  }
  else if  (statusflags & STATUS_ERROR)
  {
    if (getDataFlag) {
      Serial.print(F("XML Parsing error  Tag:"));
      Serial.print(tagName);
      Serial.print(F(" text:"));
      Serial.println(data);
    }
  }
}
