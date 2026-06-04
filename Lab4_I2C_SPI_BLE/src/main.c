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
    BUZZER_OFF;
    RED_OFF;
    GREEN_OFF;
    BLUE_OFF;
    indicators_init();
    printf("APP START\n");
    i2c_init();
    printf("I2C ready\n");
    lcd_init();
    printf("LCD ready\n");
    lcd_clear();
    printf("Cleared\n");
    ds1307_init();
    printf("RTC\n");
    ds1307_write_hours(0, 0, 15, 3, 3, 6, 26); // 15:00:00, día 3 de la semana, 03/06/26 
    rfid_spi_init();
    mfrc522_init();
    ble_init();
    //lcd_command(0x01); //Borrar la pantalla

    uint64_t now = 0;
    uint64_t last_led = 0;
    uint64_t last_bz = 0;
    uint64_t last_time = 0;
    uint64_t last_msg = 0;
    bool denied = false;
    int cont = 1;
    uint8_t uid[10];
    uint8_t atqa[2];

    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); //Se pone en 0
    timer_start(TIMER_GROUP_0, TIMER_0); //Comienza a contar el timer 
    states_t last_reported_state = -1; //Variable de estado anterior
    current_state = STATE_LOCKED;
    
    while(1){
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &now);
        //Limpieza de registros SPI inicial al volver a ejecutar:
        rfid_write(CommandReg, PCD_IDLE);
        rfid_write(FIFOLevelReg, 0x80);
        rfid_write(ComIrqReg, 0x7F);
        rfid_write(ErrorReg, 0x00);
        rfid_write(BitFramingReg, 0x00);

        if(current_state != last_reported_state){ //Acciones a ejecutar una sola vez:
            switch (current_state){
                case STATE_LOCKED:
                    lcd_clear();
                    lcd_print("Panel bloqueado");
                    lcd_set_cursor(1,0);
                    lcd_print("Acerque credencial");
                    RED_ON;
                    GREEN_OFF;
                    BLUE_OFF;
                    break;
                case STATE_GRANTED:
                    lcd_clear();
                    lcd_print("Acceso concedido");
                    ds1307_read_time();
                    RED_OFF;
                    GREEN_ON;
                    BUZZER_ON;
                    last_bz = now; //Cuenta al entrar al estado la primera vez
                    last_led = now;
                    break;
                case STATE_DENIED:
                    lcd_clear();
                    BUZZER_ON;
                    cont = 1;
                    denied = true;
                    last_bz = now;
                    last_led = now;
                    break;
                case STATE_ACTIVATED:
                    lcd_clear();
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
                if (mfrc522_request(atqa)) { // Hay tarjeta
                    if (mfrc522_anticoll(uid)) { // UID leído correctamente
                        if (uid_autorizado(uid)) {
                            current_state = STATE_GRANTED;
                        } else {
                            current_state = STATE_DENIED;
                            denied = true;
                        }
                    }
                }
                break;
            case STATE_GRANTED:
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
                    lcd_clear();
                    lcd_print("Acceso denegado");
                    lcd_set_cursor(1,0);
                    lcd_print("UID noregistrado");
                } else{
                    lcd_clear();
                    lcd_print("Panel bloqueado");
                    lcd_set_cursor(1,0);
                    lcd_print("Acerque credencial");
                    current_state = STATE_LOCKED; //Si no, se queda en este estado siempre
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
                lcd_print(lcd_message); //Mensaje enviado por BLE
                if((now - last_time) >= 1000000){ //1000ms = 1s
                    ds1307_read_time();
                    last_time = now;
                }
                if((now - last_msg) >= 10000000){ //10000ms = 10s
                    lcd_clear();
                    lcd_print("Sin mensajes");
                    ds1307_read_time();
                    last_msg = now;
                }
                if(uid_autorizado(uid)){ //access granted: true 
                  current_state = STATE_LOGOUT;
                }
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
        vTaskDelay(pdMS_TO_TICKS(100)); // Pequeña espera para no saturar CPU
    }
}