#include "components.h"

static component_t component = {
	.name = "MQTT Client",
	.messagesIn = 1,
	.messagesOut = 1
};

static void task(void * arg) {

	int isReady;

	while (1) {

		if (!componentReadyWait("WiFi")) {
			continue;
		}

		ESP_LOGW(component.name, "test");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void mqttClientInit(void){

	component.task = task;
	componentsAdd(&component);

	ESP_LOGI(component.name, "Init");
}
