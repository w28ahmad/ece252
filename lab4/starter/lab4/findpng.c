#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <search.h> /* hashmap */
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"
#include "utils.c"

#define DEBUG printf("D\n");

extern int errno;

/* create a hashtable */
ENTRY slot;
char* urls[HASH_TABLE_SIZE];

/* Global Vars */
int url_count=0; 
int png_count=0;
int uindex = -1;
FILE* png_log_file;
pthread_mutex_t lock;
pthread_mutex_t process;
sem_t empty;

struct thread_args              /* thread input parameters struct */
{
    char* v;
    int m;
	FILE* logfile;
};

void print_hashmap();

int is_png(U8 *buf, size_t n){
		U8 SIGNATURE[PNG_SIG_SIZE] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
		if (memcmp(SIGNATURE, buf, n)){
				return 0;
		}
		return 1;
}

/* @brief: adds url to hashmap: 
 * key      value
 * url----> NULL
 *
 * Notes: If the size of the hashmap is not large enough, cannot allocate memory will show in the output
 *		  If the hashmap becomes full there will also be an Entry erroor
 */
int add_url_to_map(char* url){
		slot.key=url;
		slot.data = NULL;
		pthread_mutex_lock(&lock);
		ENTRY* result = hsearch(slot, FIND);
		pthread_mutex_unlock(&lock);
		if(!result){
				/* Add url to array and update counts */
				pthread_mutex_lock(&lock);
				size_t url_size = sizeof(char)*(strlen((char*)url));
				urls[url_count] = (char*)malloc(url_size);
				memset(urls[url_count], 0, url_size);
				memcpy(urls[url_count], (char*)url, url_size);
			
				/* Invalid url parsing - does not end with a null terminator*/
				
				if(urls[url_count][url_size] != '\0'){
					free(urls[url_count]);
					pthread_mutex_unlock(&lock);
					return 1;
				}
				
	
				slot.key=urls[url_count];
				url_count++;
				pthread_mutex_unlock(&lock);

				/* Add the data to the table */
				pthread_mutex_lock(&lock);
				ENTRY* result = hsearch(slot, ENTER);
				pthread_mutex_unlock(&lock);
				if (result == NULL) {
						int errornum = errno;
						fprintf(stderr, "Entry error: %s\n", strerror( errornum ));
						pthread_mutex_lock(&lock);
						free(urls[url_count]);
						url_count--;
						pthread_mutex_unlock(&lock);
						return 1;
				} else {
					sem_post(&empty);
				}
		}
		return 0;
}

int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
		int follow_relative_link = 1;
		char *url = NULL; 

		curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
		find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 

		return 0;
}

/* Commented write file for the same reason */
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf, int m)
{
		char *eurl = NULL;          /* effective URL */
		curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);

		if ( eurl != NULL) {
				/* Validate the the image has a png signature */
				U8* buf=(U8*)malloc(sizeof(U8)*PNG_SIG_SIZE);
				memcpy(buf, p_recv_buf->buf, sizeof(U8)*PNG_SIG_SIZE);
				if(is_png(buf, sizeof(U8)*PNG_SIG_SIZE) && png_count < m){
						pthread_mutex_lock(&lock);
						png_count++;						
						/* printf("Adding PNG\n"); */
						/* save url to log file */
						fprintf(png_log_file, "%s\n", eurl);
						pthread_mutex_unlock(&lock);
				}else{
					/*printf("Invalid PNG\n");*/
				}
					free(buf);
		}		

		return 0;
}


/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */

int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf, int m)
{
		CURLcode res;
		long response_code;

		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
		if ( res == CURLE_OK ) {
				/* printf("Response code: %ld\n", response_code); */
		}

		if ( response_code >= 400 ) { 
				/* fprintf(stderr, "Error. Respose Code: %ld\n", response_code); */
				return 1;
		}

		char *ct = NULL;
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
		if ( res == CURLE_OK && ct != NULL ) {
				/* printf("Content-Type: %s, len=%ld\n", ct, strlen(ct)); */
		} else {
				/* fprintf(stderr, "Failed obtain Content-Type\n"); */
				return 2;
		}
		int val;
		if ( strstr(ct, CT_HTML) ) {
			pthread_mutex_lock(&process);
				val = process_html(curl_handle, p_recv_buf);
			pthread_mutex_unlock(&process);
			return val;
		} else if ( strstr(ct, CT_PNG) ) {
			pthread_mutex_lock(&process);
				val = process_png(curl_handle, p_recv_buf, m);
			pthread_mutex_unlock(&process);
			return val;
		} 
		
		return 0;
}

/* Prints the hashmap */
void print_hashmap(){
		ENTRY* result;
		for(int i=0; i < url_count; i++){
				slot.key=urls[i];
				slot.data=NULL;
				result = hsearch(slot, FIND);
				printf("%s ----> %s\n",
								result ? result->key : "NULL", result ? (char*)(result->data) : 0);
		}	
}

void *thread_function(void* arg){

	struct thread_args *p_in = (struct thread_args*)arg;
	char* v = p_in->v;
	int m = p_in->m;
	FILE* logfile = p_in->logfile;
	char url[256];

	/* Start curl process */
	CURL *curl_handle;
	CURLcode res;
	RECV_BUF recv_buf;

	while(1){
		
		/* Wait if there are no new urls */
		sem_wait(&empty); /* maybe replace with a conditional variable */

		pthread_mutex_lock(&lock);
		/* Exit conditions */
		if (png_count >= m || (png_count >= 50 && uindex + 1 >= url_count)) {
			pthread_mutex_unlock(&lock);
			break;
		}
		/* increment uindex and copy url */
		uindex++;
		strcpy(url, urls[uindex]);
		pthread_mutex_unlock(&lock);
		
		curl_handle = easy_handle_init(&recv_buf, url);

		if ( curl_handle == NULL ) {
				fprintf(stderr, "Curl initialization failed. Exiting...\n");
				break;
		}
		/* get the data */
		res = curl_easy_perform(curl_handle);

		if( res != CURLE_OK) {
				curl_easy_cleanup(curl_handle);
				recv_buf_cleanup(&recv_buf);
				continue;
		} 

		/* process the download data */
		/* Find the links in the html */
		process_data(curl_handle, &recv_buf, m);

		/* Add urls to url_logfile if needed */
		if(v != NULL){
				fprintf(logfile, "%s\n", url);
		}
		curl_easy_cleanup(curl_handle);
		recv_buf_cleanup(&recv_buf);
		
		/* Exit condition */
		pthread_mutex_lock(&lock); /* For the read of png_count */
		if(png_count >= m || (png_count >= 50 && uindex + 1 >= url_count)) {
				pthread_mutex_unlock(&lock);
				break;
		}
		pthread_mutex_unlock(&lock);
		
	}
	sem_post(&empty);
	return NULL;
}


int main( int argc, char** argv ) 
{	
		/* Get the parameters */
		int c;
		int t=1;		/* # of threads */
		int m=50;		/* # of unique PNG urls */
		char* v=NULL;

		while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
				switch (c) {
						case 't':
								t = strtoul(optarg, NULL, 10);
								if (t <= 0) {
										fprintf(stderr, "-t must be bigger than 0");
										return -1;
								}
								break;
						case 'm':
								m = strtoul(optarg, NULL, 10);
								if (m <= 0) {
										fprintf(stderr, "-m must be greater than 0");
										return -1;
								}
								break;
						case 'v':
								v = optarg;
								break;
						default:
								return -1;
				}
		}
		char url[256];
		strcpy(url, argv[argc-1]);
		
		/* Initialize Locks and Semaphores */
		pthread_mutex_init(&lock, NULL);
		pthread_mutex_init(&process, NULL);
		if ( sem_init(&empty, 0, 0) != 0 ) {
			perror("sem_init(empty)");
			abort();
		}


		/* Start the timer */
		double times[2];
		struct timeval tv;

		if (gettimeofday(&tv, NULL) != 0) {
				perror("gettimeofday");
				abort();
		}
		times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

		/* Create a logfile for all urls and png urls*/
		FILE* logfile;
		if(v != NULL){
				logfile=fopen(v, "a");
		}
		png_log_file=fopen(PNG_LOGFILE, "a");

		/* Initialize hash table */
		/* Note: hashtable size is hardcoded, this create issues if you are searching */
		hcreate(HASH_TABLE_SIZE);	

		/* Add the initial link to the map */
		add_url_to_map(url);

		curl_global_init(CURL_GLOBAL_DEFAULT);

		/* Threads */

		pthread_t *p_tids = (pthread_t*)malloc(sizeof(pthread_t) * t);
		struct thread_args in_params;

		in_params.v = v;
		in_params.m = m;
		in_params.logfile = logfile;
		
		/* Create t threads */
		for (int i = 0; i < t; i++) {
			pthread_create(p_tids + i, NULL, thread_function, &in_params);
		}

		/* Join threads */
		for (int i = 0; i < t; i++) {
        	pthread_join(p_tids[i], NULL);
    		}

		/* print_hashmap(); */

		/* Close the logfiles */
		if(v != NULL)
				fclose(logfile);
		fclose(png_log_file);
		curl_global_cleanup();


		/* cleaning up */
		free(p_tids);

		/* Destroy Locks and Semaphore */
		pthread_mutex_destroy(&lock);
		pthread_mutex_destroy(&process);
		if (sem_destroy(&empty)) {
		        perror("sem_destroy");
		        abort();
		}
		
		/* Free URLs */
		for(int i=0; i < url_count; i++){
				free(urls[i]);
		}
		hdestroy();

		/* Stop the timer */
		if (gettimeofday(&tv, NULL) != 0) {
				perror("gettimeofday");
				abort();
		}
		times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
		printf("findpng2 execution time: %.2lf seconds\n", times[1] - times[0]);
		return 0;
}
