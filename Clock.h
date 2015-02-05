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
	void writeTime(long now);
	void off();
	void clear();
	boolean isClockOn() { return active;}

protected:

	void writeDigit(int digit, int xoffset);

	RTC_DS1307* rtc;
	LEDMatrixPanel& panel;
	long nextClockRefresh;
	long nextRtcSync;
	int glowCount;
	DateTime n;
	boolean active;

};

#endif /* CLOCK_H_ */
