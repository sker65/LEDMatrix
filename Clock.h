/*
 * Clock.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

class LEDMatrixPanel;

class Clock {
public:
	Clock(LEDMatrixPanel& panel, RTC_DS1307& rtc);
	void update(long now);
	virtual ~Clock();
	void on();
	void writeTime(long now, byte* buffer = NULL);
	void off();
	void clear();
	boolean isClockOn() {
		return active;
	}
	void adjust(DateTime& dt);
	DateTime& getActualTime();

protected:

	void writeDigit(int digit, int xoffset, byte* buffer = NULL);

	RTC_DS1307* rtc;
	LEDMatrixPanel& panel;
	long nextClockRefresh;
	long nextRtcSync;
	long lastRtcSync;
	uint8_t brightness;
	DateTime n;
	boolean active;


};

#endif /* CLOCK_H_ */
