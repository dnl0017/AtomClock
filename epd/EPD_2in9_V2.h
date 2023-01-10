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
#define PIN_BUSY 6   
#define PIN_RST  7   
#define PIN_DC   8   

#define PIN_CLK  10  
#define PIN_MOSI 11  // DIN
#define PIN_MISO 12  // NC, not used.
#define PIN_CS   9   

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
