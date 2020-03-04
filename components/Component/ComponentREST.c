#include "Component.h"
#include "ComponentHTTPServer.h"

#define TAG "ComponenetREST"

esp_err_t componentRESTReturnJSON(httpd_req_t * httpRequest, cJSON * responseJSON) {

    httpd_resp_set_type(httpRequest, "application/json");
    httpd_resp_set_hdr(httpRequest, "Access-Control-Allow-Origin", "*");

    char * response = cJSON_Print(responseJSON);

    ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, response));

    free(response);

    ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, NULL));

    

    return ESP_OK;
}



esp_err_t componentRESTURISetComponent(httpd_req_t * httpRequest) {

    strtok(httpRequest->uri, "/");
    strtok(NULL, "/");
    char * uriComponent = strtok(NULL, "/");


    if (!uriComponent) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "No component in URI<br>"));
        httpd_resp_send_404(httpRequest);
        return ESP_OK;
    }

    componentHTTPServerURIDecode(uriComponent);

    pComponent_t pComponent = componenetGetByName(uriComponent);

    if (!pComponent) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "Component '"));
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, uriComponent));
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "' not found"));
        httpd_resp_send_404(httpRequest);
        return ESP_OK;   
    }

    char * jsonRequest = componentHTTPServerGetPostData(httpRequest);

    cJSON * request = cJSON_Parse(jsonRequest);

    free(jsonRequest);

    cJSON * values = cJSON_GetObjectItemCaseSensitive(request, "values");

    esp_err_t espError;

    espError =  componentSettingsSetValues(pComponent, values);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed sp set settings");
    }

    cJSON_Delete(request);

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        componentRESTReturnJSON(httpRequest, pComponent->settingsJSON)
    );

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    if (pComponent->postSave) {
        ESP_LOGW(TAG, "postSave");
        pComponent->postSave(pComponent);
    }

    return ESP_OK;
}


esp_err_t componentRESTURIGetComponent(httpd_req_t * httpRequest) {
    
    strtok(httpRequest->uri, "/");
    strtok(NULL, "/");
    char * uriComponent = strtok(NULL, "/");
    char * setting = strtok(NULL, "/");


    if (!uriComponent) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "No component in URI<br>"));
        httpd_resp_send_404(httpRequest);
        return ESP_OK;
    }

    componentHTTPServerURIDecode(uriComponent);

    pComponent_t pComponent = componenetGetByName(uriComponent);

    if (!pComponent) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "Component '"));
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, uriComponent));
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "' not found"));
        httpd_resp_send_404(httpRequest);
        return ESP_OK;   
    }

    cJSON * response;

    if (setting){

        response = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, setting);

        if (!response) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "Setting '"));
            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, setting));
            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(httpRequest, "' not found"));
            httpd_resp_send_404(httpRequest);
            return ESP_OK;   
        }
    }
    else{
        response = pComponent->settingsJSON;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        componentRESTReturnJSON(httpRequest, response)
    );

    return ESP_OK;
}

esp_err_t componentRESTURIGetVersion(httpd_req_t * httpRequest) {

    cJSON * responseJSON = cJSON_CreateObject();

    cJSON_AddStringToObject(responseJSON, "firmwareName", "Beeline ESP32");

    pComponent_t pComponent = componenetGetByName("Device");

    if (pComponent) {
        cJSON_AddStringToObject(responseJSON, "deviceName", pComponent->name);
    }
    else{
        cJSON_AddStringToObject(responseJSON, "deviceName", "NO NAME");
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        componentRESTReturnJSON(httpRequest, responseJSON)
    );

    cJSON_Delete(responseJSON);

    return ESP_OK;
}

esp_err_t componentRESTURIGetComponents(httpd_req_t * httpRequest) {

    cJSON * responseJSON = cJSON_CreateObject();

    cJSON * components = cJSON_CreateArray();

    cJSON_AddItemToObject(responseJSON, "components", components);

    int componentsLength = componentGetComponentsLength();

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        if (!pComponent) {
            continue;
        }

        cJSON * name = cJSON_CreateStringReference(pComponent->name);

        cJSON_AddItemToArray(components, name);
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(
        componentRESTReturnJSON(httpRequest, responseJSON)
    );

    cJSON_Delete(responseJSON);

    return ESP_OK;
}


esp_err_t componentRESTInit(void) {
   return ESP_OK;
}


esp_err_t componentRESTStart(void) {

    static httpd_uri_t versionURIHandeler = {
	    .uri      	= "/rest/version",
	    .method   	= HTTP_GET,
	    .handler  	= &componentRESTURIGetVersion
	};

    ESP_ERROR_CHECK(componentHTTPServerRegister(&versionURIHandeler));

    static httpd_uri_t componentsURIHandeler = {
	    .uri      	= "/rest/components",
	    .method   	= HTTP_GET,
	    .handler  	= &componentRESTURIGetComponents
	};

    ESP_ERROR_CHECK(componentHTTPServerRegister(&componentsURIHandeler));

    static httpd_uri_t componentURIHandeler = {
	    .uri      	= "/rest/component/*",
	    .method   	= HTTP_GET,
	    .handler  	= &componentRESTURIGetComponent
	};

    ESP_ERROR_CHECK(componentHTTPServerRegister(&componentURIHandeler));

    static httpd_uri_t componentURIPOSTHandeler = {
	    .uri      	= "/rest/component/*",
	    .method   	= HTTP_POST,
	    .handler  	= &componentRESTURISetComponent
	};

    ESP_ERROR_CHECK(componentHTTPServerRegister(&componentURIPOSTHandeler));

    return ESP_OK;
}