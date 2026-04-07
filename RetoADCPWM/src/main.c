#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_timer.h" //Para antirrebote
#include "esp_adc/adc_oneshot.h" //ADC
#include "driver/ledc.h" //PWM

#define LEFT 36
#define RIGHT 39
#define SAMPLE_PERIOD_US 1000 // 1000us o 1ms de periodo de muestreo del ADC, para el potenciómetro
#define LEFT_M 25 //PWM para 4N25 del circuito de potencia motor
#define RIGHT_M 26 //PWM para la otra dirección
#define RED 32 //LED rojo al ir hacia la izquierda
#define GREEN 33 //LED verde al ir hacia la derecha
#define A_7seg 19
#define B_7seg 18
#define C_7seg 5
#define D_7seg 17
#define E_7seg 16
#define F_7seg 4
#define G_7seg 0
#define unidad 2
#define decena 5
#define centena 13

adc_oneshot_unit_handle_t adc1_handle;
volatile bool moveleft = false;
volatile bool moveright = false;
volatile int64_t last_time = 0;

static void IRAM_ATTR buttonleft_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time >= 20000){ //Espere 20ms --> antirrebote
        moveleft = true;
        last_time = now;
    }
}

static void IRAM_ATTR buttonright_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time >= 20000){ //Espere 20ms --> antirrebote
        moveright = true;
        last_time = now;
    }
}

void tablaVerdad7seg(bool anode_catode, int count){ //anodo común = 1, catodo común = 0
    bool seg_state;
    if (anode_catode == 1){
        seg_state = 0;
    } else {
        seg_state = 1;
    }

    switch(count){
        case 0:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, !seg_state);
            break;
        case 1:
            gpio_set_level(A_7seg, !seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, !seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, !seg_state);
            gpio_set_level(G_7seg, !seg_state);
            break;
        case 2:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, !seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, seg_state);
            gpio_set_level(F_7seg, !seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 3:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, !seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 4:
            gpio_set_level(A_7seg, !seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, !seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 5:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, !seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 6:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, !seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 7:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, !seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, !seg_state);
            gpio_set_level(G_7seg, !seg_state);
            break;
        case 8:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
        case 9:
            gpio_set_level(A_7seg, seg_state);
            gpio_set_level(B_7seg, seg_state);
            gpio_set_level(C_7seg, seg_state);
            gpio_set_level(D_7seg, seg_state);
            gpio_set_level(E_7seg, !seg_state);
            gpio_set_level(F_7seg, seg_state);
            gpio_set_level(G_7seg, seg_state);
            break;
    }
}

void app_main(void) {
    timer_config_t config = {
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_init(TIMER_GROUP_0, TIMER_1, &config); //Para muestreo del ADC

    adc_oneshot_unit_init_cfg_t init_config ={
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, //Default --> 12 bits
        .atten = ADC_ATTEN_DB_12, //voltaje de referencia: atenuación de 12dB, es decir, 0 a 3.3V
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, //El canal a usar será el 6, entonces conectar pin 34
    &chan_config); 

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT, //Resolución --> 0 a 4095, pues es 12 bits
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t pwmconfig_1 = { //Configuración PWM del GPIO 25
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, 
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEFT_M, //pin 25
        .duty = 0, //Duty inicial
        .hpoint = 0, //Punto de inicio del pulso: 0 (normal)
    };
    ledc_channel_config(&pwmconfig_1);
    
    ledc_channel_config_t pwmconfig_2 = { //Configuración PWM del GPIO 26
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1, 
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = RIGHT_M, //pin 26
        .duty = 0, //Duty inicial
        .hpoint = 0, //Punto de inicio del pulso: 0 (normal)
    };
    ledc_channel_config(&pwmconfig_2);


    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << RED)    |
                        (1ULL << GREEN)  |
                        (1ULL << A_7seg) |
                        (1ULL << B_7seg) |
                        (1ULL << C_7seg) |
                        (1ULL << D_7seg) |
                        (1ULL << E_7seg) |
                        (1ULL << F_7seg) |
                        (1ULL << G_7seg) |
                        (1ULL << unidad) |
                        (1ULL << decena) |
                        (1ULL << centena),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);

    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << LEFT) | (1ULL << RIGHT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, //Debe ser resistencia pull-up analógica externa
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE //interrupción, Flanco de bajada
    };
    gpio_config(&in_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(LEFT, buttonleft_isr, NULL);
    gpio_isr_handler_add(RIGHT, buttonright_isr, NULL);

    gpio_set_level(RED, 0);
    gpio_set_level(GREEN, 0);
    gpio_set_level(A_7seg, 0);
    gpio_set_level(B_7seg, 0);
    gpio_set_level(C_7seg, 0);
    gpio_set_level(D_7seg, 0);
    gpio_set_level(E_7seg, 0);
    gpio_set_level(F_7seg, 0);
    gpio_set_level(G_7seg, 0);
    gpio_set_level(unidad, 0);
    gpio_set_level(decena, 0);
    gpio_set_level(centena, 0);
    gpio_set_level(LEFT_M, 0); //PWM1
    gpio_set_level(RIGHT_M, 0); //PWM2

    uint64_t count = 0;
    uint64_t last_count = 0; // timer para contador
    uint64_t last_mux = 0; // timer para multiplexación
    uint64_t timer_value = 0; // timer para ADC
    int adc_raw, duty1, duty2;
    int pot_p; //porcentaje del potenciómetro
    static bool capture_value = false;
    static int duty_pot; 

    int unidad_val = 0;
    int decena_val = 0;
    int centena_val = 0;

    int display = 4; //unidad = 4, decena = 3, centena = 2

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
    timer_start(TIMER_GROUP_0, TIMER_1);
    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &count);
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &timer_value); //para muestreo del ADC
        if(timer_value >= SAMPLE_PERIOD_US){ //Lectura del valor del potenciómetro
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw); //Lea el valor del canal 6 (pin 34)
            //Valor digital recibido: número entero de 0 a 4095, pues es de 12 bits, entonces niveles = 2^12
            pot_p = (adc_raw * 100) / 4095; //en porcentaje
            timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
        }
        if(moveleft){
            if(!capture_value){
                duty_pot = adc_raw; //Copia del voltaje del potenciómetro
                capture_value = true; //para solo capturar el valor actual una vez
            }
            if(count - last_count >= 500000){ //cada 0.5s decrementa hasta llegar a 0
                if(duty_pot > 0){ //Cambio suave de dirección
                    duty_pot -= 409; //Quitarle al rango un 10%
                    if(duty_pot < 0) duty_pot = 0;
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_pot);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
                    last_count = count;
                } else { //Ya está en 0
                    duty1 = adc_raw;
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty1);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    gpio_set_level(RED, 1);
                    gpio_set_level(GREEN, 0);
                    moveleft = false;
                    capture_value = false;
                }
            }
        } else if(moveright){
            if(!capture_value){
                duty_pot = adc_raw; //Copia del voltaje del potenciómetro
                capture_value = true; //para solo capturar el valor actual una vez
            }
            if(count - last_count >= 500000){ //cada 0.5s decrementa hasta llegar a 0
                if(duty_pot > 0){ //Cambio suave de dirección
                    duty_pot -= 409; //Quitarle al rango un 10%
                    if(duty_pot < 0) duty_pot = 0;
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_pot);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    last_count = count;
                } else { //Ya está en 0
                    duty2 = adc_raw;
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty2);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
                    gpio_set_level(RED, 0);
                    gpio_set_level(GREEN, 1);
                    moveright = false;
                    capture_value = false;
                }
            }
        }

        centena_val = pot_p / 100;
        decena_val = (pot_p / 10) % 10;
        unidad_val = pot_p % 10;
        
        // Multiplexación de los 7seg. cada 2ms:
        if(count - last_mux >= 2000){
            last_mux = count;
            gpio_set_level(unidad, 0);
            gpio_set_level(decena, 0);
            gpio_set_level(centena, 0);
            switch(display){
                case 2:
                    tablaVerdad7seg(0, centena_val);
                    gpio_set_level(centena,1);
                    break;
                case 3:
                    tablaVerdad7seg(0, decena_val);
                    gpio_set_level(decena,1);
                    break;
                case 4:
                    tablaVerdad7seg(0, unidad_val);
                    gpio_set_level(unidad,1);
                    break;
            }
            display--;
            if(display < 2){
                display = 4;
            }
        }
    }
}