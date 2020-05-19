#pragma once

#include "Modules.hpp"

#include <string>
#include <esp_wifi.h>

#define DEFAULT_SCAN_LIST_SIZE 8

extern const char wiFiSettingsFile[] asm("_binary_WiFi_json_start");

class WiFi: public Modules::Module{
	public:

	wifi_init_config_t	initConfig = WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t		accessPointConfig;
	wifi_config_t		clientConfig;
	wifi_scan_config_t	scanConfig;
	volatile bool		scanDone = false;
	
	WiFi(Modules *modules):Modules::Module(modules, string(wiFiSettingsFile)){

		ESP_LOGI(this->tag.c_str(), "Construct");
		
		ESP_ERROR_CHECK(esp_event_loop_init(this->eventHandler, this));
		
		ESP_ERROR_CHECK(esp_wifi_init(&this->initConfig));
		
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

		this->load();

		ESP_LOGI(this->tag.c_str(), "/Construct");
	}

	void reLoad(){
		this->unLoad();
		this->load();
	}

	void unLoad(){
		ESP_ERROR_CHECK(esp_wifi_stop());
	}

	void load(){

		int mode = this->settings.getInt("mode");

		switch (mode) {

			case 1: // AP
				ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
				ESP_ERROR_CHECK(this->loadAccessPoint());
			break;

			case 2: // Client
				ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
				ESP_ERROR_CHECK(this->loadClient());
			break;

			case 3: // AP & Client
				ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
				ESP_ERROR_CHECK(this->loadAccessPoint());
				ESP_ERROR_CHECK(this->loadClient());
			break;
		}

		ESP_ERROR_CHECK(this->loadScan());

		ESP_ERROR_CHECK(esp_wifi_start());

		wifi_mode_t wiFiMode;
		ESP_ERROR_CHECK(esp_wifi_get_mode(&wiFiMode));

		if ( (wiFiMode == WIFI_MODE_STA) || (wiFiMode == WIFI_MODE_APSTA) ) {
			ESP_ERROR_CHECK(esp_wifi_connect());
		}
	}

	esp_err_t loadAccessPoint(){

		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)
		);


		//SSID
		string settingsSsid = this->settings.getString("accessPointSsid");

		if (!settingsSsid.length()) {
			ESP_LOGE(this->tag.c_str(), "Failed to get setting 'accessPointSsid'");
			return ESP_FAIL;
		}

		settingsSsid.copy( (char *) this->accessPointConfig.ap.ssid, sizeof(this->accessPointConfig.ap.ssid));
		ESP_LOGI(this->tag.c_str(), "WiFi AP SSID Set to: %s", this->accessPointConfig.ap.ssid);


		this->accessPointConfig.ap.authmode			= WIFI_AUTH_OPEN;
		this->accessPointConfig.ap.max_connection	= 1;

		return esp_wifi_set_config(ESP_IF_WIFI_AP, &this->accessPointConfig);
	}
	esp_err_t loadClient() {

		ESP_ERROR_CHECK(
			esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)
		);
		
		//SSID
		string settingsSsid = this->settings.getString("clientSsid");

		if (!settingsSsid.length()) {
			ESP_LOGE(this->tag.c_str(), "Failed to get setting 'clientSsid'");
			return ESP_FAIL;
		}

		settingsSsid.copy( (char *) this->clientConfig.sta.ssid, sizeof(this->clientConfig.sta.ssid));
		ESP_LOGI(this->tag.c_str(), "WiFi Client SSID Set to: %s", this->accessPointConfig.ap.ssid);


		//Password
		string password = this->settings.getString("clientPassword");

		if (!password.length()) {
			ESP_LOGE(this->tag.c_str(), "Failed to get setting 'clientPassword'");
			return ESP_FAIL;
		}

		password.copy( (char *) this->clientConfig.sta.password, sizeof(this->clientConfig.sta.password));
		ESP_LOGI(this->tag.c_str(), "WiFi Client password set");

		
		return esp_wifi_set_config(ESP_IF_WIFI_STA, &this->clientConfig);
	}
	esp_err_t loadScan(){

		this->scanConfig.ssid = 0;
		this->scanConfig.bssid = 0;
		this->scanConfig.channel = 0;
		this->scanConfig.show_hidden = false;

		return ESP_OK;
	}
	static esp_err_t eventHandler(void * context, system_event_t *event) {

		ESP_LOGI("_WiFI", "eventHandler 1");

		WiFi *wifi = (WiFi *) context;

		switch( (int) event->event_id) {

			/************* STA ***************/
			case SYSTEM_EVENT_STA_START:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_STA_START");

				wifi->startScan();
				break;
			
			case SYSTEM_EVENT_STA_CONNECTED:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_STA_CONNECTED");
				break;

			case SYSTEM_EVENT_STA_GOT_IP:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_STA_GOT_IP");
				break;

			case SYSTEM_EVENT_STA_DISCONNECTED:

				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_STA_DISCONNECTED");

				wifi_mode_t wiFiMode;
				ESP_ERROR_CHECK(esp_wifi_get_mode(&wiFiMode));

				if ( (wiFiMode == WIFI_MODE_STA) || (wiFiMode == WIFI_MODE_APSTA) ) {

					ESP_LOGI(wifi->tag.c_str(), "STA SSID: %s", wifi->clientConfig.sta.ssid);
					// ESP_LOGI(wifi->tag.c_str(), "STA PSK: %s", wifi->clientConfig.sta.password);

					ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
				}
				break;
			
			case SYSTEM_EVENT_STA_STOP:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_STA_STOP");
				break;


			/************* AP ***************/
			case SYSTEM_EVENT_AP_START:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_AP_START");
				break;

			case SYSTEM_EVENT_AP_STACONNECTED:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_AP_STACONNECTED " MACSTR, MAC2STR(event->event_info.sta_connected.mac));
				break;

			case SYSTEM_EVENT_AP_STAIPASSIGNED:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_AP_STAIPASSIGNED");
				break;

			case SYSTEM_EVENT_AP_STADISCONNECTED:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_AP_STADISCONNECTED " MACSTR, MAC2STR(event->event_info.sta_disconnected.mac));
				break;

			case SYSTEM_EVENT_AP_STOP:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_AP_STOP");
				break;
			

			/************* SCAN ***************/
			case SYSTEM_EVENT_SCAN_DONE:
				ESP_LOGI(wifi->tag.c_str(), "SYSTEM_EVENT_SCAN_DONE");
				wifi->scanDone = true;
				break;

			// default:
				// ESP_LOGE(wifi->tag.c_str(), "WiFi event_id %d not found", event->event_id);
				// break;
		}
		
		return ESP_OK;
	}

	void startScan() {
		this->scanDone = false;
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(&this->scanConfig, false));
	}

	void restGet(HttpUri * httpUri, string path){

		if (this->scanDone) {
			this->buildSsidOptions();
		}
		
		this->startScan();

		Module::restGet(httpUri, path);
	}

	void buildSsidOptions(){

		if (!this->scanDone) {
			return;
		}

		uint16_t ap_num = DEFAULT_SCAN_LIST_SIZE;
		wifi_ap_record_t ap_records[DEFAULT_SCAN_LIST_SIZE];
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

		LOGI("Found %d access points:", ap_num);

		cJSON *options = cJSON_CreateObject();

		if (!options) {
			LOGE("Failed to create options to store WiFi options");
			return;
		}

		for(int i = 0; i < ap_num; i++) {

			char optionLabel[128];

			char * ssid = (char *)ap_records[i].ssid;

			sprintf(optionLabel, "%-24s (Auth: %-12s Primary: %-7d RSSI: %-4d)",
				ssid,
				this->getAuthModeName(this, ap_records[i].authmode).c_str(),
				ap_records[i].primary,
				ap_records[i].rssi
			);

			cJSON_AddStringToObject(options, ssid, optionLabel);
		}

		cJSON *clientSsid = this->settings.getSetting("clientSsid");
		cJSON_ReplaceItemInObject(clientSsid, "options", options);
	}

	static string getAuthModeName(Module * module, wifi_auth_mode_t auth_mode) {

		switch (auth_mode){
			case WIFI_AUTH_OPEN: return "OPEN";
			case WIFI_AUTH_WEP: return "WEP";
			case WIFI_AUTH_WPA_PSK: return "WPA PSK";
			case WIFI_AUTH_WPA2_PSK: return "WPA2 PSK";
			case WIFI_AUTH_WPA_WPA2_PSK: return "WPA WPA2 PSK";
			case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 ENTERPRISE";
			case WIFI_AUTH_MAX: return "MAX";
		}

		return "Unkown";
	}
};