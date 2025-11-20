#!/bin/bash
# Aurora OS - Bootable ISO Generation Script

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directories
BUILD_DIR="build"
ISO_DIR="$BUILD_DIR/iso"
GRUB_DIR="$ISO_DIR/boot/grub"
KERNEL_BIN="$BUILD_DIR/aurora-kernel.bin"
ISO_OUTPUT="$BUILD_DIR/aurora-os.iso"

echo -e "${BLUE}Aurora OS - ISO Generation${NC}"
echo "=============================="

# Check if kernel binary exists
if [ ! -f "$KERNEL_BIN" ]; then
    echo -e "${RED}Error: Kernel binary not found at $KERNEL_BIN${NC}"
    echo "Please run 'make all' first to build the kernel."
    exit 1
fi

# Check for required tools
echo "Checking required tools..."
if ! command -v grub-mkrescue &> /dev/null; then
    echo -e "${RED}Error: grub-mkrescue not found${NC}"
    echo "Please install GRUB tools:"
    echo "  Ubuntu/Debian: sudo apt-get install grub-pc-bin grub-common xorriso"
    echo "  Fedora/RHEL: sudo dnf install grub2-tools xorriso"
    exit 1
fi

if ! command -v xorriso &> /dev/null; then
    echo -e "${RED}Error: xorriso not found${NC}"
    echo "Please install xorriso:"
    echo "  Ubuntu/Debian: sudo apt-get install xorriso"
    echo "  Fedora/RHEL: sudo dnf install xorriso"
    exit 1
fi

echo -e "${GREEN}✓ All required tools found${NC}"

# Create ISO directory structure
echo "Creating ISO directory structure..."
mkdir -p "$GRUB_DIR"

# Copy kernel binary
echo "Copying kernel binary..."
cp "$KERNEL_BIN" "$ISO_DIR/boot/aurora-kernel.bin"

# Create GRUB configuration
echo "Creating GRUB configuration..."
cat > "$GRUB_DIR/grub.cfg" << 'EOF'
set timeout=10
set default=0

# Set graphics mode for VESA/VBE support
set gfxmode=1920x1080x32
set gfxpayload=keep
terminal_output gfxterm

# Aurora OS Boot Options
menuentry "Aurora OS" {
    multiboot /boot/aurora-kernel.bin
    boot
}

menuentry "Aurora OS (Safe Mode - Text Mode)" {
    terminal_output console
    multiboot /boot/aurora-kernel.bin safe
    boot
}

menuentry "Aurora OS (800x600)" {
    set gfxmode=800x600x32
    set gfxpayload=keep
    multiboot /boot/aurora-kernel.bin
    boot
}

# Linux Workspace Options
menuentry "Aurora OS with Linux VM Support" {
    multiboot /boot/aurora-kernel.bin linux_vm=enabled
    boot
}

menuentry "Linux Kernel (via Aurora VM)" {
    echo "Loading Linux kernel in Aurora VM..."
    multiboot /boot/aurora-kernel.bin boot_mode=linux_vm
    boot
}

# Placeholder for future Linux installation
menuentry "Install Linux (Coming Soon)" {
    echo "Linux installer integration coming soon..."
    echo "Press any key to return to menu..."
    read
}
EOF

echo -e "${GREEN}✓ GRUB configuration created${NC}"

# Generate ISO
echo "Generating bootable ISO..."
grub-mkrescue -o "$ISO_OUTPUT" "$ISO_DIR" 2>&1 | grep -v "WARNING: Skipping" || true

# Check if ISO was created successfully
if [ -f "$ISO_OUTPUT" ]; then
    ISO_SIZE=$(du -h "$ISO_OUTPUT" | cut -f1)
    echo -e "${GREEN}✓ ISO created successfully${NC}"
    echo "  Location: $ISO_OUTPUT"
    echo "  Size: $ISO_SIZE"
    echo ""
    echo "To test the ISO in QEMU, run:"
    echo "  ./scripts/run_qemu.sh"
else
    echo -e "${RED}Error: Failed to create ISO${NC}"
    exit 1
fi

# Clean up temporary ISO directory (keep the ISO file)
rm -rf "$ISO_DIR"

echo -e "${GREEN}Done!${NC}"
