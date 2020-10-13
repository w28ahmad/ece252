/* Paster single threaded
 */

#include <string.h>
#include <curl/curl.h>
#include "helper.h"

int main(int argc, char** argv){
	CURL *curl_handle;
	CURLcode res;
	char* url = DUM_URL;
	/* Get arguments */

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	
	if(curl_handle){
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		res = curl_easy_perform(curl_handle);

		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
	}


	curl_global_cleanup();
	return 0;
}
