/**
 * Aurora OS - Package Manager
 * 
 * Application package management system
 */

#include "package_manager.h"
#include "../memory/memory.h"

// Package database
static package_t g_package_db[MAX_PACKAGES];
static int g_package_count = 0;
static uint8_t g_initialized = 0;

// Helper function to copy string
static void pkg_strcpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Helper function to compare strings
static int pkg_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Helper function to search substring
static int pkg_strstr(const char* haystack, const char* needle) {
    if (!*needle) return 1;
    
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return 1;
    }
    return 0;
}

// Initialize sample packages
static void init_sample_packages(void) {
    // Package 1: Text Editor Pro
    pkg_strcpy(g_package_db[0].name, "textedit-pro", MAX_PACKAGE_NAME);
    pkg_strcpy(g_package_db[0].version, "2.1.0", 16);
    pkg_strcpy(g_package_db[0].description, "Advanced text editor with syntax highlighting", MAX_PACKAGE_DESC);
    pkg_strcpy(g_package_db[0].author, "Aurora Team", 64);
    g_package_db[0].size = 512;
    g_package_db[0].status = PKG_STATUS_AVAILABLE;
    g_package_db[0].dependency_count = 0;
    g_package_db[0].installed = 0;
    
    // Package 2: Web Browser
    pkg_strcpy(g_package_db[1].name, "aurora-browser", MAX_PACKAGE_NAME);
    pkg_strcpy(g_package_db[1].version, "1.0.0", 16);
    pkg_strcpy(g_package_db[1].description, "Modern web browser with HTML5 support", MAX_PACKAGE_DESC);
    pkg_strcpy(g_package_db[1].author, "Aurora Team", 64);
    g_package_db[1].size = 2048;
    g_package_db[1].status = PKG_STATUS_AVAILABLE;
    g_package_db[1].dependency_count = 1;
    pkg_strcpy(g_package_db[1].dependencies[0], "libwebkit", MAX_PACKAGE_NAME);
    g_package_db[1].installed = 0;
    
    // Package 3: Media Player
    pkg_strcpy(g_package_db[2].name, "aurora-player", MAX_PACKAGE_NAME);
    pkg_strcpy(g_package_db[2].version, "1.5.2", 16);
    pkg_strcpy(g_package_db[2].description, "Audio and video player", MAX_PACKAGE_DESC);
    pkg_strcpy(g_package_db[2].author, "Aurora Team", 64);
    g_package_db[2].size = 1024;
    g_package_db[2].status = PKG_STATUS_INSTALLED;
    g_package_db[2].dependency_count = 0;
    g_package_db[2].installed = 1;
    
    // Package 4: Development Tools
    pkg_strcpy(g_package_db[3].name, "dev-tools", MAX_PACKAGE_NAME);
    pkg_strcpy(g_package_db[3].version, "3.0.0", 16);
    pkg_strcpy(g_package_db[3].description, "C/C++ compiler and debugger", MAX_PACKAGE_DESC);
    pkg_strcpy(g_package_db[3].author, "Aurora Team", 64);
    g_package_db[3].size = 4096;
    g_package_db[3].status = PKG_STATUS_AVAILABLE;
    g_package_db[3].dependency_count = 0;
    g_package_db[3].installed = 0;
    
    // Package 5: Graphics Suite
    pkg_strcpy(g_package_db[4].name, "graphics-suite", MAX_PACKAGE_NAME);
    pkg_strcpy(g_package_db[4].version, "2.3.1", 16);
    pkg_strcpy(g_package_db[4].description, "Image editor and graphics tools", MAX_PACKAGE_DESC);
    pkg_strcpy(g_package_db[4].author, "Aurora Graphics Team", 64);
    g_package_db[4].size = 3072;
    g_package_db[4].status = PKG_STATUS_AVAILABLE;
    g_package_db[4].dependency_count = 0;
    g_package_db[4].installed = 0;
    
    g_package_count = 5;
}

int pkgman_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize package database
    for (int i = 0; i < MAX_PACKAGES; i++) {
        g_package_db[i].name[0] = '\0';
        g_package_db[i].installed = 0;
        g_package_db[i].status = PKG_STATUS_AVAILABLE;
    }
    
    // Load sample packages
    init_sample_packages();
    
    g_initialized = 1;
    return 0;
}

int pkgman_install(const char* package_name) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    // Find package
    package_t* pkg = pkgman_get_package(package_name);
    if (!pkg) {
        return -1; // Package not found
    }
    
    if (pkg->installed) {
        return 0; // Already installed
    }
    
    // Check dependencies
    for (int i = 0; i < pkg->dependency_count; i++) {
        package_t* dep = pkgman_get_package(pkg->dependencies[i]);
        if (!dep || !dep->installed) {
            // Dependency not met
            return -1;
        }
    }
    
    // Install package (simplified - would actually copy files)
    pkg->status = PKG_STATUS_INSTALLING;
    pkg->installed = 1;
    pkg->status = PKG_STATUS_INSTALLED;
    
    return 0;
}

int pkgman_remove(const char* package_name) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    package_t* pkg = pkgman_get_package(package_name);
    if (!pkg || !pkg->installed) {
        return -1;
    }
    
    // Check if other packages depend on this
    for (int i = 0; i < g_package_count; i++) {
        if (!g_package_db[i].installed) continue;
        
        for (int j = 0; j < g_package_db[i].dependency_count; j++) {
            if (pkg_strcmp(g_package_db[i].dependencies[j], package_name) == 0) {
                // Another package depends on this
                return -1;
            }
        }
    }
    
    // Remove package
    pkg->status = PKG_STATUS_REMOVING;
    pkg->installed = 0;
    pkg->status = PKG_STATUS_AVAILABLE;
    
    return 0;
}

int pkgman_update(void) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    // Update package list (would fetch from repository)
    return 0;
}

int pkgman_upgrade(void) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    // Upgrade all installed packages
    int upgraded = 0;
    for (int i = 0; i < g_package_count; i++) {
        if (g_package_db[i].installed) {
            // Check for updates and upgrade
            upgraded++;
        }
    }
    
    return upgraded;
}

int pkgman_search(const char* query, package_t* results, int max_results) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    int count = 0;
    for (int i = 0; i < g_package_count && count < max_results; i++) {
        if (pkg_strstr(g_package_db[i].name, query) ||
            pkg_strstr(g_package_db[i].description, query)) {
            results[count] = g_package_db[i];
            count++;
        }
    }
    
    return count;
}

package_t* pkgman_get_package(const char* package_name) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    for (int i = 0; i < g_package_count; i++) {
        if (pkg_strcmp(g_package_db[i].name, package_name) == 0) {
            return &g_package_db[i];
        }
    }
    
    return NULL;
}

int pkgman_list_packages(package_t* packages, int max_packages, uint8_t installed_only) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    int count = 0;
    for (int i = 0; i < g_package_count && count < max_packages; i++) {
        if (!installed_only || g_package_db[i].installed) {
            packages[count] = g_package_db[i];
            count++;
        }
    }
    
    return count;
}

int pkgman_verify(const char* package_name) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    package_t* pkg = pkgman_get_package(package_name);
    if (!pkg || !pkg->installed) {
        return 0;
    }
    
    // Verify package integrity (simplified)
    return 1;
}

int pkgman_check_dependencies(void) {
    if (!g_initialized) {
        pkgman_init();
    }
    
    int broken = 0;
    for (int i = 0; i < g_package_count; i++) {
        if (!g_package_db[i].installed) continue;
        
        for (int j = 0; j < g_package_db[i].dependency_count; j++) {
            package_t* dep = pkgman_get_package(g_package_db[i].dependencies[j]);
            if (!dep || !dep->installed) {
                g_package_db[i].status = PKG_STATUS_BROKEN;
                broken++;
                break;
            }
        }
    }
    
    return broken;
}
