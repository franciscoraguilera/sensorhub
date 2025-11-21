#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <time.h>

// Data type for sensor readings
typedef struct {
    int sensor_id;      // 1 for sensor1, 2 for sensor2
    float temperature;  // Temperature reading in °C
    time_t timestamp;   // Time of the reading
} sensor_reading_t;

typedef struct node {
    sensor_reading_t data;
    struct node* next;
} node_t;

typedef struct {
    node_t* head;
    node_t* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int size;           // Current queue size
    int max_size;       // Maximum queue size (0 = unbounded)
} queue_t;

// Initialize the queue
void queue_init(queue_t *q, int max_size);

// Destroy the queue and free all nodes
void queue_destroy(queue_t *q);

// Push an item into the queue
// Returns 0 on success, -1 if queue is full
int queue_push(queue_t *q, sensor_reading_t item);

// Blocking pop from the queue. Returns 0 on success, -1 if exit is signaled.
int queue_pop(queue_t *q, sensor_reading_t *item);

// Get current queue size (thread-safe)
int queue_size(queue_t *q);

#endif // QUEUE_H