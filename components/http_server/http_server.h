#ifndef HTTP_SERVER_H

#include <esp_http_server.h>

typedef struct {
	const char * uri;
	const char * page;
	const char * type;
} httpPage_t;

typedef struct {
	const char * uri;
	const char * start;
	const char * end;
	const char * type;
} httpFile_t;

typedef struct{
	char * key;
	char * value;
} token_t;

typedef struct{
	token_t tokens[32];
	unsigned int length;
} tokens_t;

void httpServerInit(void);
void httpServerAddPage(const httpPage_t * httpPage);

void httpServerSSIPage(httpd_req_t *req, char * ssiTag);

void httpServerSSINVSGet(httpd_req_t *req, char * ssiTag);
void httpServerSSINVSSet(char * ssiTag, char * value);

void httpSSIFunctionsGet(httpd_req_t *req, char * ssiTag);

#define MAX_HTTP_SSI_KEY_LENGTH 32

#define HTTP_SERVER_H
#endif