#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#include "http_server.h"

#define TAG "HTTP Server SSI Functions"

void httpSSIFunctionsGet(httpd_req_t *req, char * ssiTag){

	char buffer[1024];

	if (strcmp(ssiTag, "runTimeStats") == 0){
		vTaskGetRunTimeStats(buffer);
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, buffer));
	}
	else if (strcmp(ssiTag, "taskList") == 0) {
		vTaskList(buffer);
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, buffer));
	}
}