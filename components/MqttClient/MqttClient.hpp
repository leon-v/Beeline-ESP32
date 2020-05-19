
#pragma once

#include "Modules.hpp"

extern const char mqttClientSettingsFile[] asm("_binary_MqttClient_json_start");

class MqttClient: public Modules::Module{
	public:
	cJSON *dataTemperature;
	QueueHandle_t mutex;
	MqttClient(Modules *modules):Modules::Module(modules, string(mqttClientSettingsFile)){

		ESP_ERROR_CHECK(this->setIsSource());
		ESP_ERROR_CHECK(this->setIsSink());

	};
};