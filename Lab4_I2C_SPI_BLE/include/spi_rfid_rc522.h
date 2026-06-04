#pragma once //para evitar que un archivo header sea incluido varias veces
#include <stdbool.h>
#include <stdint.h>
#define NUM_UIDS 2

//Pin Reset del RFID debe siempre estar en 3.3V al estar funcionando
#define VersionReg 0x37 //para prueba

#define CommandReg       0x01
#define ComIEnReg        0x02
#define DivIEnReg        0x03
#define ComIrqReg        0x04
#define ErrorReg         0x06
#define Status1Reg       0x07
#define Status2Reg       0x08
#define FIFODataReg      0x09
#define FIFOLevelReg     0x0A
#define ControlReg       0x0C
#define BitFramingReg    0x0D
#define ModeReg          0x11
#define TxControlReg     0x14
#define TxASKReg         0x15
#define CRCResultRegL    0x22
#define CRCResultRegH    0x21

#define PCD_IDLE        0x00
#define PCD_TRANSCEIVE  0x0C
#define PCD_SOFTRESET   0x0F
#define PICC_REQA       0x26
#define PICC_ANTICOLL   0x93

void rfid_write(uint8_t reg, uint8_t dato);
uint8_t rfid_read(uint8_t reg);
void rfid_spi_init(void);
void mfrc522_set_bitmask(uint8_t reg, uint8_t mask);
void mfrc522_clear_bitmask(uint8_t reg, uint8_t mask);
void mfrc522_reset(void);
void mfrc522_antenna_on(void);
void mfrc522_init(void);
bool mfrc522_transceive(uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len);
bool mfrc522_request(uint8_t *atqa);
bool mfrc522_anticoll(uint8_t *uid);
bool uid_equals(uint8_t *a, uint8_t *b, uint8_t len);
bool uid_autorizado(uint8_t *uid);