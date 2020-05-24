#pragma once

#include <string>
#include <iostream>
#include <vector>       // std::vector

using namespace std;

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <cJSON.h>

#include "NVS.hpp"
#include "HttpServer.hpp"

#define ABORT(){\
	ESP_ERROR_CHECK(ESP_FAIL);\
}
#define LOGI(format, ...){\
	ESP_LOGI(this->tag.c_str(), format, ##__VA_ARGS__);\
}
#define LOGW(format, ...){\
	ESP_LOGW(this->tag.c_str(), format, ##__VA_ARGS__);\
}
#define LOGE(format, ...){\
	ESP_LOGE(this->tag.c_str(), format, ##__VA_ARGS__);\
}

#define jlogw(json){\
	char *jlogwjson = cJSON_Print(json);\
	ESP_LOGW(this->tag.c_str(), "%s:%d: %s", __func__, __LINE__ , jlogwjson);\
	free(jlogwjson);\
}
#define jlogwm(json){\
	char *jlogwjson = cJSON_Print(json);\
	ESP_LOGW(module->tag.c_str(), "%s:%d: %s", __func__, __LINE__ , jlogwjson);\
	free(jlogwjson);\
}

class Modules{
	public:
	string		tag = "Modules";
	HttpServer	httpServer;
	string		uniqueIdValue;
	cJSON		*sinkOptions;

	class Module{
		
		public:
		string			tag				= "/Module";
		Modules			*modules;
		cJSON			*json;
		string			name;
		int				taskStackDepth	= 4096;
		int				taskPriority	= 5;
		bool			isSource		= false;
		bool			isSink			= false;
		cJSON			*sinkOption;

		// Module / Init
		class Init{
			public:
			Init(Module *module, Modules *modules, string settingsFile) {

				module->modules = modules;

				module->json = cJSON_Parse(settingsFile.c_str());

				if (!cJSON_IsObject(module->json)) {
					ESP_LOGE(module->tag.c_str(), "JSON data failed to parse");
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				cJSON * nameJSON = cJSON_GetObjectItemCaseSensitive(module->json, "name");

				if (!cJSON_IsString(nameJSON)) {
					ESP_LOGE(module->tag.c_str(), "JSON/name must be a string");
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				module->name = string(nameJSON->valuestring);

				module->tag.append(string("("));
				module->tag.append(module->name);
				module->tag.append(string(")"));

				module->tag.insert(0, module->modules->tag);
			}
		};
		Init init;
		// Modules / Module / Settings

		class Settings{

			public:
			string			tag = "/Settings";
			NVS				nvs;
			Module			*module;
			cJSON			*settings;

			Settings(Module *module): nvs(module->name){

				this->module = module;

				this->tag.insert(0, this->module->tag);

				// LOGI("Setting NVS Name: %s", this->module->name.c_str());

				this->settings = cJSON_GetObjectItemCaseSensitive(this->module->json, "settings");

				if (!this->settings) {
					LOGW("No settings found, creating local array");
					this->settings = cJSON_CreateArray();
					cJSON_AddItemToObject(this->module->json, "settings", this->settings);
				}

				if (!cJSON_IsArray(this->settings)) {
					ESP_LOGE(this->tag.c_str(), "Settings required and must be an array");
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				ESP_ERROR_CHECK(this->loadValues());

				// ESP_LOGI(this->tag.c_str(), "Construct");
				// ESP_LOGI(this->tag.c_str(), "/Construct");
			}

			cJSON * cloneJSON(cJSON * json) {

				char * jsonString = cJSON_Print(json);

				if (!jsonString) {
					return NULL;
				}

				cJSON * clone = cJSON_Parse(jsonString);

				free(jsonString);

				return clone;
			}
			cJSON *getSetting(string name) {

				cJSON *setting;
				cJSON_ArrayForEach(setting, this->settings) {

					cJSON *settingName = cJSON_GetObjectItemCaseSensitive(setting, "name");

					if (!cJSON_IsString(settingName)) {
						continue;
					}

					if (name.compare(string(settingName->valuestring)) != 0){
						continue;
					}

					return setting;
				}

				LOGE("No setting with name '%s' found.", name.c_str());

				return NULL;
			}
			esp_err_t save(cJSON * newSettings) {
				
				LOGW("save");
				if (!cJSON_IsArray(newSettings)) {
					ESP_LOGE(this->tag.c_str(), "Setgins passed is not an array");
					return ESP_FAIL;
				}

				cJSON *newSetting;
				cJSON_ArrayForEach(newSetting, newSettings) {
					
					cJSON *newSettingName = cJSON_GetObjectItemCaseSensitive(newSetting, "name");

					if (!cJSON_IsString(newSettingName)) {
						ESP_LOGE(this->tag.c_str(), "JSON setting 'name' must be  string");
						continue;
					}
					
					cJSON *newSettingValue = cJSON_GetObjectItemCaseSensitive(newSetting, "value");
					
					if (!newSettingValue) {
						ESP_LOGE(this->tag.c_str(), "JSON setting 'value' for %s must be present", newSettingName->valuestring);
						continue;
					}

					ESP_ERROR_CHECK(this->setValue(newSettingName->valuestring, newSettingValue));
				}

				return ESP_OK;
			}
			esp_err_t setValue(string name, cJSON * value) {

				if (!value) {
					ESP_LOGE(this->tag.c_str(), "Value passed to setValue is invalid");
					return ESP_FAIL;
				}

				cJSON *newValue = this->cloneJSON(value);

				if (!newValue) {
					ESP_LOGE(this->tag.c_str(), "Failed to clone value in setValue");
					return ESP_FAIL;
				}

				// If new value is a string, perform replacements
				if (cJSON_IsString(newValue)) {
					
					string newValueString = string(newValue->valuestring);
					this->module->findAndReplaceAll(newValueString, "[UID]", this->module->modules->uniqueIdValue);

					cJSON_Delete(newValue);
					newValue = cJSON_CreateString(newValueString.c_str());
				}

				cJSON * setting = this->getSetting(name);

				if (!setting) {
					ESP_LOGE(this->tag.c_str(), "Failed to get setting %s", name.c_str());
					return ESP_FAIL;
				}

				cJSON * existingValue = cJSON_GetObjectItemCaseSensitive(setting, "value");

				if (existingValue) {

					if (newValue->type != existingValue->type) {
						ESP_LOGE(this->tag.c_str(), "Trying to save JSON value type 0x%02x over 0x%02x not allowed.", newValue->type, existingValue->type);
						return ESP_FAIL;
					}

					cJSON_ReplaceItemInObject(setting, "value", newValue);
				}
				else{
					cJSON_AddItemToObject(setting, "value", newValue);
				}

				this->nvs.setJSON(name, newValue);

				return ESP_OK;
			}
			esp_err_t loadValue(cJSON *setting){

				string settingName = this->getSettingName(setting);

				cJSON *value = this->nvs.getJSON(settingName);

				// LOGW("NVS Setting Name: %s", settingName.c_str());
				
				if (!value){

					ESP_LOGW(this->tag.c_str(), "Loading of '%s' failed, using default.", settingName.c_str());

					cJSON * defaultValue = cJSON_GetObjectItemCaseSensitive(setting, "default");

					if (!defaultValue) {
						ESP_LOGE(this->tag.c_str(), "'%s' requires a default value", settingName.c_str());
						return ESP_FAIL;
					}

					ESP_ERROR_CHECK(this->setValue(settingName, defaultValue));

					value = this->nvs.getJSON(settingName);
				}

				cJSON * existingValue = cJSON_GetObjectItemCaseSensitive(setting, "value");

				if (existingValue) {
					cJSON_ReplaceItemInObject(setting, "value", value);
				}
				else{
					cJSON_AddItemToObject(setting, "value", value);
				}

				return ESP_OK;
			}
			esp_err_t loadValues(){

				cJSON *setting;
				cJSON_ArrayForEach(setting, this->settings) {

					string name = this->getSettingName(setting);
					// ESP_LOGI(this->tag.c_str(), "Loading: %s", name.c_str());

					ESP_ERROR_CHECK(this->loadValue(setting));
				}
				return ESP_OK;
			}
			string getSettingName(cJSON * setting) {

				cJSON * name = cJSON_GetObjectItemCaseSensitive(setting, "name");

				if (!cJSON_IsString(name)) {
					ESP_LOGE(this->tag.c_str(), "Module name must be a string.");
					return NULL;
				}

				return string(name->valuestring);
			}
			cJSON *getSettingValue(cJSON * setting) {

				cJSON * value = cJSON_GetObjectItemCaseSensitive(setting, "value");

				if (!value) {
					ESP_LOGE(this->tag.c_str(), "Setting has no value.");
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				return value;
			}
			cJSON *getValue(string name) {

				cJSON *setting = this->getSetting(name);

				return this->getSettingValue(setting);
			}
			int getInt(string name){

				cJSON *value = this->getValue(name);

				if (!cJSON_IsNumber(value)) {
					ESP_LOGE(this->tag.c_str(), "Failed to get '%s'. Is not number.", name.c_str());
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				return value->valueint;
			}
			double getDouble(string name){

				cJSON *value = this->getValue(name);

				if (!cJSON_IsNumber(value)) {
					ESP_LOGE(this->tag.c_str(), "Failed to get '%s'. Is not number.", name.c_str());
					ESP_ERROR_CHECK(ESP_FAIL);
				}

				return value->valuedouble;
			}
			string getString(string name){

					cJSON *value = this->getValue(name);

					if (!cJSON_IsString(value)) {
						ESP_LOGE(this->tag.c_str(), "Failed to get '%s'. Is not string.", name.c_str());
						ESP_ERROR_CHECK(ESP_FAIL);
					}

					return string(value->valuestring);
				}
		};
		Settings settings;

		class Data{
			public:
			string	tag = "/Data";
			Module	*module;
			cJSON	*json;

			Data(Module *module){

				this->module = module;

				this->tag.insert(0, this->module->tag);

				this->json = cJSON_GetObjectItemCaseSensitive(this->module->json, "data");
				
				if (!this->json) {
					this->json = cJSON_CreateObject();
					cJSON_AddItemToObject(this->module->json, "data", this->json);
				}
			}

			cJSON * add(string name, string label, string renderClass, cJSON * value) {
				
				cJSON *item = cJSON_CreateObject();

				cJSON_AddStringToObject(item, "name", name.c_str());
				cJSON_AddStringToObject(item, "label", label.c_str());
				cJSON_AddStringToObject(item, "class", renderClass.c_str());
				cJSON_AddItemToObject(item, "value", value);

				cJSON_AddItemToObject(this->json, name.c_str(), item);

				return item;
			}

			void updateValue(string name, cJSON * value) {

				cJSON *item = cJSON_GetObjectItemCaseSensitive(this->json, name.c_str());

				if (!cJSON_IsObject(item)) {
					LOGE("Item '%s' not found", name.c_str());
					return;
				}

				cJSON_DeleteItemFromObject(item, "value");
				cJSON_AddItemToObject(item, "value", value);
				// cJSON_ReplaceItemInObjectCaseSensitive(item, "value", value);
			}
		};
		Data data;

		// Modules / Module / Queue
		class Queue{
			public:
			string			tag				= "/Queue";
			unsigned int	recieveTimeoutMs= 60000;
			Module			*module;
			xQueueHandle	queue;
			size_t			maxMessageSize	= 256;

			Queue(Module *module){

				this->module = module;

				this->tag.insert(0, this->module->tag);

				// ESP_LOGI(this->tag.c_str(), "Construct");
				// ESP_LOGI(this->tag.c_str(), "/Construct");
			}

			esp_err_t create(){
				this->queue = xQueueCreate(3, this->maxMessageSize * sizeof(char) );

				if (!this->queue) {
					ESP_LOGE(this->tag.c_str(), "Failed to create queue");
					return ESP_FAIL;
				}

				return ESP_OK;
			}
			esp_err_t add(cJSON *message){

				if (!this->queue) {
					ESP_LOGE(this->tag.c_str(), "Queue not initalised before adding");
					return ESP_FAIL;
				}

				if (!uxQueueSpacesAvailable(this->queue)) {
					int freeHeap = xPortGetFreeHeapSize();
					ESP_LOGE(this->tag.c_str(), "No room in queue. Free heap: %d", freeHeap);
					return ESP_OK;
				}
				

				char *json = cJSON_Print(message);

				if (!json){
					ESP_LOGE(this->tag.c_str(), "Failed to create json");
					return ESP_FAIL;
				}

				char * buffer = (char *) calloc(this->maxMessageSize, sizeof(char));

				strncpy(buffer, json, this->maxMessageSize);

				free(json);

				xQueueSend(this->queue, buffer, 0);

				free(buffer);

				return ESP_OK;
			}
			cJSON *recieve(){

				char *queueItem = (char *) malloc(this->maxMessageSize * sizeof(char));

				if (!xQueueReceive(this->queue, queueItem, this->recieveTimeoutMs / portTICK_RATE_MS)) {
					ESP_LOGW(this->tag.c_str(), "Timed out waiting for message.");
					return NULL;
				}

				cJSON *message = cJSON_Parse(queueItem);

				free(queueItem);

				return message;
			}
			

		};
		Queue queue;

		class Message{
			public:
			Module	*module;
			string	tag = "/Message";

			Message(Module *module){

				this->module = module;

				this->tag.insert(0, this->module->tag);
			}

			esp_err_t send(cJSON *message){
				return this->module->modules->sendMessage(this->module, message);
			}

			cJSON *recieve(){
				return this->module->queue.recieve();
			}

			esp_err_t sendValue(cJSON *value){

				cJSON* message = cJSON_CreateObject();
			
				string deviceName = this->module->modules->get("Device")->settings.getString("name");

				cJSON_AddStringToObject(message, "device", deviceName.c_str());

				cJSON_AddStringToObject(message, "module", this->module->name.c_str());

				cJSON_AddItemReferenceToObject(message, "value", value);

				ESP_ERROR_CHECK_WITHOUT_ABORT(this->send(message));

				cJSON_Delete(message);

				return ESP_OK;
			}
		};
		Message message;

		Module(Modules *modules, string settingsFile):
			init(this, modules, settingsFile),
			settings(this),
			data(this),
			queue(this),
			message(this) {

			ESP_ERROR_CHECK(this->modules->add(this));
		}

		// Task 
		static void taskWrapper(void * arg){
			Module * module = (Module *) arg;
			module->task();
			vTaskDelete(NULL);
		};
		static void startTask(Module * module){

			// ESP_LOGI(module->tag.c_str(), "startTask");

			xTaskCreate(
				&taskWrapper,
				module->name.c_str(),
				module->taskStackDepth,
				module,
				module->taskPriority,
				NULL
			);

			// ESP_LOGI(module->tag.c_str(), "/startTask");
		};
		virtual void task(){
			ESP_LOGW(this->tag.c_str(), "Module has no task");
		};

		// Sink / Source
		esp_err_t setIsSink(){

			this->isSink = true;

			ESP_ERROR_CHECK(this->queue.create());

			this->modules->updateModulesSinkOptions();

			return ESP_OK;
		}
		esp_err_t setIsSource(){

			this->isSource = true;

			this->addRoutingSetting();

			return ESP_OK;
		}
		esp_err_t addRoutingSetting(){
			
			cJSON *routingSetting = cJSON_CreateObject();
			cJSON_AddStringToObject(routingSetting, "name", "routing");
			cJSON_AddStringToObject(routingSetting, "label", "Routing");
			cJSON_AddStringToObject(routingSetting, "inputType", "checkbox");
			cJSON_AddItemToObject(routingSetting, "default", cJSON_CreateArray());
			cJSON_AddItemToObject(routingSetting, "options", cJSON_CreateArray());

			cJSON_AddItemToArray(this->settings.settings, routingSetting);

			ESP_ERROR_CHECK(this->settings.loadValue(routingSetting));

			this->updateSinkOptions();

			return ESP_OK;
		}
		void updateSinkOptions() {

			cJSON * routing = this->settings.getSetting("routing");

			if (!cJSON_IsObject(routing)) {
				LOGE("Routing not found in module");
				return;
			}

			cJSON * options = this->modules->getSinkOptions();

			cJSON_ReplaceItemInObjectCaseSensitive(routing, "options", options);
		}

		bool canRouteTo(Module * module){

			if (!module->isSink) {
				return false;
			}
			// Is module->name in this routes

			cJSON *routes = this->settings.getValue("routing");
			
			if (!cJSON_IsArray(routes)) {
				return false;
			}

			cJSON *route;
			cJSON_ArrayForEach(route, routes) {

				if (!cJSON_IsString(route)) {
					continue;
				}

				if (string(route->valuestring).compare(module->name) == 0) {
					return true;
				}
			}

			return false;
		}
		// Settings
		virtual void reLoad(){
				ESP_LOGW(this->tag.c_str(), "Module has no reLoad");
			}

		// URI Handling
		virtual void restGet(HttpUri * httpUri, string path){

			if (!path.length()) {
				httpUri->sendJSON(this->json);
				return;
			}

			cJSON *value = cJSON_GetObjectItemCaseSensitive(this->json, path.c_str());

			path = httpUri->getUriComponent(4);
			if (path.length()){
				value = cJSON_GetObjectItemCaseSensitive(value, path.c_str());
			}

			path = httpUri->getUriComponent(5);
			if (path.length()){
				value = cJSON_GetObjectItemCaseSensitive(value, path.c_str());
			}

			if (value) {
				httpUri->sendJSON(value);
				return;
			}
		}
		virtual void restPost(HttpUri * httpUri, string path){

			if (!path.length()) {
				string postData = httpUri->getPostData();

				if (!postData.length()) {
					httpUri->sendJSONError("Failed to get post data", HTTPD_400_BAD_REQUEST);
					return;
				}

				cJSON * newSettings = cJSON_Parse(postData.c_str());

				if (!newSettings) {
					httpUri->sendJSONError("Failed to parse JSON post data", HTTPD_400_BAD_REQUEST);
					return;
				}

				this->settings.save(newSettings);

				cJSON_Delete(newSettings);

				this->restGet(httpUri, path);

				vTaskDelay(1000 / portTICK_PERIOD_MS);

				this->reLoad();
			}	
		}

		// Helper functions
		static string uriEncode(const string &uri) {
			ostringstream escaped;
			escaped.fill('0');
			escaped << hex;

			for (string::const_iterator i = uri.begin(), n = uri.end(); i != n; ++i) {
				string::value_type c = (*i);

				// Keep alphanumeric and other accepted characters intact
				if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
					escaped << c;
					continue;
				}

				// Any other characters are percent-encoded
				escaped << uppercase;
				escaped << '%' << setw(2) << int((unsigned char) c);
				escaped << nouppercase;
			}

			return escaped.str();
		}
		static string uriDecode(string &uri) {
			string ret;
			char ch;
			int i, ii;
			for (i=0; i<uri.length(); i++) {
				if (int(uri[i])==37) {
					sscanf(uri.substr(i+1,2).c_str(), "%x", &ii);
					ch=static_cast<char>(ii);
					ret+=ch;
					i=i+2;
				} else {
					ret+=uri[i];
				}
			}
			return (ret);
		}
		static void findAndReplaceAll(string &data, string toSearch, string replaceStr) {

			// Get the first occurrence
			size_t pos = data.find(toSearch);
		
			// Repeat till end is reached
			while( pos != std::string::npos) {
				// Replace this occurrence of Sub String
				data.replace(pos, toSearch.size(), replaceStr);
				// Get the next occurrence from the current position
				pos =data.find(toSearch, pos + replaceStr.size());
			}
		}
	
	};
	vector <Module *> modules;

	Modules(){
		// ESP_LOGI(this->tag.c_str(), "Construct");

		uint8_t mac[6];
		esp_read_mac(mac, ESP_MAC_WIFI_STA);
		
		char uniqueIdValue[8];
		uniqueIdValue[0] = 'a' + ((mac[3] >> 0) & 0x0F);
		uniqueIdValue[1] = 'a' + ((mac[3] >> 4) & 0x0F);
		uniqueIdValue[2] = 'a' + ((mac[4] >> 0) & 0x0F);
		uniqueIdValue[3] = 'a' + ((mac[4] >> 4) & 0x0F);
		uniqueIdValue[4] = 'a' + ((mac[5] >> 0) & 0x0F);
		uniqueIdValue[5] = 'a' + ((mac[5] >> 4) & 0x0F);
		uniqueIdValue[6] = 0;
		this->uniqueIdValue = string(uniqueIdValue);

		this->sinkOptions = cJSON_CreateArray();

		static HttpUri restVersion("/rest/version", HTTP_GET, &this->restVersionUri, this);
		this->httpServer.registerHttpUri(&restVersion);

		static HttpUri restModulesGetIndex("/rest/modules", HTTP_GET, &this->restModulesGetIndexUri, this);
		this->httpServer.registerHttpUri(&restModulesGetIndex);

		static HttpUri restModulesGet("/rest/modules/*", HTTP_GET, &this->restModulesGetUri, this);
		this->httpServer.registerHttpUri(&restModulesGet);

		static HttpUri restModulesPost("/rest/modules/*", HTTP_POST, &this->restModulesPostUri, this);
		this->httpServer.registerHttpUri(&restModulesPost);

		// ESP_LOGI(this->tag.c_str(), "/Construct");
	}

	esp_err_t add(Module *module){

		this->modules.push_back(module);

		return ESP_OK;
	}

	cJSON *getSinkOption(Module *module){
		cJSON *sinkOption = cJSON_CreateObject();
		cJSON_AddStringToObject(sinkOption, "name", module->name.c_str());
		cJSON_AddStringToObject(sinkOption, "value", module->name.c_str());
		return sinkOption;
	}
	cJSON *getSinkOptions(){

		cJSON *sinkOptions = cJSON_CreateArray();

		for (Module *module: this->modules){

			if (!module->isSink){
				continue;
			}

			cJSON *sinkOption = this->getSinkOption(module);

			cJSON_AddItemToArray(sinkOptions, sinkOption);
		}

		return sinkOptions;
	}
	void updateModulesSinkOptions(){

		for (Module *sourceModule: this->modules){

			if (!sourceModule->isSource){
				continue;
			}

			// LOGW("Setting sink options to '%s'", sourceModule->name.c_str());

			sourceModule->updateSinkOptions();
		}
	}
	Module * get(string name) {

		for (Module *module: this->modules){

			if (module->name.compare(name) != 0) {
				continue;
			}

			return module;
		}

		return NULL;
	}
	esp_err_t start(){

		for (Module *module: this->modules){
			module->startTask(module);
		}

		return ESP_OK;
	}

	// HTTP URI handelers
	static void restVersionUri(HttpUri * httpUri) {

		httpUri->setAccessControlAllowOrigin("*");

		cJSON * version = cJSON_CreateObject();

		cJSON_AddStringToObject(version, "firmwareName", "Beeline ESP32");

		Modules * modules = (Modules *) httpUri->context;

		string name = modules->get("Device")->settings.getString("name");

		cJSON_AddStringToObject(version, "deviceName", name.c_str());

		httpUri->sendJSON(version);

		cJSON_Delete(version);
	}
	static void restModulesGetIndexUri(HttpUri * httpUri) {

		httpUri->setAccessControlAllowOrigin("*");// Todo: Auth before allow single IP

		cJSON * moduleNames = cJSON_CreateArray();

		Modules * modules = (Modules *) httpUri->context;

		for (Module *module: modules->modules){
			cJSON_AddItemToArray(moduleNames, cJSON_CreateString(module->name.c_str()));
		}

		httpUri->sendJSON(moduleNames);

		cJSON_Delete(moduleNames);
	}
	static void restModulesGetUri(HttpUri * httpUri) {

		httpUri->setAccessControlAllowOrigin("*");// Todo: Auth before allow single IP

		string moduleName = httpUri->getUriComponent(2);

		if (!moduleName.length()) {
			httpUri->sendJSONError("No module name in URL", HTTPD_400_BAD_REQUEST);
			return;
		}

		Modules * modules = (Modules *) httpUri->context;
		
		Module * module = modules->get(moduleName);

		if (!module) {
			httpUri->sendJSONError("Module name not found", HTTPD_400_BAD_REQUEST);
			return;
		}

		string path = httpUri->getUriComponent(3);

		module->restGet(httpUri, path);

		if (!httpUri->isHandeled()){
			httpUri->sendJSONError("Module URI not found", HTTPD_404_NOT_FOUND);
		}
	}
	static void restModulesPostUri(HttpUri * httpUri) {

		httpUri->setAccessControlAllowOrigin("*");// Todo: Auth before allow single IP

		string moduleName = httpUri->getUriComponent(2);

		if (!moduleName.length()) {
			httpUri->sendJSONError("No module name in URL", HTTPD_400_BAD_REQUEST);
			return;
		}

		Modules * modules = (Modules *) httpUri->context;
		
		Module * module = modules->get(moduleName);

		if (!module) {
			httpUri->sendJSONError("Module name not found", HTTPD_400_BAD_REQUEST);
			return;
		}

		string path = httpUri->getUriComponent(3);

		module->restPost(httpUri, path);
	}
	esp_err_t sendMessage(Module *fromModule, cJSON *message) {

		for (Module *toModule: this->modules){

			if (!fromModule->canRouteTo(toModule)) {
				continue;
			}

			ESP_ERROR_CHECK_WITHOUT_ABORT(toModule->queue.add(message));
		}
		
		return ESP_OK;
	}
	
};