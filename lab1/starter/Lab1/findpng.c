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

int print_png(const char *path, int counter) {

	int pngCounter = counter;
	char newPath[1000];
	int pathlen = strlen(path);
	U8 pngCode[8];
	FILE* fp;
	int png_check;
	struct dirent *dp;
	DIR *dir = opendir(path);


	if (dir == NULL) {
		return 0;
	}

	


	while ((dp = readdir(dir)) != NULL) {
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
			int len = strlen(dp->d_name);
			
			strcpy(newPath, path);
			if (path[pathlen - 1] != '/') {
				strcat(newPath, "/");
			}
			strcat(newPath, dp->d_name);


			char *ptr;
			struct stat buf;

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
	        if (strcmp(ptr, "symbolic link") != 0) {



			if (dp->d_name[len-4] == '.' && dp->d_name[len-3] == 'p' && dp->d_name[len-2] == 'n'  && dp->d_name[len-1] == 'g') {
				fp = fopen(newPath, "r");
				fread(pngCode, sizeof(U8), 8, fp);
				png_check = is_png(pngCode, 8);
				if (png_check == 1) {
					pngCounter++;
					printf("%s\n", newPath);
				}
				fclose(fp);
			} else {
				pngCounter += print_png(newPath, pngCounter);

			}
		}
	}
	}
	closedir(dir);
	return pngCounter;
}



int main (int argc, char **argv) {


    if (argc != 2) {
        printf("invalid args. Format should be: findpng <directory>\n");
        return -1;
    }

    int i = print_png(argv[1], 0);

    if (i == 0) {
	printf("findpng: No PNG file found\n");
    }

    return 0;

}
