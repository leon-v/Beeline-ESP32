#pragma once

#include "Modules.hpp"

#include <time.h>

extern const char deviceSettingsFile[] asm("_binary_Device_json_start");

class Device: public Modules::Module{
	public:
	cJSON * runTimeStats;
	cJSON * freeHeap;
	Device(Modules *modules):Modules::Module(modules, string(deviceSettingsFile)){

		this->runTimeStats = this->data.add("time", "Time", "Preformatted", cJSON_CreateString(""));
		cJSON_AddNumberToObject(this->runTimeStats, "updateInterval", 1000);
		this->updateTime();


		this->runTimeStats = this->data.add("runTimeStats", "Run Time Stats", "Preformatted", cJSON_CreateString(""));
		cJSON_AddNumberToObject(this->runTimeStats, "updateInterval", 1000);
		this->updateRunTimeStats();

		this->runTimeStats = this->data.add("taskList", "Task List", "Preformatted", cJSON_CreateString(""));
		cJSON_AddNumberToObject(this->runTimeStats, "updateInterval", 1000);
		this->updateTaskList();

		
		this->freeHeap = this->data.add("freeHeap", "Free Heap", "GraphLog", cJSON_CreateNumber(0));
		cJSON_AddNumberToObject(this->freeHeap, "updateInterval", 1000);
		cJSON_AddNumberToObject(this->freeHeap, "max", 250000);
		this->updateFreeHeap();
	}

	void updateTime(){
		
		time_t now = 0;
		struct tm timeinfo;

		time(&now);

		localtime_r(&now, &timeinfo);

		char timestamp[64] = "";
		strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);

		this->data.updateValue("time", cJSON_CreateString(timestamp));
	}

	void updateFreeHeap(){

		int freeHeap = xPortGetFreeHeapSize();

		this->data.updateValue("freeHeap", cJSON_CreateNumber(freeHeap));
	}

	void updateTaskList(){

		char * buffer = (char *) malloc(1024 * sizeof(char));

		if (!buffer){
			LOGE("Failed to allocate buffer for task list");
			return;
		}

		vTaskList(buffer);

		this->data.updateValue("taskList", cJSON_CreateString(buffer));

		free(buffer);
	}

	void updateRunTimeStats(){

		char * buffer = (char *) malloc(1024 * sizeof(char));

		if (!buffer){
			LOGE("Failed to allocate buffer for runtime stats");
			return;
		}

		vTaskGetRunTimeStats(buffer);

		this->data.updateValue("runTimeStats", cJSON_CreateString(buffer));

		free(buffer);
	}

	void restGet(HttpUri * httpUri, string path){

		string itemPath = httpUri->getUriComponent(4);

		if (!itemPath.length()){
			this->updateRunTimeStats();
			this->updateFreeHeap();
		}

		else if (itemPath.compare("time") == 0){
			this->updateTime();
		}

		else if (itemPath.compare("runTimeStats") == 0){
			this->updateRunTimeStats();
		}

		else if (itemPath.compare("taskList") == 0){
			this->updateTaskList();
		}

		else if (itemPath.compare("freeHeap") == 0){
			this->updateFreeHeap();
		}
		
		Module::restGet(httpUri, path);
	}
};