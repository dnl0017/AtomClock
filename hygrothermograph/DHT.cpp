/**********************************************************************
* Filename    : DHT.hpp
* Description : DHT Temperature & Humidity Sensor library for Raspberry Pi Pico.
                Adapted for Raspberry Pi Pico C\C++ SDK.
*				Program transplantation by Freenove.
* Author      : Unknown
* modification: 2022/03/26
* Reference   : https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
**********************************************************************/

#include "DHT.hpp"

DHT::DHT(uint8_t pin){
	_pinNumber = pin;
    gpio_init(_pinNumber);
	gpio_pull_up(_pinNumber); // instead of 10K resistor
}
//Function: Read DHT sensor, store the original data in bits[]
// return values:DHTLIB_OK   DHTLIB_ERROR_CHECKSUM  DHTLIB_ERROR_TIMEOUT
int DHT::readSensor(){
	int mask = 0x80;
	int idx = 0;
	int i ;
	uint32_t t = time_us_32();	
	uint32_t loopCnt = DHTLIB_TIMEOUT;

	for (i=0;i<5;i++){
		bits[i] = 0;
	}
	// Clear sda
    gpio_set_dir(_pinNumber, GPIO_OUT);
    gpio_put(_pinNumber, 1);
    sleep_ms(DHTLIB_DHT11_WAKEUP);

	// Start signal
    gpio_put(_pinNumber, 0);
    sleep_ms(DHTLIB_DHT11_WAKEUP);
    gpio_set_dir(_pinNumber, GPIO_IN);
	sleep_us(40);

	// Waiting echo
	while(1){
		if(gpio_get(_pinNumber)==0){
			break;
		}
		if((time_us_32()  - t) > loopCnt){
			return DHTLIB_ERROR_TIMEOUT;
		}
	}	
	
	loopCnt = DHTLIB_TIMEOUT;
	t = time_us_32() ;
	// Waiting echo 0 level end
	while(gpio_get(_pinNumber)==0){
		if((time_us_32()  - t) > loopCnt){
			return DHTLIB_ERROR_TIMEOUT;
		}
	}
	
	loopCnt = DHTLIB_TIMEOUT;
	t = time_us_32() ;
	// Waiting echo 1 level end
	while(gpio_get(_pinNumber)==1){
		if((time_us_32()  - t) > loopCnt){
			return DHTLIB_ERROR_TIMEOUT;
		}
	}
	for (i = 0; i<40;i++){
		loopCnt = DHTLIB_TIMEOUT;
		t = time_us_32() ;
		while(gpio_get(_pinNumber)==0){
			if((time_us_32()  - t) > loopCnt)
				return DHTLIB_ERROR_TIMEOUT;
		}
		t = time_us_32() ;
		loopCnt = DHTLIB_TIMEOUT;
		while(gpio_get(_pinNumber)==1){
			if((time_us_32()  - t) > loopCnt){
				return DHTLIB_ERROR_TIMEOUT;
			}
		}
		if((time_us_32()  - t ) > 60){
			bits[idx] |= mask;
		}
		mask >>= 1;
		if(mask == 0){
			mask = 0x80;
			idx++;
		}
	}
	gpio_set_dir (_pinNumber,GPIO_OUT);
	gpio_put(_pinNumber,1);
	//printf("bits:\t%d,\t%d,\t%d,\t%d,\t%d\n",bits[0],bits[1],bits[2],bits[3],bits[4]);

	return DHTLIB_OK;
}
//Function：Read DHT sensor, analyze the data of temperature and humidity
//return：DHTLIB_OK   DHTLIB_ERROR_CHECKSUM  DHTLIB_ERROR_TIMEOUT
int DHT::readDHT11Once(dht_reading *result){
	int rv ; 
	uint8_t sum;
	rv = readSensor();
	if(rv != DHTLIB_OK){
		result->humidity = DHTLIB_INVALID_VALUE;
		result->temp_whole = DHTLIB_INVALID_VALUE;
		result->temp_frac = DHTLIB_INVALID_VALUE;
		return rv;
	}
	
	// result->temperature = bits[2] + bits[3] * 0.1;

 	result->humidity = bits[0];
	result->temp_whole = bits[2];
	result->temp_frac = bits[3];

	sum = bits[0] + bits[1] + bits[2] + bits[3];

	if(bits[4] != sum ){
		return DHTLIB_ERROR_CHECKSUM;
	}

	return DHTLIB_OK;
}

int DHT::readDHT11(dht_reading *result){
	int chk = DHTLIB_INVALID_VALUE;

	for (int i = 0; i < 15; i++){
		chk = readDHT11Once(result);	//read DHT11 and get a return value. Then determine whether data read is normal according to the return value.

		if(chk == DHTLIB_OK){
			return DHTLIB_OK;
		}
		sleep_ms(100);
	}
	return chk;
}



