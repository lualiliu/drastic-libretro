/*
 * spu.h
 * SPU (Sound Processing Unit) 音频处理单元头文件
 * 提供音频渲染相关的函数声明
 */

#ifndef SPU_H
#define SPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义（与 drastic.h 保持一致）
typedef unsigned long undefined8;

// 音频渲染函数
// param_1: SPU 状态结构指针
// param_2: 输出缓冲区（每个样本为 64-bit 累加值）
// param_3: 样本数量
extern void spu_render_samples(long param_1, undefined8 *param_2, int param_3);
// Capture/microphone render
extern void spu_render_capture(long param_1, undefined8 param_2, int param_3, unsigned param_4);

// Pre-update all channel settings to avoid in-loop updates
extern void spu_update_all_channel_settings(long param_1);

#ifdef __cplusplus
}
#endif

#endif /* SPU_H */

