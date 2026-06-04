#pragma once //para evitar que un archivo header sea incluido varias veces
// Dirección I2C del DS1307: fija en 0x68 (7 bits: 1101000) 
#define DS1307_ADDR 0x68 
#include "esp_log.h"  
// Dirección del primer registro de tiempo (segundos) 
#define DS1307_REG_SEC  0x00 

#define I2C_PORT_RTC I2C_NUM_1 //bus del RTC
 
// Frecuencia del bus: 100000 Hz = 100 kHz 
#define I2C_FREQ_HZ 100000 


uint8_t decimal2bcd(uint8_t decimal);
uint8_t bcd2decimal(uint8_t bcd);
void ds1307_init(void);
void ds1307_write_hours(uint8_t seconds, uint8_t minutes, uint8_t hour, 
                        uint8_t day, uint8_t date, uint8_t month, uint8_t year);
void ds1307_read_time(void);