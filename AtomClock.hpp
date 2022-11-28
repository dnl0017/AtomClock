#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

#define ERRBIT			 	4			
#define MARKER 			 	3
#define NOBIT			 	2
#define HIGHBIT			 	1
#define LOWBIT 			 	0
#define LCDWIDTH 		 	128
#define LCDHEIGHT 		 	64
#define FRAMESIZE 		 	60
#define DECODE_SUCCESS 	 	1
#define DECODE_FAIL 	 	0

#define RTC_CS_PIN 		 	17
#define RTC_IO_PIN 		 	16
#define RTC_SCLCK_PIN	 	18

#define ACQ_LED_PIN  	 	12

#define RADIO_IN_PIN	 	19
#define RADIO_SWITCH_PIN 	9
#define RADIO_ON_TIME_UTC   1   // 0 .. 23
#define RADIO_OFF_TIME_UTC  10   // 0 .. 23

static bool radio_on;
bool timer_callback(repeating_timer_t *);
void start_new_frame();
void check_radio_data();
void display_frame();
void display_saver();
uint8_t decode_frame(datetime_t *);
void pushDateTimeToCore1EPD(void);
void setDateTime(void);

// RTC DS1302
static void setRTCDate(datetime_t *);
static void getRTCDate(datetime_t *);

//////////////////////////////////////////   CORE1 AND EPD     //////////////////////////////////  
int initialize_core1();
void core1_epd();
void formatEPDTime(char *, int8_t, int8_t);
void updateEPD(time_t);
void formatEPDDate(char *, time_t, const char *);
time_t datetimeToTimeT(datetime_t *);
void power_radio(bool *);

void theme_olde_eng(char *, char *);
void theme_wwvb(char *, char *);
void theme_curly(char *, char *);
void theme_city(char *, char *);
void theme_text(char *, char *);
void theme_flash(char *, char *);

static unsigned int thm_counter = 0;

typedef void (*theme)(char *, char *);
const struct{
	theme thm;
	const char *name;
} theme_table[] = {
	{theme_olde_eng, "Olde"},
	{theme_text, "Text"},
	{theme_wwvb, "WWVB"},
	{theme_curly, "Curly"},
	{theme_city, "City"},
	{theme_flash, "Flash"}
};

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

void pack_dt(datetime_t *,
			 uint64_t *);

void unpack_dt(uint64_t *, 
			   datetime_t *);

#ifdef __cplusplus
}
#endif