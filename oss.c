/*
 * oss.c by Pascal Odijk 11/3/2021
 * P5 CS4760 Prof. Bhatia
 *
 * Entry point to the program.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oss.h"
#include "matrix.h"
#include "queue.h"
#include "config.h"

// Globals
FILE *filep;
static key_t key;
static struct Queue *queue;
static struct Clock forkClock;
static struct Clock *shmemClock = NULL;
static struct Data data;
static struct Message p_message;
static struct sembuf sem_operation;
static struct PCB *pcbt_shmemptr = NULL;
static int p_queueID = -1;
static int shmemClockID = -1;
static int semID = -1;
static int shmemPCBTID = -1;
static int forkNum = 0;
static pid_t pid = -1;
static unsigned char bitmap[MAX_PROCESSES];

// Main function
int main(int argc, char* argv[]) {
       //signal(SIGINT, signalHandler);
       //signal(SIGALRM, signalHandler);

        int opt;
	bool verbose = false; // Verbose mode -- default is false
        char *fileName = "outfile.log"; // Output filename

        // Check for opt args
        while((opt = getopt(argc, argv, "h")) != -1) {
                switch(opt) {
                        case 'h':
                                helpMessage();
                                exit(0);

                        default:
                                perror("oss: Error: Invalid argument");
                                helpMessage();
                                exit(1);
                }
        }

	printf("oss: Begining Program: Opening outfile and allocating shared memory/semaphores\n");
	
	// Generate random number between 1 and 2 inclusive
	// 1 = verbose mode is true, 2 = verbose mode is false
	srand(time(0));
	int n = (rand() % (2 - 1 + 1)) + 1;
	if(n == 1) {
	       	verbose = true;
		printf("oss: Verbose Mode: On\n");
	} else if (n == 2) { 
		verbose = false;
		printf("oss: Verbose Mode: Off\n");
	}

	// Open output file
        if((filep = fopen(fileName, "w")) == NULL) {
                perror("oss: Error: Failure opening file");
                exit(1);
        }

	// Init bitmap
	memset(bitmap, '\0', sizeof(bitmap));

	// Init message queue
	key = ftok("./oss.c", 1);
	p_queueID = msgget(key, IPC_CREAT | 0600);
	if(p_queueID < 0) {
		perror("oss: Error: Failed to allocate message queue");
		cleanUp();
		exit(1);
	}

	// Init shared memory (shmemTimer)
	key = ftok("./oss.c", 2);
	shmemClockID = shmget(key, sizeof(struct Clock), IPC_CREAT | 0600);
	if(shmemClockID < 0) {
		perror("oss: Error: Failed to allocate shared memory");
                cleanUp();
                exit(1);	
	}

	// Attach shared memory
	shmemClock = shmat(shmemClockID, NULL, 0);
	if(shmemClock == (void*)(-1)) {
		perror("oss: Error: Failed to attach shared memory");
                cleanUp();
                exit(1);
	}

	// Init attributes of shared memory clock
	shmemClock->sec = 0;
	shmemClock->ns = 0;
	forkClock.sec = 0;
	forkClock.ns = 0;

	// Init sempahore
	key = ftok("./oss.c", 3);
	semID = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
	if(semID == -1) {
		perror("oss: Error: Failed to allocate semaphore");
                cleanUp();
                exit(1);	
	}

	// Init our semaphore in our set
	semctl(semID, 0, SETVAL, 1);

	// Init PCB table
	key = ftok("./oss.c", 4);
	size_t PCBTSize = sizeof(struct PCB) * MAX_PROCESSES;
	shmemPCBTID = shmget(key, PCBTSize, IPC_CREAT | 0600);
	if(shmemPCBTID < 0) {
		perror("oss: Error: Failed to allocate PCB");
                cleanUp();
                exit(1);	
	}
	
	// Attach PCB to shared memory
	pcbt_shmemptr = shmat(shmemPCBTID, NULL, 0);
	if(pcbt_shmemptr == (void*)(-1)) {
		perror("oss: Error: Failed to attach PCB to shared memory");
                cleanUp();
                exit(1);	
	}
	
	// Init the pcb table
	initPCBT(pcbt_shmemptr);

	// Setup queue
	queue = createQueue();
	initResource(&data);
	displayResource(filep, data);
	
	// Invoke interrupt
	interrupt(MAX_TIME);
	
	// For multi-processing
	int last_index = -1;
	bool is_bitmap_open = false;
	while(1) {
		int spawn_nano = 100;
		if(forkClock.ns >= spawn_nano) {
			forkClock.ns = 0;
			is_bitmap_open = false;
			int count_process = 0;

			while(1) {
				last_index = (last_index + 1) % MAX_PROCESSES;
				uint32_t bit = bitmap[last_index / 8] & (1 << (last_index % 8));
				
				if(bit == 0) {
					is_bitmap_open = true;
					break;
				} else {
					is_bitmap_open = false;
				}

				if(count_process >= MAX_PROCESSES - 1) break;

				count_process++;
			}

			// If there is still space in bitmap -- continue to fork
			if(is_bitmap_open == true) {
				pid = fork();

				if(pid == -1) {
					perror("oss: Error: Fork error");
					finalize();
					cleanUp();
					exit(1);
				}

				if(pid == 0) { // In child
					signal(SIGUSR1, exitHandler);
					char exec_index[BUFFER_LENGTH];
					sprintf(exec_index, "%d", last_index);
					int exect_status = execl("./user", "./user", exec_index, NULL);
					
					if(exect_status == -1) {
						perror("oss: Error: execl");
					}
					
					finalize();
					cleanUp();
					exit(1);		
				} else { // In parent
					forkNum++;
					bitmap[last_index / 8] |= (1 << (last_index % 8));
					initPCB(&pcbt_shmemptr[last_index], last_index, pid, data);
					enqueue(queue, last_index);
					fprintf(filep, "OSS: Created process with PID %d [%d] and added to queue at time %d:%d\n", 
							pcbt_shmemptr[last_index].pidIndex, pcbt_shmemptr[last_index].pid, shmemClock->sec, shmemClock->ns);
					fflush(filep);
				}
			}
		}

		// -- Critical Section --
		incrementClock();

		struct Node t;
		struct Queue *trackingQueue = createQueue();
		int current_iteration = 0;
		t.next = queue->front;
		
		while(t.next != NULL) {
			incrementClock();

			int c_index = t.next->index;
			p_message.type = pcbt_shmemptr[c_index].pid;
			p_message.index = c_index;
			p_message.cpid = pcbt_shmemptr[c_index].pid;
			msgsnd(p_queueID, &p_message, (sizeof(struct Message) - sizeof(long)), 0);
			msgrcv(p_queueID, &p_message, (sizeof(struct Message) - sizeof(long)), 1, 0);
			incrementClock();

			// If child wants to terminate -- skip current iteration and move to next
			if(p_message.flag == 0) {
				fprintf(filep, "OSS: Process with PID %d [%d] has finished running at time %d:%d\n",
						p_message.index, p_message.cpid, shmemClock->sec, shmemClock->ns);
				fflush(filep);

				struct Node current;
				current.next = queue->front;
				while(current.next != NULL) {
					if(current.next->index != c_index) enqueue(trackingQueue, current.next->index);
					current.next = (current.next->next != NULL) ? current.next->next : NULL;
				}
				
				// Reassign current queue
				while(!isEmpty(queue)) {
					dequeue(queue);
				}

				while(!isEmpty(trackingQueue)) {
					int i = trackingQueue->front->index;
					enqueue(queue, i);
					dequeue(trackingQueue);
				}

				t.next  = queue->front;
				for(int i = 0 ; i < current_iteration ; i++) {
					t.next = (t.next->next != NULL) ? t.next->next : NULL;
				}

				continue;
			}
			
			// If a process is requesting a resource -- use the banker algorithm, if not -- add to tracking queue and point to next queue element
			if(p_message.isRequest == true) {
				fprintf(filep, "OSS: Process with PID %d [%d] is requesting resources at time %d:%d\n",
						 p_message.index, p_message.cpid, shmemClock->sec, shmemClock->ns);
				fflush(filep);

				bool isSafe = bankerAlg(filep, verbose, &data, pcbt_shmemptr, queue, c_index);

				// Send message to child regarding whether it is safe to proceed with the request or not
				p_message.type = pcbt_shmemptr[c_index].pid;
				p_message.isSafe = (isSafe) ? true : false;
				msgsnd(p_queueID, &p_message, (sizeof(struct Message) - sizeof(long)), 0);
			}

			incrementClock();

			// Check is process is releasing resources
			if(p_message.isRelease) {
				fprintf(filep, "OSS: Process with PID %d [%d] is releasing resources at time %d:%d\n",
                                                 p_message.index, p_message.cpid, shmemClock->sec, shmemClock->ns);
                                fflush(filep);
			}
			
			// Increase iteration number
			current_iteration++;
			t.next = (t.next->next != NULL) ? t.next->next : NULL;
		}

		free(trackingQueue);
		incrementClock();

		// -- End of Critical Section --
		
		int child_status = 0;
		pid_t child_pid = waitpid(-1, &child_status, WNOHANG);

		if(child_pid > 0) {
			int return_index = WEXITSTATUS(child_status);
			bitmap[return_index / 8] &= ~(1 << (return_index % 8));
		}

		if(forkNum >= TOTAL_PROCESSES) {
			timer(0);
			signalHandler(0);
		}
	}
	
	// Close output file and terminate
	fflush(filep);
	fclose(filep);

	printf("oss: Termination: Killed processes, closed outfile, and removed shared memory/semaphores\n");

	return 0;
}

// Handle ctrl-c and timer end
void signalHandler(int signal) {
        if(signal == SIGINT) {
                printf("oss: Terminate: CTRL+C encountered\n");
		fprintf(filep, "OSS: TERMINATE: CTRL+C encountered\n");
                fflush(stdout);
        } else if(signal == SIGALRM) {
                printf("oss: Terminate: Termination time has elapsed\n");
		fprintf(filep, "OSS: TERMINATE: Termination time has elapsed\n");
                fflush(stdout);
        }
	
	printf("oss: Stats: Final simulation time -- %d:%d\n", shmemClock->sec, shmemClock->ns);
	fprintf(filep, "OSS: STATS: Final simulation time -- %d:%d\n", shmemClock->sec, shmemClock->ns);

	finalize();
	cleanUp();

        // Close file and deallocate memory and queues
        fflush(filep);
        fclose(filep);

        printf("oss: Termination: Killed processes and removed shared memory/semaphores\n");
	exit(0);
}

void interrupt(int sec) {
	// Invoke timer
	timer(sec);

	// For SIGALRM
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_handler = &signalHandler;
	sa1.sa_flags = SA_RESTART;
	if(sigaction(SIGALRM, &sa1, NULL) == -1){
		perror("oss: ERROR: SIGALRM");
	}

	// For SIGINT
	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_handler = &signalHandler;
	sa2.sa_flags = SA_RESTART;
	if(sigaction(SIGINT, &sa2, NULL) == -1) {
		perror("oss: ERROR: SIGINT");
	}

	//For SIGUSR1
	signal(SIGUSR1, SIG_IGN);
}

void exitHandler(int signal) {
	printf("oss: Terminate: Successfully terminated\n");
	exit(0);
}

void finalize() {
	printf("oss: Error: Limitation has been reached -- starting termination process\n");
	kill(0, SIGUSR1);
	pid_t p = 0;

	while(p >= 0) {
		p = waitpid(-1, NULL, WNOHANG);
	}
}

// Remove/detach shared memory
void removeShmem(int shmid, void *shmaddr) {
	if(shmaddr != NULL) {
		if((shmdt(shmaddr)) << 0) {
			perror("oss: Error: Failed to detach shared memory");
		}
	}

	if(shmid > 0) {
		if((shmctl(shmid, IPC_RMID, NULL)) < 0) {
			perror("oss: Error: Failed to remove shared memory");
		}
	}
}

// Release shared memory, message queue, and semaphores
void cleanUp() {
	if(p_queueID > 0) msgctl(p_queueID, IPC_RMID, NULL);
	removeShmem(shmemClockID, shmemClock);
	if(semID > 0) semctl(semID, 0, IPC_RMID);
	removeShmem(shmemPCBTID, pcbt_shmemptr);
}

// Lock the given semaphore
void lockSem(int semIndx) {
	sem_operation.sem_num = semIndx;
	sem_operation.sem_op = -1;
	sem_operation.sem_flg = 0;
	semop(semID, &sem_operation, 1);
}

// Unlock the given semaphore
void releaseSem(int semIndx) {
	sem_operation.sem_num = semIndx;
	sem_operation.sem_op = 1;
	sem_operation.sem_flg = 0;
	semop(semID, &sem_operation, 1);
}

// Increment the shared memory clock
void incrementClock() {
	lockSem(0);
	int r_nano = rand() % 1000000 + 1;

	forkClock.ns += r_nano; 
	shmemClock->ns += r_nano;

	if(shmemClock->ns >= 1000000000) {
		shmemClock->sec++;
		shmemClock->ns = 1000000000 - shmemClock->ns;
	}

	releaseSem(0);
}

void timer(int sec) {
	struct itimerval value;
	value.it_value.tv_sec = sec;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;

	if(setitimer(ITIMER_REAL, &value, NULL) == -1) perror("oss: Error:");
}

// Init resources in data structure
void initResource(struct Data *data) {
	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		data->init_resource[i] = rand() % 10 + 1;
		data->resource[i] = data->init_resource[i];
	}

	data->sharedNum = (MAX_SHARED_RES == 0) ? 0 : rand() % (MAX_SHARED_RES - (MAX_SHARED_RES - MIN_SHARED_RES)) + MIN_SHARED_RES;
}

// Log data resources to outfile
void displayResource(FILE *filep, struct Data data) {
	fprintf(filep, "~~~~ Total Resource ~~~~\n<");
	
	for(int i = 0; i < MAX_RESOURCES; i++) {
		fprintf(filep, "%2d ", data.resource[i]);
	}

	fprintf(filep, ">\n");
	fprintf(filep, "OSS: Sharable Resources: %d\n", data.sharedNum);
	fflush(filep);
}

// Init the PCB table
void initPCBT(struct PCB *pcbt) {
	for(int i = 0 ; i < MAX_PROCESSES ; i++) {
		pcbt[i].pidIndex = -1;
		pcbt[i].pid = -1;
	}
}

// Init the PCB
void initPCB(struct PCB *pcb, int index, pid_t pid, struct Data data) {
	pcb->pidIndex = index;
	pcb->pid = pid;
	
	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		pcb->max[i] = rand() % (data.init_resource[i] + 1);
		pcb->alloc[i] = 0;
		pcb->request[i] = 0;
		pcb->release[i] = 0;
	}
}

// Prints help message
void helpMessage() {
	printf("---------- USAGE ----------\n");
        printf("./oss [-h]\n");
	printf("Invoke with no arguments\n");
        printf("-h\tDisplays usage message (optional)\n");
        printf("---------------------------\n");
}
