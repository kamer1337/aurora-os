# Optional Plugin System - Quick Reference

## Building with Plugins

### Makefile
```bash
# Build with all plugins (default)
make all

# Disable specific plugins
make ENABLE_ML_PLUGIN=0 all
make ENABLE_QUANTUM_PLUGIN=0 all
make ENABLE_SYSTEM_OPT_PLUGIN=0 all

# Disable all optional plugins
make plugins-disable-all

# Show plugin build status
make plugins
```

### CMake
```bash
# Configure with plugins
cmake -DENABLE_ML_PLUGIN=ON -DENABLE_QUANTUM_PLUGIN=ON ..

# Build
cmake --build .

# Show plugin info
cmake --build . --target plugins-info
```

## Plugin Operations

### ML Optimization Plugin

```c
uint32_t params[2];

// Add sample: params[1] = (cpu_usage << 16) | memory_usage
params[0] = 0; params[1] = 0x0050003C;
plugin_call("ML Optimization", NULL, params);

// Train models
params[0] = 1; params[1] = 0;
plugin_call("ML Optimization", NULL, params);

// Get predictions
params[0] = 2; params[1] = 0;
plugin_call("ML Optimization", NULL, params);

// Get suggestions
params[0] = 3; params[1] = 0;
plugin_call("ML Optimization", NULL, params);
```

### Quantum Computing Plugin

```c
uint32_t params[2];

// Grover's search: params[1] = target_value
params[0] = 0; params[1] = 42;
plugin_call("Quantum Computing", NULL, params);

// Quantum Fourier Transform
params[0] = 1; params[1] = 0;
plugin_call("Quantum Computing", NULL, params);

// Generate random number
params[0] = 2; params[1] = 0;
plugin_call("Quantum Computing", NULL, params);

// Compute hash
params[0] = 3; params[1] = 0;
plugin_call("Quantum Computing", NULL, params);

// Entangle: params[1] = (qubit1 << 8) | qubit2
params[0] = 4; params[1] = 0x0305;
plugin_call("Quantum Computing", NULL, params);
```

### System Optimization Plugin

```c
uint32_t params[2];

// Monitor system
params[0] = 0; params[1] = 0;
plugin_call("System Optimization", NULL, params);

// Optimize RAM
params[0] = 1; params[1] = 0;
plugin_call("System Optimization", NULL, params);

// Optimize filesystem
params[0] = 2; params[1] = 0;
plugin_call("System Optimization", NULL, params);

// Collect behavior data
params[0] = 3; params[1] = 0;
plugin_call("System Optimization", NULL, params);

// Get optimization score
params[0] = 4; params[1] = 0;
plugin_call("System Optimization", NULL, params);
```

## Configuration

```c
// Set config
plugin_set_config("ML Optimization", "learning_enabled", "1");
plugin_set_config("Quantum Computing", "qubit_count", "32");

// Get config
const char* value = plugin_get_config("ML Optimization", "learning_enabled");

// Clear config
plugin_clear_config("ML Optimization");
```

## Enable/Disable

```c
// Disable plugin
plugin_disable("ML Optimization");

// Enable plugin
plugin_enable("ML Optimization");

// Check status
if (plugin_is_enabled("ML Optimization")) {
    // Plugin is enabled
}
```

## Plugin Types

- `PLUGIN_TYPE_ML_OPTIMIZATION` - Machine Learning optimization
- `PLUGIN_TYPE_QUANTUM_COMPUTE` - Quantum computing algorithms
- `PLUGIN_TYPE_SYSTEM_OPTIMIZATION` - System resource optimization

## Plugin Names

- `"ML Optimization"` - Machine Learning plugin
- `"Quantum Computing"` - Quantum computing plugin
- `"System Optimization"` - System optimization plugin

## Registration

```c
// In kernel initialization
extern void register_ml_optimization_plugin(void);
extern void register_quantum_compute_plugin(void);
extern void register_system_optimization_plugin(void);

// Register plugins
register_ml_optimization_plugin();
register_quantum_compute_plugin();
register_system_optimization_plugin();
```

## Complete Example

```c
#include "kernel/core/plugin.h"

void example_usage(void) {
    uint32_t params[2];
    
    // Configure plugins
    plugin_set_config("ML Optimization", "learning_enabled", "1");
    plugin_set_config("Quantum Computing", "qubit_count", "32");
    
    // ML: Add sample and train
    params[0] = 0; params[1] = 0x0050003C;
    plugin_call("ML Optimization", NULL, params);
    params[0] = 1; params[1] = 0;
    plugin_call("ML Optimization", NULL, params);
    
    // Quantum: Generate random number
    params[0] = 2; params[1] = 0;
    plugin_call("Quantum Computing", NULL, params);
    
    // System: Get optimization score
    params[0] = 4; params[1] = 0;
    plugin_call("System Optimization", NULL, params);
}
```

## See Also

- `docs/OPTIONAL_PLUGIN_SYSTEM.md` - Complete documentation
- `docs/PLUGIN_SYSTEM.md` - Core plugin system docs
- `kernel/core/plugin_usage_example.c` - Full working example
