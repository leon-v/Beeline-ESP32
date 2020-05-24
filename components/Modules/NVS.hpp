#pragma once

#include <string>

#include "nvs_flash.h"
#include "esp_log.h"
#include "cJSON.h"

class NVS{
	private:
		string	tag = "NVS";
		string	name;
		nvs_handle nvsHandle;

	public:

		NVS(){
		}

		NVS(string name){

			this->name = name;

			this->open();

			this->tag.append(string("("));
			this->tag.append(this->name);
			this->tag.append(string(")"));
		}

		~NVS(){
			this->close();
		}

		void open(){
			ESP_ERROR_CHECK(
				nvs_open(this->name.c_str(), NVS_READWRITE, &this->nvsHandle)
			);
		}

		void commit(){
			ESP_ERROR_CHECK(
				nvs_commit(this->nvsHandle)
			);
		}

		void close(){
			nvs_close(this->nvsHandle);
		}

		void setJSON(string name, cJSON * value) {

			char *jsonString = cJSON_Print(value);
			ESP_LOGW(this->tag.c_str(), "setJSON: %s = %s", name.c_str(), jsonString);
			free(jsonString);
			
			char * json = cJSON_Print(value);

			ESP_ERROR_CHECK(
				nvs_set_str(this->nvsHandle, name.c_str(), json)
			);

			free(json);
		}

		cJSON * getJSON(string name) {
			
			size_t length = 0;

			ESP_ERROR_CHECK_WITHOUT_ABORT(
				nvs_get_str(this->nvsHandle, name.c_str(), NULL, &length)
			);

			if (length <= 0) {
				ESP_LOGE(this->tag.c_str(), "Zero length while getting %s", name.c_str());
				return NULL;
			}

			char * json = (char *) malloc((length + 1) * sizeof(char));

			if (!json) {
				ESP_LOGE(this->tag.c_str(), "Failed to allocate memory to load JSON string for %s", name.c_str());
				return NULL;
			}

			ESP_ERROR_CHECK(
				nvs_get_str(this->nvsHandle, name.c_str(), json, &length)
			);

			ESP_LOGW(this->tag.c_str(), "getJSON: %s = %s", name.c_str(), json);

			cJSON * value = cJSON_Parse(json);

			if (!value) {
				ESP_LOGE(this->tag.c_str(), "Failed parse JSON value for %s", name.c_str());
				return NULL;
			}

			return value;
		}
};