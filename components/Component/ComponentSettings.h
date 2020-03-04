#ifndef _COMPONENT_SETTINGS_H_
#define _COMPONENT_SETTINGS_H_

#include "Component.h"

esp_err_t componentSettingsInit(void);
esp_err_t componentSettingsSetValues(pComponent_t pComponent, cJSON * values);
esp_err_t componentSettingsGet(pComponent_t pComponent, char * variableName, cJSON * * pValue);
esp_err_t componentSettingsLoadValues(pComponent_t pComponent);
esp_err_t componentSettingsLoadFile(pComponent_t pComponent);
esp_err_t componentSettingsInitName(pComponent_t pComponent);
esp_err_t componentSettingsSetStatus(pComponent_t pComponent, char * alert, char * message, cJSON * extra);

#endif