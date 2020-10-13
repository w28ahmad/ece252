/* Paster single threaded
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>
#include "utils.c"


int save_image(char* url){
	CURL *curl_handle;
	CURLcode res;
	RECV_BUF recv_buf;
	char fname[256];
	pid_t pid =getpid();

	recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

	/* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
	/* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
		printf("%lu bytes received in memory %p, seq=%d.\n", \
			recv_buf.size, recv_buf.buf, recv_buf.seq);
    }

    sprintf(fname, "./output_%d_%d.png", recv_buf.seq, pid);
    write_file(fname, recv_buf.buf, recv_buf.size);

    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
	return 0;
}

int main(int argc, char** argv){
	char* url = IMG_URL_1;
	/* Get arguments */
	int c;
	int t = 1;
	int n = 1;
	char *str = "option requires an argument";

	while ((c = getopt (argc, argv, "t:n:")) != -1) {
		switch (c) {
			case 't':
				t = strtoul(optarg, NULL, 10);
				if (t <= 0) {
					fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
					return -1;
				}
				break;
			case 'n':
				n = strtoul(optarg, NULL, 10);
				if (n <= 0 || n > 3) {
					fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
					return -1;
				}
				switch(n){
					case 2:
						url = IMG_URL_2;
						break;
					case 3:
						url = IMG_URL_3;
						break;
				}
				break;
			default:
				return -1;
		}
	}

	/* printf("%s %i", url, t); */
	save_image(url);
	return 0;
}
