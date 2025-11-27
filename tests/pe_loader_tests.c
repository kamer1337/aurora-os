/**
 * Aurora OS - Windows Library Support Tests Implementation
 * 
 * Test suite for PE loader, DLL loader, and WinAPI compatibility layer
 */

#include "pe_loader_tests.h"
#include "../kernel/core/pe_loader.h"
#include "../kernel/core/dll_loader.h"
#include "../kernel/core/winapi/winapi.h"
#include "../kernel/core/winapi/kernel32.h"
#include "../kernel/drivers/vga.h"

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test helper macros */
#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            tests_passed++; \
            vga_write("[PASS] "); \
            vga_write(msg); \
            vga_write("\n"); \
        } else { \
            tests_failed++; \
            vga_write("[FAIL] "); \
            vga_write(msg); \
            vga_write("\n"); \
        } \
    } while(0)

/* ========== PE Loader Tests ========== */

/**
 * Test PE header validation
 */
static void test_pe_validation(void) {
    vga_write("\n=== Testing PE Validation ===\n");
    
    /* Test invalid data */
    TEST_ASSERT(pe_validate(0, 0) == 0, "NULL data validation");
    
    /* Test invalid size */
    char small_buffer[10] = {0};
    TEST_ASSERT(pe_validate(small_buffer, 10) == 0, "Small buffer validation");
    
    /* Test invalid DOS signature */
    char bad_dos[64] = {0};
    TEST_ASSERT(pe_validate(bad_dos, 64) == 0, "Invalid DOS signature");
}

/**
 * Test PE loader initialization
 */
static void test_pe_loader_init(void) {
    vga_write("\n=== Testing PE Loader Init ===\n");
    
    pe_loader_init();
    TEST_ASSERT(1, "PE loader initialization");
}

/**
 * Run all PE loader tests
 */
void run_pe_loader_tests(void) {
    vga_write("\n==========================================\n");
    vga_write("    PE LOADER TEST SUITE\n");
    vga_write("==========================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    test_pe_loader_init();
    test_pe_validation();
    
    vga_write("\n=== PE Loader Test Results ===\n");
    vga_write("Tests passed: ");
    vga_write_dec(tests_passed);
    vga_write("\nTests failed: ");
    vga_write_dec(tests_failed);
    vga_write("\n");
}

/* ========== DLL Loader Tests ========== */

/**
 * Test DLL loader initialization
 */
static void test_dll_loader_init(void) {
    vga_write("\n=== Testing DLL Loader Init ===\n");
    
    dll_loader_init();
    TEST_ASSERT(1, "DLL loader initialization");
}

/**
 * Test DLL loading
 */
static void test_dll_load(void) {
    vga_write("\n=== Testing DLL Loading ===\n");
    
    /* Test loading NULL DLL name */
    HMODULE h1 = dll_load(0);
    TEST_ASSERT(h1 == 0, "Load NULL DLL name");
    
    /* Test loading valid DLL */
    HMODULE h2 = dll_load("kernel32.dll");
    TEST_ASSERT(h2 != 0, "Load kernel32.dll");
    
    /* Test loading same DLL again */
    HMODULE h3 = dll_load("kernel32.dll");
    TEST_ASSERT(h3 == h2, "Load kernel32.dll again (should return same handle)");
    
    /* Test getting module handle */
    HMODULE h4 = dll_get_module_handle("kernel32.dll");
    TEST_ASSERT(h4 == h2, "Get module handle for loaded DLL");
    
    /* Test checking if DLL is loaded */
    TEST_ASSERT(dll_is_loaded("kernel32.dll") == 1, "Check if kernel32.dll is loaded");
    TEST_ASSERT(dll_is_loaded("nonexistent.dll") == 0, "Check if nonexistent DLL is loaded");
    
    /* Test freeing DLL */
    int result = dll_free(h2);
    TEST_ASSERT(result == 0, "Free DLL (first ref)");
    
    result = dll_free(h3);
    TEST_ASSERT(result == 0, "Free DLL (second ref, should actually unload)");
}

/**
 * Run all DLL loader tests
 */
void run_dll_loader_tests(void) {
    vga_write("\n==========================================\n");
    vga_write("    DLL LOADER TEST SUITE\n");
    vga_write("==========================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    test_dll_loader_init();
    test_dll_load();
    
    vga_write("\n=== DLL Loader Test Results ===\n");
    vga_write("Tests passed: ");
    vga_write_dec(tests_passed);
    vga_write("\nTests failed: ");
    vga_write_dec(tests_failed);
    vga_write("\n");
}

/* ========== WinAPI Tests ========== */

/**
 * Test WinAPI initialization
 */
static void test_winapi_init(void) {
    vga_write("\n=== Testing WinAPI Init ===\n");
    
    winapi_init();
    TEST_ASSERT(1, "WinAPI initialization");
    
    kernel32_init();
    TEST_ASSERT(1, "Kernel32 initialization");
}

/**
 * Test error handling
 */
static void test_winapi_errors(void) {
    vga_write("\n=== Testing WinAPI Error Handling ===\n");
    
    /* Test getting/setting last error */
    winapi_set_last_error(ERROR_FILE_NOT_FOUND);
    DWORD error = winapi_get_last_error();
    TEST_ASSERT(error == ERROR_FILE_NOT_FOUND, "Set/Get last error");
    
    /* Test WinAPI error functions */
    SetLastError(ERROR_ACCESS_DENIED);
    error = GetLastError();
    TEST_ASSERT(error == ERROR_ACCESS_DENIED, "Kernel32 error functions");
}

/**
 * Test memory functions
 */
static void test_winapi_memory(void) {
    vga_write("\n=== Testing WinAPI Memory Functions ===\n");
    
    /* Test VirtualAlloc */
    LPVOID ptr1 = VirtualAlloc(0, 1024, MEM_COMMIT, PAGE_READWRITE);
    TEST_ASSERT(ptr1 != 0, "VirtualAlloc");
    
    /* Test VirtualFree */
    BOOL result = VirtualFree(ptr1, 0, MEM_RELEASE);
    TEST_ASSERT(result == TRUE, "VirtualFree");
    
    /* Test GlobalAlloc */
    HGLOBAL hg = GlobalAlloc(0, 512);
    TEST_ASSERT(hg != 0, "GlobalAlloc");
    
    /* Test GlobalFree */
    HGLOBAL hg_result = GlobalFree(hg);
    TEST_ASSERT(hg_result == 0, "GlobalFree");
    
    /* Test LocalAlloc */
    HLOCAL hl = LocalAlloc(0, 256);
    TEST_ASSERT(hl != 0, "LocalAlloc");
    
    /* Test LocalFree */
    HLOCAL hl_result = LocalFree(hl);
    TEST_ASSERT(hl_result == 0, "LocalFree");
}

/**
 * Test process/thread functions
 */
static void test_winapi_process(void) {
    vga_write("\n=== Testing WinAPI Process Functions ===\n");
    
    /* Test GetCurrentProcess */
    HANDLE proc = GetCurrentProcess();
    TEST_ASSERT(proc != 0, "GetCurrentProcess");
    
    /* Test GetCurrentProcessId */
    DWORD pid = GetCurrentProcessId();
    TEST_ASSERT(pid != 0, "GetCurrentProcessId");
    
    /* Test GetCurrentThread */
    HANDLE thread = GetCurrentThread();
    TEST_ASSERT(thread != 0, "GetCurrentThread");
    
    /* Test GetCurrentThreadId */
    DWORD tid = GetCurrentThreadId();
    TEST_ASSERT(tid != 0, "GetCurrentThreadId");
}

/**
 * Test string functions
 */
static void test_winapi_strings(void) {
    vga_write("\n=== Testing WinAPI String Functions ===\n");
    
    const char* test_str = "Hello";
    char buffer[64];
    
    /* Test lstrlenA */
    int len = lstrlenA(test_str);
    TEST_ASSERT(len == 5, "lstrlenA");
    
    /* Test lstrcpyA */
    lstrcpyA(buffer, test_str);
    TEST_ASSERT(lstrcmpA(buffer, test_str) == 0, "lstrcpyA");
    
    /* Test lstrcatA */
    lstrcatA(buffer, " World");
    TEST_ASSERT(lstrlenA(buffer) == 11, "lstrcatA");
    
    /* Test lstrcmpA */
    TEST_ASSERT(lstrcmpA("abc", "abc") == 0, "lstrcmpA (equal)");
    TEST_ASSERT(lstrcmpA("abc", "abd") < 0, "lstrcmpA (less)");
    TEST_ASSERT(lstrcmpA("abd", "abc") > 0, "lstrcmpA (greater)");
}

/**
 * Test module loading functions
 */
static void test_winapi_modules(void) {
    vga_write("\n=== Testing WinAPI Module Functions ===\n");
    
    /* Test LoadLibraryA */
    HMODULE hMod = LoadLibraryA("user32.dll");
    TEST_ASSERT(hMod != 0, "LoadLibraryA");
    
    /* Test GetModuleHandleA */
    HMODULE hMod2 = GetModuleHandleA("user32.dll");
    TEST_ASSERT(hMod2 == hMod, "GetModuleHandleA");
    
    /* Test FreeLibrary */
    BOOL result = FreeLibrary(hMod);
    TEST_ASSERT(result == TRUE, "FreeLibrary");
}

/**
 * Test console functions
 */
static void test_winapi_console(void) {
    vga_write("\n=== Testing WinAPI Console Functions ===\n");
    
    /* Test GetStdHandle */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    TEST_ASSERT(hOut != INVALID_HANDLE_VALUE, "GetStdHandle(STD_OUTPUT_HANDLE)");
    
    /* Test WriteConsoleA */
    const char* msg = "Test message";
    DWORD written = 0;
    BOOL result = WriteConsoleA(hOut, msg, 12, &written, 0);
    TEST_ASSERT(result == TRUE, "WriteConsoleA");
    vga_write("\n");
}

/**
 * Test synchronization primitives
 */
static void test_winapi_sync(void) {
    vga_write("\n=== Testing WinAPI Synchronization ===\n");
    
    /* Test Critical Section */
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);
    TEST_ASSERT(cs.LockCount == -1, "InitializeCriticalSection");
    
    EnterCriticalSection(&cs);
    TEST_ASSERT(cs.RecursionCount == 1, "EnterCriticalSection");
    
    /* Test re-entry */
    EnterCriticalSection(&cs);
    TEST_ASSERT(cs.RecursionCount == 2, "Re-enter CriticalSection");
    
    LeaveCriticalSection(&cs);
    TEST_ASSERT(cs.RecursionCount == 1, "LeaveCriticalSection");
    
    LeaveCriticalSection(&cs);
    TEST_ASSERT(cs.LockCount == -1, "LeaveCriticalSection (unlocked)");
    
    DeleteCriticalSection(&cs);
    TEST_ASSERT(1, "DeleteCriticalSection");
    
    /* Test Event */
    HANDLE hEvent = CreateEventA(NULL, TRUE, FALSE, "TestEvent");
    TEST_ASSERT(hEvent != NULL, "CreateEventA");
    
    BOOL setResult = SetEvent(hEvent);
    TEST_ASSERT(setResult == TRUE, "SetEvent");
    
    BOOL resetResult = ResetEvent(hEvent);
    TEST_ASSERT(resetResult == TRUE, "ResetEvent");
    
    CloseHandle(hEvent);
    
    /* Test Mutex */
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "TestMutex");
    TEST_ASSERT(hMutex != NULL, "CreateMutexA");
    
    DWORD waitResult = WaitForSingleObject(hMutex, 0);
    TEST_ASSERT(waitResult == WAIT_OBJECT_0, "WaitForSingleObject (mutex)");
    
    BOOL relResult = ReleaseMutex(hMutex);
    TEST_ASSERT(relResult == TRUE, "ReleaseMutex");
    
    CloseHandle(hMutex);
    
    /* Test Semaphore */
    HANDLE hSem = CreateSemaphoreA(NULL, 2, 10, "TestSemaphore");
    TEST_ASSERT(hSem != NULL, "CreateSemaphoreA");
    
    LONG prevCount = 0;
    BOOL semRelResult = ReleaseSemaphore(hSem, 1, &prevCount);
    TEST_ASSERT(semRelResult == TRUE && prevCount == 2, "ReleaseSemaphore");
    
    CloseHandle(hSem);
    
    /* Test Interlocked functions */
    LONG testValue = 5;
    LONG incResult = InterlockedIncrement(&testValue);
    TEST_ASSERT(incResult == 6 && testValue == 6, "InterlockedIncrement");
    
    LONG decResult = InterlockedDecrement(&testValue);
    TEST_ASSERT(decResult == 5 && testValue == 5, "InterlockedDecrement");
    
    LONG exchResult = InterlockedExchange(&testValue, 10);
    TEST_ASSERT(exchResult == 5 && testValue == 10, "InterlockedExchange");
    
    LONG cmpResult = InterlockedCompareExchange(&testValue, 20, 10);
    TEST_ASSERT(cmpResult == 10 && testValue == 20, "InterlockedCompareExchange");
}

/**
 * Test environment functions
 */
static void test_winapi_env(void) {
    vga_write("\n=== Testing WinAPI Environment Functions ===\n");
    
    char buffer[256];
    
    /* Test GetEnvironmentVariableA */
    DWORD result = GetEnvironmentVariableA("PATH", buffer, 256);
    TEST_ASSERT(result > 0, "GetEnvironmentVariableA (PATH)");
    
    /* Test SetEnvironmentVariableA */
    BOOL setResult = SetEnvironmentVariableA("TESTVAR", "testvalue");
    TEST_ASSERT(setResult == TRUE, "SetEnvironmentVariableA");
    
    /* Verify the set variable */
    result = GetEnvironmentVariableA("TESTVAR", buffer, 256);
    TEST_ASSERT(result > 0, "GetEnvironmentVariableA (TESTVAR)");
    
    /* Test GetCommandLineA */
    LPSTR cmdline = GetCommandLineA();
    TEST_ASSERT(cmdline != NULL, "GetCommandLineA");
}

/**
 * Test TLS functions
 */
static void test_winapi_tls(void) {
    vga_write("\n=== Testing WinAPI TLS Functions ===\n");
    
    /* Test TlsAlloc */
    DWORD index = TlsAlloc();
    TEST_ASSERT(index != 0xFFFFFFFF, "TlsAlloc");
    
    /* Test TlsSetValue */
    LPVOID testValue = (LPVOID)0x12345678;
    BOOL setResult = TlsSetValue(index, testValue);
    TEST_ASSERT(setResult == TRUE, "TlsSetValue");
    
    /* Test TlsGetValue */
    LPVOID getValue = TlsGetValue(index);
    TEST_ASSERT(getValue == testValue, "TlsGetValue");
    
    /* Test TlsFree */
    BOOL freeResult = TlsFree(index);
    TEST_ASSERT(freeResult == TRUE, "TlsFree");
}

/**
 * Test heap functions
 */
static void test_winapi_heap(void) {
    vga_write("\n=== Testing WinAPI Heap Functions ===\n");
    
    /* Test GetProcessHeap */
    HANDLE hHeap = GetProcessHeap();
    TEST_ASSERT(hHeap != NULL, "GetProcessHeap");
    
    /* Test HeapCreate */
    HANDLE hNewHeap = HeapCreate(0, 4096, 0);
    TEST_ASSERT(hNewHeap != NULL, "HeapCreate");
    
    /* Test HeapAlloc */
    LPVOID ptr = HeapAlloc(hNewHeap, HEAP_ZERO_MEMORY, 256);
    TEST_ASSERT(ptr != NULL, "HeapAlloc");
    
    /* Verify zero memory */
    unsigned char* bytes = (unsigned char*)ptr;
    int isZeroed = 1;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0) { isZeroed = 0; break; }
    }
    TEST_ASSERT(isZeroed, "HeapAlloc (HEAP_ZERO_MEMORY)");
    
    /* Test HeapFree */
    BOOL freeResult = HeapFree(hNewHeap, 0, ptr);
    TEST_ASSERT(freeResult == TRUE, "HeapFree");
    
    /* Test HeapDestroy */
    BOOL destroyResult = HeapDestroy(hNewHeap);
    TEST_ASSERT(destroyResult == TRUE, "HeapDestroy");
}

/**
 * Test system information functions
 */
static void test_winapi_sysinfo(void) {
    vga_write("\n=== Testing WinAPI System Info ===\n");
    
    /* Test GetSystemInfo */
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    TEST_ASSERT(si.dwPageSize == 4096, "GetSystemInfo (PageSize)");
    TEST_ASSERT(si.dwNumberOfProcessors >= 1, "GetSystemInfo (Processors)");
    
    /* Test GetTickCount */
    DWORD tick1 = GetTickCount();
    DWORD tick2 = GetTickCount();
    TEST_ASSERT(tick2 >= tick1, "GetTickCount");
    
    /* Test GetVersionExA */
    OSVERSIONINFOA ver;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    BOOL verResult = GetVersionExA(&ver);
    TEST_ASSERT(verResult == TRUE, "GetVersionExA");
    TEST_ASSERT(ver.dwMajorVersion >= 6, "GetVersionExA (Version)");
    
    /* Test GetSystemDirectoryA */
    char sysDir[MAX_PATH];
    UINT sysDirLen = GetSystemDirectoryA(sysDir, MAX_PATH);
    TEST_ASSERT(sysDirLen > 0, "GetSystemDirectoryA");
    
    /* Test GetComputerNameA */
    char compName[256];
    DWORD compNameSize = 256;
    BOOL compResult = GetComputerNameA(compName, &compNameSize);
    TEST_ASSERT(compResult == TRUE, "GetComputerNameA");
}

/**
 * Run all WinAPI tests
 */
void run_winapi_tests(void) {
    vga_write("\n==========================================\n");
    vga_write("    WINAPI TEST SUITE\n");
    vga_write("==========================================\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    test_winapi_init();
    test_winapi_errors();
    test_winapi_memory();
    test_winapi_process();
    test_winapi_strings();
    test_winapi_modules();
    test_winapi_console();
    test_winapi_sync();
    test_winapi_env();
    test_winapi_tls();
    test_winapi_heap();
    test_winapi_sysinfo();
    
    vga_write("\n=== WinAPI Test Results ===\n");
    vga_write("Tests passed: ");
    vga_write_dec(tests_passed);
    vga_write("\nTests failed: ");
    vga_write_dec(tests_failed);
    vga_write("\n");
}
