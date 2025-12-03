/**
 * Aurora OS - Developer Tools Header
 */

#ifndef DEVELOPER_TOOLS_H
#define DEVELOPER_TOOLS_H

#include <stdint.h>

typedef struct {
    int compiler_available;
    int debugger_available;
    int ide_available;
    int profiler_available;
    int package_manager_available;
    int build_system_available;
} dev_tools_state_t;

/* Debugger GUI structures */
typedef struct {
    uint32_t address;
    int enabled;
    int hardware;
    uint8_t original_byte;
} breakpoint_t;

typedef struct {
    uint32_t address;
    uint32_t size;
    int type;  /* 0=write, 1=read, 2=readwrite */
    int enabled;
} watchpoint_t;

typedef struct {
    char* file;
    uint32_t line;
    char* function;
    uint32_t address;
    uint64_t frame_pointer;
} stack_frame_t;

/* Profiler structures */
typedef struct {
    uint32_t pc;
    uint64_t timestamp;
    uint32_t count;
    uint64_t total_time;
    char function_name[128];
} profile_sample_t;

typedef struct {
    uint64_t total_samples;
    uint64_t total_time;
    uint32_t num_functions;
    profile_sample_t* samples;
} profile_report_t;

/* GUI Designer structures */
typedef enum {
    WIDGET_WINDOW = 0,
    WIDGET_BUTTON = 1,
    WIDGET_LABEL = 2,
    WIDGET_TEXTBOX = 3,
    WIDGET_CHECKBOX = 4,
    WIDGET_RADIOBUTTON = 5,
    WIDGET_LISTBOX = 6,
    WIDGET_COMBOBOX = 7,
    WIDGET_PANEL = 8,
    WIDGET_MENUBAR = 9,
    WIDGET_TOOLBAR = 10
} widget_type_t;

typedef struct {
    widget_type_t type;
    uint32_t id;
    int32_t x, y, width, height;
    char text[256];
    uint32_t parent_id;
    uint32_t style_flags;
    void* event_handlers;
} gui_widget_t;

typedef struct {
    char project_name[128];
    uint32_t widget_count;
    gui_widget_t* widgets;
    char output_path[512];
} gui_designer_project_t;

// Function prototypes
int developer_tools_init(void);
int compiler_toolchain_init(void);
int debugger_gui_init(void);
int ide_init(void);
int profiler_init(void);
int package_manager_init(void);
int build_system_init(void);
int developer_tools_all_available(void);

/* Debugger GUI functions */
int debugger_set_breakpoint(uint32_t address, int hardware);
int debugger_remove_breakpoint(uint32_t address);
int debugger_set_watchpoint(uint32_t address, uint32_t size, int type);
int debugger_remove_watchpoint(uint32_t address);
int debugger_step_over(void);
int debugger_step_into(void);
int debugger_continue(void);
int debugger_get_backtrace(stack_frame_t** frames, uint32_t* count);
int debugger_read_memory(uint32_t address, void* buffer, uint32_t size);
int debugger_write_memory(uint32_t address, const void* buffer, uint32_t size);
int debugger_get_registers(void* reg_state);

/* Profiler functions */
int profiler_start_sampling(uint32_t interval_us);
int profiler_stop_sampling(void);
int profiler_generate_report(profile_report_t* report);
int profiler_reset(void);
int profiler_get_hotspots(profile_sample_t** hotspots, uint32_t* count);
int profiler_export_flamegraph(const char* output_path);

/* GUI Designer functions */
int gui_designer_init(void);
int gui_designer_create_project(const char* name, gui_designer_project_t** project);
int gui_designer_add_widget(gui_designer_project_t* project, widget_type_t type, 
                            int32_t x, int32_t y, int32_t width, int32_t height);
int gui_designer_remove_widget(gui_designer_project_t* project, uint32_t widget_id);
int gui_designer_generate_code(gui_designer_project_t* project, const char* language);
int gui_designer_save_project(gui_designer_project_t* project, const char* path);
int gui_designer_load_project(const char* path, gui_designer_project_t** project);

#endif // DEVELOPER_TOOLS_H
