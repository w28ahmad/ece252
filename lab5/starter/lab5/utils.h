#pragma once

/*********************************************************************
 * MACROS
 **********************************************************************/
#define PNG_LOGFILE "png_urls.txt"
#define HASH_TABLE_SIZE 500
#define BUF_INC  524288   		/* 1024*512  = 0.5M */
#define BUF_SIZE 1048576		/* 1024*1024 = 1M */
#define ECE252_HEADER "X-Ece252-Fragment: "
#define MAX_WAIT_MSECS 30*1000 	/* Wait max. 30 seconds */
#define CT_PNG  "image/png"
#define CT_HTML "text/html"
#define PNG_SIG_SIZE    8 		/* number of bytes of png image signature data */


#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })
/*********************************************************************
 * STRUCTURES
 **********************************************************************/
typedef unsigned char U8;
typedef struct recv_buf2 {
	char *buf;       	/* memory to hold a copy of received data */
	size_t size;     	/* size of valid data in buf in bytes*/
	size_t max_size; 	/* max capacity of buf in bytes*/
	int seq;         	/* >=0 sequence number extracted from http header */
	/* <0 indicates an invalid seq number */
} RECV_BUF;

/*********************************************************************
 * FUNCTIONS
 **********************************************************************/
int add_url_to_map(char* url);
htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath);
int find_http(char *buf, int size, int follow_relative_links, const char *base_url);

