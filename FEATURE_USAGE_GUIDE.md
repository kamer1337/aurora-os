# Aurora OS New Features Usage Guide

This document provides examples of using the newly implemented features: Image Wallpapers, USB Hot-Plug Support, and Multi-User System.

## 1. Image Wallpapers

### Setting a BMP Wallpaper

```c
#include "kernel/gui/wallpaper_manager.h"

// Initialize wallpaper manager
wallpaper_manager_init();

// Set a BMP image as wallpaper
wallpaper_manager_set_image("/wallpapers/background.bmp");

// The wallpaper will be automatically loaded and scaled to fit the screen
```

### Supported Formats
- **BMP**: Fully supported (24-bit and 32-bit color depths)
- **PNG**: Format detection implemented (full decoding requires zlib integration)

### Image Requirements
- Maximum file size: 10 MB
- Maximum dimensions: 4096 x 4096 pixels
- BMP formats: Uncompressed (BI_RGB), 24-bit or 32-bit color

### Example Usage in Desktop Configuration

```c
// Create a wallpaper selector in your GUI
void set_custom_wallpaper(const char* image_path) {
    if (wallpaper_manager_set_image(image_path) == 0) {
        // Success! Wallpaper will be displayed on next desktop refresh
        wallpaper_manager_draw(screen_width, screen_height);
    }
}
```

## 2. USB Hot-Plug Support

### Registering a Hot-Plug Callback

```c
#include "kernel/usb/usb.h"

// Define your callback function
void usb_device_callback(usb_device_t* device, usb_hotplug_event_t event, void* user_data) {
    if (event == USB_EVENT_DEVICE_ATTACHED) {
        printf("USB device attached: VID=%04x PID=%04x\n", 
               device->descriptor.idVendor, 
               device->descriptor.idProduct);
        
        // Handle device attachment
        // For example, mount a USB storage device
    } else if (event == USB_EVENT_DEVICE_DETACHED) {
        printf("USB device detached\n");
        
        // Handle device removal
        // For example, unmount the device
    }
}

// Register the callback
void setup_usb_monitoring(void) {
    usb_hotplug_register_callback(usb_device_callback, NULL);
}

// Periodically poll for device changes (call from main loop or timer)
void main_loop(void) {
    while (1) {
        usb_poll_devices();  // Check for device insertions
        // ... other main loop tasks
    }
}
```

### USB Storage Device Handling

```c
#include "kernel/usb/usb_storage.h"

void handle_usb_storage(usb_device_t* device, usb_hotplug_event_t event) {
    if (event == USB_EVENT_DEVICE_ATTACHED) {
        // Get the storage device
        usb_storage_device_t* storage = usb_storage_attach(device);
        
        if (storage && storage->status == USB_STORAGE_STATUS_ONLINE) {
            printf("USB Storage Device: %s %s\n", 
                   storage->vendor, 
                   storage->product);
            printf("Capacity: %llu MB\n", 
                   usb_storage_get_capacity_mb(storage));
            
            // Read/write data
            uint8_t buffer[512];
            usb_storage_read_blocks(storage, 0, 1, buffer);  // Read first block
        }
    }
}
```

## 3. Multi-User System

### User Management

```c
#include "kernel/security/user_manager.h"

// Initialize user manager
user_manager_init();

// Create a new user
uint32_t user_id;
if (user_create("john", "password123", 0, &user_id) == 0) {
    printf("User created with UID: %u\n", user_id);
}

// Authenticate a user
uint32_t authenticated_uid;
if (user_authenticate("john", "password123", &authenticated_uid) == 0) {
    printf("Authentication successful!\n");
} else {
    printf("Authentication failed!\n");
}

// Get user information
user_account_t* user = user_get_by_username("john");
if (user) {
    printf("Username: %s\n", user->username);
    printf("Home: %s\n", user->home_directory);
    printf("Admin: %s\n", user->is_admin ? "Yes" : "No");
}
```

### Permission Management

```c
// Set user permissions
user_set_permissions(user_id, USER_PERM_READ | USER_PERM_WRITE | USER_PERM_NETWORK);

// Check permissions
if (user_has_permission(user_id, USER_PERM_NETWORK)) {
    // Allow network access
    printf("User has network permission\n");
}

// Permission flags available:
// - USER_PERM_READ
// - USER_PERM_WRITE
// - USER_PERM_EXECUTE
// - USER_PERM_ADMIN
// - USER_PERM_NETWORK
// - USER_PERM_USB
// - USER_PERM_SYSTEM
// - USER_PERM_ALL (all permissions)
```

### User Home Directories

```c
// Get user home directory
const char* home = user_get_home_directory(user_id);
printf("User home: %s\n", home);

// Create user home directory (automatically done on user creation)
user_create_home_directory(user_id);

// Change directory to user home
vfs_chdir(home);
```

### Password Management

```c
// Change user password
if (user_change_password(user_id, "old_password", "new_password") == 0) {
    printf("Password changed successfully\n");
} else {
    printf("Failed to change password\n");
}
```

### Listing Users

```c
// List all users
user_account_t* users[MAX_USERS];
int count = user_list_all(users, MAX_USERS);

for (int i = 0; i < count; i++) {
    printf("User: %s (UID: %u)\n", users[i]->username, users[i]->uid);
}
```

### Default Users

On first boot, the system creates two default users:
- **admin** / admin123 (Administrator with all permissions)
- **user** / user123 (Regular user with basic permissions)

## Integration Example: Complete System

```c
#include "kernel/gui/wallpaper_manager.h"
#include "kernel/usb/usb.h"
#include "kernel/security/user_manager.h"

void initialize_aurora_features(void) {
    // Initialize all new subsystems
    wallpaper_manager_init();
    user_manager_init();
    usb_init();
    
    // Set up USB monitoring
    usb_hotplug_register_callback(handle_usb_events, NULL);
    
    // Authenticate user and load their preferences
    uint32_t uid;
    if (user_authenticate("admin", "admin123", &uid) == 0) {
        // Load user's custom wallpaper
        const char* home = user_get_home_directory(uid);
        char wallpaper_path[256];
        snprintf(wallpaper_path, sizeof(wallpaper_path), 
                 "%s/wallpaper.bmp", home);
        wallpaper_manager_set_image(wallpaper_path);
    }
}

void handle_usb_events(usb_device_t* device, usb_hotplug_event_t event, void* data) {
    if (event == USB_EVENT_DEVICE_ATTACHED) {
        // Check if user has USB permission
        uint32_t current_user_id = get_current_user_id();
        if (user_has_permission(current_user_id, USER_PERM_USB)) {
            // Allow USB device access
            if (device->descriptor.bDeviceClass == USB_CLASS_MASS_STORAGE) {
                usb_storage_attach(device);
            }
        }
    }
}
```

## Security Considerations

### Password Security
- Passwords are hashed using quantum cryptography
- Never stored in plain text
- Salt is included in the hash

### User Permissions
- Always check permissions before allowing privileged operations
- Admin users have all permissions by default
- Regular users have limited permissions

### File Access
- Users can only access their own home directories by default
- System files require admin permissions
- USB access requires USB permission flag

## Troubleshooting

### Image Wallpaper Issues
- **Error loading image**: Check file path and permissions
- **Image not displaying**: Ensure format is BMP (24 or 32-bit)
- **Image appears distorted**: Verify image is not corrupted

### USB Hot-Plug Issues
- **Devices not detected**: Ensure `usb_poll_devices()` is called periodically
- **Callback not triggered**: Verify callback is registered correctly
- **Storage device not accessible**: Check USB permission for current user

### Multi-User Issues
- **Authentication fails**: Verify username and password are correct
- **Permission denied**: Check user has required permission flags
- **Home directory missing**: Call `user_create_home_directory()`

## Future Enhancements

### Image Wallpapers
- PNG decompression support (requires zlib integration)
- JPEG format support
- Image caching for faster loading
- Animated wallpapers

### USB Hot-Plug
- Port status register monitoring for true hot-plug detection
- Automatic device cleanup on removal
- Support for more device classes (HID, Audio, Video)

### Multi-User
- LDAP/Active Directory integration
- Group management
- Quota management
- Session management with timeout
