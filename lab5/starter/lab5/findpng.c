#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/multi.h>
#include <string.h>
#include <search.h> /* hashmap */
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <errno.h>
#include "utils.h"
#include "utils.c"

#define DEBUG printf("D\n");
extern int errno;

int get_params(int* t, int* m, char** v, int argc, char** argv);
int add_url_to_map(char* url);
void print_hashmap();
int is_png(U8 *buf, size_t n);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
static void init(RECV_BUF* ptr, CURLM* cm, int i);
int process_data(CURL* curl_handle, RECV_BUF *p_recv_buf);

ENTRY slot; 					/* For hashmap */
char* urls[HASH_TABLE_SIZE]; 	/* Stores unique visted urls */
int url_count=0;				/* Total url count */
int png_count=0;				/* Total png url count */
FILE* png_log_file;				/* File discriptor png logfile */
int to_crawl = 0;				/* used as a loop iterator, crawls from to_crawl to url_count */

int main(int argc, char** argv){
	/* get parameters */
	int t=1;		/* Max concurrent connections; default 1 */
	int m=50;		/* Max PNG urls; default 50 */
	char* v=NULL;	/* Visited URLs logfile */

	if(get_params(&t, &m, &v, argc, argv) < 0){
		return -1;
	}
	char url[256];
	strcpy(url, argv[argc-1]);
	//printf("%d %d %s %s\n", t, m, v, url);

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
		logfile = fopen(v, "a");
	}
	png_log_file = fopen(PNG_LOGFILE, "a");

	/* Create hashtable */
	hcreate(HASH_TABLE_SIZE);
	add_url_to_map(url);

	/* Initialize curl variables */
	CURLM *cm = NULL;
	CURL *eh = NULL;
	CURLMsg *msg=NULL;
	RECV_BUF recv_buf[m];
	CURLcode return_code = (CURLcode)0;
	int still_running=0, msgs_left=0;
	int http_status_code;

	curl_global_init(CURL_GLOBAL_ALL);
	cm = curl_multi_init();

	/* TODO: loop over unique urls */
	/*************************** Start loop **************************/
	
	
	/* Initialize links */
	int i = 0; /* Ensure have less than max concurrent connections */
	for(; to_crawl < url_count && i < t ; to_crawl++, i++){
		init(&recv_buf[i], cm, to_crawl);
	}

	/* Perform */
	curl_multi_perform(cm, &still_running);
	/* Keep Performing */
	do {
		int numfds=0;
		int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
		if(res != CURLM_OK) {
			fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
			return EXIT_FAILURE;
		}
		curl_multi_perform(cm, &still_running);
	} while(still_running);

	/* Check responses */
	while ((msg = curl_multi_info_read(cm, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			eh = msg->easy_handle;

			return_code = msg->data.result;
			if(return_code!=CURLE_OK) {
				fprintf(stderr, "CURL error code: %d\n", msg->data.result);
				continue;
			}

			http_status_code=0;
			char* crawled_url;
			curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
			curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &crawled_url);
	
			if(http_status_code==200) {
				printf("200 OK for %s\n", crawled_url);	/* Not sure why szUrl is null? (But it is not necessary for the program) */
				for(int j=0; j < i; j++){
					printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf[j].size, recv_buf[j].buf, recv_buf[j].seq);
					/* printf("%s\n", recv_buf[j].buf); */

					/* Parse/Process Data */
					process_data(eh, &recv_buf[j]);
					/* Stop png limit is reached */
					if(png_count >= m)
						break;
				}
			} else {
				fprintf(stderr, "GET of %s returned http status code %d\n", crawled_url, http_status_code);
			}

			curl_multi_remove_handle(cm, eh);
			curl_easy_cleanup(eh);
		}
		else {
			fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
		}
	}

	/* Clean */
	for(int j=0; j < i; j++){
		recv_buf_cleanup(&recv_buf[j]);
	}


	
	/* TODO: Add the unique looped urls to the url_logfile */
	/*************************** End loop ****************************/

	/* Cleanup */
	curl_multi_cleanup(cm);
	/* Close the logfiles */
	if(v != NULL)
		fclose(logfile);
	fclose(png_log_file);

	for(int i=0; i < url_count; i++){
		free(urls[i]);
	}
	hdestroy();

	/* Stop the timer */
	/* TODO: Check timer rounding (its different this time?)*/
	if (gettimeofday(&tv, NULL) != 0) {
		perror("gettimeofday");
		abort();
	}
	times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
	printf("findpng3 execution time: %lf seconds\n", times[1] - times[0]);
	return 0;
}

/* @brief: adds url to hashmap: 
 * key      value
 * url----> NULL
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
		
		/* TODO: if the bug from lab4 is still present, add if cond. to  remove it */
		/* Add the data to the table */
		printf("Adding url: %s to hashmap\n", url);
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

/* Check png signature to verify png */
int is_png(U8 *buf, size_t n){
	U8 SIGNATURE[PNG_SIG_SIZE] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	if (memcmp(SIGNATURE, buf, n)){
		return 0;
	}
	return 1;
}

int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
	char *eurl = NULL;
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

/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */
int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf)
{

	CURLcode res;
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

	return 0;
}

static void init(RECV_BUF* ptr, CURLM *cm, int i)
{
	/* init user defined call back function buffer */
	if ( recv_buf_init(ptr, BUF_SIZE) != 0 ) {
		fprintf(stderr, "recv_buf_init did not work!\n");
	}

	CURL *eh = curl_easy_init();
	/* specify URL to get */
	curl_easy_setopt(eh, CURLOPT_URL, urls[i]);
	/* register write call back function to process received data */
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb_curl3);
	/* user defined data structure passed to the call back function */
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)ptr);
	/* register header call back function to process received header data */
	curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, header_cb_curl);
	/* user defined data structure passed to the call back function */
	curl_easy_setopt(eh, CURLOPT_HEADERDATA, (void *)ptr);
	/* some servers requires a user-agent field */
	curl_easy_setopt(eh, CURLOPT_USERAGENT, "ece252 lab4 crawler");
	/* follow HTTP 3XX redirects */
	curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
	/* continue to send authentication credentials when following locations */
	curl_easy_setopt(eh, CURLOPT_UNRESTRICTED_AUTH, 1L);
	/* max numbre of redirects to follow sets to 5 */
	curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 5L);
	/* supports all built-in encodings */ 
	curl_easy_setopt(eh, CURLOPT_ACCEPT_ENCODING, "");

	/* Max time in seconds that the connection phase to the server to take */
	//curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
	/* Max time in seconds that libcurl transfer operation is allowed to take */
	//curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
	/* Time out for Expect: 100-continue response in milliseconds */
	//curl_easy_setopt(curl_handle, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);

	/* Enable the cookie engine without reading any initial cookies */
	curl_easy_setopt(eh, CURLOPT_COOKIEFILE, "");
	/* allow whatever auth the proxy speaks */
	curl_easy_setopt(eh, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
	/* allow whatever auth the server speaks */
	curl_easy_setopt(eh, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	curl_multi_add_handle(cm, eh);
}

/**
 * @brief  cURL header call back function to extract image sequence number from 
 * http header data. An example header for image part n (assume n = 2) is:
 * X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
	int realsize = size * nmemb;
	RECV_BUF *p = (RECV_BUF*)userdata;

	if (realsize > (int)strlen(ECE252_HEADER) &&
			strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

		/* extract img sequence number */
		p->seq = atoi(p_recv + strlen(ECE252_HEADER));

	}
	return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
	void *p = NULL;

	if (ptr == NULL) {
		return 1;
	}

	p = malloc(max_size);
	if (p == NULL) {
		return 2;
	}

	ptr->buf = (char*)p;
	ptr->size = 0;
	ptr->max_size = max_size;
	ptr->seq = -1;              /* valid seq should be positive */
	return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
	if (ptr == NULL) {
		return 1;
	}

	free(ptr->buf);
	ptr->size = 0;
	ptr->max_size = 0;
	return 0;
}

/**
 * @brief write callback function to save a copy of received data in RAM.
 * The received libcurl data are pointed by p_recv, 
 * which is provided by libcurl and is not user allocated memory.
 * The user allocated memory is at p_userdata. One needs to
 * cast it to the proper struct to make good use of it.
 * This function maybe invoked more than once by one invokation of
 * curl_easy_perform().
 **/
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
	size_t realsize = size * nmemb;
	RECV_BUF *p = (RECV_BUF *)p_userdata;

	if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
		/* received data is not 0 terminated, add one byte for terminating 0 */
		size_t new_size = p->max_size + max(BUF_INC, (int)realsize + 1);   
		char *q = (char*)realloc(p->buf, new_size);
		if (q == NULL) {
			perror("realloc"); /* out of memory */
			return -1;
		}
		p->buf = q;
		p->max_size = new_size;
	}

	memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
	p->size += realsize;
	p->buf[p->size] = 0;

	return realsize;
}

int get_params(int* t, int* m, char** v, int argc, char** argv){
	int c;
	while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
		switch (c) {
			case 't':
				*t = strtoul(optarg, NULL, 10);
				if (*t <= 0) {
					fprintf(stderr, "-t must be bigger than 0");
					return -1;
				}
				break;
			case 'm':
				*m = strtoul(optarg, NULL, 10);
				if (*m <= 0) {
					fprintf(stderr, "-m must be greater than 0");
					return -1;
				}
				break;
			case 'v':
				*v = optarg;
				break;
			default:
				return -1;
		}

	}
	return 0;
}
