/**
 * Aurora OS - Windows Library Support Tests
 * 
 * Test suite for PE loader, DLL loader, and WinAPI compatibility layer
 */

#ifndef PE_LOADER_TESTS_H
#define PE_LOADER_TESTS_H

/**
 * Run all PE loader tests
 */
void run_pe_loader_tests(void);

/**
 * Run all DLL loader tests
 */
void run_dll_loader_tests(void);

/**
 * Run all WinAPI tests
 */
void run_winapi_tests(void);

#endif /* PE_LOADER_TESTS_H */
