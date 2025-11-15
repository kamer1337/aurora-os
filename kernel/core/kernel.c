/**
 * Aurora OS - Kernel Core Implementation
 * 
 * Main kernel entry point and initialization
 */

#include "kernel.h"
#include "../memory/memory.h"
#include "../process/process.h"
#include "../interrupt/interrupt.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../drivers/serial.h"
#include "../../filesystem/vfs/vfs.h"
#include "../../filesystem/ramdisk/ramdisk.h"
#include "../../filesystem/journal/journal.h"

/**
 * Initialize device drivers
 */
void drivers_init(void) {
    /* Initialize VGA display driver */
    vga_init();
    vga_write("Aurora OS - Initializing...\n");
    
    /* Initialize serial port for debugging */
    serial_init(SERIAL_COM1);
    serial_write(SERIAL_COM1, "Aurora OS - Serial port initialized\n");
    
    /* Initialize keyboard driver */
    keyboard_init();
    vga_write("Keyboard driver initialized\n");
    
    /* Initialize timer (100 Hz) */
    timer_init(100);
    vga_write("Timer driver initialized\n");
}

/**
 * Initialize the kernel subsystems
 */
void kernel_init(void) {
    /* Initialize device drivers first */
    drivers_init();
    
    /* Initialize interrupt handling */
    interrupt_init();
    vga_write("Interrupt handling initialized\n");
    
    /* Initialize memory management */
    memory_init();
    vga_write("Memory management initialized\n");
    
    /* Initialize VFS */
    vfs_init();
    vga_write("VFS initialized\n");
    
    /* Initialize journaling subsystem */
    journal_init();
    vga_write("Journaling subsystem initialized\n");
    
    /* Initialize ramdisk */
    ramdisk_init();
    ramdisk_create(1024 * 1024); /* 1MB ramdisk */
    vga_write("Ramdisk created\n");
    
    /* Register and mount ramdisk */
    vfs_register_fs("ramdisk", ramdisk_get_ops());
    vfs_mount("ramdisk0", "/", "ramdisk");
    vga_write("Ramdisk mounted\n");
    
    /* Initialize process management */
    process_init();
    vga_write("Process management initialized\n");
    
    /* Initialize scheduler */
    scheduler_init();
    vga_write("Scheduler initialized\n");
    
    vga_write("\nAurora OS initialization complete!\n");
}

/**
 * Main kernel loop
 */
void kernel_main(void) {
    kernel_init();
    
    /* TODO: Start scheduler */
    /* TODO: Enter main kernel loop */
    
    /* Kernel should never return */
    while (1) {
        /* Halt and wait for interrupts */
    }
}
