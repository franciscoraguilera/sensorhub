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
} queue_t;

// Initialize the queue
void queue_init(queue_t *q);

// Destroy the queue and free all nodes
void queue_destroy(queue_t *q);

// Push an item into the queue
void queue_push(queue_t *q, sensor_reading_t item);

// Blocking pop from the queue. Returns 0 on success, -1 if exit is signaled.
int queue_pop(queue_t *q, sensor_reading_t *item);

#endif // QUEUE_H