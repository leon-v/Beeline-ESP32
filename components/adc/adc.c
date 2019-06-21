#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "components.h"
#include "device.h"

#define ADC_COUNT ADC1_CHANNEL_MAX

static component_t component = {
	.name			= "ADC",
	.messagesOut	= 1
};

static const char config_html_start[] asm("_binary_adc_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/adc_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

typedef struct {
	uint32_t timerCount;
	uint32_t samples;
	adc_atten_t attenuation;
	float multiplier;
	char * name;
	uint32_t timerCounts; // Not NVS
} adc_t;

static adc_t adcs[ADC_COUNT];

static void saveNVS(nvs_handle nvsHandle){

	for (unsigned char i = 0; i < ADC_COUNT; i++) {

		adc_t * adc = &adcs[i];

		char nvsName[16];

		sprintf(nvsName, "adc%dTimerCount", i);
		componentsSetNVSu32(nvsHandle, nvsName, adc->timerCount);

		sprintf(nvsName, "adc%dSamples", i);
		componentsSetNVSu32(nvsHandle, nvsName, adc->samples);

		sprintf(nvsName, "adc%dAtten", i);
		componentsSetNVSu8(nvsHandle, nvsName, adc->attenuation);

		sprintf(nvsName, "adc%dMul", i);
		componentsSetNVSFloat(nvsHandle, nvsName, adc->multiplier);

		sprintf(nvsName, "adc%dName", i);
		componentsSetNVSString(nvsHandle, adc->name, nvsName);
	}
}

static void loadNVS(nvs_handle nvsHandle){

	for (unsigned char i = 0; i < ADC_COUNT; i++) {

		adc_t * adc = &adcs[i];

		char nvsName[16];

		sprintf(nvsName, "adc%dTimerCount", i);
		adc->timerCount = componentsGetNVSu32(nvsHandle, nvsName, 0);

		sprintf(nvsName, "adc%dSamples", i);
		adc->samples = componentsGetNVSu32(nvsHandle, nvsName, 16);

		sprintf(nvsName, "adc%dAtten", i);
		adc->attenuation = componentsGetNVSu8(nvsHandle, nvsName, ADC_ATTEN_DB_11);

		sprintf(nvsName, "adc%dMul", i);
		adc->multiplier = componentsGetNVSFloat(nvsHandle, nvsName, 1);

		sprintf(nvsName, "adc%dName", i);
		adc->name = componentsGetNVSString(nvsHandle, adc->name, nvsName, nvsName);
	}
}


static void adcCharacterize(adc_atten_t attenuation, const char * attenuationString, esp_adc_cal_characteristics_t * adc_chars) {

    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, attenuation, ADC_WIDTH_BIT_12, 3300, adc_chars);

    switch (val_type) {

    	case ESP_ADC_CAL_VAL_EFUSE_VREF:
    		ESP_LOGI(component.name, "ADC eFuse Vref will be used for %s", attenuationString);
    	break;

    	case ESP_ADC_CAL_VAL_EFUSE_TP:
    		ESP_LOGI(component.name, "ADC Two Point will be used for %s", attenuationString);
    	break;

    	default:
    		ESP_LOGI(component.name, "ADC Default will be used for %s", attenuationString);
    	break;
    }
}

static esp_adc_cal_characteristics_t adc0dbCharaterization;
static esp_adc_cal_characteristics_t adc2_5dbCharaterization;
static esp_adc_cal_characteristics_t adc6dbCharaterization;
static esp_adc_cal_characteristics_t adc11dbCharaterization;

static esp_adc_cal_characteristics_t * adGetCharaterization(adc_atten_t adc0Atten) {

	switch (adc0Atten) {
		case ADC_ATTEN_DB_0:	return &adc0dbCharaterization;
		case ADC_ATTEN_DB_2_5:	return &adc2_5dbCharaterization;
		case ADC_ATTEN_DB_6:	return &adc6dbCharaterization;
		case ADC_ATTEN_DB_11:	return &adc11dbCharaterization;
		default:				return &adc11dbCharaterization;
	}
}

static const char adc0_config_html_start[] asm("_binary_adc0_config_html_start");
static const httpPage_t adc0Page = {
	.uri	= "/adc0_config.html",
	.page	= adc0_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char adc1_config_html_start[] asm("_binary_adc1_config_html_start");
static const httpPage_t adc1Page = {
	.uri	= "/adc1_config.html",
	.page	= adc1_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char adc2_config_html_start[] asm("_binary_adc2_config_html_start");
static const httpPage_t adc2Page = {
	.uri	= "/adc2_config.html",
	.page	= adc2_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char adc3_config_html_start[] asm("_binary_adc3_config_html_start");
static const httpPage_t adc3Page = {
	.uri	= "/adc3_config.html",
	.page	= adc3_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char adc4_config_html_start[] asm("_binary_adc4_config_html_start");
static const httpPage_t adc4Page = {
	.uri	= "/adc4_config.html",
	.page	= adc4_config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static unsigned char queueItem;
static void task(void * arg) {


	httpServerAddPage(&adc0Page);
	httpServerAddPage(&adc1Page);
	httpServerAddPage(&adc2Page);
	httpServerAddPage(&adc3Page);
	httpServerAddPage(&adc4Page);

	adcCharacterize(ADC_ATTEN_DB_0		, "0db"		, &adc0dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_2_5	, "2.5db"	, &adc2_5dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_6		, "6db"		, &adc6dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_11		, "11db"	, &adc11dbCharaterization);

    static message_t message;

	while (true) {

		if (componentReadyWait("Wake Timer") != ESP_OK) {
			continue;
		}

		componentSetReady(&component);

		if (componentQueueRecieve(&component, "Wake Timer", &queueItem) != ESP_OK) {
			continue;
		}

		strcpy(message.deviceName, deviceGetUniqueName());

		for (unsigned char i = 0; i < ADC_COUNT; i++) {

			adc_t * adc = &adcs[i];

			// Skip if 0 / disabled
			if (!adc->timerCount) {
				continue;
			}

			if (++adc->timerCounts < adc->timerCount) {
				continue;
			}

			adc->timerCounts = 0;

			adc1_channel_t channel = ADC1_CHANNEL_0 + i;

			adc1_config_width(ADC_WIDTH_BIT_12);
			adc1_config_channel_atten(channel, adc->attenuation);

			esp_adc_cal_characteristics_t * adcCharaterization;
			adcCharaterization = adGetCharaterization(adc->attenuation);

			strcpy(message.sensorName, adc->name);
			message.valueType = MESSAGE_FLOAT;
			message.floatValue = 0.00;

			int adcRaw;
			uint32_t samples = 0;
			while (++samples < adc->samples) {

				ets_delay_us(2); // Reduces noise on ADC somehow
				adcRaw = adc1_get_raw(channel);

				message.floatValue+= esp_adc_cal_raw_to_voltage(adcRaw, adcCharaterization);
			}

			message.floatValue = message.floatValue / samples;

			message.floatValue = message.floatValue * adc->multiplier;

			componentSendMessage(&component, &message);
		}
	}
}

void adcInit(void){

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;
	component.queueItemLength	= sizeof(queueItem);
	component.queueLength		= 1;

	componentsAdd(&component);
}