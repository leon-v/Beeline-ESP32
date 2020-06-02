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
		adc_unit_t unit;
		adc_channel_t channel;
		adc_atten_t attenuation = ADC_ATTEN_DB_0;
		esp_adc_cal_characteristics_t characteristics;
		adc_bits_width_t width = ADC_WIDTH_BIT_12;
		uint32_t samples = 10;

		AdcChannel(Adc *adc, adc_unit_t unit, adc_channel_t channel){
			this->init(adc, unit, channel);
		}
	
		AdcChannel(Adc *adc, adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation){
			this->init(adc, unit, channel);
			this->attenuation = attenuation;
		}

		void init(Adc *adc, adc_unit_t unit, adc_channel_t channel){

			this->adc = adc;
			this->unit = unit;
			this->channel = channel;

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
	};
	vector <AdcChannel> channels;

	Adc(Modules *modules):Modules::Module(modules, string(adcSettingsFile)){

		this->mutex = xSemaphoreCreateMutex();
		vQueueAddToRegistry(this->mutex, "getAdc");

		if (!this->mutex){
			LOGE("Failed to create mutex");
			ESP_ERROR_CHECK(ESP_FAIL);
		}

		AdcChannel channel(this, ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_6);
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

		for (AdcChannel channel: this->channels) {
			this->loadChannel(&channel);
		}			
	}

	void loadChannel(AdcChannel *pChannel) {

		if (pChannel->unit == ADC_UNIT_1) {
			adc1_config_width(pChannel->width);
			adc1_config_channel_atten( (adc1_channel_t) pChannel->channel, pChannel->attenuation);
		}

		if (pChannel->unit == ADC_UNIT_2) {
			adc2_config_channel_atten( (adc2_channel_t) pChannel->channel, pChannel->attenuation);
		}

		//Characterize ADC
		esp_adc_cal_value_t calibrationType = esp_adc_cal_characterize(
			pChannel->unit,
			pChannel->attenuation,
			pChannel->width,
			3300,
			&pChannel->characteristics
		);

		if (calibrationType == ESP_ADC_CAL_VAL_EFUSE_TP) {
			SLOGI(pChannel, "Characterized using Two Point Value");
		}

		else if (calibrationType == ESP_ADC_CAL_VAL_EFUSE_VREF) {
			SLOGI(pChannel, "Characterized using eFuse Vref");
		}

		else {
			SLOGI(pChannel, "Characterized using Default Vref");
		}
	}

	uint32_t readChannel(AdcChannel *pChannel){

		uint32_t value = 0;

		for (int i = 0; i < pChannel->samples; i++) {

			xSemaphoreTake(this->mutex, portMAX_DELAY);

			if (pChannel->unit == ADC_UNIT_1) {
				value += adc1_get_raw( (adc1_channel_t) pChannel->channel);
			}

			if (pChannel->unit == ADC_UNIT_2) {
				int adc2RawValue;
				adc2_get_raw( (adc2_channel_t) pChannel->channel, pChannel->width, &adc2RawValue);
				value += adc2RawValue;
			}

			xSemaphoreGive(this->mutex);
		}

		value /= pChannel->samples;

		return value;
	}
	
	uint32_t convertValueToMv(AdcChannel *pChannel, uint32_t value){
		value = esp_adc_cal_raw_to_voltage(value, &pChannel->characteristics);
		return value;
	}

	uint32_t readChannelMv(AdcChannel *pChannel){

		uint32_t value = this->readChannel(pChannel);

		value = this->convertValueToMv(pChannel, value);

		return value;
	}

	void task(){

		while (true) {

			vTaskDelay(500 / portTICK_PERIOD_MS);

			for (AdcChannel channel: this->channels) {

				uint32_t value = this->readChannel(&channel);
				uint32_t voltage = this->readChannelMv(&channel);

				AdcChannel *pChannel = &channel;
				SLOGW(pChannel, "Raw: %d\tVoltage: %dmV", value, voltage);
			}
		}
	}
};