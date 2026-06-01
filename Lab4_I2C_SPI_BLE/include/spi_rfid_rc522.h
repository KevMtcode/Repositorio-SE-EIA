#pragma once //para evitar que un archivo header sea incluido varias veces
#include <stdbool.h>
#define NUM_UIDS 2 //Dos tarjetas
void spi_init(void);
void rfid_register(void);
int memcmp(const void *s1, const void *s2, size_t n); //Para comparación bit por bit
bool rfid_check_uid(uint8_t *uid);
bool rfid_card_present(void);
