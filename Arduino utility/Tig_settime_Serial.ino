/*	settimeSerial.ino
	Use this to get your real time clock date and time values
	synchronized closely.
	Copyright Luke Miller 2015
	
	To use this program, upload it to your board, then open the
	serial monitor at 57600 baud. Make sure the serial monitor 
	is set to send a 'newline' when you hit return (see the 
	menu in the lower right of the Arduino serial monitor window)
	
	You will be prompted to enter a date and time in the format:
			YYYY MM DD HH MM SS
	using 24hr time format.
	For example, July 8, 2015 at 3:12:30 PM (afternoon) would 
	be entered as:
	2015 7 8 15 12 30
	
	Type that in to the serial monitor window, then wait to hit
	enter until you reach exactly that time. The new time will be
	uploaded immediately, and the result will be printed to the
	serial monitor. If you miss your time, you can enter a 
	new date and time in the same format and try again without
	needing to reboot. 

*/

#include <Wire.h>
#include <SPI.h>  // not used here, but needed to prevent a RTClib compile error
#include "RTClib_Tig.h" //"RTClib.h" in LM version


// Setup an instance of DS1307 naming it rtc
// You can leave this unchanged when using a DS3231, since they are
// both updated using the same I2C commands and addresses
RTC_DS3231 rtc;     //DS1307

// Declare variables to hold user input
uint16_t myyear;
uint8_t mymonth; 
uint8_t myday; 
uint8_t myhour; 
uint8_t myminute; 
uint8_t mysec;
DateTime myTime;
DateTime currTime; 
long millisVal;

void setup() {
	Serial.begin(57600); // adjust your serial monitor baud to 57600 to match
	while (!Serial) {
		; // wait for serial port to connect. Needed for Leonardo only
	}
	Serial.println(F("Hello"));
	
	Wire.begin();
	rtc.begin();
	
	Serial.println(F("Make sure serial monitor set to 'newline' at bottom right"));
	Serial.println(F("Enter a new date and time in the following format")); 
	Serial.println(F("all on one line: "));
	Serial.println(F("\tYYYY MM DD HH MM SS"));
	Serial.println(F("and hit enter when ready to set time by Windows clock"));
	
	millisVal = millis();
	
}

void loop() {
	if (millis() > millisVal + 1000){
		millisVal = millis(); // update millisVal
		currTime = rtc.now(); // read current time from the rtc
		Serial.print(F("RTC time: "));
		char buf[20]; // create a character array to hold the time as a string
		Serial.print(currTime.year(), DEC);
    Serial.print('/');
    if (currTime.month() < 10) {
      Serial.print(F("0"));
      }
    Serial.print(currTime.month(), DEC);
    Serial.print('/');
    if (currTime.day() < 10) {
      Serial.print(F("0"));
      }
    Serial.print(currTime.day(), DEC);
    Serial.print('\t');
    if (currTime.hour() < 10){
      Serial.print(F("0"));
      }
    Serial.print(currTime.hour(), DEC);
    Serial.print(':');
    if (currTime.minute() < 10) {
      Serial.print("0");
      }
    Serial.print(currTime.minute(), DEC);
    Serial.print(':');
    if (currTime.second() < 10) {
      Serial.print(F("0"));
      }
    Serial.println(currTime.second(), DEC);
  
		//Serial.println(currTime.toString(buf,21)); // print the time as a string
      //previous line shoots error on RTClig_Tig, even when changed .h file...
      //so replaced with more cumbersome commands above to print date/time as string
	}
	// When the user has entered a date and time value in the serial 
	// monitor and hit enter, the following section will execute.
	while (Serial.available() > 0) {
		// Expect the year first
		myyear = Serial.parseInt();
		// Expect month next
		mymonth = Serial.parseInt();
		// Expect day next
		myday = Serial.parseInt();
		// Expect hour next
		myhour = Serial.parseInt();
		// Expect minute next
		myminute = Serial.parseInt();
		// Expect second next
		mysec = Serial.parseInt();
		
		// When the enter symbol '\n' comes along, convert the 
		// values to a DateTime object and set the clock
		if (Serial.read() == '\n'){
			myTime = DateTime(myyear,mymonth,myday,myhour,myminute,mysec);
			Serial.println(F("Setting time"));
			rtc.adjust(myTime);
		}
	}
}
