/**
 * Aurora OS - Package Manager
 * 
 * Application package management system
 */

#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <stdint.h>

#define MAX_PACKAGE_NAME 64
#define MAX_PACKAGE_DESC 256
#define MAX_PACKAGES 128
#define MAX_DEPENDENCIES 16

// Package status
typedef enum {
    PKG_STATUS_AVAILABLE,
    PKG_STATUS_INSTALLED,
    PKG_STATUS_INSTALLING,
    PKG_STATUS_REMOVING,
    PKG_STATUS_BROKEN
} package_status_t;

// Package metadata
typedef struct {
    char name[MAX_PACKAGE_NAME];
    char version[16];
    char description[MAX_PACKAGE_DESC];
    char author[64];
    uint32_t size;  // Size in KB
    package_status_t status;
    char dependencies[MAX_DEPENDENCIES][MAX_PACKAGE_NAME];
    uint8_t dependency_count;
    uint8_t installed;
} package_t;

/**
 * Initialize package manager
 * @return 0 on success, -1 on failure
 */
int pkgman_init(void);

/**
 * Install a package
 * @param package_name Package name to install
 * @return 0 on success, -1 on failure
 */
int pkgman_install(const char* package_name);

/**
 * Remove a package
 * @param package_name Package name to remove
 * @return 0 on success, -1 on failure
 */
int pkgman_remove(const char* package_name);

/**
 * Update package list
 * @return 0 on success, -1 on failure
 */
int pkgman_update(void);

/**
 * Upgrade all packages
 * @return 0 on success, -1 on failure
 */
int pkgman_upgrade(void);

/**
 * Search for packages
 * @param query Search query
 * @param results Array to store results
 * @param max_results Maximum number of results
 * @return Number of results found
 */
int pkgman_search(const char* query, package_t* results, int max_results);

/**
 * Get package information
 * @param package_name Package name
 * @return Pointer to package or NULL if not found
 */
package_t* pkgman_get_package(const char* package_name);

/**
 * List all packages
 * @param packages Array to store packages
 * @param max_packages Maximum number of packages
 * @param installed_only List only installed packages
 * @return Number of packages returned
 */
int pkgman_list_packages(package_t* packages, int max_packages, uint8_t installed_only);

/**
 * Verify package integrity
 * @param package_name Package name
 * @return 1 if valid, 0 if invalid
 */
int pkgman_verify(const char* package_name);

/**
 * Check for broken dependencies
 * @return Number of broken packages
 */
int pkgman_check_dependencies(void);

#endif // PACKAGE_MANAGER_H
