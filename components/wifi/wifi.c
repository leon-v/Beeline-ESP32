#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_event_loop.h>

#include "components.h"
#include "wifi_client.h"
#include "wifi_access_point.h"

static const char wifi_config_html_start[] asm("_binary_wifi_config_html_start");
const httpPage_t httpPageConfigHTML = {
	.uri	= "/wifi_config.html",
	.page	= wifi_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static component_t component = {
	.name = "WiFi",
	.messagesIn = 0,
	.messagesOut = 0,
	.configPage = &httpPageConfigHTML
};

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


void wiFiInit(void){

	tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));

	componentsAdd(&component);

	// wifiClientInit();

	wifiAccessPointInit();
}
