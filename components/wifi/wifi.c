#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_event_loop.h>

#include "components.h"
#include "device.h"

static component_t component = {
	.name = "WiFi",
};

static const char wifi_config_html_start[] asm("_binary_wifi_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/wifi_config.html",
	.page	= wifi_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

#define WIFI_AP_SSID "Beeline Configuration AP"

static char * ssid;
static char * password;

// static char resetConfig  = 0;

static wifi_config_t wifiConfig;


static void wifiLoadConfig() {

	// AP Configuration
	strcpy((char *) wifiConfig.ap.ssid, WIFI_AP_SSID);
	wifiConfig.ap.max_connection	= 1;
	wifiConfig.ap.authmode			= WIFI_AUTH_OPEN;


	// STA Configuration
	if (ssid != NULL){
		strcpy((char *) wifiConfig.sta.ssid, ssid);
	}

	if (password != NULL){
		strcpy((char *) wifiConfig.sta.password, password);
	}
}

static void saveNVS(nvs_handle nvsHandle) {
	componentsSetNVSString(nvsHandle, ssid, "ssid");
	componentsSetNVSString(nvsHandle, password, "password");

	componentsSetNVSu32(nvsHandle, "idleTimeout", component.idleTimeout);
}

static void loadNVS(nvs_handle nvsHandle){
	ssid = componentsGetNVSString(nvsHandle, ssid, "ssid", "SSID");
	password = componentsGetNVSString(nvsHandle, password, "password", "Password");

	component.idleTimeout =	componentsGetNVSu32(nvsHandle, "idleTimeout", 0);

	wifiLoadConfig();
}



static esp_err_t wifiEventHandler(void *ctx, system_event_t *event){

	switch(event->event_id) {

		case SYSTEM_EVENT_STA_START:
			ESP_LOGI(component.name, "SYSTEM_EVENT_STA_START");
			esp_wifi_connect();
        	break;

    	case SYSTEM_EVENT_STA_GOT_IP:
    		ESP_LOGI(component.name, "SYSTEM_EVENT_STA_GOT_IP");
       		componentSetReady(&component);
        	break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        	ESP_LOGI(component.name, "SYSTEM_EVENT_STA_DISCONNECTED");
        	componentSetNotReady(&component);
        	esp_wifi_connect();
        	break;

		case SYSTEM_EVENT_AP_STACONNECTED:

			ESP_LOGI(component.name, "SYSTEM_EVENT_AP_STACONNECTED "MACSTR, MAC2STR(event->event_info.sta_connected.mac));
			deviceLog("AP STA Connected "MACSTR, MAC2STR(event->event_info.sta_connected.mac));
			componentSetReady(&component);
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(component.name, "SYSTEM_EVENT_AP_STADISCONNECTED "MACSTR, MAC2STR(event->event_info.sta_disconnected.mac));
			break;

		case SYSTEM_EVENT_AP_START:
			ESP_LOGI(component.name, "SYSTEM_EVENT_AP_START");
			break;

		case SYSTEM_EVENT_AP_STOP:
			ESP_LOGI(component.name, "SYSTEM_EVENT_AP_STOP");
			break;

		default:
			break;
	}

	return ESP_OK;
}

static void wifiClientInit(void) {

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));

	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));

	ESP_LOGW(component.name, "Configured as station");
}

static void wifiAccessPointInit(void) {

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfig));

    ESP_LOGW(component.name, "Configured as access point. SSID: %s", wifiConfig.ap.ssid);
}

static void task(void * arg) {

	wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&wifiInitConfig));

	int * pAPMode = (int *) arg;

	if (* pAPMode){
		wifiAccessPointInit();
	}

	else{
		wifiClientInit();
	}

	ESP_LOGW(component.name, "Starting");

	ESP_ERROR_CHECK(esp_wifi_start());

	while(true) {

		if (componentsEndRequested(&component) == ESP_OK) {
			ESP_LOGW(component.name, "Ending loop.");
			break;
		}

		vTaskDelay(60000 / portTICK_RATE_MS);

		// if (resetConfig == 1) {
		// 	resetConfig = 0;
		// 	wifiLoadConfig();
		// 	esp_wifi_stop();
		// 	ESP_ERROR_CHECK(esp_wifi_start());
		// }
	}

	ESP_LOGW(component.name, "Ending task");

	ESP_LOGW(component.name, "Stopping");

	esp_wifi_stop();

	componentsSetEnded(&component);

	vTaskDelete(NULL);

	return;
}

void wiFiInit(int * pAPMode) {

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));

    component.configPage	= &configPage;
	component.task			= &task;
	component.taskArg		= pAPMode;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;

	componentsAdd(&component);
}
