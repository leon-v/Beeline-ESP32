
#pragma once

#include "Modules.hpp"
#include <soc/sens_reg.h>

extern const char dieTemperatureSettingsFile[] asm("_binary_DieTemperature_json_start");

class DieTemperature: public Modules::Module{
	public:
	cJSON *dataTemperature;
	QueueHandle_t mutex;
	DieTemperature(Modules *modules):Modules::Module(modules, string(dieTemperatureSettingsFile)){

		ESP_ERROR_CHECK(this->setIsSource());

		this->dataTemperature = this->data.add("temperature", "Temperature", "GraphLog", cJSON_CreateNumber(0));
		cJSON_AddNumberToObject(this->dataTemperature, "updateInterval", 2000);
		cJSON_AddNumberToObject(this->dataTemperature, "max", 100);
		this->updateTemperature();

	};

	void updateTemperature(){
		this->data.updateValue("temperature", cJSON_CreateNumber(this->getTemperature()));
	}

	void task(){

		this->mutex = xSemaphoreCreateMutex();
		vQueueAddToRegistry(this->mutex, "getTemperature");

		if (!this->mutex){
			LOGE("Failed to create mutex");
			ESP_ERROR_CHECK(ESP_FAIL);
		}
			
		while (true) {

			int interval = this->settings.getInt("interval");

			if (interval < 1) {
				interval = 1;
			}

			vTaskDelay(interval / portTICK_PERIOD_MS);	

			double temperature = 0;

			int samples = this->settings.getInt("samples");

			if (samples < 1) {
				samples = 1;
			}

			for (int i = 0; i < samples; i++){
				xSemaphoreTake(this->mutex, portMAX_DELAY);
				temperature+= this->getTemperature();
				xSemaphoreGive(this->mutex);
			}

			temperature = temperature / samples;

			cJSON *value = cJSON_CreateNumber(temperature);

			// this->message->send(value);

			this->message.sendValue(value);

			cJSON_Delete(value);
		}
	};

	int getTemperature (void) {
		SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
		SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
		CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
		CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
		SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
		SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
		ets_delay_us(100);
		SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
		ets_delay_us(5);
		int value = GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
		return value - 80;
	};

	void restGet(HttpUri * httpUri, string path){

		string itemPath = httpUri->getUriComponent(4);

		if (!itemPath.length()){
			this->updateTemperature();
		}

		else if (itemPath.compare("temperature") == 0){
			this->updateTemperature();
		};
		
		Module::restGet(httpUri, path);
	}
};