#include "Component.h"
#include "WiFi.h"

#define TAG "WiFi Scan"

#define DEFAULT_SCAN_LIST_SIZE 8

void wiFiScanSetConfig(pComponent_t pComponent, wifi_scan_config_t * scanConfig) {
    scanConfig->ssid = 0;
    scanConfig->bssid = 0;
    scanConfig->channel = 0;
    scanConfig->show_hidden = true;
}

static char* getAuthModeName(wifi_auth_mode_t auth_mode) {
	
	char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
	return names[auth_mode];
}

esp_err_t wiFiScanSSIDOptionsInit(pComponent_t pComponent) {

	cJSON * variables = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "variables");

	if (!cJSON_IsObject(variables)) {
		ESP_LOGE(TAG, "'variables' not found in 'settingsJSON' in %s", __func__);
		return ESP_FAIL;
	}

	cJSON * ClientSSID = cJSON_GetObjectItemCaseSensitive(variables, "ClientSSID");

	if (!cJSON_IsObject(variables)) {
		ESP_LOGE(TAG, "'ssid' not found in 'variables' in %s", __func__);
		return ESP_FAIL;
	}

	cJSON * ssidOptions = cJSON_GetObjectItemCaseSensitive(ClientSSID, "options");

	if (!cJSON_IsObject(ssidOptions)) {
		ssidOptions = cJSON_CreateObject();
		cJSON_AddItemToObject(pComponent->settingsJSON, "options", ssidOptions);
	}

	cJSON * ClientSsidValue;
    ESP_ERROR_CHECK(componentSettingsGet(pComponent, "ClientSSID", &ClientSsidValue));

	if (!cJSON_IsString(ClientSsidValue)) {
		ESP_LOGE(TAG, "'ClientSSID' value not found in %s", __func__);
		return ESP_OK;
	}
	
	cJSON_AddStringToObject(ssidOptions, ClientSsidValue->valuestring, ClientSsidValue->valuestring);

	return ESP_OK;
}

void wiFiScanBuildOptions(pComponent_t pComponent) {

    cJSON * variables = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "variables");

    if (!cJSON_IsObject(variables)) {
        ESP_LOGE(TAG, "Failed to get variables from settingsJSON");
        return;
    }

    cJSON * ssid = cJSON_GetObjectItemCaseSensitive(variables, "ClientSSID");

    if (!cJSON_IsObject(ssid)) {
        ESP_LOGE(TAG, "Failed to get ClientSSID from variables");
        return;
    }

    cJSON * options = cJSON_GetObjectItemCaseSensitive(ssid, "options");

    if (!cJSON_IsObject(options)) {
        ESP_LOGE(TAG, "Failed to get options from SSID");
        return;
    }

    uint16_t ap_num = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_records[DEFAULT_SCAN_LIST_SIZE];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    ESP_LOGI(TAG, "Found %d access points:", ap_num);

    for(int i = 0; i < ap_num; i++) {

        char optionLabel[128];

        char * ssid = (char *)ap_records[i].ssid;

        sprintf(optionLabel, "SSID: %-24s Auth: %-12s Primary: %-7d RSSI: %-4d",
            ssid,
            getAuthModeName(ap_records[i].authmode),
            ap_records[i].primary,
            ap_records[i].rssi
        );

        cJSON * newOption = cJSON_CreateString(optionLabel);

        cJSON * oldOption = cJSON_GetObjectItemCaseSensitive(options, ssid);

        if (!oldOption) {
            cJSON_AddItemToObject(options, ssid, newOption);
        }
        else{
            cJSON_ReplaceItemInObject(options, ssid, newOption);
        }
        
        ESP_LOGI(TAG, "%s", optionLabel);
    }

    cJSON_ReplaceItemInObject(ssid, "options", options);
}