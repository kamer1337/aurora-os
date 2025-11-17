# Aurora OS - Network Device API Documentation

**Version**: 1.0  
**Status**: Validated  
**Last Updated**: November 17, 2025

## Overview

The Aurora OS Network Device provides packet-based networking capabilities through a simple and efficient API. The network subsystem supports connection management, packet transmission, and reception with built-in queue management.

## Architecture

### Network Device Structure

```c
typedef struct {
    bool connected;                    // Connection state
    uint32_t tx_head, tx_tail;        // TX queue pointers
    uint32_t rx_head, rx_tail;        // RX queue pointers
    aurora_packet_t tx_queue[64];     // Transmit queue (64 packets)
    aurora_packet_t rx_queue[64];     // Receive queue (64 packets)
} aurora_network_t;
```

### Packet Structure

```c
typedef struct {
    uint8_t data[1500];               // Packet data (MTU = 1500 bytes)
    uint32_t length;                  // Actual packet length
} aurora_packet_t;
```

## API Functions

### Connection Management

#### `aurora_vm_net_is_connected()`
**Description**: Check if the network device is connected.

**Prototype**:
```c
bool aurora_vm_net_is_connected(const AuroraVM *vm);
```

**Parameters**:
- `vm`: Pointer to the Aurora VM instance

**Returns**: 
- `true` if connected
- `false` if disconnected

**Example**:
```c
if (aurora_vm_net_is_connected(vm)) {
    // Network is available
}
```

---

### Packet Transmission

#### `aurora_vm_net_send()`
**Description**: Send a packet through the network device.

**Prototype**:
```c
int aurora_vm_net_send(AuroraVM *vm, const uint8_t *data, uint32_t length);
```

**Parameters**:
- `vm`: Pointer to the Aurora VM instance
- `data`: Pointer to packet data buffer
- `length`: Length of packet data (max 1500 bytes)

**Returns**:
- Positive value: Number of bytes queued for transmission
- `-1`: Error (not connected, queue full, or invalid parameters)

**Error Conditions**:
- Network not connected
- TX queue full (64 packets)
- Length exceeds MTU (1500 bytes)
- Invalid parameters (NULL pointers)

**Example**:
```c
const char *message = "Hello, Network!";
int result = aurora_vm_net_send(vm, (const uint8_t *)message, strlen(message) + 1);
if (result > 0) {
    printf("Sent %d bytes\n", result);
} else {
    printf("Send failed\n");
}
```

**Network Interrupt**: Sending a packet triggers a network interrupt (`AURORA_VM_IRQ_NETWORK`) if interrupts are enabled.

---

### Packet Reception

#### `aurora_vm_net_recv()`
**Description**: Receive a packet from the network device.

**Prototype**:
```c
int aurora_vm_net_recv(AuroraVM *vm, uint8_t *buffer, uint32_t max_length);
```

**Parameters**:
- `vm`: Pointer to the Aurora VM instance
- `buffer`: Destination buffer for received data
- `max_length`: Maximum bytes to read into buffer

**Returns**:
- Positive value: Number of bytes received
- `0`: No packets available (RX queue empty)
- `-1`: Error (invalid parameters)

**Example**:
```c
uint8_t buffer[1500];
int received = aurora_vm_net_recv(vm, buffer, sizeof(buffer));
if (received > 0) {
    printf("Received %d bytes\n", received);
    // Process packet data in buffer
} else if (received == 0) {
    printf("No packets available\n");
}
```

---

## Syscall Interface

The network device can be accessed from Aurora OS programs using syscalls:

### NET_SEND Syscall

**Syscall Number**: `AURORA_SYSCALL_NET_SEND`

**Parameters**:
- `r0`: Syscall number (NET_SEND)
- `r1`: Memory address of packet data
- `r2`: Packet length

**Return Value** (in `r0`):
- Positive: Bytes sent
- `-1`: Error

**Assembly Example**:
```asm
LOADI r0, AURORA_SYSCALL_NET_SEND
LOADI r1, packet_addr              ; Address of packet data
LOADI r2, packet_len               ; Length of packet
SYSCALL
; r0 now contains result
```

### NET_RECV Syscall

**Syscall Number**: `AURORA_SYSCALL_NET_RECV`

**Parameters**:
- `r0`: Syscall number (NET_RECV)
- `r1`: Memory address for received data
- `r2`: Maximum bytes to receive

**Return Value** (in `r0`):
- Positive: Bytes received
- `0`: No packets available
- `-1`: Error

---

## Queue Management

### Transmit Queue
- **Capacity**: 64 packets
- **Behavior**: Circular buffer (ring queue)
- **Full Condition**: When `(tx_head + 1) % 64 == tx_tail`
- **Policy**: Send fails when queue is full

### Receive Queue
- **Capacity**: 64 packets
- **Behavior**: Circular buffer (ring queue)
- **Empty Condition**: When `rx_head == rx_tail`
- **Policy**: Receive returns 0 when queue is empty

---

## Connection States

### Connected State
- All send/receive operations allowed
- Packets can be queued and transmitted
- Interrupts are triggered on packet events

### Disconnected State
- Send operations fail immediately
- Receive operations return empty
- No interrupts are generated

---

## Best Practices

### 1. Check Connection Before Sending
```c
if (aurora_vm_net_is_connected(vm)) {
    aurora_vm_net_send(vm, data, length);
}
```

### 2. Handle Queue Full Conditions
```c
int result = aurora_vm_net_send(vm, data, length);
if (result == -1) {
    // Queue full or error - implement retry or backoff
    usleep(1000);  // Wait 1ms
    result = aurora_vm_net_send(vm, data, length);
}
```

### 3. Poll for Received Packets
```c
while (true) {
    int received = aurora_vm_net_recv(vm, buffer, sizeof(buffer));
    if (received > 0) {
        process_packet(buffer, received);
    } else {
        break;  // No more packets
    }
}
```

### 4. Use Network Interrupts
```c
// Set up network interrupt handler
aurora_vm_irq_set_handler(vm, AURORA_VM_IRQ_NETWORK, network_handler_addr);
aurora_vm_irq_enable(vm, true);

// Handler will be called when packets arrive or are sent
```

---

## Performance Characteristics

- **Throughput**: Limited by queue depth (64 packets × 1500 bytes = 96 KB buffer)
- **Latency**: Zero-copy within VM memory space
- **Overhead**: Minimal - direct memory operations

---

## Testing

A comprehensive test suite is available in `tests/roadmap_priority_tests.c`:

- **test_network_packet_queue()**: Validates queue management with multiple packets
- **test_network_packet_validation()**: Tests data integrity of sent/received packets
- **test_network_syscalls()**: Validates syscall interface
- **test_network_connection_state()**: Tests connection state management

All tests pass with 100% success rate.

---

## Future Enhancements

Planned improvements for future versions:

- TCP/IP protocol stack implementation
- Socket-based API for applications
- Network statistics and monitoring
- Quality of Service (QoS) support
- Hardware offload capabilities

---

**Document Owner**: Network Stack Team  
**Status**: Production Ready  
**Validation**: All tests passing
