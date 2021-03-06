/*
 * Menu.h
 *
 *  Created on: 08.02.2015
 *      Author: sr
 */

#ifndef MENU_H_
#define MENU_H_

#include "LEDMatrixPanel.h"
#include "Clock.h"
#include "Button.h"
#include "ClickHandler.h"

#define PIN_B1 3
#define PIN_B2 5

#define BUTTON_MENU 1
#define BUTTON_SEL 2

#define SET_BRIGHTNESS 0
#define SET_TIME_HOURS 1
#define SET_TIME_TENMIN 2
#define SET_TIME_ONEMIN 3
#define SET_DATE_YEAR 4
#define SET_DATE_MON 5
#define SET_DATE_DAY 6

#define SET_TIME_MODE 7

#define SET_TIME_DURATION 8
#define SET_DATE_MODE 9
#define SET_COLOR_ANI 10
#define SET_COLOR_CLOCK 11

#define SET_TEMP_MODE 12
#define SET_PIR_MODE 13

#define EEPROM_START 8
#define EEPROM_END 13

#define NMENU 14

class Menu : public ClickHandler {
public:
	Menu(LEDMatrixPanel* panel, Clock* clock);
	virtual ~Menu();

	void update(long now);

	bool isActive() {return active; }

	void loadOptions();

	void enterMenu();

	void redrawMenu();

	void leaveMenu();

	void saveOption();

	uint8_t getOption(int index) {return option[index];}

	void buttonReleased(uint8_t no, bool longPress);

private:
	static const char* mm[];
	LEDMatrixPanel* panel;
	Clock* clock;
	Button menuButton;
	Button selButton;

	bool active;
	bool dirty; // needs reconfig
	bool redrawNeeded;

	bool clockDirty;

	uint8_t actMenu;
	uint8_t actOption;



	uint8_t titleIndex[NMENU];
	uint8_t nOptions[NMENU];

	uint8_t option[NMENU];
};

#endif /* MENU_H_ */
