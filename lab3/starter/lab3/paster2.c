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
#include <time.h>		/* rand */
#include <sys/time.h>
#include "utils.h"	
#include "utils.c"		/* starter code */
#include "shm_stack.h"
#include "shm_stack.c"  /* Implementation of a stack */

/* @brief produce the images in slices according to a range set by start and end
 * @param imp_num - which image to produce
 * @param start, end - range of image slices, (given in equal intervals to each producer)
 * @param pstack - Shared stack of size B, SLICE(data, seq, size) are pushed by producers
 * @param items - items semaphore, number of items on the stack
 * @param spaces - spaces semaphore, number of spaces on the stack
 * @param mutex	- TODO remove if not needed
 **/
int produce(int img_num, int start, int end, struct buf_stack* pstack, sem_t* items, sem_t* spaces, sem_t* mutex){
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
				int server_type = 1+(rand()%3); /* random number from 1 to 3 */
				sprintf(url, "http://ece252-%d.uwaterloo.ca:2530/image?img=%d&part=%d", 
								server_type, img_num, i);

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
/*						printf("%lu bytes produced in memory %p, seq=%d for request %d.\n", \
										p_shm_recv_buf->size, \
										p_shm_recv_buf->buf, \
										p_shm_recv_buf->seq, \
										i);	*/
				}

				/* Critical section */
				if(sem_wait(spaces) != 0){
						perror("sem_wait on spaces");
						abort();
				}

				/* add data to stack */
				sem_wait(mutex);
				int ret = push(pstack, p_shm_recv_buf->seq, p_shm_recv_buf->buf, p_shm_recv_buf->size);
				if ( ret != 0 ) {
						perror("push");
						return 1;
				}
				sem_post(mutex);
				if(sem_post(items) != 0){
						perror("sem_post on items");
						abort();
				}
				free(p_shm_recv_buf);
		}
		/* cleaning up */
		curl_easy_cleanup(curl_handle);

		free(url);
		return 0;
}


/* @brief consume the image slices on the stack
 * @param items_to_consume - number of slices to consume, equally spread to the C consumers
 * @param pstack - Shared stack of size B, SLICE(data, seq, size) are pushed by producers
 * @param items - items semaphore, number of items on the stack
 * @param spaces - spaces semaphore, number of spaces on the stack
 * @param mutex	- TODO remove if not needed
 **/
int consume(int sleep_delay, int items_to_consume, struct buf_stack* pstack, sem_t* items, sem_t* spaces, sem_t* mutex){
		int i=0;
		//int seq_images[NUM_SLICES] = { 0 };
		char fname[256];
		while(i < items_to_consume){	
				usleep(sleep_delay*1000); /* Micro to mili */
				/* Critical Section */
				sem_wait(items);

				/* Read the stack */
				sem_wait(mutex);
				SLICE data;
				int ret = pop(pstack, &data);
				if(ret != 0){
						perror("pop");
						return 1;
				}
				sem_post(mutex);
				/* data holds information recieved by the producers
				 * data.seq- seq number, data.buf- png data, data.size- size of the data
				 */
				/* Create a file of the slice */
				sprintf(fname, "./output_%d.png", data.seq);
				write_file(fname, data.buf, (int)data.size);
				sem_post(spaces);
				i++;
		}
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

		/* Start the timer */
		double times[2];
		struct timeval tv;

		if (gettimeofday(&tv, NULL) != 0) {
				perror("gettimeofday");
				abort();
		}
		times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;


		srand(time(NULL)); /* For rand() */

		/* Shared memory for stack */
		int shmid;
		int shm_size = sizeof_shm_stack(B);

		/* allocate shared memory for semaphore and stack */
		sem_t* sems;
		int shmid_sems = shmget(IPC_PRIVATE, 
						sizeof(sem_t) * NUM_SEMS, 
						IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

		if (shmid_sems == -1) {
				perror("shmget");
				abort();
		}
		shmid = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		if ( shmid == -1 ) {
				perror("shmget");
				abort();
		}


		/* attach shared memory for semaphore and stack */
		sems = (sem_t*)shmat(shmid_sems, NULL, 0);
		if ( sems == (void *) -1 ) {
				perror("shmat");
				abort();
		}
		struct buf_stack* pstack;
		pstack = (struct buf_stack*)shmat(shmid, NULL, 0);

		/* initialize shared memory varaibles */
		init_shm_stack(pstack, B);

		/* Items semaphore */
		if ( sem_init(&sems[0], SEM_PROC, 0) != 0 ) {
				perror("sem_init(sem[0])");
				abort();
		}
		/* Spaces semaphore */
		if ( sem_init(&sems[1], SEM_PROC, B) != 0 ) {
				perror("sem_init(sem[1])");
				abort();
		}
		/* Mutex semaphore */
		/* TODO: not sure if I will need mutex, If not remove */
		if ( sem_init(&sems[2], SEM_PROC, 1) != 0 ) {
				perror("sem_init(sem[2])");
				abort();
		}


		/* Create P producers */
		pid_t p_pid[P];
		int slice=(int)ceil((double)NUM_SLICES/P);
		curl_global_init(CURL_GLOBAL_DEFAULT);
		for(int i=0; i < P; i++){
				if ((p_pid[i] = fork()) < 0) {
						perror("fork");
						abort();
				}else if(p_pid[i] == 0){
						produce(N,						/* Image number */ 
										i*slice, 				/* Starting slice of image */
										min((i+1)*slice, 50), 	/* Ending slice of image */
										pstack, 				/* Shared stack between prod. and cons. */
										&sems[0], 				/* Items sem */
										&sems[1], 				/* Space sem */
										&sems[2]				/* Mutex sem */
							   );

						/* detach the semaphore memory */
						if ( shmdt(sems) != 0 ) {
								perror("shmdt");
								abort();
						}
						/* detach stack memory */
						if ( shmdt(pstack) != 0 ) {
								perror("shmdt");
								abort();
						}
						exit(0);
				}		
		}

		/* Create C consumers */
		slice = (int)ceil((double)NUM_SLICES/C);
		pid_t c_pid[C];
		for(int i=0; i < C; i++){
				if ((c_pid[i] = fork()) < 0) {
						perror("fork");
						abort();
				}else if(c_pid[i] == 0){
						/* Consume stuff here */
						int items_to_consume = min((i+1)*slice, 50) - i*slice;
						consume(X,
										items_to_consume,	/* How many items to consume per consumer*/
										pstack, 
										&sems[0],
										&sems[1],
										&sems[2]
							   );
						/* detach the semaphore memory */
						if ( shmdt(sems) != 0 ) {
								perror("shmdt");
								abort();
						}
						/* detach stack memory */
						if ( shmdt(pstack) != 0 ) {
								perror("shmdt");
								abort();
						}
						exit(0);
				}		
		}

		/* Parent */
		int status=0;
		while( (waitpid(-1, &status, 0)) > 0);	/* wait for all children */

		/* Merge the images into an all.png file */

		/* Cleanup */
		curl_global_cleanup();

		/* detach stack memory */
		if ( shmdt(pstack) != 0 ) {
				perror("shmdt");
				abort();
		}

		/* Remove stack memory */
		if ( shmctl(shmid, IPC_RMID, NULL) == -1 ) {
				perror("shmctl");
				abort();
		}

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

		/* Destroy Semaphores */
		if (sem_destroy(&sems[0]) || sem_destroy(&sems[1]) || sem_destroy(&sems[2])) {
				perror("sem_destroy");
				abort();
		}

		/* Concat images using the previously built catpng program */
		
		size_t size = sizeof("output_?.png ");	
		char param[NUM_SLICES * size];
		int pos = 0;
		for(int i=0; i < NUM_SLICES; i++){
			pos += sprintf(&param[pos], "output_%d.png ", i);
		}
		
		char* cmd = concat("./catpng ", param);
		system(cmd);
		free(cmd);

		/* Remove png slice files */
		char filename[64];
		for (int i = 0; i < NUM_SLICES; i++) {
			sprintf(filename, "output_%d.png", i);
			if (remove(filename) != 0) {
				perror("remove");
				abort();
			}
		}


		/* Stop the timer */
		if (gettimeofday(&tv, NULL) != 0) {
				perror("gettimeofday");
				abort();
		}
		times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
		printf("paster2 execution time: %.2lf seconds\n", times[1] - times[0]);

		return 0;
}	
