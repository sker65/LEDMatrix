/*
 * Clock.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <Arduino.h>

class Clock {
public:
	Clock();
	void render(uint8_t* buffer, char* msg);
	virtual ~Clock();
protected:

};

#endif /* CLOCK_H_ */
