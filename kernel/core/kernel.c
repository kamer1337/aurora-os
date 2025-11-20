/**
 * Aurora OS - Kernel Core Implementation
 * 
 * Main kernel entry point and initialization
 */

#include "kernel.h"
#include "plugin.h"
#include "nfr.h"
#include "pe_loader.h"
#include "dll_loader.h"
#include "winapi/winapi.h"
#include "winapi/kernel32.h"
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
#include "../gui/framebuffer.h"
#include "../smp/smp.h"
#include "../network/network.h"
#include "../usb/usb.h"
#include "../../filesystem/vfs/vfs.h"
#include "../../filesystem/ramdisk/ramdisk.h"
#include "../../filesystem/journal/journal.h"
#include "../../include/multiboot.h"
#include "../../tests/test_suite.h"
#include "../../tests/advanced_tests.h"
#include "../../tests/plugin_tests.h"
#include "../../tests/phase5_tests.h"
#include "../../tests/pe_loader_tests.h"
#include "../../tests/font_tests.h"
#include "../../tests/nfr_tests.h"
#include "../../tests/aurora_linux_kernel_tests.h"
#include "aurora_linux_kernel.h"

/* External plugin registration functions */
extern void register_boot_diagnostic_plugin(void);
extern void register_hw_setup_plugin(void);
extern void register_ml_optimization_plugin(void);
extern void register_gfx_ml_optimization_plugin(void);
extern void register_quantum_compute_plugin(void);
extern void register_system_optimization_plugin(void);

/* Global multiboot info pointer */
static multiboot_info_t* g_multiboot_info = 0;

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
 * Initialize framebuffer from multiboot info
 */
static void init_framebuffer_from_multiboot(void) {
    if (!g_multiboot_info) {
        vga_write("WARNING: No multiboot info available\n");
        /* Fall back to default framebuffer settings */
        framebuffer_init(0, 0, 0);
        return;
    }
    
    /* Check if framebuffer info is available */
    if (g_multiboot_info->flags & MULTIBOOT_FLAG_FB) {
        vga_write("Multiboot framebuffer info detected\n");
        vga_write("  Address: ");
        vga_write_hex((uint32_t)g_multiboot_info->framebuffer_addr);
        vga_write("\n  Resolution: ");
        vga_write_dec(g_multiboot_info->framebuffer_width);
        vga_write("x");
        vga_write_dec(g_multiboot_info->framebuffer_height);
        vga_write("x");
        vga_write_dec(g_multiboot_info->framebuffer_bpp);
        vga_write("\n");
        
        /* Initialize framebuffer with multiboot info */
        framebuffer_init_from_multiboot(g_multiboot_info);
    } else if (g_multiboot_info->flags & MULTIBOOT_FLAG_VBE) {
        vga_write("VBE info available but framebuffer info not present\n");
        vga_write("Using default framebuffer settings\n");
        framebuffer_init(0, 0, 0);
    } else {
        vga_write("No video mode info from bootloader\n");
        vga_write("Using default framebuffer settings\n");
        framebuffer_init(0, 0, 0);
    }
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
    
    /* Initialize Windows binary support */
    pe_loader_init();
    dll_loader_init();
    winapi_init();
    kernel32_init();
    vga_write("Windows binary support initialized\n");
    
    /* Initialize Non-Functional Requirements monitoring */
    nfr_init();
    vga_write("NFR monitoring initialized\n");
    
    /* Initialize plugin system */
    plugin_system_init();
    
    /* Register built-in example plugins */
    vga_write("\nRegistering boot plugins...\n");
    register_hw_setup_plugin();
    register_boot_diagnostic_plugin();
    
    /* Register optional optimization plugins */
    vga_write("Registering optional optimization plugins...\n");
    register_ml_optimization_plugin();
    register_gfx_ml_optimization_plugin();
    register_quantum_compute_plugin();
    register_system_optimization_plugin();
    
    /* List registered plugins */
    plugin_list_all();
    
    /* Initialize all plugins */
    plugin_init_all();
    
    /* SECURITY: Audit plugin interference with quantum crypto */
    plugin_list_interference_flags();
    
    /* Initialize Aurora Linux Kernel with enhancements */
    vga_write("\n=== Initializing Aurora Linux Kernel ===\n");
    aurora_linux_kernel_config_t linux_config = {
        .kernel_features = KERNEL_FEATURE_SMP | 
                          KERNEL_FEATURE_PREEMPT |
                          KERNEL_FEATURE_MODULES |
                          KERNEL_FEATURE_NETWORKING |
                          KERNEL_FEATURE_FILESYSTEM |
                          KERNEL_FEATURE_CRYPTO,
        .optimization_level = AURORA_OPT_AGGRESSIVE,
        .kyber_mode = KYBER_MODE_768,
        .enable_quantum_rng = 1,
        .enable_secure_boot = 1,
        .enable_memory_encryption = 1,
        .enable_network_encryption = 0,
        .max_modules = 32,
        .memory_limit_mb = 2048
    };
    
    if (aurora_linux_kernel_init(&linux_config) == LINUX_COMPAT_SUCCESS) {
        vga_write("Aurora Linux Kernel initialized successfully!\n");
        aurora_kernel_print_info();
    } else {
        vga_write("WARNING: Aurora Linux Kernel initialization failed\n");
    }
    
    vga_write("\nAurora OS initialization complete!\n");
    
#ifdef QUANTUM_CRYPTO_TESTS
    /* Run quantum encryption tests if enabled */
    run_quantum_crypto_tests();
#endif
}

/**
 * Main kernel loop
 */
void kernel_main(uint32_t magic, uint32_t multiboot_addr) {
    /* Verify multiboot magic number */
    if (magic != MULTIBOOT_MAGIC) {
        vga_init();
        vga_write("ERROR: Invalid multiboot magic number!\n");
        vga_write("Expected: ");
        vga_write_hex(MULTIBOOT_MAGIC);
        vga_write("\nReceived: ");
        vga_write_hex(magic);
        vga_write("\n");
        /* Continue anyway for compatibility */
    }
    
    /* Save multiboot info pointer */
    g_multiboot_info = (multiboot_info_t*)multiboot_addr;
    
    kernel_init();
    
    /* Initialize framebuffer with multiboot info */
    vga_write("\n=== Initializing Graphics ===\n");
    init_framebuffer_from_multiboot();
    
    /* Run test suite to validate implementations */
    run_tests();
    
    /* Run advanced feature tests */
    run_advanced_tests();
    
    /* Run plugin system tests */
    run_plugin_tests();
    
    /* Run Phase 5 comprehensive tests */
    vga_write("\n=== Phase 5: Testing & Debugging ===\n");
    phase5_run_all_tests();
    
    /* Run Windows library support tests */
    vga_write("\n=== Testing Windows Library Support ===\n");
    run_pe_loader_tests();
    run_dll_loader_tests();
    run_winapi_tests();
    
    /* Run font tests */
    run_font_tests();
    
    /* Run NFR module tests */
    vga_write("\n=== Testing Non-Functional Requirements Module ===\n");
    run_nfr_tests();
    int nfr_passed = 0, nfr_failed = 0;
    get_nfr_test_results(&nfr_passed, &nfr_failed);
    vga_write("NFR Tests: ");
    vga_write_dec(nfr_passed);
    vga_write(" passed, ");
    vga_write_dec(nfr_failed);
    vga_write(" failed\n");
    
    /* Run Aurora Linux Kernel tests */
    vga_write("\n=== Testing Aurora Linux Kernel ===\n");
    run_aurora_linux_kernel_tests();
    int linux_passed = 0, linux_failed = 0;
    get_aurora_linux_kernel_test_results(&linux_passed, &linux_failed);
    vga_write("Aurora Linux Kernel Tests: ");
    vga_write_dec(linux_passed);
    vga_write(" passed, ");
    vga_write_dec(linux_failed);
    vga_write(" failed\n");
    
    /* Display NFR metrics report */
    vga_write("\n");
    nfr_update_all();
    nfr_print_report();
    
    vga_write("\n=== Starting GUI Demo ===\n");
    
    /* Initialize and display GUI demo */
    gui_demo_init();
    
    vga_write("GUI Demo initialized - Switching to framebuffer mode\n");
    vga_write("(Note: In real hardware, VGA text would be replaced by framebuffer)\n");
    
    /* Scheduler is already initialized and running via timer interrupts */
    /* The scheduler_schedule() function is called automatically by the timer IRQ */
    
    /* Main GUI event loop */
    vga_write("Entering main event loop...\n");
    uint32_t loop_counter = 0;
    while (1) {
        /* Handle input events (mouse and keyboard) */
        gui_handle_input();
        
        /* Update and redraw GUI */
        gui_update();
        
        /* Periodically update NFR metrics */
        if (++loop_counter % 10000 == 0) {
            nfr_update_all();
            nfr_update_performance(100); /* Simulate operation latency */
        }
        
        /* Small delay to prevent excessive CPU usage */
        /* In a real system, this would wait for interrupts */
        for (volatile int i = 0; i < 100000; i++);
    }
    
    /* Kernel should never return */
}
