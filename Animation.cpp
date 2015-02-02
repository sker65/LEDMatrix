/*
 * Animation.cpp
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#include "Animation.h"


Animation::Animation(SdFat& sd, LEDMatrixPanel& panel) : sd(sd), panel(panel) {
	actAnimation = 0;
	actFrame = 0;
	nextAnimationUpdate=0;
	numberOfAnimations=0;
	numberOfFrames=0;
}

void Animation::begin() {
	ani = sd.open("/FOO.ANI");
	// read 4 header ANIM
	ani.seekCur(4);
	// read 2 byte version (1)
	uint16_t version = ani.read()*256+ani.read();
	Serial.print("version: "); Serial.println(version);
	// read number of anis
	numberOfAnimations  = ani.read()*256+ani.read();
	Serial.print("number of animations: "); Serial.println(numberOfAnimations);
	// pos is now 8
}

void Animation::readNextAnimation() {
	ani.seekCur(14);
	// cycles 2
	// hold cycles 2
	// clock from 2
	// clock small (1 Byte)
	// clock x / y offset je 2
	// refresh delay 2
	// type (1 Byte)
	// 14 Byte insgesamt
	// frameset count
	numberOfFrames = ani.read()*256+ani.read();
	Serial.print("number of frames: ");Serial.println(numberOfFrames);
	actFrame = 0;
	actAnimation++;
}

void Animation::readNextFrame() {
	uint16_t buflen = ani.read()*256+ani.read();
	if( panel.getSizeOfBufferInByte() == buflen ) {
		//byte buf[512];
		int r = ani.readBytes(panel.getBuffers()[0], buflen);
		r += ani.readBytes(panel.getBuffers()[1], buflen);
		//panel.getBuffers()[0][0] = 0;
		//panel.getBuffers()[1][0] = 0xF0;
		//Serial.print("reading 2 buffers len: ");Serial.print(buflen);Serial.println(r);
		Serial.print("reading frame: ");Serial.println(actFrame);
	}
	actFrame++;
}

boolean Animation::update(long now) {
	if( now> nextAnimationUpdate) {
		if( hold ) {
			hold = false;
			return true;
		}
		nextAnimationUpdate = now + 100; // refresh delay
		if( actFrame==0 ) {
			if( actAnimation >= numberOfAnimations ) {
				ani.seek(8); // reset
				actAnimation = 0;
			}
			readNextAnimation();
		}
		readNextFrame();
		if( actFrame >= numberOfFrames ) {
			actFrame = 0;
			nextAnimationUpdate = now + 600; // hold
			hold = true;
		}
	}
	return false;
}

Animation::~Animation() {
}

