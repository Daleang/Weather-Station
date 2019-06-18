/* ********************************************************************
 *  Personal Weather Station
 *  With reporing to Weather Underground (weatherunderground.com)
 *  
 *  for project details visit www.jaycar.com.au/arduino
 *  
 *  17/4/2018   V1.0
 *  
 * *********************************************************************
 */

// This is where all the libraries are included
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"


// Use this library
// https://www.arduinolibraries.info/libraries/dht-sensor-library-for-es-px 


// This is where the literal declerations are made
#define timezone              10       // Time zone including day light savings
#define dst                    0       // There is a bug in the time synch, day light savings does not work
#define DHTPIN                 2       // DHT11 signal connected to WiMos D1 pin D4
#define DHTTYPE                DHT11   // DHT 11
#define OLED_RESET             0       // GPIO0
#define reading_interval      30000    // interval between readings in mSec
#define _temp                  0       // index for storing Temp value
#define _humd                  1       // index for storing humidity value


// The following code creates objects for the OLED and DTH temperature/humidity sensor
Adafruit_SSD1306 display(OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);

// Here edit the ssid and password firelds to your network details
#pragma region Globals
const char* ssid = "your WiFi SSID here";             // WIFI network name
const char* password = "your WiFi password here";     // WIFI network password

uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals

// Weather Underground web site settings
char SERVER[] = "rtupdate.wunderground.com";           // Realtime update server - RapidFire
//char SERVER [] = "weatherstation.wunderground.com";  //standard server
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "yor Weather Underground Station ID";                        // Change the ID and password to your weather station settings
char PASSWORD [] = "your Weather Underground Station password";

time_t time_now;
struct tm * now_timeinfo;
unsigned long timer;

float     h;
float     t;    
 
/*
 * This function establish the WiFi network connection. Return True (good connection) or False (could not connect)
 */
uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print(F("Connecting to "));
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);  
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print(F("Connection: TIMEOUT on attempt: "));
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println(F("Check if access point available or SSID and Password\r\n"));
        return false;
    }
    Serial.println(F("Connection: ESTABLISHED"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    return true;
}


void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}


void update_wunderground() {

   WiFiClient client;
 
 //Send data to Weather Underground
 if (client.connect(SERVER, 80)) { 
    
    client.print(WEBPAGE); 
    client.print("ID=");
    client.print(ID);
    client.print("&PASSWORD=");
    client.print(PASSWORD);
    client.print("&dateutc=");
    client.print("now");    //can use instead of RTC if sending in real time
    
    client.print("&tempf=");
    client.print(t+50.4);
//    client.print("&baromin=");
//    client.print(12);
//    client.print("&dewptf=");
//    client.print(t);
    client.print("&humidity=");
    client.print(h);
 
    //client.print("&action=updateraw");//Standard update
    client.print("&softwaretype=Arduino%20UNO%20version1&action=updateraw&realtime=1&rtfreq=2.5");//Rapid Fire
    client.print(" HTTP/1.0\r\n");
    client.print("Accept: text/html\r\n");
    client.print("Host: ");
    client.print(SERVER);
    client.print("\r\n\r\n");
    client.println();
      
   }
    client.stop();
    
}
     
/*
 * this is the first function that is executed (once) upon power-up or reset. All the required 
 * program initialisation is done here.
 */
void setup() {
          
    Serial.begin(115200);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    // Clear the buffer.
    display.clearDisplay();
    display.setTextColor(WHITE);

    display.setTextSize(1);
    display.setCursor(30, 0);
    display.println("Weather");
    display.setCursor(15, 10);  
    display.println("Underground");
    
    display.setCursor(0, 25);
    display.setTextSize(1);   
    display.println("Connecting WiFi...");
    display.display();
    
    connection_state = WiFiConnect();
    if(!connection_state)  // if not connected to WIFI
        Awaits();          // constantly trying to connect

    configTime(timezone * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
    while (!time(nullptr)) {
      display.setCursor(0, 35);
      display.println("Time Sync ...");
      display.display();
      delay(1000);
    }

  dht.begin();

  time_now = time(nullptr);
        
  display.clearDisplay();
  display.setTextSize(1);
  display.display();

  timer = millis();
  
}

/*
 * This is the main program loop. It continues to loop forvever (or until you unplug the power). 
 */
void loop() {

    if (millis() - timer > reading_interval) {
      
        h = dht.readHumidity();       // Reading temperature or humidity takes about 250 milliseconds!
        t = dht.readTemperature();    // Read temperature as Celsius (the default)

            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }

          timer = millis();
      }
     

      update_wunderground();

      time_now = time(nullptr);             // Update the current time
      now_timeinfo = localtime(&time_now);  // Format the current time into tm structure
      
      display.clearDisplay();

      display.setCursor(35, 0);
      display.println("Weather");
      display.setCursor(20, 10);  
      display.println("Underground");
    
      display.setCursor(0, 25); 
      display.print(now_timeinfo->tm_mday);
      display.print("/");
      display.print(now_timeinfo->tm_mon+1);  
      display.print("/");
      display.print(1900+now_timeinfo->tm_year);  
      display.print("  ");
      display.print(now_timeinfo->tm_hour);
      display.print(":");
      display.print(now_timeinfo->tm_min);
      display.print(":");
      display.print(now_timeinfo->tm_sec);
  
      display.setCursor(0, 40);
      display.print(F("Temp: "));
      display.print(t);
      display.print(F(" oC"));
  
      display.setCursor(0, 55);
      display.print(F("Hmd: "));
      display.print(h);
      display.print(F(" %"));

      display.display();

          
}


