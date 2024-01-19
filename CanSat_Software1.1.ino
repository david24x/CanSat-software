#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <CanSatKit.h>
#include <SD.h>
#include <RBD_LightSensor.h>
#include <DHT11.h>
#include <ArduinoNmeaParser.h>

#define buzzer 7
#define led 9

bool flightmode = false;

using namespace CanSatKit;

String minute,second,hour;
String lati,longi;
float groundpressure;

void onRmcUpdate(nmea::RmcData const);
void onGgaUpdate(nmea::GgaData const);

ArduinoNmeaParser parser(onRmcUpdate, onGgaUpdate);

//deklarowanie wwszystkiego
RBD::LightSensor light_sensor(A5); //fotorezystor
DHT11 dht11(2); //czujnik wilgoci
Adafruit_BMP280 bmp; //czujnik temperatury i cisnienia
Frame frame; // obiekt do trzymania danych
Radio radio(Pins::Radio::ChipSelect, //radio
            Pins::Radio::DIO0,
            433.0,
            Bandwidth_125000_Hz,
            SpreadingFactor_9,
            CodingRate_4_8);

void setup() {

pinMode(buzzer, OUTPUT);
pinMode(led, OUTPUT);

SerialUSB.begin(115200);
Serial.begin(9600);
if(!radio.begin()) SerialUSB.println("RADIO INIT FAILED");
radio.disable_debug();
if(!bmp.begin(0x76)) SerialUSB.println("BMP280 INIT FAILED");
if(!SD.begin(11)) SerialUSB.println("SD CARD SAVE INIT FAILED");
bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                 Adafruit_BMP280::SAMPLING_X2,
                 Adafruit_BMP280::SAMPLING_X16,
                 Adafruit_BMP280::FILTER_X16,
                 Adafruit_BMP280::STANDBY_MS_500); 


  File dataFile = SD.open("ESSA2K24.txt", FILE_WRITE); //otwiera plik
  dataFile.println("Time: Temp[*C]: Press[hPa]: Alt[m]: Light[%] Humid[%]");
  dataFile.close();
  groundpressure = bmp.readPressure()/100;
  SerialUSB.println("CANSAT INIT COMPLETE, FLY HIGH!");
  digitalWrite(led, HIGH);
}

void loop() {

if(bmp.readAltitude(groundpressure) > 100){
  flightmode = true;
  digitalWrite(led, LOW);
}

while(Serial.available()) {
  parser.encode((char)Serial.read());
}

if(flightmode){
  digitalWrite(buzzer, HIGH);
}

SDsave();
radioTransmit();
delay(500);
digitalWrite(buzzer, LOW);

}



void radioTransmit(){
  frame.print(bmp.readTemperature());
  frame.print(" ");
  frame.print(bmp.readPressure()/100);
  frame.print(" "); 
  frame.print(100 - light_sensor.getPercentValue());
  frame.print(" ");
  frame.print(dht11.readHumidity());
  frame.print(" ");
  frame.print(lati);
  frame.print(" ");
  frame.println(longi);
  radio.transmit(frame);
  frame.clear();
}

void SDsave(){
  File dataFile = SD.open("ESSA2K24.txt", FILE_WRITE); //otwiera plik
  if (dataFile) {
    dataFile.print(hour);
    dataFile.print(".");
    dataFile.print(minute);
    dataFile.print(".");
    dataFile.print(second);
    dataFile.print(" ");
    dataFile.print(bmp.readTemperature());
    dataFile.print(" ");
    dataFile.print(bmp.readPressure()/100);
    dataFile.print(" ");
    dataFile.print(100 - light_sensor.getPercentValue());
    dataFile.print(" ");
    dataFile.println(dht11.readHumidity());
    dataFile.close();
  }
}
void onGgaUpdate(nmea::GgaData const gga)
{
  if (gga.fix_quality != nmea::FixQuality::Invalid)
  {
    
    longi = String(gga.longitude, 4);
    lati = String(gga.latitude, 4);
  }else{
    longi = "";
    lati = "";
  }

}

void onRmcUpdate(nmea::RmcData const rmc)
{
  if(rmc.time_utc.hour+1 < 10) hour = "0" + String(rmc.time_utc.hour+1);
  else hour = String(rmc.time_utc.hour+1);

  if(rmc.time_utc.minute < 10) minute = "0" + String(rmc.time_utc.minute);
  else minute = String(rmc.time_utc.minute);

  if(rmc.time_utc.second < 10) second = "0" + String(rmc.time_utc.second);
  else second = String(rmc.time_utc.second);
}
