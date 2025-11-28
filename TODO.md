# Aurora OS - TODO List

**Last Updated**: November 28, 2025  
**Status**: Release Candidate v1.0

---

## Current Priority Tasks

### High Priority

- [ ] **Beta Testing Launch**
  - [ ] Recruit beta testers (target: 10-20 early adopters)
  - [ ] Setup feedback collection system (GitHub issues, surveys)
  - [ ] Monitor and respond to beta tester feedback

- [ ] **Real Hardware Testing**
  - [ ] Test on physical x86 hardware (beyond QEMU)
  - [ ] Verify VESA/VBE framebuffer on real hardware
  - [ ] Test keyboard and mouse drivers on physical devices

- [ ] **TCP/IP Network Stack**
  - [ ] Implement IP layer (IPv4)
  - [ ] Implement TCP protocol
  - [ ] Implement UDP protocol
  - [ ] Add socket API for applications

### Medium Priority

- [ ] **Persistent Storage**
  - [ ] Implement physical disk I/O (IDE/AHCI)
  - [ ] Enable ext4 filesystem on real disks
  - [ ] Add disk partition persistence

- [ ] **Package Manager**
  - [ ] Implement package installation from disk
  - [ ] Add dependency resolution
  - [ ] Create package repository infrastructure

- [ ] **Web Browser**
  - [ ] Basic HTML rendering
  - [ ] HTTP/HTTPS client
  - [ ] Integration with TCP/IP stack

### Low Priority

- [ ] **Image Wallpapers**
  - [ ] Implement PNG/BMP image loading
  - [ ] Support custom wallpaper images

- [ ] **USB Storage Support**
  - [ ] USB mass storage driver
  - [ ] USB device hot-plug support

- [ ] **Multi-User Support**
  - [ ] User account database
  - [ ] Permission system per user
  - [ ] User home directories

---

## Future Enhancements (v1.5+)

### Performance
- [ ] GPU-accelerated rendering (OpenGL/Vulkan)
- [ ] 4K/8K display support
- [ ] Boot time optimization (<3 seconds)

### Platform Support
- [ ] 64-bit kernel mode
- [ ] ARM processor support
- [ ] UEFI secure boot chain

### Enterprise Features
- [ ] LDAP/Active Directory integration
- [ ] VPN support
- [ ] Container runtime (Docker-compatible)

### Developer Tools
- [ ] Native debugger GUI
- [ ] Profiling tools
- [ ] GUI designer tool

---

## Completed Milestones ✅

All five development phases are complete:

- ✅ **Phase 1**: Design & Planning
- ✅ **Phase 2**: Kernel Implementation (memory, process, interrupts, SMP)
- ✅ **Phase 3**: File System & I/O (VFS, ramdisk, journaling, ext4)
- ✅ **Phase 4**: User Interface (GUI, 3D/4D/5D effects, desktop environment)
- ✅ **Phase 5**: Testing & Debugging (85/85 tests passing)

See [FEATURES.md](FEATURES.md) for complete list of implemented features.

---

## Known Limitations

| Limitation | Workaround | Priority |
|------------|------------|----------|
| Ramdisk only (no persistent disk) | ext4 works for VMs | High |
| No TCP/IP stack | Network bridge for VMs | High |
| No package manager | Built-in apps only | Medium |
| Basic USB support | Works for testing | Low |
| No image wallpapers | Gradients and live wallpapers work | Low |

---

## Development Guidelines

1. Test all changes in QEMU before commits
2. Run `make test` to verify tests pass
3. Keep code modular and well-documented
4. Avoid stubs and placeholders - implement features fully or defer
5. Update documentation when adding features

---

## Quick Reference

**Build**: `make all`  
**Test**: `make test`  
**Run in QEMU**: `make run`  
**Create ISO**: `make iso`

**Documentation**: See [DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
