/*
 * oss.h by Pascal Odijk 11/3/2021
 * P5 CS4760 Prof. Bhatia
 *
 * This is the header file for oss.c, it contains the function prototypes and macros.
 */

#ifndef OSS_H
#define OSS_H

// Function prototypes
void signalHandler();
void exitHandler();
void helpMessage();
void interrupt();
void timer();
void removeShmem();
void finalize();
void cleanUp();
void lockSem();
void releaseSem();
void incrementClock();
void initResource();
void displayResource();
void updateResources();
void initPCBT();
void initPCB();

#endif
