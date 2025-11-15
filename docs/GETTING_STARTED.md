# Aurora OS - Getting Started

## Prerequisites

To build and develop Aurora OS, you'll need:

- GCC (cross-compiler for i386-elf recommended)
- NASM (Netwide Assembler)
- GNU Make
- QEMU (for testing)
- Git

### Installing Prerequisites

#### On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential nasm qemu-system-x86 git
```

#### On macOS:
```bash
brew install i386-elf-gcc nasm qemu git
```

#### On Windows:
Use WSL2 with Ubuntu and follow Ubuntu instructions.

## Building Aurora OS

1. Clone the repository:
```bash
git clone https://github.com/kamer1337/aurora-os.git
cd aurora-os
```

2. Build the kernel:
```bash
make all
```

3. The compiled kernel will be located at `build/aurora-kernel.bin`

## Running in QEMU

To run Aurora OS in the QEMU emulator:

```bash
qemu-system-i386 -kernel build/aurora-kernel.bin
```

## Development Setup

1. **Set up your editor**: Configure your IDE/editor for C development
2. **Code style**: Follow the existing code style in the project
3. **Testing**: Test changes in QEMU before committing
4. **Documentation**: Update documentation when adding features

## Project Structure

See `docs/PROJECT_STRUCTURE.md` for detailed information about the project organization.

## Next Steps

- Read the main README.md for project overview
- Check docs/PROJECT_STRUCTURE.md for code organization
- Review the roadmap in README.md to see current priorities
- Pick a task from the roadmap and start coding!

## Getting Help

- Check the documentation in the `docs/` directory
- Review existing code for examples
- Submit issues on GitHub for bugs or feature requests

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

Welcome to Aurora OS development!
