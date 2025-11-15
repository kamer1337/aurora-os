/**
 * Aurora OS - USB Support Header
 * 
 * Universal Serial Bus support
 */

#ifndef AURORA_USB_H
#define AURORA_USB_H

#include <stdint.h>

/* USB device states */
typedef enum {
    USB_STATE_DETACHED,
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED
} usb_device_state_t;

/* USB device speeds */
typedef enum {
    USB_SPEED_LOW,      /* 1.5 Mbps */
    USB_SPEED_FULL,     /* 12 Mbps */
    USB_SPEED_HIGH      /* 480 Mbps */
} usb_device_speed_t;

/* USB transfer types */
typedef enum {
    USB_TRANSFER_CONTROL,
    USB_TRANSFER_ISOCHRONOUS,
    USB_TRANSFER_BULK,
    USB_TRANSFER_INTERRUPT
} usb_transfer_type_t;

/* USB request types */
#define USB_REQ_GET_STATUS        0
#define USB_REQ_CLEAR_FEATURE     1
#define USB_REQ_SET_FEATURE       3
#define USB_REQ_SET_ADDRESS       5
#define USB_REQ_GET_DESCRIPTOR    6
#define USB_REQ_SET_DESCRIPTOR    7
#define USB_REQ_GET_CONFIGURATION 8
#define USB_REQ_SET_CONFIGURATION 9

/* USB descriptor types */
#define USB_DESC_DEVICE           1
#define USB_DESC_CONFIGURATION    2
#define USB_DESC_STRING           3
#define USB_DESC_INTERFACE        4
#define USB_DESC_ENDPOINT         5

/* USB classes */
#define USB_CLASS_HID             3
#define USB_CLASS_MASS_STORAGE    8
#define USB_CLASS_HUB             9

/* USB setup packet */
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_packet_t;

/* USB device descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

/* USB configuration descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

/* USB interface descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

/* USB endpoint descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

/* USB device structure */
typedef struct usb_device {
    uint8_t address;
    usb_device_state_t state;
    usb_device_speed_t speed;
    usb_device_descriptor_t descriptor;
    struct usb_device* next;
    void* driver_data;
} usb_device_t;

/* USB transfer structure */
typedef struct {
    usb_device_t* device;
    uint8_t endpoint;
    usb_transfer_type_t type;
    uint8_t* buffer;
    uint32_t length;
    uint32_t actual_length;
    int status;
} usb_transfer_t;

/* USB host controller operations */
typedef struct {
    int (*reset_port)(uint32_t port);
    int (*enable_port)(uint32_t port);
    int (*disable_port)(uint32_t port);
    int (*submit_transfer)(usb_transfer_t* transfer);
} usb_hc_ops_t;

/* USB subsystem initialization */
void usb_init(void);

/* Device enumeration */
int usb_enumerate_device(uint32_t port);
usb_device_t* usb_get_device(uint8_t address);

/* Descriptor functions */
int usb_get_device_descriptor(usb_device_t* device);
int usb_get_config_descriptor(usb_device_t* device, uint8_t config_num, uint8_t* buffer, uint32_t length);

/* Device control */
int usb_set_address(usb_device_t* device, uint8_t address);
int usb_set_configuration(usb_device_t* device, uint8_t config);

/* Transfer functions */
int usb_control_transfer(usb_device_t* device, usb_setup_packet_t* setup, uint8_t* data, uint32_t length);
int usb_bulk_transfer(usb_device_t* device, uint8_t endpoint, uint8_t* data, uint32_t length);
int usb_interrupt_transfer(usb_device_t* device, uint8_t endpoint, uint8_t* data, uint32_t length);

/* UHCI (USB 1.1) controller */
void uhci_init(void);
int uhci_detect_devices(void);

/* USB HID driver */
void usb_hid_init(void);
int usb_hid_attach(usb_device_t* device);

/* USB mass storage driver */
void usb_msd_init(void);
int usb_msd_attach(usb_device_t* device);

#endif /* AURORA_USB_H */
