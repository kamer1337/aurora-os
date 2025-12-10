# Aurora OS - Theme Plugin System Documentation

## Overview

Aurora OS features an extensible **Theme Plugin System** that allows developers to create custom themes and 5D rendering effects without modifying the core OS code. This plugin architecture enables dynamic theme loading, custom visual effects, and enhanced 5D rendering capabilities.

## Architecture

### Plugin System Components

1. **Plugin Core** (`theme_plugin.h/c`)
   - Plugin registration and lifecycle management
   - Plugin discovery and activation
   - Effect registration and rendering pipeline

2. **5D Renderer** (`gui_5d_renderer.h/c`)
   - Enhanced 5D rendering engine with plugin support
   - Depth-based effects (fog, parallax, depth-of-field)
   - Volumetric lighting and advanced effects
   - Plugin effect integration

3. **Example Plugins** (`theme_plugins_examples.h/c`)
   - Three reference implementations: Neon, Retro, Matrix
   - Demonstrates plugin API usage
   - Shows effect creation and registration

## Plugin API

### Plugin Structure

```c
typedef struct theme_plugin {
    // Metadata
    const char* name;                   // Plugin name
    const char* author;                 // Plugin author
    const char* version;                // Version string
    const char* description;            // Description
    uint32_t api_version;               // API version (use THEME_PLUGIN_API_VERSION)
    plugin_type_t type;                 // PLUGIN_TYPE_THEME, EFFECT, or COMBINED
    
    // Callbacks
    plugin_init_fn init;                // Initialization callback
    plugin_shutdown_fn shutdown;        // Shutdown callback
    plugin_get_theme_fn get_theme;      // Theme provider (optional)
    
    // Plugin data
    void* private_data;                 // Private plugin data
    plugin_effect_t* effects;           // Linked list of custom effects
} theme_plugin_t;
```

### Plugin Types

- **PLUGIN_TYPE_THEME**: Provides theme colors only
- **PLUGIN_TYPE_EFFECT**: Provides 5D rendering effects only
- **PLUGIN_TYPE_COMBINED**: Provides both theme and effects

### Plugin Lifecycle

```c
// 1. Initialize plugin system
theme_plugin_system_init();

// 2. Register plugin
theme_plugin_register(&my_plugin);

// 3. Load plugin (calls init callback)
theme_plugin_load(&my_plugin);

// 4. Activate plugin (applies theme and effects)
theme_plugin_activate(&my_plugin);

// 5. Unload plugin (calls shutdown callback)
theme_plugin_unload(&my_plugin);

// 6. Unregister plugin
theme_plugin_unregister(&my_plugin);
```

## Creating a Theme Plugin

### Step 1: Define Plugin Metadata

```c
static theme_plugin_t my_plugin = {
    .name = "MyTheme",
    .author = "Your Name",
    .version = "1.0.0",
    .description = "My custom theme",
    .api_version = THEME_PLUGIN_API_VERSION,
    .type = PLUGIN_TYPE_COMBINED,
    .status = PLUGIN_STATUS_UNLOADED,
    .init = my_plugin_init,
    .shutdown = my_plugin_shutdown,
    .get_theme = my_plugin_get_theme,
    .private_data = NULL,
    .effects = NULL,
    .next = NULL
};
```

### Step 2: Implement Callbacks

```c
// Initialize plugin
static int my_plugin_init(theme_plugin_t* plugin) {
    // Setup theme colors
    my_theme.desktop_bg_start = (color_t){40, 40, 60, 255};
    my_theme.desktop_bg_end = (color_t){80, 80, 120, 255};
    // ... setup other colors
    
    return 0;  // Success
}

// Shutdown plugin
static void my_plugin_shutdown(theme_plugin_t* plugin) {
    // Cleanup resources if needed
}

// Provide theme
static const theme_t* my_plugin_get_theme(theme_plugin_t* plugin) {
    return &my_theme;
}
```

### Step 3: Register and Activate

```c
// Register plugin
theme_plugin_register(&my_plugin);

// Activate (automatically loads if needed)
theme_plugin_activate(&my_plugin);
```

## Creating Custom 5D Effects

### Step 1: Define Effect Function

```c
static void my_effect_render(int32_t x, int32_t y, uint32_t width, uint32_t height,
                             void* params, void* user_data) {
    // Render custom effect
    color_t glow_color = {255, 100, 0, 200};
    gui_draw_glow(x, y, width, height, glow_color, 10);
}
```

### Step 2: Create Effect Structure

```c
static plugin_effect_t my_effect = {
    .name = "my_glow",
    .description = "Custom glow effect",
    .render = my_effect_render,
    .user_data = NULL,
    .next = NULL
};
```

### Step 3: Register Effect with Plugin

```c
// In plugin init callback
theme_plugin_register_effect(plugin, &my_effect);
```

### Step 4: Render Effect

```c
// Manually render effect
theme_plugin_render_effect(plugin, "my_glow", x, y, width, height, NULL);

// Or let 5D renderer handle it automatically in enhanced mode
gui_5d_renderer_set_mode(RENDER_MODE_ENHANCED);
```

## Enhanced 5D Renderer Features

### Rendering Modes

- **RENDER_MODE_STANDARD**: Standard 5D effects only
- **RENDER_MODE_ENHANCED**: Standard + plugin effects
- **RENDER_MODE_PLUGIN_ONLY**: Only plugin effects

### Depth-Based Effects

```c
// Apply depth fog (objects farther away fade into fog)
gui_5d_apply_depth_fog(x, y, width, height, depth);

// Volumetric lighting
gui_5d_apply_volumetric_light(light_x, light_y, radius, intensity, color);

// Parallax effect
gui_5d_draw_parallax(x, y, width, height, depth, offset_x, offset_y);
```

### Advanced Effects

```c
// Holographic effect with scan lines
gui_5d_draw_hologram(x, y, width, height, color, time);

// Chromatic aberration (RGB channel separation)
gui_5d_draw_chromatic_aberration(x, y, width, height, strength);

// Lens distortion
gui_5d_apply_lens_distortion(center_x, center_y, radius, strength);
```

### Layer Rendering

```c
// Create a 5D layer
render_layer_5d_t layer = {
    .x = 100,
    .y = 100,
    .width = 400,
    .height = 300,
    .depth = 0.5f,          // 0.0 = front, 1.0 = back
    .opacity = 0.9f,        // 0.0 = transparent, 1.0 = opaque
    .tint = {255, 255, 255, 255},
    .content = window_ptr
};

// Render with all depth effects
gui_5d_renderer_render_layer(&layer);
```

## Example Plugins

### Neon Theme

Bright neon colors with intense glow effects:
- Pink/cyan color scheme
- Dark backgrounds with neon accents
- Intense glow effect on UI elements

```c
theme_plugin_t* neon = theme_plugin_get_neon();
theme_plugin_activate(neon);
```

### Retro Theme

Classic 80s/90s computing aesthetic:
- Beige/gray color palette
- Traditional window appearance
- CRT scanline effect

```c
theme_plugin_t* retro = theme_plugin_get_retro();
theme_plugin_activate(retro);
```

### Matrix Theme

Green-on-black Matrix-inspired theme:
- Terminal-style appearance
- Green phosphor colors
- Digital rain effect

```c
theme_plugin_t* matrix = theme_plugin_get_matrix();
theme_plugin_activate(matrix);
```

## Plugin Development Best Practices

### 1. API Version Compatibility

Always use the current API version:
```c
.api_version = THEME_PLUGIN_API_VERSION
```

### 2. Resource Management

- Clean up resources in shutdown callback
- Use static allocation where possible
- Avoid memory leaks

### 3. Effect Performance

- Keep effect rendering fast (<5ms)
- Use caching where appropriate
- Consider screen region size limits

### 4. Theme Consistency

- Provide all required theme colors
- Ensure text is readable on backgrounds
- Test with different window configurations

### 5. Error Handling

- Return proper error codes
- Handle NULL parameters gracefully
- Validate input ranges

## Integration with Existing Systems

### Theme Manager Integration

The plugin system integrates seamlessly with the existing theme manager:

```c
// Get active plugin
theme_plugin_t* active = theme_plugin_get_active();

// Get current theme (includes plugin themes)
const theme_t* current = theme_manager_get_current();
```

### GUI Effects Integration

Plugin effects work alongside existing GUI effects:

```c
// Use existing effects
gui_draw_shadow(x, y, w, h, offset, blur);
gui_draw_gradient(x, y, w, h, color1, color2);

// Use plugin effects
gui_5d_render_plugin_effects(x, y, w, h, params);
```

### Window Manager Integration

The 5D renderer works with window rendering:

```c
// Render window with depth
render_layer_5d_t layer = {
    .x = window->bounds.x,
    .y = window->bounds.y,
    .width = window->bounds.width,
    .height = window->bounds.height,
    .depth = calculate_window_depth(window),
    .opacity = window->transparency / 100.0f,
    // ...
};
gui_5d_renderer_render_layer(&layer);
```

## Testing

Run plugin system tests:

```c
#include "tests/theme_plugin_tests.h"

int main() {
    int failures = theme_plugin_tests_run_all();
    return failures;
}
```

Test coverage includes:
- Plugin registration/unregistration
- Plugin loading/unloading
- Plugin activation
- Effect registration/rendering
- Example plugin validation
- 5D renderer initialization
- Layer rendering
- Plugin-renderer integration

## Future Enhancements

Planned features for future versions:

1. **Dynamic Plugin Loading**: Load plugins from filesystem
2. **Hot Reload**: Update plugins without restart
3. **Plugin Dependencies**: Support plugin chains
4. **Effect Composition**: Combine multiple effects
5. **GPU Acceleration**: Hardware-accelerated rendering
6. **Shader System**: Custom shader-like effects
7. **Animation Timeline**: Time-based effect sequencing
8. **Plugin Marketplace**: Community plugin repository

## API Reference Summary

### Core Functions

| Function | Description |
|----------|-------------|
| `theme_plugin_system_init()` | Initialize plugin system |
| `theme_plugin_register()` | Register a plugin |
| `theme_plugin_load()` | Load plugin (calls init) |
| `theme_plugin_activate()` | Activate plugin |
| `theme_plugin_unload()` | Unload plugin |
| `theme_plugin_get_by_name()` | Find plugin by name |
| `theme_plugin_get_active()` | Get active plugin |

### Effect Functions

| Function | Description |
|----------|-------------|
| `theme_plugin_register_effect()` | Register custom effect |
| `theme_plugin_render_effect()` | Render specific effect |
| `gui_5d_render_plugin_effects()` | Render all plugin effects |

### 5D Renderer Functions

| Function | Description |
|----------|-------------|
| `gui_5d_renderer_init()` | Initialize 5D renderer |
| `gui_5d_renderer_set_mode()` | Set rendering mode |
| `gui_5d_renderer_render_layer()` | Render 5D layer |
| `gui_5d_apply_depth_fog()` | Apply depth fog |
| `gui_5d_apply_volumetric_light()` | Volumetric lighting |
| `gui_5d_draw_hologram()` | Holographic effect |
| `gui_5d_draw_chromatic_aberration()` | RGB separation |
| `gui_5d_draw_parallax()` | Parallax effect |

## Conclusion

The Aurora OS Theme Plugin System provides a powerful, extensible framework for creating custom themes and visual effects. The combination of the plugin API and enhanced 5D renderer enables developers to create unique, immersive user experiences while maintaining system stability and performance.

For examples and reference implementations, see the included Neon, Retro, and Matrix plugins in `theme_plugins_examples.c`.
