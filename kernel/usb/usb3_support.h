/**
 * Aurora OS - USB 3.0/3.1 Support Header
 */

#ifndef USB3_SUPPORT_H
#define USB3_SUPPORT_H

typedef struct {
    int initialized;
    int xhci_controller;
    int superspeed_enabled;
} usb3_state_t;

// Function prototypes
int usb3_support_init(void);
int usb3_enable_superspeed(void);
int usb3_enable_superspeed_plus(void);

#endif // USB3_SUPPORT_H
