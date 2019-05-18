// Application includes
#include <components.h>



unsigned char componentsLength = 0;
component_t * components[10];

void componentsAdd(component_t * component){

	components[componentsLength] = component;
	componentsLength++;

	ESP_LOGI(component->name, "Add");
}

void componentsInit(void){

	printf("componentsInit\n");

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		component->eventGroup = xEventGroupCreate();

		componentSetReady(component, 0);

		ESP_LOGI(component->name, "Init");
	}


}

void componentsStart(void){
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		xTaskCreate(component->task, component->name, 2048, NULL, 10, NULL);
	}
}

int componentReadyWait(const char * name){

	EventBits_t eventBits;

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		if (strcmp(component->name, name) == 0) {

			eventBits = xEventGroupWaitBits(component->eventGroup, COMPONENT_READY, false, true, 4000 / portTICK_RATE_MS);

			if (!(eventBits & COMPONENT_READY)) {
				ESP_LOGW(component->name, " not ready yet");
				return 0;
			}

			return 1;
		}
	}

	return -1;
}

void componentSetReady(component_t * component, int isReady) {

	if (isReady){
		xEventGroupSetBits(component->eventGroup, COMPONENT_READY);
		ESP_LOGW(component->name, " is ready.");
	}
	else{
		xEventGroupClearBits(component->eventGroup, COMPONENT_READY);
		ESP_LOGW(component->name, " not ready.");
	}

}