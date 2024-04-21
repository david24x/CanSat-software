#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <CanSatKit.h>
#include <SD.h>
#include <RBD_LightSensor.h>
#include <DHT11.h>
#include <ArduinoNmeaParser.h>

//   ___       _     _ _         ___        __  __             _      __  __          
//  / _ \ _ __| |__ (_) |_      / _ \      |  \/  | __ _ _ __ (_) __ _\ \/ /          
// | | | | '__| '_ \| | __|____| | | |_____| |\/| |/ _` | '_ \| |/ _` |\  /           
// | |_| | |  | |_) | | ||_____| |_| |_____| |  | | (_| | | | | | (_| |/  \           
//  \___/|_|  |_.__/|_|\__|     \___/      |_|  |_|\__,_|_| |_|_|\__,_/_/\_\          
//   ___                       _   _               ____            _                  
//  / _ \ _ __   ___ _ __ __ _| |_(_)_ __   __ _  / ___| _   _ ___| |_ ___ _ __ ___   
// | | | | '_ \ / _ \ '__/ _` | __| | '_ \ / _` | \___ \| | | / __| __/ _ \ '_ ` _ \  
// | |_| | |_) |  __/ | | (_| | |_| | | | | (_| |  ___) | |_| \__ \ ||  __/ | | | | | 
//  \___/| .__/ \___|_|  \__,_|\__|_|_| |_|\__, | |____/ \__, |___/\__\___|_| |_| |_| 
//       |_|                               |___/         |___/                        
// Written and developed by Adam Ciupa

using namespace CanSatKit;

String lati,longi;
float groundpressure;
bool issue = false;
bool flightmode = false;              //declaring variables
bool landingmode = false;
int bov = 0; //*buzzer operational variable*

void onRmcUpdate(nmea::RmcData const);
void onGgaUpdate(nmea::GgaData const);

//declaring some stuff
ArduinoNmeaParser parser(onRmcUpdate, onGgaUpdate);
RBD::LightSensor light_sensor(A5);
#define buzzer 5
#define led 9
DHT11 dht11(2); //humidity sensor
Adafruit_BMP280 bmp; //temperature and pressure sensor
Frame frame; // radio frame to hold the data
Radio radio(Pins::Radio::ChipSelect, //radio
            Pins::Radio::DIO0,
            433.6,
            Bandwidth_125000_Hz,
            SpreadingFactor_9,
            CodingRate_4_8);

void setup() {
pinMode(buzzer, OUTPUT);
pinMode(led, OUTPUT);

SerialUSB.begin(115200);
Serial.begin(9600);
if(!radio.begin()) issue = true; //radio init
radio.disable_debug();
if(!bmp.begin(0x76)) issue = true; //bmp280 init
if(!SD.begin(11)) issue = true; // SD init
bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                 Adafruit_BMP280::SAMPLING_X2,
                 Adafruit_BMP280::SAMPLING_X16, //bmp280 settings
                 Adafruit_BMP280::FILTER_X16,
                 Adafruit_BMP280::STANDBY_MS_500); 


File dataFile = SD.open("ESSA2K24.txt", FILE_WRITE);
dataFile.println("Temp[*C]: Press[Pa]: Light[%]: Humid[%]:"); //SD card file header 
dataFile.close();

groundpressure = bmp.readPressure()/100; // reading pressure for flightmode and landingmode enabling purposes

if(issue){
for(int i = 0; i<=10; i++){
digitalWrite(led, HIGH);
tone(buzzer, 2000);
delay(250);                     //led indicating sensors issue
digitalWrite(led, LOW);
noTone(buzzer);
delay(250);
}
}
digitalWrite(led, HIGH); //led indicating readiness for flight
tone(buzzer, 2000);
delay(500);
noTone(buzzer);
radio.transmit("CANSAT INIT COMPLETE, FLY HIGH ESSA!");
}

void loop() {
bov++; //variable operation for buzzer loop
if(flightmode == false && bmp.readAltitude(groundpressure) > 100){
  flightmode = true;                                                  //checking for flightmode conditions and enabling flightmode
  digitalWrite(led, LOW);
}

if(flightmode == true && landingmode == false && bmp.readAltitude(groundpressure) < 70){
  landingmode = true;                                                                               //checking for landingmode conditions and enabling landingmode
}

while(Serial.available()) {
  parser.encode((char)Serial.read());             //reading data from gps
}


if(landingmode==true && bov%2==0){
  tone(buzzer, 2000);
}else{                               //buzzer buzzing loop
  noTone(buzzer);
}
SDsave();
radioTransmit();
delay(500);
}



void radioTransmit() {
  frame.print(bmp.readTemperature());
  frame.print(" ");
  frame.print(bmp.readPressure());
  frame.print(" "); 
  frame.print(100 - light_sensor.getPercentValue());
  frame.print(" ");
  frame.print(dht11.readHumidity());
  frame.print(" ");
  frame.print(lati);
  frame.print(" ");
  frame.print(longi);
  if(radio.transmit(frame) == false) radio.flush();
  frame.clear();
}

void SDsave() {
  File dataFile = SD.open("ESSA2K24.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print(bmp.readTemperature());
    dataFile.print(" ");
    dataFile.print(bmp.readPressure());
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
  }else{                                          //gps data parsing
    longi = "-";
    lati = "-";
  }

}

void onRmcUpdate(nmea::RmcData const rmc)
{
// You can enjoy this beautiful ASCII art of the Shrek from twichquotes.com
//  ⢀⡴⠑⡄⠀⠀⠀⠀⠀⠀⠀⣀⣀⣤⣤⣤⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
//  ⠸⡇⠀⠿⡀⠀⠀⠀⣀⡴⢿⣿⣿⣿⣿⣿⣿⣿⣷⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠑⢄⣠⠾⠁⣀⣄⡈⠙⣿⣿⣿⣿⣿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⢀⡀⠁⠀⠀⠈⠙⠛⠂⠈⣿⣿⣿⣿⣿⠿⡿⢿⣆⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⢀⡾⣁⣀⠀⠴⠂⠙⣗⡀⠀⢻⣿⣿⠭⢤⣴⣦⣤⣹⠀⠀⠀⢀⢴⣶⣆ 
//  ⠀⠀⢀⣾⣿⣿⣿⣷⣮⣽⣾⣿⣥⣴⣿⣿⡿⢂⠔⢚⡿⢿⣿⣦⣴⣾⠁⠸⣼⡿ 
//  ⠀⢀⡞⠁⠙⠻⠿⠟⠉⠀⠛⢹⣿⣿⣿⣿⣿⣌⢤⣼⣿⣾⣿⡟⠉⠀⠀⠀⠀⠀ 
//  ⠀⣾⣷⣶⠇⠀⠀⣤⣄⣀⡀⠈⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
//  ⠀⠉⠈⠉⠀⠀⢦⡈⢻⣿⣿⣿⣶⣶⣶⣶⣤⣽⡹⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⠀⠉⠲⣽⡻⢿⣿⣿⣿⣿⣿⣿⣷⣜⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣷⣶⣮⣭⣽⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⣀⣀⣈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
//  ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠻⠿⠿⠿⠿⠛⠉
}
