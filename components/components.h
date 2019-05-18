#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_log.h>

enum{
	COMPONENT_READY
}

typedef struct{
	const char * name;
	const char * tag;
	const unsigned int messagesIn : 1;
	const unsigned int messagesOut : 1;
	void (* task)(void *);
	wifiEventGroup eventGroup;
} component_t;

void componentsAdd(component_t * component);

void componentsInit(void);
void componentsStart(void);


#endif