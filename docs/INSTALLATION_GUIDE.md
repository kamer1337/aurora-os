# Aurora OS - User Installation Guide

**Version**: 1.0  
**Status**: Production Ready  
**Last Updated**: November 18, 2025

## Overview

This guide provides step-by-step instructions for installing and running Aurora OS on your system. Aurora OS can be run in a virtual machine (recommended for testing) or installed on physical hardware.

---

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Quick Start (Virtual Machine)](#quick-start-virtual-machine)
3. [Building from Source](#building-from-source)
4. [Installation Methods](#installation-methods)
5. [First Boot](#first-boot)
6. [Troubleshooting](#troubleshooting)
7. [Uninstallation](#uninstallation)

---

## System Requirements

### Minimum Requirements
- **CPU**: x86-compatible processor (32-bit)
- **RAM**: 64 MB
- **Storage**: 10 MB free space
- **Graphics**: VGA-compatible display adapter
- **Display**: 1024x768 minimum resolution

### Recommended Requirements
- **CPU**: x86-compatible processor with multi-core support
- **RAM**: 256 MB or more
- **Storage**: 100 MB free space
- **Graphics**: VESA BIOS compatible graphics with framebuffer support
- **Display**: 1920x1080 resolution for best visual experience

### Supported Virtual Machines
- **QEMU** (recommended) - Version 5.0 or later
- **VirtualBox** - Version 6.0 or later
- **VMware Workstation** - Version 15 or later
- **KVM/QEMU** on Linux

---

## Quick Start (Virtual Machine)

The easiest way to try Aurora OS is using QEMU. This method requires minimal setup and is ideal for testing.

### Prerequisites

Install QEMU on your system:

**Ubuntu/Debian:**
```bash
sudo apt-get install qemu-system-x86
```

**Fedora/RHEL:**
```bash
sudo dnf install qemu-system-x86
```

**macOS (with Homebrew):**
```bash
brew install qemu
```

**Windows:**
Download and install QEMU from https://www.qemu.org/download/

### Running Aurora OS

1. **Download or Build Aurora OS**
   
   Clone the repository:
   ```bash
   git clone https://github.com/kamer1337/aurora-os.git
   cd aurora-os
   ```

2. **Build the OS** (see [Building from Source](#building-from-source) for details)
   ```bash
   make clean
   make all
   make iso
   ```

3. **Run in QEMU**
   ```bash
   make run
   ```
   
   Or manually:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga std
   ```

4. **Aurora OS will boot automatically**
   - You'll see the GRUB bootloader
   - Aurora OS will start and show the desktop environment
   - Use mouse and keyboard to interact with the GUI

---

## Building from Source

Aurora OS is built from source using standard development tools.

### Prerequisites

**Required Tools:**
- GCC cross-compiler for i686-elf target
- NASM assembler
- GNU Make
- GRUB (for creating bootable ISO)
- xorriso (for ISO creation)

**Ubuntu/Debian Installation:**
```bash
sudo apt-get update
sudo apt-get install build-essential nasm grub-pc-bin xorriso mtools
```

**Fedora/RHEL Installation:**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install nasm grub2 grub2-tools xorriso
```

### Building Aurora OS

1. **Clone the Repository**
   ```bash
   git clone https://github.com/kamer1337/aurora-os.git
   cd aurora-os
   ```

2. **Build the Kernel**
   ```bash
   make clean
   make all
   ```
   
   This will compile:
   - Kernel core modules
   - Memory management subsystem
   - Process scheduler
   - File system (VFS, ramdisk, journal)
   - Device drivers
   - GUI framework
   - Network stack
   - Security modules

3. **Create Bootable ISO**
   ```bash
   make iso
   ```
   
   This creates `aurora-os.iso` in the current directory.

4. **Verify Build**
   ```bash
   ls -lh aurora-os.iso
   ```
   
   The ISO should be approximately 2-5 MB in size.

### Build Options

**Enable/Disable Optional Plugins:**
```bash
# Disable Machine Learning plugin
make ENABLE_ML_PLUGIN=0

# Disable Quantum Computing plugin
make ENABLE_QUANTUM_PLUGIN=0

# Disable System Optimization plugin
make ENABLE_SYSTEM_OPT_PLUGIN=0
```

**Clean Build:**
```bash
make clean      # Remove build artifacts
make all        # Rebuild everything
```

---

## Installation Methods

### Method 1: Running in QEMU (Recommended for Testing)

**Basic Launch:**
```bash
qemu-system-i386 -cdrom aurora-os.iso -m 256
```

**Recommended Options:**
```bash
qemu-system-i386 \
  -cdrom aurora-os.iso \
  -m 256 \
  -vga std \
  -rtc base=localtime \
  -device e1000,netdev=net0 \
  -netdev user,id=net0
```

**Options Explained:**
- `-cdrom aurora-os.iso`: Boot from ISO image
- `-m 256`: Allocate 256 MB RAM
- `-vga std`: Use standard VGA adapter
- `-rtc base=localtime`: Set real-time clock
- `-device e1000`: Add network card (Intel E1000)
- `-netdev user,id=net0`: Enable user-mode networking

**Enable KVM Acceleration (Linux):**
```bash
qemu-system-i386 -cdrom aurora-os.iso -m 256 -enable-kvm
```

### Method 2: Running in VirtualBox

1. **Create New Virtual Machine**
   - Open VirtualBox
   - Click "New"
   - Name: Aurora OS
   - Type: Other
   - Version: Other/Unknown

2. **Configure VM Settings**
   - RAM: 256 MB (minimum), 512 MB (recommended)
   - Hard Disk: Not required for ISO boot
   - Enable PAE/NX in System settings
   - Graphics Controller: VBoxVGA or VMSVGA
   - Video Memory: 32 MB or more

3. **Attach ISO**
   - Go to Settings → Storage
   - Click on "Empty" under Controller: IDE
   - Click disk icon → Choose disk file
   - Select `aurora-os.iso`

4. **Boot VM**
   - Click "Start"
   - Aurora OS will boot from ISO

### Method 3: Physical Hardware Installation

**⚠️ Warning**: Installing on physical hardware will modify your system. Backup important data first!

**Requirements:**
- USB drive (minimum 16 MB)
- BIOS with legacy boot support or UEFI with CSM enabled

**Steps:**

1. **Create Bootable USB**
   
   On Linux:
   ```bash
   sudo dd if=aurora-os.iso of=/dev/sdX bs=4M status=progress
   sudo sync
   ```
   Replace `/dev/sdX` with your USB device (e.g., `/dev/sdb`).

   On Windows (use Rufus or similar tool):
   - Download Rufus from https://rufus.ie
   - Select aurora-os.iso
   - Choose "DD Image" mode
   - Click Start

2. **Boot from USB**
   - Insert USB drive into target computer
   - Restart computer
   - Enter BIOS/UEFI (usually F2, F12, Del, or Esc key)
   - Change boot order to boot from USB first
   - Save and exit BIOS

3. **Aurora OS Will Boot**
   - GRUB bootloader will appear
   - Aurora OS will start automatically
   - Desktop environment will load

**Known Hardware Compatibility:**
- Intel Core i3/i5/i7 processors: ✓ Compatible
- AMD processors: ✓ Compatible (limited testing)
- VGA/VESA graphics: ✓ Required for GUI
- PS/2 keyboard: ✓ Recommended
- USB keyboard: ⚠️ Basic support
- PS/2 mouse: ✓ Supported
- USB mouse: ⚠️ Basic support

---

## First Boot

### What to Expect

1. **GRUB Bootloader**
   - Shows "Aurora OS" entry
   - Automatically boots in 5 seconds
   - Press Enter to boot immediately

2. **Kernel Initialization**
   - VGA text mode shows initialization messages
   - Memory, processes, interrupts initialized
   - Drivers loaded
   - File systems mounted

3. **Test Suite (Optional)**
   - Comprehensive system tests run automatically
   - Tests verify kernel subsystems
   - Results displayed in VGA text mode
   - Tests complete in ~10-30 seconds

4. **GUI Desktop Environment**
   - Framebuffer graphics mode activated
   - Desktop with gradient wallpaper appears
   - Taskbar at bottom of screen
   - Start menu button in bottom-left corner

### Using Aurora OS

**Desktop Features:**
- **Start Menu**: Click the start button (bottom-left) to open application menu
- **Taskbar**: Shows running applications and system tray
- **Windows**: Click and drag window title bars to move windows
- **Context Menus**: Right-click on window title bars for options
- **Desktop Actions**: Right-click on desktop for actions

**Available Applications:**
- Calculator
- Text Editor
- Terminal
- File Manager
- Settings

**Keyboard Shortcuts:**
- `Esc`: Close start menu or context menu
- Arrow keys: Navigate menus
- Enter: Select menu item

**Mouse Controls:**
- Left-click: Select, activate, click buttons
- Right-click: Context menu
- Drag: Move windows

---

## Troubleshooting

### Issue: Black Screen After Boot

**Possible Causes:**
- Incompatible graphics hardware
- Missing VESA BIOS support

**Solutions:**
1. Try different QEMU graphics options:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga cirrus
   ```

2. In VirtualBox, change Graphics Controller:
   - Settings → Display → Graphics Controller
   - Try: VBoxVGA, VMSVGA, or VBoxSVGA

3. Check BIOS settings for VGA/graphics support

### Issue: Keyboard Not Working

**Solutions:**
1. In QEMU, ensure proper input device:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -device usb-kbd
   ```

2. In VirtualBox:
   - Settings → System → Enable USB Controller
   - Try USB 1.1 (OHCI) Controller

3. Physical hardware: Use PS/2 keyboard instead of USB

### Issue: Mouse Not Detected

**Solutions:**
1. In QEMU:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -usb -device usb-mouse
   ```

2. In VirtualBox:
   - Settings → System → Pointing Device → PS/2 Mouse
   - Disable Mouse Integration temporarily

3. Check if mouse is properly connected (PS/2 recommended)

### Issue: Slow Performance

**Solutions:**
1. Allocate more RAM:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 512
   ```

2. Enable KVM acceleration (Linux only):
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -enable-kvm
   ```

3. In VirtualBox:
   - Enable VT-x/AMD-V in BIOS
   - Settings → System → Enable VT-x/AMD-V
   - Increase CPU cores to 2 or more

4. Close background applications on host system

### Issue: Build Errors

**Missing GCC cross-compiler:**
```bash
# Install i686-elf cross-compiler
# See: https://wiki.osdev.org/GCC_Cross-Compiler
```

**Missing NASM:**
```bash
sudo apt-get install nasm
```

**Linking errors:**
```bash
make clean
make all
```

### Issue: Cannot Create ISO

**Missing xorriso:**
```bash
sudo apt-get install xorriso mtools
```

**GRUB errors:**
```bash
sudo apt-get install grub-pc-bin grub-common
```

---

## Uninstallation

### Removing from Virtual Machine

**QEMU:**
- Simply delete the `aurora-os.iso` file
- No installation is performed on the host system

**VirtualBox:**
1. Select the Aurora OS VM
2. Right-click → Remove
3. Choose "Delete all files"
4. Delete `aurora-os.iso`

### Removing from USB Drive

**Linux:**
```bash
# Reformat USB drive
sudo mkfs.vfat /dev/sdX
```

**Windows:**
- Open Disk Management
- Select USB drive
- Format → FAT32

### Removing Build Files

```bash
cd aurora-os
make clean
```

To remove the entire source:
```bash
cd ..
rm -rf aurora-os
```

---

## Getting Help

### Documentation
- **API Reference**: See [API_REFERENCE.md](API_REFERENCE.md)
- **System Administration**: See [SYSTEM_ADMINISTRATION.md](SYSTEM_ADMINISTRATION.md)
- **FAQ**: See [FAQ.md](FAQ.md)

### Community
- **Repository**: https://github.com/kamer1337/aurora-os
- **Issues**: Report bugs on GitHub Issues
- **Discussions**: GitHub Discussions

### Reporting Issues

When reporting issues, please include:
1. Aurora OS version
2. Host operating system
3. Virtualization software and version
4. Hardware specifications
5. Steps to reproduce the issue
6. Screenshots or error messages

---

## Next Steps

After successful installation:
1. Read the [System Administration Manual](SYSTEM_ADMINISTRATION.md)
2. Explore the [API Reference](API_REFERENCE.md) for development
3. Check [Troubleshooting and FAQ](FAQ.md) for common questions
4. Review the [Release Notes](RELEASE_NOTES_v1.0.md) for features and known issues

---

**Document Maintainer**: Aurora OS Project Team  
**Review Cycle**: Per release  
**Next Review**: v1.1 Release (Q2 2026)
