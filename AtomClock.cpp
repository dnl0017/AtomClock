#include "AtomClock.hpp"

volatile uint8_t sampleCounter = 100;
volatile uint8_t newBit = NOBIT;
volatile uint8_t oldBit = NOBIT;
volatile uint8_t pulseWidth = 0;

uint8_t frame[FRAMESIZE];
uint8_t frameindex = 0;
datetime_t current_dt, last_sync_time;

//  EPD BackImage ref
uint8_t *BackImage;

// INIT SSD1306 DISPLAY
// Wire is GPIO 4 & 5, Wire1 is GPIO 10 & 11
Adafruit_SSD1306 display(LCDWIDTH, LCDHEIGHT, &Wire);

// Eastern Time Zone (Toronto, New York)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

// pointer to the time change rule, use to get TZ abbrev
TimeChangeRule *tcr;  

void initialize_core0()
{
	// Init RTC
  	ds1302setup(RTC_SCLCK_PIN, RTC_IO_PIN, RTC_CS_PIN);
	sleep_ms(100);

    // Init SSD1306 display
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(4);
	display.setCursor(0,0);
	display.println("WWVB");
	display.display();
	sleep_ms(500);

	// Init acquiring LED
	gpio_init(ACQ_LED_PIN);
	gpio_set_dir(ACQ_LED_PIN, GPIO_OUT);    
	gpio_put(ACQ_LED_PIN, 0);	
	sleep_ms(100);

	// Init Radio switch
	gpio_init(RADIO_SWITCH_PIN);
	gpio_set_dir(RADIO_SWITCH_PIN, GPIO_OUT);    
    gpio_pull_up(RADIO_SWITCH_PIN);
	gpio_put(RADIO_SWITCH_PIN, 0);
	radio_on = false;	
	sleep_ms(100);
}

int main() {
    stdio_init_all();

	initialize_core0();

	//setDateTime();

    // lcd handled by core1.
    multicore_launch_core1(core1_epd);   

	pushDateTimeToCore1EPD();

	//  INIT TIMER SAMPLING
    int hz = 100;
    repeating_timer_t timer;

    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &timer)) {
        printf("Failed to add timer\n");
        exit(-1);
    }

	while(1)check_radio_data();

    return 0;
}

bool timer_callback(repeating_timer_t *rt) 
{
	pulseWidth += radio_on ? !gpio_get(RADIO_IN_PIN) : 0;	
	sampleCounter--;
	if(sampleCounter<1)
	{
		if((pulseWidth>61)&&(pulseWidth<90))
			newBit = MARKER;
		else if((pulseWidth>31)&&(pulseWidth<60))
			newBit = HIGHBIT;
		else if((pulseWidth>5)&&(pulseWidth<30))
			newBit = LOWBIT;
		else
			newBit = ERRBIT;

		sampleCounter = 100;
		pulseWidth = 0;
	}
    return true; // keep repeating
}

void check_radio_data()
{
	if(newBit != NOBIT)
	{		
		if(frameindex>59)
			start_new_frame();
		if((newBit==MARKER)&&(oldBit==MARKER))
			start_new_frame();

		frame[frameindex++] = newBit;

		if(radio_on)
			display_frame();
		else
			display_saver();

		oldBit = newBit;
		newBit = NOBIT;
	}
}

void start_new_frame()
{
	char buffer[60];
	datetime_t frame_datetime;

	if(decode_frame(&frame_datetime)==DECODE_FAIL)
	{
		printf("Bad Frame\n");
		gpio_put(ACQ_LED_PIN, 0);	
	}
	else
	{
		//frame_datetime.min += 1;  //  Add the lost minute;
		setRTCDate(&frame_datetime);
		gpio_put(ACQ_LED_PIN, 1);	

		last_sync_time = frame_datetime;

		datetime_to_str(buffer, 60, &frame_datetime);
		printf("%s\n", buffer);
	}

	pushDateTimeToCore1EPD();
	frameindex = 0;
}

void pushDateTimeToCore1EPD()
{
	uint64_t tx_buffer;
	char console_buffer[60];
	datetime_t t;

	getRTCDate(&t);

	datetime_to_str(console_buffer, 60, &t);
	printf("Core0 RTC Time : %s\n", console_buffer);

	radio_on = (((RADIO_OFF_TIME_UTC-RADIO_ON_TIME_UTC)%24)+24)%24-(((t.hour-RADIO_ON_TIME_UTC)%24)+24)%24>0;
	pack_dt(&t, &tx_buffer);

    multicore_fifo_push_blocking(tx_buffer>>0x20);
    multicore_fifo_push_blocking(tx_buffer);

}

void display_frame()
{
	char frameindex_str [2];

	display.clearDisplay();
	display.cp437(true);
	display.setTextSize(1);
	display.setCursor(32,0);
	display.write('B');
	display.write('i');
	display.write('t');
	display.write(' ');

	itoa(frameindex-1, frameindex_str, 10);
	display.write(frameindex_str[0]);
	display.write(frameindex_str[1]);

	for(uint8_t i = 0; i < frameindex; i++)
	{
		if(i%10==0) display.write('\n');	
		else display.write(' ');
		switch(frame[i])
		{
			case LOWBIT : 
				display.write('0');
				break;
			case HIGHBIT : 
				display.write('1');;
				break;
			case MARKER : 
				display.write('M');
				break;
			case ERRBIT : 
				display.write('X');
				break;
		}		
	}
	display.display();
}

void display_saver()
{
	char frameindex_str [2];

	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(6);
	display.setCursor(32, 12);

	itoa(frameindex-1, frameindex_str, 10);

	if(strlen(frameindex_str) < 2)
	{
		display.write(' ');
		display.write(frameindex_str[0]);	
	}
	else
	{
		display.write(frameindex_str[0]);
		display.write(frameindex_str[1]);
	}

	display.display();

}

uint8_t decode_frame(datetime_t *t)
{
	const int16_t millenium = 2000;
	uint8_t daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if( frame[0]  != MARKER ) 
		return DECODE_FAIL;

	for(uint8_t i = 1; i<60; i++)
		if( frame[i] == ERRBIT || i % 10 == 9 && frame[i] != MARKER ) 
			return DECODE_FAIL;

	uint8_t leap = 0;
	leap += frame[55]==HIGHBIT?1:0;

	uint8_t dst = 0;
	dst += frame[58]==HIGHBIT?1:0;

	uint16_t dayOfYear = 0;
 	dayOfYear += frame[22]==HIGHBIT?200:0;
 	dayOfYear += frame[23]==HIGHBIT?100:0;
 	dayOfYear += frame[25]==HIGHBIT?80:0;
 	dayOfYear += frame[26]==HIGHBIT?40:0;
 	dayOfYear += frame[27]==HIGHBIT?20:0;
 	dayOfYear += frame[28]==HIGHBIT?10:0;
 	dayOfYear += frame[30]==HIGHBIT?8:0;
 	dayOfYear += frame[31]==HIGHBIT?4:0;
 	dayOfYear += frame[32]==HIGHBIT?2:0;
 	dayOfYear += frame[33]==HIGHBIT?1:0;
	
	t->month = 1;
	while(1)
	{
		uint16_t dim = daysInMonth[t->month];
		if(t->month == 2 && leap == 1) dim++;
		if(dayOfYear <= dim) break;
		dayOfYear -= dim;
		t->month++;
	}

	t->day = dayOfYear;
  	t->dotw = 0;
	t->sec = 0;

	t->min = 0;
	t->min += frame[1]==HIGHBIT?40:0;
	t->min += frame[2]==HIGHBIT?20:0;
	t->min += frame[3]==HIGHBIT?10:0;
	t->min += frame[5]==HIGHBIT?8:0;
	t->min += frame[6]==HIGHBIT?4:0;
	t->min += frame[7]==HIGHBIT?2:0;
	t->min += frame[8]==HIGHBIT?1:0;
 
	t->hour = 0;
	t->hour += frame[12]==HIGHBIT?20:0;
	t->hour += frame[13]==HIGHBIT?10:0;
	t->hour += frame[15]==HIGHBIT?8:0;
	t->hour += frame[16]==HIGHBIT?4:0;
	t->hour += frame[17]==HIGHBIT?2:0;
	t->hour += frame[18]==HIGHBIT?1:0;

	t->year = millenium;
	t->year += frame[45]==HIGHBIT?80:0;
	t->year += frame[46]==HIGHBIT?40:0;
	t->year += frame[47]==HIGHBIT?20:0;
	t->year += frame[48]==HIGHBIT?10:0;
	t->year += frame[50]==HIGHBIT?8:0;
	t->year += frame[51]==HIGHBIT?4:0;
	t->year += frame[52]==HIGHBIT?2:0;
	t->year += frame[53]==HIGHBIT?1:0;

	return DECODE_SUCCESS;
}

static void setRTCDate(datetime_t * t)
{  
  uint32_t clock [8];

  clock [0] = dToBcd (t->sec) ;	// seconds 0-59
  clock [1] = dToBcd (t->min) ;	// mins 0-59
  clock [2] = dToBcd (t->hour) ;	// hours 0-23
  clock [3] = dToBcd (t->day) ;	// day of the month 1-31
  clock [4] = dToBcd (t->month); // months 0-11 --> 1-12
  clock [5] = dToBcd (t->dotw) ;	// weekdays (sun 0)
  clock [6] = dToBcd (t->year-2000) ; // years
  clock [7] = 0 ;			// W-Protect off
  ds1302clockWrite (clock) ;

}

static void getRTCDate(datetime_t * t)
{
    uint32_t clock [8] ;

    ds1302clockRead(clock) ;
    t->sec = bcdToD(clock[0], masks [0]);
    t->min = bcdToD(clock[1], masks [1]);
    t->hour = bcdToD(clock[2], masks [2]);
    t->day = bcdToD(clock[3], masks [3]);
    t->month = bcdToD(clock[4], masks [4]);
    t->dotw = bcdToD(clock[5], masks [5]);
    t->year = bcdToD(clock[6], masks [6]) + 2000;

}

void setDateTime()
{
	datetime_t t;
	t.day = 20;
	t.dotw = 0;
	t.hour = 0;
	t.min = 50;
	t.month = 11;
	t.sec = 0;
	t.year = 2022;
	setRTCDate(&t);
}

//////////////////////////////////////////   CORE1 AND EPD     //////////////////////////////////  

int initialize_core1()
{
	// Init EPD
	// Use SPI1 at 10MHz.
    spi_init(spi1, 10 * 1000 * 1000);

    gpio_set_function(PIN_CS,   GPIO_FUNC_SPI);  
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);  
    gpio_set_function(PIN_CLK, GPIO_FUNC_SPI);   

    //  Init RST
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    //  Init DC
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);

    // Init CS
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    // Init BUSY
    gpio_init(PIN_BUSY);
    gpio_set_dir(PIN_BUSY, GPIO_IN);
    gpio_pull_up(PIN_BUSY);    

    gpio_put(PIN_CS, 1);	
	sleep_ms(100);

    uint16_t Imagesize = ((EPD_2IN9_V2_WIDTH % 8 == 0) ? (EPD_2IN9_V2_WIDTH / 8 ): (EPD_2IN9_V2_WIDTH / 8 + 1)) * EPD_2IN9_V2_HEIGHT;
    if((BackImage = (uint8_t*)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }

	return 0;
}

void core1_epd()
{
	if(0>initialize_core1()){
		printf("Error initializing EPD... \n");
		return;
	}

	uint64_t rx_buffer;	
	datetime_t current_rtc_datetime;
	static bool radio_initialized = false;

    char console_buf[60];    

	while(1)
	{
        if(multicore_fifo_rvalid()){
			rx_buffer = multicore_fifo_pop_blocking();
			rx_buffer = rx_buffer << 0x20;
			rx_buffer |= multicore_fifo_pop_blocking();

			unpack_dt(&rx_buffer, &current_rtc_datetime);

			power_radio(&radio_initialized);

			datetime_to_str(console_buf, 60, &current_rtc_datetime);
			printf("Core1 RTC Time : %s\n", console_buf);

			time_t utc = datetimeToTimeT(&current_rtc_datetime) + 60;

		    setTime(utc);

			time_t local = myTZ.toLocal(utc, &tcr);

			updateEPD(local);
		}
		tight_loop_contents();
	}
}

void updateEPD(time_t t)
{
	char time_buffer[6],
	 	 tmp_buffer[40],
		 date_buffer[30];

	EPD_2IN9_V2_Init();
    EPD_2IN9_V2_Clear();
	sleep_ms(100);

	formatEPDDate(date_buffer, t, tcr->abbrev);	
	formatEPDTime(time_buffer, hour(t), minute(t));

	theme_table[thm_counter++ % (sizeof(theme_table) / 8)].thm(date_buffer, time_buffer);

    EPD_2IN9_V2_Display(BackImage);
	sleep_ms(300);

	EPD_2IN9_V2_Sleep();
}

void formatEPDDate(char *buf, time_t t, const char *tz)
{
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%s %.2d %s %d %s", dayShortStr(weekday(t)), 
									 day(t), 
									 m, 
									 year(t), 
									 tz);
	printf("%s\n", buf);
}

void formatEPDTime(char * buf, int8_t hour, int8_t min)
{
	char tmp[3];

	// hours
	itoa(hour, tmp, 10);
	if(strlen(tmp)<2){
		tmp[1] = tmp[0] - 16;
		tmp[0] = 32; //  32 = 0, 43 = space;
	} else {
		tmp[0] -= 16;
		tmp[1] -= 16;
	}
	tmp[2] = '\0';
	strcpy(buf, tmp);

	// separator
	tmp[0] = 42;
	tmp[1] = '\0';
	strcat(buf, tmp);

	// minutes
	itoa(min, tmp, 10);
	if(strlen(tmp)<2){
		tmp[1] = tmp[0];
		tmp[0] = '0';
	}
	tmp[0] -= 16;
	tmp[1] -= 16;
	tmp[2] = '\0';
	strcat(buf, tmp);
}

time_t datetimeToTimeT(datetime_t * t)
{
	tmElements_t tm;
	tm.Month = t->month;
	tm.Day = t->day;
	tm.Year = t->year - 1970;
	tm.Hour = t->hour;
	tm.Minute = t->min;
	tm.Second = t->sec;

	return makeTime(tm);
}

void power_radio(bool * init)
{
	if(radio_on)
		if(!(*init))
		{
			gpio_init(RADIO_IN_PIN);
			gpio_set_dir(RADIO_IN_PIN, GPIO_IN);
			*init = true;
		}
	else
		*init = false;

	gpio_put(RADIO_SWITCH_PIN, radio_on);
} 

//  THEMES!!
void theme_olde_eng(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
	Paint_Clear(EPD_WHITE);
    Paint_SelectImage(BackImage);

	Paint_DrawString_EN(14, 8, date, &Font20, EPD_WHITE, EPD_BLACK);
    Paint_DrawLine(20, 30, 276, 30, EPD_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	Paint_DrawString_EN(10, 36, time, &FontOldeEng, EPD_WHITE, EPD_BLACK);	
}

void theme_wwvb(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
    Paint_SelectImage(BackImage);
	Paint_DrawBitMap(gImage_wwvb);

	Paint_DrawString_EN(84, 12, date, &Font16, EPD_WHITE, EPD_BLACK); 
	Paint_DrawString_EN(120, 70, time, &Font7seg, EPD_WHITE, EPD_BLACK);	
}

void theme_curly(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
    Paint_SelectImage(BackImage);
	Paint_DrawBitMap(gImage_curly);

    Paint_DrawRectangle(10, 23, 290, 104, EPD_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

	//Paint_DrawString_EN(10, 104, date, &Font20, EPD_WHITE, EPD_BLACK);
	Paint_DrawString_EN(10, 19, time, &FontCurly, EPD_WHITE, EPD_BLACK);	
}

void theme_city(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
    Paint_SelectImage(BackImage);
	Paint_DrawBitMap(gImage_city);

	Paint_DrawString_EN(158, 112, date, &Font12, EPD_BLACK, EPD_WHITE); 
	Paint_DrawString_EN(10, 5, time, &FontBroadway, EPD_WHITE, EPD_BLACK);	
}

void theme_text(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
	Paint_Clear(EPD_BLACK);
    Paint_SelectImage(BackImage);

	Paint_DrawString_EN(10, 38, time, &FontOldeEng, EPD_BLACK, EPD_WHITE);	
	Paint_DrawRectangle(0, 0, 296, 41, EPD_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	Paint_DrawString_EN(0, 0, "\"Without music, life would be a mistake.\" ", &Font16, EPD_WHITE, EPD_BLACK);		
	Paint_DrawString_EN(140, 28, "  -Friedrich Nietzsche", &Font12, EPD_WHITE, EPD_BLACK);	
}

void theme_flash(char *date, char *time)
{    
	Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
    Paint_SelectImage(BackImage);
	Paint_DrawBitMap(gImage_flash);

	Paint_DrawString_EN(80, 44, time, &Font7seg, EPD_WHITE, EPD_BLACK);	
	Paint_DrawString_EN(80, 116, date, &Font12, EPD_WHITE, EPD_BLACK); 
}


/////////////////////////////////////////////	 COMMON     /////////////////////////////////	
void pack(uint32_t *buf, 
		  int8_t *b1, 
		  int8_t *b2, 
		  int8_t *b3, 
		  int8_t *b4){
	*buf |= *b1;
	*buf <<= 8;
	*buf |= *b2;
	*buf <<= 8;
	*buf |= *b3;
	*buf <<= 8;
	*buf |= *b4;
}

void unpack(uint32_t *buf, 
			int8_t *b1, 
			int8_t *b2, 
			int8_t *b3, 
			int8_t *b4){
	*b1 = *buf >> 24;
	*b2 = *buf >> 16;
	*b3 = *buf >> 8;
	*b4 = *buf;
}

void pack_dt(datetime_t * src,
			 uint64_t * dest){
	*dest |= src->year;
	*dest <<= 8;
	*dest |= src->month;
	*dest <<= 8;
	*dest |= src->day;
	*dest <<= 8;
	*dest |= src->dotw;
	*dest <<= 8;
	*dest |= src->hour;
	*dest <<= 8;
	*dest |= src->min;
	*dest <<= 8;
	*dest |= src->sec;
}

void unpack_dt(uint64_t *src, 
			   datetime_t * dest){
	dest->year = *src >> 48;
	dest->month = *src >> 40;
	dest->day = *src >> 32;
	dest->dotw = *src >> 24;
	dest->hour = *src >> 16;
	dest->min = *src >> 8;
	dest->sec = *src;
}