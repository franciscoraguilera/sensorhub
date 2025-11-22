#include "../src/utils.h"
#include <stdio.h>
#include <assert.h>

void test_exit_flag() {
    printf("Testing exit flag (C11 atomics)...\n");

    init_utils();
    assert(should_exit() == 0);

    set_exit_flag();
    assert(should_exit() == 1);

    // Reset for other tests
    init_utils();
    assert(should_exit() == 0);

    printf("  PASSED\n");
}

void test_latest_reading() {
    printf("Testing latest_reading structure...\n");

    assert(latest_reading.sensor1 == 0.0f);
    assert(latest_reading.sensor2 == 0.0f);
    assert(latest_reading.average == 0.0f);

    printf("  PASSED\n");
}

int main(void) {
    printf("\n=== Utils Tests ===\n");

    test_exit_flag();
    test_latest_reading();

    printf("\nAll utils tests passed!\n\n");
    return 0;
}
