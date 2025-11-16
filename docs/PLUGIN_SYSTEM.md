# Aurora OS - Plugin System Developer Guide

## Overview

The Aurora OS plugin system allows external modules to extend kernel and bootloader functionality without modifying the core codebase. Plugins can provide optional functions that are loaded and initialized during the boot sequence.

## Features

- **Priority-based initialization**: Plugins are initialized in priority order (critical → optional)
- **API versioning**: Automatic compatibility checking between plugin and kernel
- **Lifecycle management**: Plugins have init, cleanup, and optional function callbacks
- **Type classification**: Plugins can be drivers, filesystems, diagnostics, security, etc.
- **Failure handling**: Optional plugins can fail without preventing system boot
- **Dynamic registration**: Plugins register themselves during boot

## Plugin Types

| Type | Description | Example Use Case |
|------|-------------|------------------|
| `PLUGIN_TYPE_DRIVER` | Hardware driver plugin | Custom device drivers |
| `PLUGIN_TYPE_FILESYSTEM` | Filesystem plugin | New filesystem support |
| `PLUGIN_TYPE_BOOT_SETUP` | Boot-time setup plugin | Platform-specific initialization |
| `PLUGIN_TYPE_DIAGNOSTICS` | Diagnostic/testing plugin | Boot-time system checks |
| `PLUGIN_TYPE_SECURITY` | Security extension plugin | Additional encryption/authentication |
| `PLUGIN_TYPE_NETWORK` | Network protocol plugin | Custom network protocols |
| `PLUGIN_TYPE_OTHER` | Other/custom plugin type | General purpose plugins |

## Plugin Priority Levels

Plugins are initialized in priority order:

1. `PLUGIN_PRIORITY_CRITICAL` - Must load first, failure stops boot
2. `PLUGIN_PRIORITY_HIGH` - Important but can continue on failure
3. `PLUGIN_PRIORITY_NORMAL` - Standard plugins
4. `PLUGIN_PRIORITY_LOW` - Non-essential plugins
5. `PLUGIN_PRIORITY_OPTIONAL` - Can fail without affecting boot

## Creating a Plugin

### 1. Include the Plugin Header

```c
#include "kernel/core/plugin.h"
#include "kernel/drivers/vga.h"  // For output
```

### 2. Implement Lifecycle Functions

```c
/**
 * Plugin initialization - called during boot
 * Return PLUGIN_SUCCESS on success, PLUGIN_ERROR on failure
 */
static int my_plugin_init(plugin_descriptor_t* plugin) {
    vga_write("My Plugin: Initializing...\n");
    
    // Your initialization code here
    // Allocate resources, configure hardware, etc.
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin cleanup - called on shutdown
 */
static void my_plugin_cleanup(plugin_descriptor_t* plugin) {
    vga_write("My Plugin: Cleaning up...\n");
    
    // Your cleanup code here
    // Free resources, reset hardware, etc.
}

/**
 * Optional function - can be called by kernel
 * Return PLUGIN_SUCCESS on success, PLUGIN_ERROR on failure
 */
static int my_plugin_function(void* context, void* params) {
    // Your optional function code here
    // This can be called anytime after initialization
    
    return PLUGIN_SUCCESS;
}
```

### 3. Define the Plugin Descriptor

Use the `DEFINE_PLUGIN` macro for convenience:

```c
DEFINE_PLUGIN(
    my_plugin,                    // Variable name
    "My Custom Plugin",           // Plugin name (max 64 chars)
    1,                            // Major version
    0,                            // Minor version
    PLUGIN_TYPE_DRIVER,           // Plugin type
    PLUGIN_PRIORITY_NORMAL,       // Priority level
    my_plugin_init,               // Init function
    my_plugin_cleanup,            // Cleanup function
    my_plugin_function            // Optional function (or NULL)
);
```

### 4. Create a Registration Function

```c
/**
 * Register the plugin with the system
 */
void register_my_plugin(void) {
    plugin_register(&my_plugin);
}
```

### 5. Register During Boot

In `kernel/core/kernel.c`, add your plugin registration:

```c
/* External plugin registration functions */
extern void register_my_plugin(void);

// In kernel_init():
plugin_system_init();
register_my_plugin();  // Add this line
plugin_init_all();
```

## Using Plugin Private Data

Plugins can store private data:

```c
typedef struct {
    int some_value;
    void* some_pointer;
} my_plugin_data_t;

static int my_plugin_init(plugin_descriptor_t* plugin) {
    // Allocate private data
    my_plugin_data_t* data = kmalloc(sizeof(my_plugin_data_t));
    if (!data) {
        return PLUGIN_ERROR;
    }
    
    data->some_value = 42;
    plugin->private_data = data;
    
    return PLUGIN_SUCCESS;
}

static void my_plugin_cleanup(plugin_descriptor_t* plugin) {
    if (plugin->private_data) {
        kfree(plugin->private_data);
        plugin->private_data = NULL;
    }
}
```

## Calling Plugin Functions

The kernel or other code can call a plugin's optional function:

```c
// Call plugin by name
int result = plugin_call("My Custom Plugin", context_data, parameter_data);
if (result == PLUGIN_SUCCESS) {
    vga_write("Plugin function executed successfully\n");
} else if (result == PLUGIN_ERROR_NOT_FOUND) {
    vga_write("Plugin not found\n");
} else {
    vga_write("Plugin function failed\n");
}
```

## Plugin API Functions

| Function | Description |
|----------|-------------|
| `plugin_system_init()` | Initialize the plugin system |
| `plugin_register(plugin)` | Register a plugin |
| `plugin_unregister(name)` | Unregister a plugin |
| `plugin_find(name)` | Find a plugin by name |
| `plugin_init_all()` | Initialize all registered plugins |
| `plugin_cleanup_all()` | Cleanup all plugins |
| `plugin_call(name, ctx, params)` | Call a plugin's optional function |
| `plugin_list_all()` | List all registered plugins (debug) |
| `plugin_get_count()` | Get number of registered plugins |

## Error Codes

| Code | Description |
|------|-------------|
| `PLUGIN_SUCCESS` | Operation successful |
| `PLUGIN_ERROR` | Generic error |
| `PLUGIN_ERROR_INVALID_VERSION` | API version mismatch |
| `PLUGIN_ERROR_INIT_FAILED` | Plugin initialization failed |
| `PLUGIN_ERROR_NOT_FOUND` | Plugin not found |
| `PLUGIN_ERROR_ALREADY_REGISTERED` | Plugin already registered |

## Example Plugins

### Example 1: Boot Diagnostic Plugin

See `kernel/core/plugin_example.c` for a complete example that:
- Performs memory allocation tests during boot
- Tests serial port functionality
- Tracks diagnostic statistics
- Provides runtime diagnostic capability

### Example 2: Hardware Setup Plugin

See `kernel/core/plugin_hwsetup.c` for a platform-specific hardware initialization example that:
- Runs early in boot (HIGH priority)
- Configures custom hardware
- Sets up vendor-specific features

## Best Practices

1. **Use appropriate priority**: Critical plugins should use `PLUGIN_PRIORITY_CRITICAL` or `PLUGIN_PRIORITY_HIGH`
2. **Handle failures gracefully**: Return appropriate error codes and clean up resources
3. **Keep init fast**: Boot plugins should initialize quickly to avoid delaying boot
4. **Document your plugin**: Add comments explaining what your plugin does
5. **Test thoroughly**: Test both successful and failure scenarios
6. **Use version numbers**: Increment version numbers when making changes
7. **Clean up resources**: Always implement cleanup function to free resources

## Building with Plugins

Plugins are automatically compiled when you build the kernel:

```bash
make clean
make all
```

The plugin object files are linked into the kernel binary.

## Testing

Run the plugin test suite to verify functionality:

```bash
make test
```

The tests validate:
- Plugin registration and unregistration
- Plugin initialization and cleanup
- Priority ordering
- Failure handling
- API version checking
- Function calls

## Troubleshooting

### Plugin not loading

- Check that `plugin_register()` is called during boot
- Verify API version matches (`PLUGIN_API_VERSION_MAJOR`)
- Check plugin priority (critical plugins must succeed)
- Look for error messages in boot output

### Plugin initialization fails

- Check return value from init function
- Verify all required resources are available
- Check for memory allocation failures
- Review error messages

### Plugin function not called

- Ensure plugin was initialized successfully (check `loaded` flag)
- Verify plugin name matches exactly
- Check that function pointer is not NULL

## Advanced Topics

### Dynamic Plugin Loading

Currently, plugins are statically linked into the kernel. Future enhancements could include:
- Loading plugins from filesystem
- Hot-pluggable modules
- Runtime plugin management

### Plugin Dependencies

Plugins can depend on other plugins by checking for their presence:

```c
static int my_plugin_init(plugin_descriptor_t* plugin) {
    // Check if required plugin exists
    plugin_descriptor_t* required = plugin_find("Required Plugin");
    if (!required || !required->loaded) {
        vga_write("ERROR: Required plugin not loaded\n");
        return PLUGIN_ERROR;
    }
    
    // Continue initialization...
}
```

## API Reference

For complete API documentation, see:
- `kernel/core/plugin.h` - Plugin API header with structure definitions
- `kernel/core/plugin.c` - Plugin manager implementation

## License

See main Aurora OS license for terms and conditions.
