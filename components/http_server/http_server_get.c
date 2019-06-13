#include "http_server.h"

#define TAG "http SSI Get"


void httpSSIGetGet(httpd_req_t *req, char * ssiTag){

	// ESP_LOGW(TAG, "ssiTag %s", ssiTag);

	// ESP_LOGW(TAG, "req->uri %s", req->uri);

	char * params = strstr(req->uri, "?");

	if (!params) {
		ESP_LOGE(TAG, "No URI parameters found in %s", req->uri);
		return;
	}
	params++;

	char * mParams;
	mParams = malloc(strlen(params) + 1);
	strcpy(mParams, params);


	// ESP_LOGW(TAG, "mParams %s", mParams);

	static tokens_t get;
	httpServerParseValues(&get, mParams, "&", "=", "\0");

	if (!get.length) {
		// ESP_LOGE(TAG, "No parameters found after '?' in %s", req->uri);
		free(mParams);
		return;
	}



	char * getKey;
	getKey = strtok(ssiTag, ":");

	if (!getKey) {
		ESP_LOGE(TAG, "No key found in %s", ssiTag);
		free(mParams);
		return;
	}

	char * defaultValue;
	defaultValue = strtok(NULL, ":");

	if (!defaultValue) {
		ESP_LOGE(TAG, "No default value found in %s", ssiTag);
		free(mParams);
		return;
	}

	char * getValue = httpServerGetTokenValue(&get, getKey);

	if (!getValue) {
		getValue = defaultValue;
	}

	char * extraParams;
	extraParams = strtok(NULL, "");

	if (!extraParams) {

		if (strlen(getValue)) {
			ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, getValue));
		}

	}
	else{

		char * param1;
		param1 = strtok(extraParams, ":");

		if (!param1) {
			ESP_LOGE(TAG, "No param1 found in %s", extraParams);
			free(mParams);
			return;
		}

		if (strcmp(param1, "selected") == 0) {

			char * match;
			match = strtok(NULL, "");

			if (match) {
				if (strcmp(match, getValue) == 0) {
					ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, "selected"));
				}
			}
		}

		else{
			ESP_LOGE(TAG, "command %s not handled ", param1);
		}


	}

	free(mParams);
	return;
}
/*
 (90947) http SSI Get: req->uri /adc_config.html?adc=1
W (90947) http SSI Get: ssiTag adc:selected:0
W (90957) http SSI Get: req->uri /adc_config.html?adc=1
W (90957) http SSI Get: ssiTag adc:selected:1
W (90967) http SSI Get: req->uri /adc_config.html?adc=1
W (90967) http SSI Get: ssiTag adc:selected:2
W (90977) http SSI Get: req->uri /adc_config.html?adc=1
W (90977) http SSI Get: ssiTag adc:selected:3
W (90987) http SSI Get: req->uri /adc_config.html?adc=1
W (90987) http SSI Get: ssiTag adc:selected:4
W (90997) http SSI Get: req->uri /adc_config.html?adc=1
W (90997) http SSI Get: ssiTag adc:selected:5
W (91007) http SSI Get: req->uri /adc_config.html?adc=1
W (91007) http SSI Get: ssiTag adc:selected:6
W (91017) http SSI Get: req->uri /adc_config.html?adc=1
W (91027) http SSI Get: ssiTag adc:selected:7
W (91027) http SSI Get: req->uri /adc_config.html?adc=1
W (91037) Wake Timer: Trigger
W (91037) http SSI Get: ssiTag adc:selected:8
*/