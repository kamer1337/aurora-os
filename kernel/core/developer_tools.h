/**
 * Aurora OS - Developer Tools Header
 */

#ifndef DEVELOPER_TOOLS_H
#define DEVELOPER_TOOLS_H

typedef struct {
    int compiler_available;
    int debugger_available;
    int ide_available;
    int profiler_available;
    int package_manager_available;
    int build_system_available;
} dev_tools_state_t;

// Function prototypes
int developer_tools_init(void);
int compiler_toolchain_init(void);
int debugger_gui_init(void);
int ide_init(void);
int profiler_init(void);
int package_manager_init(void);
int build_system_init(void);
int developer_tools_all_available(void);

#endif // DEVELOPER_TOOLS_H
