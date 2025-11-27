/**
 * @file display_server.c
 * @brief X11/Wayland Display Server Support for Linux VMs
 *
 * Implements display server protocol support for Linux applications
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * DISPLAY SERVER DEFINITIONS
 * ============================================================================ */

/* Display protocol types */
typedef enum {
    DISPLAY_PROTOCOL_NONE = 0,
    DISPLAY_PROTOCOL_X11,
    DISPLAY_PROTOCOL_WAYLAND
} display_protocol_t;

/* X11 Protocol Constants */
#define X11_PROTOCOL_MAJOR      11
#define X11_PROTOCOL_MINOR      0

/* X11 Request opcodes */
typedef enum {
    X11_CreateWindow = 1,
    X11_ChangeWindowAttributes = 2,
    X11_GetWindowAttributes = 3,
    X11_DestroyWindow = 4,
    X11_DestroySubwindows = 5,
    X11_ChangeSaveSet = 6,
    X11_ReparentWindow = 7,
    X11_MapWindow = 8,
    X11_MapSubwindows = 9,
    X11_UnmapWindow = 10,
    X11_UnmapSubwindows = 11,
    X11_ConfigureWindow = 12,
    X11_CirculateWindow = 13,
    X11_GetGeometry = 14,
    X11_QueryTree = 15,
    X11_InternAtom = 16,
    X11_GetAtomName = 17,
    X11_ChangeProperty = 18,
    X11_DeleteProperty = 19,
    X11_GetProperty = 20,
    X11_ListProperties = 21,
    X11_SetSelectionOwner = 22,
    X11_GetSelectionOwner = 23,
    X11_ConvertSelection = 24,
    X11_SendEvent = 25,
    X11_GrabPointer = 26,
    X11_UngrabPointer = 27,
    X11_GrabButton = 28,
    X11_UngrabButton = 29,
    X11_ChangeActivePointerGrab = 30,
    X11_GrabKeyboard = 31,
    X11_UngrabKeyboard = 32,
    X11_GrabKey = 33,
    X11_UngrabKey = 34,
    X11_AllowEvents = 35,
    X11_GrabServer = 36,
    X11_UngrabServer = 37,
    X11_QueryPointer = 38,
    X11_GetMotionEvents = 39,
    X11_TranslateCoords = 40,
    X11_WarpPointer = 41,
    X11_SetInputFocus = 42,
    X11_GetInputFocus = 43,
    X11_QueryKeymap = 44,
    X11_OpenFont = 45,
    X11_CloseFont = 46,
    X11_QueryFont = 47,
    X11_QueryTextExtents = 48,
    X11_ListFonts = 49,
    X11_ListFontsWithInfo = 50,
    X11_SetFontPath = 51,
    X11_GetFontPath = 52,
    X11_CreatePixmap = 53,
    X11_FreePixmap = 54,
    X11_CreateGC = 55,
    X11_ChangeGC = 56,
    X11_CopyGC = 57,
    X11_SetDashes = 58,
    X11_SetClipRectangles = 59,
    X11_FreeGC = 60,
    X11_ClearArea = 61,
    X11_CopyArea = 62,
    X11_CopyPlane = 63,
    X11_PolyPoint = 64,
    X11_PolyLine = 65,
    X11_PolySegment = 66,
    X11_PolyRectangle = 67,
    X11_PolyArc = 68,
    X11_FillPoly = 69,
    X11_PolyFillRectangle = 70,
    X11_PolyFillArc = 71,
    X11_PutImage = 72,
    X11_GetImage = 73,
    X11_PolyText8 = 74,
    X11_PolyText16 = 75,
    X11_ImageText8 = 76,
    X11_ImageText16 = 77,
    X11_CreateColormap = 78,
    X11_FreeColormap = 79,
    X11_CopyColormapAndFree = 80,
    X11_InstallColormap = 81,
    X11_UninstallColormap = 82,
    X11_ListInstalledColormaps = 83,
    X11_AllocColor = 84,
    X11_AllocNamedColor = 85,
    X11_AllocColorCells = 86,
    X11_AllocColorPlanes = 87,
    X11_FreeColors = 88,
    X11_StoreColors = 89,
    X11_StoreNamedColor = 90,
    X11_QueryColors = 91,
    X11_LookupColor = 92,
    X11_CreateCursor = 93,
    X11_CreateGlyphCursor = 94,
    X11_FreeCursor = 95,
    X11_RecolorCursor = 96,
    X11_QueryBestSize = 97,
    X11_QueryExtension = 98,
    X11_ListExtensions = 99,
    X11_ChangeKeyboardMapping = 100,
    X11_GetKeyboardMapping = 101,
    X11_ChangeKeyboardControl = 102,
    X11_GetKeyboardControl = 103,
    X11_Bell = 104,
    X11_ChangePointerControl = 105,
    X11_GetPointerControl = 106,
    X11_SetScreenSaver = 107,
    X11_GetScreenSaver = 108,
    X11_ChangeHosts = 109,
    X11_ListHosts = 110,
    X11_SetAccessControl = 111,
    X11_SetCloseDownMode = 112,
    X11_KillClient = 113,
    X11_RotateProperties = 114,
    X11_ForceScreenSaver = 115,
    X11_SetPointerMapping = 116,
    X11_GetPointerMapping = 117,
    X11_SetModifierMapping = 118,
    X11_GetModifierMapping = 119,
    X11_NoOperation = 127
} x11_request_t;

/* Wayland Protocol Constants */
#define WL_DISPLAY_SYNC             0
#define WL_DISPLAY_GET_REGISTRY     1
#define WL_REGISTRY_BIND            0
#define WL_COMPOSITOR_CREATE_SURFACE 0
#define WL_COMPOSITOR_CREATE_REGION 1
#define WL_SHM_CREATE_POOL          0
#define WL_SHM_POOL_CREATE_BUFFER   0
#define WL_SURFACE_DESTROY          0
#define WL_SURFACE_ATTACH           1
#define WL_SURFACE_DAMAGE           2
#define WL_SURFACE_FRAME            3
#define WL_SURFACE_COMMIT           4

/* ============================================================================
 * X11 STRUCTURES
 * ============================================================================ */

/* X11 Window */
typedef struct {
    uint32_t id;
    uint32_t parent;
    int16_t x, y;
    uint16_t width, height;
    uint16_t border_width;
    uint16_t window_class;
    uint32_t visual;
    uint32_t background_pixel;
    uint32_t border_pixel;
    bool mapped;
    bool override_redirect;
    uint32_t event_mask;
} x11_window_t;

/* X11 Pixmap */
typedef struct {
    uint32_t id;
    uint32_t drawable;
    uint16_t width, height;
    uint8_t depth;
    void* data;
} x11_pixmap_t;

/* X11 Graphics Context */
typedef struct {
    uint32_t id;
    uint32_t drawable;
    uint32_t foreground;
    uint32_t background;
    uint8_t function;
    uint32_t plane_mask;
    uint16_t line_width;
    uint8_t line_style;
    uint8_t cap_style;
    uint8_t join_style;
    uint8_t fill_style;
    uint32_t font;
} x11_gc_t;

/* X11 Atom */
typedef struct {
    uint32_t id;
    char name[64];
    bool only_if_exists;
} x11_atom_t;

/* X11 Client */
typedef struct {
    uint32_t id;
    bool connected;
    uint32_t resource_base;
    uint32_t resource_mask;
    x11_window_t windows[64];
    uint32_t window_count;
    x11_pixmap_t pixmaps[32];
    uint32_t pixmap_count;
    x11_gc_t gcs[32];
    uint32_t gc_count;
} x11_client_t;

/* X11 Server State */
typedef struct {
    bool initialized;
    bool running;
    uint32_t screen_width;
    uint32_t screen_height;
    uint8_t screen_depth;
    uint32_t root_window;
    uint32_t root_visual;
    x11_client_t clients[16];
    uint32_t client_count;
    x11_atom_t atoms[256];
    uint32_t atom_count;
    uint32_t next_resource_id;
    /* Framebuffer */
    void* framebuffer;
    uint32_t fb_size;
} x11_server_t;

/* ============================================================================
 * WAYLAND STRUCTURES
 * ============================================================================ */

/* Wayland Surface */
typedef struct {
    uint32_t id;
    int32_t x, y;
    int32_t width, height;
    void* buffer;
    uint32_t buffer_size;
    bool committed;
    bool damaged;
    int32_t damage_x, damage_y;
    int32_t damage_width, damage_height;
} wl_surface_t;

/* Wayland Buffer */
typedef struct {
    uint32_t id;
    void* data;
    int32_t width, height;
    int32_t stride;
    uint32_t format;
} wl_buffer_t;

/* Wayland SHM Pool */
typedef struct {
    uint32_t id;
    void* data;
    int32_t size;
    wl_buffer_t buffers[8];
    uint32_t buffer_count;
} wl_shm_pool_t;

/* Wayland Client */
typedef struct {
    uint32_t id;
    bool connected;
    wl_surface_t surfaces[32];
    uint32_t surface_count;
    wl_shm_pool_t shm_pools[8];
    uint32_t shm_pool_count;
} wl_client_t;

/* Wayland Compositor State */
typedef struct {
    bool initialized;
    bool running;
    uint32_t width;
    uint32_t height;
    wl_client_t clients[16];
    uint32_t client_count;
    uint32_t next_id;
    /* Framebuffer */
    void* framebuffer;
    uint32_t fb_size;
} wl_compositor_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static x11_server_t g_x11_server;
static wl_compositor_t g_wayland_compositor;
static display_protocol_t g_active_protocol = DISPLAY_PROTOCOL_NONE;

/* ============================================================================
 * X11 SERVER IMPLEMENTATION
 * ============================================================================ */

/**
 * Initialize X11 server
 */
int x11_server_init(uint32_t width, uint32_t height, uint8_t depth, void* framebuffer) {
    if (g_x11_server.initialized) {
        return 0;
    }
    
    platform_memset(&g_x11_server, 0, sizeof(x11_server_t));
    
    g_x11_server.screen_width = width;
    g_x11_server.screen_height = height;
    g_x11_server.screen_depth = depth;
    g_x11_server.framebuffer = framebuffer;
    g_x11_server.fb_size = width * height * (depth / 8);
    
    /* Create root window */
    g_x11_server.root_window = 1;
    g_x11_server.root_visual = 1;
    g_x11_server.next_resource_id = 0x100;
    
    /* Register built-in atoms */
    g_x11_server.atoms[0].id = 1;
    platform_strncpy(g_x11_server.atoms[0].name, "PRIMARY", 64);
    g_x11_server.atoms[1].id = 2;
    platform_strncpy(g_x11_server.atoms[1].name, "SECONDARY", 64);
    g_x11_server.atoms[2].id = 3;
    platform_strncpy(g_x11_server.atoms[2].name, "CLIPBOARD", 64);
    g_x11_server.atoms[3].id = 4;
    platform_strncpy(g_x11_server.atoms[3].name, "WM_NAME", 64);
    g_x11_server.atoms[4].id = 5;
    platform_strncpy(g_x11_server.atoms[4].name, "WM_CLASS", 64);
    g_x11_server.atom_count = 5;
    
    g_x11_server.initialized = true;
    g_x11_server.running = true;
    
    return 0;
}

/**
 * Process X11 request
 */
int x11_process_request(uint32_t client_id, uint8_t opcode, uint8_t* data, uint32_t length) {
    if (!g_x11_server.initialized || !g_x11_server.running) {
        return -1;
    }
    
    (void)data;
    (void)length;
    
    switch (opcode) {
        case X11_CreateWindow:
            /* Create window request */
            break;
        case X11_MapWindow:
            /* Map window request */
            break;
        case X11_UnmapWindow:
            /* Unmap window request */
            break;
        case X11_DestroyWindow:
            /* Destroy window request */
            break;
        case X11_CreateGC:
            /* Create graphics context */
            break;
        case X11_PutImage:
            /* Put image to drawable */
            break;
        case X11_PolyFillRectangle:
            /* Fill rectangles */
            break;
        case X11_InternAtom:
            /* Intern atom */
            break;
        case X11_GetProperty:
            /* Get property */
            break;
        case X11_NoOperation:
            /* No operation */
            break;
        default:
            break;
    }
    
    (void)client_id;
    return 0;
}

/**
 * Accept X11 client connection
 */
int x11_accept_client(void) {
    if (!g_x11_server.initialized) {
        return -1;
    }
    
    if (g_x11_server.client_count >= 16) {
        return -1;
    }
    
    uint32_t client_id = g_x11_server.client_count;
    x11_client_t* client = &g_x11_server.clients[client_id];
    
    platform_memset(client, 0, sizeof(x11_client_t));
    client->id = client_id;
    client->connected = true;
    client->resource_base = g_x11_server.next_resource_id;
    client->resource_mask = 0x001FFFFF;
    g_x11_server.next_resource_id += 0x00200000;
    
    g_x11_server.client_count++;
    
    return (int)client_id;
}

/**
 * Disconnect X11 client
 */
int x11_disconnect_client(uint32_t client_id) {
    if (client_id >= g_x11_server.client_count) {
        return -1;
    }
    
    g_x11_server.clients[client_id].connected = false;
    return 0;
}

/* ============================================================================
 * WAYLAND COMPOSITOR IMPLEMENTATION
 * ============================================================================ */

/**
 * Initialize Wayland compositor
 */
int wayland_compositor_init(uint32_t width, uint32_t height, void* framebuffer) {
    if (g_wayland_compositor.initialized) {
        return 0;
    }
    
    platform_memset(&g_wayland_compositor, 0, sizeof(wl_compositor_t));
    
    g_wayland_compositor.width = width;
    g_wayland_compositor.height = height;
    g_wayland_compositor.framebuffer = framebuffer;
    g_wayland_compositor.fb_size = width * height * 4;
    g_wayland_compositor.next_id = 1;
    
    g_wayland_compositor.initialized = true;
    g_wayland_compositor.running = true;
    
    return 0;
}

/**
 * Process Wayland message
 */
int wayland_process_message(uint32_t client_id, uint32_t object_id, 
                            uint16_t opcode, uint8_t* data, uint32_t length) {
    if (!g_wayland_compositor.initialized || !g_wayland_compositor.running) {
        return -1;
    }
    
    (void)data;
    (void)length;
    
    /* Handle based on object type and opcode */
    if (object_id == 1) {
        /* wl_display */
        switch (opcode) {
            case WL_DISPLAY_SYNC:
                /* Sync request */
                break;
            case WL_DISPLAY_GET_REGISTRY:
                /* Get registry */
                break;
        }
    }
    
    (void)client_id;
    return 0;
}

/**
 * Create Wayland surface
 */
int wayland_create_surface(uint32_t client_id) {
    if (client_id >= g_wayland_compositor.client_count) {
        return -1;
    }
    
    wl_client_t* client = &g_wayland_compositor.clients[client_id];
    if (client->surface_count >= 32) {
        return -1;
    }
    
    wl_surface_t* surface = &client->surfaces[client->surface_count];
    platform_memset(surface, 0, sizeof(wl_surface_t));
    surface->id = g_wayland_compositor.next_id++;
    
    client->surface_count++;
    
    return (int)surface->id;
}

/**
 * Commit Wayland surface
 */
int wayland_commit_surface(uint32_t client_id, uint32_t surface_id) {
    if (client_id >= g_wayland_compositor.client_count) {
        return -1;
    }
    
    wl_client_t* client = &g_wayland_compositor.clients[client_id];
    
    for (uint32_t i = 0; i < client->surface_count; i++) {
        if (client->surfaces[i].id == surface_id) {
            client->surfaces[i].committed = true;
            return 0;
        }
    }
    
    return -1;
}

/**
 * Accept Wayland client connection
 */
int wayland_accept_client(void) {
    if (!g_wayland_compositor.initialized) {
        return -1;
    }
    
    if (g_wayland_compositor.client_count >= 16) {
        return -1;
    }
    
    uint32_t client_id = g_wayland_compositor.client_count;
    wl_client_t* client = &g_wayland_compositor.clients[client_id];
    
    platform_memset(client, 0, sizeof(wl_client_t));
    client->id = client_id;
    client->connected = true;
    
    g_wayland_compositor.client_count++;
    
    return (int)client_id;
}

/* ============================================================================
 * UNIFIED DISPLAY SERVER API
 * ============================================================================ */

/**
 * Initialize display server
 */
int display_server_init(display_protocol_t protocol, uint32_t width, uint32_t height, void* framebuffer) {
    int result;
    
    switch (protocol) {
        case DISPLAY_PROTOCOL_X11:
            result = x11_server_init(width, height, 24, framebuffer);
            break;
        case DISPLAY_PROTOCOL_WAYLAND:
            result = wayland_compositor_init(width, height, framebuffer);
            break;
        default:
            return -1;
    }
    
    if (result == 0) {
        g_active_protocol = protocol;
    }
    
    return result;
}

/**
 * Shutdown display server
 */
void display_server_shutdown(void) {
    switch (g_active_protocol) {
        case DISPLAY_PROTOCOL_X11:
            g_x11_server.running = false;
            break;
        case DISPLAY_PROTOCOL_WAYLAND:
            g_wayland_compositor.running = false;
            break;
        default:
            break;
    }
    
    g_active_protocol = DISPLAY_PROTOCOL_NONE;
}

/**
 * Get active display protocol
 */
display_protocol_t display_server_get_protocol(void) {
    return g_active_protocol;
}

/**
 * Check if display server is running
 */
bool display_server_is_running(void) {
    switch (g_active_protocol) {
        case DISPLAY_PROTOCOL_X11:
            return g_x11_server.running;
        case DISPLAY_PROTOCOL_WAYLAND:
            return g_wayland_compositor.running;
        default:
            return false;
    }
}

/**
 * Get display server version
 */
const char* display_server_get_version(void) {
    return "1.0.0-aurora-display";
}
