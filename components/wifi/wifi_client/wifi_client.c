#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_event_loop.h>

#include "components.h"

static component_t component = {
	.name = "WiFi Client",
	.messagesIn = 0,
	.messagesOut = 0
};

static void task(void * arg) {

	while (1) {

		ESP_LOGW(component.name, "test");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void wifiClientInit(void) {

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
	    .sta = {
	        .ssid = "V2.4Ghz",
	        .password = "wifigrl7"
	    },
	};

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start() );

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

	ESP_LOGI(component.name, "WiFI Connecting to AP");

	esp_wifi_connect();

	component.task = task;
	componentsAdd(&component);

}
