/**
 * @file android_installer.h
 * @brief Android Installer GUI for Aurora OS
 * 
 * Provides user interface and functionality for installing Android distributions
 */

#ifndef ANDROID_INSTALLER_H
#define ANDROID_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>

/* Android distribution information */
typedef struct {
    char name[64];
    char version[16];
    uint32_t size_mb;
    bool available;
    char description[128];
} android_distro_t;

/* Android installer state */
typedef enum {
    ANDROID_INSTALLER_IDLE = 0,
    ANDROID_INSTALLER_SELECTING,
    ANDROID_INSTALLER_DOWNLOADING,
    ANDROID_INSTALLER_INSTALLING,
    ANDROID_INSTALLER_COMPLETED,
    ANDROID_INSTALLER_ERROR
} android_installer_state_t;

/* Android installer structure */
typedef struct {
    android_installer_state_t state;
    uint32_t selected_distro;
    uint32_t progress_percent;
    char status_message[128];
    bool installation_complete;
} android_installer_t;

/**
 * Initialize Android installer system
 * @return 0 on success, -1 on failure
 */
int android_installer_init(void);

/**
 * Get list of available Android distributions
 * @param distros Array to store distribution information
 * @param max_count Maximum number of distributions to return
 * @return Number of distributions available
 */
int android_installer_get_distros(android_distro_t* distros, uint32_t max_count);

/**
 * Show Android installer UI
 */
void android_installer_show_ui(void);

/**
 * Start Android installation
 * @param distro_index Index of distribution to install
 * @return 0 on success, -1 on failure
 */
int android_installer_start(uint32_t distro_index);

/**
 * Get current installation status
 * @param installer Pointer to installer structure to fill
 * @return 0 on success, -1 on failure
 */
int android_installer_get_status(android_installer_t* installer);

/**
 * Cancel ongoing installation
 * @return 0 on success, -1 on failure
 */
int android_installer_cancel(void);

/**
 * Check if Android is installed
 * @return true if Android is installed
 */
bool android_installer_is_installed(void);

#endif /* ANDROID_INSTALLER_H */
