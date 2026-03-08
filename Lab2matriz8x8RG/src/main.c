#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LEFT 36
#define RIGHT 39
#define ROW1 0 //ROW 1, enumerados de arriba hacia abajo
#define ROW2 1
#define ROW3 2
#define ROW4 3
#define ROW5 4
#define ROW6 5
#define ROW7 6
#define ROW8 7
#define COL_R1 8 //COL 1, enumerados de izquierda a derecha
#define COL_G1 9
#define COL_R2 10
#define COL_G2 11
#define COL_R3 12
#define COL_G3 13
#define COL_R4 14
#define COL_G4 15
#define COL_R5 16
#define COL_G5 17
#define COL_R6 18
#define COL_G6 19
#define COL_R7 21
#define COL_G7 22
#define COL_R8 23
#define COL_G8 25

static inline bool btn_pressed(gpio_num_t pin) { //Para revertir la lógica inversa, por la resistencia de pull-up
    if(gpio_get_level(pin) == 0) {
        return true;
    }
    else {
        return false;
    }
}

void turnoff(){
    gpio_set_level(ROW1, 0);
    gpio_set_level(ROW2, 0);
    gpio_set_level(ROW3, 0);
    gpio_set_level(ROW4, 0);
    gpio_set_level(ROW5, 0);
    gpio_set_level(ROW6, 0);
    gpio_set_level(ROW7, 0);
    gpio_set_level(ROW8, 0);
    gpio_set_level(COL_R1, 0);
    gpio_set_level(COL_G1, 0);
    gpio_set_level(COL_R2, 0);
    gpio_set_level(COL_G2, 0);
    gpio_set_level(COL_R3, 0);
    gpio_set_level(COL_G3, 0);
    gpio_set_level(COL_R4, 0);
    gpio_set_level(COL_G4, 0);
    gpio_set_level(COL_R5, 0);
    gpio_set_level(COL_G5, 0);
    gpio_set_level(COL_R6, 0);
    gpio_set_level(COL_G6, 0);
    gpio_set_level(COL_R7, 0);
    gpio_set_level(COL_G7, 0);
    gpio_set_level(COL_R8, 0);
    gpio_set_level(COL_G8, 0);
}

int column_G(int c){
    switch (c){
        case 1:
            return COL_G1;
            break;
        case 2:
            return COL_G2;
            break;
        case 3:
            return COL_G3;
            break;
        case 4:
            return COL_G4;
            break;
        case 5:
            return COL_G5;
            break;
        case 6:
            return COL_G6;
            break;
        case 7:
            return COL_G7;
            break;
        case 8:
            return COL_G8;
            break;
    }
}

int column_R(int c){
    switch (c){
        case 1:
            return COL_R1;
            break;
        case 2:
            return COL_R2;
            break;
        case 3:
            return COL_R3;
            break;
        case 4:
            return COL_R4;
            break;
        case 5:
            return COL_R5;
            break;
        case 6:
            return COL_R6;
            break;
        case 7:
            return COL_R7;
            break;
        case 8:
            return COL_R8;
            break;
    }
}

int row(int r){
    switch (r){
        case 1:
            return ROW1;
            break;
        case 2:
            return ROW2;
            break;
        case 3:
            return ROW3;
            break;
        case 4:
            return ROW4;
            break;
        case 5:
            return ROW5;
            break;
        case 6:
            return ROW6;
            break;
        case 7:
            return ROW7;
            break;
        case 8:
            return ROW8;
            break;
    }
}

void shipmotion(bool left, bool right){
    int m = 0;
    gpio_set_level(ROW8, 1); //Posición inicial de la nave
    gpio_set_level(COL_G1, 1);
    left = btn_pressed(LEFT);
    right = btn_pressed(RIGHT);
    for (int i=1; i<=8; i++){
        if (gpio_get_level(column_G(i)) == 1){
            vTaskDelay(pdMS_TO_TICKS(100)); //Antirrebote del botón
            if (left == 1){
                if (i==1){ 
                    continue; //no moverse hacia la izquierda si se está en el extremo izquierdo
                } else {
                    m = i-1;
                }
            } else if (right == 1){
                if (i==8){
                    continue; //no moverse hacia la derecha si se está en el extremo derecho
                } else {
                    m = i+1;
                }
            } else{
                m = i;
            }
            gpio_set_level(column_G(i), 0);
            gpio_set_level(column_G(m), 1);
        }
    }
}

void shipattack(){
    for (int i=1; i<=8; i++){
        if (gpio_get_level(column_G(i)) == 1){ //Donde la nave esté
            for (int j=7; j<=1; j--){
                if (gpio_get_level(row(j-1)) == 1){ //Hay un enemigo adelante?
                    gpio_set_level(row(j), 1);
                    gpio_set_level(column_R(i), 1);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    gpio_set_level(row(j), 0); 
                    gpio_set_level(column_R(i), 0);
                    vTaskDelay(pdMS_TO_TICKS(250));
                    gpio_set_level(row(j-1), 0); //Le dio al enemigo
                } else {
                    gpio_set_level(row(j), 1);
                    gpio_set_level(column_R(i), 1); //Dispare
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                gpio_set_level(row(j), 0);
                gpio_set_level(column_R(i), 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1500)); //Cada 1.5s disparará
    }
}

void enemiesmotion(){
    
}

void app_main() {
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << ROW1) |
                        (1ULL << ROW2) |
                        (1ULL << ROW3) |
                        (1ULL << ROW4) |
                        (1ULL << ROW5) |
                        (1ULL << ROW6) |
                        (1ULL << ROW7) |
                        (1ULL << ROW8) |
                        (1ULL << COL_R1) |
                        (1ULL << COL_G1) |
                        (1ULL << COL_R2) |
                        (1ULL << COL_G2) |
                        (1ULL << COL_R3) |
                        (1ULL << COL_G3) |
                        (1ULL << COL_R4) |
                        (1ULL << COL_G4) |
                        (1ULL << COL_R5) |
                        (1ULL << COL_G5) |
                        (1ULL << COL_R6) |
                        (1ULL << COL_G6) |
                        (1ULL << COL_R7) |
                        (1ULL << COL_G7) |
                        (1ULL << COL_R8) |
                        (1ULL << COL_G8),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);
    turnoff();

    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << LEFT) | (1ULL << RIGHT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&in_cfg);
    
    bool moveleft;
    bool moveright;

    while(1){
        shipmotion(moveleft, moveright);
        shipattack();
    }

}