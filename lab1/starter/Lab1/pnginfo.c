#define _GNU_SOURCE

#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "lab_png.h"  /* simple PNG data structures  */
#include <string.h>

int is_png(U8 *buf, size_t n) {
    U8 signature[] = {0x89, 0x50, 0x4E, 0X47, 0X0D, 0X0A, 0X1A, 0X0A};  /* first 8 bytes of a PNG in decimal */

    if (memcmp(signature, buf, n)) {  /*Compare the read bytes with the PNG identifier */
        return 0;
    }

    return 1;
}


int main (int argc, char **argv) {

    if (argc != 2) {
        printf("invalid args.\n");
        return -1;
    }

    FILE* pic;
    U8 pngCode[8];
    char* name = basename(argv[1]);


    pic = fopen(argv[1], "r");

    fread(pngCode, sizeof(U8), 8, pic); /* Read the first 8 bytes of the file into pngCode */

    fclose(pic);

    int i = is_png(pngCode, 8);
	
    if (i == 0) {   /* the first 8 bytes of the file did not match a PNG */
        printf("%s: Not a PNG file\n", name);
    } else if (i == 1) {    /* The file is a PNG */
        printf("%s: %d x %d\n", name, i, i);
    } else if (i == 2) {    /* The file is a PNG with CRC errors */

    }


    return 0;

}
