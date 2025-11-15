# Aurora OS - Advanced Features Documentation

## Overview

This document describes the advanced features implemented in Aurora OS, including multi-core support, demand paging, network stack, and USB support.

---

## 1. Multi-Core Support (SMP)

### Description

Aurora OS includes Symmetric Multiprocessing (SMP) support, allowing the kernel to utilize multiple CPU cores efficiently.

### Components

#### SMP Initialization (`kernel/smp/`)
- **CPU Detection**: Automatically detects available CPU cores
- **APIC Support**: Advanced Programmable Interrupt Controller for inter-processor communication
- **Per-CPU Data**: Each CPU maintains its own data structures
- **Spinlocks**: Synchronization primitives for multi-core environments

### Key Features

1. **APIC Management**
   - Initialize Local APIC on each CPU
   - Send Inter-Processor Interrupts (IPI)
   - Handle End-Of-Interrupt (EOI) signals

2. **Per-CPU Data Structures**
   ```c
   typedef struct {
       uint32_t cpu_id;
       cpu_state_t state;
       uint32_t apic_id;
       void* stack_ptr;
       void* current_process;
       uint32_t ticks;
   } cpu_info_t;
   ```

3. **Spinlock Implementation**
   - Lock acquisition with atomic operations
   - Optimized spinning with pause instruction
   - Lock release with memory barriers

### Usage

```c
// Initialize SMP subsystem
smp_init();

// Get CPU information
uint32_t cpu_count = smp_get_cpu_count();
uint32_t current_cpu = smp_get_current_cpu_id();
cpu_info_t* info = smp_get_cpu_info(0);

// Use spinlocks
spinlock_t lock;
spinlock_init(&lock);
spinlock_acquire(&lock);
// Critical section
spinlock_release(&lock);
```

---

## 2. Advanced Memory Management (Demand Paging)

### Description

Aurora OS implements demand paging, allowing efficient memory usage through virtual memory, page swapping, and copy-on-write.

### Components

#### Paging Subsystem (`kernel/memory/paging.c`)
- **Page Tables**: Hierarchical page directory and page table structure
- **Page Mapping**: Map virtual addresses to physical addresses
- **Page Fault Handler**: Handle page faults and load pages on demand
- **Copy-on-Write**: Share pages between processes until modification
- **Page Swapping**: Move pages between RAM and disk storage

### Key Features

1. **Page Directory Management**
   ```c
   page_directory_t* dir = paging_create_directory();
   paging_switch_directory(dir);
   ```

2. **Page Mapping**
   ```c
   paging_map_page(dir, virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
   paging_unmap_page(dir, virt_addr);
   uint32_t phys = paging_get_physical_address(dir, virt_addr);
   ```

3. **Copy-on-Write (COW)**
   ```c
   paging_mark_cow(dir, virt_addr);  // Mark page as COW
   // On write fault, page is automatically copied
   ```

4. **Page Swapping**
   ```c
   page_swap_out(virt_addr);  // Swap page to storage
   page_swap_in(virt_addr);   // Load page from storage
   ```

### Page Fault Handling

The page fault handler automatically:
- Loads pages from swap storage when needed
- Handles COW faults by copying pages
- Updates page tables and TLB

---

## 3. Network Stack

### Description

Aurora OS includes a complete TCP/IP network stack with support for Ethernet, ARP, IP, ICMP, UDP, and TCP protocols.

### Components

#### Network Subsystem (`kernel/network/`)
- **Ethernet Layer**: Frame handling and device abstraction
- **ARP Protocol**: Address resolution between IP and MAC addresses
- **IP Layer**: Internet Protocol with routing and fragmentation
- **ICMP**: Internet Control Message Protocol (ping, etc.)
- **UDP**: User Datagram Protocol for connectionless communication
- **TCP**: Transmission Control Protocol for reliable connections
- **Socket Interface**: BSD-style socket API

### Architecture

```
+-------------------+
|   Applications    |
+-------------------+
|  Socket Interface |
+-------------------+
|  TCP  |  UDP      |
+-------------------+
|      IP/ICMP      |
+-------------------+
|    ARP | Ethernet |
+-------------------+
|  Network Device   |
+-------------------+
```

### Key Features

1. **Socket Interface**
   ```c
   socket_t* sock = socket_create(PROTO_UDP);
   socket_bind(sock, 8080);
   socket_connect(sock, remote_ip, remote_port);
   socket_send(sock, data, length);
   socket_receive(sock, buffer, max_length);
   socket_close(sock);
   ```

2. **ARP Cache**
   ```c
   arp_send_request(iface, target_ip);
   uint8_t* mac = arp_lookup(ip_addr);
   ```

3. **IP Packet Handling**
   ```c
   ip_send_packet(iface, dest_ip, protocol, payload, length);
   ```

4. **ICMP Echo (Ping)**
   ```c
   icmp_send_echo(iface, dest_ip, id, sequence);
   ```

### Protocols Implemented

- **Ethernet**: Frame encapsulation and MAC addressing
- **ARP**: IP to MAC address resolution with caching
- **IP**: IPv4 with checksum validation
- **ICMP**: Echo request/reply for network diagnostics
- **UDP**: Connectionless datagram transmission
- **TCP**: Connection-oriented reliable transmission (basic)

---

## 4. USB Support

### Description

Aurora OS provides USB support with UHCI controller driver, device enumeration, and class-specific drivers for HID and mass storage devices.

### Components

#### USB Subsystem (`kernel/usb/`)
- **USB Core**: Device management and transfer handling
- **UHCI Driver**: USB 1.1 host controller driver
- **Device Enumeration**: Automatic detection and configuration
- **HID Driver**: Support for keyboards and mice
- **Mass Storage Driver**: Support for USB storage devices

### Architecture

```
+-------------------+
|  Class Drivers    |
|  (HID, Storage)   |
+-------------------+
|    USB Core       |
+-------------------+
|  Host Controller  |
|     (UHCI)        |
+-------------------+
|   USB Hardware    |
+-------------------+
```

### Key Features

1. **Device Enumeration**
   ```c
   usb_enumerate_device(port);
   usb_device_t* device = usb_get_device(address);
   ```

2. **Descriptor Handling**
   ```c
   usb_get_device_descriptor(device);
   usb_get_config_descriptor(device, config_num, buffer, length);
   ```

3. **Device Configuration**
   ```c
   usb_set_address(device, new_address);
   usb_set_configuration(device, config);
   ```

4. **Data Transfers**
   ```c
   usb_control_transfer(device, &setup, data, length);
   usb_bulk_transfer(device, endpoint, data, length);
   usb_interrupt_transfer(device, endpoint, data, length);
   ```

### Device Classes Supported

- **HID (Human Interface Devices)**: Keyboards, mice, game controllers
- **Mass Storage**: USB flash drives, external hard drives
- **Hub**: USB hubs for port expansion (basic)

### USB Standards

- **USB 1.1**: Full speed (12 Mbps) and low speed (1.5 Mbps)
- **UHCI**: Universal Host Controller Interface
- **Descriptors**: Device, configuration, interface, endpoint

---

## Integration

All four subsystems are integrated into the kernel initialization sequence:

```c
void kernel_init(void) {
    // ... other initialization ...
    
    paging_init();          // Initialize demand paging
    smp_init();            // Initialize multi-core support
    network_init();        // Initialize network stack
    usb_init();            // Initialize USB subsystem
    
    // ... rest of initialization ...
}
```

---

## Testing

The advanced test suite (`tests/advanced_tests.c`) validates:

1. **SMP Tests**
   - CPU detection
   - Per-CPU data structures
   - Spinlock operations

2. **Paging Tests**
   - Page directory management
   - Page mapping/unmapping
   - COW marking
   - Physical address translation

3. **Network Tests**
   - Socket creation and binding
   - Connection establishment
   - ARP cache operations

4. **USB Tests**
   - Device lookup
   - Subsystem initialization

To run tests:
```bash
make test
```

---

## Future Enhancements

### Multi-Core
- [ ] Application Processor (AP) startup
- [ ] Per-CPU scheduler queues
- [ ] Load balancing across CPUs

### Paging
- [ ] Page replacement algorithms (LRU, Clock)
- [ ] Disk-backed swap storage
- [ ] Transparent huge pages

### Network
- [ ] Full TCP state machine
- [ ] IPv6 support
- [ ] Network device drivers (e1000, RTL8139)

### USB
- [ ] USB 2.0/3.0 support
- [ ] EHCI/XHCI controller drivers
- [ ] Additional device classes

---

## References

- Intel® 64 and IA-32 Architectures Software Developer's Manual
- USB Specification 2.0
- RFC 791 - Internet Protocol
- RFC 793 - Transmission Control Protocol
- RFC 826 - Address Resolution Protocol

---

*Document Version: 1.0*  
*Last Updated: November 2025*  
*Aurora OS Project*
