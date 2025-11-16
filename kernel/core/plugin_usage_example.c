/**
 * Aurora OS - Plugin Usage Example
 * 
 * Demonstrates how to use the optional plugin system with ML,
 * Quantum Computing, and System Optimization plugins.
 */

#include "plugin.h"
#include "../drivers/vga.h"

/**
 * Example: Using ML Optimization Plugin
 */
void example_ml_optimization(void) {
    vga_write("\n=== ML Optimization Plugin Example ===\n");
    
    /* Check if plugin is available */
    if (!plugin_is_enabled("ML Optimization")) {
        vga_write("ML Optimization plugin is not enabled\n");
        return;
    }
    
    /* Configure the plugin */
    plugin_set_config("ML Optimization", "learning_enabled", "1");
    
    /* Add performance samples */
    uint32_t params[2];
    params[0] = 0;  /* Action: add_sample */
    params[1] = 0x0050003C;  /* CPU: 60%, Memory: 80% */
    
    plugin_call("ML Optimization", NULL, params);
    
    /* Train the models */
    params[0] = 1;  /* Action: train */
    params[1] = 0;
    plugin_call("ML Optimization", NULL, params);
    
    /* Get predictions */
    params[0] = 2;  /* Action: predict */
    params[1] = 0;
    plugin_call("ML Optimization", NULL, params);
    
    /* Get optimization suggestions */
    params[0] = 3;  /* Action: get_suggestion */
    params[1] = 0;
    plugin_call("ML Optimization", NULL, params);
    
    vga_write("ML Optimization example complete\n");
}

/**
 * Example: Using Quantum Computing Plugin
 */
void example_quantum_computing(void) {
    vga_write("\n=== Quantum Computing Plugin Example ===\n");
    
    /* Check if plugin is available */
    if (!plugin_is_enabled("Quantum Computing")) {
        vga_write("Quantum Computing plugin is not enabled\n");
        return;
    }
    
    /* Configure qubit count */
    plugin_set_config("Quantum Computing", "qubit_count", "32");
    
    uint32_t params[2];
    
    /* Run Grover's search algorithm */
    params[0] = 0;  /* Operation: grover_search */
    params[1] = 42; /* Target value */
    plugin_call("Quantum Computing", NULL, params);
    
    /* Perform Quantum Fourier Transform */
    params[0] = 1;  /* Operation: qft */
    params[1] = 0;
    plugin_call("Quantum Computing", NULL, params);
    
    /* Generate quantum random number */
    params[0] = 2;  /* Operation: random */
    params[1] = 0;
    plugin_call("Quantum Computing", NULL, params);
    
    /* Compute quantum hash */
    params[0] = 3;  /* Operation: hash */
    params[1] = 0;
    plugin_call("Quantum Computing", NULL, params);
    
    /* Entangle qubits */
    params[0] = 4;  /* Operation: entangle */
    params[1] = 0x0305;  /* Entangle qubits 5 and 3 */
    plugin_call("Quantum Computing", NULL, params);
    
    vga_write("Quantum Computing example complete\n");
}

/**
 * Example: Using System Optimization Plugin
 */
void example_system_optimization(void) {
    vga_write("\n=== System Optimization Plugin Example ===\n");
    
    /* Check if plugin is available */
    if (!plugin_is_enabled("System Optimization")) {
        vga_write("System Optimization plugin is not enabled\n");
        return;
    }
    
    uint32_t params[2];
    
    /* Monitor system resources */
    params[0] = 0;  /* Operation: monitor */
    params[1] = 0;
    plugin_call("System Optimization", NULL, params);
    
    /* Optimize RAM */
    params[0] = 1;  /* Operation: optimize_ram */
    params[1] = 0;
    plugin_call("System Optimization", NULL, params);
    
    /* Optimize filesystem */
    params[0] = 2;  /* Operation: optimize_fs */
    params[1] = 0;
    plugin_call("System Optimization", NULL, params);
    
    /* Collect user behavior data */
    params[0] = 3;  /* Operation: collect_behavior */
    params[1] = 0;
    plugin_call("System Optimization", NULL, params);
    
    /* Get optimization score */
    params[0] = 4;  /* Operation: get_score */
    params[1] = 0;
    plugin_call("System Optimization", NULL, params);
    
    vga_write("System Optimization example complete\n");
}

/**
 * Example: Managing plugins at runtime
 */
void example_plugin_management(void) {
    vga_write("\n=== Plugin Management Example ===\n");
    
    /* List all registered plugins */
    plugin_list_all();
    
    /* Get plugin count */
    int count = plugin_get_count();
    vga_write("\nTotal plugins: ");
    vga_write_dec(count);
    vga_write("\n");
    
    /* Disable a plugin */
    vga_write("\nDisabling ML Optimization plugin...\n");
    plugin_disable("ML Optimization");
    
    /* Check if plugin is enabled */
    if (!plugin_is_enabled("ML Optimization")) {
        vga_write("ML Optimization is now disabled\n");
    }
    
    /* Re-enable the plugin */
    vga_write("Re-enabling ML Optimization plugin...\n");
    plugin_enable("ML Optimization");
    
    if (plugin_is_enabled("ML Optimization")) {
        vga_write("ML Optimization is now enabled\n");
    }
    
    /* Get configuration value */
    const char* value = plugin_get_config("Quantum Computing", "qubit_count");
    if (value) {
        vga_write("Quantum Computing qubit_count: ");
        vga_write(value);
        vga_write("\n");
    }
    
    vga_write("Plugin management example complete\n");
}

/**
 * Run all plugin examples
 */
void run_plugin_examples(void) {
    vga_write("\n");
    vga_write("========================================\n");
    vga_write("  Optional Plugin System Examples\n");
    vga_write("========================================\n");
    
    /* Run individual examples */
    example_ml_optimization();
    example_quantum_computing();
    example_system_optimization();
    example_plugin_management();
    
    vga_write("\n========================================\n");
    vga_write("  All examples completed successfully!\n");
    vga_write("========================================\n\n");
}
