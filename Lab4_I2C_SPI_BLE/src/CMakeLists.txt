# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

idf_component_register(
    SRCS 
        "main.c"
        "indicators.c"
        "spi_rfid_rc522.c"
        "i2c_lcd.c"
        "i2c_ds1307.c"
        "ble_communication.c"
    INCLUDE_DIRS "."             # Para buscar en la misma carpeta src
        "${CMAKE_CURRENT_SOURCE_DIR}/../include" # Ruta absoluta a tu carpeta include
    REQUIRES bt nvs_flash driver
)