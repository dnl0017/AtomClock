/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Copyright (c) 2021 FHDM Apps https://github.com/fhdm-dev
 */

#include <stdio.h>
#include "pico/stdlib.h"
//
#include "Arduino.h"
#include "Wire.h"
//
#include "Adafruit_SSD1306.h"

#define RADIO_IN 15

int main() {
    stdio_init_all();

	printf("Starting\n");

    // Wire is GPIO 4 & 5
    // Wire1 is GPIO 10 & 11
    Adafruit_SSD1306 display(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &Wire1);

    // initialize with the I2C addr 0x3C
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
	delay(2000);
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setCursor(0,SSD1306_LCDHEIGHT/3);
	display.setTextSize(2);
	display.println("Yo bish!");
	display.display();
	delay(1000);
	display.clearDisplay();

    while(1){
        int pulseWidth = pulseIn(RADIO_IN, LOW, 2000000);
        pulseWidth /= 1000;

        if (pulseWidth > 90)
		{
            printf("pulse width: %d\n", pulseWidth);          
			// Clear the buffer.
			display.clearDisplay();
			display.setTextColor(WHITE);
			display.setCursor(0,SSD1306_LCDHEIGHT/3);
			display.setTextSize(2);
			display.println(pulseWidth);
			display.display();
        }

    };

	// // Scroll part of the screen
	// display.setCursor(0,0);
	// display.setTextSize(1);
	// display.println("Future");
	// display.println("WWVB");
	// display.println("Atomic clock.");
	// display.display();
	// display.startscrollright(0x00, 0x00);

    return 0;
}
