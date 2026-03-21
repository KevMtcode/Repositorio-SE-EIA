#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_timer.h" //Para antirrebote

//ESP-32D WROOM XX5R69
#define LEFT 36
#define RIGHT 39 
#define ROW2 32 //fila 2 (no se usará la fila 1), enumerados de arriba hacia abajo
#define ROW3 33 
#define ROW4 25
#define ROW5 26
#define ROW6 27
#define ROW7 14
#define ROW8 12
#define COL_R2 13 //columna 2 (no se usará la columna 1), enumerados de izquierda a derecha
#define COL_G2 15
#define COL_R3 23
#define COL_G3 22
#define COL_R4 3
#define COL_G4 21
#define COL_R5 19
#define COL_G5 18
#define COL_R6 5
#define COL_G6 17
#define COL_R7 16
#define COL_G7 4
#define COL_R8 0
#define COL_G8 2
int shiplives = 3;

volatile bool moveleft = false;
volatile bool moveright = false;
volatile int64_t last_time_rst = 0;

static void IRAM_ATTR buttonleft_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_rst >= 20000){ //Espere 20ms --> antirrebote
        moveleft = true;
        last_time_rst = now;
    }
}

static void IRAM_ATTR buttonright_isr(void *arg){
    int64_t now = esp_timer_get_time();
    if(now - last_time_rst >= 20000){ //Espere 20ms --> antirrebote
        moveright = true;
        last_time_rst = now;
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

void shipmotion(){
    static int ship_col = 2; //Posición inicial de la nave
    gpio_set_level(column_G(ship_col), 0); //apagar posición actual
    if (moveleft){
        if (ship_col > 2){ //Que no se mueva a la izquierda si está en el extremo izquierdo
           ship_col--;
        }
        moveleft = false;
    } else if (moveright){
        if (ship_col < 8){ //Que no se mueva a la derecha si está en el extremo derecho
            ship_col++; //no moverse hacia la derecha si se está en el extremo derecho
        }
        moveright = false;
    }
    gpio_set_level(column_G(ship_col), 1); //encender nueva posición
}

void shipattack(uint64_t attacknow, uint64_t *lastattack, uint64_t *last_step){ //Pasar la variable del timer como puntero para tener el valor original siempre al llamar la función 
    static bool led_state = 0; //static para que se recuerde esta variable cuando se vuelva a entrar a la función
    static int shoot = -1; //-1 = no hay o se pasó de la matriz
    static int col = -1;
    
    for (int i=2; i<=8; i++){
        if (gpio_get_level(column_G(i)) == 1){ //Donde la nave esté
            if(shoot == -1 && attacknow - *lastattack >= 1500000){ //Cada 1.5s, disparará
                *lastattack = attacknow;
                shoot = 7; //empieza desde esta fila a disparar
                col = i; //en qué columna está la nave
            }
        }
    }
    if(shoot != -1 && attacknow - *last_step >= 500000){ //500ms
        led_state = !led_state;
        *last_step = attacknow;
        gpio_set_level(row(shoot), 0);
        shoot--;
        if (shoot < 2){ // se pasó de la matriz
            shoot = -1; 
            return; //sale inmediatamente de la función shipattack()
        }
        gpio_set_level(row(shoot), led_state);
        gpio_set_level(column_R(col), led_state);
    }
    if (shoot > 2 && gpio_get_level(row(shoot-1)) == 1){ //Hay un enemigo adelante?
        gpio_set_level(row(shoot-1), 0); //Le dio al enemigo
        shoot = -1;
    }
}

void enemiesmotion(uint64_t movenow, uint64_t *lastmove){
    static int dir = 1; // 1 = derecha; -1 = izquierda
    static bool godown = false;
    //Movimiento hacia la derecha:
    if(movenow - *lastmove <500000){
        return;
    }
    *lastmove = movenow;
    for(int j=2; j<=7; j++){ //Detecta si se está en uno de los extremos
        if((dir == 1 && gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(8))) == 1 || (dir == -1 && gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(2))) == 1){
            godown = true;
        }
    }
    //Salto hacia abajo:
    if(godown){
        for (int i=2; i<=8; i++){ 
            for (int j=7; j>=2; j--){ //Detección de abajo hacia arriba para las filas
                if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                    gpio_set_level(row(j), 0);
                    gpio_set_level(row(j+1), 1);
                }
            }
        }
        dir = -dir; //cambiar dirección
        godown = false;
        return; //salir inmediamente de la función
    }
    //Movimiento lateral:
    if(dir == 1){
        for (int i=7; i>=2; i--){ //Revisa hasta el 7 para que no vaya a moverlo hacia la derecha cuando esté en la esquina
            for (int j=2; j<=7; j++){ 
                if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                    gpio_set_level(column_G(i), 0);
                    gpio_set_level(column_G(i+1), 1);
                }
            }
        }
    } else{
        for (int i=3; i<=8; i++){ //Revisa hasta el 4 por la misma razón de llegar hasta la esquina
            for (int j=2; j<=7; j++){ 
                if (gpio_get_level(row(j)) == 1 && gpio_get_level(column_G(i)) == 1){
                    gpio_set_level(column_G(i), 0);
                    gpio_set_level(column_G(i-1), 1);
                }
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

void app_main(void) {
    timer_config_t timer_conf = {
        .divider = 80, //1 tick cada 1us --> prescaler
        .counter_dir = TIMER_COUNT_UP, 
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf); //movimiento del disparo de nave
    timer_init(TIMER_GROUP_0, TIMER_1, &timer_conf); //movimiento del disparo enemigo
    timer_init(TIMER_GROUP_1, TIMER_0, &timer_conf); //movimiento del enemigo
    
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
        .intr_type = GPIO_INTR_NEGEDGE //interrupción, Flanco de bajada
    };
    gpio_config(&in_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(LEFT, buttonleft_isr, NULL);
    gpio_isr_handler_add(RIGHT, buttonright_isr, NULL);
    
    uint64_t last_s = 0;
    uint64_t now = 0; 
    uint64_t last = 0;
    uint64_t now_e = 0; 
    uint64_t last_e = 0;

    restart();

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); //Se pone en 0
    timer_start(TIMER_GROUP_0, TIMER_0); //Comienza a contar
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0); 
    timer_start(TIMER_GROUP_0, TIMER_1); 

    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &now);
        timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &now_e);
        shipmotion();
        shipattack(now, &last, &last_s);
        enemiesmotion(now_e, &last_e);
        enemiesattack();
    }

}