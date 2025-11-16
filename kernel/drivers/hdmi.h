/**
 * Aurora OS - HDMI Driver Header
 * 
 * HDMI/DisplayPort display output driver
 */

#ifndef AURORA_HDMI_H
#define AURORA_HDMI_H

#include <stdint.h>

/* HDMI standards */
#define HDMI_VERSION_1_0    0x10
#define HDMI_VERSION_1_4    0x14
#define HDMI_VERSION_2_0    0x20
#define HDMI_VERSION_2_1    0x21

/* Video formats */
#define HDMI_FORMAT_640x480     0
#define HDMI_FORMAT_720x480     1
#define HDMI_FORMAT_720x576     2
#define HDMI_FORMAT_1280x720    3
#define HDMI_FORMAT_1920x1080   4
#define HDMI_FORMAT_3840x2160   5
#define HDMI_FORMAT_7680x4320   6

/* Refresh rates */
#define HDMI_REFRESH_24HZ   24
#define HDMI_REFRESH_25HZ   25
#define HDMI_REFRESH_30HZ   30
#define HDMI_REFRESH_50HZ   50
#define HDMI_REFRESH_60HZ   60
#define HDMI_REFRESH_120HZ  120
#define HDMI_REFRESH_144HZ  144

/* Color depths */
#define HDMI_COLOR_DEPTH_8BIT   8
#define HDMI_COLOR_DEPTH_10BIT  10
#define HDMI_COLOR_DEPTH_12BIT  12
#define HDMI_COLOR_DEPTH_16BIT  16

/* Color spaces */
#define HDMI_COLOR_RGB          0
#define HDMI_COLOR_YCbCr_444    1
#define HDMI_COLOR_YCbCr_422    2
#define HDMI_COLOR_YCbCr_420    3

/* Audio formats */
#define HDMI_AUDIO_NONE         0
#define HDMI_AUDIO_LPCM         1
#define HDMI_AUDIO_AC3          2
#define HDMI_AUDIO_DTS          3
#define HDMI_AUDIO_DOLBY_ATMOS  4

/* Connection status */
typedef enum {
    HDMI_STATUS_DISCONNECTED,
    HDMI_STATUS_CONNECTED,
    HDMI_STATUS_ACTIVE,
    HDMI_STATUS_ERROR
} hdmi_status_t;

/* Video mode structure */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t refresh_rate;
    uint8_t color_depth;
    uint8_t color_space;
    uint8_t interlaced;
} hdmi_video_mode_t;

/* Audio configuration */
typedef struct {
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint8_t bit_depth;
} hdmi_audio_config_t;

/* EDID information */
typedef struct {
    char manufacturer[4];
    char model[14];
    uint16_t product_code;
    uint32_t serial_number;
    uint8_t week_of_manufacture;
    uint16_t year_of_manufacture;
    
    /* Display characteristics */
    uint16_t max_width_cm;
    uint16_t max_height_cm;
    
    /* Supported video modes */
    uint8_t video_mode_count;
    hdmi_video_mode_t video_modes[16];
    
    /* Supported audio formats */
    uint8_t audio_format_count;
    uint8_t audio_formats[8];
} hdmi_edid_t;

/* HDMI port structure */
typedef struct {
    uint8_t port_num;
    hdmi_status_t status;
    uint8_t hdmi_version;
    
    /* Current configuration */
    hdmi_video_mode_t video_mode;
    hdmi_audio_config_t audio_config;
    
    /* Display information */
    hdmi_edid_t edid;
    
    /* Capabilities */
    uint8_t hdcp_supported;
    uint8_t cec_supported;
    uint8_t arc_supported;
    uint8_t hdr_supported;
} hdmi_port_t;

/* HDMI subsystem initialization */
void hdmi_init(void);

/* Port management */
int hdmi_get_port_count(void);
hdmi_port_t* hdmi_get_port(uint8_t port_num);
hdmi_status_t hdmi_get_port_status(uint8_t port_num);

/* Display detection */
int hdmi_detect_display(uint8_t port_num);
int hdmi_read_edid(uint8_t port_num, hdmi_edid_t* edid);
int hdmi_get_supported_modes(uint8_t port_num, hdmi_video_mode_t* modes, uint8_t max_modes, uint8_t* count);

/* Video configuration */
int hdmi_set_video_mode(uint8_t port_num, hdmi_video_mode_t* mode);
int hdmi_get_video_mode(uint8_t port_num, hdmi_video_mode_t* mode);
int hdmi_set_color_space(uint8_t port_num, uint8_t color_space);
int hdmi_set_color_depth(uint8_t port_num, uint8_t color_depth);

/* Audio configuration */
int hdmi_set_audio_config(uint8_t port_num, hdmi_audio_config_t* config);
int hdmi_get_audio_config(uint8_t port_num, hdmi_audio_config_t* config);
int hdmi_enable_audio(uint8_t port_num);
int hdmi_disable_audio(uint8_t port_num);

/* Framebuffer operations */
int hdmi_set_framebuffer(uint8_t port_num, void* framebuffer, uint32_t pitch);
int hdmi_get_framebuffer(uint8_t port_num, void** framebuffer, uint32_t* pitch);
int hdmi_update_display(uint8_t port_num);

/* Hot-plug detection */
int hdmi_enable_hotplug_detection(uint8_t port_num);
int hdmi_disable_hotplug_detection(uint8_t port_num);
void hdmi_hotplug_handler(uint8_t port_num);

/* HDCP (High-bandwidth Digital Content Protection) */
int hdmi_enable_hdcp(uint8_t port_num);
int hdmi_disable_hdcp(uint8_t port_num);
int hdmi_get_hdcp_status(uint8_t port_num, uint8_t* enabled);

/* CEC (Consumer Electronics Control) */
int hdmi_cec_send_command(uint8_t port_num, uint8_t* command, uint8_t length);
int hdmi_cec_receive_command(uint8_t port_num, uint8_t* buffer, uint8_t max_length, uint8_t* actual_length);

/* HDR (High Dynamic Range) */
int hdmi_enable_hdr(uint8_t port_num);
int hdmi_disable_hdr(uint8_t port_num);
int hdmi_set_hdr_metadata(uint8_t port_num, void* metadata, uint32_t size);

/* Power management */
int hdmi_power_on(uint8_t port_num);
int hdmi_power_off(uint8_t port_num);
int hdmi_set_power_save(uint8_t port_num, int enabled);

/* Diagnostics */
int hdmi_get_link_status(uint8_t port_num, uint8_t* status);
int hdmi_run_diagnostics(uint8_t port_num);

#endif /* AURORA_HDMI_H */
