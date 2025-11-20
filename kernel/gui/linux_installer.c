/**
 * @file linux_installer.c
 * @brief Linux Installation System Implementation
 */

#include "linux_installer.h"
#include "framebuffer.h"

/* Simple string functions for freestanding environment */
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

static void simple_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void simple_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void simple_strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i = 0;
    while (i < n && src[i]) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
}

/* Global installer state */
static linux_installer_t g_installer;
static bool g_initialized = false;
static bool g_linux_installed = false;

/* Available Linux distributions */
static linux_distro_t g_available_distros[] = {
    {
        .name = "Tiny Linux",
        .version = "1.0",
        .description = "Minimal Linux distribution for embedded systems",
        .size_mb = 50,
        .available = true
    },
    {
        .name = "Aurora Linux",
        .version = "1.0",
        .description = "Custom Linux build optimized for Aurora OS",
        .size_mb = 100,
        .available = true
    },
    {
        .name = "Debian Minimal",
        .version = "11.0",
        .description = "Minimal Debian installation (Coming Soon)",
        .size_mb = 500,
        .available = false
    },
    {
        .name = "Alpine Linux",
        .version = "3.14",
        .description = "Security-oriented, lightweight Linux (Coming Soon)",
        .size_mb = 150,
        .available = false
    }
};

#define NUM_DISTROS (sizeof(g_available_distros) / sizeof(linux_distro_t))

int linux_installer_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    /* Initialize installer state */
    simple_memset(&g_installer, 0, sizeof(linux_installer_t));
    g_installer.state = INSTALLER_STATE_IDLE;
    
    g_initialized = true;
    return 0;
}

int linux_installer_get_distros(linux_distro_t* distros, uint32_t max_count) {
    if (!distros || max_count == 0) {
        return 0;
    }
    
    uint32_t count = (max_count < NUM_DISTROS) ? max_count : NUM_DISTROS;
    
    for (uint32_t i = 0; i < count; i++) {
        simple_memcpy(&distros[i], &g_available_distros[i], sizeof(linux_distro_t));
    }
    
    return count;
}

void linux_installer_show_ui(void) {
    if (!g_initialized) {
        linux_installer_init();
    }
    
    /* Get screen dimensions */
    int screen_w = framebuffer_get_info()->width;
    int screen_h = framebuffer_get_info()->height;
    
    int ui_w = 700;
    int ui_h = 500;
    int ui_x = (screen_w - ui_w) / 2;
    int ui_y = (screen_h - ui_h) / 2;
    
    /* Draw main window */
    color_t bg = {30, 30, 40, 255};
    framebuffer_draw_rect(ui_x, ui_y, ui_w, ui_h, bg);
    
    color_t border = {100, 150, 255, 255};
    framebuffer_draw_rect_outline(ui_x, ui_y, ui_w, ui_h, border);
    
    /* Title */
    color_t title_color = {255, 255, 255, 255};
    framebuffer_draw_string(ui_x + 20, ui_y + 20, "Linux Installer", title_color, (color_t){0, 0, 0, 0});
    
    /* Description */
    color_t desc_color = {200, 200, 200, 255};
    framebuffer_draw_string(ui_x + 20, ui_y + 50, 
        "Select a Linux distribution to install:", desc_color, (color_t){0, 0, 0, 0});
    
    /* List distributions */
    int list_y = ui_y + 90;
    int item_h = 80;
    
    for (uint32_t i = 0; i < NUM_DISTROS; i++) {
        int item_x = ui_x + 20;
        int item_y = list_y + (i * (item_h + 10));
        
        /* Item background */
        color_t item_bg = g_available_distros[i].available ? 
            (color_t){50, 50, 70, 255} : (color_t){40, 40, 50, 255};
        framebuffer_draw_rect(item_x, item_y, ui_w - 40, item_h, item_bg);
        framebuffer_draw_rect_outline(item_x, item_y, ui_w - 40, item_h, border);
        
        /* Distribution name */
        color_t name_color = g_available_distros[i].available ? 
            (color_t){255, 255, 255, 255} : (color_t){150, 150, 150, 255};
        framebuffer_draw_string(item_x + 10, item_y + 10, 
            g_available_distros[i].name, name_color, (color_t){0, 0, 0, 0});
        
        /* Version */
        char version_text[64];
        simple_strcpy(version_text, "Version: ");
        simple_strcat(version_text, g_available_distros[i].version);
        framebuffer_draw_string(item_x + 10, item_y + 30, 
            version_text, desc_color, (color_t){0, 0, 0, 0});
        
        /* Size */
        char size_text[32];
        size_text[0] = 'S'; size_text[1] = 'i'; size_text[2] = 'z'; size_text[3] = 'e';
        size_text[4] = ':'; size_text[5] = ' ';
        
        uint32_t size = g_available_distros[i].size_mb;
        int idx = 6;
        if (size >= 100) {
            size_text[idx++] = '0' + (size / 100);
            size %= 100;
        }
        if (size >= 10 || idx > 6) {
            size_text[idx++] = '0' + (size / 10);
            size %= 10;
        }
        size_text[idx++] = '0' + size;
        size_text[idx++] = ' ';
        size_text[idx++] = 'M';
        size_text[idx++] = 'B';
        size_text[idx] = '\0';
        
        framebuffer_draw_string(item_x + 10, item_y + 50, 
            size_text, desc_color, (color_t){0, 0, 0, 0});
    }
    
    /* Instructions */
    color_t info_color = {150, 150, 150, 255};
    framebuffer_draw_string(ui_x + 20, ui_y + ui_h - 40, 
        "Press number key (1-4) to select, ESC to cancel", info_color, (color_t){0, 0, 0, 0});
}

int linux_installer_start(uint32_t distro_index) {
    if (!g_initialized) {
        linux_installer_init();
    }
    
    if (distro_index >= NUM_DISTROS) {
        return -1;
    }
    
    if (!g_available_distros[distro_index].available) {
        simple_strcpy(g_installer.error_message, "Distribution not available yet");
        g_installer.error = true;
        g_installer.state = INSTALLER_STATE_ERROR;
        return -1;
    }
    
    /* Copy selected distribution info */
    simple_memcpy(&g_installer.selected_distro, &g_available_distros[distro_index], 
           sizeof(linux_distro_t));
    
    g_installer.state = INSTALLER_STATE_INSTALLING;
    g_installer.progress_percent = 0;
    simple_strcpy(g_installer.status_message, "Installing...");
    
    /* TODO: Actual installation logic */
    /* For now, simulate completion */
    g_installer.progress_percent = 100;
    g_installer.state = INSTALLER_STATE_COMPLETED;
    simple_strcpy(g_installer.status_message, "Installation completed successfully");
    g_linux_installed = true;
    
    return 0;
}

int linux_installer_get_status(linux_installer_t* installer) {
    if (!installer) {
        return -1;
    }
    
    if (!g_initialized) {
        linux_installer_init();
    }
    
    simple_memcpy(installer, &g_installer, sizeof(linux_installer_t));
    return 0;
}

int linux_installer_cancel(void) {
    if (!g_initialized) {
        return -1;
    }
    
    if (g_installer.state == INSTALLER_STATE_INSTALLING) {
        g_installer.state = INSTALLER_STATE_IDLE;
        simple_strcpy(g_installer.status_message, "Installation cancelled");
        return 0;
    }
    
    return -1;
}

bool linux_installer_is_installed(void) {
    return g_linux_installed;
}
