#pragma once

#include "Module.hpp"
#include "NVS.hpp"

#include <string.h>

#include <esp_wifi.h>



class WiFi: public Module{
	public:
		// const char *settingsFile = &json;
		wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
		wifi_config_t accessPointConfig;
		wifi_config_t clientConfig;

		WiFi() : Module(){

			ESP_LOGI(this->tag, "Wifi Construct");

			extern const char settingsFile[] asm("_binary_WiFi_json_start");
			this->loadSettingsFile(settingsFile);
			
			ESP_ERROR_CHECK(esp_event_loop_init(this->eventHandler, this));
			
    		ESP_ERROR_CHECK(esp_wifi_init(&this->initConfig));
			
			ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

			this->load();
		}

		void reLoad(){
			this->unLoad();
			this->load();
		}

		void unLoad(){
			ESP_ERROR_CHECK(esp_wifi_stop());
		}

		void load(){

			int mode = this->settings->getInt("mode");

			switch (mode) {
				case 1: // AP

					ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

					this->loadAccessPoint();	
					
				break;
			}

			ESP_ERROR_CHECK(esp_wifi_start());
			// ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &this->clientConfig));
		}

		void loadAccessPoint(){
			
			char * settingsSsid = this->settings->getString("accessPointSsid");

			if (!settingsSsid) {
				ESP_LOGE(this->name, "Failed to get setting 'accessPointSsid'");
				ESP_ERROR_CHECK(ESP_FAIL);
			}

			ESP_ERROR_CHECK(
				esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)
			);

			strcpy((char *) &this->accessPointConfig.ap.ssid, settingsSsid);
			ESP_LOGI(this->name, "WiFi AP SSID Set to: %s", this->accessPointConfig.ap.ssid);

			this->accessPointConfig.ap.authmode			= WIFI_AUTH_OPEN;
			this->accessPointConfig.ap.max_connection	= 1;

			ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &this->accessPointConfig));
		}

		static esp_err_t eventHandler(void * context, system_event_t *event) {

			ESP_LOGI("_WiFI", "eventHandler 1");

			WiFi * wifi = (WiFi *) context;

			switch(event->event_id) {

				/************* STA ***************/
				case SYSTEM_EVENT_STA_START:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_STA_START");
					break;
				
				case SYSTEM_EVENT_STA_CONNECTED:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_STA_CONNECTED");
					break;

				case SYSTEM_EVENT_STA_GOT_IP:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_STA_GOT_IP");
					break;

				case SYSTEM_EVENT_STA_DISCONNECTED:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_STA_DISCONNECTED");
					break;
				
				case SYSTEM_EVENT_STA_STOP:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_STA_STOP");
					break;


				/************* AP ***************/
				case SYSTEM_EVENT_AP_START:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_AP_START");
					break;

				case SYSTEM_EVENT_AP_STACONNECTED:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_AP_STACONNECTED " MACSTR, MAC2STR(event->event_info.sta_connected.mac));
					break;

				case SYSTEM_EVENT_AP_STAIPASSIGNED:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_AP_STAIPASSIGNED");
					break;

				case SYSTEM_EVENT_AP_STADISCONNECTED:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_AP_STADISCONNECTED " MACSTR, MAC2STR(event->event_info.sta_disconnected.mac));
					break;

				case SYSTEM_EVENT_AP_STOP:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_AP_STOP");
					break;
				

				/************* SCAN ***************/
				case SYSTEM_EVENT_SCAN_DONE:
					ESP_LOGI(wifi->name, "SYSTEM_EVENT_SCAN_DONE");
					break;

				default:
					ESP_LOGE(wifi->name, "WiFi event_id %d not found", event->event_id);
					break;
			}
			
			return ESP_OK;
		}
};