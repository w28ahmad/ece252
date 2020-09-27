/* look at ls starter for help */
#define _GNU_SOURCE

#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "lab_png.h"  /* simple PNG data structures  */
#include <string.h>
#include <arpa/inet.h>  /* for ntohl */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


int is_png(U8 *buf, size_t n) {
    U8 signature[] = {0x89, 0x50, 0x4E, 0X47, 0X0D, 0X0A, 0X1A, 0X0A};  /* first 8 bytes of a PNG in decimal */

    if (memcmp(signature, buf, n)) {  /*Compare the read bytes with the PNG identifier */
        return 0;
    }

    return 1;
}

int printPng(const char *path, int counter) {

	int pngCounter = counter;		/* number of PNG files found */
	char newPath[1000];			/* string to hold the next filepath */
	int pathlen = strlen(path);		/* length of given string */
	U8 pngCode[8];				/* 8 byte PNG identifier */
	FILE* pic;				/* PNG file */
	int pngCheck;				/* return value from is_png - determines whether a file is a PNG */
	struct dirent *pd;			/* p_dirent */
	DIR *p_dir = opendir(path);		/* opened directory at given path */


	if (p_dir == NULL) {			/* directory was not valid - return 0 */
		return 0;
	}

	while ((pd = readdir(p_dir)) != NULL) {	/* observe each item in the directory */
		if (strcmp(pd->d_name, ".") != 0 && strcmp(pd->d_name, "..") != 0) {	/* the item is not . or .. */
			int len = strlen(pd->d_name);	/* get the length of the current file's name */
			
			strcpy(newPath, path);		/* determine the filepath of this file */
			if (path[pathlen - 1] != '/') {	/* add a / to separate the directory and the current file */
				strcat(newPath, "/");	/* if it is not already there */
			}
			strcat(newPath, pd->d_name);


			char *ptr;			/* check the file type */
			struct stat buf;		/* code taken from ls_ftype.c */

		        if (lstat(newPath, &buf) < 0) {
				perror("lstat error");
		        }   

		        if      (S_ISREG(buf.st_mode))  ptr = "regular";
		        else if (S_ISDIR(buf.st_mode))  ptr = "directory";
		        else if (S_ISCHR(buf.st_mode))  ptr = "character special";
		        else if (S_ISBLK(buf.st_mode))  ptr = "block special";
		        else if (S_ISFIFO(buf.st_mode)) ptr = "fifo";
		#ifdef S_ISLNK
		        else if (S_ISLNK(buf.st_mode))  ptr = "symbolic link";
		#endif
		#ifdef S_ISSOCK
		        else if (S_ISSOCK(buf.st_mode)) ptr = "socket";
		#endif
        	else                            ptr = "**unknown mode**";
	        if (strcmp(ptr, "symbolic link") != 0) {	/* if the file is not a symbolic link, proceed */



			if (pd->d_name[len-4] == '.' && pd->d_name[len-3] == 'p' && pd->d_name[len-2] == 'n'  && pd->d_name[len-1] == 'g') {	/* the file is a .png */
				pic = fopen(newPath, "r");		/* open the file */
				if (!pic){
					fprintf(stderr, "fopen failed %d %s\n", errno, strerror(errno));
					exit(EXIT_FAILURE);
				}
				fread(pngCode, sizeof(U8), 8, pic);	/* read the 8 byte identifier */
				pngCheck = is_png(pngCode, 8);		/* compare with known PNG identifier */
				if (pngCheck == 1) {			/* if it is a valid PNG, print the filepath */
					pngCounter++;			/* increment pngCounter */
					printf("%s\n", newPath);
				}
				fclose(pic);
			} else {
				pngCounter += printPng(newPath, pngCounter);	/* call pngPrint with newPath */
			}
		}
	}
	}
	closedir(p_dir);
	return pngCounter;
}



int main (int argc, char **argv) {


    if (argc != 2) {			/* invalid args */
        printf("invalid args. Format should be: findpng <directory>\n");
        return -1;
    }

    int i = printPng(argv[1], 0);	/* print all PNG files in given directory */

    if (i == 0) {
	printf("findpng: No PNG file found\n");	/* No valid PNG files were found */
    }

    return 0;

}
