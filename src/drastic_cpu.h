#ifndef DRASTIC_CPU_H
#define DRASTIC_CPU_H

// ========================================================================
// drastic_cpu.h
// 从 cpu_next_action_arm9_to_arm7 调用树提取的函数
// ========================================================================

#include "drastic_val.h"
#include "drastic_functions.h"

// ========================================================================
// 前向声明和全局变量声明
// ========================================================================

#ifdef __cplusplus
extern "C" {
#endif

// 编译器内部变量声明
//extern long __stack_chk_guard;

// 函数前向声明（解决调用顺序问题）
// 注意: load_memory32 和 load_memory16 已在 drastic_functions.h 中声明，不在此重复声明
void _execute_cpu(long param_1);
void step_debug(long *param_1,ulong param_2,undefined4 param_3);
// convert_thumb_instruction_to_arm 在 drastic_functions.h 中声明为 void，但实际返回 ulong
// 不在此声明，直接使用实现
uint execute_arm_condition(long param_1,ulong param_2);
ulong execute_arm_instruction(long param_1,ulong param_2);
void execute_arm_branch_op(long param_1,ulong param_2);
void execute_arm_undefined_or_thumb_ext_op(long param_1,ulong param_2);
void execute_arm_memory_op(long param_1,ulong param_2,int param_3,int param_4,int param_5,int param_6,int param_7,int param_8);
void execute_arm_alu_op(long param_1,ulong param_2,uint param_3);
void execute_arm_msr_op(long param_1,ulong param_2,uint param_3);
void execute_arm_block_memory_op(long param_1,ulong param_2);
void execute_arm_saturating_alu_op(long param_1,ulong param_2);
void execute_arm_raise_exception(long param_1,uint param_2);
void execute_arm_coprocessor_register_transfer_op(long param_1,ulong param_2);
ulong execute_arm_alu_load_op2_reg(long param_1,ulong param_2);
void store_memory32(long param_1,ulong param_2,undefined4 param_3);
void store_memory16(long param_1,ulong param_2,undefined2 param_3);
void store_memory8(long param_1,ulong param_2,undefined1 param_3);
void execute_arm_set_cpsr(long param_1,undefined4 param_2);
void coprocessor_register_store(long *param_1,int param_2,int param_3,int param_4,ulong param_5);
// __printf_chk 是 glibc 的内部函数，需要声明
int __printf_chk(int flag, const char *format, ...);

// ========================================================================
// 函数实现
// ========================================================================

// --- 函数实现: cpu_next_action_arm9_to_arm7 ---
// 使用 static inline 避免与 drastic_val.h 中的 extern undefined 声明冲突
// 注意: 这会在每个编译单元中创建副本，但可以避免链接错误
static inline void cpu_next_action_arm9_to_arm7_impl(long param_1)

{
  *(int *)(nds_system + param_1 + 0x20d45d0) =
       *(int *)(nds_system + param_1 + 0x20d45d0) + *(int *)(param_1 + 0x10);
  _execute_cpu(param_1 + 0x25ce340);
                    // WARNING: Could not recover jumptable at 0x001282b8. Too many branches
                    // WARNING: Treating indirect jump as call
  ((void (*)(undefined8))**(code **)(nds_system + param_1 + 0x20d46f0))(*(undefined8 *)(nds_system + param_1 + 0x20d4598));
                    // WARNING: Could not recover jumptable at 0x001282b8. Too many branches
                    // WARNING: Treating indirect jump as call
  ((void (*)(undefined8))**(code **)(nds_system + param_1 + 0x20d46f0))(*(undefined8 *)(nds_system + param_1 + 0x20d4598));
  return;
}

// 定义别名以匹配 drastic_val.h 中的声明
#define cpu_next_action_arm9_to_arm7 cpu_next_action_arm9_to_arm7_impl



// --- 函数实现: _execute_cpu ---
// 注意：在 libretro 模式下，此函数由 drastic.cpp 提供
#ifndef DRASTIC_LIBRETRO
void _execute_cpu(long param_1)

{
  uint uVar1;
  bool bVar2;
  undefined2 uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  ulong uVar8;
  int iVar9;
  uint uVar10;
  undefined4 uVar11;
  uint uVar12;
  ulong uVar13;
  int iVar14;
  uint uVar16;
  int local_c;
  long local_8;
  ulong uVar15;
  
  local_8 = __stack_chk_guard;
  uVar16 = *(uint *)(param_1 + 0x2290);
  if (-1 < (int)uVar16) {
    if (*(int *)(param_1 + 0x2110) == 0) {
      iVar6 = 0;
      do {
        uVar11 = *(undefined4 *)(param_1 + 0x23bc);
        if (*(char *)(param_1 + 0x2249) == '\x01') {
          *(long *)(param_1 + 0x2220) = *(long *)(param_1 + 0x2220) + 1;
        }
        else {
          step_debug(param_1 + 0x2118,uVar11,uVar16);
          uVar11 = *(undefined4 *)(param_1 + 0x23bc);
        }
        if ((*(uint *)(param_1 + 0x23c0) >> 5 & 1) == 0) {
          uVar8 = load_memory32(param_1 + 0x23d0,uVar11);
          uVar8 = uVar8 & 0xffffffff;
          uVar13 = uVar8 >> 0x19;
          *(int *)(param_1 + 0x23ac) = *(int *)(param_1 + 0x23bc) + 8;
          *(int *)(param_1 + 0x23bc) = *(int *)(param_1 + 0x23bc) + 4;
          uVar4 = (uint)uVar13 & 7;
          uVar15 = uVar8 >> 0x1c;
          iVar14 = (int)uVar15;
          if (uVar4 == 5) goto LAB_001280f8;
LAB_00127e5c:
          iVar9 = 1;
          if (uVar4 == 4 && iVar14 == 0xe) {
            iVar9 = (uint)(byte)(&bit_count)[uVar8 >> 8 & 0xff] +
                    (uint)(byte)(&bit_count)[(uint)uVar8 & 0xff];
          }
LAB_00127e6c:
          iVar5 = *(int *)(param_1 + 0x210c);
          iVar7 = iVar9 * 2;
          if (iVar5 == 1) {
            iVar7 = iVar9;
          }
          iVar6 = iVar6 + iVar7;
          if (uVar4 != 4) goto LAB_00127e88;
LAB_00128134:
          if (((uint)uVar8 & 0x108000) == 0x108000) goto LAB_00128088;
LAB_00127ec8:
          *(uint *)(param_1 + 0x2290) = uVar16;
          execute_arm_instruction(param_1,uVar8 & 0xffffffff);
          uVar4 = *(uint *)(param_1 + 0x22a8);
          uVar16 = *(uint *)(param_1 + 0x2290);
          if (uVar4 != 0) {
            uVar12 = 0;
LAB_00127ee8:
            if ((uVar4 & (triggered_flags_11721 ^ 0xffffffff)) != 0) {
              triggered_flags_11721 = triggered_flags_11721 | uVar4;
            }
            if ((uVar4 & 8) != 0) {
              iVar6 = 0;
            }
            bVar2 = (uVar4 & 4) == 0;
            if ((!bVar2 && uVar16 != 0) && (bVar2 || -1 < (int)uVar16)) {
              uVar12 = 0;
              iVar14 = *(int *)(*(long *)(param_1 + 0x2258) + 0x10) - uVar16;
              uVar16 = 0;
              *(int *)(*(long *)(param_1 + 0x2258) + 0x10) = iVar14;
            }
            else {
              uVar12 = uVar12 & uVar16 >> 0x1f;
            }
            if (((uVar4 >> 1 & 1) == 0) ||
               (uVar4 = *(uint *)(param_1 + 0x23c0), (uVar4 >> 7 & 1) != 0)) {
              *(undefined4 *)(param_1 + 0x22a8) = 0;
            }
            else {
              uVar1 = *(uint *)(param_1 + 0x23bc);
              if ((uVar1 & 1) == 0) {
                uVar10 = *(uint *)(param_1 + 0x2104);
                iVar6 = uVar1 + 4;
                if (uVar10 == 2) {
                  *(int *)(param_1 + 0x23a8) = iVar6;
                }
                else {
LAB_00127f8c:
                  *(undefined8 *)(param_1 + (ulong)uVar10 * 8 + 0x2090) =
                       *(undefined8 *)(param_1 + 0x23a4);
                  if (uVar10 == 1) {
                    *(undefined4 *)(param_1 + 0x23a0) = *(undefined4 *)(param_1 + 0x20d8);
                    uVar11 = *(undefined4 *)(param_1 + 0x20dc);
                    *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
                    *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
                  }
                  else {
                    uVar11 = *(undefined4 *)(param_1 + 0x20a0);
                  }
                  *(undefined4 *)(param_1 + 0x2104) = 2;
                  *(undefined4 *)(param_1 + 0x23a4) = uVar11;
                  *(int *)(param_1 + 0x23a8) = iVar6;
                  if ((uVar1 & 1) != 0) goto LAB_00127fc0;
                }
                *(uint *)(param_1 + 0x20f0) = uVar4;
              }
              else {
                uVar10 = *(uint *)(param_1 + 0x2104);
                *(uint *)(param_1 + 0x23bc) = uVar1 & 0xfffffffe;
                iVar6 = (uVar1 & 0xfffffffe) + 4;
                if (uVar10 != 2) goto LAB_00127f8c;
                *(int *)(param_1 + 0x23a8) = iVar6;
LAB_00127fc0:
                *(uint *)(param_1 + 0x20f0) = uVar4 | 0x20;
              }
              iVar14 = 0x18;
              if (*(int *)(param_1 + 0x210c) == 1) {
                iVar14 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 0x18;
              }
              iVar6 = 0;
              *(undefined4 *)(param_1 + 0x22a8) = 0;
              *(int *)(param_1 + 0x23bc) = iVar14;
              *(uint *)(param_1 + 0x23c0) = uVar4 & 0xffffffc0 | 0x92;
            }
            goto joined_r0x00127f28;
          }
        }
        else {
          uVar3 = load_memory16(param_1 + 0x23d0,0);
          *(int *)(param_1 + 0x23bc) = *(int *)(param_1 + 0x23bc) + 2;
          uVar8 = convert_thumb_instruction_to_arm(uVar3,&local_c);
          uVar8 = uVar8 & 0xffffffff;
          uVar4 = *(int *)(param_1 + 0x23bc) + 2;
          if (local_c != 0) {
            uVar4 = uVar4 & 0xfffffffd;
          }
          uVar13 = uVar8 >> 0x19;
          *(uint *)(param_1 + 0x23ac) = uVar4;
          uVar4 = (uint)uVar13 & 7;
          uVar15 = uVar8 >> 0x1c;
          iVar14 = (int)uVar15;
          if (uVar4 != 5) goto LAB_00127e5c;
LAB_001280f8:
          iVar14 = (int)uVar15;
          if (iVar14 == 0xe) {
            iVar9 = 3;
            goto LAB_00127e6c;
          }
          iVar7 = execute_arm_condition(param_1,uVar15);
          iVar5 = *(int *)(param_1 + 0x210c);
          iVar9 = 3;
          if (iVar7 == 0) {
            iVar9 = 1;
          }
          iVar7 = iVar9 * 2;
          if (iVar5 == 1) {
            iVar7 = iVar9;
          }
          iVar6 = iVar6 + iVar7;
          if (uVar4 == 4) goto LAB_00128134;
LAB_00127e88:
          uVar12 = (uint)uVar8;
          if (uVar4 < 4) {
            if (uVar4 == 1) {
              if ((((uint)(uVar8 >> 0x17) & 3) != 2) || ((uVar12 >> 0x14 & 1) != 0)) {
LAB_00128060:
                if (3 < ((uint)(uVar8 >> 0x15) & 0xf) - 8) {
LAB_00127eb8:
                  if (((uint)(uVar8 >> 0xc) & 0xf) == 0xf) goto LAB_00128088;
                }
              }
            }
            else if ((uVar13 & 6) == 0) {
              if (uVar4 == 0) {
                if ((uVar12 & 0x90) == 0x90) {
                  if ((uVar8 & 0x60) != 0) goto LAB_00127eb4;
                }
                else {
                  if ((((uint)(uVar8 >> 0x17) & 3) != 2) || ((uVar12 >> 0x14 & 1) != 0))
                  goto LAB_00128060;
                  if ((uVar12 & 0x90) == 0x10) {
                    uVar4 = (uint)(uVar8 >> 5) & 3;
                    if (uVar4 == 1) {
                      if (iVar5 == 1) goto LAB_00128088;
                    }
                    else if (uVar4 == 3) {
                      if (iVar14 == 0xe && iVar5 == 1) goto LAB_00128088;
                    }
                    else if (((uVar8 >> 5 & 3) == 0) && ((uVar12 >> 0x16 & 1) == 0))
                    goto LAB_00128088;
                  }
                }
              }
            }
            else if (uVar4 != 4) {
              if (((uVar12 ^ 0xffffffff) & 0x2000010) == 0) {
                if ((*(uint *)(param_1 + 0x23c0) >> 5 & 1) != 0) {
                  uVar12 = uVar12 >> 0x10;
                  goto joined_r0x00128084;
                }
              }
              else {
LAB_00127eb4:
                if ((uVar12 >> 0x14 & 1) != 0) goto LAB_00127eb8;
              }
            }
            goto LAB_00127ec8;
          }
          if (uVar4 != 5) {
            if (uVar4 == 7) {
              uVar12 = uVar12 >> 0x18;
joined_r0x00128084:
              if ((uVar12 & 1) != 0) goto LAB_00128088;
            }
            goto LAB_00127ec8;
          }
LAB_00128088:
          *(uint *)(param_1 + 0x2290) = uVar16 - iVar6;
          iVar6 = execute_arm_instruction(param_1,uVar8 & 0xffffffff);
          uVar4 = *(uint *)(param_1 + 0x22a8);
          uVar16 = *(uint *)(param_1 + 0x2290);
          if (uVar4 != 0) {
            uVar12 = (uint)(iVar6 != 0);
            iVar6 = 0;
            goto LAB_00127ee8;
          }
          uVar12 = (uint)(iVar6 != 0) & uVar16 >> 0x1f;
          iVar6 = 0;
joined_r0x00127f28:
          if (uVar12 != 0) goto LAB_00127f40;
        }
      } while (*(int *)(param_1 + 0x2110) == 0);
    }
    uVar16 = 0xffffffff;
LAB_00127f40:
    *(uint *)(param_1 + 0x2290) = uVar16;
  }
  if (local_8 - __stack_chk_guard == 0) {
    return;
  }
                    // WARNING: Subroutine does not return
  //__stack_chk_fail();
}

// --- 函数实现: step_debug ---
static inline void step_debug(long *param_1,ulong param_2,undefined4 param_3)

{
  ushort *puVar1;
  int *piVar2;
  long lVar3;
  uint uVar4;
  ushort uVar5;
  ushort uVar6;
  ushort uVar7;
  int iVar8;
  uint uVar9;
  long lVar10;
  bool bVar11;
  undefined1 uVar12;
  bool bVar13;
  bool bVar14;
  undefined1 uVar15;
  undefined2 uVar16;
  int iVar17;
  int iVar18;
  uint uVar19;
  uint uVar20;
  undefined4 uVar21;
  char *pcVar22;
  size_t sVar23;
  long lVar24;
  ushort *puVar25;
  ushort **ppuVar26;
  long lVar27;
  undefined4 *puVar28;
  ulong uVar29;
  undefined8 uVar30;
  byte bVar31;
  uint uVar32;
  ulong uVar33;
  ushort uVar34;
  ushort uVar35;
  long lVar36;
  undefined1 *__s;
  char cVar37;
  int iVar38;
  int iVar39;
  char **ppcVar40;
  ulong uVar41;
  ulong uVar42;
  uint *puVar43;
  uint uVar44;
  long lVar45;
  uint uVar46;
  long lVar47;
  char *local_228;
  uint local_220;
  long local_1d0;
  char *local_1c8 [4];
  undefined *local_1a8;
  undefined *puStack_1a0;
  undefined *local_198;
  char *pcStack_190;
  char *local_188;
  char *local_180 [15];
  undefined8 local_108;
  undefined8 local_100;
  long local_8;
  
  lVar36 = param_1[0x24];
  local_8 = __stack_chk_guard;
  bVar31 = *(byte *)((long)param_1 + 0x131);
  *(int *)(param_1 + 0x24) = (int)param_2;
  *(int *)((long)param_1 + 0x124) = (int)lVar36;
  lVar36 = *param_1;
  if (bVar31 == 3) {
    if ((param_2 & 0xfffffffe) != param_1[0x22]) goto LAB_00183148;
    // __printf_chk 和 print_debug 未在提取列表中，暂时注释
    // __printf_chk(1,"breaking at %lx\n");
    *(undefined2 *)((long)param_1 + 0x131) = 0x300;
    // print_debug(param_1,param_3);
  }
LAB_00183148:
  return;
}

// --- 函数实现: load_memory32 ---
static inline ulong load_memory32(long param_1,ulong param_2)

{
  long lVar1;
  long lVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  ulong uVar6;
  uint *puVar7;
  ulong uVar8;
  ulong uVar9;
  long lVar10;
  long lVar11;
  ulong uVar12;
  long lVar13;
  uint uVar14;
  
  uVar6 = (param_2 & 0xffffffff) >> 0xb;
  uVar9 = *(ulong *)(param_1 + uVar6 * 8);
  if ((uVar9 & 0x3fffffffffffffff) == 0) {
    uVar14 = (uint)param_2;
    if (uVar14 < 0x10000000) {
      lVar11 = *(long *)(param_1 + 0x1000000);
      lVar10 = (ulong)(uVar14 >> 0x17) * 0x60;
      lVar1 = lVar11 + lVar10;
      cVar4 = *(char *)(lVar1 + 0x58);
      if (cVar4 == '\x01') {
        uVar3 = uVar14 & 0x7ff;
        puVar7 = (uint *)((uint *(*)(undefined8))**(code **)(lVar1 + 8))(*(undefined8 *)(nds_system + param_1 + 0xb04008));
        uVar5 = uVar14 - uVar3;
        lVar10 = param_1 + (ulong)(uVar5 >> 0x15) * 4;
        lVar1 = param_1 + (ulong)(ushort)(uVar5 >> 0x10) * 4;
        *(uint *)(nds_system + lVar10 + 0xb08018) =
             1 << (ulong)(uVar5 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb08018);
        *(uint *)(nds_system + lVar1 + 0xb04018) =
             1 << (ulong)(uVar5 >> 0xb & 0x1f) | *(uint *)(nds_system + lVar1 + 0xb04018);
        *(ulong *)(param_1 + (ulong)(uVar5 >> 0xb) * 8) =
             (long)((long)puVar7 + (-(ulong)(uVar14 - uVar3) - (ulong)uVar3)) >> 2 |
             0x4000000000000000;
        uVar6 = (ulong)*puVar7;
      }
      else {
        if (cVar4 == '\x02') {
                    // WARNING: Could not recover jumptable at 0x001197fc. Too many branches
                    // WARNING: Treating indirect jump as call
          uVar6 = ((uint (*)(undefined8, uint))**(code **)(lVar1 + 0x18))
                            (*(undefined8 *)(nds_system + param_1 + 0xb04008),
                             uVar14 & *(uint *)(lVar11 + lVar10));
          return uVar6;
        }
        if (cVar4 != '\0') {
          return 0;
        }
        lVar2 = param_1 + (ulong)(uVar14 >> 0x15) * 4;
        lVar13 = *(long *)(lVar1 + 8);
        uVar12 = (ulong)(uVar14 & *(uint *)(lVar11 + lVar10));
        lVar10 = param_1 + (param_2 >> 0x10 & 0xffff) * 4;
        cVar4 = *(char *)(lVar1 + 0x59);
        *(uint *)(nds_system + lVar2 + 0xb08018) =
             1 << (ulong)(uVar14 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar2 + 0xb08018);
        uVar8 = (long)((lVar13 + (uVar12 & 0xfffff800)) - (param_2 & 0xfffff800)) >> 2;
        *(uint *)(nds_system + lVar10 + 0xb04018) =
             1 << (ulong)((uint)uVar6 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb04018);
        uVar9 = uVar8 | 0x4000000000000000;
        if (cVar4 == '\0') {
          uVar9 = uVar8;
        }
        *(ulong *)(param_1 + uVar6 * 8) = uVar9;
        uVar6 = (ulong)*(uint *)(lVar13 + uVar12);
      }
    }
    else {
      uVar6 = 0xffffffff;
    }
  }
  return uVar6;
}

// --- 函数实现: load_memory16 ---
static inline ulong load_memory16(long param_1,ulong param_2)

{
  long lVar1;
  long lVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  ushort *puVar6;
  ulong uVar7;
  uint uVar8;
  ulong uVar9;
  long lVar10;
  ulong uVar11;
  long lVar12;
  ulong uVar13;
  long lVar14;
  
  uVar11 = param_2 >> 0xb & 0x1fffff;
  uVar9 = *(ulong *)(param_1 + uVar11 * 8);
  if ((uVar9 & 0x3fffffffffffffff) == 0) {
    uVar8 = (uint)param_2;
    if (uVar8 < 0x10000000) {
      lVar12 = *(long *)(param_1 + 0x1000000);
      lVar10 = (ulong)(uVar8 >> 0x17) * 0x60;
      lVar1 = lVar12 + lVar10;
      cVar4 = *(char *)(lVar1 + 0x58);
      if (cVar4 == '\x01') {
        uVar3 = uVar8 & 0x7ff;
        puVar6 = (ushort *)
                 ((uint (*)(undefined8))**(code **)(lVar1 + 8))(*(undefined8 *)(nds_system + param_1 + 0xb04008));
        uVar5 = uVar8 - uVar3;
        lVar10 = param_1 + (ulong)(uVar5 >> 0x15) * 4;
        lVar1 = param_1 + (ulong)(ushort)(uVar5 >> 0x10) * 4;
        *(uint *)(nds_system + lVar10 + 0xb08018) =
             1 << (ulong)(uVar5 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb08018);
        *(uint *)(nds_system + lVar1 + 0xb04018) =
             1 << (ulong)(uVar5 >> 0xb & 0x1f) | *(uint *)(nds_system + lVar1 + 0xb04018);
        *(ulong *)(param_1 + (ulong)(uVar5 >> 0xb) * 8) =
             (long)((long)puVar6 + (-(ulong)(uVar8 - uVar3) - (ulong)uVar3)) >> 2 |
             0x4000000000000000;
        uVar9 = (ulong)*puVar6;
      }
      else {
        if (cVar4 == '\x02') {
                    // WARNING: Could not recover jumptable at 0x00119630. Too many branches
                    // WARNING: Treating indirect jump as call
          uVar9 = ((uint (*)(undefined8, uint))**(code **)(lVar1 + 0x10))
                            (*(undefined8 *)(nds_system + param_1 + 0xb04008),
                             uVar8 & *(uint *)(lVar12 + lVar10));
          return uVar9;
        }
        if (cVar4 != '\0') {
          return 0;
        }
        lVar2 = param_1 + (ulong)(uVar8 >> 0x15) * 4;
        lVar14 = *(long *)(lVar1 + 8);
        uVar13 = (ulong)(uVar8 & *(uint *)(lVar12 + lVar10));
        lVar10 = param_1 + (param_2 >> 0x10 & 0xffff) * 4;
        cVar4 = *(char *)(lVar1 + 0x59);
        *(uint *)(nds_system + lVar2 + 0xb08018) =
             1 << (ulong)(uVar8 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar2 + 0xb08018);
        uVar7 = (long)((lVar14 + (uVar13 & 0xfffff800)) - (param_2 & 0xfffff800)) >> 2;
        *(uint *)(nds_system + lVar10 + 0xb04018) =
             1 << (ulong)((uint)uVar11 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb04018);
        uVar9 = uVar7 | 0x4000000000000000;
        if (cVar4 == '\0') {
          uVar9 = uVar7;
        }
        *(ulong *)(param_1 + uVar11 * 8) = uVar9;
        uVar9 = (ulong)*(ushort *)(lVar14 + uVar13);
      }
    }
    else {
      uVar9 = 0xffff;
    }
  }
  return uVar9;
}

// --- 函数实现: convert_thumb_instruction_to_arm ---
// 注意: 在 drastic_functions.h 中也有定义，这里注释掉整个实现以避免重定义
/*
static inline ulong convert_thumb_instruction_to_arm(ulong param_1,undefined4 *param_2)

{
  uint uVar1;
  long lVar2;
  uint uVar3;
  ulong uVar4;
  uint uVar5;
  uint uVar6;
  int local_50 [18];
  long local_8;
  
  lVar2 = __stack_chk_guard;
  local_8 = __stack_chk_guard;
  uVar4 = 0;
  *param_2 = 0;
  uVar6 = (uint)(param_1 >> 0xd) & 7;
  uVar3 = (uint)param_1;
  if (uVar6 == 4) {
    uVar6 = (uVar3 >> 0xb & 1) << 0x14;
    if ((uVar3 >> 0xc & 1) == 0) {
      uVar4 = param_1 >> 6 & 0x1f;
      uVar6 = (uVar3 & 7) << 0xc | ((uint)(param_1 >> 9) & 3) << 8 |
              (uVar3 >> 3 & 7) << 0x10 | uVar6 | ((uint)uVar4 & 7) << 1 | 0xe1c000b0;
    }
    else {
      uVar1 = (uVar3 & 0xff) << 2;
      uVar4 = (ulong)uVar1;
      uVar6 = (uVar3 >> 8 & 7) << 0xc | uVar6 | uVar1 | 0xe58d0000;
    }
    goto LAB_00129050;
  }
  if (uVar6 < 5) {
    uVar1 = (uint)(param_1 >> 8);
    if (uVar6 == 2) {
      uVar6 = uVar3 >> 10;
      if ((uVar3 >> 0xc & 1) != 0) {
        uVar5 = (uVar3 >> 3 & 7) << 0x10;
        uVar1 = (uint)(param_1 >> 6) & 7 | (uVar3 & 7) << 0xc;
        uVar4 = (ulong)uVar1;
        if ((uVar3 >> 9 & 1) == 0) {
          uVar6 = (uVar3 >> 0xb & 1) << 0x14 | uVar1 | (uVar6 & 1) << 0x16 | uVar5 | 0xe7800000;
        }
        else {
          uVar4 = (ulong)uVar6 & 3;
          local_50[2] = 1;
          local_50[3] = 3;
          local_50[0] = 1;
          local_50[1] = 2;
          uVar6 = (uint)((uVar6 & 3) != 0) << 0x14 | local_50[uVar4] << 5 | uVar5 | uVar1 |
                  0xe1800090;
        }
        goto LAB_00129050;
      }
      if ((uVar6 & 6) != 0) {
        uVar4 = 0xe59f0000;
        *param_2 = 1;
        uVar6 = (uVar3 >> 8 & 7) << 0xc | (uVar3 & 0xff) << 2 | 0xe59f0000;
        goto LAB_00129050;
      }
      if ((uVar6 & 7) == 1) {
        uVar1 = uVar1 & 3;
        uVar5 = (uint)(param_1 >> 3) & 0xf;
        uVar6 = uVar3 >> 7;
        uVar4 = (ulong)(uVar5 | 0xe0800000);
        if (uVar1 == 2) {
          uVar6 = uVar5 | ((uVar6 & 1) << 3 | uVar3 & 7) << 0xc | 0xe1a00000;
        }
        else if (uVar1 == 3) {
          uVar6 = (uVar6 & 1) << 5 | uVar5 | 0xe12fff10;
        }
        else {
          uVar6 = (uVar6 & 1) << 3 | uVar3 & 7;
          if (uVar1 == 1) {
            uVar6 = uVar5 | uVar6 << 0x10 | 0xe1500000;
          }
          else {
            uVar6 = uVar6 << 0x10 | uVar6 << 0xc | uVar5 | 0xe0800000;
          }
        }
        goto LAB_00129050;
      }
      uVar6 = uVar3 >> 6 & 0xf;
      uVar1 = uVar3 & 7;
      uVar4 = (ulong)uVar1;
      uVar5 = (uint)(param_1 >> 3) & 7;
      if (uVar6 == 9) {
        uVar6 = uVar5 << 0x10 | uVar1 << 0xc | 0xe2700000;
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
          uVar6 = uVar1 | local_50[uVar3] << 5;
          uVar4 = (ulong)uVar6;
          uVar6 = uVar5 << 8 | uVar1 << 0xc | uVar6 | 0xe1b00010;
          goto LAB_00129050;
        }
        if ((uVar3 >> 9 & 1) == 0) {
          uVar3 = uVar6 - 2;
          if (uVar3 < 3) goto LAB_001294c8;
        }
        else if (uVar6 == 8) goto LAB_00129434;
      }
      else {
        if (uVar6 == 0xd) {
          uVar6 = uVar1 << 0x10 | 0xe0100090;
          uVar4 = (ulong)uVar6;
          uVar6 = uVar6 | uVar1 | uVar5 << 8;
          goto LAB_00129050;
        }
        if (uVar6 < 0xe) {
          if (uVar6 - 10 < 2) {
LAB_00129434:
            local_50[2] = 10;
            local_50[3] = 0xb;
            local_50[0] = 8;
            local_50[1] = 0;
            uVar6 = local_50[uVar6 - 8] << 0x15 | 0xe0100000U | uVar5 | uVar1 << 0x10;
            goto LAB_00129050;
          }
        }
        else if (uVar6 == 0xf) {
          uVar6 = uVar5 | uVar1 << 0xc | 0xe1f00000;
          goto LAB_00129050;
        }
      }
      local_50[4] = 0;
      local_50[5] = 5;
      local_50[6] = 6;
      local_50[7] = 0;
      local_50[0] = 0;
      local_50[1] = 1;
      local_50[2] = 0;
      local_50[3] = 0;
      uVar5 = uVar5 | uVar1 << 0xc;
      uVar4 = (ulong)uVar5;
      local_50[8] = 0;
      local_50[9] = 0;
      local_50[10] = 0;
      local_50[0xb] = 0;
      local_50[0xc] = 0xc;
      local_50[0xd] = 0;
      local_50[0xe] = 0xe;
      local_50[0xf] = 0;
      uVar6 = uVar1 << 0x10 | local_50[uVar6] << 0x15 | uVar5 | 0xe0100000;
      goto LAB_00129050;
    }
    if (uVar6 == 3) {
      uVar1 = (uint)(param_1 >> 6) & 0x1f;
      uVar6 = uVar1 << 2;
      if ((param_1 >> 0xc & 1) != 0) {
        uVar6 = uVar1;
      }
      uVar6 = (uVar3 & 7) << 0xc | uVar6;
      uVar4 = (ulong)uVar6;
      uVar6 = ((uint)(param_1 >> 3) & 7) << 0x10 | ((uint)(param_1 >> 0xb) & 1) << 0x14 | uVar6 |
              ((uint)(param_1 >> 0xc) & 1) << 0x16 | 0xe5800000;
      goto LAB_00129050;
    }
    if (uVar6 == 1) {
      uVar1 = uVar1 & 7;
      local_50[2] = 4;
      local_50[3] = 2;
      local_50[0] = 0xd;
      local_50[1] = 10;
      uVar6 = uVar1 << 0xc;
      uVar4 = (ulong)uVar6;
      if (local_50[param_1 >> 0xb & 3] == 0xd) {
        uVar6 = uVar6 | uVar3 & 0xff | 0xe3b00000;
      }
      else {
        uVar6 = uVar3 & 0xff | local_50[param_1 >> 0xb & 3] << 0x15 | uVar6 | uVar1 << 0x10 |
                0xe2100000;
      }
      goto LAB_00129050;
    }
  }
  else {
    if (uVar6 == 6) {
      if ((uVar3 >> 0xc & 1) == 0) {
        uVar4 = (ulong)(uVar3 & 0xff);
        uVar6 = (uVar3 >> 0xb & 1) << 0x14 | (uVar3 >> 8 & 7) << 0x10 | uVar3 & 0xff | 0xe8a00000;
      }
      else {
        uVar6 = uVar3 >> 8 & 0xf;
        if (uVar6 == 0xf) {
          uVar6 = (uVar3 & 0xff) << 0x10 | 0xef000000;
        }
        else {
          uVar6 = (int)(char)param_1 & 0xffffffU | uVar6 << 0x1c | 0xa000000;
        }
      }
      goto LAB_00129050;
    }
    if (uVar6 == 7) {
      if (((uint)(param_1 >> 0xb) & 3) - 1 < 3) {
        uVar6 = uVar3 << 5 | 0xe6000010;
      }
      else {
        uVar6 = (uint)((long)(param_1 << 0x35) >> 0x35) & 0xffffff | 0xea000000;
      }
      goto LAB_00129050;
    }
    if (uVar6 == 5) {
      if ((uVar3 >> 0xc & 1) == 0) {
        uVar4 = (ulong)(uVar3 & 0xff);
        uVar6 = (uVar3 >> 8 & 7) << 0xc | uVar3 & 0xff;
        if ((uVar3 >> 0xb & 1) == 0) {
          uVar4 = 1;
          *param_2 = 1;
          uVar6 = uVar6 | 0xe28f0f00;
        }
        else {
          uVar6 = uVar6 | 0xe28d0f00;
        }
      }
      else if ((uVar3 >> 8 & 0xf) == 0) {
        uVar4 = 0xe28ddf00;
        uVar6 = uVar3 & 0x7f | 0xe28ddf00;
        if ((param_1 & 0x80) != 0) {
          uVar6 = uVar3 & 0x7f | 0xe24ddf00;
        }
      }
      else {
        uVar6 = uVar3 & 0xff;
        if ((uVar3 >> 0xb & 1) == 0) {
          uVar3 = uVar6 | 0x4000;
          if ((param_1 & 0x100) == 0) {
            uVar3 = uVar6;
          }
          uVar6 = uVar3 | 0xe92d0000;
        }
        else {
          uVar3 = uVar6 | 0x8000;
          if ((param_1 & 0x100) == 0) {
            uVar3 = uVar6;
          }
          uVar6 = uVar3 | 0xe8bd0000;
        }
      }
      goto LAB_00129050;
    }
  }
  uVar6 = (uint)(param_1 >> 0xb) & 3;
  uVar4 = param_1 >> 3 & 7;
  if (uVar6 == 3) {
    uVar6 = 0x800000;
    if ((param_1 & 0x200) != 0) {
      uVar6 = 0x400000;
    }
    uVar3 = (uVar3 & 7) << 0xc | uVar3 >> 6 & 7 | uVar6 | (uint)uVar4 << 0x10;
    uVar4 = 0xe0100000;
    uVar6 = uVar3 | 0xe0100000;
    if ((param_1 & 0x400) != 0) {
      uVar6 = uVar3 | 0xe2100000;
    }
  }
  else {
    uVar6 = (uVar3 >> 6 & 0x1f) << 7 | uVar6 << 5 | (uVar3 & 7) << 0xc | (uint)uVar4 | 0xe1b00000;
  }
  // 注意: 函数体不完整，需要从源文件完整提取
  // 临时修复: 添加 LAB_00129050 标签和 return 语句
  LAB_00129050:
  return (ulong)uVar6;
}
*/

// --- 函数实现: execute_arm_condition ---
static inline uint execute_arm_condition(long param_1,ulong param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = (uint)(param_2 >> 1) & 0x7fffffff;
  uVar2 = (uint)param_2;
  if (uVar3 == 4) {
    uVar1 = uVar2 & 1 ^
            *(uint *)(param_1 + 0x23c0) >> 0x1d & 1 &
            (*(uint *)(param_1 + 0x23c0) ^ 0x40000000) >> 0x1e;
  }
  else if (uVar3 < 5) {
    if (uVar3 == 2) {
      uVar1 = uVar2 & 1 ^ *(uint *)(param_1 + 0x23c0) >> 0x1f;
    }
    else if (uVar3 < 3) {
      if ((param_2 >> 1 & 0x7fffffff) == 0) {
        uVar1 = uVar2 & 1 ^ *(uint *)(param_1 + 0x23c0) >> 0x1e & 1;
      }
      else {
        if (uVar3 != 1) {
          return 0;
        }
        uVar1 = uVar2 & 1 ^ *(uint *)(param_1 + 0x23c0) >> 0x1d & 1;
      }
    }
    else {
      if (uVar3 != 3) {
        return 0;
      }
      uVar1 = uVar2 & 1 ^ *(uint *)(param_1 + 0x23c0) >> 0x1c & 1;
    }
  }
  else if (uVar3 == 6) {
    uVar3 = *(uint *)(param_1 + 0x23c0);
    uVar1 = uVar2 & 1 ^ (uint)((uVar3 & 0x40000000) == 0 && (uVar3 >> 0x1c & 1) == uVar3 >> 0x1f);
  }
  else {
    uVar1 = 1;
    if (uVar3 != 7) {
      if (uVar3 != 5) {
        return 0;
      }
      uVar1 = uVar2 & 1 ^
              (uint)((*(uint *)(param_1 + 0x23c0) >> 0x1c & 1) ==
                    -((int)*(uint *)(param_1 + 0x23c0) >> 0x1f));
    }
  }
  return uVar1;
}

// WARNING: Type propagation algorithm not settling

void execute_arm_undefined_or_thumb_ext_op(long param_1,ulong param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  ulong uVar6;
  
  uVar1 = *(uint *)(param_1 + 0x23c0);
  if ((uVar1 >> 5 & 1) != 0) {
    uVar6 = param_2 >> 5 & 0x7ffffff;
    if (((uint)param_2 >> 0x10 & 1) != 0) {
      uVar5 = ((uint)uVar6 & 0x7ff) * 2 + *(int *)(param_1 + 0x23a8);
      if (((uint)param_2 >> 0x11 & 1) == 0) {
        uVar5 = uVar5 & 0xfffffffc;
        *(uint *)(param_1 + 0x23c0) = uVar1 & 0xffffffdf;
      }
      uVar1 = *(uint *)(param_1 + 0x23bc);
      *(uint *)(param_1 + 0x23bc) = uVar5;
      *(uint *)(param_1 + 0x23a8) = uVar1 | 1;
      return;
    }
    *(int *)(param_1 + 0x23a8) =
         *(int *)(param_1 + 0x23ac) + (int)((long)(uVar6 << 0x35) >> 0x35) * 0x1000;
    return;
  }
  uVar3 = *(uint *)(param_1 + 0x23bc);
  uVar2 = *(uint *)(param_1 + 0x2104);
  uVar5 = uVar3 & 1;
  if ((uVar3 & 1) == 0) {
    if (uVar2 == 5) {
      *(uint *)(param_1 + 0x23a8) = uVar3;
    }
    else {
LAB_0012726c:
      *(undefined8 *)(param_1 + (ulong)uVar2 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar2 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20b8);
      }
      *(undefined4 *)(param_1 + 0x2104) = 5;
      *(uint *)(param_1 + 0x23a8) = uVar3;
      if (uVar5 != 0) goto LAB_001272a0;
    }
    *(uint *)(param_1 + 0x20fc) = uVar1;
  }
  else {
    uVar3 = uVar3 & 0xfffffffe;
    *(uint *)(param_1 + 0x23bc) = uVar3;
    if (uVar2 != 5) goto LAB_0012726c;
    *(uint *)(param_1 + 0x23a8) = uVar3;
LAB_001272a0:
    *(uint *)(param_1 + 0x20fc) = uVar1 | 0x20;
  }
  iVar4 = 4;
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar4 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 4;
  }
  *(int *)(param_1 + 0x23bc) = iVar4;
  *(uint *)(param_1 + 0x23c0) = uVar1 & 0xffffffc0 | 0x9b;
  return;
}
#endif // DRASTIC_LIBRETRO

ulong execute_arm_instruction(long param_1,ulong param_2)

{
  long lVar1;
  int iVar2;
  undefined4 uVar3;
  short sVar4;
  uint uVar5;
  undefined4 uVar6;
  uint uVar7;
  undefined4 uVar8;
  long lVar9;
  uint uVar10;
  int iVar11;
  ulong uVar12;
  uint uVar13;
  ulong uVar14;
  ulong uVar15;
  ulong uVar16;
  int iVar17;
  
  uVar12 = param_2 >> 0x1c & 0xf;
  uVar7 = (uint)uVar12;
  uVar16 = param_2 & 0xffffffff;
  uVar13 = (uint)param_2;
  if (uVar7 == 0xe) {
LAB_00127474:
    uVar10 = (uint)(uVar16 >> 0x19);
  }
  else {
    uVar10 = (uint)(param_2 >> 0x1d) & 7;
    if (uVar10 == 4) {
      uVar10 = *(uint *)(param_1 + 0x23c0) >> 0x1d & 1 &
               (*(uint *)(param_1 + 0x23c0) ^ 0x40000000) >> 0x1e;
    }
    else if (uVar10 < 5) {
      if (uVar10 == 2) {
        uVar10 = *(uint *)(param_1 + 0x23c0) >> 0x1f;
      }
      else if (uVar10 == 3) {
        uVar10 = *(uint *)(param_1 + 0x23c0) >> 0x1c & 1;
      }
      else {
        if (uVar10 != 1) goto LAB_0012778c;
        uVar10 = *(uint *)(param_1 + 0x23c0) >> 0x1d & 1;
      }
    }
    else if (uVar10 == 6) {
      uVar10 = *(uint *)(param_1 + 0x23c0);
      uVar10 = (uint)((uVar10 & 0x40000000) == 0 && (uVar10 >> 0x1c & 1) == uVar10 >> 0x1f);
    }
    else {
      if (uVar10 == 7) goto LAB_00127474;
      if (uVar10 == 5) {
        uVar10 = (uint)((*(uint *)(param_1 + 0x23c0) >> 0x1c & 1) ==
                       -((int)*(uint *)(param_1 + 0x23c0) >> 0x1f));
      }
      else {
LAB_0012778c:
        uVar10 = *(uint *)(param_1 + 0x23c0) >> 0x1e & 1;
      }
    }
    if ((uVar7 & 1) == uVar10) {
      return 0;
    }
    uVar10 = uVar13 >> 0x19;
  }
  uVar5 = uVar10 & 7;
  if (uVar5 == 5) {
    execute_arm_branch_op(param_1,uVar16);
    return 1;
  }
  if (uVar5 < 5) {
    if ((uVar10 >> 2 & 1) == 0) {
      if ((uVar10 & 6) != 0) {
        uVar14 = uVar16 >> 0x14 & 1;
        uVar15 = uVar16 >> 0x16 & 1;
        if ((uVar13 >> 0x19 & 1) == 0) {
          uVar12 = (ulong)(uVar13 & 0xfff);
        }
        else {
          if ((uVar13 >> 4 & 1) != 0) {
            execute_arm_undefined_or_thumb_ext_op(param_1,uVar16);
            return 1;
          }
          uVar13 = (uint)(uVar16 >> 5) & 3;
          uVar7 = *(uint *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
          uVar12 = uVar16 >> 7 & 0x1f;
          iVar17 = (int)uVar12;
          if (uVar13 == 2) {
            uVar13 = (int)uVar7 >> iVar17;
            if (iVar17 == 0) {
              uVar13 = (int)uVar7 >> 0x1f;
            }
            uVar12 = (ulong)uVar13;
          }
          else if (uVar13 == 3) {
            if (iVar17 == 0) {
              uVar12 = CONCAT44(*(uint *)(param_1 + 0x23c0) >> 0x1d,uVar7) >> 1 & 0xffffffff;
            }
            else {
              uVar12 = (ulong)(uVar7 >> iVar17 | uVar7 << 0x20 - iVar17);
            }
          }
          else if (uVar13 == 1) {
            uVar7 = uVar7 >> uVar12;
            if (iVar17 == 0) {
              uVar7 = 0;
            }
            uVar12 = (ulong)uVar7;
          }
          else {
            uVar12 = (ulong)(uVar7 << uVar12);
          }
        }
        execute_arm_memory_op(param_1,uVar16,uVar12,uVar14,uVar15,0,0,0);
        return 1;
      }
      if (uVar5 == 1) {
        uVar7 = (uint)(uVar16 >> 8);
        if ((((uint)(uVar16 >> 0x17) & 3) != 2) || ((param_2 & 0x100000) != 0)) {
          uVar7 = uVar7 & 0xf;
          iVar17 = uVar7 * 2;
          if (((param_2 & 0x100000) != 0 && (param_2 & 0xf00) != 0) &&
             (((uVar13 >> 0x15 & 0xe) == 0 || (0xb < (uVar13 >> 0x15 & 0xf))))) {
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
                 *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
                 (((uVar13 & 0xff) >> iVar17 | (uVar13 & 0xff) << uVar7 * -2 + 0x20) >>
                  (ulong)(iVar17 - 1U & 0x1f) & 1) << 0x1d;
          }
          execute_arm_alu_op(param_1,uVar16,0);
          return 1;
        }
        if (((uint)(uVar16 >> 0xc) & 0xf) == 0xf) {
          uVar7 = uVar7 & 0xf;
          execute_arm_msr_op(param_1,uVar16,
                             (uVar13 & 0xff) >> uVar7 * 2 | (uVar13 & 0xff) << uVar7 * -2 + 0x20);
          return 1;
        }
        goto LAB_00127778;
      }
    }
    else if (uVar5 == 4) {
      execute_arm_block_memory_op(param_1,uVar16);
      return 1;
    }
    if ((uVar13 & 0x90) == 0x90) {
      if ((param_2 & 0x60) != 0) {
        uVar5 = (uint)(uVar16 >> 0x14) & 1;
        uVar10 = (uVar5 ^ 1) & (uint)(uVar16 >> 6) & 1;
        uVar7 = uVar13 >> 5 & 1;
        if (uVar10 != 0) {
          uVar5 = uVar7 ^ 1;
        }
        if ((uVar13 >> 0x16 & 1) == 0) {
          uVar13 = *(uint *)(param_1 + ((ulong)(uVar13 & 0xf) + 0x8dc) * 4);
        }
        else {
          uVar13 = (uVar13 >> 8 & 0xf) << 4 | uVar13 & 0xf;
        }
        execute_arm_memory_op(param_1,uVar16,uVar13,uVar5,1,uVar7,uVar10,0);
        return 1;
      }
      if ((uVar13 >> 0x18 & 1) == 0) {
        uVar10 = *(uint *)(param_1 + ((uVar16 >> 8 & 0xf) + 0x8dc) * 4);
        uVar7 = *(uint *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
        uVar14 = uVar16 >> 0xc & 0xf;
        uVar12 = uVar16 >> 0x10 & 0xf;
        if ((uVar13 >> 0x17 & 1) == 0) {
          uVar7 = uVar7 * uVar10;
          if ((param_2 & 0x200000) != 0) {
            uVar7 = uVar7 + *(int *)(param_1 + (uVar14 + 0x8dc) * 4);
          }
          if ((uVar13 >> 0x14 & 1) != 0) {
            uVar13 = 0;
            if (uVar7 == 0) {
              uVar13 = 0x40000000;
            }
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0x3fffffff | uVar13 | uVar7 & 0x80000000;
          }
          *(uint *)(param_1 + (uVar12 + 0x8dc) * 4) = uVar7;
          return 1;
        }
        if ((uVar13 >> 0x16 & 1) == 0) {
          lVar9 = (ulong)uVar7 * (ulong)uVar10;
          if ((param_2 & 0x200000) != 0) {
            lVar9 = lVar9 + CONCAT44(*(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4),
                                     *(undefined4 *)(param_1 + (uVar14 + 0x8dc) * 4));
          }
          uVar7 = (uint)((ulong)lVar9 >> 0x20);
          if ((uVar13 >> 0x14 & 1) != 0) {
            uVar13 = 0;
            if (lVar9 == 0) {
              uVar13 = 0x40000000;
            }
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0x3fffffff | uVar13 | uVar7 & 0x80000000;
          }
          *(int *)(param_1 + (uVar14 + 0x8dc) * 4) = (int)lVar9;
          *(uint *)(param_1 + (uVar12 + 0x8dc) * 4) = uVar7;
          return 1;
        }
        lVar9 = (long)(int)uVar7 * (long)(int)uVar10;
        if ((param_2 & 0x200000) != 0) {
          lVar9 = CONCAT44(*(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4),
                           *(undefined4 *)(param_1 + (uVar14 + 0x8dc) * 4)) + lVar9;
        }
        uVar7 = (uint)((ulong)lVar9 >> 0x20);
        if ((uVar13 >> 0x14 & 1) != 0) {
          uVar13 = 0;
          if (lVar9 == 0) {
            uVar13 = 0x40000000;
          }
          *(uint *)(param_1 + 0x23c0) =
               *(uint *)(param_1 + 0x23c0) & 0x3fffffff | uVar13 | uVar7 & 0x80000000;
        }
        *(int *)(param_1 + (uVar14 + 0x8dc) * 4) = (int)lVar9;
        *(uint *)(param_1 + (uVar12 + 0x8dc) * 4) = uVar7;
        return 1;
      }
      uVar7 = (uint)(uVar16 >> 0x15) & 0xf;
      if (uVar7 == 8) {
        uVar6 = *(undefined4 *)(param_1 + ((uVar16 >> 0x10 & 0xf) + 0x8dc) * 4);
        uVar3 = *(undefined4 *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
        uVar8 = load_memory32(param_1 + 0x23d0,uVar6);
        *(undefined4 *)(param_1 + ((uVar16 >> 0xc & 0xf) + 0x8dc) * 4) = uVar8;
        store_memory32(param_1 + 0x23d0,uVar6,uVar3);
        return 1;
      }
      if (uVar7 == 10) {
        uVar6 = *(undefined4 *)(param_1 + ((uVar16 >> 0x10 & 0xf) + 0x8dc) * 4);
        uVar3 = *(undefined4 *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
        uVar7 = load_memory8(param_1 + 0x23d0,uVar6);
        *(uint *)(param_1 + ((uVar16 >> 0xc & 0xf) + 0x8dc) * 4) = uVar7 & 0xff;
        store_memory8(param_1 + 0x23d0,uVar6,uVar3);
        return 1;
      }
    }
    else {
      if ((((uint)(uVar16 >> 0x17) & 3) != 2) || ((uVar13 >> 0x14 & 1) != 0)) {
        uVar6 = execute_arm_alu_load_op2_reg(param_1,uVar16);
        execute_arm_alu_op(param_1,param_2 & 0xffffffff,uVar6);
        return 1;
      }
      if ((uVar13 >> 7 & 1) == 0) {
        if ((uVar13 >> 4 & 1) == 0) {
          if ((uVar13 >> 0x15 & 1) != 0) {
            execute_arm_msr_op(param_1,param_2 & 0xffffffff,
                               *(undefined4 *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4));
            return 1;
          }
          uVar12 = uVar16 >> 0xc & 0xf;
          if ((uVar13 >> 0x16 & 1) == 0) {
            *(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4) = *(undefined4 *)(param_1 + 0x23c0);
            return 1;
          }
          *(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4) =
               *(undefined4 *)(param_1 + (ulong)*(uint *)(param_1 + 0x2104) * 4 + 0x20e8);
          return 1;
        }
        uVar12 = uVar16 >> 5 & 3;
        iVar17 = (int)uVar12;
        if (iVar17 == 2) {
          if (*(int *)(param_1 + 0x210c) != 1) {
            return 1;
          }
          execute_arm_saturating_alu_op(param_1,param_2 & 0xffffffff);
          return 1;
        }
        if (iVar17 == 3) {
          if (*(int *)(param_1 + 0x210c) == 1 && uVar7 == 0xe) {
            execute_arm_raise_exception(param_1,3);
            return 1;
          }
          return 1;
        }
        if (iVar17 == 1) {
          if (*(int *)(param_1 + 0x210c) != 1) {
            return uVar12;
          }
        }
        else if ((uVar13 >> 0x16 & 1) != 0) {
          if (*(int *)(param_1 + 0x210c) != 1) {
            return 1;
          }
          iVar17 = *(int *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
          lVar9 = (uVar16 >> 0xc & 0xf) + 0x8dc;
          if (iVar17 == 0) {
            *(undefined4 *)(param_1 + lVar9 * 4) = 0x20;
            return 1;
          }
          *(int *)(param_1 + lVar9 * 4) = (int)LZCOUNT(iVar17);
          return 1;
        }
        uVar7 = *(uint *)(param_1 + 0x23c0);
        uVar10 = *(uint *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
        if ((uVar13 >> 5 & 1) != 0) {
          uVar13 = *(uint *)(param_1 + 0x23bc);
          if ((uVar7 & 0x20) != 0) {
            uVar13 = *(uint *)(param_1 + 0x23bc) | 1;
          }
          *(uint *)(param_1 + 0x23a8) = uVar13;
        }
        *(uint *)(param_1 + 0x23bc) = uVar10 & 0xfffffffe;
        *(uint *)(param_1 + 0x23c0) = uVar7 & 0xffffffc0 | uVar7 & 0x1f | (uVar10 & 1) << 5;
        return 1;
      }
      if (*(int *)(param_1 + 0x210c) == 1) {
        uVar10 = (uint)(uVar16 >> 0x15) & 3;
        iVar11 = *(int *)(param_1 + ((uVar16 >> 8 & 0xf) + 0x8dc) * 4);
        iVar2 = *(int *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
        uVar7 = uVar13 >> 0x10;
        iVar17 = iVar11 >> 0x10;
        sVar4 = (short)iVar11;
        if (uVar10 == 1) {
          iVar11 = (int)sVar4;
          if ((param_2 & 0x40) != 0) {
            iVar11 = iVar17;
          }
          iVar11 = (int)((ulong)((long)iVar2 * (long)iVar11) >> 0x10);
          iVar17 = iVar11;
          if (((uVar13 >> 5 & 1) == 0) &&
             (iVar2 = *(int *)(param_1 + ((uVar16 >> 0xc & 0xf) + 0x8dc) * 4),
             iVar17 = iVar2 + iVar11, (uint)(iVar17 < iVar11) != -(iVar2 >> 0x1f))) {
            *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
          }
          *(int *)(param_1 + ((long)(int)(uVar7 & 0xf) + 0x8dc) * 4) = iVar17;
          return 1;
        }
        if (uVar10 == 2) {
          iVar11 = (int)sVar4;
          if ((param_2 & 0x40) != 0) {
            iVar11 = iVar17;
          }
          lVar1 = param_1 + (uVar16 >> 0xc & 0xf) * 4;
          param_1 = param_1 + ((ulong)uVar7 & 0xf) * 4;
          lVar9 = CONCAT44(*(undefined4 *)(param_1 + 0x2370),*(undefined4 *)(lVar1 + 0x2370)) +
                  (long)((iVar2 / 2) * (iVar11 / 2));
          *(int *)(lVar1 + 0x2370) = (int)lVar9;
          *(int *)(param_1 + 0x2370) = (int)((ulong)lVar9 >> 0x20);
          return 1;
        }
        iVar11 = (int)(short)iVar2;
        if ((param_2 & 0x20) != 0) {
          iVar11 = iVar2 >> 0x10;
        }
        iVar2 = (int)sVar4;
        if ((param_2 & 0x40) != 0) {
          iVar2 = iVar17;
        }
        iVar11 = iVar11 * iVar2;
        iVar17 = iVar11;
        if (((uVar13 >> 0x16 & 1) == 0) &&
           (iVar2 = *(int *)(param_1 + ((uVar16 >> 0xc & 0xf) + 0x8dc) * 4), iVar17 = iVar11 + iVar2
           , (uint)(iVar17 < iVar11) != -(iVar2 >> 0x1f))) {
          *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
        }
        *(int *)(param_1 + ((long)(int)(uVar7 & 0xf) + 0x8dc) * 4) = iVar17;
        return 1;
      }
    }
  }
  else if (uVar5 == 6) {
    if (*(long *)(param_1 + 0x2250) != 0) {
      __printf_chk(1,"Game attempted LDC/STC instruction (%x) on P15. (pc %x, %lx in)\n",uVar16,
                   *(undefined4 *)(param_1 + 0x23bc),*(undefined8 *)(param_1 + 0x2220));
      return 1;
    }
  }
  else {
    if ((uVar13 >> 0x18 & 1) != 0) {
      execute_arm_raise_exception(param_1,2);
      return 1;
    }
    if (*(long *)(param_1 + 0x2250) != 0) {
      if ((uVar13 >> 4 & 1) == 0) {
        __printf_chk(1,"Game attempted CDP instruction (%x) on P15. (pc %x, %lx in)\n",uVar16,
                     *(undefined4 *)(param_1 + 0x23bc),*(undefined8 *)(param_1 + 0x2220));
        return 1;
      }
      execute_arm_coprocessor_register_transfer_op(param_1,uVar16);
      return 1;
    }
  }
LAB_00127778:
  execute_arm_raise_exception(param_1,0);
  return 1;
}

// --- 函数实现: execute_arm_branch_op ---

void execute_arm_branch_op(long param_1,ulong param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  long lVar5;
  int iVar6;
  
  iVar3 = *(int *)(param_1 + 0x23bc);
  iVar1 = (int)((long)(param_2 << 0x28) >> 0x28) + 1;
  if (((uint)(param_2 >> 0x1c) & 0xf) == 0xf) {
    iVar6 = iVar1 * 4 + ((uint)(param_2 >> 0x18) & 1) * 2;
    *(int *)(param_1 + 0x23a8) = iVar3;
    *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x20;
  }
  else {
    iVar6 = iVar1 * 4;
    if ((*(uint *)(param_1 + 0x23c0) & 0x20) != 0) {
      iVar6 = iVar1 * 2;
    }
    if (((uint)param_2 >> 0x18 & 1) != 0) {
      *(int *)(param_1 + 0x23a8) = iVar3;
    }
  }
  uVar4 = *(uint *)(param_1 + 0x2370);
  *(int *)(param_1 + 0x23bc) = iVar3 + iVar6;
  if (uVar4 + 0xfe000000 < 0x1000000) {
    uVar2 = uVar4 & 0x3fffff;
    lVar5 = **(long **)(param_1 + 0x2260);
    if ((((*(char *)(lVar5 + (ulong)uVar2) == 'h') && (*(char *)(lVar5 + (ulong)(uVar2 + 1)) == 't')
         ) && (*(char *)(lVar5 + (ulong)(uVar2 + 2)) == 't')) &&
       (*(char *)(lVar5 + (ulong)(uVar2 + 3)) == 'p')) {
      __printf_chk(1,"branch w/r0 %08x -> \'%s\': pc %08x, cpu %d\n",(ulong)uVar4,
                   lVar5 + ((ulong)uVar4 & 0x3fffff),iVar3 + iVar6,*(undefined4 *)(param_1 + 0x210c)
                  );
      return;
    }
  }
  return;
}
// --- 函数实现: execute_arm_memory_op ---
static inline void execute_arm_memory_op(long param_1,ulong param_2,int param_3,int param_4,int param_5,int param_6,
               int param_7,int param_8)

{
  uint uVar1;
  uint uVar2;
  ulong uVar3;
  int iVar4;
  ulong uVar5;
  bool bVar6;
  byte bVar7;
  ushort uVar8;
  uint uVar9;
  undefined8 uVar10;
  long lVar11;
  ulong uVar12;
  
  uVar3 = param_2 >> 0xc;
  if (((uint)(param_2 >> 0x1c) & 0xf) != 0xf) {
    lVar11 = param_1 + ((param_2 & 0xffffffff) >> 0x10 & 0xf) * 4;
    uVar1 = (uint)uVar3 & 0xf;
    uVar12 = (ulong)uVar1;
    uVar9 = *(uint *)(lVar11 + 0x2370);
    iVar4 = -param_3;
    if ((param_2 & 0x800000) != 0) {
      iVar4 = param_3;
    }
    uVar2 = uVar9 + iVar4;
    if (((uint)param_2 >> 0x18 & 1) == 0) {
      *(uint *)(lVar11 + 0x2370) = uVar2;
      uVar2 = uVar9;
    }
    else if (((uint)param_2 >> 0x15 & 1) != 0) {
      *(uint *)(lVar11 + 0x2370) = uVar2;
    }
    uVar5 = (ulong)uVar2;
    if (param_4 == 0) {
      if (param_7 == 0) {
        if (param_6 != 0) {
          store_memory16(param_1 + 0x23d0,uVar2 & 0xfffffffe,
                         *(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4));
          return;
        }
        if (param_5 != 0) {
          store_memory8(param_1 + 0x23d0,uVar2,*(undefined1 *)(param_1 + (uVar12 + 0x8dc) * 4));
          return;
        }
        store_memory32(param_1 + 0x23d0,uVar2 & 0xfffffffc,*(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4));
        return;
      }
      if ((uVar3 & 1) == 0) {
        // load_memory64 和 store_memory64 未在提取列表中，暂时注释
        // store_memory64(param_1 + 0x23d0,uVar5,
        //                CONCAT44(*(undefined4 *)(param_1 + ((ulong)(uVar1 + 1) + 0x8dc) * 4),
        //                         *(undefined4 *)(param_1 + (uVar12 + 0x8dc) * 4)));
        return;
      }
LAB_00126968:
      execute_arm_raise_exception(param_1,1);
      return;
    }
    if (param_7 == 0) {
      bVar6 = *(int *)(param_1 + 0x210c) != 1;
      if (param_6 == 0) {
        if (param_5 == 0) {
          if ((bVar6) || ((uVar2 & 0xfc000003) != 0)) {
            if ((uVar2 & 3) == 0) {
              uVar9 = load_memory32(param_1 + 0x23d0,*(undefined4 *)(param_1 + 0x23bc));
            }
            else {
              uVar9 = load_memory32(param_1 + 0x23d0,uVar2 & 0xfffffffc);
              uVar9 = uVar9 >> (uVar2 & 3) * 8 | uVar9 << (uVar2 & 3) * -8 + 0x20;
            }
          }
          else {
            uVar9 = *(uint *)(*(long *)(*(long *)(param_1 + 0x2260) + 0xfd4f0) + uVar5);
          }
        }
        else if ((bVar6) || ((uVar2 & 0xfc000000) != 0)) {
          bVar7 = load_memory8(param_1 + 0x23d0,uVar2);
          uVar9 = (int)(char)bVar7;
          if (param_8 == 0) {
            uVar9 = (uint)bVar7;
          }
        }
        else {
          lVar11 = *(long *)(*(long *)(param_1 + 0x2260) + 0xfd4f0);
          uVar9 = (int)*(char *)(lVar11 + uVar5);
          if (param_8 == 0) {
            uVar9 = (uint)*(byte *)(lVar11 + uVar5);
          }
        }
      }
      else if ((bVar6) || ((uVar2 & 0xfc000001) != 0)) {
        uVar8 = load_memory16(param_1 + 0x23d0,uVar2 & 0xfffffffe);
        uVar9 = (int)(short)uVar8;
        if (param_8 == 0) {
          uVar9 = (uint)uVar8;
        }
      }
      else {
        lVar11 = *(long *)(*(long *)(param_1 + 0x2260) + 0xfd4f0);
        uVar9 = (uint)*(short *)(lVar11 + uVar5);
        if (param_8 == 0) {
          uVar9 = (uint)*(ushort *)(lVar11 + uVar5);
        }
      }
      *(uint *)(param_1 + (uVar12 + 0x8dc) * 4) = uVar9;
    }
    else {
      if ((uVar3 & 1) != 0) goto LAB_00126968;
      // load_memory64 未在提取列表中，暂时注释
      // uVar10 = load_memory64(param_1 + 0x23d0,uVar5);
      // *(int *)(param_1 + (uVar12 + 0x8dc) * 4) = (int)uVar10;
      // *(int *)(param_1 + ((ulong)(uVar1 + 1) + 0x8dc) * 4) = (int)((ulong)uVar10 >> 0x20);
    }
    if (uVar1 == 0xf) {
      uVar1 = *(uint *)(param_1 + 0x23ac);
      *(uint *)(param_1 + 0x23bc) = uVar1;
      if (*(int *)(param_1 + 0x210c) == 1) {
        *(uint *)(param_1 + 0x23bc) = uVar1 & 0xfffffffe;
        *(uint *)(param_1 + 0x23c0) =
             *(uint *)(param_1 + 0x23c0) & 0xffffffc0 |
             *(uint *)(param_1 + 0x23c0) & 0x1f | (uVar1 & 1) << 5;
      }
    }
  }
  return;
}

// --- 函数实现: execute_arm_alu_op ---

void execute_arm_set_cpsr(long param_1,uint param_2)

{
  long lVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  ulong uVar5;
  
  *(uint *)(param_1 + 0x23c0) = param_2;
  uVar2 = (param_2 & 0x1f) - 0x10;
  uVar3 = *(uint *)(param_1 + 0x2104);
  if (uVar2 < 0x10) {
    uVar2 = *(uint *)(&CSWTCH_70 + (ulong)uVar2 * 4);
    uVar5 = (ulong)uVar2;
    if (uVar2 != uVar3) {
      if (uVar2 != 1) goto LAB_001256b4;
      *(undefined8 *)(param_1 + 0x20c8) = *(undefined8 *)(param_1 + 0x2390);
      *(undefined8 *)(param_1 + 0x20d0) = *(undefined8 *)(param_1 + 0x2398);
      *(undefined8 *)(param_1 + 0x20d8) = *(undefined8 *)(param_1 + 0x23a0);
      *(undefined4 *)(param_1 + 0x20e0) = *(undefined4 *)(param_1 + 0x23a8);
      if (uVar3 == 1) goto LAB_00125754;
LAB_001256d0:
      lVar1 = uVar5 * 8 + 0x2090;
      *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + lVar1);
      *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + lVar1 + 4);
      goto LAB_001256f0;
    }
  }
  else {
    uVar5 = 6;
    if (uVar3 != 6) {
LAB_001256b4:
      *(undefined8 *)(param_1 + (ulong)uVar3 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar3 != 1) goto LAB_001256d0;
LAB_00125754:
      *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
      *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x20e0);
      *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
      *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
LAB_001256f0:
      uVar3 = (uint)uVar5;
      *(uint *)(param_1 + 0x2104) = uVar3;
    }
  }
  if (((param_2 >> 7 & 1) != 0) || (*(int *)(param_1 + 0x2108) == 0)) {
    return;
  }
  uVar2 = *(uint *)(param_1 + 0x23bc);
  *(uint *)(param_1 + 0x22a8) = *(uint *)(param_1 + 0x22a8) | 8;
  if ((uVar2 & 1) == 0) {
    iVar4 = uVar2 + 4;
    if (uVar3 != 2) goto LAB_001257a0;
    *(int *)(param_1 + 0x23a8) = iVar4;
LAB_00125820:
    *(uint *)(param_1 + 0x20f0) = param_2;
  }
  else {
    *(uint *)(param_1 + 0x23bc) = uVar2 & 0xfffffffe;
    iVar4 = (uVar2 & 0xfffffffe) + 4;
    if (uVar3 == 2) {
      *(int *)(param_1 + 0x23a8) = iVar4;
    }
    else {
LAB_001257a0:
      *(undefined8 *)(param_1 + (ulong)uVar3 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar3 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20a0);
      }
      *(undefined4 *)(param_1 + 0x2104) = 2;
      *(int *)(param_1 + 0x23a8) = iVar4;
      if ((uVar2 & 1) == 0) goto LAB_00125820;
    }
    *(uint *)(param_1 + 0x20f0) = param_2 | 0x20;
  }
  iVar4 = 0x18;
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar4 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 0x18;
  }
  *(int *)(param_1 + 0x23bc) = iVar4;
  *(uint *)(param_1 + 0x23c0) = param_2 & 0xffffffc0 | 0x92;
  return;
}

void execute_arm_alu_op(long param_1,ulong param_2,uint param_3)

{
  uint uVar1;
  ulong uVar2;
  long lVar3;
  uint uVar4;
  uint uVar5;
  ulong uVar6;
  uint uVar7;
  ulong uVar8;
  
  uVar4 = (uint)(param_2 >> 0x15) & 0xf;
  uVar7 = *(uint *)(param_1 + ((param_2 >> 0x10 & 0xf) + 0x8dc) * 4);
  uVar8 = (ulong)uVar7;
  uVar2 = param_2 >> 0x14;
  if (uVar4 == 8) {
    uVar4 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
    if ((uVar7 & param_3) != 0) {
      uVar4 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
    }
    *(uint *)(param_1 + 0x23c0) = uVar7 & param_3 & 0x80000000 | uVar4 & 0x7fffffff;
    return;
  }
  if (8 < uVar4) {
    if (uVar4 == 0xc) {
      uVar4 = uVar7 | param_3;
    }
    else {
      if (uVar4 < 0xd) {
        if (uVar4 == 10) {
          if ((uVar2 & 1) == 0) {
            return;
          }
          uVar8 = (ulong)~param_3 + 1 + uVar8;
          uVar5 = (uint)uVar8;
          uVar4 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
          if (uVar5 != 0) {
            uVar4 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
          }
          uVar1 = uVar5 & 0x80000000 | uVar4 & 0x7fffffff | 0x20000000;
          if ((uVar8 & 0xffffffff00000000) == 0) {
            uVar1 = uVar5 & 0x80000000 | uVar4 & 0x5fffffff;
          }
          uVar4 = uVar1 | 0x10000000;
          if ((long)(int)~param_3 + (long)(int)uVar7 + 1 == (long)(int)uVar5) {
            uVar4 = uVar1 & 0xefffffff;
          }
          *(uint *)(param_1 + 0x23c0) = uVar4;
          return;
        }
        if (uVar4 == 0xb) {
          if ((uVar2 & 1) == 0) {
            return;
          }
          uVar4 = uVar7 + param_3;
          uVar5 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
          if (uVar4 != 0) {
            uVar5 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
          }
          uVar1 = uVar4 & 0x80000000 | uVar5 & 0x7fffffff | 0x20000000;
          if (param_3 + uVar8 == (ulong)uVar4) {
            uVar1 = uVar4 & 0x80000000 | uVar5 & 0x5fffffff;
          }
          uVar5 = uVar1 | 0x10000000;
          if ((long)(int)param_3 + (long)(int)uVar7 == (long)(int)uVar4) {
            uVar5 = uVar1 & 0xefffffff;
          }
          *(uint *)(param_1 + 0x23c0) = uVar5;
          return;
        }
        if (uVar4 == 9) {
          uVar4 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
          if (uVar7 != param_3) {
            uVar4 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
          }
          *(uint *)(param_1 + 0x23c0) = (uVar7 ^ param_3) & 0x80000000 | uVar4 & 0x7fffffff;
          return;
        }
      }
      else if (uVar4 == 0xe) {
        param_3 = param_3 ^ 0xffffffff;
      }
      else {
        if (uVar4 == 0xf) {
          uVar4 = ~param_3;
          goto joined_r0x00125d90;
        }
        if (uVar4 == 0xd) {
          uVar4 = param_3;
          if ((uVar2 & 1) != 0) {
            uVar7 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
            if (param_3 != 0) {
              uVar7 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
            }
            *(uint *)(param_1 + 0x23c0) = param_3 & 0x80000000 | uVar7 & 0x7fffffff;
            goto LAB_00125b04;
          }
          goto LAB_00125d10;
        }
      }
joined_r0x00125dfc:
      uVar4 = uVar7 & param_3;
    }
joined_r0x00125d90:
    if ((uVar2 & 1) == 0) {
LAB_00125d10:
      uVar8 = param_2 >> 0xc & 0xf;
      *(uint *)(param_1 + (uVar8 + 0x8dc) * 4) = uVar4;
      if ((int)uVar8 != 0xf) {
        return;
      }
      *(uint *)(param_1 + 0x23bc) = uVar4 & 0xfffffffe;
      return;
    }
    uVar7 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
    if (uVar4 != 0) {
      uVar7 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
    }
    *(uint *)(param_1 + 0x23c0) = uVar4 & 0x80000000 | uVar7 & 0x7fffffff;
    param_3 = uVar4;
    goto LAB_00125b04;
  }
  if (uVar4 == 4) {
    uVar4 = uVar7 + param_3;
    if ((uVar2 & 1) == 0) goto LAB_00125d10;
    uVar5 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
    if (uVar4 != 0) {
      uVar5 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
    }
    uVar1 = uVar4 & 0x80000000 | uVar5 & 0x7fffffff | 0x20000000;
    if (param_3 + uVar8 == (ulong)uVar4) {
      uVar1 = uVar4 & 0x80000000 | uVar5 & 0x5fffffff;
    }
    uVar5 = uVar1 | 0x10000000;
    if ((long)(int)param_3 + (long)(int)uVar7 == (long)(int)uVar4) {
      uVar5 = uVar1 & 0xefffffff;
    }
    *(uint *)(param_1 + 0x23c0) = uVar5;
    param_3 = uVar4;
    goto LAB_00125b04;
  }
  if (uVar4 < 5) {
    if (uVar4 != 2) {
      if (uVar4 != 3) {
        if (uVar4 == 1) {
          uVar4 = uVar7 ^ param_3;
          if ((uVar2 & 1) != 0) {
            uVar5 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
            if (uVar7 != param_3) {
              uVar5 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
            }
            *(uint *)(param_1 + 0x23c0) = uVar4 & 0x80000000 | uVar5 & 0x7fffffff;
            param_3 = uVar4;
            goto LAB_00125b04;
          }
          goto LAB_00125d10;
        }
        goto joined_r0x00125dfc;
      }
      if ((uVar2 & 1) == 0) {
        uVar4 = param_3 - uVar7;
        goto LAB_00125d10;
      }
      uVar8 = (ulong)param_3 + 1 + (ulong)~uVar7;
      uVar4 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
      if ((uint)uVar8 != 0) {
        uVar4 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
      }
      uVar4 = (uint)uVar8 & 0x80000000 | uVar4 & 0x7fffffff;
      lVar3 = (long)(int)param_3 + (long)(int)~uVar7 + 1;
      goto LAB_00125bf0;
    }
    if ((uVar2 & 1) == 0) {
      uVar4 = uVar7 - param_3;
      goto LAB_00125d10;
    }
    uVar8 = (ulong)~param_3 + 1 + uVar8;
    uVar4 = *(uint *)(param_1 + 0x23c0) | 0x40000000;
    if ((uint)uVar8 != 0) {
      uVar4 = *(uint *)(param_1 + 0x23c0) & 0xbfffffff;
    }
    uVar5 = (uint)uVar8 & 0x80000000;
    lVar3 = (long)(int)~param_3 + (long)(int)uVar7 + 1;
    uVar7 = uVar5 | uVar4 & 0x7fffffff | 0x20000000;
    if ((uVar8 & 0xffffffff00000000) == 0) {
      uVar7 = uVar5 | uVar4 & 0x5fffffff;
    }
  }
  else {
    if (uVar4 != 6) {
      if (uVar4 == 7) {
        uVar4 = *(uint *)(param_1 + 0x23c0);
        uVar6 = (ulong)(uVar4 >> 0x1d) & 1;
        if ((uVar2 & 1) == 0) {
          uVar4 = ((int)uVar6 - uVar7) + (param_3 - 1);
          goto LAB_00125d10;
        }
        uVar7 = ~uVar7;
        uVar8 = uVar6 + param_3 + (ulong)uVar7;
        uVar5 = uVar4 | 0x40000000;
        if ((int)uVar8 != 0) {
          uVar5 = uVar4 & 0xbfffffff;
        }
      }
      else {
        if (uVar4 != 5) goto joined_r0x00125dfc;
        uVar4 = *(uint *)(param_1 + 0x23c0);
        uVar6 = (ulong)(uVar4 >> 0x1d) & 1;
        if ((uVar2 & 1) == 0) {
          uVar4 = param_3 + (int)uVar6 + uVar7;
          goto LAB_00125d10;
        }
        uVar8 = uVar6 + param_3 + uVar8;
        uVar5 = uVar4 | 0x40000000;
        if ((int)uVar8 != 0) {
          uVar5 = uVar4 & 0xbfffffff;
        }
      }
      uVar4 = (uint)uVar8 & 0x80000000 | uVar5 & 0x7fffffff;
      lVar3 = (long)(int)param_3 + (long)(int)uVar7 + uVar6;
LAB_00125bf0:
      uVar7 = uVar4 | 0x20000000;
      if ((uVar8 & 0xffffffff00000000) == 0) {
        uVar7 = uVar4 & 0xdfffffff;
      }
      uVar4 = uVar7 | 0x10000000;
      if (lVar3 == (int)(uint)uVar8) {
        uVar4 = uVar7 & 0xefffffff;
      }
      *(uint *)(param_1 + 0x23c0) = uVar4;
      param_3 = (uint)uVar8;
      goto LAB_00125b04;
    }
    uVar4 = *(uint *)(param_1 + 0x23c0);
    param_3 = ~param_3;
    uVar6 = (ulong)(uVar4 >> 0x1d) & 1;
    if ((uVar2 & 1) == 0) {
      uVar4 = param_3 + (int)uVar6 + uVar7;
      goto LAB_00125d10;
    }
    uVar8 = uVar6 + param_3 + uVar8;
    uVar5 = uVar4 | 0x40000000;
    if ((uint)uVar8 != 0) {
      uVar5 = uVar4 & 0xbfffffff;
    }
    uVar4 = (uint)uVar8 & 0x80000000;
    lVar3 = (long)(int)param_3 + (long)(int)uVar7 + uVar6;
    uVar7 = uVar4 | uVar5 & 0x7fffffff | 0x20000000;
    if ((uVar8 & 0xffffffff00000000) == 0) {
      uVar7 = uVar4 | uVar5 & 0x5fffffff;
    }
  }
  uVar4 = uVar7 | 0x10000000;
  if (lVar3 == (int)(uint)uVar8) {
    uVar4 = uVar7 & 0xefffffff;
  }
  *(uint *)(param_1 + 0x23c0) = uVar4;
  param_3 = (uint)uVar8;
LAB_00125b04:
  uVar8 = param_2 >> 0xc & 0xf;
  *(uint *)(param_1 + (uVar8 + 0x8dc) * 4) = param_3;
  if ((int)uVar8 != 0xf) {
    return;
  }
  *(uint *)(param_1 + 0x23bc) = param_3 & 0xfffffffe;
  if ((*(uint *)(param_1 + 0x23c0) & 0x1f) == 0x10) {
    return;
  }
  execute_arm_set_cpsr
            (param_1,*(undefined4 *)(param_1 + (ulong)*(uint *)(param_1 + 0x2104) * 4 + 0x20e8));
  return;
}


// --- 函数实现: execute_arm_msr_op ---

void execute_arm_msr_op(long param_1,ulong param_2,uint param_3)

{
  long lVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  ulong uVar5;
  
  uVar3 = *(uint *)(param_1 + 0x2104);
  param_3 = *(uint *)(psr_masks_11490 + (param_2 >> 0x10 & 0xf) * 4) & param_3;
  uVar2 = ~*(uint *)(psr_masks_11490 + (param_2 >> 0x10 & 0xf) * 4);
  if (((uint)param_2 >> 0x16 & 1) != 0) {
    param_1 = param_1 + (ulong)uVar3 * 4;
    *(uint *)(param_1 + 0x20e8) = uVar2 & *(uint *)(param_1 + 0x20e8) | param_3;
    return;
  }
  param_3 = uVar2 & *(uint *)(param_1 + 0x23c0) | param_3;
  *(uint *)(param_1 + 0x23c0) = param_3;
  uVar2 = (param_3 & 0x1f) - 0x10;
  if (uVar2 < 0x10) {
    uVar2 = *(uint *)(&CSWTCH_70 + (ulong)uVar2 * 4);
    uVar5 = (ulong)uVar2;
    if (uVar2 != uVar3) {
      if (uVar2 != 1) goto LAB_001265e4;
      *(undefined8 *)(param_1 + 0x20c8) = *(undefined8 *)(param_1 + 0x2390);
      *(undefined8 *)(param_1 + 0x20d0) = *(undefined8 *)(param_1 + 0x2398);
      *(undefined8 *)(param_1 + 0x20d8) = *(undefined8 *)(param_1 + 0x23a0);
      *(undefined4 *)(param_1 + 0x20e0) = *(undefined4 *)(param_1 + 0x23a8);
LAB_00126508:
      if (uVar3 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x20e0);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        lVar1 = uVar5 * 8 + 0x2090;
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + lVar1);
        *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + lVar1 + 4);
      }
      uVar3 = (uint)uVar5;
      *(uint *)(param_1 + 0x2104) = uVar3;
    }
  }
  else {
    uVar5 = 6;
    if (uVar3 != 6) {
LAB_001265e4:
      *(undefined8 *)(param_1 + (ulong)uVar3 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      goto LAB_00126508;
    }
  }
  if (((param_3 >> 7 & 1) != 0) || (*(int *)(param_1 + 0x2108) == 0)) {
    return;
  }
  uVar2 = *(uint *)(param_1 + 0x23bc);
  *(uint *)(param_1 + 0x22a8) = *(uint *)(param_1 + 0x22a8) | 8;
  if ((uVar2 & 1) == 0) {
    iVar4 = uVar2 + 4;
    if (uVar3 == 2) {
      *(int *)(param_1 + 0x23a8) = iVar4;
    }
    else {
LAB_00126568:
      *(undefined8 *)(param_1 + (ulong)uVar3 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar3 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20a0);
      }
      *(undefined4 *)(param_1 + 0x2104) = 2;
      *(int *)(param_1 + 0x23a8) = iVar4;
      if ((uVar2 & 1) != 0) goto LAB_0012659c;
    }
    *(uint *)(param_1 + 0x20f0) = param_3;
  }
  else {
    *(uint *)(param_1 + 0x23bc) = uVar2 & 0xfffffffe;
    iVar4 = (uVar2 & 0xfffffffe) + 4;
    if (uVar3 != 2) goto LAB_00126568;
    *(int *)(param_1 + 0x23a8) = iVar4;
LAB_0012659c:
    *(uint *)(param_1 + 0x20f0) = param_3 | 0x20;
  }
  iVar4 = 0x18;
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar4 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 0x18;
  }
  *(int *)(param_1 + 0x23bc) = iVar4;
  *(uint *)(param_1 + 0x23c0) = param_3 & 0xffffffc0 | 0x92;
  return;
}


void execute_arm_mrs_op(long param_1,ulong param_2)

{
  ulong uVar1;
  
  uVar1 = param_2 >> 0xc & 0xf;
  if (((uint)param_2 >> 0x16 & 1) != 0) {
    *(undefined4 *)(param_1 + (uVar1 + 0x8dc) * 4) =
         *(undefined4 *)(param_1 + (ulong)*(uint *)(param_1 + 0x2104) * 4 + 0x20e8);
    return;
  }
  *(undefined4 *)(param_1 + (uVar1 + 0x8dc) * 4) = *(undefined4 *)(param_1 + 0x23c0);
  return;
}


// --- 函数实现: execute_arm_saturating_alu_op ---


void execute_arm_saturating_alu_op(long param_1,ulong param_2)

{
  int iVar1;
  int iVar2;
  long lVar3;
  
  iVar1 = *(int *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
  iVar2 = *(int *)(param_1 + ((param_2 >> 0x10 & 0xf) + 0x8dc) * 4);
  if (((uint)param_2 >> 0x16 & 1) == 0) {
LAB_00126e78:
    lVar3 = (long)iVar2;
  }
  else {
    if (-1 < iVar2) {
      *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
      lVar3 = (long)iVar1 + -1;
      if ((param_2 & 0x200000) != 0) {
        lVar3 = (long)iVar1 + 1;
      }
      goto joined_r0x00126e98;
    }
    if (-0x40000001 < iVar2) {
      iVar2 = iVar2 * 2;
      goto LAB_00126e78;
    }
    lVar3 = -0x40000000;
    *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
  }
  if ((param_2 & 0x200000) != 0) {
    lVar3 = -lVar3;
  }
  lVar3 = iVar1 + lVar3;
joined_r0x00126e98:
  if (lVar3 < 0x80000000) {
    if (lVar3 < -0x80000000) {
      lVar3 = 0x80000000;
      *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
    }
  }
  else {
    lVar3 = 0x7fffffff;
    *(uint *)(param_1 + 0x23c0) = *(uint *)(param_1 + 0x23c0) | 0x8000000;
  }
  *(int *)(param_1 + ((param_2 >> 0xc & 0xf) + 0x8dc) * 4) = (int)lVar3;
  return;
}
// --- 函数实现: execute_arm_raise_exception ---

void execute_arm_raise_exception(long param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  ulong uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  
  uVar6 = *(uint *)(param_1 + 0x23bc);
  uVar1 = *(uint *)(param_1 + 0x2104);
  iVar5 = param_2 * 4;
  uVar2 = uVar6 & 1;
  if ((uVar6 & 1) == 0) {
    if (param_2 == 3) goto LAB_00125354;
    if (param_2 < 4) {
      if (param_2 == 1) {
        param_2 = 0;
        goto LAB_001252a4;
      }
      if (param_2 == 2) goto LAB_00125078;
      if (param_2 != 0) goto LAB_00125278;
      uVar2 = param_2;
      if (uVar1 != 3) goto LAB_00125180;
      uVar6 = 0x13;
      uVar3 = 3;
    }
    else {
      if (param_2 == 6) goto LAB_001252ec;
      if (param_2 == 7) {
        iVar4 = uVar6 + 4;
        if (uVar1 != 1) goto LAB_00125124;
        uVar6 = 0x11;
        uVar3 = 1;
        *(int *)(param_1 + 0x23a8) = iVar4;
      }
      else {
        if (param_2 == 4) goto LAB_001251c0;
LAB_00125278:
        uVar3 = (ulong)uVar1;
        uVar6 = *(uint *)(cpu_modes_cpsr + uVar3 * 4);
      }
    }
LAB_00125290:
    uVar2 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + uVar3 * 4 + 0x20e8) = uVar2;
  }
  else {
    *(uint *)(param_1 + 0x23bc) = uVar6 & 0xfffffffe;
    if (param_2 == 3) {
LAB_00125354:
      if (uVar1 != 4) {
        *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
        if (uVar1 == 1) {
          *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
          *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
          *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
        }
        else {
          *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20b0);
        }
        *(undefined4 *)(param_1 + 0x2104) = 4;
      }
      uVar3 = 4;
      uVar6 = 0x17;
      *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x23bc);
LAB_001250c0:
      if (uVar2 == 0) goto LAB_00125290;
    }
    else {
      if (3 < param_2) {
        if (param_2 == 6) {
LAB_001252ec:
          if (uVar1 != 2) {
            *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4)
            ;
            if (uVar1 == 1) {
              *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
              *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
              *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
            }
            else {
              *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20a0);
            }
            *(undefined4 *)(param_1 + 0x2104) = 2;
          }
          uVar6 = 0x12;
          uVar3 = 2;
          *(int *)(param_1 + 0x23a8) = *(int *)(param_1 + 0x23bc) + 4;
        }
        else if (param_2 == 7) {
          iVar4 = (uVar6 & 0xfffffffe) + 4;
          if (uVar1 == 1) {
            uVar6 = 0x11;
            uVar3 = 1;
            *(int *)(param_1 + 0x23a8) = iVar4;
            goto LAB_001250c4;
          }
LAB_00125124:
          *(undefined8 *)(param_1 + 0x20c8) = *(undefined8 *)(param_1 + 0x2390);
          *(undefined8 *)(param_1 + 0x20d0) = *(undefined8 *)(param_1 + 0x2398);
          *(undefined8 *)(param_1 + 0x20d8) = *(undefined8 *)(param_1 + 0x23a0);
          uVar3 = 1;
          *(undefined4 *)(param_1 + 0x20e0) = *(undefined4 *)(param_1 + 0x23a8);
          uVar6 = 0x11;
          *(undefined4 *)(param_1 + 0x2104) = 1;
          *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x2098);
          *(int *)(param_1 + 0x23a8) = iVar4;
        }
        else {
          if (param_2 != 4) goto LAB_0012533c;
LAB_001251c0:
          if (uVar1 != 4) {
            *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4)
            ;
            if (uVar1 == 1) {
              *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
              *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
              *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
            }
            else {
              *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20b0);
            }
            *(undefined4 *)(param_1 + 0x2104) = 4;
          }
          uVar6 = 0x17;
          uVar3 = 4;
          *(int *)(param_1 + 0x23a8) = *(int *)(param_1 + 0x23bc) + -4;
        }
        goto LAB_001250c0;
      }
      if (param_2 == 1) {
LAB_001252a4:
        if (uVar1 != 5) {
          *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
          if (uVar1 == 1) {
            *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
            *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
            *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
          }
          else {
            *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20b8);
          }
          *(undefined4 *)(param_1 + 0x2104) = 5;
        }
        uVar6 = 0x1b;
        uVar3 = 5;
        *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x23bc);
        uVar2 = param_2;
        goto LAB_001250c0;
      }
      if (param_2 == 2) {
LAB_00125078:
        if (uVar1 != 3) {
          *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
          if (uVar1 == 1) {
            *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
            *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
            *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
          }
          else {
            *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20a8);
          }
          *(undefined4 *)(param_1 + 0x2104) = 3;
        }
        uVar3 = 3;
        uVar6 = 0x13;
        *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x23bc);
        goto LAB_001250c0;
      }
      if (param_2 == 0) {
        if (uVar1 == 3) {
          uVar6 = 0x13;
          uVar3 = 3;
          goto LAB_001250c4;
        }
LAB_00125180:
        *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
        if (uVar1 == 1) {
          *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
          *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x20e0);
          *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
          *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
        }
        else {
          *(undefined8 *)(param_1 + 0x23a4) = *(undefined8 *)(param_1 + 0x20a8);
        }
        uVar6 = 0x13;
        uVar3 = 3;
        *(undefined4 *)(param_1 + 0x2104) = 3;
        goto LAB_001250c0;
      }
LAB_0012533c:
      uVar3 = (ulong)uVar1;
      uVar6 = *(uint *)(cpu_modes_cpsr + uVar3 * 4);
    }
LAB_001250c4:
    uVar2 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + uVar3 * 4 + 0x20e8) = uVar2 | 0x20;
  }
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar5 = iVar5 + *(int *)(*(long *)(param_1 + 0x2250) + 0x10);
  }
  *(int *)(param_1 + 0x23bc) = iVar5;
  *(uint *)(param_1 + 0x23c0) = (uVar2 & 0xffffffe0 | uVar6) & 0xffffffdf | 0x80;
  return;
}

// --- 函数实现: execute_arm_coprocessor_register_transfer_op ---

void coprocessor_register_store(long *param_1,int param_2,int param_3,int param_4,ulong param_5)

{
  bool bVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  ulong uVar6;
  ulong uVar7;
  
  if (param_2 == 7) {
    if ((param_3 == 0 && param_4 == 4) || (param_3 == 8 && param_4 == 2)) {
      *(undefined4 *)(*param_1 + 0x2110) = 1;
      return;
    }
  }
  else {
    uVar5 = (uint)param_5;
    if (param_2 == 9) {
      if (param_3 == 1) {
        if (param_4 == 0) {
          uVar4 = 0x200 << ((ulong)((uVar5 & 0xfffff03e) >> 1) & 0x1f);
          *(uint *)(param_1 + 3) = uVar5 & 0xfffff03e;
          *(uint *)(param_1 + 4) = uVar5 & 0xfffff000;
          *(uint *)((long)param_1 + 0x2c) = uVar4;
          if (uVar4 < 0x1000) {
            uVar4 = 0x1000;
            *(undefined4 *)((long)param_1 + 0x2c) = 0x1000;
          }
          remap_dtcm(param_1[1],uVar5 & 0xfffff000,uVar4);
          bVar1 = (uint)((int)param_1[4] + *(int *)((long)param_1 + 0x2c)) < 0x4000000;
          cVar2 = *(char *)((long)param_1 + 0x3c);
          *(bool *)((long)param_1 + 0x3c) = bVar1;
          if ((bool)cVar2 != bVar1) {
            __printf_chk(1,"DTCM in mapped memory status changed (to %d)\n");
            return;
          }
        }
        else if (param_4 == 1) {
          *(uint *)((long)param_1 + 0x1c) = uVar5 & 0x3e;
          uVar5 = 0x200 << (ulong)((uVar5 & 0x3e) >> 1);
          if (uVar5 < 0x1000) {
            uVar5 = 0x1000;
            *(undefined4 *)(param_1 + 7) = 0x1000;
          }
          else {
            *(uint *)(param_1 + 7) = uVar5;
          }
          remap_itcm(param_1[1],uVar5);
          return;
        }
      }
    }
    else if ((param_2 == 1) && (param_3 == 0 && param_4 == 0)) {
      uVar4 = *(uint *)((long)param_1 + 0x14);
      if (((uVar4 ^ uVar5) >> 0xd & 1) != 0) {
        __printf_chk(1,"Changing coprocessor exception vector offset to %x\n",
                     ((uint)(param_5 >> 0xd) & 1) * -0x10000);
        uVar4 = *(uint *)((long)param_1 + 0x14);
        param_5 = param_5 & 0xffffffff;
      }
      uVar3 = (uint)param_5 & 0xff085 | uVar4 & 0xfff00f7a;
      uVar7 = NEON_ushl(CONCAT44(uVar3,uVar3),0xffffffeffffffff0,4);
      uVar6 = NEON_ushl(CONCAT44(uVar3,uVar3),0xffffffedffffffee,4);
      uVar5 = (uint)param_5 & 0x2000;
      uVar4 = *(uint *)(param_1 + 2);
      if ((param_5 & 0x2000) != 0) {
        uVar5 = 0xffff0000;
      }
      *(uint *)(param_1 + 2) = uVar5;
      *(uint *)((long)param_1 + 0x14) = uVar3;
      *(ulong *)((long)param_1 + 0x24) = uVar7 & 0x100000001;
      param_1[6] = uVar6 & 0x100000001;
      if (uVar4 != uVar5) {
        printf("Changing exception vector offset from %08x to %08x\n");
        return;
      }
    }
  }
  return;
}
void execute_arm_coprocessor_register_transfer_op(long param_1,ulong param_2)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  ulong uVar6;
  ulong uVar7;
  
  uVar6 = param_2 & 0xffffffff;
  if ((param_2 & 0xe00000) == 0 && ((uint)(uVar6 >> 8) & 0xf) == 0xf) {
    uVar7 = uVar6 >> 0xc & 0xf;
    if (((uint)param_2 >> 0x14 & 1) != 0) {
      uVar2 = coprocessor_register_load(param_1,param_2,0,0);
      *(undefined4 *)(param_1 + (uVar7 + 0x8dc) * 4) = uVar2;
      return;
    }
    coprocessor_register_store((long *)(param_1 + 0x2250),uVar6 >> 0x10 & 0xf,(uint)param_2 & 0xf,
               uVar6 >> 5 & 7,*(undefined4 *)(param_1 + (uVar7 + 0x8dc) * 4));
    return;
  }
  uVar1 = *(uint *)(param_1 + 0x2104);
  uVar3 = *(uint *)(param_1 + 0x23bc);
  uVar4 = uVar3 & 1;
  if ((uVar3 & 1) == 0) {
    if (uVar1 == 5) {
      *(uint *)(param_1 + 0x23a8) = uVar3;
    }
    else {
LAB_00127138:
      *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar1 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20b8);
      }
      *(undefined4 *)(param_1 + 0x2104) = 5;
      *(uint *)(param_1 + 0x23a8) = uVar3;
      if (uVar4 != 0) goto LAB_0012716c;
    }
    uVar4 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + 0x20fc) = uVar4;
  }
  else {
    uVar3 = uVar3 & 0xfffffffe;
    *(uint *)(param_1 + 0x23bc) = uVar3;
    if (uVar1 != 5) goto LAB_00127138;
    *(uint *)(param_1 + 0x23a8) = uVar3;
LAB_0012716c:
    uVar4 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + 0x20fc) = uVar4 | 0x20;
  }
  iVar5 = 4;
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar5 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 4;
  }
  *(int *)(param_1 + 0x23bc) = iVar5;
  *(uint *)(param_1 + 0x23c0) = uVar4 & 0xffffffc0 | 0x9b;
  return;
}

// --- 函数实现: execute_arm_alu_load_op2_reg ---

ulong execute_arm_alu_load_op2_reg(long param_1,ulong param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  ulong uVar5;
  uint uVar6;
  
  uVar3 = *(uint *)(param_1 + ((param_2 & 0xf) + 0x8dc) * 4);
  uVar5 = (ulong)uVar3;
  uVar2 = (uint)(param_2 >> 5) & 3;
  uVar6 = (uint)(param_2 >> 0x14) & 1;
  if (((param_2 >> 0x15 & 0xe) != 0) && (((uint)(param_2 >> 0x15) & 0xf) < 0xc)) {
    uVar6 = 0;
  }
  if (((uint)param_2 >> 4 & 1) == 0) {
    param_2 = param_2 >> 7;
    uVar1 = (uint)param_2;
    uVar4 = uVar1 & 0x1f;
    if (uVar2 == 2) {
      if ((param_2 & 0x1f) != 0) {
        if (uVar6 != 0) {
          *(uint *)(param_1 + 0x23c0) =
               *(uint *)(param_1 + 0x23c0) & 0xdfffffff |
               (uVar3 >> (ulong)(uVar4 - 1 & 0x1f)) << 0x1d;
        }
        return (ulong)(uint)((int)uVar3 >> (uVar1 & 0x1f));
      }
LAB_00124f04:
      if (uVar6 != 0) {
        *(uint *)(param_1 + 0x23c0) =
             *(uint *)(param_1 + 0x23c0) & 0xdfffffff | (int)(uVar5 >> 0x1f) << 0x1d;
      }
      return (ulong)(uint)((int)uVar5 >> 0x1f);
    }
    if (uVar2 == 3) {
      if ((param_2 & 0x1f) != 0) {
        if (uVar6 != 0) {
          *(uint *)(param_1 + 0x23c0) =
               *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
               *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
               (uVar3 >> (ulong)(uVar4 - 1 & 0x1f) & 1) << 0x1d;
        }
        return (ulong)(uVar3 >> (uVar1 & 0x1f) | uVar3 << 0x20 - (uVar1 & 0x1f));
      }
      uVar2 = *(uint *)(param_1 + 0x23c0);
      if (uVar6 != 0) {
        *(uint *)(param_1 + 0x23c0) = uVar2 & 0xc0000000 | uVar2 & 0x1fffffff | (uVar3 & 1) << 0x1d;
      }
      return CONCAT44(uVar2 >> 0x1d,uVar3) >> 1 & 0xffffffff;
    }
    if (uVar2 == 1) {
      if ((param_2 & 0x1f) != 0) {
        if (uVar6 != 0) {
          *(uint *)(param_1 + 0x23c0) =
               *(uint *)(param_1 + 0x23c0) & 0xdfffffff |
               (uVar3 >> (ulong)(uVar4 - 1 & 0x1f)) << 0x1d;
        }
        return (ulong)(uVar3 >> (ulong)(uVar1 & 0x1f));
      }
      if (uVar6 == 0) {
        return 0;
      }
      uVar6 = *(uint *)(param_1 + 0x23c0) & 0xdfffffff | (uVar3 >> 0x1f) << 0x1d;
      goto LAB_00124f54;
    }
    if ((param_2 & 0x1f) != 0) {
      if (uVar6 != 0) {
        *(uint *)(param_1 + 0x23c0) =
             *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
             *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
             (uVar3 >> (ulong)(-uVar4 & 0x1f) & 1) << 0x1d;
      }
      return (ulong)(uVar3 << (ulong)(uVar1 & 0x1f));
    }
  }
  else {
    uVar4 = *(uint *)(param_1 + ((param_2 >> 8 & 0xf) + 0x8dc) * 4);
    uVar1 = uVar3 + 4;
    if (((uint)param_2 & 0xf) != 0xf) {
      uVar1 = uVar3;
    }
    uVar5 = (ulong)uVar1;
    uVar3 = uVar4 & 0xff;
    if (uVar2 == 2) {
      if (uVar3 != 0) {
        if ((uVar4 & 0xe0) == 0) {
          if (uVar6 != 0) {
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
                 *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
                 (uVar1 >> (ulong)(uVar3 - 1 & 0x1f) & 1) << 0x1d;
          }
          return (ulong)(uint)((int)uVar1 >> (uVar4 & 0x1f));
        }
        goto LAB_00124f04;
      }
    }
    else if (uVar2 == 3) {
      if (uVar3 != 0) {
        if ((uVar4 & 0x1f) != 0) {
          if (uVar6 != 0) {
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
                 *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
                 (uVar1 >> (ulong)(uVar3 - 1 & 0x1f) & 1) << 0x1d;
          }
          return (ulong)(uVar1 >> (uVar4 & 0x1f) | uVar1 << 0x20 - (uVar4 & 0x1f));
        }
        if (uVar6 != 0) {
          *(uint *)(param_1 + 0x23c0) =
               *(uint *)(param_1 + 0x23c0) & 0xdfffffff | (uVar1 >> 0x1f) << 0x1d;
          return uVar5;
        }
      }
    }
    else if (uVar2 == 1) {
      if (uVar3 != 0) {
        if ((uVar4 & 0xe0) == 0) {
          if (uVar6 != 0) {
            *(uint *)(param_1 + 0x23c0) =
                 *(uint *)(param_1 + 0x23c0) & 0xdfffffff |
                 (uVar1 >> (ulong)(uVar3 - 1 & 0x1f)) << 0x1d;
          }
          return (ulong)(uVar1 >> (ulong)(uVar4 & 0x1f));
        }
        if (uVar6 == 0) {
          return 0;
        }
        uVar6 = *(uint *)(param_1 + 0x23c0) & 0xdfffffff;
        if (uVar3 == 0x20) {
          *(uint *)(param_1 + 0x23c0) = uVar6 | (uVar1 >> 0x1f) << 0x1d;
          return 0;
        }
LAB_00124f54:
        *(uint *)(param_1 + 0x23c0) = uVar6;
        return 0;
      }
    }
    else if (uVar3 != 0) {
      if ((uVar4 & 0xe0) != 0) {
        if (uVar6 == 0) {
          return 0;
        }
        uVar6 = *(uint *)(param_1 + 0x23c0) & 0xdfffffff;
        if (uVar3 == 0x20) {
          *(uint *)(param_1 + 0x23c0) = (uVar1 & 1) << 0x1d | uVar6;
          return 0;
        }
        goto LAB_00124f54;
      }
      if (uVar6 != 0) {
        *(uint *)(param_1 + 0x23c0) =
             *(uint *)(param_1 + 0x23c0) & 0xc0000000 |
             *(uint *)(param_1 + 0x23c0) & 0x1fffffff |
             (uVar1 >> (ulong)(-uVar3 & 0x1f) & 1) << 0x1d;
      }
      uVar5 = (ulong)(uVar1 << (ulong)(uVar4 & 0x1f));
    }
  }
  return uVar5;
}

// --- 函数实现: execute_arm_block_memory_op ---
static inline void execute_arm_block_memory_op(long param_1,ulong param_2)

{
  int iVar1;
  long lVar2;
  long lVar3;
  undefined4 uVar4;
  uint uVar5;
  ulong uVar6;
  uint uVar7;
  uint uVar8;
  ulong uVar9;
  uint uVar10;
  uint local_10;
  uint local_8;
  
  uVar5 = (uint)param_2;
  uVar6 = param_2 >> 0x10 & 0xf;
  uVar7 = uVar5 & 0xffff;
  lVar2 = param_1 + uVar6 * 4;
  iVar1 = (uint)(byte)(&bit_count)[uVar5 & 0xff] + (uint)(byte)(&bit_count)[param_2 >> 8 & 0xff];
  local_10 = (uint)(param_2 >> 0x15) & 1;
  uVar10 = (uint)(param_2 >> 0x17) & 3;
  uVar8 = *(uint *)(lVar2 + 0x2370);
  if (uVar10 == 2) {
    uVar8 = uVar8 + iVar1 * -4;
    local_8 = uVar8;
  }
  else if (uVar10 == 3) {
    local_8 = uVar8 + iVar1 * 4;
    uVar8 = uVar8 + 4;
  }
  else {
    local_8 = uVar8 + iVar1 * 4;
    if (uVar10 != 1) {
      local_8 = uVar8 + iVar1 * -4;
      uVar8 = local_8 + 4;
    }
  }
  uVar9 = 0xffffffff;
  if (((uVar5 >> 0x16 & 1) != 0) && ((uVar5 & 0x108000) != 0x108000)) {
    uVar10 = *(uint *)(param_1 + 0x2104);
    uVar9 = 0;
    if (uVar10 != 0) {
      *(undefined8 *)(param_1 + (ulong)uVar10 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar10 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x20e0);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined8 *)(param_1 + 0x23a4) = *(undefined8 *)(param_1 + 0x2090);
      }
      uVar9 = (ulong)uVar10;
      *(undefined4 *)(param_1 + 0x2104) = 0;
    }
  }
  uVar10 = 1 << uVar6;
  uVar8 = uVar8 & 0xfffffffc;
  if ((uVar7 & uVar10) == 0) {
    if ((param_2 & 0xffff) == 0) goto LAB_00126bc8;
    if ((uVar5 >> 0x14 & 1) != 0) goto LAB_00126c10;
  }
  else if ((uVar5 >> 0x14 & 1) != 0) {
    if (((uVar7 & (uVar10 ^ 0xffffffff)) != 0) && ((-uVar10 & uVar7) == uVar10)) {
      local_10 = 0;
    }
LAB_00126c10:
    uVar6 = 0;
    do {
      if ((uVar7 & 1) != 0) {
        uVar4 = load_memory32(param_1 + 0x23d0,uVar8);
        *(undefined4 *)(param_1 + 0x2370 + uVar6 * 4) = uVar4;
        uVar8 = uVar8 + 4;
      }
      uVar7 = uVar7 >> 1;
      uVar6 = (ulong)((int)uVar6 + 1);
    } while (uVar7 != 0);
    goto LAB_00126bc8;
  }
  uVar10 = uVar5 & 0x100000;
  do {
    if ((uVar7 & 1) != 0) {
      store_memory32(param_1 + 0x23d0,uVar8,*(undefined4 *)(param_1 + 0x2370 + (ulong)uVar10 * 4));
      uVar8 = uVar8 + 4;
    }
    uVar7 = uVar7 >> 1;
    uVar10 = uVar10 + 1;
  } while (uVar7 != 0);
LAB_00126bc8:
  uVar8 = (uint)uVar9;
  if ((uVar8 != 0xffffffff) && (uVar7 = *(uint *)(param_1 + 0x2104), uVar8 != uVar7)) {
    if (uVar8 == 1) {
      *(undefined8 *)(param_1 + 0x20c8) = *(undefined8 *)(param_1 + 0x2390);
      *(undefined8 *)(param_1 + 0x20d0) = *(undefined8 *)(param_1 + 0x2398);
      *(undefined8 *)(param_1 + 0x20d8) = *(undefined8 *)(param_1 + 0x23a0);
      *(undefined4 *)(param_1 + 0x20e0) = *(undefined4 *)(param_1 + 0x23a8);
    }
    else {
      *(undefined8 *)(param_1 + (ulong)uVar7 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
    }
    if (uVar7 == 1) {
      *(uint *)(param_1 + 0x2104) = uVar8;
      *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
      *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
      *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      *(undefined4 *)(param_1 + 0x23a8) = *(undefined4 *)(param_1 + 0x20e0);
    }
    else {
      lVar3 = uVar9 * 8 + 0x2090;
      *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + lVar3);
      uVar4 = *(undefined4 *)(param_1 + lVar3 + 4);
      *(uint *)(param_1 + 0x2104) = uVar8;
      *(undefined4 *)(param_1 + 0x23a8) = uVar4;
    }
  }
  if (local_10 != 0) {
    *(uint *)(lVar2 + 0x2370) = local_8;
  }
  if ((uVar5 & 0x108000) == 0x108000) {
    uVar8 = *(uint *)(param_1 + 0x23ac);
    *(uint *)(param_1 + 0x23bc) = uVar8;
    if (*(int *)(param_1 + 0x210c) == 1) {
      *(uint *)(param_1 + 0x23bc) = uVar8 & 0xfffffffe;
      *(uint *)(param_1 + 0x23c0) =
           *(uint *)(param_1 + 0x23c0) & 0xffffffc0 |
           *(uint *)(param_1 + 0x23c0) & 0x1f | (uVar8 & 1) << 5;
    }
    if (((param_2 & 0x400000) != 0) && ((*(uint *)(param_1 + 0x23c0) & 0x1f) != 0x10)) {
      // execute_arm_set_cpsr 函数未在提取列表中，暂时注释
      // execute_arm_set_cpsr(param_1,*(undefined4 *)(param_1 + (ulong)*(uint *)(param_1 + 0x2104) * 4 + 0x20e8));
      return;
    }
  }
  return;
}

// --- 函数实现: store_memory32 ---

void store_memory32(long param_1,ulong param_2,undefined4 param_3)

{
  ushort uVar1;
  long lVar2;
  char cVar3;
  ulong uVar4;
  long lVar5;
  undefined4 *puVar6;
  ulong uVar7;
  long lVar8;
  long lVar9;
  uint uVar10;
  
  uVar4 = (param_2 & 0xffffffff) >> 0xb;
  uVar7 = *(ulong *)(param_1 + uVar4 * 8);
  if ((uVar7 >> 0x3e & 1) == 0) {
    *(undefined4 *)((param_2 & 0xffffffff) + uVar7 * 4) = param_3;
    return;
  }
  uVar10 = (uint)param_2;
  if (uVar10 < 0x10000000) {
    lVar8 = *(long *)(param_1 + 0x1000000);
    lVar5 = (ulong)(uVar10 >> 0x17) * 0x60;
    lVar2 = lVar8 + lVar5;
    cVar3 = *(char *)(lVar2 + 0x59);
    uVar1 = (ushort)(param_2 >> 0x10);
    if (cVar3 == '\x01') {
      puVar6 = (undefined4 *)
               ((undefined4 *(*)(undefined8))**(code **)(lVar2 + 0x20))(*(undefined8 *)(nds_system + param_1 + 0xb04008));
      if (*(char *)(lVar2 + 0x58) == *(char *)(lVar2 + 0x59)) {
        lVar5 = param_1 + (ulong)(uVar10 >> 0x15) * 4;
        lVar2 = param_1 + (ulong)uVar1 * 4;
        *(uint *)(nds_system + lVar5 + 0xb08018) =
             1 << (ulong)(uVar10 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar5 + 0xb08018);
        *(uint *)(nds_system + lVar2 + 0xb04018) =
             1 << (ulong)(uVar10 >> 0xb & 0x1f) | *(uint *)(nds_system + lVar2 + 0xb04018);
        *(long *)(param_1 + (ulong)(uVar10 >> 0xb) * 8) =
             (long)((long)puVar6 + (-(param_2 & 0xfffff800) - (param_2 & 0x7ff))) >> 2;
      }
      *puVar6 = param_3;
    }
    else {
      if (cVar3 == '\x02') {
                    // WARNING: Could not recover jumptable at 0x00119fdc. Too many branches
                    // WARNING: Treating indirect jump as call
        ((void (*)(undefined8,uint))**(code **)(lVar2 + 0x30))
                  (*(undefined8 *)(nds_system + param_1 + 0xb04008),
                   uVar10 & *(uint *)(lVar8 + lVar5));
        return;
      }
      if (cVar3 == '\0') {
        lVar9 = *(long *)(lVar2 + 0x20);
        uVar7 = (ulong)(uVar10 & *(uint *)(lVar8 + lVar5));
        if (*(char *)(lVar2 + 0x58) == '\0') {
          lVar5 = param_1 + (ulong)(uVar10 >> 0x15) * 4;
          lVar2 = param_1 + (ulong)uVar1 * 4;
          *(uint *)(nds_system + lVar5 + 0xb08018) =
               1 << (ulong)(uVar10 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar5 + 0xb08018);
          *(uint *)(nds_system + lVar2 + 0xb04018) =
               1 << (ulong)((uint)uVar4 & 0x1f) | *(uint *)(nds_system + lVar2 + 0xb04018);
          *(long *)(param_1 + uVar4 * 8) =
               (long)((lVar9 + (uVar7 & 0xfffff800)) - (param_2 & 0xfffff800)) >> 2;
        }
        *(undefined4 *)(lVar9 + uVar7) = param_3;
      }
    }
  }
  return;
}// --- 函数实现: store_memory16 ---

void store_memory16(long param_1,ulong param_2,undefined2 param_3)

{
  long lVar1;
  char cVar2;
  long lVar3;
  undefined2 *puVar4;
  uint uVar5;
  long lVar6;
  ulong uVar7;
  ulong uVar8;
  long lVar9;
  ulong uVar10;
  
  uVar8 = param_2 >> 0xb & 0x1fffff;
  uVar7 = *(ulong *)(param_1 + uVar8 * 8);
  uVar10 = param_2 & 0xffffffff;
  uVar5 = (uint)param_2;
  if ((uVar7 >> 0x3e & 1) == 0) {
    *(undefined2 *)(uVar7 * 4 + (param_2 & 0xffffffff)) = param_3;
    return;
  }
  if (uVar5 < 0x10000000) {
    lVar6 = *(long *)(param_1 + 0x1000000);
    lVar3 = (ulong)(uVar5 >> 0x17) * 0x60;
    lVar1 = lVar6 + lVar3;
    cVar2 = *(char *)(lVar1 + 0x59);
    if (cVar2 == '\x01') {
      puVar4 = (undefined2 *)
               ((undefined2 *(*)(undefined8))**(code **)(lVar1 + 0x20))(*(undefined8 *)(nds_system + param_1 + 0xb04008));
      if (*(char *)(lVar1 + 0x58) == *(char *)(lVar1 + 0x59)) {
        lVar3 = param_1 + (uVar10 >> 0x15) * 4;
        lVar1 = param_1 + (uVar10 >> 0x10) * 4;
        *(uint *)(nds_system + lVar3 + 0xb08018) =
             1 << (ulong)((uint)(uVar10 >> 0x10) & 0x1f) | *(uint *)(nds_system + lVar3 + 0xb08018);
        *(uint *)(nds_system + lVar1 + 0xb04018) =
             1 << (ulong)((uint)(uVar10 >> 0xb) & 0x1f) | *(uint *)(nds_system + lVar1 + 0xb04018);
        *(long *)(param_1 + (uVar10 >> 0xb) * 8) =
             (long)((long)puVar4 + (-(param_2 & 0xfffff800) - (param_2 & 0x7ff))) >> 2;
      }
      *puVar4 = param_3;
    }
    else {
      if (cVar2 == '\x02') {
                    // WARNING: Could not recover jumptable at 0x00119e14. Too many branches
                    // WARNING: Treating indirect jump as call
        ((void (*)(undefined8,uint,undefined2))**(code **)(lVar1 + 0x28))
                  (*(undefined8 *)(nds_system + param_1 + 0xb04008),uVar5 & *(uint *)(lVar6 + lVar3)
                   ,param_3);
        return;
      }
      if (cVar2 == '\0') {
        lVar9 = *(long *)(lVar1 + 0x20);
        uVar7 = (ulong)(uVar5 & *(uint *)(lVar6 + lVar3));
        if (*(char *)(lVar1 + 0x58) == '\0') {
          lVar3 = param_1 + (ulong)(uVar5 >> 0x15) * 4;
          lVar1 = param_1 + (param_2 >> 0x10 & 0xffff) * 4;
          *(uint *)(nds_system + lVar3 + 0xb08018) =
               1 << (ulong)(uVar5 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar3 + 0xb08018);
          *(uint *)(nds_system + lVar1 + 0xb04018) =
               1 << (ulong)((uint)uVar8 & 0x1f) | *(uint *)(nds_system + lVar1 + 0xb04018);
          *(long *)(param_1 + uVar8 * 8) =
               (long)((lVar9 + (uVar7 & 0xfffff800)) - (param_2 & 0xfffff800)) >> 2;
        }
        *(undefined2 *)(lVar9 + uVar7) = param_3;
      }
    }
  }
  return;
}


// --- 函数实现: load_memory8 ---

ulong load_memory8(long param_1,ulong param_2)

{
  long lVar1;
  long lVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  byte *pbVar6;
  ulong uVar7;
  uint uVar8;
  ulong uVar9;
  long lVar10;
  ulong uVar11;
  long lVar12;
  ulong uVar13;
  long lVar14;
  
  uVar11 = param_2 >> 0xb & 0x1fffff;
  uVar9 = *(ulong *)(param_1 + uVar11 * 8);
  if ((uVar9 & 0x3fffffffffffffff) == 0) {
    uVar8 = (uint)param_2;
    if (uVar8 < 0x10000000) {
      lVar12 = *(long *)(param_1 + 0x1000000);
      lVar10 = (ulong)(uVar8 >> 0x17) * 0x60;
      lVar1 = lVar12 + lVar10;
      cVar4 = *(char *)(lVar1 + 0x58);
      if (cVar4 == '\x01') {
        uVar3 = uVar8 & 0x7ff;
        pbVar6 = (byte *)((byte *(*)(undefined8))**(code **)(lVar1 + 8))(*(undefined8 *)(nds_system + param_1 + 0xb04008));
        uVar5 = uVar8 - uVar3;
        lVar10 = param_1 + (ulong)(uVar5 >> 0x15) * 4;
        lVar1 = param_1 + (ulong)(ushort)(uVar5 >> 0x10) * 4;
        *(uint *)(nds_system + lVar10 + 0xb08018) =
             1 << (ulong)(uVar5 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb08018);
        *(uint *)(nds_system + lVar1 + 0xb04018) =
             1 << (ulong)(uVar5 >> 0xb & 0x1f) | *(uint *)(nds_system + lVar1 + 0xb04018);
        *(ulong *)(param_1 + (ulong)(uVar5 >> 0xb) * 8) =
             (long)(pbVar6 + (-(ulong)(uVar8 - uVar3) - (ulong)uVar3)) >> 2 | 0x4000000000000000;
        uVar9 = (ulong)*pbVar6;
      }
      else {
        if (cVar4 == '\x02') {
                    // WARNING: Could not recover jumptable at 0x00119460. Too many branches
                    // WARNING: Treating indirect jump as call
          uVar9 = ((uint (*)(undefined8,uint))**(code **)(lVar1 + 8))
                            (*(undefined8 *)(nds_system + param_1 + 0xb04008),
                             uVar8 & *(uint *)(lVar12 + lVar10));
          return uVar9;
        }
        if (cVar4 != '\0') {
          return 0;
        }
        lVar2 = param_1 + (ulong)(uVar8 >> 0x15) * 4;
        lVar14 = *(long *)(lVar1 + 8);
        uVar13 = (ulong)(uVar8 & *(uint *)(lVar12 + lVar10));
        lVar10 = param_1 + (param_2 >> 0x10 & 0xffff) * 4;
        cVar4 = *(char *)(lVar1 + 0x59);
        *(uint *)(nds_system + lVar2 + 0xb08018) =
             1 << (ulong)(uVar8 >> 0x10 & 0x1f) | *(uint *)(nds_system + lVar2 + 0xb08018);
        uVar7 = (long)((lVar14 + (uVar13 & 0xfffff800)) - (param_2 & 0xfffff800)) >> 2;
        *(uint *)(nds_system + lVar10 + 0xb04018) =
             1 << (ulong)((uint)uVar11 & 0x1f) | *(uint *)(nds_system + lVar10 + 0xb04018);
        uVar9 = uVar7 | 0x4000000000000000;
        if (cVar4 == '\0') {
          uVar9 = uVar7;
        }
        *(ulong *)(param_1 + uVar11 * 8) = uVar9;
        uVar9 = (ulong)*(byte *)(lVar14 + uVar13);
      }
    }
    else {
      uVar9 = 0xff;
    }
  }
  else {
    uVar9 = (ulong)*(byte *)(uVar9 * 4 + (param_2 & 0xffffffff));
  }
  return uVar9;
}

// --- 函数实现: store_memory8 ---
static inline void store_memory8(long param_1,ulong param_2,undefined1 param_3)

{
  long lVar1;
  char cVar2;
  long lVar3;
  undefined1 *puVar4;
  uint uVar5;
  long lVar6;
  ulong uVar7;
  ulong uVar8;
  long lVar9;
  ulong uVar10;
  
  uVar8 = param_2 >> 0xb & 0x1fffff;
  uVar7 = *(ulong *)(param_1 + uVar8 * 8);
  uVar10 = param_2 & 0xffffffff;
  uVar5 = (uint)param_2;
  if ((uVar7 >> 0x3e & 1) == 0) {
    *(undefined1 *)(uVar7 * 4 + (param_2 & 0xffffffff)) = param_3;
    return;
  }
}

#ifdef __cplusplus
}
#endif

#endif // DRASTIC_CPU_H
