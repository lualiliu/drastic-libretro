/*
 * interpreter.h
 * ARM CPU 解释器相关函数声明
 * 
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "drastic.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// CPU 执行函数
void _execute_cpu(long param_1);

// 内存访问函数
uint32_t load_memory32_simple(long param_1, uint32_t address);
uint16_t load_memory16_simple(long param_1, uint32_t address);
uint8_t load_memory8_simple(long param_1, uint32_t address);
void store_memory32_simple(long param_1, uint32_t address, uint32_t value);
void store_memory16_simple(long param_1, uint32_t address, uint16_t value);
void store_memory8_simple(long param_1, uint32_t address, uint8_t value);

// 指令转换函数
uint32_t convert_thumb_instruction_to_arm_simple(uint16_t thumb_inst, int *local_c);

// 指令执行函数
int execute_arm_condition_simple(long param_1, uint32_t condition);
int execute_arm_instruction_simple(long param_1, uint32_t instruction);
void execute_arm_branch_op_simple(long param_1, uint32_t instruction);
void execute_arm_block_memory_op_simple(long param_1, uint32_t instruction);
void execute_arm_memory_op_simple(long param_1, uint32_t instruction, uint32_t offset, 
                                   uint32_t is_load, uint32_t is_signed, uint32_t is_halfword, 
                                   uint32_t is_writeback, uint32_t is_preindex);
void execute_arm_alu_op_simple(long param_1, uint32_t instruction, uint32_t operand2);
void execute_arm_msr_op_simple(long param_1, uint32_t instruction, uint32_t value);
uint32_t execute_arm_alu_load_op2_reg_simple(long param_1, uint32_t instruction);

#ifdef __cplusplus
}
#endif

#endif // INTERPRETER_H

