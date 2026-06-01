#include "spi_rfid_rc522.h"
#include "system_lib.h"
spi_device_handle_t rc522; 

uint8_t uids_autorizados[NUM_UIDS][4] = {
    {0xAA, 0xBB, 0xCC, 0xDD},  // tarjeta 1 — reemplaza con bytes reales
    {0x11, 0x22, 0x33, 0x44},  // tarjeta 2 — reemplaza con bytes reales
};

uint8_t num_uids = sizeof(uids_autorizados) / sizeof(uids_autorizados[0]);

static void rfid_write(uint8_t reg, uint8_t dato) { 
    uint8_t buf[2]; 
    buf[0] = reg & 0x7E;   //AND con 01111110 para garantizar 0 en escritura, 0 en bit 0 por formato y lo demás queda igual
    buf[1] = dato;  // Tamaño de caracter final, valor del registro, calculado con peso decimal
 
    spi_transaction_t t = { 
        .length    = 16, 
        .tx_buffer = buf, 
    }; 
    spi_device_transmit(rc522, &t); 
} 
static uint8_t rfid_read(uint8_t reg) {
    uint8_t buf_tx[2];
    uint8_t buf_rx[2];
    buf_tx[0] = (reg | 0x80) & 0xFE;; //OR con 10000000 y  AND con 11111110
    buf_tx[1] = 0x00; // byte dummy para generar el clock

    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = buf_tx,
        .rx_buffer = buf_rx,  // aquí recibes la respuesta
    };

    spi_device_transmit(rc522, &t);
    return buf_rx[1];
}

void rfid_spi_init(void){
    spi_bus_config_t bus = { 
        .mosi_io_num   = PIN_MOSI, 
        .miso_io_num   = PIN_MISO, 
        .sclk_io_num   = PIN_SClK, 
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1, 
    }; 
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO); 

    // Agregar el RFID como dispositivo SPI 
    spi_device_interface_config_t dev = { 
        .clock_speed_hz = 1000000,  // 1 MHz 
        .mode           = 0,        // CPOL=0, CPHA=0 
        .spics_io_num   = PIN_SS, 
        .queue_size     = 1, 
    }; 
    spi_bus_add_device(SPI2_HOST, &dev, &rc522); 

}

void rfid_register(void){
    // SoftReset:
    rfid_write(0x01, 0x0F);
    vTaskDelay(pdMS_TO_TICKS(50)); // espera que el reset termine

    // Timer (para que los comandos no queden colgados)
    rfid_write(0x2A, 0x80); //bit 7 TAuto, 128 de peso en decimal --> 0x80
    rfid_write(0x2B, 0xA9); 
    rfid_write(0x2C, 0x03); //TReloadReg
    rfid_write(0x2D, 0XE8);

    // Forzar 100% ASK:
    rfid_write(0x15, 0x40); // bit 6  --> 64 en decimal --> 0x40

    // Modo CRC:
    rfid_write(0x11, 0x01); // 6363 para ISO 14443A

    // Encender la antena
    rfid_write(0x14, 0x03); //bits 0 y 1 --> 1 + 2 en decimal --> 0x03
}

void rfid_read_uid(uint8_t *uid) {

    rfid_write(0x01, 0x00); // Idle command

    // Limpiar el FIFO:
    rfid_write(0x0A, 0x80);

    // Escribir el comando REQA en el FIFO
    rfid_write(0x09, 0x26);

    // Configurar BitFramingReg para enviar 7 bits y activar StartSend
    rfid_write(0x0D, 0x87); //StartSend y TxLasBits --> bit 7 + 2 a 0 bit

    // Activar comando Transceive en CommandReg:
    rfid_write(0x01, 0x0C);

    // Esperar respuesta de la tarjeta:
    // Busca en ComIrqReg (0x04) los bits RxIRq e IdleIRq
    // El loop termina cuando alguno de esos bits se activa
    uint8_t irq;
    do {
        irq = rfid_read(0x04);
    } while ((irq & 0x30) == 0); //Solo si RxIrq y IdleIrq están activados --> 0x30 = 00110000 

    // Leer cuántos bytes llegaron al FIFO:
    uint8_t len = rfid_read(0x0A);

    // PASO 8: Leer los bytes del FIFO uno por uno
    for (uint8_t i = 0; i < len; i++) {
        uid[i] = rfid_read(0x09); 
    }
}

bool rfid_check_uid(uint8_t *uid) {
    for (uint8_t i = 0; i < NUM_UIDS; i++) {
        if (memcmp(uid, uids_autorizados[i], 4) == 0) { //es cero si ambos UIDs son iguales
            return true;
        }
    }
    return false;
}

bool rfid_card_present(void) { // Detecta si hay una tarjeta presente
    rfid_write(0x01, 0x00);
    rfid_write(0x0A, 0x80);
    rfid_write(0x09, 0x26);
    rfid_write(0x0D, 0x87);
    rfid_write(0x01, 0x0C);

    uint8_t irq;
    do {
        irq = rfid_read(0x04);
    } while ((irq & 0x30) == 0);

    // Leer ErrorReg y verificar:
    uint8_t error = rfid_read(0x06);
    return (error & 0x08) == 0; // sin error = tarjeta presente
}
