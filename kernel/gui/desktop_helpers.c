/**
 * Aurora OS - Desktop Helper Functions Implementation
 * 
 * Utility functions for desktop operations and UI helpers
 */

#include "desktop_helpers.h"
#include "application.h"
#include "../drivers/storage.h"

static uint8_t desktop_helpers_initialized = 0;

void desktop_helpers_init(void) {
    if (desktop_helpers_initialized) return;
    
    /* Initialize helper subsystems */
    desktop_helpers_initialized = 1;
}

int desktop_perform_action(desktop_action_t action) {
    if (!desktop_helpers_initialized) {
        desktop_helpers_init();
    }
    
    switch (action) {
        case DESKTOP_ACTION_NEW_FOLDER:
            /* Would create a new folder in current directory */
            return 0;
            
        case DESKTOP_ACTION_NEW_FILE:
            /* Would create a new file */
            return 0;
            
        case DESKTOP_ACTION_REFRESH:
            /* Would refresh the desktop icons and windows */
            return 0;
            
        case DESKTOP_ACTION_PROPERTIES:
            /* Would show desktop properties dialog */
            return 0;
            
        case DESKTOP_ACTION_OPEN_TERMINAL:
            return app_launch(APP_TERMINAL);
            
        case DESKTOP_ACTION_OPEN_FILE_MANAGER:
            return app_launch(APP_FILE_MANAGER);
            
        case DESKTOP_ACTION_OPEN_DISK_MANAGER:
            return app_launch(APP_DISK_MANAGER);
            
        default:
            return -1;
    }
}

const char* desktop_format_size(uint64_t bytes, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 16) {
        return "0 B";
    }
    
    int pos = 0;
    
    /* Use bit shifting to avoid 64-bit division */
    if (bytes >= (1ULL << 40)) {
        /* TB range */
        uint32_t tb = (uint32_t)(bytes >> 40);
        if (tb >= 100) buffer[pos++] = '0' + (tb / 100);
        if (tb >= 10) buffer[pos++] = '0' + ((tb / 10) % 10);
        buffer[pos++] = '0' + (tb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'T';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 30)) {
        /* GB range */
        uint32_t gb = (uint32_t)(bytes >> 30);
        if (gb >= 100) buffer[pos++] = '0' + (gb / 100);
        if (gb >= 10) buffer[pos++] = '0' + ((gb / 10) % 10);
        buffer[pos++] = '0' + (gb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'G';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 20)) {
        /* MB range */
        uint32_t mb = (uint32_t)(bytes >> 20);
        if (mb >= 100) buffer[pos++] = '0' + (mb / 100);
        if (mb >= 10) buffer[pos++] = '0' + ((mb / 10) % 10);
        buffer[pos++] = '0' + (mb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'M';
        buffer[pos++] = 'B';
    } else if (bytes >= (1ULL << 10)) {
        /* KB range */
        uint32_t kb = (uint32_t)(bytes >> 10);
        if (kb >= 100) buffer[pos++] = '0' + (kb / 100);
        if (kb >= 10) buffer[pos++] = '0' + ((kb / 10) % 10);
        buffer[pos++] = '0' + (kb % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'K';
        buffer[pos++] = 'B';
    } else {
        /* Bytes */
        uint32_t b = (uint32_t)bytes;
        if (b >= 100) buffer[pos++] = '0' + (b / 100);
        if (b >= 10) buffer[pos++] = '0' + ((b / 10) % 10);
        buffer[pos++] = '0' + (b % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'B';
    }
    
    buffer[pos] = '\0';
    return buffer;
}

const char* desktop_format_number(uint32_t num, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 12) {
        return "0";
    }
    
    int pos = 0;
    
    /* Format number with commas for readability */
    if (num >= 1000000000) {
        buffer[pos++] = '0' + (num / 1000000000);
        buffer[pos++] = ',';
        num %= 1000000000;
        buffer[pos++] = '0' + (num / 100000000);
        buffer[pos++] = '0' + ((num / 10000000) % 10);
        buffer[pos++] = '0' + ((num / 1000000) % 10);
        buffer[pos++] = ',';
        num %= 1000000;
        buffer[pos++] = '0' + (num / 100000);
        buffer[pos++] = '0' + ((num / 10000) % 10);
        buffer[pos++] = '0' + ((num / 1000) % 10);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else if (num >= 1000000) {
        buffer[pos++] = '0' + (num / 1000000);
        buffer[pos++] = ',';
        num %= 1000000;
        buffer[pos++] = '0' + (num / 100000);
        buffer[pos++] = '0' + ((num / 10000) % 10);
        buffer[pos++] = '0' + ((num / 1000) % 10);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else if (num >= 1000) {
        buffer[pos++] = '0' + (num / 1000);
        buffer[pos++] = ',';
        num %= 1000;
        buffer[pos++] = '0' + (num / 100);
        buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    } else {
        if (num >= 100) buffer[pos++] = '0' + (num / 100);
        if (num >= 10) buffer[pos++] = '0' + ((num / 10) % 10);
        buffer[pos++] = '0' + (num % 10);
    }
    
    buffer[pos] = '\0';
    return buffer;
}

int desktop_get_icon_count(void) {
    /* Would return actual count from desktop_modules.c */
    return 3;  /* File Manager, System Info, Disk Manager */
}

int desktop_launch_app(uint8_t app_type) {
    if (app_type >= APP_MAX) {
        return -1;
    }
    
    return app_launch((app_type_t)app_type);
}
