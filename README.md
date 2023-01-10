# AtomClock
## Hardware Components :
1. RaspberryPi Pico 
2. Canaduino WWVB 60kHz Atomic Clock receiver module (serial)
3. SSD1306 monochrome 128x64 OLED display (secondary display, i2c)
4. Waveshare 2.9" e-Paper Module  (main display, SPI)
5. DS1302 RTC module with CR2023 battery backup (serial)
6. S8050 BJT (for radio on / off switch)
7. 1/4W, 1K resistor
8. DHT11 Hygrothermograph

## Software Components :
1. Raspberry Pi Pico C/C++ SDK
2. fhdm-dev's arduino port for raspberry pi (https://github.com/fhdm-dev/pico-arduino-compat). For SSD1306 display mostly.

## Acknowledgements
- This project was heavily inspired by [WWVB Clock Project by Bruce Hall](http://w8bh.net/wwvb_clock.pdf). The basic functioning is the same, some hardware components used are different. 
    - Added a hygrothermograph for humidity and temperature readings.
    - Uses a 2.9" epaper display as main display, SSD1306 OLED display as secondary.
    - RP2040 instead of STM32F103C8T6 (blue pill).
- I've adapted the DS1302 RTC code from [wiringPi library](http://wiringpi.com) for the pico. 



