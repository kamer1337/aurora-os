/**
 * Aurora OS - Example Hardware Setup Plugin
 * 
 * Demonstrates a plugin that performs platform-specific
 * hardware initialization during boot.
 */

#include "../core/plugin.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"

/**
 * Plugin initialization function
 * Performs custom hardware setup
 */
static int hw_setup_init(plugin_descriptor_t* plugin) {
    vga_write("  Hardware Setup Plugin: Initializing...\n");
    
    /* In a real plugin, this might:
     * - Initialize custom hardware devices
     * - Configure platform-specific features
     * - Set up vendor-specific extensions
     * - Calibrate hardware timings
     */
    
    vga_write("    Checking for custom hardware...\n");
    vga_write("    Configuring platform features...\n");
    vga_write("    Hardware setup complete\n");
    
    serial_write(SERIAL_COM1, "Hardware setup plugin initialized\n");
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin cleanup function
 */
static void hw_setup_cleanup(plugin_descriptor_t* plugin) {
    vga_write("  Hardware Setup Plugin: Cleaning up...\n");
    serial_write(SERIAL_COM1, "Hardware setup plugin cleanup\n");
}

/**
 * Plugin optional function
 * Provides hardware reconfiguration capability
 */
static int hw_setup_function(void* context, void* params) {
    vga_write("Reconfiguring hardware settings...\n");
    
    /* In a real plugin, this might reconfigure hardware
     * based on runtime parameters */
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin descriptor - demonstrates security setup
 */
DEFINE_SECURE_PLUGIN(
    hw_setup_plugin,                   /* Variable name */
    "Hardware Setup",                  /* Plugin name */
    1,                                  /* Major version */
    0,                                  /* Minor version */
    PLUGIN_TYPE_BOOT_SETUP,            /* Type */
    PLUGIN_PRIORITY_HIGH,               /* Priority - runs early */
    PLUGIN_PERM_IO | PLUGIN_PERM_KERNEL,  /* Permissions: I/O and kernel access */
    PLUGIN_INTERFERE_NONE,              /* No interference with critical systems */
    hw_setup_init,                      /* Init function */
    hw_setup_cleanup,                   /* Cleanup function */
    hw_setup_function                   /* Optional function */
);

/**
 * Plugin registration function
 */
void register_hw_setup_plugin(void) {
    plugin_register(&hw_setup_plugin);
}
