#include "Component.h"

#include "driver/gpio.h"

// TODO Make settings support array of component names so we can store the routing values in NVS
// Avoids IDs since they change in development
// Maybe some string with a special terminator and use strtok


#define TAG "Component Settings"

#define NVS_KEY_ADD 4

static char uniqueId[8] = {0};

esp_err_t componentSettingsGet(pComponent_t pComponent, char * variableName, cJSON * * pValue) {

    ESP_LOGD(TAG, "Loading %s->%s", pComponent->name, variableName);
    
    cJSON * variables = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "variables");

    if (!cJSON_IsObject(variables)) {
        ESP_LOGE(TAG, "Failed to get variables from %s in %s", pComponent->name, __func__);
        return ESP_FAIL;
    }

    cJSON * variable = cJSON_GetObjectItemCaseSensitive(variables, variableName);

    if (!cJSON_IsObject(variable)) {
        ESP_LOGE(TAG, "Failed to get variable '%s' for '%s' in %s", variableName, pComponent->name, __func__);
        return ESP_FAIL;
    }
    
    * pValue = cJSON_GetObjectItemCaseSensitive(variable, "value");

    if (!* pValue) {
        ESP_LOGE(TAG, "Failed to get value for %s->%s in %s", variableName, pComponent->name, __func__);
        return ESP_FAIL;
    }

    char * valueJSON = cJSON_Print(* pValue);

    ESP_LOGD(TAG, "Loaded %s->%s as %s", pComponent->name, variableName, valueJSON);

    free(valueJSON);

    return ESP_OK;
}

esp_err_t componentSettingsSetValue(pComponent_t pComponent, cJSON * variable, cJSON * value, nvs_handle nvsHandle) {

    esp_err_t espError = ESP_FAIL;

    char * variableName = variable->string;

    char * newValueJSON = cJSON_Print(value);

    ESP_LOGI(TAG, "Setting value %s->%s to %s", pComponent->name, variableName, newValueJSON);

    cJSON * newValueClone = cJSON_Parse(newValueJSON);
    
    if (cJSON_GetObjectItemCaseSensitive(variable, "value")) {
        cJSON_ReplaceItemInObjectCaseSensitive(variable, "value", newValueClone);
    }
    else{
        cJSON_AddItemToObject(variable, "value", newValueClone);
        
    }

    espError = nvs_set_str(nvsHandle, variableName, newValueJSON);
    
    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save for %s->%s in %s", pComponent->name, variableName, __func__);
    }

    free(newValueJSON);

    return ESP_OK;
}


esp_err_t componentSettingsSetValues(pComponent_t pComponent, cJSON * values) {

    esp_err_t espError;

    cJSON * variables = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "variables");

    if (!variables){
        ESP_LOGE(TAG, "JSON variables passed to %s not valid", __func__);
        return ESP_FAIL;
    }

    nvs_handle nvsHandle;
    espError = nvs_open(pComponent->name, NVS_READWRITE, &nvsHandle);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get NVSin  %s", __func__);
        return espError;
    }

    cJSON * variable;
    cJSON_ArrayForEach(variable, variables) {

        char * variableName = variable->string;

        ESP_LOGI(TAG, "setting variable %s", variableName);

        cJSON * value = cJSON_GetObjectItemCaseSensitive(values, variableName);

        if (!value) {
            ESP_LOGI(TAG, "Failed to get value for %s->%s in %s", pComponent->name, variableName, __func__);
            continue;
        }

        espError = componentSettingsSetValue(pComponent, variable, value, nvsHandle);

        if (espError != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set variable %s in %s", variableName, __func__);
            continue;
        }

    }

    espError = nvs_commit(nvsHandle);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS variable %s in %s", pComponent->name , __func__);
        nvs_close(nvsHandle);
        return espError;
    }

    nvs_close(nvsHandle);

    return ESP_OK;
}


esp_err_t componentSettingsLoadValue(pComponent_t pComponent, cJSON * variable, nvs_handle nvsHandle) {

    char * variableName = variable->string;

    ESP_LOGD(TAG, "Loading %s->%s", pComponent->name, variableName);

    esp_err_t espError = ESP_FAIL;

    size_t length;

    if (pComponent->resetDefaults) {

        ESP_LOGW(TAG, "Resetting %s->%s to defaults", variableName, pComponent->name);

        cJSON * defaultValue = cJSON_GetObjectItemCaseSensitive(variable, "default");

        if (!defaultValue) {
            defaultValue = cJSON_CreateNull();
        }

        if (cJSON_IsString(defaultValue)) {

            char * defaultValueString = str_replace(defaultValue->valuestring, "%ID%", uniqueId);

            defaultValue = cJSON_CreateString(defaultValueString);

            cJSON_ReplaceItemInObject(variable, "default", defaultValue);

            free(defaultValueString);
        }

        char * defaultValueJSON = cJSON_Print(defaultValue);
        
        espError = nvs_set_str(nvsHandle, variableName, defaultValueJSON);

        free(defaultValueJSON);

        if (espError != ESP_OK) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(espError);
            ESP_LOGE(TAG, "Failed to reset default value for %s->%s in %s", variableName, pComponent->name, __func__);
        }
    }

    // Load JSON value from NVS
    espError = nvs_get_str(nvsHandle, variableName, NULL, &length);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load NVS value for %s->%s in %s", variableName, pComponent->name, __func__);
        return espError;
    }

    char * valueJSON = malloc((length + 1) * sizeof(char));

    espError = nvs_get_str(nvsHandle, variableName, valueJSON, &length);
    
    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load string into buffer for %s->%s in %s", pComponent->name, variableName, __func__);
        return espError;
    }

    // Save JSON value to variable value

    cJSON_AddItemToObject(variable, "value", cJSON_Parse(valueJSON));

    ESP_LOGD(TAG, "Laoded %s->%s as %s", pComponent->name, variableName, valueJSON);

    free(valueJSON);

    return espError;
}
esp_err_t componentSettingsLoadValues(pComponent_t pComponent) {
    
    esp_err_t espError = ESP_FAIL;

    cJSON * variables = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "variables");

    if (!cJSON_IsObject(variables)) {
        ESP_LOGW(TAG, "Component %s has no variables, skipping loading of variables.", pComponent->name);
        return ESP_OK;
    }

    nvs_handle nvsHandle;
    espError = nvs_open(pComponent->name, NVS_READWRITE, &nvsHandle);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return espError;
    }

    cJSON * nvsKey = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "nvsKey");

    if (!cJSON_IsNumber(nvsKey)) {
        ESP_LOGE(TAG, "'nvsKey' is required in settings object.");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    pComponent->resetDefaults = 0;

    uint8_t nvsKeyu8;

    espError = nvs_get_u8(nvsHandle, "nvsKey", &nvsKeyu8);

    if (espError == ESP_ERR_NVS_NOT_FOUND) {

        pComponent->resetDefaults = 1;

        espError = nvs_set_u8(nvsHandle, "nvsKey", nvsKey->valueint);

        ESP_LOGW(TAG, "NVS value for 'nvsKey' missing.");
        ESP_LOGW(TAG, "Settig new value and resetting defaults for %s in %s.", pComponent->name, __func__);
    }

    if (!gpio_get_level(GPIO_NUM_0)) {
        ESP_LOGW(TAG, "Resetting NVS due to button pressed");
        pComponent->resetDefaults = 1;
    }

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load or save NVS value for 'nvsKey'");
        nvs_close(nvsHandle);
        return espError;
    }

    int nvsKeyCurrentValue = NVS_KEY_ADD + nvsKey->valueint;
    if (nvsKeyu8 != nvsKeyCurrentValue) {

        espError = nvs_set_u8(nvsHandle, "nvsKey", nvsKeyCurrentValue);

        ESP_LOGW(TAG, "NVS value for 'nvsKey' changed.");
        ESP_LOGW(TAG, "Settig new value and resetting defaults for %s in %s.", pComponent->name, __func__);
        pComponent->resetDefaults = 1;
    }

    cJSON * variable;
    cJSON_ArrayForEach(variable, variables) {

        ESP_ERROR_CHECK(componentSettingsLoadValue(pComponent, variable, nvsHandle));
    }

    espError = nvs_commit(nvsHandle);

    if (espError != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS variable %s in %s", pComponent->name, __func__);
        nvs_close(nvsHandle);
        return espError;
    }

    nvs_close(nvsHandle);

    return espError;
}

esp_err_t componentSettingsInit(void) {

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    uniqueId[0] = 'a' + ((mac[3] >> 0) & 0x0F);
    uniqueId[1] = 'a' + ((mac[3] >> 4) & 0x0F);
    uniqueId[2] = 'a' + ((mac[4] >> 0) & 0x0F);
    uniqueId[3] = 'a' + ((mac[4] >> 4) & 0x0F);
    uniqueId[4] = 'a' + ((mac[5] >> 0) & 0x0F);
    uniqueId[5] = 'a' + ((mac[5] >> 4) & 0x0F);
    uniqueId[6] = 0;


    gpio_config_t io_conf = {
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,
        .pin_bit_mask   = GPIO_SEL_0
    };

    gpio_config(&io_conf);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    return ESP_OK;
}

esp_err_t componentSettingsPostInit(void) {

    // gpio_reset_pin(GPIO_NUM_0);

    return ESP_OK;
}

esp_err_t componentSettingsInitName(pComponent_t pComponent) {

    cJSON * name = cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "name");

    if (!cJSON_IsString(name)) {
        ESP_LOGW(TAG, "Component 'name' missing or is not a string.");
        return ESP_FAIL;
    }
    
    pComponent->name = name->valuestring;

    return ESP_OK;
}

esp_err_t componentSettingsLoadFile(pComponent_t pComponent) {

    if (!pComponent->settingsFile) {
        ESP_LOGE(TAG, "Failed to find settings file.");
        return ESP_FAIL;
    }

    pComponent->settingsJSON = cJSON_Parse(pComponent->settingsFile);

    if (!cJSON_IsObject(pComponent->settingsJSON)) {
        ESP_LOGE(TAG, "Failed to load settings file.");
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool componentSettingsVariableIsNVS(cJSON * variable) {

    cJSON * nvs = cJSON_GetObjectItemCaseSensitive(variable, "nvs");

    if(!cJSON_IsBool(nvs)) {
        return true;
    }

    return (nvs->valueint > 0) ? true : false;
}

esp_err_t componentSettingsSetStatus(pComponent_t pComponent, char * alert, char * message, cJSON * extra) {

    return ESP_OK;
    
    cJSON * status = cJSON_CreateObject();

    cJSON_AddStringToObject(status, "alert", alert);
    cJSON_AddStringToObject(status, "message", message);

    if (cJSON_IsObject(extra)) {
        cJSON_AddItemReferenceToObject(status, "extra", extra);
    }

    if (cJSON_GetObjectItemCaseSensitive(pComponent->settingsJSON, "status")) {
        cJSON_ReplaceItemInObjectCaseSensitive(pComponent->settingsJSON, "status", status);
    }
    else{
        cJSON_AddItemToObject(pComponent->settingsJSON, "status", status);
    }

    return ESP_OK;
}