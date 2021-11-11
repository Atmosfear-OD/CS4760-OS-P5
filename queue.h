/*
 * queue.h by Pascal Odijk 11/4/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This file contains the queue helper functions to init/add/remove/etc.
 * This code has been modified from the website: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

// A strucutre to represent the nodes within the queue
struct Node {
	int index;
	struct Node *next;
};

// A structure to represent a queue
struct Queue {
        struct Node *front, *rear;
	int size;
};

// Function prototypes
struct Queue* createQueue();
struct Node* addNode(int index);
bool isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, int index);
struct Node* dequeue(struct Queue* queue);
struct Node* front(struct Queue* queue);
struct Node* rear(struct Queue* queue);
int getSize(struct Queue* queue);

#endif
