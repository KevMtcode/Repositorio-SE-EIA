#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/timer.h"
#include "esp_adc/adc_oneshot.h" //ADC

#define UART_PORT UART_NUM_0
#define SAMPLE_PERIOD_US 1000 //Para el ADC, f = 1kHz --> T = 10^-3 s = 1ms = 1000us
#define PIN_MOSI 32
#define PIN_CLK 25
#define PIN_CS 33

spi_device_handle_t mcp4132;
adc_oneshot_unit_handle_t adc1_handle;

void spi_bus_init(void){
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 1000000, //1MHz
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &dev, &mcp4132);




}

void app_main(void) {
    spi_bus_init();
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0);

    timer_config_t config = {
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false,
    }
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, //Default = 12 bits
        .atten = ADC_ATTEN_DB_12, //Voltaje de referencia = 3.3V
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_config);

    uint64_t count = 0;
    uint64_t last_count = 0; //Para UART
    uint64_t timer_value = 0; //Para timer ADC
    int adc_raw;
    float signal;
    int n;

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);

    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &count); //mostrar en consola con UART
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &timer_value); //muestreo ADC
        if(timer_value >= SAMPLE_PERIOD_US){
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
            signal = ((float)adc_raw * 3.30f) / 4095.0f; //convertir a voltaje

            timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
        }
        if(signal > 1.40f){
            n = 95;
        } else if (signal < 0.90f){
            n = 42;
        }


    }

}