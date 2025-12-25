/*
 * interpreter.c
 * ARM CPU 解释器实现
 * 包含 CPU 指令执行、内存访问、指令转换等功能
 * 
 */

#include "interpreter.h"
#include "drastic.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// 声明全局系统内存（定义在 drastic_impl.c 中）
extern undefined1 nds_system[NDS_SYSTEM_SIZE];

// CPU 执行函数
// 基于 drastic.cpp 中 _execute_cpu 的实现（第24829行）
// param_1 是指向 CPU 状态结构的指针（通常是 param_1 + 0x15c7d50 对于 ARM7）
void _execute_cpu(long param_1) {
    if (!param_1) {
        return;
    }
    
    // 根据 drastic.cpp 第24852行，获取执行时间限制
    uint32_t time_limit = *(uint32_t*)(param_1 + 0x2290);
    
    // 检查时间限制是否有效（第24853行）
    if ((int)time_limit < 0) {
        return;  // 无效时间限制
    }
    
    // 检查是否有中断等待（第24854行）
    int interrupt_flag = *(int*)(param_1 + 0x2110);
    if (interrupt_flag != 0) {
        // 有中断等待，需要处理
        // 这里简化处理，实际应该处理中断
        return;
    }
    
    // 指令执行循环（第24855-25072行）
    int cycles = 0;
    
    // 主循环：执行指令直到达到时间限制或遇到中断
    do {
        // 获取当前 PC（程序计数器，第24857行）
        uint32_t pc = *(uint32_t*)(param_1 + 0x23bc);
        
        // 检查调试模式（第24858行）
        // 如果处于调试模式，调用 step_debug
        char debug_mode = *(char*)(param_1 + 0x2249);
        if (debug_mode == '\x01') {
            // 调试模式：增加计数器
            *(long*)(param_1 + 0x2220) = *(long*)(param_1 + 0x2220) + 1;
        } else {
            // 非调试模式：调用 step_debug（如果需要）
            // step_debug(param_1 + 0x2118, pc, time_limit);
            // 简化处理：跳过调试步骤
            pc = *(uint32_t*)(param_1 + 0x23bc);
        }
        
        // 检查 CPU 状态：ARM 模式还是 Thumb 模式（第24865行）
        uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
        int thumb_mode = (cpsr >> 5) & 1;
        
        uint32_t instruction;
        int instruction_cycles = 1;
        
        if (thumb_mode == 0) {
            // ARM 模式：加载 32 位指令（第24866-24870行）
            instruction = load_memory32_simple(param_1 + 0x23d0, pc);
            
            // 更新 PC（第24869-24870行）
            *(int*)(param_1 + 0x23ac) = pc + 8;  // 链接寄存器
            *(int*)(param_1 + 0x23bc) = pc + 4;  // PC
            
            // 解码指令类型（第24871-24873行）
            uint32_t inst_type = (instruction >> 25) & 7;  // bits 27-25
            uint32_t inst_cond = (instruction >> 28) & 0xF;  // bits 31-28
            
            // 计算指令周期（第24875-24887行）
            instruction_cycles = 1;
            if (inst_type == 4 && inst_cond == 0xE) {
                // LDM/STM 指令：根据寄存器数量计算周期
                // 简化处理：使用固定值
                instruction_cycles = 2;
            }
            
            // 根据 CPU 模式调整周期（第24882-24886行）
            int cpu_mode = *(int*)(param_1 + 0x210c);
            int adjusted_cycles = instruction_cycles * 2;
            if (cpu_mode == 1) {
                adjusted_cycles = instruction_cycles;
            }
            cycles += adjusted_cycles;
            
            // 执行指令（第24893行）
            execute_arm_instruction_simple(param_1, instruction);
            
            // 检查执行结果标志（第24894-24895行）
            uint32_t result_flags = *(uint32_t*)(param_1 + 0x22a8);
            time_limit = *(uint32_t*)(param_1 + 0x2290);
            
            if (result_flags != 0) {
                // 有结果标志，需要处理
                // 简化处理：清除标志
                *(uint32_t*)(param_1 + 0x22a8) = 0;
            }
            
        } else {
            // Thumb 模式：加载 16 位指令（第24969-24982行）
            uint16_t thumb_inst = load_memory16_simple(param_1 + 0x23d0, pc);
            
            // 更新 PC（第24970行）
            *(int*)(param_1 + 0x23bc) = pc + 2;
            
            // 转换 Thumb 指令到 ARM 格式（第24971行）
            int local_c = 0;
            instruction = convert_thumb_instruction_to_arm_simple(thumb_inst, &local_c);
            
            // 调整 PC（第24973-24976行）
            uint32_t adjusted_pc = pc + 2;
            if (local_c != 0) {
                adjusted_pc = adjusted_pc & 0xFFFFFFFD;
            }
            *(uint32_t*)(param_1 + 0x23ac) = adjusted_pc;
            
            // 解码指令类型
            uint32_t inst_type = (instruction >> 25) & 7;
            uint32_t inst_cond = (instruction >> 28) & 0xF;
            
            // 计算指令周期
            if (inst_type == 5) {
                // 特殊情况
                if (inst_cond == 0xE) {
                    instruction_cycles = 3;
                } else {
                    // 检查条件（第24989行）
                    int cond_result = execute_arm_condition_simple(param_1, inst_cond);
                    instruction_cycles = (cond_result == 0) ? 1 : 3;
                }
            } else {
                instruction_cycles = 1;
                if (inst_type == 4 && inst_cond == 0xE) {
                    instruction_cycles = 2;
                }
            }
            
            // 调整周期
            int cpu_mode = *(int*)(param_1 + 0x210c);
            int adjusted_cycles = instruction_cycles * 2;
            if (cpu_mode == 1) {
                adjusted_cycles = instruction_cycles;
            }
            cycles += adjusted_cycles;
            
            // 执行指令
            execute_arm_instruction_simple(param_1, instruction);
            
            // 检查结果标志
            uint32_t result_flags = *(uint32_t*)(param_1 + 0x22a8);
            time_limit = *(uint32_t*)(param_1 + 0x2290);
            
            if (result_flags != 0) {
                *(uint32_t*)(param_1 + 0x22a8) = 0;
            }
        }
        
        // 检查是否达到时间限制（第25072行）
        if (cycles >= (int)time_limit) {
            break;
        }
        
        // 检查中断标志（第25072行）
        interrupt_flag = *(int*)(param_1 + 0x2110);
        
    } while (interrupt_flag == 0);
    
    // 更新剩余时间（第25074-25076行）
    if (cycles < (int)time_limit) {
        *(uint32_t*)(param_1 + 0x2290) = time_limit - cycles;
    } else {
        *(uint32_t*)(param_1 + 0x2290) = 0xFFFFFFFF;
    }
}

// 辅助函数实现（基于 drastic.cpp 的实现逻辑）
// load_memory32: 基于 drastic.cpp 第13329行的实现
// param_1 是内存控制器指针（param_1 + 0x23d0），address 是虚拟地址
uint32_t load_memory32_simple(long param_1, uint32_t address) {
    // 根据 drastic.cpp，使用内存映射表查找
    // 地址的高21位（bits 31-11）用于查找映射表
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    // 检查页面是否已映射（bit 62-0 为 0 表示未映射）
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) == 0) {
        // 未映射的页面，根据 drastic.cpp 需要处理内存区域映射
        // 简化实现：如果是低地址区域（< 0x10000000），尝试直接访问
        if (address < 0x10000000) {
            // 尝试从 nds_system 直接访问
            if (address < sizeof(nds_system) - 3) {
                return *(uint32_t*)(nds_system + address);
            }
        }
        return 0xFFFFFFFF;  // 未映射返回全1
    }
    
    // 页面已映射，使用映射的物理地址
    // 根据 drastic.cpp，物理地址 = page_value * 4 + address
    uint32_t *physical_addr = (uint32_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
    return *physical_addr;
}

// load_memory16: 基于 drastic.cpp 第13244行的实现
uint16_t load_memory16_simple(long param_1, uint32_t address) {
    // 类似 load_memory32 的逻辑，但加载 16 位
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) == 0) {
        if (address < 0x10000000) {
            if (address < sizeof(nds_system) - 1) {
                return *(uint16_t*)(nds_system + address);
            }
        }
        return 0xFFFF;  // 未映射返回全1
    }
    
    uint16_t *physical_addr = (uint16_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
    return *physical_addr;
}

// load_memory8: 基于 drastic.cpp 第13161行的实现
// param_1 是内存控制器指针（param_1 + 0x23d0），address 是虚拟地址
uint8_t load_memory8_simple(long param_1, uint32_t address) {
    // 根据 drastic.cpp，使用内存映射表查找
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    // 检查页面是否已映射（bit 62-0 为 0 表示未映射）
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) == 0) {
        // 未映射的页面，根据 drastic.cpp 需要处理内存区域映射
        // 简化实现：如果是低地址区域（< 0x10000000），尝试直接访问
        if (address < 0x10000000) {
            // 尝试从 nds_system 直接访问
            if (address < sizeof(nds_system)) {
                return *(uint8_t*)(nds_system + address);
            }
        }
        return 0xFF;  // 未映射返回全1
    }
    
    // 页面已映射，使用映射的物理地址
    // 根据 drastic.cpp，物理地址 = page_value * 4 + address
    uint8_t *physical_addr = (uint8_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
    return *physical_addr;
}

// store_memory32: 基于 drastic.cpp 第13643行的实现
// param_1 是内存控制器指针（param_1 + 0x23d0），address 是虚拟地址，value 是要存储的32位值
void store_memory32_simple(long param_1, uint32_t address, uint32_t value) {
    // 根据 drastic.cpp，使用内存映射表查找
    // 地址的高21位（bits 31-11）用于查找映射表
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    // 检查页面是否已映射
    // 在 drastic.cpp 中，条件是 (uVar7 >> 0x3.0p0 & 1) == 0
    // 0x3.0p0 是浮点字面量，表示 3.0，在位移中会被转换为 3
    // 所以检查的是 bit 3 是否为 0（bit 3 == 0 表示已映射）
    // 但根据 load_memory 的逻辑，更合理的检查是低62位是否为0
    // 这里使用简化实现：检查 page_value 是否非零（表示已映射）
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) != 0) {
        // 页面已映射，使用映射的物理地址写入（第13660行）
        uint32_t *physical_addr = (uint32_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
        *physical_addr = value;
        return;
    }
    
    // 未映射的页面，根据 drastic.cpp 需要处理内存区域映射
    // 简化实现：如果是低地址区域（< 0x10000000），尝试直接写入 nds_system
    if (address < 0x10000000) {
        // 尝试直接写入 nds_system
        if (address < sizeof(nds_system) - 3) {
            *(uint32_t*)(nds_system + address) = value;
            return;
        }
    }
    
    // 对于高地址区域或超出范围的地址，忽略写入（简化处理）
}

// store_memory16: 基于 drastic.cpp 第13570行的实现
// param_1 是内存控制器指针（param_1 + 0x23d0），address 是虚拟地址，value 是要存储的16位值
void store_memory16_simple(long param_1, uint32_t address, uint16_t value) {
    // 类似 store_memory32 的逻辑，但存储 16 位
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    // 检查页面是否已映射（类似 store_memory32 的逻辑）
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) != 0) {
        // 页面已映射，使用映射的物理地址写入（第13589行）
        uint16_t *physical_addr = (uint16_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
        *physical_addr = value;
        return;
    }
    
    // 未映射的页面，简化处理
    if (address < 0x10000000) {
        if (address < sizeof(nds_system) - 1) {
            *(uint16_t*)(nds_system + address) = value;
            return;
        }
    }
    
    // 对于高地址区域或超出范围的地址，忽略写入（简化处理）
}

// store_memory8: 基于 drastic.cpp 第13497行的实现
// param_1 是内存控制器指针（param_1 + 0x23d0），address 是虚拟地址，value 是要存储的8位值
void store_memory8_simple(long param_1, uint32_t address, uint8_t value) {
    // 类似 store_memory32 的逻辑，但存储 8 位
    uint32_t page_index = address >> 11;
    unsigned long *page_entry = (unsigned long*)(param_1 + page_index * 8);
    unsigned long page_value = *page_entry;
    
    // 检查页面是否已映射（类似 store_memory32 的逻辑）
    if ((page_value & 0x3FFFFFFFFFFFFFFFUL) != 0) {
        // 页面已映射，使用映射的物理地址写入（第13516行）
        uint8_t *physical_addr = (uint8_t*)((unsigned long)(page_value * 4) + (address & 0x7FF));
        *physical_addr = value;
        return;
    }
    
    // 未映射的页面，简化处理
    if (address < 0x10000000) {
        if (address < sizeof(nds_system)) {
            *(uint8_t*)(nds_system + address) = value;
            return;
        }
    }
    
    // 对于高地址区域或超出范围的地址，忽略写入（简化处理）
}

// convert_thumb_instruction_to_arm: 基于 drastic.cpp 第25901行的实现
// 将 Thumb 16位指令转换为等效的 ARM 32位指令
uint32_t convert_thumb_instruction_to_arm_simple(uint16_t thumb_inst, int *local_c) {
    // 基于 drastic.cpp 的完整实现逻辑
    // param_1 = thumb_inst (16位指令)
    // param_2 = local_c (输出参数，指示是否需要调整 PC)
    
    uint32_t uVar1, uVar3, uVar4, uVar5, uVar6;
    int local_50[18];
    
    // 初始化（第25915-25916行）
    uVar4 = 0;
    if (local_c) {
        *local_c = 0;
    }
    
    // 提取指令格式（bits 15-13，第25917行）
    uVar6 = (thumb_inst >> 13) & 7;
    uVar3 = (uint32_t)thumb_inst;
    
    // 根据指令格式进行转换（第25919行开始）
    if (uVar6 == 4) {
        // Format 4: Load/Store with register offset / Load/Store sign-extended byte/halfword
        uVar6 = ((uVar3 >> 11) & 1) << 20;
        if (((uVar3 >> 12) & 1) == 0) {
            // Halfword/Byte data transfer
            uVar4 = (thumb_inst >> 6) & 0x1F;
            uVar6 = ((uVar3 & 7) << 12) | (((thumb_inst >> 9) & 3) << 8) |
                    ((uVar3 >> 3) & 7) << 16 | uVar6 | ((uVar4 & 7) << 1) | 0xE1C000B0;
        } else {
            // Load/Store with register offset
            uVar1 = (uVar3 & 0xFF) << 2;
            uVar4 = (unsigned long)uVar1;
            uVar6 = ((uVar3 >> 8) & 7) << 12 | uVar6 | uVar1 | 0xE58D0000;
        }
        goto LAB_00129050;
    }
    
    if (uVar6 < 5) {
        uVar1 = (thumb_inst >> 8);
        
        if (uVar6 == 2) {
            // Format 2: ALU operations / Hi register operations / BX
            uVar6 = uVar3 >> 10;
            if ((uVar3 >> 12) & 1) {
                // Hi register operations / BX
                uVar5 = ((uVar3 >> 3) & 7) << 16;
                uVar1 = ((thumb_inst >> 6) & 7) | ((uVar3 & 7) << 12);
                uVar4 = (unsigned long)uVar1;
                if (((uVar3 >> 9) & 1) == 0) {
                    uVar6 = ((uVar3 >> 11) & 1) << 20 | uVar1 | ((uVar6 & 1) << 22) | uVar5 | 0xE7800000;
                } else {
                    uVar4 = (unsigned long)(uVar6 & 3);
                    local_50[2] = 1;
                    local_50[3] = 3;
                    local_50[0] = 1;
                    local_50[1] = 2;
                    uVar6 = (((uVar6 & 3) != 0) << 20) | (local_50[uVar4] << 5) | uVar5 | uVar1 | 0xE1800090;
                }
                goto LAB_00129050;
            }
            
            if ((uVar6 & 6) != 0) {
                // LDR (literal)
                if (local_c) {
                    *local_c = 1;
                }
                uVar6 = ((uVar3 >> 8) & 7) << 12 | ((uVar3 & 0xFF) << 2) | 0xE59F0000;
                goto LAB_00129050;
            }
            
            if ((uVar6 & 7) == 1) {
                // ADD/SUB/CMP/MOV (register)
                uVar1 = uVar1 & 3;
                uVar5 = (thumb_inst >> 3) & 0xF;
                uVar6 = uVar3 >> 7;
                uVar4 = (unsigned long)(uVar5 | 0xE0800000);
                if (uVar1 == 2) {
                    // CMP
                    uVar6 = uVar5 | (((uVar6 & 1) << 3 | (uVar3 & 7)) << 12) | 0xE1A00000;
                } else if (uVar1 == 3) {
                    // BX
                    uVar6 = ((uVar6 & 1) << 5) | uVar5 | 0xE12FFF10;
                } else {
                    uVar6 = ((uVar6 & 1) << 3) | (uVar3 & 7);
                    if (uVar1 == 1) {
                        // CMP
                        uVar6 = uVar5 | (uVar6 << 16) | 0xE1500000;
                    } else {
                        // ADD/SUB/MOV
                        uVar6 = (uVar6 << 16) | (uVar6 << 12) | uVar5 | 0xE0800000;
                    }
                }
                goto LAB_00129050;
            }
            
            // ALU operations (bits 9-6)
            uVar6 = (uVar3 >> 6) & 0xF;
            uVar1 = uVar3 & 7;
            uVar4 = (unsigned long)uVar1;
            uVar5 = (thumb_inst >> 3) & 7;
            
            if (uVar6 == 9) {
                // RSB (Reverse Subtract)
                uVar6 = (uVar5 << 16) | (uVar1 << 12) | 0xE2700000;
                goto LAB_00129050;
            }
            
            if (uVar6 < 10) {
                if (uVar6 == 7) {
                    uVar3 = 5;
LAB_001294c8:
                    local_50[0] = 0;
                    local_50[1] = 1;
                    local_50[2] = 2;
                    local_50[3] = 0;
                    local_50[4] = 0;
                    local_50[5] = 3;
                    uVar6 = uVar1 | (local_50[uVar3] << 5);
                    uVar4 = (unsigned long)uVar6;
                    uVar6 = (uVar5 << 8) | (uVar1 << 12) | uVar6 | 0xE1B00010;
                    goto LAB_00129050;
                }
                if (((uVar3 >> 9) & 1) == 0) {
                    uVar3 = uVar6 - 2;
                    if (uVar3 < 3) goto LAB_001294c8;
                } else if (uVar6 == 8) {
                    goto LAB_00129434;
                }
            } else {
                if (uVar6 == 0xD) {
                    // MOV (high registers)
                    uVar6 = (uVar1 << 16) | 0xE0100090;
                    uVar4 = (unsigned long)uVar6;
                    uVar6 = uVar6 | uVar1 | (uVar5 << 8);
                    goto LAB_00129050;
                }
                if (uVar6 < 0xE) {
                    if (uVar6 - 10 < 2) {
LAB_00129434:
                        local_50[2] = 10;
                        local_50[3] = 0xB;
                        local_50[0] = 8;
                        local_50[1] = 0;
                        uVar6 = (local_50[uVar6 - 8] << 21) | 0xE0100000 | uVar5 | (uVar1 << 16);
                        goto LAB_00129050;
                    }
                } else if (uVar6 == 0xF) {
                    // SWI
                    uVar6 = uVar5 | (uVar1 << 12) | 0xE1F00000;
                    goto LAB_00129050;
                }
            }
            
            // Default ALU operations
            local_50[4] = 0;
            local_50[5] = 5;
            local_50[6] = 6;
            local_50[7] = 0;
            local_50[0] = 0;
            local_50[1] = 1;
            local_50[2] = 0;
            local_50[3] = 0;
            uVar5 = uVar5 | (uVar1 << 12);
            uVar4 = (unsigned long)uVar5;
            local_50[8] = 0;
            local_50[9] = 0;
            local_50[10] = 0;
            local_50[0xB] = 0;
            local_50[0xC] = 0xC;
            local_50[0xD] = 0;
            local_50[0xE] = 0xE;
            local_50[0xF] = 0;
            uVar6 = (uVar1 << 16) | (local_50[uVar6] << 21) | uVar5 | 0xE0100000;
            goto LAB_00129050;
        }
        
        if (uVar6 == 3) {
            // Format 3: Load/Store with immediate offset
            uVar1 = (thumb_inst >> 6) & 0x1F;
            uVar6 = uVar1 << 2;
            if ((thumb_inst >> 12) & 1) {
                uVar6 = uVar1;  // Byte transfer
            }
            uVar6 = ((uVar3 & 7) << 12) | uVar6;
            uVar4 = (unsigned long)uVar6;
            uVar6 = (((thumb_inst >> 3) & 7) << 16) | (((thumb_inst >> 11) & 1) << 20) | uVar6 |
                    (((thumb_inst >> 12) & 1) << 22) | 0xE5800000;
            goto LAB_00129050;
        }
        
        if (uVar6 == 1) {
            // Format 1: Move shifted register / Add/Subtract
            uVar1 = uVar1 & 7;
            local_50[2] = 4;
            local_50[3] = 2;
            local_50[0] = 0xD;
            local_50[1] = 10;
            uVar6 = uVar1 << 12;
            uVar4 = (unsigned long)uVar6;
            if (local_50[(thumb_inst >> 11) & 3] == 0xD) {
                // CMP
                uVar6 = uVar6 | (uVar3 & 0xFF) | 0xE3B00000;
            } else {
                // MOV/ADD/SUB
                uVar6 = (uVar3 & 0xFF) | (local_50[(thumb_inst >> 11) & 3] << 21) | uVar6 | (uVar1 << 16) | 0xE2100000;
            }
            goto LAB_00129050;
        }
    } else {
        if (uVar6 == 6) {
            // Format 6: Load address / Add offset to Stack Pointer
            if (((uVar3 >> 12) & 1) == 0) {
                // ADD SP, #immed_7*4
                uVar4 = (unsigned long)(uVar3 & 0xFF);
                uVar6 = (((uVar3 >> 11) & 1) << 20) | (((uVar3 >> 8) & 7) << 16) | (uVar3 & 0xFF) | 0xE8A00000;
            } else {
                // Load address
                uVar6 = (uVar3 >> 8) & 0xF;
                if (uVar6 == 0xF) {
                    // SWI
                    uVar6 = ((uVar3 & 0xFF) << 16) | 0xEF000000;
                } else {
                    // ADD Rd, PC/SP, #immed_8*4
                    uVar6 = ((int)(char)thumb_inst & 0xFFFFFF) | (uVar6 << 28) | 0xA000000;
                }
            }
            goto LAB_00129050;
        }
        
        if (uVar6 == 7) {
            // Format 7: Load/Store SP-relative / Load address
            if (((thumb_inst >> 11) & 3) - 1 < 3) {
                uVar6 = (uVar3 << 5) | 0xE6000010;
            } else {
                // B (conditional branch)
                uVar6 = ((uint32_t)((long)(thumb_inst << 0x35) >> 0x35) & 0xFFFFFF) | 0xEA000000;
            }
            goto LAB_00129050;
        }
        
        if (uVar6 == 5) {
            // Format 5: Load/Store with immediate offset (word) / Load/Store SP-relative / Load address
            if (((uVar3 >> 12) & 1) == 0) {
                uVar4 = (unsigned long)(uVar3 & 0xFF);
                uVar6 = ((uVar3 >> 8) & 7) << 12 | (uVar3 & 0xFF);
                if (((uVar3 >> 11) & 1) == 0) {
                    // LDR Rd, [PC, #immed_8*4]
                    uVar4 = 1;
                    if (local_c) {
                        *local_c = 1;
                    }
                    uVar6 = uVar6 | 0xE28F0F00;
                } else {
                    // STR/LDR Rd, [SP, #immed_8*4]
                    uVar6 = uVar6 | 0xE28D0F00;
                }
            } else if (((uVar3 >> 8) & 0xF) == 0) {
                // ADD SP, #immed_7*4
                uVar4 = 0xE28DDF00;
                uVar6 = (uVar3 & 0x7F) | 0xE28DDF00;
                if (thumb_inst & 0x80) {
                    uVar6 = (uVar3 & 0x7F) | 0xE24DDF00;
                }
            } else {
                // PUSH/POP
                uVar6 = uVar3 & 0xFF;
                if (((uVar3 >> 11) & 1) == 0) {
                    // PUSH
                    uVar3 = uVar6 | 0x4000;
                    if ((thumb_inst & 0x100) == 0) {
                        uVar3 = uVar6;
                    }
                    uVar6 = uVar3 | 0xE92D0000;
                } else {
                    // POP
                    uVar3 = uVar6 | 0x8000;
                    if ((thumb_inst & 0x100) == 0) {
                        uVar3 = uVar6;
                    }
                    uVar6 = uVar3 | 0xE8BD0000;
                }
            }
            goto LAB_00129050;
        }
    }
    
    // Format 0: Move shifted register (default case, 第26152-26168行)
    uVar6 = (thumb_inst >> 11) & 3;
    uVar4 = (thumb_inst >> 3) & 7;
    if (uVar6 == 3) {
        // Shift type
        uVar6 = 0x800000;
        if (thumb_inst & 0x200) {
            uVar6 = 0x400000;
        }
        uVar3 = ((uVar3 & 7) << 12) | ((uVar3 >> 6) & 7) | uVar6 | ((uint32_t)uVar4 << 16);
        uVar4 = 0xE0100000;
        uVar6 = uVar3 | 0xE0100000;
        if (thumb_inst & 0x400) {
            uVar6 = uVar3 | 0xE2100000;
        }
    } else {
        // Move shifted register
        uVar6 = ((uVar3 >> 6) & 0x1F) << 7 | uVar6 << 5 | ((uVar3 & 7) << 12) | (uint32_t)uVar4 | 0xE1B00000;
    }
    
LAB_00129050:
    return (uint32_t)uVar6;
}

// execute_arm_condition: 基于 drastic.cpp 第24100行的实现
// 检查 ARM 指令的条件码是否满足
int execute_arm_condition_simple(long param_1, uint32_t condition) {
    // 根据 drastic.cpp，condition 的高4位是条件码（bits 31-28）
    // 我们提取 bits 30-1 来判断条件类型
    uint32_t cond_high3 = (condition >> 1) & 0x7FFFFFFF;  // bits 30-1
    uint32_t cond_bit0 = condition & 1;  // bit 0
    uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
    
    // 提取 CPSR 标志位
    // N (Negative) = bit 31, Z (Zero) = bit 30, C (Carry) = bit 29, V (Overflow) = bit 28
    uint32_t n_flag = (cpsr >> 31) & 1;
    uint32_t z_flag = (cpsr >> 30) & 1;
    uint32_t c_flag = (cpsr >> 29) & 1;
    uint32_t v_flag = (cpsr >> 28) & 1;
    
    uint32_t result = 0;
    
    // 根据条件码处理（基于 drastic.cpp 第24109-24151行）
    if (cond_high3 == 4) {
        // GE (Greater or Equal, signed): (N == V)
        result = cond_bit0 ^ (c_flag & ((cpsr ^ 0x40000000) >> 1));
    } else if (cond_high3 < 5) {
        if (cond_high3 == 2) {
            // CS/HS (Carry Set/Unsigned Higher or Same): (C == 1)
            result = cond_bit0 ^ n_flag;
        } else if (cond_high3 < 3) {
            if (cond_high3 == 0) {
                // EQ (Equal): (Z == 1)
                result = cond_bit0 ^ z_flag;
            } else if (cond_high3 == 1) {
                // NE (Not Equal): (Z == 0)
                result = cond_bit0 ^ c_flag;
            } else {
                return 0;
            }
        } else {
            if (cond_high3 == 3) {
                // VS (Overflow Set): (V == 1)
                result = cond_bit0 ^ v_flag;
            } else {
                return 0;
            }
        }
    } else if (cond_high3 == 6) {
        // GT (Signed Greater Than): (Z == 0 && N == V)
        result = cond_bit0 ^ ((cpsr & 0x40000000) == 0 && v_flag == n_flag);
    } else {
        // 无条件执行 (AL): 总是执行
        result = 1;
        if (cond_high3 != 7) {
            if (cond_high3 == 5) {
                // PL (Plus/Positive or Zero): (N == 0)
                result = cond_bit0 ^ (v_flag == (n_flag ? 0 : 1));
            } else {
                return 0;
            }
        }
    }
    
    return (int)result;
}

// execute_arm_instruction: 基于 drastic.cpp 第24158行的实现
// 执行 ARM 指令的主要分发函数
int execute_arm_instruction_simple(long param_1, uint32_t instruction) {
    if (!param_1) {
        return 0;
    }
    
    // 提取条件码（bits 31-28，第24179行）
    uint32_t condition = (instruction >> 28) & 0xF;
    uint32_t instruction_u32 = (uint32_t)instruction;
    uint32_t instruction_full = instruction & 0xFFFFFFFF;
    
    // 检查条件码（第24183-24223行）
    if (condition != 0xE) {  // 不是 AL (Always)
        // 使用 execute_arm_condition_simple 检查条件
        // 注意：execute_arm_condition_simple 的参数应该是 condition 值，不是整个 instruction
        int cond_result = execute_arm_condition_simple(param_1, condition);
        if (!cond_result) {
            // 条件不满足，不执行指令
            return 0;
        }
    }
    
    // 提取指令类型（bits 27-25，第24225行）
    uint32_t inst_type = (instruction >> 25) & 7;
    
    // 根据指令类型分发（第24226行开始）
    if (inst_type == 5) {
        // 分支指令（Branch，第24226-24228行）
        execute_arm_branch_op_simple(param_1, instruction_full);
        return 1;
    }
    
    if (inst_type < 5) {
        // 检查 bit 27 (第24230行)
        uint32_t bit27 = (instruction >> 26) & 1;
        
        if (bit27 == 0) {
            // bit 27 == 0: Data Processing / PSR Transfer / Multiply / Extra Load/Store
            uint32_t bit26 = (instruction >> 25) & 1;
            
            if (bit26 != 0) {
                // bit 25 == 1: Load/Store with immediate offset / Load/Store with register offset
                uint32_t bit20 = (instruction >> 20) & 1;  // Load/Store bit
                uint32_t bit21 = (instruction >> 21) & 1;  // Write-back bit
                uint32_t bit22 = (instruction >> 22) & 1;  // Byte/Word bit
                uint32_t bit23 = (instruction >> 23) & 1;  // Up/Down bit
                uint32_t bit24 = (instruction >> 24) & 1;  // Pre/Post indexing bit
                
                // 计算偏移量（第24235-24272行）
                uint32_t offset = 0;
                uint32_t is_register_offset = ((instruction >> 25) & 1) != 0;
                
                if (!is_register_offset) {
                    // 立即数偏移（第24236行）
                    offset = instruction_u32 & 0xFFF;
                } else {
                    // 寄存器偏移（第24239-24271行）
                    if ((instruction_u32 >> 4) & 1) {
                        // 移位寄存器（需要特殊处理，这里简化）
                        // execute_arm_undefined_or_thumb_ext_op_simple(...)
                        // 暂时跳过未定义指令
                        return 1;
                    }
                    
                    // 读取偏移寄存器（第24244行）
                    uint32_t rm = instruction_u32 & 0xF;
                    uint32_t rm_value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
                    
                    // 提取移位类型和数量（第24243-24246行）
                    uint32_t shift_type = (instruction >> 5) & 3;
                    uint32_t shift_amount = (instruction >> 7) & 0x1F;
                    
                    // 执行移位操作（第24247-24271行）
                    if (shift_type == 2) {
                        // ASR (Arithmetic Shift Right)
                        if (shift_amount == 0) {
                            offset = (int32_t)rm_value >> 31;
                        } else {
                            offset = (int32_t)rm_value >> shift_amount;
                        }
                    } else if (shift_type == 3) {
                        // ROR (Rotate Right)
                        if (shift_amount == 0) {
                            // RRX
                            uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                            uint32_t carry = (cpsr >> 29) & 1;
                            offset = (rm_value >> 1) | (carry << 31);
                        } else {
                            offset = (rm_value >> shift_amount) | (rm_value << (32 - shift_amount));
                        }
                    } else if (shift_type == 1) {
                        // LSR (Logical Shift Right)
                        if (shift_amount == 0) {
                            offset = 0;
                        } else {
                            offset = rm_value >> shift_amount;
                        }
                    } else {
                        // LSL (Logical Shift Left)
                        offset = rm_value << shift_amount;
                    }
                }
                
                // 调用内存操作函数（第24273行）
                // 参数：param_1, instruction, offset, is_load, is_signed, is_halfword, is_writeback, is_preindex
                // bit20: is_load (1=load, 0=store)
                // bit22: is_halfword/byte (1=halfword/byte, 0=word)
                // bit21: is_writeback (1=writeback, 0=no writeback)
                // bit24: is_preindex (1=preindex, 0=postindex)
                // is_signed: 对于加载指令，bit6 表示是否符号扩展（LDRSH/LDRSB）
                // 根据 drastic.cpp 第24273行，这里传递 uVar14 (bit20, is_load), uVar15 (bit22, is_halfword), 0, 0, 0
                // 但我们的函数签名不同，需要正确映射
                uint32_t is_signed = 0;  // LDR/STR 默认无符号，LDRSH/LDRSB 需要特殊处理
                uint32_t is_halfword = bit22;  // bit22 表示是否为半字/字节
                execute_arm_memory_op_simple(param_1, instruction_full, offset, bit20, is_signed, is_halfword, bit21, bit24);
                return 1;
            }
            
            // bit 26 == 0: Data Processing / PSR Transfer / Multiply
            if (inst_type == 1) {
                // Data Processing with immediate shift / PSR Transfer（第24276行）
                uint32_t bit23_22 = (instruction >> 22) & 3;
                uint32_t bit21 = (instruction >> 21) & 1;
                
                if (bit23_22 != 2 || (instruction & 0x100000) != 0) {
                    // 数据处理指令（第24278行）
                    uint32_t bit24 = (instruction >> 24) & 1;
                    uint32_t bit25 = (instruction >> 8) & 0xF;
                    
                    // 处理立即数旋转（第24280-24287行）
                    if ((instruction & 0x100000) != 0 && (instruction & 0xF00) != 0) {
                        uint32_t rotate_imm = (instruction >> 8) & 0xF;
                        uint32_t imm8 = instruction_u32 & 0xFF;
                        uint32_t rotated_value = (imm8 >> (rotate_imm * 2)) | (imm8 << (32 - rotate_imm * 2));
                        
                        // 更新 CPSR C 位（如果 bit 20 被设置）
                        if ((instruction & 0x100000) != 0 && ((instruction >> 15) & 0xE) == 0) {
                            uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                            cpsr = (cpsr & 0xCFFFFFFF) | ((rotated_value >> 31) & 1) << 29;
                            *(uint32_t*)(param_1 + 0x23c0) = cpsr;
                        }
                    }
                    
                    // 加载操作数2（第24289行）
                    uint32_t operand2 = execute_arm_alu_load_op2_reg_simple(param_1, instruction_full);
                    execute_arm_alu_op_simple(param_1, instruction_full, operand2);
                    return 1;
                }
                
                // MSR 指令（第24292-24296行）
                if (((instruction >> 12) & 0xF) == 0xF) {
                    uint32_t rotate_imm = (instruction >> 8) & 0xF;
                    uint32_t imm8 = instruction_u32 & 0xFF;
                    uint32_t rotated_value = (imm8 >> (rotate_imm * 2)) | (imm8 << (32 - rotate_imm * 2));
                    execute_arm_msr_op_simple(param_1, instruction_full, rotated_value);
                    return 1;
                }
            }
        } else if (inst_type == 4) {
            // Block Data Transfer (LDM/STM)（第24301-24303行）
            execute_arm_block_memory_op_simple(param_1, instruction_full);
            return 1;
        }
        
        // 处理其他指令类型（第24305行开始）
        if ((instruction_u32 & 0x90) == 0x90) {
            // Multiply / Multiply-Accumulate / Extra Load/Store（第24305-24397行）
            uint32_t bit5_6 = (instruction_u32 >> 5) & 3;
            
            if (bit5_6 != 0) {
                // Extra Load/Store（LDRD/STRD/LDREX/STREX等，第24306-24321行）
                // 简化处理：暂时跳过
                return 1;
            }
            
            if ((instruction_u32 >> 24) & 1) {
                // 交换指令 SWP/SWPB（第24380-24396行）
                uint32_t opcode = (instruction >> 20) & 0xF;
                uint32_t rn = (instruction >> 16) & 0xF;
                uint32_t rd = (instruction >> 12) & 0xF;
                uint32_t rm = instruction_u32 & 0xF;
                
                uint32_t base_addr = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
                
                if (opcode == 8) {
                    // SWP: Swap word
                    uint32_t mem_value = load_memory32_simple(param_1 + 0x23d0, base_addr);
                    *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = mem_value;
                    uint32_t reg_value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
                    store_memory32_simple(param_1 + 0x23d0, base_addr, reg_value);
                    return 1;
                } else if (opcode == 10) {
                    // SWPB: Swap byte
                    uint32_t mem_value = load_memory16_simple(param_1 + 0x23d0, base_addr) & 0xFF;
                    *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = mem_value;
                    uint8_t reg_value = (uint8_t)(*(uint32_t*)(param_1 + (rm + 0x8dc) * 4) & 0xFF);
                    store_memory8_simple(param_1 + 0x23d0, base_addr, reg_value);
                    return 1;
                }
                // 其他交换指令（LDREX/STREX等）简化处理
                return 1;
            }
            
            // 乘法指令（MUL/MLA/UMULL/UMLAL/SMULL/SMLAL，第24322-24379行）
            uint32_t bit22 = (instruction_u32 >> 22) & 1;
            uint32_t bit21 = (instruction_u32 >> 21) & 1;
            uint32_t rs = (instruction >> 8) & 0xF;
            uint32_t rm = instruction_u32 & 0xF;
            uint32_t rd = (instruction >> 12) & 0xF;
            uint32_t rn = (instruction >> 16) & 0xF;
            
            uint32_t rs_value = *(uint32_t*)(param_1 + (rs + 0x8dc) * 4);
            uint32_t rm_value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
            
            if (bit22 == 0) {
                // 32位乘法（MUL/MLA，第24327-24341行）
                uint32_t result = rm_value * rs_value;
                
                if ((instruction_u32 >> 21) & 1) {
                    // MLA: Multiply-Accumulate
                    uint32_t acc = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
                    result = result + acc;
                }
                
                // 更新标志位（如果设置了 S 位，第24332-24339行）
                if ((instruction_u32 >> 20) & 1) {
                    uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                    cpsr = (cpsr & 0x3FFFFFFF);
                    if (result == 0) {
                        cpsr |= 0x40000000;  // Z flag
                    }
                    cpsr |= (result & 0x80000000);  // N flag
                    *(uint32_t*)(param_1 + 0x23c0) = cpsr;
                }
                
                *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = result;
                return 1;
            } else {
                // 64位乘法（UMULL/UMLAL/SMULL/SMLAL，第24343-24378行）
                // bit22 == 1 表示64位乘法
                // 根据 drastic.cpp 第24343-24378行：
                // - bit22==0: 32位乘法（MUL/MLA）
                // - bit22==1, bit21==0: UMULL（无符号）
                // - bit22==1, bit21==1: UMLAL（无符号累加）或 SMULL/SMLAL（有符号）
                // 判断有符号/无符号需要看 bit23，但这里简化处理
                // 累加通过 bit21 (A bit, 0x200000) 判断
                int is_signed = 0;  // 简化处理，假设无符号（实际需要根据 bit23 判断）
                int is_accumulate = ((instruction_u32 & 0x200000) != 0);  // bit21 == 1 表示累加
                
                uint64_t result;
                // 简化实现：假设无符号乘法（UMULL/UMLAL）
                // 实际应该根据 bit23 判断是否有符号
                uint64_t unsigned_rm = (uint64_t)rm_value;
                uint64_t unsigned_rs = (uint64_t)rs_value;
                result = unsigned_rm * unsigned_rs;
                
                if (is_accumulate) {
                    // UMLAL/SMLAL: 累加（第24345行）
                    uint32_t acc_lo = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
                    uint32_t acc_hi = *(uint32_t*)(param_1 + (rd + 0x8dc) * 4);
                    uint64_t acc = ((uint64_t)acc_hi << 32) | (uint64_t)acc_lo;
                    result = result + acc;
                }
                
                // 更新标志位（如果设置了 S 位）
                if ((instruction_u32 >> 20) & 1) {
                    uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                    cpsr = (cpsr & 0x3FFFFFFF);
                    if (result == 0) {
                        cpsr |= 0x40000000;  // Z flag
                    }
                    uint32_t result_hi = (uint32_t)(result >> 32);
                    cpsr |= (result_hi & 0x80000000);  // N flag
                    *(uint32_t*)(param_1 + 0x23c0) = cpsr;
                }
                
                // 存储64位结果：低32位在 rn，高32位在 rd
                *(uint32_t*)(param_1 + (rn + 0x8dc) * 4) = (uint32_t)(result & 0xFFFFFFFF);
                *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = (uint32_t)(result >> 32);
                return 1;
            }
        } else {
            // 其他数据处理指令（第24398-24403行）
            if ((((instruction >> 23) & 3) != 2) || ((instruction_u32 >> 20) & 1)) {
                uint32_t operand2 = execute_arm_alu_load_op2_reg_simple(param_1, instruction_full);
                execute_arm_alu_op_simple(param_1, instruction_full, operand2);
                return 1;
            }
            
            // 处理其他特殊情况（第24404-24465行）
            // 包括 BX、MRS、MSR、CLZ 等指令
            uint32_t bit7 = (instruction_u32 >> 7) & 1;
            
            if (bit7 == 0) {
                uint32_t bit4 = (instruction_u32 >> 4) & 1;
                
                if (bit4 == 0) {
                    // MRS 指令（第24406-24418行）
                    uint32_t bit21 = (instruction_u32 >> 21) & 1;
                    uint32_t rd = (instruction >> 12) & 0xF;
                    
                    if (bit21 != 0) {
                        // MSR (register): 写状态寄存器
                        uint32_t rm = instruction_u32 & 0xF;
                        uint32_t value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
                        execute_arm_msr_op_simple(param_1, instruction_full, value);
                        return 1;
                    }
                    
                    // MRS: 读取状态寄存器
                    uint32_t bit22 = (instruction_u32 >> 22) & 1;
                    if (bit22 == 0) {
                        // 读取 CPSR
                        *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = *(uint32_t*)(param_1 + 0x23c0);
                    } else {
                        // 读取 SPSR
                        uint32_t mode = *(uint32_t*)(param_1 + 0x2104);
                        *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = *(uint32_t*)(param_1 + mode * 4 + 0x20e8);
                    }
                    return 1;
                }
                
                // 其他指令（第24420-24453行）
                uint32_t shift_type = (instruction >> 5) & 3;
                
                if (shift_type == 2) {
                    // 饱和运算指令（需要 ARMv5TE，简化处理）
                    return 1;
                }
                
                if (shift_type == 3) {
                    // 未定义指令或异常
                    return 1;
                }
                
                if (shift_type == 1) {
                    // 特殊指令（简化处理）
                    return 1;
                }
                
                // CLZ 指令（Count Leading Zeros，第24441-24452行）
                if (((instruction_u32 >> 22) & 1) != 0) {
                    uint32_t rm = instruction_u32 & 0xF;
                    uint32_t rd = (instruction >> 12) & 0xF;
                    int32_t rm_value = *(int32_t*)(param_1 + (rm + 0x8dc) * 4);
                    
                    if (rm_value == 0) {
                        *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = 32;
                    } else {
                        // 计算前导零个数（简化实现）
                        int count = 0;
                        if ((rm_value & 0xFFFF0000) == 0) { count += 16; rm_value <<= 16; }
                        if ((rm_value & 0xFF000000) == 0) { count += 8; rm_value <<= 8; }
                        if ((rm_value & 0xF0000000) == 0) { count += 4; rm_value <<= 4; }
                        if ((rm_value & 0xC0000000) == 0) { count += 2; rm_value <<= 2; }
                        if ((rm_value & 0x80000000) == 0) { count += 1; }
                        *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = count;
                    }
                    return 1;
                }
            }
            
            // BX 指令（Branch and Exchange，第24454-24465行）
            uint32_t bit5 = (instruction_u32 >> 5) & 1;
            if (bit5 != 0) {
                uint32_t rm = instruction_u32 & 0xF;
                uint32_t rm_value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
                uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                
                // 保存链接寄存器（如果 bit 7 被设置）
                uint32_t pc = *(uint32_t*)(param_1 + 0x23bc);
                if ((instruction_u32 >> 7) & 1) {
                    if ((cpsr & 0x20) != 0) {
                        pc = pc | 1;  // Thumb 模式
                    }
                    *(uint32_t*)(param_1 + 0x23a8) = pc;
                }
                
                // 更新 PC（第24463行）
                *(uint32_t*)(param_1 + 0x23bc) = rm_value & 0xFFFFFFFE;
                
                // 更新 CPSR 的 T 位（bit 5）和其他标志位（第24464行）
                cpsr = (cpsr & 0xFFFFFFC0) | (cpsr & 0x1F) | ((rm_value & 1) << 5);
                *(uint32_t*)(param_1 + 0x23c0) = cpsr;
                return 1;
            }
            
            // 其他未处理的指令
            return 1;
        }
    }
    
    // 未知指令类型
    return 0;
}

// execute_arm_branch_op: 基于 drastic.cpp 第22954行的实现
// 执行 ARM 分支指令
void execute_arm_branch_op_simple(long param_1, uint32_t instruction) {
    // 获取当前 PC（第22964行）
    int32_t pc = *(int32_t*)(param_1 + 0x23bc);
    
    // 提取偏移量（bits 23-0，带符号扩展，第22965行）
    int32_t offset = (int32_t)(instruction << 8) >> 6;  // 24位有符号数，左移2位
    
    // 检查是否是 BL 指令（bit 24 = 1，第22966行）
    uint32_t condition = (instruction >> 28) & 0xF;
    uint32_t is_bl = (instruction >> 24) & 1;
    
    if (condition == 0xF) {
        // BLX（第22966-22969行）
        int32_t branch_offset = offset + (is_bl ? 2 : 0);
        *(int32_t*)(param_1 + 0x23a8) = pc;  // 保存链接寄存器
        *(uint32_t*)(param_1 + 0x23c0) = *(uint32_t*)(param_1 + 0x23c0) | 0x20;  // 设置 Thumb 模式
        *(int32_t*)(param_1 + 0x23bc) = pc + branch_offset;
    } else {
        // B 或 BL（第22971-22981行）
        int32_t branch_offset = offset;
        if ((*(uint32_t*)(param_1 + 0x23c0) & 0x20) != 0) {
            // Thumb 模式：偏移是16位的
            branch_offset = offset >> 1;
        }
        
        if (is_bl) {
            // BL：保存链接寄存器（第22977-22978行）
            *(int32_t*)(param_1 + 0x23a8) = pc;
        }
        
        // 更新 PC（第22981行）
        *(int32_t*)(param_1 + 0x23bc) = pc + branch_offset;
    }
}

// execute_arm_block_memory_op: 基于 drastic.cpp 第23564行的实现
// 执行块内存传输指令（LDM/STM）
void execute_arm_block_memory_op_simple(long param_1, uint32_t instruction) {
    if (!param_1) {
        return;
    }
    
    // 提取指令字段（第23580-23586行）
    uint32_t instruction_u32 = (uint32_t)instruction;
    uint32_t rn = (instruction >> 16) & 0xF;  // 基址寄存器编号（bits 19-16）
    uint32_t register_list = instruction_u32 & 0xFFFF;  // 寄存器列表（bits 15-0）
    uint32_t is_load = (instruction >> 20) & 1;  // bit 20: LDM (1) / STM (0)
    uint32_t is_writeback = (instruction >> 21) & 1;  // bit 21: Write-back
    uint32_t is_s_bit = (instruction >> 22) & 1;  // bit 22: S bit（用户模式寄存器组或加载 CPSR）
    uint32_t addressing_mode = (instruction >> 23) & 3;  // bits 24-23: 寻址模式
    
    // 计算寄存器数量（第23584行）
    // bit_count 是一个查找表，用于计算一个字节中1的个数
    // 简化实现：直接计算寄存器列表中1的个数
    int register_count = 0;
    uint32_t temp_list = register_list;
    while (temp_list) {
        register_count += (temp_list & 1);
        temp_list >>= 1;
    }
    
    // 读取基址寄存器值（第23587行）
    // 注意：寄存器在 param_1 + (rn + 0x8dc) * 4，但原始代码使用 lVar2 + 0x2370
    // 这意味着：lVar2 = param_1 + rn * 4，所以实际地址是 param_1 + rn * 4 + 0x2370
    // 0x2370 - 0x2370 = 0，所以偏移是 rn * 4 + 0x8dc * 4 = (rn + 0x8dc) * 4
    uint32_t base_addr = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
    uint32_t address;
    uint32_t final_addr;
    
    // 根据寻址模式计算地址（第23588-23602行）
    // addressing_mode: 0=IA, 1=IB, 2=DA, 3=DB
    // IA: Increment After (后递增) - 先访问后递增
    // IB: Increment Before (前递增) - 先递增后访问
    // DA: Decrement After (后递减) - 先访问后递减
    // DB: Decrement Before (前递减) - 先递减后访问
    if (addressing_mode == 2) {
        // DA: 从 base_addr - register_count * 4 开始（第23589-23590行）
        address = base_addr - (register_count * 4);
        final_addr = address;
    } else if (addressing_mode == 3) {
        // DB: 从 base_addr - 4 开始，递减访问，最终地址是 base_addr - register_count * 4
        final_addr = base_addr - (register_count * 4);
        address = base_addr - 4;
    } else {
        // IA (mode 0) 或 IB (mode 1)
        final_addr = base_addr + (register_count * 4);
        if (addressing_mode == 1) {
            // IB: 从 base_addr + 4 开始
            address = base_addr + 4;
        } else {
            // IA: 从 base_addr 开始
            address = base_addr;
        }
    }
    
    // 处理 S bit 和用户模式寄存器组（第23604-23621行）
    uint32_t saved_mode = 0xFFFFFFFF;
    if (is_s_bit != 0 && (instruction_u32 & 0x108000) != 0x108000) {
        // S bit 设置且寄存器列表不包含 PC (bit 15) 或写回未设置
        uint32_t current_mode = *(uint32_t*)(param_1 + 0x2104);
        if (current_mode != 0) {
            // 保存当前模式下的寄存器
            saved_mode = current_mode;
            // 切换到用户模式寄存器组（简化处理）
            *(uint32_t*)(param_1 + 0x2104) = 0;
        }
    }
    
    // 检查基址寄存器是否在寄存器列表中（第23622-23623行）
    uint32_t rn_bit = 1 << rn;
    address = address & 0xFFFFFFFC;  // 地址对齐到4字节边界
    
    // 处理加载指令（LDM，第23624-23643行）
    if (is_load != 0) {
        // 检查寄存器列表是否为空（第23625行）
        if (register_list == 0) {
            // 空列表，不执行操作
            goto writeback_label;
        }
        
        // 如果基址寄存器在列表中且设置了写回，需要特殊处理（第23628-23632行）
        if ((register_list & rn_bit) != 0 && is_writeback != 0) {
            // 检查条件：列表中有其他寄存器，且基址寄存器是第一个（最低位设置）
            // 原始代码：((uVar7 & (uVar10 ^ 0xffffffff)) != 0) && ((-uVar10 & uVar7) == uVar10)
            // 这表示：列表中有其他位，且基址寄存器位是第一个（即它是最低设置的位）
            uint32_t other_bits = register_list & (~rn_bit);
            uint32_t bits_before_rn = register_list & (rn_bit - 1);
            if (other_bits != 0 && bits_before_rn == 0) {
                // 基址寄存器是第一个，不写回
                is_writeback = 0;
            }
        }
        
        // 遍历寄存器列表，加载数据（第23634-23642行）
        uint32_t reg_index = 0;
        uint32_t list = register_list;
        // 注意：原始代码中地址总是递增（uVar8 = uVar8 + 4）
        // 地址方向由初始地址计算决定，遍历时始终递增
        while (list != 0) {
            if ((list & 1) != 0) {
                uint32_t value = load_memory32_simple(param_1 + 0x23d0, address);
                *(uint32_t*)(param_1 + (reg_index + 0x8dc) * 4) = value;
                address = address + 4;  // 总是递增
            }
            list = list >> 1;
            reg_index++;
        }
    } else {
        // 处理存储指令（STM，第23645-23653行）
        uint32_t reg_index = 0;
        uint32_t list = register_list;
        // 存储指令始终递增地址（根据原始代码）
        while (list != 0) {
            if ((list & 1) != 0) {
                uint32_t value = *(uint32_t*)(param_1 + (reg_index + 0x8dc) * 4);
                store_memory32_simple(param_1 + 0x23d0, address, value);
                address = address + 4;
            }
            list = list >> 1;
            reg_index++;
        }
    }
    
writeback_label:
    
    // 恢复保存的模式（第23655-23680行）
    if (saved_mode != 0xFFFFFFFF) {
        uint32_t current_mode = *(uint32_t*)(param_1 + 0x2104);
        if (current_mode != saved_mode) {
            // 恢复保存的寄存器组
            if (saved_mode == 1) {
                // FIQ 模式
                *(uint64_t*)(param_1 + 0x20c8) = *(uint64_t*)(param_1 + 0x2390);
                *(uint64_t*)(param_1 + 0x20d0) = *(uint64_t*)(param_1 + 0x2398);
                *(uint64_t*)(param_1 + 0x20d8) = *(uint64_t*)(param_1 + 0x23a0);
                *(uint32_t*)(param_1 + 0x20e0) = *(uint32_t*)(param_1 + 0x23a8);
            } else {
                // 其他模式
                *(uint64_t*)(param_1 + saved_mode * 8 + 0x2090) = *(uint64_t*)(param_1 + 0x23a4);
            }
            
            // 恢复当前模式
            if (current_mode == 1) {
                *(uint32_t*)(param_1 + 0x2104) = saved_mode;
                *(uint64_t*)(param_1 + 0x23a0) = *(uint64_t*)(param_1 + 0x20d8);
                *(uint64_t*)(param_1 + 0x2398) = *(uint64_t*)(param_1 + 0x20d0);
                *(uint64_t*)(param_1 + 0x2390) = *(uint64_t*)(param_1 + 0x20c8);
                *(uint32_t*)(param_1 + 0x23a8) = *(uint32_t*)(param_1 + 0x20e0);
            } else {
                *(uint64_t*)(param_1 + 0x23a4) = *(uint64_t*)(param_1 + current_mode * 8 + 0x2090);
                *(uint32_t*)(param_1 + 0x2104) = saved_mode;
                *(uint32_t*)(param_1 + 0x23a8) = *(uint32_t*)(param_1 + current_mode * 8 + 0x2094);
            }
        }
    }
    
    // 写回基址寄存器（如果设置了写回，第23681-23683行）
    if (is_writeback != 0) {
        *(uint32_t*)(param_1 + (rn + 0x8dc) * 4) = final_addr;
    }
    
    // 处理加载 PC 的特殊情况（第23684-23699行）
    if ((instruction_u32 & 0x108000) == 0x108000) {
        // 寄存器列表包含 PC (bit 15) 且设置了写回 (bit 21)
        // 从 R15 (PC) 寄存器加载值（在加载操作后，R15 已经被更新）
        uint32_t new_pc = *(uint32_t*)(param_1 + (15 + 0x8dc) * 4);  // R15 (PC)
        *(uint32_t*)(param_1 + 0x23bc) = new_pc;
        
        // 如果是 ARM9，需要特殊处理（第23687-23692行）
        int cpu_type = *(int*)(param_1 + 0x210c);
        if (cpu_type == 1) {
            *(uint32_t*)(param_1 + 0x23bc) = new_pc & 0xFFFFFFFE;
            uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
            cpsr = (cpsr & 0xFFFFFFC0) | (cpsr & 0x1F) | ((new_pc & 1) << 5);
            *(uint32_t*)(param_1 + 0x23c0) = cpsr;
        }
        
        // 如果设置了 S bit 且不是用户模式，加载 SPSR 到 CPSR（第23693-23698行）
        if ((instruction_u32 & 0x400000) != 0 && ((*(uint32_t*)(param_1 + 0x23c0) & 0x1F) != 0x10)) {
            uint32_t mode = *(uint32_t*)(param_1 + 0x2104);
            uint32_t spsr = *(uint32_t*)(param_1 + mode * 4 + 0x20e8);
            // 调用 execute_arm_set_cpsr（简化处理，直接设置）
            *(uint32_t*)(param_1 + 0x23c0) = spsr;
        }
    }
}

// execute_arm_memory_op: 基于 drastic.cpp 第23433行的实现
// 执行内存访问指令（LDR/STR/LDRB/STRB/LDRH/STRH等）
void execute_arm_memory_op_simple(long param_1, uint32_t instruction, uint32_t offset, 
                                         uint32_t is_load, uint32_t is_signed, uint32_t is_halfword, 
                                         uint32_t is_writeback, uint32_t is_preindex) {
    // 提取寄存器编号（第23433行）
    uint32_t rd = (instruction >> 12) & 0xF;  // 目标/源寄存器
    uint32_t rn = (instruction >> 16) & 0xF;  // 基址寄存器
    
    // 读取基址（第23433行）
    uint32_t base_addr = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
    
    // 计算地址
    uint32_t address;
    if (is_preindex) {
        // 前变址：先计算地址
        address = base_addr + offset;
    } else {
        // 后变址：使用原始基址
        address = base_addr;
    }
    
    if (is_load) {
        // 加载指令
        if (is_halfword) {
            // LDRH/LDRSH
            uint16_t value = load_memory16_simple(param_1 + 0x23d0, address);
            if (is_signed) {
                // 符号扩展
                int32_t signed_value = (int16_t)value;
                *(int32_t*)(param_1 + (rd + 0x8dc) * 4) = signed_value;
            } else {
                *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = value;
            }
        } else {
            // LDR/LDRB
            if ((instruction >> 22) & 1) {
                // LDRB
                uint8_t value8 = load_memory8_simple(param_1 + 0x23d0, address);
                // 零扩展或符号扩展（根据指令）
                uint32_t value;
                if (is_signed) {
                    // LDRSB: 符号扩展
                    value = (int32_t)(int8_t)value8;
                } else {
                    // LDRB: 零扩展
                    value = (uint32_t)value8;
                }
                *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = value;
            } else {
                // LDR
                uint32_t value = load_memory32_simple(param_1 + 0x23d0, address);
                *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = value;
            }
        }
    } else {
        // 存储指令
        uint32_t value = *(uint32_t*)(param_1 + (rd + 0x8dc) * 4);
        if (is_halfword) {
            // STRH
            uint16_t value16 = (uint16_t)(value & 0xFFFF);
            store_memory16_simple(param_1 + 0x23d0, address, value16);
        } else {
            // STR/STRB
            if ((instruction >> 22) & 1) {
                // STRB
                uint8_t value8 = (uint8_t)(value & 0xFF);
                store_memory8_simple(param_1 + 0x23d0, address, value8);
            } else {
                // STR
                store_memory32_simple(param_1 + 0x23d0, address, value);
            }
        }
    }
    
    // 写回基址寄存器（如果设置了写回）
    if (is_writeback) {
        if (is_preindex) {
            *(uint32_t*)(param_1 + (rn + 0x8dc) * 4) = address;
        } else {
            *(uint32_t*)(param_1 + (rn + 0x8dc) * 4) = base_addr + offset;
        }
    }
}

// execute_arm_alu_op: 基于 drastic.cpp 第22646行的实现
// 执行 ALU 操作（AND、ORR、ADD、SUB等）
void execute_arm_alu_op_simple(long param_1, uint32_t instruction, uint32_t operand2) {
    // 提取操作码（bits 24-21，第22658行）
    uint32_t opcode = (instruction >> 21) & 0xF;
    uint32_t rd = (instruction >> 12) & 0xF;
    uint32_t rn = (instruction >> 16) & 0xF;
    
    // 读取操作数1（第22659行）
    uint32_t operand1 = *(uint32_t*)(param_1 + (rn + 0x8dc) * 4);
    
    // 读取 CPSR（第22659行）
    uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
    uint32_t carry = (cpsr >> 29) & 1;
    
    uint32_t result = 0;
    uint32_t new_cpsr = cpsr & 0xF0000000;  // 保留条件标志位
    
    // 根据操作码执行操作（第22662行开始）
    switch (opcode) {
        case 0x0: // AND
            result = operand1 & operand2;
            // 标志位在 S 位检查时更新
            break;
        case 0x1: // EOR
            result = operand1 ^ operand2;
            // 标志位在 S 位检查时更新
            break;
        case 0x2: // SUB
            result = operand1 - operand2;
            // 更新标志位
            new_cpsr |= (result & 0x80000000);  // N
            new_cpsr |= ((result == 0) ? 0x40000000 : 0);  // Z
            new_cpsr |= ((operand1 >= operand2) ? 0x20000000 : 0);  // C
            new_cpsr |= ((((int32_t)operand1 ^ (int32_t)operand2) & ((int32_t)operand1 ^ (int32_t)result)) >> 31) << 28;  // V
            break;
        case 0x3: // RSB
            result = operand2 - operand1;
            break;
        case 0x4: // ADD
            result = operand1 + operand2;
            // 更新标志位
            new_cpsr |= (result & 0x80000000);  // N
            new_cpsr |= ((result == 0) ? 0x40000000 : 0);  // Z
            new_cpsr |= ((result < operand1) ? 0x20000000 : 0);  // C
            new_cpsr |= ((((int32_t)operand1 ^ (int32_t)operand2 ^ 0x80000000) & ((int32_t)operand1 ^ (int32_t)result)) >> 31) << 28;  // V
            break;
        case 0x5: // ADC
            result = operand1 + operand2 + carry;
            break;
        case 0x6: // SBC
            result = operand1 - operand2 - (1 - carry);
            break;
        case 0x7: // RSC
            result = operand2 - operand1 - (1 - carry);
            break;
        case 0x8: // TST
            result = operand1 & operand2;
            // 不写回结果，只更新标志位
            rd = 0xF;  // 不写回
            break;
        case 0x9: // TEQ
            result = operand1 ^ operand2;
            rd = 0xF;  // 不写回
            break;
        case 0xA: // CMP
            result = operand1 - operand2;
            rd = 0xF;  // 不写回
            break;
        case 0xB: // CMN
            result = operand1 + operand2;
            rd = 0xF;  // 不写回
            break;
        case 0xC: // ORR
            result = operand1 | operand2;
            // 标志位在 S 位检查时更新
            break;
        case 0xD: // MOV
            result = operand2;
            // 标志位在 S 位检查时更新
            break;
        case 0xE: // BIC
            result = operand1 & (~operand2);
            // 标志位在 S 位检查时更新
            break;
        case 0xF: // MVN
            result = ~operand2;
            // 标志位在 S 位检查时更新
            break;
    }
    
    // 更新标志位（如果设置了 S 位）
    if ((instruction >> 20) & 1) {
        if (opcode >= 0x8 && opcode <= 0xB) {
            // TST/TEQ/CMP/CMN：总是更新标志位
            // 对于这些操作，new_cpsr 已经包含正确的标志位
            *(uint32_t*)(param_1 + 0x23c0) = (cpsr & 0x0FFFFFFF) | (new_cpsr & 0xF0000000);
        } else {
            // 其他操作：如果设置了 S 位，更新标志位
            // 对于已经设置了标志位的操作（SUB, RSB, ADD, ADC, SBC, RSC），使用 new_cpsr
            // 对于其他操作，需要计算标志位
            if (opcode == 0x2 || opcode == 0x3 || opcode == 0x4 || opcode == 0x5 || opcode == 0x6 || opcode == 0x7) {
                // 已经设置了标志位的操作
                *(uint32_t*)(param_1 + 0x23c0) = (cpsr & 0x0FFFFFFF) | (new_cpsr & 0xF0000000);
            } else {
                // 需要计算标志位的操作（AND, EOR, ORR, MOV, BIC, MVN）
                new_cpsr = (cpsr & 0x0FFFFFFF);
                new_cpsr |= (result & 0x80000000);  // N
                new_cpsr |= ((result == 0) ? 0x40000000 : 0);  // Z
                // C 和 V 保持不变（这些操作不影响）
                *(uint32_t*)(param_1 + 0x23c0) = new_cpsr;
            }
        }
    }
    
    // 写回结果（如果目标寄存器不是 R15）
    if (rd != 0xF) {
        *(uint32_t*)(param_1 + (rd + 0x8dc) * 4) = result;
    }
}

// execute_arm_msr_op: 基于 drastic.cpp 第23219行的实现
// 执行 MSR 指令（写状态寄存器）
void execute_arm_msr_op_simple(long param_1, uint32_t instruction, uint32_t value) {
    // 提取目标（bit 22：CPSR 或 SPSR）
    uint32_t is_spsr = (instruction >> 22) & 1;
    
    // 提取字段掩码（bits 19-16）
    uint32_t field_mask = (instruction >> 16) & 0xF;
    
    // 读取当前 CPSR/SPSR（第23219行）
    uint32_t *psr_ptr;
    if (is_spsr) {
        // SPSR
        uint32_t mode = *(uint32_t*)(param_1 + 0x2104);
        psr_ptr = (uint32_t*)(param_1 + mode * 4 + 0x20e8);
    } else {
        // CPSR
        psr_ptr = (uint32_t*)(param_1 + 0x23c0);
    }
    
    uint32_t psr = *psr_ptr;
    
    // 根据字段掩码更新 PSR（第23219行）
    if (field_mask & 1) {
        // c 字段（bits 7-0）
        psr = (psr & 0xFFFFFF00) | (value & 0xFF);
    }
    if (field_mask & 2) {
        // x 字段（bits 15-8）
        psr = (psr & 0xFFFF00FF) | ((value << 8) & 0xFF00);
    }
    if (field_mask & 4) {
        // s 字段（bits 23-16）
        psr = (psr & 0xFF00FFFF) | ((value << 16) & 0xFF0000);
    }
    if (field_mask & 8) {
        // f 字段（bits 31-24）
        psr = (psr & 0x00FFFFFF) | (value & 0xFF000000);
    }
    
    *psr_ptr = psr;
}

// execute_arm_alu_load_op2_reg: 基于 drastic.cpp 第21902行的实现
// 加载 ALU 操作的操作数2（可能是立即数或寄存器移位）
uint32_t execute_arm_alu_load_op2_reg_simple(long param_1, uint32_t instruction) {
    // 检查 bit 25：立即数还是寄存器
    if ((instruction >> 25) & 1) {
        // 立即数（bits 7-0）+ 旋转（bits 11-8）
        uint32_t rotate = (instruction >> 8) & 0xF;
        uint32_t imm8 = instruction & 0xFF;
        return (imm8 >> (rotate * 2)) | (imm8 << (32 - rotate * 2));
    } else {
        // 寄存器移位
        uint32_t rm = instruction & 0xF;
        uint32_t rm_value = *(uint32_t*)(param_1 + (rm + 0x8dc) * 4);
        
        uint32_t shift_type = (instruction >> 5) & 3;
        uint32_t shift_amount;
        
        if ((instruction >> 4) & 1) {
            // 由寄存器指定移位量
            uint32_t rs = (instruction >> 8) & 0xF;
            shift_amount = *(uint32_t*)(param_1 + (rs + 0x8dc) * 4) & 0xFF;
        } else {
            // 立即数移位量
            shift_amount = (instruction >> 7) & 0x1F;
        }
        
        // 执行移位操作
        switch (shift_type) {
            case 0: // LSL
                return rm_value << shift_amount;
            case 1: // LSR
                if (shift_amount == 0) return 0;
                return rm_value >> shift_amount;
            case 2: // ASR
                if (shift_amount == 0) return (int32_t)rm_value >> 31;
                return (int32_t)rm_value >> shift_amount;
            case 3: // ROR/RRX
                if (shift_amount == 0) {
                    // RRX：通过进位旋转
                    uint32_t cpsr = *(uint32_t*)(param_1 + 0x23c0);
                    uint32_t carry = (cpsr >> 29) & 1;
                    return (rm_value >> 1) | (carry << 31);
                } else {
                    return (rm_value >> shift_amount) | (rm_value << (32 - shift_amount));
                }
        }
    }
    
    return 0;
}

#ifdef __cplusplus
}
#endif