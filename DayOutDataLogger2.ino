/*
 Copyright (c) 2014 Gordon Pimblott

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 ***********************************************************************
 Description and hardware design can be found here:
 
 http://gordonsprojects.blogspot.co.uk/2014/05/arduino-gps-tracker-part-2.html
 
 */
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include <SPI.h>
#include <SD.h>

#include "SimpleTimer.h"

// This is the serial rate for your terminal program. It must be this 
// fast because we need to print everything before a new sentence 
// comes in. If you slow it down, the messages might not be valid and 
// you will likely get checksum errors.
// Set this value equal to the baud rate of your terminal program
static const int RXPin = 3, TXPin = 4;
static const uint32_t TERMBAUD = 115200;
static const uint32_t GPSBaud = 9600;

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int SDled = 6;
const int GPSled = 7;
const int StatusLed = 8;

// State variables
boolean sdCardOk = false;
boolean gpsValid = false;

// Setup the GPS
TinyGPSPlus gps;

// Setup the software serial port
SoftwareSerial uart_gps(RXPin, TXPin);
const int chipSelect = 2; 

// the timer object
SimpleTimer timer;

// The data buffers
char filename[12];
char data[60];

//Initialize GPS variables
double lat, lon, speed, alt, course;

// Local functions
void writeFile();

/**
 * Perform the setup of the SD card and the GPS
 */
void setup()
{
  Serial.begin(TERMBAUD);

  // SPI SS pin must be output for SD card to work
  pinMode(10, OUTPUT);     // SS pin must be output

  // Setup the status LED - light up once we have a signal
  pinMode( SDled , OUTPUT );
  digitalWrite( SDled , LOW );

  pinMode( GPSled , OUTPUT );
  digitalWrite( GPSled , HIGH );

  pinMode( StatusLed , OUTPUT );
  digitalWrite( StatusLed , LOW );


  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("SD NOK (1)");
  } 
  else if (!volume.init(card)) {
    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    Serial.println("SD NOK (2)");
  } 
  else if (!SD.begin(chipSelect)) {
    Serial.println("SD NOK (3)");
  } 
  else {
    sdCardOk = true;
    Serial.println("SD OK");
  }

  // Setup the timer to gather stats and write to SD
  // It doesn't have to be accurate so simpleTimer is fine
  timer.setInterval(5000, writeFile);

  // flash LED 3 times if sd card is ok
  if( sdCardOk ) {
    for (int i=0; i<2; i++){    
      digitalWrite(SDled, HIGH);   // set the LED on
      delay(300);              // wait for a second
      digitalWrite(SDled, LOW);    // set the LED off 
      delay(300); 
    }   
  } 
  else {
    digitalWrite( StatusLed , HIGH );
  }

  // Off we go ;)
  uart_gps.begin(GPSBaud);
}

// This is the main loop of the code. All it does is check for data on 
// the RX pin of the ardiuno, makes sure the data is valid NMEA sentences, 
// then updates the timer
void loop()
{
  while (uart_gps.available() > 0) {
    gps.encode(uart_gps.read());
    }

    timer.run();
}



//
// Write the data to the SD card
//
void writeFile(){

  // If the GPS location is invalid then keep the
  // GPS LED lit
  if( !gps.location.isValid() ) {
    digitalWrite( GPSled , HIGH );
    return;
  }

  // Set the GPS led off (we have a position) 
  // and the SD led on (we are writing)
  digitalWrite( GPSled , LOW );
  digitalWrite( SDled , HIGH );

  // Create the start of the csv string
  sprintf(data, "%d/%d/%d %02d:%02d:%02d, ", 
    gps.date.day(), gps.date.month() , gps.date.year(),
    gps.time.hour(), gps.time.minute(), gps.time.second() );

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  sprintf(filename, "%02d%02d.csv", gps.date.day() , gps.date.month() );   
  File csvFile = SD.open(filename, FILE_WRITE);
  if (csvFile) {
    csvFile.print(data);
    csvFile.print(gps.location.lat() , 6);
    csvFile.print(", ");
    csvFile.print(gps.location.lng() , 6);
    csvFile.print(", ");
    csvFile.print(gps.altitude.meters());
    csvFile.print(", ");
    csvFile.print(gps.speed.mph());
    csvFile.print(", ");
    csvFile.print(gps.course.deg());
    csvFile.print(", ");
    csvFile.print(gps.satellites.value());
    csvFile.println();
    csvFile.close();
  } 
  else {
	// Something has gone wrong light the status LED
    digitalWrite( StatusLed , HIGH );
  }

  // Finished writing so turn off the LED
  digitalWrite( SDled , LOW );
}













