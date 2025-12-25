/*
 * spu.c
 * SPU (Sound Processing Unit) 音频处理单元实现
 * 基于 drastic.cpp 的音频处理逻辑移植
 */

#include "spu.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ADPCM tables - 标准 NDS ADPCM 解码表 */
/* adpcm_step_table: 89 个元素的步长表 */
const int16_t adpcm_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

/* adpcm_index_step_table: 8 个元素的索引步长表 */
const int8_t adpcm_index_step_table[8] = {
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* 前向声明 */
static void spu_adpcm_decode_block(long param_1);
void spu_update_channel_settings(long param_1, long param_2);

/* spu_adpcm_decode_block: decode next ADPCM block for a channel (ported from drastic.cpp) */
static void spu_adpcm_decode_block(long param_1) {
    uint32_t uVar1 = *(uint32_t *)(param_1 + 0x90);
    long lVar3 = ((unsigned long)uVar1 & 0x3f) * 2;
    uint32_t uVar7 = *(uint32_t *)(*(long *)(param_1 + 0xa0) + (unsigned long)(uVar1 >> 1));
    int iVar8 = (int)*(int16_t *)(param_1 + 0xba);
    uint32_t uVar9 = *(uint8_t *)(param_1 + 0xc0);
    *(uint32_t *)(param_1 + 0x90) = uVar1 + 8;

    int16_t *puVar11 = (int16_t *)(param_1 + lVar3);
    int16_t *puVarEnd = (int16_t *)(param_1 + 0x10 + lVar3);

    while (puVar11 != puVarEnd) {
        int16_t sVar2 = adpcm_step_table[uVar9];
        uint32_t uVar5;
        uint32_t tmp = ((uint32_t)(int32_t)sVar2 >> 3) & 0x1fffffff;
        uVar1 = tmp + ((uint32_t)(int32_t)sVar2 >> 2);
        if ((uVar7 & 1) == 0) uVar1 = tmp;
        uVar5 = uVar1 + ((uint32_t)(int32_t)sVar2 >> 1);
        if ((uVar7 & 2) == 0) uVar5 = uVar1;
        uVar1 = uVar5 + (int32_t)sVar2;
        if ((uVar7 & 4) == 0) uVar1 = uVar5;

        if (((uint32_t)uVar7 >> 3 & 1) == 0) {
            iVar8 = iVar8 - (int)uVar1;
            if (iVar8 < -0x7fff) iVar8 = -0x7fff;
        } else {
            iVar8 = (int)uVar1 + iVar8;
            if (iVar8 > 0x7fff) iVar8 = 0x7fff;
        }

        uint32_t uVar6 = uVar7 & 7;
        uVar7 >>= 4;
        uVar9 = uVar9 + (int8_t)adpcm_index_step_table[uVar6];
        *puVar11++ = (int16_t)iVar8;
    }

    *(int16_t *)(param_1 + 0xba) = (int16_t)iVar8;
    *(uint8_t *)(param_1 + 0xc0) = (uint8_t)uVar9;
}

/* Update all channel settings for SPU at once (exposed helper) */
void spu_update_all_channel_settings(long param_1) {
    long ch;

    /* first channel group */
    for (ch = param_1 + 0x40028; ch != param_1 + 0x40348; ch += 200) {
        if (*(uint8_t *)(ch + 0xbd) != 0) {
            spu_update_channel_settings(param_1, ch);
        }
    }

    /* second channel group */
    for (ch = param_1 + 0x40348; ch != param_1 + 0x40ca8; ch += 200) {
        if (*(uint8_t *)(ch + 0xbd) != 0) {
            spu_update_channel_settings(param_1, ch);
        }
    }
}


/* spu_update_channel_settings: recalc channel volumes / stepping (ported from drastic.cpp) */
void spu_update_channel_settings(long param_1, long param_2) {
    if (!param_1 || !param_2) {
        return;
    }
    
    // 外部声明 nds_system 用于边界检查
    extern unsigned char nds_system[62042112];
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long system_end = system_base + 62042112;
    unsigned long param1_addr = (unsigned long)param_1;
    unsigned long param2_addr = (unsigned long)param_2;
    
    // 安全检查：确保 param_1 和 param_2 在有效范围内
    if (param1_addr < system_base || param1_addr >= system_end ||
        param2_addr < system_base || param2_addr >= system_end) {
        return;
    }
    
    if ((*(uint8_t *)(param_2 + 0xbd) >> 1 & 1) != 0) {
        // 安全检查：确保 param_2 + 0x98 在有效范围内
        if (param2_addr + 0x98 + 8 >= system_end) {
            return;
        }
        
        long *ctrl_ptr = (long *)(param_2 + 0x98);
        if (!ctrl_ptr || !*ctrl_ptr) {
            return;
        }
        
        unsigned long ctrl_addr = (unsigned long)*ctrl_ptr;
        if (ctrl_addr < system_base || ctrl_addr >= system_end) {
            return;
        }
        
        uint32_t uVar3 = **(uint32_t **)(param_2 + 0x98);
        uint32_t uVar7 = (uVar3 >> 8) & 3;
        uint32_t uVar5 = 4 - uVar7;
        uint32_t uVar1 = uVar3 & 0x7f;
        
        // 安全检查：确保 param_1 + 0x40ce8 在有效范围内
        if (param1_addr + 0x40ce8 + 8 >= system_end) {
            return;
        }
        
        long *master_ptr = (long *)(param_1 + 0x40ce8);
        if (!master_ptr || !*master_ptr) {
            return;
        }
        
        unsigned long master_addr = (unsigned long)*master_ptr;
        if (master_addr < system_base || master_addr + 0x100 + 4 >= system_end) {
            return;
        }
        
        uint32_t uVar2 = *(uint32_t *)(*master_ptr + 0x100) & 0x7f;
        if (uVar7 == 3) uVar5 = 0;
        if (uVar1 == 0x7f) uVar1 = 0x80;
        if (uVar2 == 0x7f) uVar2 = 0x80;
        uVar3 = (uVar3 >> 0x10) & 0x7f;
        int iVar4 = (int)(uVar1 * uVar2 << (uVar5 & 0x1f));
        *(int16_t *)(param_2 + 0xb4) = (int16_t)(((0x7f - uVar3) * iVar4) >> 0xd);
        *(int16_t *)(param_2 + 0xb6) = (int16_t)((uVar3 * iVar4) >> 0xd);
    }
    if ((*(uint8_t *)(param_2 + 0xbd) & 1) != 0) {
        // 安全检查：确保 param_2 + 0x98 在有效范围内
        if (param2_addr + 0x98 + 8 >= system_end) {
            return;
        }
        
        long *ctrl_ptr = (long *)(param_2 + 0x98);
        if (!ctrl_ptr || !*ctrl_ptr) {
            return;
        }
        
        unsigned long ctrl_addr = (unsigned long)*ctrl_ptr;
        if (ctrl_addr < system_base || ctrl_addr + 8 + 2 >= system_end) {
            return;
        }
        
        // 安全检查：确保 param_1 + 0x40010 在有效范围内
        if (param1_addr + 0x40010 + 4 >= system_end) {
            return;
        }
        
        uint64_t uVar8 = (uint64_t)((0x10000 - (uint32_t)*(uint16_t *)(*ctrl_ptr + 8)) *
                                    (uint32_t)*(int32_t *)(param_1 + 0x40010));
        uint64_t uVar6 = 0;
        if (uVar8 != 0) uVar6 = 0x1006f4300000000ULL / uVar8;
        
        // 安全检查：确保 param_2 + 0x88 在有效范围内
        if (param2_addr + 0x88 + 8 <= system_end) {
            *(uint64_t *)(param_2 + 0x88) = uVar6;
        }
        
        if (*(uint8_t *)(param_2 + 0xc2) != 0xff) {
            // 安全检查：确保计算的地址在有效范围内
            uint8_t idx = *(uint8_t *)(param_2 + 0xc2);
            unsigned long target_addr = param1_addr + ((unsigned long)idx + 0x2065ULL) * 0x20ULL + 0x10ULL;
            if (target_addr < system_end && target_addr + 8 <= system_end) {
                *(uint64_t *)(param_1 + ((uint64_t)idx + 0x2065ULL) * 0x20ULL + 0x10ULL) = uVar6;
            }
        }
    }
    
    // 安全检查：确保 param_2 + 0xbd 在有效范围内
    if (param2_addr + 0xbd + 1 <= system_end) {
        *(uint8_t *)(param_2 + 0xbd) = 0;
    }
}

// 音频渲染（已移植实现）
void spu_render_samples(long param_1, undefined8 *param_2, int param_3) {
    if (!param_1 || !param_2 || param_3 <= 0) {
        return;
    }

    /*
     * 基于 drastic.cpp 的实现进行移植：
     * - 遍历所有 SPU 通道（两组）
     * - 根据通道模式（8-bit/16-bit/ADPCM 等）获取样本
     * - 按左右声道音量混合到 param_2（每个样本为一个 64-bit 累加值，低32位为左/右之一，
     *   与 drastic.cpp 的 CONCAT44 行为一致）
     * - 在需要时调用 spu_adpcm_decode_block
     */

    // 外部声明 nds_system 用于边界检查
    extern unsigned char nds_system[62042112];
    unsigned long system_base = (unsigned long)nds_system;
    unsigned long system_end = system_base + 62042112;
    unsigned long param1_addr = (unsigned long)param_1;
    
    // 安全检查：确保 param_1 在有效范围内
    if (param1_addr < system_base || param1_addr >= system_end) {
        return;
    }

    uint64_t *out = (uint64_t *)param_2; /* 每个样本为 64-bit 累加器 */
    long chan_base = param_1 + 0x40028; /* 第一组通道起始 */
    long out_end_addr = (long)param_2 + (long)(param_3 - 1) * 8 + 0xc;
    long sample_write_offset = 0x3ffc;

    while (1) {
        // 安全检查：确保 chan_base 在有效范围内
        unsigned long chan_base_addr = (unsigned long)chan_base;
        if (chan_base_addr < system_base || chan_base_addr + 0xc4 >= system_end) {
            break;  // 通道地址无效，退出循环
        }
        
        if (*(char *)(chan_base + 0xbe) != 0) {
            uint32_t mode = (uint32_t)*(uint8_t *)(chan_base + 0xbc);
            if (*(char *)(chan_base + 0xbd) == 0) {
                int left_vol = (int)*(short *)(chan_base + 0xb4);
                int right_vol = (int)*(short *)(chan_base + 0xb6);
                uint32_t loop_limit = *(uint32_t *)(chan_base + 0xac);
                uint64_t pos = *(uint64_t *)(chan_base + 0x80);
                long step = *(long *)(chan_base + 0x88);
                long sample_tab = *(long *)(chan_base + 0xa0);
                
                // 安全检查：确保 sample_tab 有效
                if (!sample_tab) {
                    chan_base += 200;
                    continue;
                }
                
                unsigned long sample_tab_addr = (unsigned long)sample_tab;
                // sample_tab 可能指向 nds_system 内部，也可能指向外部内存（如 ROM 数据）
                // 我们只检查它是否是一个合理的地址（不为0且在合理范围内）
                if (sample_tab_addr == 0 || sample_tab_addr > 0x100000000ULL) {
                    chan_base += 200;
                    continue;
                }

                if (mode == 2) {
                    /* ADPCM 模式 */
                    if (param_3 != 0) {
                        long write_ptr = (long)param_2 + 4;
                        long cur_write = write_ptr;
                        long final_write = out_end_addr;
                        do {
                            uint32_t idx = (uint32_t)(pos >> 32);
                            if (*(uint32_t *)(chan_base + 0x90) <= idx) {
                                /* 需要解码更多的 ADPCM 数据 */
                                do {
                                    spu_adpcm_decode_block(chan_base);
                                    /* spu_adpcm_decode_block 会修改通道结构，重新读取 pos 等 */
                                    pos = *(uint64_t *)(chan_base + 0x80);
                                    step = *(long *)(chan_base + 0x88);
                                    /* idx 将在下一次循环根据 pos 更新 */
                                } while (*(uint32_t *)(chan_base + 0x90) <= (uint32_t)(pos >> 32));
                            }

                            pos += (uint64_t)step;
                            int16_t s = *(int16_t *)(chan_base + ((uint32_t)(pos >> 32) & 0x3f) * 2);

                            /* 将低 4bit 以特殊方式写入（保留原实现行为的一部分） */
                            *(short *)(cur_write + sample_write_offset) = (short)(((int)s & 0xfU) << 0xc);

                            /* 累加到 64-bit 输出（低32位累加 left, 高32位累加 right） */
                            {
                                int32_t low = (int32_t)(*(uint32_t *)cur_write);
                                int32_t high = (int32_t)(*(uint32_t *)(cur_write - 4));
                                low += left_vol * s;
                                high += right_vol * s;
                                *(uint32_t *)cur_write = (uint32_t)low;
                                *(uint32_t *)(cur_write - 4) = (uint32_t)high;
                            }

                            if (loop_limit <= (uint32_t)(pos >> 32)) {
                                uint32_t ctrl = **(uint32_t **)(chan_base + 0x98);
                                if (((ctrl >> 0x1b) & 1) == 0) {
                                    /* 禁用通道 */
                                    **(uint32_t **)(chan_base + 0x98) = ctrl & 0x7fffffff;
                                    *(char *)(chan_base + 0xbe) = 0;
                                    break;
                                }

                                uint32_t add = *(uint32_t *)(chan_base + 0xb0);
                                if (*(char *)(chan_base + 0xc1) == 0) {
                                    loop_limit = loop_limit + add;
                                    *(uint32_t *)(chan_base + 0xac) = loop_limit;
                                    *(uint16_t *)(chan_base + 0xb8) = *(uint16_t *)(chan_base + 0xba);
                                    *(uint8_t *)(chan_base + 0xbf) = *(uint8_t *)(chan_base + 0xc0);
                                    *(uint8_t *)(chan_base + 0xc1) = 1;
                                } else {
                                    pos = pos - ((uint64_t)add << 32);
                                    *(uint32_t *)(chan_base + 0x90) = *(int32_t *)(chan_base + 0x90) - (int32_t)add;
                                    *(uint16_t *)(chan_base + 0xba) = *(uint16_t *)(chan_base + 0xb8);
                                    *(uint8_t *)(chan_base + 0xc0) = *(uint8_t *)(chan_base + 0xbf);
                                }
                            }

                            cur_write += 8;
                        } while ((long)cur_write != final_write);
                    }
                }
                else if (mode == 0) {
                    /* 8-bit PCM (signed) */
                    if (param_3 != 0) {
                        uint64_t pos_local = pos;
                        for (int i = 0; i < param_3; i++) {
                            uint32_t idx = (uint32_t)(pos_local >> 32);
                            pos_local += (uint64_t)step;
                            
                            // 安全检查：确保 idx 在合理范围内（防止越界）
                            // 限制 idx 最大为 16MB（0x1000000），避免访问无效内存
                            if (idx > 0x1000000) {
                                break;  // idx 过大，跳过这个通道
                            }
                            
                            // 尝试访问 sample_tab + idx，如果失败则跳过
                            unsigned long sample_addr = sample_tab_addr + idx;
                            if (sample_addr < sample_tab_addr) {
                                break;  // 地址溢出
                            }
                            
                            int sample = (int8_t)*(char *)(sample_tab + idx) * 0x100;

                            /* 累加 */
                            uint64_t cur = out[i];
                            int32_t low = (int32_t)(cur & 0xffffffff) + left_vol * sample;
                            int32_t high = (int32_t)(cur >> 32) + right_vol * sample;
                            out[i] = ((uint64_t)(uint32_t)high << 32) | (uint32_t)low;

                            if (loop_limit <= (uint32_t)(pos_local >> 32)) {
                                uint32_t ctrl = **(uint32_t **)(chan_base + 0x98);
                                if (((ctrl >> 0x1b) & 1) == 0) {
                                    **(uint32_t **)(chan_base + 0x98) = ctrl & 0x7fffffff;
                                    *(char *)(chan_base + 0xbe) = 0;
                                    break;
                                }
                                pos_local = pos_local - ((uint64_t)*(uint32_t *)(chan_base + 0xb0) << 32);
                            }
                        }
                        pos = pos_local;
                    }
                }
                else {
                    /* modes 1,3,4: treat as 16-bit or signed 8-bit * 256 variants
                     * 这里以 16-bit 为主（mode 1/3）或 8-bit(4) 映射到相同的流程
                     */
                    if (param_3 != 0) {
                        uint64_t pos_local = pos;
                        for (int i = 0; i < param_3; i++) {
                            uint32_t idx = (uint32_t)(pos_local >> 32);
                            pos_local += (uint64_t)step;
                            
                            // 安全检查：确保 idx 在合理范围内
                            if (idx > 0x1000000) {
                                break;  // idx 过大，跳过这个通道
                            }
                            
                            int sample;
                            if (mode == 4) {
                                // 8-bit 模式
                                unsigned long sample_addr = sample_tab_addr + idx;
                                if (sample_addr < sample_tab_addr) {
                                    break;  // 地址溢出
                                }
                                sample = (int8_t)*(char *)(sample_tab + idx) * 0x100;
                            } else {
                                // 16-bit 模式
                                unsigned long sample_addr = sample_tab_addr + idx * 2;
                                if (sample_addr < sample_tab_addr || sample_addr + 2 < sample_addr) {
                                    break;  // 地址溢出或超出范围
                                }
                                sample = *(int16_t *)(sample_tab + idx * 2);
                            }

                            uint64_t cur = out[i];
                            int32_t low = (int32_t)(cur & 0xffffffff) + left_vol * sample;
                            int32_t high = (int32_t)(cur >> 32) + right_vol * sample;
                            out[i] = ((uint64_t)(uint32_t)high << 32) | (uint32_t)low;

                            if (loop_limit <= (uint32_t)(pos_local >> 32)) {
                                uint32_t ctrl = **(uint32_t **)(chan_base + 0x98);
                                if (((ctrl >> 0x1b) & 1) == 0) {
                                    **(uint32_t **)(chan_base + 0x98) = ctrl & 0x7fffffff;
                                    *(char *)(chan_base + 0xbe) = 0;
                                    break;
                                }
                                pos_local = pos_local - ((uint64_t)*(uint32_t *)(chan_base + 0xb0) << 32);
                            }
                        }
                        pos = pos_local;
                    }
                }

                *(uint64_t *)(chan_base + 0x80) = pos;
            }
            else {
                /* 如果需要，更新通道设置一次；为避免在本帧内进入无限循环，不会回到本通道再次处理 */
                spu_update_channel_settings(param_1, chan_base);
                /* 如果更新后仍然需要进一步处理，后续帧会处理该通道 */
            }
        }

        sample_write_offset += 2;
        chan_base += 200;
        if (sample_write_offset == 0x4004) break;
    }

    /* 第二组通道（起始偏移）: 处理方式与上面类似，映射到输出数组 param_2 */
    long ch2 = param_1 + 0x40348;
    uint64_t *out_end = out + param_3;
    while (ch2 != param_1 + 0x40ca8) {
        // 安全检查：确保 ch2 在有效范围内
        unsigned long ch2_addr = (unsigned long)ch2;
        if (ch2_addr < system_base || ch2_addr + 0xc4 >= system_end) {
            break;  // 通道地址无效，退出循环
        }
        
        if (*(char *)(ch2 + 0xbe) != 0) {
            if (*(char *)(ch2 + 0xbd) == 0) {
                int left_vol = (int)*(short *)(ch2 + 0xb4);
                int right_vol = (int)*(short *)(ch2 + 0xb6);
                uint64_t pos = *(uint64_t *)(ch2 + 0x80);
                long step = *(long *)(ch2 + 0x88);
                long sample_tab = *(long *)(ch2 + 0xa0);
                
                // 安全检查：确保 sample_tab 有效
                if (!sample_tab) {
                    ch2 += 200;
                    continue;
                }
                
                unsigned long sample_tab_addr = (unsigned long)sample_tab;
                if (sample_tab_addr == 0 || sample_tab_addr > 0x100000000ULL) {
                    ch2 += 200;
                    continue;
                }
                uint32_t loop_limit = *(uint32_t *)(ch2 + 0xac);

                if (param_3 != 0) {
                    for (int i = 0; i < param_3; i++) {
                        uint32_t idx = (uint32_t)(pos >> 32);
                        pos += (uint64_t)step;
                        
                        // 安全检查：确保 idx 在合理范围内
                        if (idx > 0x1000000) {
                            break;  // idx 过大，跳过这个通道
                        }
                        
                        int sample = 0;
                        uint32_t mode = (uint32_t)*(uint8_t *)(ch2 + 0xbc);
                        if (mode == 0 || mode == 4) {
                            // 8-bit 模式
                            unsigned long sample_addr = sample_tab_addr + idx;
                            if (sample_addr < sample_tab_addr) {
                                break;  // 地址溢出
                            }
                            sample = (int8_t)*(char *)(sample_tab + idx) * 0x100;
                        } else {
                            // 16-bit 模式
                            unsigned long sample_addr = sample_tab_addr + idx * 2;
                            if (sample_addr < sample_tab_addr || sample_addr + 2 < sample_addr) {
                                break;  // 地址溢出或超出范围
                            }
                            sample = *(int16_t *)(sample_tab + idx * 2);
                        }

                        uint64_t cur = out[i];
                        int32_t low = (int32_t)(cur & 0xffffffff) + left_vol * sample;
                        int32_t high = (int32_t)(cur >> 32) + right_vol * sample;
                        out[i] = ((uint64_t)(uint32_t)high << 32) | (uint32_t)low;

                        if (loop_limit <= (uint32_t)(pos >> 32)) {
                            uint32_t ctrl = **(uint32_t **)(ch2 + 0x98);
                            if (((ctrl >> 0x1b) & 1) == 0) {
                                **(uint32_t **)(ch2 + 0x98) = ctrl & 0x7fffffff;
                                *(char *)(ch2 + 0xbe) = 0;
                                break;
                            }
                            pos = pos - ((uint64_t)*(uint32_t *)(ch2 + 0xb0) << 32);
                        }
                    }
                }
                *(uint64_t *)(ch2 + 0x80) = pos;
            }
            else {
                spu_update_channel_settings(param_1, ch2);
            }
        }
        ch2 += 200;
    }
}

#ifdef __cplusplus
}
#endif

