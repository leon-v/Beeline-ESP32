#include "components.h"

static const char config_html_start[] asm("_binary_mqtt_client_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/mqtt_client_config.html",
	.page	= config_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static char * host;
static unsigned int port;
static unsigned int keepalive;
static char * username;
static char * password;
static char * inTopic;
static char * outTopic;

static void saveNVS(nvs_handle nvsHandle){
	componentsSetNVSString(nvsHandle, host, "host");
	componentsSetNVSu32(nvsHandle, "port", port);
	componentsSetNVSu32(nvsHandle, "keepalive", keepalive);
	componentsSetNVSString(nvsHandle, username, "username");
	componentsSetNVSString(nvsHandle, password, "password");
	componentsSetNVSString(nvsHandle, inTopic, "inTopic");
	componentsSetNVSString(nvsHandle, outTopic, "outTopic");
}

static void loadNVS(nvs_handle nvsHandle){
	host =		componentsGetNVSString(nvsHandle, host, "host", "mqtt.example.com");
	port =		componentsGetNVSu32(nvsHandle, "port", 8332);
	keepalive =	componentsGetNVSu32(nvsHandle, "keepalive", 30);
	username = 	componentsGetNVSString(nvsHandle, username, "username", "");
	password = 	componentsGetNVSString(nvsHandle, password, "password", "");
	inTopic = 	componentsGetNVSString(nvsHandle, inTopic, "inTopic", "/beeline/out/#");
	outTopic = 	componentsGetNVSString(nvsHandle, outTopic, "outTopic", "/beeline/in");
}

static void task(void * arg) {

	while (1) {

		if (componentReadyWait("WiFi") != ESP_OK) {
			continue;
		}

		ESP_LOGW("MQTT", "test");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

static component_t component = {
	.name =			"MQTT Client",
	.messagesIn =	1,
	.messagesOut =	1,
	.configPage =	&configPage,
	.task =			&task,
	.loadNVS =		&loadNVS,
	.saveNVS =		&saveNVS
};

void mqttClientInit(void){

	componentsAdd(&component);

	ESP_LOGI(component.name, "Init");
}
