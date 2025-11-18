# Aurora OS - API Reference Guide

**Version**: 1.0  
**Status**: Production Ready  
**Last Updated**: November 18, 2025

## Overview

This document serves as the comprehensive API reference for Aurora OS. It provides documentation for all major subsystems, libraries, and interfaces available to developers building applications and system components for Aurora OS.

---

## Table of Contents

1. [Core Kernel APIs](#core-kernel-apis)
2. [Memory Management](#memory-management)
3. [Process and Threading](#process-and-threading)
4. [File System APIs](#file-system-apis)
5. [Network APIs](#network-apis)
6. [GUI Framework](#gui-framework)
7. [Device Drivers](#device-drivers)
8. [Security and Cryptography](#security-and-cryptography)
9. [System Calls](#system-calls)
10. [Advanced Features](#advanced-features)

---

## Core Kernel APIs

### Kernel Initialization
- **Function**: `kernel_main()`
- **Purpose**: Main kernel entry point
- **Documentation**: See `kernel/core/kernel.h`

### Plugin System
- **Module**: Plugin Manager
- **Purpose**: Dynamic plugin loading and management
- **Documentation**: See [PLUGIN_SYSTEM.md](PLUGIN_SYSTEM.md)
- **API Header**: `kernel/core/plugin.h`

Key Functions:
```c
void plugin_manager_init(void);
int plugin_register(plugin_descriptor_t* plugin);
int plugin_init_all(void);
void plugin_cleanup_all(void);
```

---

## Memory Management

### Physical Memory Allocator
- **Module**: Memory Manager
- **Purpose**: Physical page allocation and management
- **API Header**: `kernel/memory/memory.h`

Key Functions:
```c
void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
```

### Virtual Memory and Paging
- **Module**: Paging System
- **Purpose**: Virtual memory management with page tables
- **API Header**: `kernel/memory/paging.h`

Key Functions:
```c
void paging_init(void);
void* paging_map(void* physical, void* virtual, uint32_t flags);
void paging_unmap(void* virtual);
```

---

## Process and Threading

### Process Management
- **Module**: Process Manager
- **Purpose**: Process creation, scheduling, and lifecycle management
- **API Header**: `kernel/process/process.h`

Key Functions:
```c
void process_init(void);
process_t* process_create(void (*entry)(void), uint32_t priority);
void process_terminate(uint32_t pid);
void process_yield(void);
```

### Scheduler
- **Module**: Round-Robin Scheduler
- **Purpose**: Process scheduling and context switching
- **Algorithm**: Round-robin with priority support

Key Functions:
```c
void scheduler_init(void);
void scheduler_schedule(void);
```

---

## File System APIs

### Virtual File System (VFS)
- **Module**: VFS Layer
- **Purpose**: Unified file system interface
- **API Header**: `filesystem/vfs/vfs.h`
- **Documentation**: See [VFS documentation in existing docs]

Key Functions:
```c
int vfs_init(void);
int vfs_open(const char* path, int flags);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);
int vfs_close(int fd);
```

### Ramdisk
- **Module**: Ramdisk Driver
- **Purpose**: In-memory file system
- **API Header**: `filesystem/ramdisk/ramdisk.h`

Key Functions:
```c
int ramdisk_init(void);
int ramdisk_create(size_t size);
int ramdisk_read(uint32_t block, void* buffer);
int ramdisk_write(uint32_t block, const void* buffer);
```

### Journaling Layer
- **Module**: File System Journal
- **Purpose**: Transaction-based file system consistency
- **API Header**: `filesystem/journal/journal.h`

Key Functions:
```c
int journal_init(void);
int journal_begin_transaction(void);
int journal_commit_transaction(void);
int journal_rollback_transaction(void);
```

### File Cache
- **Module**: File Cache System
- **Purpose**: Performance optimization through caching
- **API Header**: `filesystem/cache/file_cache.h`
- **Documentation**: See [FILE_CACHE_SYSTEM.md](FILE_CACHE_SYSTEM.md)

---

## Network APIs

### Network Device
- **Module**: Network Subsystem
- **Purpose**: Packet-based network I/O
- **API Header**: `kernel/network/network.h`
- **Documentation**: See [NETWORK_API.md](NETWORK_API.md)

Key Functions:
```c
bool aurora_vm_net_is_connected(const AuroraVM *vm);
int aurora_vm_net_send(AuroraVM *vm, const uint8_t *data, uint32_t length);
int aurora_vm_net_receive(AuroraVM *vm, uint8_t *buffer, uint32_t max_length);
```

**Features**:
- Connection management
- Packet transmission (MTU: 1500 bytes)
- Packet reception
- Queue management (64 packet queues)
- Network interrupt support

---

## GUI Framework

### Core GUI System
- **Module**: GUI Framework
- **Purpose**: Windowing system and visual rendering
- **API Header**: `kernel/gui/gui.h`
- **Documentation**: See [GUI_VISUAL_CHANGES.md](GUI_VISUAL_CHANGES.md)

Key Functions:
```c
void gui_init(void);
window_t* gui_create_window(const char* title, int32_t x, int32_t y, 
                            int32_t width, int32_t height);
widget_t* gui_create_button(window_t* window, const char* text, 
                             int32_t x, int32_t y, int32_t width, int32_t height);
widget_t* gui_create_label(window_t* window, const char* text, 
                            int32_t x, int32_t y);
void gui_update(void);
void gui_handle_input(void);
```

### Font Manager
- **Module**: Font Management System
- **Purpose**: Multi-font support and text rendering
- **API Header**: `kernel/gui/font_manager.h`
- **Documentation**: See [FONT_MANAGER.md](FONT_MANAGER.md)

Key Functions:
```c
int font_manager_init(void);
font_type_t font_manager_get_current(void);
int font_manager_set_current(font_type_t type);
void font_manager_draw_string(const char* str, int32_t x, int32_t y, color_t color);
```

**Supported Fonts**:
- Standard 8x8
- Crystalline 8x8 (default)
- Crystalline 5x7
- Monospace 6x8

### Theme Manager
- **Module**: Theme System
- **Purpose**: Visual customization and theming
- **API Header**: `kernel/gui/theme_manager.h`
- **Documentation**: See [THEME_WALLPAPER_SYSTEM.md](THEME_WALLPAPER_SYSTEM.md)

### Wallpaper Manager
- **Module**: Wallpaper System
- **Purpose**: Desktop background management
- **API Header**: `kernel/gui/wallpaper_manager.h`

Supported Wallpaper Types:
- Solid colors
- Gradients (vertical, horizontal, radial)
- Live wallpapers (animated nature scenes)
- Static images (implementation planned)

### Desktop Configuration
- **Module**: Desktop Settings
- **Purpose**: System-wide desktop configuration
- **API Header**: `kernel/gui/desktop_config.h`

Key Functions:
```c
int desktop_config_init(void);
desktop_config_t* desktop_config_get(void);
int desktop_config_apply(const desktop_config_t* config);
int desktop_config_save(void);
int desktop_config_load(void);
```

### Advanced Visual Effects
- **Documentation**: See [3D_4D_5D_GUI_IMPLEMENTATION.md](3D_4D_5D_GUI_IMPLEMENTATION.md)

Features:
- 3D depth effects (shadows, gradients, rounded corners)
- 4D animation system (7 easing functions)
- 5D interactive effects (particles, glow, glass/blur)
- Alpha blending and transparency

### Live Wallpaper System
- **Module**: Live Wallpaper Engine
- **Purpose**: Animated, interactive desktop backgrounds
- **API Header**: `kernel/gui/live_wallpaper.h`
- **Documentation**: See [LIVE_WALLPAPER.md](LIVE_WALLPAPER.md)

Wallpaper Types:
- Forest scene
- Ocean scene
- Mountain landscape
- Meadow with flowers

---

## Device Drivers

### Framebuffer Driver
- **Module**: Framebuffer
- **Purpose**: Graphics output and pixel manipulation
- **API Header**: `kernel/gui/framebuffer.h`

Key Functions:
```c
void framebuffer_init(uint32_t addr, uint32_t width, uint32_t height);
void framebuffer_draw_pixel(int32_t x, int32_t y, color_t color);
void framebuffer_draw_rect(int32_t x, int32_t y, int32_t width, 
                           int32_t height, color_t color);
void framebuffer_clear(color_t color);
```

### VGA Driver
- **Module**: VGA Text Mode
- **Purpose**: Text-based console output
- **API Header**: `kernel/drivers/vga.h`

### Keyboard Driver
- **Module**: PS/2 Keyboard
- **Purpose**: Keyboard input handling
- **API Header**: `kernel/drivers/keyboard.h`

### Timer Driver
- **Module**: PIT Timer
- **Purpose**: System timing and scheduling
- **API Header**: `kernel/drivers/timer.h`

### Serial Port Driver
- **Module**: Serial Communication
- **Purpose**: Debug output and serial I/O
- **API Header**: `kernel/drivers/serial.h`

### USB Subsystem
- **Module**: USB Stack
- **Purpose**: USB device support
- **API Header**: `kernel/usb/usb.h`

---

## Security and Cryptography

### Quantum Cryptography Module
- **Module**: Quantum Crypto
- **Purpose**: Quantum-resistant encryption
- **API Header**: `kernel/security/quantum_crypto.h`
- **Documentation**: See [QUANTUM_ENCRYPTION.md](QUANTUM_ENCRYPTION.md)

Key Functions:
```c
void quantum_crypto_init(void);
int quantum_encrypt(const uint8_t* data, size_t len, uint8_t* output);
int quantum_decrypt(const uint8_t* data, size_t len, uint8_t* output);
int quantum_hash(const uint8_t* data, size_t len, uint8_t* hash);
```

Features:
- Quantum Random Number Generator (QRNG)
- Quantum key management
- Quantum-resistant encryption algorithms
- Quantum hash functions

---

## System Calls

### Aurora VM System Calls
- **Module**: Aurora VM
- **Purpose**: Virtual machine syscall interface
- **Documentation**: See [AURORA_VM.md](AURORA_VM.md)

Supported System Calls:
- `AURORA_SYSCALL_EXIT` - Exit program
- `AURORA_SYSCALL_WRITE` - Write to output
- `AURORA_SYSCALL_READ` - Read from input
- `AURORA_SYSCALL_OPEN` - Open file
- `AURORA_SYSCALL_CLOSE` - Close file
- `AURORA_SYSCALL_ALLOC` - Allocate memory
- `AURORA_SYSCALL_FREE` - Free memory
- `AURORA_SYSCALL_NETWORK_CONNECT` - Network connection
- `AURORA_SYSCALL_NETWORK_SEND` - Send network packet
- `AURORA_SYSCALL_NETWORK_RECEIVE` - Receive network packet

---

## Advanced Features

### Atomic Operations
- **Module**: Atomic Instructions
- **Purpose**: Lock-free synchronization
- **Documentation**: See [ATOMIC_OPERATIONS.md](ATOMIC_OPERATIONS.md)

Supported Operations:
- `XCHG` - Atomic exchange
- `CAS` - Compare-and-swap
- `FADD` - Fetch-and-add

### SMP Support
- **Module**: Multi-core Processing
- **Purpose**: Symmetric multiprocessing
- **API Header**: `kernel/smp/smp.h`

### Display Manager
- **Module**: Display Management
- **Purpose**: Multiple display and resolution management
- **API Header**: `kernel/gui/display_manager.h`
- **Documentation**: See [DISPLAY_MANAGER.md](DISPLAY_MANAGER.md)

### Non-Functional Requirements (NFR) Module
- **Module**: NFR Monitoring
- **Purpose**: Performance metrics and monitoring
- **API Header**: `kernel/core/nfr.h`

Monitored Metrics:
- Boot time
- GUI frame rate
- Memory usage
- System responsiveness
- Interrupt latency

---

## Additional Documentation

### Implementation Guides
- [Phase 1-5 Implementation](PHASE_COMPLETION_REPORT.md)
- [Plugin System Guide](PLUGIN_QUICKSTART.md)
- [Desktop Environment](DESKTOP_ENVIRONMENT_COMPLETION.md)
- [Font 5x7 Encoding](FONT_5X7_ENCODING.md)

### Testing Documentation
- [Phase 5 Testing](PHASE5_COMPLETION_REPORT.md)
- [Plugin Test Validation](PLUGIN_TEST_VALIDATION.md)

### Project Structure
- [Project Organization](PROJECT_STRUCTURE.md)
- [Getting Started](GETTING_STARTED.md)

---

## API Versioning

Aurora OS follows semantic versioning for its API:
- **Major version**: Breaking API changes
- **Minor version**: Backward-compatible new features
- **Patch version**: Backward-compatible bug fixes

**Current API Version**: 1.0.0

---

## Developer Support

For questions, issues, or contributions:
- **Repository**: https://github.com/kamer1337/aurora-os
- **Documentation**: `/docs` directory
- **Examples**: `/examples` directory

---

## API Stability Guarantees

### Stable APIs (v1.0+)
These APIs are guaranteed to remain backward-compatible:
- Core kernel initialization
- Memory management (kmalloc/kfree)
- Process management
- VFS layer
- System calls
- GUI core functions

### Experimental APIs
These APIs may change in future versions:
- Live wallpaper system
- Advanced visual effects
- Plugin system extensions

---

## License

Aurora OS API documentation is provided under the same license as the Aurora OS project.

---

**Document Maintainer**: Aurora OS Project Team  
**Review Cycle**: Quarterly  
**Next Review**: February 2026
