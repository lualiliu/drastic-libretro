/*
 * sdl_wrapper.h
 * SDL2 包装层，将 SDL2 调用重定向到 libretro 接口
 * 用于将 drastic.cpp 集成到 libretro 核心
 * 
 * 重要说明：
 * - 此项目已完全移除对 SDL2 库的依赖
 * - 所有 SDL2 函数都是存根实现，不执行实际操作
 * - 实际的视频渲染通过 libretro 的 video_cb 回调完成
 * - 窗口管理和显示由 RetroArch 处理
 * - 不需要链接 SDL2 库，所有函数都在 sdl_wrapper.c 中实现
 */

#ifndef SDL_WRAPPER_H
#define SDL_WRAPPER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

// SDL2 类型定义（简化版，使用不透明指针）
typedef void SDL_Window;
typedef void SDL_Renderer;
typedef void SDL_Texture;
typedef struct SDL_Rect {
    int x, y, w, h;
} SDL_Rect;

// SDL2 常量定义
#define SDL_WINDOWPOS_UNDEFINED 0x1FFF0000
#define SDL_WINDOW_SHOWN 0x00000004
#define SDL_WINDOW_FULLSCREEN 0x00000001
#define SDL_RENDERER_ACCELERATED 0x00000002
#define SDL_RENDERER_PRESENTVSYNC 0x00000004
#define SDL_BLENDMODE_BLEND 1
#define SDL_PIXELFORMAT_RGB565 0x15151002
#define SDL_PIXELFORMAT_RGBA8888 0x16362004

// SDL2 函数声明（包装为 libretro 接口）
SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags);
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags);
SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode);
int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode);
int SDL_SetHint(const char* name, const char* value);
int SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
int SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
void SDL_RenderPresent(SDL_Renderer* renderer);
void SDL_ShowWindow(SDL_Window* window);
void SDL_RaiseWindow(SDL_Window* window);
int SDL_SetWindowFullscreen(SDL_Window* window, uint32_t flags);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
int SDL_GetCurrentDisplayMode(int displayIndex, void* mode);
int SDL_SetWindowSize(SDL_Window* window, int w, int h);
const char* SDL_GetError(void);
void SDL_Quit(void);
int SDL_Init(uint32_t flags);
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void* SDL_memset(void* dst, int c, size_t len);
int SDL_OpenAudio(void* desired, void* obtained);
void SDL_PauseAudio(int pause_on);
int SDL_PollEvent(void* event);
void SDL_Delay(uint32_t ms);
uint32_t SDL_GetTicks(void);
int SDL_HapticRumbleStop(void* haptic);
int SDL_HapticRumblePlay(void* haptic, float strength, uint32_t length);
char* SDL_JoystickName(int device_index);
void SDL_JoystickEventState(int state);
void* SDL_HapticOpenFromJoystick(void* joystick);
int SDL_HapticRumbleInit(void* haptic);
int SDL_HapticRumbleSupported(void* haptic);
int SDL_CaptureMouse(int enabled);
typedef struct SDL_DisplayMode SDL_DisplayMode;
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h);
int SDL_GetModState(void);
int SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch);
void SDL_UnlockTexture(SDL_Texture* texture);
int SDL_NumJoysticks(void);
int SDL_NumHaptics(void);
void* SDL_HapticOpen(int device_index);
void* SDL_JoystickOpen(int device_index);

// SDL2 类型定义
typedef struct SDL_AudioSpec SDL_AudioSpec;
typedef struct SDL_Event SDL_Event;
typedef void SDL_Haptic;

// 全局变量（在 libretro 中不使用，但 drastic.cpp 需要）
// 注意：这些变量在 drastic_val.h 中已经定义，这里只声明（不定义）
// 在 libretro 模式下，使用 drastic_val.h 中的定义
#ifdef DRASTIC_LIBRETRO
// 在 libretro 模式下，这些变量已经在 drastic_val.h 中定义
// 这里不需要重新声明
#else
extern SDL_Window* SDL_screen;
extern SDL_Renderer* DAT_04031578;
extern SDL_Window* DAT_04031570;
extern SDL_Texture* DAT_04031588;
extern SDL_Texture* DAT_04031590;
extern void* DAT_04031528[10];
extern int DAT_04031530;
extern int DAT_04031540;
extern int DAT_04031548;
extern int DAT_04031558;
extern int DAT_04031568;
extern int DAT_04031569;
extern int DAT_04031541;
extern int DAT_040315ec;
extern uint32_t DAT_040315a0;
extern uint32_t DAT_040315a4;
extern uint32_t DAT_040315a8;
extern uint32_t DAT_040315ac;
extern uint64_t DAT_040315c4;
extern uint32_t DAT_040315cc;
extern uint32_t DAT_040315d4;
#endif

#ifdef __cplusplus
}
#endif

#endif /* SDL_WRAPPER_H */

