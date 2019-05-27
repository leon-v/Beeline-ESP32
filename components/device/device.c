#include "components.h"

static component_t component = {
	.name = "Device"
};

static const char config_html_start[] asm("_binary_device_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/device_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static char * uniqueName;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSString(nvsHandle, uniqueName, "uniqueName");
}

static void loadNVS(nvs_handle nvsHandle){
	uniqueName = componentsGetNVSString(nvsHandle, uniqueName, "uniqueName", "device");
}

void deviceInit(void) {

	// Set device unique ID
    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open(component.name, NVS_READWRITE, &nvsHandle));

	size_t nvsLength;
	esp_err_t espError = nvs_get_str(nvsHandle, "uniqueName", NULL, &nvsLength);

	if (espError == ESP_ERR_NVS_NOT_FOUND){
		uint8_t mac[6];
	    char id_string[8] = {0};
	    esp_read_mac(mac, ESP_MAC_WIFI_STA);

	    id_string[0] = 'a' + ((mac[3] >> 0) & 0x0F);
	    id_string[1] = 'a' + ((mac[3] >> 4) & 0x0F);
	    id_string[2] = 'a' + ((mac[4] >> 0) & 0x0F);
	    id_string[3] = 'a' + ((mac[4] >> 4) & 0x0F);
	    id_string[4] = 'a' + ((mac[5] >> 0) & 0x0F);
	    id_string[5] = 'a' + ((mac[5] >> 4) & 0x0F);
	    id_string[6] = 0;

	    ESP_LOGW(component.name, "Unique name not found, defaulting to  %s", id_string);

		ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "uniqueName", id_string));
		ESP_ERROR_CHECK(nvs_commit(nvsHandle));
		nvs_close(nvsHandle);
	}

	component.configPage	= &configPage;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;

	componentsAdd(&component);
}

char * deviceGetUniqueName(void){
	return uniqueName;
}