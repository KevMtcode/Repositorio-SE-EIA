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
// (C cuenta índices desde 0) 
bool G[9][9] = {0}; //Matriz 9x9 (C cuenta índices desde 0) Verde (enemigos, nave)
bool R[9][9] = {0}; //Matriz Rojo (disparos)

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

void multiplexar(){
    for(int j=2; j<=8; j++){
        turnoff();
        gpio_set_level(row(j), 1);
        for(int i=2; i<=8; i++){
            if(R[j][i]){
                gpio_set_level(column_R(i), 1);
            } else if(G[j][i]){
                gpio_set_level(column_G(i), 1);
            }
        }

        ets_delay_us(1000); //1ms 
    }
}

void restart(){
    for(int j=2; j<=8; j++){
        for(int i=2; i<=8; i++){
            G[j][i] = 0;
            R[j][i] = 0;
        }
    }
    G[8][2] = 1; //Posición inicial de la nave, fila 8, columna 2
    for(int j=2; j<=4; j++){ //Posición inicial de los enemigos
        G[j][3] = 1;
        G[j][5] = 1;
        G[j][7] = 1;
    }
}

void shipmotion(){
    static int ship_col = 2; //Posición inicial de la nave
    G[8][ship_col] = 0; //apagar posición actual
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
    G[8][ship_col] = 1; //encender nueva posición
}

void shipattack(uint64_t attacknow, uint64_t *lastattack, uint64_t *last_step){ //Pasar la variable del timer como puntero para tener el valor original siempre al llamar la función 
    static int shoot = -1; //-1 = no hay o se pasó de la matriz
    static int col = -1; // static para que se recuerde esta variable cuando se vuelva a entrar a la función
    
    for (int i=2; i<=8; i++){
        if (G[8][i]){ //Donde la nave esté
            if(shoot == -1 && attacknow - *lastattack >= 1500000){ //Cada 1.5s, disparará
                *lastattack = attacknow;
                shoot = 7; //empieza desde esta fila a disparar
                col = i; //en qué columna está la nave
            }
        }
    }
    if(shoot != -1 && attacknow - *last_step >= 500000){ //500ms
        *last_step = attacknow;
        R[shoot][col] = 0;
        shoot--;
        if (shoot < 2){ // se pasó de la matriz
            shoot = -1; 
            return; //sale inmediatamente de la función shipattack()
        }
        if(G[shoot][col]){ //Hay un enemigo aquí?
            G[shoot][col] = 0; //le dio al enemigo
            shoot = -1;
            return;
        }
        R[shoot][col] = 1; //Si no, avanza el disparo
    }
}

void enemiesmotion(uint64_t movenow, uint64_t *lastmove){
    static bool dir = 1; // 1 = derecha; 0 = izquierda
    static bool godown = false;
    if(movenow - *lastmove <500000){ //Ya pasaron 500ms para mover los enemigos?
        return;
    }
    *lastmove = movenow;
    godown = false;
    for(int j=2; j<=7; j++){ //Detecta si se está en uno de los extremos
        if( (dir && G[j][8]) || (!dir && G[j][2])){
            godown = true;
            break;
        }
    }
    //Salto hacia abajo:
    if(godown){
        for (int j=7; j>=2; j--){ 
            for (int i=2; i<=8; i++){ //Detección de abajo hacia arriba para las filas
                if (G[j][i]){
                    G[j][i] = 0;
                    G[j+1][i] = 1;
                }
            }
        }
        for (int i=2; i<=8; i++){
            if (G[7][i]){ //Los enemigos acorralaron la nave
                restart();
                return;
            }
        }
        dir = !dir; //cambiar dirección
        return; //salir inmediamente de la función
    }
    //Movimiento lateral:
    if(dir){
        for (int i=7; i>=2; i--){ //Mover de izquierda a derecha
            for (int j=2; j<=7; j++){ 
                if (G[j][i]){
                    G[j][i] = 0;
                    G[j][i+1] = 1;
                }
            }
        }
    } else{
        for (int i=3; i<=8; i++){ //Mover de derecha a izquierda
            for (int j=2; j<=7; j++){ 
                if (G[j][i]){
                    G[j][i] = 0;
                    G[j][i-1] = 1;
                }
            }
        }
    }
}

void enemiesattack(uint64_t shootnow, uint64_t *lastshot, uint64_t *lastmove, uint64_t *lasthit){
    static bool s_led_state = 0; 
    static int shoot_e = 9; //9 = no hay o se pasó de la matriz
    static int col_e = 9;
    static bool dir_det = 1;
    static int count = 0;  
    if (shoot_e == 9 && shootnow - *lastshot >= 2000000){ //cada 2s
        for (int j=6; j>=2; j--){  //Detección de abajo hacia arriba para las filas; dispara hasta la sexta fila
            if(dir_det){
                for (int i=2; i<=8; i++){ 
                    if (G[j][i] && !G[j+1][i]){
                        dir_det = !dir_det; //Cambia de dirección de detección
                        *lastshot = shootnow;
                        shoot_e = j;
                        col_e = i;
                        break;
                    }
                }
            } else {
                for (int i=8; i>=2; i--){ 
                    if (G[j][i] && !G[j+1][i]){
                        dir_det = !dir_det;
                        *lastshot = shootnow;
                        shoot_e = j;
                        col_e = i;
                        break;
                    }
                }
            }
            if (shoot_e != 9){
                break; //Sale también del loop de filas
            }
        }
    }
    if(shoot_e != 9 && shootnow - *lastmove >= 500000){ //500ms
        *lastmove = shootnow;
        if(shoot_e >= 2 && shoot_e <=8){
            R[shoot_e][col_e] = 0; //apagar posición actual del disparo
        }
        shoot_e++;
        if (shoot_e == 8){ //Fila de la nave
            if (G[8][col_e]){
                count = 0; 
            }
            shoot_e = 9;
            return;
        }
        R[shoot_e][col_e] = 1;
    }
    if(count < 6 && shootnow - *lasthit >= 250000){ //Titila cada 250ms cuando le dieron a la nave
        *lasthit = shootnow;
        count++;
        s_led_state = !s_led_state;
        for(int i=2; i<=8; i++){
            if(G[8][i]){
                G[8][i] = s_led_state;
            }
        }
        if(count == 6){
            shiplives--;
            if (shiplives == 0){
                restart();
                shiplives = 3;
            }
        }
    }
}

bool win(){
    for (int j=2; j<=7; j++){ 
        for (int i=2; i<=8; i++){
            if (G[j][i]){
                return false; //Aún hay enemigos
            }
        }
    }
    return true; //No se encontró enemigo --> se han eliminado
}

void app_main(void) {
    timer_config_t timer_conf = {
        .divider = 80, //1 tick cada 1us --> prescaler
        .counter_dir = TIMER_COUNT_UP, 
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = false
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf);
    
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
    
    uint64_t now = 0; 
    uint64_t last = 0; //movimiento nave
    uint64_t last_s = 0; //disparo nave
    uint64_t last_e = 0; //movimiento enemigos
    uint64_t last_e_s = 0; //inicio de disparo enemigo
    uint64_t last_es = 0; //disparo enemigo
    uint64_t last_hs = 0; //hit nave
    bool won = false;
    bool game = false;
    restart();

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); //Se pone en 0
    timer_start(TIMER_GROUP_0, TIMER_0); //Comienza a contar

    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &now);
        if(!game){
            shipmotion();
            shipattack(now, &last, &last_s);
            enemiesmotion(now, &last_e);
            enemiesattack(now, &last_e_s, &last_es, &last_hs);
        }
        won = win();
        if (won && !game){
            restart();
            last = 0;
            last_s = 0;
            last_e = 0;
            last_e_s = 0;
            last_es = 0;
            last_hs = 0;
            game = true;
        }
        if(!won){
            game = false;
        }
        multiplexar();
    }
}