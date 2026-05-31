#include "indicators.h"
#include "system_lib.h"
void indicators_init(void){
    gpio_config_t out_cfg = { 
        .pin_bit_mask = (1ULL << BUZZER | 1ULL << RED |
                         1ULL << GREEN | 1ULL << BLUE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);
    
    timer_config_t timer_conf = {
        .divider = 80, //1 tick cada 1us --> prescaler
        .counter_dir = TIMER_COUNT_UP, 
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf);
    
}