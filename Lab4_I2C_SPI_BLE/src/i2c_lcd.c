#include "i2c_lcd.h"
#include "system_lib.h"


void i2c_init(void){
    i2c_config_t config = { //El ESP32 va a ser el MAESTRO 
        .mode = I2C_MODE_MASTER,
        .sda_io_num = LCD_SDA,
        .scl_io_num = LCD_SCL,
        .sda_pullup_en = GPIO_PULLDOWN_DISABLE,
        .scl_pullup_en = GPIO_PULLDOWN_DISABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    i2c_param_config(I2C_PORT, &config);
    i2c_driver_install(I2C_PORT, config.mode, 0, 0, 0);
}
static void pcf8574_write(uint8_t data){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));

    i2c_cmd_link_delete(cmd);
}

static void lcd_pulse_enable(uint8_t data){
    pcf8574_write(data); //Enable = 0
    esp_rom_delay_us(1);

    pcf8574_write(data | LCD_EN); //Enable = 1
    esp_rom_delay_us(1);

    pcf8574_write(data);
    esp_rom_delay_us(100);
}

static void lcd_send_nibble(uint8_t nibble, uint8_t rs){
    uint8_t data = LCD_BL;
    if (rs){
        data |= LCD_RS;
    }
    data |= (nibble << 4);
    printf("Nibble: 0x%02X -> Data: 0x%02X\n", nibble, data);

    lcd_pulse_enable(data);
}

static void lcd_send_byte(uint8_t value, uint8_t rs){
    lcd_send_nibble(value >> 4, rs);
    lcd_send_nibble(value & 0x0F, rs);
}

static void lcd_command(uint8_t cmd){
    lcd_send_byte(cmd, 0);
}
void lcd_data(uint8_t data){
    lcd_send_byte(data, LCD_RS);
}
void lcd_print(const char *str){
    while(*str){
        lcd_data(*str++);
    }
}
void lcd_set_cursor(uint8_t row, uint8_t col){
    //Fila 0: 0x00; Fila 1: 0x40
    uint8_t addr = col;
    if(row == 1)
    {
        addr += 0x40;
    }
    lcd_command(0x80 | addr);
}

void lcd_init(void){
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_send_nibble(0x03, 0);
    esp_rom_delay_us(150);

    lcd_send_nibble(0x03, 0);
    esp_rom_delay_us(150);

    lcd_send_nibble(0x02, 0);

    lcd_command(0x28);      // 4 bits, 2 líneas
    lcd_command(0x08);      // Display OFF
    //lcd_command(0x01);      // Clear
    vTaskDelay(pdMS_TO_TICKS(2));

    lcd_command(0x06);      // Cursor avanza
    lcd_command(0x0C);      // Display ON
}

void lcd_clear(void){
    lcd_set_cursor(0,0);
    for(int i=0; i < 16; i++){
        lcd_data(' ');
    }
    lcd_set_cursor(1,0);
    for(int i=0; i < 16; i++){
        lcd_data(' ');
    }
    lcd_set_cursor(0,0);
}