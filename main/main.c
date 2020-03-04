#include "esp_spi_flash.h"

#include <Component.h>
#include <Device.h>
#include <DeviceTimer.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <DieTemperature.h>
#include <ConsoleSink.h>

#define TAG "main"

void app_main() {
    printf("Hello world!\n");

     // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    tcpip_adapter_init();

    componentAdd(wiFiGetComponent());

    componentAdd(deviceGetComponent());

    componentAdd(deviceTimerGetComponent());
    
    componentAdd(NTPClientGetComponent());

    componentAdd(dieTemperatureGetComponent());

    componentAdd(consoleSinkGetComponent());

    componentsInit();

    componentsStart();

    // for (int i = 500000; i >= 0; i--) {
    //     printf("Restarting in %d ...\n", i);
    //     vTaskDelay(10000 / portTICK_PERIOD_MS);
    // }
    // printf("Restarting now.\n");
    // fflush(stdout);
    // esp_restart();
}