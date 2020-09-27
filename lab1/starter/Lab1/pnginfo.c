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

	/* invalid args */
    if (argc != 2) {
        printf("invalid args. Usage: pnginfo <filename>\n");
        return EXIT_FAILURE;
    }

    FILE* pic;			/* FILE for PNG */
    U8 pngCode[8];		/* to hold 8 byte identifier from given file */
    U32 wh[5];			/* to hold IHDR data for width and height */
    int width = 0;
    int height = 0;
    U8 data[4];			/* to hold data from the PNG */	
    U32 IDAT_len;		/* length of IDAT raw data */
    U32 IDAT_crc;		/* IDAT crc extracted from given file */
    U32 IHDR_crc;		/* IHDR crc extracted from given file */
    U8 *IHDR_crc2 = malloc(13*sizeof(U8));	/* calculated IHDR crc */
    U32 IEND_crc;		/* IEND crc extracted from given file */
    U8 *IEND_crc2 = malloc(4*sizeof(U8));	/* calculated IEND crc */
    char* name = basename(argv[1]);		/* filename of the PNG */


    pic = fopen(argv[1], "r");
    if (!pic) {
    	fprintf(stderr, "fopen failed %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    fread(pngCode, sizeof(U8), 8, pic); /* Read the first 8 bytes of the file into pngCode */

    int i = is_png(pngCode, 8);	/* check if the file's PNG code is correct */
	
    if (i == 0) {   /* the first 8 bytes of the file did not match a PNG */
        printf("%s: Not a PNG file\n", name);
    } else if (i == 1) {    /* The file is a PNG */

	fread(wh, sizeof(U32), 1, pic);	/* length of IHDR data = 13 */
	fread(wh, sizeof(U32), 4, pic);	/* read IHDR data */
	width = ntohl(wh[1]);	/* use ntohl to get width and height */
	height = ntohl(wh[2]);

        printf("%s: %d x %d\n", name, width, height);	/* print dimensions */

        fread(data, sizeof(U8), 1, pic);	/* read remaining IHDR data */
        fread(&IHDR_crc, sizeof(U32), 1, pic);	/* read the IHDR crc from the file */
        IHDR_crc = ntohl(IHDR_crc);		/* convert crc value */

        memcpy(IHDR_crc2, wh, 4*sizeof(U32));	/* copy data into IHDR_crc2 */
        memcpy(IHDR_crc2 + 16, data, sizeof(U8));

        U32 crc_val = crc(IHDR_crc2, 17);	/* use crc() and store the value in crc_val */
        if (crc_val != IHDR_crc) {		/* check if crc values match. print error if they don't */
            printf("IHDR chunk CRC error: computed %0x, expected %0x\n", crc_val, IHDR_crc);
        } else {	/* check IDAT crc value */

            fread(&IDAT_len, sizeof(U32), 1, pic);	/* read IDAT length */
            fread(data, sizeof(U8), 4, pic);		/* read IDAT type */
            int length = ntohl(IDAT_len);		/* get value of length */
            U8 IDAT_data[length];			/* Initialize IDAT_data to hold all the data */
            fread(IDAT_data, sizeof(U8), length, pic);	/* read the data */
            fread(&IDAT_crc, sizeof(U32), 1, pic);	/* read the IDAT crc from the file */

            IDAT_crc = ntohl(IDAT_crc);			/* convert crc value */

            U8 *IDAT_crc2 = malloc((4 + length)*sizeof(U8));	/* calculated IDAT crc */
            memcpy(IDAT_crc2, data, 4*sizeof(U8));		/* copy type and data into IDAT_crc2 */
            memcpy(IDAT_crc2 + 4, IDAT_data, length*sizeof(U8));


            crc_val = crc(IDAT_crc2, 4 + length);	/* use crc() and store the value in crc_val */

            if (crc_val != IDAT_crc) {		/* check if crc values match */
                    printf("IDAT chunk CRC error: computed %0x, expected %0x\n", crc_val, IDAT_crc);
            } else {
		fread(data, sizeof(U8), 4, pic);	/* IEND length = 0 */
		fread(data, sizeof(U8), 4, pic);	/* read IEND type */
		fread(&IEND_crc, sizeof(U8), 4, pic);	/* read IEND crc value */
		IEND_crc = ntohl(IEND_crc);		/* convert crc value */
		memcpy(IEND_crc2, data, 4*sizeof(U8));	/* copy type into IEND_crc2 */
		crc_val = crc(IEND_crc2, 4);		/* use crc() and store the value in crc_val */
		if (crc_val != IEND_crc) {		/* check if crc values match */
			printf("IEND chunk CRC error: computed %0x, expected %0x\n", crc_val, IEND_crc);
		}
	    }
	    free(IDAT_crc2);
        }
    }

    fclose(pic);
    free(IHDR_crc2);
    free(IEND_crc2);
    return 0;

}
