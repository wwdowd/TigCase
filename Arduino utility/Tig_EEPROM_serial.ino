#include <EEPROM.h>
//from http://www.arduino.cc/playground/Code/EEPROMWriteAnything
#include <EEPROMAnything.h>


// Enter serialNumber to upload here
char serialNumber[] = "Tig08";

int pos = 0; //start at first spot in EEPROM memory

void setup()
{
    Serial.begin(57600);
    
    EEPROM_writeAnything(pos, serialNumber);
}

void loop()
{
  char *readvalue = EEPROM.get(0,serialNumber);
  if (readvalue[0] == 'T') {
    
    Serial.print(F("Read serial number: "));
    Serial.println(readvalue);
  } else {
    Serial.print(F("No valid serial number: "));
    Serial.println(readvalue);
    
  }
}
