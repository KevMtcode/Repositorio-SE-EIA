#include "system_lib.h" //Se reunen aquí las librerías necesarias de C y PlatformIO, así como también los pines GPIO
#include "indicators.c"
#include "spi_rfid_rc522.c"
#include "i2c_lcd.c"
#include "i2c_ds1307.c"
#include "ble_communication.c"

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
    


    states_t last_reported_state = -1; //Variable de estado anterior
    current_state = STATE_LOCKED;
    while(1){
        if(current_state != last_reported_state){ //Acciones a ejecutar una sola vez:
            switch (current_state){
                case STATE_LOCKED:
                    
                    break;
                case STATE_GRANTED:
                    
                    break;
                case STATE_DENIED:
                    
                    break;
                case STATE_ACTIVATED:
                    
                    break;
                case STATE_LOGOUT:
                    
                    break;
                default: //Por seguridad
                    break;
            }
            last_reported_state = current_state; //Guardar el estado actual
        }
        switch (current_state){ //Acciones continuas al encontrarse en el estado:
            case STATE_LOCKED:
                
                break;
            case STATE_GRANTED:
                
                break;
            case STATE_DENIED:
                
                break;
            case STATE_ACTIVATED:
                
                break;
            case STATE_LOGOUT:
                
                break;
            default: //Por seguridad
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // Pequeña espera para no saturar CPU
    }
}