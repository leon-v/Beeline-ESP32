#include <mqtt_client.h>
#include <sys/param.h>

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

    char topic[64] = {0};
	char value[64] = {0};

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

        	strncpy(topic, event->topic,  MIN(event->topic_len, sizeof(topic)));
        	strncpy(value, event->data,  MIN(event->data_len, sizeof(value)));

        	// ESP_LOGI(TAG, "topic %s", topic);

        	const char * delim = "/";
        	char * subTopic = strtok(topic, delim);

        	char * subTopics[3] = {NULL, NULL, NULL};

        	int index = 0;
        	while (subTopic != NULL){

        		if (index > 1) {
        			subTopics[0] = subTopics[1];
        		}
        		if (index > 0) {
        			subTopics[1] = subTopics[2];
        		}

        		subTopics[2] = subTopic;

        		index++;

        		subTopic = strtok(NULL, delim);
        	}

        	if (!(subTopics[0] && subTopics[1] && subTopics[2])) {
        		ESP_LOGE(component.name, "Unable to get last 3 sub topics device/sensor/valueype");
        		break;
        	}

        	message_t message;

        	strcpy(message.deviceName, subTopics[0]);
        	strcpy(message.sensorName, subTopics[1]);

        	if (strcmp(subTopics[2], "int") == 0) {
				message.valueType = MESSAGE_INT;
				message.intValue = atoi(value);
			}
			else if (strcmp(subTopics[2], "float") == 0) {
				message.valueType = MESSAGE_FLOAT;
				message.floatValue = atof(value);
			}
			else if (strcmp(subTopics[2], "double") == 0) {
				message.valueType = MESSAGE_DOUBLE;
				message.doubleValue = atof(value);
			}
			else if (strcmp(subTopics[2], "string") == 0) {
				message.valueType = MESSAGE_STRING;
				strcpy(message.stringValue, value);
			}
			else{
				ESP_LOGE(component.name, "Unknown type '%s'", subTopics[2]);
        		break;
			}

			componentSendMessage(&component, &message);

		break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(component.name, "MQTT_EVENT_ERROR");
		break;

    }
    return ESP_OK;
}

char * mqttConnectionTopicFromMessage(message_t * message){

	static char topic[member_size(message_t, deviceName) + member_size(message_t, sensorName) + 64] = {0};

	// ESP_LOG

	strcpy(topic, outTopic);
	strcat(topic, "/");
	strcat(topic, message->deviceName);
	strcat(topic, "/");
	strcat(topic, message->sensorName);

	switch (message->valueType){

		case MESSAGE_INT:
			strcat(topic, "/int");
		break;

		case MESSAGE_FLOAT:
			strcat(topic, "/float");
		break;

		case MESSAGE_DOUBLE:
			strcat(topic, "/double");
		break;

		case MESSAGE_STRING:
			strcat(topic, "/string");
		break;
	}

	return topic;
}
static char * mqttConnectionValueFromMessage(message_t * message){

	static char value[member_size(message_t, stringValue)] = {0};

	switch (message->valueType){

		case MESSAGE_INT:
			sprintf(value, "%d", message->intValue);
		break;

		case MESSAGE_FLOAT:
			sprintf(value, "%.4f", message->floatValue);
		break;

		case MESSAGE_DOUBLE:
			sprintf(value, "%.8f", message->doubleValue);
		break;

		case MESSAGE_STRING:
			sprintf(value, "%s", message->stringValue);
		break;
	}

	return value;
}

static esp_mqtt_client_handle_t client;
static void task(void * arg) {

	while (true) {

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

    	while (componentReadyWait(component.name) == ESP_OK) {

    		message_t message;
    		if (componentMessageRecieve(&component, &message) != ESP_OK) {
    			continue;
    		}

    		ESP_LOGW(component.name, "Got messsage from %s", message.sensorName);

    		char * topic = mqttConnectionTopicFromMessage(&message);
    		char * value = mqttConnectionValueFromMessage(&message);

    		int mqttMessageID;
			mqttMessageID = esp_mqtt_client_publish(client, topic, value, 0, 1, 0);
    	}

		esp_mqtt_client_stop(client);
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
