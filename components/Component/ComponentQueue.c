#include "Component.h"

#define TAG "Component Queue"

esp_err_t componentQueueSettingsInit(pComponent_t pSourceComponent) {

    if (!pSourceComponent->messageSource) {
        return ESP_OK;
    }

    cJSON * variables = cJSON_GetObjectItemCaseSensitive(pSourceComponent->settingsJSON, "variables");

    if (!cJSON_IsObject(variables)) {
        ESP_LOGE(pSourceComponent->name, "Unable to find variables for %s in %s", pSourceComponent->name, __func__);
        return ESP_FAIL;
    }

    cJSON * routing = cJSON_CreateObject();

    cJSON_AddStringToObject(routing, "inputType", "multiCheckbox");
    cJSON_AddStringToObject(routing, "type", "stringArray");
    cJSON_AddStringToObject(routing, "description", "Check the components that you wish to forward messages to.");
    
    cJSON * options = cJSON_CreateObject();
    cJSON_AddItemToObject(routing, "options", options);

    cJSON_AddItemToObject(variables, "Routing", routing);


    int componentsLength = componentGetComponentsLength();

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pSinkComponent = componenetGetByIndex(index);

        if (!pSinkComponent->messageSink) {
            continue;
        }

        cJSON_AddStringToObject(options, pSinkComponent->name, pSinkComponent->name);

        ESP_LOGI(TAG, "Adding message sink routing option '%s' to source '%s'", pSinkComponent->name, pSourceComponent->name);
    }

    return ESP_OK;
}

esp_err_t componentQueueInit(pComponent_t pComponent) {

    if (pComponent->messageSink) {

        pComponent->queue = xQueueCreate(3, sizeof(componentMessage_t));
    }

    componentQueueSettingsInit(pComponent);

    return ESP_OK;
}

esp_err_t componentQueueRecieve(pComponent_t pComponent, pComponentMessage_t pMessage) {

    if (!xQueueReceive(pComponent->queue, pMessage, 60000 / portTICK_RATE_MS)) {
        ESP_LOGW(pComponent->name, "Timed out waiting for message.");
		return ESP_FAIL;
	}

    return ESP_OK;
}