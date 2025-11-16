#!/bin/bash
# Aurora OS - Aurora VM Testing Script

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default configuration
VM_TEST_BIN="bin/aurora_vm_test"
MEMORY="128M"
ENABLE_DEBUG=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--memory)
            MEMORY="$2"
            shift 2
            ;;
        -d|--debug)
            ENABLE_DEBUG="yes"
            echo -e "${YELLOW}Debug mode enabled${NC}"
            shift
            ;;
        -h|--help)
            echo "Aurora OS - Aurora VM Testing Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -m, --memory SIZE    Set RAM size (default: 128M)"
            echo "  -d, --debug          Enable debug mode"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                   # Run normally"
            echo "  $0 -m 256M           # Run with 256MB RAM"
            echo "  $0 -d                # Run with debug mode"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Aurora OS - Aurora VM Testing${NC}"
echo "=============================="

# Build Aurora VM test suite if needed
if [ ! -f "$VM_TEST_BIN" ]; then
    echo -e "${YELLOW}Aurora VM test binary not found. Building...${NC}"
    make -f Makefile.vm all
    
    if [ ! -f "$VM_TEST_BIN" ]; then
        echo -e "${RED}Error: Failed to build Aurora VM test binary${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}Running Aurora VM test suite...${NC}"
echo "Memory: $MEMORY"
echo ""

# Run the Aurora VM test suite
if [ "$ENABLE_DEBUG" = "yes" ]; then
    # Run with debug output
    ./"$VM_TEST_BIN" --debug
else
    # Run normally
    ./"$VM_TEST_BIN"
fi

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}Aurora VM session ended successfully${NC}"
else
    echo -e "${RED}Aurora VM session ended with errors (exit code: $EXIT_CODE)${NC}"
fi

exit $EXIT_CODE
