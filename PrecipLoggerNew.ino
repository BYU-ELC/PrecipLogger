
//Here we go at logging the rain and lid position data to the logger.

// Call a few libraries
#include <SPI.h>
#include "SD.h"
#include <Wire.h>
#include "RTClib.h"

//Set some constants
// A simple data logger for the Arduino analog pins
#define LOG_INTERVAL  3000 // mills between entries(reduce to take more/faster data)

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 3000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()

///////////////////////////////////////////////////////
int ledPin  = 13;
// lowest and highest sensor readings:
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum
//H_Bridge
int motorA = 5;
int motorB = 6;
int doorStatus = 0; //0 = closed 1 = open
//Next we want to write a few helper functions to make our code easier to read and maintain.
void extendActuator() {
  digitalWrite(motorA, HIGH);
  digitalWrite(motorB, LOW);
}

void retractActuator() {
  digitalWrite(motorA, LOW);
  digitalWrite(motorB, HIGH);
}

void stopActuator() {
  digitalWrite(motorA, LOW);
  digitalWrite(motorB, LOW);
}

///////////////////////////////////////////////////
//now we're going to define the pin for the SD card and catch any errors
//in loading the card.
RTC_PCF8523 RTC; // define the Real Time Clock object


// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void error(const char *str)
{
  Serial.print("error: ");
  Serial.println(str);


  //while (1);
}

void setup(void) {

  pinMode(ledPin,    OUTPUT);// Set up pin 13 as an output.

  pinMode(motorA, OUTPUT);
  pinMode(motorB, OUTPUT);

  Serial.begin(115000);  // initialize serial communication @ 115000 baud:
  Serial.println();

#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // Wire.begin(); // why is this here?
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  if (! RTC.isrunning()) {
    // following line sets the RTC to the date & time this sketch was compiled
    Serial.println("reseting clock");
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("clock reset");
  }

//  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));


  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    Serial.println(filename);
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      Serial.println("Got here");
      break;  // leave the loop!
    }
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  if (! logfile) {
    error("couldnt create file");
  }
  Serial.print("After error handling file");


  
  /////////////////////////////////////////////


  //Here we define the header for the .csv file
  logfile.println("datetime,precip,state");
  Serial.print("After define the header");
#if ECHO_TO_SERIAL
  Serial.println("datetime,precip,state");
#endif

  ////////////////////////////////////////////////
  //I commented this code because the program would not compile. Now it does.

  // #if ECHO_TO_SERIAL// attempt to write out the header to the file
  // if (logfile.writeError || !logfile.sync()) {
  //   error("write header");
  // }
  // #endif
  

  // If you want to set the aref to something other than 5v
  //analogReference(EXTERNAL);
}

/////////////////////////////////

int state = 0;

//////////////


void loop() {

/*
extendActuator();
delay(3500);
stopActuator();
delay(3500);
retractActuator();
delay(3500);
*/





  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));


  //  // log milliseconds since starting
  //  uint32_t m = millis();
  //  logfile.print(m);           // milliseconds since start
  //  logfile.print(", ");
  //#if ECHO_TO_SERIAL
  //  Serial.print(m);         // milliseconds since start
  //  Serial.print(", ");
  //#endif
  // fetch the time

  //Timestamping
  DateTime now = RTC.now();
  // log time
  //  logfile.print(now.unixtime()); // seconds since 2000
  //  logfile.print(", ");


  int sensorReadingA = analogRead(A0); // read the sensor on analog A0:

 

if(state == 0) // Waiting for rain
{
  Serial.println("Waiting for rain");
  stopActuator();
  if(sensorReadingA < 700) // Rain Detected
  {
   state = 1; // Opening 
  }
}
else if (state == 1) // Opening
{
  Serial.println("Opening");
  extendActuator();
  delay(5000);
  state = 2; // Stopped
}
else if (state == 2) // Stopped
{
  Serial.println("Stopped");
  stopActuator();
  if(sensorReadingA > 700) // Rain Not Detected
  {
    
   state = 3; // Closing  
  } 

  Serial.println(sensorReadingA);
}
else if (state == 3) // Closing
{
  Serial.println("Closing");
  retractActuator();
  delay(5000);
  state = 0; // Waiting for Rain
}
else
{
 state = 0; // Default 
}







  
  
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(", ");
  logfile.print(sensorReadingA);
  logfile.print(", ");
  logfile.println(state);
  logfile.flush();
#if ECHO_TO_SERIAL
  //  Serial.print(now.unixtime()); // seconds since 2000
  //  Serial.print(", ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(", ");
  Serial.print(sensorReadingA);
  Serial.print(", ");
  Serial.println(state);
#endif //ECHO_TO_SERIAL



}
