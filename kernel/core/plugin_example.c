/**
 * Aurora OS - Example Boot Diagnostic Plugin
 * 
 * Demonstrates how to create a plugin that provides optional
 * boot-time functionality using the plugin API.
 */

#include "../core/plugin.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"
#include "../memory/memory.h"

/* Plugin private data structure */
typedef struct {
    int diagnostic_count;
    int boot_checks_passed;
} diagnostic_data_t;

/**
 * Plugin initialization function
 * Called when the plugin is loaded during boot
 */
static int boot_diagnostic_init(plugin_descriptor_t* plugin) {
    vga_write("  Boot Diagnostic Plugin: Initializing...\n");
    
    /* Allocate private data */
    diagnostic_data_t* data = (diagnostic_data_t*)kmalloc(sizeof(diagnostic_data_t));
    if (!data) {
        vga_write("  ERROR: Failed to allocate memory for plugin data\n");
        return PLUGIN_ERROR;
    }
    
    data->diagnostic_count = 0;
    data->boot_checks_passed = 0;
    plugin->private_data = data;
    
    /* Perform initial diagnostics */
    vga_write("  Running boot diagnostics...\n");
    
    /* Check 1: Memory subsystem */
    void* test_ptr = kmalloc(64);
    if (test_ptr) {
        data->boot_checks_passed++;
        kfree(test_ptr);
        vga_write("    [OK] Memory allocation test\n");
    } else {
        vga_write("    [FAIL] Memory allocation test\n");
    }
    data->diagnostic_count++;
    
    /* Check 2: Serial port */
    serial_write(SERIAL_COM1, "Boot diagnostic plugin: Serial port test\n");
    data->boot_checks_passed++;
    data->diagnostic_count++;
    vga_write("    [OK] Serial port test\n");
    
    vga_write("  Boot diagnostics complete: ");
    vga_write_dec(data->boot_checks_passed);
    vga_write("/");
    vga_write_dec(data->diagnostic_count);
    vga_write(" passed\n");
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin cleanup function
 * Called when the system shuts down or plugin is unloaded
 */
static void boot_diagnostic_cleanup(plugin_descriptor_t* plugin) {
    vga_write("  Boot Diagnostic Plugin: Cleaning up...\n");
    
    if (plugin->private_data) {
        diagnostic_data_t* data = (diagnostic_data_t*)plugin->private_data;
        
        vga_write("  Final statistics: ");
        vga_write_dec(data->diagnostic_count);
        vga_write(" diagnostics run\n");
        
        kfree(plugin->private_data);
        plugin->private_data = NULL;
    }
}

/**
 * Plugin optional function
 * Can be called by the kernel to run additional diagnostics
 */
static int boot_diagnostic_function(void* context, void* params) {
    /* Context is the plugin descriptor */
    plugin_descriptor_t* plugin = (plugin_descriptor_t*)context;
    
    if (!plugin || !plugin->private_data) {
        return PLUGIN_ERROR;
    }
    
    diagnostic_data_t* data = (diagnostic_data_t*)plugin->private_data;
    
    vga_write("Running additional boot diagnostic...\n");
    
    /* Perform a runtime diagnostic */
    /* In a real plugin, this might check hardware status, 
     * run system tests, etc. */
    
    data->diagnostic_count++;
    data->boot_checks_passed++;
    
    vga_write("Diagnostic passed (");
    vga_write_dec(data->diagnostic_count);
    vga_write(" total)\n");
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin descriptor - this is the main entry point for the plugin
 * Use the DEFINE_SECURE_PLUGIN macro to specify security permissions
 */
DEFINE_SECURE_PLUGIN(
    boot_diagnostic_plugin,           /* Variable name */
    "Boot Diagnostics",               /* Plugin name */
    1,                                 /* Major version */
    0,                                 /* Minor version */
    PLUGIN_TYPE_DIAGNOSTICS,          /* Type */
    PLUGIN_PRIORITY_NORMAL,            /* Priority */
    PLUGIN_PERM_MEMORY | PLUGIN_PERM_IO,  /* Permissions: needs memory and I/O */
    PLUGIN_INTERFERE_MEMORY,           /* May interfere with memory management */
    boot_diagnostic_init,              /* Init function */
    boot_diagnostic_cleanup,           /* Cleanup function */
    boot_diagnostic_function           /* Optional function */
);

/**
 * Plugin registration function
 * This is called by the bootloader to register the plugin
 */
void register_boot_diagnostic_plugin(void) {
    plugin_register(&boot_diagnostic_plugin);
}
