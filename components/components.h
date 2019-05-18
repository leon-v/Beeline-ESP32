#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>

#define COMPONENT_READY	BIT0

typedef struct {
	const char * name;
	const unsigned int messagesIn : 1;
	const unsigned int messagesOut : 1;
	void (* task)(void *);
	EventGroupHandle_t eventGroup;
} component_t;

void componentsAdd(component_t * component);

void componentsInit(void);
void componentsStart(void);

int componentReadyWait(const char * name);
void componentSetReady(component_t * component, int ready);

#endif