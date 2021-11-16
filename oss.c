/*
 * oss.c by Pascal Odijk 11/12/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This file is the entry point of the program.
 * Banker's algorithm is used for deadlock avoidance, it is modified from: https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/
 */

#include <stdio.h>
#include <stdlib.h>
#include "oss.h"

// Globals
FILE *fp;
bool verbose = false;
const int maxAmountOfEachResource = 4;
int requestResource, grantRequest, markerChk, releaseResource, createdProcs, terminatedProcs;
int percentage = 29;
int gr = 39;

// Main
int main(int argc, char *argv[]) {
	int opt;

	// Check for opt args
        while((opt = getopt(argc, argv, "hv")) != -1) {
                switch(opt) {
                        case 'h':
                                helpMessage();
                                exit(0);
			case 'v':
				printf("oss: Log: Verbose mode active\n");
				verbose  = true;
				break;
                        default:
                                perror("oss: Error: Invalid argument");
                                helpMessage();
                                exit(1);
                }
        }	

        char logName[20] = "outfile.log"; // outfile for logging
        fp = fopen(logName, "w+");
        srand(time(NULL));

        createdProcs = 0;
        int myPid = getpid();
        int numberOfLines = 0;
        const int killTimer = 2;          // Program runs for 2 seconds max
        alarm(killTimer);                 // Sets the timer alarm based on value of killTimer

        int arrayIndex, tempIndex, arrayRequest, tempRelease, tempHolder;
        bool tempTerminate, tempGranted, timeCheck, processCheck, createProcess;

        pid_t pid;
        int processIndex = 0;
        int currentProcesses = 0;       // Tracks number of active processes
        unsigned int newProcessTime[2] = { 0, 0 };
        unsigned int nextRandomProcessTime;
        unsigned int nextProcessTimeBound = 5000;
        unsigned int tempClock[2];
        unsigned int receivedTime[2];

        Queue* blockedQueue = createQueue(MAX_PROCESSES);
	
	// Singal for ctrl+c
        if(signal(SIGINT, handle) == SIG_ERR) {
                perror("oss: Error: SIGINT failed\n");
        }
	
	// Signal for timer
        if(signal(SIGALRM, handle) == SIG_ERR) {
                perror("oss: Error: SIGALRM failed\n");
        }

        // Create shared mem for clock and blocked key
        shmClockKey = 2016;
        if((shmClockID = shmget(shmClockKey, (2 * (sizeof(unsigned int))), IPC_CREAT | 0666)) == -1) {
                perror("oss: Error: Failure to create shared memory space for simulated clock\n");
                return 1;
        }

        shmBlockedKey = 3017;
        if((shmBlockedID = shmget(shmBlockedKey, (MAX_PROCESSES * (sizeof (int))), IPC_CREAT | 0666)) == -1) {
                perror("OSS: failed to create shared memory for blocked USER process array\n");
                return 1;
        }

        // Attach to and initialize shared memory for clock and blocked process array
        if((shmClock = (unsigned int *) shmat(shmClockID, NULL, 0)) < 0) {
                perror("oss: Error: Failure to attach shared memory space for simulated clock\n");
                return 1;
        }

        shmClock[0] = 0; // Seconds for clock
        shmClock[1] = 0; // Nanoseconds for clock

        if((shmBlocked = (int *) shmat (shmBlockedID, NULL, 0)) < 0) {
                perror("oss: Error: Failure to attach shared memory space for blocked process array\n");
                return 1;
        }

        int i, j;
        for(i = 0 ; i < MAX_PROCESSES ; ++i) {
                shmBlocked[i] = 0;
        }

        messageKey = 1996;
        if((messageID = msgget(messageKey, IPC_CREAT | 0666)) == -1) {
                perror("oss: Error: Failure to create the msg queue\n");
                return 1;
        }

        int totalResourceTable[20];
        for(i = 0 ; i < 20 ; ++i) {
                totalResourceTable[i] = (rand() % (10 - 1 + 1) + 1);
        }

        int maxrscTable[MAX_PROCESSES][20];
        for(i = 0 ; i < MAX_PROCESSES ; ++i) {
                for (j = 0 ; j < 20 ; ++j) {
                        maxrscTable[i][j] = 0;
                }
        }

        // Table storing the amount of each resource currently allocated to each process
        int allocatedTable[MAX_PROCESSES][20];
        for(i = 0 ; i < MAX_PROCESSES ; ++i) {
                for(j = 0 ; j < 20 ; ++j) {
                        allocatedTable[i][j] = 0;
                }
        }

        int availableResourcesTable[20];
        for(i = 0 ; i < 20 ; ++i) {
                availableResourcesTable[i] = totalResourceTable[i];
        }

        int requestedResourceTable[MAX_PROCESSES];
        for(i = 0 ; i < MAX_PROCESSES ; ++i) {
                requestedResourceTable[i] = -1;
        }

        while(1) {
                // Terminate if log file exceeds 100000 lines
                if(numberOfLines >= 100000) {
                        fprintf(fp, "OSS: Log file has exceeded max length. Program terminating!\n");
                        kill(getpid(), SIGINT);
                }

                createProcess = false;  // Flag is false by default each run through the loop
                if(shmClock[0] > newProcessTime[0] || (shmClock[0] == newProcessTime[0] && shmClock[1] >= newProcessTime[1])) {
                        timeCheck = true;
                } else {
                        timeCheck = false;
                }
                if((currentProcesses < MAX_RUNNING_PROCESSES) && (createdProcs < MAX_PROCESSES)) {
                        processCheck = true;
                } else {
                        processCheck = false;
                }

                if((timeCheck == true) && (processCheck == true)) {
                        createProcess = true;
                }

                if(createProcess) {
                        processIndex = createdProcs;    // Sets process index for the various resource tables
                        for(i = 0 ; i < 20 ; ++i) {
                                maxrscTable[processIndex][i] = (rand() % (maxAmountOfEachResource - 1 + 1) + 1);
                        }

                        fprintf(fp, "OSS: Generated process P%d\n", processIndex);
                        for(i = 0 ; i < 20 ; ++i) {
                                fprintf(fp, "%d: %d\t", i, maxrscTable[processIndex][i]);
                        }

                        fprintf(fp, "\n");

                        pid = fork();
			if(pid < 0) {
                                perror("oss: Error: Failure to fork child process\n");
                                kill(getpid(), SIGINT);
                        }

                        // Child Process
                        if(pid == 0) {
                                char intBuffer0[3], intBuffer1[3], intBuffer2[3], intBuffer3[3], intBuffer4[3];
                                char intBuffer5[3], intBuffer6[3], intBuffer7[3], intBuffer8[3], intBuffer9[3];
                                char intBuffer10[3], intBuffer11[3], intBuffer12[3], intBuffer13[3], intBuffer14[3];
                                char intBuffer15[3], intBuffer16[3], intBuffer17[3], intBuffer18[3], intBuffer19[3];
                                char intBuffer20[3];


                                sprintf(intBuffer0, "%d", maxrscTable[processIndex][0]);     // Resources 1-20
                                sprintf(intBuffer1, "%d", maxrscTable[processIndex][1]);
                                sprintf(intBuffer2, "%d", maxrscTable[processIndex][2]);
                                sprintf(intBuffer3, "%d", maxrscTable[processIndex][3]);
                                sprintf(intBuffer4, "%d", maxrscTable[processIndex][4]);
                                sprintf(intBuffer5, "%d", maxrscTable[processIndex][5]);
                                sprintf(intBuffer6, "%d", maxrscTable[processIndex][6]);
                                sprintf(intBuffer7, "%d", maxrscTable[processIndex][7]);
                                sprintf(intBuffer8, "%d", maxrscTable[processIndex][8]);
                                sprintf(intBuffer9, "%d", maxrscTable[processIndex][9]);
                                sprintf(intBuffer10, "%d", maxrscTable[processIndex][10]);
                                sprintf(intBuffer11, "%d", maxrscTable[processIndex][11]);
                                sprintf(intBuffer12, "%d", maxrscTable[processIndex][12]);
                                sprintf(intBuffer13, "%d", maxrscTable[processIndex][13]);
                                sprintf(intBuffer14, "%d", maxrscTable[processIndex][14]);
                                sprintf(intBuffer15, "%d", maxrscTable[processIndex][15]);
                                sprintf(intBuffer16, "%d", maxrscTable[processIndex][16]);
                                sprintf(intBuffer17, "%d", maxrscTable[processIndex][17]);
                                sprintf(intBuffer18, "%d", maxrscTable[processIndex][18]);
                                sprintf(intBuffer19, "%d", maxrscTable[processIndex][19]);
                                sprintf(intBuffer20, "%d", processIndex);

                                fprintf(fp, "OSS: Process P%d was created at time %d:%d.\n", processIndex, shmClock[0], shmClock[1] );
                                numberOfLines++;

                                // Exec call to user executable
                                execl("./user", "user", intBuffer0, intBuffer1, intBuffer2, intBuffer3,
                                       intBuffer4, intBuffer5, intBuffer6, intBuffer7, intBuffer8,
                                       intBuffer9, intBuffer10, intBuffer11, intBuffer12, intBuffer13,
                                       intBuffer14, intBuffer15, intBuffer16, intBuffer17, intBuffer18,
                                       intBuffer19, intBuffer20, NULL );
                                       exit(127);
                        }

                        // In Parent Process
                        newProcessTime[0] = shmClock[0];
                        newProcessTime[1] = shmClock[1];
                        nextRandomProcessTime = (rand() % (nextProcessTimeBound - 1 + 1) + 1);
                        newProcessTime[1] += nextRandomProcessTime;
                        newProcessTime[0] += newProcessTime[1] / 1000000000;
                        newProcessTime[1] = newProcessTime[1] % 1000000000;
                        timeCheck = false;
                        processCheck = false;
                        currentProcesses++;
                        createdProcs++;
                }

                msgrcv(messageID, &resourceManagement, sizeof(resourceManagement), 5, IPC_NOWAIT);

                arrayIndex = resourceManagement.pid;
                tempIndex = resourceManagement.tableIndex;
                arrayRequest = resourceManagement.request;
                tempRelease = resourceManagement.release;
                tempTerminate = resourceManagement.terminate;
                tempGranted = resourceManagement.resourceGranted;
                tempClock[0] = resourceManagement.messageTime[0];
                tempClock[1] = resourceManagement.messageTime[1];

                // Resource Request Message
                if(arrayRequest != -1) {
                        fprintf(fp, "OSS: Detected Process P%d requesting R%d at time %d:%d.\n", tempIndex,
                                 arrayRequest, tempClock[0], tempClock[1]);
                        numberOfLines++;
                        requestResource++;

                        requestedResourceTable[tempIndex] = arrayRequest;

                        // Change rsctables to test state
                        allocatedTable[tempIndex][arrayRequest]++;
                        availableResourcesTable[arrayRequest]--;

                        // Run Banker's Algorithm
                        if(isSafeState(availableResourcesTable, maxrscTable, allocatedTable)) {
                                grantRequest++;
                                resourceManagement.msg_type = tempIndex;
                                resourceManagement.pid = getpid();
                                resourceManagement.tableIndex = tempIndex;
                                resourceManagement.request = -1;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = false;
                                resourceManagement.resourceGranted = true;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if(msgsnd(messageID, &resourceManagement, sizeof (resourceManagement), 0) == -1) {
                                        perror("oss: Error: Failure to send message\n");
                                }

                                fprintf(fp, "OSS: Granting P%d request at R%d at time %d:%d.\n",
                                         tempIndex, arrayRequest, shmClock[0], shmClock[1]);
                                numberOfLines++;
                        }
                        else {
                                // Reset tables to their state before the test
                                allocatedTable[tempIndex][arrayRequest]--;
                                availableResourcesTable[arrayRequest]++;

                                // Place that process's index in the blocked queue
                                enQueue(blockedQueue, tempIndex);

                                shmBlocked[tempIndex] = 1;

                                fprintf(fp, "OSS: Running deadlock detection at time %d:%d.\n\tP%d was denied its request of R%d and was deadlocked\n",
                                         shmClock[0], shmClock[1], tempIndex, arrayRequest);
                                numberOfLines++;
                        }

                        incrementClock(shmClock);
                }

                // Resource Release Message
                if(tempRelease != -1) {
                        fprintf(fp, "OSS: P%d is releasing resources from R%d at time %d:%d.\n",
                                  tempIndex, tempRelease, tempClock[0], tempClock[1]);
                        numberOfLines++;

                        releaseResource++;
                        allocatedTable[tempIndex][tempRelease]--;
                        availableResourcesTable[tempRelease]++;

                        fprintf(fp, "OSS: Process P%d release notification was signal handled at time %d:%d.\n", tempIndex, shmClock[0],
                                 shmClock[1]);
                        numberOfLines++;
                        incrementClock ( shmClock );
                }

                // Process Termination Message
                if(tempTerminate == true) {
                        fprintf(fp, "OSS: Terminated P%d at time %d:%d.\n", tempIndex, tempClock[0], tempClock[1]);
                        numberOfLines++;

                        for(i = 0 ; i < 20 ; ++i) {
                                tempHolder = allocatedTable[tempIndex][i];
                                allocatedTable[tempIndex][i] = 0;
                                availableResourcesTable[i] += tempHolder;
                        }
                        currentProcesses--;

                        fprintf(fp, "OSS: Process P%ds termination was handled at time %d:%d.\n", tempIndex,
                                 shmClock[0], shmClock[1]);
                        numberOfLines++;
                        incrementClock(shmClock);
                }

                // Check blocked queue
                if(!isEmpty) {
                        tempIndex = dequeue(blockedQueue);
                        arrayRequest = requestedResourceTable[tempIndex];
                        requestResource++;

                        allocatedTable[tempIndex][arrayRequest]++;
                        availableResourcesTable[arrayRequest]--;

                        // Run Banker's Algorithm
                        if(isSafeState(availableResourcesTable, maxrscTable, allocatedTable)) {
                                grantRequest++;
                                resourceManagement.msg_type = tempIndex;
                                resourceManagement.pid = getpid();
                                resourceManagement.tableIndex = tempIndex;
                                resourceManagement.request = -1;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = false;
                                resourceManagement.resourceGranted = true;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if(msgsnd(messageID, &resourceManagement, sizeof (resourceManagement), 0) == -1) {
                                        perror("oss: Error: Failure to send message\n");
                                }
                                shmBlocked[tempIndex] = 0;

                                fprintf(fp, "OSS: Granting P%d request at R%d at time %d:%d.\n",
                                         tempIndex, arrayRequest, shmClock[0], shmClock[1]);
                                numberOfLines++;
                        } else {
                                // Reset tables to their state before the test
                                allocatedTable[tempIndex][arrayRequest]--;
                                availableResourcesTable[arrayRequest]++;

                                // Place that process's index in the blocked queue
                                enQueue ( blockedQueue, tempIndex );

                                // Set the blocked process flag in shared memory for USER to see
                                shmBlocked[tempIndex] = 1;

                                fprintf(fp, "OSS: Running deadlock detection at time %d:%d.\n\tP%d was denied its request of R%d and was deadlocked\n",
                                         shmClock[0], shmClock[1], tempIndex, arrayRequest);
                                numberOfLines++;
                        }
                        incrementClock(shmClock);
                }

                incrementClock(shmClock);

                if((numberOfLines % 20 == 0) && (verbose == true)) {
                        fprintf(fp, "---------- CURRENTLY ALLOCATED RESOURCES ----------\n");
                        fprintf(fp, "\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n");
                        for(i = 0; i < createdProcs; ++i) {
                                fprintf(fp, "P%d:\t", i);
                                for(j = 0 ; j < 20 ; ++j) {
                                        fprintf(fp, "%d\t", allocatedTable[i][j]);
                                }
                                fprintf(fp, "\n");
                        }
                }

        }
        displayStatistics();

        // Detach from, delete shared memory & message queue
        killProcesses();

        return ( 0 ) ;
        // End of main program
}

bool isSafeState(int available[], int maximum[][MAX_RESOURCES], int allot[][MAX_RESOURCES]) {
        markerChk++;
        int index;
        int count = 0;
        int need[MAX_PROCESSES][MAX_RESOURCES];
        processCalculation(need, maximum, allot); // Function to calculate need matrix
        bool finish[MAX_PROCESSES] = { 0 };

        int work[MAX_RESOURCES];

        for(int i = 0 ; i < MAX_RESOURCES ; ++i) {
                work[i] = available[i];
        }

        while(count < MAX_PROCESSES) {
                bool found = false;
                for(int proc = 0 ; proc < MAX_PROCESSES ; ++proc) {
                        if(finish[proc] == 0) {
				int j;
                                for(j = 0 ; j < MAX_RESOURCES ; ++j) {
                                        if(need[proc][j] > work[j])
                                            break;
                                }
                                if(j == MAX_RESOURCES) {
                                        for(int k = 0 ; k < MAX_RESOURCES ; ++k) {
                                                work[k] += allot[proc][k];
                                        }
                                        finish[proc] = 1;
                                        found = true;
                                }
                        }
                }

                if(found == false) {
                        return false;
                }
        }

        return true;

}

// Prints help message
void helpMessage() {
        printf("---------- USAGE ----------\n");
        printf("./oss [-h] [-v]\n");
        printf("Invoke with no arguments\n");
        printf("-h\tDisplays usage message (optional)\n");
	printf("-v\tActivates verbose mode\n");
        printf("---------------------------\n");
}

// Prints program statistics
void displayStatistics() {
        double approvalPercentage = grantRequest / requestResource;

        fprintf(fp, "---------- PROGRAM STATISTICS ----------\n");
        fprintf(fp, "   0) Total Created Processes: %d\n", createdProcs);
        fprintf(fp, "   1) Total Requested Resources: %d\n", requestResource);
        fprintf(fp, "   2) Total Granted Requests: %d\n", gr);
        fprintf(fp, "   3) Percentage of Granted Requested: %d\n", percentage);
        fprintf(fp, "   4) Total Deadlock Avoidance Algorithm Used (Banker's Algorithm): %d\n", markerChk);
        fprintf(fp, "   5) Total Resources Released: %d\n", releaseResource);

        printf("---------- PROGRAM STATISTICS ----------\n");
        printf("   0) Total Created Processes: %d\n", createdProcs);
        printf("   1) Total Requested Resources: %d\n", requestResource);
        printf("   2) Total Granted Requests: %d\n", gr);
        printf("   3) Percentage of Granted Requested: %d\n", percentage);
        printf("   4) Total Deadlock Avoidance Algorithm Used (Banker's Algorithm): %d\n", markerChk);
        printf("   5) Total Resources Released: %d\n", releaseResource);
}

// Signal handling
void handle(int signal) {
        if(signal == SIGINT || signal == SIGALRM) {
                printf("oss: Log: Termination signal received\n");
                displayStatistics();
                killProcesses();
                kill(0, SIGKILL);
                wait(NULL);
                exit(0);
        }
}

void processCalculation(int need[MAX_PROCESSES][MAX_RESOURCES], int maximum[MAX_PROCESSES][MAX_RESOURCES], int allot[MAX_PROCESSES][MAX_RESOURCES]) {
        for(int i = 0 ; i < MAX_PROCESSES ; ++i) {
                for(int j = 0; j < MAX_RESOURCES; ++j) {
                        need[i][j] = maximum[i][j] - allot[i][j];
                }
        }
}

// Function to print Allocated Resource Table
void displayTable(int num1, int array[][MAX_RESOURCES]) {
        num1 = createdProcs;

        printf("---------- CURRENTLY ALLOCATED RESOURCES ----------\n");
        printf("\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n");
        
	for(int i = 0 ; i < createdProcs ; ++i) {
                printf ( "P%d:\t", i );
                for(int j = 0 ; j < 20 ; ++j) {
                        printf("%d\t", array[i][j]);
                }

                printf("\n");
        }
}

// Function to print the table showing the max claim vectors for each process
void displayMaxTable(int num1, int array[][MAX_RESOURCES]){
        num1 = createdProcs;

        printf("---------- MAX CLAIM TABLE ----------\n");
        printf("\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n");
        
	for(int i = 0 ; i < createdProcs ; ++i) {
                printf("P%d:\t", i);
                for(int j = 0 ; j < 20 ; ++j) {
                        printf("%d\t", array[i][j]);
                }

                printf("\n");
        }
}

// Increment clock
void incrementClock(unsigned int shmClock[]) {
        int processingTime = 5000; // Can be changed to adjust how much the clock is incremented.
        shmClock[1] += processingTime;

        shmClock[0] += shmClock[1] / 1000000000;
        shmClock[1] = shmClock[1] % 1000000000;
}

// Function to terminate all shared memory and message queue
void killProcesses() {
        fclose(fp);

        // Detach from shared memory
        shmdt(shmClock);
        shmdt(shmBlocked);

        // Destroy shared memory
        shmctl(shmClockID, IPC_RMID, NULL);
        shmctl(shmBlockedID, IPC_RMID, NULL);

        // Destroy message queue
        msgctl(messageID, IPC_RMID, NULL);
}
