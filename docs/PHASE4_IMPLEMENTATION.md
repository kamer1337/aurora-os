# Aurora OS - Phase 4 Implementation Documentation

## Overview

This document describes the implementation of Phase 4 (User Interface) for Aurora OS, including the framebuffer driver, basic GUI framework, and windowing system.

## Phase 4: User Interface

### Framebuffer Driver

The framebuffer driver provides direct pixel-level access to the graphics hardware for rendering graphics mode output.

#### Features:
- **Resolution Support**: Default 1024x768, configurable
- **Color Depth**: 32-bit RGBA color (8 bits per channel)
- **Drawing Primitives**: Pixels, rectangles, lines
- **Text Rendering**: 8x8 bitmap font for text output
- **Scrolling Support**: Vertical scrolling for text mode

#### API:
```c
int framebuffer_init(uint32_t width, uint32_t height, uint8_t bpp);
framebuffer_info_t* framebuffer_get_info(void);
int framebuffer_is_available(void);

void framebuffer_draw_pixel(uint32_t x, uint32_t y, color_t color);
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);
void framebuffer_draw_rect_outline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);
void framebuffer_draw_hline(uint32_t x1, uint32_t x2, uint32_t y, color_t color);
void framebuffer_draw_vline(uint32_t x, uint32_t y1, uint32_t y2, color_t color);
void framebuffer_clear(color_t color);

void framebuffer_draw_char(uint32_t x, uint32_t y, char c, color_t fg_color, color_t bg_color);
void framebuffer_draw_string(uint32_t x, uint32_t y, const char* str, color_t fg_color, color_t bg_color);
void framebuffer_scroll_up(uint32_t lines, color_t bg_color);
```

#### Color System:
```c
typedef struct {
    uint8_t r;  // Red channel (0-255)
    uint8_t g;  // Green channel (0-255)
    uint8_t b;  // Blue channel (0-255)
    uint8_t a;  // Alpha channel (0-255)
} color_t;
```

#### Predefined Colors:
- COLOR_BLACK, COLOR_WHITE
- COLOR_RED, COLOR_GREEN, COLOR_BLUE
- COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
- COLOR_GRAY, COLOR_LIGHT_GRAY, COLOR_DARK_GRAY

#### Implementation Details:
- Framebuffer address: 0xE0000000 (typical VESA address)
- 32-bit pixel format: 0xAARRGGBB
- 8x8 bitmap font for text rendering
- Direct memory access for fast rendering
- Clipping to prevent out-of-bounds access

### GUI Framework

The GUI framework provides a complete windowing system with widgets and event handling.

#### Features:
- **Window Management**: Create, destroy, show, hide windows
- **Widget System**: Buttons, labels, panels
- **Event Handling**: Mouse and keyboard events
- **Window Decorations**: Titlebar, borders, close button
- **Taskbar**: System taskbar with start button
- **Focus Management**: Window focus tracking

#### API:

**Initialization:**
```c
int gui_init(void);
void gui_shutdown(void);
void gui_update(void);
void gui_process_event(event_t* event);
```

**Window Management:**
```c
window_t* gui_create_window(const char* title, int32_t x, int32_t y, 
                            uint32_t width, uint32_t height);
void gui_destroy_window(window_t* window);
void gui_show_window(window_t* window);
void gui_hide_window(window_t* window);
void gui_focus_window(window_t* window);
void gui_draw_window(window_t* window);
```

**Widget Creation:**
```c
widget_t* gui_create_button(window_t* window, const char* text, 
                            int32_t x, int32_t y, uint32_t width, uint32_t height);
widget_t* gui_create_label(window_t* window, const char* text, 
                          int32_t x, int32_t y);
widget_t* gui_create_panel(window_t* window, int32_t x, int32_t y, 
                          uint32_t width, uint32_t height);
```

**Event Handling:**
```c
void gui_set_widget_click_handler(widget_t* widget, 
                                  void (*handler)(widget_t*, int32_t, int32_t));
```

#### Data Structures:

**Window Structure:**
```c
typedef struct window {
    char* title;              // Window title
    rect_t bounds;            // Position and size
    color_t bg_color;         // Background color
    color_t title_color;      // Titlebar color
    uint8_t visible;          // Visibility flag
    uint8_t focused;          // Focus flag
    uint8_t has_border;       // Border flag
    uint8_t has_titlebar;     // Titlebar flag
    widget_t* widgets;        // Linked list of widgets
    window_t* next;           // Next window in list
} window_t;
```

**Widget Structure:**
```c
typedef struct widget {
    widget_type_t type;       // Widget type
    rect_t bounds;            // Position and size
    color_t bg_color;         // Background color
    color_t fg_color;         // Foreground color
    char* text;               // Widget text
    uint8_t visible;          // Visibility flag
    uint8_t enabled;          // Enabled flag
    void (*on_paint)(widget_t*);                    // Paint callback
    void (*on_click)(widget_t*, int32_t, int32_t);  // Click callback
    window_t* parent_window;  // Parent window
    widget_t* next;           // Next widget
} widget_t;
```

**Event Structure:**
```c
typedef struct {
    event_type_t type;        // Event type
    int32_t x;                // Mouse X coordinate
    int32_t y;                // Mouse Y coordinate
    uint32_t button;          // Mouse button
    uint32_t key;             // Keyboard key
    void* data;               // Additional data
} event_t;
```

#### Widget Types:
- **WIDGET_BUTTON**: Clickable button with text
- **WIDGET_LABEL**: Static text label
- **WIDGET_PANEL**: Container panel for grouping
- **WIDGET_TEXTBOX**: Text input (future)
- **WIDGET_CUSTOM**: Custom widget type (future)

#### Event Types:
- **EVENT_MOUSE_MOVE**: Mouse cursor moved
- **EVENT_MOUSE_DOWN**: Mouse button pressed
- **EVENT_MOUSE_UP**: Mouse button released
- **EVENT_KEY_DOWN**: Key pressed
- **EVENT_KEY_UP**: Key released
- **EVENT_PAINT**: Repaint request
- **EVENT_CLOSE**: Window close request

#### Implementation Details:
- Windows stored in linked list
- Widgets stored per window in linked list
- Z-order determined by list order (last = top)
- Focus management with visual feedback
- Coordinate system: top-left origin (0,0)
- Widget coordinates relative to window content area
- Taskbar height: 40 pixels at bottom

### Visual Design

The Aurora OS GUI follows modern design principles:

**Color Scheme:**
- Desktop background: Sky blue (30, 130, 200)
- Active window titlebar: Windows blue (0, 120, 215)
- Inactive window titlebar: Gray (128, 128, 128)
- Window content: White (255, 255, 255)
- Taskbar: Dark gray (45, 45, 48)
- Buttons: Light gray (192, 192, 192)

**Window Elements:**
- Titlebar: 24 pixels tall
- Border: 1 pixel dark gray outline
- Close button: 16x16 red square with white X
- Window title: Left-aligned in titlebar

**Taskbar:**
- Height: 40 pixels
- Position: Bottom of screen
- Start button: 80x30 pixels, Windows blue
- Top border: Gray line separator

### Usage Example

```c
// Initialize GUI system
gui_init();

// Create a window
window_t* win = gui_create_window("Hello Aurora!", 100, 100, 400, 300);

// Add widgets
widget_t* label = gui_create_label(win, "Welcome to Aurora OS", 20, 20);
widget_t* button = gui_create_button(win, "Click Me", 20, 60, 120, 30);

// Set button click handler
void button_click(widget_t* w, int32_t x, int32_t y) {
    // Handle button click
}
gui_set_widget_click_handler(button, button_click);

// Show window
gui_show_window(win);

// Update display
gui_update();

// Process events
event_t event = {EVENT_MOUSE_DOWN, 150, 180, 1, 0, NULL};
gui_process_event(&event);
```

## Build System Updates

### Makefile Changes:
- Added `kernel/gui/*.c` to kernel sources
- Added `build/kernel/gui` to build directories
- New targets:
  - `make iso`: Create bootable ISO image
  - `make run`: Build and run in Aurora VM
  - `make test`: Build and run Aurora VM test suite

### Scripts:
- **scripts/create_iso.sh**: Creates bootable ISO using GRUB
- **scripts/run_aurora_vm.sh**: Runs Aurora VM test suite

## Testing

### Manual Testing:
1. Build kernel: `make all`
2. Create ISO: `make iso`
3. Run Aurora VM tests: `make run` or `make test`
4. Test GUI rendering visually
5. Test window creation and drawing

### Integration Testing:
The GUI framework integrates with:
- Memory management (kmalloc/kfree)
- Framebuffer driver
- Event system (future: keyboard/mouse drivers)

## Performance Considerations

- Direct framebuffer access for speed
- No double buffering yet (causes flicker)
- Minimal clipping checks
- Simple font rendering (8x8 bitmap)
- Memory allocated for windows/widgets (freed on destroy)

## Known Limitations

1. **No Hardware Acceleration**: Software rendering only
2. **No Double Buffering**: May cause screen tearing
3. **Limited Font**: Only 8x8 ASCII characters
4. **No Anti-aliasing**: Sharp pixels only
5. **Fixed Resolution**: Not dynamically changeable
6. **No True Multiboot**: Framebuffer address hardcoded
7. **No Mouse Cursor**: Hardware cursor not implemented
8. **No Transparency**: Alpha channel not used yet

## Future Enhancements

### Short-term:
- Implement hardware mouse cursor
- Add keyboard input handling for GUI
- Implement double buffering
- Add more widget types (textbox, checkbox, etc.)
- Implement menu system
- Add window minimizing/maximizing
- Implement drag and drop

### Long-term:
- Hardware acceleration (GPU drivers)
- Vector graphics support
- True Type font rendering
- Transparency and compositing
- Animation framework
- Theming system
- Accessibility features
- Multi-monitor support

## Conclusion

Phase 4 provides a solid foundation for Aurora OS's user interface:
- Functional framebuffer driver for graphics output
- Complete GUI framework with windows and widgets
- Event handling system ready for input drivers
- Modern visual design inspired by Windows
- Extensible architecture for future enhancements

The GUI system is ready for:
- Integration with input drivers (keyboard, mouse)
- Development of user applications
- Desktop environment features
- User interaction and feedback

**Project Status**: Phase 4 Started ✅ - Basic GUI implemented
