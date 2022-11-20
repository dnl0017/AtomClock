#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdarg.h>
#include "ds1302.h"

// Register defines
#define	RTC_SECS	 0
#define	RTC_MINS	 1
#define	RTC_HOURS	 2
#define	RTC_DATE	 3
#define	RTC_MONTH	 4
#define	RTC_DAY		 5
#define	RTC_YEAR	 6
#define	RTC_WP		 7
#define	RTC_TC		 8
#define	RTC_BM		31

// Locals
static uint8_t dataPin, clockPin, cePin ;

/*
 * dsShiftIn:
 *	Shift a number in from the chip, LSB first. Note that the data is
 *	sampled on the trailing edge of the last clock, so it's valid immediately.
 *********************************************************************************
 */
static uint32_t dsShiftIn (void)
{
  uint8_t value = 0 ;
  uint8_t i ;

  gpio_set_dir(dataPin, GPIO_IN) ;	sleep_us(1) ;

  for (i = 0 ; i < 8 ; ++i)
  {
    value |= (gpio_get (dataPin) << i) ;sleep_us(1);
    gpio_put (clockPin, 1) ; sleep_us(1) ;
    gpio_put (clockPin, 0) ; sleep_us(1) ;
  }

  return value;
}

/*
 * dsShiftOut:
 *	A normal LSB-first shift-out, just slowed down a bit - the Pi is
 *	a bit faster than the chip can handle.
 *********************************************************************************
 */
static void dsShiftOut (uint32_t data)
{
  uint8_t i ;

  gpio_set_dir(dataPin, GPIO_OUT) ;

  for (i = 0 ; i < 8 ; ++i)
  {
    gpio_put (dataPin, data & (1 << i)) ;	sleep_us(1) ;
    gpio_put (clockPin, 1) ;			sleep_us(1) ;
    gpio_put (clockPin, 0) ;			sleep_us(1) ;
  }
}

/*
 * ds1302regRead: ds1302regWrite:
 *	Read/Write a value to an RTC Register or RAM location on the chip
 *********************************************************************************
 */
static uint32_t ds1302regRead (const uint32_t reg)
{
  uint32_t data ;

  gpio_put (cePin, 1) ; sleep_us(1) ;
    dsShiftOut (reg) ;
    data = dsShiftIn () ;
  gpio_put (cePin, 0)  ; sleep_us(1) ;

  return data ;
}

static void ds1302regWrite (const uint32_t reg, const uint32_t data)
{
  gpio_put (cePin, 1) ; sleep_us(1) ;
  dsShiftOut (reg) ;
  dsShiftOut (data) ;
  gpio_put (cePin, 0)  ; sleep_us(1) ;
}

/*
 * ds1302rtcWrite: ds1302rtcRead:
 *	Writes/Reads the data to/from the RTC register
 *********************************************************************************
 */
uint32_t ds1302rtcRead (const uint32_t reg)
{
  return ds1302regRead (0x81 | ((reg & 0x1F) << 1)) ;
}

void ds1302rtcWrite (uint32_t reg, uint32_t data)
{
  ds1302regWrite (0x80 | ((reg & 0x1F) << 1), data) ;
}

/*
 * ds1302ramWrite: ds1302ramRead:
 *	Writes/Reads the data to/from the RTC register
 *********************************************************************************
 */
uint32_t ds1302ramRead (const uint32_t addr)
{
  return ds1302regRead (0xC1 | ((addr & 0x1F) << 1)) ;
}

void ds1302ramWrite (const uint32_t addr, const uint32_t data)
{
  ds1302regWrite ( 0xC0 | ((addr & 0x1F) << 1), data) ;
}

/*
 * ds1302clockRead:
 *	Read all 8 bytes of the clock in a single operation
 *********************************************************************************
 */
void ds1302clockRead (uint32_t * clockData)
{
  int i ;
  uint32_t regVal = 0x81 | ((RTC_BM & 0x1F) << 1) ;

  gpio_put (cePin, 1) ; sleep_ms(10) ;

  dsShiftOut (regVal) ;
  for (i = 0 ; i < 8 ; ++i)
    clockData [i] = dsShiftIn () ;

  gpio_put (cePin, 0) ;  sleep_ms(10) ;
}

/*
 * ds1302clockWrite:
 *	Write all 8 bytes of the clock in a single operation
 *********************************************************************************
 */
void ds1302clockWrite (const uint32_t * clockData)
{
  int i ;

  uint32_t b;  
  for (i = 0 ; i < 8 ; ++i)
    b = clockData [i];

  uint32_t regVal = 0x80 | ((RTC_BM & 0x1F) << 1) ;

  gpio_put (cePin, 1) ; sleep_ms(10) ;

  dsShiftOut (regVal) ;
  for (i = 0 ; i < 8 ; ++i)
    dsShiftOut (clockData [i]) ;

  gpio_put (cePin, 0) ;  sleep_ms(10) ;
}

/*
 * ds1302trickleCharge:
 *	Set the bits on the trickle charger.
 *	Probably best left alone...
 *********************************************************************************
 */
void ds1302trickleCharge (const uint32_t diodes, const uint32_t resistors)
{
  if (diodes + resistors == 0)
    ds1302rtcWrite (RTC_TC, 0x5C) ;	// Disabled
  else
    ds1302rtcWrite (RTC_TC, 0xA0 | ((diodes & 3) << 2) | (resistors & 3)) ;
}

/*
 * ds1302setup:
 *	Initialise the chip & remember the pins we're using
 *********************************************************************************
 */
void ds1302setup (const uint8_t cPin, const uint8_t dPin, const uint8_t sPin)
{
  dataPin = dPin ;
  clockPin = cPin ;
  cePin = sPin ;

  gpio_init(dataPin);
  gpio_init(clockPin);
  gpio_init(cePin);

  gpio_put (dataPin, 0) ;
  gpio_put (clockPin, 0) ;
  gpio_put (cePin, 0) ;

  gpio_set_dir(dataPin, GPIO_OUT) ;
  gpio_set_dir(clockPin, GPIO_OUT) ;
  gpio_set_dir(cePin, GPIO_OUT) ;

  ds1302rtcWrite (RTC_WP, 0) ;	// Remove write-protect
}

int32_t bcdToD (uint32_t byte, uint32_t mask)
{
  uint32_t b1, b2 ;
  byte &= mask ;
  b1 = byte & 0x0F ;
  b2 = ((byte >> 4) & 0x0F) * 10 ;
  return b1 + b2 ;
}

uint32_t dToBcd (uint32_t byte)
{
  return ((byte / 10) << 4) + (byte % 10) ;
}
