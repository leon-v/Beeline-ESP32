/*

W (70838) httpd_txrx: httpd_sock_err: error in send : 104
ESP_EROR_CHECK failed: esp_err_t 0x8006 (ERROR) at 0x4008a258
0x4008a258: _esp_error_check_failed at /mnt/c/Users/leonv/Documents/ESP32/esp-idf/components/esp32/panic.c:720

file: "/mnt/c/Users/leonv/Documents/ESP32/components/http_server/http_server.c" line 250
*/

// esp-idf includes
#include <esp_spi_flash.h>
#include <nvs_flash.h>
#include <esp_pm.h>

// Application includes
#include "components.h"
#include "wifi.h"
#include "http_server.h"
#include "device.h"
#include "mqtt_connection.h"
#include "datetime.h"
#include "die_temperature.h"
#include "die_hall.h"
#include "wake_timer.h"
#include "elastic.h"
#include "radio.h"
#include "display.h"
#include "hcsr04.h"

void app_main() {

	esp_err_t error;

	/*
    * Initialise NVS
    */
    error = nvs_flash_init();

    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		error = nvs_flash_init();
    }

    ESP_ERROR_CHECK(error);


    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
		chip_info.cores,
		(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : ""
	);

    printf("silicon revision %d, ", chip_info.revision);
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    #if CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    esp_pm_config_esp32_t pm_config = {
            .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
            .min_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
	#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = false
	#endif
	};
	ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
	#endif // CONFIG_PM_ENABLE

    // Setup config button
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (0x01 << 0);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    int apMode = !gpio_get_level(0);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);

    /*
    * Init Components
    */
	wiFiInit(apMode);

	httpServerInit();

	deviceInit();

	mqttConnectionInit();

	dateTimeInit();

	dieTemperatureInit();

	dieHallInit();

	hcsr04Init();

	wakeTimerInit();

	elasticInit();

	radioInit();

	displayInit();

	/*
    * Call Init on components
    */
	componentsInit();

	/*
    * Start component tasks
    */
	componentsStart();
}
