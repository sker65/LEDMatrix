/*
 * Animation.h
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#ifndef ANIMATION_H_
#define ANIMATION_H_
#include "LEDMatrixPanel.h"
#include <SdFat.h>

class Animation {
public:
	Animation(SdFat& sd, LEDMatrixPanel& panel);
	virtual ~Animation();
	void readNextAnimation();
	void readNextFrame();
	boolean update(long now);
	void begin();

protected:
	SdFat& sd;
	File ani;
	LEDMatrixPanel& panel;
	uint16_t numberOfAnimations;
	uint16_t numberOfFrames;

	uint16_t actAnimation;
	uint16_t actFrame;

	long nextAnimationUpdate;
	boolean hold = false;
};

#endif /* ANIMATION_H_ */
