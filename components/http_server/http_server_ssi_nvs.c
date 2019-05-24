#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

#include "http_server.h"

#define TAG "HTTP Server SSI NVS"

void httpServerSSINVSSetBit(nvs_handle nvsHandle, char * nvsKey, int bit, char * postValue){

	uint8_t value;

	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, nvsKey, &value));

	if (atoi(postValue)) {
		value|= (0x01 << bit);
	}
	else{
		value&= ~(0x01 << bit);
	}

	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, nvsKey, value));
}
void httpServerSSINVSGetBit(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint8_t value;
	char intValStr[4];
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, nvsKey, &value));

	value = (value >> bit) & 0x01;

	itoa(value, intValStr, 10);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
}

void httpServerSSINVSGetChecked(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint8_t value;
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, nvsKey, &value));

	value = (value >> bit) & 0x01;

	if (value){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "checked"));
	}
}

void httpServerSSINVSSetu32(nvs_handle nvsHandle, char * nvsKey, char * postValue){
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u32(nvsHandle, nvsKey, atoi(postValue)));
}

void httpServerSSINVSGetu32(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){
	uint32_t value;
	char intValStr[32];

	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u32(nvsHandle, nvsKey, &value));

	itoa(value, intValStr, 10);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
}

void httpServerSSINVSSetString(nvs_handle nvsHandle, char * nvsKey, char * value){
	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, nvsKey, value));
}

void httpServerSSINVSGetString(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){
	size_t nvsLength = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;
	char strVal[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];

	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_str(nvsHandle, nvsKey, strVal, &nvsLength));
	nvsLength--;

	if (nvsLength > 0) {
		ESP_ERROR_CHECK(httpd_resp_send_chunk(req, strVal, nvsLength));
	}

}

void httpServerSSINVSGet(httpd_req_t *req, char * ssiTag){

	char * nvsName = strtok(ssiTag, ":");
	esp_err_t espError;
	if (!nvsName) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS name"));
		return;
	}

	char * nvsType = strtok(NULL, ":");
	if (!nvsType) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS type"));
		return;
	}

	char * nvsKey = strtok(NULL, ":");
	if (!nvsKey) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS key"));
		return;
	}

	nvs_handle nvsHandle;
	espError = nvs_open(nvsName, NVS_READONLY, &nvsHandle);

	if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	if (strcmp(nvsType, "string") == 0){
		httpServerSSINVSGetString(req, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "u32") == 0){
		httpServerSSINVSGetu32(req, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "bit") == 0){

		char * bitStr = strtok(NULL, ":");
		httpServerSSINVSGetBit(req, nvsHandle, nvsKey, atoi(bitStr));
	}
	else if (strcmp(nvsType, "checked") == 0){

		char * bitStr = strtok(NULL, ":");
		httpServerSSINVSGetChecked(req, nvsHandle, nvsKey, atoi(bitStr));
	}
	else{
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Failed to parse NVS type"));
	}

	nvs_close(nvsHandle);
}


void httpServerSSINVSSet(char * ssiTag, char * value){

	char * nvsName = strtok(ssiTag, ":");
	if (!nvsName) {
		ESP_LOGE(TAG, "Missing NVS name");
		return;
	}

	char * nvsType = strtok(NULL, ":");
	if (!nvsType) {
		ESP_LOGE(TAG, "Missing NVS type");
		return;
	}

	char * nvsKey = strtok(NULL, ":");
	if (!nvsKey) {
		ESP_LOGE(TAG, "Missing NVS key");
		return;
	}


	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open(nvsName, NVS_READWRITE, &nvsHandle));

	if (strcmp(nvsType, "string") == 0){
		httpServerSSINVSSetString(nvsHandle, nvsKey, value);
	}
	else if (strcmp(nvsType, "u32") == 0){
		httpServerSSINVSSetu32(nvsHandle, nvsKey, value);
	}
	else if (strcmp(nvsType, "bit") == 0){

		char * bitStr = strtok(NULL, ":");
		httpServerSSINVSSetBit(nvsHandle, nvsKey, atoi(bitStr), value);
	}
	else{
		ESP_LOGE(TAG, "SSI NVS type %s not handled", nvsType);
	}

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}