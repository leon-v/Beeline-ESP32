#include <sys/param.h>
#include <esp_http_server.h>
#include <http_parser.h>
#include <esp_log.h>

#include "components.h"
#include "http_server.h"

static component_t component = {
	.name = "HTTP Server",
	.messagesIn = 0,
	.messagesOut = 0,
	.priority = 10
};

httpd_handle_t server = NULL;

#define START_SSI "<!--#"
#define END_SSI "-->"

static const char index_html_start[] asm("_binary_index_html_start");
const httpPage_t httpPageIndexHTML = {
	.uri	= "/",
	.page	= index_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char menu_html_start[] asm("_binary_menu_html_start");
const httpPage_t httpPageMenuHTML = {
	.uri	= "/menu.html",
	.page	= menu_html_start,
	.type	= HTTPD_TYPE_TEXT
};

static const char menu_css_start[] asm("_binary_menu_css_start");
const httpPage_t httpPageMenuCSS = {
	.uri	= "/menu.css",
	.page	= menu_css_start,
	.type	= "text/css"
};

static const char style_css_start[] asm("_binary_style_css_start");
const httpPage_t httpPageStyleCSS = {
	.uri	= "/style.css",
	.page	= style_css_start,
	.type	= "text/css"
};

static const char javascript_js_start[] asm("_binary_javascript_js_start");
const httpPage_t httpPageJavascriptJS = {
	.uri	= "/javascript.js",
	.page	= javascript_js_start,
	.type	= "application/javascript"
};

static const char favicon_png_start[]	asm("_binary_favicon_png_start");
static const char favicon_png_end[]		asm("_binary_favicon_png_end");
httpFile_t httpFileFaviconPNG = {
	.uri	= "/favicon.png",
	.start	= favicon_png_start,
	.end	= favicon_png_end,
	.type	= "image/png"
};


httpPage_t * httpPages[32];
unsigned char httpPagesLength = 0;

httpFile_t * httpFiles[4];
unsigned char httpFilesLength = 0;

void httpServerAddPage(const httpPage_t * httpPage){
	httpPages[httpPagesLength] = httpPage;
	httpPagesLength++;
}

void httpServerAddFile(const httpFile_t * httpFile){
	httpFiles[httpFilesLength] = httpFile;
	httpFilesLength++;
}

static void httpServerURLDecode(char * input, int length) {

    char * output = input;
    char hex[3] = "\0\0\0";

    while (input[0] != '\0') {

    	if (!length--){
    		break;
    	}


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

char * httpServerParseValues(tokens_t * tokens, char * buffer, const char * rowDelimiter, const char * valueDelimiter, const char * endMatch){

	tokens->length = 0;

	// Start parsing the values by creating a new string from the payload
	char * token = strtok(buffer, rowDelimiter);

	char * end = buffer + strlen(buffer);

	// break apart the string getting all the parts delimited by &
	while (token != NULL) {

		if (strlen(endMatch) > 0){
			end = token + strlen(token) + 1;

			if (strncmp(end, endMatch, strlen(endMatch)) == 0) {
				end+= strlen(endMatch);
				break;
			}
		}

		tokens->tokens[tokens->length++].key = token;


		token = strtok(NULL, rowDelimiter);
	}

	// Re-parse the strigns and break them apart into key / value pairs
	for (unsigned int index = 0; index < tokens->length; index++){

		tokens->tokens[index].key = strtok(tokens->tokens[index].key, valueDelimiter);

		tokens->tokens[index].value = strtok(NULL, valueDelimiter);

		// If the value is NULL, make it point to an empty string.
		if (tokens->tokens[index].value == NULL){
			tokens->tokens[index].value = tokens->tokens[index].key + strlen(tokens->tokens[index].key);
		}

		httpServerURLDecode(tokens->tokens[index].key, CONFIG_HTTP_NVS_MAX_STRING_LENGTH);
		httpServerURLDecode(tokens->tokens[index].value, CONFIG_HTTP_NVS_MAX_STRING_LENGTH);
	}

	return end;
}


char * httpServerGetTokenValue(tokens_t * tokens, const char * key){

	for (unsigned int index = 0; index < tokens->length; index++){

		if (strcmp(tokens->tokens[index].key, key) == 0){
			return tokens->tokens[index].value;
		}
	}

	return NULL;
}

void httpServerPageReplaceTag(httpd_req_t *req, char * tag) {

	char * module = strtok(tag, ":");
	char * error = NULL;

	if (!module){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Failed to get module from tag"));
	}

	char * ssiTag = strtok(NULL, "");

	if (strcmp(module, "nvs") == 0){
		httpServerSSINVSGet(req, ssiTag);
	}

	else if (strcmp(module, "functions") == 0){
		httpSSIFunctionsGet(req, ssiTag);
	}

	else if (strcmp(module, "page") == 0){
		httpServerSSIPage(req, ssiTag);
	}
	else if (strcmp(module, "components") == 0){
		componentsGetHTML(req, ssiTag);
	}
	else if (strcmp(module, "get") == 0) {
		httpSSIGetGet(req, ssiTag);
	}


	else{
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Failed to parse module for tag"));
	}
}
static void httpServerPageGetContent(httpd_req_t *req){

	httpPage_t * httpPage = (httpPage_t *) req->user_ctx;

	char * tagEndHTMLStart = httpPage->page;
	char * tagStartHTMLEnd = strstr(tagEndHTMLStart, START_SSI);

	int length;

	if (!tagStartHTMLEnd){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, tagEndHTMLStart));
		return;
	}


	while (tagStartHTMLEnd != NULL) {

		length = tagStartHTMLEnd - tagEndHTMLStart;

		if (length > 0){
			ESP_ERROR_CHECK(httpd_resp_send_chunk(req, tagEndHTMLStart, length));
		}

		tagStartHTMLEnd+= strlen(START_SSI);

		tagEndHTMLStart = strstr(tagEndHTMLStart, END_SSI);

		if (!tagEndHTMLStart){
			tagStartHTMLEnd = strstr(tagStartHTMLEnd, START_SSI);
			continue;
		}

		length = tagEndHTMLStart - tagStartHTMLEnd;

		char * tag;
		tag = malloc(length + 1);

		memcpy(tag, tagStartHTMLEnd, length);
		tag[length] = '\0';

		httpServerPageReplaceTag(req, tag);

		free(tag);

		tagEndHTMLStart+= strlen(END_SSI);
		tagStartHTMLEnd = strstr(tagEndHTMLStart, START_SSI);
	}

	tagStartHTMLEnd = httpPage->page + strlen(httpPage->page);

	length = tagStartHTMLEnd - tagEndHTMLStart;
	if (length > 0){
		ESP_ERROR_CHECK(httpd_resp_send_chunk(req, tagEndHTMLStart, length));
	}
}

void httpServerPagePost(httpd_req_t *req){

	if (req->content_len <= 0) {
		ESP_LOGE(component.name, "No POST data");
		return;
	}

	char * buffer;
	buffer = malloc(req->content_len + 1);
	int bufferLength = 0;
	int result;

	while (bufferLength < req->content_len) {

        /* Read the data for the request */
        result = httpd_req_recv(req, buffer, req->content_len - bufferLength);

        if (result <= 0) {

            if (result == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }

            break;
        }

        bufferLength+= result;

        buffer[bufferLength] = '\0';
    }

    if (bufferLength < req->content_len) {
    	httpd_resp_send_408(req);
    	ESP_LOGE(component.name, "Failed to get POST data");
    	return;
    }

	static tokens_t post;
	httpServerParseValues(&post, buffer, "&", "=", "\0");

	for (int tokenIndex = 0; tokenIndex < post.length; tokenIndex++){

		token_t * token = &post.tokens[tokenIndex];

		char * module = strtok(token->key, ":");

		if (!module){
			ESP_LOGE(component.name, "Failed to get module from tag");
			continue;
		}

		if (strcmp(module, "nvs") == 0){

			char * ssiTag = strtok(NULL, "");
			httpServerSSINVSSet(ssiTag, token->value);

		}
		else{
			ESP_LOGE(component.name, "Failed to parse module for tag");
		}
	}

	free(buffer);
}


/*
*
* Page Handlers
*
*/


static esp_err_t httpServerPageHandler(httpd_req_t *req){

	ESP_LOGI(component.name, "Start %s", req->uri);

	httpPage_t * httpPage = (httpPage_t *) req->user_ctx;

	if (req->method == HTTP_POST) {
		httpServerPagePost(req);
	}

	if (httpPage->type){
		httpd_resp_set_type(req, httpPage->type);
	}

	if (httpPage->page){
		httpServerPageGetContent(req);
	}
	else{
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Nothing found to populate content."));
	}

	/* Send empty chunk to signal HTTP response completion */
    ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, NULL));
    ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, NULL));

    ESP_LOGI(component.name, "End %s", req->uri);

    return ESP_OK;
}

static esp_err_t httpServerFileHandler(httpd_req_t *req){

	httpFile_t * httpFile = (httpFile_t *) req->user_ctx;

	if (httpFile->type){
		httpd_resp_set_type(req, httpFile->type);
	}

	size_t length = httpFile->end - httpFile->start;
	return httpd_resp_send(req, httpFile->start, length);
}

void httpServerPageRegister(const httpPage_t * httpPage){

	httpd_uri_t getURI = {
	    .uri      	= httpPage->uri,
	    .method   	= HTTP_GET,
	    .handler  	= httpServerPageHandler,
	    .user_ctx	= httpPage,
	};

	httpd_register_uri_handler(server, &getURI);

	ESP_LOGI(component.name, "Registered %s for GET", getURI.uri);

	httpd_uri_t postURI = {
	    .uri      = httpPage->uri,
	    .method   = HTTP_POST,
	    .handler  = httpServerPageHandler,
	    .user_ctx	= httpPage,
	};

	httpd_register_uri_handler(server, &postURI);

	ESP_LOGI(component.name, "Registered %s for POST", getURI.uri);
}

void httpServerFileRegister(const httpFile_t * httpFile){

	httpd_uri_t getURI = {
	    .uri      	= httpFile->uri,
	    .method   	= HTTP_GET,
	    .handler  	= httpServerFileHandler,
	    .user_ctx	= httpFile,
	};

	httpd_register_uri_handler(server, &getURI);

	ESP_LOGI(component.name, "Registered %s for GET", getURI.uri);
}

static void httpServerStart(void) {

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 64;

    // Start the httpd server
    ESP_LOGI(component.name, "Starting server on port: %d", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(component.name, "Registering URI handlers");

        int i;
        for (i = 0; i < httpPagesLength; i++){

        	httpPage_t * httpPage = httpPages[i];
        	httpServerPageRegister(httpPage);
        }

        for (i = 0; i < httpFilesLength; i++){

        	httpFile_t * httpFile = httpFiles[i];
        	httpServerFileRegister(httpFile);
        }

        return;
    }
    else{
    	ESP_LOGE(component.name, "Error starting server!");
    	server = NULL;
    }


}

void httpServerStop(void) {
    httpd_stop(server);
}

static void task(void *arg){

	EventBits_t EventBits;

	componentSetReady(&component);

	while (1){

		if (componentReadyWait("WiFi") == ESP_OK) {
			/* Start the web server */
	        if (server == NULL) {
	        	ESP_LOGI(component.name, "Start");
	        	httpServerStart();
	        }
		}

		if (componentNotReadyWait("WiFi") == ESP_OK) {
			if (server) {
				ESP_LOGI(component.name, "Stop");
	            httpServerStop();
	            server = NULL;
	        }
		}
	}

	vTaskDelete(NULL);
    return;
}

void httpServerInit(void){

	httpServerAddPage(&httpPageIndexHTML);
	httpServerAddPage(&httpPageMenuHTML);
	httpServerAddPage(&httpPageMenuCSS);
	httpServerAddPage(&httpPageStyleCSS);
	httpServerAddPage(&httpPageJavascriptJS);

	httpServerAddFile(&httpFileFaviconPNG);


	component.task = task;
	componentsAdd(&component);

	ESP_LOGI(component.name, "Init");
}
