#pragma once

#include "ModuleSettings.hpp"

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
		const char * tag = "Module";
		ModuleSettings moduleSettings;
		ModuleSettings *settings;
		char * name;
		int taskPriority = 5;
		int taskStackDepth = 2048;
		static void startTask(Module *);
		static void taskWrapper(void * arg);
		
		Module() {
			ESP_LOGI(this->tag, "Module Constuuct");
		}

		esp_err_t loadSettingsFile(const char * settingsFile){

			this->settings = &this->moduleSettings;

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

