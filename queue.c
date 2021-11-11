/*
 * queue.h by Pascal Odijk 11/4/2021
 * P5 CMPSCI 4760 Prof. Bhatia
 *
 * This file contains the queue helper functions to init/add/remove/etc.
 * This code has been modified from the website: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include "queue.h"

// Create and initialize queue
struct Queue* createQueue() {
        struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
        queue->front = queue->rear = NULL;
        queue->size = 0;
        return queue;
}

// Adds new node to queue
struct Node* addNode(int index) {
        struct Node* t = (struct Node*) malloc(sizeof(struct Node));
        t->index = index;
        t->next = NULL;
        return t;
}

// Check if queue is empty
bool isEmpty(struct Queue* queue) {
        if(queue->rear == NULL) return true;
	else return false;
}

// Add an item to the queue
void enqueue(struct Queue* queue, int index) {
        struct Node* t = addNode(index);
        queue->size = queue->size + 1;

        // If the queue is empty, the new node will be both front and rear
        if(queue->rear == NULL) {
                queue->rear = queue->front = t;
                return;
        } else {
                queue->rear = queue->rear->next = t;
                return;
        }
}

// Remove function from queue
struct Node* dequeue(struct Queue* queue) {
        if(isEmpty(queue)) return NULL;

        struct Node* t = queue->front;
        free(t);
        queue->front = queue->front->next;
        queue->size = queue->size - 1;

        // If front is null, then rear needs to be null
        if(queue->front == NULL) queue->rear = NULL;

        queue->size = queue->size - 1;
        return t;
}

// Return front of queue
struct Node* front(struct Queue* queue) {
        if(isEmpty(queue)) return NULL;
        return queue->front;
}

// Return rear of queue
struct Node* rear(struct Queue* queue) {
        if (isEmpty(queue)) return NULL;
        return queue->rear;
}

// Return current size of queue
int getSize(struct Queue* queue) {
        return queue->size;
}
