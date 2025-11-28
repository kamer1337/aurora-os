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

- [x] **TCP/IP Network Stack** ✅
  - [x] Implement IP layer (IPv4)
  - [x] Implement TCP protocol
  - [x] Implement UDP protocol
  - [x] Add socket API for applications

### Medium Priority

- [x] **Persistent Storage** ✅
  - [x] Implement physical disk I/O (IDE/AHCI)
  - [x] Enable ext4 filesystem on real disks
  - [ ] Add disk partition persistence

- [x] **Package Manager** ✅
  - [x] Implement package installation from disk
  - [x] Add dependency resolution
  - [ ] Create package repository infrastructure

- [x] **Web Browser** ✅
  - [x] Basic HTML rendering
  - [x] HTTP/HTTPS client
  - [x] Integration with TCP/IP stack

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

### Recently Completed (November 28, 2025) ✅

- ✅ **Network Configuration Persistence**
  - Implemented VFS-integrated save/load for network configuration
  - Added checksum validation and file format versioning
  
- ✅ **DNS Reverse Lookup**
  - Full PTR record query implementation
  - Socket-based DNS query/response handling
  
- ✅ **JIT Compilation Backend**
  - x86 native code generation for ADD, SUB, LOADI instructions
  - Basic block scanning and control flow analysis
  
- ✅ **GDB Remote Debugging Protocol**
  - Full RSP protocol implementation (registers, memory, breakpoints)
  - Supports continue, single-step, and query commands
  
- ✅ **PE Loader Import Resolution**
  - Imports by ordinal and name now resolved via DLL loader
  - IAT updated with resolved function addresses
  
- ✅ **Android Boot Device Loading**
  - Storage driver integration for boot image loading
  - Partition table scanning and boot partition detection
  
- ✅ **Android Kernel Boot Sequence**
  - Kernel/ramdisk copy to target addresses
  - Boot parameters setup (Linux setup header)
  
- ✅ **AVB Signature Verification**
  - AVB header parsing and validation
  - Hash computation for boot image verification

- ✅ **TCP/IP Network Stack**
  - Full IPv4/TCP/UDP/ICMP implementation
  - Socket API for applications (create, bind, connect, send, receive, close)
  
- ✅ **Persistent Storage (AHCI/ext4)**
  - SATA AHCI controller driver for physical disk I/O
  - ext4 filesystem implementation for real disks
  
- ✅ **Package Manager**
  - Package installation with dependency resolution
  - Package search, update, upgrade, and removal operations
  
- ✅ **Web Browser**
  - Multi-tab browsing with tab management
  - Cookie/session management, download manager, extension support
  - Custom HTML/CSS rendering with DevTools integration
  
See [FEATURES.md](FEATURES.md) for complete list of implemented features.

---

## Known Limitations

| Limitation | Workaround | Priority |
|------------|------------|----------|
| ~~Ramdisk only (no persistent disk)~~ | ~~ext4 works for VMs~~ | ~~High~~ ✅ AHCI/ext4 implemented |
| ~~No TCP/IP stack~~ | ~~Network bridge for VMs~~ | ~~High~~ ✅ TCP/IP implemented |
| ~~No package manager~~ | ~~Built-in apps only~~ | ~~Medium~~ ✅ Package manager implemented |
| Basic USB support | USB 3.0/3.1 framework exists but needs full implementation | Low |
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
