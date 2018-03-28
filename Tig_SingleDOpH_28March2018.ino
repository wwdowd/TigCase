//Before running, use Tig_settime_serial.ino to set RTC time to accurate Windows time
//Change pH DO setpoints to single starting values on lines 37 and 41
//Type '1' and enter into serial window to enter new pH,DO setpoints as '7.45,8.37' (without '')


#include <Wire.h> // built in library, for I2C communications
#include "RTClib_Tig.h" // Date and time functions using a DS3231 RTC connected via I2C
#include <SPI.h> //build in library; SD card communicates through SPI using library SdFat
#include <EEPROM.h> // WD; built in library, for reading the serial number stored in EEPROM
#include <SdFat.h>
//Thermocouple library for Max31855
#include "Adafruit_MAX31855.h"
//ADC library for ADS1115, 16-bit and 4 channel (we use one here) connected via I2C
#include <Adafruit_ADS1015.h>
//To create software serial to communicate with the DO probe by RS232
#include <SoftwareSerial.h>


//#define ECHO_TO_SERIAL // see if prevents restart when plug in serial

/*  GLOBAL VARIABLES  */

RTC_DS3231 rtc; // Real-time clock
SdFat sd; //SD card
SdFile myFile; //file to manipulate/add data to on SD
const int sdCS = 10; //CS is pin 10 for SD handling
Adafruit_ADS1115 adc; //ADC chip
int Model;
int SerialNum;
float DOmgLSet;
SoftwareSerial doProbe(9,8); // RX, TX lines on Uno board, Dissolved Oxygen probe
DateTime newtime; // used to track time in main loop
DateTime curr; //used as current time in loop

//SETPOINT AND TOLERANCE DATA SET-UP

//pH
float pHset = 8.50; 
double pHtolerance = 0.03;
double phMult = 1;

//DO
float DOset = 8.5;
double DOtolerance = 0.06;
double DOMult = 1;

double phUpper = pHset + pHtolerance;
double phLower = pHset - pHtolerance;
double doUpper = DOset + DOtolerance;
double doLower = DOset - DOtolerance;

double c = 15; //hold readings from thermocouple
double phMeas = 7.77; //hold readings from pH probe, was pHVolt
double doMeas = 7.77; //hold readings from DO probe, change from DOmgLMeas
const unsigned long tenSec = 10*1000; //keeping track of time
unsigned long lastSampleTime = 0; // so always writes first value

// Declare placeholder name for output files written to SD card
char filename[] = "YYYYMMDD_HHMM_00.csv";

uint32_t unixtimeArray[1]; // store unixtime values temporarily

Adafruit_MAX31855 thermocouple(5, 6, 7);

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
  doProbe.begin(9600); //communicates with DO probe
  pinMode(O2valve, OUTPUT); //control pin for O2 solenoid valve
  pinMode(N2valve, OUTPUT); //control pin for O2 solenoid valve
  pinMode(CO2valve, OUTPUT); //control pin for O2 solenoid valve

  delay(2000); // wait for console opening
  if (serialEcho) {
    Serial.println("Hello, commencing set up...");
  }

  /*  Set-up RTC  */
  Wire.begin(); //WD Start the I2C library with default options
  rtc.begin(); //WD Start the rtc object with default options
  //if RTC is unresponsive, code enters indefinite loop, does not continue "loop"
  if (! rtc.begin()) {
    Serial.println("couldn't find RTC, stopping set-up.");
    while (1); 
  }
  newtime = rtc.now(); // read a time from the real time clock
  //printTimeSerial(rtc.now()); // print time to serial monitor
  //Serial.println(newtime);
  

  //Test removing once battery is added; WD removed 14MAR2018 
//  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
//  rtc.adjust(compileTime); //remove once battery added
//  Serial.println("");
//  Serial.print("setting RTC to compile time:\t");
//  printTimeToSerial(compileTime);
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, use Tig_settime_Serial.ino to set the time before proceeding!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  if (serialEcho) {
    Serial.println(", rtc set up...");
  }

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
   newtime = rtc.now(); // grab the current time
   initFileName(newtime); // WD wants to include file version in output, uses initFileName fxn below
   Serial.print("Writing to ");
   Serial.println(filename);
    
  //Start file and print header line
//  if (!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END)) {
//    sd.errorHalt("opening data file for write failed");
//  }
  //myFile.println ("date \t time \t temp_degC \t pH_rawV \t O2_mgL"); //this line from old text file
  //myFile.close();

  /*  Set-up ADC  */
  adc.begin();
  adc.setGain(GAIN_ONE);

  /*  Begin reading DO Probe  */
  Serial.println("Beginning reading...");
  WaitforSerialChar('M');

  /*  Set-up complete */
  if (serialEcho) {
    Serial.println("set up complete.");
  }
}

//// ----------------------  data collection & control LOOP-----------------------------------   ////
void loop() {
  curr = rtc.now();
  
  unsigned long milli = millis();
  unsigned long milli2 = millis();
  int milliInt = (int)milli;
  
  if (serialEcho) {
    printTimeToSerial (curr);
    //printTimeToSD (curr);
  }

  c = thermocouple.readCelsius();
   if (isnan(c)) {
     Serial.println("No data/something wrong with thermocouple!");
   } else {
      if (c > TCupperlimit | c < TClowerlimit) {
      Serial.println("Temperature data outside t/c bounds!");}
     else {
     if (serialEcho) {
     printTempToSerial (c);
     ////printTempToSD(c);
     }
   }
   }
  
  phMeas = getpH();
  if (phMeas > pHupperlimit | phMeas < pHlowerlimit) {
    Serial.println("pH data outside bounds!");
  }
  if(serialEcho) {
    printpHToSerial(phMeas);
    ////printpHToSD(phMeas);
  }
  
  // Read from O2 Probe
  doMeas = getDOmgL(); //get DO measurement in mg/L
  if (doMeas > DOupperlimit | doMeas < DOlowerlimit) {
    Serial.println("DO data outside bounds!");
  }
  if (serialEcho) {
    printDOmgLToSerial(doMeas);
    ////printDOmgLToSD(doMeas);
  }
//  newtime=rtc.now();
//  unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//  writeToSD();


Serial.println("    Begin");
Serial.print("pHset, DOset = ");
Serial.print(pHset);
Serial.print(", ");
Serial.println(DOset);
Serial.println("Type 1 in serial monitor and hit Enter to initiate setpoint change");

while (curr.year() < 2100) { //((milli - milli2) < 21600000) { //REPLACE while LOOP WITH TIME-BASED SETPOINT
    
  //loop to read Serial.input of typed pH DO setpoint values-----------
  if (Serial.available() > 0) {

    int input = Serial.parseInt();
    Serial.read();
    if (input == 1) {
      Serial.println("Enter new pHset value: ");
      while (Serial.available()==0) {             //Wait for user input
        pHset = Serial.parseFloat();
      }
      Serial.read();
      Serial.println("Enter new DOset value: ");
      while (Serial.available()==0) {             //Wait for user input
        DOset = Serial.parseFloat();
      }
      Serial.read();
      phUpper = pHset + pHtolerance; //recalculate Upper and Lower bounds when change setpoints,duh!
      phLower = pHset - pHtolerance;
      doUpper = DOset + DOtolerance;
      doLower = DOset - DOtolerance;
    Serial.print("New pH and DO setpoints entered: ");
    Serial.print(pHset);
    Serial.print(", ");
    Serial.println(DOset);
    Serial.read(); 
    }
  } // end serial.available input loop
    //end loop to read Serial.input of pHset DOset----------

   curr = rtc.now(); //take current time
   milli=millis();
   //read from sensors in loop
      c = thermocouple.readCelsius();
      phMeas = getpH();
      doMeas = getDOmgL();
      if (serialEcho) {
        printTimeToSerial (curr);
        //printTimeToSD (curr);
      }
      if (serialEcho) {
        printTempToSerial (c);
        ////printTempToSD(c);
      }
      if(serialEcho) {
        printpHToSerial(phMeas);
        ////printpHToSD(phMeas);
      }
      if (serialEcho) {
        printDOmgLToSerial(doMeas);
        ////printDOmgLToSD(doMeas);
      }
      Serial.print(" ");
      Serial.print(milli);
      Serial.println(" "); 

  //28mar2018 calculate how far off we are from setpoint, then multiplier of how much time to leave open valves
  double phDiff = abs((phMeas - pHset)/pHtolerance); //how many x tolerances are we from setpoint
  if (phDiff > 4) {
    phMult = 4;
  }
  else if (phDiff > 2) {
    phMult = 1.5;
  }
  else if (phDiff > 1.5) {
    phMult = 1;
  }
  double DODiff = abs((doMeas - DOset)/DOtolerance);
  if (DODiff > 4) {
    DOMult = 2.5;
  }
  else if (DODiff > 2) {
    DOMult = 1.5;
  }
  else if (DODiff > 1.5) {
    DOMult = 1;
  }
  //end calc multipliers-----------------

      
    //begin scenarios for pHset and DOset vs current reading - pulse gases appropriately
    if (getpH() > phLower && getpH() < phUpper && getDOmgL() > doLower && getDOmgL() < doUpper) {  //scenario 1 pH = / DO =
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      c = thermocouple.readCelsius();
//      phMeas = getpH();
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      

        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() > phLower && getpH() < phUpper && getDOmgL() > doUpper) { //scenario 2 pH = / DO +
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(300*DOMult);
//      digitalWrite(N2valve, HIGH);
//      digitalWrite(CO2valve, LOW);
//      digitalWrite(O2valve, LOW);
//      delay(00);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      c = thermocouple.readCelsius();
//      phMeas = getpH();
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() > phLower && getpH() < phUpper && getDOmgL() < doLower) { //scenario 3 ph = / DO -
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, HIGH);
      delay(50*DOMult);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      c = thermocouple.readCelsius();
//      phMeas = getpH();
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() > phUpper && getDOmgL() > doUpper) { //scenario 4 pH + / DO + 
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, HIGH);
      digitalWrite(O2valve, LOW);
      delay(10);
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(250*DOMult);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() > phUpper && getDOmgL() < doLower) {  //scenario 5 pH + / DO -
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, HIGH);
      digitalWrite(O2valve, HIGH);
      delay(10);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, HIGH);
      delay(50*DOMult);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() > phUpper && getDOmgL() > doLower && getDOmgL() < doUpper) {  //scenario 6 pH + / DO =
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, HIGH);
      digitalWrite(O2valve, LOW);
      delay(10);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() < phLower && getDOmgL() < doLower) { //scenario 7 pH - / DO - 
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, HIGH);
      delay(300*phMult);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, HIGH);
      delay(50*DOMult);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() < phLower && getDOmgL() > doUpper) { //scenario 8 pH - / DO + 
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(300*phMult);
//      digitalWrite(N2valve, HIGH);
//      digitalWrite(CO2valve, LOW);
//      digitalWrite(O2valve, LOW);
//      delay(450);
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        ////printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        ////printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        ////printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
        //      newtime=rtc.now();
        //      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
        //      writeToSD();
    }
    else if (getpH() < phLower && getDOmgL() > doLower && getDOmgL() < doUpper) {  //scenario 9 pH - / DO =
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(300*phMult);
      digitalWrite(N2valve, HIGH);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, HIGH); //was low!
      delay(20*phMult); //compensate for N2 by pumping O2 proportion to amount of N2 added to get O2 constant
      digitalWrite(N2valve, LOW);
      digitalWrite(CO2valve, LOW);
      digitalWrite(O2valve, LOW);
      delay(2000);
//      milli = millis();
//      milliInt = (int)milli;
//      curr = rtc.now();
//      if (serialEcho) {
//        printTimeToSerial (curr);
//        //printTimeToSD (curr);
//      }
//      c = thermocouple.readCelsius();
//      if (serialEcho) {
//        printTempToSerial (c);
//        //printTempToSD(c);
//      }
//      phMeas = getpH();
//      if(serialEcho) {
//        printpHToSerial(phMeas);
//        //printpHToSD(phMeas);
//      }
//      doMeas = getDOmgL();
//      if (serialEcho) {
//        printDOmgLToSerial(doMeas);
//        //printDOmgLToSD(doMeas);
//      }
//      Serial.print(" ");
//      Serial.print(milli);
//      Serial.println(" ");
      
    } // end if/else if loop 9 scenarios of pH DO
    
    newtime=rtc.now();
//    if (newtime.second() >= 9 & newtime.second() <= 11) {
//      newtime.second() == 10;
//      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//      }
//    else if (newtime.second() >= 19 & newtime.second() <= 21) {
//      newtime.second() == 20;
//      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//      }
//    else if (newtime.second() >= 29 & newtime.second() <= 31) {
//      newtime.second() == 30;
//      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//      }
//    else if (newtime.second() >= 39 & newtime.second() <= 41) {
//      newtime.second() == 40;
//      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//      }
//    else if (newtime.second() >= 49 & newtime.second() <= 51) {
//      newtime.second() == 50;
//      unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
//      }
    
    //set delay on recording data to 10s, based on https://forum.arduino.cc/index.php?topic=154787.0
    //const unsigned long tenSec = 10*1000;
    //static unsigned long lastSampleTime = 0 - tenSec;
    //unsigned long whattimenow = millis();
    if (milli - lastSampleTime >= tenSec) {
      lastSampleTime += tenSec;
      unixtimeArray[1] = curr.unixtime();  //fill array with currenttime from if loops above (was newtime)
      writeToSD();
    }
    
    //if (newtime.second() == 10 | newtime.second() == 20 | newtime.second() == 30 | newtime.second() == 40 | newtime.second() == 50 | newtime.second() == 0) {
    //unixtimeArray[1] = newtime.unixtime();  //fill array with currenttime
    //writeToSD();
    //}
  } //end while time loop
  Serial.println();
  //myFile.close();
  delay(5000);
  
  
}// ----------------end  data collection LOOP----------------------------------

//// ------------   FUNCTIONS -----------------   ////

static void printTimeToSD (DateTime curr) {
  myFile.print(curr.year(), DEC);
  myFile.print('/');
  myFile.print(curr.month(), DEC);
  myFile.print('/');
  myFile.print(curr.day(), DEC);
  myFile.print('\t');
  myFile.print(curr.hour(), DEC);
  myFile.print(':');
  myFile.print(curr.minute(), DEC);
  myFile.print(':');
  myFile.print(curr.second(), DEC);
}

static void printTimeToSerial (DateTime curr) {
  Serial.print(curr.year(), DEC);
  Serial.print('/');
  if (curr.month() < 10) {
    Serial.print(F("0"));
    }
  Serial.print(curr.month(), DEC);
  Serial.print('/');
  if (curr.day() < 10) {
    Serial.print(F("0"));
    }
  Serial.print(curr.day(), DEC);
  Serial.print('\t');
  if (curr.hour() < 10){
    Serial.print(F("0"));
    }
  Serial.print(curr.hour(), DEC);
  Serial.print(':');
  if (curr.minute() < 10) {
    Serial.print("0");
    }
  Serial.print(curr.minute(), DEC);
  Serial.print(':');
  if (curr.second() < 10) {
    Serial.print(F("0"));
    }
  Serial.print(curr.second(), DEC);
}

static void printTempToSD (double temp) {
  myFile.print('\t');
  myFile.print(temp);
}

static void printTempToSerial (double temp) {
  Serial.print("\t C = ");
  Serial.print(temp);
}

static void printpHToSD (double pH) {
  myFile.print('\t');
  myFile.print("*");
  myFile.print(pH);
  myFile.print("*");
}

static void printpHToSerial (double pH) {
  Serial.print('\t');
  Serial.print("pH (from ADC) = "); 
  Serial.print(pH);
}

static void printDOuMolToSD (double measure) {
  myFile.print("\t");
  myFile.print(measure);
}

static void printDOuMolToSerial (double measure) {
  Serial.print("\t DO (uMol) = ");
  Serial.print(measure);
}

static void printDOmgLToSD (double measure) {
  myFile.print("\t");
  myFile.print("-");
  myFile.println(measure);
  myFile.print("-");
}

static void printDOmgLToSerial (double measure) {
  Serial.print("\t DO (mg/L) = ");
  Serial.print(measure);
}

static double getpH () {
   int16_t pHRaw; //16-bit int dedicated to pH readings to be taken on A2
   pHRaw = adc.readADC_SingleEnded(2);
   double pHVolt = (pHRaw+2390.8)/3135.7; //with 1x gain 1 bit = 0.125mV; this conversion empirical
   return pHVolt;
}

void WaitforSerialChar(char start){
  char currChar = doProbe.read();
  while (currChar != (start)){
    currChar = doProbe.read();
  }
}

double getDOuMol(){
  WaitforSerialChar('M');
  doProbe.parseInt();
  doProbe.parseInt();
  float DOuMolMeas = doProbe.parseFloat();
}

double getDOmgL(){
  float DOuMolMeas = getDOuMol();
  float DOmgLMeas = DOuMolMeas*0.031998;  //convert uMol to mg/L CHECK THIS!!!
  return DOmgLMeas;
}

//-------------- initFileName --------------------------------------------------
// initFileName - a function to create a filename for the SD card based
// on the 4-digit year, month, day, hour, minutes and a 2-digit counter. 
// The character array 'filename' was defined as a global array 
// at the top of the sketch in the form "YYYYMMDD_HHMM_00.csv"
void initFileName(DateTime time1) {
  
  char buf[5];
  // integer to ascii function itoa(), supplied with numeric year value,
  // a buffer to hold output, and the base for the conversion (base 10 here)
  itoa(time1.year(), buf, 10);
  // copy the ascii year into the filename array
  for (byte i = 0; i < 4; i++){
    filename[i] = buf[i];
  }
  // Insert the month value
  if (time1.month() < 10) {
    filename[4] = '0';
    filename[5] = time1.month() + '0';
  } else if (time1.month() >= 10) {
    filename[4] = (time1.month() / 10) + '0';
    filename[5] = (time1.month() % 10) + '0';
  }
  // Insert the day value
  if (time1.day() < 10) {
    filename[6] = '0';
    filename[7] = time1.day() + '0';
  } else if (time1.day() >= 10) {
    filename[6] = (time1.day() / 10) + '0';
    filename[7] = (time1.day() % 10) + '0';
  }
  // Insert an underscore between date and time
  filename[8] = '_';
  // Insert the hour
  if (time1.hour() < 10) {
    filename[9] = '0';
    filename[10] = time1.hour() + '0';
  } else if (time1.hour() >= 10) {
    filename[9] = (time1.hour() / 10) + '0';
    filename[10] = (time1.hour() % 10) + '0';
  }
  // Insert minutes
    if (time1.minute() < 10) {
    filename[11] = '0';
    filename[12] = time1.minute() + '0';
  } else if (time1.minute() >= 10) {
    filename[11] = (time1.minute() / 10) + '0';
    filename[12] = (time1.minute() % 10) + '0';
  }
  // Insert another underscore after time
  filename[13] = '_';
//  // If there is a valid serialnumber, insert it into 
//  // the file name in positions 17-20. 
//  if (serialValid) {
//    byte serCount = 0;
//    for (byte i = 17; i < 21; i++){
//      filename[i] = serialNumber[serCount];
//      serCount++;
//    }
//  }
  // Next change the counter on the end of the filename
  // (digits 14+15) to increment count for files generated on
  // the same day. This shouldn't come into play
  // during a normal data run, but can be useful when 
  // troubleshooting.
  for (uint8_t i = 0; i < 100; i++) {
    filename[14] = i / 10 + '0';
    filename[15] = i % 10 + '0';
    
    if (!sd.exists(filename)) {
      // when sd.exists() returns false, this block
      // of code will be executed to open the file
      if (!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END)) {
        // If there is an error opening the file, notify the
        // user. Otherwise, the file is open and ready for writing
        // Turn both indicator LEDs on to indicate a failure
        // to create the log file
//        digitalWrite(ERRLED, !digitalRead(ERRLED)); // Toggle error led 
//        digitalWrite(GREENLED, !digitalRead(GREENLED)); // Toggle indicator led 
//        delay(5);
      }
      break; // Break out of the for loop when the
      // statement if(!logfile.exists())
      // is finally false (i.e. you found a new file name to use).
    } // end of if(!sd.exists())
  } // end of file-naming for loop
  //------------------------------------------------------------
  // Write 1st header line to SD file based on mission info
  myFile.println(F("unixtime,date_time,temp_degC,pH,O2_mgL"));
  // Update the file's creation date, modify date, and access date.
  myFile.timestamp(T_CREATE, time1.year(), time1.month(), time1.day(), 
      time1.hour(), time1.minute(), time1.second());
  myFile.timestamp(T_WRITE, time1.year(), time1.month(), time1.day(), 
      time1.hour(), time1.minute(), time1.second());
  myFile.timestamp(T_ACCESS, time1.year(), time1.month(), time1.day(), 
      time1.hour(), time1.minute(), time1.second());
  myFile.close(); // force the data to be written to the file by closing it
} // end of initFileName function




//------------- writeToSD -----------------------------------------------
// writeToSD function. This formats the available data in the
// data arrays and writes them to the SD card file in a
// comma-separated value format.
void writeToSD (void) {//removed 'void' from (), didn't work
   
  // Flash the green LED every 30 seconds to show data is being collected
//  if (newtime.second() % PULSE_INTERVAL == 0) {
//    digitalWrite(GREENLED,HIGH);
//    delay(5);
//    digitalWrite(GREENLED,LOW);
//  }
  // Reopen logfile. If opening fails, notify the user

//TESTED if removing this loop allows write to SD, it doesn't
  if (!myFile.isOpen()) {
    if (!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END)) {
      Serial.println(F("couldn't find filename"));//digitalWrite(ERRLED, HIGH); // turn on error LED
    }
  }
  
  // Step through each element of the sample data arrays
  // and write them to the SD card
  //for (byte i = 1) { //byte i = 0; i < 1; i++) { //i = 01 was SAMPLES_PER_SECOND in LM code
    // Write the unixtime
    myFile.print(unixtimeArray[1], DEC); // POSIX time value
    myFile.print(F(","));
    printTimeToSD(myFile, unixtimeArray[1]); // human-readable date
    myFile.print(F(","));
    // Print thermocouple value. 
    myFile.print(c); 
    myFile.print(F(","));
    // Print ph sensor value
    myFile.print(phMeas); // 
    myFile.print(F(","));
    // Print DO reading
    myFile.println(doMeas); // printlnmakes new line
  //}
    myFile.close(); // force the buffer to empty
    DateTime t1 = DateTime(unixtimeArray[0]);
    // If the seconds value is 30, update the file modified timestamp
    if (t1.second() % 30 == 0){
      myFile.timestamp(T_WRITE, t1.year(),t1.month(),t1.day(),t1.hour(),t1.minute(),t1.second());
    }
}

////----------------------------------------------------------------------
// printTimeToSD function. This formats the available data in the
// data arrays and writes them to the SD card file in a
// comma-separated value format.
void printTimeToSD (SdFile, DateTime tempTime) { // 'tempTime' is placeholder
    // Write the date and time in a human-readable format
    // to the file on the SD card. 
    myFile.print(tempTime.year(), DEC);
    myFile.print(F("-"));
    if (tempTime.month() < 10) {
      myFile.print("0");
    }
    myFile.print(tempTime.month(), DEC);
    myFile.print(F("-"));
    if (tempTime.day() < 10) {
      myFile.print("0");
    }
    myFile.print(tempTime.day(), DEC);
    myFile.print(F(" "));
    if (tempTime.hour() < 10){
      myFile.print("0");
    }
    myFile.print(tempTime.hour(), DEC);
    myFile.print(F(":"));
    if (tempTime.minute() < 10) {
      myFile.print("0");
    }
    myFile.print(tempTime.minute(), DEC);
    myFile.print(F(":"));
    if (tempTime.second() < 10) {
      myFile.print("0");
    }
    myFile.print(tempTime.second(), DEC);
}

