#include "Module.hpp"

void Module::taskWrapper(void * arg){
	Module * module = (Module *) arg;
	module->task();
	vTaskDelete(NULL);
};

void Module::startTask(Module * module){
	xTaskCreate(
		&taskWrapper,
		module->name,
		module->taskStackDepth,
		module,
		module->taskPriority,
		NULL
	);

};