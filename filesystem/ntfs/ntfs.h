/**
 * Aurora OS - NTFS Read-Only File System Driver
 * 
 * NTFS read support for dual-boot scenarios with Windows
 */

#ifndef AURORA_NTFS_H
#define AURORA_NTFS_H

#include <stdint.h>
#include <stddef.h>
#include "../vfs/vfs.h"

/* NTFS constants */
#define NTFS_SIGNATURE          "NTFS    "
#define NTFS_SECTOR_SIZE        512
#define NTFS_MFT_ENTRY_SIZE     1024
#define NTFS_MAX_PATH           32767

/* File record flags */
#define NTFS_FILE_RECORD_IN_USE     0x0001
#define NTFS_FILE_RECORD_IS_DIR     0x0002
#define NTFS_FILE_RECORD_IS_4       0x0004
#define NTFS_FILE_RECORD_IS_VIEW    0x0008

/* Attribute types */
#define NTFS_ATTR_STANDARD_INFO     0x10
#define NTFS_ATTR_ATTR_LIST         0x20
#define NTFS_ATTR_FILE_NAME         0x30
#define NTFS_ATTR_OBJECT_ID         0x40
#define NTFS_ATTR_SECURITY_DESC     0x50
#define NTFS_ATTR_VOLUME_NAME       0x60
#define NTFS_ATTR_VOLUME_INFO       0x70
#define NTFS_ATTR_DATA              0x80
#define NTFS_ATTR_INDEX_ROOT        0x90
#define NTFS_ATTR_INDEX_ALLOC       0xA0
#define NTFS_ATTR_BITMAP            0xB0
#define NTFS_ATTR_REPARSE_POINT     0xC0
#define NTFS_ATTR_EA_INFO           0xD0
#define NTFS_ATTR_EA                0xE0
#define NTFS_ATTR_END               0xFFFFFFFF

/* File name flags */
#define NTFS_FNAME_POSIX            0x00
#define NTFS_FNAME_WIN32            0x01
#define NTFS_FNAME_DOS              0x02
#define NTFS_FNAME_WIN32_AND_DOS    0x03

/* NTFS boot sector */
typedef struct __attribute__((packed)) {
    uint8_t  jump_boot[3];
    uint8_t  oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  zeros1[3];
    uint16_t zeros2;
    uint8_t  media_descriptor;
    uint16_t zeros3;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t zeros4;
    uint32_t zeros5;
    uint64_t total_sectors;
    uint64_t mft_cluster;
    uint64_t mft_mirror_cluster;
    int8_t   clusters_per_mft_record;
    uint8_t  reserved1[3];
    int8_t   clusters_per_index_buffer;
    uint8_t  reserved2[3];
    uint64_t volume_serial_number;
    uint32_t checksum;
    uint8_t  boot_code[426];
    uint16_t end_marker;
} ntfs_boot_sector_t;

/* MFT record header */
typedef struct __attribute__((packed)) {
    uint32_t magic;                 /* "FILE" */
    uint16_t update_seq_offset;
    uint16_t update_seq_size;
    uint64_t log_file_seq_number;
    uint16_t sequence_number;
    uint16_t hard_link_count;
    uint16_t first_attr_offset;
    uint16_t flags;
    uint32_t used_size;
    uint32_t allocated_size;
    uint64_t base_record;
    uint16_t next_attr_id;
    uint16_t reserved;
    uint32_t mft_record_number;
} ntfs_mft_record_t;

/* Attribute header (resident) */
typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t length;
    uint8_t  non_resident;
    uint8_t  name_length;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attribute_id;
    /* Resident specific */
    uint32_t value_length;
    uint16_t value_offset;
    uint8_t  indexed_flag;
    uint8_t  padding;
} ntfs_attr_resident_t;

/* Attribute header (non-resident) */
typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t length;
    uint8_t  non_resident;
    uint8_t  name_length;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attribute_id;
    /* Non-resident specific */
    uint64_t starting_vcn;
    uint64_t ending_vcn;
    uint16_t data_runs_offset;
    uint16_t compression_unit;
    uint32_t padding;
    uint64_t allocated_size;
    uint64_t real_size;
    uint64_t initialized_size;
} ntfs_attr_nonresident_t;

/* Standard information attribute */
typedef struct __attribute__((packed)) {
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t mft_modification_time;
    uint64_t access_time;
    uint32_t file_attributes;
    uint32_t reserved;
} ntfs_standard_info_t;

/* File name attribute */
typedef struct __attribute__((packed)) {
    uint64_t parent_directory;
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t mft_modification_time;
    uint64_t access_time;
    uint64_t allocated_size;
    uint64_t real_size;
    uint32_t flags;
    uint32_t reparse_value;
    uint8_t  filename_length;
    uint8_t  filename_namespace;
    uint16_t filename[1];  /* Variable length */
} ntfs_filename_attr_t;

/* NTFS mount information */
typedef struct {
    uint64_t mft_cluster;
    uint64_t mft_mirror_cluster;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    int32_t  clusters_per_mft_record;
    uint32_t mft_record_size;
    void*    device;
} ntfs_mount_t;

/* NTFS initialization */
void ntfs_init(void);
int ntfs_mount_device(const char* device);
int ntfs_unmount_device(void);

/* Get NTFS file system operations */
fs_ops_t* ntfs_get_ops(void);

/* NTFS utility functions */
int ntfs_read_mft_record(ntfs_mount_t* mount, uint64_t mft_num, void* buffer);
void* ntfs_find_attribute(ntfs_mft_record_t* mft, uint32_t attr_type);
int ntfs_read_attribute(ntfs_mount_t* mount, ntfs_mft_record_t* mft, 
                        uint32_t attr_type, void* buffer, size_t size);

#endif /* AURORA_NTFS_H */
