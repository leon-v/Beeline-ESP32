#include <mqtt_client.h>

#include "components.h"
#include "device.h"

static component_t component = {
	.name			= "MQTT Client",
	.messagesIn		= 1,
	.messagesOut	= 1
};

static const char config_html_start[] asm("_binary_mqtt_connection_config_html_start");
static const httpPage_t configPage = {
	.uri	= "/mqtt_connection_config.html",
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
	port =		componentsGetNVSu32(nvsHandle, "port", 1883);
	keepalive =	componentsGetNVSu32(nvsHandle, "keepalive", 30);
	username = 	componentsGetNVSString(nvsHandle, username, "username", "");
	password = 	componentsGetNVSString(nvsHandle, password, "password", "");
	inTopic = 	componentsGetNVSString(nvsHandle, inTopic, "inTopic", "/beeline/out/#");
	outTopic = 	componentsGetNVSString(nvsHandle, outTopic, "outTopic", "/beeline/in");
}

static esp_err_t mqttConnectionEventHandler(esp_mqtt_event_handle_t event){
	esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(component.name, "MQTT_EVENT_CONNECTED");
            componentSetReady(&component);

            msg_id = esp_mqtt_client_subscribe(client, inTopic, 0);
            ESP_LOGI(component.name, "sent subscribe successful, msg_id=%d", msg_id);
		break;

		case MQTT_EVENT_BEFORE_CONNECT:

		break;

        case MQTT_EVENT_DISCONNECTED:
        	componentSetNotReady(&component);
            ESP_LOGI(component.name, "MQTT_EVENT_DISCONNECTED");
		break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(component.name, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(component.name, "sent publish successful, msg_id=%d", msg_id);
		break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(component.name, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(component.name, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;

        case MQTT_EVENT_DATA:
        	ESP_LOGI(component.name, "MQTT_EVENT_DATA");
		break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(component.name, "MQTT_EVENT_ERROR");
		break;

    }
    return ESP_OK;
}

static esp_mqtt_client_handle_t client;
static void task(void * arg) {

	while (1) {

		if (componentReadyWait("WiFi") != ESP_OK) {
			continue;
		}

		esp_mqtt_client_config_t mqttConfig = {
			.host = host,
			.port = port,
			.client_id = deviceGetUniqueName(),
			.username = username,
			.password = password,
			.keepalive = keepalive,
			.event_handle = mqttConnectionEventHandler
		};

		ESP_LOGI(component.name, "Connecting...\n");

    	client = esp_mqtt_client_init(&mqttConfig);

    	esp_mqtt_client_start(client);

    	if (componentReadyWait(component.name) != ESP_OK) {
			continue;
		}

		ESP_LOGW(component.name, "Connected!!!!!!\n");

		vTaskDelay(10000 / portTICK_RATE_MS);

		esp_mqtt_client_stop(client);

		vTaskDelay(10000 / portTICK_RATE_MS);
	}

	vTaskDelete(NULL);
	return;
}


void mqttConnectionInit(void){

	component.configPage	= &configPage;
	component.task			= &task;
	component.loadNVS		= &loadNVS;
	component.saveNVS		= &saveNVS;

	componentsAdd(&component);
}
