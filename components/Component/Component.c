#include "Component.h"
#include "ComponentHTTPServer.h"
#include "ComponentREST.h"

#define TAG "Component"

static pComponent_t components[MAX_COMPONENTS];

static int componentsLength = 0;

void componentAdd(pComponent_t pComponent) {

    if (componentsLength >= MAX_COMPONENTS) {
        ESP_LOGE(TAG, "No space left in components array");
        return;
    }

    components[componentsLength] = pComponent;
    componentsLength++;
}

int componentGetComponentsLength(void) {
    return componentsLength;
}

void componentStart(pComponent_t pComponent) {

    if (!pComponent->task) {
        ESP_LOGD(TAG, "Component %s has no task, skipping task start.", pComponent->name);
        return;
    }

    if (!pComponent->taskStackDepth) {
		pComponent->taskStackDepth = 2048;
	}

    if (!pComponent->taskPriority) {
		pComponent->taskPriority = 5;
	}

    xTaskCreate(
		pComponent->task,
		pComponent->name,
		pComponent->taskStackDepth,
		pComponent,
		pComponent->taskPriority,
		NULL
	);
}

void componentsInit(void){

    esp_err_t espError;

    ESP_ERROR_CHECK(componentHTTPServerInit());

    ESP_ERROR_CHECK(componentRESTInit());

    ESP_ERROR_CHECK(componentSettingsInit());

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        espError = componentSettingsLoadFile(pComponent);

        if (espError != ESP_OK) {
            ESP_LOGE(TAG, "Component missing setting JSON file");
            continue;
        }

        espError = componentSettingsInitName(pComponent);
    
        if (espError != ESP_OK) {
            ESP_LOGW(TAG, "Skipping init on componenet with no name");
            continue;
        }

        componentEventInit(pComponent);
    }

    ESP_ERROR_CHECK(componentSettingsPostInit());

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        ESP_ERROR_CHECK(componentQueueInit(pComponent));

        ESP_ERROR_CHECK(componentSettingsLoadValues(pComponent));

        if (pComponent->init) {
            pComponent->init(pComponent);
        }

    }
}

void componentsStart(void){

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        componentStart(pComponent);
    }

    ESP_ERROR_CHECK(componentHTTPServerStart());

    ESP_ERROR_CHECK(componentRESTStart());
}

pComponent_t componenetGetByIndex(int index) {

    pComponent_t pComponent = components[index];

    if (!pComponent) {
        return NULL;
    }

    return pComponent;
}

pComponent_t componenetGetByName(char * name) {

    for (int index = 0;index < componentsLength;index++) {

        pComponent_t pComponent = componenetGetByIndex(index);

        if (!pComponent->name) {
            continue;
        }

        if (strcmp(pComponent->name, name) != 0) {
            continue;
        }

        return pComponent;
    }

    return NULL;
}

// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}