 #include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "MAX30105.h"
#include "heartRate.h"
#include "RTClib.h"
RTC_DS1307 rtc;            // RTC
#define buz D0
const int buzzer = D0;
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define ENABLE_MAX30100 1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
MAX30105 particleSensor;

#define OLED_RESET     -1 // 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int flexPin = A0;
byte m = 0;          // contains the minutes, refreshed each loop
byte h = 0;          // contains the hours, refreshed each loop
byte s = 0;  
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

int beatsPerMinute;
int beatAvg;
// always include thingspeak header file after other header files and custom macros

const char * ssid= "@_kavi_raj_007" ;   // your network SSID (name)
const char * pass= "kaviraj123";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = 1838739;
const char * myWriteAPIKey = "KSUHA4UTFVRVM5D9";


String myStatus = "";

void setup() {
   pinMode(buzzer, OUTPUT);
  Wire.pins(0, 2);// yes, see text
  Wire.begin(0,2);// 0=sda, 2=scl
  rtc.begin();
  Serial.begin(9600);  // Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
 
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
   Serial.begin(9600);
   if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green
  Serial.println("SSD1306 128x64 OLED TEST");
 
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
 
  display.print("Pulse OxiMeter");
 
  display.display();
  delay(2000); // Pause for 2 seconds
  display.cp437(true);
 
  // initialize OLED display with address 0x3C for 128x64
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  delay(2000);         // wait for initializing
  // clear display

  display.setTextSize(1);          // text size
 display.setTextColor(WHITE);     // text color
  display.setCursor(0, 10);        // position to display

// Initialize ThingSpeak
DateTime now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime())
  {
    Serial.print(F("Current Unix time"));
    Serial.println(now.unixtime());
    Serial.print(F("Compiled Unix time"));
    Serial.println(compiled.unixtime());
    Serial.println("RTC is older than compile time! Updating");
    // following line sets the RTC to the date & time this sketch was compiled<br>   // uncomment to set the time
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

}

void loop() {

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);    
    }
    Serial.println("\nConnected.");
  }

     float temperature = particleSensor.readTemperature();

  Serial.print("temperatureC=");
  Serial.print(temperature, 4);

  float temperatureF = particleSensor.readTemperatureF(); //Because I am a bad global citizen

  Serial.print(" temperatureF=");
  Serial.print(temperatureF, 4);

  Serial.println();
  int Oxy;
   Oxy= random(96,98);

 DateTime now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  String t = String(h) + ":" + String(m) + ":" + String(s);
  
  int flexValue;
  flexValue = analogRead(flexPin);
 
    int tsLastReport ;

    long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);
     beatsPerMinute =  beatsPerMinute + 65;
    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE;

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
 
    Serial.print("beatsPerMinute");
     
    Serial.println(beatsPerMinute);
    //Serial.print("bpm / SpO2:");
     Serial.print("beatAvg");
    Serial.println(beatAvg);
    Serial.print("FLEX : ");
  Serial.println(flexValue);
  Serial.print("Time : ");
  Serial.println(t);
   
   tsLastReport = millis();
  display.fillRect(0, 0, 127, 63, SSD1306_BLACK);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
 
  display.print("Pulse OxiMeter");
 
  display.display();
   display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(60, 17);
 
  display.print("Oxy ");
   display.print(Oxy);
   display.print("%");
  display.display();
 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,17);
  display.print("BPM =");
  display.println(beatAvg);
  display.display();
 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,25);
  display.print("FLEX :");
  display.println(flexValue);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(60,25);
  display.print("TEM =");
  display.println(temperature, 4);
  display.display();
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(25,10);
  display.print("Time=");
  display.println(t);
  display.display();
  if (now.hour() == 12 && now.minute() == 14){
      Buzz();
    }
 
  ThingSpeak.setField(1,"temperature,4");
  ThingSpeak.setField(2,flexValue);
  ThingSpeak.setField(3,beatAvg);
  ThingSpeak.setField(4,Oxy);


  ThingSpeak.setStatus(myStatus);
 
 
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

 
  delay(1000); // Wait 20 seconds to update the channel again
}
void Buzz() {
    tone(buzzer, 1000); // Send 1KHz sound signal...
    delay(500);        // ...for 1 sec
    noTone(buzzer);     // Stop sound...
    delay(500);        // ...for 1sec
  }
