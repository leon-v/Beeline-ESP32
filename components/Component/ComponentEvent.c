#include "Component.h"

int eventCount = 0;

void componentEventInit(pComponent_t pComponent) {
    pComponent->eventGroup = xEventGroupCreate();
}

EventBits_t componentEventRegister(pComponent_t pComponent) {
    EventBits_t event = 0x01 << eventCount;
    eventCount++;
    return event;
}

esp_err_t componentEventWait(pComponent_t pComponent, componentEvent_t event) {

    EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, event, true, true, 60000 / portTICK_RATE_MS);

	if (!(eventBits & event)) {
		return ESP_FAIL;
	}

    return ESP_OK;
}

esp_err_t componentEventSet(pComponent_t pComponent, componentEvent_t event){
    xEventGroupSetBits(pComponent->eventGroup, event);

    return ESP_OK;
}

esp_err_t componentEventSetAll(pComponent_t pFromComponent, componentEvent_t event) {

    int componentsLength = componentGetComponentsLength();

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        if (!pComponent) {
            continue;
        }

        componentEventSet(pComponent, event);
    }
    
    return ESP_OK;
}