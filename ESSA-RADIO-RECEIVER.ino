#include <CanSatKit.h>
#include <SD.h>
using namespace CanSatKit;

// set radio receiver parameters - see comments below
// remember to set the same radio parameters in
// transmitter and receiver boards!
Radio radio(Pins::Radio::ChipSelect,
            Pins::Radio::DIO0,
            433.0,                  // frequency in MHz
            Bandwidth_125000_Hz,    // bandwidth - check with CanSat regulations to set allowed value
            SpreadingFactor_9,      // see provided presentations to determine which setting is the best
            CodingRate_4_8);        // see provided presentations to determine which setting is the best

void setup() {
  SerialUSB.begin(115200);  
  if (!radio.begin()) SerialUSB.println("RADIO ISSUE OCCURED");
  if (!SD.begin(11)) SerialUSB.println("SD CARD ISSUE OCCURED");
  SerialUSB.println("GROUND STATION READY");
  File dataFile = SD.open("BackupReceivedData.txt", FILE_WRITE);
  dataFile.println("RSSI: Temp[*C]: Press[Pa]: Alt[m]: Light[%]: Humid[%]: Lat[*]: Lon[*]:");
  dataFile.close();
}


void loop() {
  char data[256];
  radio.receive(data);
  //sending data trough serial communication:
  SerialUSB.print(radio.get_rssi_last());
  SerialUSB.print(" ");
  SerialUSB.println(data);
//saving data on a backup file:  
  File dataFile = SD.open("BackupReceivedData.txt", FILE_WRITE);
  dataFile.println(data);
  dataFile.close();
}