#ifndef _COMPONENT_EVENT_H_
#define _COMPONENT_EVENT_H_

#include "Component.h"

void componentEventInit(pComponent_t pComponent);
EventBits_t componentEventRegister(pComponent_t pComponent);
esp_err_t componentEventSetAll(pComponent_t pFromComponent, componentEvent_t event);
esp_err_t componentEventWait(pComponent_t pComponent, componentEvent_t event);

#endif