#include <Component.h>

#include <WiFi.h>

#define TAG "WiFi"

static wifi_config_t wifiClientConfig;
static wifi_config_t wifiAccessPointConfig;

static wifi_scan_config_t scanConfig;

#define MODE_AP_CLIENT 1
#define MODE_AP 2
#define MODE_CLIENT 3

#define STATE_SCAN 1
#define STATE_RECONNECT 2

static cJSON * statusExtra = NULL;
static uint8_t state = STATE_SCAN;

static esp_err_t wifiEventHandler(void * context, system_event_t *event) {

	pComponent_t pComponent = (pComponent_t) context;

	cJSON * mode;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "Mode", &mode));

	switch(event->event_id) {

		/************* STA ***************/
		case SYSTEM_EVENT_STA_START:
			ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
			componentSettingsSetStatus(pComponent, "warning", "Started, not conencting", statusExtra);
        	break;
		
		case SYSTEM_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
			componentSettingsSetStatus(pComponent, "success", "Connected, waiting for IP", statusExtra);
        	break;

    	case SYSTEM_EVENT_STA_GOT_IP:
    		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");

			char * ipAddress = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
			cJSON_ReplaceItemInObject(statusExtra, "ipAddress", cJSON_CreateString(ipAddress));
			
			componentSettingsSetStatus(pComponent, "success", "Connected, got IP", statusExtra);
        	break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
			cJSON_ReplaceItemInObject(statusExtra, "ipAddress", cJSON_CreateString(""));
			componentSettingsSetStatus(pComponent, "danger", "Disconnected", statusExtra);
			state = STATE_RECONNECT;

        	break;
		
		case SYSTEM_EVENT_STA_STOP:
			ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
			componentSettingsSetStatus(pComponent, "danger", "Stopped", statusExtra);
			break;


		/************* AP ***************/
		case SYSTEM_EVENT_AP_START:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
			break;

		case SYSTEM_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED "MACSTR, MAC2STR(event->event_info.sta_connected.mac));
			break;

		case SYSTEM_EVENT_AP_STAIPASSIGNED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STAIPASSIGNED");
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED "MACSTR, MAC2STR(event->event_info.sta_disconnected.mac));
			break;

		case SYSTEM_EVENT_AP_STOP:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");
			break;
		

		/************* SCAN ***************/
		case SYSTEM_EVENT_SCAN_DONE:
			ESP_LOGI(TAG, "SYSTEM_EVENT_SCAN_DONE");

			wiFiScanBuildOptions(pComponent);
			
			esp_wifi_scan_stop();

			ESP_ERROR_CHECK(esp_wifi_connect());

			break;

		default:
			ESP_LOGE(TAG, "WiFi event_id %d not found", event->event_id);
			break;
	}

	return ESP_OK;
}

void wiFiSetupAP(pComponent_t pComponent){

	cJSON * longRange;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "APLR", &longRange));

	if (longRange->valueint) {
		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR)
		);
	}
	else{
		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)
		);
	}

	wiFiAccessPointSetConfig(pComponent, &wifiAccessPointConfig);
}

void wiFiSetupClient(pComponent_t pComponent) {

	wiFiClientSetConfig(pComponent, &wifiClientConfig);

	cJSON * longRange;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "ClientLR", &longRange));

	if (longRange->valueint) {
		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR)
		);
	}
	else{
		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)
		);
	}
}

void wiFiSetup(pComponent_t pComponent) {

	cJSON * mode;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "Mode", &mode));

	switch (mode->valueint) {

		case MODE_AP_CLIENT:
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
			wiFiSetupAP(pComponent);
			wiFiSetupClient(pComponent);
		break;

		case MODE_AP:
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
			wiFiSetupAP(pComponent);
		break;

		case MODE_CLIENT:
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
			wiFiSetupClient(pComponent);
		break;

		default:
			ESP_LOGE(TAG, "Undnown Mode: %d", mode->valueint);
		break;
	}
}

void wifiInit(pComponent_t pComponent) {

	statusExtra = cJSON_CreateObject();
	cJSON_AddStringToObject(statusExtra, "ipAddress", "");
	componentSettingsSetStatus(pComponent, "secondary", "Initialising", statusExtra);

	printf("WiFi - Initialisation - Start.\n");

	// Make sure we have options for WiFi SSIDs and that the current SSID is set
	ESP_ERROR_CHECK(wiFiScanSSIDOptionsInit(pComponent));

	ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, pComponent));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	wiFiSetup(pComponent);

	wiFiScanSetConfig(pComponent, &scanConfig);

	ESP_ERROR_CHECK(esp_wifi_start());

	cJSON * mode;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "Mode", &mode));

	switch (mode->valueint) {

		case MODE_AP_CLIENT:
		case MODE_AP:
			cJSON_ReplaceItemInObject(statusExtra, "ipAddress", cJSON_CreateString("192.168.4.1"));
			componentSettingsSetStatus(pComponent, "secondary", "Starting AP", statusExtra);
		break;

		case MODE_CLIENT:
			ESP_ERROR_CHECK(esp_wifi_connect());
		break;
		
		default:
			ESP_LOGE(TAG, "Undnown Mode: %d", mode->valueint);
		break;
	}

	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
}

static void task(void * arg) {

	// pComponent_t pComponent = (pComponent_t) arg;
	esp_err_t espError;

	while (true) {

		switch (state) {

			case STATE_SCAN:

				espError = esp_wifi_scan_start(&scanConfig, false);

				if (espError == ESP_OK) {
					state = STATE_RECONNECT;
				}
			break;

			case STATE_RECONNECT:
				ESP_ERROR_CHECK(esp_wifi_connect());
			break;
		}
		

		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
	return;
}

static void postSave(pComponent_t pComponent) {

	// TODO Make one single init / de-init function that is used by settings save and main init

	ESP_ERROR_CHECK(esp_wifi_stop());

	wiFiSetup(pComponent);

	ESP_ERROR_CHECK(esp_wifi_start());

	cJSON * mode;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "Mode", &mode));

	state = STATE_SCAN;
	
	ESP_LOGW(TAG, "Saved");
}

extern const uint8_t settingsFile[] asm("_binary_WiFi_json_start");
static component_t component = {
	.settingsFile	= (char *) settingsFile,
	.task			= &task,
    .init           = &wifiInit,
	.postSave		= &postSave
};

pComponent_t wiFiGetComponent(void) {
	return &component;
}