# Aurora OS - Phase 2 & 3 Implementation Documentation

## Overview

This document describes the implementation of Phase 2 (Kernel Implementation) and Phase 3 (File System & I/O) for Aurora OS.

## Phase 2: Kernel Implementation

### Memory Management

The memory management subsystem provides physical and virtual memory allocation capabilities.

#### Features:
- **Page Frame Allocator**: Manages physical memory using a bitmap-based allocator
  - Supports 1024 frames (4MB physical memory)
  - Frame size: 4KB
  - Allocation tracking via bitmap

- **Heap Allocator**: Kernel heap management with first-fit allocation
  - Heap location: 0x00100000 (1MB)
  - Heap size: 1MB
  - Block-based allocation with metadata
  - Automatic block coalescing on free

- **Virtual Memory**: Basic virtual memory support
  - Page-aligned allocations
  - Integration with physical frame allocator

#### API:
```c
void memory_init(void);              // Initialize memory management
void* kmalloc(size_t size);          // Allocate kernel memory
void kfree(void* ptr);               // Free kernel memory
void* vm_alloc(size_t size, uint32_t flags);  // Allocate virtual memory
void vm_free(void* ptr);             // Free virtual memory
```

#### Implementation Details:
- Memory blocks contain size and free status
- Linked list of blocks for tracking
- 8-byte alignment for all allocations
- Kernel memory marked as used (0-1MB)

### Process Management

The process management subsystem handles process creation, scheduling, and lifecycle management.

#### Features:
- **Process Table**: Static array of 64 process slots
- **Process States**: READY, RUNNING, BLOCKED, TERMINATED
- **Round-Robin Scheduler**: Fair time-sharing scheduling
- **Stack Management**: 4KB stack per process
- **Idle Process**: System idle process for CPU halting

#### API:
```c
void process_init(void);                                    // Initialize process management
process_t* process_create(void (*entry)(void), uint32_t priority);  // Create process
void process_terminate(uint32_t pid);                       // Terminate process
void process_yield(void);                                   // Yield CPU
void scheduler_init(void);                                  // Initialize scheduler
void scheduler_schedule(void);                              // Schedule next process
```

#### Process Control Block (PCB):
```c
typedef struct process {
    uint32_t pid;              // Process ID
    process_state_t state;     // Current state
    uint32_t* stack_ptr;       // Stack pointer
    uint32_t priority;         // Priority level
    struct process* next;      // Next in queue
} process_t;
```

#### Implementation Details:
- Queue-based ready queue (FIFO)
- Process IDs assigned sequentially
- Context switching support (simplified)
- Automatic idle process creation

### Interrupt Handling

The interrupt subsystem provides interrupt management and system call interface.

#### Features:
- **IDT (Interrupt Descriptor Table)**: 256-entry IDT
- **Interrupt Handlers**: Customizable interrupt handler registration
- **System Call Interface**: INT 0x80 based syscalls

#### API:
```c
void interrupt_init(void);                                  // Initialize interrupts
void interrupt_enable(void);                                // Enable interrupts (STI)
void interrupt_disable(void);                               // Disable interrupts (CLI)
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);  // Register handler
void syscall_init(void);                                    // Initialize syscalls
int syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);  // Handle syscall
```

#### Supported System Calls:
- EXIT: Terminate process
- FORK: Create child process (stub)
- READ: Read from file descriptor (stub)
- WRITE: Write to file descriptor (stub)
- OPEN: Open file (stub)
- CLOSE: Close file descriptor (stub)
- WAIT: Wait for child process (stub)
- EXEC: Execute program (stub)
- YIELD: Yield CPU time

## Phase 3: File System & I/O

### Virtual File System (VFS)

The VFS layer provides a unified interface for file system operations.

#### Features:
- **File Descriptors**: 256 open files support
- **Mount System**: File system mounting at mount points
- **File Operations**: Open, close, read, write, seek
- **Directory Operations**: Create, remove, read directories

#### API:
```c
void vfs_init(void);                                        // Initialize VFS
int vfs_register_fs(const char* name, fs_ops_t* ops);      // Register filesystem
int vfs_mount(const char* device, const char* mountpoint, const char* fstype);  // Mount
int vfs_unmount(const char* mountpoint);                    // Unmount
int vfs_open(const char* path, int flags);                  // Open file
int vfs_close(int fd);                                      // Close file
int vfs_read(int fd, void* buffer, size_t size);           // Read
int vfs_write(int fd, const void* buffer, size_t size);    // Write
int vfs_seek(int fd, long offset, int whence);             // Seek
int vfs_mkdir(const char* path);                           // Create directory
int vfs_rmdir(const char* path);                           // Remove directory
int vfs_create(const char* path);                          // Create file
int vfs_unlink(const char* path);                          // Delete file
int vfs_stat(const char* path, inode_t* stat);             // Get file info
```

#### File Open Flags:
- O_RDONLY: Read-only
- O_WRONLY: Write-only
- O_RDWR: Read-write
- O_CREAT: Create if not exists
- O_APPEND: Append mode
- O_TRUNC: Truncate file

### Ramdisk File System

In-memory file system implementation for initial root filesystem.

#### Features:
- **Capacity**: 128 files, 2048 blocks
- **Block Size**: 512 bytes
- **File Operations**: Full read/write support
- **Directory Support**: Basic directory operations
- **File Name Storage**: 64-character file names

#### API:
```c
void ramdisk_init(void);                 // Initialize ramdisk
int ramdisk_create(size_t size);         // Create ramdisk
fs_ops_t* ramdisk_get_ops(void);        // Get filesystem operations
file_ops_t* ramdisk_get_file_ops(void); // Get file operations
```

#### Data Structures:
- **Superblock**: Magic number, block counts, inode counts
- **Inodes**: File metadata (size, type, block list)
- **Block Bitmap**: Track allocated blocks
- **File Table**: File name to inode mapping

#### Implementation Details:
- Direct block addressing (32 blocks per file)
- Block allocation on demand
- First-fit block allocation
- File name lookup via hash table

### Journaling Subsystem

Transaction-based journaling for file system consistency.

#### Features:
- **Transaction Management**: Begin, commit, abort transactions
- **Operation Types**: CREATE, DELETE, WRITE, METADATA
- **Crash Recovery**: Journal replay on mount
- **ACID Properties**: Atomicity, consistency, isolation, durability

#### API:
```c
void journal_init(void);                                    // Initialize journal
void journal_enable(void);                                  // Enable journaling
void journal_disable(void);                                 // Disable journaling
int journal_is_enabled(void);                              // Check if enabled
transaction_t* journal_begin_transaction(void);            // Begin transaction
int journal_add_operation(transaction_t* txn, journal_operation_t* op);  // Add operation
int journal_commit_transaction(transaction_t* txn);        // Commit transaction
int journal_abort_transaction(transaction_t* txn);         // Abort transaction
int journal_recover(void);                                 // Recover from journal
int journal_checkpoint(void);                              // Checkpoint journal
```

#### Helper Functions:
```c
journal_operation_t journal_create_write_op(uint32_t block_num, void* old_data, void* new_data, size_t size);
journal_operation_t journal_create_metadata_op(uint32_t block_num, void* old_data, void* new_data, size_t size);
journal_operation_t journal_create_create_op(uint32_t block_num);
journal_operation_t journal_create_delete_op(uint32_t block_num);
```

#### Transaction States:
- PENDING: Transaction in progress
- COMMITTED: Transaction committed but not yet applied
- COMPLETED: Transaction fully applied
- ABORTED: Transaction rolled back

#### Implementation Details:
- In-memory journal (256 transactions)
- 64 operations per transaction
- Automatic memory management for operation data
- Transaction ID tracking

## Testing

A comprehensive test suite validates all implementations.

### Test Categories:

1. **Memory Management Tests**
   - Single allocation/deallocation
   - Multiple allocations
   - Virtual memory allocation

2. **Process Management Tests**
   - Process creation
   - Process termination
   - Multiple process creation

3. **File System Tests**
   - File creation
   - File opening/closing
   - Directory creation
   - File deletion

4. **Journaling Tests**
   - Transaction creation
   - Operation addition
   - Transaction commit
   - Transaction abort

### Running Tests:

Tests are automatically run during kernel initialization. Output is displayed via VGA text mode.

## Build System

The build system compiles all components into a single kernel binary.

### Build Commands:
```bash
make all    # Build the kernel
make clean  # Clean build artifacts
make help   # Show help
```

### Build Output:
- Kernel binary: `build/aurora-kernel.bin`
- Object files in `build/` directory tree

### Compilation Flags:
- `-Wall -Wextra`: Enable all warnings
- `-nostdlib`: No standard library
- `-ffreestanding`: Freestanding environment
- `-m32`: 32-bit compilation
- `-fno-pie`: Disable PIE

## Integration

All subsystems are integrated in kernel initialization:

```c
void kernel_init(void) {
    drivers_init();           // Device drivers
    interrupt_init();         // Interrupt handling
    memory_init();           // Memory management
    vfs_init();              // Virtual file system
    journal_init();          // Journaling
    ramdisk_init();          // Ramdisk
    ramdisk_create(1MB);     // Create 1MB ramdisk
    vfs_register_fs(...);    // Register ramdisk
    vfs_mount(...);          // Mount at root
    process_init();          // Process management
    scheduler_init();        // Scheduler
    run_tests();             // Run test suite
}
```

## Future Enhancements

### Short-term:
- Implement actual context switching with register saving/restoring
- Add demand paging for virtual memory
- Implement proper interrupt handlers for hardware
- Add more system calls
- Enhance error handling

### Long-term:
- Multi-core support (SMP)
- Network stack
- USB support
- Additional file systems (ext2/3/4, FAT32)
- GUI framework
- User space applications

## Conclusion

Phases 2 and 3 provide a solid foundation for Aurora OS with:
- Functional memory management
- Process scheduling and management
- Interrupt handling and system calls
- Complete VFS layer
- Working ramdisk file system
- Transaction journaling
- Comprehensive test suite

This implementation establishes the core operating system functionality needed for building higher-level features.
