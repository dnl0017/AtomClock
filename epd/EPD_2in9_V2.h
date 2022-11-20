#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#ifndef __EPD_2IN9_V2_H_
#define __EPD_2IN9_V2_H_


#ifdef __cplusplus
 extern "C" {
#endif

// Display resolution
#define EPD_2IN9_V2_WIDTH       128
#define EPD_2IN9_V2_HEIGHT      296

// SPI Defines
#define PIN_BUSY 22
#define PIN_RST  21
#define PIN_DC   20

#define PIN_CLK  14
#define PIN_MOSI 15  // DIN
#define PIN_MISO 12
#define PIN_CS   13

extern void EPD_2IN9_V2_Init(void);
extern void EPD_2IN9_V2_Clear(void);
extern void EPD_2IN9_V2_Display(uint8_t *Image);
extern void EPD_2IN9_V2_Display_Base(uint8_t *Image);
extern void EPD_2IN9_V2_Display_Partial(uint8_t *Image);
extern void EPD_2IN9_V2_Sleep(void);

#ifdef __cplusplus
}
#endif

#endif
