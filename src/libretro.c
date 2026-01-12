/*
 * RetroArch core for DraStic DS Emulator
 * Based on decompiled drastic.cpp
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <wchar.h>

// POSIX headers (guarded for non-POSIX platforms)
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#include "libretro.h"

// 定义 NDS 系统大小（在包含其他头文件之前）
// 注意：这里定义为常量而不是宏，以便其他文件可以通过 extern 引用
// 使用 __attribute__((visibility("default"))) 确保符号被导出
__attribute__((visibility("default"))) const size_t NDS_SYSTEM_SIZE = 62042112;

// 定义 DRASTIC_LIBRETRO 宏，以便头文件知道这是 libretro 模式
#define DRASTIC_LIBRETRO

// 现在包含其他头文件
#include "drastic_cpu.h"
#include "drastic_functions.h"
#include "drastic_val.h"

// 定义 nds_system 数组（在包含头文件之后，因为头文件中可能有 extern 声明）
// 在 libretro 模式下，nds_system 应该是一个全局数组
// 使用内存对齐属性，确保在某些系统上不会出现对齐问题
__attribute__((aligned(16))) undefined1 nds_system[NDS_SYSTEM_SIZE];

// 调试输出标志
static int debug_enabled = 1;  // 可以通过环境变量控制

// Global state
static int initialized = 0;
static int game_loaded = 0;
static uint16_t *frame_buffer = NULL;
static int frame_width = 256 * 2;  // Dual screen width (side by side)
static int frame_height = 192;     // Single screen height
static uint8_t *rom_data = NULL;
static size_t rom_size = 0;
static char *temp_rom_path = NULL;
static int use_recompiler = 0;  // 0 = interpreter, 1 = recompiler

// Libretro callbacks
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Input state - DS按键映射
// DS按键位: A=0, B=1, Select=2, Start=3, Right=4, Left=5, Up=6, Down=7, R=8, L=9
static uint16_t ds_input_state = 0xFFFF;

// 音频缓冲区（用于临时存储音频样本）
#define AUDIO_BUFFER_SIZE 4096
static int16_t audio_buffer[AUDIO_BUFFER_SIZE * 2];  // 立体声：左右声道  // 初始状态为所有按键未按下（低有效）

// 屏幕缓冲区（用于临时存储屏幕数据，使用堆分配避免栈溢出）
// 每个屏幕是 256x192 = 49152 像素（16位，即 256*192 = 49152 个 uint16_t）
#define SCREEN_BUFFER_SIZE (256 * 192)
static uint16_t *screen0_buffer = NULL;
static uint16_t *screen1_buffer = NULL;

// 环境设置标志，确保环境设置只执行一次
static int environment_set = 0;
// AV 信息设置标志，确保 AV 信息只在 video_cb 设置后设置
static int av_info_set = 0;

// 注意：对于软件渲染核心，不应该声明硬件渲染回调结构
// RetroArch 在初始化视频驱动时可能会检测到这个结构并尝试使用它
// 即使我们不调用 SET_HW_RENDER，如果结构存在，RetroArch 仍可能尝试调用其中的回调函数
// 这会导致段错误（如果回调函数是 NULL）

// 核心选项定义
// 根据 libretro API，核心选项格式为 "key;Description|value1|value2|..."
// 当前核心不使用任何选项，在 retro_init 中会设置一个空的变量数组
// 如果将来需要核心选项，可以定义如下：
// static struct retro_variable core_options[] = {
//     { "drastic_cpu_mode", "CPU Mode; Interpreter|Recompiler" },
//     { NULL, NULL }  // 数组必须以 NULL 结尾
// };

// 核心选项值存储（使用默认值）
// static const char *cpu_mode_value = "Interpreter";

// RetroArch API implementation
// 所有 libretro API 函数必须使用 C 链接，以便 RetroArch 能够正确找到符号
extern "C" {

// 注意：构造函数在库加载时立即执行，此时 RetroArch 可能还没有完全初始化
// 在某些情况下，构造函数可能会导致堆栈损坏或其他问题
// 因此，我们完全禁用构造函数，所有初始化都在 retro_init 中进行
// 如果将来需要构造函数，应该确保它不会影响 RetroArch 的初始化
/*
__attribute__((constructor))
static void drastic_libretro_constructor(void) {
    // 使用简单的输出，避免复杂的操作
    const char msg[] = "[DRASTIC] Library loaded, constructor called\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}
*/

void retro_init(void)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Starting initialization...\n");
    }
    
    if (initialized) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: Already initialized, returning\n");
        }
        return;
    }
    
    // Allocate frame buffer for dual screen (side by side: 512x192)
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Allocating frame buffer (%dx%d)\n", frame_width, frame_height);
    }
    // nds_system 是一个全局数组，不需要动态分配，只需要初始化为0
    // 注意：大数组的 memset 可能需要一些时间，但这是必要的
    // 使用分段初始化可能更快，但对于大数组，一次性 memset 通常更高效
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Initializing nds_system array (%zu bytes, %zu MB)...\n", 
                sizeof(nds_system), sizeof(nds_system) / (1024 * 1024));
    }
    memset(nds_system, 0, sizeof(nds_system));  // 初始化为0
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: nds_system array initialized\n");
    }
    
    frame_buffer = (uint16_t*)calloc(frame_width * frame_height, sizeof(uint16_t));
    if (!frame_buffer) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: Failed to allocate frame buffer\n");
        }
        return;
    }
    
    // 分配屏幕缓冲区（使用堆分配避免栈溢出）
    screen0_buffer = (uint16_t*)calloc(SCREEN_BUFFER_SIZE, sizeof(uint16_t));
    screen1_buffer = (uint16_t*)calloc(SCREEN_BUFFER_SIZE, sizeof(uint16_t));
    if (!screen0_buffer || !screen1_buffer) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: Failed to allocate screen buffers\n");
        }
        if (screen0_buffer) free(screen0_buffer);
        if (screen1_buffer) free(screen1_buffer);
        screen0_buffer = NULL;
        screen1_buffer = NULL;
        if (frame_buffer) {
            free(frame_buffer);
            frame_buffer = NULL;
        }
        return;
    }
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Screen buffers allocated (screen0=%p, screen1=%p)\n",
                (void*)screen0_buffer, (void*)screen1_buffer);
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Initializing system...\n");
        fprintf(stderr, "[DRASTIC] retro_init: nds_system address: %p, size: %zu bytes\n", 
                (void*)nds_system, sizeof(nds_system));
    }
    // Initialize drastic system (returns void)
    // 注意：initialize_system 可能会修改 nds_system 的内容
    // 确保 nds_system 已经正确初始化（已通过 memset 完成）
    initialize_system((long)nds_system);
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: System initialized\n");
    }
    
    // 加载 BIOS 文件
    // 获取系统目录路径
    const char *system_dir = NULL;
    if (environ_cb) {
        environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir);
    }
    
    // 如果没有系统目录，使用默认路径
    if (!system_dir || strlen(system_dir) == 0) {
        // 尝试使用常见的 RetroArch 系统目录
        const char *home = getenv("HOME");
        if (home) {
            static char default_system_dir[512];
            snprintf(default_system_dir, sizeof(default_system_dir), "%s/.config/retroarch", home);
            system_dir = default_system_dir;
        } else {
            system_dir = ".";  // 当前目录
        }
    }
    
    // 再次检查 system_dir 是否为 NULL（防御性编程）
    if (!system_dir) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: system_dir is NULL after initialization!\n");
        }
        return;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: System directory: %s\n", system_dir);
    }
    
    // 设置系统目录路径（load_system_file 会从 nds_system + 0x8a780 指向的路径下的 system 子目录加载文件）
    // 将系统目录路径复制到 nds_system + 0x8a780
    // 安全检查：确保 nds_system 不为 NULL 且偏移量在有效范围内
    if (!nds_system) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: nds_system is NULL!\n");
        }
        return;
    }
    
    const size_t path_offset = 0x8a780;
    const size_t max_path_size = 1024;  // 路径缓冲区大小
    if (path_offset + max_path_size > sizeof(nds_system)) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: ERROR: Path offset 0x%zx exceeds system size!\n", path_offset);
        }
        return;
    }
    
    char *base_path = (char *)(nds_system + path_offset);
    size_t system_dir_len = strlen(system_dir);
    
    // 确保路径长度不超过缓冲区大小
    size_t copy_len = (system_dir_len < max_path_size - 1) ? system_dir_len : max_path_size - 1;
    
    // 使用 memcpy 和手动添加 null 终止符，避免 strncpy 的问题
    memcpy(base_path, system_dir, copy_len);
    base_path[copy_len] = '\0';
    
    if (system_dir_len >= max_path_size - 1) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: WARNING: System directory path truncated (length=%zu, max=%zu)\n", 
                    system_dir_len, max_path_size - 1);
        }
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Base path set to: %s\n", base_path);
    }
    
    // 初始化系统目录（这会创建 system 子目录）
    initialize_system_directory((long)nds_system, "system");
    
    // 加载 BIOS（使用 load_system_file 函数）
    // 注意：BIOS 加载失败不应该阻止核心初始化
    // 如果没有 BIOS，核心可能无法正常运行，但至少可以加载
    // ARM9 BIOS: 4KB (0x1000) at offset 0x2004 (或 0x35e4950)
    // ARM7 BIOS: 16KB (0x4000) at offset 0x2204 (或 0x35e5950)
    // 根据 source_ref.cpp，初始化时使用 0x2004 和 0x2204
    // load_system_file 返回 undefined8 (unsigned long)，但实际返回 -1 (0xffffffff) 表示失败，0 表示成功
    long bios_result = 0;
    
    // 尝试加载 ARM9 BIOS
    undefined8 arm9_result = load_system_file((long)nds_system, (undefined8)(unsigned long)(char*)"nds_bios_arm9.bin", 
                                        nds_system + 0x2004, 0x1000);
    if ((long)arm9_result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: Can't find Nintendo ARM9 BIOS. Trying free DraStic ARM9 BIOS.\n");
        }
        arm9_result = load_system_file((long)nds_system, (undefined8)(unsigned long)(char*)"drastic_bios_arm9.bin", 
                                        nds_system + 0x2004, 0x1000);
        if ((long)arm9_result >= 0) {
            // 设置标志位（bit 2）
            nds_system[0xfd512] |= 2;
        }
    }
    
    // 尝试加载 ARM7 BIOS
    undefined8 arm7_result = load_system_file((long)nds_system, (undefined8)(unsigned long)(char*)"nds_bios_arm7.bin", 
                                        nds_system + 0x2204, 0x4000);
    if ((long)arm7_result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: Can't find Nintendo ARM7 BIOS. Trying free DraStic ARM7 BIOS.\n");
        }
        arm7_result = load_system_file((long)nds_system, (undefined8)(unsigned long)(char*)"drastic_bios_arm7.bin", 
                                        nds_system + 0x2204, 0x4000);
    }
    
    if ((long)arm9_result < 0 || (long)arm7_result < 0) {
        bios_result = -1;
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: WARNING: BIOS files not found. Core will continue but may not work properly.\n");
            fprintf(stderr, "[DRASTIC] retro_init: Please place BIOS files in: %s/system/\n", system_dir);
            fprintf(stderr, "[DRASTIC] retro_init: Required files: nds_bios_arm9.bin (4KB), nds_bios_arm7.bin (16KB)\n");
        }
        // 不返回错误，允许核心继续初始化
    } else {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: BIOS files loaded successfully\n");
        }
    }
    
    initialized = 1;
    
    // 使用默认设置
    use_recompiler = 0;  // 默认使用解释器模式
    
    // 明确提供一个空的变量数组，告诉 RetroArch 这个核心没有选项
    // 这样可以防止 RetroArch 尝试解析可能损坏的配置文件
    // 必须在系统完全初始化后调用，此时 RetroArch 的核心选项管理器已经准备好
    if (environ_cb) {
        // 注意：不在 retro_init 中设置任何视频相关的环境变量
        // SET_PIXEL_FORMAT 和 SET_SYSTEM_AV_INFO 都应该在游戏加载后调用
        // 在 retro_init 中设置这些会导致 RetroArch 过早尝试初始化视频驱动，而此时视频回调可能还未设置
        // 这会导致 "Cannot open video driver" 错误
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: Note: Video settings (SET_PIXEL_FORMAT, SET_SYSTEM_AV_INFO) will be set in retro_load_game after game is loaded\n");
            fprintf(stderr, "[DRASTIC] retro_init: Using software rendering (no hardware render callbacks)\n");
        }
        
        // 定义一个只包含终止符的空数组
        static const struct retro_variable vars;
        
        // 设置空的变量数组，明确告诉 RetroArch 没有核心选项
        // 注意：不在 retro_init 中调用 SET_VARIABLES，因为这可能导致 RetroArch 尝试访问未初始化的硬件渲染回调
        //bool vars_result = environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, &vars);
        bool vars_result = true;
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: SET_VARIABLES skipped (will be set later if needed)\n");
        }
        
        // 设置不支持无游戏模式
        bool no_content = false;
        //environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_init: SET_SUPPORT_NO_GAME skipped (will be set in retro_run)\n");
        }
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_init: Core has no options, using default settings\n");
        fprintf(stderr, "[DRASTIC] retro_init: Initialization complete\n");
    }
}

void retro_deinit(void)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_deinit: Called (initialized=%d, game_loaded=%d)\n", initialized, game_loaded);
    }
    
    if (!initialized) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_deinit: Not initialized, returning\n");
        }
        return;
    }
    
    retro_unload_game();
    
    if (frame_buffer) {
        free(frame_buffer);
        frame_buffer = NULL;
    }
    
    // 释放屏幕缓冲区
    if (screen0_buffer) {
        free(screen0_buffer);
        screen0_buffer = NULL;
    }
    if (screen1_buffer) {
        free(screen1_buffer);
        screen1_buffer = NULL;
    }
    
    if (temp_rom_path) {
        unlink(temp_rom_path);  // 删除临时文件
        free(temp_rom_path);
        temp_rom_path = NULL;
    }
    // nds_system 是全局数组，不需要释放
    initialized = 0;
    game_loaded = 0;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_deinit: Deinitialization complete\n");
    }
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    // Handle controller device changes if needed
}

void retro_get_system_info(struct retro_system_info *info)
{
    // 使用静态字符串，确保指针在库的生命周期内有效
    static const char library_name[] = "DraStic";
    static const char library_version[] = "r2.5.2.2";
    static const char valid_extensions[] = "nds|bin";
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_system_info: Called\n");
        fflush(stderr);
    }
    
    if (!info) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_get_system_info: ERROR: info is NULL!\n");
            fflush(stderr);
        }
        return;
    }
    
    memset(info, 0, sizeof(*info));
    info->library_name = library_name;
    info->library_version = library_version;
    info->valid_extensions = valid_extensions;
    info->need_fullpath = false;
    info->block_extract = false;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_system_info: Returning name=%s, version=%s, extensions=%s, need_fullpath=%d\n",
                info->library_name, info->library_version, info->valid_extensions, info->need_fullpath);
        fflush(stderr);
    }
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_system_av_info: Called (game_loaded=%d)\n", game_loaded);
    }
    
    if (!info) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_get_system_av_info: ERROR: info is NULL!\n");
        }
        return;
    }
    
    // 确保结构体被清零
    memset(info, 0, sizeof(*info));
    
    float aspect = (float)frame_width / (float)frame_height;
    
    info->geometry.base_width = frame_width;
    info->geometry.base_height = frame_height;
    info->geometry.max_width = frame_width;
    info->geometry.max_height = frame_height;
    info->geometry.aspect_ratio = aspect;
    
    info->timing.fps = 60.0;
    info->timing.sample_rate = 32768.0;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_system_av_info: Returning geometry=%dx%d, aspect=%.3f, fps=%.2f, sample_rate=%.2f\n",
                info->geometry.base_width, info->geometry.base_height, info->geometry.aspect_ratio,
                info->timing.fps, info->timing.sample_rate);
    }
}

void retro_set_environment(retro_environment_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_environment: Called with cb=%p\n", (void*)cb);
    }
    
    environ_cb = cb;
    
    if (!cb) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_set_environment: cb is NULL, returning\n");
        }
        return;
    }
    
    // 对于软件渲染核心，不应该初始化硬件渲染回调结构
    // RetroArch 在初始化视频驱动时可能会检查并尝试使用硬件渲染回调
    // 如果回调结构被初始化但某些字段是 NULL，会导致段错误
    // 因此，我们完全不清除或初始化这个结构，让 RetroArch 知道我们使用纯软件渲染
    // 注意：我们不调用 SET_HW_RENDER，因为我们使用软件渲染
    // 根据 libretro API，核心应该只在需要硬件渲染时调用 SET_HW_RENDER
    // 如果核心不调用 SET_HW_RENDER，RetroArch 会自动使用软件渲染
    
    // 注意：不要在 retro_set_environment 中立即调用 SET_VARIABLES
    // 这会导致 RetroArch 的核心选项管理器在未完全初始化时被调用，导致 SIGSEGV
    // SET_VARIABLES 将在 retro_init 中延迟调用，此时系统已完全初始化
    // 在 retro_init 中会设置一个空的变量数组，明确告诉 RetroArch 这个核心没有选项
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_environment: Environment callback registered (using software rendering, SET_VARIABLES will be called in retro_init)\n");
    }
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_video_refresh: Called with cb=%p\n", (void*)cb);
    }
    video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_audio_sample: Called with cb=%p\n", (void*)cb);
    }
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_audio_sample_batch: Called with cb=%p\n", (void*)cb);
    }
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_input_poll: Called with cb=%p\n", (void*)cb);
    }
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_set_input_state: Called with cb=%p\n", (void*)cb);
    }
    input_state_cb = cb;
}

void retro_reset(void)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_reset: Called (initialized=%d, game_loaded=%d)\n", initialized, game_loaded);
    }
    if (initialized) {
        reset_system((long)nds_system);
    }
}

int retro_load_game(const struct retro_game_info *info)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Called with info=%p\n", (void*)info);
        if (info) {
            fprintf(stderr, "[DRASTIC] retro_load_game: info->path=%s, info->data=%p, info->size=%zu\n",
                    info->path ? info->path : "(null)", (void*)info->data, info->size);
        }
    }
    
    if (!info || !info->data) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: Invalid parameters, returning 0\n");
        }
        return 0;
    }
    
    // Initialize system if not already done
    if (!initialized) {
        retro_init();
        if (!initialized) {
            return 0;
        }
    }
    
    // Unload previous game if any
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Unloading previous game if any (game_loaded=%d)\n", game_loaded);
    }
    //retro_unload_game();
    
    // 注意：不在这里调用环境回调
    // RetroArch 在处理环境回调时可能会查询核心选项，导致段错误
    // 环境设置将在 retro_run 中进行，此时系统已完全初始化且游戏已加载
    
    rom_size = info->size;
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: ROM size: %zu bytes (%.2f MB)\n",
                rom_size, rom_size / (1024.0 * 1024.0));
    }
    
    rom_data = (uint8_t*)malloc(rom_size);
    if (!rom_data) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] ERROR: Failed to allocate ROM data buffer\n");
        }
        return 0;
    }
    
    memcpy(rom_data, info->data, rom_size);
    
    // 检查 ROM 文件头（NDS 文件头验证）
    if (debug_enabled && rom_size >= 512) {
        // NDS 文件头在偏移 0x00-0x0F 处有游戏标题
        // 偏移 0x0C 处有游戏代码（4字节）
        // 偏移 0x68 处有 ARM9 程序入口点
        // 偏移 0x6C 处有 ARM9 ROM 地址
        // 偏移 0x70 处有 ARM9 程序大小
        // 偏移 0x78 处有 ARM7 ROM 地址
        // 偏移 0x7C 处有 ARM7 程序大小
        uint32_t *rom_header = (uint32_t*)rom_data;
        char game_title[13] = {0};
        memcpy(game_title, rom_data, 12);
        uint32_t game_code = rom_header[3];  // 偏移 0x0C
        uint32_t arm9_entry = rom_header[26];  // 偏移 0x68
        uint32_t arm9_rom_addr = rom_header[27];  // 偏移 0x6C
        uint32_t arm9_size = rom_header[28];  // 偏移 0x70
        uint32_t arm7_entry = rom_header[29];  // 偏移 0x74 (ARM7 入口点)
        uint32_t arm7_rom_addr = rom_header[30];  // 偏移 0x78
        uint32_t arm7_size = rom_header[31];  // 偏移 0x7C
        
        fprintf(stderr, "[DRASTIC] ROM Header Info:\n");
        fprintf(stderr, "  Game Title: %.12s\n", game_title);
        fprintf(stderr, "  Game Code: 0x%08x\n", game_code);
        fprintf(stderr, "  ARM9 Entry: 0x%08x, ROM Addr: 0x%08x, Size: 0x%08x (%u bytes)\n",
                arm9_entry, arm9_rom_addr, arm9_size, arm9_size);
        fprintf(stderr, "  ARM7 Entry: 0x%08x, ROM Addr: 0x%08x, Size: 0x%08x (%u bytes)\n",
                arm7_entry, arm7_rom_addr, arm7_size, arm7_size);
        
        // 检查 ARM7 大小是否为 0
        // 注意：某些 NDS ROM 可能确实有 ARM7 大小为 0 的情况
        // 但通常 ARM7 程序是必需的，因为它负责系统功能（如音频、WiFi 等）
        if (arm7_size == 0) {
            fprintf(stderr, "[DRASTIC] WARNING: ARM7 program size is 0!\n");
            fprintf(stderr, "[DRASTIC] NOTE: This ROM may be incomplete or corrupted.\n");
            fprintf(stderr, "[DRASTIC] NOTE: DraStic requires BIOS files (nds_bios_arm9.bin, nds_bios_arm7.bin) for proper emulation.\n");
            fprintf(stderr, "[DRASTIC] NOTE: However, ARM7 size 0 is unusual and may indicate a ROM format issue.\n");
        }
        
        // 检查 ARM9 ROM 地址是否合理
        // 正常的 NDS ROM 中，ARM9 和 ARM7 程序通常在 ROM 文件的开头部分
        // ARM9 ROM 地址通常是一个相对较小的值（在 ROM 文件内）
        if (arm9_rom_addr > 0x10000000) {
            fprintf(stderr, "[DRASTIC] WARNING: ARM9 ROM address (0x%08x) seems unusually high\n",
                    arm9_rom_addr);
            fprintf(stderr, "[DRASTIC] NOTE: This might indicate the ROM address is an absolute address rather than file offset.\n");
        }
        
        // 检查 ARM7 ROM 地址
        if (arm7_rom_addr == 0 && arm7_size > 0) {
            fprintf(stderr, "[DRASTIC] WARNING: ARM7 ROM address is 0 but size is non-zero (0x%08x)!\n",
                    arm7_size);
            fprintf(stderr, "[DRASTIC] NOTE: This indicates a malformed ROM header.\n");
        } else if (arm7_rom_addr == 0 && arm7_size == 0 && arm7_entry != 0) {
            fprintf(stderr, "[DRASTIC] WARNING: ARM7 ROM address and size are both 0, but entry point is non-zero (0x%08x)!\n",
                    arm7_entry);
            fprintf(stderr, "[DRASTIC] NOTE: This ROM header appears inconsistent. The ROM may be corrupted or use an unusual format.\n");
        } else if (arm7_rom_addr > 0x10000000 && arm7_size > 0) {
            fprintf(stderr, "[DRASTIC] WARNING: ARM7 ROM address (0x%08x) seems unusually high\n",
                    arm7_rom_addr);
        }
        fprintf(stderr, "  First 16 bytes: ");
        for (int i = 0; i < 16; i++) {
            fprintf(stderr, "%02x ", rom_data[i]);
        }
        fprintf(stderr, "\n");
    }
    
    // load_nds 需要文件路径，所以我们需要创建一个临时文件
    // 使用 info->path 如果可用，否则创建临时文件
    const char *rom_path = NULL;
    
    if (info->path && strlen(info->path) > 0) {
        rom_path = info->path;
    } else {
        // 创建临时文件路径（使用mkstemp替代tmpnam）
        char tmpname_template[] = "/tmp/drastic_rom_XXXXXX";
        int fd = mkstemp(tmpname_template);
        if (fd < 0) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_load_game: ERROR: Failed to create temp file\n");
            }
            free(rom_data);
            rom_data = NULL;
            return 0;
        }
        close(fd);  // 关闭文件描述符，我们只需要文件名
        temp_rom_path = (char*)malloc(strlen(tmpname_template) + 1);
        if (!temp_rom_path) {
            unlink(tmpname_template);
            free(rom_data);
            rom_data = NULL;
            return 0;
        }
        strcpy(temp_rom_path, tmpname_template);

        // 写入 ROM 数据到临时文件
        FILE *fp = fopen(temp_rom_path, "wb");
        if (!fp) {
            free(temp_rom_path);
            temp_rom_path = NULL;
            free(rom_data);
            rom_data = NULL;
            return 0;
        }

        size_t written = fwrite(rom_data, 1, rom_size, fp);
        fclose(fp);

        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] Wrote ROM to temp file: %s (%zu bytes)\n",
                    temp_rom_path, written);
        }

        if (written != rom_size) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] ERROR: Failed to write ROM to temp file: wrote %zu/%zu bytes\n",
                        written, rom_size);
            }
            unlink(temp_rom_path);
            free(temp_rom_path);
            temp_rom_path = NULL;
            free(rom_data);
            rom_data = NULL;
            return 0;
        }

        rom_path = temp_rom_path;
    }
    
    if (debug_enabled && info->path && strlen(info->path) > 0) {
        fprintf(stderr, "[DRASTIC] Using ROM file path: %s\n", rom_path);
    }
    
    // 初始化屏幕（如果需要）
    // 根据 drastic.cpp line 4937，initialize_screen 期望从 nds_system[0x3b2a9a9] 读取一个字节值
    // 但根据 line 5888，实际写入的是 nds_system[param_1 + 0x362e9a9]，所以偏移是 0x362e9a9
    // 这个值通常是 8 或 16（对应屏幕位深度），存储为单个字节
    // 注意：在系统初始化之前，这个值可能未初始化，所以使用默认值
    uint8_t screen_param_byte = nds_system[0x362e9a9];
    uint screen_param = (uint)screen_param_byte;
    if (screen_param == 0 || screen_param > 0x100) {
        // 如果值无效，使用默认值 8（对应 16 位颜色）
        screen_param = 8;
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: Using default screen_param=8 (invalid value at offset 0x362e9a9, read 0x%02x)\n", screen_param_byte);
        }
    }
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Calling initialize_screen with param=%u\n", screen_param);
    }
    // 修复 orientation 问题：确保 DAT_040315c4 被正确初始化为 0xffffffffffffffff
    // 这样在屏幕更新时，如果 DAT_040315c4._4_4_ 不等于 -1，才会被复制到 DAT_040315ac._4_4_
    // 否则 DAT_040315ac._4_4_ 会保持为 0（默认值）
    DAT_040315c4 = 0xffffffffffffffffULL;
    DAT_040315ac = 0;  // 确保 DAT_040315ac 也被初始化为 0
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: Initialized DAT_040315c4=0x%016llx, DAT_040315ac=0x%016llx\n", 
                (unsigned long long)DAT_040315c4, (unsigned long long)DAT_040315ac);
    }
    initialize_screen(screen_param);
    set_screen_menu_off();
    
    // 加载 NDS 文件
    // 根据 drastic.cpp main 函数，load_nds 的第一个参数是 nds_system + 0x320 (800 in decimal)
    // load_nds 会从 param_1 + 0x918 读取指向 nds_system 的指针
    // 所以我们需要在调用前设置这个指针
    long load_nds_param = (long)nds_system + 0x320;
    *(long *)(load_nds_param + 0x918) = (long)nds_system;  // 设置指向 nds_system 的指针
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: calling load_nds with path=%s\n", rom_path);
        fprintf(stderr, "[DRASTIC] retro_load_game: load_nds_param=%p, nds_system pointer at offset 0x918=%p\n",
                (void*)load_nds_param, (void*)*(long *)(load_nds_param + 0x918));
    }
    int result = load_nds(load_nds_param, (char *)rom_path);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: load_nds returned: %d\n", result);
    }
    
    if (result != 0) {
        // 加载失败
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] ERROR: load_nds failed with result=%d\n", result);
        }
        if (temp_rom_path) {
            unlink(temp_rom_path);
            free(temp_rom_path);
            temp_rom_path = NULL;
        }
        free(rom_data);
        rom_data = NULL;
        return 0;
    }
    
    // 重置系统以开始游戏
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: calling reset_system\n");
    }
    reset_system((long)nds_system);
    
    game_loaded = 1;
    ds_input_state = 0xFFFF;  // 重置输入状态
    
    // 注意：不要在 retro_load_game 中调用大多数环境回调
    // RetroArch 在处理环境回调时可能会查询核心选项，导致段错误
    // 但是 SET_SYSTEM_AV_INFO 是安全的，必须在游戏加载后立即调用
    // 这样 RetroArch 才能知道游戏已加载并正确初始化显示
    // 
    // 重要：确保视频回调已经设置，否则 RetroArch 在初始化视频驱动时可能会出现问题
    // 如果 video_cb 还没有设置，延迟 SET_SYSTEM_AV_INFO 到 retro_run 中
    if (environ_cb && game_loaded) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: Setting video info (video_cb=%p)\n", (void*)video_cb);
        }
        
        // 首先设置像素格式 - 必须在 SET_SYSTEM_AV_INFO 之前设置
        // 这样 RetroArch 在初始化视频驱动时就知道使用什么像素格式
        // 使用 RGB565 格式明确表示我们使用软件渲染（不是硬件渲染）
        unsigned pixel_format = RETRO_PIXEL_FORMAT_RGB565;
        int pixel_format_result = environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: SET_PIXEL_FORMAT called (RGB565, result=%d)\n", pixel_format_result);
            if (!pixel_format_result) {
                fprintf(stderr, "[DRASTIC] retro_load_game: WARNING: SET_PIXEL_FORMAT returned false, RetroArch may not support RGB565\n");
            }
        }
        
        // 检查视频回调是否已设置
        // 如果 video_cb 还没有设置，延迟 SET_SYSTEM_AV_INFO 到 retro_run 中
        // 这样可以避免 RetroArch 在视频回调未设置时尝试初始化视频驱动
        if (video_cb) {
            // 视频回调已设置，可以安全地调用 SET_SYSTEM_AV_INFO
            struct retro_system_av_info av_info;
            memset(&av_info, 0, sizeof(av_info));  // 确保结构体被清零
            retro_get_system_av_info(&av_info);
            
            // 保存调用前的值用于调试（因为 RetroArch 可能会修改结构体）
            unsigned int before_width = av_info.geometry.base_width;
            unsigned int before_height = av_info.geometry.base_height;
            
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_load_game: Before SET_SYSTEM_AV_INFO: geometry=%ux%u, aspect=%.3f, fps=%.2f, sample_rate=%.2f\n",
                        before_width, before_height, av_info.geometry.aspect_ratio,
                        av_info.timing.fps, av_info.timing.sample_rate);
            }
            
            int result = environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
            
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_load_game: SET_SYSTEM_AV_INFO result=%d\n", result);
                fprintf(stderr, "[DRASTIC] retro_load_game: Note: av_info may be modified by RetroArch after call\n");
            }
            
            av_info_set = 1;  // 标记 AV 信息已设置
        } else {
            // 视频回调还未设置，延迟 SET_SYSTEM_AV_INFO 到 retro_run 中
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_load_game: WARNING: video_cb is NULL! Delaying SET_SYSTEM_AV_INFO to retro_run.\n");
                fprintf(stderr, "[DRASTIC] retro_load_game: This prevents RetroArch from initializing video driver before video callback is set.\n");
            }
            av_info_set = 0;  // 标记 AV 信息未设置，将在 retro_run 中设置
        }
        
        // 注意：不在 retro_load_game 中调用 SET_SUPPORT_NO_GAME
        // 即使是在 SET_SYSTEM_AV_INFO 之后调用，仍然会触发核心选项管理器查询，导致 SIGSEGV
        // SET_SUPPORT_NO_GAME 将在 retro_run 的第一次调用时设置
        // 虽然这可能导致 RetroArch 暂时认为核心支持无游戏模式，但一旦 retro_run 被调用，就会正确设置
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_load_game: SET_SUPPORT_NO_GAME will be set in retro_run to avoid SIGSEGV\n");
        }
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_load_game: game loaded successfully\n");
        fprintf(stderr, "[DRASTIC] retro_load_game: game_loaded flag set to %d\n", game_loaded);
        fprintf(stderr, "[DRASTIC] retro_load_game: initialized=%d, game_loaded=%d\n", initialized, game_loaded);
        fprintf(stderr, "[DRASTIC] retro_load_game: Returning 1 (success)\n");
    }
    
    // 确保返回 1 表示成功
    return 1;
}

void retro_unload_game(void)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_unload_game: Called (game_loaded=%d)\n", game_loaded);
    }
    
    if (rom_data) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_unload_game: Freeing ROM data (%zu bytes)\n", rom_size);
        }
        free(rom_data);
        rom_data = NULL;
        rom_size = 0;
    }
    
    if (temp_rom_path) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_unload_game: Removing temp file: %s\n", temp_rom_path);
        }
        unlink(temp_rom_path);
        free(temp_rom_path);
        temp_rom_path = NULL;
    }
    
    game_loaded = 0;
    environment_set = 0;  // 重置环境设置标志，以便下次加载游戏时重新设置
    av_info_set = 0;      // 重置 AV 信息设置标志，以便下次加载游戏时重新设置
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_unload_game: Game unloaded (game_loaded=%d)\n", game_loaded);
    }
}

unsigned retro_get_region(void)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_region: Called (game_loaded=%d), returning RETRO_REGION_NTSC\n", game_loaded);
    }
    return RETRO_REGION_NTSC; // DS is region-free but NTSC timing
}

void *retro_get_memory_data(unsigned id)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_memory_data: Called with id=%u (game_loaded=%d)\n", id, game_loaded);
    }
    
    // Return memory regions if needed
    switch (id) {
        case RETRO_MEMORY_SAVE_RAM:
            // NDS 保存 RAM (SRAM) - 通常存储在 nds_system 的特定区域
            // 如果没有游戏加载，返回 NULL 是正常的
            if (!game_loaded) {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] retro_get_memory_data: SAVE_RAM requested but no game loaded, returning NULL\n");
                }
                return NULL;
            }
            // TODO: 返回实际的保存 RAM 地址
            // 目前返回 NULL 表示没有保存 RAM（某些游戏可能不需要）
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_data: SAVE_RAM requested, returning NULL (not implemented yet)\n");
            }
            return NULL;
        case RETRO_MEMORY_RTC:
            // NDS 实时时钟 (RTC) - 通常不需要
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_data: RTC requested, returning NULL (not needed for NDS)\n");
            }
            return NULL;
        case RETRO_MEMORY_SYSTEM_RAM:
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_data: Returning SYSTEM_RAM pointer=%p\n", (void*)nds_system);
            }
            return nds_system;
        default:
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_data: Unknown id=%u, returning NULL\n", id);
            }
            return NULL;
    }
}

size_t retro_get_memory_size(unsigned id)
{
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_get_memory_size: Called with id=%u (game_loaded=%d)\n", id, game_loaded);
    }
    
    switch (id) {
        case RETRO_MEMORY_SAVE_RAM:
            // NDS 保存 RAM 大小 - 通常是 512KB 或更小
            // 如果没有游戏加载，返回 0 是正常的
            if (!game_loaded) {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] retro_get_memory_size: SAVE_RAM requested but no game loaded, returning 0\n");
                }
                return 0;
            }
            // TODO: 返回实际的保存 RAM 大小
            // 目前返回 0 表示没有保存 RAM
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_size: SAVE_RAM requested, returning 0 (not implemented yet)\n");
            }
            return 0;
        case RETRO_MEMORY_RTC:
            // NDS 实时时钟大小 - 通常不需要
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_size: RTC requested, returning 0 (not needed for NDS)\n");
            }
            return 0;
        case RETRO_MEMORY_SYSTEM_RAM:
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_size: Returning SYSTEM_RAM size=62042112\n");
            }
            return 62042112;
        default:
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_get_memory_size: Unknown id=%u, returning 0\n", id);
            }
            return 0;
    }
}

// 将 RetroArch 输入映射到 DS 按键
static void update_ds_input(void)
{
    if (!input_state_cb) {
        return;
    }
    
    // DS 按键寄存器格式（低有效，即 0=按下，1=未按下）
    // Bit 0: A
    // Bit 1: B
    // Bit 2: Select
    // Bit 3: Start
    // Bit 4: Right
    // Bit 5: Left
    // Bit 6: Up
    // Bit 7: Down
    // Bit 8: R
    // Bit 9: L
    // Bit 10-15: 未使用
    
    uint16_t input = 0xFFFF;  // 初始状态：所有按键未按下
    
    // 读取 RetroArch 输入并映射到 DS 按键
    // 使用端口 0
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
        input &= ~(1 << 0);  // A
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) {
        input &= ~(1 << 1);  // B
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT)) {
        input &= ~(1 << 2);  // Select
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START)) {
        input &= ~(1 << 3);  // Start
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
        input &= ~(1 << 4);  // Right
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
        input &= ~(1 << 5);  // Left
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) {
        input &= ~(1 << 6);  // Up
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN)) {
        input &= ~(1 << 7);  // Down
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)) {
        input &= ~(1 << 8);  // R
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L)) {
        input &= ~(1 << 9);  // L
    }
    
    ds_input_state = input;
    
    // 调用 update_input 以更新输入系统（由 drastic.cpp 提供）
    // update_input 的参数是 nds_system + 0xAAA (2730 in decimal)
    // 注意：update_input 在 drastic.cpp 中定义，这里直接调用
    extern void update_input(long param_1);
    long input_offset = (long)nds_system + 0xAAA;
    update_input(input_offset);
}

// 保存状态支持（可选但推荐实现）
size_t retro_serialize_size(void)
{
    // 返回保存状态所需的大小
    // 对于 NDS 模拟器，需要保存整个系统状态
    // 这里返回 nds_system 的大小加上一些额外的元数据
    return sizeof(nds_system) + 1024;  // 额外空间用于元数据
}

bool retro_serialize(void *data, size_t size)
{
    if (!initialized || !game_loaded || !data) {
        return false;
    }
    
    // 检查缓冲区大小
    if (size < sizeof(nds_system)) {
        return false;
    }
    
    // 简单实现：直接复制整个系统状态
    // 实际实现可能需要更复杂的序列化逻辑
    memcpy(data, nds_system, sizeof(nds_system));
    
    return true;
}

bool retro_unserialize(const void *data, size_t size)
{
    if (!initialized || !game_loaded || !data) {
        return false;
    }
    
    // 检查数据大小
    if (size < sizeof(nds_system)) {
        return false;
    }
    
    // 简单实现：直接恢复整个系统状态
    // 实际实现可能需要更复杂的反序列化逻辑
    memcpy(nds_system, data, sizeof(nds_system));
    
    return true;
}

// 作弊系统支持（可选但推荐实现）
void retro_cheat_reset(void)
{
    // 重置所有作弊码
    // 对于 NDS 模拟器，这里可以清除所有已应用的作弊码
    // 当前实现为空，因为还没有实现作弊系统
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    // 设置作弊码
    // index: 作弊码索引
    // enabled: 是否启用
    // code: 作弊码字符串
    // 当前实现为空，因为还没有实现作弊系统
    (void)index;
    (void)enabled;
    (void)code;
}

// 加载特殊类型游戏（可选函数）
// 用于加载光盘、子系统等特殊类型的游戏
// NDS 模拟器通常不需要这个功能，但 RetroArch 可能会查找这个符号
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    // NDS 模拟器不支持特殊游戏类型，直接调用普通的加载函数
    (void)game_type;
    (void)num_info;
    
    if (!info) {
        return false;
    }
    
    // 对于 NDS，特殊加载和普通加载是一样的
    return retro_load_game(info) != 0;
}

void retro_run(void)
{

    // 立即输出调试信息，确保即使函数立即返回也能看到
    // 这必须在函数的最开始，在任何条件检查之前
    static int frame_count = 0;
    static int first_frame = 1;
    static int call_count = 0;
    
    call_count++;
    
    // 总是输出前几次调用的调试信息，以便诊断问题
    // 使用 fflush 确保输出立即刷新
    // 这必须在任何 return 语句之前执行
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] retro_run: ENTRY #%d - initialized=%d, game_loaded=%d, frame_count=%d\n", 
                call_count, initialized, game_loaded, frame_count);
        fflush(stderr);
        
        if (first_frame) {
            first_frame = 0;
        }
    }
    
    // 即使游戏未加载，也输出前几次调用的调试信息
    if (!initialized || !game_loaded) {
        if (debug_enabled && call_count <= 10) {
            fprintf(stderr, "[DRASTIC] retro_run: Skipping execution (call_count=%d, initialized=%d, game_loaded=%d, frame=%d)\n", 
                    call_count, initialized, game_loaded, frame_count);
        }
        return;
    }
    
    frame_count++;
    
    if (debug_enabled && frame_count == 1) {
        fprintf(stderr, "[DRASTIC] retro_run: First frame execution (call_count=%d, initialized=%d, game_loaded=%d)\n", 
                call_count, initialized, game_loaded);
    }
    
    // 在第一次运行时设置环境变量（延迟设置，避免在加载游戏时崩溃）
    // 这必须在游戏加载后且 retro_run 被调用时进行
    // 此时 RetroArch 的核心选项管理器已经完全初始化，可以安全地调用环境回调
    // 注意：SET_PIXEL_FORMAT 现在在 retro_load_game 中设置，这里只需要设置 SET_SUPPORT_NO_GAME
    if (!environment_set && environ_cb && game_loaded) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_run: Setting environment variables (first frame, game_loaded=%d)\n", game_loaded);
        }
        
        // Set support no game to false (core requires a game to run)
        // 注意：false 表示核心不支持无游戏模式，必须加载游戏
        // 这必须在 retro_run 中设置，因为在 retro_load_game 中设置会触发 SIGSEGV
        // 虽然这会导致 RetroArch 暂时认为核心支持无游戏模式，但一旦 retro_run 被调用，就会正确设置
        bool no_content = false;
        environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_run: SET_SUPPORT_NO_GAME called (no_content=false)\n");
            fprintf(stderr, "[DRASTIC] retro_run: Note: SET_PIXEL_FORMAT was already set in retro_load_game\n");
        }
        
        environment_set = 1;
    }
    
    // 如果 AV 信息还没有设置（因为 video_cb 在 retro_load_game 时还未设置），现在设置它
    // 此时 video_cb 应该已经设置，可以安全地调用 SET_SYSTEM_AV_INFO
    if (!av_info_set && environ_cb && game_loaded && video_cb) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_run: Setting AV info (delayed from retro_load_game, video_cb=%p)\n", (void*)video_cb);
        }
        
        // 确保像素格式已设置（在设置 AV 信息之前）
        // 这很重要，因为 RetroArch 需要知道使用什么像素格式
        unsigned pixel_format = RETRO_PIXEL_FORMAT_RGB565;
        environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_run: SET_PIXEL_FORMAT called (RGB565) before SET_SYSTEM_AV_INFO\n");
        }
        
        struct retro_system_av_info av_info;
        memset(&av_info, 0, sizeof(av_info));  // 确保结构体被清零
        retro_get_system_av_info(&av_info);
        
        int result = environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] retro_run: SET_SYSTEM_AV_INFO called (delayed, result=%d)\n", result);
            fprintf(stderr, "[DRASTIC] retro_run: geometry=%ux%u, aspect=%.3f, fps=%.2f, sample_rate=%.2f\n",
                    av_info.geometry.base_width, av_info.geometry.base_height, av_info.geometry.aspect_ratio,
                    av_info.timing.fps, av_info.timing.sample_rate);
        }
        
        av_info_set = 1;
    } else if (!av_info_set && environ_cb && game_loaded && !video_cb) {
        // video_cb 仍然未设置，继续等待
        if (debug_enabled && frame_count <= 10) {
            fprintf(stderr, "[DRASTIC] retro_run: Still waiting for video_cb (frame=%d, video_cb=%p)\n", frame_count, (void*)video_cb);
        }
    }
    
    // Poll input
    if (input_poll_cb) {
        input_poll_cb();
    }
    
    // 更新 DS 输入状态
    update_ds_input();
    
    // 运行一帧模拟
    // 根据 drastic.cpp 的 main 函数：
    // - 使用 _setjmp 进行异常处理
    // - 根据 nds_system[0x3b2a9a8] 判断使用解释器还是重编译器
    // - 解释器模式：cpu_next_action_arm7_to_event_update(nds_system)
    // - 重编译器模式：recompiler_entry(nds_system, nds_system._57482784_8_)
    
    // 这里我们需要运行到下一帧
    // 由于 drastic 使用事件驱动模式，我们需要执行事件直到一帧完成
    // 通常这涉及执行到下一个 VSync 事件
    
    // 简化的实现：直接调用模拟器的主循环函数
    // 实际实现可能需要更复杂的逻辑来确保只运行一帧
    // 根据 drastic.cpp，检查是否使用重编译器模式
    if (nds_system[0x3b2a9a8] == '\0') {
        // 解释器模式
        cpu_next_action_arm7_to_event_update(nds_system);
    } else {
        // 重编译器模式
        // 从 nds_system 的偏移 0x57482784 获取 translate_cache
        //undefined8 translate_cache = *(undefined8 *)(nds_system + 0x57482784);
        //recompiler_entry(nds_system, translate_cache);
        cpu_next_action_arm7_to_event_update(nds_system);
    }
    
    // 更新屏幕（渲染）
    // update_screens() 会更新屏幕数据，确保在复制之前屏幕数据是最新的
    update_screens();
    
    // 获取视频缓冲区并发送到 RetroArch
    // 使用 screen_copy16 来复制两个屏幕的数据
    // 上屏（screen 0）和下屏（screen 1）并排放置
    // 注意：即使屏幕是黑色的，也要发送，这样 RetroArch 才能显示窗口
    if (debug_enabled && frame_count <= 5) {
        fprintf(stderr, "[DRASTIC] retro_run frame %d: video_cb=%p, frame_buffer=%p\n", 
                frame_count, (void*)video_cb, (void*)frame_buffer);
    }
    
    if (video_cb) {
        if (!frame_buffer) {
            // 如果 frame_buffer 未分配，重新分配
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] Allocating frame_buffer: %dx%d\n", frame_width, frame_height);
            }
            frame_buffer = (uint16_t*)calloc(frame_width * frame_height, sizeof(uint16_t));
            if (!frame_buffer) {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] ERROR: Failed to allocate frame_buffer\n");
                }
                return;  // 无法分配内存
            }
        }
        // 检查屏幕缓冲区是否已分配（防御性编程）
        if (!screen0_buffer || !screen1_buffer) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] retro_run: ERROR: Screen buffers not allocated! screen0=%p, screen1=%p\n",
                        (void*)screen0_buffer, (void*)screen1_buffer);
            }
            return;  // 无法继续，返回
        }
        
        // 准备临时缓冲区用于复制屏幕数据
        // screen_copy16 会将屏幕数据复制到提供的缓冲区
        // 每个屏幕是 256x192 = 49152 像素（对于16位，即 256*192 = 49152 个 uint16_t）
        // 注意：这些缓冲区现在使用堆分配（在 retro_init 中分配），避免栈溢出
        
        // 复制上屏（screen 0）
        // 先获取屏幕指针，检查是否有效
        void *screen0_ptr = get_screen_ptr(0);
        if (debug_enabled && frame_count <= 5) {
            fprintf(stderr, "[DRASTIC] Screen 0 ptr: %p\n", screen0_ptr);
            if (screen0_ptr == NULL) {
                fprintf(stderr, "[DRASTIC] ERROR: Screen 0 ptr is NULL! Screen data will be empty.\n");
            }
        }
        
        // 尝试复制屏幕数据
        // screen_copy16 会从内部屏幕缓冲区复制数据到提供的缓冲区
        // 即使 screen0_ptr 为 NULL，screen_copy16 内部也会处理
        screen_copy16(screen0_buffer, 0);
        
        // 如果复制后缓冲区仍然全为0，可能是屏幕数据未准备好
        // 检查前几个像素，如果全为0且不是第一帧，可能需要等待
        if (frame_count > 1) {
            int has_data = 0;
            for (int i = 0; i < 256 && !has_data; i++) {
                if (screen0_buffer[i] != 0) has_data = 1;
            }
            if (!has_data && screen0_ptr == NULL && debug_enabled && frame_count <= 10) {
                fprintf(stderr, "[DRASTIC] WARNING: Screen 0 appears empty after copy (frame %d)\n", frame_count);
            }
        }
        
        // 复制下屏（screen 1）
        void *screen1_ptr = get_screen_ptr(1);
        if (debug_enabled && frame_count <= 5) {
            fprintf(stderr, "[DRASTIC] Screen 1 ptr: %p\n", screen1_ptr);
            if (screen1_ptr == NULL) {
                fprintf(stderr, "[DRASTIC] ERROR: Screen 1 ptr is NULL! Screen data will be empty.\n");
            }
        }
        
        // 尝试复制屏幕数据
        screen_copy16(screen1_buffer, 1);
        
        // 检查下屏数据
        if (frame_count > 1) {
            int has_data = 0;
            for (int i = 0; i < 256 && !has_data; i++) {
                if (screen1_buffer[i] != 0) has_data = 1;
            }
            if (!has_data && screen1_ptr == NULL && debug_enabled && frame_count <= 10) {
                fprintf(stderr, "[DRASTIC] WARNING: Screen 1 appears empty after copy (frame %d)\n", frame_count);
            }
        }
        
        // 检查屏幕缓冲区内容（前几个像素）
        if (debug_enabled && frame_count <= 3) {
            fprintf(stderr, "[DRASTIC] Screen 0 first 4 pixels: 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    screen0_buffer[0], screen0_buffer[1], screen0_buffer[2], screen0_buffer[3]);
            fprintf(stderr, "[DRASTIC] Screen 1 first 4 pixels: 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    screen1_buffer[0], screen1_buffer[1], screen1_buffer[2], screen1_buffer[3]);
            
            // 检查屏幕缓冲区是否在 nds_system 范围内
            if (screen0_ptr != NULL) {
                unsigned long screen0_addr = (unsigned long)screen0_ptr;
                unsigned long nds_system_addr = (unsigned long)nds_system;
                unsigned long nds_system_end = nds_system_addr + 62042112;
                
                fprintf(stderr, "[DRASTIC] Screen 0 addr: %p, nds_system: %p-%p, offset: 0x%lx\n",
                        screen0_ptr, (void*)nds_system_addr, (void*)nds_system_end,
                        screen0_addr - nds_system_addr);
            }
            if (screen1_ptr != NULL) {
                unsigned long screen1_addr = (unsigned long)screen1_ptr;
                unsigned long nds_system_addr = (unsigned long)nds_system;
                
                fprintf(stderr, "[DRASTIC] Screen 1 addr: %p, offset: 0x%lx\n",
                        screen1_ptr, screen1_addr - nds_system_addr);
            }
            
            // 检查屏幕缓冲区中间和末尾的像素
            fprintf(stderr, "[DRASTIC] Screen 0 middle pixels (idx 16384-16387): 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    screen0_buffer[16384], screen0_buffer[16385], screen0_buffer[16386], screen0_buffer[16387]);
            fprintf(stderr, "[DRASTIC] Screen 0 last 4 pixels: 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    screen0_buffer[49148], screen0_buffer[49149], screen0_buffer[49150], screen0_buffer[49151]);
            
            // 检查 DAT_04031528 数组状态
            fprintf(stderr, "[DRASTIC] DAT_04031528[0]=%p, DAT_04031528[1]=%p, DAT_04031598=%p\n",
                    DAT_04031528[0], DAT_04031528[1], DAT_04031598);
            fprintf(stderr, "[DRASTIC] DAT_04031540=%d, DAT_040315cc=%d\n",
                    DAT_04031540, DAT_040315cc);
        }

        // 立即检测是否两个屏幕均全黑（前几帧），以便快速发现渲染问题
        if (debug_enabled && frame_count <= 5) {
            int all_zero0 = 1, all_zero1 = 1;
            for (int i = 0; i < 256 * 192; i++) {
                if (screen0_buffer[i] != 0) { all_zero0 = 0; break; }
            }
            for (int i = 0; i < 256 * 192; i++) {
                if (screen1_buffer[i] != 0) { all_zero1 = 0; break; }
            }
            if (all_zero0 && all_zero1) {
                fprintf(stderr, "[DRASTIC] retro_run: WARNING: both screens appear all black on frame %d\n", frame_count);
            }
        }
        
        // 每 60 帧检查一次屏幕数据（1秒）
        if (debug_enabled && frame_count % 60 == 0) {
            // 统计非零像素数量
            int non_zero0 = 0, non_zero1 = 0;
            for (int i = 0; i < 256 * 192; i++) {
                if (screen0_buffer[i] != 0) non_zero0++;
                if (screen1_buffer[i] != 0) non_zero1++;
            }
            fprintf(stderr, "[DRASTIC] Frame %d: Screen 0 non-zero pixels: %d/%d, Screen 1: %d/%d\n",
                    frame_count, non_zero0, 256*192, non_zero1, 256*192);
        }
        
        // 将两个屏幕并排放置到 frame_buffer
        // 左侧是上屏（screen 0），右侧是下屏（screen 1）
        // 每个屏幕是 256x192，组合后是 512x192
        // 使用 memcpy 逐行复制，确保数据正确对齐
        for (int y = 0; y < 192; y++) {
            // 复制上屏的一行到左侧（y * frame_width 是当前行的起始位置）
            uint16_t *dst_left = frame_buffer + y * frame_width;
            uint16_t *src_screen0 = screen0_buffer + y * 256;
            memcpy(dst_left, src_screen0, 256 * sizeof(uint16_t));
            
            // 复制下屏的一行到右侧（偏移256像素）
            uint16_t *dst_right = frame_buffer + y * frame_width + 256;
            uint16_t *src_screen1 = screen1_buffer + y * 256;
            memcpy(dst_right, src_screen1, 256 * sizeof(uint16_t));
        }
        
        // 发送到 RetroArch
        // 注意：即使屏幕是黑色的，也要发送，这样 RetroArch 才能显示窗口
        // pitch 是每行的字节数，对于 RGB565 格式，pitch = width * 2
        size_t pitch = frame_width * sizeof(uint16_t);
        if (debug_enabled && frame_count <= 5) {
            fprintf(stderr, "[DRASTIC] Calling video_cb: width=%d, height=%d, pitch=%zu\n",
                    frame_width, frame_height, pitch);
            // 检查 frame_buffer 的前几个像素值
            fprintf(stderr, "[DRASTIC] Frame buffer first 4 pixels: 0x%04x 0x%04x 0x%04x 0x%04x\n",
                    frame_buffer[0], frame_buffer[1], frame_buffer[2], frame_buffer[3]);
        }
        
        // 调用视频回调函数，将画面数据发送给 RetroArch
        // RetroArch 会负责将数据渲染到屏幕上
        video_cb(frame_buffer, frame_width, frame_height, pitch);
        
        if (debug_enabled && frame_count == 5) {
            fprintf(stderr, "[DRASTIC] First 5 frames processed, reducing debug output\n");
        }
    } else {
        if (debug_enabled && frame_count <= 5) {
            fprintf(stderr, "[DRASTIC] WARNING: video_cb not set! video_cb=%p\n", (void*)video_cb);
        }
    }
    
    // 处理音频
    // 从 SPU 获取音频样本
    // SPU 位于 nds_system + 0x1587000 (根据 initialize_spu 的偏移)
    if (audio_batch_cb) {
        long spu_offset = (long)nds_system + 0x1587000;

        // 渲染音频样本（32768Hz，每帧约546个样本）
        int samples_per_frame = (int)(32768.0 / 60.0 + 0.5); // ~546

        // 临时累加器数组（每个样本为一个 64-bit 累加对：低32位/高32位）
        // 使用堆分配以避免过大的栈消耗
        uint64_t *accum = (uint64_t*)calloc(samples_per_frame, sizeof(uint64_t));
        if (!accum) return;

        // 先更新所有通道设置，避免在渲染过程中出现需要在循环内部更新并导致死循环的情况
        // 注意：spu_update_all_channel_settings 不存在，如果需要可以循环调用 spu_update_channel_settings
        // spu_update_all_channel_settings(spu_offset);

        // 调用 SPU 渲染，将混音结果写入 accum（每元素为两个32位累加值）
        spu_render_samples(spu_offset, (undefined8*)accum, samples_per_frame);
        // 处理 capture（麦克风），与 drastic.cpp 的流程一致（channel 0/1）
        //spu_render_capture(spu_offset, (undefined8)accum, samples_per_frame, 0);
        //spu_render_capture(spu_offset, (undefined8)accum, samples_per_frame, 1);

        // 将累加值转换为 int16_t 输出（右移 12 来匹配 drastic 的 >>0xc 缩放）
        for (int i = 0; i < samples_per_frame; i++) {
            int32_t low = (int32_t)(accum[i] & 0xFFFFFFFFUL);
            int32_t high = (int32_t)((accum[i] >> 32) & 0xFFFFFFFFUL);

            int32_t l = low >> 12;
            int32_t r = high >> 12;
            if (l < -32768) l = -32768;
            if (l > 32767) l = 32767;
            if (r < -32768) r = -32768;
            if (r > 32767) r = 32767;

            audio_buffer[i * 2 + 0] = (int16_t)l;
            audio_buffer[i * 2 + 1] = (int16_t)r;
        }

        // 发送到 RetroArch（帧数为 samples_per_frame）
        audio_batch_cb(audio_buffer, samples_per_frame);

        free(accum);
    }
}

} // extern "C" - 结束所有 libretro API 函数的 C 链接块

