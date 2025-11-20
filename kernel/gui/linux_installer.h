/**
 * @file linux_installer.h
 * @brief Linux Installation System for Aurora OS
 * 
 * Provides interface for installing Linux distributions
 * to run in Aurora OS workspaces
 */

#ifndef LINUX_INSTALLER_H
#define LINUX_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>

/* Installer state */
typedef enum {
    INSTALLER_STATE_IDLE = 0,
    INSTALLER_STATE_SELECTING,
    INSTALLER_STATE_DOWNLOADING,
    INSTALLER_STATE_INSTALLING,
    INSTALLER_STATE_COMPLETED,
    INSTALLER_STATE_ERROR
} installer_state_t;

/* Linux distribution info */
typedef struct {
    char name[64];
    char version[32];
    char description[256];
    uint32_t size_mb;
    bool available;
} linux_distro_t;

/* Installer instance */
typedef struct {
    installer_state_t state;
    linux_distro_t selected_distro;
    uint32_t progress_percent;
    char status_message[256];
    bool error;
    char error_message[256];
} linux_installer_t;

/**
 * Initialize Linux installer system
 * @return 0 on success, -1 on failure
 */
int linux_installer_init(void);

/**
 * Get list of available Linux distributions
 * @param distros Output array for distributions
 * @param max_count Maximum number of distributions to return
 * @return Number of distributions found
 */
int linux_installer_get_distros(linux_distro_t* distros, uint32_t max_count);

/**
 * Show installer UI
 */
void linux_installer_show_ui(void);

/**
 * Start installation of selected distribution
 * @param distro_index Index of distribution to install
 * @return 0 on success, -1 on failure
 */
int linux_installer_start(uint32_t distro_index);

/**
 * Get installer status
 * @param installer Pointer to store installer state
 * @return 0 on success, -1 on failure
 */
int linux_installer_get_status(linux_installer_t* installer);

/**
 * Cancel ongoing installation
 * @return 0 on success, -1 on failure
 */
int linux_installer_cancel(void);

/**
 * Check if Linux is installed
 * @return true if Linux is installed and available
 */
bool linux_installer_is_installed(void);

#endif /* LINUX_INSTALLER_H */
