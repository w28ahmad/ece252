#pragma once

/*********************************************************************
 * MACROS
 *********************************************************************/
#define ECE252_HEADER "X-Ece252-Fragment: "
#define NUM_SLICES 50
#define NUM_SEMS 2
#define SEM_PROC 1
#define min(a,b) \
		({ __typeof__ (a) _a = (a); \
		 __typeof__ (b) _b = (b); \
		 _a > _b ? _b : _a; })
/*********************************************************************
 * STRUCTURES
 *********************************************************************/
/* This is a flattened structure, buf points to 
 * the memory address immediately after 
 * the last member field (i.e. seq) in the structure.
 * Here is the memory layout. 
 * Note that the memory is a chunk of continuous bytes.
 * On a 64-bit machine, the memory layout is as follows:
 * +================+
 * | buf            | 8 bytes
 * +----------------+
 * | size           | 8 bytes
 * +----------------+
 * | max_size       | 8 bytes
 * +----------------+
 * | seq            | 4 bytes
 * +----------------+
 * | padding        | 4 bytes
 * +----------------+
 * | buf[0]         | 1 byte
 * +----------------+
 * | buf[1]         | 1 byte
 * +----------------+
 * | ...            | 1 byte
 * +----------------+
 * | buf[max_size-1]| 1 byte
 * +================+
 **/
typedef struct recv_buf_flat {
		char *buf;       /* memory to hold a copy of received data */
		size_t size;     /* size of valid data in buf in bytes*/
		size_t max_size; /* max capacity of buf in bytes*/
		int seq;         /* >=0 sequence number extracted from http header */
				   		 /* <0 indicates an invalid seq number */
} RECV_BUF;

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
int sizeof_shm_recv_buf(size_t nbytes);
int shm_recv_buf_init(RECV_BUF *ptr, size_t nbytes);
