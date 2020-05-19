#pragma once

#include "Module.hpp"

class TestSource: public Module{
	public:
		TestSource() : Module(){
			
			extern const char settingsFile[] asm("_binary_TestSource_json_start");
			this->loadSettingsFile(settingsFile);
			this->setMessageSource();
		}

		void task(){

			const char * messageText = "Test Source Message";
			
			while (true) {

				vTaskDelay(3000 / portTICK_PERIOD_MS);

				cJSON * message = cJSON_CreateString(messageText);

				this->sendMessage(message);

				ESP_LOGI(this->name, "Sent: %s", message->valuestring);

				cJSON_Delete(message);
			}
		};
};