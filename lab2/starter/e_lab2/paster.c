#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h> /* For boolean type */
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>
#include "utils.c"

/* Requests image from the server, if the image seq is new save it to a png file */
int save_image(char* url, bool* seq_images, pthread_mutex_t* lock){
	CURL *curl_handle;
	CURLcode res;
	RECV_BUF recv_buf;
	char fname[256];
	//pid_t pid =getpid();

	recv_buf_init(&recv_buf, BUF_SIZE);
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
    }

	/* Add a lock to avoid the possibility of 2 threads overwritting the same file */
	pthread_mutex_lock(lock);
	/* Save the image if not already saved */
	if(seq_images[recv_buf.seq] == 0){
    	sprintf(fname, "./output_%d.png", recv_buf.seq);
    	write_file(fname, recv_buf.buf, recv_buf.size);
		seq_images[recv_buf.seq] = 1;
	}
	pthread_mutex_unlock(lock);

    /* cleaning up */
    curl_easy_cleanup(curl_handle);
	recv_buf_cleanup(&recv_buf);
	return 0;
}


struct thread_args              /* thread input parameters struct */
{
    bool* images;
    int size;
	char* url;
	pthread_mutex_t* lock;
};


int array_sum(bool* seq_images, int size){
	int res=0;
	for(int i=0; i < size; i++){
		res+= (int)seq_images[i];
	}
	return res;
}


void *thread_function(void* arg){
	struct thread_args *p_in = (struct thread_args*)arg;
	int *res = (int*)malloc(sizeof(int));
	while(array_sum(p_in->images, NUM_IMAGES) != NUM_IMAGES){
		save_image(p_in->url, p_in->images, p_in->lock);
	}
	return ((void *)res);
}


int main(int argc, char** argv){
	char* url = (char*)IMG_URL_1; /* default url */
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
						url = (char*)IMG_URL_2;
						break;
					case 3:
						url = (char*)IMG_URL_3;
						break;
				}
				break;
			default:
				return -1;
		}
	}

	/* Create threads to get image segments */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	pthread_t *p_tids = (pthread_t*)malloc(sizeof(pthread_t) * t);
    pthread_mutex_t lock;
	pthread_mutex_init(&lock, NULL);

	struct thread_args in_params;
    struct thread_ret *p_results[t];
    
	in_params.images = seq_images;
	in_params.url = url;
	in_params.size = t;
	in_params.lock = &lock;

    for (int i = 0; i < t; i++) {
        pthread_create(p_tids + i, NULL, thread_function, &in_params);
    }

    for (int i = 0; i < t; i++) {
        pthread_join(p_tids[i], (void **)&(p_results[i]));
        /* printf("Thread ID %lu joined.\n", p_tids[i]); */
    }
	curl_global_cleanup();

    /* cleaning up */
    free(p_tids);
    
    /* the memory was allocated for return values */
    /* we need to free the memory here */
    for (int i = 0; i < t; i++) {
        free(p_results[i]);
    }


	/* Concat images using the previously built catpng program */
	size_t size = sizeof("output_?.png ");	
	char param[NUM_IMAGES * size];
	int pos = 0;
	for(int i=0; i < NUM_IMAGES; i++){
		pos += sprintf(&param[pos], "output_%d.png ", i);
	}
	
	char* cmd = concat("./catpng ", param);
	system(cmd);
	free(cmd);
	
	return 0;
}
