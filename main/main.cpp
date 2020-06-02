#include "Modules.hpp"


#include "Device.hpp"
#include "WiFi.hpp"
#include "NtpClient.hpp"
#include "DieTemperature.hpp"
#include "ElasticSearch.hpp"
#include "MqttClient.hpp"
#include "Adc.hpp"

static const char *TAG = "main";

extern "C" void app_main(void) {
    // Do example setup
    ESP_LOGI(TAG, "Start");

	esp_err_t err = nvs_flash_init();

	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}

	gpio_config_t io_conf;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_SEL_0;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);

	printf("Press and hold prog (GPIO0) low to erase NVS\r\n");

	vTaskDelay(1000 / portTICK_PERIOD_MS);
	if (!gpio_get_level(GPIO_NUM_0)) {
		ESP_LOGW(TAG, "Resetting NVS");
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	


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

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	
	static Modules modules;

	static Device device(&modules);

	static WiFi wifi(&modules);

	static NtpClient ntpClient(&modules);

	static DieTemperature dieTemperature(&modules);

	static ElasticSearch elasticSearch(&modules);

	static MqttClient mqttClient(&modules);

	static Adc adc(&modules);

	ESP_ERROR_CHECK(modules.start());

    ESP_LOGI(TAG, "End");
}