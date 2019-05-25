// Application includes
#include "components.h"

#define TAG "Components"

unsigned char componentsLength = 0;
component_t * components[10];

void componentsAdd(component_t * pComponent){

	components[componentsLength] = pComponent;
	componentsLength++;

	ESP_LOGI(pComponent->name, "Add");
}

void componentsLoadNVS(component_t * pComponent){

	if (pComponent->loadNVS == NULL){
		return;
	}

	nvs_handle nvsHandle;
	esp_err_t espError;

	espError = nvs_open(pComponent->name, NVS_READONLY, &nvsHandle);

	if ( (espError == ESP_ERR_NVS_NOT_FOUND) && (pComponent->saveNVS) ) {

		ESP_ERROR_CHECK(nvs_open(pComponent->name, NVS_READWRITE, &nvsHandle));

		pComponent->loadNVS(nvsHandle);
		pComponent->saveNVS(nvsHandle);

		ESP_ERROR_CHECK(nvs_commit(nvsHandle));

		ESP_LOGW(pComponent->name, "NVS default saved");
	}
	else if (espError != ESP_OK) {

		ESP_LOGE(pComponent->name, "Abort NVS lading");

		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);

		return;
	}

	pComponent->loadNVS(nvsHandle);

	nvs_close(nvsHandle);
}

void componentsInit(void){

	printf("componentsInit\n");

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponent = components[i];

		pComponent->eventGroup = xEventGroupCreate();

		componentSetNotReady(pComponent);

		if ( (pComponent->queueItemLength) && (pComponent->queueLength) ) {
			pComponent->queue = xQueueCreate(pComponent->queueLength, pComponent->queueItemLength);
		}

		if (pComponent->messagesIn) {
			pComponent->messageQueue = xQueueCreate(3, sizeof(message_t));
		}

		if (pComponent->configPage != NULL){
			httpServerAddPage(pComponent->configPage);
		}

		componentsLoadNVS(pComponent);

		ESP_LOGI(pComponent->name, "Init");
	}
}

void componentsStart(void){
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponent = components[i];

		if (pComponent->task != NULL){
			xTaskCreate(pComponent->task, pComponent->name, 2048, NULL, 10, NULL);
		}

	}
}

component_t * componentsGet(const char * name) {

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponent = components[i];

		if (strcmp(pComponent->name, name) == 0) {
			return pComponent;
		}
	}

	ESP_LOGE(TAG, "Unable to find %s in componentsGet", name);

	return NULL;
}

esp_err_t componentReadyWait(const char * name){

	component_t * pComponent = componentsGet(name);

	if (!pComponent){
		return ESP_FAIL;
	}

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_READY, false, true, 60000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_READY)) {
		return ESP_FAIL;
	}

	ESP_LOGI(pComponent->name, " ready");

	return ESP_OK;
}

esp_err_t componentNotReadyWait(const char * name){

	component_t * pComponent = componentsGet(name);

	if (!pComponent){
		return ESP_FAIL;
	}

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_NOT_READY, false, true, 60000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_NOT_READY)) {
		return ESP_FAIL;
	}

	ESP_LOGW(pComponent->name, " not ready");

	return ESP_OK;
}

// componentsQueueSubscribe() ??
esp_err_t componentsQueueSend(component_t * pComponent, void * buffer) {

	esp_err_t espError = ESP_FAIL;
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponentTo = components[i];

		if (!pComponentTo->queueRecieveWait) {
			continue;
		}

		if (strcmp(pComponentTo->queueRecieveWait, pComponent->name) != 0) {
			continue;
		}

		if (!uxQueueSpacesAvailable(pComponentTo->queue)) {
			ESP_LOGE(pComponentTo->name, "No room in queue for %s", pComponent->name);
			continue;
		}

		espError = ESP_OK;

		xQueueSend(pComponentTo->queue, buffer, 0);
	}

	return espError;
}

esp_err_t componentQueueRecieve(component_t * pComponent, const char * name, void * buffer) {

	if (!pComponent){
		return ESP_FAIL;
	}

	if ( (!pComponent->queueItemLength) || (!pComponent->queueLength) ) {
		return ESP_FAIL;
	}

	pComponent->queueRecieveWait = name;

	if (!xQueueReceive(pComponent->queue, buffer, 60000 / portTICK_RATE_MS)) {
		pComponent->queueRecieveWait = NULL;
		return ESP_FAIL;
	}

	pComponent->queueRecieveWait = NULL;

	return ESP_OK;
}


void componentSetReady(component_t * pComponent) {

	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_READY);
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_NOT_READY);
	ESP_LOGW(pComponent->name, " is ready.");
}

void componentSetNotReady(component_t * pComponent) {

	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_READY);
	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_NOT_READY);
	ESP_LOGW(pComponent->name, " not ready.");
}

void componentsGetHTML(httpd_req_t *req, char * ssiTag){

	if (strcmp(ssiTag, "configLinks") == 0){

		for (unsigned char i=0; i < componentsLength; i++) {

			component_t * pComponent = components[i];

			if (pComponent->configPage == NULL){
				continue;
			}

			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<li>"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<a "));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "href=\""));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, pComponent->configPage->uri));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ">"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, pComponent->name));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "</li>"));
		}


	}
}

char * componentsGetNVSString(nvs_handle nvsHandle, char * string, const char * key, const char * def) {

	size_t length = 512;

	esp_err_t espError = nvs_get_str(nvsHandle, key, NULL, &length);

	if (espError == ESP_ERR_NVS_NOT_FOUND) {

		length = strlen(def) + 1;

		string = (string == NULL) ? malloc(length) : realloc(string, length);

		strcpy(string, def);

		return string;
	}

	else if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	string = (string == NULL) ? malloc(length) : realloc(string, length);

	ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, string, &length));

	return string;
}

void componentsSetNVSString(nvs_handle nvsHandle, char * string, const char * key) {

	if (!string){
		return;
	}

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, key, string));
}

// esp_err_t nvs_get_u32 (nvs_handle handle, const char* key, uint32_t* out_value);
uint32_t componentsGetNVSu32(nvs_handle nvsHandle, const char * key, const uint32_t def) {
	uint32_t value;

	esp_err_t espError = nvs_get_u32(nvsHandle, key, &value);

	if (espError == ESP_ERR_NVS_NOT_FOUND) {
		return def;
	}

	else if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return;
	}

	return value;
}

// esp_err_t nvs_set_u32 (nvs_handle handle, const char* key, uint32_t value);
void componentsSetNVSu32(nvs_handle nvsHandle, const char * key, uint32_t value) {
	ESP_ERROR_CHECK(nvs_set_u32 (nvsHandle, key, value));
}


void componentSendMessage(component_t * pComponentFrom, message_t * pMessage) {

	esp_err_t espError = ESP_FAIL;

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponentTo = components[i];

		if (!pComponentTo->messagesIn) {
			continue;
		}

		if (!uxQueueSpacesAvailable(pComponentTo->messageQueue)) {
			ESP_LOGE(pComponentTo->name, "No room in message queue for %s", pComponentFrom->name);
			continue;
		}

		espError = ESP_OK;

		xQueueSend(pComponentTo->messageQueue, pMessage, 0);
	}

	return espError;
}

esp_err_t componentMessageRecieve(component_t * pComponent, message_t * pMessage) {

	if (!pComponent){
		return ESP_FAIL;
	}

	if (!pComponent->messagesIn) {
		return ESP_FAIL;
	}

	if (!xQueueReceive(pComponent->messageQueue, pMessage, 60000 / portTICK_RATE_MS)) {
		return ESP_FAIL;
	}

	return ESP_OK;
}