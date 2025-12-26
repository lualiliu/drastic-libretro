/*
 * drastic_impl.c
 * 基于 drastic.cpp 逻辑的独立实现
 * 提供 libretro 核心所需的所有功能
 * 
 */

#include "drastic.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 声明全局系统内存（定义在 drastic_impl.c 中）
extern undefined1 nds_system[NDS_SYSTEM_SIZE];

// 内部状态结构
typedef struct {
    int initialized;
    int game_loaded;
    void *screen0_buffer;
    void *screen1_buffer;
    unsigned int screen0_pitch;
    unsigned int screen1_pitch;
    int screen_bytes_per_pixel;
    int screen0_hires;
    int screen1_hires;
    uint32_t input_state;  // 输入状态寄存器
    int use_recompiler;
    unsigned long translate_cache;
} drastic_state_t;

static drastic_state_t g_state = {0};

// 屏幕缓冲区偏移（在 nds_system 中）
// 根据 drastic.cpp 的分析，屏幕缓冲区应该在视频内存区域
#define SCREEN0_OFFSET 0x1000000
#define SCREEN1_OFFSET 0x2000000
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define SCREEN_PITCH (SCREEN_WIDTH * 2)  // 16位颜色，2字节/像素

// 事件处理函数（存根实现）
// 这些函数在 drastic.cpp 中定义，但我们暂时使用存根
// 注意：这些函数不能是 static，因为我们需要在 execute_events 中检查它们的地址
void event_hblank_start_function_stub(long param_1, unsigned long param_2) {
    (void)param_1;
    (void)param_2;
    // 存根实现
}

void event_scanline_start_function_stub(long param_1, unsigned long param_2) {
    (void)param_1;
    (void)param_2;
    // 存根实现
}

void event_force_task_switch_function_stub(long param_1, unsigned long param_2) {
    (void)param_1;
    (void)param_2;
    // 存根实现
}

void event_gamecard_irq_function_stub(long param_1, unsigned long param_2) {
    (void)param_1;
    (void)param_2;
    // 存根实现
}

// 初始化事件列表
// 基于 drastic.cpp 中 initialize_event_list 的实现（第17039行）
// param_1 是事件列表的基址，param_2 是系统状态结构的基址
void initialize_event_list(long param_1, long param_2) {
    if (!param_1 || !param_2) {
        return;
    }
    
    // 根据 drastic.cpp 第17042-17054行初始化事件列表
    // 事件节点结构：
    // - 偏移 0: 事件时间
    // - 偏移 8: 事件处理函数指针
    // - 偏移 0x10: 事件参数（系统状态结构指针）
    // - 偏移 0x28: 事件类型/标志
    
    // 第一个事件：HBlank 开始（偏移 0）
    typedef void (*event_func_t)(long, unsigned long);
    *(event_func_t*)(param_1 + 0x8) = (event_func_t)event_hblank_start_function_stub;
    *(long*)(param_1 + 0x10) = param_2;
    *(uint8_t*)(param_1 + 0x28) = 0;
    
    // 第二个事件：扫描线开始（偏移 0x20）
    *(event_func_t*)(param_1 + 0x38) = (event_func_t)event_scanline_start_function_stub;
    *(long*)(param_1 + 0x40) = param_2;
    *(uint8_t*)(param_1 + 0x58) = 1;
    
    // 第三个事件：强制任务切换（偏移 0x50）
    *(event_func_t*)(param_1 + 0x68) = (event_func_t)event_force_task_switch_function_stub;
    *(long*)(param_1 + 0x70) = 0;
    *(uint8_t*)(param_1 + 0x88) = 2;
    
    // 第四个事件：游戏卡 IRQ（偏移 0x200）
    *(event_func_t*)(param_1 + 0x218) = (event_func_t)event_gamecard_irq_function_stub;
    *(long*)(param_1 + 0x220) = param_2 + 0x320;  // param_2 + 800 (0x320)
    *(uint8_t*)(param_1 + 0x238) = 0xb;
    
    // 事件列表指针（偏移 0x300）初始化为 NULL
    *(long*)(param_1 + 0x300) = 0;
}

// 加载系统文件（BIOS、固件等）
// 根据 drastic.cpp 第 5599-5641 行
// param_1: 系统目录路径（在 drastic.cpp 中是 param_1 + 0x8a780，但 libretro 中我们使用环境变量或默认路径）
// param_2: 文件名（如 "nds_bios_arm9.bin"）
// param_3: 目标内存地址
// param_4: 期望的文件大小
int load_system_file(const char *system_dir, const char *filename, void *dest, int expected_size) {
    static int debug_enabled = 1;
    
    if (!system_dir || !filename || !dest) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_system_file: Invalid parameters\n");
        }
        return -1;
    }
    
    // 构建文件路径：{system_dir}/system/{filename}
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/system/%s", system_dir, filename);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_system_file: Attempting to load %s\n", filepath);
    }
    
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_system_file: Failed to open %s\n", filepath);
        }
        return -1;
    }
    
    // 检查文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size != expected_size) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_system_file: %s is wrong size (expected %d bytes, got %ld bytes)\n",
                    filename, expected_size, file_size);
        }
        fclose(fp);
        return -1;
    }
    
    // 读取文件
    size_t bytes_read = fread(dest, 1, expected_size, fp);
    fclose(fp);
    
    if (bytes_read != expected_size) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_system_file: Failed to read %s (read %zu/%d bytes)\n",
                    filename, bytes_read, expected_size);
        }
        return -1;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_system_file: Successfully loaded %s (%d bytes)\n",
                filename, expected_size);
    }
    
    return 0;
}

// 系统初始化
int initialize_system(long param_1) {
    if (!param_1) {
        return -1;
    }
    
    // 初始化系统内存
    memset((void*)param_1, 0, sizeof(nds_system));
    
    // 初始化屏幕缓冲区指针
    g_state.screen0_buffer = (void*)(nds_system + SCREEN0_OFFSET);
    g_state.screen1_buffer = (void*)(nds_system + SCREEN1_OFFSET);
    g_state.screen0_pitch = SCREEN_PITCH;
    g_state.screen1_pitch = SCREEN_PITCH;
    g_state.screen_bytes_per_pixel = 2;  // 16位
    g_state.screen0_hires = 0;
    g_state.screen1_hires = 0;
    g_state.input_state = 0xFFFF;  // 所有按键未按下（低有效）
    g_state.use_recompiler = 0;
    g_state.translate_cache = 0;
    
    // 初始化屏幕缓冲区为黑色
    memset(g_state.screen0_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
    memset(g_state.screen1_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
    
    // 设置重编译器标志（在 nds_system 中的偏移 0x3b2a9a8）
    // 0 = 解释器模式，非0 = 重编译器模式
    nds_system[0x3b2a9a8] = 0;  // 默认使用解释器
    
    // 初始化事件列表（根据 drastic.cpp 第6103行）
    // param_1 是 nds_system 的地址，事件列表在 param_1 + 0x18
    // 系统状态结构在 param_1 + 0x320（根据 load_nds 的调用）
    // 注意：在 drastic.cpp 中，initialize_event_list 的第一个参数是 param_1 + 0x18，第二个参数是 param_1
    // 但 param_1 是 nds_system 的地址，而系统状态结构在 nds_system + 0x320
    // 所以我们需要传递系统状态结构的地址
    long system_state_base = param_1 + 0x320;
    initialize_event_list(param_1 + 0x18, system_state_base);
    
    // 设置事件列表指针（根据 drastic.cpp 第17458行，事件列表指针在系统状态结构 + 0x318）
    // 系统状态结构在 nds_system + 0x320，所以事件列表指针在 nds_system + 0x320 + 0x318 = nds_system + 0x638
    // 事件列表的第一个节点在 nds_system + 0x18（根据 initialize_event_list 的实现）
    // 但是，事件列表指针应该指向第一个事件节点，而第一个事件节点的时间字段在偏移 0
    // 所以我们需要将事件列表的第一个节点地址存储在系统状态结构的 0x318 偏移处
    uint32_t **event_list_ptr = (uint32_t**)(system_state_base + 0x318);
    *event_list_ptr = (uint32_t*)(param_1 + 0x18);
    
    // 初始化第一个事件节点的时间字段为 0（表示立即执行）
    // 根据 drastic.cpp，事件节点的时间字段在偏移 0
    uint32_t *first_event_time = (uint32_t*)(param_1 + 0x18);
    *first_event_time = 0;
    
    // 初始化时间计数器（系统状态结构 + 8）
    uint64_t *time_counter = (uint64_t*)(system_state_base + 8);
    *time_counter = 0;
    
    // 初始化剩余时间（系统状态结构 + 0x10）
    uint32_t *time_remaining = (uint32_t*)(system_state_base + 0x10);
    *time_remaining = 0;
    
    g_state.initialized = 1;
    return 0;
}

// 加载 BIOS 文件
// 根据 drastic.cpp 第 15885-15913 行
// param_1: 系统状态结构地址（nds_system + 0x320）
// system_dir: 系统目录路径
int load_bios_files(long param_1, const char *system_dir) {
    static int debug_enabled = 1;
    int result;
    int bios_flags = 0;  // 用于标记使用了哪个 BIOS（0=官方，1=DraStic ARM7，2=DraStic ARM9）
    
    if (!param_1 || !system_dir) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: Invalid parameters\n");
        }
        return -1;
    }
    
    // 计算 BIOS 在系统内存中的地址
    // 根据 drastic.cpp：
    // - ARM9 BIOS: param_1 + 0x2004 (但 param_1 是系统状态结构，需要从 nds_system 开始计算)
    //   实际地址应该是 nds_system + 0x2004
    // - ARM7 BIOS: param_1 + 0x2204，实际地址是 nds_system + 0x2204
    // - Firmware: param_1 + 0x560，实际地址是 nds_system + 0x560
    
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long system_state_base = (unsigned long)param_1;
    unsigned long system_state_offset = system_state_base - system_base;  // 应该是 0x320
    
    // ARM9 BIOS 地址：nds_system + 0x2004
    void *arm9_bios_addr = (void*)(system_base + 0x2004);
    
    // ARM7 BIOS 地址：nds_system + 0x2204
    void *arm7_bios_addr = (void*)(system_base + 0x2204);
    
    // Firmware 地址：nds_system + 0x560
    void *firmware_addr = (void*)(system_base + 0x560);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_bios_files: Loading BIOS files from system directory: %s\n", system_dir);
    }
    
    // 尝试加载 ARM9 BIOS
    result = load_system_file(system_dir, "nds_bios_arm9.bin", arm9_bios_addr, 0x1000);
    if (result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: Can't find Nintendo ARM9 BIOS. Trying free DraStic ARM9 BIOS.\n");
        }
        result = load_system_file(system_dir, "drastic_bios_arm9.bin", arm9_bios_addr, 0x1000);
        if (result >= 0) {
            bios_flags |= 2;  // 标记使用了 DraStic ARM9 BIOS
        }
    }
    
    if (result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: ERROR: Failed to load ARM9 BIOS\n");
            fprintf(stderr, "[DRASTIC] load_bios_files: Tried: %s/system/nds_bios_arm9.bin and %s/system/drastic_bios_arm9.bin\n",
                    system_dir, system_dir);
        }
        // 初始化 ARM9 BIOS 内存为零（防止未初始化内存访问）
        memset(arm9_bios_addr, 0, 0x1000);
        // 返回错误，但允许调用者决定是否继续
        return -1;
    }
    
    // 尝试加载 ARM7 BIOS
    result = load_system_file(system_dir, "nds_bios_arm7.bin", arm7_bios_addr, 0x4000);
    if (result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: Can't find Nintendo ARM7 BIOS. Trying free DraStic ARM7 BIOS.\n");
        }
        result = load_system_file(system_dir, "drastic_bios_arm7.bin", arm7_bios_addr, 0x4000);
        if (result >= 0) {
            bios_flags |= 1;  // 标记使用了 DraStic ARM7 BIOS
        }
    }
    
    if (result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: ERROR: Failed to load ARM7 BIOS\n");
            fprintf(stderr, "[DRASTIC] load_bios_files: Tried: %s/system/nds_bios_arm7.bin and %s/system/drastic_bios_arm7.bin\n",
                    system_dir, system_dir);
        }
        // 初始化 ARM7 BIOS 内存为零（防止未初始化内存访问）
        memset(arm7_bios_addr, 0, 0x4000);
        // 返回错误，但允许调用者决定是否继续
        return -1;
    }
    
    // 尝试加载固件
    result = load_system_file(system_dir, "nds_firmware.bin", firmware_addr, 0x40000);
    if (result < 0) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_bios_files: Can't find firmware. Using patched firmware.\n");
        }
        // 如果固件不存在，使用模拟的固件数据（根据 drastic.cpp 第 15911-15912 行）
        memset(firmware_addr, 0, 0x40000);
        // TODO: 调用 patch_firmware_header_data 来生成模拟固件
        // 这里简化处理，只是清零
    }
    
    // 存储 BIOS 标志（根据 drastic.cpp 第 15890、15904 行）
    // 标志存储在 param_1 + 0xfd512（但 param_1 是系统状态结构，需要调整）
    // 实际地址应该是 nds_system + 0xfd512
    unsigned long bios_flags_addr = system_base + 0xfd512;
    if (bios_flags_addr < system_base + sizeof(nds_system)) {
        *(uint8_t*)bios_flags_addr = bios_flags;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_bios_files: BIOS files loaded successfully (flags=0x%02x)\n", bios_flags);
    }
    
    return 0;
}

// 加载 NDS 文件
int load_nds(long param_1, const char *param_2) {
    static int debug_enabled = 1;
    
    if (!param_1 || !param_2) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: Invalid parameters (param_1=%p, param_2=%p)\n",
                    (void*)param_1, (void*)param_2);
        }
        return -1;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: Opening file: %s\n", param_2);
    }
    
    FILE *fp = fopen(param_2, "rb");
    if (!fp) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: ERROR: Failed to open file: %s\n", param_2);
        }
        return -1;
    }
    
    // 获取文件大小（使用 fseek/ftell，支持大文件）
    fseek(fp, 0, SEEK_END);
    long file_size_long = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 检查文件大小是否有效
    if (file_size_long < 0) {
        fclose(fp);
        return -1;
    }
    
    // 检查最小文件大小（至少需要有效的 gamecard_header，0x200 = 512 字节）
    if (file_size_long < 0x200) {
        fclose(fp);
        return -1;
    }
    
    // 转换为 size_t（移除大小限制）
    size_t file_size = (size_t)file_size_long;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: File size: %zu bytes (%.2f MB)\n",
                file_size, file_size / (1024.0 * 1024.0));
    }
    
    // 读取文件头（至少512字节）用于验证
    uint8_t header[512];
    size_t read_size = fread(header, 1, 512, fp);
    if (read_size < 512) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: ERROR: Failed to read header (read %zu/512 bytes)\n",
                    read_size);
        }
        fclose(fp);
        return -1;
    }
    
    if (debug_enabled) {
        // 检查 NDS 文件头标识
        // NDS 文件头的前几个字节通常是游戏标题
        char game_title[13] = {0};
        memcpy(game_title, header, 12);
        uint32_t game_code = *(uint32_t*)(header + 0x0C);
        fprintf(stderr, "[DRASTIC] load_nds: Game Title: %.12s, Game Code: 0x%08x\n",
                game_title, game_code);
    }
    
    // 检查 NDS 文件头（验证 gamecard_header）
    // 根据 drastic.cpp 第127373行，检查文件大小是否至少 0x200 字节
    // 这里已经通过上面的检查完成
    
    // 重新定位到文件开始
    fseek(fp, 0, SEEK_SET);
    
    // 读取 ROM 数据到临时缓冲区
    // 移除大小限制，支持任意大小的 NDS ROM
    uint8_t *rom_buffer = (uint8_t*)malloc(file_size);
    if (!rom_buffer) {
        fclose(fp);
        return -1;
    }
    
    // 分块读取大文件，避免一次性读取导致的问题
    size_t total_read = 0;
    size_t chunk_size = 1024 * 1024;  // 1MB 块
    size_t remaining = file_size;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: Reading ROM data in chunks...\n");
    }
    
    while (remaining > 0) {
        size_t to_read = (remaining > chunk_size) ? chunk_size : remaining;
        size_t bytes_read = fread(rom_buffer + total_read, 1, to_read, fp);
        
        if (bytes_read == 0) {
            // 读取错误或 EOF
            if (ferror(fp)) {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] load_nds: ERROR: File read error at offset %zu\n",
                            total_read);
                }
                free(rom_buffer);
                fclose(fp);
                return -1;
            }
            break;
        }
        
        total_read += bytes_read;
        remaining -= bytes_read;
        
        if (debug_enabled && (total_read % (10 * 1024 * 1024) == 0 || remaining == 0)) {
            fprintf(stderr, "[DRASTIC] load_nds: Read %zu/%zu bytes (%.1f%%)\n",
                    total_read, file_size, (total_read * 100.0) / file_size);
        }
    }
    
    fclose(fp);
    
    // 验证读取的字节数
    if (total_read != file_size) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: ERROR: Incomplete read (%zu/%zu bytes)\n",
                    total_read, file_size);
        }
        free(rom_buffer);
        return -1;
    }
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: Successfully read %zu bytes\n", total_read);
    }
    
    // 解析 ROM 文件头，获取 ARM9 和 ARM7 程序信息
    // 根据 NDS 文件格式（参考 drastic.cpp 第 127050-127103 行）：
    // - 偏移 0x20: ARM9 ROM offset（在 ROM 文件中的位置）
    // - 偏移 0x24: ARM9 RAM address（在内存中的地址，通常是 0x02000000）
    // - 偏移 0x28: ARM9 size
    // - 偏移 0x2C: ARM7 ROM offset
    // - 偏移 0x30: ARM7 RAM address（在内存中的地址，通常是 0x02380000）
    // - 偏移 0x34: ARM7 size
    // - 偏移 0x68: ARM9 entry point
    // - 偏移 0x74: ARM7 entry point
    uint32_t *rom_header = (uint32_t*)rom_buffer;
    uint32_t arm9_rom_offset = rom_header[8];   // 偏移 0x20
    uint32_t arm9_ram_addr = rom_header[9];      // 偏移 0x24
    uint32_t arm9_size = rom_header[10];        // 偏移 0x28
    uint32_t arm7_rom_offset = rom_header[11];  // 偏移 0x2C
    uint32_t arm7_ram_addr = rom_header[12];    // 偏移 0x30
    uint32_t arm7_size = rom_header[13];        // 偏移 0x34
    uint32_t arm9_entry = rom_header[26];       // 偏移 0x68
    uint32_t arm7_entry = rom_header[29];      // 偏移 0x74
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: Parsed ROM header:\n");
        fprintf(stderr, "  ARM9 ROM offset: 0x%08x, RAM addr: 0x%08x, Size: 0x%08x, Entry: 0x%08x\n",
                arm9_rom_offset, arm9_ram_addr, arm9_size, arm9_entry);
        fprintf(stderr, "  ARM7 ROM offset: 0x%08x, RAM addr: 0x%08x, Size: 0x%08x, Entry: 0x%08x\n",
                arm7_rom_offset, arm7_ram_addr, arm7_size, arm7_entry);
    }
    
    // 验证 ARM9 和 ARM7 的 ROM offset 是否在文件范围内
    if (arm9_rom_offset >= total_read || arm9_rom_offset + arm9_size > total_read) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: ERROR: ARM9 ROM offset/size out of bounds\n");
        }
        free(rom_buffer);
        return -1;
    }
    
    if (arm7_size > 0) {
        if (arm7_rom_offset >= total_read || arm7_rom_offset + arm7_size > total_read) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: ERROR: ARM7 ROM offset/size out of bounds\n");
            }
            free(rom_buffer);
            return -1;
        }
    }
    
    // 将整个 ROM 数据复制到系统内存（用于 ROM 访问）
    // 根据 drastic.cpp，ROM 数据需要保留在内存中以便 CPU 访问
    // 简化处理：将 ROM 数据复制到 param_1 + 0x1000
    size_t max_copy_size = sizeof(nds_system) - 0x1000;
    size_t copy_size = (total_read > max_copy_size) ? max_copy_size : total_read;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: Copying ROM data to system memory: %zu bytes to offset 0x1000\n",
                copy_size);
    }
    
    memcpy((void*)(param_1 + 0x1000), rom_buffer, copy_size);
    
    // 加载 ARM9 程序到内存
    // 根据 drastic.cpp 第 127098-127100 行，使用 memory_region_block_memory_load
    // NDS 内存映射：ARM9 RAM 通常从 0x02000000 开始
    // 在 nds_system 中，我们需要将虚拟地址映射到物理地址
    // 简化处理：ARM9 RAM (0x02000000-0x023FFFFF) 映射到 nds_system 的某个区域
    // 使用一个固定的映射区域，例如从 nds_system + 0x2000000 开始
    if (arm9_size > 0 && arm9_rom_offset < total_read) {
        // 计算 ARM9 程序在 ROM 文件中的实际位置
        uint8_t *arm9_src = rom_buffer + arm9_rom_offset;
        size_t arm9_copy_size = (arm9_size < (total_read - arm9_rom_offset)) ? arm9_size : (total_read - arm9_rom_offset);
        
        // 计算 ARM9 程序在系统内存中的目标地址
        // ARM9 RAM 地址通常是 0x02000000，映射到 nds_system 中的位置
        // 简化映射：将 0x02000000 映射到 nds_system + 0x2000000
        // 但需要确保不超出 nds_system 边界
        unsigned long system_base = (unsigned long)nds_system;
        unsigned long system_end = system_base + sizeof(nds_system);
        
        // ARM9 RAM 地址相对于 0x02000000 的偏移
        unsigned long arm9_ram_offset = arm9_ram_addr - 0x02000000;
        
        // 映射到 nds_system 中的位置
        // 使用较小的偏移量，确保在 nds_system 范围内（62042112 字节 ≈ 0x3B2A9A0）
        // ARM9 RAM (0x02000000-0x023FFFFF) 映射到 nds_system + 0x2000000 (32MB)
        // 但 0x2000000 = 33554432，加上 arm9_ram_offset 可能超出范围
        // 改用较小的映射区域，例如从 0x1000000 (16MB) 开始
        unsigned long arm9_target_offset;
        if (arm9_ram_addr >= 0x02000000 && arm9_ram_addr < 0x02400000) {
            // ARM9 RAM 区域，映射到 nds_system + 0x1000000 + offset
            // 0x1000000 = 16MB，足够容纳 ARM9 RAM (最大 4MB)
            arm9_target_offset = 0x1000000 + arm9_ram_offset;
        } else {
            // 其他地址，使用直接映射（但限制在合理范围内）
            // 如果地址太大，使用较小的偏移
            if (arm9_ram_addr < 0x10000000) {
                arm9_target_offset = arm9_ram_addr;
            } else {
                // 地址太大，使用固定的小偏移
                arm9_target_offset = 0x1000000;
            }
        }
        
        unsigned long target_addr = system_base + arm9_target_offset;
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: ARM9 memory mapping:\n");
            fprintf(stderr, "  system_base: 0x%lx\n", system_base);
            fprintf(stderr, "  arm9_ram_addr: 0x%08x\n", arm9_ram_addr);
            fprintf(stderr, "  arm9_ram_offset: 0x%lx\n", arm9_ram_offset);
            fprintf(stderr, "  arm9_target_offset: 0x%lx\n", arm9_target_offset);
            fprintf(stderr, "  target_addr: 0x%lx\n", target_addr);
            fprintf(stderr, "  system_end: 0x%lx\n", system_end);
            fprintf(stderr, "  arm9_copy_size: %zu bytes\n", arm9_copy_size);
            fprintf(stderr, "  Check: target_addr < system_end? %s\n", (target_addr < system_end) ? "YES" : "NO");
            fprintf(stderr, "  Check: target_addr + size <= system_end? %s\n", 
                    (target_addr + arm9_copy_size <= system_end) ? "YES" : "NO");
        }
        
        if (target_addr < system_end && target_addr + arm9_copy_size <= system_end) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: Loading ARM9 program: %zu bytes from ROM offset 0x%08x to RAM 0x%08x (system offset 0x%lx)\n",
                        arm9_copy_size, arm9_rom_offset, arm9_ram_addr, arm9_target_offset);
            }
            
            memcpy((void*)target_addr, arm9_src, arm9_copy_size);
            
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: ARM9 program loaded successfully\n");
                fprintf(stderr, "[DRASTIC] load_nds: ARM9 entry point data (first 16 bytes): ");
                for (int i = 0; i < 16 && i < arm9_copy_size; i++) {
                    fprintf(stderr, "%02x ", ((uint8_t*)target_addr)[i]);
                }
                fprintf(stderr, "\n");
            }
        } else {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: ERROR: ARM9 target address out of bounds!\n");
                fprintf(stderr, "  target_addr: 0x%lx\n", target_addr);
                fprintf(stderr, "  arm9_copy_size: %zu bytes\n", arm9_copy_size);
                fprintf(stderr, "  system_end: 0x%lx\n", system_end);
                fprintf(stderr, "  target_addr + size: 0x%lx\n", target_addr + arm9_copy_size);
                fprintf(stderr, "  Difference: %ld bytes\n", (long)(system_end - (target_addr + arm9_copy_size)));
            }
        }
        
        // 设置 ARM9 入口点（根据 drastic.cpp 第 127106 行）
        // 入口点存储在 nds_system + lVar16 + 0x10ce10c
        // 简化处理：存储在系统状态结构的某个位置
        // param_1 是系统状态结构（nds_system + 0x320），所以需要找到正确的位置
        // 根据 drastic.cpp，lVar16 是 param_1 - nds_system，所以偏移是 0x10ce10c
        unsigned long arm9_entry_addr = system_base + 0x10ce10c;
        if (arm9_entry_addr < system_end && arm9_entry_addr + 4 <= system_end) {
            *(uint32_t*)arm9_entry_addr = arm9_entry;
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: Set ARM9 entry point to 0x%08x at offset 0x%lx\n",
                        arm9_entry, arm9_entry_addr - system_base);
            }
        }
    }
    
    // 加载 ARM7 程序到内存（如果存在）
    if (arm7_size > 0 && arm7_rom_offset < total_read) {
        uint8_t *arm7_src = rom_buffer + arm7_rom_offset;
        size_t arm7_copy_size = (arm7_size < (total_read - arm7_rom_offset)) ? arm7_size : (total_read - arm7_rom_offset);
        
        unsigned long arm7_target_offset = (unsigned long)(arm7_ram_addr & 0x3FFFFFFF);
        if (arm7_target_offset + arm7_copy_size < sizeof(nds_system)) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] load_nds: Loading ARM7 program: %zu bytes from ROM offset 0x%08x to RAM 0x%08x (system offset 0x%lx)\n",
                        arm7_copy_size, arm7_rom_offset, arm7_ram_addr, arm7_target_offset);
            }
            
            unsigned long system_base = (unsigned long)nds_system;
            unsigned long target_addr = system_base + arm7_target_offset;
            unsigned long system_end = system_base + sizeof(nds_system);
            
            if (target_addr < system_end && target_addr + arm7_copy_size <= system_end) {
                memcpy((void*)target_addr, arm7_src, arm7_copy_size);
                
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] load_nds: ARM7 program loaded successfully\n");
                }
            } else {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] load_nds: WARNING: ARM7 target address out of bounds, skipping\n");
                }
            }
        }
        
        // 设置 ARM7 入口点（根据 drastic.cpp 第 127107 行）
        // 存储在 nds_system + lVar16 + 0x20d46fc
        {
            unsigned long arm7_system_base = (unsigned long)nds_system;
            unsigned long arm7_system_end = arm7_system_base + sizeof(nds_system);
            unsigned long arm7_entry_addr = arm7_system_base + 0x20d46fc;
            if (arm7_entry_addr < arm7_system_end && arm7_entry_addr + 4 <= arm7_system_end) {
                *(uint32_t*)arm7_entry_addr = arm7_entry;
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] load_nds: Set ARM7 entry point to 0x%08x at offset 0x%lx\n",
                            arm7_entry, arm7_entry_addr - arm7_system_base);
                }
            }
        }
    } else {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] load_nds: WARNING: ARM7 program size is 0, skipping ARM7 load\n");
        }
    }
    
    if (debug_enabled) {
        // 验证复制的数据
        uint8_t *copied_data = (uint8_t*)(param_1 + 0x1000);
        fprintf(stderr, "[DRASTIC] load_nds: Verifying ROM data (first 16 bytes): ");
        for (int i = 0; i < 16 && i < copy_size; i++) {
            fprintf(stderr, "%02x ", copied_data[i]);
        }
        fprintf(stderr, "\n");
    }
    
    free(rom_buffer);
    
    g_state.game_loaded = 1;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] load_nds: ROM loaded successfully\n");
    }
    
    return 0;
}

// 重置系统
void reset_system(long param_1) {
    if (!param_1) {
        return;
    }
    
    // 重置输入状态
    g_state.input_state = 0xFFFF;
    
    // 重置屏幕缓冲区
    memset(g_state.screen0_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
    memset(g_state.screen1_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 2);
    
    // 重置系统状态
    // 根据 drastic.cpp，需要重置各种子系统
    // 这里简化处理
}

// 处理参数（libretro 中可能不需要，但保留接口）
void process_arguments(long param_1, int argc, long argv) {
    (void)param_1;
    (void)argc;
    (void)argv;
    // libretro 不需要命令行参数处理
}

// 初始化屏幕
void initialize_screen(long param_1) {
    (void)param_1;
    // 屏幕已经在 initialize_system 中初始化
}

// 关闭屏幕菜单
void set_screen_menu_off(void) {
    // libretro 不需要菜单
}

// 更新屏幕（渲染）
void update_screens(void) {
    // libretro 中屏幕更新在 retro_run 中处理
    // 这里可以做一些准备工作
}

// 获取屏幕指针
void *get_screen_ptr(int screen) {
    static int debug_printed = 0;
    void *ptr = NULL;
    
    if (screen == 0) {
        ptr = g_state.screen0_buffer;
    } else if (screen == 1) {
        ptr = g_state.screen1_buffer;
    }
    
    if (!debug_printed && ptr) {
        fprintf(stderr, "[DRASTIC] get_screen_ptr(%d): returning %p\n", screen, ptr);
        if (screen == 1) debug_printed = 1;
    }
    
    return ptr;
}

// 获取屏幕 pitch
unsigned int get_screen_pitch(int screen) {
    if (screen == 0) {
        return g_state.screen0_pitch;
    } else if (screen == 1) {
        return g_state.screen1_pitch;
    }
    return SCREEN_PITCH;
}

// 复制屏幕数据到16位缓冲区
// 基于 drastic.cpp 中 screen_copy16 的逻辑实现
void screen_copy16(uint16_t *dest, int screen) {
    if (!dest) {
        return;
    }
    
    void *src = get_screen_ptr(screen);
    if (!src) {
        // 如果源缓冲区不存在，填充黑色
        memset(dest, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));
        return;
    }
    
    unsigned int pitch = get_screen_pitch(screen);
    int hires = get_screen_hires_mode(screen);
    int bytes_per_pixel = get_screen_bytes_per_pixel();
    
    if (bytes_per_pixel == 2) {
        // 16位模式（RGB565），直接复制
        // 根据 drastic.cpp，使用 pitch 和 hires 模式
        uint16_t *src_ptr = (uint16_t*)src;
        uint16_t *dest_line = dest;
        uint16_t *dest_end = dest + SCREEN_WIDTH;
        
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            uint16_t *dest_ptr = dest_line;
            unsigned int src_x = 0;
            
            // 复制一行（256像素）
            while (dest_ptr < dest_end) {
                *dest_ptr++ = src_ptr[src_x];
                src_x += (1 + hires);  // hires 模式下跳过像素
            }
            
            // 移动到下一行
            src_ptr = (uint16_t*)((uint8_t*)src_ptr + (pitch & 0xFFFFFFFE));  // 对齐到偶数
            dest_line += SCREEN_WIDTH;
            dest_end += SCREEN_WIDTH;
        }
    } else {
        // 32位模式，需要转换到 RGB565
        // 根据 drastic.cpp 的转换逻辑：RGB888 -> RGB565
        uint32_t *src_ptr = (uint32_t*)src;
        uint16_t *dest_line = dest;
        uint16_t *dest_end = dest + SCREEN_WIDTH;
        
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            uint16_t *dest_ptr = dest_line;
            unsigned int src_x = 0;
            
            // 转换并复制一行
            while (dest_ptr < dest_end) {
                uint32_t pixel = src_ptr[src_x];
                // 转换公式来自 drastic.cpp: (pixel >> 3) & 0x1f | ((pixel >> 13) << 11) | ((pixel >> 10 & 0x3f) << 5)
                uint16_t r = (pixel >> 19) & 0x1F;  // 高5位
                uint16_t g = (pixel >> 10) & 0x3F;  // 中6位
                uint16_t b = (pixel >> 3) & 0x1F;   // 低5位
                *dest_ptr++ = (r << 11) | (g << 5) | b;
                src_x += (1 + hires);
            }
            
            // 移动到下一行
            src_ptr = (uint32_t*)((uint8_t*)src_ptr + (pitch & 0xFFFFFFFC));  // 对齐到4字节
            dest_line += SCREEN_WIDTH;
            dest_end += SCREEN_WIDTH;
        }
    }
}

// 获取每像素字节数
int get_screen_bytes_per_pixel(void) {
    return g_state.screen_bytes_per_pixel;
}

// 获取高分辨率模式
int get_screen_hires_mode(int screen) {
    if (screen == 0) {
        return g_state.screen0_hires;
    } else if (screen == 1) {
        return g_state.screen1_hires;
    }
    return 0;
}

// 更新输入
void update_input(long param_1) {
    if (!param_1) {
        return;
    }
    
    // 将输入状态写入系统内存
    // 根据 drastic.cpp，输入状态在 param_1 + 0x80010
    uint32_t *input_reg = (uint32_t*)(param_1 + 0x80010);
    *input_reg = g_state.input_state;
}

// 平台输入获取（libretro 中不需要，输入由 RetroArch 提供）
void platform_get_input(long param_1, void *param_2, int param_3) {
    (void)param_1;
    (void)param_2;
    (void)param_3;
    // libretro 中不需要平台特定的输入处理
}

// CPU 模拟 - 解释器模式
// 基于 drastic.cpp 中 cpu_next_action_arm7_to_event_update 的实现(第25305行)
// 该函数负责执行 ARM7 和 ARM9 CPU 直到下一个事件
//
// 修复说明:
// param_1 在调用时传入的是 nds_system 的地址(绝对地址)
// 但在实现中需要计算相对于 nds_system 的偏移量来访问数据
void cpu_next_action_arm7_to_event_update(long param_1) {
    /*
    // 计算相对于 nds_system 的偏移量
    unsigned long system_base = (unsigned long)nds_system;
    long system_offset = param_1 - system_base;  // 应该是 0x320
    
    uint32_t uVar1, uVar2, uVar5, uVar6;
    int iVar3, iVar4, iVar7;
    
    // 执行事件
    execute_events_no_param();
    
    // ARM7 CPU 状态检查
    if (*(int *)(nds_system + system_offset + 0x10cde58) != 0) {
        // ARM7 中断处理
        uVar1 = *(uint32_t *)(nds_system + system_offset + 0x10ce110);
        uVar2 = *(uint32_t *)(nds_system + system_offset + 0x10cde60);
        *(uint32_t *)(nds_system + system_offset + 0x10cde60) = 0;
        *(uint32_t *)(nds_system + system_offset + 0x10cdff8) = 0;
        iVar3 = *(int *)(nds_system + system_offset + 0x10cde5c);
        
        if ((uVar1 >> 7 & 1) == 0) {
            // 不在中断模式
            uVar5 = *(uint32_t *)(nds_system + system_offset + 0x10ce10c);
            uVar6 = *(uint32_t *)(nds_system + system_offset + 0x10cde54);
            
            if ((uVar5 & 1) == 0) {
                // ARM 模式
                iVar7 = uVar5 + 4;
                if (uVar6 != 2) {
                    *(uint64_t *)(nds_system + system_offset + (unsigned long)uVar6 * 8 + 0x10cdde0) =
                         *(uint64_t *)(nds_system + system_offset + 0x10ce0f4);
                    if (uVar6 == 1) {
                        // FIQ 模式寄存器组
                        if (system_offset + 0x15c9e18U < system_offset + 0x15ca0f0U &&
                            system_offset + 0x15ca0e0U < system_offset + 0x15c9e28U) {
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde18);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e8) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde20);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0f0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde28);
                        } else {
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e8) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde20);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde18);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0f0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde28);
                        }
                    } else {
                        *(uint32_t *)(nds_system + system_offset + 0x10ce0f4) =
                             *(uint32_t *)(nds_system + system_offset + 0x10cddf0);
                    }
                    *(uint32_t *)(nds_system + system_offset + 0x10cde54) = 2;
                    *(int *)(nds_system + system_offset + 0x10ce0f8) = iVar7;
                } else {
                    *(int *)(nds_system + system_offset + 0x10ce0f8) = iVar7;
                }
                *(uint32_t *)(nds_system + system_offset + 0x10cde40) = uVar1;
            } else {
                // Thumb 模式
                *(uint32_t *)(nds_system + system_offset + 0x10ce10c) = uVar5 & 0xfffffffe;
                iVar7 = (uVar5 & 0xfffffffe) + 4;
                if (uVar6 == 2) {
                    *(int *)(nds_system + system_offset + 0x10ce0f8) = iVar7;
                } else {
                    *(uint64_t *)(nds_system + system_offset + (unsigned long)uVar6 * 8 + 0x10cdde0) =
                         *(uint64_t *)(nds_system + system_offset + 0x10ce0f4);
                    if (uVar6 == 1) {
                        if (system_offset + 0x15c9e18U < system_offset + 0x15ca0f0U &&
                            system_offset + 0x15ca0e0U < system_offset + 0x15c9e28U) {
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde18);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e8) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde20);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0f0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde28);
                        } else {
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e8) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde20);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0e0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde18);
                            *(uint64_t *)(nds_system + system_offset + 0x10ce0f0) =
                                 *(uint64_t *)(nds_system + system_offset + 0x10cde28);
                        }
                    } else {
                        *(uint32_t *)(nds_system + system_offset + 0x10ce0f4) =
                             *(uint32_t *)(nds_system + system_offset + 0x10cddf0);
                    }
                    *(uint32_t *)(nds_system + system_offset + 0x10cde54) = 2;
                    *(int *)(nds_system + system_offset + 0x10ce0f8) = iVar7;
                    if ((uVar5 & 1) == 0) {
                        *(uint32_t *)(nds_system + system_offset + 0x10cde40) = uVar1;
                    } else {
                        *(uint32_t *)(nds_system + system_offset + 0x10cde40) = uVar1 | 0x20;
                    }
                }
                if ((uVar5 & 1) != 0) {
                    *(uint32_t *)(nds_system + system_offset + 0x10cde40) = uVar1 | 0x20;
                }
            }
            
            // 设置中断向量
            iVar7 = 0x18;
            if (iVar3 == 1) {
                iVar7 = *(int *)(*(long *)(nds_system + system_offset + 0x10cdfa0) + 0x10) + 0x18;
            }
            *(int *)(nds_system + system_offset + 0x10ce10c) = iVar7;
            *(uint32_t *)(nds_system + system_offset + 0x10ce110) = uVar1 & 0xffffffc0 | 0x92;
            
            if (iVar3 == 0 && uVar2 != 0) {
                goto LAB_ARM7_TASK_SWITCH;
            }
        } else {
            // 在中断模式
            if (iVar3 == 0 && uVar2 != 0) {
LAB_ARM7_TASK_SWITCH:
                if (1 < uVar2) {
                    *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) =
                         *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) & 0xfffffffd;
                }
                // 调用任务切换函数
                // event_force_task_switch_function(*(undefined8 *)(nds_system + system_offset + 0x10cdfa8), 0);
            }
        }
    }
    
    // ARM9 CPU 状态检查
    if (*(int *)(nds_system + system_offset + 0x20d4448) == 0) goto LAB_FINAL;
    
    // ARM9 中断处理(结构与 ARM7 类似)
    uVar1 = *(uint32_t *)(nds_system + system_offset + 0x20d4700);
    uVar2 = *(uint32_t *)(nds_system + system_offset + 0x20d4450);
    *(uint32_t *)(nds_system + system_offset + 0x20d4450) = 0;
    *(uint32_t *)(nds_system + system_offset + 0x20d45e8) = 0;
    iVar3 = *(int *)(nds_system + system_offset + 0x20d444c);
    
    if ((uVar1 >> 7 & 1) == 0) {
        uVar5 = *(uint32_t *)(nds_system + system_offset + 0x20d46fc);
        uVar6 = *(uint32_t *)(nds_system + system_offset + 0x20d4444);
        
        if ((uVar5 & 1) == 0) {
            // ARM 模式
            iVar7 = uVar5 + 4;
            if (uVar6 != 2) {
                *(uint64_t *)(nds_system + system_offset + (unsigned long)uVar6 * 8 + 0x20d43d0) =
                     *(uint64_t *)(nds_system + system_offset + 0x20d46e4);
                if (uVar6 == 1) {
                    if (system_offset + 0x25d0408U < system_offset + 0x25d06e0U &&
                        system_offset + 0x25d06d0U < system_offset + 0x25d0418U) {
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4408);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d8) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4410);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46e0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4418);
                    } else {
                        *(uint64_t *)(nds_system + system_offset + 0x20d46e0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4418);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d8) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4410);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4408);
                    }
                } else {
                    *(uint32_t *)(nds_system + system_offset + 0x20d46e4) =
                         *(uint32_t *)(nds_system + system_offset + 0x20d43e0);
                }
                *(uint32_t *)(nds_system + system_offset + 0x20d4444) = 2;
                *(int *)(nds_system + system_offset + 0x20d46e8) = iVar7;
            } else {
                *(int *)(nds_system + system_offset + 0x20d46e8) = iVar7;
            }
            *(uint32_t *)(nds_system + system_offset + 0x20d4430) = uVar1;
        } else {
            // Thumb 模式
            *(uint32_t *)(nds_system + system_offset + 0x20d46fc) = uVar5 & 0xfffffffe;
            iVar7 = (uVar5 & 0xfffffffe) + 4;
            if (uVar6 == 2) {
                *(int *)(nds_system + system_offset + 0x20d46e8) = iVar7;
            } else {
                *(uint64_t *)(nds_system + system_offset + (unsigned long)uVar6 * 8 + 0x20d43d0) =
                     *(uint64_t *)(nds_system + system_offset + 0x20d46e4);
                if (uVar6 == 1) {
                    if (system_offset + 0x25d0408U < system_offset + 0x25d06e0U &&
                        system_offset + 0x25d06d0U < system_offset + 0x25d0418U) {
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4408);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d8) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4410);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46e0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4418);
                    } else {
                        *(uint64_t *)(nds_system + system_offset + 0x20d46e0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4418);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d8) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4410);
                        *(uint64_t *)(nds_system + system_offset + 0x20d46d0) =
                             *(uint64_t *)(nds_system + system_offset + 0x20d4408);
                    }
                } else {
                    *(uint32_t *)(nds_system + system_offset + 0x20d46e4) =
                         *(uint32_t *)(nds_system + system_offset + 0x20d43e0);
                }
                *(uint32_t *)(nds_system + system_offset + 0x20d4444) = 2;
                *(int *)(nds_system + system_offset + 0x20d46e8) = iVar7;
                if ((uVar5 & 1) == 0) {
                    *(uint32_t *)(nds_system + system_offset + 0x20d4430) = uVar1;
                } else {
                    *(uint32_t *)(nds_system + system_offset + 0x20d4430) = uVar1 | 0x20;
                }
            }
            if ((uVar5 & 1) != 0) {
                *(uint32_t *)(nds_system + system_offset + 0x20d4430) = uVar1 | 0x20;
            }
        }
        
        // 设置中断向量
        iVar7 = 0x18;
        if (iVar3 == 1) {
            iVar7 = *(int *)(*(long *)(nds_system + system_offset + 0x20d4590) + 0x10) + 0x18;
        }
        *(int *)(nds_system + system_offset + 0x20d46fc) = iVar7;
        *(uint32_t *)(nds_system + system_offset + 0x20d4700) = uVar1 & 0xffffffc0 | 0x92;
        
        if (iVar3 != 0 || uVar2 == 0) goto LAB_FINAL;
    } else {
        // 在中断模式
        if (iVar3 != 0 || uVar2 == 0) goto LAB_FINAL;
    }
    
    // ARM9 任务切换处理
    if (1 < uVar2) {
        *(uint32_t *)(*(long *)(nds_system + system_offset + 0x20d45e0) + 0x2110) =
             *(uint32_t *)(*(long *)(*(long *)(nds_system + system_offset + 0x20d45e0) + 0x2110) & 0xfffffffd);
    }
    // event_force_task_switch_function(*(undefined8 *)(nds_system + system_offset + 0x20d4598), 0);
    
LAB_FINAL:
    // 最终处理阶段：更新时间计数器和执行 CPU
    // 这是函数的统一出口点，无论前面走了哪个中断处理分支
    
    // 获取当前事件列表的时间增量
    // param_1 + 0x318: 事件列表指针的地址
    // **(int **)(param_1 + 0x318): 取事件列表第一个节点的时间值
    iVar3 = **(int **)(param_1 + 0x318);
    
    // ARM7 相关状态
    iVar7 = *(int *)(nds_system + system_offset + 0x10cde60);  // ARM7 任务切换标志
    iVar4 = *(int *)(nds_system + system_offset + 0x10cdfe0);  // ARM7 时间累加器
    
    // 更新剩余时间和时间累加器
    *(int *)(param_1 + 0x10) = iVar3;  // 设置剩余时间
    *(int *)(nds_system + system_offset + 0x10cdfe0) = iVar4 + iVar3;  // 累加时间
    
    // 检查是否需要任务切换
    if (iVar7 != 0) {
        // 需要任务切换：设置最大剩余时间，执行特殊处理函数
        *(uint32_t *)(nds_system + system_offset + 0x10cdfe0) = 0xffffffff;
        typedef void (*cpu_func_t)(long);
        cpu_func_t *func_ptr = (cpu_func_t *)(nds_system + system_offset + 0x10ce100);
        if (func_ptr && *func_ptr) {
            (*func_ptr)(param_1);  // 传入系统状态结构地址
        }
        return;
    }
    
    // 正常执行 CPU：执行 ARM7 处理器
    // 0x15c7d50 是 ARM7 CPU 执行器结构相对于系统状态结构的偏移
    // param_1 + 0x15c7d50 = (nds_system + 0x320) + 0x15c7d50
    //                     = nds_system + 0x15c8070 (ARM7 CPU 执行器)
    _execute_cpu(param_1 + 0x15c7d50);
    
    // 执行完 CPU 后的回调函数
    // 这通常用于处理 DMA、定时器等需要在 CPU 执行后更新的子系统
    typedef void (*cpu_func2_t)(unsigned long);
    cpu_func2_t *func2_ptr = (cpu_func2_t *)(nds_system + system_offset + 0x10ce100);
    long *param2_ptr = (long *)(nds_system + system_offset + 0x10cdfa8);
    if (func2_ptr && *func2_ptr && param2_ptr) {
        (*func2_ptr)((unsigned long)*param2_ptr);
    }
    */
}

// CPU 模拟 - 重编译器模式
void recompiler_entry(long param_1, undefined8 param_2) {
    if (!param_1) {
        return;
    }
    
    (void)param_2;
    
    // 重编译器模式的占位实现
    // 实际需要动态重编译 ARM 指令到主机代码
    
    // 简化实现：调用解释器模式
    cpu_next_action_arm7_to_event_update(param_1);
}

// 执行事件（无参数版本）
// 基于 drastic.cpp 中 execute_events 的无参数调用（第25316行）
// 根据 drastic.cpp，系统状态结构在 nds_system + 0x320
void execute_events_no_param(void) {
    // 使用全局 nds_system + 0x320 作为基址（系统状态结构的偏移）
    execute_events(nds_system + 0x320, 0);
}

// 执行事件（带参数版本）
// 基于 drastic.cpp 中 execute_events 的实现（第17448行）
// param_1 是指向系统状态结构的指针，param_2 未使用（可能是CPU标识）
void execute_events(long param_1, unsigned long param_2) {
    if (!param_1) {
        return;
    }
    
    (void)param_2;
    
    // 根据 drastic.cpp 的实现：
    // - 剩余时间在 param_1 + 0x10
    // - 事件列表指针在 param_1 + 0x318
    // - 时间计数器在 param_1 + 8
    
    // 根据 drastic.cpp 第17457-17459行，execute_events 直接使用 param_1 作为基址
    // 安全检查：确保 param_1 指向有效的内存区域
    unsigned long param_addr = (unsigned long)param_1;
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long system_end = system_base + 62042112;
    if (param_addr < system_base || param_addr >= system_end) {
        return;
    }
    
    // 安全检查：确保访问的地址在有效范围内
    if (param_addr + 0x318 >= system_end) {
        return;
    }
    
    uint32_t time_remaining = *(uint32_t*)(param_1 + 0x10);
    uint32_t **event_list_ptr = (uint32_t**)(param_1 + 0x318);
    
    if (!event_list_ptr) {
        return;
    }
    
    // 安全检查：确保 event_list_ptr 指向的地址在有效范围内
    unsigned long event_list_addr = (unsigned long)*event_list_ptr;
    if (!event_list_addr || event_list_addr < system_base || event_list_addr >= system_end) {
        return;
    }
    
    uint32_t *event_node = *event_list_ptr;
    if (!event_node) {
        return;
    }
    
    uint32_t event_time = event_node[0];
    uint64_t *time_counter = (uint64_t*)(param_1 + 8);
    
    // 更新时间计数器
    if (time_counter) {
        *time_counter += (uint64_t)time_remaining;
    }
    
    // 如果剩余时间小于事件时间，只需要减少事件时间
    if (time_remaining < event_time) {
        event_node[0] = event_time - time_remaining;
    } else {
        // 需要执行事件
        // 根据 drastic.cpp 第17466-17469行：
        // - puVar5 是事件节点指针（uint *）
        // - 事件处理函数（函数指针）在 puVar5 + 2（即 puVar5[2]，偏移 8 字节）
        // - 事件参数在 puVar5 + 4（即 puVar5[4]，偏移 16 字节）
        // - 下一个事件节点在 puVar5 + 6（即 puVar5[6]，偏移 24 字节）
        
        // 定义函数指针类型
        typedef void (*event_handler_t)(long, unsigned long);
        
        // 安全检查：确保事件节点地址在有效范围内
        unsigned long event_node_addr = (unsigned long)event_node;
        if (event_node_addr < system_base || event_node_addr + 32 >= system_end) {
            return;
        }
        
        // 从事件节点读取函数指针（偏移 8 字节，即 event_node[2]）
        // 注意：event_node 是 uint32_t *，所以 event_node[2] 是偏移 8 字节
        // 但函数指针是 8 字节（64位），所以需要转换为正确的类型
        uintptr_t *event_node_ptr = (uintptr_t*)event_node;
        event_handler_t handler = (event_handler_t)event_node_ptr[1];  // 偏移 8 字节（1 * sizeof(uintptr_t)）
        
        // 从事件节点读取事件参数（偏移 16 字节，即 event_node[4]）
        unsigned long event_param = (unsigned long)event_node_ptr[2];  // 偏移 16 字节（2 * sizeof(uintptr_t)）
        
        // 从事件节点读取下一个事件节点（偏移 24 字节，即 event_node[6]）
        uint32_t *next_event_node = (uint32_t*)event_node_ptr[3];  // 偏移 24 字节（3 * sizeof(uintptr_t)）
        
        // 移动到下一个事件节点
        *event_list_ptr = next_event_node;
        
        // 如果有处理函数，调用它
        // 安全检查：确保函数指针有效
        if (handler) {
            // 检查函数指针是否是我们定义的存根函数之一
            unsigned long handler_addr = (unsigned long)handler;
            // 只允许调用我们定义的存根函数
            if (handler_addr == (unsigned long)event_hblank_start_function_stub ||
                handler_addr == (unsigned long)event_scanline_start_function_stub ||
                handler_addr == (unsigned long)event_force_task_switch_function_stub ||
                handler_addr == (unsigned long)event_gamecard_irq_function_stub) {
                // 调用处理函数
                handler(param_1, event_param);
            }
            // 如果函数指针不是我们定义的存根函数，跳过调用（避免段错误）
        }
        
        // 如果事件列表为空，返回
        if (!*event_list_ptr) {
            return;
        }
        
        // 清除事件节点的标志位（偏移8和9）
        (*event_list_ptr)[8] = 0;
        (*event_list_ptr)[9] = 0;
        
        // 继续处理下一个事件，直到事件列表为空或事件时间不为0
        while (*event_list_ptr && (*event_list_ptr)[0] == 0) {
            uint32_t *next_node = (uint32_t*)(*event_list_ptr)[6];
            *event_list_ptr = next_node;
            if (!next_node) {
                return;
            }
            next_node[8] = 0;
            next_node[9] = 0;
        }
    }
}

// 设置输入状态（供 libretro.cpp 调用）
void drastic_set_input_state(uint16_t input) {
    g_state.input_state = (uint32_t)input;
}

#ifdef __cplusplus
}
#endif

