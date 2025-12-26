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

extern int debug_enabled; // 是否启用调试模式
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
    int test_pattern_written;
} drastic_state_t;

static drastic_state_t g_state = {0};

// Backup of event_list pointer for debugging/repair purposes
static uintptr_t g_event_list_ptr_backup = 0;

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
// Event handlers (implemented based on drastic.cpp behavior)
void event_hblank_start_function(long param_1, unsigned long param_2) {
    (void)param_2;
    // Call video_render_scanlines for current scanline if available
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] event_hblank_start: param=0x%lx scanline=%u\n", (unsigned long)param_1, *(unsigned short*)(param_1 + 0x14));
    }
    // Attempt to render the just-completed scanline (best-effort)
    video_render_scanlines((long*)(param_1 + 0x36d1ec0), *(int*)(param_1 + 0x14));
}

void event_scanline_start_function(long param_1, unsigned long param_2) {
    (void)param_2;
    // Advance scanline counter (ushort at offset 0x14)
    uint16_t *scanline_ptr = (uint16_t*)(param_1 + 0x14);
    uint16_t scanline = *scanline_ptr;
    scanline++;

    if (scanline == 0x107) {
        // End of frame: start frame processing and reset scanline
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] event_scanline_start: end of frame reached, calling start_frame/update_frame\n");
        }
        // start_frame expects a pointer to a per-frame state; original calls at +0x6da3d8
        start_frame((long*)(param_1 + 0x6da3d8));
        scanline = 0;
    } else if (scanline == 0xc0) {
        // Mid-frame: commit frame and update input
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] event_scanline_start: scanline reached 0xC0, calling update_frame/update_input\n");
        }
        update_frame((long*)(param_1 + 0x6da3d8));
        //update_input(param_1 + 0xAAA);
    }

    *scanline_ptr = scanline;
}

void event_force_task_switch_function(long param_1, unsigned long param_2) {
    (void)param_2;
    // Minimal implementation: set a flag that a task switch should occur.
    // Original code pushes a special node into the event list — here we simply log.
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] event_force_task_switch: param=0x%lx (noop minimal implementation)\n", (unsigned long)param_1);
    }
}

void event_gamecard_irq_function(long param_1, unsigned long param_2) {
    (void)param_2;
    // Gamecard IRQ minimal handling: log and set a flag in system if appropriate
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] event_gamecard_irq: param=0x%lx (noop minimal implementation)\n", (unsigned long)param_1);
    }
}

// DMA完成事件处理函数
// 基于 drastic.cpp 中 event_dma_complete_function 的实现（第16304行）
// 当DMA传输完成时，此函数被调用以处理完成后的操作
//
// 参数说明:
// param_1: 系统状态结构指针（未使用，但保持函数签名一致）
// param_2: DMA通道结构指针（指向DMA通道的控制结构）
void event_dma_complete_function(long param_1, long param_2) {
    (void)param_1;  // param_1 在原始实现中未使用
    
    if (!param_2) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] event_dma_complete: NULL param_2\n");
        }
        return;
    }
    
    // 读取DMA控制寄存器（偏移0x20）
    // 控制寄存器位定义：
    // - bit 25 (0x2000000): 重复模式标志
    // - bit 30 (0x40000000): IRQ使能标志
    // - bit 31 (0x80000000): DMA启用标志
    uint32_t uVar1 = *(uint32_t *)(param_2 + 0x20);
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] event_dma_complete: param_2=0x%lx control=0x%08x\n",
                (unsigned long)param_2, uVar1);
    }
    
    // 检查bit 25（重复模式）：如果未设置，则禁用DMA
    if ((uVar1 >> 0x19 & 1) == 0) {
        // 清除bit 31（DMA启用位），禁用DMA
        uVar1 = uVar1 & 0x7fffffff;
        *(uint32_t *)(param_2 + 0x20) = uVar1;
        
        // 通过指针写入DMA控制寄存器（param_2 + 0x10 是指向DMA控制寄存器的指针）
        // 偏移8是控制寄存器的偏移（字节偏移）
        long *dma_ctrl_ptr = *(long **)(param_2 + 0x10);
        if (dma_ctrl_ptr) {
            *(uint32_t *)((char *)dma_ctrl_ptr + 8) = uVar1;
        }
        
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] event_dma_complete: disabled DMA (non-repeat mode)\n");
        }
    }
    
    // 检查bit 30（IRQ使能）：如果设置了，则触发中断
    if ((uVar1 >> 0x1e & 1) != 0) {
        // 获取中断控制器指针
        // param_2 + 0x8 是指向CPU结构的指针
        // CPU结构 + 0x2080 是中断控制器指针
        long *cpu_ptr = *(long **)(param_2 + 8);
        if (!cpu_ptr) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] event_dma_complete: NULL cpu_ptr\n");
            }
        } else {
            // 使用字节偏移访问（drastic.cpp使用字节偏移，不是指针偏移）
            // cpu_ptr + 0x2080 在drastic.cpp中是字节偏移，需要转换为char*再偏移
            long lVar2 = *(long *)((char *)cpu_ptr + 0x2080);
            if (!lVar2) {
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] event_dma_complete: NULL interrupt controller\n");
                }
            } else {
                // 计算中断标志位
                // param_2 + 0x25 是DMA通道编号（低5位）
                uint8_t channel_num = *(uint8_t *)(param_2 + 0x25) & 0x1f;
                uint32_t irq_bit = 0x100 << channel_num;  // 每个DMA通道有对应的中断位
                
                // 读取当前中断标志寄存器（偏移0x214，使用字节偏移）
                uint32_t irq_flags = *(uint32_t *)((char *)lVar2 + 0x214);
                uVar1 = irq_bit | irq_flags;
                
                // 设置中断标志
                *(uint32_t *)((char *)lVar2 + 0x214) = uVar1;
                
                if (debug_enabled) {
                    fprintf(stderr, "[DRASTIC] event_dma_complete: set IRQ channel=%d bit=0x%08x flags=0x%08x\n",
                            (int)channel_num, irq_bit, uVar1);
                }
                
                // 检查中断掩码（CPU结构 + 0x2110，使用字节偏移）
                // bit 1和2（值6）如果都未设置，则处理中断请求
                uint32_t interrupt_mask = *(uint32_t *)((char *)cpu_ptr + 0x2110);
                if ((interrupt_mask & 6) == 0) {
                    // 计算中断请求寄存器（偏移0x2108，使用字节偏移）
                    // 中断请求 = (~中断使能) & 中断标志 & 中断掩码
                    int32_t irq_enable = *(int32_t *)((char *)lVar2 + 0x208);  // 中断使能寄存器（偏移0x208）
                    uint32_t irq_mask = *(uint32_t *)((char *)lVar2 + 0x210);  // 中断掩码寄存器（偏移0x210）
                    
                    uint32_t irq_request = ((uint32_t)(~irq_enable)) & irq_mask & uVar1;
                    *(uint32_t *)((char *)cpu_ptr + 0x2108) = irq_request;
                    
                    if (debug_enabled) {
                        fprintf(stderr, "[DRASTIC] event_dma_complete: IRQ request=0x%08x (enable=0x%08x mask=0x%08x)\n",
                                irq_request, (uint32_t)irq_enable, irq_mask);
                    }
                } else {
                    if (debug_enabled) {
                        fprintf(stderr, "[DRASTIC] event_dma_complete: interrupts masked (mask=0x%08x)\n",
                                interrupt_mask);
                    }
                }
            }
        }
    }
    
    // 清除DMA完成标志（偏移0x26）
    *(uint8_t *)(param_2 + 0x26) = 0;
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] event_dma_complete: cleared completion flag\n");
    }
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
    *(event_func_t*)(param_1 + 0x8) = (event_func_t)event_hblank_start_function;
    *(long*)(param_1 + 0x10) = param_2;
    *(uint8_t*)(param_1 + 0x28) = 0;
    
    // 第二个事件：扫描线开始（偏移 0x20）
    *(event_func_t*)(param_1 + 0x38) = (event_func_t)event_scanline_start_function;
    *(long*)(param_1 + 0x40) = param_2;
    *(uint8_t*)(param_1 + 0x58) = 1;
    
    // 第三个事件：强制任务切换（偏移 0x50）
    *(event_func_t*)(param_1 + 0x68) = (event_func_t)event_force_task_switch_function;
    *(long*)(param_1 + 0x70) = 0;
    *(uint8_t*)(param_1 + 0x88) = 2;
    
    // 第四个事件：游戏卡 IRQ（偏移 0x200）
    *(event_func_t*)(param_1 + 0x218) = (event_func_t)event_gamecard_irq_function;
    *(long*)(param_1 + 0x220) = param_2 + 0x320;  // param_2 + 800 (0x320)
    *(uint8_t*)(param_1 + 0x238) = 0xb;
    
    // 事件列表指针（偏移 0x300）初始化为 NULL
    *(long*)(param_1 + 0x300) = 0;
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_event_list: wrote NULL at event_list addr %p\n", (void*)(param_1 + 0x300));
    }
}

// 初始化单个事件（根据 drastic.cpp 的 initialize_event）
// param_1: 事件列表基址
// param_2: 事件编号（用来计算偏移：entry = base + id*0x30）
// param_3: 事件处理函数指针
// param_4: 事件参数（通常是系统状态结构或事件相关指针）
void initialize_event(long param_1, unsigned int param_2, void *param_3, long param_4) {
    if (!param_1) return;
    long entry = param_1 + (long)param_2 * 0x30L;
    // 写入函数指针（8 字节）
    *(void **)(entry + 8) = param_3;
    // 写入参数（8 字节）
    *(long *)(entry + 0x10) = param_4;
    // 写入事件类型/编号（1 字节）
    *(uint8_t *)(entry + 0x28) = (uint8_t)param_2;

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_event: base=0x%lx id=0x%x entry=0x%lx handler=%p param=0x%lx\n",
                (unsigned long)param_1, param_2, (unsigned long)entry, param_3, (unsigned long)param_4);
    }
}

// 初始化 DMA 通道（简化实现，基于 drastic.cpp initialize_dma）
// param_1: DMA channel struct base (address in nds_system)
// param_2: value to store at [0]
// param_3: value to store at [1]
// param_4: base offset used to compute several offsets (caller-specific)
// param_5: system state base (used to register events into the system event list)
void initialize_dma(long param_1, long param_2, long param_3, long param_4, long param_5) {
    if (!param_1) return;

    unsigned char *p = (unsigned char *)param_1;

    // clear control byte at +0x35
    *(uint8_t *)(param_1 + 0x35) = 0;

    // store references according to drastic.cpp layout
    *(long *)(param_1 + 3 * sizeof(long)) = param_5;               // param_1[3] = param_5
    *(long *)(param_1 + 4 * sizeof(long)) = param_4 + 0xb0;         // param_1[4] = param_4 + 0xb0

    int iVar1 = *(int *)(param_5 + 0x210c);
    if (iVar1 == 1) {
        // register event for DMA channel 0 (id 0xC)
        initialize_event(*(long *)(param_5 + 0x2258) + 0x18, 0xc, (void *)event_dma_complete_function, param_1 + 2);
        *(uint8_t *)(param_1 + 0x5d) = 1;
        *(long *)(param_1 + 8 * sizeof(long)) = param_5;              // param_1[8] = param_5
        *(long *)(param_1 + 9 * sizeof(long)) = param_4 + 0xbc;       // param_1[9] = param_4 + 0xbc
    } else {
        *(uint8_t *)(param_1 + 0x5d) = 1;
        *(long *)(param_1 + 8 * sizeof(long)) = param_5;
        *(long *)(param_1 + 9 * sizeof(long)) = param_4 + 0xbc;
    }

    iVar1 = *(int *)(param_5 + 0x210c);
    if (iVar1 == 1) {
        initialize_event(*(long *)(param_5 + 0x2258) + 0x18, 0xd, (void *)event_dma_complete_function, param_1 + 7);
        *(uint8_t *)(param_1 + 0x85) = 2;
        *(long *)(param_1 + 0xd * sizeof(long)) = param_5;            // param_1[0xd] = param_5
        *(long *)(param_1 + 0xe * sizeof(long)) = param_4 + 200;      // param_1[0xe] = param_4 + 200
    } else {
        *(uint8_t *)(param_1 + 0x85) = 2;
        *(long *)(param_1 + 0xd * sizeof(long)) = param_5;
        *(long *)(param_1 + 0xe * sizeof(long)) = param_4 + 200;
    }

    iVar1 = *(int *)(param_5 + 0x210c);
    if (iVar1 == 1) {
        initialize_event(*(long *)(param_5 + 0x2258) + 0x18, 0xe, (void *)event_dma_complete_function, param_1 + 0xc);
        *(uint8_t *)(param_1 + 0xad) = 3;
        *(long *)(param_1 + 0x12 * sizeof(long)) = param_5;           // param_1[0x12] = param_5
        *(long *)(param_1 + 0x13 * sizeof(long)) = param_4 + 0xd4;    // param_1[0x13] = param_4 + 0xd4
    } else {
        *(uint8_t *)(param_1 + 0xad) = 3;
        *(long *)(param_1 + 0x12 * sizeof(long)) = param_5;
        *(long *)(param_1 + 0x13 * sizeof(long)) = param_4 + 0xd4;
    }

    iVar1 = *(int *)(param_5 + 0x210c);
    if (iVar1 == 1) {
        initialize_event(*(long *)(param_5 + 0x2258) + 0x18, 0xf, (void *)event_dma_complete_function, param_1 + 0x11);
        *(long *)(param_1 + 0) = param_2;
        *(long *)(param_1 + 1 * sizeof(long)) = param_3;
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] initialize_dma: registered events 0xc..0xf for channel @0x%lx\n", (unsigned long)param_1);
        }
        return;
    }

    *(long *)(param_1 + 0) = param_2;
    *(long *)(param_1 + 1 * sizeof(long)) = param_3;
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_dma: channel @0x%lx initialized without event registration\n", (unsigned long)param_1);
    }
}

// 初始化内存：建立简单的页面表并映射几个常用区域
// 这是对 drastic.cpp 更复杂实现的简化版本，能让解释器正常工作
void initialize_memory(long param_1) {
    if (!param_1) return;

    // 在全局 nds_system 中分配两个页面表（ARM7/ARM9），每个表包含 131072 项（address>>11 索引），每项 8 字节
    // 大小约 1MB/表，放在 nds_system 的安全区域
    const unsigned long PAGE_ENTRIES = 131072UL; // covers up to 0x04000000 with 2KB pages
    const unsigned long PAGE_TABLE_BYTES = PAGE_ENTRIES * 8UL;
    // 选择两个偏移作为页面表位置（在系统可用范围内）
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long table1_offset = 0x300000; // 3MB
    unsigned long table2_offset = 0x400000; // 4MB

    if (system_base + table2_offset + PAGE_TABLE_BYTES >= system_base + sizeof(nds_system)) {
        // 如果超出范围就不用初始化（防御性处理）
        return;
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_memory: table1_offset=0x%lx table2_offset=0x%lx entries=%lu bytes=%lu\n",
                table1_offset, table2_offset, PAGE_ENTRIES, PAGE_TABLE_BYTES);
    }

    unsigned long *table1 = (unsigned long*)(system_base + table1_offset);
    unsigned long *table2 = (unsigned long*)(system_base + table2_offset);
    memset(table1, 0, PAGE_TABLE_BYTES);
    memset(table2, 0, PAGE_TABLE_BYTES);

    // 简单映射常用区域：ARM9 RAM (0x02000000 - 0x02400000) -> nds_system + 0x1000000
    unsigned long vstart = 0x02000000UL;
    unsigned long vend = 0x02400000UL;
    unsigned long target_base = system_base + 0x1000000UL;
    unsigned long start_idx = vstart >> 11;
    unsigned long end_idx = (vend - 1) >> 11;
    for (unsigned long i = start_idx; i <= end_idx; i++) {
        unsigned long page_offset = (i - start_idx) * 2048UL;
        unsigned long physical = target_base + page_offset;
        table1[i] = physical / 4UL; // interpreter expects page_value*4 + (addr & 0x7FF)
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_memory: mapped 0x%08lx-0x%08lx -> 0x%08lx (entries %lu..%lu)\n",
                0x02000000UL, 0x02400000UL, target_base, start_idx, end_idx);
    }

    // 简单映射常用区域：ARM7 RAM (0x02380000 - 0x023BFFFF) -> nds_system + 0x2000000
    vstart = 0x02380000UL;
    vend = 0x023C0000UL;
    target_base = system_base + 0x2000000UL;
    start_idx = vstart >> 11;
    end_idx = (vend - 1) >> 11;
    for (unsigned long i = start_idx; i <= end_idx; i++) {
        unsigned long page_offset = (i - start_idx) * 2048UL;
        unsigned long physical = target_base + page_offset;
        table1[i] = physical / 4UL;
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_memory: mapped ARM7 region 0x%08lx-0x%08lx -> 0x%08lx\n",
                0x02380000UL, 0x023C0000UL, system_base + 0x2000000UL);
    }

    // 将页面表地址写入系统结构，供解释器使用
    // 解释器期望 memory controller 指针在 CPU 结构的偏移 0x23d0
    // 我们把 ARM7/ARM9 的内存控制器指向 table1/table2
    unsigned long arm7_cpu_abs = system_base + 0x15c8070UL; // as used by _execute_cpu callers
    unsigned long arm9_cpu_abs = system_base + 0x25d0408UL; // heuristic location for ARM9

    // 写入 pointer 值（绝对地址作为 long 存储）
    if (arm7_cpu_abs + 0x23d0 + sizeof(void*) <= system_base + sizeof(nds_system)) {
        *(unsigned long*)(arm7_cpu_abs + 0x23d0) = system_base + table1_offset;
    }
    if (arm9_cpu_abs + 0x23d0 + sizeof(void*) <= system_base + sizeof(nds_system)) {
        *(unsigned long*)(arm9_cpu_abs + 0x23d0) = system_base + table2_offset;
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_memory: arm7_cpu=%p arm9_cpu=%p table1=%p table2=%p\n",
                (void*)arm7_cpu_abs, (void*)arm9_cpu_abs, (void*)(system_base + table1_offset), (void*)(system_base + table2_offset));
    }
}

// 初始化游戏卡（gamecard）子系统：将游戏卡区域清零并设置标志
void initialize_gamecard(long param_1) {
    if (!param_1) return;
    // 将常见的 gamecard 缓冲区清零（在很多实现中位于系统内存的低地址）
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long gamecard_offset = 0x1000UL; // ROM 被拷贝到 param+0x1000，确保此处清理
    memset((void*)(system_base + gamecard_offset), 0, 0x100000); // 清零 1MB 的 ROM 区域缓存

    // 设置 gamecard 状态标志（如存在位置），这里使用系统状态结构附近的一个字节
    unsigned long state_flag_addr = system_base + 0x320 + 0x50; // 任意选址的标志字节
    if (state_flag_addr < system_base + sizeof(nds_system)) {
        *(uint8_t*)state_flag_addr = 0; // 0 表示未插入 ROM
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_gamecard: cleared ROM area at 0x%lx, state_flag=%p\n",
                (unsigned long)gamecard_offset, (void*)state_flag_addr);
    }
}

// 初始化 CPU 状态（简化实现）：清零 CPU 结构并设置默认寄存器/时间片
void initialize_cpu(long param_1) {
    if (!param_1) return;

    unsigned long system_base = (unsigned long)nds_system;

    // ARM7 CPU 结构基址（与 _execute_cpu 的调用一致）
    unsigned long arm7_cpu_abs = system_base + 0x15c8070UL;
    // 初始化一个合理大小的 CPU 状态区域
    size_t cpu_struct_size = 0x20000; // 128KB
    if (arm7_cpu_abs + cpu_struct_size <= system_base + sizeof(nds_system)) {
        memset((void*)arm7_cpu_abs, 0, cpu_struct_size);
        // 设置一些默认寄存器/字段（偏移按 interpreter 期望）
        // time limit (offset 0x2290)
        *(uint32_t*)(arm7_cpu_abs + 0x2290) = 0x1000; // 默认执行周期限制
        // PC (offset 0x23bc)
        *(uint32_t*)(arm7_cpu_abs + 0x23bc) = 0;
        // CPSR (offset 0x23c0)
        *(uint32_t*)(arm7_cpu_abs + 0x23c0) = 0x000000d3; // SVC mode, interrupts enabled (heuristic)
        // 指向页面表（如果没有设置过）
        if (*(unsigned long*)(arm7_cpu_abs + 0x23d0) == 0) {
            *(unsigned long*)(arm7_cpu_abs + 0x23d0) = system_base + 0x300000; // our table1
        }
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_cpu: ARM7 cpu struct @ %p, time_limit=0x%08x, PC_ptr=%p\n",
                (void*)arm7_cpu_abs, *(uint32_t*)(arm7_cpu_abs + 0x2290), (void*)(arm7_cpu_abs + 0x23bc));
    }

    // ARM9 CPU 结构（简化）：只清零并设置少数字段
    unsigned long arm9_cpu_abs = system_base + 0x25d0408UL;
    if (arm9_cpu_abs + 0x10000 <= system_base + sizeof(nds_system)) {
        memset((void*)arm9_cpu_abs, 0, 0x10000);
        *(uint32_t*)(arm9_cpu_abs + 0x2290) = 0x1000;
        *(uint32_t*)(arm9_cpu_abs + 0x23bc) = 0;
        *(uint32_t*)(arm9_cpu_abs + 0x23c0) = 0x000000d3;
        if (*(unsigned long*)(arm9_cpu_abs + 0x23d0) == 0) {
            *(unsigned long*)(arm9_cpu_abs + 0x23d0) = system_base + 0x400000; // our table2
        }
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_cpu: ARM9 cpu struct @ %p, time_limit=0x%08x\n",
                (void*)arm9_cpu_abs, *(uint32_t*)(arm9_cpu_abs + 0x2290));
    }
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
    
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_system: nds_system=%p, param_1=%p\n", (void*)nds_system, (void*)param_1);
    }

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

    // Write a visible test pattern to both screens so we can verify the
    // rendering path and confirm video_cb receives non-black content.
    if (!g_state.test_pattern_written) {
        uint16_t *s0 = (uint16_t*)g_state.screen0_buffer;
        uint16_t *s1 = (uint16_t*)g_state.screen1_buffer;
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                // Checkerboard 8x8 blocks: two contrasting 5:6:5 colors
                int bx = (x / 8) & 1;
                int by = (y / 8) & 1;
                uint16_t color = (bx ^ by) ? 0x07E0 : 0x001F; // green / blue
                s0[y * SCREEN_WIDTH + x] = color;
                // Make screen1 the inverse-ish pattern so they differ
                s1[y * SCREEN_WIDTH + x] = (uint16_t)((~color) & 0x7FFF);
            }
        }
        g_state.test_pattern_written = 1;
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] initialize_system: wrote test pattern to screens (checkerboard)\n");
        }
    }
    
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

    // Immediately set the event-list pointer here to ensure it is present before
    // memory/cpu/gamecard initialization (those routines previously could
    // overwrite the same area in some configurations).
    {
        uintptr_t *event_list_ptr_early = (uintptr_t*)(system_state_base + 0x318);
        *event_list_ptr_early = (uintptr_t)(param_1 + 0x18);
        // Initialize first event time and timing fields early
        uint32_t *first_event_time_early = (uint32_t*)(param_1 + 0x18);
        *first_event_time_early = 0;
        uint64_t *time_counter_early = (uint64_t*)(system_state_base + 8);
        if (time_counter_early) *time_counter_early = 0;
        uint32_t *time_remaining_early = (uint32_t*)(system_state_base + 0x10);
        if (time_remaining_early) *time_remaining_early = 0;
    }

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_system: system_state_base=0x%lx, event_list_node=%p, screen0=%p, screen1=%p\n",
                (unsigned long)system_state_base, (void*)(param_1 + 0x18), g_state.screen0_buffer, g_state.screen1_buffer);
    }

    // 初始化内存映射、CPU和游戏卡结构
    // 这些函数在 drastic.cpp 中有更复杂的实现，这里提供一个合理的简化实现
    initialize_memory(param_1);
    initialize_gamecard(param_1);
    initialize_cpu(param_1);

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] initialize_system: memory/cpu/gamecard initialized\n");
    }
    
    // 设置事件列表指针（根据 drastic.cpp 第17458行，事件列表指针在系统状态结构 + 0x318）
    // 系统状态结构在 nds_system + 0x320，所以事件列表指针在 nds_system + 0x320 + 0x318 = nds_system + 0x638
    // 事件列表的第一个节点在 nds_system + 0x18（根据 initialize_event_list 的实现）
    // 但是，事件列表指针应该指向第一个事件节点，而第一个事件节点的时间字段在偏移 0
    // 所以我们需要将事件列表的第一个节点地址存储在系统状态结构的 0x318 偏移处
    // 事件列表指针（使用指针大小写入以兼容 32/64 位）
    uintptr_t *event_list_ptr = (uintptr_t*)(system_state_base + 0x318);
    *event_list_ptr = (uintptr_t)(param_1 + 0x18);
    
    // 初始化第一个事件节点的时间字段为 0（表示立即执行）
    // 根据 drastic.cpp，事件节点的时间字段在偏移 0（此实现中时间位于 param_1 + 0x18）
    uint32_t *first_event_time = (uint32_t*)(param_1 + 0x18);
    *first_event_time = 0;
    
    // 初始化时间计数器（系统状态结构 + 8）
    uint64_t *time_counter = (uint64_t*)(system_state_base + 8);
    *time_counter = 0;
    
    // 初始化剩余时间（系统状态结构 + 0x10）
    uint32_t *time_remaining = (uint32_t*)(system_state_base + 0x10);
    *time_remaining = 0;
    
    // Verify the event-list pointer wasn't corrupted by earlier initializers; restore if necessary
    {
        uintptr_t stored = *(uintptr_t*)(system_state_base + 0x318);
        unsigned long sys_base = (unsigned long)nds_system;
        unsigned long sys_end = sys_base + NDS_SYSTEM_SIZE;
        if (!(stored >= sys_base && stored < sys_end)) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] initialize_system: event_list_ptr corrupted (0x%lx), restoring to 0x%lx\n",
                        (unsigned long)stored, (unsigned long)(param_1 + 0x18));
            }
            *(uintptr_t*)(system_state_base + 0x318) = (uintptr_t)(param_1 + 0x18);
        }
    }

    // Store a backup of the correctly initialized event list pointer so we can detect
    // and optionally repair corruption during early debugging.
    {
        g_event_list_ptr_backup = (uintptr_t)(param_1 + 0x18);
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] initialize_system: g_event_list_ptr_backup set to 0x%016lx\n", (unsigned long)g_event_list_ptr_backup);
        }
    }

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
    
    // Diagnostic: print event_list pointer and nearby slots after ROM copy
    if (debug_enabled) {
        unsigned long sys_base = (unsigned long)nds_system;
        unsigned long sys_state = sys_base + 0x320;
        uintptr_t e_ptr = *(uintptr_t*)(sys_state + 0x318);
        unsigned long slot0 = *(unsigned long*)(sys_state + 0x300);
        unsigned long slot1 = *(unsigned long*)(sys_state + 0x308);
        unsigned long slot2 = *(unsigned long*)(sys_state + 0x310);
        unsigned long slot3 = *(unsigned long*)(sys_state + 0x318);
        fprintf(stderr, "[DRASTIC] load_nds: after ROM memcpy: event_list_ptr=0x%016lx slots=0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
                (unsigned long)e_ptr, slot0, slot1, slot2, slot3);
    }
    
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
            
            // Diagnostic: print event_list pointer and nearby slots after ARM9 load
            if (debug_enabled) {
                unsigned long sys_base = (unsigned long)nds_system;
                unsigned long sys_state = sys_base + 0x320;
                uintptr_t e_ptr = *(uintptr_t*)(sys_state + 0x318);
                unsigned long slot0 = *(unsigned long*)(sys_state + 0x300);
                unsigned long slot1 = *(unsigned long*)(sys_state + 0x308);
                unsigned long slot2 = *(unsigned long*)(sys_state + 0x310);
                unsigned long slot3 = *(unsigned long*)(sys_state + 0x318);
                fprintf(stderr, "[DRASTIC] load_nds: after ARM9 memcpy: event_list_ptr=0x%016lx slots=0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
                        (unsigned long)e_ptr, slot0, slot1, slot2, slot3);
                
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
                
                // Diagnostic: print event_list pointer and nearby slots after ARM7 load
                if (debug_enabled) {
                    unsigned long sys_base = (unsigned long)nds_system;
                    unsigned long sys_state = sys_base + 0x320;
                    uintptr_t e_ptr = *(uintptr_t*)(sys_state + 0x318);
                    unsigned long slot0 = *(unsigned long*)(sys_state + 0x300);
                    unsigned long slot1 = *(unsigned long*)(sys_state + 0x308);
                    unsigned long slot2 = *(unsigned long*)(sys_state + 0x310);
                    unsigned long slot3 = *(unsigned long*)(sys_state + 0x318);
                    fprintf(stderr, "[DRASTIC] load_nds: after ARM7 memcpy: event_list_ptr=0x%016lx slots=0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
                            (unsigned long)e_ptr, slot0, slot1, slot2, slot3);
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
    // NOTE: Preserve existing framebuffer contents to avoid wiping the test pattern
    // which helps debugging when the render path isn't producing output.
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] reset_system: preserving screen buffers (not clearing)");
        // Dump a small sample of the first 8 pixels from each screen for quick verification
        uint16_t *s0 = (uint16_t*)g_state.screen0_buffer;
        uint16_t *s1 = (uint16_t*)g_state.screen1_buffer;
        fprintf(stderr, " S0 first8:");
        for (int i = 0; i < 8; i++) fprintf(stderr, " %04x", s0[i]);
        fprintf(stderr, " S1 first8:");
        for (int i = 0; i < 8; i++) fprintf(stderr, " %04x", s1[i]);
        fprintf(stderr, "\n");

        // Also dump the event list pointer located at system_state + 0x318
        unsigned long sys_base = (unsigned long)nds_system;
        unsigned long sys_state = sys_base + 0x320;
        uintptr_t stored = *(uintptr_t*)(sys_state + 0x318);
        fprintf(stderr, "[DRASTIC] reset_system: event_list_ptr currently 0x%016lx (addr %p)\n",
                (unsigned long)stored, (void*)(sys_state + 0x318));
    }
    
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
    
    if (debug_enabled) {
        if (ptr) {
            fprintf(stderr, "[DRASTIC] get_screen_ptr(%d): returning %p\n", screen, ptr);
        } else {
            fprintf(stderr, "[DRASTIC] get_screen_ptr(%d): returning NULL\n", screen);
        }
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
    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] screen_copy16: screen=%d src=%p pitch=%u hires=%d bpp=%d\n",
                screen, src, get_screen_pitch(screen), get_screen_hires_mode(screen), get_screen_bytes_per_pixel());
    }
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
            if (debug_enabled && y == 0) {
                // 打印第一行前几个像素样本（如果可能）
                uint16_t *s = (uint16_t*)src_ptr;
                fprintf(stderr, "[DRASTIC] screen_copy16: screen=%d first pixels: 0x%04x 0x%04x 0x%04x\n",
                        screen, s[0], s[1], s[2]);
            }
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
    // 计算相对于 nds_system 的偏移量（兼容传入绝对地址或偏移）
    unsigned long system_base = (unsigned long)nds_system;
    long system_offset;
    if ((unsigned long)param_1 >= system_base && (unsigned long)param_1 < system_base + NDS_SYSTEM_SIZE) {
        system_offset = (long)((unsigned long)param_1 - system_base);
    } else {
        system_offset = param_1;
    }

    uint32_t uVar1, uVar2, uVar5, uVar6;
    int iVar3, iVar4, iVar7;

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] cpu_next_action: ENTRY system_offset=0x%lx\n", (unsigned long)system_offset);
    }

    // 触发并处理事件（使用系统状态结构在 nds_system + 0x320）
    execute_events((long)(nds_system + 0x320), 0);

    // ---- ARM7 中断 / 任务切换处理 ----
    if (*(int *)(nds_system + system_offset + 0x10cde58) != 0) {
        // 读取中断/任务切换信息
        uVar1 = *(uint32_t *)(nds_system + system_offset + 0x10ce110);
        uVar2 = *(uint32_t *)(nds_system + system_offset + 0x10cde60);
        *(uint32_t *)(nds_system + system_offset + 0x10cde60) = 0;
        *(uint32_t *)(nds_system + system_offset + 0x10cdff8) = 0;
        iVar3 = *(int *)(nds_system + system_offset + 0x10cde5c);

        if (((uVar1 >> 7) & 1) == 0) {
            // 不在中断模式：保存返回地址/状态并设置向量
            uVar5 = *(uint32_t *)(nds_system + system_offset + 0x10ce10c);
            uVar6 = *(uint32_t *)(nds_system + system_offset + 0x10cde54);

            if ((uVar5 & 1) == 0) {
                // ARM 模式
                iVar7 = (int)uVar5 + 4;
                if (uVar6 != 2) {
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
            *(uint32_t *)(nds_system + system_offset + 0x10ce110) = (uVar1 & 0xffffffc0) | 0x92;

            if (iVar3 == 0 && uVar2 != 0) {
                // 需要任务切换：跳转到任务切换处理
                if (1 < uVar2) {
                    *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) =
                        *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) & 0xfffffffd;
                }
            }
        } else {
            // 在中断模式
            if (iVar3 == 0 && uVar2 != 0) {
                if (1 < uVar2) {
                    *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) =
                        *(uint32_t *)(*(long *)(nds_system + system_offset + 0x10cdff0) + 0x2110) & 0xfffffffd;
                }
            }
        }
    }

    // ---- ARM9 中断 / 任务切换处理 ----
    if (*(int *)(nds_system + system_offset + 0x20d4448) != 0) {
        uVar1 = *(uint32_t *)(nds_system + system_offset + 0x20d4700);
        uVar2 = *(uint32_t *)(nds_system + system_offset + 0x20d4450);
        *(uint32_t *)(nds_system + system_offset + 0x20d4450) = 0;
        *(uint32_t *)(nds_system + system_offset + 0x20d45e8) = 0;
        iVar3 = *(int *)(nds_system + system_offset + 0x20d444c);

        if (((uVar1 >> 7) & 1) == 0) {
            uVar5 = *(uint32_t *)(nds_system + system_offset + 0x20d46fc);
            uVar6 = *(uint32_t *)(nds_system + system_offset + 0x20d4444);

            if ((uVar5 & 1) == 0) {
                // ARM 模式
                iVar7 = (int)uVar5 + 4;
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
            *(uint32_t *)(nds_system + system_offset + 0x20d4700) = (uVar1 & 0xffffffc0) | 0x92;

            if (iVar3 == 0 && uVar2 != 0) {
                // 需要任务切换时设置标志位
                if (1 < uVar2) {
                    *(uint32_t *)(*(long *)(nds_system + system_offset + 0x20d45e0) + 0x2110) =
                        *(uint32_t *)(*(long *)(*(long *)(nds_system + system_offset + 0x20d45e0) + 0x2110) & 0xfffffffd);
                }
            }
        }
    }

LAB_FINAL:
    // 最终处理阶段：更新时间计数器并执行 CPU

    if (debug_enabled) {
        fprintf(stderr, "[DRASTIC] cpu_next_action: LAB_FINAL - preparing to run CPU or task switch\n");
    }

    // 读取下一个事件的时间（事件列表在 param_1 + 0x318）
    // 如果 param_1 是绝对地址，使用 param_1；否则计算为 system_base + system_offset + 0x320
    long sys_state_addr = (long)(system_base + system_offset + 0x320);

    // 小心读取事件列表指针，防止无效指针导致崩溃（参见 SIGSEGV 报告）
    uintptr_t event_list_addr = *(uintptr_t*)(sys_state_addr + 0x318);
    unsigned long system_end = system_base + NDS_SYSTEM_SIZE;

    // Treat a NULL event list pointer as "no scheduled events" (quietly).
    // Only log/attempt repair when the pointer is non-zero and outside valid memory.
    if (event_list_addr == 0) {
        // no events scheduled
        iVar3 = 0;
    } else if (event_list_addr < system_base || event_list_addr + 4 > system_end) {
        if (debug_enabled) {
            fprintf(stderr, "[DRASTIC] cpu_next_action: invalid non-zero event_list pointer at 0x%lx (value=0x%lx) - attempting repair\n",
                    (unsigned long)(sys_state_addr + 0x318), (unsigned long)event_list_addr);
            // Dump raw nearby words for debugging
            unsigned long slot0 = *(unsigned long*)(sys_state_addr + 0x300);
            unsigned long slot1 = *(unsigned long*)(sys_state_addr + 0x308);
            unsigned long slot2 = *(unsigned long*)(sys_state_addr + 0x310);
            unsigned long slot3 = *(unsigned long*)(sys_state_addr + 0x318);
            fprintf(stderr, "[DRASTIC] cpu_next_action: raw slots @+0x300..+0x318: 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
                    slot0, slot1, slot2, slot3);
        }
        // Attempt to repair using backup if available and valid
        if (g_event_list_ptr_backup && g_event_list_ptr_backup >= system_base && g_event_list_ptr_backup < system_end) {
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] cpu_next_action: repairing event_list_ptr to backup 0x%016lx\n", (unsigned long)g_event_list_ptr_backup);
            }
            *(uintptr_t*)(sys_state_addr + 0x318) = g_event_list_ptr_backup;
            event_list_addr = g_event_list_ptr_backup;
            iVar3 = *(int *)event_list_addr;
        } else {
            iVar3 = 0;
        }
    } else {
        iVar3 = *(int *)event_list_addr;  // 读取事件节点的时间字段
    }

    // ARM7 状态
    iVar7 = *(int *)(nds_system + system_offset + 0x10cde60);  // ARM7 任务切换标志
    iVar4 = *(int *)(nds_system + system_offset + 0x10cdfe0);  // ARM7 时间累加器

    // 更新剩余时间和累加器
    *(int *)(sys_state_addr + 0x10) = iVar3;  // 设置剩余时间
    *(int *)(nds_system + system_offset + 0x10cdfe0) = iVar4 + iVar3;  // 累加时间

    // 如果需要任务切换，调用回调并返回
    if (iVar7 != 0) {
        *(uint32_t *)(nds_system + system_offset + 0x10cdfe0) = 0xffffffff;
        typedef void (*cpu_func_t)(long);
        cpu_func_t *func_ptr = (cpu_func_t *)(nds_system + system_offset + 0x10ce100);
        if (func_ptr && *func_ptr) {
            (*func_ptr)(sys_state_addr);
        }
        return;
    }

    // 执行 ARM7 CPU
    _execute_cpu(sys_state_addr + 0x15c7d50);
    /*
    // CPU 执行后的回调（例如 DMA/定时器等）
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
            if (debug_enabled) {
                fprintf(stderr, "[DRASTIC] execute_events: event at node %p handler=%p param=0x%lx next=%p\n",
                        (void*)event_node, (void*)handler, event_param, (void*)next_event_node);
            }
            // Call handler unconditionally (if non-NULL). This allows the event system
            // to invoke the appropriate internal handlers implemented in this core.
            handler(param_1, event_param);

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

