#include "spi_rfid_rc522.h"
#include "system_lib.h"

spi_device_handle_t rc522;

uint8_t uids_autorizados[NUM_UIDS][4] = {
    {0xEB, 0xF8, 0xEE, 0x1A},  // tarjeta azul — reemplaza con bytes reales
    {0x37, 0x44, 0x16, 0x17},  // tarjeta blanca — reemplaza con bytes reales
};

void rfid_write(uint8_t reg, uint8_t dato) { 
    uint8_t buf[2]; 
    buf[0] = (reg << 1) & 0x7E;   //AND con 01111110 para garantizar 0 en escritura, 0 en bit 0 por formato y lo demás queda igual
    buf[1] = dato;  // Tamaño de caracter final, valor del registro, calculado con peso decimal
 
    spi_transaction_t t = { 
        .length    = 16, 
        .tx_buffer = buf, 
    }; 
    esp_err_t ret = spi_device_transmit(rc522, &t);
    if(ret != ESP_OK)
    {
        printf("SPI Error\n");
    }
} 
uint8_t rfid_read(uint8_t reg) {
    uint8_t buf_tx[2];
    uint8_t buf_rx[2];
    buf_tx[0] =  ((reg << 1) & 0x7E) | 0x80; //OR con 10000000 y  AND con 11111110
    buf_tx[1] = 0x00; // byte dummy para generar el clock

    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = buf_tx,
        .rx_buffer = buf_rx,  // aquí recibes la respuesta
    };
    
    esp_err_t ret = spi_device_transmit(rc522, &t);
    if(ret != ESP_OK)
    {
        printf("SPI Error\n");
    }
    return buf_rx[1];
}

void rfid_spi_init(void){
    spi_bus_config_t bus = { 
        .mosi_io_num   = PIN_MOSI, 
        .miso_io_num   = PIN_MISO, 
        .sclk_io_num   = PIN_SCLK, 
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1, 
    }; 
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO); 

    // Agregar el RFID como dispositivo SPI 
    spi_device_interface_config_t dev = { 
        .clock_speed_hz = 100000,  // 100kHz 
        .mode           = 0,        // CPOL=0, CPHA=0 
        .spics_io_num   = PIN_SS, 
        .queue_size     = 1, 
    }; 
    spi_bus_add_device(SPI2_HOST, &dev, &rc522); 

}

void mfrc522_set_bitmask(uint8_t reg, uint8_t mask){
    uint8_t tmp = rfid_read(reg);

    rfid_write(reg,
                      tmp | mask);
}

void mfrc522_clear_bitmask(uint8_t reg, uint8_t mask){ //apagar bits
    uint8_t tmp = rfid_read(reg);

    rfid_write(reg,
                      tmp & (~mask));
}

void mfrc522_reset(void){
    rfid_write(CommandReg,
                      PCD_SOFTRESET);

    vTaskDelay(pdMS_TO_TICKS(50));
}

void mfrc522_antenna_on(void){
    uint8_t value =
        rfid_read(TxControlReg);

    if (!(value & 0x03))
    {
        rfid_write(
            TxControlReg,
            value | 0x03);
    }
}

void mfrc522_init(void){
    mfrc522_reset();

    rfid_write(0x2A, 0x8D); // TModeReg
    rfid_write(0x2B, 0x3E); // TPrescalerReg
    rfid_write(0x2D, 30);   // TReloadRegL
    rfid_write(0x2C, 0);    // TReloadRegH

    rfid_write(TxASKReg, 0x40);
    rfid_write(ModeReg, 0x3D);

    mfrc522_antenna_on();

    //Comprobar:
    /*printf("ModeReg = 0x%02X\n", rfid_read(ModeReg)); //Aparece 0x3D
    printf("TxASKReg = 0x%02X\n", rfid_read(TxASKReg)); //Aparece 0X40
    printf("TxControlReg = 0x%02X\n", rfid_read(TxControlReg)); //0x#3, terminando en 3 significa que los bits 0 y 1 están activados para la antena
    */   
}

bool mfrc522_transceive(uint8_t *tx_data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len){
    uint8_t irq;
    uint16_t timeout = 2000;

    rfid_write(ComIrqReg, 0x7F); //Limpiar interrupciones con 01111111

    rfid_write(FIFOLevelReg, 0x80); //Vaciar FIFO con FlushBuffer

    rfid_write(CommandReg, PCD_IDLE); //Detener comando anterior

    for(uint8_t i = 0; i < tx_len; i++){ //Cargar FIFO
        rfid_write(FIFODataReg, tx_data[i]);
    }

    rfid_write(CommandReg, PCD_TRANSCEIVE); //Lanzar transacción

    mfrc522_set_bitmask(BitFramingReg, 0x80); //StartSend (bit 7)

    while(timeout--){   //Esperar respuesta
        irq = rfid_read(ComIrqReg);

       if (irq & 0x20){ // RxIRq = SOLO recepción válida
            break;
        }
        if (irq & 0x01){ // TimerIRq = timeout real
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    printf("IRQ = 0x%02X\n", irq); //Si es 0x20, hay recepción; si es 0x01, timeout

    mfrc522_clear_bitmask(BitFramingReg, 0x80); //Apagar StartSend

    if(timeout == 0){
        return false;
    }
    if(irq & 0x01){
        printf("TIMEOUT\n");
        return false;
    }

    //Revisar errores:
    uint8_t error = rfid_read(ErrorReg);

    if(error & 0X02){
        return false;
    }

    uint8_t count = rfid_read(FIFOLevelReg); //Número de bytes recibidos
    *rx_len = count;

    for(uint8_t i = 0; i < count; i++){ //Leer FIFO
        rx_data[i] = rfid_read(FIFODataReg);
    }
    rfid_write(BitFramingReg, 0x00);
    return true;
}

bool mfrc522_request(uint8_t *atqa){
    uint8_t reqa = PICC_REQA;
    uint8_t len;

    // REQA usa 7 bits
    rfid_write(BitFramingReg, 0x07);

    return mfrc522_transceive(
        &reqa,
        1,
        atqa,
        &len
    );
}

bool mfrc522_anticoll(uint8_t *uid){
    uint8_t cmd[2];
    uint8_t len;

    cmd[0] = PICC_ANTICOLL;
    cmd[1] = 0x20;

    rfid_write(BitFramingReg, 0x00);

    if(!mfrc522_transceive(
            cmd,
            2,
            uid,
            &len))
    {
        return false;
    }

    printf("UID response (%d bytes): ", len);

    for(uint8_t i = 0; i < len; i++)
    {
        printf("%02X ", uid[i]);
    }

    printf("\n");

    // Anticollision nivel 1 debe devolver:
    // UID0 UID1 UID2 UID3 BCC
    if(len != 5)
    {
        return false;
    }

    uint8_t bcc =
        uid[0] ^
        uid[1] ^
        uid[2] ^
        uid[3];

    if(bcc != uid[4])
    {
        printf("BCC error\n");
        return false;
    }

    return true;
}

bool uid_equals(uint8_t *a, uint8_t *b, uint8_t len){ //Comparación de UID
    for (int i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool uid_autorizado(uint8_t *uid){ //Validación
    for (int i = 0; i < NUM_UIDS; i++) {
        if (uid_equals(uid, uids_autorizados[i], 4)) {
            return true;
        }
    }
    return false;
}

