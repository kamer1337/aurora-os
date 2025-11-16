# Aurora OS - Font Manager and Desktop Configuration

## Overview

This document describes the font management and desktop configuration systems added to Aurora OS, allowing users to customize fonts and desktop appearance.

## Font Manager System

### Purpose

The Font Manager provides a unified interface for managing multiple fonts in Aurora OS, with runtime font switching capabilities and support for various font styles.

### Supported Fonts

1. **Standard 8x8** - Classic bitmap font
2. **Crystalline 8x8** (Default) - Enhanced angular/crystalline design with futuristic aesthetic
3. **Crystalline 5x7** - Compact crystalline font (45% fewer pixels than 8x8)
4. **Monospace 6x8** - Monospace variant for code and terminal displays

### API Reference

#### Initialization

```c
int font_manager_init(void);
```

Initializes the font manager system and sets the default font to Crystalline 8x8.

#### Font Selection

```c
font_type_t font_manager_get_current(void);
int font_manager_set_current(font_type_t type);
```

Get and set the currently active font.

#### Drawing Functions

```c
void font_manager_draw_char(uint32_t x, uint32_t y, char c, 
                           color_t fg_color, color_t bg_color);
void font_manager_draw_string(uint32_t x, uint32_t y, const char* str, 
                             color_t fg_color, color_t bg_color);
```

Draw characters and strings using the current font. These functions automatically handle font-specific rendering details.

#### Font Properties

```c
uint8_t font_manager_get_char_width(void);
uint8_t font_manager_get_char_height(void);
uint8_t font_manager_get_char_advance(void);
```

Query the current font's dimensions and spacing.

### Usage Example

```c
#include "font_manager.h"

// Initialize font manager
font_manager_init();

// Switch to 5x7 font for compact display
font_manager_set_current(FONT_5X7_CRYSTALLINE);

// Draw text with current font
font_manager_draw_string(10, 10, "Hello Aurora OS!", 
                        COLOR_WHITE, COLOR_BLACK);

// Get font properties for layout calculations
uint8_t advance = font_manager_get_char_advance();
uint32_t next_x = 10 + (strlen("Hello") * advance);
```

## Desktop Configuration System

### Purpose

The Desktop Configuration system manages desktop appearance and behavior settings, providing users with control over colors, effects, and desktop features.

### Configuration Options

#### Font Settings
- Default system font selection

#### Color Scheme
- Desktop background gradient (start and end colors)
- Taskbar colors (background and foreground)
- Window title colors (active and inactive)
- Window border color

#### Desktop Behavior
- Show/hide desktop icons
- Show/hide taskbar
- Show/hide system tray
- Enable/disable animations
- Enable/disable shadows
- Enable/disable transparency

#### Icon Settings
- Icon size
- Icon spacing

#### Window Settings
- Window snapping
- Window animations
- Animation speed

### API Reference

#### Initialization

```c
int desktop_config_init(void);
```

Initialize desktop configuration with sensible defaults.

#### Configuration Access

```c
desktop_config_t* desktop_config_get(void);
int desktop_config_apply(const desktop_config_t* config);
void desktop_config_reset(void);
```

Get, apply, and reset configuration settings.

#### Settings UI

```c
void desktop_config_show_settings(void);
```

Display the desktop settings window, allowing users to configure appearance and behavior.

### Usage Example

```c
#include "desktop_config.h"

// Initialize configuration
desktop_config_init();

// Get current configuration
desktop_config_t* config = desktop_config_get();

// Modify settings
config->default_font = FONT_5X7_CRYSTALLINE;
config->enable_shadows = 1;
config->desktop_bg_start = (color_t){20, 100, 200, 255};

// Apply changes
desktop_config_apply(config);

// Show settings dialog to user
desktop_config_show_settings();
```

## Desktop Helper Enhancements

### Implemented Functions

The following desktop actions have been fully implemented (replacing previous stubs):

#### NEW_FOLDER
Creates a new folder dialog with input field and action buttons.

#### NEW_FILE
Creates a new file dialog with name input and action buttons.

#### REFRESH
Triggers a full desktop redraw, refreshing all icons and windows.

#### PROPERTIES
Shows a comprehensive desktop properties dialog with:
- Screen resolution information
- Color depth
- Desktop icon count
- Quick access to settings

#### OPEN_TERMINAL / OPEN_FILE_MANAGER / OPEN_DISK_MANAGER
Launch respective applications through the app framework.

### Usage Example

```c
#include "desktop_helpers.h"

// Initialize desktop helpers
desktop_helpers_init();

// Create a new folder
desktop_perform_action(DESKTOP_ACTION_NEW_FOLDER);

// Show desktop properties
desktop_perform_action(DESKTOP_ACTION_PROPERTIES);

// Refresh desktop
desktop_perform_action(DESKTOP_ACTION_REFRESH);
```

## Integration with GUI System

### GUI Integration

The GUI system has been updated to use the font manager throughout:

- Window titles use the current font
- Button labels use the current font
- All text labels use the current font
- Taskbar text uses the current font
- Menu text uses the current font

This ensures consistent font rendering across the entire desktop environment.

### Default Behavior

- **Default Font**: Crystalline 8x8 (set during `gui_init()`)
- **Font Manager**: Initialized automatically with GUI system
- **Desktop Config**: Initialized automatically with GUI system

## Performance Considerations

### Font Rendering

- Font data is stored in static arrays (no dynamic allocation)
- Font switching is instant (no loading required)
- Character rendering is optimized for each font type

### Configuration

- Configuration is stored in a single global structure
- No file I/O overhead (persistence is a future enhancement)
- Applying configuration is lightweight

## Future Enhancements

### Planned Features

1. **Font Persistence**: Save and load font preferences from file
2. **Variable-Width Fonts**: Support for proportional fonts
3. **Font Styles**: Bold, italic, and underline variants
4. **Additional Fonts**: More font sizes and styles
5. **Unicode Support**: Extended character set support
6. **Anti-Aliasing**: Smoother font rendering on high-DPI displays
7. **Configuration Persistence**: Save desktop settings to disk
8. **Theme System**: Pre-defined color schemes and themes
9. **Live Preview**: Real-time preview of setting changes
10. **Font Upload**: Allow users to add custom fonts

## Testing

Font tests are available in `tests/font_tests.c` and validate:
- Font rendering for all character classes
- Font switching functionality
- String wrapping and layout
- Color rendering

Run tests with:
```bash
make test
```

## Files Changed

### New Files
- `kernel/gui/font_manager.h` - Font manager header
- `kernel/gui/font_manager.c` - Font manager implementation
- `kernel/gui/desktop_config.h` - Desktop configuration header
- `kernel/gui/desktop_config.c` - Desktop configuration implementation
- `docs/FONT_MANAGER.md` - This documentation

### Modified Files
- `kernel/gui/gui.c` - Updated to use font manager
- `kernel/gui/gui.h` - No changes (API maintained)
- `kernel/gui/desktop_helpers.c` - Implemented stub functions
- `kernel/gui/desktop_modules.c` - Enhanced module implementations
- `kernel/gui/application.c` - Integrated desktop configuration

## Compatibility

All changes are backward compatible:
- Existing code continues to work without modification
- Old framebuffer drawing functions still available
- New font manager is optional (GUI defaults to crystalline font)
- Configuration system is optional (sensible defaults provided)

## Author

Implemented as part of the Aurora OS desktop enhancement initiative.

## License

Same as Aurora OS project.
