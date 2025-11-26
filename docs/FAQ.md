# Aurora OS - Troubleshooting and FAQ

**Version**: 1.0  
**Status**: Production Ready  
**Last Updated**: November 18, 2025

## Overview

This document provides answers to frequently asked questions and solutions to common problems encountered when using Aurora OS.

---

## Table of Contents

1. [General Questions](#general-questions)
2. [Installation Issues](#installation-issues)
3. [Boot Problems](#boot-problems)
4. [Graphics and Display](#graphics-and-display)
5. [Input Devices](#input-devices)
6. [Performance Issues](#performance-issues)
7. [Network Problems](#network-problems)
8. [Development and Building](#development-and-building)
9. [Advanced Troubleshooting](#advanced-troubleshooting)

---

## General Questions

### What is Aurora OS?

Aurora OS is a modern, secure operating system built from scratch with advanced features including:
- Microkernel architecture
- Quantum cryptography integration
- Advanced GUI with 3D/4D/5D visual effects
- VFS (Virtual File System) support
- Network capabilities
- Multi-core processor support

### What are the system requirements?

**Minimum**:
- x86-compatible 32-bit processor
- 64 MB RAM
- 10 MB storage
- VGA-compatible graphics

**Recommended**:
- Multi-core processor
- 256 MB RAM or more
- 100 MB storage
- VESA BIOS framebuffer support
- 1920x1080 display

### Is Aurora OS open source?

Yes, Aurora OS is available on GitHub: https://github.com/kamer1337/aurora-os

### Can I run Aurora OS on my PC?

Yes, but it's recommended to start with a virtual machine (QEMU or VirtualBox) for testing. Physical hardware installation is possible but may require specific hardware compatibility.

### What programming languages does Aurora OS support?

Aurora OS is written in C and x86 assembly. The kernel provides a C API for development. Future versions may support additional languages through userspace runtimes.

### Can I install applications on Aurora OS?

Currently, Aurora OS comes with built-in applications (Calculator, Text Editor, File Manager, etc.). An application installation system is planned for future releases.

---

## Installation Issues

### Q: ISO creation fails with "xorriso not found"

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install xorriso mtools

# Fedora/RHEL
sudo dnf install xorriso

# Verify installation
xorriso -version
```

### Q: Build fails with "gcc: command not found"

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential gcc make

# Fedora/RHEL
sudo dnf groupinstall "Development Tools"

# Verify installation
gcc --version
```

### Q: Build fails with "nasm: command not found"

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install nasm

# Fedora/RHEL
sudo dnf install nasm

# macOS
brew install nasm
```

### Q: Linking error: "cannot find -lgcc"

**Solution**:
This usually means you need a cross-compiler. Aurora OS targets i686-elf:

```bash
# Option 1: Use system GCC with 32-bit support
make CC="gcc -m32"

# Option 2: Build i686-elf cross-compiler
# See: https://wiki.osdev.org/GCC_Cross-Compiler
```

### Q: "grub-mkrescue: command not found"

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install grub-pc-bin grub-common

# Fedora/RHEL
sudo dnf install grub2 grub2-tools
```

---

## Boot Problems

### Q: GRUB shows "Error: no such partition" or "Error 15"

**Causes**:
- Corrupted ISO image
- Incorrect USB writing method
- BIOS incompatibility

**Solutions**:
1. Rebuild ISO:
   ```bash
   make clean
   make all
   make iso
   ```

2. For USB, use DD mode in Rufus (Windows) or dd command (Linux):
   ```bash
   sudo dd if=aurora-os.iso of=/dev/sdX bs=4M status=progress
   ```

3. Try booting in legacy BIOS mode (disable UEFI in BIOS settings)

### Q: Kernel panics immediately after GRUB

**Possible Causes**:
- Corrupted kernel binary
- Incompatible hardware
- Memory issues

**Solutions**:
1. Rebuild kernel:
   ```bash
   make clean
   make all
   ```

2. Try with more RAM in VM:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 512
   ```

3. Check build output for errors

### Q: System hangs at "Initializing..."

**Diagnosis**:
The boot process is stuck during initialization.

**Solutions**:
1. Enable serial port debugging in QEMU:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -serial stdio
   ```
   
2. Check which component is failing in the serial output

3. Try disabling optional plugins:
   ```bash
   make clean
   make ENABLE_ML_PLUGIN=0 ENABLE_QUANTUM_PLUGIN=0 all
   ```

### Q: Tests run forever and never reach GUI

**Explanation**:
Aurora OS runs comprehensive tests during boot. This is normal and can take 10-60 seconds.

**Solutions**:
1. Wait for tests to complete
2. To skip tests, modify `kernel/core/kernel.c` and comment out test functions
3. Build custom kernel without test suite

---

## Graphics and Display

### Q: Black screen after boot, but system seems running

**Diagnosis**:
Framebuffer initialization failed. System is running in background.

**Solutions**:

1. **In QEMU**: Try different VGA options
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga std
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga cirrus
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga vmware
   ```

2. **In VirtualBox**: Change graphics controller
   - Settings → Display → Graphics Controller
   - Try: VBoxVGA, VMSVGA, VBoxSVGA

3. **Physical Hardware**: Ensure VESA BIOS support in BIOS settings

### Q: Display resolution is wrong

**Explanation**:
Aurora OS attempts to use multiboot framebuffer info from bootloader.

**Solutions**:
1. Modify GRUB configuration to set specific resolution:
   ```
   set gfxmode=1920x1080x32
   set gfxpayload=keep
   ```

2. In VirtualBox: Enable 3D acceleration and adjust video memory

3. Default fallback is 1024x768x32

### Q: Colors look wrong or distorted

**Possible Causes**:
- Incorrect color depth
- Framebuffer format mismatch

**Solutions**:
1. Ensure 32-bit color depth:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -vga std
   ```

2. Check framebuffer initialization in serial output

3. Verify RGBA color format support

### Q: GUI is very slow or choppy

**Causes**:
- Insufficient RAM
- Software rendering without acceleration
- Too many visual effects

**Solutions**:
1. Allocate more RAM:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 512
   ```

2. Enable KVM acceleration (Linux):
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -enable-kvm
   ```

3. Disable visual effects in Settings:
   - Open Settings app
   - Disable Animations
   - Disable Transparency
   - Use solid color wallpaper

4. In VirtualBox:
   - Enable VT-x/AMD-V in BIOS
   - Settings → System → Enable VT-x/AMD-V
   - Increase video memory to 128 MB

### Q: Screen tearing or artifacts

**Solutions**:
1. Increase QEMU graphics memory
2. Enable vsync if available
3. Reduce animation speed in Settings

---

## Input Devices

### Q: Keyboard doesn't work

**Diagnosis**:
Keyboard driver failed to initialize or input not being processed.

**Solutions**:

1. **In QEMU**: Ensure proper input device
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -device usb-kbd
   ```

2. **In VirtualBox**:
   - Settings → System → Enable USB Controller
   - Try USB 1.1 (OHCI) Controller
   - Disable Mouse Integration temporarily

3. **Physical Hardware**:
   - Use PS/2 keyboard (better compatibility)
   - Check BIOS for USB legacy support
   - Try different USB ports

4. Check serial output for keyboard initialization errors

### Q: Mouse cursor doesn't appear or move

**Solutions**:

1. **In QEMU**: Add USB mouse
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -usb -device usb-mouse
   ```

2. **In VirtualBox**:
   - Settings → System → Pointing Device → PS/2 Mouse
   - Disable Mouse Integration in Input menu
   - Verify mouse is captured (click in window)

3. **Physical Hardware**:
   - Use PS/2 mouse for best compatibility
   - Enable USB mouse support in BIOS

### Q: Keyboard input is delayed or repeats

**Causes**:
- High system load
- Interrupt handling issues
- Timer configuration

**Solutions**:
1. Check NFR report for interrupt latency
2. Reduce GUI effects to lower system load
3. Allocate more CPU resources to VM

### Q: Special keys (arrows, function keys) don't work

**Status**: Limited support in v1.0

**Workaround**:
- Arrow keys work in menus
- Function keys not currently mapped
- Future versions will expand keyboard support

---

## Performance Issues

### Q: System boots slowly (>30 seconds)

**Explanation**:
Boot time includes:
- Hardware initialization
- Driver loading
- Test suite execution (~10-20 seconds)
- GUI initialization

**Solutions**:
1. Skip tests by commenting out test calls in `kernel_main()`
2. Allocate more RAM to speed up ramdisk creation
3. Enable KVM/hardware acceleration

### Q: GUI frame rate is low (<60 FPS)

**Target**: Aurora OS aims for ~125 FPS software rendering

**Solutions**:
1. Check NFR report for actual FPS:
   - It's displayed during boot
   - Shows target vs. actual performance

2. Optimize VM settings:
   - More RAM: 512 MB or 1 GB
   - More CPU cores: 2-4 cores
   - Enable hardware virtualization

3. Reduce visual complexity:
   - Solid color wallpaper
   - Disable animations
   - Close extra windows

### Q: High memory usage

**Diagnosis**:
Check NFR report for memory statistics.

**Normal Usage**:
- Kernel: ~10-20 MB
- GUI buffers: ~6-16 MB (depending on resolution)
- File cache: ~4 MB
- Total: ~30-50 MB typical

**Solutions if memory is low**:
1. Flush file cache: `file_cache_flush()`
2. Close unused windows
3. Allocate more RAM to VM

### Q: Unresponsive system or hanging

**Diagnosis**:
1. Check NFR metrics for issues
2. Review interrupt latency
3. Check for infinite loops

**Solutions**:
1. Restart the system
2. Enable serial debugging for diagnosis
3. Check for resource exhaustion (memory, processes)

---

## Network Problems

### Q: Network device not detected

**Status**: Network support is through Aurora VM abstraction layer

**Requirements**:
- Running in QEMU with network device
- Network device must be configured in VM

**QEMU Setup**:
```bash
qemu-system-i386 -cdrom aurora-os.iso -m 256 \
  -device e1000,netdev=net0 \
  -netdev user,id=net0
```

### Q: Cannot send or receive packets

**Diagnosis**:
Check network device connection status in code.

**Solutions**:
1. Verify network device initialization
2. Check TX/RX queue status
3. Verify packet size (max 1500 bytes MTU)
4. Enable network interrupts

### Q: Network performance is slow

**Current Limitations**:
- Basic packet I/O only
- No TCP/IP stack yet
- Limited protocol support

**Future Enhancements** (v1.x+):
- Full TCP/IP stack
- Socket API
- Network applications

---

## Development and Building

### Q: How do I add a new application?

**Steps**:
1. Create widget elements in GUI
2. Register in start menu (see `application.c`)
3. Implement application logic
4. Rebuild kernel

**Example**: See existing apps in `kernel/gui/application.c`

### Q: How do I modify the GUI theme?

**Steps**:
1. Edit `kernel/gui/theme_manager.c`
2. Modify theme definitions
3. Rebuild: `make clean && make all`
4. Apply theme through Settings or API

### Q: How do I add a new font?

**Steps**:
1. Create font bitmap data (8x8 or 5x7 grid)
2. Add to `kernel/gui/font_manager.c`
3. Register in font registry
4. Rebuild kernel

**Reference**: See [FONT_MANAGER.md](FONT_MANAGER.md)

### Q: How do I create a plugin?

**Steps**:
1. Study existing plugins in `kernel/plugins/`
2. Create plugin descriptor structure
3. Implement init, update, cleanup functions
4. Register plugin in `kernel/core/kernel.c`
5. Rebuild with plugin enabled

**Reference**: See [PLUGIN_SYSTEM.md](PLUGIN_SYSTEM.md)

### Q: Build succeeds but ISO doesn't work

**Checklist**:
1. Verify kernel binary exists: `build/aurora-kernel.bin`
2. Check kernel size (should be ~500KB-2MB)
3. Verify ISO was created: `aurora-os.iso`
4. Check ISO size (should be ~2-5 MB)
5. Try rebuilding completely: `make clean && make all && make iso`

### Q: How do I debug kernel code?

**Methods**:

1. **Serial Port Output**:
   ```c
   serial_write(SERIAL_COM1, "Debug point reached\n");
   ```
   
   Run QEMU with serial:
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -serial stdio
   ```

2. **VGA Text Output**:
   ```c
   vga_write("Variable value: ");
   vga_write_dec(my_variable);
   vga_write("\n");
   ```

3. **GDB Debugging** (advanced):
   ```bash
   qemu-system-i386 -cdrom aurora-os.iso -m 256 -s -S
   # In another terminal:
   gdb build/aurora-kernel.bin
   (gdb) target remote localhost:1234
   (gdb) continue
   ```

---

## Advanced Troubleshooting

### Enabling Verbose Boot

Modify `kernel/core/kernel.c` to add more status messages:
```c
vga_write("Checkpoint: Component initialized\n");
```

### Analyzing NFR Metrics

The NFR (Non-Functional Requirements) module provides performance data:
- Boot time
- GUI frame rate
- Memory usage
- Interrupt latency
- System responsiveness

Check the report during boot or call `nfr_print_report()` in code.

### Memory Leak Detection

**Current Status**: Manual inspection required

**Future**: Automated leak detection in v1.x

**Manual Check**:
1. Monitor memory usage over time
2. Check for growing allocations
3. Verify all `kmalloc()` have matching `kfree()`

### Interrupt Debugging

Check interrupt handlers:
```c
// View interrupt statistics
vga_write("Timer ticks: ");
vga_write_dec(get_timer_ticks());
vga_write("\n");
```

### File System Issues

**Ramdisk Problems**:
1. Verify ramdisk creation succeeded
2. Check ramdisk size allocation
3. Test with smaller ramdisk if memory limited

**VFS Issues**:
1. Check VFS initialization
2. Verify file operations return correct error codes
3. Check journal status

### GUI Debugging

**Widget Issues**:
1. Verify window is visible: `gui_show_window(window)`
2. Check widget bounds (x, y, width, height)
3. Verify parent window exists
4. Check z-order for overlapping widgets

**Rendering Issues**:
1. Check framebuffer initialization
2. Verify color format (RGBA)
3. Test with simple solid colors first
4. Reduce complexity to isolate issue

### Plugin Problems

**Plugin Won't Load**:
1. Verify plugin is registered
2. Check API version compatibility
3. Verify initialization function exists
4. Check for initialization errors

**Plugin Crashes System**:
1. Review plugin code for memory issues
2. Check for null pointer dereferences
3. Verify proper resource cleanup
4. Test plugin in isolation

---

## Getting Additional Help

### Documentation Resources
- **Installation Guide**: [INSTALLATION_GUIDE.md](INSTALLATION_GUIDE.md)
- **System Administration**: [SYSTEM_ADMINISTRATION.md](SYSTEM_ADMINISTRATION.md)
- **API Reference**: [API_REFERENCE.md](API_REFERENCE.md)

### Community Support
- **GitHub Repository**: https://github.com/kamer1337/aurora-os
- **Issue Tracker**: Report bugs on GitHub Issues
- **Discussions**: Ask questions on GitHub Discussions

### Reporting Bugs

When reporting issues, include:
1. **Aurora OS Version**: v1.0
2. **Environment**: VM type, host OS, hardware specs
3. **Steps to Reproduce**: Detailed steps
4. **Expected Behavior**: What should happen
5. **Actual Behavior**: What actually happens
6. **Logs**: Serial output, error messages
7. **Screenshots**: If visual issue

### Contributing Fixes

Found a solution? Contribute back:
1. Fork the repository
2. Create a feature branch
3. Implement fix with tests
4. Submit pull request
5. Document the fix in CHANGELOG

---

## Known Issues

### v1.0 Known Limitations (Updated November 2025)

1. ~~**Single-user system**: No user authentication~~ ✅ RESOLVED: Login/guest screen with user authentication implemented
2. **Ramdisk only**: No persistent storage (ramdisk data lost on reboot)
3. **Limited network**: Packet I/O only, no TCP/IP
4. **USB support**: Basic only
5. **File systems**: Ramdisk only (no Ext2/FAT32)
6. **Applications**: Built-in only, no app installation
7. ~~**Configuration**: Not persisted across reboots~~ ✅ RESOLVED: Desktop Configuration Persistence implemented with VFS-integrated save/load

### Recently Resolved Issues (November 2025) ✅

- **User Authentication**: Login/guest screen now available with session management
- **Desktop Configuration**: Settings now persist via VFS with checksum validation
- **Multiple Fonts**: 4 configurable fonts now available (Standard 8x8, Crystalline 8x8, Crystalline 5x7, Monospace 6x8)
- **Linux/Android VM**: Syscall implementations complete with installer workflows

### Planned Fixes (v1.1+)

See [ROADMAP.md](../ROADMAP.md) for future enhancements.

---

**Document Maintainer**: Aurora OS Project Team  
**Last Updated**: November 26, 2025  
**Next Review**: v1.1 Release (Q2 2026)
