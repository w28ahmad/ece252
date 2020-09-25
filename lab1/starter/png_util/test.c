#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "lab_png.h"


void read8(U8* buf, size_t size){
	for (int i = 0; i < size; i++)
	{
		if (i > 0) printf(":");
		printf("%02X", buf[i]);
	}
	printf("\n");
}


void read32(U32* buf, size_t size){
	for (int i = 0; i < size; i++)
	{
		if (i > 0) printf(":");
		printf("%02X", buf[i]);
	}
	printf("\n");
}
/* TODO REMOVE THIS FUNCTION */
void view_IHDR(struct data_IHDR png_IHDR){
	printf("Width: %d\n", png_IHDR.width);
	printf("Height: %d\n", png_IHDR.height);
	read8(&png_IHDR.bit_depth, sizeof(png_IHDR.bit_depth));
	read8(&png_IHDR.color_type, sizeof(png_IHDR.color_type));
	read8(&png_IHDR.compression, sizeof(png_IHDR.compression));
	read8(&png_IHDR.filter, sizeof(png_IHDR.filter));
	read8(&png_IHDR.interlace, sizeof(png_IHDR.interlace));
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
	
	/* TODO Remove - Print the buffer */
	/* read8(buf, PNG_SIG_SIZE); */

	/* Ensure the file is a png file */
	if(!is_png(buf, PNG_SIG_SIZE)){
		fprintf(stderr, "File is not a png\n");
		exit(EXIT_FAILURE);
	}

	/* Get IHDR chunk */
	struct data_IHDR png_IHDR;
	get_png_data_IHDR(&png_IHDR, fp, (size_t)8, SEEK_CUR);
	/* view_IHDR(png_IHDR); TODO REMOVE*/

	/* Get IDAT chuck */
	

	fclose(fp);
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
