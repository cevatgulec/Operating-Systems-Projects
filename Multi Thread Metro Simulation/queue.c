#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

//constructing train
typedef struct {
    int ID;
    char starting_point;
    char destination_point;
    int length;
    int duration;
    int arrivalTime;
} Train;


typedef struct Node_t {
    Train data;
    struct Node_t *prev;
} NODE;

//Creating queue
typedef struct Queue {
    NODE *head;
    NODE *tail;
    int size;
    int duration;
    int limit;
} Queue;

Queue *ConstructQueue(int limit);
int Enqueue(Queue *train_queue, Train t);
int isEmpty(Queue* train_queue);
Train DequeueByStartingPoint(Queue *train_queue, char startingPoint);
char* QueueIDsToString(Queue* train_queue);
char MostCommonStartingPoint(Queue *train_queue);
int QueueSize(Queue *train_queue);

Queue *ConstructQueue(int limit) {
    Queue *queue = (Queue*) malloc(sizeof (Queue));
    if (queue == NULL) {
        return NULL;
    }
    if (limit <= 0) {
        limit = 65535;
    }
    queue->limit = limit;
    queue->size = 0;
    queue->duration = 0;
    queue->head = NULL;
    queue->tail = NULL;

    return queue;
}

//adding new item to the queue
int Enqueue(Queue *train_queue, Train t) {
    
    NODE* item = (NODE*) malloc(sizeof (NODE));
    item->data = t;

    if ((train_queue == NULL) || (item == NULL)) {
        return FALSE;
    }
    
    if (train_queue->size >= train_queue->limit) {
        return FALSE;
    }
    
    item->prev = NULL;
    if (train_queue->size == 0) {
        train_queue->head = item;
        train_queue->tail = item;

    } else {
        
        train_queue->tail->prev = item;
        train_queue->tail = item;
    }
    train_queue->size++;
    train_queue->duration += t.duration;
    return TRUE;
}


int isEmpty(Queue* train_queue) {
    if (train_queue == NULL) {
        return FALSE;
    }
    if (train_queue->size == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

//dequeue the element by given starting point for ex starting point ='A' or 'B', etc.
Train DequeueByStartingPoint(Queue *train_queue, char startingPoint) {
    Train ret;
    ret.ID = -1; // if empty return -1

    if (train_queue == NULL || isEmpty(train_queue)) {
        return ret;
    }

    NODE *current = train_queue->head;
    NODE *previous = NULL;

    while (current != NULL) {
        if (current->data.starting_point == startingPoint) {
            
            if (previous == NULL) {
                
                train_queue->head = current->prev;
            } else {
                
                previous->prev = current->prev;
            }

            if (train_queue->tail == current) {
                
                train_queue->tail = previous;
            }

            train_queue->size--;
            train_queue->duration -= current->data.duration;
            ret = current->data;
            free(current);
            return ret;
        }

        previous = current;
        current = current->prev;
    }

    return ret; 
}

//this function gives train ID's for every element in the queue
char* QueueIDsToString(Queue* train_queue) {
    if (train_queue == NULL || isEmpty(train_queue)) {
        return strdup(""); // return empty string if the queue is empty 
    }

    
    int maxLength = train_queue->size * 11;
    char *result = malloc(maxLength);
    if (result == NULL) {
        return NULL; 
    }

    result[0] = '\0'; 

    NODE *current = train_queue->head;
    while (current != NULL) {
        char buffer[12]; 
        snprintf(buffer, sizeof(buffer), "%d, ", current->data.ID);
        strcat(result, buffer);
        current = current->prev;
    }

    
    int len = strlen(result);
    if (len > 0) {
        result[len - 2] = '\0';
    }

    return result;
}

//this function returns the most occured Starting point in the queue because most occured Starting point has the most priority
// Since I calculated in the order A,B,E,F I handle the if tie happens choose A>B>E>F
char MostCommonStartingPoint(Queue *train_queue) {
    if (train_queue == NULL || isEmpty(train_queue)) {
        return '\0'; 
    }

    int countA = 0, countB = 0, countE = 0, countF = 0;
    NODE *current = train_queue->head;
    
    while (current != NULL) {
        switch (current->data.starting_point) {
            case 'A':
                countA++;
                break;
            case 'B':
                countB++;
                break;
            case 'E':
                countE++;
                break;
            case 'F':
                countF++;
                break;
        }
        current = current->prev;
    }

    // find the most common starting point
    char mostCommon = 'A';
    int maxCount = countA;

    if (countB > maxCount) {
        mostCommon = 'B';
        maxCount = countB;
    }
    if (countE > maxCount) {
        mostCommon = 'E';
        maxCount = countE;
    }
    if (countF > maxCount) {
        mostCommon = 'F';
    }

    return mostCommon;
}

//gives queue size
int QueueSize(Queue *train_queue) {
    if (train_queue == NULL) {
        return 0; 
    }

    return train_queue->size; 
}
