#include "queue.h"
#include <stdio.h>

queue_t* createQueue(unsigned capacity)
{
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (queue_el_t*) malloc(queue->capacity * sizeof(queue_el_t));
    return queue;
}
 
// queue_t is full when size becomes equal to the capacity
int isFull(queue_t* queue)
{  return (queue->size == queue->capacity);  }

// queue_t is empty when size is 0
int isEmpty(queue_t* queue)
{  return (queue->size == 0); }

// Function to add an item to the queue.
void enqueue(queue_t* queue, queue_el_t item)
{
    if (isFull(queue))
        return;
    queue->rear = ++queue->rear % queue->capacity;
    queue->array[queue->rear] = item;
    ++queue->size;
}
 
// Function to remove an item from queue.
queue_el_t dequeue(queue_t* queue)
{
    queue_el_t item = queue->array[queue->front];
    queue->front = ++queue->front % queue->capacity;
    --queue->size;
    return item;
}

queue_el_t front(queue_t* queue) 
{ 
    return queue->array[queue->front]; 
}

queue_t* sort(queue_t* queue)
{
    if (queue->size <= 1)
        return queue;
    for (int i = 0; i < queue->size; ++i)
    {
        for (int j = 0; j < queue->size - i - 1; ++j)
        {
            if (
                queue->array[(queue->front + j) % queue->capacity].time > queue->array[(queue->front + j + 1) % queue->capacity].time
                ||
                (
                    queue->array[(queue->front + j) % queue->capacity].time == queue->array[(queue->front + j + 1) % queue->capacity].time
                    &&
                    queue->array[(queue->front + j) % queue->capacity].id > queue->array[(queue->front + j + 1) % queue->capacity].id
                )
                )
            {
                queue_el_t temp = queue->array[(queue->front + j + 1) % queue->capacity];
                queue->array[(queue->front + j + 1) % queue->capacity] = queue->array[(queue->front + j) % queue->capacity];
                queue->array[(queue->front + j) % queue->capacity] = temp;
            }
        }
    }
    return queue;
}

int isDONE(queue_t* queue, local_id id)
{
    for (int i = 0; i < queue->size; ++i)
    {
        if (queue->array[(queue->front + i) % queue->capacity].id == id && queue->array[(queue->front + i) % queue->capacity].time == DONE_TIME)
            return 0;
    }
    return -1;
}

void print_q(queue_t* queue)
{
    printf("Current queue:\n");
    for (int i = 0; i < queue->size; ++i)
    {
        printf("%2d - %2d\n", queue->array[(queue->front + i) % queue->capacity].time, queue->array[(queue->front + i) % queue->capacity].id);
    }
    printf("*****\n");
}
