







#include <DHT.h>
#include <DHT_U.h>


#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
#define DHTTYPE DHT11
#define ECHO_TO_SERIAL 1 //Sends datalogging to serial if 1, nothing if 0
#define LOG_INTERVAL 36000 //milliseconds between entries (6 minutes = 360000)


const int soilMoisturePin = A1;
const int sunlightPin = A2;
const int dhtPin = 2;
const int chipSelect = 4;
const int LEDPinGreen = 6;
const int LEDPinRed = 7;
const int solenoidPin = 3;
const long int wateringTime = 10000; //Set the watering time (10 min for a start)
const float wateringThreshold = 400; //Value above which the garden gets watered

const double k = 5.0/1024;
const double luxFactor = 500000;
const double R2 = 220;
const double LowLightLimit = 200; 
const double B = 1.3*pow(10.0,7);
const double m = -1.4;

DHT dht(dhtPin, DHTTYPE);
RTC_DS1307 rtc;


float soilMoistureRaw = 0; //Raw analog input of soil moisture sensor (volts)
float soilMoisture = 0; //Scaled value of volumetric water content in soil (percent)
float humidity = 0; //Relative humidity (%)
float airTemp = 0; //Air temp (degrees F)
float heatIndex = 0; //Heat index (degrees F)
double sunlight = 0; //Sunlight illumination in lux
bool watering = false;
bool wateredToday = false;
DateTime now;
File logfile;

/*
Soil Moisture Reference
Air = 0%
Really dry soil = 10%
Probably as low as you'd want = 40%
Well watered = 70%
Cup of water = 100%
*/

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(LEDPinRed, HIGH);
  
  while(1);
}
double light_intensity (int RawADC0) {  
    double V2 = k*RawADC0;
    double R1 = (5.0/V2 - 1)*R2;
    double lux = B*pow(R1,m);
    return lux;}



void setup() {
  
  //Initialize serial connection
  Serial.begin(9600); //Just for testing
  Serial.println("Initializing SD card...");
  
  
  pinMode(chipSelect, OUTPUT); //Pin for writing to SD card
  pinMode(LEDPinGreen, OUTPUT); //LED green pint
  pinMode(LEDPinRed, OUTPUT); //LED red pin
  pinMode(solenoidPin, OUTPUT); //solenoid pin
  digitalWrite(solenoidPin, LOW); //Make sure the valve is off

  
  //Establish connection with DHT sensor
  dht.begin();
  
  //Establish connection with real time clock
  Wire.begin();

  
  //Set the time and date on the real time clock if necessary
  if (! rtc.isrunning()) {
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  //Check if SD card is present and can be initialized
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");        
  }
  
  Serial.println("Card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  
  
  logfile.println("Air Temp (F),Soil Moisture Content (%),Relative Humidity (%),Heat Index (F),Sunlight Illumination (lux),Watering?");   //HEADER 
#if ECHO_TO_SERIAL
  Serial.println("Air Temp (F),Soil Moisture Content (%),Relative Humidity (%),Heat Index (F),Sunlight Illumination (lux),Watering?");
#endif ECHO_TO_SERIAL// attempt to write out the header to the file

  now = rtc.now();
  
}

void loop() {
    
  //delay software
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  //Three blinks means start of new cycle
  digitalWrite(LEDPinGreen, HIGH);
  delay(150);
  digitalWrite(LEDPinGreen, LOW);
  delay(150);
  digitalWrite(LEDPinGreen, HIGH);
  delay(150);
  digitalWrite(LEDPinGreen, LOW);
  delay(150);
  digitalWrite(LEDPinGreen, HIGH);
  delay(150);
  digitalWrite(LEDPinGreen, LOW);
  

  
  
  
  //Collect Variables
 
  
  soilMoistureRaw = analogRead(soilMoisturePin);
//   soilMoistureRaw = analogRead(soilMoisturePin)*(3.3/1024);
  delay(20);
  
  Volumetric Water Content is a piecewise function of the voltage from the sensor
 if (soilMoistureRaw < 1.1) {
   soilMoisture = (10 * soilMoistureRaw) - 1;
 }
 else if (soilMoistureRaw < 1.3) {
   soilMoisture = (25 * soilMoistureRaw) - 17.5;
 }
 else if (soilMoistureRaw < 1.82) {
   soilMoisture = (48.08 * soilMoistureRaw) - 47.5;
 }
 else if (soilMoistureRaw < 2.2) {
   soilMoisture = (26.32 * soilMoistureRaw) - 7.89;
 }
 else {
   soilMoisture = (62.5 * soilMoistureRaw) - 87.5;
 }
    
  humidity = dht.readHumidity();
  delay(20);
  
  airTemp = dht.readTemperature(true);
  delay(20);
  
  heatIndex = dht.computeHeatIndex(airTemp,humidity);
  
  //This is a rough conversion that I tried to calibrate using a flashlight of a "known" brightness
  sunlight =light_intensity(analogRead(sunlightPin));
  delay(20);
  
  //Log variables
  
  logfile.print(airTemp);
  logfile.print(",");
  logfile.print(soilMoistureRaw);
  logfile.print(",");
  logfile.print(humidity);
  logfile.print(",");
  logfile.print(heatIndex);
  logfile.print(",");
  logfile.print(sunlight);
  logfile.print(",");
#if ECHO_TO_SERIAL

  Serial.print(airTemp);
  Serial.print(",");
  Serial.print(soilMoistureRaw);
  Serial.print(",");
  Serial.print(humidity);
  Serial.print(",");
  Serial.print(heatIndex);
  Serial.print(",");
  Serial.print(sunlight);
  Serial.print(",");
#endif
  
  if ((soilMoistureRaw > wateringThreshold) ) {
    //water the garden
    digitalWrite(solenoidPin, LOW);

  
    //record that we're watering
    logfile.print("TRUE");
#if ECHO_TO_SERIAL
    Serial.print("TRUE");
#endif
  
    wateredToday = true;
  }
  else {
    digitalWrite(solenoidPin, HIGH);
    logfile.print("FALSE");
#if ECHO_TO_SERIAL
    Serial.print("FALSE");
#endif
  }
  
  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif
  delay(50);
  
  //Write to SD card
  logfile.flush();
  delay(5000);
}
