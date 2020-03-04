#include <Component.h>

static EventBits_t event;

static void init(pComponent_t pComponent){

	event = componentEventRegister(pComponent);

	ESP_LOGI(pComponent->name, "Registered event %d", event);
}

EventBits_t deviceTimerEvent(void) {
	return event;
}

static void task(void * arg) {

	pComponent_t pComponent = (pComponent_t) arg;

	cJSON * interval;
	ESP_ERROR_CHECK(componentSettingsGet(pComponent, "Interval", &interval));

	if (!cJSON_IsNumber(interval)) {
		ESP_LOGE(pComponent->name, "Interval is not a number.");
	}

	while (true) {

		vTaskDelay(interval->valueint / portTICK_PERIOD_MS);

		ESP_ERROR_CHECK(componentEventSetAll(pComponent, event));
	}

	vTaskDelete(NULL);
	return;
}

extern const uint8_t settingsFile[] asm("_binary_DeviceTimer_json_start");
static component_t component = {
	.settingsFile	= (char *) settingsFile,
	.init			= &init,
	.task			= &task
};

pComponent_t deviceTimerGetComponent(void) {
	return &component;
}