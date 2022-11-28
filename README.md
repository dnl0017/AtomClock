# AtomClock
## Hardware Components :
1. RaspberryPi Pico x1
2. Canaduino WWVB 60kHz Atomic Clock receiver module (serial)
3. SSD1306 monochrome 128x64 OLED display (i2c)
4. Waveshare 2.9" e-Paper Module  (SPI)
5. DS1302 RTC (serial)
6. S8050 BJT (for radio on / off switch)

## Software Components :
1. Raspberry Pi Pico C/C++ SDK
2. fhdm-dev's arduino port for raspberry pi (https://github.com/fhdm-dev/pico-arduino-compat)

I've adapted the DS1302 RTC code from wiringPi library for the pico. 


