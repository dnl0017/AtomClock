#include "AtomClock.hpp"

volatile uint8_t sampleCounter = 100;
volatile uint8_t newBit = NOBIT;
volatile uint8_t oldBit = NOBIT;
volatile uint8_t pulseWidth = 0;

uint8_t frame[FRAMESIZE];
uint8_t frameindex = 0;
datetime_t current_dt;

//  EPD BackImage ref
uint8_t *BackImage;

// INIT SSD1306 DISPLAY
// Wire is GPIO 4 & 5, Wire1 is GPIO 10 & 11
Adafruit_SSD1306 display(LCDWIDTH, LCDHEIGHT, &Wire);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

// pointer to the time change rule, use to get TZ abbrev
TimeChangeRule *tcr;      

void initialize_core0()
{
	// Init RTC
  	ds1302setup(RTC_SCLCK_PIN, RTC_IO_PIN, RTC_CS_PIN);
	//sleep_ms(100);

	//  Init WWVB
    gpio_init(RADIO_IN);
    gpio_set_dir(RADIO_IN, GPIO_IN);
	//sleep_ms(2000);

    // Init SSD1306 display
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(4);
	display.setCursor(0,0);
	display.println("WWVB");
	display.display();
	sleep_ms(1000);
	display.clearDisplay();
	display.cp437(true);
	display.setTextSize(1);

	// // INIT EPD LIGHTING LEDs
	// gpio_set_function(RGB_R_PIN, GPIO_FUNC_PWM);
	// gpio_set_function(RGB_G_PIN, GPIO_FUNC_PWM);
	// gpio_set_function(RGB_B_PIN, GPIO_FUNC_PWM); 

    // // Get some sensible defaults for the slice configuration. By default, the
    // // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    // pwm_config config = pwm_get_default_config();

    // // Set divider, reduces counter clock to sysclock/this value
    // pwm_config_set_clkdiv(&config, 4.f);    
	// pwm_config_set_wrap(&config, 255);

    // uint slice_num1 = pwm_gpio_to_slice_num(RGB_R_PIN);
    // pwm_init(slice_num1, &config, true);

    // uint slice_num2 = pwm_gpio_to_slice_num(RGB_G_PIN);
    // pwm_init(slice_num2, &config, true);

    // uint slice_num3 = pwm_gpio_to_slice_num(RGB_B_PIN);
    // pwm_init(slice_num3, &config, true);

	// sleep_ms(100);

	// Init acquiring LED
	gpio_init(ACQ_LED_PIN);
	gpio_set_dir(ACQ_LED_PIN, GPIO_OUT);    
	gpio_put(ACQ_LED_PIN, 0);	

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

	// Wait until marker
	// int pw;
	// do
	// 	pw = pulseIn(RADIO_IN, LOW, 2000000)/1000;
	// while((pw < 630) && (pw > 900));
	// delay(200);

    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / hz, timer_callback, NULL, &timer)) {
        printf("Failed to add timer\n");
        exit(-1);
    }

	setLEDColor();

	start_new_frame();

	while(1)
	{
		check_radio_data();
	}

    return 0;
}

bool timer_callback(repeating_timer_t *rt) 
{
	pulseWidth += !gpio_get(RADIO_IN);	
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

void start_new_frame()
{
	char buffer[60];
	datetime_t frame_datetime;

	//if(decode_frame(&frame_datetime)==DECODE_FAIL)
	if(decode_frame(&frame_datetime)==DECODE_FAIL)
	{
		printf("Bad Frame\n");
		gpio_put(ACQ_LED_PIN, 0);	
	}
	else
	{
		setRTCDate(&frame_datetime);
		gpio_put(ACQ_LED_PIN, 1);	

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

	pack_dt(&tx_buffer, &t);

    multicore_fifo_push_blocking(tx_buffer>>32);
    multicore_fifo_push_blocking(tx_buffer);

}

void display_frame()
{
	char frameindex_str [2];

	display.clearDisplay();
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

void check_radio_data()
{
	if(newBit != NOBIT)
	{		
		if(frameindex>59)
			start_new_frame();
		if((newBit==MARKER)&&(oldBit==MARKER))
			start_new_frame();

		frame[frameindex++] = newBit;

		display_frame();

		oldBit = newBit;
		newBit = NOBIT;
	}
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

static void setLEDColor()
{
	//srand(time_us_32());

	for(int i = 0; i < 256; i+=32)
		for(int j = 0; j < 256; j+=32)
			for(int k = 0; k < 256; k+=32)
			{				
				pwm_set_gpio_level(RGB_R_PIN, i);
				pwm_set_gpio_level(RGB_G_PIN, j);
				pwm_set_gpio_level(RGB_B_PIN, k);
				sleep_ms(10);	
			}
}

void setDateTime()
{
	datetime_t t;
	t.day = 20;
	t.dotw = 0;
	t.hour = 3;
	t.min = 6;
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

    char console_buf[60];    
	char epd_buf[32];

	while(1)
	{
        if(multicore_fifo_rvalid()){
			rx_buffer = multicore_fifo_pop_blocking();
			rx_buffer = rx_buffer << 32;
			rx_buffer |= multicore_fifo_pop_blocking();

			unpack_dt(&rx_buffer, &current_rtc_datetime);

			datetime_to_str(console_buf, 60, &current_rtc_datetime);
			printf("Core1 RTC Time : %s\n", console_buf);

			time_t utc = datetimeToTimeT(&current_rtc_datetime);

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

    Paint_NewImage(BackImage, EPD_2IN9_V2_WIDTH, EPD_2IN9_V2_HEIGHT, 90, EPD_WHITE);  
    Paint_SelectImage(BackImage);

    Paint_DrawBitMap(gImage_wwvb);
	
	// datetime_to_str(tmp_buffer, 40, t);
	// char * token = strtok(tmp_buffer, " ");
	// strcat(date_buffer, token);
	// strcat(date_buffer, ", ");
	// token = strtok(NULL, " ");
	// strcat(date_buffer, token);
	// strcat(date_buffer, " ");
	// token = strtok(NULL, " ");
	// strcat(date_buffer, token);
	// strcat(date_buffer, " ");
	// strtok(NULL, " ");
	// token = strtok(NULL, " ");
	// strcat(date_buffer, token );
	// strcat(date_buffer, " ");

	formatEPDDate(date_buffer, t, tcr->abbrev);
	Paint_DrawString_EN(94, 13, date_buffer, &Font12, EPD_WHITE, EPD_BLACK);

	formatEPDTime(time_buffer, hour(t), minute(t));	
	Paint_DrawString_EN(110, 67, time_buffer, &Font7seg, EPD_WHITE, EPD_BLACK);		

    EPD_2IN9_V2_Display(BackImage);
	sleep_ms(300);

	EPD_2IN9_V2_Sleep();
}

// format a time_t value, with a time zone appended.
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
		tmp[0] = 43;
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

void pack_dt(uint64_t *src, 
			 datetime_t * dest){
	*src |= dest->year;
	*src <<= 8;
	*src |= dest->month;
	*src <<= 8;
	*src |= dest->day;
	*src <<= 8;
	*src |= dest->dotw;
	*src <<= 8;
	*src |= dest->hour;
	*src <<= 8;
	*src |= dest->min;
	*src <<= 8;
	*src |= dest->sec;
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