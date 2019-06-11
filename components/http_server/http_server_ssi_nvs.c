#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

#include "http_server.h"
#include "components.h"

#define TAG "HTTP Server SSI NVS"

void httpServerSSINVSSetBit(nvs_handle nvsHandle, char * nvsKey, int bit, char * postValue){

	uint64_t value = 0;

	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u64(nvsHandle, nvsKey, &value));

	if (atoi(postValue)) {
		value|= (0x01 << bit);
	}
	else{
		value&= ~(0x01 << bit);
	}

	ESP_ERROR_CHECK(nvs_set_u64(nvsHandle, nvsKey, value));
}

void httpServerSSINVSGetBit(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){

	uint64_t value;
	char intValStr[4];
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u64(nvsHandle, nvsKey, &value));

	if ((value >> bit) & 0x01) {
		strcpy(intValStr, "1");
	}
	else{
		strcpy(intValStr, "0");
	}

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
}

void httpServerSSINVSGetChecked(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint64_t value;
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u64(nvsHandle, nvsKey, &value));

	if ((value >> bit) & 0x01) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "checked"));
	}
}

void httpServerSSINVSGetu8Selected(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, uint8_t match){

	uint8_t value;
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, nvsKey, &value));

	if (match == value) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "selected"));
	}
}

void httpServerSSINVSSetu8(nvs_handle nvsHandle, char * nvsKey, char * postValue){
	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvsHandle, nvsKey, atoi(postValue)));
}

void httpServerSSINVSGetu8(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){
	uint32_t value;
	char intValStr[8];

	ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u8(nvsHandle, nvsKey, &value));

	itoa(value, intValStr, 10);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
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

void httpServerSSINVSSetFloat(nvs_handle nvsHandle, char * nvsKey, char * postValue) {

	float value = atof(postValue);

	ESP_ERROR_CHECK(nvs_set_blob(nvsHandle, nvsKey, &value, sizeof(float)));
}

void httpServerSSINVSGetFloat(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey) {

	size_t nvsLength = sizeof(float);

	float value;

	esp_err_t espError = nvs_get_blob(nvsHandle, nvsKey, &value, &nvsLength);

	if (espError != ESP_OK) {
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	char strVal[32];

	sprintf(strVal, "%.4f", value);

	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_chunk(req, strVal, nvsLength));
}

void httpServerSSINVSSetString(nvs_handle nvsHandle, char * nvsKey, char * value){
	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, nvsKey, value));
}

void httpServerSSINVSGetString(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){

	size_t nvsLength = 0;

	char * strVal = NULL;

	esp_err_t espError;

	espError = nvs_get_str(nvsHandle, nvsKey, strVal, &nvsLength);

	if (espError != ESP_OK) {
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	if (nvsLength <= 0) {
		return;
	}

	strVal = malloc(nvsLength);

	espError = nvs_get_str(nvsHandle, nvsKey, strVal, &nvsLength);

	if (espError != ESP_OK) {
		free(strVal);
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	nvsLength--;

	ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_chunk(req, strVal, nvsLength));

	free(strVal);
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
	else if (strcmp(nvsType, "float") == 0){
		httpServerSSINVSGetFloat(req, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "u8") == 0){
		httpServerSSINVSGetu8(req, nvsHandle, nvsKey);
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
	else if (strcmp(nvsType, "u8Selected") == 0){
		char * match = strtok(NULL, ":");
		httpServerSSINVSGetu8Selected(req, nvsHandle, nvsKey, atoi(match));
	}
	else{
		ESP_LOGE(TAG, "Failed to parse NVS type: %s", nvsType);
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
	else if (strcmp(nvsType, "float") == 0){
		httpServerSSINVSSetFloat(nvsHandle, nvsKey, value);
	}
	else if (strcmp(nvsType, "u8") == 0){
		httpServerSSINVSSetu8(nvsHandle, nvsKey, value);
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

	component_t * pComponent = componentsGet(nvsName);

	if (pComponent){
		componentsLoadNVS(pComponent);
	}

}