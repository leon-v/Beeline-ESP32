#include "Component.h"
#include "WiFi.h"

#define TAG "WiFi Access Point"

void wiFiAccessPointSetConfig(component_t * pComponent, wifi_config_t * wiFiConfig) {

	wiFiConfig->ap.max_connection	= 8;

	cJSON * accessPointSsid;
    ESP_ERROR_CHECK(componentSettingsGet(pComponent, "APSSID", &accessPointSsid));

    if (!cJSON_IsString(accessPointSsid)) {
        ESP_LOGE(TAG, "Failed to get SSID while loading config.");
    }
    else{
		strcpy((char *) &wiFiConfig->ap.ssid, accessPointSsid->valuestring);
        ESP_LOGI(TAG, "SSID set to: %s", wiFiConfig->ap.ssid);
    }

    cJSON * clinetPassword;
    ESP_ERROR_CHECK(componentSettingsGet(pComponent, "APPassword", &clinetPassword));
    
    if (!cJSON_IsString(clinetPassword)) {
        ESP_LOGE(TAG, "Failed to get Password while loading config.");
    }
    else{
        if (strlen(clinetPassword->valuestring)) {
            strcpy((char *) &wiFiConfig->ap.password, clinetPassword->valuestring);
            wiFiConfig->ap.authmode = WIFI_AUTH_WPA2_PSK;
        }
        else{
			wiFiConfig->ap.authmode			= WIFI_AUTH_OPEN;
		}
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, wiFiConfig));
}