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
	pinMode(13,OUTPUT);
	panel.println(VERSION);
	Serial.println("calling wire begin");
	panel.println("start wire ..");
	Wire.begin();
	Serial.println("calling rtc begin");
	panel.println("start rtc ..");
	rtc.begin();

	pinMode(SD_CS_PIN, OUTPUT);
	Serial.println("calling sd card begin");
	panel.println("start sd card ..");

	if( !SD.begin(SD_CS_PIN,SPI_FULL_SPEED) ){
		Serial.println("sd card begin failed");
		panel.println("sd card failed");
		delay(12000);
	} else {
		Serial.println("sd card begin success");
		delay(500);
		panel.println("boot ok!");
		panel.println("(c)2015 by Steve");
	}

	if( !animation.begin() ) {
		panel.println("init anis failed");
		panel.println("*.ani not found");
	}
	delay(5000);
	panel.clear();
}

int clockShowTime = 2000; // millis to show clock

// The loop function is called in an endless loop
void loop()
{
	long count = 0;
	int led = HIGH;
	long switchToAni = millis() + 2000;
	clock.on();
	byte state = 0;
	while( true ) {

		long now = millis();

		switch( state ) {
		case 0:
			if( now > switchToAni ) {
				state = 1;
				clock.off();
			}
			clock.update(now);
			break;
		case 1:
			if( animation.update(now) ) { // true means ani finished
				switchToAni = now + clockShowTime; // millis to show clock
				clock.on();
				state = 0;
			}
			break;
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

