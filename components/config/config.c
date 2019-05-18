#include <nvs_flash.h>

#include "components.h"

static component_t component = {
	.name = "Config",
	.messagesIn = 0,
	.messagesOut = 0
};

static void task(void * arg) {

	while (1) {

		ESP_LOGW(component.name, "test");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void configInit(void) {

	esp_err_t error;
	//Initialize NVS
    error = nvs_flash_init();

    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		error = nvs_flash_init();
    }

    ESP_ERROR_CHECK(error);

    component.task = task;
	componentsAdd(&component);
}