#!/bin/bash
# Aurora OS - QEMU Testing Script

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default configuration
ISO_FILE="build/aurora-os.iso"
KERNEL_BIN="build/aurora-kernel.bin"
MEMORY="128M"
DISPLAY_MODE="stdio"
ENABLE_GDB=""
ENABLE_SERIAL=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--memory)
            MEMORY="$2"
            shift 2
            ;;
        -g|--gdb)
            ENABLE_GDB="-s -S"
            echo -e "${YELLOW}GDB debugging enabled. Connect with: gdb build/aurora-kernel.bin -ex 'target remote :1234'${NC}"
            shift
            ;;
        -s|--serial)
            ENABLE_SERIAL="-serial mon:stdio"
            DISPLAY_MODE="none"
            shift
            ;;
        -k|--kernel)
            USE_KERNEL_DIRECT="yes"
            shift
            ;;
        -h|--help)
            echo "Aurora OS - QEMU Testing Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -m, --memory SIZE    Set RAM size (default: 128M)"
            echo "  -g, --gdb           Enable GDB debugging on port 1234"
            echo "  -s, --serial        Redirect output to serial (console)"
            echo "  -k, --kernel        Boot kernel directly (no ISO)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                  # Run normally with ISO"
            echo "  $0 -m 256M          # Run with 256MB RAM"
            echo "  $0 -g               # Run with GDB debugging"
            echo "  $0 -s               # Run with serial console"
            echo "  $0 -k               # Boot kernel directly"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Aurora OS - QEMU Testing${NC}"
echo "=========================="

# Check for QEMU
if ! command -v qemu-system-i386 &> /dev/null; then
    echo -e "${RED}Error: qemu-system-i386 not found${NC}"
    echo "Please install QEMU:"
    echo "  Ubuntu/Debian: sudo apt-get install qemu-system-x86"
    echo "  Fedora/RHEL: sudo dnf install qemu-system-x86"
    echo "  macOS: brew install qemu"
    exit 1
fi

# Determine boot method
if [ "$USE_KERNEL_DIRECT" = "yes" ]; then
    # Boot kernel directly
    if [ ! -f "$KERNEL_BIN" ]; then
        echo -e "${RED}Error: Kernel binary not found at $KERNEL_BIN${NC}"
        echo "Please run 'make all' first."
        exit 1
    fi
    
    echo -e "${GREEN}Booting kernel directly...${NC}"
    echo "Memory: $MEMORY"
    
    qemu-system-i386 \
        -kernel "$KERNEL_BIN" \
        -m "$MEMORY" \
        -display "$DISPLAY_MODE" \
        $ENABLE_SERIAL \
        $ENABLE_GDB
else
    # Boot from ISO
    if [ ! -f "$ISO_FILE" ]; then
        echo -e "${YELLOW}Warning: ISO not found at $ISO_FILE${NC}"
        echo "Creating ISO..."
        ./scripts/create_iso.sh
        
        if [ ! -f "$ISO_FILE" ]; then
            echo -e "${RED}Error: Failed to create ISO${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}Booting from ISO...${NC}"
    echo "ISO: $ISO_FILE"
    echo "Memory: $MEMORY"
    
    qemu-system-i386 \
        -cdrom "$ISO_FILE" \
        -m "$MEMORY" \
        -display "$DISPLAY_MODE" \
        $ENABLE_SERIAL \
        $ENABLE_GDB
fi

echo ""
echo -e "${GREEN}QEMU session ended${NC}"
