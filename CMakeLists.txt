# Aurora OS - CMake Build System
cmake_minimum_required(VERSION 3.10)
project(AuroraOS C ASM)

# Build options for optional plugins
option(ENABLE_ML_PLUGIN "Enable Machine Learning optimization plugin" ON)
option(ENABLE_QUANTUM_PLUGIN "Enable Quantum computing plugin" ON)
option(ENABLE_SYSTEM_OPT_PLUGIN "Enable System optimization plugin" ON)

# Compiler settings
set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_COMPILER nasm)
set(CMAKE_LINKER ld)

# Compiler flags - GCC 15.2 compatible with optimizations
set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-attributes -nostdlib -ffreestanding -m32 -fno-pie -O3 -march=native -fno-strict-aliasing")
set(CMAKE_ASM_FLAGS "-f elf32")
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -nostdlib")

# Add plugin definitions
if(ENABLE_ML_PLUGIN)
    add_definitions(-DENABLE_ML_PLUGIN)
    message(STATUS "ML Optimization Plugin: ENABLED")
else()
    message(STATUS "ML Optimization Plugin: DISABLED")
endif()

if(ENABLE_QUANTUM_PLUGIN)
    add_definitions(-DENABLE_QUANTUM_PLUGIN)
    message(STATUS "Quantum Computing Plugin: ENABLED")
else()
    message(STATUS "Quantum Computing Plugin: DISABLED")
endif()

if(ENABLE_SYSTEM_OPT_PLUGIN)
    add_definitions(-DENABLE_SYSTEM_OPT_PLUGIN)
    message(STATUS "System Optimization Plugin: ENABLED")
else()
    message(STATUS "System Optimization Plugin: DISABLED")
endif()

# Source files
file(GLOB_RECURSE KERNEL_SOURCES
    "kernel/core/*.c"
    "kernel/memory/*.c"
    "kernel/process/*.c"
    "kernel/drivers/*.c"
    "kernel/security/*.c"
    "kernel/interrupt/*.c"
    "kernel/gui/*.c"
    "kernel/smp/*.c"
    "kernel/network/*.c"
    "kernel/usb/*.c"
)

# VFS sources
file(GLOB VFS_SOURCES
    "filesystem/vfs/*.c"
    "filesystem/ramdisk/*.c"
    "filesystem/journal/*.c"
)

set(ASM_SOURCES "kernel/core/boot.s")

# Create executable
add_executable(aurora-kernel
    ${ASM_SOURCES}
    ${KERNEL_SOURCES}
    ${VFS_SOURCES}
)

# Set linker script
set_target_properties(aurora-kernel PROPERTIES
    LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/linker.ld"
)

# Custom targets
add_custom_target(iso
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/create_iso.sh
    DEPENDS aurora-kernel
    COMMENT "Creating bootable ISO"
)

add_custom_target(run
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_qemu.sh
    DEPENDS iso
    COMMENT "Running in QEMU"
)

# Test target removed - use Makefile for testing

# Plugin-specific targets
add_custom_target(plugins-info
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin Status:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ML Plugin: ${ENABLE_ML_PLUGIN}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Quantum Plugin: ${ENABLE_QUANTUM_PLUGIN}"
    COMMAND ${CMAKE_COMMAND} -E echo "  System Opt Plugin: ${ENABLE_SYSTEM_OPT_PLUGIN}"
)

# Installation (not applicable for kernel, but good practice)
install(TARGETS aurora-kernel
    RUNTIME DESTINATION bin
)
