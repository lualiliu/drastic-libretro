#ifndef SDL_h_
#define SDL_h_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 基本类型定义
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int64_t Sint64;

// SDL 结构体占位符
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Haptic SDL_Haptic;
typedef struct SDL_DisplayMode SDL_DisplayMode;
typedef union SDL_Event SDL_Event;

// SDL 常量
#define SDL_INIT_VIDEO 0x00000020
#define SDL_INIT_AUDIO 0x00000010
#define SDL_INIT_JOYSTICK 0x00000200
#define SDL_INIT_HAPTIC 0x00001000
#define SDL_INIT_GAMECONTROLLER 0x00002000
#define SDL_INIT_EVENTS 0x00004000
#define SDL_INIT_EVERYTHING 0x00000020

#define SDL_WINDOW_FULLSCREEN 0x00000001
#define SDL_WINDOW_SHOWN 0x00000004
#define SDL_WINDOW_RESIZABLE 0x00000020

#define SDL_TEXTUREACCESS_STATIC 0
#define SDL_TEXTUREACCESS_STREAMING 1

#define SDL_PIXELFORMAT_RGB565 0x15151002
#define SDL_PIXELFORMAT_RGBA8888 0x16462004

// 函数声明
int SDL_Init(Uint32 flags);
void SDL_Quit(void);
void SDL_Delay(Uint32 ms);
Uint32 SDL_GetTicks(void);
const char* SDL_GetError(void);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_SetWindowSize(SDL_Window* window, int w, int h);
int SDL_SetWindowFullscreen(SDL_Window* window, Uint32 flags);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const void* srcrect, const void* dstrect);
void SDL_RenderPresent(SDL_Renderer* renderer);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h);
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_UpdateTexture(SDL_Texture* texture, const void* rect, const void* pixels, int pitch);
int SDL_LockTexture(SDL_Texture* texture, const void* rect, void** pixels, int* pitch);
void SDL_UnlockTexture(SDL_Texture* texture);
int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode);

int SDL_PollEvent(SDL_Event* event);
int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode* mode);
int SDL_CaptureMouse(int enabled);
const char* SDL_GetKeyName(int key);
Uint16 SDL_GetModState(void);

// 音频
typedef struct SDL_AudioSpec {
    int freq;
    Uint16 format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    void (*callback)(void* userdata, Uint8* stream, int len);
    void* userdata;
} SDL_AudioSpec;

int SDL_OpenAudio(SDL_AudioSpec* desired, SDL_AudioSpec* obtained);
void SDL_CloseAudio(void);
void SDL_PauseAudio(int pause_on);

// 游戏手柄
int SDL_NumJoysticks(void);
SDL_Joystick* SDL_JoystickOpen(int device_index);
void SDL_JoystickClose(SDL_Joystick* joystick);
const char* SDL_JoystickName(SDL_Joystick* joystick);
int SDL_JoystickEventState(int state);

// 力反馈
int SDL_NumHaptics(void);
SDL_Haptic* SDL_HapticOpen(int device_index);
SDL_Haptic* SDL_HapticOpenFromJoystick(SDL_Joystick* joystick);
void SDL_HapticClose(SDL_Haptic* haptic);
int SDL_HapticRumbleSupported(SDL_Haptic* haptic);
int SDL_HapticRumbleInit(SDL_Haptic* haptic);
int SDL_HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 length);
void SDL_HapticRumbleStop(SDL_Haptic* haptic);

// 其他
void* SDL_memset(void* dst, int c, size_t len);
int SDL_SetHint(const char* name, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* SDL_h_ */
