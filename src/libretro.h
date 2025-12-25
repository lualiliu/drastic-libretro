/*
 * libretro.h - RetroArch libretro API header
 * This is a minimal version for drastic core
 */

#ifndef LIBRETRO_H
#define LIBRETRO_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RETRO_API_VERSION 1

// Memory types
#define RETRO_MEMORY_MASK 0xff
#define RETRO_MEMORY_SAVE_RAM 0
#define RETRO_MEMORY_RTC 1
#define RETRO_MEMORY_SYSTEM_RAM 2
#define RETRO_MEMORY_VIDEO_RAM 3

// Region codes
#define RETRO_REGION_NTSC 0
#define RETRO_REGION_PAL 1

// Log levels
enum retro_log_level
{
   RETRO_LOG_DEBUG = 0,
   RETRO_LOG_INFO = 1,
   RETRO_LOG_WARN = 2,
   RETRO_LOG_ERROR = 3
};

// Core options
#define RETRO_ENVIRONMENT_EXPERIMENTAL 0x10000
#define RETRO_ENVIRONMENT_SET_ROTATION 1
#define RETRO_ENVIRONMENT_GET_OVERSCAN 2
#define RETRO_ENVIRONMENT_GET_CAN_DUPE 3
#define RETRO_ENVIRONMENT_SET_MESSAGE 4
#define RETRO_ENVIRONMENT_SHUTDOWN 5
#define RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL 6
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY 7
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 8
#define RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS 9
#define RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK 10
#define RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE 11
#define RETRO_ENVIRONMENT_SET_HW_RENDER 12
#define RETRO_ENVIRONMENT_GET_VARIABLE 13
#define RETRO_ENVIRONMENT_SET_VARIABLES 14
#define RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE 15
#define RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME 16
#define RETRO_ENVIRONMENT_GET_LIBRETRO_PATH 17
#define RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK 18
#define RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK 19
#define RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE 20
#define RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES 21
#define RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE 22
#define RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE 23
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE 24
#define RETRO_ENVIRONMENT_GET_PERF_INTERFACE 25
#define RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE 26
#define RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY 27
#define RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY 28
#define RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY 29
#define RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO 30
#define RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK 31
#define RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO 32
#define RETRO_ENVIRONMENT_SET_CONTROLLER_INFO 33
#define RETRO_ENVIRONMENT_SET_MEMORY_MAPS 34
#define RETRO_ENVIRONMENT_SET_GEOMETRY 35
#define RETRO_ENVIRONMENT_GET_USERNAME 36
#define RETRO_ENVIRONMENT_GET_LANGUAGE 37

// Pixel formats
#define RETRO_PIXEL_FORMAT_0RGB1555 0
#define RETRO_PIXEL_FORMAT_XRGB8888 1
#define RETRO_PIXEL_FORMAT_RGB565 2

// Input device types
#define RETRO_DEVICE_NONE         0
#define RETRO_DEVICE_JOYPAD       1
#define RETRO_DEVICE_MOUSE        2
#define RETRO_DEVICE_KEYBOARD     3
#define RETRO_DEVICE_LIGHTGUN     4
#define RETRO_DEVICE_ANALOG       5
#define RETRO_DEVICE_POINTER      6

// Joystick IDs
#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L       10
#define RETRO_DEVICE_ID_JOYPAD_R       11
#define RETRO_DEVICE_ID_JOYPAD_L2      12
#define RETRO_DEVICE_ID_JOYPAD_R2      13
#define RETRO_DEVICE_ID_JOYPAD_L3      14
#define RETRO_DEVICE_ID_JOYPAD_R3      15

struct retro_game_info
{
   const char *path;
   const void *data;
   size_t size;
   const char *meta;
};

struct retro_system_info
{
   const char *library_name;
   const char *library_version;
   const char *valid_extensions;
   int need_fullpath;
   int block_extract;
};

struct retro_game_geometry
{
   unsigned base_width;
   unsigned base_height;
   unsigned max_width;
   unsigned max_height;
   float aspect_ratio;
};

struct retro_system_timing
{
   double fps;
   double sample_rate;
};

struct retro_system_av_info
{
   struct retro_game_geometry geometry;
   struct retro_system_timing timing;
};

struct retro_variable
{
   const char *key;
   const char *value;
};

// Hardware render callback structure
struct retro_hw_render_callback
{
   const char *context_type;  // Context type (e.g., "gl", "vulkan", etc.), NULL = software rendering
   uintptr_t (*get_current_framebuffer)(void);
   void (*get_proc_address)(const char *sym);
   void (*set_resize)(unsigned width, unsigned height);
   uint32_t version_major;
   uint32_t version_minor;
   bool depth;
   bool stencil;
   bool bottom_left_origin;
   bool cache_context;
   void (*context_reset)(void);
   void (*destroy_context)(void);
   bool cache_context_ack;
   void (*debug_callback)(enum retro_log_level level, const char *log);
   void *debug_context;
};

// Callback types
typedef void (*retro_video_refresh_t)(const void *data, unsigned width, unsigned height, size_t pitch);
typedef int16_t (*retro_audio_sample_t)(int16_t left, int16_t right);
typedef size_t (*retro_audio_sample_batch_t)(const int16_t *data, size_t frames);
typedef void (*retro_input_poll_t)(void);
typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);
typedef int (*retro_environment_t)(unsigned cmd, void *data);

// Core functions
void retro_init(void);
void retro_deinit(void);
unsigned retro_api_version(void);
void retro_set_controller_port_device(unsigned port, unsigned device);
void retro_get_system_info(struct retro_system_info *info);
void retro_get_system_av_info(struct retro_system_av_info *info);
void retro_set_environment(retro_environment_t cb);
void retro_set_video_refresh(retro_video_refresh_t cb);
void retro_set_audio_sample(retro_audio_sample_t cb);
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb);
void retro_set_input_poll(retro_input_poll_t cb);
void retro_set_input_state(retro_input_state_t cb);
void retro_reset(void);
int retro_load_game(const struct retro_game_info *info);
void retro_unload_game(void);
unsigned retro_get_region(void);
void *retro_get_memory_data(unsigned id);
size_t retro_get_memory_size(unsigned id);
void retro_run(void);

#ifdef __cplusplus
}
#endif

#endif

