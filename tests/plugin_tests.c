/**
 * Aurora OS - Plugin System Tests
 * 
 * Tests for the plugin system functionality
 */

#include "../kernel/core/plugin.h"
#include "../kernel/drivers/vga.h"
#include "../kernel/memory/memory.h"

/* Test plugin counters */
static int test_init_called = 0;
static int test_cleanup_called = 0;
static int test_function_called = 0;

/**
 * Test plugin initialization function
 */
static int test_plugin_init(plugin_descriptor_t* plugin) {
    test_init_called++;
    return PLUGIN_SUCCESS;
}

/**
 * Test plugin cleanup function
 */
static void test_plugin_cleanup(plugin_descriptor_t* plugin) {
    test_cleanup_called++;
}

/**
 * Test plugin optional function
 */
static int test_plugin_function(void* context, void* params) {
    test_function_called++;
    return PLUGIN_SUCCESS;
}

/**
 * Test plugin that fails initialization
 */
static int test_plugin_fail_init(plugin_descriptor_t* plugin) {
    return PLUGIN_ERROR_INIT_FAILED;
}

/**
 * Test plugin registration
 */
static void test_plugin_registration(void) {
    vga_write("\n=== Testing Plugin Registration ===\n");
    
    /* Create test plugin using macro */
    DEFINE_PLUGIN(test_plugin, "Test Plugin", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_NORMAL, test_plugin_init, test_plugin_cleanup, test_plugin_function);
    
    /* Test registration */
    int result = plugin_register(&test_plugin);
    if (result == PLUGIN_SUCCESS) {
        vga_write("Plugin registration: PASS\n");
    } else {
        vga_write("Plugin registration: FAIL\n");
    }
    
    /* Test finding plugin */
    plugin_descriptor_t* found = plugin_find("Test Plugin");
    if (found && found == &test_plugin) {
        vga_write("Plugin find: PASS\n");
    } else {
        vga_write("Plugin find: FAIL\n");
    }
    
    /* Test duplicate registration */
    result = plugin_register(&test_plugin);
    if (result == PLUGIN_ERROR_ALREADY_REGISTERED) {
        vga_write("Duplicate registration detection: PASS\n");
    } else {
        vga_write("Duplicate registration detection: FAIL\n");
    }
    
    /* Test unregistration */
    result = plugin_unregister("Test Plugin");
    if (result == PLUGIN_SUCCESS) {
        vga_write("Plugin unregistration: PASS\n");
    } else {
        vga_write("Plugin unregistration: FAIL\n");
    }
    
    /* Verify plugin is gone */
    found = plugin_find("Test Plugin");
    if (found == NULL) {
        vga_write("Plugin removed verification: PASS\n");
    } else {
        vga_write("Plugin removed verification: FAIL\n");
    }
}

/**
 * Test plugin initialization
 */
static void test_plugin_initialization(void) {
    vga_write("\n=== Testing Plugin Initialization ===\n");
    
    /* Reset counters */
    test_init_called = 0;
    test_cleanup_called = 0;
    test_function_called = 0;
    
    /* Create and register test plugin using macro */
    DEFINE_PLUGIN(test_plugin, "Init Test Plugin", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_NORMAL, test_plugin_init, test_plugin_cleanup, test_plugin_function);
    
    plugin_register(&test_plugin);
    
    /* Initialize plugin */
    int result = plugin_init_all();
    if (result == PLUGIN_SUCCESS && test_init_called == 1) {
        vga_write("Plugin initialization: PASS\n");
    } else {
        vga_write("Plugin initialization: FAIL\n");
    }
    
    /* Test calling plugin function */
    result = plugin_call("Init Test Plugin", &test_plugin, NULL);
    if (result == PLUGIN_SUCCESS && test_function_called == 1) {
        vga_write("Plugin function call: PASS\n");
    } else {
        vga_write("Plugin function call: FAIL\n");
    }
    
    /* Test cleanup */
    plugin_cleanup_all();
    if (test_cleanup_called == 1) {
        vga_write("Plugin cleanup: PASS\n");
    } else {
        vga_write("Plugin cleanup: FAIL\n");
    }
    
    /* Cleanup */
    plugin_unregister("Init Test Plugin");
}

/**
 * Test plugin priority ordering
 */
static void test_plugin_priority(void) {
    vga_write("\n=== Testing Plugin Priority ===\n");
    
    /* Create plugins with different priorities using macros */
    DEFINE_PLUGIN(low_priority, "Low Priority", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_LOW, test_plugin_init, NULL, NULL);
    
    DEFINE_PLUGIN(high_priority, "High Priority", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_HIGH, test_plugin_init, NULL, NULL);
    
    DEFINE_PLUGIN(critical_priority, "Critical Priority", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_CRITICAL, test_plugin_init, NULL, NULL);
    
    /* Register in wrong order */
    plugin_register(&low_priority);
    plugin_register(&high_priority);
    plugin_register(&critical_priority);
    
    /* Check that count is correct */
    int count = plugin_get_count();
    if (count == 3) {
        vga_write("Plugin count: PASS (");
        vga_write_dec(count);
        vga_write(")\n");
    } else {
        vga_write("Plugin count: FAIL\n");
    }
    
    /* List plugins to verify order */
    vga_write("Plugin order verification:\n");
    plugin_list_all();
    
    /* Cleanup */
    plugin_unregister("Low Priority");
    plugin_unregister("High Priority");
    plugin_unregister("Critical Priority");
}

/**
 * Test plugin failure handling
 */
static void test_plugin_failure_handling(void) {
    vga_write("\n=== Testing Plugin Failure Handling ===\n");
    
    /* Create plugin that fails initialization using macro */
    DEFINE_PLUGIN(fail_plugin, "Fail Plugin", 1, 0, PLUGIN_TYPE_OTHER,
                  PLUGIN_PRIORITY_OPTIONAL, test_plugin_fail_init, NULL, NULL);
    
    plugin_register(&fail_plugin);
    
    /* Try to initialize - should handle failure gracefully for optional plugins */
    int result = plugin_init_all();
    if (result == PLUGIN_ERROR) {
        vga_write("Optional plugin failure handling: PASS\n");
    } else {
        vga_write("Optional plugin failure handling: FAIL\n");
    }
    
    /* Test calling function on non-initialized plugin */
    result = plugin_call("Fail Plugin", &fail_plugin, NULL);
    if (result == PLUGIN_ERROR) {
        vga_write("Call on failed plugin: PASS\n");
    } else {
        vga_write("Call on failed plugin: FAIL\n");
    }
    
    /* Cleanup */
    plugin_unregister("Fail Plugin");
}

/**
 * Test API version checking
 */
static void test_api_version(void) {
    vga_write("\n=== Testing API Version Checking ===\n");
    
    /* Create plugin with wrong API version - must manually initialize */
    plugin_descriptor_t bad_version;
    
    /* Copy name */
    const char* name = "Bad Version Plugin";
    for (int i = 0; i < PLUGIN_NAME_MAX && name[i] != '\0'; i++) {
        bad_version.name[i] = name[i];
    }
    bad_version.name[PLUGIN_NAME_MAX-1] = '\0';
    
    bad_version.version_major = 1;
    bad_version.version_minor = 0;
    bad_version.type = PLUGIN_TYPE_OTHER;
    bad_version.priority = PLUGIN_PRIORITY_NORMAL;
    bad_version.api_version_major = 99;  /* Wrong major version */
    bad_version.api_version_minor = 0;
    
    /* Initialize security fields */
    for (int i = 0; i < PLUGIN_SIGNATURE_SIZE; i++) {
        bad_version.signature[i] = 0;
    }
    bad_version.permissions = PLUGIN_PERM_NONE;
    bad_version.interference_flags = PLUGIN_INTERFERE_NONE;
    bad_version.verified = 0;
    
    bad_version.init = test_plugin_init;
    bad_version.cleanup = NULL;
    bad_version.function = NULL;
    bad_version.private_data = NULL;
    bad_version.next = NULL;
    bad_version.loaded = 0;
    
    /* Try to register - should fail */
    int result = plugin_register(&bad_version);
    if (result == PLUGIN_ERROR_INVALID_VERSION) {
        vga_write("API version validation: PASS\n");
    } else {
        vga_write("API version validation: FAIL\n");
    }
}

/**
 * Run all plugin tests
 */
void run_plugin_tests(void) {
    vga_write("\n========================================\n");
    vga_write("    PLUGIN SYSTEM TEST SUITE\n");
    vga_write("========================================\n");
    
    test_plugin_registration();
    test_plugin_initialization();
    test_plugin_priority();
    test_plugin_failure_handling();
    test_api_version();
    
    vga_write("\n========================================\n");
    vga_write("    PLUGIN TESTS COMPLETE\n");
    vga_write("========================================\n\n");
}
