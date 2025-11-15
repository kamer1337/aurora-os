# Aurora OS - TODO List

## Immediate Priorities (Current Sprint)

### Project Setup ✅
- [x] Create directory structure
- [x] Add basic kernel files
- [x] Create build system
- [x] Add documentation
- [x] Setup .gitignore

### Kernel Development (Next Up)
- [ ] Implement bootloader (GRUB multiboot)
- [ ] Add interrupt descriptor table (IDT)
- [ ] Implement basic interrupt handlers
- [ ] Create GDT (Global Descriptor Table)
- [ ] Setup paging for virtual memory
- [ ] Implement basic heap allocator
- [ ] Add kernel logging/debug output

### Process Management
- [ ] Implement context switching
- [ ] Create round-robin scheduler
- [ ] Add process creation/termination
- [ ] Implement system call interface
- [ ] Add basic IPC mechanisms

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

### Build & Testing
- [ ] Add linker script
- [ ] Create bootable ISO generation
- [ ] Setup QEMU testing scripts
- [ ] Add unit test framework
- [ ] Create continuous integration

## Long-term Goals

### Phase 2: Core Functionality
- [ ] Multi-core support
- [ ] Advanced memory management
- [ ] Network stack
- [ ] USB support

### Phase 3: File Systems
- [ ] Ext2/3/4 support
- [ ] FAT32 support
- [ ] Custom Aurora FS

### Phase 4: User Interface
- [ ] Framebuffer driver
- [ ] GUI framework
- [ ] Window manager
- [ ] Desktop environment

## Notes

- Priority should be on getting a minimal bootable kernel first
- Focus on core functionality before adding features
- Keep code modular and well-documented
- Test frequently in QEMU
