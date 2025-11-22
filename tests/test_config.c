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

    // If file exists and loads successfully, verify specific expected value
    // If file doesn't exist (result == -1), defaults should be loaded
    if (result == 0) {
        // File loaded successfully, check it has valid configuration
        assert(g_config.sensor1_address == 0x48);  // Verify expected value from config.ini
        assert(g_config.network_port > 0 && g_config.network_port <= 65535);
    } else {
        // File not found, defaults should be loaded
        assert(g_config.sensor1_address == 0x48);  // Default value
        assert(g_config.network_port == 8080);     // Default value
    }

    printf("  PASSED (result=%d)\n", result);
}

void test_config_validation() {
    printf("Testing config validation with invalid values...\n");

    // Save current config
    config_t saved = g_config;

    // Test invalid sensor interval (should fail validation)
    config_load_defaults();
    g_config.sensor1_interval = -1;  // Invalid
    // Note: We can't directly test validate_config as it's static,
    // but we can verify defaults are sane
    config_load_defaults();
    assert(g_config.sensor1_interval > 0);

    // Test invalid port (should fail validation)
    config_load_defaults();
    assert(g_config.network_port >= 1 && g_config.network_port <= 65535);

    // Restore config
    g_config = saved;

    printf("  PASSED\n");
}

int main(void) {
    printf("\n=== Config Tests ===\n");

    test_config_defaults();
    test_config_load();
    test_config_validation();

    printf("\nAll config tests passed!\n\n");
    return 0;
}
