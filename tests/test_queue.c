#include "../src/queue.h"
#include "../src/utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>

void test_queue_init() {
    printf("Testing queue_init...\n");
    queue_t q;
    queue_init(&q, 10);
    assert(q.head == NULL);
    assert(q.tail == NULL);
    assert(q.size == 0);
    assert(q.max_size == 10);
    queue_destroy(&q);
    printf("  PASSED\n");
}

void test_queue_push_pop() {
    printf("Testing queue_push and queue_pop...\n");
    queue_t q;
    queue_init(&q, 10);

    sensor_reading_t reading1 = {1, 23.5f, time(NULL)};
    sensor_reading_t reading2 = {2, 24.6f, time(NULL)};

    // Push items
    assert(queue_push(&q, reading1) == 0);
    assert(queue_size(&q) == 1);
    assert(queue_push(&q, reading2) == 0);
    assert(queue_size(&q) == 2);

    // Pop items
    sensor_reading_t popped;
    assert(queue_pop(&q, &popped) == 0);
    assert(popped.sensor_id == 1);
    assert(popped.temperature == 23.5f);
    assert(queue_size(&q) == 1);

    assert(queue_pop(&q, &popped) == 0);
    assert(popped.sensor_id == 2);
    assert(popped.temperature == 24.6f);
    assert(queue_size(&q) == 0);

    queue_destroy(&q);
    printf("  PASSED\n");
}

void test_queue_max_size() {
    printf("Testing queue max_size limit...\n");
    queue_t q;
    queue_init(&q, 3);

    sensor_reading_t reading = {1, 23.5f, time(NULL)};

    // Fill queue to max
    assert(queue_push(&q, reading) == 0);
    assert(queue_push(&q, reading) == 0);
    assert(queue_push(&q, reading) == 0);
    assert(queue_size(&q) == 3);

    // Try to push beyond max
    assert(queue_push(&q, reading) == -1);
    assert(queue_size(&q) == 3);  // Size should not increase

    queue_destroy(&q);
    printf("  PASSED\n");
}

void test_queue_unbounded() {
    printf("Testing unbounded queue (max_size=0)...\n");
    queue_t q;
    queue_init(&q, 0);  // Unbounded

    sensor_reading_t reading = {1, 23.5f, time(NULL)};

    // Push many items
    for (int i = 0; i < 100; i++) {
        assert(queue_push(&q, reading) == 0);
    }
    assert(queue_size(&q) == 100);

    queue_destroy(&q);
    printf("  PASSED\n");
}

int main(void) {
    printf("\n=== Queue Tests ===\n");

    init_utils();  // Initialize exit flag

    test_queue_init();
    test_queue_push_pop();
    test_queue_max_size();
    test_queue_unbounded();

    printf("\nAll queue tests passed!\n\n");
    return 0;
}
