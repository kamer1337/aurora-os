/**
 * Aurora OS - Kernel Core Header
 * 
 * Main kernel header file containing core definitions and structures
 */

#ifndef AURORA_KERNEL_H
#define AURORA_KERNEL_H

#include <stdint.h>
#include <stddef.h>

/* Kernel version information */
#define AURORA_VERSION_MAJOR 0
#define AURORA_VERSION_MINOR 1
#define AURORA_VERSION_PATCH 0

/* Kernel status codes */
#define AURORA_SUCCESS 0
#define AURORA_ERROR -1

/* Basic type definitions */
typedef uint32_t process_id_t;
typedef uint32_t thread_id_t;

/* Kernel initialization */
void kernel_init(void);
void kernel_main(uint64_t magic, uint64_t multiboot_addr);

/* Driver initialization */
void drivers_init(void);

#endif /* AURORA_KERNEL_H */
