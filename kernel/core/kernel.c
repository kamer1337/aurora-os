/**
 * Aurora OS - Kernel Core Implementation
 * 
 * Main kernel entry point and initialization
 */

#include "kernel.h"
#include "plugin.h"
#include "../memory/memory.h"
#include "../memory/paging.h"
#include "../process/process.h"
#include "../interrupt/interrupt.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../drivers/serial.h"
#include "../security/quantum_crypto.h"
#include "../gui/gui.h"
#include "../gui/gui_demo.h"
#include "../smp/smp.h"
#include "../network/network.h"
#include "../usb/usb.h"
#include "../../filesystem/vfs/vfs.h"
#include "../../filesystem/ramdisk/ramdisk.h"
#include "../../filesystem/journal/journal.h"
#include "../../tests/test_suite.h"
#include "../../tests/advanced_tests.h"
#include "../../tests/plugin_tests.h"

/* External plugin registration functions */
extern void register_boot_diagnostic_plugin(void);
extern void register_hw_setup_plugin(void);

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
    
    /* Initialize paging subsystem */
    paging_init();
    vga_write("Paging subsystem initialized\n");
    
    /* Initialize SMP (multi-core) support */
    smp_init();
    vga_write("SMP support initialized\n");
    
    /* Initialize quantum encryption subsystem */
    if (quantum_crypto_init() == QCRYPTO_SUCCESS) {
        vga_write("Quantum encryption initialized\n");
        serial_write(SERIAL_COM1, "Quantum cryptography subsystem online\n");
    } else {
        vga_write("WARNING: Quantum encryption initialization failed\n");
    }
    
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
    
    /* Initialize network stack */
    network_init();
    vga_write("Network stack initialized\n");
    
    /* Initialize USB subsystem */
    usb_init();
    vga_write("USB subsystem initialized\n");
    
    /* Initialize plugin system */
    plugin_system_init();
    
    /* Register built-in example plugins */
    vga_write("\nRegistering boot plugins...\n");
    register_hw_setup_plugin();
    register_boot_diagnostic_plugin();
    
    /* List registered plugins */
    plugin_list_all();
    
    /* Initialize all plugins */
    plugin_init_all();
    
    /* SECURITY: Audit plugin interference with quantum crypto */
    plugin_list_interference_flags();
    
    vga_write("\nAurora OS initialization complete!\n");
    
#ifdef QUANTUM_CRYPTO_TESTS
    /* Run quantum encryption tests if enabled */
    run_quantum_crypto_tests();
#endif
}

/**
 * Main kernel loop
 */
void kernel_main(void) {
    kernel_init();
    
    /* Run test suite to validate implementations */
    run_tests();
    
    /* Run advanced feature tests */
    run_advanced_tests();
    
    /* Run plugin system tests */
    run_plugin_tests();
    
    vga_write("\n=== Starting GUI Demo ===\n");
    
    /* Initialize and display GUI demo */
    gui_demo_init();
    
    vga_write("GUI Demo initialized - Switching to framebuffer mode\n");
    vga_write("(Note: In real hardware, VGA text would be replaced by framebuffer)\n");
    
    /* TODO: Start scheduler */
    
    /* Main GUI event loop */
    vga_write("Entering main event loop...\n");
    while (1) {
        /* Handle input events (mouse and keyboard) */
        gui_handle_input();
        
        /* Update and redraw GUI */
        gui_update();
        
        /* Small delay to prevent excessive CPU usage */
        /* In a real system, this would wait for interrupts */
        for (volatile int i = 0; i < 100000; i++);
    }
    
    /* Kernel should never return */
}
