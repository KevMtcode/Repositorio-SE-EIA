#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_timer.h" //timer del sistema que ya viene corriendo, cuenta en us

#define A_7seg 32
#define B_7seg 33
#define C_7seg 25
#define D_7seg 26
#define E_7seg 27
#define F_7seg 14
#define G_7seg 12
#define unidad 22
#define decena 23
#define Pmas 19
#define Pmenos 18
#define CAL 5
#define RST 4
#define RED 0
#define GREEN 2
#define YELLOW 15

volatile bool reset_flag = false;
volatile bool increase = false;
volatile bool decrease = false;
volatile bool calc = false;
volatile int64_t last_time_pmas = 0;
volatile int64_t last_time_pmenos = 0;
volatile int64_t last_time_cal = 0;
volatile int64_t last_time_rst = 0;

static void IRAM_ATTR pmas_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_pmas >= 20000){ //20ms
        increase = true;
        last_time_pmas = now;
    }
}

static void IRAM_ATTR pmenos_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_pmenos >= 20000){ //20ms
        decrease = true;
        last_time_pmenos = now;
    }
}

static void IRAM_ATTR cal_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_cal >= 20000){ //20ms
        calc = true;
        last_time_cal = now;
    }
}

static void IRAM_ATTR buttonreset_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_rst >= 20000){ //20ms
        reset_flag = true;
        last_time_rst = now;
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

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << A_7seg) |
                        (1ULL << B_7seg) |
                        (1ULL << C_7seg) |
                        (1ULL << D_7seg) |
                        (1ULL << E_7seg) |
                        (1ULL << F_7seg) |
                        (1ULL << G_7seg) |
                        (1ULL << unidad) |
                        (1ULL << decena) |
                        (1ULL << RED) |
                        (1ULL << GREEN) |
                        (1ULL << YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);

    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << Pmas) | 
                        (1ULL << Pmenos) |
                        (1ULL << CAL) | 
                        (1ULL << RST),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE //Flanco de bajada
    };

    gpio_config(&in_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(Pmas, pmas_isr, NULL);
    gpio_isr_handler_add(Pmenos, pmenos_isr, NULL);
    gpio_isr_handler_add(CAL, cal_isr, NULL);
    gpio_isr_handler_add(RST, buttonreset_isr, NULL);

    gpio_set_level(A_7seg, 0);
    gpio_set_level(B_7seg, 0);
    gpio_set_level(C_7seg, 0);
    gpio_set_level(D_7seg, 0);
    gpio_set_level(E_7seg, 0);
    gpio_set_level(F_7seg, 0);
    gpio_set_level(G_7seg, 0);
    gpio_set_level(unidad, 0);
    gpio_set_level(decena, 0);
    gpio_set_level(RED, 0);
    gpio_set_level(GREEN, 0);
    gpio_set_level(YELLOW, 0);

    uint64_t count = 0;
    uint64_t last_mux = 0; // timer para multiplexación
    int unidad_val = 0;
    int decena_val = 0;
    float un; //para unir la unidad a la decena
    int peso;
    int d;

    bool display = 0; //unidad = 0, decena = 1

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &count);
        if(reset_flag){
            unidad_val = 0;
            decena_val = 0;
            timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
            gpio_set_level(RED, 0);
            gpio_set_level(GREEN, 0);
            gpio_set_level(YELLOW, 0);
            calc = false;
            reset_flag = false;
        }
        if(increase && !calc){
            if(unidad_val == 0 && decena_val == 2){
                //no haga nada
            } else{
                unidad_val++;
                if(unidad_val > 9){
                    unidad_val = 0; //Añado este cambio
                    decena_val++;
                }
            }
            increase = false;
        }
        if(decrease && !calc){
            if(unidad_val == 0 && decena_val == 0){
                //no haga nada
            } else{
                unidad_val--;
                if(unidad_val <= 0){
                    unidad_val = 9; //Añado este cambio
                    decena_val--;
                }
            }
            decrease = false;
        }
        un = unidad_val/10;
        peso = (un + decena_val)*10; //e.g. (0.5 + 1)*10 = 15
        if(calc){
            d = (3*peso+5)-peso;
            unidad_val = d % 10;
            decena_val = d / 10;
            if(d >= 0 && d <= 7){
                gpio_set_level(RED, 1);
                gpio_set_level(GREEN, 0);
                gpio_set_level(YELLOW, 0);
            }
            if(d >= 8 && d <= 14){
                gpio_set_level(RED, 0);
                gpio_set_level(GREEN, 1);
                gpio_set_level(YELLOW, 0);
            }
            if(d >= 15 && d <= 20){ //cambio: d en vez de peso
                gpio_set_level(RED, 0);
                gpio_set_level(GREEN, 0);
                gpio_set_level(YELLOW, 1);
            }
        }
        
        // Multiplexación cada 2ms:
        if(count - last_mux >= 2000){

            last_mux = count;
            gpio_set_level(unidad,0);
            gpio_set_level(decena,0);

            if(display == 0){
                gpio_set_level(unidad,1);
                gpio_set_level(decena,0);
                tablaVerdad7seg(0, unidad_val);
            }
            else{
                gpio_set_level(unidad,0);
                gpio_set_level(decena,1);
                tablaVerdad7seg(0, decena_val);
            }

            display = !display;
        }
    }
}