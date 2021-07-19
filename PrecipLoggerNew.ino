
// Here we go at logging the rain and switching the lid position.

// libraries
#include <SPI.h>
#include "SD.h"
#include <Wire.h>
#include "RTClib.h"

// Set some constants
#define LOG_INTERVAL  3000 // mills between entries(reduce to take more/faster data)

// compile settings
#define ECHO_TO_SERIAL     1 // echo data to serial port
#define WAIT_TO_START      0 // Wait for serial input in setup()
#define MANUAL_CLOCK_RESET 0 // force reset the clock to the time of compile

///////////////////////////////////////////////////////

int state = 0; // state machine state

const int rainSensorPin = A0;

// H_Bridge pins
const int motorA = 5;
const int motorB = 6;
int doorStatus = 0; // 0 = closed 1 = open
// Next we want to write a few helper functions to make our code easier to read and maintain.
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

// now we're going to set up the data logging shield
RTC_PCF8523 RTC; // define the Real Time Clock object
File logfile; // the logging file

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

void error(const char *str) {
	Serial.print("error: ");
	Serial.println(str);

	// while (1);
}

void setup(void) {
	// lid motor output pin modes
	pinMode(motorA, OUTPUT);
	pinMode(motorB, OUTPUT);

	Serial.begin(115000); // initialize serial communication @ 115000 baud:
	Serial.println();

	#if WAIT_TO_START
		Serial.println("Type any character to start");
		while (!Serial.available());
	#endif // WAIT_TO_START

	// init clock module
	if (!RTC.begin()) {
		logfile.println("RTC failed");
	#if ECHO_TO_SERIAL
		Serial.println("RTC failed");
	#endif // ECHO_TO_SERIAL
	}
	if (! RTC.isrunning()) {
		// following line sets the RTC to the date & time this sketch was compiled
		RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
		#if ECHO_TO_SERIAL
			Serial.println("clock reset");
		#endif
	}
	#if MANUAL_CLOCK_RESET
		RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
	#endif

	// initialize the SD card
	Serial.print("Initializing SD card...");
	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(chipSelect, OUTPUT);
	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		return;
	}
	else {
		Serial.println("card initialized.");
	}

	// create a new file
	char filename[] = "LOGGER00.CSV";
	for (uint8_t i = 0; i < 100; i++) {
		filename[6] = i / 10 + '0';
		filename[7] = i % 10 + '0';
		Serial.println(filename);
		if (! SD.exists(filename)) {
			// only open a new file if it doesn't exist
			logfile = SD.open(filename, FILE_WRITE);
			break;  // leave the loop!
		}
	}

	Serial.print("Logging to: ");
	Serial.println(filename);
	if (! logfile) {
		error("couldnt create file");
	}

	/////////////////////////////////////////////

	//Here we define the header for the .csv file
	logfile.println("datetime,precip,state");
	#if ECHO_TO_SERIAL
		Serial.println("datetime,precip,state");
	#endif
}

/////////////////////////////////

void loop() {
	// delay for the amount of time we want between readings
	delay(LOG_INTERVAL);

	// get timestamp from RTC
	DateTime now = RTC.now();

	int rainSensorVal = analogRead(rainSensorPin); // read the sensor on analog A0

	if(state == 0) { // Waiting for rain
		Serial.println("Waiting for rain");
		stopActuator();
		if(rainSensorVal < 700) { // Rain Detected
			state = 1; // Opening 
		}
	}
	else if (state == 1) { // Opening
		Serial.println("Opening");
		extendActuator();
		delay(5000);
		state = 2; // Stopped open
	}
	else if (state == 2) { // Stopped open
		Serial.println("Stopped open");
		stopActuator();
		if(rainSensorVal > 700) { // Rain no longer detected
			state = 3; // Closing
		}
	}
	else if (state == 3) { // Closing
		Serial.println("Closing");
		retractActuator();
		delay(5000);
		state = 0; // Waiting for Rain
	}
	else {
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
	logfile.print(rainSensorVal);
	logfile.print(", ");
	logfile.println(state);
	logfile.flush();
	#if ECHO_TO_SERIAL
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
		Serial.print(rainSensorVal);
		Serial.print(", ");
		Serial.println(state);
	#endif //ECHO_TO_SERIAL
}
