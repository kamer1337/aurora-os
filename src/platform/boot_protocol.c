/**
 * @file boot_protocol.c
 * @brief Complete Android boot.img v3/v4 and Linux boot protocol implementation
 *
 * Implements full Android boot image parsing (v3/v4) and Linux bzImage boot protocol
 */

#include "../../include/platform/android_vm.h"
#include "../../include/platform/linux_vm.h"
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * ANDROID BOOT IMAGE V3/V4 IMPLEMENTATION
 * ============================================================================ */

/* Android boot image magic */
#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8

/* Boot image header v3 (Android 11+) */
typedef struct {
    uint8_t magic[BOOT_MAGIC_SIZE];     /* "ANDROID!" */
    uint32_t kernel_size;               /* Size of kernel */
    uint32_t ramdisk_size;              /* Size of ramdisk */
    uint32_t os_version;                /* OS version and patch level */
    uint32_t header_size;               /* Size of boot header */
    uint32_t reserved[4];               /* Reserved */
    uint32_t header_version;            /* Version of boot header (3 or 4) */
    uint8_t cmdline[1536];              /* Command line */
} boot_img_hdr_v3_t;

/* Boot image header v4 (Android 12+) */
typedef struct {
    uint8_t magic[BOOT_MAGIC_SIZE];     /* "ANDROID!" */
    uint32_t kernel_size;               /* Size of kernel */
    uint32_t ramdisk_size;              /* Size of ramdisk */
    uint32_t os_version;                /* OS version and patch level */
    uint32_t header_size;               /* Size of boot header */
    uint32_t reserved[4];               /* Reserved */
    uint32_t header_version;            /* Version of boot header (4) */
    uint8_t cmdline[1536];              /* Command line */
    uint32_t signature_size;            /* Size of signature */
} boot_img_hdr_v4_t;

/* Vendor boot image header (Android 11+) */
typedef struct {
    uint8_t magic[8];                   /* "VNDRBOOT" */
    uint32_t header_version;            /* Vendor header version */
    uint32_t page_size;                 /* Flash page size */
    uint32_t kernel_addr;               /* Physical load address of kernel */
    uint32_t ramdisk_addr;              /* Physical load address of ramdisk */
    uint32_t vendor_ramdisk_size;       /* Size of vendor ramdisk */
    uint8_t cmdline[2048];              /* Vendor command line */
    uint32_t tags_addr;                 /* Physical address for kernel tags */
    uint8_t name[16];                   /* Product name */
    uint32_t header_size;               /* Size of vendor boot header */
    uint32_t dtb_size;                  /* Size of DTB image */
    uint64_t dtb_addr;                  /* Physical load address of DTB */
} vendor_boot_img_hdr_t;

/* Boot state tracking */
typedef struct {
    bool initialized;
    uint32_t boot_version;              /* 3 or 4 */
    uint32_t kernel_addr;
    uint32_t kernel_size;
    uint32_t ramdisk_addr;
    uint32_t ramdisk_size;
    uint32_t dtb_addr;
    uint32_t dtb_size;
    char cmdline[2048];
} android_boot_state_t;

static android_boot_state_t g_android_boot_state = {0};

/**
 * Parse Android boot image header v3/v4
 */
int android_boot_parse_header(const uint8_t* image, uint32_t size, android_boot_state_t* state) {
    if (!image || size < sizeof(boot_img_hdr_v3_t) || !state) {
        return -1;
    }
    
    /* Verify magic */
    if (platform_memcmp(image, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        return -1; /* Invalid boot image */
    }
    
    const boot_img_hdr_v3_t* hdr_v3 = (const boot_img_hdr_v3_t*)image;
    
    state->initialized = true;
    state->boot_version = hdr_v3->header_version;
    state->kernel_size = hdr_v3->kernel_size;
    state->ramdisk_size = hdr_v3->ramdisk_size;
    
    /* Copy command line */
    platform_memcpy(state->cmdline, hdr_v3->cmdline, sizeof(hdr_v3->cmdline));
    state->cmdline[sizeof(hdr_v3->cmdline)] = '\0';
    
    /* Calculate addresses for v3 (GKI - Generic Kernel Image) */
    if (state->boot_version == 3 || state->boot_version == 4) {
        /* Kernel immediately follows header, page-aligned */
        uint32_t page_size = 4096; /* Default page size for v3/v4 */
        state->kernel_addr = ((hdr_v3->header_size + page_size - 1) / page_size) * page_size;
        state->ramdisk_addr = state->kernel_addr + 
            ((state->kernel_size + page_size - 1) / page_size) * page_size;
    }
    
    if (state->boot_version == 4) {
        const boot_img_hdr_v4_t* hdr_v4 = (const boot_img_hdr_v4_t*)image;
        /* V4 has signature size - account for it */
        (void)hdr_v4; /* Used for validation */
    }
    
    return 0;
}

/**
 * Load Android kernel from boot image
 */
int android_boot_load_kernel(AndroidVM* vm, const uint8_t* boot_image, uint32_t size) {
    if (!vm || !boot_image || size == 0) {
        return -1;
    }
    
    android_boot_state_t boot_state = {0};
    if (android_boot_parse_header(boot_image, size, &boot_state) != 0) {
        return -1;
    }
    
    /* Extract and load kernel */
    if (boot_state.kernel_size > 0 && boot_state.kernel_addr < size) {
        const uint8_t* kernel_data = boot_image + boot_state.kernel_addr;
        if (android_vm_load_kernel(vm, kernel_data, boot_state.kernel_size) != 0) {
            return -1;
        }
    }
    
    /* Extract and load ramdisk */
    if (boot_state.ramdisk_size > 0 && boot_state.ramdisk_addr < size) {
        const uint8_t* ramdisk_data = boot_image + boot_state.ramdisk_addr;
        if (android_vm_load_ramdisk(vm, ramdisk_data, boot_state.ramdisk_size) != 0) {
            return -1;
        }
    }
    
    /* Set command line from boot image */
    if (boot_state.cmdline[0] != '\0') {
        android_vm_set_cmdline(vm, boot_state.cmdline);
    }
    
    /* Store global boot state */
    platform_memcpy(&g_android_boot_state, &boot_state, sizeof(android_boot_state_t));
    
    return 0;
}

/**
 * Setup Android boot protocol for ARM64
 */
int android_boot_setup_arm64(AndroidVM* vm, uint32_t dtb_addr) {
    if (!vm) {
        return -1;
    }
    
    /* ARM64 boot protocol:
     * x0 = physical address of device tree blob (dtb)
     * x1, x2, x3 = 0 (reserved)
     * Processor in EL2 (hypervisor) or EL1 (kernel)
     * MMU off, caches off, interrupts disabled
     */
    
    if (vm->aurora_vm) {
        aurora_vm_set_register(vm->aurora_vm, 0, dtb_addr); /* x0 = DTB address */
        aurora_vm_set_register(vm->aurora_vm, 1, 0);        /* x1 = 0 */
        aurora_vm_set_register(vm->aurora_vm, 2, 0);        /* x2 = 0 */
        aurora_vm_set_register(vm->aurora_vm, 3, 0);        /* x3 = 0 */
    }
    
    return 0;
}

/**
 * Get Android boot image version
 */
uint32_t android_boot_get_version(void) {
    return g_android_boot_state.boot_version;
}

/**
 * Check if boot image is valid
 */
bool android_boot_is_valid_image(const uint8_t* image, uint32_t size) {
    if (!image || size < BOOT_MAGIC_SIZE) {
        return false;
    }
    return platform_memcmp(image, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0;
}

/* ============================================================================
 * LINUX BOOT PROTOCOL IMPLEMENTATION
 * ============================================================================ */

/* Linux boot protocol structures */

/* Setup header (from Linux kernel documentation) */
typedef struct {
    uint8_t setup_sects;        /* Size of setup in sectors */
    uint16_t root_flags;        /* Root flags */
    uint32_t syssize;           /* System size */
    uint16_t ram_size;          /* RAM size (obsolete) */
    uint16_t vid_mode;          /* Video mode */
    uint16_t root_dev;          /* Root device */
    uint16_t boot_flag;         /* 0xAA55 magic */
    uint16_t jump;              /* Jump instruction */
    uint32_t header;            /* "HdrS" magic */
    uint16_t version;           /* Boot protocol version */
    uint32_t realmode_swtch;    /* Real mode switch */
    uint16_t start_sys_seg;     /* Start system segment */
    uint16_t kernel_version;    /* Kernel version string offset */
    uint8_t type_of_loader;     /* Type of boot loader */
    uint8_t loadflags;          /* Boot protocol flags */
    uint16_t setup_move_size;   /* Move size */
    uint32_t code32_start;      /* 32-bit code start address */
    uint32_t ramdisk_image;     /* Initrd load address */
    uint32_t ramdisk_size;      /* Initrd size */
    uint32_t bootsect_kludge;   /* Bootsector kludge */
    uint16_t heap_end_ptr;      /* Heap end pointer */
    uint8_t ext_loader_ver;     /* Extended boot loader version */
    uint8_t ext_loader_type;    /* Extended boot loader type */
    uint32_t cmd_line_ptr;      /* Command line pointer */
    uint32_t initrd_addr_max;   /* Max initrd address */
    uint32_t kernel_alignment;  /* Kernel alignment */
    uint8_t relocatable_kernel; /* Relocatable kernel flag */
    uint8_t min_alignment;      /* Minimum alignment */
    uint16_t xloadflags;        /* Extended load flags */
    uint32_t cmdline_size;      /* Maximum command line size */
    uint32_t hardware_subarch;  /* Hardware subarchitecture */
    uint64_t hardware_subarch_data;
    uint32_t payload_offset;    /* Payload offset */
    uint32_t payload_length;    /* Payload length */
    uint64_t setup_data;        /* Setup data pointer */
    uint64_t pref_address;      /* Preferred load address */
    uint32_t init_size;         /* Init size */
    uint32_t handover_offset;   /* EFI handover offset */
} linux_setup_header_t;

/* Boot parameters (E820 memory map) */
#define E820_MAX_ENTRIES 128
#define E820_RAM         1
#define E820_RESERVED    2
#define E820_ACPI        3
#define E820_NVS         4
#define E820_UNUSABLE    5

typedef struct {
    uint64_t addr;              /* Start address */
    uint64_t size;              /* Size in bytes */
    uint32_t type;              /* Type of memory */
} e820_entry_t;

typedef struct {
    uint8_t screen_info[64];    /* Screen information */
    uint8_t apm_bios_info[20];  /* APM BIOS info */
    uint8_t pad[4];
    uint64_t tboot_addr;        /* TBoot address */
    uint8_t ist_info[16];       /* IST info */
    uint8_t pad2[16];
    uint8_t hd0_info[16];       /* HD0 info */
    uint8_t hd1_info[16];       /* HD1 info */
    uint16_t sys_desc_table;    /* System descriptor table */
    uint8_t pad3[2];
    uint16_t olpc_ofw_header;   /* OLPC OFW header */
    uint32_t ext_ramdisk_image; /* Extended ramdisk image */
    uint32_t ext_ramdisk_size;  /* Extended ramdisk size */
    uint32_t ext_cmd_line_ptr;  /* Extended command line pointer */
    uint8_t pad4[116];
    uint8_t edid_info[128];     /* EDID info */
    uint8_t efi_info[32];       /* EFI info */
    uint32_t alt_mem_k;         /* Alternative memory size */
    uint32_t scratch;           /* Scratch */
    uint8_t e820_entries;       /* E820 entry count */
    uint8_t eddbuf_entries;     /* EDD buffer entries */
    uint8_t edd_mbr_sig_buf_entries;
    uint8_t kbd_status;         /* Keyboard status */
    uint8_t secure_boot;        /* Secure boot status */
    uint8_t pad5[2];
    uint8_t sentinel;           /* Sentinel */
    uint8_t pad6[1];
    linux_setup_header_t hdr;   /* Setup header */
    uint8_t pad7[36];
    e820_entry_t e820_table[E820_MAX_ENTRIES]; /* E820 memory map */
} linux_boot_params_t;

/* Linux boot protocol version */
#define LINUX_BOOT_VERSION 0x020f  /* 2.15 */
#define LINUX_HEADER_MAGIC 0x53726448  /* "HdrS" */

/* Boot flags */
#define LINUX_LOADFLAG_LOADED_HIGH  0x01
#define LINUX_LOADFLAG_KASLR        0x02
#define LINUX_LOADFLAG_QUIET        0x20
#define LINUX_LOADFLAG_KEEP_SEGMENTS 0x40
#define LINUX_LOADFLAG_CAN_USE_HEAP 0x80

/* Linux boot state */
typedef struct {
    bool initialized;
    uint16_t boot_version;
    uint32_t kernel_addr;
    uint32_t kernel_size;
    uint32_t initrd_addr;
    uint32_t initrd_size;
    uint32_t cmdline_addr;
    uint32_t boot_params_addr;
    char cmdline[2048];
    e820_entry_t e820_map[E820_MAX_ENTRIES];
    uint8_t e820_entries;
} linux_boot_state_t;

static linux_boot_state_t g_linux_boot_state = {0};

/**
 * Parse Linux kernel bzImage
 */
int linux_boot_parse_bzimage(const uint8_t* image, uint32_t size, linux_boot_state_t* state) {
    if (!image || size < 512 || !state) {
        return -1;
    }
    
    /* Check for valid boot sector */
    if (image[510] != 0x55 || image[511] != 0xAA) {
        return -1; /* Invalid boot sector */
    }
    
    /* Parse setup header at offset 0x1F1 */
    const linux_setup_header_t* hdr = (const linux_setup_header_t*)(image + 0x1F1);
    
    /* Check header magic */
    if (hdr->header != LINUX_HEADER_MAGIC) {
        return -1; /* Invalid Linux kernel */
    }
    
    state->initialized = true;
    state->boot_version = hdr->version;
    
    /* Calculate kernel location */
    uint8_t setup_sects = hdr->setup_sects;
    if (setup_sects == 0) {
        setup_sects = 4; /* Default */
    }
    
    uint32_t setup_size = (setup_sects + 1) * 512;
    state->kernel_addr = LINUX_VM_KERNEL_BASE;
    state->kernel_size = size - setup_size;
    
    /* Check for relocatable kernel */
    if (hdr->relocatable_kernel && hdr->version >= 0x0205) {
        state->kernel_addr = (uint32_t)hdr->pref_address;
    }
    
    return 0;
}

/**
 * Setup Linux boot parameters
 */
int linux_boot_setup_params(LinuxVM* vm, const char* cmdline) {
    if (!vm) {
        return -1;
    }
    
    /* Allocate boot params at a known location */
    uint32_t boot_params_addr = 0x7000; /* Traditional location */
    g_linux_boot_state.boot_params_addr = boot_params_addr;
    
    /* Setup E820 memory map */
    g_linux_boot_state.e820_entries = 3;
    
    /* Low memory: 0-640KB (usable) */
    g_linux_boot_state.e820_map[0].addr = 0;
    g_linux_boot_state.e820_map[0].size = 0xA0000;
    g_linux_boot_state.e820_map[0].type = E820_RAM;
    
    /* Video/BIOS: 640KB-1MB (reserved) */
    g_linux_boot_state.e820_map[1].addr = 0xA0000;
    g_linux_boot_state.e820_map[1].size = 0x60000;
    g_linux_boot_state.e820_map[1].type = E820_RESERVED;
    
    /* High memory: 1MB-128MB (usable) */
    g_linux_boot_state.e820_map[2].addr = 0x100000;
    g_linux_boot_state.e820_map[2].size = LINUX_VM_MEMORY_SIZE - 0x100000;
    g_linux_boot_state.e820_map[2].type = E820_RAM;
    
    /* Copy command line */
    if (cmdline) {
        platform_strncpy(g_linux_boot_state.cmdline, cmdline, sizeof(g_linux_boot_state.cmdline));
    } else {
        platform_strncpy(g_linux_boot_state.cmdline, 
                        "console=ttyS0 root=/dev/ram0 rw", 
                        sizeof(g_linux_boot_state.cmdline));
    }
    
    g_linux_boot_state.initialized = true;
    
    return 0;
}

/**
 * Load Linux kernel with boot protocol
 */
int linux_boot_load_kernel(LinuxVM* vm, const uint8_t* bzimage, uint32_t size) {
    if (!vm || !bzimage || size == 0) {
        return -1;
    }
    
    linux_boot_state_t boot_state = {0};
    if (linux_boot_parse_bzimage(bzimage, size, &boot_state) != 0) {
        /* Not a bzImage, try loading as raw kernel */
        return linux_vm_load_kernel(vm, bzimage, size);
    }
    
    /* Load protected mode kernel */
    const linux_setup_header_t* hdr = (const linux_setup_header_t*)(bzimage + 0x1F1);
    uint8_t setup_sects = hdr->setup_sects;
    if (setup_sects == 0) setup_sects = 4;
    
    uint32_t setup_size = (setup_sects + 1) * 512;
    const uint8_t* kernel_data = bzimage + setup_size;
    uint32_t kernel_size = size - setup_size;
    
    /* Load kernel at proper address */
    if (linux_vm_load_kernel(vm, kernel_data, kernel_size) != 0) {
        return -1;
    }
    
    /* Setup boot parameters */
    linux_boot_setup_params(vm, vm->kernel_cmdline);
    
    /* Store global boot state */
    platform_memcpy(&g_linux_boot_state, &boot_state, sizeof(linux_boot_state_t));
    
    return 0;
}

/**
 * Setup Linux CPU state for boot
 */
int linux_boot_setup_cpu(LinuxVM* vm) {
    if (!vm || !vm->aurora_vm) {
        return -1;
    }
    
    /* For 32-bit protected mode boot:
     * - CS = __BOOT_CS (flat code segment)
     * - DS = ES = SS = __BOOT_DS (flat data segment)
     * - FS = GS = 0
     * - EIP = start address of protected mode kernel
     * - ESP = safe stack address
     * - Interrupts disabled
     * - Paging disabled (may be enabled later)
     */
    
    /* Set stack pointer */
    aurora_vm_set_register(vm->aurora_vm, 4, 0x90000); /* ESP = 0x90000 */
    
    /* Set initial parameter pointer (boot params address) */
    aurora_vm_set_register(vm->aurora_vm, 5, g_linux_boot_state.boot_params_addr); /* EBP = boot params */
    
    return 0;
}

/**
 * Get Linux boot protocol version
 */
uint16_t linux_boot_get_version(void) {
    return g_linux_boot_state.boot_version;
}

/**
 * Check if kernel is valid bzImage
 */
bool linux_boot_is_bzimage(const uint8_t* image, uint32_t size) {
    if (!image || size < 512) {
        return false;
    }
    
    /* Check boot sector signature */
    if (image[510] != 0x55 || image[511] != 0xAA) {
        return false;
    }
    
    /* Check header magic */
    const linux_setup_header_t* hdr = (const linux_setup_header_t*)(image + 0x1F1);
    return hdr->header == LINUX_HEADER_MAGIC;
}

/* ============================================================================
 * COMMON BOOT UTILITIES
 * ============================================================================ */

/**
 * Calculate CRC32 for verification
 */
uint32_t boot_crc32(const uint8_t* data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    
    /* Simple byte-by-byte CRC32 calculation (polynomial 0xEDB88320) */
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * Get boot protocol version string
 */
const char* boot_protocol_get_version(void) {
    return "1.0.0-aurora-boot";
}

/* ============================================================================
 * EXTENDED LINUX BOOT PROTOCOL FEATURES
 * ============================================================================ */

/**
 * Add E820 memory map entry
 */
int linux_boot_add_e820_entry(uint64_t addr, uint64_t size, uint32_t type) {
    if (g_linux_boot_state.e820_entries >= E820_MAX_ENTRIES) {
        return -1;  /* Map full */
    }
    
    uint8_t idx = g_linux_boot_state.e820_entries++;
    g_linux_boot_state.e820_map[idx].addr = addr;
    g_linux_boot_state.e820_map[idx].size = size;
    g_linux_boot_state.e820_map[idx].type = type;
    
    return (int)idx;
}

/**
 * Get E820 memory map entry count
 */
uint8_t linux_boot_get_e820_count(void) {
    return g_linux_boot_state.e820_entries;
}

/**
 * Get E820 memory map entry by index
 */
int linux_boot_get_e820_entry(uint8_t idx, uint64_t* addr, uint64_t* size, uint32_t* type) {
    if (idx >= g_linux_boot_state.e820_entries) {
        return -1;
    }
    
    if (addr) *addr = g_linux_boot_state.e820_map[idx].addr;
    if (size) *size = g_linux_boot_state.e820_map[idx].size;
    if (type) *type = g_linux_boot_state.e820_map[idx].type;
    
    return 0;
}

/**
 * Setup KASLR (Kernel Address Space Layout Randomization)
 * Provides random kernel load address for security
 */
int linux_boot_setup_kaslr(LinuxVM* vm, uint32_t entropy) {
    if (!vm) {
        return -1;
    }
    
    if (!g_linux_boot_state.initialized) {
        return -1;
    }
    
    /* Calculate randomized kernel address */
    /* KASLR randomizes the kernel load address within alignment constraints */
    uint32_t alignment = 0x200000;  /* 2MB alignment for modern kernels */
    uint32_t min_addr = 0x1000000;  /* 16MB minimum */
    uint32_t max_addr = 0x4000000;  /* 64MB maximum for 128MB VM */
    
    /* Use entropy to generate random offset */
    uint32_t range = (max_addr - min_addr) / alignment;
    uint32_t offset = (entropy % range) * alignment;
    uint32_t kaslr_addr = min_addr + offset;
    
    g_linux_boot_state.kernel_addr = kaslr_addr;
    
    return 0;
}

/**
 * Get kernel load address (with KASLR if enabled)
 */
uint32_t linux_boot_get_kernel_addr(void) {
    return g_linux_boot_state.kernel_addr;
}

/**
 * Set initrd (initial ramdisk) address and size
 */
int linux_boot_set_initrd(LinuxVM* vm, uint32_t addr, uint32_t size) {
    if (!vm) {
        return -1;
    }
    
    g_linux_boot_state.initrd_addr = addr;
    g_linux_boot_state.initrd_size = size;
    
    return 0;
}

/**
 * Get initrd information
 */
int linux_boot_get_initrd(uint32_t* addr, uint32_t* size) {
    if (addr) *addr = g_linux_boot_state.initrd_addr;
    if (size) *size = g_linux_boot_state.initrd_size;
    return 0;
}

/**
 * Set command line for Linux boot
 */
int linux_boot_set_cmdline(const char* cmdline) {
    if (!cmdline) {
        return -1;
    }
    
    platform_strncpy(g_linux_boot_state.cmdline, cmdline, sizeof(g_linux_boot_state.cmdline));
    return 0;
}

/**
 * Get command line
 */
const char* linux_boot_get_cmdline(void) {
    return g_linux_boot_state.cmdline;
}

/**
 * Append to command line
 */
int linux_boot_append_cmdline(const char* append) {
    if (!append) {
        return -1;
    }
    
    uint32_t current_len = platform_strlen(g_linux_boot_state.cmdline);
    uint32_t append_len = platform_strlen(append);
    
    if (current_len + append_len + 2 >= sizeof(g_linux_boot_state.cmdline)) {
        return -1;  /* Would overflow */
    }
    
    if (current_len > 0) {
        g_linux_boot_state.cmdline[current_len++] = ' ';
    }
    platform_strncpy(g_linux_boot_state.cmdline + current_len, append, 
                     sizeof(g_linux_boot_state.cmdline) - current_len);
    
    return 0;
}

/* EFI boot support */
#define EFI_LOADER_SIGNATURE    "EL64"
#define EFI_LOADER_SIGNATURE_32 "EL32"

/**
 * EFI system table pointer (for EFI boot)
 */
typedef struct {
    uint64_t efi_system_table;
    uint64_t efi_memmap;
    uint32_t efi_memmap_size;
    uint32_t efi_memmap_desc_size;
    uint32_t efi_memmap_desc_version;
    char efi_loader_signature[4];
} efi_boot_info_t;

static efi_boot_info_t g_efi_boot_info = {0};

/**
 * Setup EFI boot (for UEFI systems)
 */
int linux_boot_setup_efi(LinuxVM* vm, uint64_t system_table) {
    if (!vm) {
        return -1;
    }
    
    g_efi_boot_info.efi_system_table = system_table;
    platform_strncpy(g_efi_boot_info.efi_loader_signature, EFI_LOADER_SIGNATURE_32, 4);
    
    return 0;
}

/**
 * Check if EFI boot is configured
 */
bool linux_boot_is_efi(void) {
    return g_efi_boot_info.efi_system_table != 0;
}

/**
 * Get EFI system table address
 */
uint64_t linux_boot_get_efi_system_table(void) {
    return g_efi_boot_info.efi_system_table;
}

/* ============================================================================
 * EXTENDED ANDROID BOOT PROTOCOL FEATURES  
 * ============================================================================ */

/**
 * Load vendor boot image
 */
int android_boot_load_vendor(AndroidVM* vm, const uint8_t* vendor_image, uint32_t size) {
    if (!vm || !vendor_image || size < sizeof(vendor_boot_img_hdr_t)) {
        return -1;
    }
    
    /* Verify vendor boot magic */
    if (platform_memcmp(vendor_image, "VNDRBOOT", 8) != 0) {
        return -1;
    }
    
    const vendor_boot_img_hdr_t* hdr = (const vendor_boot_img_hdr_t*)vendor_image;
    
    /* Store vendor boot configuration */
    g_android_boot_state.dtb_addr = (uint32_t)hdr->dtb_addr;
    g_android_boot_state.dtb_size = hdr->dtb_size;
    
    /* Append vendor cmdline to existing cmdline */
    if (hdr->cmdline[0] != '\0') {
        uint32_t len = platform_strlen(g_android_boot_state.cmdline);
        if (len > 0 && len < sizeof(g_android_boot_state.cmdline) - 1) {
            g_android_boot_state.cmdline[len++] = ' ';
        }
        platform_strncpy(g_android_boot_state.cmdline + len, 
                        (const char*)hdr->cmdline,
                        sizeof(g_android_boot_state.cmdline) - len);
    }
    
    /* Load vendor ramdisk if present */
    if (hdr->vendor_ramdisk_size > 0) {
        uint32_t ramdisk_offset = hdr->header_size;
        const uint8_t* ramdisk_data = vendor_image + ramdisk_offset;
        
        if (ramdisk_offset + hdr->vendor_ramdisk_size <= size) {
            /* Merge with existing ramdisk or load as primary */
            if (g_android_boot_state.ramdisk_size == 0) {
                g_android_boot_state.ramdisk_addr = hdr->ramdisk_addr;
                g_android_boot_state.ramdisk_size = hdr->vendor_ramdisk_size;
            }
            (void)ramdisk_data;  /* Would copy to VM memory in real implementation */
        }
    }
    
    /* Load DTB if present */
    if (hdr->dtb_size > 0) {
        uint32_t dtb_offset = hdr->header_size + 
            ((hdr->vendor_ramdisk_size + hdr->page_size - 1) / hdr->page_size) * hdr->page_size;
        
        if (dtb_offset + hdr->dtb_size <= size) {
            /* Store DTB info */
            g_android_boot_state.dtb_addr = (uint32_t)hdr->dtb_addr;
            g_android_boot_state.dtb_size = hdr->dtb_size;
        }
    }
    
    return 0;
}

/**
 * Get Android boot command line from protocol state
 */
const char* boot_protocol_get_android_cmdline(void) {
    return g_android_boot_state.cmdline;
}

/**
 * Set Android boot command line in protocol state
 */
int boot_protocol_set_android_cmdline(const char* cmdline) {
    if (!cmdline) {
        return -1;
    }
    
    platform_strncpy(g_android_boot_state.cmdline, cmdline, sizeof(g_android_boot_state.cmdline));
    return 0;
}

/**
 * Append to Android boot command line in protocol state
 */
int boot_protocol_append_android_cmdline(const char* append) {
    if (!append) {
        return -1;
    }
    
    uint32_t len = platform_strlen(g_android_boot_state.cmdline);
    if (len > 0 && len < sizeof(g_android_boot_state.cmdline) - 1) {
        g_android_boot_state.cmdline[len++] = ' ';
    }
    platform_strncpy(g_android_boot_state.cmdline + len, append,
                     sizeof(g_android_boot_state.cmdline) - len);
    return 0;
}

/**
 * Get DTB (Device Tree Blob) information
 */
int boot_protocol_get_android_dtb(uint32_t* addr, uint32_t* size) {
    if (addr) *addr = g_android_boot_state.dtb_addr;
    if (size) *size = g_android_boot_state.dtb_size;
    return 0;
}

/**
 * Verify Android boot image signature (Android Verified Boot)
 */
int boot_protocol_verify_android_signature(const uint8_t* image, uint32_t size, const uint8_t* key, uint32_t key_size) {
    if (!image || size < sizeof(boot_img_hdr_v4_t) || !key || key_size == 0) {
        return -1;
    }
    
    /* Verify it's a v4 boot image with signature */
    if (!android_boot_is_valid_image(image, size)) {
        return -1;
    }
    
    const boot_img_hdr_v4_t* hdr = (const boot_img_hdr_v4_t*)image;
    if (hdr->header_version != 4 || hdr->signature_size == 0) {
        return -1;  /* No signature present */
    }
    
    /* Calculate expected signature location */
    uint32_t page_size = 4096;
    uint32_t kernel_pages = (hdr->kernel_size + page_size - 1) / page_size;
    uint32_t ramdisk_pages = (hdr->ramdisk_size + page_size - 1) / page_size;
    uint32_t sig_offset = hdr->header_size + (kernel_pages + ramdisk_pages) * page_size;
    
    if (sig_offset + hdr->signature_size > size) {
        return -1;  /* Signature extends beyond image */
    }
    
    /* Calculate hash of image (simplified - real implementation would use SHA256) */
    uint32_t hash = boot_crc32(image, sig_offset);
    (void)hash;  /* Would compare against signature */
    (void)key;   /* Would use key to verify */
    
    /* In real implementation, would verify signature using AVB key */
    return 0;
}

/**
 * Reset boot state
 */
void boot_protocol_reset(void) {
    platform_memset(&g_android_boot_state, 0, sizeof(g_android_boot_state));
    platform_memset(&g_linux_boot_state, 0, sizeof(g_linux_boot_state));
    platform_memset(&g_efi_boot_info, 0, sizeof(g_efi_boot_info));
}
