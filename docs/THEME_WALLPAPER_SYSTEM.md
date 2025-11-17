# Aurora OS - Theme and Wallpaper System

## Overview

Aurora OS now includes a comprehensive theme and wallpaper management system that allows users to customize the desktop appearance with various color schemes and background options.

## Features

### Theme System

The theme system provides complete desktop appearance customization with coordinated color schemes.

#### Available Themes

1. **Aurora (Default)** - Vivid blue gradients with modern aesthetics
2. **Dark** - Dark gray theme for reduced eye strain
3. **Light** - Bright, clean appearance with light colors
4. **Ocean** - Blue and teal ocean-inspired colors
5. **Forest** - Green nature-inspired theme
6. **Sunset** - Warm orange and pink colors
7. **Purple** - Purple and violet theme

#### Theme Components

Each theme includes coordinated colors for:
- Desktop background (gradient start and end)
- Taskbar (background, foreground, highlight)
- Windows (active/inactive title bars, borders, background)
- Buttons (default, hover, pressed states)
- Text (primary, secondary, disabled states)

### Wallpaper System

The wallpaper system supports multiple background types:

#### Wallpaper Modes

1. **Solid Color** - Single color background
2. **Gradient (Vertical)** - Top-to-bottom color transition
3. **Gradient (Horizontal)** - Left-to-right color transition
4. **Gradient (Radial)** - Center-outward color transition
5. **Live Wallpaper** - Animated nature scenes with parallax effects

#### Predefined Gradient Styles

1. **Blue Sky** - Default sky blue gradient
2. **Ocean** - Deep ocean colors
3. **Sunset** - Orange to pink transition
4. **Forest** - Green forest tones
5. **Purple Dream** - Purple gradient
6. **Night** - Dark nighttime colors
7. **Warm** - Brown and orange warm tones
8. **Cool** - Blue and cyan cool tones
9. **Grayscale** - Gray gradient

## API Reference

### Theme Manager API

```c
#include "kernel/gui/theme_manager.h"

// Initialize the theme manager
int theme_manager_init(void);

// Get a theme by type
const theme_t* theme_manager_get_theme(theme_type_t type);

// Set the active theme
int theme_manager_set_theme(theme_type_t type);

// Get the current active theme
const theme_t* theme_manager_get_current(void);

// Show theme selection window
void theme_manager_show_selector(void);
```

### Wallpaper Manager API

```c
#include "kernel/gui/wallpaper_manager.h"

// Initialize the wallpaper manager
int wallpaper_manager_init(void);

// Set wallpaper mode
int wallpaper_manager_set_mode(wallpaper_mode_t mode);

// Set solid color wallpaper
int wallpaper_manager_set_solid_color(color_t color);

// Set gradient wallpaper with predefined style
int wallpaper_manager_set_gradient(gradient_style_t style, wallpaper_mode_t mode);

// Set custom gradient wallpaper
int wallpaper_manager_set_custom_gradient(color_t start, color_t end, wallpaper_mode_t mode);

// Set live wallpaper
int wallpaper_manager_set_live(wallpaper_type_t type);

// Draw the current wallpaper
void wallpaper_manager_draw(uint32_t width, uint32_t height);

// Update wallpaper (for animated wallpapers)
void wallpaper_manager_update(uint32_t delta_time, int32_t cursor_x, int32_t cursor_y);

// Show wallpaper selection window
void wallpaper_manager_show_selector(void);
```

### Desktop Configuration Integration

```c
#include "kernel/gui/desktop_config.h"

// Show theme selection window
void desktop_config_show_theme_selector(void);

// Show wallpaper selection window
void desktop_config_show_wallpaper_selector(void);

// Show desktop settings window (now includes theme/wallpaper options)
void desktop_config_show_settings(void);
```

## Usage Examples

### Example 1: Change Theme

```c
// Initialize managers
theme_manager_init();

// Change to dark theme
theme_manager_set_theme(THEME_DARK);

// Theme colors are automatically applied to desktop configuration
```

### Example 2: Set Gradient Wallpaper

```c
// Initialize managers
wallpaper_manager_init();

// Set ocean gradient wallpaper (vertical)
wallpaper_manager_set_gradient(GRADIENT_STYLE_OCEAN, WALLPAPER_MODE_GRADIENT_V);

// Draw the wallpaper
wallpaper_manager_draw(screen_width, screen_height);
```

### Example 3: Custom Gradient

```c
// Create custom colors
color_t start = {50, 100, 200, 255};  // Light blue
color_t end = {200, 100, 50, 255};    // Orange

// Set custom horizontal gradient
wallpaper_manager_set_custom_gradient(start, end, WALLPAPER_MODE_GRADIENT_H);
```

### Example 4: Enable Live Wallpaper

```c
// Set live forest wallpaper
wallpaper_manager_set_live(WALLPAPER_NATURE_FOREST);

// In your render loop:
wallpaper_manager_update(delta_time, mouse_x, mouse_y);
wallpaper_manager_draw(screen_width, screen_height);
```

## User Interface

### Accessing Settings

The theme and wallpaper settings can be accessed through:

1. **Desktop Settings Window** - Shows options to launch theme/wallpaper selectors
2. **Theme Selector** - Dedicated window with all available themes
3. **Wallpaper Selector** - Dedicated window with wallpaper options

### Desktop Settings Window

To open the desktop settings:
- Click "Desktop Settings" from the start menu or system tray
- Look for "Theme & Wallpaper" section
- Click "Change Theme..." or "Change Wallpaper..." buttons

### Theme Selector Window

The theme selector displays:
- All available themes as buttons
- Each button colored with the theme's primary color for preview
- Click any theme button to apply it

### Wallpaper Selector Window

The wallpaper selector shows:
- Solid color options
- Predefined gradient styles
- Live wallpaper options
- Each with an "Apply" button

## Technical Details

### File Structure

```
kernel/gui/
├── theme_manager.h       # Theme system API
├── theme_manager.c       # Theme implementation
├── wallpaper_manager.h   # Wallpaper system API
├── wallpaper_manager.c   # Wallpaper implementation
├── desktop_config.h      # Desktop configuration (updated)
└── desktop_config.c      # Desktop configuration (updated)
```

### Memory Usage

- Theme Manager: ~3KB for theme definitions
- Wallpaper Manager: ~2KB for gradient definitions
- Both managers are initialized once at startup
- No dynamic allocations after initialization

### Performance

- **Theme Switching**: Instant (updates configuration only)
- **Solid Color Wallpaper**: Single framebuffer fill operation
- **Gradient Wallpapers**: 
  - Vertical/Horizontal: O(width) or O(height) operations
  - Radial: O(width × height) operations - more expensive
- **Live Wallpapers**: See live_wallpaper.h documentation

## Integration with Existing Systems

### Desktop Configuration

The theme and wallpaper managers integrate seamlessly with the existing desktop_config system:
- Theme changes automatically update desktop_config colors
- Wallpaper changes update desktop background settings
- All changes are immediately applied to the GUI

### Live Wallpaper System

The wallpaper manager wraps and extends the existing live wallpaper system:
- Live wallpaper can be enabled through wallpaper_manager
- All existing live wallpaper types are supported
- Wallpaper manager handles the live wallpaper lifecycle

## Testing

### Unit Tests

The system includes comprehensive unit tests in `tests/theme_wallpaper_tests.c`:

```c
// Run all tests
int failures = run_all_theme_wallpaper_tests();

// Get test results
int passed = get_theme_wallpaper_tests_passed();
int failed = get_theme_wallpaper_tests_failed();
```

### Test Coverage

- Theme manager initialization and operations
- Wallpaper manager initialization and operations
- Integration with desktop configuration
- Theme and wallpaper switching
- Invalid parameter handling

## Future Enhancements

Potential future improvements:

1. **Image Wallpapers** - Support for loading BMP/PNG images as wallpapers
2. **Theme Editor** - UI for creating custom themes
3. **Theme Persistence** - Save/load themes from filesystem
4. **Theme Import/Export** - Share themes between systems
5. **Animated Themes** - Themes with time-based color transitions
6. **Per-Window Themes** - Different themes for different applications
7. **Wallpaper Slideshow** - Rotate through multiple wallpapers

## Troubleshooting

### Theme not applying

- Ensure `theme_manager_init()` was called
- Check that `desktop_config_apply()` is called after theme changes
- Verify GUI system is properly initialized

### Wallpaper not displaying

- Ensure `wallpaper_manager_init()` was called
- Check that `wallpaper_manager_draw()` is called in render loop
- Verify framebuffer is properly initialized

### Performance issues with radial gradients

- Radial gradients are computationally expensive
- Consider using vertical or horizontal gradients instead
- Or use solid colors/live wallpapers for better performance

## Conclusion

The theme and wallpaper system provides a flexible, extensible foundation for desktop appearance customization in Aurora OS. The modular design allows for easy addition of new themes and wallpaper types while maintaining compatibility with existing systems.

---

**Document Version**: 1.0  
**Last Updated**: November 17, 2025  
**Author**: Aurora OS Development Team
