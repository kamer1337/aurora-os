/**
 * @file binder_ipc.h
 * @brief Binder IPC Implementation for Aurora OS
 * 
 * Provides Android Binder Inter-Process Communication mechanism
 * for Android service communication
 */

#ifndef BINDER_IPC_H
#define BINDER_IPC_H

#include <stdint.h>
#include <stdbool.h>

/* Binder Protocol Version */
#define BINDER_CURRENT_PROTOCOL_VERSION 8

/* Binder Node Types */
typedef enum {
    BINDER_TYPE_BINDER = 0x62696e64,     /* 'bind' */
    BINDER_TYPE_WEAK_BINDER = 0x77626e64, /* 'wbnd' */
    BINDER_TYPE_HANDLE = 0x68646c65,     /* 'hdle' */
    BINDER_TYPE_WEAK_HANDLE = 0x77686465, /* 'whde' */
    BINDER_TYPE_FD = 0x66646573          /* 'fdes' */
} binder_type_t;

/* Binder Transaction Codes */
typedef enum {
    BC_TRANSACTION = 0x40406300,
    BC_REPLY = 0x40406301,
    BC_ACQUIRE_RESULT = 0x40046302,
    BC_FREE_BUFFER = 0x40046303,
    BC_INCREFS = 0x40046304,
    BC_ACQUIRE = 0x40046305,
    BC_RELEASE = 0x40046306,
    BC_DECREFS = 0x40046307,
    BC_INCREFS_DONE = 0x40086308,
    BC_ACQUIRE_DONE = 0x40086309,
    BC_REGISTER_LOOPER = 0x4004630b,
    BC_ENTER_LOOPER = 0x4004630c,
    BC_EXIT_LOOPER = 0x4004630d,
    BC_REQUEST_DEATH_NOTIFICATION = 0x4010630e,
    BC_CLEAR_DEATH_NOTIFICATION = 0x400c630f,
    BC_DEAD_BINDER_DONE = 0x40046310
} binder_command_t;

/* Binder Return Codes */
typedef enum {
    BR_ERROR = 0x80047200,
    BR_OK = 0x80047201,
    BR_TRANSACTION = 0x80587202,
    BR_REPLY = 0x80587203,
    BR_ACQUIRE_RESULT = 0x80047204,
    BR_DEAD_REPLY = 0x80047205,
    BR_TRANSACTION_COMPLETE = 0x80047206,
    BR_INCREFS = 0x80087207,
    BR_ACQUIRE = 0x80087208,
    BR_RELEASE = 0x80087209,
    BR_DECREFS = 0x8008720a,
    BR_NOOP = 0x8004720c,
    BR_SPAWN_LOOPER = 0x8004720d,
    BR_FINISHED = 0x8004720e,
    BR_DEAD_BINDER = 0x8008720f,
    BR_CLEAR_DEATH_NOTIFICATION_DONE = 0x80087210,
    BR_FAILED_REPLY = 0x80047211
} binder_return_t;

/* Binder Transaction Flags */
#define TF_ONE_WAY 0x01   /* Async transaction */
#define TF_ROOT_OBJECT 0x04
#define TF_STATUS_CODE 0x08
#define TF_ACCEPT_FDS 0x10

/* Binder Object Types */
typedef struct {
    uint32_t type;              /* BINDER_TYPE_* */
    uint32_t flags;             /* Transaction flags */
    union {
        void* binder;           /* Local binder object */
        uint32_t handle;        /* Remote binder handle */
    } object;
    void* cookie;               /* Extra data */
} binder_object_t;

/* Binder Transaction Data */
typedef struct {
    uint32_t target_handle;     /* Target binder handle (0 for context manager) */
    void* target_cookie;        /* Target object cookie */
    uint32_t code;              /* Transaction code */
    uint32_t flags;             /* Transaction flags */
    uint32_t sender_pid;        /* Sender process ID */
    uint32_t sender_euid;       /* Sender effective user ID */
    uint32_t data_size;         /* Data buffer size */
    uint32_t offsets_size;      /* Offsets array size */
    union {
        struct {
            const void* buffer; /* Data buffer */
            const void* offsets; /* Binder object offsets */
        } ptr;
        uint8_t buf[8];
    } data;
} binder_transaction_t;

/* Parcel - Marshalling container */
#define PARCEL_MAX_SIZE 4096

typedef struct {
    uint8_t data[PARCEL_MAX_SIZE]; /* Data buffer */
    uint32_t data_pos;          /* Current position in data */
    uint32_t data_size;         /* Total data size */
    uint32_t objects_count;     /* Number of binder objects */
    uint32_t objects_offsets[64]; /* Offsets to binder objects */
} parcel_t;

/* Binder Node - Represents a binder object */
typedef struct binder_node {
    uint32_t handle;            /* Unique handle */
    void* ptr;                  /* Pointer to actual object */
    void* cookie;               /* Extra data */
    uint32_t refs;              /* Reference count */
    uint32_t weak_refs;         /* Weak reference count */
    bool dead;                  /* Node is dead */
    struct binder_node* next;   /* Next node in list */
} binder_node_t;

/* Binder Thread - Per-thread binder state */
typedef struct {
    uint32_t pid;               /* Process ID */
    uint32_t tid;               /* Thread ID */
    bool looper_registered;     /* Thread registered as looper */
    bool looper_entered;        /* Thread in looper */
    parcel_t* transaction_stack[16]; /* Transaction stack */
    uint32_t transaction_depth; /* Stack depth */
} binder_thread_t;

/* Binder Process - Per-process binder state */
typedef struct binder_process {
    uint32_t pid;               /* Process ID */
    binder_node_t* nodes;       /* List of binder nodes */
    uint32_t next_handle;       /* Next available handle */
    binder_thread_t* threads[64]; /* Thread array */
    uint32_t thread_count;      /* Number of threads */
    bool context_manager;       /* Is context manager */
} binder_process_t;

/* Binder Driver State */
typedef struct {
    binder_process_t* processes[256]; /* Process array */
    uint32_t process_count;     /* Number of processes */
    binder_process_t* context_mgr; /* Context manager process */
    bool initialized;           /* Driver initialized */
} binder_driver_t;

/* Service Manager Interface */
typedef enum {
    SVC_MGR_GET_SERVICE = 1,
    SVC_MGR_CHECK_SERVICE = 2,
    SVC_MGR_ADD_SERVICE = 3,
    SVC_MGR_LIST_SERVICES = 4
} service_manager_cmd_t;

typedef struct {
    char name[128];             /* Service name */
    uint32_t handle;            /* Service binder handle */
    bool allow_isolated;        /* Allow access from isolated processes */
} service_entry_t;

typedef struct {
    service_entry_t services[256]; /* Service registry */
    uint32_t service_count;     /* Number of registered services */
} service_manager_t;

/**
 * Initialize Binder IPC subsystem
 * @return 0 on success, -1 on failure
 */
int binder_init(void);

/**
 * Create process binder state
 * @param pid Process ID
 * @return Process state or NULL on failure
 */
binder_process_t* binder_create_process(uint32_t pid);

/**
 * Destroy process binder state
 * @param process Process state
 */
void binder_destroy_process(binder_process_t* process);

/**
 * Create thread binder state
 * @param process Process state
 * @param tid Thread ID
 * @return Thread state or NULL on failure
 */
binder_thread_t* binder_create_thread(binder_process_t* process, uint32_t tid);

/**
 * Handle binder transaction
 * @param process Source process
 * @param thread Source thread
 * @param transaction Transaction data
 * @return 0 on success, -1 on failure
 */
int binder_transact(binder_process_t* process, binder_thread_t* thread, 
                    binder_transaction_t* transaction);

/**
 * Send reply to transaction
 * @param thread Thread state
 * @param reply Reply data
 * @return 0 on success, -1 on failure
 */
int binder_reply(binder_thread_t* thread, parcel_t* reply);

/**
 * Create new binder node
 * @param process Process state
 * @param ptr Object pointer
 * @param cookie Cookie data
 * @return Binder handle or 0 on failure
 */
uint32_t binder_new_node(binder_process_t* process, void* ptr, void* cookie);

/**
 * Get binder node by handle
 * @param process Process state
 * @param handle Binder handle
 * @return Node or NULL if not found
 */
binder_node_t* binder_get_node(binder_process_t* process, uint32_t handle);

/**
 * Increment node reference count
 * @param node Binder node
 * @return 0 on success, -1 on failure
 */
int binder_inc_ref(binder_node_t* node);

/**
 * Decrement node reference count
 * @param node Binder node
 * @return 0 on success, -1 on failure
 */
int binder_dec_ref(binder_node_t* node);

/**
 * Initialize parcel
 * @param parcel Parcel to initialize
 */
void parcel_init(parcel_t* parcel);

/**
 * Write data to parcel
 * @param parcel Parcel
 * @param data Data to write
 * @param size Data size
 * @return 0 on success, -1 on failure
 */
int parcel_write_data(parcel_t* parcel, const void* data, uint32_t size);

/**
 * Read data from parcel
 * @param parcel Parcel
 * @param data Buffer to read into
 * @param size Data size
 * @return 0 on success, -1 on failure
 */
int parcel_read_data(parcel_t* parcel, void* data, uint32_t size);

/**
 * Write int32 to parcel
 * @param parcel Parcel
 * @param value Value to write
 * @return 0 on success, -1 on failure
 */
int parcel_write_int32(parcel_t* parcel, int32_t value);

/**
 * Read int32 from parcel
 * @param parcel Parcel
 * @param value Pointer to store value
 * @return 0 on success, -1 on failure
 */
int parcel_read_int32(parcel_t* parcel, int32_t* value);

/**
 * Write string to parcel
 * @param parcel Parcel
 * @param str String to write
 * @return 0 on success, -1 on failure
 */
int parcel_write_string(parcel_t* parcel, const char* str);

/**
 * Read string from parcel
 * @param parcel Parcel
 * @param str Buffer to store string
 * @param max_size Maximum buffer size
 * @return 0 on success, -1 on failure
 */
int parcel_read_string(parcel_t* parcel, char* str, uint32_t max_size);

/**
 * Write binder object to parcel
 * @param parcel Parcel
 * @param obj Binder object
 * @return 0 on success, -1 on failure
 */
int parcel_write_binder(parcel_t* parcel, binder_object_t* obj);

/**
 * Initialize service manager
 * @return 0 on success, -1 on failure
 */
int service_manager_init(void);

/**
 * Register service with service manager
 * @param name Service name
 * @param handle Service binder handle
 * @return 0 on success, -1 on failure
 */
int service_manager_add_service(const char* name, uint32_t handle);

/**
 * Get service from service manager
 * @param name Service name
 * @return Service binder handle or 0 if not found
 */
uint32_t service_manager_get_service(const char* name);

/**
 * Check if service exists
 * @param name Service name
 * @return true if service exists, false otherwise
 */
bool service_manager_check_service(const char* name);

/**
 * List all registered services
 * @param names Buffer to store service names
 * @param max_count Maximum number of services
 * @return Number of services returned
 */
uint32_t service_manager_list_services(char names[][128], uint32_t max_count);

/**
 * Get Binder driver instance
 * @return Binder driver state
 */
binder_driver_t* binder_get_driver(void);

/**
 * Get Service Manager instance
 * @return Service manager state
 */
service_manager_t* binder_get_service_manager(void);

/**
 * Get Binder version string
 * @return Version string
 */
const char* binder_get_version(void);

#endif /* BINDER_IPC_H */
