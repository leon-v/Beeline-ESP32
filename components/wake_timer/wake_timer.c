#include "components.h"

static component_t component = {
	.name				= "Wake Timer",
};

static const char config_html_start[] asm("_binary_wake_timer_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/wake_timer_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static uint32_t time;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSu32(nvsHandle, "time", time);
}

static void loadNVS(nvs_handle nvsHandle){
	time =		componentsGetNVSu32(nvsHandle, "time", 60000);
}

static unsigned char queueItem = 1;
static void task(void * arg) {

	while (true){

		ESP_LOGW(component.name, "Delay %d ms", time);

		vTaskDelay(time / portTICK_RATE_MS);

		ESP_LOGW(component.name, "Trigger");

		componentsQueueSend(&component, &queueItem);
	}

	vTaskDelete(NULL);
	return;
}

void wakeTimerInit(void){

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;

	componentsAdd(&component);
}