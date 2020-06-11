#pragma once

#include "Modules.hpp"

#include <time.h>
#include <math.h>
#include <esp_http_client.h>
#include <algorithm>

extern const char elasticSearchSettingsFile[] asm("_binary_ElasticSearch_json_start");

class ElasticSearch: public Modules::Module{
	public:
	string url;
	esp_http_client_config_t clientConfig;
	ElasticSearch(Modules *modules):Modules::Module(modules, string(elasticSearchSettingsFile)){
		ESP_ERROR_CHECK(this->setIsSink());
	}

	void addTime(cJSON * message){

		// If the message already has a tiemstamp, don't add one
		cJSON *messageTimestamp = cJSON_GetObjectItemCaseSensitive(message, "timestamp");
		if(messageTimestamp) {
			return;
		}

		struct timeval now;

		gettimeofday(&now, NULL);

		int millisec = lrint(now.tv_usec/1000.0); // Round to nearest millisec
		if (millisec>=1000) { // Allow for rounding up to nearest second
			millisec -=1000;
			now.tv_sec++;
		}

		struct tm* timeinfo;
		timeinfo = localtime(&now.tv_sec);

		char timestamp[64] = "";
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.", timeinfo);
		
		char millisecStr[5] = "";
		itoa(millisec, millisecStr, 10);

		strcat(timestamp, millisecStr);
		strcat(timestamp, "Z");

		cJSON_AddItemToObject(message, "timestamp", cJSON_CreateString(timestamp));
	}

	void addNamedValue(cJSON *message){

		cJSON * device = cJSON_GetObjectItemCaseSensitive(message, "device");

		if (!cJSON_IsString(device)){
			return;
		}

		cJSON * module = cJSON_GetObjectItemCaseSensitive(message, "module");

		if (!cJSON_IsString(module)){
			return;
		}

		cJSON *value = cJSON_GetObjectItemCaseSensitive(message, "value");

		if (!value){
			return;
		}

		string name = "";
		name.append(string(device->valuestring));
		name.append("/");
		name.append(string(module->valuestring));

		cJSON_AddItemReferenceToObject(message, name.c_str(), value);
	}

	void task(){
		
		while (true) {

			cJSON *message = this->message.recieve();

			if (!message) {
				continue;
			}

			this->addTime(message);

			this->addNamedValue(message);

			char *postData = cJSON_Print(message);

			if (!postData){
				LOGE("Failed to stringify JSON");
				cJSON_Delete(message);
				continue;
			}

			this->url = this->settings.getString("host");

			this->url.append("/beeline_");

			cJSON *device = cJSON_GetObjectItemCaseSensitive(message, "device");

			if (!cJSON_IsString(device)){
				LOGE("Missing device in message");
				cJSON_Delete(message);
				free(postData);
				continue;
			}

			string deviceName = string(device->valuestring);
			deviceName = this->cleanIndexName(deviceName);
			deviceName = this->uriEncode(deviceName);
			this->url.append(deviceName);

			this->url.append("/router");

			this->clientConfig.url = this->url.c_str();
			this->clientConfig.event_handler = this->eventHandler;
			this->clientConfig.user_data = this;
			this->clientConfig.auth_type = HTTP_AUTH_TYPE_BASIC;
			this->clientConfig.method = HTTP_METHOD_POST;

			esp_http_client_handle_t client = esp_http_client_init(&this->clientConfig);

			if (!client){
				LOGE("Failed to build client");
				cJSON_Delete(message);
				free(postData);
				continue;
			}

			ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_method(client, HTTP_METHOD_POST));

			ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_post_field(client, postData, strlen(postData)));

			ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_header(client, "Content-Type", "application/json"));

			esp_err_t espError = esp_http_client_perform(client);

			if (espError != ESP_OK) {
				LOGE("Failed to perform http request. ESP Error %d", espError);
			}

			esp_http_client_cleanup(client);

			// LOGI("ES: %s", postData);

			free(postData);

			cJSON_Delete(message);
		}
	};

	string cleanIndexName(string name){

		/*
		Lowercase only
		Cannot include \, /, *, ?, ", <, >, |, ` ` (space character), ,, #
		Indices prior to 7.0 could contain a colon (:), but that’s been deprecated and won’t be supported in 7.0+
		Cannot start with -, _, +
		Cannot be . or ..
		Cannot be longer than 255 bytes (note it is bytes, so multi-byte characters will count towards the 255 limit faster)
		Names starting with . are deprecated, except for hidden indices and internal indices managed by plugins
		*/
		// convert string to back to lower case
		std::transform(name.begin(), name.end(), name.begin(),[](unsigned char c){
			return std::tolower(c);
		});
		
		this->findAndReplaceAll(name,  "\\", "-");
		this->findAndReplaceAll(name,  "\"", "-");
		this->findAndReplaceAll(name,  "/", "-");
		this->findAndReplaceAll(name,  "*", "-");
		this->findAndReplaceAll(name,  "?", "-");
		this->findAndReplaceAll(name,  "<", "-");
		this->findAndReplaceAll(name,  ">", "-");
		this->findAndReplaceAll(name,  "|", "-");
		this->findAndReplaceAll(name,  " ", "-");
		this->findAndReplaceAll(name,  ",", "-");
		this->findAndReplaceAll(name,  "@", "-");
		this->findAndReplaceAll(name,  ".", "-");
		this->findAndReplaceAll(name,  "+", "-");
		this->findAndReplaceAll(name,  "_", "-");
		
		return name;
	}

	static esp_err_t eventHandler(esp_http_client_event_t *evt) {

		Module * self = (Module *) evt->user_data;

		int httpResponseCode = 0;

		switch(evt->event_id) {
			case HTTP_EVENT_ERROR:
				ESP_LOGE(self->tag.c_str(), "HTTP_EVENT_ERROR");
				break;

			case HTTP_EVENT_ON_CONNECTED:
				break;

			case HTTP_EVENT_HEADER_SENT:
				break;

			case HTTP_EVENT_ON_HEADER:
				ESP_LOGE(self->tag.c_str(), "HTTP_EVENT_ON_HEADER");
				break;

			case HTTP_EVENT_ON_DATA:
				ESP_LOGE(self->tag.c_str(), "HTTP_EVENT_ON_DATA");
				httpResponseCode = esp_http_client_get_status_code(evt->client);

				if ( (httpResponseCode < 200) || (httpResponseCode > 299) ) {
					ESP_LOGE(self->tag.c_str(), "%.*s", evt->data_len, (char*)evt->data);
				}

				break;

			case HTTP_EVENT_ON_FINISH:
				ESP_LOGE(self->tag.c_str(), "HTTP_EVENT_ON_FINISH");
				break;

			case HTTP_EVENT_DISCONNECTED:
				ESP_LOGE(self->tag.c_str(), "HTTP_EVENT_DISCONNECTED");
				break;

		}

		return ESP_OK;
	};
};