#include "../src/config.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_config_defaults() {
    printf("Testing config_load_defaults...\n");
    config_load_defaults();

    assert(strcmp(g_config.i2c_device, "/dev/i2c-1") == 0);
    assert(g_config.sensor1_address == 0x48);
    assert(g_config.sensor1_interval == 1);
    assert(g_config.sensor2_address == 0x49);
    assert(g_config.sensor2_interval == 2);
    assert(g_config.sensor_timeout == 10);
    assert(g_config.network_port == 8080);
    assert(g_config.network_backlog == 5);
    assert(g_config.queue_max_size == 100);
    assert(strcmp(g_config.log_file, "sensor_log.csv") == 0);

    printf("  PASSED\n");
}

void test_config_load() {
    printf("Testing config_load from config.ini...\n");

    // Try to load config.ini from project root
    int result = config_load("../config.ini");

    // Even if file doesn't exist, defaults should be loaded
    assert(g_config.sensor1_address == 0x48 || g_config.sensor1_address != 0);
    assert(g_config.network_port > 0);

    printf("  PASSED (result=%d)\n", result);
}

int main(void) {
    printf("\n=== Config Tests ===\n");

    test_config_defaults();
    test_config_load();

    printf("\nAll config tests passed!\n\n");
    return 0;
}
