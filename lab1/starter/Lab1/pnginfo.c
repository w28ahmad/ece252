#define _GNU_SOURCE

#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "lab_png.h"  /* simple PNG data structures  */
#include <string.h>
#include <arpa/inet.h>  /* for ntohl */
#include "crc.h"


int is_png(U8 *buf, size_t n) {
    U8 signature[] = {0x89, 0x50, 0x4E, 0X47, 0X0D, 0X0A, 0X1A, 0X0A};  /* first 8 bytes of a PNG in decimal */

    if (memcmp(signature, buf, n)) {  /*Compare the read bytes with the PNG identifier */
        return 0;
    }

    return 1;
}


int main (int argc, char **argv) {

    if (argc != 2) {
        printf("invalid args. Format should be: pnginfo <filename>\n");
        return -1;
    }

    FILE* pic;
    U8 pngCode[8];
    U32 wh[6];
    U8 spare[4];
    U32 IDAT_len;
    U32 IDAT_crc;
    char* name = basename(argv[1]);


    pic = fopen(argv[1], "r");

    fread(pngCode, sizeof(U8), 8, pic); /* Read the first 8 bytes of the file into pngCode */
    fread(wh, sizeof(U32), 6, pic);
    fread(spare, sizeof(U8), 1, pic);


    for (int i = 0; i < 6; i++){
    	wh[i] = ntohl(wh[i]);
    }


    int i = is_png(pngCode, 8);


	
    if (i == 0) {   /* the first 8 bytes of the file did not match a PNG */
        printf("%s: Not a PNG file\n", name);
    } else if (i == 1) {    /* The file is a PNG */
        printf("%s: %d x %d\n", name, wh[2], wh[3]);

        fread(&IDAT_len, sizeof(U32), 1, pic);
        fread(spare, sizeof(U8), 4, pic);
        int length = ntohl(IDAT_len);
        U8 IDAT_data[length];
        fread(IDAT_data, sizeof(U8), length, pic);
        fread(&IDAT_crc, sizeof(U32), 1, pic);

        IDAT_crc = ntohl(IDAT_crc);

        U8 *IDAT_crc2 = malloc((4 + length)*sizeof(U8));
        memcpy(IDAT_crc2, spare, 4*sizeof(U8));
        memcpy(IDAT_crc2 + 4, IDAT_data, length*sizeof(U8));


        U32 crc_val = crc(IDAT_crc2, 4 + length);

	if (crc_val != IDAT_crc) {
            printf("IDAT chunk CRC error: computed %0x, expected %0x\n", crc_val, IDAT_crc);
	}
    }

    fclose(pic);
    return 0;

}
