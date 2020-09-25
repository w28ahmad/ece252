/**
 * @brief Fill in Function Prototypes
 * is_png
 * get_png_height
 * get_png_width
 * get_png_data_IHDR
 **/
#include "lab_png.h"
#include <string.h> /* For memcmpy */

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

	return 0;
}

int get_png_width(struct data_IHDR *buf){



	return 0;
}


int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence){

	return 0;
}
