#include <math.h>
#include <stdint.h>
#include <string.h> //para los caracteres
#include <stdio.h>
#include <stdlib.h> //Para atoi() --> ASCII to integer
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_adc/adc_oneshot.h" //ADC
#include "driver/ledc.h" //PWM
#include "driver/uart.h" //Comunicación serial UART

#define UART_PORT UART_NUM_0 
#define SAMPLE_PERIOD_US 1000 // 1000us o 1ms de periodo de muestreo del ADC, para el NTC
#define BZ 22 //buzzer
#define LED_G 32
#define LED_B 33

adc_oneshot_unit_handle_t adc1_handle;

void app_main(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_param_config(UART_PORT, &uart_config); // & para mandar de una vez como pointer
    uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0);

    timer_config_t config = {
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config); //Para mostrar las variables al usuario en consola
    timer_init(TIMER_GROUP_0, TIMER_1, &config); //Para muestreo del ADC 
    timer_init(TIMER_GROUP_1, TIMER_0, &config); //cambio de estado

    adc_oneshot_unit_init_cfg_t init_config ={ //Se usará solo ADC1
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, //Default --> 12 bits
        .atten = ADC_ATTEN_DB_12, //voltaje de referencia: atenuación de 12dB, es decir, 0 a 3.3V
    };
    
    //Canal 6: GPIO34, NTC
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6,
    &chan_config); 

    ledc_timer_config_t ledc_timer = { //por acá está la frecuencia del PWM
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT, //Resolución --> 0 a 4095, pues es 12 bits
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t pwmconfig_1 = { //Configuración PWM para Buzzer
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, 
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BZ,
        .duty = 0, //Duty inicial
        .hpoint = 0, //Punto de inicio del pulso: 0 (normal)
    };
    ledc_channel_config(&pwmconfig_1);

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << LED_G) |
                        (1ULL << LED_B),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);

    gpio_set_level(LED_G, 0);
    gpio_set_level(LED_B, 0);

    uint64_t count = 0;
    uint64_t count_state = 0; //forma 1 de timers
    uint64_t timer_value = 0; // timer para ADCs
    int adc_raw_NTC;
    int dutyC = 0;
    int dutyC_p = 0; //Duty Cycle en porcentaje 
    float termistor = 0;
    int temp = 0;

    //Es más eficiente trabajar con punteros de char, en vez de cadenas de texto de string:
    char *mensaje_inicio = "\nIngresar umbral de detección:\n"; //* es para crearlo como pointer,
    uart_write_bytes(UART_PORT, mensaje_inicio, strlen(mensaje_inicio)); //para mostrar en consola
    float umbral = 0;
    bool apnea = false;
    int cont_ins = 0;
    int cont_exp = 0;
    int cont_bz = 0;

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);
    timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
    timer_start(TIMER_GROUP_1, TIMER_0);

    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &count); //para mostrar en la consola
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &timer_value); //para muestreo del ADC
        timer_get_counter_value(TIMER_GROUP_1, TIMER_0, &count_state); //para el buzzer indicando apnea 
        if(timer_value >= SAMPLE_PERIOD_US){
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw_NTC);
            //Valor digital recibido: número entero de 0 a 4095, pues es de 12 bits, entonces niveles = 2^12
            termistor = (adc_raw_NTC * 3.3) / 4095.0; // adc * (3.3*10)/(4095*10) para considerar los decimales en la operación con entero
            timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
        }
        
        dutyC = (dutyC_p*4095) / 100; //de 0 a 4095
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyC);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        //Ingresar dos caracteres:
        static char rx_buffer[16];
        static int idx = 0; //índice o posición donde se guarda el caracter
        uint8_t c; //caracter
        int n = uart_read_bytes(UART_PORT, &c, 1, 0); // con 0, no bloque por tiempo de espera
        if(n > 0){
            if(c == '\n' || c == '\r'){ //Si el caracter es enter (\n o \r). el usuario ya terminó de escribir
                rx_buffer[idx] = '\0'; //agregar caracter nulo para que atoi() funcione bien
                temp = atoi(rx_buffer); //Convierte string a entero 
                if(temp > 0 && temp < 100){
                    umbral = temp / 10.0; //ingresar el valor en decenas, es decir, 15 --> 1.5V
                }
                idx = 0; //reincia índice para recibir nuevo número
            } else {
                if(idx < sizeof(rx_buffer)-1){ //si no es enter, se recibe un caracter y sin desbordar el buffer
                    rx_buffer[idx++] = c; //guarda el caracter, luego se incrementa el índice para otro caracter
                }
            }
        }
        if(termistor <= umbral){ 
            dutyC_p = 0; //porcentaje del buzzer
            cont_exp = 0;
            cont_ins ++;
            if(cont_ins < 2){
                apnea = false;
                cont_bz = 0;
                char *msg = "Umbral: ";
                uart_write_bytes(UART_PORT, msg, strlen(msg));
                char num1[16]; //[16] es que quiero guardar un espacio de 16 bytes para ese caracter
                sprintf(num1, "%f V\n", umbral);
                uart_write_bytes(UART_PORT, num1, strlen(num1));
                
                char *msg1 = "INSPIRANDO\n";
                uart_write_bytes(UART_PORT, msg1, strlen(msg1));
                gpio_set_level(LED_G, 1);
                gpio_set_level(LED_B, 0);
                timer_start(TIMER_GROUP_1, TIMER_0);
            } else if(count_state >= 10000000){
                apnea = true;
                count_state = 0;
                timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
                timer_pause(TIMER_GROUP_1, TIMER_0);
            }
        } else if (termistor > umbral){
            dutyC_p = 0; //porcentaje del buzzer
            cont_ins = 0;
            cont_exp ++;
            if(cont_exp < 2){
                apnea = false;
                cont_bz = 0;
                char *msg = "Umbral: ";
                uart_write_bytes(UART_PORT, msg, strlen(msg));
                char num1[16]; //[16] es que quiero guardar un espacio de 16 bytes para ese caracter
                sprintf(num1, "%f V\n", umbral);
                uart_write_bytes(UART_PORT, num1, strlen(num1));

                char *msg2 = "EXHALANDO\n";
                uart_write_bytes(UART_PORT, msg2, strlen(msg2));
                gpio_set_level(LED_G, 0);
                gpio_set_level(LED_B, 1);
                timer_start(TIMER_GROUP_1, TIMER_0);
            } else if(count_state >= 10000000){
                apnea = true;
                count_state = 0;
                timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
                timer_pause(TIMER_GROUP_1, TIMER_0);
            }
        }

        if(apnea){
            cont_bz++;
            if(cont_bz > 100){ //evitar desbordamiento
                cont_bz = 2;
            }
            if(cont_bz < 2){
                char *msg3 = "ALERTA: APNEA DETECTADA\n";
                uart_write_bytes(UART_PORT, msg3, strlen(msg3));
            }
            gpio_set_level(LED_G, 0);
            gpio_set_level(LED_B, 0);
            dutyC_p = 75; //porcentaje del buzzer
        }
        dutyC = (dutyC_p*4095) / 100; //de 0 a 4095
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyC);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(100)); //para no saturar la CPU
    }

}