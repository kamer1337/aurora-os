# Aurora OS - Windows Library Support Documentation

## Overview

Aurora OS now includes support for loading and running Windows PE (Portable Executable) binaries through a compatibility layer that emulates Windows DLL functions and system calls.

## Architecture

The Windows library support is organized into three main components:

### 1. PE/COFF Binary Loader (`kernel/core/pe_loader.c`)

The PE loader provides functionality to parse and load Windows executable files in PE/COFF format.

**Features:**
- DOS header validation
- PE signature verification
- COFF header parsing
- Optional header processing (PE32 format)
- Section loading (code, data, BSS)
- Import/export table parsing
- Relocation support (foundation)

**Key Functions:**
```c
void pe_loader_init(void);                          // Initialize PE loader
int pe_validate(const void* data, uint32_t size);  // Validate PE format
int pe_load(const void* data, uint32_t size, pe_image_t* image);  // Load PE image
void pe_unload(pe_image_t* image);                 // Unload PE image
void* pe_get_entry_point(pe_image_t* image);       // Get entry point
int pe_resolve_imports(pe_image_t* image);         // Resolve imports
```

**Supported PE Format:**
- PE32 (32-bit) executables
- Machine types: i386, AMD64
- Standard PE sections (.text, .data, .bss, .rdata, etc.)

### 2. DLL Loader (`kernel/core/dll_loader.c`)

The DLL loader manages dynamic link libraries with reference counting and module management.

**Features:**
- DLL loading with reference counting
- Module handle management
- GetProcAddress functionality (foundation)
- DLL search path support
- Automatic unloading when reference count reaches zero

**Key Functions:**
```c
void dll_loader_init(void);                         // Initialize DLL loader
HMODULE dll_load(const char* dll_name);            // Load a DLL
void* dll_get_proc_address(HMODULE module, const char* proc_name);  // Get function address
int dll_free(HMODULE module);                       // Free a DLL
HMODULE dll_get_module_handle(const char* dll_name);  // Get module handle
int dll_is_loaded(const char* dll_name);           // Check if DLL is loaded
```

**DLL Management:**
- Maximum 64 simultaneously loaded DLLs
- Automatic reference counting
- Module handle validation
- Thread-safe operations (foundation)

### 3. Windows API Compatibility Layer (`kernel/core/winapi/`)

The WinAPI compatibility layer provides stub implementations of common Windows API functions.

#### Core API (`winapi.h`, `winapi.c`)

**Types:**
- Standard Windows types (HANDLE, DWORD, BOOL, LPVOID, etc.)
- Error codes (ERROR_SUCCESS, ERROR_FILE_NOT_FOUND, etc.)
- File access modes and attributes
- Memory allocation flags

**Functions:**
```c
void winapi_init(void);                    // Initialize WinAPI layer
DWORD winapi_get_last_error(void);         // Get last error
void winapi_set_last_error(DWORD error);   // Set last error
```

#### Kernel32.dll API (`kernel32.h`, `kernel32.c`)

**Process and Thread Functions:**
- `GetCurrentProcess()` - Get current process handle
- `GetCurrentProcessId()` - Get current process ID
- `GetCurrentThread()` - Get current thread handle
- `GetCurrentThreadId()` - Get current thread ID
- `ExitProcess(exit_code)` - Exit current process
- `TerminateProcess(process, exit_code)` - Terminate a process

**Memory Management Functions:**
- `VirtualAlloc(address, size, alloc_type, protect)` - Allocate virtual memory
- `VirtualFree(address, size, free_type)` - Free virtual memory
- `GlobalAlloc(flags, bytes)` - Allocate global memory
- `GlobalFree(mem)` - Free global memory
- `LocalAlloc(flags, bytes)` - Allocate local memory
- `LocalFree(mem)` - Free local memory

**File Management Functions:**
- `CreateFileA(filename, access, share_mode, security, creation, flags, template)` - Create/open file
- `ReadFile(file, buffer, bytes_to_read, bytes_read, overlapped)` - Read from file
- `WriteFile(file, buffer, bytes_to_write, bytes_written, overlapped)` - Write to file
- `CloseHandle(handle)` - Close handle
- `GetFileSize(file, high_size)` - Get file size
- `DeleteFileA(filename)` - Delete file

**Console Functions:**
- `GetStdHandle(std_handle)` - Get standard handle (stdin/stdout/stderr)
- `WriteConsoleA(console, buffer, length, written, reserved)` - Write to console

**String Functions:**
- `lstrlenA(string)` - Get string length
- `lstrcpyA(dest, src)` - Copy string
- `lstrcatA(dest, src)` - Concatenate string
- `lstrcmpA(str1, str2)` - Compare strings

**Module Loading Functions:**
- `LoadLibraryA(lib_filename)` - Load a DLL
- `FreeLibrary(module)` - Free a DLL
- `GetProcAddress(module, proc_name)` - Get function address from DLL
- `GetModuleHandleA(module_name)` - Get module handle

**Error Functions:**
- `GetLastError()` - Get last error code
- `SetLastError(error_code)` - Set last error code

**System Information Functions:**
- `GetSystemInfo(system_info)` - Get system information
- `GetTickCount()` - Get tick count

## Usage Examples

### Loading a Windows Binary

```c
#include "pe_loader.h"

// Load PE binary from memory
void* pe_data = /* load from file */;
uint32_t pe_size = /* file size */;

pe_image_t image;
if (pe_load(pe_data, pe_size, &image) == 0) {
    void* entry_point = pe_get_entry_point(&image);
    
    // Resolve imports
    pe_resolve_imports(&image);
    
    // Execute
    void (*main_func)(void) = (void (*)(void))entry_point;
    main_func();
    
    // Cleanup
    pe_unload(&image);
}
```

### Loading a DLL

```c
#include "dll_loader.h"
#include "winapi/kernel32.h"

// Load DLL
HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
if (hKernel32) {
    // Get function address
    FARPROC func = GetProcAddress(hKernel32, "CreateFileA");
    
    // Use function...
    
    // Free DLL
    FreeLibrary(hKernel32);
}
```

### Using WinAPI Functions

```c
#include "winapi/kernel32.h"

// Allocate memory
LPVOID buffer = VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
if (buffer) {
    // Use buffer...
    
    // Free memory
    VirtualFree(buffer, 0, MEM_RELEASE);
}

// Console output
HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
const char* msg = "Hello from WinAPI\n";
DWORD written;
WriteConsoleA(hStdOut, msg, lstrlenA(msg), &written, NULL);
```

## Testing

A comprehensive test suite is provided in `tests/pe_loader_tests.c`:

```c
// Run all Windows library tests
run_pe_loader_tests();   // Test PE loader functionality
run_dll_loader_tests();  // Test DLL loading
run_winapi_tests();      // Test WinAPI functions
```

**Test Coverage:**
- PE format validation (3 tests)
- PE loader initialization
- DLL loading and unloading (7 tests)
- Module handle management
- WinAPI error handling (2 tests)
- Memory allocation functions (6 tests)
- Process/thread functions (4 tests)
- String functions (5 tests)
- Module loading functions (3 tests)
- Console functions (2 tests)

Total: 30+ tests

## Current Limitations

1. **File Loading**: Currently, PE/DLL files must be loaded from memory. File system integration is pending.

2. **Import Resolution**: Import table parsing is implemented, but actual symbol resolution requires:
   - Export table parsing in loaded DLLs
   - Symbol name/ordinal lookup
   - IAT (Import Address Table) patching

3. **Relocation**: Base relocation support is planned but not yet implemented.

4. **Threading**: Multi-threaded Windows applications require additional synchronization primitives.

5. **Advanced APIs**: Currently only Kernel32.dll basic functions are stubbed. Additional DLLs needed:
   - User32.dll (GUI functions)
   - NTDLL.dll (NT native API)
   - GDI32.dll (Graphics)
   - WS2_32.dll (Networking)

6. **Security**: No implementation of Windows security features (ACLs, tokens, etc.)

## Future Enhancements

1. **Complete Import Resolution**
   - Full symbol table parsing
   - Ordinal-based lookup
   - IAT patching
   - Delayed import support

2. **Additional DLL Support**
   - User32.dll for GUI applications
   - NTDLL.dll for native API
   - GDI32.dll for graphics
   - WS2_32.dll for networking

3. **Enhanced File Support**
   - File system integration
   - DLL search path implementation
   - Side-by-side assembly support

4. **Threading Support**
   - CreateThread implementation
   - Thread synchronization primitives
   - Thread Local Storage (TLS)

5. **Exception Handling**
   - Structured Exception Handling (SEH)
   - Vectored Exception Handling (VEH)

6. **Performance Optimization**
   - JIT compilation for system calls
   - Caching of frequently used functions
   - Optimized memory allocation

## Integration with Aurora OS

The Windows library support is integrated into the Aurora OS kernel initialization:

```c
void kernel_init(void) {
    // ... other initialization ...
    
    /* Initialize Windows binary support */
    pe_loader_init();
    dll_loader_init();
    winapi_init();
    kernel32_init();
    
    // ... continue initialization ...
}
```

Tests are automatically run during system boot:

```c
/* Run Windows library support tests */
run_pe_loader_tests();
run_dll_loader_tests();
run_winapi_tests();
```

## References

- [PE Format Specification](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Windows API Documentation](https://docs.microsoft.com/en-us/windows/win32/api/)
- [DLL Best Practices](https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-libraries)

## Contributing

To add support for additional Windows APIs:

1. Add function declarations to appropriate header (`kernel32.h`, etc.)
2. Implement stub functions in corresponding `.c` file
3. Add tests to `tests/pe_loader_tests.c`
4. Update this documentation

## License

This implementation is part of Aurora OS and follows the project's licensing terms.
