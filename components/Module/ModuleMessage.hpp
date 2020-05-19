#pragma once

#include "string.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <esp_system.h>
#include <esp_event_loop.h>

#include "cJSON.h"

class ModuleMessage{

	
	public:
		const char *tag = "ModuleMessage";
		size_t maxMessageSize = 128;
		xQueueHandle queue;
		EventGroupHandle_t eventGroup;
		cJSON *routingSetting;
		bool isSource = false;
		bool isSink = false;

		void setSource() {
			this->isSource = true;
		}

		void setSink() {
			this->isSink = true;

			this->queue = xQueueCreate(3, this->maxMessageSize * sizeof(char) );
		}

		cJSON * getRoutingSetting(cJSON * sinksArray) {

			this->routingSetting = cJSON_CreateObject();

			cJSON_AddStringToObject(this->routingSetting, "name", "routing");
			cJSON_AddStringToObject(this->routingSetting, "label", "Routing");
			cJSON_AddStringToObject(this->routingSetting, "inputType", "checkbox");
			cJSON_AddItemToObject(this->routingSetting, "options", sinksArray);
			// cJSON_AddItemReferenceToObject(this->routingSetting, "options", sinksArray);

			return this->routingSetting;
		}

		cJSON *getRoutingOption(char *name) {
			
		}

		void send(cJSON *message){
			
			char * messageString = cJSON_Print(message);

			// Route here? Or in modules?
			ESP_LOGI(this->tag, "send() %s", messageString);

			free(messageString);
		}

		cJSON *recieve(){
			return cJSON_CreateString("Test RX from ModuleMessage");
		}

};