/*
 * libretro.cpp - DraStic Libretro Core Implementation
 * Based on drastic.cpp main function
 */

#define DRASTIC_LIBRETRO

// 定义 RETRO_API 宏（确保函数被导出）
#define RETRO_API __attribute__((visibility("default")))

#include "libretro.h"

// 先包含系统头文件，避免与 drastic_functions.h 中的声明冲突
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "drastic_val.h"
#include "drastic_functions.h"
#include "sdl_wrapper.h"

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>

// 全局变量声明（在 drastic_val.h 中已声明）
// extern undefined1 nds_system[];  // 在 libretro.c 中定义

// 静态变量
static bool initialized = false;
static bool game_loaded = false;
static retro_environment_t environ_cb = nullptr;
static retro_video_refresh_t video_cb = nullptr;
static retro_audio_sample_t audio_cb = nullptr;
static retro_audio_sample_batch_t audio_batch_cb = nullptr;
static retro_input_poll_t input_poll_cb = nullptr;
static retro_input_state_t input_state_cb = nullptr;

// ROM 数据
static uint8_t *rom_data = nullptr;
static size_t rom_size = 0;
static char *temp_rom_path = nullptr;

// 视频相关
static int frame_width = 256 * 2;  // 双屏并排宽度
static int frame_height = 192;      // 单屏高度
static uint16_t *frame_buffer = nullptr;
static uint16_t *screen0_buffer = nullptr;
static uint16_t *screen1_buffer = nullptr;

// 调试标志
static bool debug_enabled = true;

// 辅助函数：创建临时文件
static char* create_temp_rom_file(const uint8_t *data, size_t size) {
    char tmpname[] = "/tmp/drastic_rom_XXXXXX";
    int fd = mkstemp(tmpname);
    if (fd < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] ERROR: Failed to create temp file\n");
        }
        return nullptr;
    }
    
    ssize_t written = write(fd, data, size);
    close(fd);
    
    if (written != (ssize_t)size) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] ERROR: Failed to write ROM data to temp file\n");
        }
        unlink(tmpname);
        return nullptr;
    }
    
    char *path = (char*)malloc(strlen(tmpname) + 1);
    if (path) {
        strcpy(path, tmpname);
    }
    
    return path;
}

// Libretro API 实现

RETRO_API void retro_init(void) {
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Starting initialization...\n");
    }
    
    // 1. 初始化系统（参考 drastic.cpp main 函数 line 168）
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Calling initialize_system\n");
    }
    initialize_system((long)nds_system);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: initialize_system completed\n");
    }
    
    // 2. 初始化屏幕（参考 drastic.cpp main 函数 line 179）
    // 从 nds_system[0x362e9a9] 读取屏幕参数，如果无效则使用默认值 8
    uint screen_param = *(uint*)(nds_system + 0x362e9a9);
    if (screen_param == 0 || screen_param > 0x100) {
        screen_param = 8;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Calling initialize_screen (param=%u)\n", screen_param);
    }
    initialize_screen(screen_param);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: initialize_screen completed\n");
    }
    
    // 3. 分配帧缓冲区
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Allocating frame buffers (%dx%d)\n", frame_width, frame_height);
    }
    
    frame_buffer = (uint16_t*)calloc(frame_width * frame_height, sizeof(uint16_t));
    screen0_buffer = (uint16_t*)calloc(256 * 192, sizeof(uint16_t));
    screen1_buffer = (uint16_t*)calloc(256 * 192, sizeof(uint16_t));
    
    if (!frame_buffer || !screen0_buffer || !screen1_buffer) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: Failed to allocate frame buffers\n");
        }
        return;
    }
    
    initialized = true;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Initialization complete\n");
    }
}

RETRO_API void retro_deinit(void) {
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_deinit: Called\n");
    }
    
    if (frame_buffer) {
        free(frame_buffer);
        frame_buffer = nullptr;
    }
    
    if (screen0_buffer) {
        free(screen0_buffer);
        screen0_buffer = nullptr;
    }
    
    if (screen1_buffer) {
        free(screen1_buffer);
        screen1_buffer = nullptr;
    }
    
    if (rom_data) {
        free(rom_data);
        rom_data = nullptr;
    }
    
    if (temp_rom_path) {
        unlink(temp_rom_path);
        free(temp_rom_path);
        temp_rom_path = nullptr;
    }
    
    initialized = false;
    game_loaded = false;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_deinit: Deinitialization complete\n");
    }
}

RETRO_API unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port;
    (void)device;
}

RETRO_API void retro_get_system_info(struct retro_system_info *info) {
    static const char library_name[] = "DraStic";
    static const char library_version[] = "r2.5.2.2";
    static const char valid_extensions[] = "nds|bin";
    
    if (!info) {
        return;
    }
    
    memset(info, 0, sizeof(*info));
    info->library_name = library_name;
    info->library_version = library_version;
    info->valid_extensions = valid_extensions;
    info->need_fullpath = false;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info) {
    if (!info) {
        return;
    }
    
    memset(info, 0, sizeof(*info));
    
    float aspect = (float)frame_width / (float)frame_height;
    
    info->geometry.base_width = frame_width;
    info->geometry.base_height = frame_height;
    info->geometry.max_width = frame_width;
    info->geometry.max_height = frame_height;
    info->geometry.aspect_ratio = aspect;
    
    info->timing.fps = 60.0;
    info->timing.sample_rate = 32768.0;
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

RETRO_API void retro_reset(void) {
    if (!initialized || !game_loaded) {
        return;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_reset: Calling reset_system\n");
    }
    
    reset_system((undefined8*)nds_system);
}

RETRO_API int retro_load_game(const struct retro_game_info *info) {
    // 参考 drastic.cpp main 函数的游戏加载流程
    // 流程：验证参数 -> 准备ROM文件路径 -> load_nds -> set_screen_menu_off -> reset_system
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Called\n");
        if (info) {
            fprintf(stderr, "[DRASTIC] retro_load_game: path=%s, data=%p, size=%zu\n",
                    info->path ? info->path : "(null)", (void*)info->data, info->size);
        }
    }
    
    // 1. 验证参数
    if (!info || !info->data || info->size < 512) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: Invalid parameters\n");
        }
        return 0;
    }
    
    // 2. 确保系统已初始化
    if (!initialized) {
        retro_init();
        if (!initialized) {
            return 0;
        }
    }
    
    // 3. 保存 ROM 数据
    rom_size = info->size;
    rom_data = (uint8_t*)malloc(rom_size);
    if (!rom_data) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: Failed to allocate ROM buffer\n");
        }
        return 0;
    }
    memcpy(rom_data, info->data, rom_size);
    
    // 4. 准备 ROM 文件路径
    const char *rom_path = nullptr;
    
    if (info->path && strlen(info->path) > 0) {
        rom_path = info->path;
    } else {
        // 创建临时文件（load_nds 需要文件路径）
        temp_rom_path = create_temp_rom_file(info->data, info->size);
        if (!temp_rom_path) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: Failed to create temp file\n");
            }
            free(rom_data);
            rom_data = nullptr;
            return 0;
        }
        rom_path = temp_rom_path;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: ROM path: %s\n", rom_path);
    }
    
    // 5. 加载 NDS 文件（参考 drastic.cpp main 函数 line 197）
    // load_nds 参数：nds_system + 0x320 (800)
    long load_nds_param = (long)nds_system + 0x320;
    *(long *)(load_nds_param + 0x918) = (long)nds_system;  // 设置 nds_system 指针
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Calling load_nds\n");
    }
    
    int result = load_nds(load_nds_param, (char *)rom_path);
    if (result != 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: load_nds failed (%d)\n", result);
        }
        if (temp_rom_path) {
            unlink(temp_rom_path);
            free(temp_rom_path);
            temp_rom_path = nullptr;
        }
        free(rom_data);
        rom_data = nullptr;
        return 0;
    }
    
    // 6. 设置屏幕菜单关闭（参考 drastic.cpp main 函数 line 205）
    set_screen_menu_off();
    
    // 7. 重置系统（参考 drastic.cpp main 函数 line 209）
    // 注意：在 drastic.cpp 中 reset_system 被注释掉了，但我们应该调用它
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Calling reset_system\n");
    }
    reset_system((undefined8*)nds_system);
    
    // 8. 设置游戏加载标志
    game_loaded = true;
    
    // 9. 设置视频信息和环境变量
    if (environ_cb) {
        // 设置像素格式
        unsigned pixel_format = RETRO_PIXEL_FORMAT_RGB565;
        environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
        
        // 设置系统 AV 信息
        struct retro_system_av_info av_info;
        memset(&av_info, 0, sizeof(av_info));
        retro_get_system_av_info(&av_info);
        environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Game loaded successfully\n");
    }
    
    return 1;
}

RETRO_API void retro_unload_game(void) {
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_unload_game: Called\n");
    }
    
    if (rom_data) {
        free(rom_data);
        rom_data = nullptr;
    }
    
    if (temp_rom_path) {
        unlink(temp_rom_path);
        free(temp_rom_path);
        temp_rom_path = nullptr;
    }
    
    game_loaded = false;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_unload_game: Game unloaded\n");
    }
}

RETRO_API unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

RETRO_API void *retro_get_memory_data(unsigned id) {
    (void)id;
    return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
    (void)id;
    return 0;
}

RETRO_API void retro_run(void) {
    if (!initialized || !game_loaded) {
        return;
    }
    
    // Poll input
    if (input_poll_cb) {
        input_poll_cb();
    }
    
    // 运行一帧（参考 drastic.cpp main 函数 line 217-230）
    // 检查是否已经加载了游戏
    if (*(long *)(nds_system + 0x920) != 0) {
        // 运行 CPU（参考 drastic.cpp main 函数 line 220）
        if (nds_system[0x362e9a8] == '\0') {
            cpu_next_action_arm7_to_event_update(0);
        } else {
            cpu_next_action_arm7_to_event_update(0);
        }
    }
    
    // 复制屏幕数据到帧缓冲区
    // TODO: 实现屏幕数据复制逻辑
    
    // 发送视频帧
    if (video_cb && frame_buffer) {
        size_t pitch = frame_width * sizeof(uint16_t);
        video_cb(frame_buffer, frame_width, frame_height, pitch);
    }
    
    // 发送音频数据
    // TODO: 实现音频输出逻辑
}

} // extern "C"

