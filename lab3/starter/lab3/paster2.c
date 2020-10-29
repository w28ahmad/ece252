#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		/* fork */
#include <sys/wait.h>	/* waidpid */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>		/* memset */
#include <math.h>		/* ceil */
#include <curl/curl.h>
#include "utils.h"	
#include "utils.c"		/* starter code */

#define SIZE_OF_SLICE 10000

/* @param start, end - range of slices to curl */
int produce(int img_num, int start, int end){
		char* url = (char*)malloc(50*sizeof(char));

		/* init a curl session */
		CURL *curl_handle;
		CURLcode res;
		curl_handle = curl_easy_init();
		if (curl_handle == NULL) {
				fprintf(stderr, "curl_easy_init: returned NULL\n");
				return 1;
		}
		for(int i=start; i < end; i++){
				/* Generate url */
				memset(url, 0, 50*sizeof(char));
				sprintf(url, "http://ece252-1.uwaterloo.ca:2520/image?img=%d&part=%d", img_num, i);
				
				/* Data structure to store the slices */
				int shm_size = sizeof_shm_recv_buf(SIZE_OF_SLICE);
				RECV_BUF* p_shm_recv_buf=(RECV_BUF*)malloc(shm_size);
				shm_recv_buf_init(p_shm_recv_buf, shm_size);

				/* Create the curl request */
				/* specify URL to get */
				curl_easy_setopt(curl_handle, CURLOPT_URL, url);
				/* register write call back function to process received data */
				curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 
				/* user defined data structure passed to the call back function */
				curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_shm_recv_buf);
				/* register header call back function to process received header data */
				curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
				/* user defined data structure passed to the call back function */
				curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_shm_recv_buf);
				/* some servers requires a user-agent field */
				curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

				/* get it! */
				res = curl_easy_perform(curl_handle);

				if( res != CURLE_OK) {
						fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				} else {
						printf("%lu bytes received in memory %p, seq=%d.\n", \
										p_shm_recv_buf->size, \
										p_shm_recv_buf->buf, \
										p_shm_recv_buf->seq);
				}

				free(p_shm_recv_buf);
		}
		/* cleaning up */
		curl_easy_cleanup(curl_handle);

		free(url);
		return 0;
}

int main(int argc, char** argv){
		if(argc != 6){
				printf("The program needs 5 parameters\n");
				return 0;
		}
		/* Get parameters */
		int B=strtoul(argv[1], NULL, 10);	/* Buffer size */
		int P=strtoul(argv[2], NULL, 10);	/* # of producers */
		int C=strtoul(argv[3], NULL, 10);	/* # of consumers */
		int X=strtoul(argv[4], NULL, 10);	/* # of ms sleeps before it starts (FOR CONSUMER)*/
		int N=strtoul(argv[5], NULL, 10);	/* Image number, must be from 1-3 */

		//RECV_BUF* p_shm_recv_buf;
		//int shm_size = sizeof_shm_recv_buf(B);
		//int shmid;

		/* allocate shared memory for semaphore and RECV_BUF */
		sem_t* sems;
		int shmid_sems = shmget(IPC_PRIVATE, 
						sizeof(sem_t) * NUM_SEMS, 
						IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

		if (shmid_sems == -1) {
				perror("shmget");
				abort();
		}
		//shmid = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		//if ( shmid == -1 ) {
		//	perror("shmget");
		//	abort();
		//}


		/* attach shared memory for semaphorei and RECV_BUF */
		sems = (sem_t*)shmat(shmid_sems, NULL, 0);
		if ( sems == (void *) -1 ) {
				perror("shmat");
				abort();
		}
		//p_shm_recv_buf = (RECV_BUF*)shmat(shmid, NULL, 0);

		/* initialize shared memory varaibles */
		if ( sem_init(&sems[0], SEM_PROC, 1) != 0 ) {
				perror("sem_init(sem[0])");
				abort();
		}
		if ( sem_init(&sems[1], SEM_PROC, 1) != 0 ) {
				perror("sem_init(sem[1])");
				abort();
		}
		//shm_recv_buf_init(p_shm_recv_buf, B);

		/* Create P producers */
		pid_t p_pid[P];
		int slice=(int)ceil((double)NUM_SLICES/P);
		curl_global_init(CURL_GLOBAL_DEFAULT);
		for(int i=0; i < P; i++){
				if ((p_pid[i] = fork()) < 0) {
						perror("fork");
						abort();
				}else if(p_pid[i] == 0){
						produce(N, i*slice, min((i+1)*slice, 50));

						/* detach the semaphore memory */
						if ( shmdt(sems) != 0 ) {
								perror("shmdt");
								abort();
						}
						exit(0);
				}		
		}

		/* Create C consumers */
		pid_t c_pid[C];
		for(int i=0; i < C; i++){
				if ((c_pid[i] = fork()) < 0) {
						perror("fork");
						abort();
				}else if(c_pid[i] == 0){
						/* Consume stuff here */
						printf("I am a C\n");

						/* detach the semaphore memory */
						if ( shmdt(sems) != 0 ) {
								perror("shmdt");
								abort();
						}
						exit(0);
				}		
		}

		/* Parent */
		int status=0;
		while( (waitpid(-1, &status, 0)) > 0);	/* wait for all children */
		printf("I am parent\n");

		/* Cleanup */
		curl_global_cleanup();

		/* detach the semaphore memory */
		if ( shmdt(sems) != 0 ) {
				perror("shmdt");
				abort();
		}

		/* Remove shared memory */
		if ( shmctl(shmid_sems, IPC_RMID, NULL) == -1 ) {
				perror("shmctl");
				abort();
		}

		/* Destroy Semaphore */
		if (sem_destroy(&sems[0]) || sem_destroy(&sems[1])) {
				perror("sem_destroy");
				abort();
		}

		return 0;
}	
