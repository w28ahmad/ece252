/* Paster single threaded
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>
#include "utils.c"


int save_image(char* url, bool* seq_images){
	CURL *curl_handle;
	CURLcode res;
	RECV_BUF recv_buf;
	char fname[256];
	//pid_t pid =getpid();

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
	
	/* Save the image if not already saved */
	if(seq_images[recv_buf.seq] == 0){
    	sprintf(fname, "./output_%d.png", recv_buf.seq);
    	write_file(fname, recv_buf.buf, recv_buf.size);
		seq_images[recv_buf.seq] = 1;
	}else{
		printf("seq %d already exists\n", recv_buf.seq);
	}

    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
	return 0;
}

int array_sum(bool* seq_images, int size){
	int res=0;
	for(int i=0; i < size; i++){
		res+= (int)seq_images[i];
	}
	return res;
}

int main(int argc, char** argv){
	char* url = IMG_URL_1; /* default url */
	bool seq_images[NUM_IMAGES] = { 0 };

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
	
	/* Keep requesting for images until all images are saved */
	while(array_sum(seq_images, NUM_IMAGES) != NUM_IMAGES){
		save_image(url, seq_images);
	}

	size_t size = sizeof("output_?.png ");	
	char param[NUM_IMAGES * size];
	int pos = 0;

	for(int i=0; i < NUM_IMAGES; i++){
		pos += sprintf(&param[pos], "output_%d.png ", i);
	}
	
	char* cmd = concat("./catpng.out ", param);
	/* Concat images using the previously built concat program */
	system(cmd);
	
	free(cmd);
	return 0;
}
