// Application includes
#include "components.h"

#define TAG "Components"

unsigned char componentsLength = 0;
component_t * components[10];

void componentsAdd(component_t * component){

	components[componentsLength] = component;
	componentsLength++;

	ESP_LOGI(component->name, "Add");
}

void componentsLoadNVS(component_t * component){

	if (component->loadNVS == NULL){
		return;
	}

	nvs_handle nvsHandle;
	esp_err_t espError = nvs_open(component->name, NVS_READONLY, &nvsHandle);

	ESP_ERROR_CHECK_WITHOUT_ABORT(espError);

	if (espError != ESP_OK) {
		return;
	}

	component->loadNVS(nvsHandle);

	nvs_close(nvsHandle);
}

void componentsInit(void){

	printf("componentsInit\n");

	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		component->eventGroup = xEventGroupCreate();

		componentSetNotReady(component);

		if (component->configPage != NULL){
			httpServerAddPage(component->configPage);
		}

		componentsLoadNVS(&component);

		ESP_LOGI(component->name, "Init");
	}
}

void componentsStart(void){
	for (unsigned char i=0; i < componentsLength; i++) {

		component_t * component = components[i];

		if (component->task != NULL){
			xTaskCreate(component->task, component->name, 2048, NULL, 10, NULL);
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

	component_t * component = componentsGet(name);

	if (!component){
		return ESP_FAIL;
	}

	EventBits_t eventBits = xEventGroupWaitBits(component->eventGroup, COMPONENT_READY, false, true, 4000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_READY)) {
		return ESP_FAIL;
	}

	ESP_LOGI(component->name, " ready");

	return ESP_OK;
}

esp_err_t componentNotReadyWait(const char * name){

	component_t * component = componentsGet(name);

	if (!component){
		return ESP_FAIL;
	}

	EventBits_t eventBits = xEventGroupWaitBits(component->eventGroup, COMPONENT_NOT_READY, false, true, 4000 / portTICK_RATE_MS);

	if (!(eventBits & COMPONENT_NOT_READY)) {
		return ESP_FAIL;
	}

	ESP_LOGW(component->name, " not ready");

	return ESP_OK;
}


void componentSetReady(component_t * component) {

	xEventGroupSetBits(component->eventGroup, COMPONENT_READY);
	xEventGroupClearBits(component->eventGroup, COMPONENT_NOT_READY);
	ESP_LOGW(component->name, " is ready.");
}

void componentSetNotReady(component_t * component) {

	xEventGroupClearBits(component->eventGroup, COMPONENT_READY);
	xEventGroupSetBits(component->eventGroup, COMPONENT_NOT_READY);
	ESP_LOGW(component->name, " not ready.");
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

void componentsLoadNVSString(nvs_handle nvsHandle, char * string, const char * key) {

	size_t length = 512;
	nvs_get_str(nvsHandle, key, NULL, length);
	length++;

	string = (string == NULL) ? malloc(length) : realloc(string, length);

	nvs_get_str(nvsHandle, key, string, length);
}