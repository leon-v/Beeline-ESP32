#pragma once

#include "Module.hpp"

#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_log.h"

#define maxModules 10

class Modules;

static Modules *_modules;

class Modules{
	
	private:
		const char *tag = "Modules";
		Module * modules[maxModules];
		unsigned int length = 0;
		HTTPServer *httpServer;
	public:

		Modules(HTTPServer *httpServer) {

			_modules = this;

			this->httpServer = httpServer;

			static HTTPURI restVersion("/rest/version", HTTP_GET, &this->restVersionUri);
			this->httpServer->registerHttpUri(&restVersion);

			static HTTPURI restModulesGetIndex("/rest/modules", HTTP_GET, &this->restModulesGetIndexUri);
			this->httpServer->registerHttpUri(&restModulesGetIndex);

			static HTTPURI restModulesGet("/rest/modules/*", HTTP_GET, &this->restModulesGetUri);
			this->httpServer->registerHttpUri(&restModulesGet);

			static HTTPURI restModulesPost("/rest/modules/*", HTTP_POST, &this->restModulesPostUri);
			this->httpServer->registerHttpUri(&restModulesPost);
		}

		static void restVersionUri(HTTPURI * httpUri) {

			httpUri->setHeader("Access-Control-Allow-Origin", "*");

			cJSON * version = cJSON_CreateObject();

			cJSON_AddStringToObject(version, "firmwareName", "Beeline ESP32");

			cJSON_AddStringToObject(version, "deviceName", "NO NAME");

			httpUri->sendJSON(version);

			cJSON_Delete(version);
		}
		

		static void restModulesGetIndexUri(HTTPURI * httpUri) {

			httpUri->setHeader("Access-Control-Allow-Origin", "*");

			cJSON * moduleNames = cJSON_CreateArray();

			for (unsigned int index = 0; index < _modules->length; index++) {

				Module * module = _modules->modules[index];

				if (!module->name) {
					continue;
				}

				cJSON_AddItemToArray(moduleNames, cJSON_CreateString(module->name));
			}

			httpUri->sendJSON(moduleNames);

			cJSON_Delete(moduleNames);
		}

		static void restModulesGetUri(HTTPURI * httpUri) {

			httpUri->setHeader("Access-Control-Allow-Origin", "*");

			char * moduleName = httpUri->getUriComponent(2);

			if (!moduleName) {
				httpUri->sendJSONError("No module name in URL", HTTPD_400_BAD_REQUEST);
				return;
			}
			
			Module * module = _modules->getModuleByName(moduleName);
			
			if (!module) {
				httpUri->sendJSONError("Module name not found", HTTPD_400_BAD_REQUEST);
				free(moduleName);
				return;
			}

			free(moduleName);

			httpUri->sendJSON(module->settings->json);
		}

		static void restModulesPostUri(HTTPURI * httpUri) {
			
			char * moduleName = httpUri->getUriComponent(2);

			if (!moduleName) {
				httpUri->sendJSONError("No module name in URL", HTTPD_400_BAD_REQUEST);
				return;
			}

			Module *module = _modules->getModuleByName(moduleName);

			if (!module) {
				httpUri->sendJSONError("Module name not found", HTTPD_400_BAD_REQUEST);
				return;
			}

			char * postData = httpUri->getPostData();

			if (!postData) {
				httpUri->sendJSONError("Failed to get post data", HTTPD_400_BAD_REQUEST);
				free(postData);
				return;
			}

			cJSON * newSettings = cJSON_Parse(postData);

			free(postData);

			if (!newSettings) {
				httpUri->sendJSONError("Failed to parse JSON post data", HTTPD_400_BAD_REQUEST);
				return;
			}

			module->settings->save(newSettings);

			cJSON_Delete(newSettings);

			_modules->restModulesGetUri(httpUri);

			vTaskDelay(1000 / portTICK_PERIOD_MS);

			module->reLoad();
		}
		



		esp_err_t add(Module *module) {

			if (this->length > maxModules) {
				ESP_LOGE(this->tag, "Module limit reached, module can not be added.");
				return ESP_FAIL;
			}

			this->modules[this->length] = module;

			this->length++;

			return ESP_OK;
		}

		esp_err_t start() {

			for (unsigned int index = 0; index < this->length; index ++) {

				Module * module = this->modules[index];

				module->startTask(module);
			}

			return ESP_OK;
		}

		Module * module(char * name) {
			return this->getModuleByName(name);
		}

		Module * getModuleByName(char * name) {

			for (unsigned int index = 0; index < this->length; index ++) {

				Module * module = this->modules[index];

				if (!module->name) {
					continue;
				}

				if (strcmp(name, module->name) != 0) {
					continue;
				}

				return module;
			}

			return NULL;
		}
};