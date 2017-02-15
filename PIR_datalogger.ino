//****************************************************************

// OSBSS PIR motion sensor datalogger - v0.01
// Last edited on March 30, 2015

//****************************************************************

#include <PowerSaver.h>
#include <DS3234lib3.h>
#include <SdFat.h>
#include <EEPROM.h>

// Global objects and variables   ******************************
char filename[15] = "log.csv";    // Filename. Format: "12345678.123". Cannot be more than 8 characters in length, contain spaces or start with a number

PowerSaver chip; // declare object for PowerSaver class
DS3234 RTC;      // declare object for DS3234 class
SdFat sd;        // declare object for SdFat class
SdFile myFile;     // declare object for SdFile class

#define POWER 4        // pin 4 supplies power to microSD card breakout
#define RTCPOWER 6     // pin 6 supplies power to DS3234 RTC breakout
#define LED 7          // pin 7 controls LED
int chipSelect = 9;       // pin 9 is micrSD card breakout CS pin
int state = 0;         // this variable stores the state of PIR sensor's output (either 1 or 0)

// ISR ****************************************************************
ISR(PCINT0_vect)  // Interrupt Service Routine for PCINT0 vector (pin 8)
{
  asm("nop");  // do nothing
}

// Main code ****************************************************************
void setup()
{
  Serial.begin(19200); // open serial at 19200 bps
  Serial.println("setup");
  delay(10);
  
  pinMode(POWER, OUTPUT); // set output pins
  pinMode(RTCPOWER, OUTPUT);
  pinMode(LED, OUTPUT);
  
  digitalWrite(POWER, HIGH);     // turn on SD card
  digitalWrite(RTCPOWER, HIGH);     // turn on RTC
  delay(1);                      // give some delay to ensure SD card is turned on properly
  
  if(!sd.begin(chipSelect, SPI_HALF_SPEED))  // initialize microSD card
  {
    SDcardError(3);
  }
  else
  {
    // open the file for write at end like the Native SD library
    if(!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END))
    {
      SDcardError(2);
    }

    else
    {    
      myFile.println();
      myFile.println("Date/Time,Occupancy");
      myFile.println();
      myFile.close();
      
      digitalWrite(LED, HIGH);
      delay(10);
      digitalWrite(LED, LOW);
    }
  }
  chip.sleepInterruptSetup();    // setup sleep function & pin change interrupts on the ATmega328p. Power-down mode is used here
}

void loop()
{
  digitalWrite(POWER, LOW);  // turn off SD card
  digitalWrite(RTCPOWER, LOW);     // turn off RTC
  delay(1);
  
  chip.turnOffADC();  // turn off ADC to save power
  chip.turnOffSPI();  // turn off SPI bus to save power
  chip.turnOffWDT();  // turn off WatchDog timer. This doesn't work for Pro Mini (Rev 11); only works for Arduino Uno
  chip.turnOffBOD();

  chip.goodNight();    // put the processor in power-down mode
  
  // code will resume from here once the processor wakes up ============== //
  chip.turnOnADC();   // turn on ADC once the processor wakes up
  chip.turnOnSPI();   // turn on SPI bus once the processor wakes up
  delay(1);    // important delay to ensure SPI bus is properly activated
  
  digitalWrite(POWER, HIGH);  // turn on SD card
  digitalWrite(RTCPOWER, HIGH);     // turn on RTC
  delay(1);    // give some delay to ensure SD card is turned on properly
  
  RTC.checkDST(); // check and account for Daylight Savings Time in US
  
  printToSD(); // print data to SD card
}

void printToSD()
{
  String time = RTC.timeStamp();    // get date and time from RTC
  Serial.println(time);
  delay(10);
  
  if(!sd.begin(chipSelect, SPI_HALF_SPEED))
  {
    SDcardError(3);
  }
  else
  {
    if(!myFile.open(filename, O_RDWR | O_CREAT | O_AT_END))
    {
      SDcardError(2);
    }
  
    else
    {
      state = digitalRead(8); // read current state of PIR. 1 = movement detected, 0 = no movement.
      
      if(state == 1)
      {
        myFile.print(time);
        myFile.print(",");
        myFile.print("0");
        myFile.println();
        myFile.print(time);
        myFile.print(",");
        myFile.print("1");
      }
      else if(state == 0)
      {
        myFile.print(time);
        myFile.print(",");
        myFile.print("1");
        myFile.println();
        myFile.print(time);
        myFile.print(",");
        myFile.print("0");
      }
      myFile.println();
      myFile.close();    // close file - very important
                       // give some delay by blinking status LED to wait for the file to properly close
      digitalWrite(LED, HIGH);
      delay(10);
      digitalWrite(LED, LOW);
    }
  }
}

// SD card Error response  ****************************************************************
void SDcardError(int n)
{
    for(int i=0;i<3;i++)   // blink LED 3 times to indicate SD card write error; 2 to indicate file read error
    {
      digitalWrite(LED, HIGH);
      delay(50);
      digitalWrite(LED, LOW);
      delay(150);
    }
}

//****************************************************************
