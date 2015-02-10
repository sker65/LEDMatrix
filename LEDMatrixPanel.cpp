/*
 * LEDMatrixPanel.cpp
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#include <Arduino.h>

#include "LEDMatrixPanel.h"
#include <avr/pgmspace.h>

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

static LEDMatrixPanel *activePanel = NULL;

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

	nBuffers = bitsPerPixel * colorChannels + 1;

    this->buffptr = (volatile uint8_t**) malloc(sizeof(*buffptr) * nBuffers);
	for (int i = 0; i < nBuffers; i++) {
		buffptr[i] = (volatile uint8_t*) malloc(buffersize);
		memset((void*)buffptr[i], 0xFF, buffersize);
	}
	plane = 0;
	actBuffer = 0;
	timeColor = 0;
	aniColor = 0;
	timeBright = 0;
	brightness=0;

	// init text
	col = row = 0;
	// clear text buffer
	for(int c = 0; c <16; c++ ) {
		for(int r=0; r<4;r++) {
			textbuffer[r][c] = ' ';
		}
	}
}

LEDMatrixPanel::~LEDMatrixPanel() {
	// release memory
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
	// green
	pinMode(32,OUTPUT);
	pinMode(33,OUTPUT);

// maybe extend to simultaneously drive two colors at once (synchronous)
	DATAPORT = 0b11110000;

	activePanel = this;

	 // Set up Timer1 for interrupt:
	TCCR1A = _BV(WGM11); // Mode 14 (fast PWM), OC1A off
	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Mode 14, no prescale
	ICR1 = 100;
	TIMSK1 |= _BV(TOIE1); // Enable Timer1 interrupt
	sei();
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
	uint16_t duration = 800; // overall brightness

//	if( blank ) {
//		blank = !blank;
//		ICR1 = duration; // Set interval for next interrupt
//		TCNT1 = 0; // Restart interrupt timer
//		return;
//	}
//	blank = true;

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

	uint8_t tick, tock,  *ptr;
	uint8_t p1;

	tock = SCLKPORT;
	tick = tock | sclkpin;

	ptr = (uint8_t *) buffptr[actBuffer] + plane * width / 4;

	uint8_t col = 0;

	if (actBuffer == 2) { // clock plane
		col = timeColor;
	} else {
		col = aniColor;
	}

	for (uint8_t xb = 0; xb < width / 4; xb++) { // weil 2 bits geshifted wird

		p1 = *ptr++;
		for (int j = 0; j < 4; j++) { // nur 4 shifts da pro ausgabe 2 pixel

			if( col == 1) {
				// green
				DATAPORT = (p1 & 0b11000000)>>2 | 0b11000000;
			} else if( col == 0) {
				// red
				DATAPORT = (p1 & 0b11000000) | 0b00110000;
			} else {
				DATAPORT = (p1 & 0b11000000) | ((p1 & 0b11000000)>>2);
			}
			// shift out
			SCLKPORT = tick;	// Clock lo
			SCLKPORT = tock; // Clock hi
			p1 <<= 2;
		}

	} // bytes to shift

	*latport &= ~latpin;  // Latch down
	*latport |= latpin; // Latch data loaded

	// todo brightness
	duration = 200+brightness*200; // was 800
	if( actBuffer==1 ) duration +=2300;
	if( actBuffer==2 ) duration +=timeBright*800;

	plane++;
	if (plane >= planes) {
		plane = 0;
		actBuffer++;
		if (actBuffer >= nBuffers) {
			actBuffer = 0;
		}
	}
	enableLEDs();

	ICR1 = duration; // Set interval for next interrupt
	TCNT1 = 0; // Restart interrupt timer

}

// interrupt handling
ISR(TIMER1_OVF_vect, ISR_BLOCK) { // ISR_BLOCK important -- see notes later
	activePanel->updateScreen(); // Call refresh func for active display
	TIFR1 |= TOV1; // Clear Timer1 interrupt flag
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

	uint8_t* ptr =  (uint8_t *) buffptr[1] + yoffset  + x/4;
	uint8_t pix = *ptr;

	if( on ) {
		*ptr = pix & mask[(x&3)+bitpos];
	} else {
		*ptr = pix | ~ mask[(x&3)+bitpos];
	}

}

void LEDMatrixPanel::clear() {
	for( int i = 0; i<buffersize; i++) {
		buffptr[0][i]=0xff;
		buffptr[1][i]=0xff;
	}
}

// font mit 96 ascii zeichen ab 32 - 128
// PROGMEM sorgt dafür dass die konstanten Pixel Daten im Flash landen
// und nicht im RAM!!
PROGMEM const char font8[96][8] = {
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
{ 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // U+0021 (!)
{ 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0022 (")
{ 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // U+0023 (#)
{ 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // U+0024 ($)
{ 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // U+0025 (%)
{ 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // U+0026 (&)
{ 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
{ 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // U+0028 (()
{ 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // U+0029 ())
{ 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // U+002A (*)
{ 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // U+002B (+)
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+002C (,)
{ 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // U+002D (-)
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+002E (.)
{ 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
{ 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // U+0030 (0)
{ 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // U+0031 (1)
{ 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // U+0032 (2)
{ 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // U+0033 (3)
{ 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // U+0034 (4)
{ 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // U+0035 (5)
{ 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // U+0036 (6)
{ 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // U+0037 (7)
{ 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // U+0038 (8)
{ 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // U+0039 (9)
{ 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+003A (:)
{ 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+003B (//)
{ 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // U+003C (<)
{ 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // U+003D (=)
{ 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // U+003E (>)
{ 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // U+003F (?)
{ 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // U+0040 (@)
{ 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // U+0041 (A)
{ 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // U+0042 (B)
{ 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // U+0043 (C)
{ 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // U+0044 (D)
{ 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // U+0045 (E)
{ 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // U+0046 (F)
{ 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // U+0047 (G)
{ 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // U+0048 (H)
{ 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0049 (I)
{ 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // U+004A (J)
{ 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // U+004B (K)
{ 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // U+004C (L)
{ 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // U+004D (M)
{ 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // U+004E (N)
{ 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // U+004F (O)
{ 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // U+0050 (P)
{ 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // U+0051 (Q)
{ 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // U+0052 (R)
{ 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // U+0053 (S)
{ 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0054 (T)
{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U+0055 (U)
{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0056 (V)
{ 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // U+0057 (W)
{ 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // U+0058 (X)
{ 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // U+0059 (Y)
{ 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // U+005A (Z)
{ 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // U+005B ([)
{ 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // U+005C (\)
{ 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // U+005D (])
{ 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // U+005E (^)
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // U+005F (_)
{ 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0060 (`)
{ 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // U+0061 (a)
{ 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // U+0062 (b)
{ 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // U+0063 (c)
{ 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // U+0064 (d)
{ 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // U+0065 (e)
{ 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // U+0066 (f)
{ 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0067 (g)
{ 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // U+0068 (h)
{ 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0069 (i)
{ 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // U+006A (j)
{ 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // U+006B (k)
{ 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+006C (l)
{ 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // U+006D (m)
{ 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // U+006E (n)
{ 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // U+006F (o)
{ 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // U+0070 (p)
{ 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // U+0071 (q)
{ 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // U+0072 (r)
{ 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // U+0073 (s)
{ 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // U+0074 (t)
{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // U+0075 (u)
{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0076 (v)
{ 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // U+0077 (w)
{ 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // U+0078 (x)
{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0079 (y)
{ 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // U+007A (z)
{ 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // U+007B ({)
{ 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // U+007C (|)
{ 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // U+007D (})
{ 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+007E (~)
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // U+007F
};

void LEDMatrixPanel::println(const char* text) {
	char ch;
	if( row == 4) {
		scrollUp();
		row=3;
	}
	for(int c = 0; c<16; c++) {
		ch = text[c];
		if( ch == '\0' ) break;
		textbuffer[row][c] = ch;
	}
	writeText(textbuffer[row],0,row*8,16);
	row++;
}

void LEDMatrixPanel::scrollUp() {
	// copy up
	for(int r=1; r<4; r++) {
		for(int c = 0; c<16; c++) {
			textbuffer[r-1][c] = textbuffer[r][c];
		}
	}
	// clear last line
	for(int c = 0; c<16; c++) {
		textbuffer[3][c] = ' ';
	}
	clear();
	// redraw first three
	for(int r=0; r<3; r++) {
		writeText(textbuffer[r],0,r*8,16);
	}

}


void LEDMatrixPanel::writeText(const char* text, uint8_t x, uint8_t y, int len) {
	const char* p = text;
	int j = 0;
	while( *p != 0 && j<len) {
		int ch = *p++;
		if( ch >= 32 ) {
			ch -= 32;
			for(int i = 0; i<8; i++) {
				for( int k = 0; k <8; k++) {
					setPixel(x+k+j*8,y+i,
							// use pgm_read_byte to read from flash
							pgm_read_byte(&(font8[ch][i]))
							& (1<<k));
				}
			}
		}
		j++;
	}
}

void LEDMatrixPanel::setTimeColor(uint8_t col) {
	this->timeColor = col;
}

void LEDMatrixPanel::setAnimationColor(uint8_t col) {
	this->aniColor = col;
}
