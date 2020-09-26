#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "lab_png.h"

#include <errno.h>
#include <assert.h>

extern int errno;

int incr_height(int* height, char* filepath){
	/* Open */
	FILE* fp = fopen(filepath, "rb");
	if(!fp){
		fprintf(stderr, "fopen failed\n");
		exit(EXIT_FAILURE);
	}

	/* Get IHDR chunk */
	struct data_IHDR png_IHDR;
	get_png_data_IHDR(  &png_IHDR, 
			fp, 
			(size_t)(PNG_SIG_SIZE+CHUNK_LEN_SIZE+CHUNK_TYPE_SIZE),
			SEEK_CUR
			);

	/* Increment height */
	*height += get_png_height(&png_IHDR);

	/* Close */
	fclose(fp);

	return 0;
}

void file_seek(FILE* fp, size_t offset, int whence){
	if(fseek(fp, offset, whence) != 0){
		fprintf(stderr, "fseek failed\n");
		exit(EXIT_FAILURE);
	}
}

void read8(U8* buf, size_t size){
	for (int i = 0; i < size; i++)
	{
		if (i > 0) printf(" ");
		printf("%02X", buf[i]);
	}
	printf("\n");
}

void read32(U32* buf, size_t size){
	for (int i = 0; i < size; i++)
	{
		if (i > 0) printf(" ");
		printf("%02X", buf[i]);
	}
	printf("\n");
}



int create_simple_png(struct simple_PNG* png, char* filepath){
	/* Open */
	FILE* fp = fopen(filepath, "rb");
	if(!fp){
		fprintf(stderr, "fopen failed %d %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	size_t length;

	/* Fill in IHDR */
	png->p_IHDR = malloc(sizeof(struct chunk));
	file_seek(fp, (size_t)PNG_SIG_SIZE, SEEK_CUR);
	fread(&(png->p_IHDR->length), (size_t)CHUNK_LEN_SIZE, sizeof(U8), fp);
	fread(&(png->p_IHDR->type), (size_t)CHUNK_TYPE_SIZE, sizeof(U8), fp);

	length = htonl(png->p_IHDR->length);
	png->p_IHDR->p_data = malloc(sizeof(U8)*length);
	fread(png->p_IHDR->p_data, length, sizeof(U8), fp);
	fread(&(png->p_IHDR->crc), (size_t)CHUNK_CRC_SIZE, sizeof(U8), fp);
	
	/* Fill in IDAT */
	png->p_IDAT = malloc(sizeof(struct chunk));
	fread(&(png->p_IDAT->length), (size_t)CHUNK_LEN_SIZE, sizeof(U8), fp);
	fread(&(png->p_IDAT->type), (size_t)CHUNK_TYPE_SIZE, sizeof(U8), fp);

	length = htonl(png->p_IDAT->length);
	png->p_IDAT->p_data = malloc(sizeof(U8)*length);
	fread(png->p_IDAT->p_data, length, sizeof(U8), fp);
	/* printf("p_data Size: %lu", sizeof(*(png->p_IDAT->p_data))); */
	fread(&(png->p_IDAT->crc), (size_t)CHUNK_CRC_SIZE, sizeof(U8), fp);

	/* Fill in IEND */
	png->p_IEND = malloc(sizeof(struct chunk));
	fread(&(png->p_IEND->length), (size_t)CHUNK_LEN_SIZE, sizeof(U8), fp);
	fread(&(png->p_IEND->type), (size_t)CHUNK_TYPE_SIZE, sizeof(U8), fp);
	
	length = htonl(png->p_IEND->length);
	fread(&(png->p_IEND->crc), (size_t)CHUNK_CRC_SIZE, sizeof(U8), fp);
	read32(&(png->p_IEND->crc), (size_t)CHUNK_CRC_SIZE);

	/* Close */
	fclose(fp);
	return 0;
}

int main(int argc, char* argv[]){
	if(argc == 1){
		fprintf(stderr, "PNG image required");
		return EXIT_FAILURE;
	}

	int height=0;
	int i;
	for(i=1; i < argc; i++){
		/* get the total height */
		incr_height(&height, argv[i]);
	}

	struct simple_PNG png_list[argc-1];

	for(i=1; i < argc; i++){
		/* simple_PNG of all pics */
		create_simple_png(&png_list[i-1], argv[i]);
	}

	for(i=1; i < argc; i++){
		/* Get filtered chunk */
		/* Concat chunk vertically*/
	}

	/* Apply compression, generate new IDAT chunk */

	/* Write to ouput file */

	for(i=1; i < argc; i++){
		/* Free allocated memory */
		free(png_list[i-1].p_IDAT->p_data);
		free(png_list[i-1].p_IDAT);
		free(png_list[i-1].p_IHDR->p_data);
		free(png_list[i-1].p_IHDR);
		free(png_list[i-1].p_IEND);
	}

	return 0;
}
