#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "epd/EPD_2in9_V2.h"
#include "epd/GUI_Paint.h"
#include "epd/ImageData.h"
#include "epd/fonts.h"
#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_SSD1306.h"
#include "rtc/ds1302.h"
#include "Timezone/Timezone.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RADIO_IN 		19
#define ERRBIT			4		
#define MARKER 			3
#define NOBIT			2
#define HIGHBIT			1
#define LOWBIT 			0
#define LCDWIDTH 		128
#define LCDHEIGHT 		64
#define FRAMESIZE 		60
#define DECODE_SUCCESS 	1
#define DECODE_FAIL 	0

#define RTC_CS_PIN 		17
#define RTC_IO_PIN 		16
#define RTC_SCLCK_PIN	18

#define RGB_R_PIN 		7
#define RGB_G_PIN 		8
#define RGB_B_PIN 		9

#define ACQ_LED_PIN  	12

bool timer_callback(repeating_timer_t *);
void start_new_frame();
void check_radio_data();
void display_frame();
uint8_t decode_frame(datetime_t *);
void pushDateTimeToCore1EPD(void);
void setDateTime(void);

// RTC DS1302
static void setRTCDate(datetime_t *);
static void getRTCDate(datetime_t *);

// RGB
static void setLEDColor(void);
void on_pwm_wrap(void);

//////////////////////////////////////////   CORE1 AND EPD     //////////////////////////////////  
int initialize_core1();
void core1_epd();
void formatEPDTime(char *, int8_t, int8_t);
void updateEPD(time_t);
void formatEPDDate(char *, time_t, const char *);
time_t datetimeToTimeT(datetime_t *);

/////////////////////////////////////////////	 COMMON     /////////////////////////////////	
void pack(uint32_t *, 
		  int8_t *, 
		  int8_t *, 
		  int8_t *, 
		  int8_t *);

void unpack(uint32_t *, 
		  int8_t *, 
		  int8_t *, 
		  int8_t *, 
		  int8_t *);

void pack_dt(uint64_t *, 
			   datetime_t *);

void unpack_dt(uint64_t *, 
			   datetime_t *);

#ifdef __cplusplus
}
#endif