#pragma once

#include "Modules.hpp"

#include <time.h>
#include <sys/time.h>
#include <lwip/apps/sntp.h>

extern const char ntpClientSettingsFile[] asm("_binary_NtpClient_json_start");

class NtpClient: public Modules::Module{
	public:
	string host;
	NtpClient(Modules *modules):Modules::Module(modules, string(ntpClientSettingsFile)){

		sntp_setoperatingmode(SNTP_OPMODE_POLL);

		sntp_init();

		this->host = this->settings.getString("host");
		sntp_setservername(0, this->host.c_str());
	}

	void task(){
		
		static time_t now = 0;
		static struct tm timeinfo;
		while(timeinfo.tm_year < (2019 - 1900)) {

			ESP_LOGW(this->tag.c_str(), "Waiting for time to get updated...");

			vTaskDelay(10000 / portTICK_PERIOD_MS);

			time(&now);

			localtime_r(&now, &timeinfo);
		}

		ESP_LOGI(this->tag.c_str(), "Time updated");

		char timestamp[64] = "";
		strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);

		ESP_LOGI(this->tag.c_str(), "%s", timestamp);
	};
};