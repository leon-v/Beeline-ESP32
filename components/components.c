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
	esp_err_t espError = nvs_open(pComponent->name, NVS_READONLY, &nvsHandle);

	ESP_ERROR_CHECK_WITHOUT_ABORT(espError);

	if (espError != ESP_OK) {
		ESP_LOGE(pComponent->name, "Abort NVS lading");
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

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_READY, false, true, 4000 / portTICK_RATE_MS);

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

	EventBits_t eventBits = xEventGroupWaitBits(pComponent->eventGroup, COMPONENT_NOT_READY, false, true, 4000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_NOT_READY)) {
		return ESP_FAIL;
	}

	ESP_LOGW(pComponent->name, " not ready");

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

char * componentsLoadNVSString(nvs_handle nvsHandle, char * string, const char * key) {

	size_t length = 512;

	esp_err_t espError = nvs_get_str(nvsHandle, key, NULL, &length);

	ESP_ERROR_CHECK_WITHOUT_ABORT(espError);

	if (espError != ESP_OK){
		return;
	}

	length++;

	string = (string == NULL) ? malloc(length) : realloc(string, length);

	nvs_get_str(nvsHandle, key, string, &length);

	string[length] = "\0";

	return string;
}