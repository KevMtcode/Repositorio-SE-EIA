#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

//ESP-32D WROOM XX5R69
#define LEFT 36
#define RIGHT 39 
#define ROW2 0 //fila 2 (no se usará la fila 1), enumerados de arriba hacia abajo
#define ROW3 2 
#define ROW4 3
#define ROW5 4
#define ROW6 5
#define ROW7 12
#define ROW8 13
#define COL_R2 14 //columna 2 (no se usará la columna 1), enumerados de izquierda a derecha
#define COL_G2 15
#define COL_R3 16
#define COL_G3 17
#define COL_R4 18
#define COL_G4 19
#define COL_R5 21
#define COL_G5 22
#define COL_R6 23
#define COL_G6 25
#define COL_R7 26
#define COL_G7 27
#define COL_R8 32
#define COL_G8 33
int shiplives = 3;

static inline bool btn_pressed(gpio_num_t pin) { //Para revertir la lógica inversa, por la resistencia de pull-up
    if(gpio_get_level(pin) == 0) {
        return true;
    }
    else {
        return false;
    }
}

void turnoff(){
    gpio_set_level(ROW2, 0);
    gpio_set_level(ROW3, 0);
    gpio_set_level(ROW4, 0);
    gpio_set_level(ROW5, 0);
    gpio_set_level(ROW6, 0);
    gpio_set_level(ROW7, 0);
    gpio_set_level(ROW8, 0);
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

void restart(){
    turnoff();
    gpio_set_level(ROW8, 1); //Posición inicial de la nave
    gpio_set_level(COL_G2, 1);
    gpio_set_level(ROW2, 1);
    gpio_set_level(ROW3, 1);
    gpio_set_level(ROW4, 1);
    gpio_set_level(COL_G3, 1); //Posición inicial en columnas de los enemigos
    gpio_set_level(COL_G5, 1);
    gpio_set_level(COL_G7, 1);

}

int column_G(int c){
    switch (c){
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
    for (int i=2; i<=8; i++){
        if (gpio_get_level(column_G(i)) == 1){
            vTaskDelay(pdMS_TO_TICKS(100)); //Antirrebote del botón
            if (left == 1){
                if (i==2){ 
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
    for (int i=2; i<=8; i++){
        if (gpio_get_level(column_G(i)) == 1){ //Donde la nave esté
            for (int j=7; j>=2; j--){
                gpio_set_level(row(j), 1);
                gpio_set_level(column_R(i), 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(row(j), 0); 
                gpio_set_level(column_R(i), 0);
                if (gpio_get_level(row(j-1)) == 1){ //Hay un enemigo adelante?
                    gpio_set_level(row(j-1), 0); //Le dio al enemigo
                } 
                vTaskDelay(pdMS_TO_TICKS(1500)); //Cada 1.5s disparará
            }
        }
    }
}

void enemiesmotion(){
    //Movimiento hacia la derecha:
    for (int i=3; i<=7; i++){ //Revisa hasta el 7 para que no vaya a moverlo hacia la derecha cuando esté en la esquina
        for (int j=2; j<=7; j++){ 
            if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                gpio_set_level(column_G(i), 0);
                gpio_set_level(column_G(i+1), 1);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    //Primer salto hacia abajo:
    for (int i=2; i<=8; i++){ 
        for (int j=7; j>=2; j--){ //Detección de abajo hacia arriba para las filas
            if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                gpio_set_level(row(j), 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(row(j+1), 1);
            }
        }
    }
    //Movimiento hacia la izquierda:
    for (int i=8; i>=4; i--){ //Revisa hasta el 4 por la misma razón de llegar hasta la esquina
        for (int j=1; j<=7; j++){ 
            if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                gpio_set_level(column_G(i), 0);
                gpio_set_level(column_G(i-1), 1);
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    //Segundo salto hacia abajo:
    for (int i=2; i<=8; i++){ 
        for (int j=7; j>=2; j--){ //Detección de abajo hacia arriba para las filas
            if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                gpio_set_level(row(j), 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(row(j+1), 1);
            }
        }
    }
}

void enemiesattack(){
    for (int j=6; j>=2; j--){  //Detección de abajo hacia arriba para las filas; dispara hasta la sexta fila  
        for (int i=2; i<=7; i++){ 
            if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){ //Donde encuentre un enemigo
                if (gpio_get_level(row(j-1)) == 0){ //y no haya uno de los suyos al frente
                    for (int k=2; k<=7; k++){ //empieza a disparar
                        gpio_set_level(row(k), 1);
                        gpio_set_level(column_R(i), 1);
                        vTaskDelay(pdMS_TO_TICKS(500));
                        gpio_set_level(row(k), 0); 
                        gpio_set_level(column_R(i), 0);
                        if (gpio_get_level(row(8)) == 1 && gpio_get_level(column_G(i)) == 1){ //Hay una nave adelante?
                            for (int s=1; s<=3; s++){ //Titila cuando le dio a la nave
                                gpio_set_level(row(8), 0); 
                                vTaskDelay(pdMS_TO_TICKS(250));
                                gpio_set_level(row(8), 1);
                            }
                            shiplives--;
                            if (shiplives == 0){
                                restart();
                                shiplives = 3;
                            }
                        }
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1500)); //Cada 1.5s disparará
        }
    }
}

void app_main() {
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << ROW2) |
                        (1ULL << ROW3) |
                        (1ULL << ROW4) |
                        (1ULL << ROW5) |
                        (1ULL << ROW6) |
                        (1ULL << ROW7) |
                        (1ULL << ROW8) |
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

    restart();

    while(1){
        moveleft = btn_pressed(LEFT);
        moveright = btn_pressed(RIGHT);
        shipmotion(moveleft, moveright);
        shipattack();
        enemiesmotion();
        enemiesattack();
    }

}