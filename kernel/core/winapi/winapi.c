/**
 * Aurora OS - Windows API Compatibility Layer Implementation
 * 
 * Main implementation for Windows API emulation
 */

#include "winapi.h"
#include "../../drivers/vga.h"

/* Thread-local storage for last error (simplified) */
static DWORD g_last_error = ERROR_SUCCESS;

/**
 * Initialize Windows API compatibility layer
 */
void winapi_init(void) {
    g_last_error = ERROR_SUCCESS;
    vga_write("WinAPI: Compatibility layer initialized\n");
}

/**
 * Get last error code
 */
DWORD winapi_get_last_error(void) {
    return g_last_error;
}

/**
 * Set last error code
 */
void winapi_set_last_error(DWORD error_code) {
    g_last_error = error_code;
}
