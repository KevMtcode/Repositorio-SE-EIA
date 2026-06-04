#pragma once //para evitar que un archivo header sea incluido varias veces

#define DEVICE_NAME "PanelHMI"
//Funciones static son privadas en el .c
extern char lcd_message[17]; //buffer para mensaje en LCD
void nus_send_response(const char *msg);
void ble_init(void);