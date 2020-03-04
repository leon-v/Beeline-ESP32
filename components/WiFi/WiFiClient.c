#include "Component.h"
#include "WiFi.h"

#define TAG "WiFi Client"


void wiFiClientSetConfig(pComponent_t pComponent, wifi_config_t * wiFiConfig) {

    cJSON * clinetSsid;
    ESP_ERROR_CHECK(componentSettingsGet(pComponent, "ClientSSID", &clinetSsid));

    if (!cJSON_IsString(clinetSsid)) {
        ESP_LOGE(TAG, "Failed to get SSID while loading config.");
    }
    else{
        strcpy((char *) &wiFiConfig->sta.ssid, clinetSsid->valuestring);
    }
    

    cJSON * clinetPassword;
    ESP_ERROR_CHECK(componentSettingsGet(pComponent, "ClientPassword", &clinetPassword));
    if (!cJSON_IsString(clinetPassword)) {
        ESP_LOGE(TAG, "Failed to get Password while loading config.");
    }
    else{
        strcpy((char *) &wiFiConfig->sta.password, clinetPassword->valuestring);
    }
    
    wiFiConfig->sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    ESP_LOGD(TAG, "WiFi Client Config Set %s:%s", wiFiConfig->sta.ssid, wiFiConfig->sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wiFiConfig));
}