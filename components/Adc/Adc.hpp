#pragma once

#include "Modules.hpp"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_system.h"
#include "esp_adc_cal.h"

extern const char adcSettingsFile[] asm("_binary_Adc_json_start");

class Adc: public Modules::Module{
	public:
	QueueHandle_t mutex;

	class AdcChannel{
		public:

		Adc *adc;
		string tag;
		float multiplier = 1.967834853576572;
		float offset = 0;
		uint32_t samples = 100;
		adc_unit_t unit;
		adc_atten_t attenuation;
		adc_channel_t channel;
		adc_bits_width_t width = ADC_WIDTH_BIT_12;
		esp_adc_cal_characteristics_t characteristics;
		

		AdcChannel(Adc *adc, adc_unit_t unit, adc_channel_t channel){

			this->init(adc, unit, channel);

			this->setAttenuation(ADC_ATTEN_DB_0);
		}
	
		AdcChannel(Adc *adc, adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation){

			this->init(adc, unit, channel);

			this->setAttenuation(attenuation);
		}

		void init(Adc *adc, adc_unit_t unit, adc_channel_t channel) {

			this->adc = adc;
			this->unit = unit;
			this->channel = channel;

			this->setTag();
		}

		void setTag(){

			this->tag.append(this->adc->tag);

			if (this->unit == ADC_UNIT_1){
				this->tag.append(" ADC1");
			}

			if (this->unit == ADC_UNIT_2){
				this->tag.append(" ADC2");
			}

			this->tag.append(":CH");

			this->tag.append(to_string(this->channel));
		}

		void setAttenuation(adc_atten_t attenuation){

			this->attenuation = attenuation;

			if (this->unit == ADC_UNIT_1) {
				adc1_config_width(this->width);
				adc1_config_channel_atten( (adc1_channel_t) this->channel, this->attenuation);
			}

			if (this->unit == ADC_UNIT_2) {
				adc2_config_channel_atten( (adc2_channel_t) this->channel, this->attenuation);
			}

			this->setCharacteristics();
		}

		void setCharacteristics(){
			//Characterize ADC
			esp_adc_cal_value_t calibrationType = esp_adc_cal_characterize(
				this->unit,
				this->attenuation,
				this->width,
				3300,
				&this->characteristics
			);

			if (calibrationType == ESP_ADC_CAL_VAL_EFUSE_TP) {
				SLOGI(this, "Characterized using Two Point Value");
			}

			else if (calibrationType == ESP_ADC_CAL_VAL_EFUSE_VREF) {
				SLOGI(this, "Characterized using eFuse Vref");
			}

			else {
				SLOGI(this, "Characterized using Default Vref");
			}
		}

		void setMultiplier(float multiplier){
			this->multiplier = multiplier;
		}

		void setOffset(float offset){
			this->offset = offset;
		}
	};
	vector <AdcChannel> channels;

	Adc(Modules *modules):Modules::Module(modules, string(adcSettingsFile)){

		this->setIsSource();

		this->mutex = xSemaphoreCreateMutex();
		vQueueAddToRegistry(this->mutex, "getAdc");

		if (!this->mutex){
			LOGE("Failed to create mutex");
			ESP_ERROR_CHECK(ESP_FAIL);
		}

		AdcChannel channel(this, ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_11);
		this->add(channel);

		this->load();
	}

	void add(AdcChannel channel){
		this->channels.push_back(channel);
	}

	void load(){

		//Check TP is burned into eFuse
		if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
			LOGI("eFuse Two Point: Supported");
		} else {
			LOGI("eFuse Two Point: NOT supported");
		}

		//Check Vref is burned into eFuse
		if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
			LOGI("eFuse Vref: Supported");
		} else {
			LOGI("eFuse Vref: NOT supported");
		}
	}

	double readChannel(AdcChannel *pChannel){

		double value = 0;

		for (int i = 0; i < pChannel->samples; i++) {

			xSemaphoreTake(this->mutex, portMAX_DELAY);

			int adcRawValue;

			if (pChannel->unit == ADC_UNIT_1) {
				adcRawValue = adc1_get_raw( (adc1_channel_t) pChannel->channel);
			}

			if (pChannel->unit == ADC_UNIT_2) {
				adc2_get_raw( (adc2_channel_t) pChannel->channel, pChannel->width, &adcRawValue);
			}

			value+= esp_adc_cal_raw_to_voltage(adcRawValue, &pChannel->characteristics);

			xSemaphoreGive(this->mutex);
		}

		value*= pChannel->multiplier;
		value+= pChannel->offset;
		value /= pChannel->samples;

		cJSON *valueJson = cJSON_CreateNumber(value);

		this->message.sendValue(valueJson);

		cJSON_Delete(valueJson);

		return value;
	}

	void task(){

		while (true) {

			vTaskDelay(2000 / portTICK_PERIOD_MS);

			for (AdcChannel channel: this->channels) {

				float value = this->readChannel(&channel);

				AdcChannel *pChannel = &channel;
				SLOGW(pChannel, "Voltage: %.2fmV", value);
			}
		}
	}
};