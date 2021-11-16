/* 
 * config.h by Pascal Odijk 11/11/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This file contains the macros and structures used through the program.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>

// Macros
#define MAX_RUNNING_PROCESSES 18
#define MAX_PROCESSES 100
#define MAX_RESOURCES 20

// Message queue structure
typedef struct {
	long msg_type;		
	int pid;	
	int tableIndex;		
	int request;		
	int release;		
	bool terminate;		
	bool resourceGranted;	
	unsigned int messageTime[2];	
} MessageQueue;

void handle(int signal);

MessageQueue resourceManagement;
int messageID;
key_t messageKey;

int shmClockID;
int *shmClock;
key_t shmClockKey;

int shmBlockedID;
int *shmBlocked;
key_t shmBlockedKey;

#endif
