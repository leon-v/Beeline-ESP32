#include "http_server.h"

static const char  template_top_html[]	asm("_binary_template_top_html_start");
static const char  template_bottom_html[]	asm("_binary_template_bottom_html_start");

void httpServerSSIPage(httpd_req_t *req, char * ssiTag) {

	if (strcmp(ssiTag, "template_top_html") == 0){
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, template_top_html));
	}

	else if (strcmp(ssiTag, "template_bottom_html") == 0){
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, template_bottom_html));
	}

	else{
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, "SSI Page tag not found "));
		ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_resp_sendstr_chunk(req, ssiTag));

	}

}