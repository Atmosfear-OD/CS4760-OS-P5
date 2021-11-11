/*
 * matrix.h by Pascal Odijk 11/5/2021
 * P5 CMPSCI4760 Prof. Bhatia
 *
 * This is the header file for matrix.c. It contains the function prototypes.
 */

#ifndef MATRIX_H
#define MATRIX_H

#include <stdbool.h>
#include "config.h"
#include "queue.h"

void initMatrix();
void calcMatrix();
void displayVec();
void displayMatrix();
bool bankerAlg(FILE* filep, bool verbose, struct Data* data, struct PCB* pcb, struct Queue* queue, int cIndex);

#endif
