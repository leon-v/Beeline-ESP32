#pragma once

#include "nvs_flash.h"
#include "esp_log.h"
#include "cJSON.h"

class NVS{
	private:
		const char * tag = "NVS";
		nvs_handle nvsHandle;

	public:

		NVS(){

		}
		
		NVS(char * name) {
			this->open(name);
		}

		void open(char * name){
			ESP_ERROR_CHECK(
				nvs_open(name, NVS_READWRITE, &this->nvsHandle)
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

		void setJSON(char * name, cJSON * value) {
			
			char * json = cJSON_Print(value);

			ESP_ERROR_CHECK(
				nvs_set_str(this->nvsHandle, name, json)
			);

			free(json);
		}

		cJSON * getJSON(char * name) {
			
			size_t length = 0;

			ESP_ERROR_CHECK_WITHOUT_ABORT(
				nvs_get_str(this->nvsHandle, name, NULL, &length)
			);

			if (length <= 0) {
				ESP_LOGE(this->tag, "Zero length while getting %s", name);
				return NULL;
			}

			char * json = (char *) malloc((length + 1) * sizeof(char));

			if (!json) {
				ESP_LOGE(this->tag, "Failed to allocate memory to load JSON string for %s", name);
				return NULL;
			}

			ESP_ERROR_CHECK(
				nvs_get_str(this->nvsHandle, name, json, &length)
			);

			cJSON * value = cJSON_Parse(json);

			if (!value) {
				ESP_LOGE(this->tag, "Failed parse JSON value for %s", name);
				return NULL;
			}

			return value;
		}
};