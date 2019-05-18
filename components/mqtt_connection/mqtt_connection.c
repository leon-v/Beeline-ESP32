#include "components.h"

static component_t component = {
	.name = "MQTT Client",
	.tag = "mqtt"
	.messagesIn = 1,
	.messagesOut = 1
};

void task(void * arg) {

	while (1) {

		ESP_LOGW(component.name, "test");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void mqttInit(void){

	component.task = task;
	componentsAdd(&component);

	printf("mqttInit %s\n", component.name);
}
