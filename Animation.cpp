/*
 * Animation.cpp
 *
 *  Created on: 01.02.2015
 *      Author: sr
 */

#include "Animation.h"


Animation::Animation(SdFat& sd, LEDMatrixPanel& panel, Clock& clock) :
sd(sd), panel(panel), clock(clock) {
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

void readString(File& f) {
	char name[40];
	int len = f.read()*256+f.read();
	for(int i=0;i<40 && i < len;i++) {
		name[i] = f.read();
	}
	name[len] = '\0';
	Serial.print("name: ");Serial.println(name);
}

void Animation::readNextAnimation() {
	actFilePos = ani.position();
	readString(ani);
	// cycles 2
	cycles = ani.read()*256+ani.read();
	Serial.print("cycles: ");Serial.println(cycles);
	// hold cycles 2
	holdCycles = ani.read()*256+ani.read();
	Serial.print("hold: ");Serial.println(hold);

	// clock from 2
	clockFrom = ani.read()*256+ani.read();
	Serial.print("clockFrom: ");Serial.println(clockFrom);
	ani.seekCur(5);
	// clock small (1 Byte)
	// clock x / y offset je 2
	// refresh delay 2
	refreshDelay = ani.read()*256+ani.read();
	Serial.print("refreshDelay: ");Serial.println(refreshDelay);
	// type (1 Byte)
	ani.seekCur(1);
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
		int r = ani.readBytes(panel.getBuffers()[0], buflen);
		r += ani.readBytes(panel.getBuffers()[1], buflen);
		//Serial.print("reading 2 buffers len: ");Serial.print(buflen);Serial.println(r);
//		Serial.print("reading frame: ");Serial.println(actFrame);
	}
	actFrame++;
}

boolean Animation::update(long now) {
	if( now> nextAnimationUpdate) {
		if( hold ) {
			hold = false;
			panel.clear();
			clock.on();
			return true;
		}
		nextAnimationUpdate = now + refreshDelay;
		if( actFrame==0 ) {
			if( actAnimation >= numberOfAnimations ) {
				ani.seek(8); // reset
				actAnimation = 0;
			}
			readNextAnimation();
		}

		readNextFrame();
		if( actFrame >= clockFrom ) {
			clock.on();
			clock.update(now);
			// write with mask / mot only with update
		}
		// todo raus ziehen in den Handler
		if( actFrame >= numberOfFrames ) { // ende der ani, check auf cycle
//			if(--cycles>0) {
//				ani.seek(actFilePos);
//				actFrame = 0;
//				actAnimation--;
//			} else {
				// gehe in den Hold status
				actFrame = 0;
				// TODO es darf nicht über die lange Zeit gewartet werden
				// sondern es werden die cycles gezählt, damit die Uhr
				// einen update bekommt
				nextAnimationUpdate = now + 200*holdCycles; // hold
				hold = true;
//			}
		}
	}
	return false;
}

Animation::~Animation() {
}

