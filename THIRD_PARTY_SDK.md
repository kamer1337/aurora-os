# Aurora OS - Third-Party Application SDK

**Version**: 1.0  
**Last Updated**: November 20, 2025  
**Target Audience**: Application Developers

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Development Environment](#development-environment)
4. [Application Structure](#application-structure)
5. [GUI API](#gui-api)
6. [File System API](#file-system-api)
7. [Package Format](#package-format)
8. [Building Applications](#building-applications)
9. [Testing](#testing)
10. [Publishing](#publishing)

---

## Introduction

Welcome to the Aurora OS Third-Party Application SDK. This guide will help you develop, build, test, and publish applications for Aurora OS.

### What You Can Build

- Desktop applications with GUI
- Command-line utilities
- System tools
- Games
- Productivity software
- Multimedia applications

### SDK Features

- Complete GUI framework
- File system access
- Network capabilities
- Process management
- Hardware access (with permissions)
- Package management integration

---

## Getting Started

### Prerequisites

1. **Aurora OS Development Environment**:
   ```bash
   # Clone SDK
   git clone https://github.com/kamer1337/aurora-os-sdk.git
   cd aurora-os-sdk
   ```

2. **Required Tools**:
   - GCC compiler (32-bit)
   - Make build system
   - NASM assembler
   - Aurora OS headers

3. **Install SDK**:
   ```bash
   make install-sdk
   # This installs headers and libraries to /usr/local/aurora-sdk
   ```

### Hello World Application

Create `hello.c`:

```c
#include <aurora/gui.h>
#include <aurora/application.h>

int main() {
    // Initialize GUI
    gui_init();
    
    // Create window
    window_t* window = gui_create_window("Hello Aurora", 300, 200, 400, 300);
    
    // Create label
    widget_t* label = gui_create_widget(WIDGET_LABEL);
    label->bounds.x = 50;
    label->bounds.y = 100;
    label->bounds.width = 300;
    label->bounds.height = 30;
    label->text = "Hello, Aurora OS!";
    label->fg_color = (color_t){255, 255, 255, 255};
    
    gui_add_widget(window, label);
    gui_show_window(window);
    
    // Event loop
    while (1) {
        event_t event;
        if (gui_poll_event(&event)) {
            if (event.type == EVENT_CLOSE) {
                break;
            }
        }
        gui_render();
    }
    
    gui_shutdown();
    return 0;
}
```

Build:
```bash
aurora-gcc -o hello hello.c -laura-gui
```

---

## Development Environment

### Directory Structure

```
my-app/
├── src/
│   ├── main.c
│   ├── ui.c
│   └── logic.c
├── include/
│   └── myapp.h
├── resources/
│   ├── icon.png
│   └── data.txt
├── Makefile
└── package.yaml
```

### Recommended IDE Setup

1. **Visual Studio Code** with extensions:
   - C/C++ Extension Pack
   - Aurora OS Syntax Highlighter

2. **CLion** with custom toolchain configuration

3. **Vim/Neovim** with LSP support

---

## Application Structure

### Basic Application Template

```c
#include <aurora/gui.h>
#include <aurora/fs.h>
#include <aurora/system.h>

typedef struct {
    window_t* main_window;
    // Application state
} app_state_t;

static app_state_t g_app_state;

// Initialize application
int app_init(void) {
    gui_init();
    
    g_app_state.main_window = gui_create_window(
        "My Application", 
        100, 100, 
        800, 600
    );
    
    if (!g_app_state.main_window) {
        return -1;
    }
    
    // Setup UI
    setup_ui();
    
    return 0;
}

// Setup user interface
void setup_ui(void) {
    // Create widgets
    widget_t* button = gui_create_widget(WIDGET_BUTTON);
    button->bounds = (rect_t){10, 10, 100, 30};
    button->text = "Click Me";
    button->on_click = on_button_click;
    
    gui_add_widget(g_app_state.main_window, button);
}

// Event handlers
void on_button_click(widget_t* widget, int32_t x, int32_t y) {
    // Handle button click
}

// Main event loop
void app_run(void) {
    gui_show_window(g_app_state.main_window);
    
    while (1) {
        event_t event;
        if (gui_poll_event(&event)) {
            if (event.type == EVENT_CLOSE) {
                break;
            }
            handle_event(&event);
        }
        
        gui_render();
    }
}

// Cleanup
void app_shutdown(void) {
    gui_close_window(g_app_state.main_window);
    gui_shutdown();
}

int main(int argc, char** argv) {
    if (app_init() != 0) {
        return 1;
    }
    
    app_run();
    app_shutdown();
    
    return 0;
}
```

---

## GUI API

### Window Management

```c
// Create window
window_t* gui_create_window(const char* title, 
                            int32_t x, int32_t y,
                            uint32_t width, uint32_t height);

// Show/hide window
void gui_show_window(window_t* window);
void gui_hide_window(window_t* window);

// Close window
void gui_close_window(window_t* window);

// Maximize/minimize
void gui_maximize_window(window_t* window);
void gui_minimize_window(window_t* window);
void gui_restore_window(window_t* window);
```

### Widget Types

Available widgets:
- `WIDGET_BUTTON` - Clickable button
- `WIDGET_LABEL` - Text label
- `WIDGET_TEXTBOX` - Text input field
- `WIDGET_PANEL` - Container widget
- `WIDGET_MENUBAR` - Menu bar
- `WIDGET_MENU` - Drop-down menu

```c
// Create widget
widget_t* gui_create_widget(widget_type_t type);

// Add widget to window
void gui_add_widget(window_t* window, widget_t* widget);

// Remove widget
void gui_remove_widget(window_t* window, widget_t* widget);
```

### Event Handling

```c
typedef enum {
    EVENT_NONE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_PAINT,
    EVENT_CLOSE
} event_type_t;

// Poll for events
int gui_poll_event(event_t* event);

// Register event handler
void gui_set_event_handler(window_t* window,
                          void (*handler)(event_t*));
```

### Drawing Functions

```c
// Draw primitives
void fb_draw_pixel(int32_t x, int32_t y, color_t color);
void fb_draw_line(int32_t x1, int32_t y1, 
                 int32_t x2, int32_t y2, color_t color);
void fb_draw_rect(int32_t x, int32_t y,
                 uint32_t width, uint32_t height, color_t color);
void fb_fill_rect(int32_t x, int32_t y,
                 uint32_t width, uint32_t height, color_t color);
void fb_draw_circle(int32_t cx, int32_t cy,
                   uint32_t radius, color_t color);

// Draw text
void fb_draw_text(int32_t x, int32_t y,
                 const char* text, color_t color);
```

---

## File System API

### File Operations

```c
#include <aurora/fs.h>

// Open file
int fs_open(const char* path, int flags);

// Read/Write
ssize_t fs_read(int fd, void* buffer, size_t count);
ssize_t fs_write(int fd, const void* buffer, size_t count);

// Close file
int fs_close(int fd);

// Seek
off_t fs_seek(int fd, off_t offset, int whence);

// File flags
#define FS_O_RDONLY  0x01
#define FS_O_WRONLY  0x02
#define FS_O_RDWR    0x03
#define FS_O_CREATE  0x04
#define FS_O_TRUNC   0x08
```

### Directory Operations

```c
// Create directory
int fs_mkdir(const char* path);

// Remove directory
int fs_rmdir(const char* path);

// List directory
dir_entry_t* fs_readdir(int fd);

// Directory entry structure
typedef struct {
    char name[256];
    uint32_t size;
    uint32_t type;  // FS_TYPE_FILE or FS_TYPE_DIR
} dir_entry_t;
```

---

## Package Format

### Package Metadata (package.yaml)

```yaml
name: my-awesome-app
version: 1.0.0
description: A great application for Aurora OS
author: Your Name <your.email@example.com>
license: MIT
category: productivity

dependencies:
  - libaurora-gui >= 1.0
  - libaurora-fs >= 1.0

files:
  - src: build/my-app
    dest: /usr/local/bin/my-app
  - src: resources/icon.png
    dest: /usr/share/my-app/icon.png

permissions:
  - filesystem.read
  - filesystem.write.home
  - network.access
```

### Package Structure

```
my-app-1.0.0.apkg
├── package.yaml
├── bin/
│   └── my-app
├── resources/
│   └── icon.png
├── docs/
│   └── README.md
└── checksums.sha256
```

---

## Building Applications

### Makefile Template

```makefile
# Application name
APP_NAME = my-app
VERSION = 1.0.0

# SDK paths
SDK_ROOT = /usr/local/aurora-sdk
SDK_INCLUDE = $(SDK_ROOT)/include
SDK_LIB = $(SDK_ROOT)/lib

# Compiler settings
CC = aurora-gcc
CFLAGS = -Wall -Wextra -I$(SDK_INCLUDE) -I./include
LDFLAGS = -L$(SDK_LIB) -laura-gui -laura-fs

# Source files
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

# Build targets
all: $(APP_NAME)

$(APP_NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(APP_NAME)

package: $(APP_NAME)
	aurora-package --create \
		--name $(APP_NAME) \
		--version $(VERSION) \
		--config package.yaml

install:
	aurora-package --install $(APP_NAME)-$(VERSION).apkg

.PHONY: all clean package install
```

### Build Commands

```bash
# Build application
make

# Create package
make package

# Install locally for testing
make install

# Clean build artifacts
make clean
```

---

## Testing

### Unit Testing

```c
#include <aurora/test.h>

TEST(my_feature) {
    int result = my_function(42);
    ASSERT_EQUAL(result, 42);
}

TEST(my_other_feature) {
    void* ptr = my_allocate(100);
    ASSERT_NOT_NULL(ptr);
    my_free(ptr);
}

int main() {
    RUN_TEST(my_feature);
    RUN_TEST(my_other_feature);
    return TEST_SUMMARY();
}
```

### Integration Testing

Test in Aurora OS VM:
```bash
# Start Aurora OS in QEMU
make run

# In Aurora OS, install your app
pkgman install my-app-1.0.0.apkg

# Launch and test
my-app
```

---

## Publishing

### Publishing to Aurora App Store

1. **Create Developer Account**:
   ```bash
   aurora-dev register --email your@email.com
   ```

2. **Verify Your Package**:
   ```bash
   aurora-package --verify my-app-1.0.0.apkg
   ```

3. **Submit to Store**:
   ```bash
   aurora-dev publish \
       --package my-app-1.0.0.apkg \
       --category productivity \
       --screenshots screenshots/
   ```

4. **Update Application**:
   ```bash
   aurora-dev update \
       --package my-app-1.0.1.apkg \
       --changelog "Bug fixes and improvements"
   ```

### Store Guidelines

✅ **Required**:
- Valid package.yaml
- Working binary
- Icon (PNG, 256x256)
- Description (< 500 chars)
- Screenshots (at least 2)

✅ **Recommended**:
- User documentation
- Changelog
- Source code repository link
- Support contact

❌ **Prohibited**:
- Malware or malicious code
- Unauthorized data collection
- Copyright infringement
- Cryptojacking

---

## API Reference

### Complete API Documentation

For complete API documentation, see:
- **GUI API**: `/usr/local/aurora-sdk/docs/gui-api.md`
- **File System API**: `/usr/local/aurora-sdk/docs/fs-api.md`
- **Network API**: `/usr/local/aurora-sdk/docs/network-api.md`
- **System API**: `/usr/local/aurora-sdk/docs/system-api.md`

### Online Resources

- **Developer Portal**: https://dev.aurora-os.org
- **API Reference**: https://api.aurora-os.org
- **Examples**: https://github.com/kamer1337/aurora-os-examples
- **Forum**: https://forum.aurora-os.org

---

## Support

### Getting Help

- **Documentation**: Read the docs first
- **Forum**: Ask questions in developer forum
- **Discord**: Join #dev-help channel
- **Email**: dev-support@aurora-os.org

### Reporting SDK Bugs

https://github.com/kamer1337/aurora-os-sdk/issues

---

## License

Aurora OS SDK is released under the MIT License.

---

**Document Version**: 1.0  
**Last Updated**: November 20, 2025

Happy Coding! 🚀
