#include <time.h>
#include <sys/time.h>
#include <lwip/apps/sntp.h>

#include "Component.h"

#define TAG "NTP Client"

void ntpClientInit(pComponent_t pComponent) {
	
	sntp_setoperatingmode(SNTP_OPMODE_POLL);

	sntp_init();

	cJSON * host;
	componentSettingsGet(pComponent, "Host", &host);

	if (!cJSON_IsString(host)) {
		ESP_LOGE(pComponent->name, "Failed to find Host setting in %s", __func__);
		return;
	}

	sntp_setservername(0, host->valuestring);
}

void ntpClientTask(void * arg) {
	
	static time_t now = 0;
    static struct tm timeinfo = { 0 };
    while(timeinfo.tm_year < (2016 - 1900)) {

    	ESP_LOGW(TAG, "Waiting for time to get updated");

    	vTaskDelay(30000 / portTICK_PERIOD_MS);

    	time(&now);

    	localtime_r(&now, &timeinfo);
    }

	ESP_LOGI(TAG, "Time updated");

	char timestamp[64] = "";
	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);

	ESP_LOGI(TAG, "%s", timestamp);

	vTaskDelete(NULL);
	return;
}

// Reflects the path in platformio.ini
extern const uint8_t settingsFile[] asm("_binary_NTPClient_json_start");

static component_t component = {
	.settingsFile	= (char *) settingsFile,
	.task			= &ntpClientTask,
	.init			= &ntpClientInit,
};

pComponent_t NTPClientGetComponent(){
	return &component;
}

