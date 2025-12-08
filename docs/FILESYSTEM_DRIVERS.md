# Aurora OS - File System Drivers Documentation

## Overview

Aurora OS now supports multiple file system types for maximum compatibility and advanced features:

1. **FAT32** - Windows/DOS compatible file system
2. **NTFS** - Windows NT file system (read-only)
3. **AuroraFS** - Custom advanced file system with modern features
4. **NFS** - Network File System client
5. **CIFS/SMB** - Windows network shares client

---

## FAT32 File System Driver

### Features
- Full read/write support
- Compatible with Windows, Linux, and macOS
- Long filename support (LFN)
- Suitable for removable media (USB drives, SD cards)

### Usage

```c
#include "filesystem/fat32/fat32.h"

// Initialize FAT32 driver
fat32_init();

// Register with VFS
vfs_register_fs("fat32", fat32_get_ops());

// Mount FAT32 device
vfs_mount("/dev/sda1", "/mnt/usb", "fat32");

// Use standard VFS operations
int fd = vfs_open("/mnt/usb/file.txt", O_RDONLY);
```

### Technical Details
- **Block Size**: 512 bytes (sector size)
- **Max File Size**: 4 GB
- **Max Volume Size**: 2 TB
- **Directory Entries**: 8.3 format with LFN support
- **Cluster Sizes**: 512B to 32KB

### Limitations
- No Unix-style permissions
- No ownership information
- File size limited to 4 GB
- No journaling support

---

## NTFS File System Driver

### Features
- **Read-only** support for Windows NTFS volumes
- Dual-boot compatibility
- MFT (Master File Table) support
- Resident and non-resident attributes
- Compressed file support (future)
- Encrypted file support (future)

### Usage

```c
#include "filesystem/ntfs/ntfs.h"

// Initialize NTFS driver
ntfs_init();

// Register with VFS
vfs_register_fs("ntfs", ntfs_get_ops());

// Mount NTFS volume (read-only)
vfs_mount("/dev/sda2", "/mnt/windows", "ntfs");

// Read files only
int fd = vfs_open("/mnt/windows/Documents/file.txt", O_RDONLY);
```

### Technical Details
- **MFT Entry Size**: 1024 bytes
- **Cluster Size**: Configurable (typically 4 KB)
- **Max File Size**: 16 EB
- **Max Volume Size**: 256 TB
- **Attribute Types**: Standard info, filename, data, index

### Limitations
- **Read-only** - no write operations
- No file creation or deletion
- No permission modifications
- Simplified implementation (basic features only)

---

## AuroraFS Advanced File System

### Features

AuroraFS is a modern, feature-rich file system designed for Aurora OS with:

#### 1. **Deduplication**
- Block-level deduplication using SHA-256 hashes
- Reference counting for shared blocks
- Automatic space savings

#### 2. **Compression**
- Multiple algorithms: LZ4, ZSTD, LZMA
- Per-file compression settings
- Transparent compression/decompression

#### 3. **Snapshots**
- Copy-on-write snapshots
- Instant snapshot creation
- Snapshot rollback support
- Space-efficient storage

#### 4. **Encryption at Rest**
- AES-256 encryption
- ChaCha20 support
- Per-file encryption keys
- Master key derivation

### Usage

```c
#include "filesystem/aurorafs/aurorafs.h"

// Initialize AuroraFS
aurorafs_init();

// Format new AuroraFS volume
uint32_t features = AURORAFS_FEAT_DEDUP | 
                    AURORAFS_FEAT_COMPRESS | 
                    AURORAFS_FEAT_SNAPSHOT | 
                    AURORAFS_FEAT_ENCRYPT;
aurorafs_format("/dev/sdb1", 1024*1024*1024, features);

// Register with VFS
vfs_register_fs("aurorafs", aurorafs_get_ops());

// Mount with encryption key
uint8_t master_key[32] = { /* your key */ };
aurorafs_mount_device("/dev/sdb1", master_key);

// Use standard VFS operations
int fd = vfs_open("/data/document.txt", O_RDWR | O_CREAT);
```

### Deduplication Example

```c
// Deduplication is automatic
// Write duplicate data
vfs_write(fd1, data, size);  // Original
vfs_write(fd2, data, size);  // Deduplicated - shares blocks
```

### Compression Example

```c
// Compress a block manually
uint8_t input[4096];
uint8_t output[4096];
size_t output_size;

aurorafs_compress_block(input, 4096, output, &output_size, 
                        AURORAFS_COMPRESS_LZ4);
```

### Snapshot Example

```c
// Create snapshot
aurorafs_create_snapshot(mount, "backup-2024", 
                         "System backup before update");

// List snapshots
aurorafs_snapshot_t snapshots[256];
uint32_t count;
aurorafs_list_snapshots(mount, snapshots, &count);

// Rollback to snapshot
aurorafs_rollback_snapshot(mount, snapshot_id);
```

### Encryption Example

```c
// Encryption is automatic when enabled
// Master key protects individual file keys
uint8_t master_key[32];
// ... initialize master key ...

// Files are automatically encrypted/decrypted
int fd = vfs_open("/encrypted/secret.txt", O_RDWR);
vfs_write(fd, plaintext, size);  // Encrypted on disk
vfs_read(fd, buffer, size);       // Decrypted to buffer
```

### Technical Details
- **Block Size**: 4096 bytes
- **Max File Size**: 16 EB
- **Max Volume Size**: 16 EB
- **Hash Algorithm**: SHA-256
- **Compression**: LZ4 (default), ZSTD, LZMA
- **Encryption**: AES-256, ChaCha20
- **Max Snapshots**: 256

---

## NFS Client

### Features
- NFSv3 protocol support
- TCP transport
- Remote file access
- Mount protocol support

### Usage

```c
#include "filesystem/network/nfs_client.h"

// Initialize NFS client
nfs_client_init();

// Register with VFS
vfs_register_fs("nfs", nfs_get_ops());

// Mount NFS share
vfs_mount("server:/export/path", "/mnt/nfs", "nfs");

// Use standard VFS operations
int fd = vfs_open("/mnt/nfs/shared/file.txt", O_RDONLY);
```

### Technical Details
- **Protocol**: NFSv3 over TCP
- **Port**: 2049
- **RPC**: Sun RPC (ONC RPC)
- **Authentication**: AUTH_UNIX
- **Operations**: LOOKUP, READ, WRITE, CREATE, REMOVE, READDIR

---

## CIFS/SMB Client

### Features
- SMB2/SMB3 protocol support
- Windows network shares
- NTLM authentication
- Directory browsing

### Usage

```c
#include "filesystem/network/cifs_client.h"

// Initialize CIFS client
cifs_client_init();

// Register with VFS
vfs_register_fs("cifs", cifs_get_ops());

// Mount CIFS share
vfs_mount("//server/share", "/mnt/windows", "cifs");

// Use standard VFS operations
int fd = vfs_open("/mnt/windows/Documents/file.txt", O_RDONLY);
```

### Authentication

```c
// Mount with credentials
cifs_connect("server", "share", "username", "password");
```

### Technical Details
- **Protocol**: SMB2/SMB3
- **Port**: 445 (SMB), 139 (NetBIOS)
- **Authentication**: NTLMSSP
- **Encryption**: SMB3 encryption support
- **Operations**: CREATE, CLOSE, READ, WRITE, QUERY_DIRECTORY

---

## VFS Integration

All file systems integrate seamlessly with the VFS layer:

```c
// Initialize VFS
vfs_init();

// Register all filesystems
vfs_register_fs("fat32", fat32_get_ops());
vfs_register_fs("ntfs", ntfs_get_ops());
vfs_register_fs("aurorafs", aurorafs_get_ops());
vfs_register_fs("nfs", nfs_get_ops());
vfs_register_fs("cifs", cifs_get_ops());

// Mount filesystems
vfs_mount("/dev/sda1", "/", "aurorafs");        // Root
vfs_mount("/dev/sdb1", "/usb", "fat32");        // USB drive
vfs_mount("/dev/sdc1", "/windows", "ntfs");     // Windows partition
vfs_mount("server:/share", "/nfs", "nfs");      // NFS share
vfs_mount("//server/share", "/smb", "cifs");    // Windows share
```

---

## Performance Comparison

| File System | Read Speed | Write Speed | Features | Best Use Case |
|-------------|-----------|-------------|----------|---------------|
| FAT32       | Good      | Good        | Basic    | USB drives, compatibility |
| NTFS        | Good      | N/A (RO)    | Medium   | Dual-boot, read Windows |
| AuroraFS    | Excellent | Excellent   | Advanced | System disk, data storage |
| NFS         | Network   | Network     | Network  | Remote file access |
| CIFS/SMB    | Network   | Network     | Network  | Windows file shares |

---

## Security Considerations

### FAT32
- No built-in security
- Suitable for non-sensitive data
- Physical access = full access

### NTFS
- Read-only prevents modifications
- NTFS permissions visible but not enforced
- Encrypted files not supported

### AuroraFS
- Strong encryption (AES-256)
- Secure key derivation
- Block-level deduplication security
- Snapshot integrity

### Network Filesystems
- Use secure channels (VPN recommended)
- NTLM authentication for CIFS
- Consider network security policies

---

## Future Enhancements

### Planned Features
- [ ] FAT32 file fragmentation optimization
- [ ] NTFS write support (when stable)
- [ ] AuroraFS B-tree index optimization
- [ ] AuroraFS online resize
- [ ] NFS v4 support
- [ ] SMB3 encryption
- [ ] Distributed filesystem (cluster support)

---

## Testing

Run filesystem tests:

```bash
# Build with tests
make test

# Run specific filesystem tests
./tests/filesystem_tests
```

All filesystem drivers include comprehensive test suites covering:
- Initialization and registration
- Mount/unmount operations
- File operations (read/write/create/delete)
- Directory operations
- Feature-specific tests (compression, encryption, etc.)

---

## Troubleshooting

### FAT32
- **Issue**: Cannot write to file
- **Solution**: Check if filesystem is mounted read-write

### NTFS
- **Issue**: Cannot modify files
- **Solution**: NTFS is read-only by design

### AuroraFS
- **Issue**: Cannot decrypt files
- **Solution**: Verify master key is correct

### Network FS
- **Issue**: Connection timeout
- **Solution**: Check network connectivity and firewall rules

---

## API Reference

See individual header files for complete API documentation:
- `filesystem/fat32/fat32.h`
- `filesystem/ntfs/ntfs.h`
- `filesystem/aurorafs/aurorafs.h`
- `filesystem/network/nfs_client.h`
- `filesystem/network/cifs_client.h`
