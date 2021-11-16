/*
 * queue.h by Pascal Odijk 11/4/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This file contains the queue helper functions to init/add/remove/etc.
 * This code has been modified from the website: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// Queue structure
typedef struct Queue {
        int topQueue;
        int bottomQueue;
        int size;
        int *array;
        int pid;
        unsigned capacity;
} Queue;

Queue* createQueue(unsigned capacity) {
        Queue* queue = (Queue*) malloc (sizeof(Queue));
        queue->capacity = capacity;
        queue->topQueue = queue->size = 0;
        queue->bottomQueue = capacity - 1;      // This is important, see the enqueue
        queue->array = (int*) malloc (queue->capacity * sizeof (int));

        return queue;
}

int isFull(Queue* queue) {
        return (queue->size == queue->capacity);
}


int isEmpty(Queue* queue) {
        return (queue->size == 0);
}

void enQueue(Queue* queue, int item) {
        if(isFull(queue))
                return;

        queue->bottomQueue = ( queue->bottomQueue + 1 ) % queue->capacity;
        queue->array[queue->bottomQueue] = item;
        queue->size = queue->size + 1;
}

int dequeue(Queue* queue) {
        if(isEmpty(queue))
                return INT_MIN;

        int item = queue->array[queue->topQueue];
        queue->topQueue = ( queue->topQueue + 1 ) % queue->capacity;
        queue->size = queue->size - 1;

        return item;
}

int topQueue(Queue* queue) {
        if(isEmpty(queue))
                return INT_MIN;

        return queue->array[queue->topQueue];
}

int bottomQueue ( Queue* queue ) {
        if(isEmpty(queue))
                return INT_MIN;

        return queue->array[queue->bottomQueue];
}
