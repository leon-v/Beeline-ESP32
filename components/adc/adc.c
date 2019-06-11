#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "components.h"
#include "device.h"

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

static uint32_t adc0TimerCount;
static adc_atten_t adc0Atten;
static float adc0Mul;
static char * adc0Name;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSu32(nvsHandle, "adc0TimerCount", adc0TimerCount);
	componentsSetNVSu8(nvsHandle, "adc0Atten", adc0Atten);
	componentsSetNVSFloat(nvsHandle, "adc0Mul", adc0Mul);
	componentsSetNVSString(nvsHandle, adc0Name, "adc0Name");
}

static void loadNVS(nvs_handle nvsHandle){
	adc0TimerCount	= componentsGetNVSu32(nvsHandle, "adc0TimerCount", 0);
	adc0Atten		= componentsGetNVSu8(nvsHandle, "adc0Atten", ADC_ATTEN_DB_11);
	adc0Mul			= componentsGetNVSFloat(nvsHandle, "adc0Mul", 1);
	adc0Name		= componentsGetNVSString(nvsHandle, adc0Name, "adc0Name", "ADC0");
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

static unsigned char queueItem;
static void task(void * arg) {


	adcCharacterize(ADC_ATTEN_DB_0		, "0db"		, &adc0dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_2_5	, "2.5db"	, &adc2_5dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_6		, "6db"		, &adc6dbCharaterization);
	adcCharacterize(ADC_ATTEN_DB_11		, "11db"	, &adc11dbCharaterization);

    static message_t message;
    int adcRaw;

    int adc0Count = 0;

    esp_adc_cal_characteristics_t * adcCharaterization;

	while (true) {

		if (componentReadyWait("Wake Timer") != ESP_OK) {
			continue;
		}

		componentSetReady(&component);

		if (componentQueueRecieve(&component, "Wake Timer", &queueItem) != ESP_OK) {
			continue;
		}

		strcpy(message.deviceName, deviceGetUniqueName());

		if (++adc0Count >= adc0TimerCount) {
			adc0Count = 0;

			adc1_config_width(ADC_WIDTH_BIT_12);
			adc1_config_channel_atten(ADC1_CHANNEL_0, adc0Atten);
			adcRaw = adc1_get_raw(ADC1_CHANNEL_0);

			adcCharaterization = adGetCharaterization(adc0Atten);
			message.floatValue = esp_adc_cal_raw_to_voltage(adcRaw, adcCharaterization);
			message.floatValue = message.floatValue * adc0Mul;

			strcpy(message.sensorName, adc0Name);
			message.valueType = MESSAGE_FLOAT;

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