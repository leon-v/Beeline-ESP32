#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>

#include "http_server.h"

#define COMPONENT_READY		BIT0
#define COMPONENT_NOT_READY	BIT1

typedef struct {
	const char * name;
	const unsigned int messagesIn : 1;
	const unsigned int messagesOut : 1;
	void (* task)(void *);
	void (* loadNVS)(nvs_handle nvsHandle);
	EventGroupHandle_t eventGroup;
	httpPage_t * configPage;
} component_t;

void componentsAdd(component_t * component);

void componentsInit(void);
void componentsStart(void);

component_t * componentsGet(const char * name);

esp_err_t componentReadyWait(const char * name);
esp_err_t componentNotReadyWait(const char * name);

void componentSetReady(component_t * component);
void componentSetNotReady(component_t * component);

void componentsGetHTML(httpd_req_t *req, char * ssiTag);

char * componentsLoadNVSString(nvs_handle nvsHandle, char * string, const char * key);
#endif