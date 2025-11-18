/**
 * Aurora OS - File System I/O Optimization Header
 */

#ifndef FS_OPTIMIZATION_H
#define FS_OPTIMIZATION_H

typedef struct {
    int readahead_enabled;
    int write_back_cache;
    int async_io;
    int io_scheduler_enabled;
    int parallel_io;
} fs_opt_state_t;

// Function prototypes
int fs_optimization_init(void);
void fs_enable_readahead(void);
void fs_enable_write_back_cache(void);
void fs_enable_async_io(void);
void fs_enable_io_scheduler(void);

#endif // FS_OPTIMIZATION_H
