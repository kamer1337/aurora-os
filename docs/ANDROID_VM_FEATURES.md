# Android VM Features Guide

## Overview

Aurora OS now includes comprehensive Android Virtual Machine support, allowing users to run Android workloads within dedicated workspaces. This feature extends Aurora OS's multi-workspace capabilities to support Android Open Source Project (AOSP) and custom Android distributions alongside native Aurora OS and Linux VM workspaces.

## Key Features

### 1. Android Virtual Machine
- **Android Kernel Support**: Full Android kernel compatibility layer
- **Multiple Architectures**: Support for ARM32, ARM64, x86, and x86_64
- **Bionic libc Compatibility**: Android-specific syscall emulation
- **Memory Management**: 256MB default VM memory (configurable)
- **Property System**: Android property service support
- **Dalvik/ART VM**: Runtime environment for Android applications

### 2. Workspace Integration
- **OS Type Selection**: Choose Android VM for any workspace
- **Visual Indicators**: Green color coding for Android workspaces
- **Workspace Naming**: Automatic naming (e.g., "Android 1", "Android 2")
- **Independent Execution**: Each Android workspace runs independently

### 3. Android Installer
- **Distribution Selection**: Multiple pre-configured Android distributions
- **Installation Progress**: Real-time progress tracking and status updates
- **Available Distributions**:
  - **AOSP Android 13.0** (800 MB) - Pure Android Open Source Project
  - **LineageOS 20** (900 MB) - Popular custom ROM
  - **AOSP Android 14.0** (850 MB) - Coming Soon
  - **Pixel Experience** (950 MB) - Coming Soon
  - **BlissOS x86** (1.2 GB) - Coming Soon

### 4. Boot Options
Enhanced GRUB boot menu with Android support:
- **Aurora OS with Android VM Support**: Boot with Android VM enabled
- **Android (via Aurora VM)**: Direct Android boot mode
- **Install Android**: Launch Android installer

## Architecture

### Android VM Structure

```
AndroidVM
├── Aurora VM (underlying virtual machine)
├── Kernel Image (boot.img format)
├── Ramdisk (initrd)
├── System Partition (system.img)
├── Data Partition (data.img)
├── Property System
└── Dalvik/ART VM
```

### Memory Layout

```
0x00000000 - 0x0007FFFF : Reserved
0x00080000 - 0x0FFFFFFF : Android Kernel
0x01000000 - 0x0FFFFFFF : Ramdisk
0x10000000 - 0x2FFFFFFF : System Partition
0x30000000 - 0x6FFFFFFF : Data Partition
0x70000000 - 0x9FFFFFFF : Application Memory
```

## Usage

### Creating an Android Workspace

1. **Launch Aurora OS** with GUI enabled
2. **Open Workspace Switcher** (Super + W or from taskbar)
3. **Select a workspace** to configure
4. **Press '3'** or click "Android VM" button
5. **Workspace updates** to Android type with green indicator

### Installing Android

1. **Boot Aurora OS** and select "Install Android" from GRUB menu
2. **Select Distribution** using arrow keys
3. **Press Enter** to start installation
4. **Wait for completion** (progress shown in real-time)
5. **Reboot** to use installed Android

### Starting Android VM

```c
// Example: Starting Android VM programmatically

// Create Android VM instance
AndroidVM* vm = android_vm_create(ANDROID_ARCH_ARM64);

// Load kernel image
android_vm_load_kernel(vm, kernel_data, kernel_size);

// Load ramdisk
android_vm_load_ramdisk(vm, ramdisk_data, ramdisk_size);

// Load system partition
android_vm_load_system(vm, system_data, system_size);

// Set Android version
android_vm_set_version(vm, 13, 0);  // Android 13.0

// Configure kernel command line
android_vm_set_cmdline(vm, "console=ttyAMA0 androidboot.hardware=aurora");

// Start Android VM
android_vm_start(vm);
```

## Android Syscall Support

### Implemented Syscalls

Aurora OS Android VM supports essential Bionic libc syscalls:

| Syscall | Number | Description | Status |
|---------|--------|-------------|--------|
| EXIT | 1 | Process termination | ✅ Implemented |
| FORK | 2 | Create child process | 🚧 Stub |
| READ | 3 | Read from file descriptor | ✅ Implemented |
| WRITE | 4 | Write to file descriptor | ✅ Implemented |
| OPEN | 5 | Open file | 🚧 Stub |
| CLOSE | 6 | Close file descriptor | 🚧 Stub |
| GETPID | 20 | Get process ID | ✅ Implemented |
| GETUID | 24 | Get user ID | ✅ Implemented |
| BRK | 45 | Change data segment | ✅ Implemented |
| IOCTL | 54 | Device control | 🚧 Stub |
| MMAP | 90 | Memory mapping | ✅ Implemented |
| MUNMAP | 91 | Memory unmapping | 🚧 Stub |
| CLONE | 120 | Create thread/process | ✅ Implemented |
| PRCTL | 172 | Process control | ✅ Implemented |
| FUTEX | 240 | Fast userspace mutex | ✅ Implemented |
| OPENAT | 295 | Open file (relative) | ✅ Implemented |

*Note: ✅ = Fully implemented, 🚧 = Stub/Partial implementation*

### Expanding Syscall Coverage

To add more syscalls, update `android_vm_handle_syscall()` in `src/platform/android_vm.c`:

```c
case ANDROID_SYSCALL_NEW_SYSCALL:
    /* Implementation here */
    return result;
```

## Android Property System

The Android property system allows setting and querying system properties:

```c
// Set property
android_vm_set_property(vm, "ro.build.version.sdk", "33");
android_vm_set_property(vm, "ro.product.model", "Aurora VM");

// Get property
char value[128];
android_vm_get_property(vm, "ro.build.version.sdk", value, sizeof(value));
```

### Common Android Properties

- `ro.build.version.sdk` - Android SDK version
- `ro.build.version.release` - Android release version
- `ro.product.manufacturer` - Device manufacturer
- `ro.product.model` - Device model
- `ro.product.device` - Device codename
- `ro.hardware` - Hardware platform

## API Reference

### Core Functions

#### `android_vm_init()`
Initialize Android VM subsystem.

**Returns**: 0 on success, -1 on failure

#### `android_vm_create(arch)`
Create new Android VM instance.

**Parameters**:
- `arch`: Target architecture (ARM32, ARM64, x86, x86_64)

**Returns**: Pointer to AndroidVM or NULL on failure

#### `android_vm_destroy(vm)`
Destroy Android VM instance and free resources.

**Parameters**:
- `vm`: Android VM instance

#### `android_vm_load_kernel(vm, kernel_data, size)`
Load Android kernel image (boot.img format).

**Parameters**:
- `vm`: Android VM instance
- `kernel_data`: Kernel image data
- `size`: Kernel image size

**Returns**: 0 on success, -1 on failure

#### `android_vm_start(vm)`
Start Android VM execution.

**Parameters**:
- `vm`: Android VM instance

**Returns**: 0 on success, -1 on failure

#### `android_vm_get_state(vm)`
Get current VM state.

**Parameters**:
- `vm`: Android VM instance

**Returns**: Current state enum value

### State Management

Android VM supports the following states:

- `ANDROID_VM_STATE_UNINITIALIZED` - VM not initialized
- `ANDROID_VM_STATE_INITIALIZED` - VM initialized, ready to load images
- `ANDROID_VM_STATE_BOOTING` - VM is booting Android kernel
- `ANDROID_VM_STATE_RUNNING` - VM is running normally
- `ANDROID_VM_STATE_PAUSED` - VM is paused
- `ANDROID_VM_STATE_STOPPED` - VM is stopped
- `ANDROID_VM_STATE_ERROR` - VM encountered an error

## Configuration

### Android VM Configuration Constants

```c
#define ANDROID_VM_MEMORY_SIZE      (256 * 1024 * 1024)   // 256MB
#define ANDROID_VM_KERNEL_BASE      0x80000               // ARM load address
#define ANDROID_VM_RAMDISK_BASE     0x01000000           // Ramdisk address
#define ANDROID_VM_MAX_PROCESSES    128                   // Max processes
#define ANDROID_VM_SYSTEM_SIZE      (512 * 1024 * 1024)  // 512MB system
#define ANDROID_VM_DATA_SIZE        (1024 * 1024 * 1024) // 1GB data
```

### Kernel Command Line

Default Android kernel command line:
```
console=ttyAMA0 androidboot.hardware=aurora androidboot.selinux=permissive
```

Customize with `android_vm_set_cmdline()`:
```c
android_vm_set_cmdline(vm, "console=tty0 androidboot.hardware=custom root=/dev/ram0");
```

## Workspace OS Selector

The workspace OS selector provides a GUI for choosing workspace types:

### Key Bindings

- **'1'**: Select Aurora OS
- **'2'**: Select Linux VM
- **'3'**: Select Android VM
- **ESC**: Cancel selection

### Color Coding

- **Blue** (RGB 100, 200, 255): Aurora OS workspaces
- **Orange** (RGB 255, 200, 100): Linux VM workspaces
- **Green** (RGB 150, 255, 150): Android VM workspaces

## Limitations and Future Work

### Current Limitations

1. **Boot Protocol**: Basic Android boot implementation
   - Full boot.img header parsing needed
   - Device tree blob (DTB) support pending
   - ATAGS support for older Android versions

2. **Syscall Coverage**: Currently ~15 syscalls implemented
   - Target: 200+ syscalls for full Android compatibility
   - Focus on bionic libc requirements

3. **File System**: Stub implementation
   - Need ext4 filesystem support for system/data partitions
   - SELinux extended attributes support
   - Android-specific filesystem features

4. **Graphics**: Android UI rendering
   - Framebuffer integration for SurfaceFlinger
   - Hardware acceleration via GPU passthrough
   - OpenGL ES support

5. **Binder IPC**: Android's primary IPC mechanism
   - Binder kernel driver emulation
   - ServiceManager implementation
   - Parcel serialization

6. **Dalvik/ART**: Runtime environment
   - DEX file execution
   - JIT compilation
   - Native method support

### Planned Enhancements

- [ ] Complete Android boot protocol (boot.img v3/v4)
- [ ] Expand syscall table to 200+ syscalls
- [ ] ext4 filesystem with Android extensions
- [ ] Binder IPC implementation
- [ ] SurfaceFlinger integration for graphics
- [ ] Hardware-accelerated graphics (GPU passthrough)
- [ ] Audio support (ALSA/PulseAudio bridge)
- [ ] Network bridge to Aurora OS networking
- [ ] SELinux policy enforcement
- [ ] Multi-core support for Android VMs
- [ ] ADB (Android Debug Bridge) support
- [ ] Google Play Services compatibility layer
- [ ] ARM instruction translation for x86 hosts

## Advanced Topics

### Android Boot Image Format

Android uses a special boot.img format:

```
Boot Image Header (1 page)
├── Magic: "ANDROID!"
├── Kernel size
├── Kernel address
├── Ramdisk size
├── Ramdisk address
├── Second stage size/address
├── Tags address
├── Page size
├── OS version
└── Product name

Kernel (N pages)
Ramdisk (M pages)
Second Stage (Optional)
Device Tree (Optional)
```

### Property Service

Android's property service stores system-wide configuration:

```
/system/build.prop        → System properties
/vendor/build.prop        → Vendor properties
/data/local.prop          → Local overrides
```

Properties are limited to 92 bytes (name + value).

### Init Process

Android's init process (first userspace process):

1. Parse init.rc scripts
2. Mount filesystems
3. Set properties
4. Start services (servicemanager, surfaceflinger, etc.)
5. Handle property changes
6. Restart crashed services

## Troubleshooting

### Android VM won't start

**Issue**: `android_vm_start()` returns -1

**Solutions**:
- Ensure kernel image is loaded: `android_vm_load_kernel()`
- Check VM state: `android_vm_get_state()`
- Verify kernel command line is set
- Check ramdisk is loaded if required

### Syscall returns -1

**Issue**: Android application crashes with syscall error

**Solutions**:
- Check if syscall is implemented in `android_vm_handle_syscall()`
- Verify syscall arguments are valid
- Check Aurora OS logs for error messages
- Implement missing syscall if needed

### No graphics output

**Issue**: Android boots but no display

**Solutions**:
- Framebuffer integration not yet complete
- SurfaceFlinger requires graphics stack
- Use serial console output for debugging
- Check kernel command line for console= parameter

## Examples

### Example 1: Basic Android VM

```c
#include "platform/android_vm.h"

void basic_android_example(void) {
    // Initialize Android VM subsystem
    android_vm_init();
    
    // Create VM for ARM64
    AndroidVM* vm = android_vm_create(ANDROID_ARCH_ARM64);
    if (!vm) {
        return;
    }
    
    // Set Android version
    android_vm_set_version(vm, 13, 0);
    
    // Set properties
    android_vm_set_property(vm, "ro.build.version.sdk", "33");
    android_vm_set_property(vm, "ro.product.model", "Aurora VM");
    
    // Enable Dalvik/ART
    android_vm_enable_dalvik(vm, true);
    
    // Start VM (requires kernel/ramdisk loaded separately)
    android_vm_start(vm);
    
    // ... VM running ...
    
    // Cleanup
    android_vm_stop(vm);
    android_vm_destroy(vm);
}
```

### Example 2: Android Installer Usage

```c
#include "kernel/gui/android_installer.h"

void install_android_example(void) {
    // Initialize installer
    android_installer_init();
    
    // Get available distributions
    android_distro_t distros[10];
    int count = android_installer_get_distros(distros, 10);
    
    // Show installer UI
    android_installer_show_ui();
    
    // Start installation (e.g., AOSP 13.0 - index 0)
    android_installer_start(0);
    
    // Poll installation status
    android_installer_t status;
    while (1) {
        android_installer_get_status(&status);
        if (status.state == ANDROID_INSTALLER_COMPLETED) {
            break;
        }
        // Wait and update UI
    }
}
```

### Example 3: Workspace Configuration

```c
#include "kernel/gui/virtual_desktop.h"

void configure_android_workspace(void) {
    // Initialize virtual desktop
    vdesktop_init();
    
    // Set workspace 2 to Android VM
    vdesktop_set_os_type(2, WORKSPACE_OS_ANDROID);
    
    // Get current OS type
    int os_type = vdesktop_get_os_type(2);
    if (os_type == WORKSPACE_OS_ANDROID) {
        // Workspace is now Android VM
    }
    
    // Show OS selector for workspace 3
    vdesktop_show_os_selector(3);
    
    // Switch to Android workspace
    vdesktop_switch_to(2);
}
```

## References

### Documentation
- **Android VM Header**: `include/platform/android_vm.h`
- **Android VM Implementation**: `src/platform/android_vm.c`
- **Android Installer Header**: `kernel/gui/android_installer.h`
- **Android Installer Implementation**: `kernel/gui/android_installer.c`
- **Virtual Desktop Header**: `kernel/gui/virtual_desktop.h`
- **Boot Configuration**: `scripts/create_iso.sh`

### External Resources
- [Android Open Source Project](https://source.android.com/)
- [Android Boot Image Format](https://source.android.com/docs/core/architecture/bootloader/boot-image-header)
- [Bionic libc Documentation](https://android.googlesource.com/platform/bionic/)
- [Android Init Process](https://source.android.com/docs/core/architecture/bootloader/init)
- [Android Property System](https://source.android.com/docs/core/architecture/configuration/add-system-properties)

## Version History

### Version 1.0.0 (Current)
- Initial Android VM implementation
- Basic syscall emulation (15 syscalls)
- Android installer with 5 distributions
- Workspace integration with OS selector
- GRUB boot menu integration
- Property system support
- Multi-architecture support (ARM32/64, x86/64)

### Planned for Version 1.1.0
- Expanded syscall coverage (50+ syscalls)
- ext4 filesystem support
- Basic Binder IPC
- Framebuffer integration
- ADB support

### Planned for Version 2.0.0
- Full Android compatibility
- Hardware acceleration
- Google Play Services layer
- Complete Binder IPC
- SurfaceFlinger integration

---

**Document Version**: 1.0  
**Last Updated**: November 20, 2025  
**Author**: Aurora OS Team  
**License**: Proprietary – Aurora OS Project
