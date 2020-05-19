#pragma once

#include "ModuleSettings.hpp"
#include "ModuleMessage.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <esp_system.h>
#include <esp_event_loop.h>

#include <string>

#define TXT_FILE_PREFIX "_binary_"
#define TXT_FILE_SUFFIX "_json_start"

class Module{
	public:
		// Modules * modules;
		const char * tag = "Module";
		ModuleSettings moduleSettings;
		ModuleSettings *settings;
		ModuleMessage moduleMessage;
		ModuleMessage *message;
		char * name;
		int taskPriority = 5;
		int taskStackDepth = 2048;
		bool isMessageSource = false;
		bool isMessageSink = false;
		static void startTask(Module *);
		static void taskWrapper(void * arg);

		
		Module() {
			ESP_LOGI(this->tag, "Module Constuuct");
			this->settings = &this->moduleSettings;
			this->message = &this->moduleMessage;
		}

		void setMessageSource(){
			this->message->setSource();
		}

		void setMessageSink(){
			this->message->setSink();
		}

		void initSourceRouting(cJSON *sinksArray) {

			cJSON * routingSetting = this->message->getRoutingSetting(sinksArray);

			this->settings->addSetting(routingSetting);

			char * test = cJSON_Print(this->settings->json);
			ESP_LOGW(this->name, "Set source options: %s", test);
			free(test);
		}

		void initSinkRouting(cJSON *sinks){

			cJSON *sinkOption = this->message->getRoutingOption(this->name);

			cJSON_AddItemToArray(sinkOption);
		}

		void sendMessage(cJSON *message) {
			this->message->send(message);
		}

		cJSON *recieveMessage(){
			return this->message->recieve();
		}

		esp_err_t loadSettingsFile(const char * settingsFile){

			ESP_ERROR_CHECK(this->settings->loadFile(settingsFile));

			this->name = this->settings->getName();

			if (!this->name) {
				ESP_LOGE(this->tag, "Failed to get module name from settings. Module name required.");
				return ESP_FAIL;
			}
			
			return ESP_OK;
		}

		virtual void task(){
			ESP_LOGW(this->name, "Module has no task");
		};

		virtual void reLoad(){
			ESP_LOGW(this->name, "Module has no reLoad");
		}
};

