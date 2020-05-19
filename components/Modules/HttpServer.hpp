#pragma once

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

#include "esp_http_server.h"
#include "cJSON.h"
#include "esp_log.h"

class HttpUri;

typedef void (HttpUriHandler)(HttpUri *httpUri);

class HttpUri{
	public:
	string			tag = "HttpUri";
	string			uri;
	string			type;
	string			accessControlAllowOrigin;
	httpd_method_t	method;
	HttpUriHandler	*httpUriHandler;
	void 			*context;
	httpd_req_t		*httpRequest;
	bool			handeled = false;

	HttpUri(string uri, httpd_method_t method, HttpUriHandler *httpUriHandler, void *context){
		this->uri = uri;
		this->method = method;
		this->httpUriHandler = httpUriHandler;
		this->context = context;
	}
	
	HttpUri(string uri, httpd_method_t method, HttpUriHandler *httpUriHandler){
		this->uri = uri;
		this->method = method;
		this->httpUriHandler = httpUriHandler;
	}

	static esp_err_t httpHandler(httpd_req_t * httpRequest) {

		HttpUri *httpUri = (HttpUri *) httpRequest->user_ctx;

		httpUri->httpRequest = httpRequest;

		httpUri->handeled = false;

		httpUri->httpUriHandler(httpUri);

		if (!httpUri->handeled) {
			ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_send_404(httpUri->httpRequest));
		}

		return ESP_OK;
	}

	bool isHandeled(){
		return this->handeled;
	}
	void sendChunk(string response) {

		if (!response.length()) {
			ESP_LOGE(this->tag.c_str(), "String passed empty in sendChunk");
			ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
			return;
		}

		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(this->httpRequest, response.c_str()));
	}

	void endChunks(){
		this->handeled = true;
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(this->httpRequest, NULL));
	}

	void sendJSON(const cJSON * json) {

		if (!json) {
			this->sendJSONError("JSON object passed is null", HTTPD_500_INTERNAL_SERVER_ERROR);
			return;
		}

		char * jsonStringChar = cJSON_Print(json);

		if (!jsonStringChar) {
			ESP_LOGE(this->tag.c_str(), "cJSON_Print failed in sendJSON");
			ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
			return;
		}

		this->setType("application/json");

		this->handeled = true;
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr(this->httpRequest, jsonStringChar));

		free(jsonStringChar);

		// this->sendChunk(jsonString);
		// this->endChunks();
	}

	void sendJSONError(string userMessage, httpd_err_code_t errorCode) {

		this->setResponseCode(errorCode);

		cJSON * response = cJSON_CreateObject();

		cJSON * success = cJSON_CreateBool(false);
		cJSON_AddItemToObject(response, "success", success);

		cJSON * message = cJSON_CreateString(userMessage.c_str());
		cJSON_AddItemToObject(response, "message", message);

		this->sendJSON(response);
		
		cJSON_Delete(response);
	}

	void setType(string type) {
		this->type = type;
		httpd_resp_set_type(this->httpRequest, this->type.c_str());
	}

	void setAccessControlAllowOrigin(string value) {
		this->accessControlAllowOrigin = value;
		httpd_resp_set_hdr(this->httpRequest, "Access-Control-Allow-Origin", this->accessControlAllowOrigin.c_str());
	}
	void setResponseCode(httpd_err_code_t error){
		string status;
		switch (error) {
			case HTTPD_501_METHOD_NOT_IMPLEMENTED:
				status = "501 Method Not Implemented";
				break;
			case HTTPD_505_VERSION_NOT_SUPPORTED:
				status = "505 Version Not Supported";
				break;
			case HTTPD_400_BAD_REQUEST:
				status = "400 Bad Request";
				break;
			case HTTPD_404_NOT_FOUND:
				status = "404 Not Found";
				break;
			case HTTPD_405_METHOD_NOT_ALLOWED:
				status = "405 Method Not Allowed";
				break;
			case HTTPD_408_REQ_TIMEOUT:
				status = "408 Request Timeout";
				break;
			case HTTPD_414_URI_TOO_LONG:
				status = "414 URI Too Long";
				break;
			case HTTPD_411_LENGTH_REQUIRED:
				status = "411 Length Required";
				break;
			case HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE:
				status = "431 Request Header Fields Too Large";
				break;
			case HTTPD_500_INTERNAL_SERVER_ERROR:
			default:
				status = "500 Internal Server Error";
		}

		/* Set error code in HTTP response */
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_status(this->httpRequest, status.c_str()));
	}

	string getUriComponent(int index) {
		
		string uri = string(this->httpRequest->uri);

		uri = this->uriDecode(uri);

		size_t start = uri.find("/");
		start++;

		while (index--){
			start = uri.find("/", start);
			start++;
		}

		size_t end = uri.find("/", start);

		if (end == string::npos) {
			end = uri.length();
		}

		return uri.substr(start, end - start);
	}
	string getPostData() {

		size_t length = this->httpRequest->content_len + 1;
		
		char * buffer = (char *) malloc(length * sizeof(char));

		int bytesRecieved = httpd_req_recv(this->httpRequest, buffer, httpRequest->content_len);

		if (bytesRecieved <= 0) {  /* 0 return value indicates connection closed */
			/* Check if timeout occurred */
			if (bytesRecieved == HTTPD_SOCK_ERR_TIMEOUT) {
				/* In case of timeout one can choose to retry calling
				* httpd_req_recv(), but to keep it simple, here we
				* respond with an HTTP 408 (Request Timeout) error */
				// httpd_resp_send_408(httpRequest);
				free(buffer);
				return NULL;
			}
			/* In case of error, returning ESP_FAIL will
			* ensure that the underlying socket is closed */
			free(buffer);
			return NULL;
		}

		buffer[length - 1] = '\0';

		string result = string(buffer);

		free(buffer);

		return result;
	}
	string uriEncode(const string &uri) {
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
	string uriDecode(string &uri) {
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
};

class HttpServer{
	private:
	httpd_handle_t server = NULL;
	httpd_config_t config;


	public:
	string		tag = "HttpServer";

	HttpServer(){

		this->config = HTTPD_DEFAULT_CONFIG();

		this->config.max_uri_handlers = 64;
		this->config.uri_match_fn = httpd_uri_match_wildcard;

		ESP_ERROR_CHECK(httpd_start(&this->server, &this->config));
	}

	esp_err_t registerHttpUri(HttpUri *httpUri) {

		httpd_uri_t uriHandeler;
		uriHandeler.uri = httpUri->uri.c_str();
		uriHandeler.method = httpUri->method;
		uriHandeler.handler = &httpUri->httpHandler;
		uriHandeler.user_ctx = httpUri;

		return httpd_register_uri_handler(server, &uriHandeler);
	}
};