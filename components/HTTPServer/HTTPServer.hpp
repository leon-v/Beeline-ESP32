#include "string.h"

#include "esp_http_server.h"
#include "cJSON.h"


class HTTPURI;

typedef void (HTTPURIHandler)(HTTPURI *httpUri);

class HTTPURI{
	private:
		const char * tag = "HTTPURI";
	public:
		const char * uri;
		httpd_method_t method;
		HTTPURIHandler *httpUriHandler;
		httpd_req_t * httpRequest;

		HTTPURI(const char * uri, httpd_method_t method, HTTPURIHandler *httpUriHandler){
			this->uri = uri;
			this->method = method;
			this->httpUriHandler = httpUriHandler;
		}

		static esp_err_t httpHandler(httpd_req_t * httpRequest) {

			HTTPURI *httpUri = (HTTPURI *) httpRequest->user_ctx;

			httpUri->httpRequest = httpRequest;

			httpUri->httpUriHandler(httpUri);

			return ESP_OK;
		}

		void sendChunk(char * response) {

			if (!response) {
				ESP_LOGE(this->tag, "string passed if null in sendChunk");
				ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
				return;
			}

			if (!strlen(response)) {
				return;
			}

			ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(this->httpRequest, response));
		}

		void sendChunk(const char * response) {

			this->sendChunk( (char *) response);
		}

		void endChunks(){
			ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(this->httpRequest, NULL));
		}

		void sendJSON(const cJSON * json) {

			if (!json) {
				this->sendJSONError("JSON object passed is null", HTTPD_500_INTERNAL_SERVER_ERROR);
				return;
			}

			char * jsonString = cJSON_Print(json);

			if (!jsonString) {
				ESP_LOGE(this->tag, "cJSON_Print failed in sendJSON");
				ESP_ERROR_CHECK_WITHOUT_ABORT(ESP_FAIL);
				return;
			}

			this->setType("application/json");

			this->sendChunk(jsonString);

			free(jsonString);

			this->endChunks();
		}

		void sendJSONError(const char * userMessage, httpd_err_code_t errorCode) {

			this->setResponseCode(errorCode);

			cJSON * response = cJSON_CreateObject();

			cJSON * success = cJSON_CreateBool(false);
			cJSON_AddItemToObject(response, "success", success);

			cJSON * message = cJSON_CreateString(userMessage);
			cJSON_AddItemToObject(response, "message", message);

			this->sendJSON(response);
			
			cJSON_Delete(response);
		}

		void setType(const char * type) {
			httpd_resp_set_type(this->httpRequest, type);
		}

		void setHeader(const char * field, const char * value) {
			httpd_resp_set_hdr(this->httpRequest, field, value);
		}
		
		char * getUriComponent(int index) {

			int currentIndex = 0;

			char * start = strchr(this->httpRequest->uri, '/');

			char * end = start + strlen(this->httpRequest->uri);

			if (start) {
				start++;
			}

			if (start >= end) {
				ESP_LOGE("getUriComponent", "Delimiter error 1");
				return NULL;
			}

			while (currentIndex < index) {

				start = strchr(start, '/');

				if (start) {
					start++;
				}

				if (!start) {
					ESP_LOGE("getUriComponent", "Delimiter error 2");
					return NULL;
				}

				if (start >= end) {
					ESP_LOGE("getUriComponent", "Delimiter error 3");
					return NULL;
				}

				currentIndex++;
			}
			
			char * endToken = strchr(start, '/');

			end = endToken ? endToken : end;

			size_t length = end - start;

			char * output = (char *) malloc((length + 1) * sizeof(char));

			if (!output) {
				ESP_LOGE("getUriComponent", "Failed to malloc");
				return NULL;
			}

			output[0] = '\0';

			strncat(output, start, length);

			return output;
		}

		void setResponseCode(httpd_err_code_t error){
			const char *status;
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
			ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_set_status(this->httpRequest, status));
		}

		char * getPostData() {

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

			return buffer;
		}
};

class HTTPServer{
	private:
		httpd_handle_t server = NULL;
		httpd_config_t config;


	public:
		HTTPServer(){

			this->config = HTTPD_DEFAULT_CONFIG();

			this->config.max_uri_handlers = 64;
			this->config.uri_match_fn = httpd_uri_match_wildcard;
			// this->config.stack_size = (4096 * 2);

			ESP_ERROR_CHECK(httpd_start(&this->server, &this->config));
		}

		esp_err_t registerHttpUri(HTTPURI *httpUri) {

			httpd_uri_t uriHandeler;
			uriHandeler.uri = httpUri->uri;
			uriHandeler.method = httpUri->method;
			uriHandeler.handler = &httpUri->httpHandler;
			uriHandeler.user_ctx = httpUri;

			return httpd_register_uri_handler(server, &uriHandeler);
		}
};