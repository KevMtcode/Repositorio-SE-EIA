#include "system_lib.h" //Se reunen aquí las librerías necesarias de C y PlatformIO, así como también los pines GPIO
#include "indicators.h"
#include "spi_rfid_rc522.h"
#include "i2c_lcd.h"
#include "i2c_ds1307.h"
#include "ble_communication.h"

typedef enum { //Estados
    STATE_INIT, //inicialización del sistema
    //Estados para el funcionamiento:
    STATE_LOCKED,
    STATE_GRANTED,
    STATE_DENIED,
    STATE_ACTIVATED,
    STATE_LOGOUT
} states_t;
volatile states_t current_state = STATE_INIT; //Variable de estado actual

void app_main(void) {
    
    indicators_init();
    uint64_t now = 0;
    uint64_t last_led = 0;
    uint64_t last_bz = 0;
    bool denied = false;
    int cont = 1;

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); //Se pone en 0
    timer_start(TIMER_GROUP_0, TIMER_0); //Comienza a contar el timer 
    states_t last_reported_state = -1; //Variable de estado anterior
    current_state = STATE_LOCKED;
    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &now);
        if(current_state != last_reported_state){ //Acciones a ejecutar una sola vez:
            switch (current_state){
                case STATE_LOCKED:
                    RED_ON;
                    GREEN_OFF;
                    BLUE_OFF;
                    break;
                case STATE_GRANTED:
                    RED_OFF;
                    GREEN_ON;
                    BUZZER_ON;
                    last_bz = now; //Cuenta al entrar al estado la primera vez
                    last_led = now;
                    break;
                case STATE_DENIED:
                    BUZZER_ON;
                    denied = true;
                    last_bz = now;
                    last_led = now;
                    break;
                case STATE_ACTIVATED:
                    BLUE_ON;
                    break;
                case STATE_LOGOUT:
                    BUZZER_ON;
                    last_bz = now;
                    break;
                default: //Por seguridad
                    break;
            }
            last_reported_state = current_state; //Guardar el estado actual
        }
        switch (current_state){ //Acciones continuas al encontrarse en el estado:
            case STATE_LOCKED:
                //show_locked(); //mostrar "Panel bloqueado" y "Acerque credencial"
                //if(read_uid() == true){ //access granted: true 
                //  current_state = STATE_GRANTED;
                //}
                break;
            case STATE_GRANTED:
                //show_access_granted(); //mostrar "Acceso concedido" y hora actual HH:MM:SS
                if((now - last_bz) >= 500000){ //500ms = 0.5s
                    BUZZER_OFF;
                    last_bz = now;
                }
                if((now - last_led) >= 1000000){ //1000ms = 1s
                    GREEN_OFF;
                    last_led = now;
                    current_state = STATE_ACTIVATED;
                }
                break;
            case STATE_DENIED:
                if(denied){
                    //show_access_denied(); //mostrar "Acceso denegado" y "UID no registrado"
                } else{
                    //show_locked(); //mostrar mensajes del estado bloqueado
                }
                if((now - last_led) >= 250000){ //250ms = 0.25s
                    if(cont % 2 != 0){
                        RED_ON;
                    } else{
                        RED_OFF;
                    }
                    if(cont < 6){ // Para que solo sean tres veces que se encienda y apague
                        cont++;
                    }
                    last_led = now;
                }
                if((now - last_bz) >= 2000000){ //2000ms = 2s
                    BUZZER_OFF;
                    last_bz = now;
                    denied = false;
                }
                break;
            case STATE_ACTIVATED:
                //if(read_uid() == true){ //access granted: true 
                //  current_state = STATE_LOGOUT;
                //}
                break;
            case STATE_LOGOUT:
                if((now - last_bz) >= 500000){ //500ms = 0.5s
                    BUZZER_OFF;
                    last_bz = now;
                    current_state = STATE_LOCKED;
                }
                break;
            default: //Por seguridad
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // Pequeña espera para no saturar CPU
    }
}