/*
 * matrix.c by Pascal Odijk 11/5/2021
 * P5 CMPSCI4760 Prof. Bhatia
 *
 * The functions initialize/display the matrix and resource vector and determines if the system is in safe mode or not
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "matrix.h"

// Create and initialize the 2D matrix
void initMatrix(struct PCB* pcb, struct Queue* queue, int m[][MAX_RESOURCES], int allot[][MAX_RESOURCES], int total) {
	struct Node t;
	t.next = queue->front;
	int nIndex = t.next->index;

	for(int i = 0 ; i < total ; i++) {
		for(int j = 0 ; j < MAX_RESOURCES ; j++) {
			m[i][j] = pcb[nIndex].max[j];
			allot[i][j] = pcb[nIndex].alloc[j];
		}

		if(t.next->next != NULL) {
			t.next = t.next->next;
			nIndex = t.next->index;
		} else {
			t.next = NULL;
		}
	}
}

// Calculate the matrix need, so max - allot
void calcMatrix(int need[][MAX_RESOURCES], int m[][MAX_RESOURCES], int allot[][MAX_RESOURCES], int total) {
	for(int i = 0 ; i < total ; i++) {
                for(int j = 0 ; j < MAX_RESOURCES ; j++) {
                        need[i][j] = m[i][j] - allot[i][j];
	        }
	}
}

// Printf the resource vector to outfile
void displayVec(FILE* filep, char* vecName, char* lName, int vec[MAX_RESOURCES]) {
	fprintf(filep, "---- %s Resource ----\n", vecName);
	fprintf(filep, "%3s: <", lName);

	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		fprintf(filep, "%2d\t", vec[i]);
	}

	fprintf(filep, ">\n");
	fflush(filep);
}

// Print the matrix to outfile
void displayMatrix(FILE* filep, char* mName, struct Queue* queue, int matrix[][MAX_RESOURCES], int total) {
	struct Node t;
	t.next = queue->front;
	fprintf(filep, "---- %s Matrix ----\n", mName);

	for(int i = 0 ; i < total ; i++) {
		fprintf(filep, "P%2d\t", t.next->index);

		for(int j = 0 ; j < MAX_RESOURCES ; j++) {
			fprintf(filep, "%2d\t", matrix[i][j]);
		}

		fprintf(filep, "\n");
		fflush(filep);

		t.next = (t.next->next != NULL) ? t.next->next : NULL;
	}
}

// Determines if system is in safe mode or not -- returns true if yes, returns false if not
bool bankerAlg(FILE* filep, bool verbose, struct Data* data, struct PCB* pcb, struct Queue* queue, int cIndex) {
	// If queue is null -- return true
	struct Node t;
	t.next  = queue->front;
	if(t.next == NULL) return true;

	int proc, res;
	int total = getSize(queue);
	int m[total][MAX_RESOURCES];
	int allot[total][MAX_RESOURCES];
	int req[MAX_RESOURCES];
	int need[total][MAX_RESOURCES];
	int avail[MAX_RESOURCES];
	
	// Initialize the matrix with given pcb table
	initMatrix(pcb, queue, m, allot, total);
	calcMatrix(need, m, allot, total);
	
	// Initialize the available and request vectors
	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		avail[i] = data->resource[i];
		req[i] = pcb[cIndex].request[i];
	}

	for(int i = 0 ; i < total ; i++) {
		for(int j = 0 ; j < MAX_RESOURCES ; j++) {
			avail[j] = avail[j] - allot[i][j];
		}
	}

	int k = 0;
	t.next = queue->front;
	while(t.next != NULL) {
		if(t.next->index == cIndex) {
			break;
		}

		k++;
		t.next = (t.next->next != NULL) ? t.next->next : NULL;
	}
	
	// If verbose mode is on, display matrices
	if(verbose) {
		displayMatrix(filep, "Maximum", queue, m, total);
		displayMatrix(filep, "Allocation", queue, m, total);
		char str[BUFFER_LENGTH];
		sprintf(str, "P%2d", cIndex);
		displayVec(filep, "Request", str, req);
	}

	bool finish[total];
	int work[MAX_RESOURCES];
	int safeSeq[total]; // For safe sequence
	memset(finish, 0, total * sizeof(finish[0])); // Set all processes to not finished

	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		work[i] = avail[i];
	}

	// For requesting resources
	for(int i = 0 ; i < MAX_RESOURCES ; i++) {
		if(need[k][i] < req[i] && i < data->sharedNum) {
			fprintf(filep, "OSS: WARNING: Requested more than initial max request\n");
			fflush(filep);

			if(verbose) {
				displayVec(filep, "Available", "A ", avail);
				displayMatrix(filep, "Need", queue, need, total);
			}

			return false;
		}
		
		// If request is less than or equal to availble resources -- allot, else -- notify that there is not enough available resources
		if(req[i] <= avail[i] && i < data->sharedNum) {
			avail[i] -= req[i];
			allot[k][i] += req[i];
			need[k][i] -= req[i];
		} else {
			fprintf(filep, "OSS: WARNING: Insufficient available resources to fulfill request\n");
                        fflush(filep);

                        if(verbose) {
                                displayVec(filep, "Available", "A ", avail);
                                displayMatrix(filep, "Need", queue, need, total);
                        }

                        return false;
		}
	}

	// For safety -- while all processes have not finished or system is not in safe state
	int indx = 0;
	int j = 0;
	while(indx < total) {
		bool found  = false;
		for(int i = 0 ; i < total ; i++) {
			if(finish[i] == 0) {
				for(j = 0 ; j < MAX_RESOURCES ; j++) {
					if(need[i][j] > work[j] && data->sharedNum) {
						break;
					}
				}

				if(j == MAX_RESOURCES) {
					for(int l = 0 ; l < MAX_RESOURCES ; l++) {
						work[l] += allot[i][l];
					}

					safeSeq[indx++] = i;
					finish[i] = 1;
					found = true;
				}
			}
		}
	
		// If there was not a process in safe sequence
		if(found == false) {
			fprintf(filep, "OSS: NOTICE: System is in unsafe state\n");
			fflush(filep);

			return false;
		}
	}
	
	if(verbose) {
		displayVec(filep, "Available", "A ", avail);
		displayMatrix(filep, "Need", queue, need, total);
	}

	int seq[total];
	int seqIndex = 0;
	t.next = queue->front;

	while(t.next != NULL) {
		seq[seqIndex++] = t.next->index;
		t.next = (t.next->next != NULL) ? t.next->next : NULL;
	}
	
	// Log safe sequence
	fprintf(filep, "OSS: NOTICE: System is in safe state -- safe sequence is: ");
	for(int i = 0 ; i < total ; i++) {
		fprintf(filep, "%2d ", seq[safeSeq[i]]);
	}

	fprintf(filep, "\n\n");
	fflush(filep);

	return true;
}
