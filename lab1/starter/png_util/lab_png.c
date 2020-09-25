/**
 * @brief Fill in Function Prototypes
 * is_png
 * get_png_height
 * get_png_width
 * get_png_data_IHDR
 **/
#include "lab_png.h"
#include <string.h> /* For memcmp memcpy */
#include <stdlib.h> /* For EXIT_FAILURE */
#include <arpa/inet.h> /* For htonl */

#define FALSE (1==0)
#define TRUE  (1==1)

int is_png(U8 *buf, size_t n){
	U8 SIGNATURE[PNG_SIG_SIZE] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	if (memcmp(SIGNATURE, buf, n)){
		return FALSE;
	}
	return TRUE;
}

int get_png_height(struct data_IHDR *buf){
	return buf->height;
}

int get_png_width(struct data_IHDR *buf){
	return buf->width;
}


int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence){
	
	if(fseek(fp, offset, whence) != 0){
		return EXIT_FAILURE;
	}
	
	U8 buf[DATA_IHDR_SIZE];
	size_t ret = fread(buf, sizeof(buf)/sizeof(*buf), sizeof(*buf), fp);

	if (ret != sizeof(*buf)) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}
	
	/* Copy the data from the buffer to out */
	memcpy(out, &buf, sizeof(struct data_IHDR));

	/* Width and Height are bin endian */
	out->width = htonl(out->width);
	out->height = htonl(out->height);

	return 0;
}
