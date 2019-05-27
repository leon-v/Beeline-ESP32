#include <string.h>

#include "components.h"
#include "device.h"
#include "ssd1306.h"

static component_t component = {
	.name = "Display",
	.messagesIn = 1
};

static const char config_html_start[] asm("_binary_display_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/display_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static char * template;

static int messagesLength;
static message_t messages[16];

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSString(nvsHandle, template, "template");
}

static void loadNVS(nvs_handle nvsHandle){

	template = componentsGetNVSString(nvsHandle, template, "template", "Battery: [Devicename:Battery]");

	messagesLength = 0;

	char tempTemplate[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];

	strcpy(tempTemplate, template);

	char * token;
	token = strtok(tempTemplate, "[");

	while (token != NULL){

		char * device;
		device = strtok(NULL, ":]");

		char * sensor;
		sensor = strtok(NULL, "]");

		if (!sensor){
			sensor = device;
			device = deviceGetUniqueName();
		}

		if (sensor){

			if (messagesLength >= sizeof(messages)) {
				ESP_LOGE(component.name, "No room left in display value message buffer");
			}

			else{
				message_t * message = &messages[messagesLength];

				strcpy(message->deviceName, device);
				strcpy(message->sensorName, sensor);
				message->valueType = MESSAGE_STRING;
				strcpy(message->stringValue, "???");

				ESP_LOGI(component.name, "Found tag %s:%s", message->deviceName, message->sensorName);

				messagesLength++;
			}

		}

		token = strtok(NULL, "[");
	}
}

static void displayGetMessageValue(char * device, char * sensor, char * value) {

	for (int i = 0; i < messagesLength; i++){

		message_t * displayMessage = &messages[i];

		if (strcmp(displayMessage->deviceName, device) == 0) {
			if (strcmp(displayMessage->sensorName, sensor) == 0) {
				switch (displayMessage->valueType) {

					case MESSAGE_INT:
						sprintf(value, "%d", displayMessage->intValue);
					break;

					case MESSAGE_FLOAT:
						sprintf(value, "%.2f", displayMessage->floatValue);
					break;

					case MESSAGE_DOUBLE:
						sprintf(value, "%.4f", displayMessage->floatValue);
					break;

					case MESSAGE_STRING:
						strcpy(value, displayMessage->stringValue);
					break;
				}
			}
		}
	}
}

static void displayUpdate(void){

	char tempTemplate[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
	strcpy(tempTemplate, template);

	char * token;
	token = strtok(tempTemplate, "[");

	char display[CONFIG_HTTP_NVS_MAX_STRING_LENGTH] = {0};

	while (token != NULL){

		strcat(display, token);

		char * device;
		device = strtok(NULL, ":]");

		char * sensor;
		sensor = strtok(NULL, "]");

		if (!sensor){
			sensor = device;
			device = deviceGetUniqueName();
		}

		if (sensor){

			char value[32];
			displayGetMessageValue(device, sensor, value);
			strcat(display, value);
		}

		token = strtok(NULL, "[");
	}

	ssd1306SetText(display);
}

static void updateVariable(message_t * message){

	for (int i = 0; i < messagesLength; i++) {

		message_t * displayMessage = &messages[i];

		if (strcmp(displayMessage->deviceName, message->deviceName) == 0) {
			if (strcmp(displayMessage->sensorName, message->sensorName) == 0) {

				displayMessage->valueType = message->valueType;
				switch (displayMessage->valueType) {

					case MESSAGE_INT:
						displayMessage->intValue = message->intValue;
					break;

					case MESSAGE_FLOAT:
						displayMessage->floatValue = message->floatValue;
					break;

					case MESSAGE_DOUBLE:
						displayMessage->doubleValue = message->doubleValue;
					break;

					case MESSAGE_STRING:
						strcpy(displayMessage->stringValue, message->stringValue);
					break;
				}
			}
		}
	}
}

static void task(void * arg) {

	while (1) {

		message_t message;
		if (componentMessageRecieve(&component, &message) != ESP_OK) {
			continue;
		}

		updateVariable(&message);

		displayUpdate();
	}
}


void displayInit(void){

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;

	// componentsAdd(&component);
}