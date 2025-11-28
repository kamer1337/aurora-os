/**
 * Aurora OS - eBPF Framework Header
 * 
 * Provides a basic eBPF (extended Berkeley Packet Filter) implementation
 * for kernel programmability and dynamic tracing.
 */

#ifndef AURORA_EBPF_H
#define AURORA_EBPF_H

#include <stdint.h>
#include <stddef.h>

/* eBPF return codes */
#define BPF_OK              0
#define BPF_ERROR          -1
#define BPF_INVALID_PARAM  -2
#define BPF_NOT_FOUND      -3
#define BPF_LIMIT_REACHED  -4
#define BPF_VERIFY_FAILED  -5

/* eBPF program types */
typedef enum {
    BPF_PROG_TYPE_UNSPEC = 0,
    BPF_PROG_TYPE_SOCKET_FILTER,
    BPF_PROG_TYPE_KPROBE,
    BPF_PROG_TYPE_SCHED_CLS,
    BPF_PROG_TYPE_SCHED_ACT,
    BPF_PROG_TYPE_TRACEPOINT,
    BPF_PROG_TYPE_XDP,
    BPF_PROG_TYPE_PERF_EVENT,
    BPF_PROG_TYPE_CGROUP_SKB,
    BPF_PROG_TYPE_CGROUP_SOCK,
    BPF_PROG_TYPE_LSM,
    BPF_PROG_TYPE_RAW_TRACEPOINT,
    BPF_PROG_TYPE_COUNT
} bpf_prog_type_t;

/* eBPF map types */
typedef enum {
    BPF_MAP_TYPE_UNSPEC = 0,
    BPF_MAP_TYPE_HASH,
    BPF_MAP_TYPE_ARRAY,
    BPF_MAP_TYPE_PROG_ARRAY,
    BPF_MAP_TYPE_PERF_EVENT_ARRAY,
    BPF_MAP_TYPE_PERCPU_HASH,
    BPF_MAP_TYPE_PERCPU_ARRAY,
    BPF_MAP_TYPE_STACK_TRACE,
    BPF_MAP_TYPE_LRU_HASH,
    BPF_MAP_TYPE_RINGBUF,
    BPF_MAP_TYPE_COUNT
} bpf_map_type_t;

/* eBPF instruction encoding */
#define BPF_CLASS(code)    ((code) & 0x07)
#define BPF_LD             0x00
#define BPF_LDX            0x01
#define BPF_ST             0x02
#define BPF_STX            0x03
#define BPF_ALU            0x04
#define BPF_JMP            0x05
#define BPF_RET            0x06
#define BPF_ALU64          0x07

/* ALU operations */
#define BPF_ADD            0x00
#define BPF_SUB            0x10
#define BPF_MUL            0x20
#define BPF_DIV            0x30
#define BPF_OR             0x40
#define BPF_AND            0x50
#define BPF_LSH            0x60
#define BPF_RSH            0x70
#define BPF_NEG            0x80
#define BPF_MOD            0x90
#define BPF_XOR            0xa0
#define BPF_MOV            0xb0
#define BPF_ARSH           0xc0

/* Jump operations */
#define BPF_JA             0x00
#define BPF_JEQ            0x10
#define BPF_JGT            0x20
#define BPF_JGE            0x30
#define BPF_JSET           0x40
#define BPF_JNE            0x50
#define BPF_JSGT           0x60
#define BPF_JSGE           0x70
#define BPF_EXIT           0x90
#define BPF_CALL           0x80

/* Source operand */
#define BPF_K              0x00
#define BPF_X              0x08

/* eBPF registers */
#define BPF_REG_0          0   /* Return value */
#define BPF_REG_1          1   /* Arg 1 */
#define BPF_REG_2          2   /* Arg 2 */
#define BPF_REG_3          3   /* Arg 3 */
#define BPF_REG_4          4   /* Arg 4 */
#define BPF_REG_5          5   /* Arg 5 */
#define BPF_REG_6          6   /* Callee saved */
#define BPF_REG_7          7   /* Callee saved */
#define BPF_REG_8          8   /* Callee saved */
#define BPF_REG_9          9   /* Callee saved */
#define BPF_REG_10         10  /* Stack pointer (read-only) */
#define BPF_REG_COUNT      11

/* Maximum values */
#define BPF_MAX_PROGS      256
#define BPF_MAX_MAPS       256
#define BPF_MAX_INSNS      4096
#define BPF_STACK_SIZE     512
#define BPF_MAX_ATTACH     64

/* eBPF instruction */
typedef struct {
    uint8_t opcode;        /* Operation code */
    uint8_t dst_reg : 4;   /* Destination register */
    uint8_t src_reg : 4;   /* Source register */
    int16_t offset;        /* Signed offset */
    int32_t imm;           /* Immediate value */
} __attribute__((packed)) bpf_insn_t;

/* eBPF map definition */
typedef struct bpf_map {
    uint32_t id;
    bpf_map_type_t type;
    uint32_t key_size;
    uint32_t value_size;
    uint32_t max_entries;
    uint32_t flags;
    void* data;            /* Map data storage */
    uint32_t entry_count;
} bpf_map_t;

/* eBPF program */
typedef struct bpf_prog {
    uint32_t id;
    bpf_prog_type_t type;
    bpf_insn_t* insns;
    uint32_t insn_count;
    uint32_t flags;
    char name[64];
    uint8_t verified;
    uint8_t jit_compiled;
    void* jit_image;       /* JIT compiled code */
    size_t jit_size;
} bpf_prog_t;

/* eBPF execution context */
typedef struct {
    uint64_t regs[BPF_REG_COUNT];
    uint8_t stack[BPF_STACK_SIZE];
    void* ctx;             /* Context pointer */
    bpf_prog_t* prog;
    uint32_t insn_idx;
    uint32_t insn_processed;
} bpf_exec_ctx_t;

/* eBPF attach point */
typedef struct {
    uint32_t prog_id;
    bpf_prog_type_t type;
    void* target;          /* Target function/tracepoint */
    char target_name[64];
    uint8_t active;
} bpf_attach_t;

/* eBPF helper function type */
typedef uint64_t (*bpf_helper_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

/* eBPF verifier state */
typedef struct {
    uint8_t reg_type[BPF_REG_COUNT];  /* Type of each register */
    int32_t reg_value[BPF_REG_COUNT]; /* Known value (if constant) */
    uint32_t stack_slots[BPF_STACK_SIZE / 8];  /* Stack slot types */
} bpf_verifier_state_t;

/* Register types for verifier */
#define BPF_REG_TYPE_UNINIT    0
#define BPF_REG_TYPE_PTR       1
#define BPF_REG_TYPE_SCALAR    2
#define BPF_REG_TYPE_CTX       3
#define BPF_REG_TYPE_MAP_PTR   4
#define BPF_REG_TYPE_STACK_PTR 5

/* Initialization and cleanup */
int bpf_subsystem_init(void);
void bpf_subsystem_cleanup(void);

/* Program management */
int bpf_prog_load(bpf_prog_type_t type, bpf_insn_t* insns, uint32_t insn_count, 
                  const char* name, bpf_prog_t** out_prog);
int bpf_prog_unload(uint32_t prog_id);
bpf_prog_t* bpf_prog_find(uint32_t prog_id);

/* Program verification */
int bpf_prog_verify(bpf_prog_t* prog);

/* Program execution */
int bpf_prog_run(bpf_prog_t* prog, void* ctx, uint64_t* result);

/* Map management */
int bpf_map_create(bpf_map_type_t type, uint32_t key_size, uint32_t value_size,
                   uint32_t max_entries, bpf_map_t** out_map);
int bpf_map_destroy(uint32_t map_id);
bpf_map_t* bpf_map_find(uint32_t map_id);

/* Map operations */
int bpf_map_lookup(bpf_map_t* map, const void* key, void* value);
int bpf_map_update(bpf_map_t* map, const void* key, const void* value, uint64_t flags);
int bpf_map_delete(bpf_map_t* map, const void* key);
int bpf_map_get_next_key(bpf_map_t* map, const void* key, void* next_key);

/* Attachment */
int bpf_prog_attach(uint32_t prog_id, bpf_prog_type_t type, void* target, const char* name);
int bpf_prog_detach(uint32_t prog_id, void* target);

/* Helper functions */
int bpf_register_helper(uint32_t helper_id, bpf_helper_fn_t fn);

/* JIT compilation */
int bpf_prog_jit_compile(bpf_prog_t* prog);

/* Tracing helpers */
int bpf_trace_printk(const char* fmt, uint64_t arg1, uint64_t arg2, uint64_t arg3);
int bpf_get_current_pid_tgid(uint64_t* pid_tgid);
int bpf_get_current_uid_gid(uint64_t* uid_gid);
int bpf_ktime_get_ns(uint64_t* time);
int bpf_get_current_comm(char* buf, uint32_t size);

/* Ring buffer helpers */
int bpf_ringbuf_reserve(bpf_map_t* ringbuf, uint32_t size, void** data);
int bpf_ringbuf_submit(void* data, uint64_t flags);
int bpf_ringbuf_discard(void* data, uint64_t flags);

#endif /* AURORA_EBPF_H */
