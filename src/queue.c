#include "queue.h"
#include "utils.h"  // For should_exit()
#include <stdlib.h>

void queue_init(queue_t *q) {
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_destroy(queue_t *q) {
    // Free all nodes in the queue
    node_t *current = q->head;
    while (current) {
        node_t *temp = current;
        current = current->next;
        free(temp);
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void queue_push(queue_t *q, sensor_reading_t item) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) return;  // Allocation failure – ideally handle error.
    new_node->data = item;
    new_node->next = NULL;
    
    pthread_mutex_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = new_node;
        q->tail = new_node;
    } else {
        q->head = q->tail = new_node;
    }
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(queue_t *q, sensor_reading_t *item) {
    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL && !should_exit()) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    if (q->head == NULL && should_exit()) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    node_t *temp = q->head;
    *item = temp->data;
    q->head = temp->next;
    if (q->head == NULL)
        q->tail = NULL;
    free(temp);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}