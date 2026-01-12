/*
 * sdl_wrapper.c
 * SDL2 包装层实现，将 SDL2 调用重定向到 libretro 接口
 * 
 * 注意：此文件完全移除了对 SDL2 库的依赖
 * 所有 SDL2 函数都是存根实现，实际的渲染通过 libretro 的 video_cb 完成
 * 实际的显示由 RetroArch 管理，不需要 SDL2 窗口系统
 */

#include "sdl_wrapper.h"
#include "libretro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局变量定义（drastic.cpp 需要）
// 注意：在 libretro 模式下，这些变量在 drastic_val.h 中已经定义
// 这里不定义，避免重复定义错误

// libretro 回调（从 libretro.c 获取）
extern retro_video_refresh_t video_cb;
extern retro_environment_t environ_cb;

// SDL2 函数实现（存根/包装）
SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags) {
    (void)title; (void)x; (void)y; (void)w; (void)h; (void)flags;
    // 在 libretro 中，窗口由 RetroArch 管理
    // 返回一个非 NULL 指针以满足 drastic.cpp 的检查
    static char dummy_window[1];
    return (SDL_Window*)dummy_window;
}

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags) {
    (void)window; (void)index; (void)flags;
    // 在 libretro 中，渲染由 RetroArch 管理
    static char dummy_renderer[1];
    return (SDL_Renderer*)dummy_renderer;
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, uint32_t format, int access, int w, int h) {
    (void)renderer; (void)format; (void)access; (void)w; (void)h;
    // 在 libretro 中，纹理由 RetroArch 管理
    static char dummy_texture[1];
    return (SDL_Texture*)dummy_texture;
}

void SDL_DestroyWindow(SDL_Window* window) {
    (void)window;
    // libretro 中不需要销毁窗口
}

void SDL_DestroyRenderer(SDL_Renderer* renderer) {
    (void)renderer;
    // libretro 中不需要销毁渲染器
}

void SDL_DestroyTexture(SDL_Texture* texture) {
    (void)texture;
    // libretro 中不需要销毁纹理
}

int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode) {
    (void)renderer; (void)blendMode;
    return 0;
}

int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode) {
    (void)texture; (void)blendMode;
    return 0;
}

int SDL_SetHint(const char* name, const char* value) {
    (void)name; (void)value;
    return 0;
}

int SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch) {
    (void)texture; (void)rect; (void)pixels; (void)pitch;
    return 0;
}

int SDL_RenderClear(SDL_Renderer* renderer) {
    (void)renderer;
    return 0;
}

int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) {
    (void)renderer; (void)texture; (void)srcrect; (void)dstrect;
    return 0;
}

void SDL_RenderPresent(SDL_Renderer* renderer) {
    (void)renderer;
    // 在 libretro 中，渲染由 retro_run 中的 video_cb 处理
}

void SDL_ShowWindow(SDL_Window* window) {
    (void)window;
    // libretro 中窗口由 RetroArch 管理
}

void SDL_RaiseWindow(SDL_Window* window) {
    (void)window;
    // libretro 中窗口由 RetroArch 管理
}

int SDL_SetWindowFullscreen(SDL_Window* window, uint32_t flags) {
    (void)window; (void)flags;
    return 0;
}

int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h) {
    (void)renderer; (void)w; (void)h;
    return 0;
}

int SDL_GetCurrentDisplayMode(int displayIndex, void* mode) {
    (void)displayIndex; (void)mode;
    return 0;
}

int SDL_SetWindowSize(SDL_Window* window, int w, int h) {
    (void)window; (void)w; (void)h;
    return 0;
}

const char* SDL_GetError(void) {
    return "SDL wrapper (libretro mode)";
}

void SDL_Quit(void) {
    // libretro 中不需要退出 SDL
}

int SDL_Init(uint32_t flags) {
    (void)flags;
    return 0;  // 成功
}

int SDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)renderer; (void)r; (void)g; (void)b; (void)a;
    return 0;
}

void* SDL_memset(void* dst, int c, size_t len) {
    return memset(dst, c, len);
}

int SDL_OpenAudio(void* desired, void* obtained) {
    (void)desired; (void)obtained;
    // 在 libretro 中，音频由 RetroArch 管理
    return 0;  // 成功
}

void SDL_PauseAudio(int pause_on) {
    (void)pause_on;
    // 在 libretro 中，音频由 RetroArch 管理
}

int SDL_PollEvent(void* event) {
    (void)event;
    // 在 libretro 中，输入由 RetroArch 管理
    return 0;  // 没有事件
}

void SDL_Delay(uint32_t ms) {
    (void)ms;
    // 在 libretro 中，不需要延迟，RetroArch 会控制帧率
}

uint32_t SDL_GetTicks(void) {
    // 返回从启动以来的毫秒数（简化实现）
    static uint32_t ticks = 0;
    ticks += 16;  // 假设每帧16ms
    return ticks;
}

int SDL_HapticRumbleStop(void* haptic) {
    (void)haptic;
    return 0;
}

int SDL_HapticRumblePlay(void* haptic, float strength, uint32_t length) {
    (void)haptic; (void)strength; (void)length;
    return 0;
}

char* SDL_JoystickName(int device_index) {
    (void)device_index;
    return (char*)"Libretro Controller";
}

void SDL_JoystickEventState(int state) {
    (void)state;
    // libretro 中不需要
}

void* SDL_HapticOpenFromJoystick(void* joystick) {
    (void)joystick;
    return NULL;  // libretro 中不支持触觉反馈
}

int SDL_HapticRumbleInit(void* haptic) {
    (void)haptic;
    return 0;
}

int SDL_HapticRumbleSupported(void* haptic) {
    (void)haptic;
    return 0;  // 不支持
}

int SDL_CaptureMouse(int enabled) {
    (void)enabled;
    return 0;
}

void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h) {
    (void)renderer;
    if (w) *w = 512;  // 双屏宽度
    if (h) *h = 192;  // 单屏高度
}

int SDL_GetModState(void) {
    return 0;  // 无修饰键
}

// 在 libretro 模式下，SDL_LockTexture 返回一个临时缓冲区
// 这个缓冲区实际上不会被使用，因为渲染是通过 libretro 的 video_cb 完成的
// 但我们需要返回有效的指针，以避免 update_screen() 中的代码失败
static uint16_t dummy_texture_buffer[256 * 192 * 2];  // 足够大的缓冲区（双屏）

int SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch) {
    (void)texture; (void)rect;
    // 在 libretro 模式下，返回一个临时缓冲区
    // 实际的渲染数据已经通过 get_screen_ptr() 和 screen_copy16() 获取
    // 并通过 video_cb 发送到 RetroArch
    if (pixels) {
        *pixels = dummy_texture_buffer;
    }
    if (pitch) {
        // 默认 pitch：256 像素 * 2 字节（RGB565）
        *pitch = 256 * 2;
    }
    return 0;  // 成功
}

void SDL_UnlockTexture(SDL_Texture* texture) {
    (void)texture;
    // 在 libretro 模式下，不需要做任何事情
    // 数据已经通过 video_cb 发送
}

int SDL_NumJoysticks(void) {
    return 1;  // 假设有一个手柄
}

int SDL_NumHaptics(void) {
    return 0;  // 无触觉设备
}

void* SDL_HapticOpen(int device_index) {
    (void)device_index;
    return NULL;
}

void* SDL_JoystickOpen(int device_index) {
    (void)device_index;
    static char dummy_joystick[1];
    return dummy_joystick;
}

