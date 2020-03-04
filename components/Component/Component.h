#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>

#include <esp_log.h>
#include <cJSON.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_http_server.h>

#include <stdio.h>
#include <string.h>

#define MAX_COMPONENTS 16

struct component_t_;

typedef enum {
  COMPONENT_EVENT_INTERVAL_TIMEOUT = 0
} componentEvent_t;

typedef void (* componentInit_t) (struct component_t_ *);

typedef struct component_t_ {
    const char *    name;
    const char *    settingsFile;
    cJSON *         settingsJSON;
    unsigned int    resetDefaults   : 1;
    void            (* task)(void *);
    void            (* postSave)(struct component_t_ *);
    componentInit_t init;
    uint16_t        taskStackDepth;
    uint8_t         taskPriority;
    bool            messageSource   : 1;
    bool            messageSink     : 1;
    xQueueHandle    queue;
    EventGroupHandle_t eventGroup;
} component_t, * pComponent_t;

enum componentMessageType_t {
	MESSAGE_INT,
	MESSAGE_DOUBLE,
	MESSAGE_STRING,
	MESSAGE_INTERRUPT
};

typedef struct{
	char deviceName[32];
	char sensorName[32];
	int valueType;
	union {
		int intValue;
		double doubleValue;
		char stringValue[32];
	};
} componentMessage_t, * pComponentMessage_t;

void componentAdd(pComponent_t pComponent);

void componentsInit(void);
void componentsStart(void);
pComponent_t componenetGetByName(char * name);
pComponent_t componenetGetByIndex(int index);
pComponent_t componenetGetByName(char * name);
int componentGetComponentsLength(void);

char *str_replace(char *orig, char *rep, char *with);

#include "ComponentSettings.h"
#include "ComponentEvent.h"
#include "ComponentQueue.h"

#endif