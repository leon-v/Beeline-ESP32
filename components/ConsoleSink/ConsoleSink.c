#include <Component.h>

static void task(void * arg) {

	pComponent_t pComponent = (pComponent_t) arg;

    esp_err_t espError;
    componentMessage_t message;

	while (true) {

        espError = componentQueueRecieve(pComponent, &message);

        if (espError != ESP_OK) {
            ESP_LOGW(pComponent->name, "Timed out waiting for message");
            continue;
        }

        switch (message.valueType) {
            case MESSAGE_INT:
                ESP_LOGW(pComponent->name, "Got: %s:%s = %d", message.sensorName, message.deviceName, message.intValue);
            break;
            case MESSAGE_DOUBLE:
                ESP_LOGW(pComponent->name, "Got: %s:%s = %f", message.sensorName, message.deviceName, message.doubleValue);
            break;
            case MESSAGE_STRING:
                ESP_LOGW(pComponent->name, "Got: %s:%s = %s", message.sensorName, message.deviceName, message.stringValue);
            break;
            case MESSAGE_INTERRUPT:
                ESP_LOGW(pComponent->name, "Got: %s:%s = %s", message.sensorName, message.deviceName, "MESSAGE_INTERRUPT");
            break;
        }
	}

	vTaskDelete(NULL);
	return;
}

extern const uint8_t settingsFile[] asm("_binary_ConsoleSink_json_start");
static component_t component = {
	.settingsFile	= (char *) settingsFile,
	.task			= &task,
    .messageSink    = true
};

pComponent_t consoleSinkGetComponent(void) {
	return &component;
}