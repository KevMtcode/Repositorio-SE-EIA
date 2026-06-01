#pragma once //para evitar que un archivo header sea incluido varias veces

#define DEVICE_NAME "PanelHMI"
//Funciones static son privadas en el .c
void nus_send_response(const char *msg);
void ble_init(void);