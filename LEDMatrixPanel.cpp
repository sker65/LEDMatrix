/*
 * LEDMatrixPanel.cpp
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#include <Arduino.h>

#include "LEDMatrixPanel.h"

// use this to control the color resolution per pixel per color channel
#define bitsPerPixel 2

// A full PORT register is required for the data lines, though only the
// top 6 output bits are used.  For performance reasons, the port # cannot
// be changed via library calls, only by changing constants in the library.
// For similar reasons, the clock pin is only semi-configurable...it can
// be specified as any pin within a specific PORT register stated below.

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
// Arduino Mega is now tested and confirmed, with the following caveats:
// Because digital pins 2-7 don't map to a contiguous port register,
// the Mega requires connecting the matrix data lines to different pins.
// Digital pins 24-29 are used for the data interface, and 22 & 23 are
// unavailable for other outputs because the software needs to write to
// the full PORTA register for speed.  Clock may be any pin on PORTB --
// on the Mega, this CAN'T be pins 8 or 9 (these are on PORTH), thus the
// wiring will need to be slightly different than the tutorial's
// explanation on the Uno, etc.  Pins 10-13 are all fair game for the
// clock, as are pins 50-53.
// port C is PIN 37 - 30 / PC0 - PC7
#define DATAPORT PORTC
#define DATADIR  DDRC

#define SCLKPORT PORTB

#elif defined(__AVR_ATmega32U4__)
// Arduino Leonardo: this is vestigial code an unlikely to ever be
// finished -- DO NOT USE!!!  Unlike the Uno, digital pins 2-7 do NOT
// map to a contiguous port register, dashing our hopes for compatible
// wiring.  Making this work would require significant changes both to
// the bit-shifting code in the library, and how this board is wired to
// the LED matrix.  Bummer.
#define DATAPORT PORTD
#define DATADIR  DDRD
#define SCLKPORT PORTB
#else
// Ports for "standard" boards (Arduino Uno, Duemilanove, etc.)
#define DATAPORT PORTD
#define DATADIR  DDRD
#define SCLKPORT PORTB
#endif

LEDMatrixPanel::LEDMatrixPanel(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
		uint8_t sclk, uint8_t latch, uint8_t oe, uint8_t width, uint8_t height,
		uint8_t planes, uint8_t colorChannels) {

	// Save pin numbers for use by begin() method later.
	_a = a;
	_b = b;
	_c = c;
	_d = d;
	_sclk = sclk;
	_latch = latch;
	_oe = oe;

	// Look up port registers and pin masks ahead of time,
	// avoids many slow digitalWrite() calls later.
	sclkpin = digitalPinToBitMask(sclk);
	latport = portOutputRegister(digitalPinToPort(latch));
	latpin = digitalPinToBitMask(latch);
	oeport = portOutputRegister(digitalPinToPort(oe));
	oepin = digitalPinToBitMask(oe);

	// address bits for rows
	addraport = portOutputRegister(digitalPinToPort(a));
	addrapin = digitalPinToBitMask(a);
	addrbport = portOutputRegister(digitalPinToPort(b));
	addrbpin = digitalPinToBitMask(b);
	addrcport = portOutputRegister(digitalPinToPort(c));
	addrcpin = digitalPinToBitMask(c);
	// only if four row address bits used
	addrdport = portOutputRegister(digitalPinToPort(d));
	addrdpin = digitalPinToBitMask(d);

	// meta data
	this->width = width;
	this->height = height;
	this->planes = planes;

	this->colorsChannels = colorChannels;

	// byte align width
	buffersize = width / 8 * height;

	nBuffers = bitsPerPixel * colorChannels;

    this->buffptr = (volatile uint8_t**) malloc(sizeof(*buffptr) * nBuffers);
	for (int i = 0; i < nBuffers; i++) {
		buffptr[i] = (volatile uint8_t*) malloc(buffersize);
		memset((void*)buffptr[i], 0xFF, buffersize);
	}

	scanCycle = 0;
	plane = 0;

}

LEDMatrixPanel::~LEDMatrixPanel() {
	// release memory
}

void LEDMatrixPanel::selectPlane() {
	// output new plane address
	if (plane & 0x1)
		*addraport |= addrapin;
	else
		*addraport &= ~addrapin;
	if (plane & 0x2)
		*addrbport |= addrbpin;
	else
		*addrbport &= ~addrbpin;
	if (plane & 0x4)
		*addrcport |= addrcpin;
	else
		*addrcport &= ~addrcpin;
	if (planes > 8) {
		if (plane & 0x8)
			*addrdport |= addrdpin;
		else
			*addrdport &= ~addrdpin;
	}
}

void LEDMatrixPanel::updateScreen() {
	// called periodically to refresh led matrix

	disableLEDs();

	selectPlane();

	// shift out plane data

	// Record current state of SCLKPORT register, as well as a second
	// copy with the clock bit set.  This makes the innnermost data-
	// pushing loops faster, as they can just set the PORT state and
	// not have to load/modify/store bits every single time.  It's a
	// somewhat rude trick that ONLY works because the interrupt
	// handler is set ISR_BLOCK, halting any other interrupts that
	// might otherwise also be twiddling the port at the same time
	// (else this would clobber them).

	uint8_t tick, tock,  *ptr, *ptr1;
	uint8_t p1, p2;

	tock = SCLKPORT;
	tick = tock | sclkpin;

	// erst mal nur den low buffer

	ptr = (uint8_t *) buffptr[0] + plane * width / 4;
	ptr1 = (uint8_t *) buffptr[1] + plane * width / 4;

	for (uint8_t xb = 0; xb < width / 4; xb++) { // weil 2 bits geshifted wird

		p1 = *ptr++;
		for (int j = 0; j < 4; j++) { // nur 4 shifts da pro ausgabe 2 pixel
			DATAPORT = p1 & 0b11000000;
			// shift out
			SCLKPORT = tick;	// Clock lo
			SCLKPORT = tock; // Clock hi
			p1 <<= 2;
		}

	} // bytes to shift

	*latport &= ~latpin;  // Latch down

	*latport |= latpin; // Latch data loaded

	plane++;
	if (plane >= planes) {
		plane = 0;
		scanCycle++;
		if (scanCycle >= 8) {
			scanCycle = 0;
		}
	}

}

void LEDMatrixPanel::begin() {
// Enable all comm & address pins as outputs, set default states:
	pinMode(_sclk, OUTPUT);
	SCLKPORT &= ~sclkpin;  // Low
	pinMode(_latch, OUTPUT);
	*latport &= ~latpin;   // Low
	pinMode(_oe, OUTPUT);
	*oeport |= oepin;     // High (disable output)
	pinMode(_a, OUTPUT);
	*addraport &= ~addrapin; // Low
	pinMode(_b, OUTPUT);
	*addrbport &= ~addrbpin; // Low
	pinMode(_c, OUTPUT);
	*addrcport &= ~addrcpin; // Low
	if (planes > 8) {
		pinMode(_d, OUTPUT);
		*addrdport &= ~addrdpin; // Low
	}

// set data output bit to out
	//DATADIR = !(255 >> ( bitsPerPixel * colorsChannels));
	pinMode(30,OUTPUT);
	pinMode(31,OUTPUT);

// maybe extend to simultaneously drive two colors at once (synchronous)
	DATAPORT = 0b11000000;

}

void LEDMatrixPanel::enableLEDs() {
	*oeport &= ~oepin;   // Re-enable output
}

void LEDMatrixPanel::disableLEDs() {
	*oeport |= oepin;  // Disable LED output during row/plane switchover
}

uint8_t** LEDMatrixPanel::getBuffers() {
	return (uint8_t**)buffptr;
}

// masken zum setzen von pixeln
uint8_t mask [] = {
		0b01111111,
		0b11011111,
		0b11110111,
		0b11111101,

		0b10111111,
		0b11101111,
		0b11111011,
		0b11111110

};

void LEDMatrixPanel::setPixel(uint8_t x, uint8_t y, boolean on) {
	uint8_t bitpos = 0;
	int yoffset = y * (width/4);
	if( y >= height/2) {
		bitpos = 4;
		yoffset = (y - height/2 ) * (width/4);
	}
	uint8_t* ptr =  (uint8_t *) buffptr[0] + yoffset  + x/4;
	uint8_t pix = *ptr;

	if( on ) {
		*ptr = pix & mask[(x&3)+bitpos];
	} else {
		*ptr = pix | ~ mask[(x&3)+bitpos];
	}

}
