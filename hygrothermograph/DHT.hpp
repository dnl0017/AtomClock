/**********************************************************************
* Filename    : DHT.hpp
* Description : DHT Temperature & Humidity Sensor library for Raspberry Pi Pico.
                Adapted for Raspberry Pi Pico C\C++ SDK.
*				Program transplantation by Freenove.
* Author      : Unknown
* modification: 2022/03/26
* Reference   : https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
**********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifndef _DHT_H_
#define _DHT_H_

////read return flag of sensor
#define DHTLIB_OK               0
#define DHTLIB_ERROR_CHECKSUM   -1
#define DHTLIB_ERROR_TIMEOUT    -2
#define DHTLIB_INVALID_VALUE    -127

#define DHTLIB_DHT11_WAKEUP     20
#define DHTLIB_DHT_WAKEUP       1

#define DHTLIB_TIMEOUT          100

typedef struct {
    int8_t temp_whole;
    int8_t temp_frac;   
    int8_t humidity;
} dht_reading;

class DHT {      
    public:
        DHT(uint8_t pin);
        int readDHT11Once(dht_reading *result); //read DHT11
        int readDHT11(dht_reading *result );     //read DHT11
    private:
        uint8_t bits[5];    //Buffer to receives data
        int readSensor(void); 
        uint8_t _pinNumber;        
};




#endif
