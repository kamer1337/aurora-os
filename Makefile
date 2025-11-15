# Aurora OS - Build System

CC = gcc
AS = nasm
LD = ld

# Directories
KERNEL_DIR = kernel
BUILD_DIR = build
BOOT_DIR = boot
FS_DIR = filesystem
TEST_DIR = tests

# Compiler flags
CFLAGS = -Wall -Wextra -nostdlib -ffreestanding -m32 -fno-pie
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -nostdlib

# Source files
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/core/*.c) \
                 $(wildcard $(KERNEL_DIR)/memory/*.c) \
                 $(wildcard $(KERNEL_DIR)/process/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/*.c) \
                 $(wildcard $(KERNEL_DIR)/security/*.c) \
                 $(wildcard $(KERNEL_DIR)/interrupt/*.c)

ASM_SOURCES = $(KERNEL_DIR)/core/boot.s

VFS_SOURCES = $(wildcard $(FS_DIR)/vfs/*.c) \
              $(wildcard $(FS_DIR)/ramdisk/*.c) \
              $(wildcard $(FS_DIR)/journal/*.c)

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)

# Object files
KERNEL_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SOURCES))
ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
VFS_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(VFS_SOURCES))
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

ALL_OBJECTS = $(ASM_OBJECTS) $(KERNEL_OBJECTS) $(VFS_OBJECTS) $(TEST_OBJECTS)

# Output
KERNEL_BIN = $(BUILD_DIR)/aurora-kernel.bin

.PHONY: all clean directories

all: directories $(KERNEL_BIN)

directories:
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/process
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/interrupt
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/security
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/vfs
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/ramdisk
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/journal
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@echo "Assembling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ALL_OBJECTS)
	@echo "Linking kernel"
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ $^

clean:
	@echo "Cleaning build artifacts"
	@rm -rf $(BUILD_DIR)/*

help:
	@echo "Aurora OS Build System"
	@echo "====================="
	@echo "make all    - Build the kernel"
	@echo "make clean  - Clean build artifacts"
	@echo "make help   - Show this help message"
