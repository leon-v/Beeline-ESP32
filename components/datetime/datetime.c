#include <time.h>
#include <sys/time.h>
#include <lwip/apps/sntp.h>

#include "components.h"


static component_t component = {
	.name			= "Date Time",
	.messagesIn		= 1,
	.messagesOut	= 1
};

static const char config_html_start[] asm("_binary_datetime_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/datetime_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static char * host;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSString(nvsHandle, host, "host");
}

static void loadNVS(nvs_handle nvsHandle){
	host =		componentsGetNVSString(nvsHandle, host, "host", "pool.ntp.org");
	sntp_setservername(0, host);
}

static void task(void *arg){

	sntp_setoperatingmode(SNTP_OPMODE_POLL);

	sntp_init();

	while (componentReadyWait("WiFi") != ESP_OK) {
		ESP_LOGI(component.name, "Waiting for WiFi");
	}


    time_t now = 0;
    struct tm timeinfo = { 0 };
    while(timeinfo.tm_year < (2016 - 1900)) {

    	ESP_LOGI(component.name, "Waiting for time to get updated from %s", host);

    	vTaskDelay(2000 / portTICK_PERIOD_MS);

    	time(&now);

    	localtime_r(&now, &timeinfo);
    }

    componentSetReady(&component);

    char timestamp[64] = "";
	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);

	ESP_LOGI(component.name, "%s", timestamp);

	vTaskDelete(NULL);
	return;
}

void dateTimeInit(void){
	component.configPage =	&configPage;
	component.task =		&task;
	component.loadNVS =		&loadNVS;
	component.saveNVS =		&saveNVS;

	componentsAdd(&component);
}