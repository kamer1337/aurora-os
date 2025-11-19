# Aurora OS - Graphics Drivers and ML Optimization Build Guide

## Overview

Aurora OS now includes comprehensive graphics driver support and optional machine learning optimization plugins that can enhance both processor and GPU performance.

## Graphics Drivers

### Supported GPUs

Aurora OS includes drivers for the following GPU vendors:

- **Intel HD Graphics** - Support for Intel integrated GPUs
- **NVIDIA** - Support for NVIDIA discrete GPUs
- **AMD** - Support for AMD discrete GPUs

### Driver Integration

Graphics drivers are automatically detected and initialized during boot through the driver manager. The system will:

1. Detect available GPU hardware via PCI enumeration
2. Identify the GPU vendor
3. Initialize the appropriate driver
4. Enable hardware acceleration when available

## Machine Learning Optimization Plugins

Aurora OS includes two optional ML optimization plugins:

### 1. Processor ML Optimization (plugin_ml_optimization)

**Purpose**: Optimizes CPU and memory performance using machine learning.

**Features**:
- Performance metrics collection (CPU, memory)
- User behavior pattern tracking
- Trend prediction using linear regression
- Adaptive optimization recommendations

**When to use**: Enable for workloads that benefit from CPU and memory optimization.

### 2. Graphics ML Optimization (plugin_gfx_ml_optimization)

**Purpose**: Optimizes GPU rendering performance using machine learning.

**Features**:
- GPU performance metrics collection
- Frame time tracking and prediction
- Adaptive rendering quality adjustment
- Support for multiple GPU vendors
- Automatic quality scaling based on performance
- Target FPS management

**When to use**: Enable for graphics-intensive workloads that require adaptive quality scaling.

## Build Configuration

### Building with All Plugins (Default)

By default, all optional plugins are enabled:

```bash
make clean
make all
```

This is equivalent to:

```bash
make ENABLE_ML_PLUGIN=1 ENABLE_QUANTUM_PLUGIN=1 ENABLE_SYSTEM_OPT_PLUGIN=1 all
```

### Building without ML Plugins

To build without machine learning optimization plugins:

```bash
make ENABLE_ML_PLUGIN=0 all
```

### Building without Any Optional Plugins

To build a minimal kernel without optional plugins:

```bash
make ENABLE_ML_PLUGIN=0 ENABLE_QUANTUM_PLUGIN=0 ENABLE_SYSTEM_OPT_PLUGIN=0 all
```

Or use the convenient target:

```bash
make plugins-disable-all
```

### Available Build Targets

- `make plugins` - Show plugin status
- `make plugins-disable-ml` - Build without ML plugin
- `make plugins-disable-quantum` - Build without Quantum plugin
- `make plugins-disable-sysopt` - Build without System Optimization plugin
- `make plugins-disable-all` - Build without any optional plugins

## Runtime Configuration

### Enabling/Disabling Plugins at Runtime

Plugins can be controlled at runtime:

```c
// Disable a plugin
plugin_disable("ML Optimization");

// Re-enable a plugin
plugin_enable("ML Optimization");

// Check if enabled
if (plugin_is_enabled("GFX ML Optimization")) {
    // Plugin is enabled
}
```

### Configuring ML Plugins

Both ML plugins support configuration:

```c
// Configure processor ML plugin
plugin_set_config("ML Optimization", "learning_enabled", "1");

// Configure GFX ML plugin
plugin_set_config("GFX ML Optimization", "adaptive_quality", "1");
plugin_set_config("GFX ML Optimization", "learning_enabled", "1");
```

### Using GFX ML Plugin

Example usage for graphics optimization:

```c
uint32_t params[2];

// Add GPU performance sample
params[0] = 0;  // Action: Add sample
params[1] = (frame_time_us << 16) | gpu_usage_percent;
plugin_call("GFX ML Optimization", NULL, params);

// Train ML models
params[0] = 1;  // Action: Train
plugin_call("GFX ML Optimization", NULL, params);

// Get performance predictions
params[0] = 2;  // Action: Predict
plugin_call("GFX ML Optimization", NULL, params);

// Get suggested rendering quality
params[0] = 3;  // Action: Get quality
plugin_call("GFX ML Optimization", NULL, params);

// Set target FPS
params[0] = 4;  // Action: Set target FPS
params[1] = 200;  // Target: 200 FPS
plugin_call("GFX ML Optimization", NULL, params);
```

## Performance Considerations

### With ML Plugins Enabled

- Slightly increased boot time due to plugin initialization
- Small memory overhead for ML data structures
- Runtime overhead depends on usage frequency
- Benefits include adaptive performance optimization

### Without ML Plugins

- Faster boot time
- Lower memory footprint
- No ML-based optimizations
- Static performance characteristics

## Recommendations

### For Desktop/Gaming Systems

Enable all plugins, especially GFX ML Optimization:

```bash
make all  # Default build includes all plugins
```

### For Servers

Consider disabling GFX ML if no GUI is needed:

```bash
# Keep processor ML for CPU optimization
make ENABLE_ML_PLUGIN=1 ENABLE_QUANTUM_PLUGIN=1 ENABLE_SYSTEM_OPT_PLUGIN=1 all
```

### For Embedded/Minimal Systems

Disable all optional plugins for minimal footprint:

```bash
make plugins-disable-all
```

## Troubleshooting

### GPU Not Detected

If GPU drivers don't initialize:
1. Check PCI device enumeration in boot logs
2. Verify GPU is present and enabled in BIOS/UEFI
3. Check driver_manager output during boot

### ML Plugin Not Working

If ML plugins don't function:
1. Verify plugin is enabled: `plugin_is_enabled("GFX ML Optimization")`
2. Check configuration: `plugin_get_config("GFX ML Optimization", "learning_enabled")`
3. Review plugin initialization messages in boot logs

### Build Errors

If build fails:
1. Run `make clean` first
2. Verify all source files are present
3. Check compiler version (GCC recommended)
4. Review build output for specific errors

## See Also

- `docs/OPTIONAL_PLUGIN_IMPLEMENTATION.md` - Complete plugin system documentation
- `docs/OPTIONAL_PLUGINS_QUICKREF.md` - Quick reference for plugin operations
- `docs/OPTIONAL_PLUGIN_SYSTEM.md` - Detailed API reference
- `kernel/core/plugin_usage_example.c` - Example code for plugin usage
