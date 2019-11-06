#include <stdlib.h>
#include "ipc.h"

enum
{
	DONE_TIME = 10000
};

typedef struct
{
    timestamp_t time;
    local_id id;
} queue_el_t;

typedef struct
{
    int front, rear, size;
    unsigned capacity;
    queue_el_t* array;
} queue_t;


queue_t* createQueue(unsigned capacity);
  
int isFull(queue_t* queue);  

int isEmpty(queue_t* queue);

void enqueue(queue_t* queue, queue_el_t item);

queue_el_t dequeue(queue_t* queue);

queue_el_t front(queue_t* queue);

queue_t* sort(queue_t* queue);

int isDONE(queue_t* queue, local_id id);

void print_q(queue_t* queue);
