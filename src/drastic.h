/*
 * drastic_wrapper.h
 * 包装 drastic.cpp 中的必要函数和数据结构
 * 由于 drastic.cpp 不可编译，此文件提供必要的声明
 */

 #ifndef DRASTIC_WRAPPER_H
 #define DRASTIC_WRAPPER_H
 
 #include <stdint.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // 类型定义
 typedef unsigned char undefined1;
 typedef unsigned int undefined4;
 typedef unsigned long undefined8;
 
// 全局系统内存
#define NDS_SYSTEM_SIZE 62042112UL  // UL表示无符号长整型

 // 全局系统内存（62042112 字节）
 extern undefined1 nds_system[NDS_SYSTEM_SIZE];
 
// 调试输出标志
extern int debug_enabled;  // 可以通过环境变量控制

// 系统初始化和管理函数
extern int initialize_system(long param_1);
extern int load_nds(long param_1, const char *param_2);
extern void reset_system(long param_1);
extern void process_arguments(long param_1, int argc, long argv);
extern void initialize_event_list(long param_1, long param_2);
// Event / DMA helpers (from drastic.cpp)
extern void initialize_event(long param_1, unsigned int param_2, void *param_3, long param_4);
extern void initialize_dma(long param_1, long param_2, long param_3, long param_4, long param_5);
// DMA event handler (defined in drastic.cpp)
extern void event_dma_complete_function(long param_1, long param_2);
// 初始化子系统
extern void initialize_memory(long param_1);
extern void initialize_cpu(long param_1);
extern void initialize_gamecard(long param_1);
extern int load_system_file(const char *system_dir, const char *filename, void *dest, int expected_size);
extern int load_bios_files(long param_1, const char *system_dir);
 
 // 屏幕相关函数
 extern void initialize_screen(long param_1);
 extern void set_screen_menu_off(void);
 extern void update_screens(void);
 extern void *get_screen_ptr(int screen);
 extern unsigned int get_screen_pitch(int screen);
 extern void screen_copy16(uint16_t *dest, int screen);
 extern int get_screen_bytes_per_pixel(void);
 extern int get_screen_hires_mode(int screen);
 
 // 视频/帧处理（来自 drastic.cpp）
 extern unsigned long video_render_scanlines(long *param_1, int param_2);
 extern void start_frame(long *param_1);
 extern void update_frame(long *param_1);
 
 // 输入处理函数
 extern void update_input(long param_1);
 extern void platform_get_input(long param_1, void *param_2, int param_3);
 
 // CPU 模拟函数
 extern void cpu_next_action_arm7_to_event_update(long param_1);
 extern void recompiler_entry(long param_1, undefined8 param_2);
 extern void execute_events(long param_1, unsigned long param_2);
 extern void _execute_cpu(long param_1);
 
// 音频处理函数（在 spu.h 中声明）
#include "spu.h"
 
 // 辅助函数（供 libretro.cpp 使用）
 extern void drastic_set_input_state(uint16_t input);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* DRASTIC_WRAPPER_H */
 
 