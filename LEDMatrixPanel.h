/*
 * LEDMatrixPanel.h
 *
 *  Created on: 29.01.2015
 *      Author: sr
 */

#ifndef LEDMATRIXPANEL_H_
#define LEDMATRIXPANEL_H_
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

	void setPixel(uint8_t x, uint8_t y, boolean on);

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

	volatile uint8_t **buffptr; // array of back buffers

	uint8_t planes; // planes / scanning pattern 1/16 scanning means 4 planes, 1/8 means means 3 planes

	uint16_t width;
	uint16_t height;

	uint16_t buffersize; // size in byte of one buffer

	uint8_t nBuffers; // how many buffers in use

	uint8_t colorsChannels; // color channels to use: 1 = monochrom, 2 = RG, 3 = RGB

	uint8_t scanCycle; // the actual scan cycle

	uint8_t plane;

};

#endif /* LEDMATRIXPANEL_H_ */
