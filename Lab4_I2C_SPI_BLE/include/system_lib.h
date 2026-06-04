#pragma once //para evitar que un archivo header sea incluido varias veces
#include <math.h>
#include <stdint.h>
#include <string.h> //para los caracteres
#include <stdio.h>
#include <stdlib.h> //Para atoi() --> ASCII to integer
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
//  Comunicaciones:
#include "driver/i2c.h"
#include "driver/spi_master.h" 
//BLE:
#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "pin_map.h" //GPIOs