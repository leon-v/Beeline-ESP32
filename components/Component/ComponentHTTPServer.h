#ifndef _COMPONENT_HTTP_SERVER_H_
#define _COMPONENT_HTTP_SERVER_H_

#include "Component.h"

esp_err_t componentHTTPServerInit(void);
esp_err_t componentHTTPServerStart(void);
esp_err_t componentHTTPServerRegister(httpd_uri_t * uriHandeler);
void componentHTTPServerURIDecode(char * input);
char * componentHTTPServerGetPostData(httpd_req_t * httpRequest);

#endif