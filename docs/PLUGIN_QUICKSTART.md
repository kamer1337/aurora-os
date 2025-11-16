# Plugin System - Quick Start Example

This is a minimal example showing how to create and register a plugin for Aurora OS.

## Step 1: Create Your Plugin File

Create a new file `kernel/core/my_plugin.c`:

```c
#include "../core/plugin.h"
#include "../drivers/vga.h"

/**
 * Plugin initialization
 */
static int my_plugin_init(plugin_descriptor_t* plugin) {
    vga_write("  My Plugin: Hello from boot!\n");
    return PLUGIN_SUCCESS;
}

/**
 * Plugin cleanup
 */
static void my_plugin_cleanup(plugin_descriptor_t* plugin) {
    vga_write("  My Plugin: Goodbye!\n");
}

/**
 * Optional function that can be called by kernel
 */
static int my_plugin_function(void* context, void* params) {
    vga_write("My Plugin function called!\n");
    return PLUGIN_SUCCESS;
}

/**
 * Define the plugin descriptor
 */
DEFINE_PLUGIN(
    my_plugin,                    // Variable name
    "My Plugin",                  // Plugin name
    1,                            // Major version
    0,                            // Minor version
    PLUGIN_TYPE_OTHER,            // Type
    PLUGIN_PRIORITY_NORMAL,       // Priority
    my_plugin_init,               // Init function
    my_plugin_cleanup,            // Cleanup function
    my_plugin_function            // Optional function
);

/**
 * Registration function
 */
void register_my_plugin(void) {
    plugin_register(&my_plugin);
}
```

## Step 2: Register Your Plugin

Edit `kernel/core/kernel.c`:

```c
/* Add external declaration at the top */
extern void register_my_plugin(void);

/* In kernel_init(), add after plugin_system_init() */
register_my_plugin();  // Add this line
```

## Step 3: Build and Test

```bash
make clean
make all
make test
```

You should see "My Plugin: Hello from boot!" during initialization.

## Step 4: Call Plugin Functions (Optional)

From anywhere in the kernel after plugin initialization:

```c
#include "kernel/core/plugin.h"

// Call the plugin's optional function
int result = plugin_call("My Plugin", NULL, NULL);
if (result == PLUGIN_SUCCESS) {
    vga_write("Plugin function succeeded!\n");
}
```

## More Examples

See the following files for complete examples:
- `kernel/core/plugin_example.c` - Boot diagnostic plugin
- `kernel/core/plugin_hwsetup.c` - Hardware setup plugin

## Full Documentation

See `docs/PLUGIN_SYSTEM.md` for complete API reference and advanced topics.
