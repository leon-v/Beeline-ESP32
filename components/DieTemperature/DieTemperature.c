#include <Component.h>

#include <../DeviceTimer/DeviceTimer.h>

#include <soc/sens_reg.h>

static int dieTemperatureGet (void) {
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

static void task(void * argument){

	pComponent_t pComponent = (pComponent_t)  argument;

	esp_err_t espError;
	while (true) {

		espError = componentEventWait(pComponent, deviceTimerEvent());

		if (espError != ESP_OK) {
			ESP_LOGW(pComponent->name, "Waiting for timer event failed in %s", __func__);
			continue;
		}

		ESP_LOGW(pComponent->name, "Heard Timer");

	}

	vTaskDelete(NULL);
	return;
}

extern const uint8_t settingsFile[] asm("_binary_DieTemperature_json_start");
static component_t component = {
	.settingsFile	= (char *) settingsFile,
	.task			= &task,
	.messageSource	= true
};

pComponent_t dieTemperatureGetComponent(void) {
	return &component;
}