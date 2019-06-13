#include <driver/adc.h>

#include "components.h"
#include "device.h"

static component_t component = {
	.name = "Die Hall",
	.messagesIn = 0,
	.messagesOut = 1,
};

static const char config_html_start[] asm("_binary_die_hall_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/die_hall_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static unsigned int timerCount;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSu32(nvsHandle, "timerCount", timerCount);
}

static void loadNVS(nvs_handle nvsHandle){
	timerCount =		componentsGetNVSu32(nvsHandle, "timerCount", 1);
}

static unsigned char queueItem;
static void task(void * arg) {

	int count = 0;

	while (true) {

		if (componentReadyWait("Wake Timer") != ESP_OK) {
			continue;
		}

		componentSetReady(&component);

		if (componentQueueRecieve(&component, "Wake Timer", &queueItem) != ESP_OK) {
			continue;
		}

		// Skip if 0 / disabled
		if (!timerCount) {
			continue;
		}

		if (++count < timerCount) {
			continue;
		}

		count = 0;

		static message_t message;
		strcpy(message.deviceName, deviceGetUniqueName());
		strcpy(message.sensorName, component.name);

		message.valueType = MESSAGE_INT;
		message.intValue = hall_sensor_read();

		componentSendMessage(&component, &message);
	}

	vTaskDelete(NULL);
	return;
}


void dieHallInit(void) {

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;
	component.queueItemLength	= sizeof(queueItem);
	component.queueLength		= 1;

	componentsAdd(&component);
}