// Application includes
#include <components.h>



unsigned char componentsLength = 0;
component_t * components[10];

void componentsAdd(component_t * component){

	components[componentsLength] = component;
	componentsLength++;

	printf("componentsAdd %s\n", component->name);
}

void componentsInit(void){

	printf("componentsInit\n");

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		component->eventGroup = xEventGroupCreate();



		printf("component Init %s\n", component->name);

	}
	

}

void componentsStart(void){
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		xTaskCreate(component->task, component->tag, 2048, NULL, 10, NULL);
	}
}

void componentReadyWait(char * tag){

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		if (strcmp(component->tag, )

		
	}
}