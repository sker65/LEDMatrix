/*
 * LEDMatrixPanel.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef LEDMATRIXPANEL_H_
#define LEDMATRIXPANEL_H_
#include <Arduino.h>
/**
 * class to drive led matric panel (china stuff) via HUB08 etc. code is partly copied from Adafruit RGBMatrixPanel
 * see https://github.com/adafruit/RGB-matrix-Panel
 * @author Stefan Rinke (C) 2015
 */
class LEDMatrixPanel {
public:
	LEDMatrixPanel(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
		    uint8_t sclk, uint8_t latch, uint8_t oe, uint8_t width, uint8_t height, uint8_t planes, uint8_t colorChannels);

	virtual ~LEDMatrixPanel();

	uint8_t** getBuffers();

	void enableLEDs();

	void disableLEDs();

	uint8_t getNumberOfBuffers() { return nBuffers; }
	uint16_t getSizeOfBufferInByte() { return buffersize; }

	void updateScreen();

	void begin();

	void clear();

	void setTimeBrightness(uint8_t bright) { timeBright = bright;}

	void setPixel(uint8_t x, uint8_t y, boolean on);

	void writeText(const char* text, uint8_t x, uint8_t y, int len);
	void println(const char* text);
	void scrollUp();
	void setTimeColor(uint8_t col);// { timeColor = col; }
	void setAnimationColor(uint8_t col);// { aniColor = col; }

	uint16_t getHeight() const {
		return height;
	}

	uint16_t getWidth() const {
		return width;
	}

	void setBrightness(volatile uint8_t brightness) {
		this->brightness = brightness;
	}

private:

	void selectPlane();

	// PORT register pointers, pin bitmasks, pin numbers:
	volatile uint8_t *latport, *oeport, *addraport, *addrbport, *addrcport,
			*addrdport;

	uint8_t sclkpin, latpin, oepin, addrapin, addrbpin, addrcpin, addrdpin,
			_sclk, // clock
			_latch, // latch pin
			_oe, // output enable pin
			_a, _b, _c, _d; // row adress pins

	volatile uint8_t timeBright;
	volatile uint8_t **buffptr; // array of back buffers

	uint8_t planes; // planes / scanning pattern 1/16 scanning means 4 planes, 1/8 means means 3 planes

	uint16_t width;
	uint16_t height;

	uint16_t buffersize; // size in byte of one buffer

	uint8_t nBuffers; // how many buffers in use

	uint8_t colorsChannels; // color channels to use: 1 = monochrom, 2 = RG, 3 = RGB

	char textbuffer[4][16]; // text buffer for text output (boot / menü)
	uint8_t col, row;

	volatile uint8_t plane;

	volatile uint8_t actBuffer;

	volatile uint8_t timeColor;
	volatile uint8_t aniColor;
	volatile uint8_t brightness;

};

#endif /* LEDMATRIXPANEL_H_ */
