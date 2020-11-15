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
#include "utils.h"
#include "utils.c"

#define DEBUG printf("D\n");

extern int errno;

/* FEW NOTES
 * 
 * RESULTS = List of Unique PNG files that creates png_urls.txt file
 * VISITED = List of URLs the web crawler has seen
 * FRONTIER = List of URLs that are pending to be visited.
 *
 * Currently items of these lists are all inside urls[]
 * urls-> [url_1, url_2, url_3, url_4, url_6, ..., url_n]
 * 			 ^					  ^					 ^
 *			 0					  i					 n
 * urls[0..i] is the visited list
 * urls[i+1..n] is the frontier list
 * where i the url the crawler is currently on
 * The multithreaded version may need these lists seperated (not sure)
 *
 * Currently I am not storing RESULTS in a list, they are being written straight to png_urls.txt directly
 *
 * BUGS
 * I have seen 1 or 2 duplicate urls in url logfile, but those duplicates only occur from 1 or 2 specific urls
 * you can vew duplicate lines in a txt file using the following command
 * sort <filename>  | uniq -d
 *
 * If the hashtable is not large enough you might see entry errors. Currently the hashtable is set to 500 entries
 */ 



/* create a hashtable */
ENTRY slot;
char* urls[HASH_TABLE_SIZE];

/* Global Vars */
int url_count=0; 
int png_count=0;
FILE* png_log_file;

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
						int errornum = errno;
						fprintf(stderr, "Entry error: %s\n", strerror( errornum ));
						free(urls[url_count]);
						url_count--;
						return 1;
				}


		}
		return 0;
}

/* I commented the write file becuase it was not needed in single threaded version
 * It might be useful in the multithreaded version.
 */
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

/* Commented write file for the same reason */
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
		//pid_t pid =getpid();
		//char fname[256];
		char *eurl = NULL;          /* effective URL */
		curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);

		if ( eurl != NULL) {
				/* Validate the the image has a png signature */
				U8* buf=(U8*)malloc(sizeof(U8)*PNG_SIG_SIZE);
				memcpy(buf, p_recv_buf->buf, sizeof(U8)*PNG_SIG_SIZE);
				if(is_png(buf, sizeof(U8)*PNG_SIG_SIZE)){
						png_count++;
						printf("Adding PNG\n");
						/* save url to log file */
						fprintf(png_log_file, "%s\n", eurl);
				}else{
					printf("Invalid PNG\n");
				}
				free(buf);
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
				fprintf(stderr, "Error. Respose Code: %ld\n", response_code);
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
		
		/* If the data is for instance jpeg data
		 * the following code will write it to a file 
		 */
		
		/*else {
		  sprintf(fname, "./output_%d", pid);
		  }

		  return write_file(fname, p_recv_buf->buf, p_recv_buf->size); */
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

		/* Start curl process */	   
		CURL *curl_handle;
		CURLcode res;
		RECV_BUF recv_buf;
		curl_global_init(CURL_GLOBAL_DEFAULT);

		/* This loop will continue until all urls are visited */
		/* url_count will be incrementing inside the loop each time a url is found */
		for(int i=0; i < url_count; i++){
				strcpy(url, urls[i]);
				//printf("%s: URL is %s\n", argv[0], url);
				
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

				/* else {
				   printf("%lu bytes received in memory %p, seq=%d.\n", \
				   recv_buf.size, recv_buf.buf, recv_buf.seq);
				   }
				   */

				/* process the download data */
				/* Find the links in the html */
				process_data(curl_handle, &recv_buf);

				/* Add urls to url_logfile if needed */
				if(v != NULL){
						fprintf(logfile, "%s\n", url);
				}
				curl_easy_cleanup(curl_handle);
				recv_buf_cleanup(&recv_buf);
				if(png_count >= m)
						break;
		}

		/* url ---> NULL */
		/* print_hashmap(); */
		
		/* Close the logfiles */
		if(v != NULL)
				fclose(logfile);
		fclose(png_log_file);
		curl_global_cleanup();


		/* cleaning up */
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
