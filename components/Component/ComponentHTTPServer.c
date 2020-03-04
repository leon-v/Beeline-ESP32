#include "Component.h"

static httpd_handle_t server = NULL;

static httpd_config_t config = HTTPD_DEFAULT_CONFIG();

esp_err_t componentHTTPServerInit(void) {

    config.max_uri_handlers = 64;
    config.uri_match_fn = httpd_uri_match_wildcard;

	
    return httpd_start(&server, &config);
}

esp_err_t componentHTTPServerStart(void) {

    return ESP_OK;
}

esp_err_t componentHTTPServerRegister(httpd_uri_t * uriHandeler) {

	return httpd_register_uri_handler(server, uriHandeler);
}

void componentHTTPServerURIDecode(char * input) {

    char * output = input;
    char hex[3] = "\0\0\0";

    while (input[0] != '\0') {

    	if (input[0] == '+') {
    		output[0] = ' ';
    		input+= 1;
    	}

    	else if (input[0] == '%') {

    		hex[0] = input[1];
    		hex[1] = input[2];
    		output[0] = strtol(hex, NULL, 16);
    		input+= 3;
    	}

    	else{
    		output[0] = input[0];
    		input+= 1;
    	}

    	output+= 1;
    }

    output[0] = '\0';
}

char * componentHTTPServerGetPostData(httpd_req_t * httpRequest) {

	char * buffer = malloc(httpRequest->content_len * sizeof(char) + 1);

	int bytesRecieved = httpd_req_recv(httpRequest, buffer, httpRequest->content_len);

    if (bytesRecieved <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (bytesRecieved == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(httpRequest);
			free(buffer);
			return NULL;
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
		free(buffer);
        return NULL;
    }

	return buffer;
}