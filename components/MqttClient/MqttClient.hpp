
#pragma once

#include "Modules.hpp"

#include "mqtt_client.h"
#include "esp_event.h"

extern const char mqttClientSettingsFile[] asm("_binary_MqttClient_json_start");

#define ESP_MQTT_EVENT_ANY_ID (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID

class MqttClient : public Modules::Module {
	public:
	esp_mqtt_client_config_t config;
	esp_mqtt_client_handle_t client;
	string host;
	string clientId;
	string username;
	string password;
	bool connected = false;
	bool loaded = false;
	bool subscribed = false;
	// string lwtTopic;
	string pubTopic;
	int pubQos = 1;
	string subTopic;
	int subQos = 1;


	MqttClient(Modules *modules) : Modules::Module(modules, string(mqttClientSettingsFile)) {

		ESP_ERROR_CHECK(this->setIsSource());
		ESP_ERROR_CHECK(this->setIsSink());
	};

	void load(){

		this->host = this->settings.getString("host");
		this->config.host = this->host.c_str();

		this->config.port = this->settings.getInt("port");

		this->clientId = this->settings.getString("clientId");
		this->config.client_id = this->clientId.c_str();

		this->username = this->settings.getString("username");
		this->config.username = this->username.c_str();

		this->password = this->settings.getString("password");
		this->config.password = this->password.c_str();

		this->client = esp_mqtt_client_init(&this->config);

		ESP_ERROR_CHECK(esp_mqtt_client_register_event(this->client, ESP_MQTT_EVENT_ANY_ID, this->eventHandler, this));

		ESP_ERROR_CHECK(esp_mqtt_client_start(this->client));

		this->loaded = true;
	};

	void unLoad(){
		this->connected = false;
		this->subscribed = false;

		if (!this->client) {
			return;
		}
		
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_stop(this->client));
		ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_destroy(this->client));
	};

	void reLoad(){
		this->unLoad();
		this->load();
	};

	void task(){
		
		while (true) {

			cJSON *message = this->message.recieve();

			if (!message) {
				ESP_LOGI(this->name.c_str(), "No message recieved");
				continue;
			}

			if (!this->loaded){
				this->load();
			}

			if (!this->connected) {
				ESP_LOGI(this->name.c_str(), "Not connected, discarding message.");
				cJSON_Delete(message);
				continue;
			}

			if (!this->subscribed){
				ESP_LOGI(this->name.c_str(), "Not subscribed, discarding message.");
				cJSON_Delete(message);
				continue;
			}

			char *data = cJSON_Print(message);

			if (!data){
				LOGE("Failed to stringify JSON");
				cJSON_Delete(message);
				continue;
			}

			this->pubTopic = this->settings.getString("pubTopic");
			this->pubQos = this->settings.getInt("pubQos");

			esp_mqtt_client_publish(this->client, this->pubTopic.c_str(), data, strlen(data) + 1, this->pubQos, 0);

			free(data);

			cJSON_Delete(message);
		}
	}

	static void eventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event) {

		MqttClient *mqttClient = (MqttClient *) handler_args;

		ESP_LOGD(mqttClient->tag.c_str(), "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
		mqttClient->eventHandlerCallback( (esp_mqtt_event_handle_t) event);
	}

	esp_err_t  eventHandlerCallback(esp_mqtt_event_handle_t event) {

		this->subTopic = this->settings.getString("subTopic");
		this->subQos = this->settings.getInt("subQos");
		cJSON *message;

		int msg_id;
		// your_context_t *context = event->context;
		switch (event->event_id) {


			case MQTT_EVENT_CONNECTED:
				this->connected = true;
				LOGI("MQTT_EVENT_CONNECTED");

				msg_id = esp_mqtt_client_subscribe(client, this->subTopic.c_str(), this->subQos);
				// LOGI("sent subscribe successful, msg_id=%d", msg_id);
				break;


			case MQTT_EVENT_DISCONNECTED:
				this->connected = false;
				this->subscribed = false;
				LOGI("MQTT_EVENT_DISCONNECTED");
				break;


			case MQTT_EVENT_SUBSCRIBED:
				this->subscribed = true;
				LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
				break;


			case MQTT_EVENT_UNSUBSCRIBED:
				LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
				break;


			case MQTT_EVENT_PUBLISHED:
				// LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
				break;


			case MQTT_EVENT_DATA:
				// LOGI("MQTT_EVENT_DATA");

				// printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
				// printf("DATA=%.*s\r\n", event->data_len, event->data);

				message = cJSON_Parse(event->data);

				if (!message){
					LOGE("Failed to parse message");
					break;
				}

				if (cJSON_IsObject(message)) {
					cJSON_AddStringToObject(message, "mqttTopic", event->topic);
				}
				

				this->message.send(message);
				
				cJSON_Delete(message);

				break;


			case MQTT_EVENT_ERROR:
				LOGI("MQTT_EVENT_ERROR");
				break;
			
			case MQTT_EVENT_BEFORE_CONNECT:
				this->connected = false;
				this->subscribed = false;
				LOGI("MQTT_EVENT_BEFORE_CONNECT");
				LOGW("this->config.host: %s", this->config.host);
				break;

			default:
				LOGI("Other event id:%d", event->event_id);
				break;
		}
		return ESP_OK;
	}
};