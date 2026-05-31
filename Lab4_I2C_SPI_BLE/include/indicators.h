#pragma once //para evitar que un archivo header sea incluido varias veces
#define BUZZER_ON gpio_set_level(BUZZER, 1)
#define BUZZER_OFF gpio_set_level(BUZZER, 0)
#define RED_ON gpio_set_level(RED, 1)
#define RED_OFF gpio_set_level(RED, 0)
#define GREEN_ON gpio_set_level(GREEN, 1)
#define GREEN_OFF gpio_set_level(GREEN, 0)
#define BLUE_ON gpio_set_level(BLUE, 1)
#define BLUE_OFF gpio_set_level(BLUE, 0)
void indicators_init(void);