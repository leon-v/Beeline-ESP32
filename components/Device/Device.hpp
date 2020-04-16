#pragma once

#include "Module.hpp"

class Device: public Module{
	public:
		Device() : Module(){

			ESP_LOGI(this->tag, "Device Construct");

			extern const char settingsFile[] asm("_binary_Device_json_start");
			this->loadSettingsFile(settingsFile);

			// this->load();
		}

		char * getName(){
			return this->settings->getString("name");
		}
};