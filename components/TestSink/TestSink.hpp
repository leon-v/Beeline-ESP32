#pragma once

#include "Module.hpp"

class TestSink: public Module{
	public:
		TestSink() : Module(){
			extern const char settingsFile[] asm("_binary_TestSink_json_start");
			this->loadSettingsFile(settingsFile);
			this->setMessageSink();
		}

		void task(){
			
			while (true) {

				vTaskDelay(3000 / portTICK_PERIOD_MS);

				cJSON * message = this->recieveMessage();

				char * messageString = cJSON_Print(message);

				ESP_LOGI(this->name, "recieved: %s", messageString);

				free(messageString);

				cJSON_Delete(message);
			}
		};
};