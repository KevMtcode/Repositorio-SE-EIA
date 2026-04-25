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
#define SAMPLE_PERIOD_US 1000 // 1000us o 1ms de periodo de muestreo del ADC, para la LDR y LM35
#define LightB 23 //bombilla 120VAC 60Hz
#define PLED 22 //pin para PWM del LED de potencia 3.2V 3W 
#define OUT1 19 //IN1 del puente H doble L298N
#define OUT2 18 //IN2 del puente H doble L298N
#define OUT3 5 //IN3 del puente H doble L298N
#define OUT4 17 //IN4 del puente H doble L298N

volatile bool cw_ccw = 0; // horario o antihorario
adc_oneshot_unit_handle_t adc1_handle;

uint8_t turn[8][4] = {{1, 0, 1, 0},
                   {1, 0, 0, 0}, // secuencia half-step (2 filas = 1 paso)
                   {1, 0, 0, 1},
                   {0, 0, 0, 1}, 
                   {0, 1, 0, 1}, 
                   {0, 1, 0, 0},
                   {0, 1, 1, 0},
                   {0, 0, 1, 0}};

void stepper_motor(bool direction){
    static int i = 0;
    if(direction == 0){
        i = (i + 1) % 8;
    } else {
        i = (i - 1 + 8) % 8;
    }

    gpio_set_level(OUT1, turn[i][0]);
    gpio_set_level(OUT2, turn[i][1]);
    gpio_set_level(OUT3, turn[i][2]);
    gpio_set_level(OUT4, turn[i][3]);
}

static bool IRAM_ATTR timer_isr(void *arg){ //en interrupciones de timers
    stepper_motor(cw_ccw);
    return false; //deben retornar función booleana
}
void app_main(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,  //poner en platformio.ini --> monitor_speed = 9600, es decir, velocidad del computador debe ser la misma del ESP32
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_param_config(UART_PORT, &uart_config); // & para mandar de una vez comox pointer
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

    timer_config_t config_i = { //Timer de interrupción para motor stepper
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN, //alarma de timer de interrupción
        .auto_reload = true,  
    };
    timer_init(TIMER_GROUP_1, TIMER_0, &config_i);
    timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
    uint64_t alarm = 1000000;
    timer_set_alarm_value(
        TIMER_GROUP_1,
        TIMER_0,
        alarm 
    );
    timer_isr_callback_add(
        TIMER_GROUP_1,
        TIMER_0,
        timer_isr,
        NULL,
        0
    );
    timer_enable_intr(TIMER_GROUP_1, TIMER_0);
    timer_start(TIMER_GROUP_1, TIMER_0);

    adc_oneshot_unit_init_cfg_t init_config ={ //Se usará solo ADC1
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, //Default --> 12 bits
        .atten = ADC_ATTEN_DB_12, //voltaje de referencia: atenuación de 12dB, es decir, 0 a 3.3V
    };
    
    //Canal 6: GPIO34, LDR
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6,
    &chan_config); 
    //Canal 7: GPIO35, LM35
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7,
    &chan_config); 

    ledc_timer_config_t ledc_timer = { //por acá está la frecuencia del PWM
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT, //Resolución --> 0 a 4095, pues es 12 bits
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t pwmconfig_1 = { //Configuración PWM para los LEDs de potencia
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, 
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PLED, //pin 22
        .duty = 0, //Duty inicial
        .hpoint = 0, //Punto de inicio del pulso: 0 (normal)
    };
    ledc_channel_config(&pwmconfig_1);

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << LightB) |
                        (1ULL << OUT1)   |
                        (1ULL << OUT2)   |
                        (1ULL << OUT3)   |
                        (1ULL << OUT4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);

    gpio_set_level(LightB, 0);
    gpio_set_level(OUT1, 0);
    gpio_set_level(OUT2, 0);
    gpio_set_level(OUT3, 0);
    gpio_set_level(OUT4, 0);

    uint64_t count = 0;
    uint64_t last_print_time = 0; // timer para mostrar las variables en consola
    uint64_t timer_value = 0; // timer para ADCs
    int adc_raw_LDR, adc_raw_LM35, dutyC = 0;
    int dutyC_p = 0; //Duty Cycle en porcentaje 
    int ni = 0; //ilumanción según LDR
    int st = 100; //steps
    int T = 0;
    int temp = 0;
    static int last_st = -1;

    //Es más eficiente trabajar con punteros de char, en vez de cadenas de texto de string:
    char *mensaje_inicio = "\nControl de temperatura e iluminación\n"; //* es para crearlo como pointer,
    uart_write_bytes(UART_PORT, mensaje_inicio, strlen(mensaje_inicio)); //para mostrar en consola
    int Tc = 0; //Temperatura de control

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);
    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &count); //para mostrar en la consola
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &timer_value); //para muestreo del ADC
        if(timer_value >= SAMPLE_PERIOD_US){
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw_LDR);
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &adc_raw_LM35);
            //Valor digital recibido: número entero de 0 a 4095, pues es de 12 bits, entonces niveles = 2^12
            
            ni = (adc_raw_LDR * 100) / 4095; //en porcentaje
            timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
        }
        if(ni <= 20){
            dutyC_p = 100;
        } else if(ni > 20 && ni <= 30){
            dutyC_p = 80;
        } else if(ni > 30 && ni <= 40){
            dutyC_p = 60;
        } else if(ni > 40 && ni <= 60){
            dutyC_p = 50;
        } else if(ni > 60 && ni <= 80){
            dutyC_p = 30;
        } else if(ni > 80){
            dutyC_p = 0;
        }
        dutyC = (dutyC_p*4095) / 100; //de 0 a 4095
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyC);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        T = (adc_raw_LM35*330) / 4095; //330 es 3.3V * 100; T en voltaje * 100°C / 1°C, pues LM35 entrega 10mV/1°C
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
                    Tc = temp;
                }
                idx = 0; //reincia índice para recibir nuevo número
            } else {
                if(idx < sizeof(rx_buffer)-1){ //si no es enter, se recibe un caracter y sin desbordar el buffer
                    rx_buffer[idx++] = c; //guarda el caracter, luego se incrementa el índice para otro caracter
                }
            }
        }
        if(count - last_print_time >= 3000000){ //3s en pantalla y actualiza valores
            char *msg = "SET_TEMP: ";
            uart_write_bytes(UART_PORT, msg, strlen(msg));
            char num1[16]; //[16] es que quiero guardar un espacio de 16 bytes para ese caracter
            sprintf(num1, "%d °C\n", Tc);
            uart_write_bytes(UART_PORT, num1, strlen(num1));
            
            char *msg1 = "Temperatura medida: ";
            uart_write_bytes(UART_PORT, msg1, strlen(msg1));
            char num[16]; //[16] es que quiero guardar un espacio de 16 bytes para ese caracter
            sprintf(num, "%d °C\n", T);
            uart_write_bytes(UART_PORT, num, strlen(num));

            char *msg2 = "Iluminación: ";
            uart_write_bytes(UART_PORT, msg2, strlen(msg2));
            char num2[16];
            sprintf(num2, "%d %%\n", ni);
            uart_write_bytes(UART_PORT, num2, strlen(num2));

            last_print_time = count;
        }
        if(T >= Tc-1 && T <= Tc+1){ //apagado
            gpio_set_level(LightB, 0);
            gpio_set_level(OUT1, 0);
            gpio_set_level(OUT2, 0);
            gpio_set_level(OUT3, 0);
            gpio_set_level(OUT4, 0);
        } else if (T < Tc-1){ //horario, 100 steps/s
            gpio_set_level(LightB, 1);
            st = 100; //2 filas son 1 paso
            cw_ccw = 0;
        } else if (T > Tc+1 && T < Tc+3){ //antihorario, 100 steps/s
            gpio_set_level(LightB, 0);
            st = 100;
            cw_ccw = 1;
        } else if (T >= Tc+3 && T <= Tc+5){ //antihorario, 300 steps/s
            gpio_set_level(LightB, 0);
            st = 300;
            cw_ccw = 1;
        } else if (T > Tc+5){ //antihorario, 600 steps/s
            gpio_set_level(LightB, 0);
            st = 600;
            cw_ccw = 1;
        }

        if(st != last_st){
            timer_disable_intr(TIMER_GROUP_1, TIMER_0);
            if(st > 0){
                uint64_t alarm = 1000000 / (st*8); //100 steps * 8líneas de la tabla de verdad turn
                timer_pause(TIMER_GROUP_1, TIMER_0);
                timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
                timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, alarm);
                timer_start(TIMER_GROUP_1, TIMER_0);
            } else {
                timer_pause(TIMER_GROUP_1, TIMER_0);
            }
            timer_enable_intr(TIMER_GROUP_1, TIMER_0);
            last_st = st;
        }
        vTaskDelay(pdMS_TO_TICKS(10)); //para no saturar la CPU
    }
}