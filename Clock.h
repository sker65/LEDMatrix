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

protected:

	void writeTime(boolean tick);
	void writeDigit(int digit, int xoffset);

	RTC_DS1307* rtc;
	LEDMatrixPanel& panel;
	long nextClockRefresh;
	long nextRtcSync;


	DateTime n;

};

#endif /* CLOCK_H_ */
