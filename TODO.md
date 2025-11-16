# Aurora OS - TODO List

## Immediate Priorities (Current Sprint)

### Project Setup ✅
- [x] Create directory structure
- [x] Add basic kernel files
- [x] Create build system
- [x] Add documentation
- [x] Setup .gitignore

### Kernel Development ✅
- [x] Implement bootloader (GRUB multiboot)
- [x] Add interrupt descriptor table (IDT)
- [x] Implement basic interrupt handlers
- [x] Create GDT (Global Descriptor Table)
- [x] Setup paging for virtual memory
- [x] Implement basic heap allocator
- [x] Add kernel logging/debug output

### Process Management ✅
- [x] Implement context switching
- [x] Create round-robin scheduler
- [x] Add process creation/termination
- [x] Implement system call interface
- [x] Add basic IPC mechanisms

### File System
- [x] Complete VFS implementation
- [x] Create initial ramdisk support
- [x] Implement basic file operations
- [x] Add directory operations
- [x] Design journaling layer

### Device Drivers
- [x] Implement VGA text mode driver
- [x] Add keyboard driver
- [x] Create timer driver (PIT)
- [x] Implement serial port driver (for debugging)

### Security & Encryption ✅
- [x] Implement quantum cryptography module
- [x] Add quantum random number generator (QRNG)
- [x] Create quantum key management system
- [x] Implement quantum encryption/decryption
- [x] Add quantum hash functions
- [x] Integrate quantum crypto with kernel boot

### Build & Testing
- [x] Add quantum encryption for kernel
- [x] Add linker script
- [x] Create bootable ISO generation
- [x] Setup QEMU testing scripts
- [ ] Add unit test framework
- [ ] Create continuous integration

## Long-term Goals

### Phase 2: Core Functionality ✅
- [x] Basic memory management (heap allocator)
- [x] Process management with scheduler
- [x] Interrupt handling and system calls
- [ ] Multi-core support
- [ ] Advanced memory management (demand paging)
- [ ] Network stack
- [ ] USB support

### Phase 3: File Systems ✅
- [x] VFS implementation
- [x] Ramdisk support with read/write
- [x] Journaling layer
- [x] File operations (create, read, write, delete)
- [x] Directory operations
- [ ] Ext2/3/4 support
- [ ] FAT32 support
- [ ] Custom Aurora FS

### Phase 4: User Interface 🔄
- [x] Framebuffer driver
- [x] GUI framework (basic)
- [x] Basic drawing primitives
- [x] Window management (basic)
- [x] Widget system (buttons, labels, panels)
- [x] Mouse cursor support
- [x] Keyboard input for GUI
- [x] Advanced window manager (window dragging, focus)
- [ ] Desktop environment (partial)
- [ ] Menu system (widget types added)
- [ ] Minimize/maximize buttons
- [ ] Context menus
- [ ] Window list in taskbar

## Notes

- Priority should be on getting a minimal bootable kernel first
- Focus on core functionality before adding features
- Keep code modular and well-documented
- Test frequently in QEMU
