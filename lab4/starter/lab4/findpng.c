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
#include <stdint.h>  /* intptr_t */

#include "utils.h"
#include "utils.c"

#define DEBUG printf("D\n");

/* create a hashtable */
ENTRY slot;
char* urls[HASH_TABLE_SIZE];
char* url_logfile=NULL; 
int url_count=0; 
int png_count=0;

void print_hashmap();

int add_url_to_map(char* url){
		slot.key=url;
		slot.data = NULL;
		ENTRY* result = hsearch(slot, FIND);
		if(!result){
				/* Add url to array and update counts */
				size_t url_size = sizeof(char)*(strlen((const char*)url));
				urls[url_count] = (char*)malloc(url_size);
				memcpy(urls[url_count], (const char*)url, url_size);
				slot.key=urls[url_count];
				url_count++;

				/* Add the data to the table */
				ENTRY* result = hsearch(slot, ENTER);
				if (result == NULL) {
						fprintf(stderr, "entry failed\n");
						return 1;
				}

				/* Add urls to url_logfile if needed */
				if(url_logfile != NULL){
					FILE* fp;
					fp=fopen(url_logfile, "a");
					fprintf(fp, "%s\n", url);
					fclose(fp);
				}

		}
		return 0;
}

int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
		//char fname[256];
		int follow_relative_link = 1;
		char *url = NULL; 
		//pid_t pid =getpid();

		curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
		find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 

		/*
		   sprintf(fname, "./output_%d.html", pid);
		   return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
		   */
		return 0;
}

int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
		//pid_t pid =getpid();
		//char fname[256];
		char *eurl = NULL;          /* effective URL */
		curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);

		if ( eurl != NULL) {
				png_count++;
				printf("PNG url found: %s\n", eurl);

				/* save url to log file */
				FILE* png_file;
				png_file=fopen(PNG_LOGFILE, "a");
				fprintf(png_file, "%s\n", eurl);
				fclose(png_file);
		}		

		/*
		   sprintf(fname, "./output_%d_%d.png", p_recv_buf->seq, pid);
		   return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
		   */
		return 0;
}
/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */

int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
		CURLcode res;
		//char fname[256];
		//pid_t pid =getpid();
		long response_code;

		res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
		if ( res == CURLE_OK ) {
				printf("Response code: %ld\n", response_code);
		}

		if ( response_code >= 400 ) { 
				fprintf(stderr, "Error.\n");
				return 1;
		}

		char *ct = NULL;
		res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
		if ( res == CURLE_OK && ct != NULL ) {
				printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));
		} else {
				fprintf(stderr, "Failed obtain Content-Type\n");
				return 2;
		}

		if ( strstr(ct, CT_HTML) ) {
				return process_html(curl_handle, p_recv_buf);
		} else if ( strstr(ct, CT_PNG) ) {
				return process_png(curl_handle, p_recv_buf);
		} 
		/*else {
		  sprintf(fname, "./output_%d", pid);
		  }

		  return write_file(fname, p_recv_buf->buf, p_recv_buf->size); */
		return 0;
}

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

		/* Start the timer */
		double times[2];
		struct timeval tv;

		if (gettimeofday(&tv, NULL) != 0) {
				perror("gettimeofday");
				abort();
		}
		times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

		/* Establish a global url_logfile */
		if(v != NULL){
			url_logfile = v;
		}

		/* Initialize hash table */
		/* TODO hashtable size is hardcoded, might create a bottleneck */
		hcreate(HASH_TABLE_SIZE);	

		/* Add the initial link to the map */
		add_url_to_map(url);

		/* Start curl process */	   
		CURL *curl_handle;
		CURLcode res;
		RECV_BUF recv_buf;
		curl_global_init(CURL_GLOBAL_DEFAULT);

		/* This loop will continue until all urls are visited */
		/* url_count will be incrementing inside the loop each time a url is found */
		for(int i=0; i < url_count; i++){
				strcpy(url, urls[i]);
				printf("%s: URL is %s\n", argv[0], url);

				curl_handle = easy_handle_init(&recv_buf, url);

				if ( curl_handle == NULL ) {
						fprintf(stderr, "Curl initialization failed. Exiting...\n");
						curl_global_cleanup();
						abort();
				}
				/* get the data */
				res = curl_easy_perform(curl_handle);

				if( res != CURLE_OK) {
						/*
						   fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
						   cleanup(curl_handle, &recv_buf);
						   exit(1);
						   */
						continue;
				} 

				/* else {
				   printf("%lu bytes received in memory %p, seq=%d.\n", \
				   recv_buf.size, recv_buf.buf, recv_buf.seq);
				   }
				   */

				/* process the download data */
				/* Find the links in the html */
				process_data(curl_handle, &recv_buf);

				if(png_count >= m)
						break;
		}
		print_hashmap();

		/* cleaning up */
		for(int i=0; i < url_count; i++){
				free(urls[i]);
		}
		cleanup(curl_handle, &recv_buf);
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
