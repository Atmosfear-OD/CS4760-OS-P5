/*
 * oss.h by Pascal Odijk 11/12/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This is the header file for oss.c. It contains the function prototypes.
 */

#ifndef OSS_H
#define OSS_H

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
#include "config.h"
#include "queue.h"

// Function prototypes
bool isSafeState();
void helpMessage();
void processCalculation();
void incrementClock();
void displayTable();
void displayMaxTable();
void displayStatistics();
void killProcesses();

#endif
