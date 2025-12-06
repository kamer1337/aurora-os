# Aurora OS - Build System

CC = gcc
AS = nasm
LD = ld

# Cross-compilation support for ARM
ARM_CC = arm-none-eabi-gcc
ARM_AS = arm-none-eabi-as
ARM_LD = arm-none-eabi-ld
ARM64_CC = aarch64-linux-gnu-gcc
ARM64_AS = aarch64-linux-gnu-as
ARM64_LD = aarch64-linux-gnu-ld

# Directories
KERNEL_DIR = kernel
BUILD_DIR = build
BOOT_DIR = boot
FS_DIR = filesystem
TEST_DIR = tests

# Optional plugin compilation flags
ENABLE_ML_PLUGIN = 1
ENABLE_QUANTUM_PLUGIN = 1
ENABLE_SYSTEM_OPT_PLUGIN = 1

# Architecture selection (32, 64, arm, arm64)
ARCH = 64
TARGET_PLATFORM = x86_64

# Architecture-specific compiler selection
ifeq ($(ARCH),arm)
    CC = $(ARM_CC)
    AS = $(ARM_AS)
    LD = $(ARM_LD)
    TARGET_PLATFORM = arm
endif

ifeq ($(ARCH),arm64)
    CC = $(ARM64_CC)
    AS = $(ARM64_AS)
    LD = $(ARM64_LD)
    TARGET_PLATFORM = aarch64
endif

# Compiler flags - GCC 15.2 compatible with optimizations
ifeq ($(ARCH),64)
CFLAGS = -Wall -Wextra -Wno-attributes -nostdlib -ffreestanding -m64 -fno-pie -mcmodel=large -O2 -fno-strict-aliasing -fno-builtin-memcpy -fno-builtin-memset
ASFLAGS = -f elf64
LDFLAGS = -m elf_x86_64 -nostdlib
LINKER_SCRIPT = linker64.ld
else
CFLAGS = -Wall -Wextra -Wno-attributes -nostdlib -ffreestanding -m32 -fno-pie -O2 -fno-strict-aliasing -fno-builtin-memcpy -fno-builtin-memset
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -nostdlib
LINKER_SCRIPT = linker.ld
endif

# Add plugin flags
ifeq ($(ENABLE_ML_PLUGIN),1)
    CFLAGS += -DENABLE_ML_PLUGIN
endif
ifeq ($(ENABLE_QUANTUM_PLUGIN),1)
    CFLAGS += -DENABLE_QUANTUM_PLUGIN
endif
ifeq ($(ENABLE_SYSTEM_OPT_PLUGIN),1)
    CFLAGS += -DENABLE_SYSTEM_OPT_PLUGIN
endif

# Source files
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/core/*.c) \
                 $(wildcard $(KERNEL_DIR)/core/winapi/*.c) \
                 $(wildcard $(KERNEL_DIR)/memory/*.c) \
                 $(wildcard $(KERNEL_DIR)/process/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/*.c) \
                 $(wildcard $(KERNEL_DIR)/security/*.c) \
                 $(wildcard $(KERNEL_DIR)/interrupt/*.c) \
                 $(wildcard $(KERNEL_DIR)/gui/*.c) \
                 $(wildcard $(KERNEL_DIR)/smp/*.c) \
                 $(wildcard $(KERNEL_DIR)/network/*.c) \
                 $(wildcard $(KERNEL_DIR)/usb/*.c) \
                 $(wildcard $(KERNEL_DIR)/android/*.c)

PLATFORM_SOURCES = $(filter-out src/platform/aurora_vm_stub.c, $(wildcard src/platform/*.c))

ASM_SOURCES = $(KERNEL_DIR)/core/boot.s

VFS_SOURCES = $(wildcard $(FS_DIR)/vfs/*.c) \
              $(wildcard $(FS_DIR)/ramdisk/*.c) \
              $(wildcard $(FS_DIR)/journal/*.c) \
              $(wildcard $(FS_DIR)/cache/*.c)

TEST_SOURCES = $(filter-out $(TEST_DIR)/aurora_os_vm_integration_test.c $(TEST_DIR)/test_fp_simd.c $(TEST_DIR)/roadmap_priority_tests.c $(TEST_DIR)/test_math_lib.c, $(wildcard $(TEST_DIR)/*.c))

# Object files
KERNEL_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SOURCES))
PLATFORM_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(PLATFORM_SOURCES))
ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
VFS_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(VFS_SOURCES))
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

ALL_OBJECTS = $(ASM_OBJECTS) $(KERNEL_OBJECTS) $(PLATFORM_OBJECTS) $(VFS_OBJECTS) $(TEST_OBJECTS)

# Output
KERNEL_BIN = $(BUILD_DIR)/aurora-kernel.bin

.PHONY: all clean directories iso run test

all: directories $(KERNEL_BIN)

directories:
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/winapi
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/process
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/interrupt
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/security
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/gui
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/smp
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/network
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/usb
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/android
	@mkdir -p $(BUILD_DIR)/src/platform
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/vfs
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/ramdisk
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/journal
	@mkdir -p $(BUILD_DIR)/$(FS_DIR)/cache
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@echo "Assembling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ALL_OBJECTS)
	@echo "Linking kernel"
	@$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $@ $^

clean:
	@echo "Cleaning build artifacts"
	@rm -rf $(BUILD_DIR)/*

iso: all
	@echo "Creating bootable ISO"
	@./scripts/create_iso.sh

run: all
	@echo "Running in Aurora VM"
	@./scripts/run_aurora_vm.sh

test: all
	@echo "Running Aurora VM test suite"
	@./scripts/run_aurora_vm.sh

help:
	@echo "Aurora OS Build System"
	@echo "====================="
	@echo "make all    - Build the kernel"
	@echo "make clean  - Clean build artifacts"
	@echo "make iso    - Create bootable ISO image"
	@echo "make run    - Build and run in Aurora VM"
	@echo "make test   - Build and run Aurora VM test suite"
	@echo "make help   - Show this help message"

# Plugin-specific targets
plugins: all
	@echo "Building optional plugins..."
	@echo "ML Plugin: $(ENABLE_ML_PLUGIN)"
	@echo "Quantum Plugin: $(ENABLE_QUANTUM_PLUGIN)"
	@echo "System Opt Plugin: $(ENABLE_SYSTEM_OPT_PLUGIN)"

plugins-disable-ml:
	$(MAKE) ENABLE_ML_PLUGIN=0

plugins-disable-quantum:
	$(MAKE) ENABLE_QUANTUM_PLUGIN=0

plugins-disable-sysopt:
	$(MAKE) ENABLE_SYSTEM_OPT_PLUGIN=0

plugins-disable-all:
	$(MAKE) ENABLE_ML_PLUGIN=0 ENABLE_QUANTUM_PLUGIN=0 ENABLE_SYSTEM_OPT_PLUGIN=0
