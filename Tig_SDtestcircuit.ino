//Before running, use Tig_settime_serial.ino to set RTC time to accurate Windows time


#include <Wire.h> // built in library, for I2C communications
//#include "RTClib_Tig.h" // Date and time functions using a DS3231 RTC connected via I2C
#include <SPI.h> //build in library; SD card communicates through SPI using library SdFat
//#include <EEPROM.h> // WD; built in library, for reading the serial number stored in EEPROM
#include <SdFat.h>
//Thermocouple library for Max31855
//#include "Adafruit_MAX31855.h"
//ADC library for ADS1115, 16-bit and 4 channel (we use one here) connected via I2C
//#include <Adafruit_ADS1015.h>
//To create software serial to communicate with the DO probe by RS232
//#include <SoftwareSerial.h>


//#define ECHO_TO_SERIAL // see if prevents restart when plug in serial

/*  GLOBAL VARIABLES  */

//RTC_DS3231 rtc; // Real-time clock
SdFat sd; //SD card
SdFile myFile; //file to manipulate/add data to on SD
const int sdCS = 10; //CS is pin 10 for SD handling
//Adafruit_ADS1115 adc; //ADC chip
//int Model;
//int SerialNum;
//float DOmgLSet;
//SoftwareSerial doProbe(9,8); // RX, TX lines on Uno board, Dissolved Oxygen probe
//DateTime newtime; // used to track time in main loop
//DateTime curr; //used as current time in loop

double c = 15; //hold readings from thermocouple
double phMeas = 7.77; //hold readings from pH probe, was pHVolt
double doMeas = 7.77; //hold readings from DO probe, change from DOmgLMeas
const unsigned long tenSec = 10*1000; //keeping track of time
unsigned long lastSampleTime = 0; // so always writes first value

// Declare placeholder name for output files written to SD card
//char filename[] = "YYYYMMDD_HHMM_00.csv";

//uint32_t unixtimeArray[1]; // store unixtime values temporarily

//Adafruit_MAX31855 thermocouple(5, 6, 7);

unsigned long milli;
const int milliDelay = 2000; //delay time between readings in milliseconds
bool serialEcho = 1; //1 to write information to Serial for troubleshooting
                     //0 to supress

//gas control digital inputs on Uno board
const int O2valve = 2;
const int N2valve = 3;
const int CO2valve = 4;

// Flags to mark when sensors go bad
bool tcfail = false;
bool pHfail = false;
bool DOfail = false;

// Define two temperature limits for the thermocouples, pH, and DO values outside
// this range trigger error
float TClowerlimit = 5.0;
float TCupperlimit = 60.0;
float pHlowerlimit = 5;
float pHupperlimit = 10;
float DOlowerlimit = 0;
float DOupperlimit = 50;




//// -----------------------   SET-UP  LOOP------------------------------------------------ ////

void setup() {
  Serial.begin(57600); //was 4800 (tried 57600) communicates with computer via Serial port
  //doProbe.begin(9600); //communicates with DO probe
  pinMode(O2valve, OUTPUT); //control pin for O2 solenoid valve
  pinMode(N2valve, OUTPUT); //control pin for O2 solenoid valve
  pinMode(CO2valve, OUTPUT); //control pin for O2 solenoid valve

  delay(2000); // wait for console opening
  if (serialEcho) {
    Serial.println("Hello, commencing set up...");
  }

  /*  Set-up RTC  */
  Wire.begin(); //WD Start the I2C library with default options
  //rtc.begin(); //WD Start the rtc object with default options
  //if RTC is unresponsive, code enters indefinite loop, does not continue "loop"
  //if (! rtc.begin()) {
    //Serial.println("couldn't find RTC, stopping set-up.");
    //while (1); 
  //}
  //newtime = rtc.now(); // read a time from the real time clock
  //printTimeSerial(rtc.now()); // print time to serial monitor
  //Serial.println(newtime);
  

  //Test removing once battery is added; WD removed 14MAR2018 
//  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
//  rtc.adjust(compileTime); //remove once battery added
//  Serial.println("");
//  Serial.print("setting RTC to compile time:\t");
//  printTimeToSerial(compileTime);
  
//  if (rtc.lostPower()) {
//    Serial.println("RTC lost power, use Tig_settime_Serial.ino to set the time before proceeding!");
//    // following line sets the RTC to the date & time this sketch was compiled
//    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//  }
//  if (serialEcho) {
//    Serial.println(", rtc set up...");
//  }

  //initialize SD card------------------------------
  //pinMode(sdCS, OUTPUT); //set chip select for SD card to output
  if (!sd.begin(sdCS, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }
  if (serialEcho) {
    Serial.println("SD card set up...");
  }
  //Delete current test.txt file if present
  if (sd.exists("test.txt")) {
    sd.remove("test.txt");
  }
  
  //---------------------------------new 29jan2018
   //newtime = rtc.now(); // grab the current time
   //initFileName(newtime); // WD wants to include file version in output, uses initFileName fxn below
   //Serial.print("Writing to ");
   //Serial.println(filename);
    
  //Start file and print header line
//  if (!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END)) {
//    sd.errorHalt("opening data file for write failed");
//  }
  //myFile.println ("date \t time \t temp_degC \t pH_rawV \t O2_mgL"); //this line from old text file
  //myFile.close();

  //*  Set-up ADC  */
  //adc.begin();
  //adc.setGain(GAIN_ONE);

  /*  Begin reading DO Probe  */
  //Serial.println("Beginning reading...");
  //WaitforSerialChar('M');

  /*  Set-up complete */
  if (serialEcho) {
    Serial.println("set up complete.");
  }
}

//// ----------------------  data collection & control LOOP-----------------------------------   ////
void loop() {
  milli=millis();
  Serial.println(milli);
  delay(2000);
}
