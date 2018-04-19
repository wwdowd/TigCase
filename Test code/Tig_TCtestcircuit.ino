//Before running, use Tig_settime_serial.ino to set RTC time to accurate Windows time


#include <Wire.h> // built in library, for I2C communications
#include "RTClib_Tig.h" // Date and time functions using a DS3231 RTC connected via I2C
#include <SPI.h> //build in library; SD card communicates through SPI using library SdFat
//#include <EEPROM.h> // WD; built in library, for reading the serial number stored in EEPROM
#include <SdFat.h>
//Thermocouple library for Max31855
#include "Adafruit_MAX31855.h"
//ADC library for ADS1115, 16-bit and 4 channel (we use one here) connected via I2C
//#include <Adafruit_ADS1015.h>
//To create software serial to communicate with the DO probe by RS232
//#include <SoftwareSerial.h>


//#define ECHO_TO_SERIAL // see if prevents restart when plug in serial

/*  GLOBAL VARIABLES  */

RTC_DS3231 rtc; // Real-time clock
SdFat sd; //SD card
SdFile myFile; //file to manipulate/add data to on SD
const int sdCS = 10; //CS is pin 10 for SD handling
//Adafruit_ADS1115 adc; //ADC chip
//int Model;
//int SerialNum;
//float DOmgLSet;
//SoftwareSerial doProbe(9,8); // RX, TX lines on Uno board, Dissolved Oxygen probe
DateTime newtime; // used to track time in main loop
DateTime curr; //used as current time in loop
DateTime tempTime; 

double c = 15; //hold readings from thermocouple
double phMeas = 7.77; //hold readings from pH probe, was pHVolt
double doMeas = 7.77; //hold readings from DO probe, change from DOmgLMeas
const unsigned long tenSec = 10*1000; //keeping track of time
unsigned long lastSampleTime = 0; // so always writes first value

// Declare placeholder name for output files written to SD card
char filename[] = "YYYYMMDD_HHMM_00.csv";

uint32_t unixtimeArray[1]; // store unixtime values temporarily

Adafruit_MAX31855 thermocouple(5, 6, 7);

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
  curr = rtc.now();
  c = thermocouple.readCelsius(); //c = c + 0.1;
  writeToSD();
  printTimeToSerial(curr);
  Serial.print(" ");
  Serial.print(milli);
  Serial.print(" ");
  Serial.println(c);
  delay(2000);
  
   
  phMeas = phMeas + 0.01; 
  doMeas = doMeas + 0.01;
  //myFile.close();
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


