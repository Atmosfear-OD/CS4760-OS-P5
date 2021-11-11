/*
 * config.h by Pascal Odijk 11/4/2021
 * P5 CS4760 Prof. Bhatia
 *
 * This file contains macros and structures needed by oss and user programs.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// File macros
#define BUFFER_LENGTH 1024
#define MAX_FILE 100000

// Process macros
#define MAX_TIME 10
#define MAX_PROCESSES 18
#define TOTAL_PROCESSES 100

// Resource macros
#define MAX_RESOURCES 20
#define MIN_SHARED_RES (int)(MAX_RESOURCES * 0.15)
#define MAX_SHARED_RES (int)(MAX_RESOURCES * 0.25)

// Structure for shared clock
struct Clock {
	unsigned int sec, ns;
};

// Strucutre for messages
struct Message {
	long type;
	int index, flag;
	pid_t cpid;
	bool isRequest, isRelease, isSafe;
	char message[BUFFER_LENGTH];
};

// Strucutre for data
struct Data {
	int init_resource[MAX_RESOURCES], resource[MAX_RESOURCES], sharedNum;
};

// Structure for process control block
struct PCB {
	int pidIndex, max[MAX_RESOURCES], alloc[MAX_RESOURCES];
	int request[MAX_RESOURCES], release[MAX_RESOURCES];
	pid_t pid;
};

#endif
