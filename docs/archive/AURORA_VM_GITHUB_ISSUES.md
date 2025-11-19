# Aurora VM - GitHub Issues Template

This document contains GitHub issue templates for the problems discovered during Aurora OS testing on Aurora VM. Copy each issue below into a new GitHub issue for tracking and resolution.

---

## Issue #1: Memory allocation syscall returns incorrect values

**Labels:** `bug`, `high-priority`, `memory-management`, `aurora-vm`

**Milestone:** Aurora VM v2.1

### Description

The `AURORA_SYSCALL_ALLOC` syscall does not properly allocate memory. After allocating memory and writing a value, reading it back returns incorrect data.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

uint32_t program[] = {
    aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_ALLOC),
    aurora_encode_i_type(AURORA_OP_LOADI, 1, 1024),  // Allocate 1KB
    aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
    aurora_encode_r_type(AURORA_OP_MOVE, 2, 0, 0),   // Save address in r2
    
    // Write value 0x1234
    aurora_encode_i_type(AURORA_OP_LOADI, 3, 0x1234),
    aurora_encode_r_type(AURORA_OP_STORE, 2, 3, 0),
    
    // Read back
    aurora_encode_r_type(AURORA_OP_LOAD, 4, 2, 0),
    
    aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
};

aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
aurora_vm_run(vm);

uint32_t written = 0x1234;
uint32_t readback = aurora_vm_get_register(vm, 4);
// Expected: readback == 0x1234
// Actual: readback != 0x1234
```

### Expected Behavior

After allocating memory with ALLOC, writing a value, and reading it back, the value should match what was written.

### Actual Behavior

The read value does not match the written value, indicating memory allocation or access is broken.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64
- Compiler: GCC 9.4.0
- Test: Aurora OS VM Integration Test Suite

### Impact

- Prevents dynamic memory allocation in Aurora OS
- Breaks heap-based data structures
- Critical for process management and applications

### Proposed Solution

1. Verify `aurora_vm_syscall_alloc()` implementation in `src/platform/aurora_vm.c`
2. Ensure allocated addresses are in heap range (0x4000-0xBFFF)
3. Check that pages are marked as writable
4. Validate STORE/LOAD instructions work with allocated memory
5. Add unit test specifically for ALLOC/STORE/LOAD sequence

### Additional Context

- Core VM unit tests pass (29/29)
- Extension tests pass (46/46)
- Integration test reveals this issue when combining syscalls with memory operations

### Related Issues

- Potentially related to Issue #4 (MMIO write operations)

---

## Issue #2: Interrupts not marked as pending after trigger

**Labels:** `bug`, `high-priority`, `interrupt-system`, `aurora-vm`

**Milestone:** Aurora VM v2.1

### Description

The interrupt controller does not properly mark interrupts as pending when `aurora_vm_irq_trigger()` is called. This breaks the entire interrupt handling system.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

// Enable interrupts
aurora_vm_irq_enable(vm, true);

// Set handler for IRQ 0
aurora_vm_irq_set_handler(vm, 0, 0x2000);

// Trigger interrupt
aurora_vm_irq_trigger(vm, 0);

// Check if pending
bool pending = vm->irq_ctrl.interrupts[0].pending;
// Expected: pending == true
// Actual: pending == false
```

### Expected Behavior

After calling `aurora_vm_irq_trigger()`, the corresponding interrupt should be marked as pending in the IRQ controller and should be dispatched on the next VM step.

### Actual Behavior

The interrupt is not marked as pending, and the handler is never invoked.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64
- Test: Aurora OS VM Integration Test Suite

### Impact

**Critical - Breaks all interrupt-driven I/O:**
- Timer interrupts don't fire (affects scheduling)
- Keyboard/mouse interrupts don't work (affects GUI input)
- Network interrupts don't trigger (affects networking)
- Prevents preemptive multitasking

### Proposed Solution

1. Review `aurora_vm_irq_trigger()` in `src/platform/aurora_vm.c`
2. Ensure it sets `vm->irq_ctrl.interrupts[irq].pending = true`
3. Verify VM execution loop in `aurora_vm_run()` checks for pending interrupts
4. Implement interrupt dispatch logic:
   ```c
   if (vm->irq_ctrl.enabled && has_pending_interrupt(vm)) {
       uint32_t irq = get_highest_priority_interrupt(vm);
       save_context(vm);
       vm->cpu.pc = vm->irq_ctrl.interrupts[irq].handler;
       vm->irq_ctrl.interrupts[irq].pending = false;
   }
   ```
5. Add integration test for interrupt handling

### Additional Context

- Interrupt data structures are properly defined
- Handler registration works correctly
- Only the trigger and dispatch mechanism is broken

### References

- See `include/platform/aurora_vm.h` lines defining `aurora_irq_ctrl_t`
- See `docs/AURORA_VM.md` section on "Interrupt Support"

---

## Issue #3: File operations syscalls return invalid file descriptors

**Labels:** `enhancement`, `file-system`, `aurora-vm`, `good-first-issue`

**Milestone:** Aurora VM v2.2

### Description

File operation syscalls (OPEN, CLOSE, READ_FILE, WRITE_FILE) are currently stubbed and return invalid file descriptors. This prevents testing Aurora OS file system components on the VM.

### Current Status

This is a **known limitation** documented in `docs/AURORA_VM.md`:
> "Limited I/O: File operations are stubs (not fully implemented)"

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

// Write path to memory
const char *path = "/test/file.txt";
aurora_vm_write_memory(vm, 0x5000, strlen(path) + 1, path);

uint32_t program[] = {
    aurora_encode_i_type(AURORA_OP_LOADI, 0, AURORA_SYSCALL_OPEN),
    aurora_encode_i_type(AURORA_OP_LOADI, 1, 0x5000),  // Path address
    aurora_encode_i_type(AURORA_OP_LOADI, 2, 2),       // Mode (read/write)
    aurora_encode_r_type(AURORA_OP_SYSCALL, 0, 0, 0),
    aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
};

aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
aurora_vm_run(vm);

uint32_t fd = aurora_vm_get_register(vm, 0);
// Expected: fd > 0 (valid file descriptor)
// Actual: fd == 0 or fd == -1 (invalid)
```

### Expected Behavior

File operations should work with a basic in-memory file system:
- OPEN returns valid file descriptor
- WRITE_FILE writes data to file
- READ_FILE reads data from file
- CLOSE releases file descriptor

### Actual Behavior

All file operations return errors or invalid values because they are not implemented.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64

### Impact

**Medium - Prevents file system testing:**
- Cannot test Aurora OS VFS layer
- Cannot test ramdisk functionality
- Cannot test journaling file system
- Applications cannot persist data

### Proposed Implementation

1. **File Descriptor Table** (8-16 entries)
   ```c
   typedef struct {
       char path[256];
       uint32_t offset;
       uint32_t size;
       uint32_t data_addr;  // In VM storage
       bool open;
   } aurora_file_t;
   ```

2. **File Storage** (Use VM storage device - 1MB available)
   - Store files in `vm->storage` memory region
   - Simple allocation scheme (e.g., 64KB per file max)

3. **Syscall Implementation**
   - OPEN: Find free fd, initialize file structure
   - CLOSE: Mark fd as free
   - READ_FILE: Copy from storage to VM memory
   - WRITE_FILE: Copy from VM memory to storage

4. **Initial File System**
   - 16 files maximum
   - 64KB max file size
   - Simple linear allocation

### Workaround

Test file system functionality separately from VM using direct memory access or QEMU testing.

### Priority

Medium - This is a known limitation and enhances testing capability but is not critical for basic VM operation.

---

## Issue #4: MMIO write operations fail with memory access error

**Labels:** `bug`, `low-priority`, `mmio`, `aurora-vm`

**Milestone:** Aurora VM v2.2

### Description

While MMIO regions are properly defined (0xC000-0xDFFF), writing to these addresses via `aurora_vm_write_memory()` returns an error code. The addresses are correct but device handlers are not implemented.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

uint32_t test_value = 0x12345678;
int result = aurora_vm_write_memory(vm, AURORA_VM_MMIO_DISPLAY, 4, &test_value);
// Expected: result == 0 (success)
// Actual: result != 0 (error)
```

### Expected Behavior

MMIO writes should either:
1. Succeed and forward to device handlers, OR
2. Be documented as syscall-only (not direct memory access)

### Actual Behavior

MMIO writes fail with error code, preventing direct device manipulation.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64

### Impact

**Low - Workaround available:**
- Cannot test memory-mapped device access patterns
- Must use syscall APIs instead (which work fine)
- Not critical since device APIs are functional

### Proposed Solution

**Option 1: Implement MMIO Handlers**
```c
// In aurora_vm_write_memory()
if (addr >= AURORA_VM_MMIO_BASE && addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE) {
    return aurora_vm_mmio_write(vm, addr, size, buffer);
}

// Add MMIO dispatch
int aurora_vm_mmio_write(AuroraVM *vm, uint32_t addr, size_t size, const void *buffer) {
    if (addr >= AURORA_VM_MMIO_DISPLAY && addr < AURORA_VM_MMIO_KEYBOARD) {
        return aurora_vm_display_mmio_write(vm, addr, size, buffer);
    }
    // ... other devices
    return -1;
}
```

**Option 2: Document as Syscall-Only**

Update documentation to clarify MMIO is for address space layout only, and devices must be accessed via syscalls.

### Additional Context

- MMIO addresses are correctly defined
- Device APIs (syscalls) work correctly
- Only direct memory access to MMIO fails

### Priority

Low - Syscall-based device access works fine, MMIO is a nice-to-have feature.

---

## Issue #5: Network packet send returns error code

**Labels:** `bug`, `medium-priority`, `network`, `aurora-vm`

**Milestone:** Aurora VM v2.1

### Description

The `aurora_vm_net_send()` function returns an error code even when the network device is connected. Packet is not added to TX queue.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

// Connect network
vm->network.connected = true;

// Send packet
uint8_t data[] = "Hello, Network!";
int result = aurora_vm_net_send(vm, data, sizeof(data));
// Expected: result == 0 (success)
// Actual: result != 0 (error)

// Check TX queue
// Expected: vm->network.tx_tail > 0
// Actual: vm->network.tx_tail == 0
```

### Expected Behavior

When network is connected, sending a packet should:
1. Return success (0)
2. Add packet to TX queue
3. Increment TX queue tail pointer

### Actual Behavior

Send operation fails and packet is not queued.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64

### Impact

**Medium - Blocks network testing:**
- Cannot test network stack
- Cannot test packet transmission
- NET_SEND syscall may also be affected

### Proposed Solution

1. Review `aurora_vm_net_send()` implementation
2. Check for initialization requirements
3. Verify queue full detection
4. Ensure packet is copied to TX queue
5. Test with unit test before integration test

### Additional Context

- Network device structure is defined correctly
- TX/RX queues allocated (64 packets each)
- Connection state tracking exists

---

## Issue #6: Atomic operations program fails to execute

**Labels:** `bug`, `medium-priority`, `atomic-ops`, `aurora-vm`

**Milestone:** Aurora VM v2.1

### Description

A program using atomic operations (XCHG, CAS, FADD_ATOMIC) fails to run to completion. Individual atomic operation tests pass, but combined program execution fails.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

uint32_t program[] = {
    // XCHG test
    aurora_encode_i_type(AURORA_OP_LOADI, 1, 42),
    aurora_encode_i_type(AURORA_OP_LOADI, 2, 100),
    aurora_encode_r_type(AURORA_OP_XCHG, 3, 1, 2),
    
    // CAS test
    aurora_encode_i_type(AURORA_OP_LOADI, 4, 100),
    aurora_encode_i_type(AURORA_OP_LOADI, 5, 200),
    aurora_encode_r_type(AURORA_OP_CAS, 6, 1, 4),
    
    // FADD test
    aurora_encode_i_type(AURORA_OP_LOADI, 7, 10),
    aurora_encode_i_type(AURORA_OP_LOADI, 8, 5),
    aurora_encode_r_type(AURORA_OP_FADD_ATOMIC, 9, 7, 8),
    
    aurora_encode_r_type(AURORA_OP_HALT, 0, 0, 0),
};

aurora_vm_load_program(vm, (uint8_t *)program, sizeof(program), 0);
int result = aurora_vm_run(vm);
// Expected: result == 0
// Actual: result != 0
```

### Expected Behavior

Program should execute successfully with result == 0.

### Actual Behavior

Program execution fails with non-zero result code.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64

### Impact

**Medium - Affects multi-threading:**
- Atomic operations needed for thread synchronization
- Prevents testing concurrent data structures
- May indicate instruction encoding issues

### Proposed Solution

1. Run with debugger enabled to see where execution fails
2. Test each atomic operation individually
3. Check instruction encoding for multi-instruction programs
4. Verify register state between operations
5. Add detailed logging to atomic operation handlers

### Additional Context

- Extension tests show individual atomic ops work
- May be related to instruction sequencing or PC advancement
- Worth checking if HALT instruction is reached

---

## Issue #7: JIT cache allocation returns NULL

**Labels:** `enhancement`, `jit`, `aurora-vm`

**Milestone:** Aurora VM v2.2

### Description

Despite JIT being enabled by default, the `vm->jit.cache` pointer is NULL, indicating the cache was not allocated.

### Steps to Reproduce

```c
AuroraVM *vm = aurora_vm_create();
aurora_vm_init(vm);

// Expected: vm->jit.enabled == true
// Expected: vm->jit.cache != NULL
// Actual: vm->jit.cache == NULL
```

### Expected Behavior

If JIT is enabled, cache should be allocated (256KB).

### Actual Behavior

JIT cache pointer is NULL.

### Environment

- Aurora VM v2.0
- Platform: Linux x86_64

### Impact

**Low - JIT is infrastructure only:**
- JIT code generation not implemented yet
- Documented as "infrastructure for future implementation"
- Does not affect core VM operation

### Proposed Solution

**Option 1: Fix Allocation**
```c
// In aurora_vm_init()
if (vm->jit.enabled) {
    vm->jit.cache = malloc(AURORA_VM_JIT_CACHE_SIZE);
    if (!vm->jit.cache) {
        vm->jit.enabled = false;
    }
}
```

**Option 2: Update Documentation**

Clarify that JIT is planned but cache allocation is not yet implemented.

### Additional Context

- JIT compilation infrastructure exists
- Code generation backend not implemented
- This is a minor inconsistency

### Priority

Low - Doesn't affect functionality, mostly documentation issue.

---

## Summary Statistics

**Total Issues:** 7

**By Priority:**
- Critical: 0
- High: 2 (Issues #1, #2)
- Medium: 3 (Issues #3, #5, #6)
- Low: 2 (Issues #4, #7)

**By Component:**
- Memory Management: 1
- Interrupt System: 1
- File System: 1
- MMIO: 1
- Network: 1
- Atomic Operations: 1
- JIT: 1

**Recommended Fix Order:**
1. Issue #1 (Memory allocation)
2. Issue #2 (Interrupt handling)
3. Issue #5 (Network send)
4. Issue #6 (Atomic operations)
5. Issue #3 (File system - enhancement)
6. Issue #4 (MMIO - low priority)
7. Issue #7 (JIT cache - documentation)

---

**Generated:** November 16, 2025  
**Test Report:** See `AURORA_VM_TEST_ISSUES.md` for detailed analysis  
**Test Suite:** `tests/aurora_os_vm_integration_test.c`
