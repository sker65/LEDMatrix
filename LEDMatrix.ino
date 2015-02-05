#include <Arduino.h>
#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Animation.h"

#include <SdFat.h>

#define CLK 11 // 8 uno MUST be on PORTB! (Use pin 11 on Mega)
#define OE 9
#define LAT 12
#define A A0
#define B A1
#define C A2
#define D A3
#define WIDTH 128
#define HEIGHT 32
#define PLANES 16
#define COLORCHANNELS 1

LEDMatrixPanel panel( A,B,C,D, CLK, LAT, OE, WIDTH, HEIGHT, PLANES, COLORCHANNELS);

RTC_DS1307 rtc;
Clock clock(panel, rtc);

SdFat SD;
Animation animation(SD, panel, clock);

#define SD_CS_PIN 53
#define VERSION "go dmd v1.0"

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(57600);
	Serial.println("calling panel begin");
	panel.begin();
	int z = 0;
	pinMode(13,OUTPUT);
	panel.writeText(VERSION,0,0 * 8);
	Serial.println("calling wire begin");
//	panel.writeText("starting wire",0,z++ * 8);
	Wire.begin();
	Serial.println("calling rtc begin");
	panel.writeText("rtc ..",0,1 * 8);
	rtc.begin();

	pinMode(SD_CS_PIN, OUTPUT);
	Serial.println("calling sd card begin");
	panel.writeText("sd card ..",0,2 * 8);

	if( !SD.begin(SD_CS_PIN,SPI_FULL_SPEED) ){
		Serial.println("sd card begin failed");
		panel.writeText("sd card failed",0,3 * 8);
		delay(12000);
	} else {
		Serial.println("sd card begin success");
		delay(500);
		panel.writeText("boot ok (c) 2015",0,2 * 8);
		panel.writeText("        by Steve",0,3 * 8);
	}

	animation.begin();
	delay(5000);
	panel.clear();
}

// The loop function is called in an endless loop
void loop()
{
	long count = 0;
	int led = HIGH;
	long switchToAni = millis() + 2000;
	while( true ) {

		long now = millis();

		if( now < switchToAni ) {
			clock.update(now);
		} else {
			clock.off();
			if( animation.update(now) ) { // true means ani finished
				switchToAni = now + 2000; // millis to show clock
			}
		}

		count++;
		if( count > 100) {

//			panel.setPixel(random(WIDTH),random(HEIGHT),random(2)==1);

			count = 0;
			// toggle 13
			led = (led==HIGH?LOW:HIGH);
			digitalWrite(13,led);
		}
	}
}

//	File root = SD.open("/");
//
//	printDirectory(root, 0);
//
//	SdVolume* vol = SD.vol();
//	Serial.print(vol->freeClusterCount()); Serial.println(" clusters free.");

//
//	  //rtc.adjust(DateTime(2015, 1, 29, 0, 28, 50));
//	  if (! rtc.isrunning()) {
//	    Serial.println("RTC is NOT running!");
//	    // following line sets the RTC to the date & time this sketch was compiled
//	    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//	    // This line sets the RTC with an explicit date & time, for example to set
//	    // January 21, 2014 at 3am you would call:
//	    //rtc.adjust(DateTime(2015, 1, 29, 0, 14, 0));
//	  }

#if false
void printDirectory(File dir, int numTabs) {
   while(true) {

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
#endif

