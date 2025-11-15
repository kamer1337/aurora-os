/**
 * Aurora OS - Journaling Layer Header
 * 
 * Transaction journaling for file system integrity
 */

#ifndef AURORA_JOURNAL_H
#define AURORA_JOURNAL_H

#include <stdint.h>
#include <stddef.h>

/* Journal configuration */
#define JOURNAL_MAX_TRANSACTIONS 256
#define JOURNAL_MAX_OPERATIONS 64
#define JOURNAL_BLOCK_SIZE 512

/* Transaction states */
typedef enum {
    TRANSACTION_PENDING,
    TRANSACTION_COMMITTED,
    TRANSACTION_COMPLETED,
    TRANSACTION_ABORTED
} transaction_state_t;

/* Operation types */
typedef enum {
    JOURNAL_OP_CREATE,
    JOURNAL_OP_DELETE,
    JOURNAL_OP_WRITE,
    JOURNAL_OP_METADATA
} journal_op_type_t;

/* Journal operation */
typedef struct journal_operation {
    journal_op_type_t type;
    uint32_t block_num;
    void* old_data;
    void* new_data;
    size_t data_size;
} journal_operation_t;

/* Transaction */
typedef struct transaction {
    uint32_t txn_id;
    transaction_state_t state;
    uint32_t timestamp;
    journal_operation_t operations[JOURNAL_MAX_OPERATIONS];
    uint32_t op_count;
} transaction_t;

/* Journal superblock */
typedef struct journal_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t next_txn_id;
} journal_superblock_t;

/* Journal functions */
void journal_init(void);
void journal_enable(void);
void journal_disable(void);
int journal_is_enabled(void);
transaction_t* journal_begin_transaction(void);
int journal_add_operation(transaction_t* txn, journal_operation_t* op);
int journal_commit_transaction(transaction_t* txn);
int journal_abort_transaction(transaction_t* txn);
int journal_replay(void);

/* Recovery functions */
int journal_recover(void);
int journal_checkpoint(void);

/* Helper functions to create journal operations */
journal_operation_t journal_create_write_op(uint32_t block_num, void* old_data, void* new_data, size_t size);
journal_operation_t journal_create_metadata_op(uint32_t block_num, void* old_data, void* new_data, size_t size);
journal_operation_t journal_create_create_op(uint32_t block_num);
journal_operation_t journal_create_delete_op(uint32_t block_num);

#endif /* AURORA_JOURNAL_H */
