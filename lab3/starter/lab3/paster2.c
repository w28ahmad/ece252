#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		/* fork */
#include <sys/wait.h>	/* waidpid */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "utils.h"		/* starter code */

#define NUM_SEMS 2
#define SEM_PROC 1		/* PSHARED for sem_init */

int produce(char* url, int* slices){
	printf("%s \n", url);	

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
	int X=strtoul(argv[4], NULL, 10);	/* # of milliseconds sleeps before it starts */
	int N=strtoul(argv[5], NULL, 10);	/* Image number */

	/* Create url */
	char* url;
	switch(N){
		case 1:
			url = (char*)IMG_URL_1;
			break;
		case 2:
			url = (char*)IMG_URL_2;
			break;
		case 3:
			url = (char*)IMG_URL_3;
			break;
		default:
			return -1;
	}
	
	/* allocate shared memory for semaphore */
	sem_t *sems;
	int shmid_sems = shmget(IPC_PRIVATE, 
							sizeof(sem_t) * NUM_SEMS, 
							IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    
	if (shmid_sems == -1) {
		perror("shmget");
		abort();
    }
	
	/* attach shared memory for semaphore */
	sems = (sem_t*)shmat(shmid_sems, NULL, 0);
	if ( sems == (void *) -1 ) {
		perror("shmat");
		abort();
	}

	/* initialize shared memory varaibles */
	if ( sem_init(&sems[0], SEM_PROC, 1) != 0 ) {
		perror("sem_init(sem[0])");
		abort();
	}
	
	int img_slices = 0;
	/* Create P producers */
	pid_t p_pid[P];
	for(int i=0; i < P; i++){
		if ((p_pid[i] = fork()) < 0) {
			perror("fork");
			abort();
		}else if(p_pid[i] == 0){
			printf("I am a P\n");
			produce(url, &img_slices);

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
