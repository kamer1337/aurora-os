# Linux VM and Workspace Selection Features

## Overview

Aurora OS now includes support for workspace OS selection and Linux kernel virtual machine integration. This allows users to:

1. **Select OS Type per Workspace**: Choose between native Aurora OS or Linux VM for each virtual desktop workspace
2. **Run Linux Workloads**: Execute Linux applications in dedicated workspaces via the Aurora VM
3. **Multi-Boot Options**: Boot into Aurora OS with optional Linux VM support or Linux kernel directly

## Features

### 1. Workspace OS Selection

Each of Aurora OS's 4 virtual workspaces can now be configured to run either:
- **Aurora OS** (default): Native Aurora OS environment with full desktop features
- **Linux VM**: Linux environment running through Aurora VM with compatibility layer

#### Using Workspace OS Selection

**Via Workspace Switcher:**
1. Press the workspace switcher hotkey (typically Alt+Tab or configured shortcut)
2. The workspace switcher overlay displays all workspaces with their current OS type:
   - Aurora OS workspaces show in blue
   - Linux VM workspaces show in orange/yellow
3. Select a workspace to switch to it

**Via OS Type Selector:**
1. Access the OS selector for a specific workspace
2. Choose between "Aurora OS" or "Linux VM"
3. The workspace name updates automatically (e.g., "Linux 1" for Linux workspaces)

**Programmatic API:**
```c
#include "kernel/gui/virtual_desktop.h"

// Set workspace to Linux VM mode
vdesktop_set_os_type(workspace_id, WORKSPACE_OS_LINUX);

// Get current workspace OS type
int os_type = vdesktop_get_os_type(workspace_id);

// Show OS selector UI for a workspace
vdesktop_show_os_selector(workspace_id);
```

### 2. Linux Kernel Virtual Machine

Aurora OS includes a Linux compatibility layer built on top of the Aurora VM:

#### Features
- Linux kernel image loading
- Initial ramdisk (initrd) support
- Basic Linux syscall emulation
- Kernel command line configuration
- VM state management (start, pause, resume, stop)

#### Supported Linux Syscalls
- `exit` - Process termination
- `read` - Read from file descriptor
- `write` - Write to file descriptor
- `open` - Open file
- `close` - Close file
- `getpid` - Get process ID
- `brk` - Memory allocation
- `mmap` / `munmap` - Memory mapping

Additional syscalls can be added as needed in `src/platform/linux_vm.c`.

#### Using Linux VM Programmatically

```c
#include "include/platform/linux_vm.h"

// Initialize Linux VM subsystem
linux_vm_init();

// Create a new Linux VM instance
LinuxVM* vm = linux_vm_create();

// Load a Linux kernel image
uint8_t* kernel_data = ...; // Your kernel image
uint32_t kernel_size = ...;
linux_vm_load_kernel(vm, kernel_data, kernel_size);

// Optional: Load initrd
uint8_t* initrd_data = ...;
uint32_t initrd_size = ...;
linux_vm_load_initrd(vm, initrd_data, initrd_size);

// Set kernel command line
linux_vm_set_cmdline(vm, "console=ttyS0 root=/dev/ram0");

// Start the VM
linux_vm_start(vm);

// ... VM runs ...

// Stop the VM
linux_vm_stop(vm);

// Clean up
linux_vm_destroy(vm);
```

### 3. Linux Installer

The Linux installer provides a user-friendly interface for installing Linux distributions:

#### Available Distributions

Currently available for installation:
- **Tiny Linux 1.0**: Minimal Linux distribution for embedded systems (50 MB)
- **Aurora Linux 1.0**: Custom Linux build optimized for Aurora OS (100 MB)

Coming soon:
- Debian Minimal 11.0 (500 MB)
- Alpine Linux 3.14 (150 MB)

#### Using the Installer

**Via GUI:**
1. Boot Aurora OS
2. Access the Linux installer from the start menu or system settings
3. Select a distribution from the list
4. Press the corresponding number key (1-4) to select
5. Installation proceeds automatically
6. Once complete, Linux VM workspaces can be configured

**Programmatic API:**
```c
#include "kernel/gui/linux_installer.h"

// Initialize installer
linux_installer_init();

// Get available distributions
linux_distro_t distros[10];
int count = linux_installer_get_distros(distros, 10);

// Start installation
linux_installer_start(distro_index);

// Check installation status
linux_installer_t status;
linux_installer_get_status(&status);

// Check if Linux is installed
if (linux_installer_is_installed()) {
    // Linux is ready to use
}
```

### 4. Boot Menu Options

Aurora OS uses GRUB as its bootloader with the following options:

1. **Aurora OS** (default): Standard Aurora OS boot
2. **Aurora OS (Safe Mode - Text Mode)**: Boot in text mode for troubleshooting
3. **Aurora OS (800x600)**: Boot with lower resolution
4. **Aurora OS with Linux VM Support**: Boot with Linux VM features enabled
5. **Linux Kernel (via Aurora VM)**: Boot directly into Linux kernel via Aurora VM
6. **Install Linux**: Access Linux installer (coming soon)

## Architecture

```
┌─────────────────────────────────────────────┐
│           Aurora OS User Space              │
├─────────────────────────────────────────────┤
│  Workspace 1   Workspace 2   Workspace 3    │
│  [Aurora OS]   [Linux VM]    [Aurora OS]    │
├─────────────────────────────────────────────┤
│      Virtual Desktop Manager                │
├─────────────────────────────────────────────┤
│  Linux VM Layer (syscall emulation)         │
├─────────────────────────────────────────────┤
│        Aurora VM (RISC VM Engine)           │
├─────────────────────────────────────────────┤
│          Aurora OS Kernel                   │
└─────────────────────────────────────────────┘
```

## Technical Details

### Workspace OS Types

```c
typedef enum {
    WORKSPACE_OS_AURORA = 0,    // Native Aurora OS workspace
    WORKSPACE_OS_LINUX = 1,     // Linux VM workspace
} workspace_os_type_t;
```

### Linux VM States

```c
typedef enum {
    LINUX_VM_STATE_UNINITIALIZED = 0,
    LINUX_VM_STATE_INITIALIZED,
    LINUX_VM_STATE_RUNNING,
    LINUX_VM_STATE_PAUSED,
    LINUX_VM_STATE_STOPPED,
    LINUX_VM_STATE_ERROR
} linux_vm_state_t;
```

### Memory Layout

- **Linux VM Memory**: 128 MB allocated per VM instance
- **Kernel Load Address**: 0x100000 (1 MB)
- **Maximum Processes**: 64 per Linux VM instance

## Limitations

Current implementation limitations:

1. **Linux Kernel Support**: Basic Linux kernel booting is implemented but full kernel execution requires:
   - Complete boot protocol implementation
   - Full syscall table
   - Device driver emulation

2. **File System**: Linux file system operations are stubbed and require implementation

3. **Networking**: Network stack integration between Linux VM and Aurora OS is pending

4. **Graphics**: Linux graphical applications require framebuffer/display integration

5. **Performance**: VM-based execution has inherent overhead compared to native execution

## Future Enhancements

Planned improvements:

- [ ] Complete Linux boot protocol implementation
- [ ] Full syscall table support
- [ ] File system persistence
- [ ] Network bridge between Linux VM and Aurora OS
- [ ] X11/Wayland display server support
- [ ] GPU passthrough for graphics acceleration
- [ ] Multi-core support for Linux VMs
- [ ] Container-based isolation
- [ ] Docker compatibility layer

## Building

The workspace selection and Linux VM features are built into the kernel by default:

```bash
# Build kernel with new features
make all

# Create bootable ISO with enhanced GRUB menu
make iso

# Run in QEMU
./scripts/run_qemu.sh
```

## Configuration Files

- **GRUB Config**: Generated by `scripts/create_iso.sh`
- **Workspace Config**: `kernel/gui/virtual_desktop.c`
- **Linux VM Config**: `include/platform/linux_vm.h`
- **Installer Config**: `kernel/gui/linux_installer.c`

## API Reference

### Virtual Desktop API

```c
// Initialize virtual desktop system
int vdesktop_init(void);

// Switch to workspace
int vdesktop_switch_to(uint8_t workspace_id);

// Set workspace OS type
int vdesktop_set_os_type(uint8_t workspace_id, workspace_os_type_t os_type);

// Get workspace OS type
int vdesktop_get_os_type(uint8_t workspace_id);

// Show OS selector
void vdesktop_show_os_selector(uint8_t workspace_id);
```

### Linux VM API

```c
// Initialize Linux VM
int linux_vm_init(void);

// VM lifecycle
LinuxVM* linux_vm_create(void);
void linux_vm_destroy(LinuxVM* vm);

// Kernel management
int linux_vm_load_kernel(LinuxVM* vm, const uint8_t* kernel_data, uint32_t size);
int linux_vm_load_initrd(LinuxVM* vm, const uint8_t* initrd_data, uint32_t size);
int linux_vm_set_cmdline(LinuxVM* vm, const char* cmdline);

// Execution control
int linux_vm_start(LinuxVM* vm);
int linux_vm_pause(LinuxVM* vm);
int linux_vm_resume(LinuxVM* vm);
int linux_vm_stop(LinuxVM* vm);

// Status
linux_vm_state_t linux_vm_get_state(LinuxVM* vm);
```

### Linux Installer API

```c
// Initialize installer
int linux_installer_init(void);

// Get available distributions
int linux_installer_get_distros(linux_distro_t* distros, uint32_t max_count);

// Installation
int linux_installer_start(uint32_t distro_index);
int linux_installer_cancel(void);

// Status
int linux_installer_get_status(linux_installer_t* installer);
bool linux_installer_is_installed(void);

// UI
void linux_installer_show_ui(void);
```

## Examples

See the following for usage examples:
- Workspace switching: `kernel/gui/virtual_desktop.c`
- Linux VM usage: `src/platform/linux_vm.c`
- Installer usage: `kernel/gui/linux_installer.c`

## Support

For issues, questions, or contributions:
- GitHub: https://github.com/kamer1337/aurora-os
- Documentation: See README.md and docs/ directory

## License

Aurora OS and its components are proprietary software.
See LICENSE file for details.
