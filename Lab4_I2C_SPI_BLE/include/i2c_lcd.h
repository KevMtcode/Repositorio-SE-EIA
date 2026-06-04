#pragma once //para evitar que un archivo header sea incluido varias veces
#include <stdint.h>
#define TAG "LCD"
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

#define LCD_ADDR 0x27
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

void i2c_init(void);
void lcd_data(uint8_t data);
void lcd_print(const char *str);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_init(void);
void lcd_clear(void);