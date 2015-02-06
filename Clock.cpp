/*
 * Clock.cpp
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#include "Clock.h"

TimeSpan onesec(1);
#include "LEDMatrixPanel.h"

#define maxBrightness 2

Clock::Clock(LEDMatrixPanel& p, RTC_DS1307& rtc)
: panel(p), nextClockRefresh(0), nextRtcSync(0) {
	this->rtc = &rtc;
	brightness = 0;
	active=false;
}

Clock::~Clock() {
}

/**
 * updates the clock cyclic
 * @param now actual time in millis
 */
void Clock::update(long now) {
	if( nextRtcSync < now ) {
		nextRtcSync = now + 30L*60L*1000L;  // 30 min
		n = rtc->now();
	}
	if( nextClockRefresh < now ) {
		nextClockRefresh = now + 500;
		boolean tick = (now%1000) > 500;
		//debug panel.setPixel(1,0,true);
		writeTime(now);
		if( tick ) {
			n = n + onesec;
		}
		// by forcing updates max bright is reached faster
		if( brightness < maxBrightness ) {
			brightness++;
			nextClockRefresh = now + 200;
			panel.setTimeBrightness(brightness);
		}
	}
}

void Clock::writeDigit(int digit, int xoffset, byte* buffer) {
	if( buffer == NULL ) {
		byte* src = digits[digit];
		for( int row = 0; row < 16; row++ ) {
			uint8_t* ptr = panel.getBuffers()[2] + row * panel.getWidth()/4 +xoffset;
			for( int p=0;p<4;p++) {
		    	*ptr++ = *src++;
		    }
		}
	} else {
		byte* src = mask[digit];
		for (int row = 0; row < 16; row++) {
			uint8_t* p1 = buffer + row * panel.getWidth() / 4
					+ xoffset;
			for (int p = 0; p < 4; p++) {
			// unroll
				byte v = *p1;
				*p1++ = v | ~ (*src++);
			}
		}
	}
}


void Clock::writeTime(long now,  byte* buffer) {
	if(active) {
		boolean tick = (now%1000) > 500;
		int x = 5;
		if( n.hour()>10 ) {
			writeDigit(n.hour()/10,x,buffer);
		}
		x+=4;
		writeDigit(n.hour()%10,x,buffer);
		x+=4;
		writeDigit(tick?10:11,x,buffer);
		x+=4;
		if( n.minute()>=10 ) {
			writeDigit(n.minute()/10,x,buffer);
		} else {
			writeDigit(0,x,buffer);
		}
		x+=4;
		writeDigit(n.minute()%10,x,buffer);
	}
}

void Clock::clear() {
	byte* p = panel.getBuffers()[2];
	byte* pend = p + panel.getSizeOfBufferInByte();
	while(p<pend) {
		*p++ = 0xFF;
	}
}

void Clock::off() {
	if( active ) {
		active = false;
		panel.setTimeBrightness(0);
		clear();
	}
}

void Clock::on() {
	if( !active ) {
		active = true;
		brightness = 0;
		update(millis());
	}
}

