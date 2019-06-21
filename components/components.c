// Application includes
#include "components.h"

#define TAG "Components"

unsigned char componentsLength = 0;
component_t * components[16];

void componentsRemove(const char * name) {

	unsigned char shuffle = 0;
	for (unsigned char i=0; i < componentsLength; i++) {

		if (shuffle){
			components[i - 1] = components[i];
		}

		else{
			component_t * pComponent = components[i];

			if (strcmp(pComponent->name, name) == 0) {
				shuffle = 1;
			}
		}
	}

	if (shuffle){
		componentsLength--;
	}

	ESP_LOGW(name, "Removed");
}
void componentsAdd(component_t * pComponent){

	components[componentsLength] = pComponent;
	componentsLength++;

	ESP_LOGI(pComponent->name, "Add");
}

void componentsLoadNVS(component_t * pComponent) {

	if (!pComponent){
		ESP_LOGE(TAG, "Component passed is not valid");
		return;
	}

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

	if ( (pComponent->idleTimeout) && (pComponent->idleTimer) ) {

		if (xTimerChangePeriod(
				pComponent->idleTimer,
				(pComponent->idleTimeout * 1000) / portTICK_RATE_MS,
				0
			) != pdPASS) {
			ESP_LOGE(pComponent->name, "Failed to re-set timer period");
		}
	}

}

void componentsTimerAlarm(TimerHandle_t xTimer) {

	component_t * pComponent = (component_t *) pvTimerGetTimerID(xTimer);

	ESP_LOGW(pComponent->name, "Idle time expired");

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (eventBits & COMPONENT_TASK_RUNNING) {
		ESP_LOGW(pComponent->name, "Requesting task to end due to idle");

		xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_RUNNING);
		xEventGroupSetBits(pComponent->eventGroup, COMPONENT_TASK_END_REQUEST);
		xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_ENDED);
	}
}

esp_err_t componentsEndRequested(component_t * pComponent) {

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (eventBits & COMPONENT_TASK_END_REQUEST) {
		return ESP_OK;
	}

	return ESP_FAIL;
}

void componentsSetEnded(component_t * pComponent) {
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_RUNNING);
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_END_REQUEST);
	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_TASK_ENDED);
}

void componentsInit(void) {

	printf("componentsInit\n");

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponent = components[i];

		// Setup event group
		pComponent->eventGroup = xEventGroupCreate();

		xEventGroupClearBits(pComponent->eventGroup, COMPONENT_READY);
		xEventGroupSetBits(pComponent->eventGroup, COMPONENT_NOT_READY);

		componentsSetEnded(pComponent);

		if ( (pComponent->queueItemLength) && (pComponent->queueLength) ) {
			pComponent->queue = xQueueCreate(pComponent->queueLength, pComponent->queueItemLength);
		}

		if (pComponent->messagesIn) {
			pComponent->messageQueue = xQueueCreate(8, sizeof(message_t));
		}

		if (pComponent->configPage != NULL){
			httpServerAddPage(pComponent->configPage);
		}

		if (pComponent->statusPage != NULL){
			httpServerAddPage(pComponent->statusPage);
		}

		componentsLoadNVS(pComponent);

		if (pComponent->idleTimeout) {

			pComponent->idleTimer = xTimerCreate(
				pComponent->name,
				(pComponent->idleTimeout * 1000) / portTICK_RATE_MS,
				pdFALSE,
				(void *) pComponent,
				componentsTimerAlarm
			);

			ESP_LOGI(pComponent->name, "Starting idle timer with a timeout of %u seconds.", pComponent->idleTimeout);

			if (xTimerStart(pComponent->idleTimer, 0) != pdPASS) {
				ESP_LOGE(pComponent->name, "Timer start error");
			}
		}

		ESP_LOGI(pComponent->name, "Init");
	}
}

void componentsStartTask(component_t * pComponent) {

	if (pComponent->task == NULL){
		return;
	}

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (!(eventBits & COMPONENT_TASK_ENDED)) {
		return;
	}

	if (!pComponent->tasStackDepth) {
		pComponent->tasStackDepth = 2048;
	}

	xTaskCreate(
		pComponent->task,
		pComponent->name,
		pComponent->tasStackDepth,
		pComponent->taskArg,
		5 + pComponent->priority,
		NULL
	);

	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_TASK_RUNNING);
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_END_REQUEST);
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_TASK_ENDED);
}

void componentsUsed(component_t * pComponent) {

	if (!pComponent->idleTimer) {
		return;
	}

	ESP_LOGW(pComponent->name, "Used, Timer reset");

	if (xTimerReset(pComponent->idleTimer, 0) != pdPASS) {
		ESP_LOGE(pComponent->name, "Timer reset error");
	}

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (eventBits & COMPONENT_TASK_ENDED) {
		ESP_LOGW(pComponent->name, "Restarting task");
		componentsStartTask(pComponent);
	}
}

void componentsStart(void){
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponent = components[i];

		componentsStartTask(pComponent);

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

esp_err_t componentsQueueSend(component_t * pComponent, void * buffer) {

	componentsUsed(pComponent);

	esp_err_t espError = ESP_FAIL;
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponentTo = components[i];

		if (!pComponentTo->queueRecieveWait) {
			continue;
		}

		if (strcmp(pComponentTo->queueRecieveWait, pComponent->name) != 0) {
			continue;
		}

		componentsUsed(pComponentTo);

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
		ESP_LOGE(pComponent->name, "has no queue to listen for. Set queueItemLength and queueLength.");
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

esp_err_t componentReadyWait(const char * name){

	component_t * pComponent = componentsGet(name);

	if (!pComponent){
		return ESP_FAIL;
	}

	componentsUsed(pComponent);

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_READY, false, true, 60000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_READY)) {
		return ESP_FAIL;
	}

	componentsUsed(pComponent);

	return ESP_OK;
}

esp_err_t componentNotReadyWait(const char * name){

	component_t * pComponent = componentsGet(name);

	if (!pComponent){
		return ESP_FAIL;
	}

	componentsUsed(pComponent);

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_NOT_READY, false, true, 60000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_NOT_READY)) {
		return ESP_FAIL;
	}

	componentsUsed(pComponent);

	return ESP_OK;
}

void componentSetReady(component_t * pComponent) {

	componentsUsed(pComponent);

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (eventBits & COMPONENT_READY) {
		return;
	}

	if (!(eventBits & COMPONENT_NOT_READY)) {
		return;
	}

	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_READY);
	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_NOT_READY);

	ESP_LOGI(pComponent->name, "Changed from not ready to ready.");
}

void componentSetNotReady(component_t * pComponent) {

	componentsUsed(pComponent);

	EventBits_t eventBits = xEventGroupGetBits(pComponent->eventGroup);

	if (!(eventBits & COMPONENT_READY)) {
		return;
	}

	if (eventBits & COMPONENT_NOT_READY) {
		return;
	}

	xEventGroupClearBits(pComponent->eventGroup, COMPONENT_READY);
	xEventGroupSetBits(pComponent->eventGroup, COMPONENT_NOT_READY);

	ESP_LOGI(pComponent->name, "Changed from ready to not ready.");
}

void componentsGetHTML(httpd_req_t *req, char * ssiTag){

	char * command = strtok(ssiTag, ":");
	command = command ? command : ssiTag;

	if (strcmp(command, "configLinks") == 0){

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
	else if (strcmp(command, "statusLinks") == 0){

		for (unsigned char i=0; i < componentsLength; i++) {

			component_t * pComponent = components[i];

			if (pComponent->statusPage == NULL){
				continue;
			}

			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<li>"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<a "));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "href=\""));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, pComponent->statusPage->uri));
				ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ">"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, pComponent->name));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "</li>"));
		}


	}
	else if(strcmp(command, "routing") == 0) {

		char * name = strtok(NULL, ":");
		if (!name) {
			ESP_LOGE(TAG, "Missing NVS type");
			return;
		}

		nvs_handle nvsHandle;
		uint64_t routeBits = 0;
		if (nvs_open(name, NVS_READONLY, &nvsHandle) == ESP_OK){
			ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u64(nvsHandle, "_route", &routeBits));
			nvs_close(nvsHandle);
		}

		for (unsigned char i=0; i < componentsLength; i++) {

			component_t * pComponent = components[i];

			if (!pComponent->messagesIn){
				continue;
			}

			char iString[4];
			itoa(i, iString, 10);

			if (i > 64){
				ESP_LOGE(TAG, "Currently using uint32_t so cant use bit %d", i);
				continue;
			}

			char checked[26];
			char boolChecked[2];

			if ((routeBits >> i) & 0x01) {
				strcpy(checked, "checked=\"checked\" ");
				strcpy(boolChecked, "1");
			}
			else{
				strcpy(checked, "notChecked=\"notChecked\" ");
				strcpy(boolChecked, "0");
			}

			/*
			<input name="nvs:name:bit:_route:0" type="hidden" id="__route0" value="<!--#nvs:name:bit:_route:0-->">
			<input type="checkbox" id="_route0" for="__route0" onchange="checkboxChange(this)" <!--#nvs:name:checked:_route:0--> >
			<label for="__route0">Forward to LoRa</label><br>
			*/

			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<input "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "name=\"nvs:"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, name));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ":bit:_route:"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, iString));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "type=\"hidden\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "id=\"__route"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, iString));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "value=\""));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, boolChecked));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ">"));

			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<input "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "type=\"checkbox\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "id=\"_route"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, iString));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "for=\"__route"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, iString));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "onchange=\"checkboxChange(this)\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, checked));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ">"));

			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<label "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "for=\"__route"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, iString));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "\" "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ">"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Forward to "));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, pComponent->name));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "</label>"));
			ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "<br>"));
		}


	}
}

char * componentsGetNVSString(nvs_handle nvsHandle, char * string, const char * key, const char * def) {

	size_t length = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;

	esp_err_t espError = nvs_get_str(nvsHandle, key, NULL, &length);

	if (espError == ESP_ERR_NVS_NOT_FOUND) {

		length = strlen(def) + 1;

		string = (string == NULL) ? malloc(length) : realloc(string, length);

		strcpy(string, def);

		return string;
	}

	else if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
		return string;
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

float componentsGetNVSFloat(nvs_handle nvsHandle, const char * key, const uint8_t def) {
	float value;

	size_t length = sizeof(float);
	esp_err_t espError = nvs_get_blob(nvsHandle, key, &value, &length);

	if (espError == ESP_ERR_NVS_NOT_FOUND) {
		return def;
	}

	else if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
	}

	return value;
}

void componentsSetNVSFloat(nvs_handle nvsHandle, const char * key, float value) {
	ESP_ERROR_CHECK(nvs_set_blob(nvsHandle, key, &value, sizeof(float)));
}

uint8_t componentsGetNVSu8(nvs_handle nvsHandle, const char * key, const uint8_t def) {
	uint8_t value;

	esp_err_t espError = nvs_get_u8(nvsHandle, key, &value);

	if (espError == ESP_ERR_NVS_NOT_FOUND) {
		return def;
	}

	else if (espError != ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
	}

	return value;
}

void componentsSetNVSu8(nvs_handle nvsHandle, const char * key, uint8_t value) {
	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, key, value));
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
	}

	return value;
}

// esp_err_t nvs_set_u32 (nvs_handle handle, const char* key, uint32_t value);
void componentsSetNVSu32(nvs_handle nvsHandle, const char * key, uint32_t value) {
	ESP_ERROR_CHECK(nvs_set_u32 (nvsHandle, key, value));
}

void componentLogMessage(component_t * pComponent, message_t * pMessage, const char * prefix) {

	char * deviceName = pMessage->deviceName ? pMessage->deviceName : "ERROR";
	char * sensorName = pMessage->sensorName ? pMessage->sensorName : "ERROR";

	switch (pMessage->valueType){

		case MESSAGE_INT:
			ESP_LOGI(pComponent->name, "%s %s/%s/int/%d",
				prefix,
				deviceName,
				sensorName,
				pMessage->intValue
			);
		break;

		case MESSAGE_FLOAT:
			ESP_LOGI(pComponent->name, "%s %s/%s/float/%.4f",
				prefix,
				deviceName,
				sensorName,
				pMessage->floatValue
			);
		break;

		case MESSAGE_DOUBLE:
			ESP_LOGI(pComponent->name, "%s %s/%s/double/%.8f",
				prefix,
				deviceName,
				sensorName,
				pMessage->doubleValue
			);
		break;

		case MESSAGE_STRING:
			ESP_LOGI(pComponent->name, "%s %s/%s/string/%s",
				prefix,
				deviceName,
				sensorName,
				pMessage->stringValue
			);
		break;

		default:
			ESP_LOGI(pComponent->name, "%s %s/%s: Type %d unhandeled",
				prefix,
				deviceName,
				sensorName,
				pMessage->valueType
			);
		break;
	}
}

void componentSendMessage(component_t * pComponentFrom, message_t * pMessage) {

	// componentLogMessage(pComponentFrom, pMessage, "Forwarding ");
	componentsUsed(pComponentFrom);

	pComponentFrom->messagesSent++;

	nvs_handle nvsHandle;
	uint64_t routeBits = 0;
	if (nvs_open(pComponentFrom->name, NVS_READONLY, &nvsHandle) == ESP_OK){
		ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_get_u64(nvsHandle, "_route", &routeBits));
		nvs_close(nvsHandle);
	}

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * pComponentTo = components[i];

		if (!pComponentTo->messagesIn) {
			continue;
		}

		if ( ((routeBits >> i) & 0x01) == 0){
			continue;
		}

		componentsUsed(pComponentTo);

		pComponentTo->messagesRecieved++;

		if (!uxQueueSpacesAvailable(pComponentTo->messageQueue)) {
			ESP_LOGE(pComponentTo->name, "No room in message queue for %s", pMessage->sensorName);
			continue;
		}

		// componentLogMessage(pComponentTo, pMessage, "Got ");

		xQueueSend(pComponentTo->messageQueue, pMessage, 0);
	}
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

	pComponent->messagesHandeled++;

	return ESP_OK;
}