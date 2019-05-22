#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_event_loop.h>

#include "components.h"

static component_t component = {
	.name = "WiFi Client",
	.messagesIn = 0,
	.messagesOut = 0
};

static char * ssid;
static char * password;

wifi_config_t wifiConfig;

void wifiClientSetConfig(void) {

	if (ssid != NULL){
		strcpy((char *) wifiConfig.sta.ssid, ssid);
	}

	if (password != NULL){
		strcpy((char *) wifiConfig.sta.password, password);
	}

	esp_wifi_stop();

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));

	ESP_ERROR_CHECK(esp_wifi_start());
}

void loadNVS(nvs_handle nvsHandle){
	componentsLoadNVSString(nvsHandle, ssid, "ssid");
	componentsLoadNVSString(nvsHandle, password, "password");
}



void wifiClientInit(void) {

	esp_wifi_stop();

	wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitConfig));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));

    component.loadNVS = &loadNVS;
	ESP_LOGI(component.name, "WiFI Connecting to AP");

	componentsAdd(&component);
}
