#pragma once

#include "string.h"

#include <esp_system.h>

#include "NVS.hpp"
#include "cJSON.h"

class ModuleSettings{

	private:
		const char * tag = "ModuleSettings";
		NVS nvs;
		char *name;
		bool initalised = false;
		char * uniqueIdValue;
		char * uniqueIdKey = (char *) "[UID]";

	public:
		cJSON *settings;
		cJSON *json;
		cJSON * routing;

		ModuleSettings() {

			uint8_t mac[6];
			esp_read_mac(mac, ESP_MAC_WIFI_STA);

			static char uniqueIdValue[8];

			uniqueIdValue[0] = 'a' + ((mac[3] >> 0) & 0x0F);
			uniqueIdValue[1] = 'a' + ((mac[3] >> 4) & 0x0F);
			uniqueIdValue[2] = 'a' + ((mac[4] >> 0) & 0x0F);
			uniqueIdValue[3] = 'a' + ((mac[4] >> 4) & 0x0F);
			uniqueIdValue[4] = 'a' + ((mac[5] >> 0) & 0x0F);
			uniqueIdValue[5] = 'a' + ((mac[5] >> 4) & 0x0F);
			uniqueIdValue[6] = 0;

			this->uniqueIdValue = uniqueIdValue;
		}
	
		esp_err_t loadFile(const char * file) {

			cJSON *json = cJSON_Parse(file);

			this->json = json;
			
			if (!cJSON_IsObject(this->json)){
				return ESP_FAIL;
			}

			cJSON * name = cJSON_GetObjectItemCaseSensitive(this->json, "name");

			if (!cJSON_IsString(name)) {
				ESP_LOGE(this->tag, "Missing name in json");
				return ESP_FAIL;
			}

			this->name = name->valuestring;

			this->nvs.open(this->name);

			this->settings = cJSON_GetObjectItemCaseSensitive(this->json, "settings");

			if (!this->settings) {
				this->settings = cJSON_CreateArray();
				cJSON_AddItemToObject(this->json, "settings", this->settings);
				this->settings = cJSON_GetObjectItemCaseSensitive(this->json, "settings");
			}

			ESP_LOGW(this->name, "Setting Set");

			if (!cJSON_IsArray(this->settings)) {
				ESP_LOGE(this->name, "Settings must be an array in json");
				return ESP_FAIL;
			}

			this->load();

			this->initalised = true;

			return ESP_OK;
		}

		void addSetting(cJSON * setting) {

			if (!this->settings) {
				ESP_LOGE(this->name, "settings not initalised before adding setting.");
				ESP_ERROR_CHECK(ESP_FAIL);
			}

			cJSON_AddItemReferenceToArray(this->settings, setting);
		}

		cJSON * cloneJSON(cJSON * json) {

			char * jsonString = cJSON_Print(json);

			if (!jsonString) {
				return NULL;
			}

			cJSON * clone = cJSON_Parse(jsonString);

			free(jsonString);

			return clone;
		}

		void save(cJSON * newSettings) {

			if (!cJSON_IsArray(newSettings)) {
				ESP_LOGE(this->tag, "Setgins passed is not an array");
				return;
			}

			cJSON *newSetting;
			cJSON_ArrayForEach(newSetting, newSettings) {
				
				cJSON *newSettingName = cJSON_GetObjectItemCaseSensitive(newSetting, "name");

				if (!cJSON_IsString(newSettingName)) {
					ESP_LOGE(this->tag, "JSON setting 'name' must be  string");
					continue;
				}
				
				cJSON *newSettingValue = cJSON_GetObjectItemCaseSensitive(newSetting, "value");
				
				if (!newSettingValue) {
					ESP_LOGE(this->tag, "JSON setting 'value' for %s must be present", newSettingName->valuestring);
					continue;
				}

				this->setValue(newSettingName->valuestring, newSettingValue);
			}

		}

		void load(){
			
			cJSON * setting;
    		cJSON_ArrayForEach(setting, this->settings) {

				char * settingName = this->getSettingName(setting);

				cJSON * value = this->nvs.getJSON(settingName);
				
				if (!value){

					ESP_LOGW(this->tag, "Loading of %s->%s failed, using default.", this->name, settingName);

					cJSON * defaultValue = cJSON_GetObjectItemCaseSensitive(setting, "default");

					if (!defaultValue) {
						ESP_LOGE(this->tag, "%s->%s requires a default value", this->name, settingName);
						ESP_ERROR_CHECK(ESP_FAIL);
					}

					value = this->cloneJSON(defaultValue);

					this->setValue(settingName, value);
				}

				cJSON * existingValue = cJSON_GetObjectItemCaseSensitive(setting, "value");

				if (existingValue) {
					cJSON_ReplaceItemInObject(setting, "value", value);
				}
				else{
					cJSON_AddItemToObject(setting, "value", value);
				}
				
			}

		}

		char * getSettingName(cJSON * setting) {

			cJSON * name = cJSON_GetObjectItemCaseSensitive(setting, "name");

			if (!cJSON_IsString(name)) {
				ESP_LOGE(this->tag, "Module name must be a string.");
				return NULL;
			}

			return name->valuestring;
		}



		char * getName(){

			if (!this->initalised) {
				return NULL;
			}

			cJSON * name = cJSON_GetObjectItemCaseSensitive(this->json, "name");

			if (!cJSON_IsString(name)) {
				return NULL;
			}

			return name->valuestring;
		}

		cJSON *getSetting(const char * name) {

			cJSON *setting;
			cJSON_ArrayForEach(setting, this->settings) {

				cJSON *settingName = cJSON_GetObjectItemCaseSensitive(setting, "name");

				if (!cJSON_IsString(settingName)) {
					continue;
				}

				if (strcmp(settingName->valuestring, name) != 0) {
					continue;
				}

				return setting;
			}

			return NULL;
		}

		void setValue(char * name, cJSON * value) {

			if (!value) {
				ESP_LOGE(this->name, "Value passed to setValue is invalid");
				return;
			}

			cJSON *newValue = this->cloneJSON(value);

			if (!newValue) {
				ESP_LOGE(this->name, "Failed to clone value in setValue");
				return;
			}

			// If new value is a string, perform replacements
			if (cJSON_IsString(newValue)) {

				char * replaced = this->stringReplace(newValue->valuestring, this->uniqueIdKey, this->uniqueIdValue);

				if (replaced) {
					cJSON_Delete(newValue);
					newValue = cJSON_CreateString(replaced);
				}
			}

			cJSON * setting = this->getSetting(name);

			if (!setting) {
				ESP_LOGE(this->name, "Failed to get setting %s", name);
				return;
			}

			cJSON * existingValue = cJSON_GetObjectItemCaseSensitive(setting, "value");

			if (existingValue) {

				if (newValue->type != existingValue->type) {
					ESP_LOGE(this->tag, "Trying to save JSON value type 0x%02x over 0x%02x not allowed.", newValue->type, existingValue->type);
					return;
				}

				cJSON_ReplaceItemInObject(setting, "value", newValue);
			}
			else{
				cJSON_AddItemToObject(setting, "value", newValue);
			}

			this->nvs.setJSON(name, newValue);
		}

		cJSON * getValue(const char * name) {

			if (!this->initalised) {
				return NULL;
			}

			cJSON * setting = this->getSetting(name);

			if (!setting) {
				ESP_LOGE(this->name, "Failed to get setting %s", name);
				return NULL;
			}

			cJSON * value = cJSON_GetObjectItemCaseSensitive(setting, "value");
			
			if (!value) {
				ESP_LOGE(this->name, "Value not found, shoul dhave been loaded.");
				ESP_ERROR_CHECK(ESP_FAIL);
			}
			
			return value;
		}

		char * getString(const char * name){

			if (!this->initalised) {
				return NULL;
			}

			cJSON * string = this->getValue(name);

			if (!cJSON_IsString(string)) {
				return NULL;
			}

			return string->valuestring;
		}

		cJSON * getNumber(const char * name) {

			if (!this->initalised) {
				return NULL;
			}

			cJSON * number = this->getValue(name);

			if (!cJSON_IsNumber(number)) {
				return NULL;
			}

			return number;
		}

		int getInt(const char * name) {

			cJSON * number = this->getNumber(name);

			if (!number) {
				return -1;
			}
			
			return number->valueint;
		}

		double getDouble(const char * name) {

			cJSON * number = this->getNumber(name);

			if (!number) {
				return -1;
			}
			
			return number->valuedouble;
		}

		// You must free the result if result is non-NULL.
		char *stringReplace(char *orig, char *rep, char *with) {

			char *result; // the return string
			char *ins;    // the next insert point
			char *tmp;    // varies
			int len_rep;  // length of rep (the string to remove)
			int len_with; // length of with (the string to replace rep with)
			int len_front; // distance between rep and end of last rep
			int count;    // number of replacements

			// sanity checks and initialization
			if (!orig || !rep) {
				return NULL;
			}

			len_rep = strlen(rep);

			if (len_rep == 0){
				return NULL; // empty rep causes infinite loop during count
			}

			if (!with) {
				with[0] = '\0';
			}

			len_with = strlen(with);

			// count the number of replacements needed
			ins = orig;

			for (count = 0; (tmp = strstr(ins, rep)); ++count) {
				ins = tmp + len_rep;
			}

			tmp = result = (char *) malloc(strlen(orig) + (len_with - len_rep) * count + 1);

			if (!result){
				return NULL;
			}

			// first time through the loop, all the variable are set correctly
			// from here on,
			//    tmp points to the end of the result string
			//    ins points to the next occurrence of rep in orig
			//    orig points to the remainder of orig after "end of rep"
			while (count--) {
				ins = strstr(orig, rep);
				len_front = ins - orig;
				tmp = strncpy(tmp, orig, len_front) + len_front;
				tmp = strcpy(tmp, with) + len_with;
				orig += len_front + len_rep; // move to next "end of rep"
			}

			strcpy(tmp, orig);
			return result;
		}

};