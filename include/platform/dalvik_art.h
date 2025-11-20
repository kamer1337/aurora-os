/**
 * @file dalvik_art.h
 * @brief Dalvik/ART Virtual Machine Implementation for Aurora OS
 * 
 * Provides Android Runtime (ART) and legacy Dalvik VM support
 * for executing Android applications in Aurora OS
 */

#ifndef DALVIK_ART_H
#define DALVIK_ART_H

#include <stdint.h>
#include <stdbool.h>

/* VM Mode */
typedef enum {
    VM_MODE_DALVIK = 0,    /* Legacy Dalvik VM */
    VM_MODE_ART            /* Android Runtime (ART) */
} vm_mode_t;

/* VM State */
typedef enum {
    DALVIK_STATE_UNINITIALIZED = 0,
    DALVIK_STATE_INITIALIZED,
    DALVIK_STATE_RUNNING,
    DALVIK_STATE_PAUSED,
    DALVIK_STATE_STOPPED,
    DALVIK_STATE_ERROR
} dalvik_state_t;

/* DEX File Format Structures */
#define DEX_FILE_MAGIC "dex\n"
#define DEX_FILE_VERSION_035 "035\0"
#define DEX_FILE_VERSION_037 "037\0"
#define DEX_FILE_VERSION_038 "038\0"

typedef struct {
    uint8_t magic[8];           /* "dex\n035\0" or similar */
    uint32_t checksum;          /* adler32 checksum */
    uint8_t signature[20];      /* SHA-1 signature */
    uint32_t file_size;         /* Size of entire file */
    uint32_t header_size;       /* Size of header (0x70) */
    uint32_t endian_tag;        /* 0x12345678 for little endian */
    uint32_t link_size;         /* Size of link section */
    uint32_t link_off;          /* Offset to link section */
    uint32_t map_off;           /* Offset to map list */
    uint32_t string_ids_size;   /* Number of strings in string ID list */
    uint32_t string_ids_off;    /* Offset to string ID list */
    uint32_t type_ids_size;     /* Number of type IDs */
    uint32_t type_ids_off;      /* Offset to type ID list */
    uint32_t proto_ids_size;    /* Number of method prototypes */
    uint32_t proto_ids_off;     /* Offset to method prototype list */
    uint32_t field_ids_size;    /* Number of field IDs */
    uint32_t field_ids_off;     /* Offset to field ID list */
    uint32_t method_ids_size;   /* Number of method IDs */
    uint32_t method_ids_off;    /* Offset to method ID list */
    uint32_t class_defs_size;   /* Number of class definitions */
    uint32_t class_defs_off;    /* Offset to class definitions */
    uint32_t data_size;         /* Size of data section */
    uint32_t data_off;          /* Offset to data section */
} dex_header_t;

/* Class Definition */
typedef struct {
    uint32_t class_idx;         /* Index into type_ids for this class */
    uint32_t access_flags;      /* Access flags */
    uint32_t superclass_idx;    /* Index into type_ids for superclass */
    uint32_t interfaces_off;    /* Offset to list of interfaces */
    uint32_t source_file_idx;   /* Index into string_ids for source file name */
    uint32_t annotations_off;   /* Offset to annotations */
    uint32_t class_data_off;    /* Offset to class data */
    uint32_t static_values_off; /* Offset to static field initial values */
} dex_class_def_t;

/* Method Structure */
typedef struct {
    uint16_t class_idx;         /* Index into type_ids for defining class */
    uint16_t proto_idx;         /* Index into proto_ids for method prototype */
    uint32_t name_idx;          /* Index into string_ids for method name */
} dex_method_id_t;

/* Field Structure */
typedef struct {
    uint16_t class_idx;         /* Index into type_ids for defining class */
    uint16_t type_idx;          /* Index into type_ids for field type */
    uint32_t name_idx;          /* Index into string_ids for field name */
} dex_field_id_t;

/* Dalvik Bytecode Opcodes (subset) */
typedef enum {
    OP_NOP = 0x00,
    OP_MOVE = 0x01,
    OP_MOVE_FROM16 = 0x02,
    OP_MOVE_16 = 0x03,
    OP_MOVE_WIDE = 0x04,
    OP_MOVE_WIDE_FROM16 = 0x05,
    OP_MOVE_WIDE_16 = 0x06,
    OP_MOVE_OBJECT = 0x07,
    OP_MOVE_OBJECT_FROM16 = 0x08,
    OP_MOVE_OBJECT_16 = 0x09,
    OP_MOVE_RESULT = 0x0a,
    OP_MOVE_RESULT_WIDE = 0x0b,
    OP_MOVE_RESULT_OBJECT = 0x0c,
    OP_MOVE_EXCEPTION = 0x0d,
    OP_RETURN_VOID = 0x0e,
    OP_RETURN = 0x0f,
    OP_RETURN_WIDE = 0x10,
    OP_RETURN_OBJECT = 0x11,
    OP_CONST_4 = 0x12,
    OP_CONST_16 = 0x13,
    OP_CONST = 0x14,
    OP_CONST_HIGH16 = 0x15,
    OP_CONST_WIDE_16 = 0x16,
    OP_CONST_WIDE_32 = 0x17,
    OP_CONST_WIDE = 0x18,
    OP_CONST_WIDE_HIGH16 = 0x19,
    OP_CONST_STRING = 0x1a,
    OP_CONST_STRING_JUMBO = 0x1b,
    OP_CONST_CLASS = 0x1c,
    OP_MONITOR_ENTER = 0x1d,
    OP_MONITOR_EXIT = 0x1e,
    OP_CHECK_CAST = 0x1f,
    OP_INSTANCE_OF = 0x20,
    OP_ARRAY_LENGTH = 0x21,
    OP_NEW_INSTANCE = 0x22,
    OP_NEW_ARRAY = 0x23,
    OP_FILLED_NEW_ARRAY = 0x24,
    OP_FILLED_NEW_ARRAY_RANGE = 0x25,
    OP_FILL_ARRAY_DATA = 0x26,
    OP_THROW = 0x27,
    OP_GOTO = 0x28,
    OP_GOTO_16 = 0x29,
    OP_GOTO_32 = 0x2a,
    OP_PACKED_SWITCH = 0x2b,
    OP_SPARSE_SWITCH = 0x2c,
    /* Comparison operations */
    OP_CMPL_FLOAT = 0x2d,
    OP_CMPG_FLOAT = 0x2e,
    OP_CMPL_DOUBLE = 0x2f,
    OP_CMPG_DOUBLE = 0x30,
    OP_CMP_LONG = 0x31,
    /* Conditional branches */
    OP_IF_EQ = 0x32,
    OP_IF_NE = 0x33,
    OP_IF_LT = 0x34,
    OP_IF_GE = 0x35,
    OP_IF_GT = 0x36,
    OP_IF_LE = 0x37,
    OP_IF_EQZ = 0x38,
    OP_IF_NEZ = 0x39,
    OP_IF_LTZ = 0x3a,
    OP_IF_GEZ = 0x3b,
    OP_IF_GTZ = 0x3c,
    OP_IF_LEZ = 0x3d,
    /* Array operations */
    OP_AGET = 0x44,
    OP_AGET_WIDE = 0x45,
    OP_AGET_OBJECT = 0x46,
    OP_AGET_BOOLEAN = 0x47,
    OP_AGET_BYTE = 0x48,
    OP_AGET_CHAR = 0x49,
    OP_AGET_SHORT = 0x4a,
    OP_APUT = 0x4b,
    OP_APUT_WIDE = 0x4c,
    OP_APUT_OBJECT = 0x4d,
    OP_APUT_BOOLEAN = 0x4e,
    OP_APUT_BYTE = 0x4f,
    OP_APUT_CHAR = 0x50,
    OP_APUT_SHORT = 0x51,
    /* Instance field operations */
    OP_IGET = 0x52,
    OP_IGET_WIDE = 0x53,
    OP_IGET_OBJECT = 0x54,
    OP_IGET_BOOLEAN = 0x55,
    OP_IGET_BYTE = 0x56,
    OP_IGET_CHAR = 0x57,
    OP_IGET_SHORT = 0x58,
    OP_IPUT = 0x59,
    OP_IPUT_WIDE = 0x5a,
    OP_IPUT_OBJECT = 0x5b,
    OP_IPUT_BOOLEAN = 0x5c,
    OP_IPUT_BYTE = 0x5d,
    OP_IPUT_CHAR = 0x5e,
    OP_IPUT_SHORT = 0x5f,
    /* Static field operations */
    OP_SGET = 0x60,
    OP_SGET_WIDE = 0x61,
    OP_SGET_OBJECT = 0x62,
    OP_SGET_BOOLEAN = 0x63,
    OP_SGET_BYTE = 0x64,
    OP_SGET_CHAR = 0x65,
    OP_SGET_SHORT = 0x66,
    OP_SPUT = 0x67,
    OP_SPUT_WIDE = 0x68,
    OP_SPUT_OBJECT = 0x69,
    OP_SPUT_BOOLEAN = 0x6a,
    OP_SPUT_BYTE = 0x6b,
    OP_SPUT_CHAR = 0x6c,
    OP_SPUT_SHORT = 0x6d,
    /* Method invocation */
    OP_INVOKE_VIRTUAL = 0x6e,
    OP_INVOKE_SUPER = 0x6f,
    OP_INVOKE_DIRECT = 0x70,
    OP_INVOKE_STATIC = 0x71,
    OP_INVOKE_INTERFACE = 0x72,
    /* Arithmetic operations */
    OP_ADD_INT = 0x90,
    OP_SUB_INT = 0x91,
    OP_MUL_INT = 0x92,
    OP_DIV_INT = 0x93,
    OP_REM_INT = 0x94,
    OP_AND_INT = 0x95,
    OP_OR_INT = 0x96,
    OP_XOR_INT = 0x97,
} dalvik_opcode_t;

/* VM Register - Dalvik uses 32-bit registers */
typedef union {
    uint32_t u32;
    int32_t i32;
    float f32;
    void* ref;              /* Object reference */
} vm_register_t;

/* VM Frame - Execution frame for method calls */
#define MAX_REGISTERS 256
#define MAX_STACK_DEPTH 1024

typedef struct vm_frame {
    struct vm_frame* prev;      /* Previous frame (caller) */
    uint32_t* method_code;      /* Method bytecode */
    uint32_t code_size;         /* Code size in 16-bit units */
    vm_register_t regs[MAX_REGISTERS]; /* Registers */
    uint32_t num_regs;          /* Number of registers used */
    uint32_t pc;                /* Program counter */
    void* method;               /* Method info */
} vm_frame_t;

/* Class Loader */
typedef struct {
    dex_header_t* dex_file;     /* Loaded DEX file */
    uint32_t num_classes;       /* Number of classes */
    void** loaded_classes;      /* Array of loaded class pointers */
} class_loader_t;

/* Dalvik/ART VM Instance */
typedef struct dalvik_vm {
    vm_mode_t mode;             /* VM mode (Dalvik or ART) */
    dalvik_state_t state;       /* Current VM state */
    class_loader_t* class_loader; /* Class loader */
    vm_frame_t* current_frame;  /* Current execution frame */
    vm_frame_t frame_stack[MAX_STACK_DEPTH]; /* Frame stack */
    uint32_t frame_depth;       /* Current frame depth */
    bool jit_enabled;           /* JIT compilation enabled */
    uint32_t heap_size;         /* Heap size in bytes */
    void* heap_base;            /* Heap base address */
    uint32_t heap_used;         /* Heap bytes used */
} dalvik_vm_t;

/**
 * Initialize Dalvik/ART VM subsystem
 * @param mode VM mode (Dalvik or ART)
 * @return 0 on success, -1 on failure
 */
int dalvik_init(vm_mode_t mode);

/**
 * Create a new VM instance
 * @param mode VM mode
 * @param heap_size Heap size in bytes
 * @return VM instance or NULL on failure
 */
dalvik_vm_t* dalvik_create(vm_mode_t mode, uint32_t heap_size);

/**
 * Destroy VM instance
 * @param vm VM instance
 */
void dalvik_destroy(dalvik_vm_t* vm);

/**
 * Load DEX file into VM
 * @param vm VM instance
 * @param dex_data DEX file data
 * @param size DEX file size
 * @return 0 on success, -1 on failure
 */
int dalvik_load_dex(dalvik_vm_t* vm, const uint8_t* dex_data, uint32_t size);

/**
 * Load class by name
 * @param vm VM instance
 * @param class_name Class name (e.g., "Ljava/lang/Object;")
 * @return Class pointer or NULL on failure
 */
void* dalvik_load_class(dalvik_vm_t* vm, const char* class_name);

/**
 * Execute method
 * @param vm VM instance
 * @param method Method to execute
 * @param args Method arguments
 * @return Return value
 */
int32_t dalvik_execute_method(dalvik_vm_t* vm, void* method, void* args);

/**
 * Execute bytecode instruction
 * @param vm VM instance
 * @param frame Current frame
 * @param opcode Instruction opcode
 * @return 0 on success, -1 on error
 */
int dalvik_execute_instruction(dalvik_vm_t* vm, vm_frame_t* frame, uint16_t opcode);

/**
 * Start VM execution
 * @param vm VM instance
 * @param entry_class Entry class name
 * @param entry_method Entry method name
 * @return 0 on success, -1 on failure
 */
int dalvik_start(dalvik_vm_t* vm, const char* entry_class, const char* entry_method);

/**
 * Stop VM execution
 * @param vm VM instance
 * @return 0 on success, -1 on failure
 */
int dalvik_stop(dalvik_vm_t* vm);

/**
 * Pause VM execution
 * @param vm VM instance
 * @return 0 on success, -1 on failure
 */
int dalvik_pause(dalvik_vm_t* vm);

/**
 * Resume VM execution
 * @param vm VM instance
 * @return 0 on success, -1 on failure
 */
int dalvik_resume(dalvik_vm_t* vm);

/**
 * Get VM state
 * @param vm VM instance
 * @return Current state
 */
dalvik_state_t dalvik_get_state(dalvik_vm_t* vm);

/**
 * Enable/disable JIT compilation
 * @param vm VM instance
 * @param enable Enable flag
 * @return 0 on success, -1 on failure
 */
int dalvik_enable_jit(dalvik_vm_t* vm, bool enable);

/**
 * Allocate object on heap
 * @param vm VM instance
 * @param size Object size
 * @return Object pointer or NULL on failure
 */
void* dalvik_alloc_object(dalvik_vm_t* vm, uint32_t size);

/**
 * Free object from heap
 * @param vm VM instance
 * @param obj Object pointer
 */
void dalvik_free_object(dalvik_vm_t* vm, void* obj);

/**
 * Garbage collection
 * @param vm VM instance
 * @return Number of bytes freed
 */
uint32_t dalvik_gc(dalvik_vm_t* vm);

/**
 * Get VM version string
 * @return Version string
 */
const char* dalvik_get_version(void);

#endif /* DALVIK_ART_H */
