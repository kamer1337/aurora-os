/**
 * @file android_installer.c
 * @brief Android Installer Implementation
 */

#include "android_installer.h"
#include "gui.h"
#include "framebuffer.h"

/* Simple memory functions for freestanding environment */
static void simple_memset(void* ptr, int value, uint32_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
}

static void simple_memcpy(void* dest, const void* src, uint32_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static void simple_strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Global Android installer state */
static android_installer_t g_installer;
static bool g_installer_initialized = false;

/* Available Android distributions */
static android_distro_t g_android_distros[] = {
    {
        "AOSP Android 13.0",
        "13.0",
        800,  /* 800 MB */
        true,
        "Pure Android Open Source Project - Latest stable"
    },
    {
        "LineageOS 20",
        "20.0",
        900,  /* 900 MB */
        true,
        "Popular Android custom ROM based on AOSP"
    },
    {
        "AOSP Android 14.0",
        "14.0",
        850,  /* 850 MB */
        false,
        "Next generation Android - Coming Soon"
    },
    {
        "Pixel Experience",
        "13.0",
        950,  /* 950 MB */
        false,
        "Google Pixel UI experience - Coming Soon"
    },
    {
        "BlissOS x86",
        "15.8",
        1200, /* 1.2 GB */
        false,
        "Android x86 optimized distribution - Coming Soon"
    }
};

#define ANDROID_DISTRO_COUNT (sizeof(g_android_distros) / sizeof(android_distro_t))

int android_installer_init(void) {
    if (g_installer_initialized) {
        return 0;
    }
    
    simple_memset(&g_installer, 0, sizeof(android_installer_t));
    g_installer.state = ANDROID_INSTALLER_IDLE;
    g_installer.selected_distro = 0;
    g_installer.progress_percent = 0;
    simple_strncpy(g_installer.status_message, "Ready to install Android", sizeof(g_installer.status_message));
    g_installer.installation_complete = false;
    
    g_installer_initialized = true;
    
    return 0;
}

int android_installer_get_distros(android_distro_t* distros, uint32_t max_count) {
    if (!g_installer_initialized) {
        android_installer_init();
    }
    
    if (!distros || max_count == 0) {
        return -1;
    }
    
    uint32_t count = (max_count < ANDROID_DISTRO_COUNT) ? max_count : ANDROID_DISTRO_COUNT;
    
    for (uint32_t i = 0; i < count; i++) {
        simple_memcpy(&distros[i], &g_android_distros[i], sizeof(android_distro_t));
    }
    
    return count;
}

void android_installer_show_ui(void) {
    if (!g_installer_initialized) {
        android_installer_init();
    }
    
    // Get framebuffer info
    int screen_w = framebuffer_get_info()->width;
    int screen_h = framebuffer_get_info()->height;
    
    // Calculate installer window dimensions
    int installer_w = 800;
    int installer_h = 600;
    int installer_x = (screen_w - installer_w) / 2;
    int installer_y = (screen_h - installer_h) / 2;
    
    // Draw semi-transparent background
    color_t bg = {20, 25, 35, 240};
    framebuffer_draw_rect(installer_x, installer_y, installer_w, installer_h, bg);
    
    // Draw border
    color_t border = {100, 200, 100, 255};  /* Green border for Android */
    framebuffer_draw_rect_outline(installer_x, installer_y, installer_w, installer_h, border);
    
    // Title
    color_t title_color = {150, 255, 150, 255};  /* Light green */
    framebuffer_draw_string(installer_x + 20, installer_y + 20, "Android Installer", title_color, (color_t){0, 0, 0, 0});
    
    // Draw distribution list
    int list_y = installer_y + 70;
    int item_height = 80;
    
    for (uint32_t i = 0; i < ANDROID_DISTRO_COUNT && i < 5; i++) {
        int item_y = list_y + (i * item_height);
        
        // Item background (highlight if selected)
        color_t item_bg = (i == g_installer.selected_distro) ? 
            (color_t){60, 80, 60, 255} : (color_t){40, 45, 50, 255};
        framebuffer_draw_rect(installer_x + 20, item_y, installer_w - 40, item_height - 5, item_bg);
        
        // Distribution name
        color_t name_color = {255, 255, 255, 255};
        framebuffer_draw_string(installer_x + 30, item_y + 10, g_android_distros[i].name, name_color, (color_t){0, 0, 0, 0});
        
        // Version and size
        char info[64];
        simple_strncpy(info, "Version: ", sizeof(info));
        int len = 9;
        for (int j = 0; j < 10 && g_android_distros[i].version[j]; j++) {
            info[len++] = g_android_distros[i].version[j];
        }
        info[len++] = ' ';
        info[len++] = '-';
        info[len++] = ' ';
        info[len++] = 'S';
        info[len++] = 'i';
        info[len++] = 'z';
        info[len++] = 'e';
        info[len++] = ':';
        info[len++] = ' ';
        
        uint32_t size = g_android_distros[i].size_mb;
        if (size >= 1000) {
            info[len++] = '0' + (size / 1000);
            info[len++] = '.';
            info[len++] = '0' + ((size % 1000) / 100);
            info[len++] = ' ';
            info[len++] = 'G';
            info[len++] = 'B';
        } else {
            if (size >= 100) info[len++] = '0' + (size / 100);
            if (size >= 10) info[len++] = '0' + ((size % 100) / 10);
            info[len++] = '0' + (size % 10);
            info[len++] = ' ';
            info[len++] = 'M';
            info[len++] = 'B';
        }
        info[len] = '\0';
        
        color_t info_color = {200, 200, 200, 255};
        framebuffer_draw_string(installer_x + 30, item_y + 30, info, info_color, (color_t){0, 0, 0, 0});
        
        // Description
        color_t desc_color = {180, 180, 180, 255};
        framebuffer_draw_string(installer_x + 30, item_y + 50, g_android_distros[i].description, desc_color, (color_t){0, 0, 0, 0});
    }
    
    // Status bar
    int status_y = installer_y + installer_h - 80;
    color_t status_bg = {30, 35, 40, 255};
    framebuffer_draw_rect(installer_x, status_y, installer_w, 80, status_bg);
    
    // Status message
    color_t status_color = {200, 255, 200, 255};
    framebuffer_draw_string(installer_x + 20, status_y + 20, g_installer.status_message, status_color, (color_t){0, 0, 0, 0});
    
    // Progress bar (if installing)
    if (g_installer.state == ANDROID_INSTALLER_DOWNLOADING || 
        g_installer.state == ANDROID_INSTALLER_INSTALLING) {
        int progress_w = installer_w - 40;
        int progress_h = 20;
        int progress_x = installer_x + 20;
        int progress_y = status_y + 50;
        
        // Progress bar background
        color_t progress_bg = {50, 50, 50, 255};
        framebuffer_draw_rect(progress_x, progress_y, progress_w, progress_h, progress_bg);
        
        // Progress bar fill
        int fill_w = (progress_w * g_installer.progress_percent) / 100;
        color_t progress_fill = {100, 220, 100, 255};  /* Green progress */
        framebuffer_draw_rect(progress_x, progress_y, fill_w, progress_h, progress_fill);
        
        // Progress percentage text
        char percent_text[16];
        percent_text[0] = '0' + (g_installer.progress_percent / 100);
        percent_text[1] = '0' + ((g_installer.progress_percent % 100) / 10);
        percent_text[2] = '0' + (g_installer.progress_percent % 10);
        percent_text[3] = '%';
        percent_text[4] = '\0';
        
        framebuffer_draw_string(progress_x + progress_w + 10, progress_y + 3, percent_text, status_color, (color_t){0, 0, 0, 0});
    }
    
    // Instructions
    const char* instr = "Arrow keys to select, Enter to install, ESC to cancel";
    color_t instr_color = {150, 150, 150, 255};
    framebuffer_draw_string(installer_x + 20, installer_y + installer_h - 20, instr, instr_color, (color_t){0, 0, 0, 0});
}

int android_installer_start(uint32_t distro_index) {
    if (!g_installer_initialized) {
        android_installer_init();
    }
    
    if (distro_index >= ANDROID_DISTRO_COUNT) {
        return -1;
    }
    
    if (!g_android_distros[distro_index].available) {
        simple_strncpy(g_installer.status_message, "Distribution not available yet", sizeof(g_installer.status_message));
        return -1;
    }
    
    g_installer.selected_distro = distro_index;
    g_installer.state = ANDROID_INSTALLER_DOWNLOADING;
    g_installer.progress_percent = 0;
    
    simple_strncpy(g_installer.status_message, "Downloading Android distribution...", sizeof(g_installer.status_message));
    
    /* TODO: Implement actual download and installation */
    /* This would involve:
     * 1. Downloading Android boot.img (kernel + ramdisk)
     * 2. Downloading system.img partition
     * 3. Downloading vendor.img partition (if needed)
     * 4. Creating data.img partition
     * 5. Setting up Android VM with images
     * 6. Configuring Android properties
     * 7. Testing Android boot sequence
     */
    
    // Simulate progress
    g_installer.progress_percent = 50;
    g_installer.state = ANDROID_INSTALLER_INSTALLING;
    simple_strncpy(g_installer.status_message, "Installing Android...", sizeof(g_installer.status_message));
    
    // Simulate completion
    g_installer.progress_percent = 100;
    g_installer.state = ANDROID_INSTALLER_COMPLETED;
    g_installer.installation_complete = true;
    simple_strncpy(g_installer.status_message, "Android installation completed!", sizeof(g_installer.status_message));
    
    return 0;
}

int android_installer_get_status(android_installer_t* installer) {
    if (!g_installer_initialized) {
        android_installer_init();
    }
    
    if (!installer) {
        return -1;
    }
    
    simple_memcpy(installer, &g_installer, sizeof(android_installer_t));
    
    return 0;
}

int android_installer_cancel(void) {
    if (!g_installer_initialized) {
        return -1;
    }
    
    if (g_installer.state == ANDROID_INSTALLER_IDLE || 
        g_installer.state == ANDROID_INSTALLER_COMPLETED) {
        return 0;
    }
    
    g_installer.state = ANDROID_INSTALLER_IDLE;
    g_installer.progress_percent = 0;
    simple_strncpy(g_installer.status_message, "Installation cancelled", sizeof(g_installer.status_message));
    
    return 0;
}

bool android_installer_is_installed(void) {
    if (!g_installer_initialized) {
        android_installer_init();
    }
    
    return g_installer.installation_complete;
}
