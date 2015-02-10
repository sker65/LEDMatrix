#include <Arduino.h>
#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Animation.h"
#include "Menu.h"

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

Menu menu(&panel, &clock);

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
	//delay(5000);
	panel.clear();
}

extern unsigned int __bss_end;
//extern unsigned int __heap_start;
extern void *__brkval;

int freeMemory() {
	int free_memory;
	if ((int) __brkval == 0)
		free_memory = ((int) &free_memory) - ((int) &__bss_end);
	else
		free_memory = ((int) &free_memory) - ((int) __brkval);
	return free_memory;
}

int clockShowTime = 2000; // millis to show clock
int dateMode = 0;

void reloadConfig() {
	panel.setAnimationColor(menu.getOption(SET_COLOR_ANI));
	panel.setTimeColor(menu.getOption(SET_COLOR_CLOCK));
	panel.setBrightness(menu.getOption(SET_BRIGHTNESS));

	Serial.print("set time mode: ");Serial.println(menu.getOption(SET_TIME_MODE));
	clock.setShowSeconds(menu.getOption(SET_TIME_MODE)==1);
	clockShowTime = 1500 * (menu.getOption(SET_TIME_DURATION)+1);
	dateMode = menu.getOption(SET_DATE_MODE);
}

// The loop function is called in an endless loop
void loop()
{
	long count = 0;
	int led = HIGH;
	long switchToAni = millis() + 2000;
	clock.on();
	byte state = 0;

	Serial.print("free Ram: "); Serial.println(freeMemory());

	reloadConfig();

	long switchToDateTime = millis() + 8000;

	while( true ) {

		long now = millis();

		menu.update(now);

		if( menu.isActive() ) state = 2;

		if( dateMode == 1 && now > switchToDateTime ) {
			boolean isShowingDate = clock.getIsShowingDate();
			clock.setIsShowingDate(!isShowingDate);
			switchToDateTime = now + isShowingDate?8000:2000; // date 2 sec / time 8
		}

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
		case 2:
			if( !menu.isActive()) {
				// menu is finish / clock is set, but reconfige ani / panel
				reloadConfig();
				state = 0;
			}
			break;
		}

//		count++;
//		if( count > 1000) {
//			count = 0;
//			// toggle pin 13 -> blick internal led to show i am alive
//			led = (led==HIGH?LOW:HIGH);
//			digitalWrite(13,led);
//		}
	}
}


