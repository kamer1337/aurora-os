# Aurora OS - Project Structure

This document describes the organization of the Aurora OS project.

## Directory Structure

```
aurora-os/
├── boot/               # Bootloader components
├── kernel/             # Kernel source code
│   ├── core/          # Core kernel functionality
│   ├── memory/        # Memory management
│   ├── process/       # Process and thread management
│   ├── ipc/           # Inter-process communication
│   ├── drivers/       # Device drivers
│   └── boot/          # Kernel boot code
├── filesystem/         # File system implementations
│   ├── vfs/           # Virtual File System layer
│   └── journal/       # Journaling support
├── userspace/          # User space components
│   ├── apps/          # User applications
│   ├── libs/          # User space libraries
│   └── shell/         # Command shell
├── build/              # Build artifacts (generated)
├── docs/               # Documentation
├── tests/              # Test suite
└── tools/              # Build and development tools
```

## Component Overview

### Kernel Components

- **core/**: Main kernel initialization and core functionality
- **memory/**: Virtual memory management, paging, heap allocation
- **process/**: Process scheduling, thread management
- **ipc/**: Message passing and synchronization primitives
- **drivers/**: Hardware device drivers
- **boot/**: Early boot code and initialization

### File System

- **vfs/**: Virtual File System abstraction layer
- **journal/**: Journaling and transaction support

### User Space

- **apps/**: End-user applications
- **libs/**: Shared libraries and system APIs
- **shell/**: Command line interface

## Building Aurora OS

See `Makefile` for build instructions:

```bash
make all     # Build the kernel
make clean   # Clean build artifacts
make help    # Show available targets
```

## Development Workflow

1. Modify source files in respective directories
2. Run `make all` to build
3. Test changes in emulator/hardware
4. Run tests with `make test` (when implemented)
5. Commit changes with descriptive messages

## Coding Standards

- Use clear, descriptive function and variable names
- Add comments for complex logic
- Follow existing code style
- Document all public APIs
- Write tests for new functionality
