#include <esp_http_client.h>
#include <cJSON.h>
#include <time.h>

#include "components.h"

static component_t component = {
	.name			= "Elastic",
	.messagesIn		= 1
};

static const char config_html_start[] asm("_binary_elastic_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/elastic_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static char * host;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSString(nvsHandle, host, "host");
}

static void loadNVS(nvs_handle nvsHandle){
	host =		componentsGetNVSString(nvsHandle, host, "host", "http://user:pass@elastic.example.com:9200");
}

static esp_err_t elasticEventHandler(esp_http_client_event_t *evt) {

	int httpResponseCode = 0;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            // ESP_LOGI(component.name, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            // ESP_LOGI(component.name, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            // ESP_LOGI(component.name, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            // ESP_LOGI(component.name, "HTTP_EVENT_ON_HEADER");
            // printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:

			httpResponseCode = esp_http_client_get_status_code(evt->client);

            // ESP_LOGD(component.name, "HTTP_EVENT_ON_DATA, code=%d, len=%d", httpResponseCode, evt->data_len);

            if ( (httpResponseCode < 200) || (httpResponseCode > 299) ) {
            	ESP_LOGE(component.name, "%.*s", evt->data_len, (char*)evt->data);
			}


            // if (!esp_http_client_is_chunked_response(evt->client)) {

            	// printf("%.*s", evt->data_len, (char*)evt->data);

//             	cJSON * response = NULL;
//             	cJSON * shards = NULL;
//             	cJSON * successful = NULL;

// 				response = cJSON_Parse((char*) evt->data);

// 				if (response == NULL) {
// 					ESP_LOGI(component.name, "Error parsing JSON");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				shards = cJSON_GetObjectItemCaseSensitive(response, "_shards");

// 				if (shards == NULL) {
// 					ESP_LOGI(component.name, "Error parsing JSON, Cant find _shards");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				successful = cJSON_GetObjectItemCaseSensitive(shards, "successful");

// 				if (successful == NULL) {
// 					ESP_LOGI(component.name, "Error parsing JSON, Cant find successful");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				if (!cJSON_IsNumber(successful)) {
// 					ESP_LOGI(component.name, "Error parsing JSON, Successful not number");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				if (successful->valueint <= 0){
// 					ESP_LOGI(component.name, "ERROR: %d shards were created in elastic.", successful->valueint);
// 				}else{break;
// 					ESP_LOGI(component.name, "%d shards were created in elastic.", successful->valueint);
// 				}

// EVENT_DATA_CLEANUP:

// 				if (shards) {
// 					cJSON_Delete(shards);
// 				}

// 				if (successful) {
// 					cJSON_Delete(successful);
// 				}

// 				if (response) {
// 					cJSON_Delete(response);
// 				}
            // }

            break;
        case HTTP_EVENT_ON_FINISH:
            // ESP_LOGI(component.name, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            // ESP_LOGI(component.name, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void task(void * arg) {

	componentSetReady(&component);

	while (true){

		if (componentReadyWait("WiFi") != ESP_OK) {
			continue;
		}

		if (componentReadyWait("Date Time") != ESP_OK) {
			continue;
		}

		message_t message;
    	if (componentMessageRecieve(&component, &message) != ESP_OK) {
    		continue;
    	}

		static cJSON * request;
		request = cJSON_CreateObject();

		cJSON_AddItemToObject(request, "deviceName", cJSON_CreateString(message.deviceName));

		cJSON_AddItemToObject(request, "sensorName", cJSON_CreateString(message.sensorName));

		static char deviceSensorName[sizeof(message.deviceName) + sizeof(message.sensorName) + 1];
		strcpy(deviceSensorName, message.deviceName);
		strcat(deviceSensorName, "/");
		strcat(deviceSensorName, message.sensorName);

		static char valueString[16] = {0};
		switch (message.valueType){
			case MESSAGE_INT:
				cJSON_AddItemToObject(request, deviceSensorName, cJSON_CreateNumber(message.intValue));
				cJSON_AddItemToObject(request, "integer", cJSON_CreateNumber(message.intValue));
			break;

			case MESSAGE_FLOAT:
				sprintf(valueString, "%.4f", message.floatValue);
				cJSON_AddItemToObject(request, deviceSensorName, cJSON_CreateRaw(valueString));
				cJSON_AddItemToObject(request, "float", cJSON_CreateRaw(valueString));
			break;

			case MESSAGE_DOUBLE:
				sprintf(valueString, "%.8f", message.doubleValue);
				cJSON_AddItemToObject(request, deviceSensorName, cJSON_CreateRaw(valueString));
				cJSON_AddItemToObject(request, "double", cJSON_CreateRaw(valueString));
			break;

			case MESSAGE_STRING:
				cJSON_AddItemToObject(request, deviceSensorName, cJSON_CreateString(message.stringValue));
				cJSON_AddItemToObject(request, "string", cJSON_CreateString(message.stringValue));
			break;
		}

		static time_t now;
		time(&now);

		static struct tm timeinfo;
		localtime_r(&now, &timeinfo);

		static char timestamp[64] = "";
		strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);
		cJSON_AddItemToObject(request, "timestamp", cJSON_CreateString(timestamp));

		static char * postData;
		postData = cJSON_Print(request);

		// ESP_LOGI(component.name, "%s", postData);

		static char fullURL[128];
		strcpy(fullURL, host);
		strcat(fullURL, "/beeline_");
		strcat(fullURL, message.deviceName);
		strcat(fullURL, "/router");

		ESP_LOGI(component.name, "Requesting %s", fullURL);

		static esp_http_client_config_t config = {
			.url = fullURL,
			.event_handler = elasticEventHandler,
			.auth_type = HTTP_AUTH_TYPE_BASIC,
			.method = HTTP_METHOD_POST,
		};

		esp_http_client_handle_t client = esp_http_client_init(&config);

		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_method(client, HTTP_METHOD_POST));

		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_post_field(client, postData, strlen(postData)));

		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_header(client, "Content-Type", "application/json"));

		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(client));

		esp_http_client_cleanup(client);

		free(postData);

		cJSON_Delete(request);
	}

	vTaskDelete(NULL);
}

void elasticInit(void){

	component.configPage	= &configPage;
	component.task			= &task;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;

	componentsAdd(&component);
}