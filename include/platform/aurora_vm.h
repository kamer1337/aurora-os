/**
 * @file aurora_vm.h
 * @brief Aurora OS Virtual Machine - Complete 32-bit RISC VM for testing
 * 
 * A standalone virtual machine for testing Aurora OS applications with:
 * - 32-bit RISC CPU with 33 opcodes
 * - 16 general purpose registers + PC/SP/FP/flags
 * - 64KB address space with 256-byte pages
 * - Virtual devices (display, keyboard, mouse, timer, storage)
 * - 12 system calls
 * - Integrated debugger
 */

#ifndef AURORA_VM_H
#define AURORA_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== VM Configuration ===== */
#define AURORA_VM_MEMORY_SIZE       (64 * 1024)     /* 64KB address space */
#define AURORA_VM_PAGE_SIZE         256              /* 256-byte pages */
#define AURORA_VM_NUM_PAGES         (AURORA_VM_MEMORY_SIZE / AURORA_VM_PAGE_SIZE)
#define AURORA_VM_NUM_REGISTERS     16               /* General purpose registers */
#define AURORA_VM_STACK_SIZE        (8 * 1024)       /* 8KB stack */
#define AURORA_VM_HEAP_SIZE         (32 * 1024)      /* 32KB heap */

/* Display configuration */
#define AURORA_VM_DISPLAY_WIDTH     320
#define AURORA_VM_DISPLAY_HEIGHT    240
#define AURORA_VM_DISPLAY_PIXELS    (AURORA_VM_DISPLAY_WIDTH * AURORA_VM_DISPLAY_HEIGHT)

/* Storage configuration */
#define AURORA_VM_STORAGE_SIZE      (1024 * 1024)    /* 1MB storage */

/* Keyboard configuration */
#define AURORA_VM_NUM_KEYS          256              /* 256 keys */

/* Timer configuration */
#define AURORA_VM_TIMER_FREQ        1000000          /* 1MHz timer */

/* Debugger configuration */
#define AURORA_VM_MAX_BREAKPOINTS   16

/* JIT configuration */
#define AURORA_VM_JIT_ENABLED       1                    /* Enable JIT compilation */
#define AURORA_VM_JIT_CACHE_SIZE    (256 * 1024)         /* 256KB JIT cache */
#define AURORA_VM_JIT_THRESHOLD     10                   /* Compile after 10 executions */

/* Interrupt configuration */
#define AURORA_VM_MAX_INTERRUPTS    32                   /* 32 interrupt vectors */
#define AURORA_VM_IRQ_TIMER         0                    /* Timer interrupt */
#define AURORA_VM_IRQ_KEYBOARD      1                    /* Keyboard interrupt */
#define AURORA_VM_IRQ_NETWORK       2                    /* Network interrupt */

/* Multi-threading configuration */
#define AURORA_VM_MAX_THREADS       8                    /* Max 8 threads */
#define AURORA_VM_THREAD_STACK_SIZE (4 * 1024)           /* 4KB per thread stack */

/* Network configuration */
#define AURORA_VM_NET_MTU           1500                 /* Network MTU */
#define AURORA_VM_NET_QUEUE_SIZE    64                   /* Packet queue size */

/* File system configuration */
#define AURORA_VM_MAX_FILES         16                   /* Max 16 open files */
#define AURORA_VM_MAX_FILENAME      256                  /* Max filename length */
#define AURORA_VM_MAX_FILE_SIZE     (64 * 1024)          /* Max 64KB per file */

/* Memory-mapped I/O regions */
#define AURORA_VM_MMIO_BASE         0xC000               /* MMIO base address */
#define AURORA_VM_MMIO_SIZE         0x2000               /* 8KB MMIO region */
#define AURORA_VM_MMIO_DISPLAY      0xC000               /* Display MMIO */
#define AURORA_VM_MMIO_KEYBOARD     0xC400               /* Keyboard MMIO */
#define AURORA_VM_MMIO_MOUSE        0xC800               /* Mouse MMIO */
#define AURORA_VM_MMIO_TIMER        0xCC00               /* Timer MMIO */
#define AURORA_VM_MMIO_NETWORK      0xD000               /* Network MMIO */
#define AURORA_VM_MMIO_IRQ_CTRL     0xD400               /* IRQ controller MMIO */

/* GDB server configuration */
#define AURORA_VM_GDB_PORT          1234                 /* Default GDB port */
#define AURORA_VM_GDB_PACKET_SIZE   4096                 /* GDB packet buffer size */

/* ===== CPU Status Flags ===== */
#define AURORA_FLAG_ZERO            0x01    /* Zero flag */
#define AURORA_FLAG_CARRY           0x02    /* Carry flag */
#define AURORA_FLAG_NEGATIVE        0x04    /* Negative flag */
#define AURORA_FLAG_OVERFLOW        0x08    /* Overflow flag */

/* ===== Memory Page Protection Bits ===== */
#define AURORA_PAGE_READ            0x01    /* Page is readable */
#define AURORA_PAGE_WRITE           0x02    /* Page is writable */
#define AURORA_PAGE_EXEC            0x04    /* Page is executable */
#define AURORA_PAGE_PRESENT         0x08    /* Page is present */

/* ===== Instruction Opcodes ===== */
typedef enum {
    /* Arithmetic operations (6) */
    AURORA_OP_ADD = 0x00,      /* Add: rd = rs1 + rs2 */
    AURORA_OP_SUB = 0x01,      /* Subtract: rd = rs1 - rs2 */
    AURORA_OP_MUL = 0x02,      /* Multiply: rd = rs1 * rs2 */
    AURORA_OP_DIV = 0x03,      /* Divide: rd = rs1 / rs2 */
    AURORA_OP_MOD = 0x04,      /* Modulo: rd = rs1 % rs2 */
    AURORA_OP_NEG = 0x05,      /* Negate: rd = -rs1 */
    
    /* Logical operations (6) */
    AURORA_OP_AND = 0x06,      /* Bitwise AND: rd = rs1 & rs2 */
    AURORA_OP_OR = 0x07,       /* Bitwise OR: rd = rs1 | rs2 */
    AURORA_OP_XOR = 0x08,      /* Bitwise XOR: rd = rs1 ^ rs2 */
    AURORA_OP_NOT = 0x09,      /* Bitwise NOT: rd = ~rs1 */
    AURORA_OP_SHL = 0x0A,      /* Shift left: rd = rs1 << rs2 */
    AURORA_OP_SHR = 0x0B,      /* Shift right: rd = rs1 >> rs2 */
    
    /* Memory operations (6) */
    AURORA_OP_LOAD = 0x0C,     /* Load: rd = mem[rs1 + rs2] */
    AURORA_OP_STORE = 0x0D,    /* Store: mem[rs1 + rs2] = rd */
    AURORA_OP_LOADI = 0x0E,    /* Load immediate: rd = imm */
    AURORA_OP_LOADB = 0x0F,    /* Load byte: rd = mem[rs1 + rs2] (byte) */
    AURORA_OP_STOREB = 0x10,   /* Store byte: mem[rs1 + rs2] = rd (byte) */
    AURORA_OP_MOVE = 0x11,     /* Move: rd = rs1 */
    
    /* Comparison operations (6) */
    AURORA_OP_CMP = 0x12,      /* Compare: flags = rs1 - rs2 */
    AURORA_OP_TEST = 0x13,     /* Test: flags = rs1 & rs2 */
    AURORA_OP_SLT = 0x14,      /* Set less than: rd = (rs1 < rs2) */
    AURORA_OP_SLE = 0x15,      /* Set less or equal: rd = (rs1 <= rs2) */
    AURORA_OP_SEQ = 0x16,      /* Set equal: rd = (rs1 == rs2) */
    AURORA_OP_SNE = 0x17,      /* Set not equal: rd = (rs1 != rs2) */
    
    /* Control flow operations (7) */
    AURORA_OP_JMP = 0x18,      /* Jump: PC = imm */
    AURORA_OP_JZ = 0x19,       /* Jump if zero: if (Z) PC = imm */
    AURORA_OP_JNZ = 0x1A,      /* Jump if not zero: if (!Z) PC = imm */
    AURORA_OP_JC = 0x1B,       /* Jump if carry: if (C) PC = imm */
    AURORA_OP_JNC = 0x1C,      /* Jump if not carry: if (!C) PC = imm */
    AURORA_OP_CALL = 0x1D,     /* Call: push PC, PC = imm */
    AURORA_OP_RET = 0x1E,      /* Return: PC = pop */
    
    /* System operations (2) */
    AURORA_OP_SYSCALL = 0x1F,  /* System call: syscall(r0) */
    AURORA_OP_HALT = 0x20,     /* Halt execution */
    
    /* Instruction set extensions - Floating point (8) */
    AURORA_OP_FADD = 0x21,     /* Float add: fd = fs1 + fs2 */
    AURORA_OP_FSUB = 0x22,     /* Float subtract: fd = fs1 - fs2 */
    AURORA_OP_FMUL = 0x23,     /* Float multiply: fd = fs1 * fs2 */
    AURORA_OP_FDIV = 0x24,     /* Float divide: fd = fs1 / fs2 */
    AURORA_OP_FCMP = 0x25,     /* Float compare: flags = fs1 - fs2 */
    AURORA_OP_FCVT = 0x26,     /* Float convert: fd = (float)rs1 */
    AURORA_OP_ICVT = 0x27,     /* Int convert: rd = (int)fs1 */
    AURORA_OP_FMOV = 0x28,     /* Float move: fd = fs1 */
    
    /* Instruction set extensions - SIMD/Vector (4) */
    AURORA_OP_VADD = 0x29,     /* Vector add: vd = vs1 + vs2 */
    AURORA_OP_VSUB = 0x2A,     /* Vector subtract: vd = vs1 - vs2 */
    AURORA_OP_VMUL = 0x2B,     /* Vector multiply: vd = vs1 * vs2 */
    AURORA_OP_VDOT = 0x2C,     /* Vector dot product: rd = vs1 . vs2 */
    
    /* Instruction set extensions - Atomic (4) */
    AURORA_OP_XCHG = 0x2D,     /* Atomic exchange: rd = mem[rs1], mem[rs1] = rs2 */
    AURORA_OP_CAS = 0x2E,      /* Compare-and-swap: if mem[rs1]==rd then mem[rs1]=rs2 */
    AURORA_OP_FADD_ATOMIC = 0x2F, /* Atomic fetch-add: rd = mem[rs1], mem[rs1] += rs2 */
    AURORA_OP_LOCK = 0x30,     /* Lock prefix for next instruction */
} aurora_opcode_t;

/* ===== Instruction Formats ===== */
typedef enum {
    AURORA_FORMAT_R,    /* R-type: opcode, rd, rs1, rs2 */
    AURORA_FORMAT_I,    /* I-type: opcode, rd, imm */
    AURORA_FORMAT_J,    /* J-type: opcode, imm */
} aurora_instruction_format_t;

/* ===== System Call Numbers ===== */
typedef enum {
    AURORA_SYSCALL_EXIT = 0,        /* Exit program: r0 = exit_code */
    AURORA_SYSCALL_PRINT = 1,       /* Print string: r0 = addr, r1 = len */
    AURORA_SYSCALL_READ = 2,        /* Read input: r0 = addr, r1 = max_len, returns len */
    AURORA_SYSCALL_OPEN = 3,        /* Open file: r0 = path, r1 = mode, returns fd */
    AURORA_SYSCALL_CLOSE = 4,       /* Close file: r0 = fd */
    AURORA_SYSCALL_READ_FILE = 5,   /* Read file: r0 = fd, r1 = addr, r2 = len */
    AURORA_SYSCALL_WRITE_FILE = 6,  /* Write file: r0 = fd, r1 = addr, r2 = len */
    AURORA_SYSCALL_GET_TIME = 7,    /* Get time: returns timestamp in r0 */
    AURORA_SYSCALL_SLEEP = 8,       /* Sleep: r0 = milliseconds */
    AURORA_SYSCALL_ALLOC = 9,       /* Allocate memory: r0 = size, returns addr */
    AURORA_SYSCALL_FREE = 10,       /* Free memory: r0 = addr */
    AURORA_SYSCALL_PIXEL = 11,      /* Draw pixel: r0 = x, r1 = y, r2 = color */
    
    /* Network syscalls */
    AURORA_SYSCALL_NET_SEND = 12,   /* Send network packet: r0 = addr, r1 = len */
    AURORA_SYSCALL_NET_RECV = 13,   /* Receive network packet: r0 = addr, r1 = max_len, returns len */
    AURORA_SYSCALL_NET_CONNECT = 14, /* Connect to address: r0 = addr_str, r1 = port */
    AURORA_SYSCALL_NET_LISTEN = 15,  /* Listen on port: r0 = port */
    
    /* Thread syscalls */
    AURORA_SYSCALL_THREAD_CREATE = 16, /* Create thread: r0 = entry_point, r1 = arg */
    AURORA_SYSCALL_THREAD_EXIT = 17,   /* Exit current thread */
    AURORA_SYSCALL_THREAD_JOIN = 18,   /* Join thread: r0 = thread_id */
    AURORA_SYSCALL_MUTEX_LOCK = 19,    /* Lock mutex: r0 = mutex_addr */
    AURORA_SYSCALL_MUTEX_UNLOCK = 20,  /* Unlock mutex: r0 = mutex_addr */
    AURORA_SYSCALL_SEM_WAIT = 21,      /* Wait on semaphore: r0 = sem_addr */
    AURORA_SYSCALL_SEM_POST = 22,      /* Post semaphore: r0 = sem_addr */
} aurora_syscall_t;

/* ===== VM Structures ===== */

/* Memory page descriptor */
typedef struct {
    uint8_t protection;     /* Protection bits */
    uint8_t flags;          /* Page flags */
} aurora_page_t;

/* CPU state */
typedef struct {
    uint32_t registers[AURORA_VM_NUM_REGISTERS];   /* General purpose registers */
    uint32_t pc;            /* Program counter */
    uint32_t sp;            /* Stack pointer */
    uint32_t fp;            /* Frame pointer */
    uint32_t flags;         /* Status flags */
    bool halted;            /* Halt flag */
} aurora_cpu_t;

/* Display device */
typedef struct {
    uint32_t pixels[AURORA_VM_DISPLAY_PIXELS];     /* RGBA pixels */
    bool dirty;             /* Dirty flag for rendering */
} aurora_display_t;

/* Keyboard device */
typedef struct {
    bool keys[AURORA_VM_NUM_KEYS];      /* Key states */
    uint8_t buffer[256];                /* Key buffer */
    uint32_t buffer_head;               /* Buffer head */
    uint32_t buffer_tail;               /* Buffer tail */
} aurora_keyboard_t;

/* Mouse device */
typedef struct {
    int32_t x;              /* X position */
    int32_t y;              /* Y position */
    uint8_t buttons;        /* Button state */
} aurora_mouse_t;

/* Timer device */
typedef struct {
    uint64_t ticks;         /* Timer ticks */
    uint64_t frequency;     /* Timer frequency */
} aurora_timer_t;

/* Storage device */
typedef struct {
    uint8_t *data;          /* Storage data */
    uint32_t size;          /* Storage size */
} aurora_storage_t;

/* Heap allocator */
typedef struct {
    uint32_t base;          /* Heap base address */
    uint32_t size;          /* Heap size */
    uint32_t used;          /* Used bytes */
} aurora_heap_t;

/* Debugger state */
typedef struct {
    bool enabled;                               /* Debugger enabled */
    bool single_step;                           /* Single-step mode */
    uint32_t breakpoints[AURORA_VM_MAX_BREAKPOINTS];   /* Breakpoint addresses */
    uint32_t num_breakpoints;                   /* Number of breakpoints */
    uint64_t instruction_count;                 /* Instruction counter */
    uint64_t cycle_count;                       /* Cycle counter */
} aurora_debugger_t;

/* Network packet */
typedef struct {
    uint8_t data[AURORA_VM_NET_MTU];            /* Packet data */
    uint32_t length;                            /* Packet length */
} aurora_net_packet_t;

/* Network device */
typedef struct {
    aurora_net_packet_t rx_queue[AURORA_VM_NET_QUEUE_SIZE];  /* RX queue */
    aurora_net_packet_t tx_queue[AURORA_VM_NET_QUEUE_SIZE];  /* TX queue */
    uint32_t rx_head;                           /* RX queue head */
    uint32_t rx_tail;                           /* RX queue tail */
    uint32_t tx_head;                           /* TX queue head */
    uint32_t tx_tail;                           /* TX queue tail */
    bool connected;                             /* Connection status */
} aurora_network_t;

/* File descriptor */
typedef struct {
    char path[AURORA_VM_MAX_FILENAME];          /* File path */
    uint32_t offset;                            /* Current read/write offset */
    uint32_t size;                              /* File size */
    uint32_t storage_offset;                    /* Offset in VM storage */
    bool open;                                  /* File descriptor is open */
    uint8_t mode;                               /* Access mode (0=read, 1=write, 2=both) */
} aurora_file_t;

/* File system */
typedef struct {
    aurora_file_t files[AURORA_VM_MAX_FILES];   /* File descriptors */
    uint32_t storage_used;                      /* Storage space used */
} aurora_filesystem_t;

/* Interrupt descriptor */
typedef struct {
    uint32_t handler;                           /* Handler address */
    bool enabled;                               /* Interrupt enabled */
    bool pending;                               /* Interrupt pending */
} aurora_interrupt_t;

/* Interrupt controller */
typedef struct {
    aurora_interrupt_t interrupts[AURORA_VM_MAX_INTERRUPTS]; /* Interrupt vectors */
    bool enabled;                               /* Global interrupt enable */
    uint32_t active;                            /* Active interrupt mask */
} aurora_irq_ctrl_t;

/* Thread context */
typedef struct {
    uint32_t id;                                /* Thread ID */
    uint32_t registers[AURORA_VM_NUM_REGISTERS]; /* Register state */
    uint32_t pc;                                /* Program counter */
    uint32_t sp;                                /* Stack pointer */
    uint32_t fp;                                /* Frame pointer */
    uint32_t flags;                             /* CPU flags */
    uint8_t stack[AURORA_VM_THREAD_STACK_SIZE]; /* Thread stack */
    bool active;                                /* Thread is active */
    bool waiting;                               /* Thread is waiting */
    uint32_t wait_target;                       /* Wait target (join/mutex) */
} aurora_thread_t;

/* Thread scheduler */
typedef struct {
    aurora_thread_t threads[AURORA_VM_MAX_THREADS]; /* Thread contexts */
    uint32_t current;                           /* Current thread */
    uint32_t count;                             /* Thread count */
} aurora_scheduler_t;

/* Mutex */
typedef struct {
    uint32_t owner;                             /* Owner thread ID */
    bool locked;                                /* Lock state */
} aurora_mutex_t;

/* Semaphore */
typedef struct {
    int32_t value;                              /* Semaphore value */
    uint32_t waiting[AURORA_VM_MAX_THREADS];    /* Waiting threads */
    uint32_t wait_count;                        /* Number waiting */
} aurora_semaphore_t;

/* JIT basic block */
typedef struct {
    uint32_t start_addr;                        /* Block start address */
    uint32_t length;                            /* Block length in bytes */
    uint8_t *native_code;                       /* Compiled native code */
    uint32_t native_length;                     /* Native code length */
    uint32_t exec_count;                        /* Execution count */
    bool compiled;                              /* Is compiled */
} aurora_jit_block_t;

/* JIT compiler state */
typedef struct {
    bool enabled;                               /* JIT enabled */
    uint8_t *cache;                             /* JIT code cache */
    uint32_t cache_size;                        /* Cache size */
    uint32_t cache_used;                        /* Cache used */
    aurora_jit_block_t blocks[256];             /* Compiled blocks */
    uint32_t num_blocks;                        /* Number of blocks */
} aurora_jit_t;

/* GDB server state */
typedef struct {
    bool enabled;                               /* GDB server enabled */
    bool connected;                             /* Client connected */
    int socket_fd;                              /* Socket file descriptor */
    char packet_buffer[AURORA_VM_GDB_PACKET_SIZE]; /* Packet buffer */
    bool break_requested;                       /* Break request */
} aurora_gdb_server_t;

/* Virtual machine instance */
typedef struct {
    /* Core components */
    aurora_cpu_t cpu;
    uint8_t memory[AURORA_VM_MEMORY_SIZE];
    aurora_page_t pages[AURORA_VM_NUM_PAGES];
    aurora_heap_t heap;
    
    /* Devices */
    aurora_display_t display;
    aurora_keyboard_t keyboard;
    aurora_mouse_t mouse;
    aurora_timer_t timer;
    aurora_storage_t storage;
    aurora_network_t network;
    aurora_filesystem_t filesystem;
    
    /* Advanced features */
    aurora_irq_ctrl_t irq_ctrl;
    aurora_scheduler_t scheduler;
    aurora_jit_t jit;
    aurora_gdb_server_t gdb;
    
    /* Debugger */
    aurora_debugger_t debugger;
    
    /* Runtime state */
    bool running;
    int exit_code;
} AuroraVM;

/* ===== VM API Functions ===== */

/**
 * Create a new VM instance
 * @return Pointer to VM instance or NULL on failure
 */
AuroraVM *aurora_vm_create(void);

/**
 * Initialize VM with default state
 * @param vm VM instance
 * @return 0 on success, -1 on failure
 */
int aurora_vm_init(AuroraVM *vm);

/**
 * Destroy VM instance and free resources
 * @param vm VM instance
 */
void aurora_vm_destroy(AuroraVM *vm);

/**
 * Load program into VM memory
 * @param vm VM instance
 * @param program Program data
 * @param size Program size in bytes
 * @param addr Load address
 * @return 0 on success, -1 on failure
 */
int aurora_vm_load_program(AuroraVM *vm, const uint8_t *program, size_t size, uint32_t addr);

/**
 * Run VM until halt or error
 * @param vm VM instance
 * @return Exit code
 */
int aurora_vm_run(AuroraVM *vm);

/**
 * Execute a single instruction
 * @param vm VM instance
 * @return 0 on success, -1 on error, 1 on halt
 */
int aurora_vm_step(AuroraVM *vm);

/**
 * Reset VM to initial state
 * @param vm VM instance
 */
void aurora_vm_reset(AuroraVM *vm);

/**
 * Get register value
 * @param vm VM instance
 * @param reg Register number (0-15)
 * @return Register value
 */
uint32_t aurora_vm_get_register(const AuroraVM *vm, uint32_t reg);

/**
 * Set register value
 * @param vm VM instance
 * @param reg Register number (0-15)
 * @param value New value
 */
void aurora_vm_set_register(AuroraVM *vm, uint32_t reg, uint32_t value);

/**
 * Read memory
 * @param vm VM instance
 * @param addr Address
 * @param size Size in bytes
 * @param buffer Output buffer
 * @return Bytes read or -1 on error
 */
int aurora_vm_read_memory(const AuroraVM *vm, uint32_t addr, size_t size, void *buffer);

/**
 * Write memory
 * @param vm VM instance
 * @param addr Address
 * @param size Size in bytes
 * @param buffer Input buffer
 * @return Bytes written or -1 on error
 */
int aurora_vm_write_memory(AuroraVM *vm, uint32_t addr, size_t size, const void *buffer);

/**
 * Set memory page protection
 * @param vm VM instance
 * @param page Page number
 * @param protection Protection bits
 * @return 0 on success, -1 on failure
 */
int aurora_vm_set_page_protection(AuroraVM *vm, uint32_t page, uint8_t protection);

/**
 * Get memory page protection
 * @param vm VM instance
 * @param page Page number
 * @return Protection bits or 0 on error
 */
uint8_t aurora_vm_get_page_protection(const AuroraVM *vm, uint32_t page);

/* ===== Debugger API ===== */

/**
 * Enable/disable debugger
 * @param vm VM instance
 * @param enabled Enable flag
 */
void aurora_vm_debugger_enable(AuroraVM *vm, bool enabled);

/**
 * Set single-step mode
 * @param vm VM instance
 * @param enabled Enable flag
 */
void aurora_vm_debugger_set_single_step(AuroraVM *vm, bool enabled);

/**
 * Add breakpoint
 * @param vm VM instance
 * @param addr Breakpoint address
 * @return 0 on success, -1 on failure
 */
int aurora_vm_debugger_add_breakpoint(AuroraVM *vm, uint32_t addr);

/**
 * Remove breakpoint
 * @param vm VM instance
 * @param addr Breakpoint address
 * @return 0 on success, -1 on failure
 */
int aurora_vm_debugger_remove_breakpoint(AuroraVM *vm, uint32_t addr);

/**
 * Clear all breakpoints
 * @param vm VM instance
 */
void aurora_vm_debugger_clear_breakpoints(AuroraVM *vm);

/**
 * Get instruction count
 * @param vm VM instance
 * @return Instruction count
 */
uint64_t aurora_vm_debugger_get_instruction_count(const AuroraVM *vm);

/**
 * Get cycle count
 * @param vm VM instance
 * @return Cycle count
 */
uint64_t aurora_vm_debugger_get_cycle_count(const AuroraVM *vm);

/**
 * Disassemble instruction
 * @param instruction Instruction word
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Number of characters written
 */
int aurora_vm_disassemble(uint32_t instruction, char *buffer, size_t buffer_size);

/* ===== Instruction Encoding ===== */

/**
 * Encode R-type instruction
 * @param opcode Operation code
 * @param rd Destination register
 * @param rs1 Source register 1
 * @param rs2 Source register 2
 * @return Encoded instruction
 */
uint32_t aurora_encode_r_type(aurora_opcode_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2);

/**
 * Encode I-type instruction
 * @param opcode Operation code
 * @param rd Destination register
 * @param imm Immediate value
 * @return Encoded instruction
 */
uint32_t aurora_encode_i_type(aurora_opcode_t opcode, uint8_t rd, int16_t imm);

/**
 * Encode J-type instruction
 * @param opcode Operation code
 * @param imm Immediate value
 * @return Encoded instruction
 */
uint32_t aurora_encode_j_type(aurora_opcode_t opcode, int32_t imm);

/* ===== Device API ===== */

/**
 * Get display pixel
 * @param vm VM instance
 * @param x X coordinate
 * @param y Y coordinate
 * @return RGBA pixel value
 */
uint32_t aurora_vm_display_get_pixel(const AuroraVM *vm, uint32_t x, uint32_t y);

/**
 * Set display pixel
 * @param vm VM instance
 * @param x X coordinate
 * @param y Y coordinate
 * @param color RGBA pixel value
 */
void aurora_vm_display_set_pixel(AuroraVM *vm, uint32_t x, uint32_t y, uint32_t color);

/**
 * Get keyboard key state
 * @param vm VM instance
 * @param key Key code
 * @return true if key is pressed
 */
bool aurora_vm_keyboard_is_key_pressed(const AuroraVM *vm, uint8_t key);

/**
 * Set keyboard key state
 * @param vm VM instance
 * @param key Key code
 * @param pressed Pressed state
 */
void aurora_vm_keyboard_set_key(AuroraVM *vm, uint8_t key, bool pressed);

/**
 * Get mouse position
 * @param vm VM instance
 * @param x Output X coordinate
 * @param y Output Y coordinate
 */
void aurora_vm_mouse_get_position(const AuroraVM *vm, int32_t *x, int32_t *y);

/**
 * Set mouse position
 * @param vm VM instance
 * @param x X coordinate
 * @param y Y coordinate
 */
void aurora_vm_mouse_set_position(AuroraVM *vm, int32_t x, int32_t y);

/**
 * Get mouse button state
 * @param vm VM instance
 * @return Button state bitmask
 */
uint8_t aurora_vm_mouse_get_buttons(const AuroraVM *vm);

/**
 * Set mouse button state
 * @param vm VM instance
 * @param buttons Button state bitmask
 */
void aurora_vm_mouse_set_buttons(AuroraVM *vm, uint8_t buttons);

/**
 * Get timer value
 * @param vm VM instance
 * @return Timer ticks
 */
uint64_t aurora_vm_timer_get_ticks(const AuroraVM *vm);

/**
 * Advance timer
 * @param vm VM instance
 * @param ticks Number of ticks to advance
 */
void aurora_vm_timer_advance(AuroraVM *vm, uint64_t ticks);

/**
 * Read from storage
 * @param vm VM instance
 * @param offset Offset in storage
 * @param buffer Output buffer
 * @param size Number of bytes to read
 * @return Bytes read or -1 on error
 */
int aurora_vm_storage_read(const AuroraVM *vm, uint32_t offset, void *buffer, size_t size);

/**
 * Write to storage
 * @param vm VM instance
 * @param offset Offset in storage
 * @param buffer Input buffer
 * @param size Number of bytes to write
 * @return Bytes written or -1 on error
 */
int aurora_vm_storage_write(AuroraVM *vm, uint32_t offset, const void *buffer, size_t size);

/* ===== Interrupt API ===== */

/**
 * Enable/disable interrupts
 * @param vm VM instance
 * @param enabled Enable flag
 */
void aurora_vm_irq_enable(AuroraVM *vm, bool enabled);

/**
 * Set interrupt handler
 * @param vm VM instance
 * @param irq Interrupt number
 * @param handler Handler address
 * @return 0 on success, -1 on failure
 */
int aurora_vm_irq_set_handler(AuroraVM *vm, uint32_t irq, uint32_t handler);

/**
 * Trigger interrupt
 * @param vm VM instance
 * @param irq Interrupt number
 * @return 0 on success, -1 on failure
 */
int aurora_vm_irq_trigger(AuroraVM *vm, uint32_t irq);

/* ===== Network API ===== */

/**
 * Send network packet
 * @param vm VM instance
 * @param data Packet data
 * @param length Packet length
 * @return Bytes sent or -1 on error
 */
int aurora_vm_net_send(AuroraVM *vm, const uint8_t *data, uint32_t length);

/**
 * Receive network packet
 * @param vm VM instance
 * @param buffer Output buffer
 * @param max_length Maximum length
 * @return Bytes received or -1 on error
 */
int aurora_vm_net_recv(AuroraVM *vm, uint8_t *buffer, uint32_t max_length);

/**
 * Check if network is connected
 * @param vm VM instance
 * @return true if connected
 */
bool aurora_vm_net_is_connected(const AuroraVM *vm);

/* ===== Thread API ===== */

/**
 * Create a new thread
 * @param vm VM instance
 * @param entry_point Thread entry point address
 * @param arg Thread argument
 * @return Thread ID or -1 on failure
 */
int aurora_vm_thread_create(AuroraVM *vm, uint32_t entry_point, uint32_t arg);

/**
 * Get current thread ID
 * @param vm VM instance
 * @return Current thread ID
 */
uint32_t aurora_vm_thread_current(const AuroraVM *vm);

/**
 * Yield to scheduler
 * @param vm VM instance
 */
void aurora_vm_thread_yield(AuroraVM *vm);

/* ===== JIT API ===== */

/**
 * Enable/disable JIT compilation
 * @param vm VM instance
 * @param enabled Enable flag
 */
void aurora_vm_jit_enable(AuroraVM *vm, bool enabled);

/**
 * Compile a basic block
 * @param vm VM instance
 * @param addr Block address
 * @return 0 on success, -1 on failure
 */
int aurora_vm_jit_compile_block(AuroraVM *vm, uint32_t addr);

/**
 * Clear JIT cache
 * @param vm VM instance
 */
void aurora_vm_jit_clear_cache(AuroraVM *vm);

/* ===== GDB Server API ===== */

/**
 * Start GDB server
 * @param vm VM instance
 * @param port Port number
 * @return 0 on success, -1 on failure
 */
int aurora_vm_gdb_start(AuroraVM *vm, int port);

/**
 * Stop GDB server
 * @param vm VM instance
 */
void aurora_vm_gdb_stop(AuroraVM *vm);

/**
 * Handle GDB server events
 * @param vm VM instance
 * @return 0 on success, -1 on error
 */
int aurora_vm_gdb_handle(AuroraVM *vm);

#endif /* AURORA_VM_H */
