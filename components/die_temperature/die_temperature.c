#include <soc/sens_reg.h>

#include "components.h"

static component_t component = {
	.name = "Die Temp",
	.messagesIn = 0,
	.messagesOut = 1,
};

static const char config_html_start[] asm("_binary_die_temperature_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/die_temperature_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

unsigned int timerCount;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSu32(nvsHandle, "timerCount", timerCount);
}

static void loadNVS(nvs_handle nvsHandle){
	timerCount =		componentsGetNVSu32(nvsHandle, "timerCount", 1);
}

static int dieSensorsGetTemperature (void) {
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    ets_delay_us(100);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    ets_delay_us(5);
    return GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
}

static unsigned char queueItem;
static void task(void * arg) {

	while (true) {

		if (componentQueueRecieve(&component, "Wake Timer", &queueItem) != ESP_OK) {
			continue;
		}

		ESP_LOGW(component.name, "Got queue item from wake timer");
	}

	vTaskDelete(NULL);
	return;
}


void dieTemperatureInit(void){

	component.configPage		= &configPage;
	component.task				= &task;
	component.loadNVS			= &loadNVS;
	component.saveNVS			= &saveNVS;
	component.queueItemLength	= sizeof(queueItem);
	component.queueLength		= 1;

	componentsAdd(&component);
}