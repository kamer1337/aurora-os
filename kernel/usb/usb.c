/**
 * Aurora OS - USB Implementation
 * 
 * Universal Serial Bus support
 */

#include "usb.h"
#include "usb_storage.h"
#include "../memory/memory.h"
#include <stddef.h>

/* USB device table */
#define MAX_USB_DEVICES 16
static usb_device_t usb_devices[MAX_USB_DEVICES];
static uint8_t next_address = 1;

/* Host controller operations */
static usb_hc_ops_t* hc_ops = NULL;

/* Hot-plug support */
#define MAX_HOTPLUG_HANDLERS 8
static usb_hotplug_handler_t hotplug_handlers[MAX_HOTPLUG_HANDLERS];
static uint32_t hotplug_handler_count = 0;

/* UHCI registers (simplified) */
#define UHCI_USBCMD     0x00
#define UHCI_USBSTS     0x02
#define UHCI_USBINTR    0x04
#define UHCI_FRNUM      0x06
#define UHCI_FRBASEADD  0x08
#define UHCI_SOFMOD     0x0C
#define UHCI_PORTSC1    0x10
#define UHCI_PORTSC2    0x12

/**
 * Initialize USB subsystem
 */
void usb_init(void) {
    /* Initialize device table */
    for (uint32_t i = 0; i < MAX_USB_DEVICES; i++) {
        usb_devices[i].address = 0;
        usb_devices[i].state = USB_STATE_DETACHED;
        usb_devices[i].next = NULL;
        usb_devices[i].driver_data = NULL;
    }
    
    /* Initialize hot-plug handler table */
    for (uint32_t i = 0; i < MAX_HOTPLUG_HANDLERS; i++) {
        hotplug_handlers[i].callback = NULL;
        hotplug_handlers[i].user_data = NULL;
        hotplug_handlers[i].next = NULL;
    }
    hotplug_handler_count = 0;
    
    /* Initialize USB drivers */
    usb_hid_init();
    usb_msd_init();
    
    /* Initialize host controller */
    uhci_init();
}

/**
 * Allocate USB device structure
 */
static usb_device_t* alloc_usb_device(void) {
    for (uint32_t i = 0; i < MAX_USB_DEVICES; i++) {
        if (usb_devices[i].state == USB_STATE_DETACHED) {
            return &usb_devices[i];
        }
    }
    return NULL;
}

/**
 * Get USB device by address
 */
usb_device_t* usb_get_device(uint8_t address) {
    for (uint32_t i = 0; i < MAX_USB_DEVICES; i++) {
        if (usb_devices[i].address == address) {
            return &usb_devices[i];
        }
    }
    return NULL;
}

/**
 * Control transfer
 */
int usb_control_transfer(usb_device_t* device, usb_setup_packet_t* setup, 
                         uint8_t* data, uint32_t length) {
    if (!device || !setup || !hc_ops) {
        return -1;
    }
    
    /* Create transfer structure */
    usb_transfer_t transfer;
    transfer.device = device;
    transfer.endpoint = 0; /* Control endpoint */
    transfer.type = USB_TRANSFER_CONTROL;
    transfer.buffer = data;
    transfer.length = length;
    transfer.actual_length = 0;
    transfer.status = 0;
    
    /* Submit transfer to host controller */
    if (hc_ops->submit_transfer) {
        return hc_ops->submit_transfer(&transfer);
    }
    
    return -1;
}

/**
 * Bulk transfer
 */
int usb_bulk_transfer(usb_device_t* device, uint8_t endpoint, 
                      uint8_t* data, uint32_t length) {
    if (!device || !data || !hc_ops) {
        return -1;
    }
    
    usb_transfer_t transfer;
    transfer.device = device;
    transfer.endpoint = endpoint;
    transfer.type = USB_TRANSFER_BULK;
    transfer.buffer = data;
    transfer.length = length;
    transfer.actual_length = 0;
    transfer.status = 0;
    
    if (hc_ops->submit_transfer) {
        return hc_ops->submit_transfer(&transfer);
    }
    
    return -1;
}

/**
 * Interrupt transfer
 */
int usb_interrupt_transfer(usb_device_t* device, uint8_t endpoint,
                           uint8_t* data, uint32_t length) {
    if (!device || !data || !hc_ops) {
        return -1;
    }
    
    usb_transfer_t transfer;
    transfer.device = device;
    transfer.endpoint = endpoint;
    transfer.type = USB_TRANSFER_INTERRUPT;
    transfer.buffer = data;
    transfer.length = length;
    transfer.actual_length = 0;
    transfer.status = 0;
    
    if (hc_ops->submit_transfer) {
        return hc_ops->submit_transfer(&transfer);
    }
    
    return -1;
}

/**
 * Get device descriptor
 */
int usb_get_device_descriptor(usb_device_t* device) {
    if (!device) {
        return -1;
    }
    
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x80; /* Device to host */
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (USB_DESC_DEVICE << 8) | 0;
    setup.wIndex = 0;
    setup.wLength = sizeof(usb_device_descriptor_t);
    
    return usb_control_transfer(device, &setup, (uint8_t*)&device->descriptor, 
                                sizeof(usb_device_descriptor_t));
}

/**
 * Get configuration descriptor
 */
int usb_get_config_descriptor(usb_device_t* device, uint8_t config_num,
                               uint8_t* buffer, uint32_t length) {
    if (!device || !buffer) {
        return -1;
    }
    
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x80;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (USB_DESC_CONFIGURATION << 8) | config_num;
    setup.wIndex = 0;
    setup.wLength = length;
    
    return usb_control_transfer(device, &setup, buffer, length);
}

/**
 * Set device address
 */
int usb_set_address(usb_device_t* device, uint8_t address) {
    if (!device) {
        return -1;
    }
    
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x00; /* Host to device */
    setup.bRequest = USB_REQ_SET_ADDRESS;
    setup.wValue = address;
    setup.wIndex = 0;
    setup.wLength = 0;
    
    int result = usb_control_transfer(device, &setup, NULL, 0);
    if (result == 0) {
        device->address = address;
        device->state = USB_STATE_ADDRESS;
    }
    
    return result;
}

/**
 * Set device configuration
 */
int usb_set_configuration(usb_device_t* device, uint8_t config) {
    if (!device) {
        return -1;
    }
    
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x00;
    setup.bRequest = USB_REQ_SET_CONFIGURATION;
    setup.wValue = config;
    setup.wIndex = 0;
    setup.wLength = 0;
    
    int result = usb_control_transfer(device, &setup, NULL, 0);
    if (result == 0) {
        device->state = USB_STATE_CONFIGURED;
    }
    
    return result;
}

/**
 * Enumerate USB device
 */
int usb_enumerate_device(uint32_t port) {
    if (!hc_ops) {
        return -1;
    }
    
    /* Allocate device structure */
    usb_device_t* device = alloc_usb_device();
    if (!device) {
        return -1;
    }
    
    /* Reset port */
    if (hc_ops->reset_port) {
        hc_ops->reset_port(port);
    }
    
    /* Device starts at address 0 */
    device->address = 0;
    device->state = USB_STATE_DEFAULT;
    device->speed = USB_SPEED_FULL; /* Assume full speed */
    
    /* Get device descriptor */
    if (usb_get_device_descriptor(device) != 0) {
        device->state = USB_STATE_DETACHED;
        return -1;
    }
    
    /* Assign address */
    uint8_t new_address = next_address++;
    if (usb_set_address(device, new_address) != 0) {
        device->state = USB_STATE_DETACHED;
        return -1;
    }
    
    /* Get full device descriptor */
    if (usb_get_device_descriptor(device) != 0) {
        device->state = USB_STATE_DETACHED;
        return -1;
    }
    
    /* Set configuration */
    if (usb_set_configuration(device, 1) != 0) {
        device->state = USB_STATE_DETACHED;
        return -1;
    }
    
    /* Attach driver based on device class */
    switch (device->descriptor.bDeviceClass) {
        case USB_CLASS_HID:
            usb_hid_attach(device);
            break;
        case USB_CLASS_MASS_STORAGE:
            usb_msd_attach(device);
            break;
        default:
            break;
    }
    
    return 0;
}

/**
 * Initialize UHCI controller (simplified)
 */
void uhci_init(void) {
    /* In a real implementation, we would:
     * 1. Detect UHCI controller via PCI
     * 2. Map MMIO/IO space
     * 3. Reset controller
     * 4. Setup frame list
     * 5. Enable interrupts
     * 6. Start controller
     */
    
    /* For now, just initialize basic structures */
}

/**
 * Detect UHCI devices
 */
int uhci_detect_devices(void) {
    /* Check ports for connected devices */
    for (uint32_t port = 0; port < 2; port++) {
        /* Would check port status here */
        /* If device connected, enumerate it */
        usb_enumerate_device(port);
    }
    
    return 0;
}

/**
 * Initialize USB HID driver
 */
void usb_hid_init(void) {
    /* Initialize HID driver structures */
}

/**
 * Attach HID device
 */
int usb_hid_attach(usb_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Get HID descriptor */
    /* Setup interrupt endpoint */
    /* Start polling for input */
    
    return 0;
}

/**
 * Initialize USB mass storage driver
 */
void usb_msd_init(void) {
    /* Initialize USB mass storage subsystem */
    usb_storage_init();
}

/**
 * Attach mass storage device
 */
int usb_msd_attach(usb_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Attach device using USB storage driver */
    usb_storage_device_t* storage_dev = usb_storage_attach(device);
    if (!storage_dev) {
        return -1;
    }
    
    /* Notify hot-plug handlers of attachment */
    usb_hotplug_notify(device, USB_EVENT_DEVICE_ATTACHED);
    
    return 0;
}

/**
 * Register a hot-plug callback handler
 */
int usb_hotplug_register_callback(usb_hotplug_callback_t callback, void* user_data) {
    if (!callback || hotplug_handler_count >= MAX_HOTPLUG_HANDLERS) {
        return -1;
    }
    
    hotplug_handlers[hotplug_handler_count].callback = callback;
    hotplug_handlers[hotplug_handler_count].user_data = user_data;
    hotplug_handlers[hotplug_handler_count].next = NULL;
    hotplug_handler_count++;
    
    return 0;
}

/**
 * Unregister a hot-plug callback handler
 */
int usb_hotplug_unregister_callback(usb_hotplug_callback_t callback) {
    if (!callback) {
        return -1;
    }
    
    for (uint32_t i = 0; i < hotplug_handler_count; i++) {
        if (hotplug_handlers[i].callback == callback) {
            /* Shift remaining handlers down */
            for (uint32_t j = i; j < hotplug_handler_count - 1; j++) {
                hotplug_handlers[j] = hotplug_handlers[j + 1];
            }
            hotplug_handler_count--;
            return 0;
        }
    }
    
    return -1;
}

/**
 * Notify all registered hot-plug handlers of a device event
 */
void usb_hotplug_notify(usb_device_t* device, usb_hotplug_event_t event) {
    if (!device) {
        return;
    }
    
    for (uint32_t i = 0; i < hotplug_handler_count; i++) {
        if (hotplug_handlers[i].callback) {
            hotplug_handlers[i].callback(device, event, hotplug_handlers[i].user_data);
        }
    }
}

/**
 * Poll USB ports for device insertion/removal
 * Should be called periodically from the main loop or timer
 * 
 * Note: This is a basic implementation that scans for new devices.
 * Full hot-plug support requires:
 * 1. Port status change detection via interrupts or polling
 * 2. Device state tracking to detect detachment
 * 3. Graceful cleanup of detached device resources
 */
int usb_poll_devices(void) {
    /* Check each device slot for state changes */
    for (uint32_t i = 0; i < MAX_USB_DEVICES; i++) {
        usb_device_t* device = &usb_devices[i];
        
        /* Check if previously attached device is now detached
         * In a real implementation, we would:
         * - Read port status registers to detect disconnect
         * - Check device response to detect unresponsive devices
         * - Call usb_hotplug_notify with USB_EVENT_DEVICE_DETACHED
         * - Clean up device resources and mark slot as available
         */
        if (device->state != USB_STATE_DETACHED && device->address != 0) {
            /* Port status checking would go here */
        }
    }
    
    /* Scan for new devices */
    uhci_detect_devices();
    
    return 0;
}

