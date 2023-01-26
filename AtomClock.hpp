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
#include "hygrothermograph/DHT.hpp"


#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG				0

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

#define RTC_SCLCK_PIN	 	16
#define RTC_IO_PIN 		 	17
#define RTC_CS_PIN 		 	18

#define DHT11_PIN			26 

#define RADIO_IN_PIN	 	20//19
#define RADIO_SWITCH_PIN 	15
#define RADIO_ON_TIME_UTC   1    // 0 .. 23
#define RADIO_OFF_TIME_UTC  10   // 0 .. 23

static bool radio_on = false;	
static bool is_datetime_acquired = false;	
static const int16_t millenium = 2000;
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

static unsigned int qt_counter = 0;
const struct{
	const char *qt;
	const char *person;
} quotes[] = {
	{"\"Without music, life would be a mistake.\"", "  -Friedrich Nietzsche"},
	{"\"Atheism is a non-prophet organization.\"", "   -George Carlin"},
	{"\"Politics are not my arena Music is.\"", "-Aretha Franklin"},
	{"\"A winner is a dreamer who never gives up.\"", "       -Nelson Mandela"},
	{"\"Cow with no legs, ground beef.\"", "   -Confucius"},
	{"\"What one fool can         understand, another can.\"", "      -Richard Feynman"},
	{"\"The spirit of envy can   destroy but never build.\"", "    -Margaret Thatcher"},
	{"\"The higher up I went, the less happy I was.\"", "        -Dave Chapelle"},
	{"\"If you're going through  hell, keep going.\"", "    -Winston Churchill"},
	{"\"Even communism works...  in theory.\"", "-Homer Simpson"},
	{"\"I enjoy the last quarter of all basketball games.\"", "      -Sarah Silverman"}
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