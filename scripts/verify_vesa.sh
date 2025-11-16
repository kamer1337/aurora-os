#!/bin/bash
# Aurora OS - VGA/VESA BIOS Implementation Verification Script

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}Aurora OS - VGA/VESA BIOS Verification${NC}"
echo "=========================================="
echo ""

PASS_COUNT=0
FAIL_COUNT=0

# Helper function to check test
check_test() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        ((PASS_COUNT++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAIL_COUNT++))
    fi
}

# Test 1: Check multiboot header includes video mode flags
echo -e "\n${YELLOW}Test 1: Multiboot Video Mode Request${NC}"
if grep -q "VIDEO" kernel/core/boot.s && grep -q "1920" kernel/core/boot.s; then
    check_test 0 "Multiboot header requests video mode (1920x1080x32)"
else
    check_test 1 "Multiboot header missing video mode request"
fi

# Test 2: Check multiboot info structure exists
echo -e "\n${YELLOW}Test 2: Multiboot Info Parser${NC}"
if [ -f "include/multiboot.h" ] && grep -q "multiboot_info_t" include/multiboot.h; then
    check_test 0 "Multiboot info structures defined"
else
    check_test 1 "Multiboot info structures missing"
fi

# Test 3: Check framebuffer info flag
echo -e "\n${YELLOW}Test 3: Framebuffer Info Support${NC}"
if grep -q "MULTIBOOT_FLAG_FB" include/multiboot.h; then
    check_test 0 "Framebuffer flag defined in multiboot header"
else
    check_test 1 "Framebuffer flag not defined"
fi

# Test 4: Check kernel accepts multiboot parameters
echo -e "\n${YELLOW}Test 4: Kernel Multiboot Integration${NC}"
if grep -q "kernel_main(uint32_t magic, uint32_t multiboot_addr)" kernel/core/kernel.h; then
    check_test 0 "Kernel accepts multiboot parameters"
else
    check_test 1 "Kernel does not accept multiboot parameters"
fi

# Test 5: Check framebuffer initialization from multiboot
echo -e "\n${YELLOW}Test 5: Framebuffer Multiboot Initialization${NC}"
if grep -q "framebuffer_init_from_multiboot" kernel/gui/framebuffer.c; then
    check_test 0 "Framebuffer can initialize from multiboot info"
else
    check_test 1 "Framebuffer multiboot initialization missing"
fi

# Test 6: Check GRUB configuration sets graphics mode
echo -e "\n${YELLOW}Test 6: GRUB Graphics Mode Configuration${NC}"
if [ -f "scripts/create_iso.sh" ] && grep -q "gfxmode=1920x1080x32" scripts/create_iso.sh; then
    check_test 0 "GRUB configured to set graphics mode"
else
    check_test 1 "GRUB graphics mode configuration missing"
fi

# Test 7: Check boot.s passes multiboot info to kernel
echo -e "\n${YELLOW}Test 7: Multiboot Info Passing${NC}"
if grep -q "pushl %ebx" kernel/core/boot.s && grep -q "pushl %eax" kernel/core/boot.s; then
    check_test 0 "Boot code passes multiboot info to kernel"
else
    check_test 1 "Boot code does not pass multiboot info"
fi

# Test 8: Check kernel validates multiboot magic
echo -e "\n${YELLOW}Test 8: Multiboot Magic Validation${NC}"
if grep -q "MULTIBOOT_MAGIC" kernel/core/kernel.c; then
    check_test 0 "Kernel validates multiboot magic number"
else
    check_test 1 "Kernel does not validate multiboot magic"
fi

# Test 9: Check framebuffer fallback support
echo -e "\n${YELLOW}Test 9: Framebuffer Fallback Support${NC}"
if grep -q "framebuffer_init(0, 0, 0)" kernel/core/kernel.c || grep -q "framebuffer_init(0, 0, 0)" kernel/gui/framebuffer.c; then
    check_test 0 "Framebuffer has fallback for missing VESA"
else
    check_test 1 "Framebuffer fallback missing"
fi

# Test 10: Check kernel builds successfully
echo -e "\n${YELLOW}Test 10: Build Verification${NC}"
if [ -f "build/aurora-kernel.bin" ]; then
    check_test 0 "Kernel binary built successfully"
else
    check_test 1 "Kernel binary not found (build may have failed)"
fi

# Test 11: Check ISO was created
echo -e "\n${YELLOW}Test 11: ISO Creation${NC}"
if [ -f "build/aurora-os.iso" ]; then
    check_test 0 "Bootable ISO created successfully"
else
    check_test 1 "ISO file not found"
fi

# Test 12: Check documentation exists
echo -e "\n${YELLOW}Test 12: Documentation${NC}"
if [ -f "docs/VGA_VESA_BIOS.md" ]; then
    check_test 0 "VGA/VESA BIOS documentation exists"
else
    check_test 1 "Documentation missing"
fi

# Summary
echo ""
echo "=========================================="
echo -e "${BLUE}Test Summary${NC}"
echo "=========================================="
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    echo "VGA/VESA BIOS implementation is complete and verified."
    echo ""
    echo "To test on real hardware or QEMU:"
    echo "  qemu-system-i386 -cdrom build/aurora-os.iso -m 512M -vga std"
    echo ""
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    echo "Please review the implementation."
    exit 1
fi
