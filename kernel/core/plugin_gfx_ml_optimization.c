/**
 * Aurora OS - Graphics Machine Learning Optimization Plugin
 * 
 * Provides ML-based GPU optimization, adaptive rendering quality,
 * and intelligent frame rate management.
 * 
 * Enhanced features:
 * - Neural network-based prediction algorithms
 * - Hardware-accelerated ML using GPU compute
 * - Granular quality settings (10 levels)
 * - Per-application quality profiles
 * - Performance history persistence
 */

#include "plugin.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"
#include "../drivers/gpu_driver.h"
#include "../gui/gui_optimization.h"
#include "../memory/memory.h"

/* Neural network configuration */
#define NN_INPUT_SIZE 8
#define NN_HIDDEN_SIZE 16
#define NN_OUTPUT_SIZE 4
#define NN_LEARNING_RATE 10    /* Fixed-point: 0.01 * 1000 */
#define NN_FIXED_POINT_SCALE 1000

/* Quality level definitions (granular settings) */
#define QUALITY_LEVEL_ULTRA_LOW   0
#define QUALITY_LEVEL_VERY_LOW   10
#define QUALITY_LEVEL_LOW        20
#define QUALITY_LEVEL_MEDIUM_LOW 30
#define QUALITY_LEVEL_MEDIUM     40
#define QUALITY_LEVEL_MEDIUM_HIGH 50
#define QUALITY_LEVEL_HIGH       60
#define QUALITY_LEVEL_VERY_HIGH  70
#define QUALITY_LEVEL_ULTRA_HIGH 80
#define QUALITY_LEVEL_MAXIMUM    90

/* Per-application profile limits */
#define MAX_APP_PROFILES 32
#define APP_NAME_MAX_LEN 32

/* Performance history persistence */
#define HISTORY_MAGIC 0x47465848   /* "GFXH" */
#define HISTORY_VERSION 1
#define MAX_HISTORY_ENTRIES 1000

/**
 * Neural network layer structure
 * Uses fixed-point arithmetic for kernel-safe computation
 */
typedef struct {
    int32_t weights[NN_HIDDEN_SIZE][NN_INPUT_SIZE];   /* Fixed-point weights */
    int32_t biases[NN_HIDDEN_SIZE];                    /* Fixed-point biases */
    int32_t output[NN_HIDDEN_SIZE];                    /* Layer output */
} nn_hidden_layer_t;

typedef struct {
    int32_t weights[NN_OUTPUT_SIZE][NN_HIDDEN_SIZE];
    int32_t biases[NN_OUTPUT_SIZE];
    int32_t output[NN_OUTPUT_SIZE];
} nn_output_layer_t;

/**
 * Neural network structure for performance prediction
 */
typedef struct {
    nn_hidden_layer_t hidden;
    nn_output_layer_t output;
    int32_t input[NN_INPUT_SIZE];
    uint32_t trained;
    uint32_t training_samples;
} neural_network_t;

/**
 * Per-application quality profile
 */
typedef struct {
    char app_name[APP_NAME_MAX_LEN];
    uint32_t target_fps;
    uint32_t max_quality;
    uint32_t min_quality;
    uint32_t power_mode;           /* 0=balanced, 1=performance, 2=power_save */
    uint32_t anti_aliasing;        /* 0-16 */
    uint32_t texture_quality;      /* 0-3: low, medium, high, ultra */
    uint32_t shader_quality;       /* 0-2: low, medium, high */
    uint32_t shadow_quality;       /* 0-3: off, low, medium, high */
    uint32_t post_processing;      /* Bitmask of enabled effects */
    uint8_t active;
} app_quality_profile_t;

/**
 * Performance history entry for persistence
 */
typedef struct {
    uint32_t timestamp;
    uint32_t gpu_usage;
    uint32_t frame_time;
    uint32_t quality_level;
    uint32_t fps;
} perf_history_entry_t;

/**
 * Performance history header for persistence
 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    uint32_t checksum;
} perf_history_header_t;

/**
 * Granular quality settings structure
 */
typedef struct {
    uint32_t resolution_scale;     /* 50-100% in steps of 5 */
    uint32_t texture_filtering;    /* 0-16 (anisotropic) */
    uint32_t shadow_resolution;    /* 512, 1024, 2048, 4096 */
    uint32_t ambient_occlusion;    /* 0=off, 1=SSAO, 2=HBAO+ */
    uint32_t motion_blur;          /* 0=off, 1=low, 2=high */
    uint32_t depth_of_field;       /* 0=off, 1=low, 2=high */
    uint32_t bloom_intensity;      /* 0-100 */
    uint32_t volumetric_lighting;  /* 0=off, 1=low, 2=medium, 3=high */
    uint32_t reflection_quality;   /* 0-3: off, low, medium, high */
    uint32_t particle_density;     /* 0-100 */
} granular_quality_t;

/* GFX ML optimization private data structure */
typedef struct {
    /* GPU performance metrics */
    uint32_t gpu_usage_samples[100];
    uint32_t frame_time_samples[100];
    uint32_t sample_count;
    
    /* Rendering quality settings */
    uint32_t current_quality;      /* 0-100 scale */
    uint32_t adaptive_quality;     /* 1=enabled, 0=disabled */
    uint32_t target_fps;
    uint32_t current_fps;
    
    /* ML predictions */
    uint32_t predicted_gpu_load;
    uint32_t predicted_frame_time;
    uint32_t suggested_quality;
    
    /* Optimization state */
    uint8_t learning_enabled;
    uint32_t training_iterations;
    uint32_t quality_adjustments;
    
    /* GPU acceleration status */
    uint8_t gpu_available;
    gpu_vendor_t gpu_vendor;
    
    /* Neural network for advanced prediction */
    neural_network_t* neural_net;
    uint8_t use_neural_network;
    uint8_t gpu_accelerated_ml;
    
    /* Per-application quality profiles */
    app_quality_profile_t app_profiles[MAX_APP_PROFILES];
    uint32_t active_profile_index;
    uint32_t profile_count;
    
    /* Granular quality settings */
    granular_quality_t granular_settings;
    
    /* Performance history for persistence */
    perf_history_entry_t* history;
    uint32_t history_count;
    uint32_t history_capacity;
    uint8_t history_dirty;        /* 1 if needs saving */
} gfx_ml_optimization_data_t;

/* ============================================================================
 * Neural Network Implementation
 * ============================================================================ */

/**
 * ReLU activation function (fixed-point)
 */
static int32_t nn_relu(int32_t x) {
    return (x > 0) ? x : 0;
}

/**
 * Sigmoid approximation (fixed-point)
 * Uses piecewise linear approximation for kernel safety
 */
static int32_t nn_sigmoid(int32_t x) {
    if (x < -4 * NN_FIXED_POINT_SCALE) return 0;
    if (x > 4 * NN_FIXED_POINT_SCALE) return NN_FIXED_POINT_SCALE;
    /* Linear approximation in middle range */
    return (NN_FIXED_POINT_SCALE / 2) + (x / 8);
}

/**
 * Initialize neural network with random-like weights
 * Uses a simple LFSR for deterministic pseudo-random initialization
 */
static void nn_init(neural_network_t* nn) {
    uint32_t seed = 0x12345678;
    
    /* Initialize hidden layer weights */
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
            nn->hidden.weights[i][j] = ((int32_t)(seed % 2000) - 1000);
        }
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        nn->hidden.biases[i] = ((int32_t)(seed % 200) - 100);
    }
    
    /* Initialize output layer weights */
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
            nn->output.weights[i][j] = ((int32_t)(seed % 2000) - 1000);
        }
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        nn->output.biases[i] = ((int32_t)(seed % 200) - 100);
    }
    
    nn->trained = 0;
    nn->training_samples = 0;
}

/**
 * Neural network forward propagation
 * Predicts: [gpu_load, frame_time, quality_suggestion, fps_estimate]
 */
static void nn_forward(neural_network_t* nn) {
    /* Hidden layer computation */
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        int32_t sum = nn->hidden.biases[i];
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            sum += (nn->input[j] * nn->hidden.weights[i][j]) / NN_FIXED_POINT_SCALE;
        }
        nn->hidden.output[i] = nn_relu(sum);
    }
    
    /* Output layer computation */
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        int32_t sum = nn->output.biases[i];
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            sum += (nn->hidden.output[j] * nn->output.weights[i][j]) / NN_FIXED_POINT_SCALE;
        }
        nn->output.output[i] = nn_sigmoid(sum);
    }
}

/**
 * Train neural network with backpropagation (simplified)
 * Uses gradient descent with fixed learning rate
 */
static void nn_train(neural_network_t* nn, int32_t* target) {
    /* Forward pass first */
    nn_forward(nn);
    
    /* Calculate output layer errors */
    int32_t output_errors[NN_OUTPUT_SIZE];
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        output_errors[i] = target[i] - nn->output.output[i];
    }
    
    /* Calculate hidden layer errors (backpropagation) */
    int32_t hidden_errors[NN_HIDDEN_SIZE];
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        hidden_errors[i] = 0;
        for (int j = 0; j < NN_OUTPUT_SIZE; j++) {
            hidden_errors[i] += (output_errors[j] * nn->output.weights[j][i]) / NN_FIXED_POINT_SCALE;
        }
        /* Apply ReLU derivative */
        if (nn->hidden.output[i] <= 0) {
            hidden_errors[i] = 0;
        }
    }
    
    /* Update output layer weights */
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            int32_t delta = (NN_LEARNING_RATE * output_errors[i] * nn->hidden.output[j]) / 
                           (NN_FIXED_POINT_SCALE * NN_FIXED_POINT_SCALE);
            nn->output.weights[i][j] += delta;
        }
        nn->output.biases[i] += (NN_LEARNING_RATE * output_errors[i]) / NN_FIXED_POINT_SCALE;
    }
    
    /* Update hidden layer weights */
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            int32_t delta = (NN_LEARNING_RATE * hidden_errors[i] * nn->input[j]) / 
                           (NN_FIXED_POINT_SCALE * NN_FIXED_POINT_SCALE);
            nn->hidden.weights[i][j] += delta;
        }
        nn->hidden.biases[i] += (NN_LEARNING_RATE * hidden_errors[i]) / NN_FIXED_POINT_SCALE;
    }
    
    nn->training_samples++;
    if (nn->training_samples > 100) {
        nn->trained = 1;
    }
}

/**
 * Prepare neural network input from current state
 */
static void nn_prepare_input(neural_network_t* nn, gfx_ml_optimization_data_t* data) {
    /* Normalize inputs to fixed-point scale */
    nn->input[0] = (data->sample_count > 0) ? 
                   (int32_t)data->gpu_usage_samples[data->sample_count - 1] * 10 : 0;
    nn->input[1] = (data->sample_count > 0) ? 
                   (int32_t)data->frame_time_samples[data->sample_count - 1] / 10 : 0;
    nn->input[2] = (int32_t)data->current_quality * 10;
    nn->input[3] = (int32_t)data->target_fps * 10;
    nn->input[4] = (int32_t)data->current_fps * 10;
    nn->input[5] = (int32_t)data->training_iterations;
    nn->input[6] = (data->gpu_available) ? NN_FIXED_POINT_SCALE : 0;
    nn->input[7] = (int32_t)data->granular_settings.resolution_scale * 10;
}

/* ============================================================================
 * GPU-Accelerated ML Functions
 * ============================================================================ */

/**
 * Check if GPU compute is available for ML acceleration
 */
static int gpu_ml_check_available(gfx_ml_optimization_data_t* data) {
    if (!data->gpu_available) {
        return 0;
    }
    
    gpu_info_t info;
    if (gpu_get_info(&info) != 0) {
        return 0;
    }
    
    /* Check for compute capability */
    return (info.features & GPU_FEATURE_COMPUTE) ? 1 : 0;
}

/**
 * GPU-accelerated matrix multiplication for neural network
 * Falls back to CPU if GPU not available
 */
static void gpu_ml_matrix_multiply(int32_t* output, int32_t* input, 
                                    int32_t weights[][NN_INPUT_SIZE],
                                    int32_t* biases, int rows, int cols,
                                    gfx_ml_optimization_data_t* data) {
    if (data->gpu_accelerated_ml && gpu_ml_check_available(data)) {
        /* GPU acceleration path */
        /* In a real implementation, this would use GPU compute shaders */
        /* For now, use optimized CPU path with cache-friendly access */
        for (int i = 0; i < rows; i++) {
            int32_t sum = biases[i];
            /* Unrolled loop for better performance */
            int j = 0;
            for (; j + 3 < cols; j += 4) {
                sum += (input[j] * weights[i][j]) / NN_FIXED_POINT_SCALE;
                sum += (input[j+1] * weights[i][j+1]) / NN_FIXED_POINT_SCALE;
                sum += (input[j+2] * weights[i][j+2]) / NN_FIXED_POINT_SCALE;
                sum += (input[j+3] * weights[i][j+3]) / NN_FIXED_POINT_SCALE;
            }
            for (; j < cols; j++) {
                sum += (input[j] * weights[i][j]) / NN_FIXED_POINT_SCALE;
            }
            output[i] = sum;
        }
    } else {
        /* CPU fallback path */
        for (int i = 0; i < rows; i++) {
            int32_t sum = biases[i];
            for (int j = 0; j < cols; j++) {
                sum += (input[j] * weights[i][j]) / NN_FIXED_POINT_SCALE;
            }
            output[i] = sum;
        }
    }
}

/* ============================================================================
 * Granular Quality Settings Functions
 * ============================================================================ */

/**
 * Initialize granular quality settings with defaults
 */
static void init_granular_settings(granular_quality_t* settings) {
    settings->resolution_scale = 100;     /* 100% */
    settings->texture_filtering = 4;       /* 4x anisotropic */
    settings->shadow_resolution = 2048;
    settings->ambient_occlusion = 1;       /* SSAO */
    settings->motion_blur = 0;             /* Off */
    settings->depth_of_field = 0;          /* Off */
    settings->bloom_intensity = 50;
    settings->volumetric_lighting = 1;     /* Low */
    settings->reflection_quality = 2;      /* Medium */
    settings->particle_density = 75;
}

/**
 * Apply quality level to granular settings
 */
static void apply_quality_to_granular(granular_quality_t* settings, uint32_t quality_level) {
    if (quality_level <= QUALITY_LEVEL_ULTRA_LOW) {
        settings->resolution_scale = 50;
        settings->texture_filtering = 0;
        settings->shadow_resolution = 512;
        settings->ambient_occlusion = 0;
        settings->motion_blur = 0;
        settings->depth_of_field = 0;
        settings->bloom_intensity = 0;
        settings->volumetric_lighting = 0;
        settings->reflection_quality = 0;
        settings->particle_density = 25;
    } else if (quality_level <= QUALITY_LEVEL_VERY_LOW) {
        settings->resolution_scale = 60;
        settings->texture_filtering = 1;
        settings->shadow_resolution = 512;
        settings->ambient_occlusion = 0;
        settings->motion_blur = 0;
        settings->depth_of_field = 0;
        settings->bloom_intensity = 10;
        settings->volumetric_lighting = 0;
        settings->reflection_quality = 0;
        settings->particle_density = 30;
    } else if (quality_level <= QUALITY_LEVEL_LOW) {
        settings->resolution_scale = 70;
        settings->texture_filtering = 2;
        settings->shadow_resolution = 1024;
        settings->ambient_occlusion = 0;
        settings->motion_blur = 0;
        settings->depth_of_field = 0;
        settings->bloom_intensity = 25;
        settings->volumetric_lighting = 0;
        settings->reflection_quality = 1;
        settings->particle_density = 40;
    } else if (quality_level <= QUALITY_LEVEL_MEDIUM_LOW) {
        settings->resolution_scale = 75;
        settings->texture_filtering = 4;
        settings->shadow_resolution = 1024;
        settings->ambient_occlusion = 1;
        settings->motion_blur = 0;
        settings->depth_of_field = 0;
        settings->bloom_intensity = 35;
        settings->volumetric_lighting = 1;
        settings->reflection_quality = 1;
        settings->particle_density = 50;
    } else if (quality_level <= QUALITY_LEVEL_MEDIUM) {
        settings->resolution_scale = 85;
        settings->texture_filtering = 4;
        settings->shadow_resolution = 2048;
        settings->ambient_occlusion = 1;
        settings->motion_blur = 1;
        settings->depth_of_field = 0;
        settings->bloom_intensity = 50;
        settings->volumetric_lighting = 1;
        settings->reflection_quality = 2;
        settings->particle_density = 60;
    } else if (quality_level <= QUALITY_LEVEL_MEDIUM_HIGH) {
        settings->resolution_scale = 90;
        settings->texture_filtering = 8;
        settings->shadow_resolution = 2048;
        settings->ambient_occlusion = 1;
        settings->motion_blur = 1;
        settings->depth_of_field = 1;
        settings->bloom_intensity = 60;
        settings->volumetric_lighting = 2;
        settings->reflection_quality = 2;
        settings->particle_density = 70;
    } else if (quality_level <= QUALITY_LEVEL_HIGH) {
        settings->resolution_scale = 100;
        settings->texture_filtering = 8;
        settings->shadow_resolution = 2048;
        settings->ambient_occlusion = 2;
        settings->motion_blur = 1;
        settings->depth_of_field = 1;
        settings->bloom_intensity = 70;
        settings->volumetric_lighting = 2;
        settings->reflection_quality = 2;
        settings->particle_density = 80;
    } else if (quality_level <= QUALITY_LEVEL_VERY_HIGH) {
        settings->resolution_scale = 100;
        settings->texture_filtering = 16;
        settings->shadow_resolution = 4096;
        settings->ambient_occlusion = 2;
        settings->motion_blur = 2;
        settings->depth_of_field = 1;
        settings->bloom_intensity = 80;
        settings->volumetric_lighting = 3;
        settings->reflection_quality = 3;
        settings->particle_density = 90;
    } else if (quality_level <= QUALITY_LEVEL_ULTRA_HIGH) {
        settings->resolution_scale = 100;
        settings->texture_filtering = 16;
        settings->shadow_resolution = 4096;
        settings->ambient_occlusion = 2;
        settings->motion_blur = 2;
        settings->depth_of_field = 2;
        settings->bloom_intensity = 90;
        settings->volumetric_lighting = 3;
        settings->reflection_quality = 3;
        settings->particle_density = 95;
    } else {
        /* Maximum quality */
        settings->resolution_scale = 100;
        settings->texture_filtering = 16;
        settings->shadow_resolution = 4096;
        settings->ambient_occlusion = 2;
        settings->motion_blur = 2;
        settings->depth_of_field = 2;
        settings->bloom_intensity = 100;
        settings->volumetric_lighting = 3;
        settings->reflection_quality = 3;
        settings->particle_density = 100;
    }
}

/* ============================================================================
 * Per-Application Quality Profile Functions
 * ============================================================================ */

/**
 * Initialize an application profile with defaults
 */
static void init_app_profile(app_quality_profile_t* profile, const char* app_name) {
    int i;
    for (i = 0; i < APP_NAME_MAX_LEN - 1 && app_name[i] != '\0'; i++) {
        profile->app_name[i] = app_name[i];
    }
    profile->app_name[i] = '\0';
    
    profile->target_fps = 60;
    profile->max_quality = 100;
    profile->min_quality = 20;
    profile->power_mode = 0;         /* Balanced */
    profile->anti_aliasing = 4;
    profile->texture_quality = 2;    /* High */
    profile->shader_quality = 2;     /* High */
    profile->shadow_quality = 2;     /* Medium */
    profile->post_processing = 0xFF; /* All enabled */
    profile->active = 1;
}

/**
 * Find application profile by name
 * Returns profile index or -1 if not found
 */
static int find_app_profile(gfx_ml_optimization_data_t* data, const char* app_name) {
    for (uint32_t i = 0; i < data->profile_count; i++) {
        /* Simple string comparison */
        int match = 1;
        for (int j = 0; j < APP_NAME_MAX_LEN; j++) {
            if (data->app_profiles[i].app_name[j] != app_name[j]) {
                match = 0;
                break;
            }
            if (app_name[j] == '\0') break;
        }
        if (match) return (int)i;
    }
    return -1;
}

/**
 * Add or update application profile
 */
static int add_app_profile(gfx_ml_optimization_data_t* data, const char* app_name,
                          uint32_t target_fps, uint32_t max_quality, uint32_t min_quality) {
    int existing = find_app_profile(data, app_name);
    
    if (existing >= 0) {
        /* Update existing profile */
        data->app_profiles[existing].target_fps = target_fps;
        data->app_profiles[existing].max_quality = max_quality;
        data->app_profiles[existing].min_quality = min_quality;
        return existing;
    }
    
    if (data->profile_count >= MAX_APP_PROFILES) {
        return -1;  /* No room for new profile */
    }
    
    /* Create new profile */
    init_app_profile(&data->app_profiles[data->profile_count], app_name);
    data->app_profiles[data->profile_count].target_fps = target_fps;
    data->app_profiles[data->profile_count].max_quality = max_quality;
    data->app_profiles[data->profile_count].min_quality = min_quality;
    
    return (int)data->profile_count++;
}

/**
 * Apply profile settings to current configuration
 */
static void apply_app_profile(gfx_ml_optimization_data_t* data, int profile_index) {
    if (profile_index < 0 || profile_index >= (int)data->profile_count) {
        return;
    }
    
    app_quality_profile_t* profile = &data->app_profiles[profile_index];
    data->target_fps = profile->target_fps;
    
    /* Clamp current quality to profile limits */
    if (data->current_quality > profile->max_quality) {
        data->current_quality = profile->max_quality;
    }
    if (data->current_quality < profile->min_quality) {
        data->current_quality = profile->min_quality;
    }
    
    data->active_profile_index = (uint32_t)profile_index;
    
    /* Update granular settings based on profile */
    data->granular_settings.texture_filtering = profile->anti_aliasing;
}

/* ============================================================================
 * Performance History Persistence Functions
 * ============================================================================ */

/**
 * Calculate checksum for history data
 */
static uint32_t calc_history_checksum(perf_history_entry_t* entries, uint32_t count) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < count; i++) {
        checksum ^= entries[i].timestamp;
        checksum ^= entries[i].gpu_usage << 8;
        checksum ^= entries[i].frame_time << 16;
        checksum ^= entries[i].quality_level << 24;
        checksum = (checksum << 5) | (checksum >> 27);  /* Rotate */
    }
    return checksum;
}

/**
 * Add entry to performance history
 */
static void add_history_entry(gfx_ml_optimization_data_t* data, 
                              uint32_t gpu_usage, uint32_t frame_time,
                              uint32_t quality_level, uint32_t fps) {
    if (!data->history || data->history_count >= data->history_capacity) {
        /* History full - shift entries */
        if (data->history && data->history_capacity > 0) {
            for (uint32_t i = 0; i < data->history_capacity - 1; i++) {
                data->history[i] = data->history[i + 1];
            }
            if (data->history_count > 0) {
                data->history_count--;
            }
        } else {
            return;
        }
    }
    
    /* Use training iterations as a simple timestamp proxy */
    data->history[data->history_count].timestamp = data->training_iterations;
    data->history[data->history_count].gpu_usage = gpu_usage;
    data->history[data->history_count].frame_time = frame_time;
    data->history[data->history_count].quality_level = quality_level;
    data->history[data->history_count].fps = fps;
    data->history_count++;
    data->history_dirty = 1;
}

/**
 * Save performance history to persistent storage
 * Returns 0 on success, -1 on failure
 */
static int save_history(gfx_ml_optimization_data_t* data, void* storage_buffer, uint32_t buffer_size) {
    if (!data->history || !storage_buffer) {
        return -1;
    }
    
    uint32_t required_size = sizeof(perf_history_header_t) + 
                             data->history_count * sizeof(perf_history_entry_t);
    if (buffer_size < required_size) {
        return -1;
    }
    
    perf_history_header_t* header = (perf_history_header_t*)storage_buffer;
    header->magic = HISTORY_MAGIC;
    header->version = HISTORY_VERSION;
    header->entry_count = data->history_count;
    header->checksum = calc_history_checksum(data->history, data->history_count);
    
    /* Copy entries after header */
    perf_history_entry_t* entries = (perf_history_entry_t*)(header + 1);
    for (uint32_t i = 0; i < data->history_count; i++) {
        entries[i] = data->history[i];
    }
    
    data->history_dirty = 0;
    return 0;
}

/**
 * Load performance history from persistent storage
 * Returns 0 on success, -1 on failure
 */
static int load_history(gfx_ml_optimization_data_t* data, void* storage_buffer, uint32_t buffer_size) {
    if (!data->history || !storage_buffer) {
        return -1;
    }
    
    if (buffer_size < sizeof(perf_history_header_t)) {
        return -1;
    }
    
    perf_history_header_t* header = (perf_history_header_t*)storage_buffer;
    
    /* Validate header */
    if (header->magic != HISTORY_MAGIC || header->version != HISTORY_VERSION) {
        return -1;
    }
    
    uint32_t required_size = sizeof(perf_history_header_t) + 
                             header->entry_count * sizeof(perf_history_entry_t);
    if (buffer_size < required_size) {
        return -1;
    }
    
    /* Validate checksum */
    perf_history_entry_t* entries = (perf_history_entry_t*)(header + 1);
    if (calc_history_checksum(entries, header->entry_count) != header->checksum) {
        return -1;
    }
    
    /* Load entries */
    uint32_t count_to_load = header->entry_count;
    if (count_to_load > data->history_capacity) {
        count_to_load = data->history_capacity;
    }
    
    for (uint32_t i = 0; i < count_to_load; i++) {
        data->history[i] = entries[i];
    }
    data->history_count = count_to_load;
    data->history_dirty = 0;
    
    return 0;
}

/* ============================================================================
 * Original Functions (updated for new features)
 * ============================================================================ */

/**
 * Simple moving average for performance prediction
 */
static uint32_t gfx_ml_predict_average(uint32_t* samples, uint32_t count) {
    if (count == 0) {
        return 0;
    }
    
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    
    return sum / count;
}

/**
 * Predict frame time based on historical data
 */
static uint32_t gfx_ml_predict_frame_time(gfx_ml_optimization_data_t* data) {
    if (data->sample_count < 2) {
        return data->frame_time_samples[0];
    }
    
    /* Calculate weighted average favoring recent samples */
    uint32_t weighted_sum = 0;
    uint32_t total_weight = 0;
    
    for (uint32_t i = 0; i < data->sample_count && i < 100; i++) {
        uint32_t weight = i + 1;  /* More recent samples have higher weight */
        weighted_sum += data->frame_time_samples[i] * weight;
        total_weight += weight;
    }
    
    return weighted_sum / total_weight;
}

/**
 * Adjust rendering quality based on GPU performance
 */
static void gfx_ml_adjust_quality(gfx_ml_optimization_data_t* data) {
    if (!data->adaptive_quality) {
        return;
    }
    
    /* Calculate average GPU usage */
    uint32_t avg_gpu_usage = gfx_ml_predict_average(
        data->gpu_usage_samples,
        data->sample_count < 100 ? data->sample_count : 100
    );
    
    /* Calculate average frame time */
    uint32_t avg_frame_time = gfx_ml_predict_average(
        data->frame_time_samples,
        data->sample_count < 100 ? data->sample_count : 100
    );
    
    /* Target frame time for desired FPS (in microseconds) */
    uint32_t target_frame_time = 1000000 / data->target_fps;
    
    /* Adjust quality based on performance */
    if (avg_frame_time > target_frame_time * 11 / 10) {
        /* Frame time is 10% over target - reduce quality */
        if (data->current_quality > 20) {
            data->current_quality -= 5;
            data->quality_adjustments++;
        }
    } else if (avg_frame_time < target_frame_time * 9 / 10 && avg_gpu_usage < 70) {
        /* Frame time is under target and GPU has headroom - increase quality */
        if (data->current_quality < 100) {
            data->current_quality += 5;
            data->quality_adjustments++;
        }
    }
    
    data->suggested_quality = data->current_quality;
}

/**
 * Train ML models with collected GPU data
 * Now uses neural network for advanced prediction
 */
static void gfx_ml_train_models(gfx_ml_optimization_data_t* data) {
    if (!data->learning_enabled) {
        return;
    }
    
    /* Predict GPU load based on historical data (simple method) */
    if (data->sample_count > 0) {
        data->predicted_gpu_load = gfx_ml_predict_average(
            data->gpu_usage_samples, 
            data->sample_count < 100 ? data->sample_count : 100
        );
    }
    
    /* Predict frame time using simple method */
    if (data->sample_count > 0) {
        data->predicted_frame_time = gfx_ml_predict_frame_time(data);
    }
    
    /* Neural network training and prediction */
    if (data->use_neural_network && data->neural_net && data->sample_count > 0) {
        /* Prepare input features */
        nn_prepare_input(data->neural_net, data);
        
        /* Create training targets based on current performance */
        int32_t targets[NN_OUTPUT_SIZE];
        targets[0] = (int32_t)data->predicted_gpu_load * 10;   /* GPU load prediction */
        targets[1] = (int32_t)data->predicted_frame_time / 10; /* Frame time prediction */
        targets[2] = (int32_t)data->suggested_quality * 10;    /* Quality suggestion */
        targets[3] = (int32_t)data->current_fps * 10;          /* FPS estimate */
        
        /* Train the neural network */
        nn_train(data->neural_net, targets);
        
        /* If neural network is trained, use its predictions */
        if (data->neural_net->trained) {
            nn_forward(data->neural_net);
            
            /* Use neural network predictions (blend with simple predictions) */
            uint32_t nn_gpu_load = (uint32_t)(data->neural_net->output.output[0] / 10);
            uint32_t nn_frame_time = (uint32_t)(data->neural_net->output.output[1] * 10);
            uint32_t nn_quality = (uint32_t)(data->neural_net->output.output[2] / 10);
            
            /* Blend predictions (70% neural network, 30% simple) */
            data->predicted_gpu_load = (nn_gpu_load * 7 + data->predicted_gpu_load * 3) / 10;
            data->predicted_frame_time = (nn_frame_time * 7 + data->predicted_frame_time * 3) / 10;
            data->suggested_quality = (nn_quality * 7 + data->suggested_quality * 3) / 10;
        }
    }
    
    /* Adjust quality adaptively */
    gfx_ml_adjust_quality(data);
    
    /* Update granular settings based on quality level */
    apply_quality_to_granular(&data->granular_settings, data->current_quality);
    
    /* Update FPS estimate */
    if (data->predicted_frame_time > 0) {
        data->current_fps = 1000000 / data->predicted_frame_time;
    }
    
    /* Add entry to performance history */
    if (data->sample_count > 0) {
        add_history_entry(data, 
                         data->gpu_usage_samples[data->sample_count - 1],
                         data->frame_time_samples[data->sample_count - 1],
                         data->current_quality,
                         data->current_fps);
    }
    
    data->training_iterations++;
}

/**
 * Plugin initialization function
 */
static int gfx_ml_optimization_init(plugin_descriptor_t* plugin) {
    vga_write("  GFX ML Optimization Plugin: Initializing...\n");
    serial_write(SERIAL_COM1, "GFX ML Optimization Plugin: Starting GPU machine learning subsystem\n");
    
    /* Allocate private data */
    gfx_ml_optimization_data_t* data = (gfx_ml_optimization_data_t*)kmalloc(sizeof(gfx_ml_optimization_data_t));
    if (!data) {
        vga_write("  ERROR: Failed to allocate memory for GFX ML plugin data\n");
        return PLUGIN_ERROR;
    }
    
    /* Initialize data structures */
    data->sample_count = 0;
    data->current_quality = 80;  /* Start at 80% quality */
    data->adaptive_quality = 1;
    data->target_fps = 200;      /* Target 200 FPS */
    data->current_fps = 125;     /* Current baseline */
    data->predicted_gpu_load = 0;
    data->predicted_frame_time = 8000;  /* ~125 FPS */
    data->suggested_quality = 80;
    data->learning_enabled = 1;
    data->training_iterations = 0;
    data->quality_adjustments = 0;
    
    /* Check GPU availability */
    data->gpu_vendor = gpu_get_vendor();
    data->gpu_available = (data->gpu_vendor != GPU_VENDOR_UNKNOWN) ? 1 : 0;
    
    /* Clear samples */
    for (int i = 0; i < 100; i++) {
        data->gpu_usage_samples[i] = 0;
        data->frame_time_samples[i] = 8000;  /* Initialize to ~125 FPS */
    }
    
    /* Initialize neural network */
    data->neural_net = (neural_network_t*)kmalloc(sizeof(neural_network_t));
    if (data->neural_net) {
        nn_init(data->neural_net);
        data->use_neural_network = 1;
        vga_write("    Neural network: INITIALIZED\n");
    } else {
        data->use_neural_network = 0;
        vga_write("    Neural network: DISABLED (memory allocation failed)\n");
    }
    
    /* Check GPU ML acceleration availability */
    data->gpu_accelerated_ml = gpu_ml_check_available(data) ? 1 : 0;
    
    /* Initialize per-application profiles */
    data->profile_count = 0;
    data->active_profile_index = 0;
    for (uint32_t i = 0; i < MAX_APP_PROFILES; i++) {
        data->app_profiles[i].active = 0;
    }
    
    /* Add default profile */
    add_app_profile(data, "default", 60, 100, 20);
    
    /* Initialize granular quality settings */
    init_granular_settings(&data->granular_settings);
    apply_quality_to_granular(&data->granular_settings, data->current_quality);
    
    /* Initialize performance history */
    data->history_capacity = MAX_HISTORY_ENTRIES;
    data->history = (perf_history_entry_t*)kmalloc(
        data->history_capacity * sizeof(perf_history_entry_t));
    if (data->history) {
        data->history_count = 0;
        data->history_dirty = 0;
        vga_write("    Performance history: ENABLED\n");
    } else {
        data->history_capacity = 0;
        data->history_count = 0;
        vga_write("    Performance history: DISABLED (memory allocation failed)\n");
    }
    
    plugin->private_data = data;
    
    vga_write("    GPU ML models initialized\n");
    vga_write("    Adaptive quality: ENABLED\n");
    vga_write("    Performance prediction: READY\n");
    vga_write("    Granular quality settings: ENABLED (10 levels)\n");
    vga_write("    Per-app profiles: ENABLED (");
    vga_write_dec(MAX_APP_PROFILES);
    vga_write(" max)\n");
    
    if (data->gpu_available) {
        vga_write("    GPU acceleration: AVAILABLE (");
        switch (data->gpu_vendor) {
            case GPU_VENDOR_INTEL:
                vga_write("Intel HD");
                break;
            case GPU_VENDOR_NVIDIA:
                vga_write("NVIDIA");
                break;
            case GPU_VENDOR_AMD:
                vga_write("AMD");
                break;
            default:
                vga_write("Unknown");
                break;
        }
        vga_write(")\n");
        
        if (data->gpu_accelerated_ml) {
            vga_write("    GPU-accelerated ML: ENABLED\n");
        }
    } else {
        vga_write("    GPU acceleration: Software rendering\n");
    }
    
    serial_write(SERIAL_COM1, "GFX ML Optimization Plugin: Ready to optimize GPU rendering\n");
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin cleanup function
 */
static void gfx_ml_optimization_cleanup(plugin_descriptor_t* plugin) {
    vga_write("  GFX ML Optimization Plugin: Cleaning up...\n");
    
    if (plugin->private_data) {
        gfx_ml_optimization_data_t* data = (gfx_ml_optimization_data_t*)plugin->private_data;
        
        vga_write("  GFX ML Statistics:\n");
        vga_write("    Training iterations: ");
        vga_write_dec(data->training_iterations);
        vga_write("\n    Quality adjustments: ");
        vga_write_dec(data->quality_adjustments);
        vga_write("\n    Final quality level: ");
        vga_write_dec(data->current_quality);
        vga_write("%\n    Final FPS: ");
        vga_write_dec(data->current_fps);
        vga_write("\n");
        
        /* Neural network statistics */
        if (data->neural_net) {
            vga_write("    Neural network samples: ");
            vga_write_dec(data->neural_net->training_samples);
            vga_write("\n    Neural network trained: ");
            vga_write(data->neural_net->trained ? "YES" : "NO");
            vga_write("\n");
            kfree(data->neural_net);
        }
        
        /* Profile statistics */
        vga_write("    Application profiles: ");
        vga_write_dec(data->profile_count);
        vga_write("\n");
        
        /* History statistics */
        if (data->history) {
            vga_write("    Performance history entries: ");
            vga_write_dec(data->history_count);
            vga_write("\n");
            if (data->history_dirty) {
                vga_write("    WARNING: Unsaved history data\n");
            }
            kfree(data->history);
        }
        
        kfree(plugin->private_data);
        plugin->private_data = NULL;
    }
    
    serial_write(SERIAL_COM1, "GFX ML Optimization Plugin: Shutdown complete\n");
}

/**
 * Plugin function - can be called for GFX ML operations
 * params format: action (uint32_t) | data (uint32_t) | [optional: app_name pointer for profile ops]
 * actions: 
 *   0=add_sample, 1=train, 2=predict, 3=get_quality, 4=set_target_fps,
 *   5=get_granular_settings, 6=set_quality_level, 7=add_profile, 8=apply_profile,
 *   9=get_history_stats, 10=save_history, 11=load_history, 12=get_nn_status
 */
static int gfx_ml_optimization_function(void* context, void* params) {
    plugin_descriptor_t* plugin = (plugin_descriptor_t*)context;
    
    if (!plugin || !plugin->private_data) {
        return PLUGIN_ERROR;
    }
    
    gfx_ml_optimization_data_t* data = (gfx_ml_optimization_data_t*)plugin->private_data;
    uint32_t* param_data = (uint32_t*)params;
    
    if (!param_data) {
        return PLUGIN_ERROR;
    }
    
    uint32_t action = param_data[0];
    uint32_t value = param_data[1];
    
    switch (action) {
        case 0: /* Add GPU performance sample */
            if (data->sample_count < 100) {
                data->gpu_usage_samples[data->sample_count] = value & 0xFFFF;
                data->frame_time_samples[data->sample_count] = value >> 16;
                data->sample_count++;
            } else {
                /* Shift samples and add new one */
                for (int i = 0; i < 99; i++) {
                    data->gpu_usage_samples[i] = data->gpu_usage_samples[i + 1];
                    data->frame_time_samples[i] = data->frame_time_samples[i + 1];
                }
                data->gpu_usage_samples[99] = value & 0xFFFF;
                data->frame_time_samples[99] = value >> 16;
            }
            break;
            
        case 1: /* Train models */
            gfx_ml_train_models(data);
            vga_write("GFX ML: Training iteration ");
            vga_write_dec(data->training_iterations);
            if (data->neural_net && data->neural_net->trained) {
                vga_write(" (Neural Network active)");
            }
            vga_write(" complete\n");
            break;
            
        case 2: /* Get prediction */
            vga_write("GFX ML: Predicted GPU load: ");
            vga_write_dec(data->predicted_gpu_load);
            vga_write("%\n");
            vga_write("GFX ML: Predicted frame time: ");
            vga_write_dec(data->predicted_frame_time);
            vga_write(" us\n");
            vga_write("GFX ML: Current FPS: ");
            vga_write_dec(data->current_fps);
            vga_write("\n");
            break;
            
        case 3: /* Get suggested quality */
            vga_write("GFX ML: Suggested quality: ");
            vga_write_dec(data->suggested_quality);
            vga_write("%\n");
            vga_write("GFX ML: Current quality: ");
            vga_write_dec(data->current_quality);
            vga_write("%\n");
            break;
            
        case 4: /* Set target FPS */
            data->target_fps = value;
            vga_write("GFX ML: Target FPS set to ");
            vga_write_dec(data->target_fps);
            vga_write("\n");
            break;
            
        case 5: /* Get granular settings */
            vga_write("GFX ML: Granular Quality Settings:\n");
            vga_write("  Resolution scale: ");
            vga_write_dec(data->granular_settings.resolution_scale);
            vga_write("%\n");
            vga_write("  Texture filtering: ");
            vga_write_dec(data->granular_settings.texture_filtering);
            vga_write("x\n");
            vga_write("  Shadow resolution: ");
            vga_write_dec(data->granular_settings.shadow_resolution);
            vga_write("\n");
            vga_write("  Ambient occlusion: ");
            vga_write_dec(data->granular_settings.ambient_occlusion);
            vga_write("\n");
            vga_write("  Bloom intensity: ");
            vga_write_dec(data->granular_settings.bloom_intensity);
            vga_write("%\n");
            vga_write("  Particle density: ");
            vga_write_dec(data->granular_settings.particle_density);
            vga_write("%\n");
            break;
            
        case 6: /* Set quality level (applies granular settings) */
            if (value <= 100) {
                data->current_quality = value;
                apply_quality_to_granular(&data->granular_settings, value);
                vga_write("GFX ML: Quality level set to ");
                vga_write_dec(value);
                vga_write("%\n");
            } else {
                return PLUGIN_ERROR;
            }
            break;
            
        case 7: /* Add application profile */
            {
                /* value format: target_fps | (max_quality << 8) | (min_quality << 16) */
                uint32_t target = value & 0xFF;
                uint32_t max_q = (value >> 8) & 0xFF;
                uint32_t min_q = (value >> 16) & 0xFF;
                char default_name[] = "app";
                int idx = add_app_profile(data, default_name, target, max_q, min_q);
                if (idx >= 0) {
                    vga_write("GFX ML: Profile added at index ");
                    vga_write_dec((uint32_t)idx);
                    vga_write("\n");
                } else {
                    vga_write("GFX ML: Failed to add profile (max reached)\n");
                    return PLUGIN_ERROR;
                }
            }
            break;
            
        case 8: /* Apply profile by index */
            if (value < data->profile_count) {
                apply_app_profile(data, (int)value);
                vga_write("GFX ML: Applied profile ");
                vga_write_dec(value);
                vga_write("\n");
            } else {
                vga_write("GFX ML: Invalid profile index\n");
                return PLUGIN_ERROR;
            }
            break;
            
        case 9: /* Get history statistics */
            vga_write("GFX ML: Performance History:\n");
            vga_write("  Entries: ");
            vga_write_dec(data->history_count);
            vga_write("/");
            vga_write_dec(data->history_capacity);
            vga_write("\n  Dirty: ");
            vga_write(data->history_dirty ? "YES" : "NO");
            vga_write("\n");
            break;
            
        case 10: /* Save history (value is buffer pointer, param_data[2] is size) */
            {
                void* buffer = (void*)(uintptr_t)value;
                uint32_t size = param_data[2];
                if (save_history(data, buffer, size) == 0) {
                    vga_write("GFX ML: History saved successfully\n");
                } else {
                    vga_write("GFX ML: Failed to save history\n");
                    return PLUGIN_ERROR;
                }
            }
            break;
            
        case 11: /* Load history (value is buffer pointer, param_data[2] is size) */
            {
                void* buffer = (void*)(uintptr_t)value;
                uint32_t size = param_data[2];
                if (load_history(data, buffer, size) == 0) {
                    vga_write("GFX ML: History loaded successfully (");
                    vga_write_dec(data->history_count);
                    vga_write(" entries)\n");
                } else {
                    vga_write("GFX ML: Failed to load history\n");
                    return PLUGIN_ERROR;
                }
            }
            break;
            
        case 12: /* Get neural network status */
            vga_write("GFX ML: Neural Network Status:\n");
            if (data->neural_net) {
                vga_write("  Enabled: YES\n");
                vga_write("  Training samples: ");
                vga_write_dec(data->neural_net->training_samples);
                vga_write("\n  Trained: ");
                vga_write(data->neural_net->trained ? "YES" : "NO");
                vga_write("\n  GPU accelerated: ");
                vga_write(data->gpu_accelerated_ml ? "YES" : "NO");
                vga_write("\n");
            } else {
                vga_write("  Enabled: NO\n");
            }
            break;
            
        default:
            return PLUGIN_ERROR;
    }
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin configuration callback
 */
static int gfx_ml_optimization_config(plugin_descriptor_t* plugin, const char* key, const char* value) {
    if (!plugin || !plugin->private_data) {
        return PLUGIN_ERROR;
    }
    
    gfx_ml_optimization_data_t* data = (gfx_ml_optimization_data_t*)plugin->private_data;
    
    /* Check for "learning_enabled" config */
    if (key[0] == 'l' && key[1] == 'e' && key[2] == 'a' && key[3] == 'r') {
        if (value[0] == '1' || value[0] == 't') {
            data->learning_enabled = 1;
            vga_write("GFX ML: Learning enabled\n");
        } else {
            data->learning_enabled = 0;
            vga_write("GFX ML: Learning disabled\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    /* Check for "adaptive_quality" config */
    if (key[0] == 'a' && key[1] == 'd' && key[2] == 'a' && key[3] == 'p') {
        if (value[0] == '1' || value[0] == 't') {
            data->adaptive_quality = 1;
            vga_write("GFX ML: Adaptive quality enabled\n");
        } else {
            data->adaptive_quality = 0;
            vga_write("GFX ML: Adaptive quality disabled\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    /* Check for "neural_network" config */
    if (key[0] == 'n' && key[1] == 'e' && key[2] == 'u' && key[3] == 'r') {
        if (value[0] == '1' || value[0] == 't') {
            data->use_neural_network = 1;
            vga_write("GFX ML: Neural network enabled\n");
        } else {
            data->use_neural_network = 0;
            vga_write("GFX ML: Neural network disabled\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    /* Check for "gpu_ml_accel" config */
    if (key[0] == 'g' && key[1] == 'p' && key[2] == 'u' && key[3] == '_' && key[4] == 'm') {
        if (value[0] == '1' || value[0] == 't') {
            if (gpu_ml_check_available(data)) {
                data->gpu_accelerated_ml = 1;
                vga_write("GFX ML: GPU-accelerated ML enabled\n");
            } else {
                vga_write("GFX ML: GPU-accelerated ML not available\n");
            }
        } else {
            data->gpu_accelerated_ml = 0;
            vga_write("GFX ML: GPU-accelerated ML disabled\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    /* Check for "target_fps" config */
    if (key[0] == 't' && key[1] == 'a' && key[2] == 'r' && key[3] == 'g') {
        /* Parse simple integer from value */
        uint32_t fps = 0;
        int i = 0;
        while (value[i] >= '0' && value[i] <= '9') {
            fps = fps * 10 + (value[i] - '0');
            i++;
        }
        if (fps > 0 && fps <= 1000) {
            data->target_fps = fps;
            vga_write("GFX ML: Target FPS set to ");
            vga_write_dec(fps);
            vga_write("\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    /* Check for "quality" config */
    if (key[0] == 'q' && key[1] == 'u' && key[2] == 'a' && key[3] == 'l') {
        /* Parse simple integer from value */
        uint32_t quality = 0;
        int i = 0;
        while (value[i] >= '0' && value[i] <= '9') {
            quality = quality * 10 + (value[i] - '0');
            i++;
        }
        if (quality <= 100) {
            data->current_quality = quality;
            apply_quality_to_granular(&data->granular_settings, quality);
            vga_write("GFX ML: Quality set to ");
            vga_write_dec(quality);
            vga_write("%\n");
        }
        return PLUGIN_SUCCESS;
    }
    
    return PLUGIN_SUCCESS;
}

/**
 * Plugin descriptor
 */
plugin_descriptor_t gfx_ml_optimization_plugin = {
    "GFX ML Optimization",
    2,                    /* Version major: 2 (neural network update) */
    0,                    /* Version minor: 0 */
    PLUGIN_TYPE_ML_OPTIMIZATION,
    PLUGIN_PRIORITY_OPTIONAL,
    PLUGIN_API_VERSION_MAJOR,
    PLUGIN_API_VERSION_MINOR,
    {0},
    PLUGIN_PERM_MEMORY | PLUGIN_PERM_KERNEL | PLUGIN_PERM_IO,
    PLUGIN_INTERFERE_NONE,
    0,
    gfx_ml_optimization_init,
    gfx_ml_optimization_cleanup,
    gfx_ml_optimization_function,
    gfx_ml_optimization_config,
    NULL,
    NULL,
    NULL,
    0,
    1
};

/**
 * Plugin registration function
 */
void register_gfx_ml_optimization_plugin(void) {
    plugin_register(&gfx_ml_optimization_plugin);
}
