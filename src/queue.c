#include "queue.h"
#include "utils.h"  // For should_exit()
#include <stdlib.h>
#include <stdio.h>

void queue_init(queue_t *q, int max_size) {
    q->head = q->tail = NULL;
    q->size = 0;
    q->max_size = max_size;
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

int queue_push(queue_t *q, sensor_reading_t item) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) {
        fprintf(stderr, "[Queue] Allocation failure\n");
        return -1;
    }
    new_node->data = item;
    new_node->next = NULL;

    pthread_mutex_lock(&q->mutex);

    // Check queue size limit
    if (q->max_size > 0 && q->size >= q->max_size) {
        pthread_mutex_unlock(&q->mutex);
        free(new_node);
        fprintf(stderr, "[Queue] Queue full (size=%d), dropping reading from sensor %d\n",
                q->size, item.sensor_id);
        return -1;
    }

    if (q->tail) {
        q->tail->next = new_node;
        q->tail = new_node;
    } else {
        q->head = q->tail = new_node;
    }
    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return 0;
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
    q->size--;
    free(temp);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int queue_size(queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    int size = q->size;
    pthread_mutex_unlock(&q->mutex);
    return size;
}
