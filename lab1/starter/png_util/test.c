#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "lab_png.h"


void read(U8* buf, size_t size){
	for (int i = 0; i < size; i++)
	{
		if (i > 0) printf(":");
		printf("%02X", buf[i]);
	}
	printf("\n");
}

/* TODO : is byteorder important if we are not using network communication? */
int readpng(char* filepath){
	/* Open the binary file */
	FILE* fp = fopen(filepath, "rb");
	if(!fp){
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	/* Create a buffer to read the head of the file */
	U8 buf[PNG_SIG_SIZE];
	size_t ret = fread(buf, sizeof(buf)/sizeof(*buf), sizeof(*buf), fp);
	if (ret != sizeof(*buf)) {
		fprintf(stderr, "fread() failed: %zu\n", ret);
		exit(EXIT_FAILURE);
	}
	
	/* Print the buffer */
	/* read(buf, PNG_SIG_SIZE); */

	/* Ensure the file is a png file */
	if(!is_png(buf, PNG_SIG_SIZE)){
		fprintf(stderr, "File is not a png\n");
		exit(EXIT_FAILURE);
	}

	/* Read File chunks */
	/* get/make prototyped functions for each of the chunks*/
	return 0;
}


int main(int argc, char* argv[]){
	if(argc == 1){
		fprintf(stderr, "PNG Image location required\n");
		return EXIT_FAILURE;
	}
	for(int i=1; i < argc; i++){
		readpng(argv[i]);
	}

	return 0;
}
