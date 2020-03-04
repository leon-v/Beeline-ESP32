#ifndef _COMPONENT_QUEUE_H_
#define _COMPONENT_QUEUE_H_

#include "Component.h"

esp_err_t componentQueueInit(pComponent_t pComponent);
esp_err_t componentQueueRecieve(pComponent_t pComponent, pComponentMessage_t pMessage);
esp_err_t componentQueueSettingsInit(pComponent_t pSourceComponent);

#endif