#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_event_loop.h>

#include "components.h"

static component_t component = {
	.name = "WiFi Client",
	.messagesIn = 0,
	.messagesOut = 0
};

static ssid;
static password;

void loadNVS(nvs_handle nvsHandle){

}

void wifiClientInit(void) {

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
	    .sta = {
	        .ssid = "",
	        .password = ""
	    },
	};

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

	ESP_LOGI(component.name, "WiFI Connecting to AP");

	componentsAdd(&component);
}
