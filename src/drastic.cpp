// 先包含标准库头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h> 
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>

#include <wchar.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>

// 包含自定义头文件
#include "drastic_val.h"
#include "drastic_functions.h"
#include "drastic_cpu.h"

// #region agent log
// Debug logging helper function
static void debug_log(const char* location, const char* message, const char* hypothesis_id, void* ptr_val, long long_val, int int_val) {
  FILE* log_file = fopen("/home/luali/github/drastic/.cursor/debug.log", "a");
  if (log_file) {
    fprintf(log_file, "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":{\"ptr\":\"%p\",\"long\":%lld,\"int\":%d},\"timestamp\":%ld}\n",
            hypothesis_id, location, message, ptr_val, (long long)long_val, int_val, time(NULL));
    fclose(log_file);
  }
}
// #endregion
// 注意：在 DRASTIC_LIBRETRO 模式下，使用 sdl_wrapper.h 而不是真正的 SDL2
// sdl_wrapper.h 已经通过 drastic_functions.h 包含，不需要包含 <SDL2/SDL.h>
// 包含标准库（在自定义头文件之后）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

// 定义nds_system为动态分配的全局变量（仅在非 libretro 模式下）
#ifndef DRASTIC_LIBRETRO
undefined1* nds_system = nullptr;
#endif

// 初始化 nds_ext 为包含 ".nds" 扩展名的字符串数组
// 注意：nds_ext 需要是一个 long* 类型的指针，但实际内容是指向字符串的指针数组
// 在64位系统上，指针是8字节，所以可以直接转换为 long*
static const char* nds_ext_array[] = { ".nds", NULL };
long* nds_ext = (long*)nds_ext_array;

// 实现 __strcpy_chk - 带边界检查的 strcpy
char* __strcpy_chk(char* dest, const char* src, size_t destlen) {
  if (dest == nullptr || src == nullptr || destlen == 0) {
    return dest;
  }
  size_t src_len = strlen(src);
  if (src_len >= destlen) {
    // 如果源字符串太长，截断并确保以null结尾
    strncpy(dest, src, destlen - 1);
    dest[destlen - 1] = '\0';
  } else {
    strcpy(dest, src);
  }
  return dest;
}

// 声明缺失的函数
extern "C" {
// printf 和 snprintf 已在标准库中定义，不需要重新声明
// int printf(int flag, const char *format, ...); // 注释掉，使用标准库版本
// int snprintf(char *str, int flag, size_t strlen, const char *format, ...); // 注释掉，使用标准库版本
void __stack_chk_fail();
int __longjmp_chk(void* env, int val);
int __xstat(int version, const char* path, struct stat* buf);
int __snprintf_chk(char* str, size_t maxlen, int flag, size_t strlen, const char* format, ...);
}


// 在 libretro 模式下，不应该有 main() 函数
// libretro 核心通过 retro_init(), retro_load_game(), retro_run() 等函数运行
#ifndef DRASTIC_LIBRETRO
int main(int param_1, char** param_2)

{
  // Increase stack size limit to prevent stack overflow
  // Default is usually 8MB, increase to 32MB for safety
  struct rlimit rl;
  if (getrlimit(RLIMIT_STACK, &rl) == 0) {
    rlim_t new_limit = 32 * 1024 * 1024; // 32MB
    fprintf(stderr, "Current stack limit: soft=%lu, hard=%lu\n", 
            (unsigned long)rl.rlim_cur, (unsigned long)rl.rlim_max);
    
    if (rl.rlim_cur < new_limit) {
      // Try to increase hard limit if it's too low
      if (rl.rlim_max < new_limit && rl.rlim_max != RLIM_INFINITY) {
        rl.rlim_max = new_limit;
      }
      rl.rlim_cur = new_limit;
      if (setrlimit(RLIMIT_STACK, &rl) != 0) {
        fprintf(stderr, "WARNING: Failed to increase stack size limit: %m\n");
        fprintf(stderr, "Trying with current hard limit: %lu\n", (unsigned long)rl.rlim_max);
        // Try with just the hard limit if it's less than our target
        if (rl.rlim_max > rl.rlim_cur) {
          rl.rlim_cur = rl.rlim_max;
          if (setrlimit(RLIMIT_STACK, &rl) == 0) {
            fprintf(stderr, "Stack size limit set to hard limit: %lu bytes\n", 
                    (unsigned long)rl.rlim_cur);
          }
        }
      } else {
        fprintf(stderr, "Stack size limit increased to %lu bytes\n", (unsigned long)rl.rlim_cur);
      }
    } else {
      fprintf(stderr, "Stack size limit already sufficient: %lu bytes\n", 
              (unsigned long)rl.rlim_cur);
    }
  }
  
  // 在函数最开始就输出，确认程序进入了main
  fputs("MAIN ENTRY\n", stderr);
  fflush(stderr);
  
  int iVar1;
  undefined8 uVar2;
  
  fputs("Before allocation\n", stderr);
  fflush(stderr);
  
  // 动态分配nds_system数组（仅在非 libretro 模式下）
  // 在 DRASTIC_LIBRETRO 模式下，nds_system 已经在 libretro.c 中定义为数组
#ifndef DRASTIC_LIBRETRO
  // #region agent log
  debug_log("main:before_allocation", "Before allocating nds_system", "A", nullptr, 0, 0);
  // #endregion
  if (nds_system == nullptr) {
    fputs("Allocating nds_system...\n", stderr);
    fflush(stderr);
    nds_system = (undefined1*)calloc(62042112, 1);
    // #region agent log
    debug_log("main:after_allocation", "After allocating nds_system", "A", (void*)nds_system, 62042112, (nds_system == nullptr) ? 1 : 0);
    // #endregion
    if (nds_system == nullptr) {
      fprintf(stderr, "ERROR: Failed to allocate nds_system array (62042112 bytes)\n");
      return -1;
    }
    fprintf(stderr, "Allocated nds_system at %p\n", (void*)nds_system);
    fflush(stderr);
  }
#endif
  
  printf("Starting DraStic (version %s)\n\n","r2.5.2.2");
  printf("DEBUG: nds_system address: %p, size: 62042112 bytes\n", (void*)nds_system);
  // 注释掉：偏移量0x57482784 (1464346500字节) 超出nds_system数组大小(62042112字节)
  // 这个值应该在内存映射完成后才设置
  // *(undefined8*)(nds_system + 0x57482784) = 0xffffffffffffffff;
  printf("DEBUG: Calling initialize_system...\n");
  fflush(stdout);
  // #region agent log
  debug_log("main:before_initialize_system", "Before calling initialize_system", "A", (void*)nds_system, (long)nds_system, 0);
  // #endregion
  initialize_system((long)nds_system);
  // #region agent log
  debug_log("main:after_initialize_system", "After calling initialize_system", "A", (void*)nds_system, (long)nds_system, 0);
  // #endregion
  printf("DEBUG: initialize_system completed\n");
  fflush(stdout);
  //process_arguments((long)nds_system,param_1,param_2);
  printf("DEBUG: Calling initialize_screen...\n");
  fflush(stdout);
  // 修复：使用正确的偏移量 0x362e9a9 而不是 0x3b2a9a9
  // 0x3b2a9a9 = (long)nds_system + 0x362e9a9，但这里应该直接使用 0x362e9a9
  initialize_screen(*(uint*)(nds_system + 0x362e9a9));
  printf("DEBUG: initialize_screen completed\n");
  fflush(stdout);
  if (param_1 < 2) {
    // menu函数应该是一个主循环，它会一直运行直到用户退出
    // 如果menu函数返回，说明用户已经退出，程序应该正常退出
    menu((long)nds_system,1);
    printf("Menu exited, cleaning up...\n");
    return 0;
  }
  else {
    // 修复：直接使用 argv[1] 而不是复杂的指针计算
    // 原来的计算 param_2 + param_1 * 8 - 8 可能在某些情况下读取到错误的内存位置
    uVar2 = (undefined8)param_2[1];
    printf("Loading gamecard file %s.\n",(char*)uVar2);
    // #region agent log
    debug_log("main:before_load_nds", "Before calling load_nds", "B", (void*)uVar2, 800, 0);
    // #endregion
    iVar1 = load_nds((long)nds_system + 800,(char*)uVar2);
    // #region agent log
    debug_log("main:after_load_nds", "After calling load_nds", "B", (void*)uVar2, iVar1, 0);
    // #endregion
    if (iVar1 != 0) {
      puts("Gamecard load failed.");
      return 0xffffffff;
    }
    set_screen_menu_off();
    // #region agent log
    debug_log("main:before_reset_system", "Before calling reset_system", "E", (void*)nds_system, (long)nds_system, 0);
    // #endregion
    //reset_system((undefined8)nds_system);
    // #region agent log
    debug_log("main:after_reset_system", "After calling reset_system", "E", (void*)nds_system, (long)nds_system, 0);
    // #endregion
  }
  // 修复：使用正确的偏移量
  // 只有在加载了游戏的情况下才执行这些代码
  // 检查是否已经加载了游戏（通过检查nds_system[0x362e9a8]或其他标志）
  if (*(long *)(nds_system + 0x920) != 0) {
    _setjmp((__jmp_buf_tag *)(nds_system + 0x362e840));
    if (nds_system[0x362e9a8] == '\0') {
      cpu_next_action_arm7_to_event_update(0);    
    }
    else {
      // 注释掉：偏移量0x22847464和0x57482784超出nds_system数组范围
      // 这些值应该在内存映射完成后才访问
      // printf("Calling recompiler event update handler (@ %p).\n",*(void**)(nds_system + 0x22847464));
      // printf("Memory map offset %p, translate cache pointer %p\n",*(void**)(nds_system + 0x57482784),
      //              (void*)0x588000);
      //recompiler_entry((long)nds_system,*(undefined8*)(nds_system + 0x57482784));
      cpu_next_action_arm7_to_event_update(0);
    }
  } else {
    printf("No game loaded, exiting.\n");
  }
  return 0;
}
#endif // DRASTIC_LIBRETRO

// --- 函数: load_nds ---

undefined4 load_nds(long param_1,char *param_2)

{
  long lVar1;
  undefined1 *puVar2;
  int iVar3;
  ::FILE *__s;
  size_t sVar4;
  long lVar5;
  char *pcVar6;
  char *pcVar7;
  undefined4 uVar8;
  long lVar9;
  undefined *puVar10;
  char acStack_c48 [1056];
  undefined1 auStack_828 [2080];
  long local_8;
  
  lVar9 = *(long *)(param_1 + 0x918);
  local_8 = (long)__stack_chk_guard;
  lVar1 = lVar9 + 0x8a780;
  snprintf((char*)auStack_828,0x820,"%s%cunzip_cache",(char*)lVar1,0x2f);
  puVar2 = auStack_828;
  if (*(int *)(lVar9 + 0x85a24) == 0) {
    puVar2 = (undefined1 *)0x0;
  }
  if (*(long *)(param_1 + 0x920) == 0) {
LAB_0016fe48:
    lVar5 = (long)nds_file_open(param_2,(long)puVar2,*(int *)(lVar9 + 0x85a3c),
                          *(int *)(lVar9 + 0x85a28));
  }
  else {
    if (*(int *)(param_1 + 0x2d84) == 0) {
      iVar3 = *(int *)(param_1 + 0x8dc);
    }
    else {
      backup_save(param_1 + 0x968);
      iVar3 = *(int *)(param_1 + 0x8dc);
    }
    if (((iVar3 != 0) && (*(char *)(param_1 + 0x8e3) != '\0')) && (*(long *)(param_1 + 0x8c8) != 0))
    {
      __s = ::fopen((char *)(param_1 + 0x4a0),"wb");
      if (__s == (::FILE *)0x0) {
        printf("ERROR: Couldn\'t save GBA backup %s\n",(char *)(param_1 + 0x4a0));
      }
      else {
        puts("Saving GBA backup file.");
        sVar4 = fwrite(*(void **)(param_1 + 0x8c8),(ulong)*(uint *)(param_1 + 0x8d4),1,__s);
        if (sVar4 != 1) {
          puts("ERROR: Couldn\'t write all of GBA backup.");
        }
        fclose(__s);
      }
    }
    free(*(void **)(param_1 + 0x2d90));
    *(undefined8 *)(param_1 + 0x2d90) = 0;
    nds_file_close(*(undefined8 *)(param_1 + 0x920));
    *(undefined8 *)(param_1 + 0x920) = 0;
    if (*(int *)(param_1 + 0x93c) < 0) goto LAB_0016fe48;
    close(*(int *)(param_1 + 0x93c));
    lVar5 = (long)nds_file_open(param_2,(long)puVar2,*(int *)(lVar9 + 0x85a3c),
                          *(int *)(lVar9 + 0x85a28));
  }
  // #region agent log
  debug_log("load_nds:before_file_check", "Checking nds_file_open result", "B", (void*)lVar5, lVar5, 0);
  // #endregion
  if ((lVar5 == 0) &&
     ((*(int *)(lVar9 + 0x85a24) != 0 ||
      (lVar5 = (long)nds_file_open(param_2,(long)auStack_828,*(int *)(lVar9 + 0x85a3c),
                             *(int *)(lVar9 + 0x85a28)), lVar5 == 0)))) {
    uVar8 = 0xffffffff;
    printf("ERROR: Could not open %s\n",param_2);
    goto LAB_0016ff3c;
  }
  // #region agent log
  debug_log("load_nds:before_pointer_deref", "Before dereferencing lVar5", "B", (void*)lVar5, lVar5, 0);
  // #endregion
  *(long *)(param_1 + 0x920) = lVar5;
  // #region agent log
  debug_log("load_nds:before_header_check", "Before checking header size", "B", (void*)lVar5, lVar5, (lVar5 != 0) ? *(uint *)(lVar5 + 0x10) : 0);
  // #endregion
  if (*(uint *)(lVar5 + 0x10) < 0x200) {
    uVar8 = 0xffffffff;
    printf("%s does not have a valid gamecard_header.\n",param_2);
    goto LAB_0016ff3c;
  }
  pcVar6 = strrchr(param_2,0x2f);
  pcVar7 = param_2;
  if (pcVar6 != (char *)0x0) {
    pcVar7 = pcVar6 + 1;
  }
  pcVar7 = strncpy(acStack_c48,pcVar7,0x400);
  pcVar7 = strrchr(pcVar7,0x2e);
  if (pcVar7 != (char *)0x0) {
    *pcVar7 = '\0';
  }
  strncpy((char *)(lVar9 + 0x8a380),param_2,0x400);
  *(undefined1 *)(lVar9 + 0x8a77f) = 0;
  pcVar6 = strrchr(param_2,0x2f);
  pcVar7 = (char *)(lVar9 + 0x8b380);
  if (pcVar6 != (char *)0x0) {
    param_2 = pcVar6 + 1;
  }
  strncpy(pcVar7,param_2,0x400);
  *(undefined1 *)(lVar9 + 0x8b77f) = 0;
  memcpy((void *)(lVar9 + 0x8af80),pcVar7,0x400);
  pcVar6 = strrchr(pcVar7,0x2e);
  if (pcVar6 != (char *)0x0) {
    *pcVar6 = '\0';
  }
  pcVar6 = getcwd((char *)(lVar9 + 0x855e4),0x400);
  if (pcVar6 == (char *)0x0) {
    uVar8 = 0xffffffff;
    goto LAB_0016ff3c;
  }
  if (*(int *)(lVar9 + 0x85a50) == 0) {
LAB_0016ff38:
    uVar8 = 0;
  }
  else {
    /*
    puVar10 = (undefined*)&DAT_0021f028;
    snprintf((char*)auStack_828,0x820,"%s%c%s%c%s.%s",(char*)lVar1,0x2f,"scripts",0x2f,pcVar7,(char*)&DAT_0021f028
                 );
    printf("Attempting to load lua script %s\n",(char*)auStack_828);
    iVar3 = lua_load_script((long)auStack_828);
    if (iVar3 != 0) {
      snprintf((char*)auStack_828,0x820,"%s%c%s%cdefault.%s",(char*)lVar1,0x2f,"scripts",0x2f,(char*)&DAT_0021f028
                    ,(char*)puVar10);
      printf("Attempting to load lua script %s\n",(char*)auStack_828);
      iVar3 = lua_load_script((long)auStack_828);
      if (iVar3 != 0) goto LAB_0016ff38;
    }
    uVar8 = 0;
    printf("Using lua script %s\n",(char*)auStack_828);
    lua_on_load_game((undefined8)pcVar7);
    */
  }
LAB_0016ff3c:
  if (local_8 != (long)__stack_chk_guard) {
                    
    __stack_chk_fail();
  }
  return uVar8;
}

// --- 函数: initialize_screen ---

void initialize_screen(uint param_1)

{
  DAT_040315a0 = 0x16362004;
  if (param_1 == 0x10) {
    DAT_040315a0 = 0x15151002;
  }
  // 初始化 DAT_04031528 数组（至少需要 0x50 字节，因为 lVar5 最大可以是 0x28，需要两个 8 字节指针）
  // 使用 memset 确保所有元素都被初始化为 0
  memset((void *)&DAT_04031528, 0, 0x50);
  SDL_screen = 0;
  // DAT_04031550 not found in header, using 0 directly
  DAT_04031548 = 0;
  DAT_04031578 = 0;
  DAT_04031570 = 0;
  DAT_04031580 = 0;
  DAT_04031598 = 0;
  
#ifdef DRASTIC_LIBRETRO
  // libretro 模式：不创建 SDL 窗口，只初始化必要的变量
  // RetroArch 会管理窗口，我们只需要初始化屏幕相关的数据结构
  // 安全检查：param_1 应该是 8 或 16（对应 DAT_040315a8 为 2 或 4）
  if (param_1 > 0x100) {
    fprintf(stderr, "WARNING: initialize_screen received invalid param_1 value: 0x%x, using default 8\n", param_1);
    fflush(stderr);
    param_1 = 8;
  }
  DAT_040315a8 = param_1 >> 2;
  DAT_040315ac = 0;
  DAT_040315c4 = 0xffffffffffffffff;
  DAT_040315cc = 0;
  DAT_040315d4 = 0;
  DAT_040315a4 = param_1;
  
  // 在 libretro 模式下，完全不创建窗口
  // 直接设置 dummy 指针，避免调用 SDL_CreateWindow（即使它是存根实现）
  // 这样可以确保不会触发 RetroArch 加载的 SDL2 库中的真实窗口创建函数
  // 使用静态 dummy 指针，避免任何 SDL 调用
  static char dummy_window_storage[1];
  static char dummy_renderer_storage[1];
  DAT_04031570 = (void*)dummy_window_storage;
  DAT_04031578 = (void*)dummy_renderer_storage;
  
  // 输出调试信息，确认跳过了 SDL_CreateWindow 调用
  fprintf(stderr, "[DRASTIC] initialize_screen: libretro mode, skipping SDL_CreateWindow, using dummy pointers\n");
  fflush(stderr);
  
  // 初始化纹理（虽然不会真正使用，但保持代码一致性）
  // 在 libretro 模式下，也使用 dummy 指针，避免任何 SDL 调用
  static char dummy_texture1_storage[1];
  static char dummy_texture2_storage[1];
  DAT_04031588 = (void*)dummy_texture1_storage;
  DAT_04031590 = (void*)dummy_texture2_storage;
  
  // 不调用任何 SDL 函数，直接设置指针
  // 这样可以确保不会触发 RetroArch 加载的 SDL2 库中的真实函数
  
  DAT_04031541 = 1;
  DAT_04031569 = 1;
  DAT_040315ec = 0;
  set_screen_hires_mode(0,0);
  set_screen_hires_mode(1,0);
  
#else
  // 非 libretro 模式：正常创建 SDL 窗口
  clear_screen();
  // 安全检查：param_1 应该是 8 或 16（对应 DAT_040315a8 为 2 或 4）
  // 如果值异常大，使用默认值 8（对应 DAT_040315a8 = 2）
  if (param_1 > 0x100) {
    fprintf(stderr, "WARNING: initialize_screen received invalid param_1 value: 0x%x, using default 8\n", param_1);
    fflush(stderr);
    param_1 = 8;
  }
  DAT_040315a8 = param_1 >> 2;
  DAT_040315ac = 0;
  DAT_040315c4 = 0xffffffffffffffff;
  DAT_040315cc = 0;
  DAT_040315d4 = 0;
  DAT_040315a4 = param_1;
  DAT_04031570 = SDL_CreateWindow("DraStic Nintendo DS Emulator",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,800,0x1e0,SDL_WINDOW_SHOWN);
  if (DAT_04031570 == NULL) {
    fprintf(stderr, "ERROR: SDL_CreateWindow failed: %s\n", SDL_GetError());
    fflush(stderr);
    return;
  } else {
    fprintf(stderr, "DEBUG: SDL_CreateWindow succeeded\n");
    fflush(stderr);
    // 确保窗口显示在前台
    SDL_ShowWindow(DAT_04031570);
    SDL_RaiseWindow(DAT_04031570);
  }
  DAT_04031578 = SDL_CreateRenderer(DAT_04031570,0xffffffff,0);
  if (DAT_04031578 == NULL) {
    fprintf(stderr, "ERROR: SDL_CreateRenderer failed: %s\n", SDL_GetError());
    fflush(stderr);
    return;
  } else {
    fprintf(stderr, "DEBUG: SDL_CreateRenderer succeeded\n");
    fflush(stderr);
  }
  SDL_SetHint("SDL_RENDER_SCALE_QUALITY","linear");
  clear_screen();
  SDL_SetRenderDrawBlendMode(DAT_04031578,1);
  DAT_04031588 = SDL_CreateTexture(DAT_04031578,0x16362004,0,0x20,32);
  DAT_04031590 = SDL_CreateTexture(DAT_04031578,0x16362004,0,0x20,32);
  SDL_SetTextureBlendMode(DAT_04031588,1);
  SDL_SetTextureBlendMode(DAT_04031590,1);
  SDL_UpdateTexture(DAT_04031588,0,stylus_raw_0,0x80);
  SDL_UpdateTexture(DAT_04031590,0,&stylus_raw_1,0x80);
  DAT_04031541 = 1;
  DAT_04031569 = 1;
  DAT_040315ec = 0;
  set_screen_hires_mode(0,0);
  set_screen_hires_mode(1,0);
#endif // DRASTIC_LIBRETRO
  
  return;
}

// --- 函数: set_screen_menu_off ---

void set_screen_menu_off(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined8 uVar6;
  undefined1 auStack_20 [4];
  undefined8 local_1c;
  long local_8;
  
  uVar2 = DAT_040315a4;
  local_8 = (long)__stack_chk_guard;
  if (*(int*)((char*)&DAT_040315c4 + 4) != -1) {
    *(int*)((char*)&DAT_040315ac + 4) = *(int*)((char*)&DAT_040315c4 + 4);
    *(int*)((char*)&DAT_040315c4 + 4) = -1;
  }
  iVar3 = *(int*)((char*)&DAT_040315ac + 4);
  if ((int)DAT_040315c4 != -1) {
    *(int*)&DAT_040315ac = (int)DAT_040315c4;
    *(int*)&DAT_040315c4 = -1;
  }
  iVar1 = (int)DAT_040315ac;
  if ((int)DAT_040315ac == 0) {
    iVar1 = 1;
  }
  printf("Initializing screen: scale %d orientation %d depth %d\n",(int)DAT_040315ac,
               *(int*)((char*)&DAT_040315ac + 4),DAT_040315a4);
  DAT_04031530 = 0;
  DAT_04031540 = 1;
  DAT_04031558 = 0;
  DAT_04031568 = 1;
  if (iVar3 == 2) {
    uVar6 = 0xc0;
    iVar3 = 0x100;
    iVar4 = 0xc0;
    iVar5 = 0x100;
    DAT_04031568 = 0;
  }
  else if (iVar3 == 3) {
    uVar6 = 0xc0;
    iVar3 = 0x100;
    iVar4 = 0xc0;
    iVar5 = 0x100;
    DAT_04031540 = 0;
  }
  else if (iVar3 == 1) {
    uVar6 = 0xc0;
    iVar3 = 0x200;
    iVar4 = 0xc0;
    iVar5 = 0x200;
    DAT_04031558 = 0x100;
  }
  else {
    uVar6 = 0x180;
    iVar3 = 0x100;
    iVar4 = 0x180;
    iVar5 = 0x100;
    DAT_04031558 = 0xc000000000;
  }
  SDL_SetWindowSize(DAT_04031570,iVar5 * iVar1,iVar4 * iVar1);
  SDL_RenderSetLogicalSize(DAT_04031578,iVar3,uVar6);
  // 安全检查：确保计算出的 DAT_040315a8 值合理（应该是 1, 2, 或 4）
  uint calculated_a8 = uVar2 >> 3;
  if (calculated_a8 > 8) {
    fprintf(stderr, "WARNING: set_screen_menu_off: Invalid DAT_040315a8 value: %u (from uVar2=0x%x), using default 2\n", 
            calculated_a8, uVar2);
    fflush(stderr);
    calculated_a8 = 2;  // 默认使用 2 字节每像素
  }
  DAT_040315a8 = calculated_a8;
  DAT_040315d4 = 0;
  *(int*)&DAT_040315e4 = (int)((double)iVar3 * 0.04);
  DAT_040315a4 = uVar2;
  DAT_040315b4 = iVar5;
  DAT_040315b8 = iVar4;
  //SDL_GetCurrentDisplayMode(0,auStack_20);
  DAT_040315bc = local_1c;
  if ((int)DAT_040315ac == 0) {
    SDL_SetWindowFullscreen(DAT_04031570,1);
  }
  else {
    SDL_SetWindowFullscreen(DAT_04031570,0);
  }
  clear_screen();
  if (local_8 != (long)__stack_chk_guard) {
                    
    __stack_chk_fail();
  }
  return;
}

// --- 函数: reset_system ---

void reset_system(undefined8 *param_1)

{
  undefined8 *puVar1;
  undefined8 *puVar2;
  int iVar3;
  undefined8 uVar4;
  long lVar5;
  long local_430;
  undefined1 auStack_428 [1056];
  long local_8;
  
  // #region agent log
  debug_log("reset_system:entry", "Entering reset_system", "E", (void*)param_1, (long)param_1, 0);
  // #endregion
  local_8 = (long)__stack_chk_guard;
  snprintf((char*)auStack_428,0x420,"%s.cfg",(char*)(param_1 + 0x11670));
  load_config_file(param_1,"drastic.cfg",0);
  load_config_file(param_1,auStack_428,1);
  // #region agent log
  debug_log("reset_system:before_reset_cpu", "Before reset_cpu calls", "E", (void*)param_1, (long)param_1, 0);
  // #endregion
  puVar1 = param_1 + 0x2b8faa;
  reset_cpu(puVar1);
  puVar2 = param_1 + 0x4b9c68;
  reset_cpu(puVar2);
  reset_cpu_block(puVar1);
  reset_translation_cache(param_1 + 0x11800);
  // #region agent log
  debug_log("reset_system:before_reset_memory", "Before reset_memory", "E", (void*)param_1, (long)(param_1 + 0x6ba926), 0);
  // #endregion
  reset_memory(param_1 + 0x6ba926);
  reset_video(param_1 + 0x6da3d8);
  reset_gamecard(param_1 + 100);
  reset_spi_peripherals(param_1 + 0x61a);
  reset_spu(param_1 + 0x2b0e00);
  reset_input(param_1 + 0xaaa);
  // #region agent log
  debug_log("reset_system:before_reset_rtc", "Before reset_rtc", "E", (void*)param_1, (long)param_1, 0);
  // #endregion
  reset_rtc(param_1 + 0xaa5,*(undefined4 *)((long)param_1 + 0x85a64),param_1[0x10b4d]);
  reset_event_list(param_1 + 3);
  *param_1 = 0;
  param_1[1] = 0;
  *(undefined4 *)(param_1 + 2) = 0;
  *(undefined2 *)((long)param_1 + 0x14) = 0x106;
  update_screens();
  event_force_task_switch_function((long)param_1);
  event_scanline_start_function((long*)param_1);
  gamecard_load_program(param_1 + 100);
  apply_cycle_adjustment_hacks(param_1);
  if (*(char *)(param_1 + 0x765535) == '\0') {
    param_1[0x2b93fd] = 0;
    param_1[0x4ba0bb] = 0;
    param_1[0x2b9420] = cpu_next_action_arm9_to_arm7;
    param_1[0x4ba0de] = cpu_next_action_arm7_to_event_update;
  }
  else {
    iVar3 = *(int *)(param_1[0x2b93f4] + 0x10);
    param_1[0x2b9420] = (undefined *)0x18cbb0;
    param_1[0x4ba0de] = (undefined *)0x18cc40;
    printf("Performing recompiler base block translation (%x %x).\n",
                 *(uint *)((long)param_1 + 0x15ca10c),
                 *(uint *)((long)param_1 + 0x25d06fc));
    uVar4 = cpu_block_lookup_base(puVar1,iVar3 + 8);
    param_1[0x2b93fb] = uVar4;
    uVar4 = cpu_block_lookup_base(puVar1,iVar3 + 0x18);
    param_1[0x2b93fa] = uVar4;
    uVar4 = cpu_block_lookup_base(puVar2,8);
    param_1[0x4ba0b9] = uVar4;
    uVar4 = cpu_block_lookup_base(puVar2,0x18);
    param_1[0x4ba0b8] = uVar4;
    lVar5 = cpu_block_lookup_base(puVar1,*(undefined4 *)((long)param_1 + 0x15ca10c));
    param_1[0x2b93fd] = lVar5 + 8;
    lVar5 = cpu_block_lookup_base(puVar2,*(undefined4 *)((long)param_1 + 0x25d06fc));
    param_1[0x4ba0bb] = lVar5 + 8;
  }
  set_font_narrow_small();
  screen_wait_for_vsync();
  get_ticks_us(&local_430);
  param_1[0x76552f] = local_430 * 3;
  param_1[0x765530] = 0;
  *(undefined1 *)((long)param_1 + 0x3b2a9a1) = 0;
  *(undefined1 *)((long)param_1 + 0x3b2a9a3) = 0;
  *(undefined4 *)((long)param_1 + 0x3b2a9a4) = 0;
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: cpu_next_action_arm7_to_event_update ---

void cpu_next_action_arm7_to_event_update(long param_1)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  
  execute_events((long)(nds_system + param_1 + 0x18));
  if (*(int *)(nds_system + param_1 + 0x10cde58) != 0) {
    uVar1 = *(uint *)(nds_system + param_1 + 0x10ce110);
    uVar2 = *(uint *)(nds_system + param_1 + 0x10cde60);
    *(undefined4 *)(nds_system + param_1 + 0x10cde60) = 0;
    *(undefined4 *)(nds_system + param_1 + 0x10cdff8) = 0;
    iVar3 = *(int *)(nds_system + param_1 + 0x10cde5c);
    if ((uVar1 >> 7 & 1) == 0) {
      uVar5 = *(uint *)(nds_system + param_1 + 0x10ce10c);
      uVar6 = *(uint *)(nds_system + param_1 + 0x10cde54);
      if ((uVar5 & 1) == 0) {
        iVar7 = uVar5 + 4;
        if (uVar6 != 2) goto LAB_001284d8;
        *(int *)(nds_system + param_1 + 0x10ce0f8) = iVar7;
LAB_00128658:
        *(uint *)(nds_system + param_1 + 0x10cde40) = uVar1;
      }
      else {
        *(uint *)(nds_system + param_1 + 0x10ce10c) = uVar5 & 0xfffffffe;
        iVar7 = (uVar5 & 0xfffffffe) + 4;
        if (uVar6 == 2) {
          *(int *)(nds_system + param_1 + 0x10ce0f8) = iVar7;
        }
        else {
LAB_001284d8:
          *(undefined8 *)(nds_system + param_1 + (ulong)uVar6 * 8 + 0x10cdde0) =
               *(undefined8 *)(nds_system + param_1 + 0x10ce0f4);
          if (uVar6 == 1) {
            if (param_1 + 0x15c9e18U < param_1 + 0x15ca0f0U &&
                param_1 + 0x15ca0e0U < param_1 + 0x15c9e28U) {
              *(undefined8 *)(nds_system + param_1 + 0x10ce0e0) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde18);
              *(undefined8 *)(nds_system + param_1 + 0x10ce0e8) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde20);
              *(undefined8 *)(nds_system + param_1 + 0x10ce0f0) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde28);
            }
            else {
              *(undefined8 *)(nds_system + param_1 + 0x10ce0e8) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde20);
              *(undefined8 *)(nds_system + param_1 + 0x10ce0e0) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde18);
              *(undefined8 *)(nds_system + param_1 + 0x10ce0f0) =
                   *(undefined8 *)(nds_system + param_1 + 0x10cde28);
            }
          }
          else {
            *(undefined4 *)(nds_system + param_1 + 0x10ce0f4) =
                 *(undefined4 *)(nds_system + param_1 + 0x10cddf0);
          }
          *(undefined4 *)(nds_system + param_1 + 0x10cde54) = 2;
          *(int *)(nds_system + param_1 + 0x10ce0f8) = iVar7;
          if ((uVar5 & 1) == 0) goto LAB_00128658;
        }
        *(uint *)(nds_system + param_1 + 0x10cde40) = uVar1 | 0x20;
      }
      iVar7 = 0x18;
      if (iVar3 == 1) {
        iVar7 = *(int *)(*(long *)(nds_system + param_1 + 0x10cdfa0) + 0x10) + 0x18;
      }
      *(int *)(nds_system + param_1 + 0x10ce10c) = iVar7;
      *(uint *)(nds_system + param_1 + 0x10ce110) = uVar1 & 0xffffffc0 | 0x92;
      if (iVar3 == 0 && uVar2 != 0) goto LAB_001285a4;
    }
    else if (iVar3 == 0 && uVar2 != 0) {
LAB_001285a4:
      if (1 < uVar2) {
        *(uint *)(*(long *)(nds_system + param_1 + 0x10cdff0) + 0x2110) =
             *(uint *)(*(long *)(nds_system + param_1 + 0x10cdff0) + 0x2110) & 0xfffffffd;
      }
      event_force_task_switch_function((long)*(undefined8 *)(nds_system + param_1 + 0x10cdfa8));
    }
  }
  if (*(int *)(nds_system + param_1 + 0x20d4448) == 0) goto LAB_00128344;
  uVar1 = *(uint *)(nds_system + param_1 + 0x20d4700);
  uVar2 = *(uint *)(nds_system + param_1 + 0x20d4450);
  *(undefined4 *)(nds_system + param_1 + 0x20d4450) = 0;
  *(undefined4 *)(nds_system + param_1 + 0x20d45e8) = 0;
  iVar3 = *(int *)(nds_system + param_1 + 0x20d444c);
  if ((uVar1 >> 7 & 1) == 0) {
    uVar5 = *(uint *)(nds_system + param_1 + 0x20d46fc);
    uVar6 = *(uint *)(nds_system + param_1 + 0x20d4444);
    if ((uVar5 & 1) == 0) {
      iVar7 = uVar5 + 4;
      if (uVar6 != 2) goto LAB_001283e8;
      *(int *)(nds_system + param_1 + 0x20d46e8) = iVar7;
LAB_00128634:
      *(uint *)(nds_system + param_1 + 0x20d4430) = uVar1;
    }
    else {
      *(uint *)(nds_system + param_1 + 0x20d46fc) = uVar5 & 0xfffffffe;
      iVar7 = (uVar5 & 0xfffffffe) + 4;
      if (uVar6 == 2) {
        *(int *)(nds_system + param_1 + 0x20d46e8) = iVar7;
      }
      else {
LAB_001283e8:
        *(undefined8 *)(nds_system + param_1 + (ulong)uVar6 * 8 + 0x20d43d0) =
             *(undefined8 *)(nds_system + param_1 + 0x20d46e4);
        if (uVar6 == 1) {
          if (param_1 + 0x25d0408U < param_1 + 0x25d06e0U &&
              param_1 + 0x25d06d0U < param_1 + 0x25d0418U) {
            *(undefined8 *)(nds_system + param_1 + 0x20d46d0) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4408);
            *(undefined8 *)(nds_system + param_1 + 0x20d46d8) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4410);
            *(undefined8 *)(nds_system + param_1 + 0x20d46e0) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4418);
          }
          else {
            *(undefined8 *)(nds_system + param_1 + 0x20d46e0) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4418);
            *(undefined8 *)(nds_system + param_1 + 0x20d46d8) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4410);
            *(undefined8 *)(nds_system + param_1 + 0x20d46d0) =
                 *(undefined8 *)(nds_system + param_1 + 0x20d4408);
          }
        }
        else {
          *(undefined4 *)(nds_system + param_1 + 0x20d46e4) =
               *(undefined4 *)(nds_system + param_1 + 0x20d43e0);
        }
        *(undefined4 *)(nds_system + param_1 + 0x20d4444) = 2;
        *(int *)(nds_system + param_1 + 0x20d46e8) = iVar7;
        if ((uVar5 & 1) == 0) goto LAB_00128634;
      }
      *(uint *)(nds_system + param_1 + 0x20d4430) = uVar1 | 0x20;
    }
    iVar7 = 0x18;
    if (iVar3 == 1) {
      iVar7 = *(int *)(*(long *)(nds_system + param_1 + 0x20d4590) + 0x10) + 0x18;
    }
    *(int *)(nds_system + param_1 + 0x20d46fc) = iVar7;
    *(uint *)(nds_system + param_1 + 0x20d4700) = uVar1 & 0xffffffc0 | 0x92;
    if (iVar3 != 0 || uVar2 == 0) goto LAB_00128344;
  }
  else if (iVar3 != 0 || uVar2 == 0) goto LAB_00128344;
  if (1 < uVar2) {
    *(uint *)(*(long *)(nds_system + param_1 + 0x20d45e0) + 0x2110) =
         *(uint *)(*(long *)(nds_system + param_1 + 0x20d45e0) + 0x2110) & 0xfffffffd;
  }
  event_force_task_switch_function((long)*(undefined8 *)(nds_system + param_1 + 0x20d4598));
LAB_00128344:
  // 修复：添加空指针检查，避免段错误
  int *ptr_318 = *(int **)(nds_system + param_1 + 0x318);
  if (ptr_318 == nullptr) {
    // 如果指针未初始化，使用默认值0
    iVar3 = 0;
  } else {
    iVar3 = *ptr_318;
  }
  iVar7 = *(int *)(nds_system + param_1 + 0x10cde60);
  iVar4 = *(int *)(nds_system + param_1 + 0x10cdfe0);
  *(int *)(nds_system + param_1 + 0x10) = iVar3;
  *(int *)(nds_system + param_1 + 0x10cdfe0) = iVar4 + iVar3;
  if (iVar7 != 0) {
  //  *(undefined4 *)(nds_system + param_1 + 0x10cdfe0) = 0xffffffff;
                    
                    
  //  (**(code1 **)(nds_system + param_1 + 0x10ce100))(param_1);
    return;
  }
  // 修复：添加空指针检查，确保 nds_system 有效
  if (nds_system != nullptr) {
    long cpu_ptr = (long)(nds_system + param_1 + 0x15c7d50);
    if (cpu_ptr != 0) {
      execute_cpu(cpu_ptr);
    }
  }
                    
                    
  // 修复：添加空指针检查，避免段错误
  code1 **func_ptr_ptr = (code1 **)(nds_system + param_1 + 0x10ce100);
  if (func_ptr_ptr != nullptr && *func_ptr_ptr != nullptr) {
    // 双重解引用：func_ptr_ptr -> *func_ptr_ptr -> **func_ptr_ptr
    code1 func_ptr = **func_ptr_ptr;  // 解引用两次，得到函数指针
    func_ptr(*(long *)(nds_system + param_1 + 0x10cdfa8));
  }
  return;
}

// --- 函数: recompiler_entry ---

void recompiler_entry(long param_1,undefined8 param_2)

{
  int iVar1;
  undefined8 uVar2;
  long lVar3;
  ulong uVar4;
  int iVar5;
  
  *(undefined8 *)(nds_system + param_1 + 0x20d4698) = param_2;
  *(long *)(nds_system + param_1 + 0x20d46a0) = param_1 + 0x8c000;
  uVar4 = 0x15c7d50;
  *(undefined8 *)(nds_system + param_1 + 0x10ce0a8) = param_2;
  *(long *)(nds_system + param_1 + 0x10ce0b0) = param_1 + 0x8c000;
  do {
    execute_events((long)(nds_system + param_1 + 0x18));
    if (*(int *)(nds_system + param_1 + 0x10cde58) != 0) {
      if ((*(uint *)(nds_system + param_1 + 0x10ce110) >> 7 & 1) == 0) {
        if (*(long *)(nds_system + param_1 + 0x10cdfe8) != 0) {
          *(undefined4 *)(nds_system + param_1 + 0x10ce10c) =
               *(undefined4 *)
                (*(long *)(nds_system + param_1 + 0x10ce0b0) +
                 (ulong)*(uint *)(*(long *)(nds_system + param_1 + 0x10cdfe8) + -0xc) + 4);
        }
        *(uint *)(nds_system + param_1 + 0x10ce110) =
             *(uint *)(nds_system + param_1 + 0x10ce108) & 0xf0000000 |
             *(uint *)(nds_system + param_1 + 0x10ce110) & 0xfffffff;
        execute_arm_raise_interrupt(param_1 + 0x15c7d50);
        lVar3 = *(long *)(nds_system + param_1 + 0x10cdfd0);
      }
      else {
        if (*(int *)(nds_system + param_1 + 0x10cde60) == 0) goto LAB_0018cd00;
        lVar3 = cpu_block_lookup_base
                          (param_1 + 0x15c7d50,*(undefined4 *)(nds_system + param_1 + 0x10ce10c));
      }
      *(undefined4 *)(nds_system + param_1 + 0x10cde60) = 0;
      *(long *)(nds_system + param_1 + 0x10cdfe8) = lVar3 + 8;
    }
LAB_0018cd00:
    if (*(int *)(nds_system + param_1 + 0x20d4448) != 0) {
      if ((*(uint *)(nds_system + param_1 + 0x20d4700) >> 7 & 1) == 0) {
        if (*(long *)(nds_system + param_1 + 0x20d45d8) != 0) {
          *(undefined4 *)(nds_system + param_1 + 0x20d46fc) =
               *(undefined4 *)
                (*(long *)(nds_system + param_1 + 0x20d46a0) +
                 (ulong)*(uint *)(*(long *)(nds_system + param_1 + 0x20d45d8) + -0xc) + 4);
        }
        if ((*(int *)(nds_system + param_1 + 0x20d4450) != 0) &&
           (event_force_task_switch_function(param_1),
           (*(uint *)(nds_system + param_1 + 0x20d4450) >> 1 & 1) != 0)) {
          *(int *)(nds_system + param_1 + 0x10cde60) =
               *(int *)(nds_system + param_1 + 0x10cde60) + -2;
        }
        *(uint *)(nds_system + param_1 + 0x20d4700) =
             *(uint *)(nds_system + param_1 + 0x20d46f8) & 0xf0000000 |
             *(uint *)(nds_system + param_1 + 0x20d4700) & 0xfffffff;
        execute_arm_raise_interrupt(param_1 + 0x25ce340);
        lVar3 = *(long *)(nds_system + param_1 + 0x20d45c0);
      }
      else {
        if (*(int *)(nds_system + param_1 + 0x20d4450) == 0) goto LAB_0018cdac;
        event_force_task_switch_function(param_1);
        if ((*(uint *)(nds_system + param_1 + 0x20d4450) >> 1 & 1) != 0) {
          *(int *)(nds_system + param_1 + 0x10cde60) =
               *(int *)(nds_system + param_1 + 0x10cde60) + -2;
        }
        lVar3 = cpu_block_lookup_base
                          (param_1 + 0x25ce340,*(undefined4 *)(nds_system + param_1 + 0x20d46fc));
      }
      *(undefined4 *)(nds_system + param_1 + 0x20d4450) = 0;
      *(long *)(nds_system + param_1 + 0x20d45d8) = lVar3 + 8;
    }
LAB_0018cdac:
    ulong nzcv;
    iVar1 = **(int **)(param_1 + 0x318);
    iVar5 = *(int *)(nds_system + param_1 + 0x10cdfe0);
    *(int *)(param_1 + 0x10) = iVar1;
    iVar5 = iVar5 + iVar1;
    nzcv = (ulong)*(uint *)(nds_system + param_1 + 0x10ce108);
    if (-1 < iVar5) {
      ///if (*(int *)(nds_system + param_1 + 0x10cde60) == 0) {
                    
                    
       // (**(code1 **)(nds_system + param_1 + 0x10cdfe8))
        //          (0,(ulong)*(uint *)(nds_system + param_1 + 0x10ce108));
        return;
      //}
      iVar5 = -1;
    }
    *(code1 **)(nds_system + param_1 + 0x10cdfe8) = *(code1 **)(nds_system + param_1 + 0x10cdfe8);
    uVar2 = nzcv;
    *(undefined4 *)(nds_system + param_1 + 0x10ce0c0) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0c0);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0c4) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0c4);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0c8) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0c8);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0cc) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0cc);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0d0) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0d0);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0d4) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0d4);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0d8) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0d8);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0dc) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0dc);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0e0) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0e0);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0e4) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0e4);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0e8) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0e8);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0ec) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0ec);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0f0) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0f0);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0f4) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0f4);
    *(undefined4 *)(nds_system + param_1 + 0x10ce0f8) =
         *(undefined4 *)(nds_system + param_1 + 0x10ce0f8);
    *(int *)(nds_system + param_1 + 0x10ce108) = (int)uVar2;
    *(int *)(nds_system + param_1 + 0x10cdfe0) = iVar5;
    lVar3 = *(long *)(nds_system + param_1 + 0x10cdff0);
    iVar5 = *(int *)(lVar3 + 0x2290) + *(int *)(*(long *)(lVar3 + 0x2258) + 0x10);
    uVar4 = (ulong)*(uint *)(lVar3 + 0x23b8);
    nzcv = uVar4;
    if (-1 < iVar5) {
      if (*(int *)(lVar3 + 0x2110) == 0) {
                    
                    
        //(**(code1 **)(lVar3 + 0x2298))();
        return;
      }
      iVar5 = -1;
    }
    *(code1 **)(lVar3 + 0x2298) = *(code1 **)(lVar3 + 0x2298);
    uVar2 = nzcv;
    *(undefined4 *)(lVar3 + 0x2370) = *(undefined4 *)(lVar3 + 0x2370);
    *(undefined4 *)(lVar3 + 0x2374) = *(undefined4 *)(lVar3 + 0x2374);
    *(undefined4 *)(lVar3 + 0x2378) = *(undefined4 *)(lVar3 + 0x2378);
    *(undefined4 *)(lVar3 + 0x237c) = *(undefined4 *)(lVar3 + 0x237c);
    *(undefined4 *)(lVar3 + 0x2380) = *(undefined4 *)(lVar3 + 0x2380);
    *(undefined4 *)(lVar3 + 0x2384) = *(undefined4 *)(lVar3 + 0x2384);
    *(undefined4 *)(lVar3 + 0x2388) = *(undefined4 *)(lVar3 + 0x2388);
    *(undefined4 *)(lVar3 + 0x238c) = *(undefined4 *)(lVar3 + 0x238c);
    *(undefined4 *)(lVar3 + 0x2390) = *(undefined4 *)(lVar3 + 0x2390);
    *(undefined4 *)(lVar3 + 0x2394) = *(undefined4 *)(lVar3 + 0x2394);
    *(undefined4 *)(lVar3 + 0x2398) = *(undefined4 *)(lVar3 + 0x2398);
    *(undefined4 *)(lVar3 + 0x239c) = *(undefined4 *)(lVar3 + 0x239c);
    *(undefined4 *)(lVar3 + 0x23a0) = *(undefined4 *)(lVar3 + 0x23a0);
    *(undefined4 *)(lVar3 + 0x23a4) = *(undefined4 *)(lVar3 + 0x23a4);
    *(undefined4 *)(lVar3 + 0x23a8) = *(undefined4 *)(lVar3 + 0x23a8);
    *(int *)(lVar3 + 0x23b8) = (int)uVar2;
    *(int *)(lVar3 + 0x2290) = iVar5;
    param_1 = *(long *)(lVar3 + 0x2258);
  } while( true );
}

// --- 函数: process_arguments ---

// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void process_arguments(long param_1,undefined4 param_2,undefined8 param_3)

{
  char *pcVar1;
  int iVar2;
  ulonglong uVar3;
  ulong uVar4;
  int local_c;
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  puts("Processing command line options.");
LAB_0010f150:
  do {
    iVar2 = getopt_long(param_2,param_3,"",(const struct option*)long_options_11199,&local_c);
    while( true ) {
      if (iVar2 == -1) {
        if (local_8 != (long)__stack_chk_guard) {
                    // WARNING: Subroutine does not return
          __stack_chk_fail();
        }
        return;
      }
      if (iVar2 != 0) goto LAB_0010f150;
      if (local_c == 7) {
        uVar4 = strtol(optarg,(char **)0x0,10);
        if (((int)uVar4 - 0x10U & 0xffffffef) == 0) {
          printf("Using a color depth of %dbpp.\n",uVar4 & 0xffffffff);
          nds_system[param_1 + 0x362e9a9] = (char)uVar4;
        }
        else {
          printf("Invalid color depth %s specified.\n",optarg);
        }
        goto LAB_0010f150;
      }
      if (7 < local_c) {
        if (local_c == 0xb) {
          *(undefined4 *)(param_1 + 0x85a04) = 1;
        }
        else if (local_c < 0xc) {
          if (local_c == 9) {
            uVar4 = strtol(optarg,(char **)0x0,10);
            initialize_benchmark
                      (param_1 + 0x8a318,param_1,uVar4 & 0xffffffff,0x7f,
                       *(undefined4 *)(param_1 + 0x85a00),1);
            *(undefined4 *)(param_1 + 0x85a04) = 1;
          }
          else if (local_c == 10) {
            uVar4 = strtol(optarg,(char **)0x0,10);
            initialize_benchmark
                      (param_1 + 0x8a318,param_1,uVar4 & 0xffffffff,2,
                       *(undefined4 *)(param_1 + 0x85a00),1);
            *(undefined4 *)(param_1 + 0x85a04) = 1;
          }
          else if (local_c == 8) {
            uVar4 = strtol(optarg,(char **)0x0,10);
            initialize_benchmark
                      (param_1 + 0x8a318,param_1,uVar4 & 0xffffffff,0x7f,
                       *(undefined4 *)(param_1 + 0x85a00),0);
            *(undefined4 *)(param_1 + 0x85a04) = 1;
          }
        }
        else if (local_c == 0xd) {
          input_log_playback(param_1 + 0x5550,optarg);
        }
        else if (local_c == 0xe) {
          puts("Using interpreter.");
          nds_system[param_1 + 0x362e9a8] = 0;
        }
        else if (local_c == 0xc) {
          input_log_record(param_1 + 0x5550,optarg);
        }
        goto LAB_0010f150;
      }
      if (local_c == 3) {
        printf("debug option %d: %s %s %s\n",3,&DAT_0021aac0,"COUNTDOWN_BREAKPOINT",optarg)
        ;
        pcVar1 = optarg;
        set_debug_mode(param_1 + 0x15c9e68,4);
        if (pcVar1 != (char *)0x0) {
          uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
          *(ulonglong *)(nds_system + param_1 + 0x10cdf78) = uVar3;
        }
        goto LAB_0010f150;
      }
      if (local_c < 4) {
        if (local_c == 1) {
          printf("debug option %d: %s %s %s\n",1,&DAT_0021aac0,"PC_BREAKPOINT",optarg);
          pcVar1 = optarg;
          set_debug_mode(param_1 + 0x15c9e68,3);
          if (pcVar1 != (char *)0x0) {
            uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
            *(ulonglong *)(nds_system + param_1 + 0x10cdf78) = uVar3;
          }
        }
        else if (local_c == 2) {
          printf("debug option %d: %s %s %s\n",2,&DAT_0021aa98,"COUNTDOWN_BREAKPOINT",
                       optarg);
          pcVar1 = optarg;
          set_debug_mode(param_1 + 0x25d0458,4);
          if (pcVar1 != (char *)0x0) {
            uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
            *(ulonglong *)(nds_system + param_1 + 0x20d4568) = uVar3;
          }
        }
        else if (local_c == 0) {
          printf("debug option %d: %s %s %s\n",0,&DAT_0021aa98,"PC_BREAKPOINT",optarg);
          pcVar1 = optarg;
          set_debug_mode(param_1 + 0x25d0458,3);
          if (pcVar1 != (char *)0x0) {
            uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
            *(ulonglong *)(nds_system + param_1 + 0x20d4568) = uVar3;
          }
        }
        goto LAB_0010f150;
      }
      if (local_c == 5) {
        printf("debug option %d: %s %s %s\n",5,&DAT_0021aac0,&DAT_0021aae0,optarg);
        pcVar1 = optarg;
        set_debug_mode(param_1 + 0x15c9e68,0);
        if (pcVar1 != (char *)0x0) {
          uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
          *(ulonglong *)(nds_system + param_1 + 0x10cdf78) = uVar3;
        }
        goto LAB_0010f150;
      }
      if (local_c != 6) break;
      puts("Using recompiler.");
      nds_system[param_1 + 0x362e9a8] = 1;
      iVar2 = getopt_long(param_2,param_3,"",(const struct option*)long_options_11199,&local_c);
    }
    if (local_c == 4) {
      printf("debug option %d: %s %s %s\n",4,&DAT_0021aa98,&DAT_0021aae0,optarg);
      pcVar1 = optarg;
      set_debug_mode(param_1 + 0x25d0458,0);
      if (pcVar1 != (char *)0x0) {
        uVar3 = strtoull(pcVar1,(char **)0x0,0x10);
        *(ulonglong *)(nds_system + param_1 + 0x20d4568) = uVar3;
      }
    }
  } while( true );
}

void initialize_system(long param_1)

{
  int iVar1;
  char *pcVar2;
  
  //initialize_lua();
  platform_initialize();
  
  // 检查 param_1 是否有效
  if (param_1 == 0) {
    fprintf(stderr, "ERROR: initialize_system called with null pointer\n");
    return;
  }
  
  // 检查 param_1 是否等于 nds_system 的地址（param_1 就是传入的 nds_system 地址）
  if (param_1 != (long)nds_system) {
    fprintf(stderr, "ERROR: initialize_system param_1 mismatch: param_1=%p, nds_system=%p\n", 
            (void*)param_1, (void*)nds_system);
    return;
  }
  
  // 检查偏移量是否在有效范围内（检查最大偏移量是否超出数组边界）
  size_t arr_size = 62042112;
  size_t max_offset = 0x8ab80 + 0x400;  // 最大使用的偏移量
  if (max_offset > arr_size) {
    fprintf(stderr, "ERROR: initialize_system max offset out of range: max_offset=%zu, arr_size=%zu\n", 
            max_offset, arr_size);
    return;
  }
  
  // #region agent log
  debug_log("initialize_system:before_getcwd", "Before getcwd", "D", (void*)param_1, param_1 + 0x8a780, 0);
  // #endregion
  pcVar2 = getcwd((char *)(param_1 + 0x8a780),0x400);
  // #region agent log
  debug_log("initialize_system:after_getcwd", "After getcwd", "D", (void*)pcVar2, (long)param_1, 0);
  // #endregion
  if (pcVar2 == (char *)0x0) {
    puts("getcwd for root path failed.");
    // 如果getcwd失败，使用当前目录的默认值
    strncpy((char *)(param_1 + 0x8a780), ".", 0x400 - 1);
    ((char *)(param_1 + 0x8a780))[0x400 - 1] = '\0';
  }
  // #region agent log
  debug_log("initialize_system:before_strcpy", "Before strcpy_chk", "D", (void*)param_1, param_1 + 0x8ab80, 0);
  // #endregion
  __strcpy_chk((char *)(param_1 + 0x8ab80),(char *)(param_1 + 0x8a780),0x400);
  fprintf(stderr, "DEBUG: After strcpy_chk, calling initialize_system_directories\n");
  fflush(stderr);
  initialize_system_directories(param_1);
  fprintf(stderr, "DEBUG: After initialize_system_directories, calling config_default\n");
  fflush(stderr);
  config_default((undefined4 *)(param_1 + 0x855a8));
  fprintf(stderr, "DEBUG: After config_default\n");
  fflush(stderr);
  load_directory_config_file(param_1,"drastic.cf2");
  load_config_file(param_1,"drastic.cfg",0);
  initialize_cpu(param_1 + 0x15c7d50,param_1,1,param_1 + 0x25ce340);
  initialize_cpu(param_1 + 0x25ce340,param_1,0,param_1 + 0x15c7d50);
  initialize_translation_cache(param_1 + 0x8c000);
  initialize_input(param_1 + 0x5550,param_1);
  /*
  iVar1 = initialize_memory(param_1 + 0x35d4930,param_1);
  if (-1 < iVar1) {
    initialize_video(param_1 + 0x36d1ec0,param_1 + 0x35d4930);
    initialize_spu(param_1 + 0x1587000,param_1);
    initialize_gamecard(param_1 + 800,param_1);
    initialize_spi_peripherals(param_1 + 0x30d0,param_1);
    initialize_rtc(param_1 + 0x5528,param_1);
    initialize_event_list(param_1 + 0x18,param_1);
    *(undefined2 *)(nds_system + param_1 + 0x362e9a8) = 0x2001;
    *(undefined4 *)(param_1 + 0x8a368) = 0;
    *(undefined1 *)(param_1 + 0x8af80) = 0;
    *(undefined1 *)(param_1 + 0x8b380) = 0;
    *(undefined4 *)(param_1 + 0x8a378) = 0;
    return;
  }
  */
  // #region agent log
  debug_log("initialize_system:before_initialize_memory", "Before initialize_memory", "D", (void*)param_1, param_1 + 0x35d4930, 0);
  // #endregion
  fprintf(stderr, "[DRASTIC] initialize_system: About to call initialize_memory(param_1=0x%lx, param_2=0x%lx)\n", 
          (unsigned long)(param_1 + 0x35d4930), (unsigned long)param_1);
  fflush(stderr);
  initialize_memory(param_1 + 0x35d4930,param_1);
  fprintf(stderr, "[DRASTIC] initialize_system: Successfully returned from initialize_memory\n");
  fflush(stderr);
  // #region agent log
  debug_log("initialize_system:after_initialize_memory", "After initialize_memory", "D", (void*)param_1, param_1 + 0x35d4930, 0);
  // #endregion
  //menu_bios_warning(param_1);
  //puts("FATAL: Could not load system files.");
  initialize_video(param_1 + 0x36d1ec0,param_1 + 0x35d4930);
  initialize_spu(param_1 + 0x1587000,param_1);
  initialize_gamecard(param_1 + 800,param_1);
  initialize_spi_peripherals(param_1 + 0x30d0,param_1);
  initialize_rtc(param_1 + 0x5528,param_1);
  initialize_event_list(param_1 + 0x18,param_1);
                    // WARNING: Subroutine does not return
  //quit(param_1);
}

// --- 函数: menu ---

void menu(long param_1,int param_2)

{
  uint *puVar1;
  long lVar2;
  undefined8 *puVar3;
  uint uVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  undefined8 *__ptr;
  bool bVar8;
  long lVar9;
  code *pcVar10;
  ulong uVar11;
  void *pvVar12;
  int local_5a0 [2];
  long local_598;
  uint *local_590;
  undefined8 *local_588;
  undefined8 uStack_580;
  undefined8 local_578;
  void *local_570;
  void *local_568;
  void *local_560;
  int local_558;
  int local_554;
  int iStack_550;
  undefined4 uStack_54c;
  uint local_548;
  uint local_544;
  char local_540[8];
  byte local_538;
  byte local_537;
  byte local_536;
  undefined1 local_535 [261];
  int local_430;
  undefined1 auStack_428 [1056];
  long local_8;
  
  local_548 = *(uint *)(param_1 + 0x859f4);
  local_8 = (long)__stack_chk_guard;
  iVar7 = *(int *)(param_1 + 0x85a34);
  if (2 < local_548) {
    local_548 = local_548 - 1;
  }
  local_540[0] = (char)CONCAT17((char)((ulong)*(undefined8 *)(param_1 + 0x855c0) >> 0x20),
                       CONCAT16((char)*(undefined8 *)(param_1 + 0x855c0),
                                CONCAT15((char)((ulong)*(undefined8 *)(param_1 + 0x855b8) >> 0x20),
                                         CONCAT14((char)*(undefined8 *)(param_1 + 0x855b8),
                                                  CONCAT13((char)((ulong)*(undefined8 *)
                                                                          (param_1 + 0x855b0) >>
                                                                 0x20),
                                                           CONCAT12((char)*(undefined8 *)
                                                                           (param_1 + 0x855b0),
                                                                    CONCAT11((char)((ulong)*(
                                                  undefined8 *)(param_1 + 0x855a8) >> 0x20),
                                                  (char)*(undefined8 *)(param_1 + 0x855a8))))))));
  local_544 = 0;
  puVar1 = (uint *)(param_1 + 0x855a8);
  local_554 = 0;
  local_538 = (byte)*(undefined4 *)(param_1 + 0x855c8);
  local_537 = (byte)*(undefined4 *)(param_1 + 0x855cc);
  local_536 = (byte)*(undefined4 *)(param_1 + 0x855d0);
  local_430 = 0;
  if (iVar7 == 100000) {
    bVar8 = true;
    uVar4 = 1;
LAB_0017fcd8:
    if (iVar7 == 0x411a) {
      uVar4 = 5;
    }
    else {
LAB_0017ff04:
      if (iVar7 != 0x37cd) goto LAB_00180120;
      uVar4 = 6;
    }
  }
  else {
    if (iVar7 != 0x8235) {
      if (iVar7 != 25000) {
        bVar8 = false;
        uVar4 = 0;
        goto LAB_0017fccc;
      }
      bVar8 = true;
      uVar4 = 3;
      goto LAB_0017ff04;
    }
    bVar8 = true;
    uVar4 = 2;
LAB_0017fccc:
    if (iVar7 != 20000) goto LAB_0017fcd8;
    bVar8 = true;
    uVar4 = 4;
LAB_00180120:
    if (iVar7 == 0x30d4) {
      uVar4 = 7;
    }
    else if (!bVar8) goto LAB_0017fcec;
  }
  local_544 = uVar4;
LAB_0017fcec:
  local_598 = param_1;
  local_590 = puVar1;
  if (*(char *)(param_1 + 0x8b380) == '\0') {
    local_558 = 0;
  }
  else {
    local_558 = 1;
    pvVar12 = malloc(0x18000);
    local_570 = pvVar12;
    local_568 = malloc(0x18000);
    screen_copy16(pvVar12,0);
    screen_copy16(local_568,1);
  }
  printf("DEBUG: menu: Loading logo...\n");
  fflush(stdout);
  load_logo(&local_598);
  printf("DEBUG: menu: Creating main menu...\n");
  fflush(stdout);
  __ptr = (undefined8 *)create_menu_main(&local_598);
  if (__ptr == (undefined8 *)0x0) {
    printf("ERROR: create_menu_main returned NULL\n");
    fflush(stdout);
    return;
  }
  uStack_580 = 0;
  local_578 = 0;
  iStack_550 = 0;
  uStack_54c = 1;
  local_535[0] = 0;
  local_588 = __ptr;
  printf("DEBUG: menu: Pausing audio...\n");
  fflush(stdout);
  uVar5 = audio_pause(param_1 + 0x1587000);
  printf("DEBUG: menu: Setting screen menu on...\n");
  fflush(stdout);
  set_screen_menu_on();
  if (param_2 != 0) {
    printf("DEBUG: menu: Loading file...\n");
    fflush(stdout);
    iVar7 = load_file(&local_598,(long *)&nds_ext,auStack_428);
    lVar9 = local_598;
    if ((iVar7 != -1) && (iVar7 = load_nds(local_598 + 800,auStack_428), -1 < iVar7)) {
      local_548 = *(uint *)(lVar9 + 0x859f4);
      local_554 = 1;
      iStack_550 = 1;
      uStack_54c = 0;
    }
  }
  printf("DEBUG: menu: Entering main loop...\n");
  fflush(stdout);
LAB_0017fd50:
  do {
    while( true ) {
      do {
        puVar3 = local_588;
        if ((iStack_550 != 0) && (*(char *)(param_1 + 0x8b380) != '\0')) {
          audio_revert_pause_state(param_1 + 0x1587000,uVar5);
          do {
            get_gui_input(local_598 + 0x5550,local_5a0);
          } while (local_5a0[0] != 0xb);
          clear_gui_actions();
          uVar11 = 0;
          if (*(int *)((long)__ptr + 0x14) != 0) {
            do {
              pvVar12 = *(void **)(__ptr[4] + uVar11 * 8);
              //if (*(code1 **)((long)pvVar12 + 0x28) != (code *)0x0) {
              //  (**(code1 **)((long)pvVar12 + 0x28))(&local_598,pvVar12);
              //}
              free(pvVar12);
              uVar4 = (int)uVar11 + 1;
              uVar11 = (ulong)uVar4;
            } while (uVar4 < *(uint *)((long)__ptr + 0x14));
          }
          free((void *)__ptr[4]);
          free(__ptr);
          uVar4 = local_548;
          if (1 < local_548) {
            uVar4 = *(uint *)(param_1 + 0x859f4) | 2;
          }
          *(uint *)(param_1 + 0x859f4) = uVar4;
          uVar4 = 0;
          if (local_544 != 0) {
            uVar4 = 0;
            if (*(uint *)(speed_override_values_11981 + (ulong)local_544 * 4) != 0) {
              uVar4 = 5000000 / *(uint *)(speed_override_values_11981 + (ulong)local_544 * 4);
            }
          }
          *(uint *)(param_1 + 0x85a34) = uVar4;
          *local_590 = (uint)local_540[0];
          local_590[1] = (uint)local_540[1];
          local_590[2] = (uint)local_540[2];
          local_590[3] = (uint)local_540[3];
          local_590[4] = (uint)local_540[4];
          local_590[5] = (uint)local_540[5];
          local_590[6] = (uint)local_540[6];
          local_590[7] = (uint)local_540[7];
          local_590[8] = (uint)local_538;
          local_590[9] = (uint)local_537;
          local_590[10] = (uint)local_536;
          config_update_settings(puVar1);
          set_screen_menu_off();
          if (local_560 != (void *)0x0) {
            free(local_560);
          }
          if (local_554 != 0) {
            reset_system(param_1);
          }
          nds_system[param_1 + 0x362e9a3] = 1;
          if (local_558 != 0) {
            free(local_570);
            free(local_568);
            if (local_554 != 0) {
              puts("Performing long jmp to reset.");
                    
              __longjmp_chk(param_1 + 0x3b2a840,0);
            }
          }
          if (local_8 - __stack_chk_guard == 0) {
            return;
          }
                    
          __stack_chk_fail();
        }
        delay_us(10000);
        draw_menu_bg(&local_598);
        uVar11 = 0;
        set_font_narrow();
        print_string(local_535,0xa676,0,0x10,0);
        set_font_wide();
        if (*(int *)((long)puVar3 + 0x14) != 0) {
          do {
            lVar9 = *(long *)(puVar3[4] + uVar11 * 8);
            iVar7 = (int)uVar11;
            uVar4 = iVar7 + 1;
            uVar11 = (ulong)uVar4;
            //(**(code3 **)(lVar9 + 0x10))(&local_598)(puVar3 + 3) == iVar7);
          } while (uVar4 < *(uint *)((long)puVar3 + 0x14));
        }
        if ((code *)*puVar3 != (code *)0x0) {
          (*(code2 *)*puVar3)(&local_598,puVar3);
        }
        lVar9 = *(long *)(puVar3[4] + (ulong)*(uint *)(puVar3 + 3) * 8);
        update_screen_menu();
        update_screen_menu();
        update_screen_menu();
        if (local_430 == 0) {
          lVar2 = local_598 + 0x5550;
          do {
            get_gui_input(lVar2,local_5a0);
          } while (local_5a0[0] == 0xb);
        }
      } while (*(code1 **)(lVar9 + 0x18) == (code *)0x0);
      iVar6 = (**(code3 **)(lVar9 + 0x18))(&local_598,lVar9,local_5a0);
      iVar7 = 1;
      if (iVar6 != 1) break;
LAB_0017fe3c:
      iVar6 = *(int *)(puVar3 + 3);
      if (*(code1 **)(lVar9 + 0x20) != (code *)0x0) {
        (**(code3 **)(lVar9 + 0x20))(&local_598,lVar9,1);
      }
      uVar4 = *(uint *)((long)puVar3 + 0x14) - 1;
      if (-1 < iVar7 + iVar6) {
        uVar4 = iVar7 + iVar6;
      }
      if (uVar4 < *(uint *)((long)puVar3 + 0x14)) {
        uVar11 = -(ulong)(uVar4 >> 0x1f) & 0xfffffff800000000 | (ulong)uVar4 << 3;
      }
      else {
        uVar4 = 0;
        uVar11 = 0;
      }
      lVar9 = *(long *)(puVar3[4] + uVar11);
      pcVar10 = *(code1 **)(lVar9 + 0x20);
      *(uint *)(puVar3 + 3) = uVar4;
      //if (pcVar10 != (code *)0x0) {
      //  (*pcVar10)(&local_598,lVar9,0);
      //}
    }
    if (iVar6 != 5) {
      if (iVar6 == 0) {
        iVar7 = -1;
        goto LAB_0017fe3c;
      }
      goto LAB_0017fd50;
    }
    select_exit_current_menu(&local_598,lVar9);
  } while( true );
}

// --- 函数: puts ---

int puts(char *__s)

{
  int iVar1;
  
  iVar1 = puts(__s);
  return iVar1;
}

// --- 函数: backup_save ---

void backup_save(long param_1)

{
  if (*(char *)(param_1 + 0x2000) == '\0') {
    return;
  }
  backup_save_part_0((uint)param_1);
  return;
}

// --- 函数: nds_file_open ---

long * nds_file_open(char *param_1,long param_2,int param_3,int param_4)

{
  uint uVar1;
  int __fd;
  int iVar2;
  int iVar3;
  char *__s1;
  void *pvVar4;
  __off_t _Var5;
  ssize_t sVar6;
  ulong uVar7;
  size_t sVar8;
  FILE *pFVar9;
  uint uVar10;
  long *__ptr;
  char *__filename;
  ulong local_1098;
  long local_1090;
  struct stat acStack_c08;
  char acStack_c08_str [1024];
  char local_808 [1023];
  undefined1 local_409;
  char local_408 [1023];
  undefined1 local_9;
  long local_8;
  
  __ptr = (long *)0x0;
  local_8 = (long)__stack_chk_guard;
  // #region agent log
  debug_log("nds_file_open:entry", "Entering nds_file_open", "B", (void*)param_1, 0, 0);
  // #endregion
  iVar3 = 1;
  if (param_4 != 0) {
    iVar3 = 0x8001;
  }
  if ((param_1 == (char *)0x0) || (__fd = open(param_1,0), __fd < 0)) {
    // #region agent log
    debug_log("nds_file_open:file_open_failed", "File open failed", "B", (void*)param_1, __fd, 0);
    // #endregion
    goto LAB_00175820;
  }
  __ptr = malloc(0x20);
  // #region agent log
  debug_log("nds_file_open:after_malloc", "After malloc", "B", (void*)__ptr, 0, 0);
  // #endregion
  if (__ptr == (long *)0x0) {
    close(__fd);
    goto LAB_00175820;
  }
  __s1 = strrchr(param_1,0x2e);
  if (__s1 == (char *)0x0) {
LAB_0017598c:
    close(__fd);
  }
  else {
    __ptr[1] = 0;
    iVar2 = strcasecmp(__s1,".nds");
    if (iVar2 == 0) {
      *__ptr = (long)__fd;
      *(undefined4 *)(__ptr + 3) = 0;
      _Var5 = lseek(__fd,0,2);
      *(undefined1 *)((long)__ptr + 0x1c) = 1;
      __ptr[2] = CONCAT44((int)_Var5,(int)_Var5);
      if (param_3 == 0) {
        lseek(__fd,0,0);
        // #region agent log
        debug_log("nds_file_open:before_mmap1", "Before first mmap", "C", nullptr, *(uint *)((long)__ptr + 0x14), 0);
        // #endregion
        pvVar4 = mmap((void *)0x0,(ulong)*(uint *)((long)__ptr + 0x14),1,iVar3,__fd,0);
        // #region agent log
        debug_log("nds_file_open:after_mmap1", "After first mmap", "C", pvVar4, (pvVar4 == (void *)0xffffffffffffffff) ? 1 : 0, 0);
        // #endregion
        __ptr[1] = (long)pvVar4;
        if (pvVar4 != (void *)0xffffffffffffffff) goto LAB_00175820;
        puts("Could not mmap cached ROM.");
        puts("Trying again with trimming enabled.");
      }
      lseek(__fd,0x80,0);
      sVar6 = read(__fd,(void *)((long)__ptr + 0x14),4);
      if (sVar6 < 1) {
        puts("ERROR: could not read trim size from header");
        uVar10 = *(uint *)((long)__ptr + 0x14);
      }
      else {
        uVar10 = *(uint *)((long)__ptr + 0x14);
        if ((uVar10 == 0) || (*(uint *)(__ptr + 2) < uVar10)) {
          printf("WARNING: invalid trim size %08x/%08x\n");
          uVar10 = *(uint *)(__ptr + 2);
          *(uint *)((long)__ptr + 0x14) = uVar10;
        }
      }
      printf("Trimmed ROM size to %d bytes.\n",uVar10);
      lseek(__fd,0,0);
      // #region agent log
      debug_log("nds_file_open:before_mmap2", "Before second mmap", "C", nullptr, uVar10, 0);
      // #endregion
      pvVar4 = mmap((void *)0x0,(ulong)*(uint *)((long)__ptr + 0x14),1,iVar3,__fd,0);
      // #region agent log
      debug_log("nds_file_open:after_mmap2", "After second mmap", "C", pvVar4, (pvVar4 == (void *)0xffffffffffffffff) ? 1 : 0, 0);
      // #endregion
      __ptr[1] = (long)pvVar4;
      if (pvVar4 != (void *)0xffffffffffffffff) goto LAB_00175820;
      puts("Could not mmap cached ROM.");
      puts("ERROR: Total failure on uncached ROM mmap.");
      goto LAB_0017598c;
    }
    close(__fd);
   }
    /*
    if (param_2 == 0) {
      __filename = "\0";
    }
    else {
      local_808[0] = '\0';
      local_808[1] = '\0';
      local_808[2] = '\0';
      local_808[3] = '\0';
      local_808[4] = '\0';
      local_808[5] = '\0';
      local_808[6] = '\0';
      local_808[7] = '\0';
      local_808[8] = '\0';
      local_808[9] = '\0';
      local_808[10] = '\0';
      local_808[0xb] = '\0';
      local_808[0xc] = '\0';
      local_808[0xd] = '\0';
      local_808[0xe] = '\0';
      local_808[0xf] = '\0';
      memset(local_808 + 0x10,0,0x3f0);
      local_408[0] = '\0';
      local_408[1] = '\0';
      local_408[2] = '\0';
      local_408[3] = '\0';
      local_408[4] = '\0';
      local_408[5] = '\0';
      local_408[6] = '\0';
      local_408[7] = '\0';
      local_408[8] = '\0';
      local_408[9] = '\0';
      local_408[10] = '\0';
      local_408[0xb] = '\0';
      local_408[0xc] = '\0';
      local_408[0xd] = '\0';
      local_408[0xe] = '\0';
      local_408[0xf] = '\0';
      memset(local_408 + 0x10,0,0x3f0);
//      __filename = acStack_c08[0].__unused + 1;
      snprintf(__filename,0x400,"%s%cunzipped_rom.nds",param_2,0x2f);
      snprintf(acStack_c08,0x400,"%s%ccache_info",param_2,0x2f);
      iVar3 = __xstat(0,(char *)__filename,acStack_c08);
      if ((iVar3 == 0) && (pFVar9 = fopen((char *)__filename,"rb"), pFVar9 != (FILE *)0x0)) {
        fseek(pFVar9,0,2);
        uVar7 = ftell(pFVar9);
        fclose(pFVar9);
        pFVar9 = fopen(acStack_c08,"rb");
        if (pFVar9 != (FILE *)0x0) {
          local_1098 = local_1098 & 0xffffffff00000000;
          local_1090 = 0;
          snprintf(local_808,0x3ff,"%s",param_1);
          local_409 = 0;
          sVar8 = fread(local_408,0x400,1,pFVar9);
          if (((((sVar8 == 0) || (sVar8 = fread(&local_1090,8,1,pFVar9), sVar8 == 0)) ||
               (sVar8 = fread(&local_1098,4,1,pFVar9), sVar8 == 0)) ||
              ((uVar7 != (local_1098 & 0xffffffff) || (acStack_c08[0].st_mtim.tv_sec != local_1090)
               ))) || (iVar3 = strcmp(local_408,local_808), iVar3 != 0)) {
            fclose(pFVar9);
          }
          else {
            printf("File %s already cached\n",param_1);
            fclose(pFVar9);
            iVar3 = nds_file_open_cached(__ptr,param_2,param_3,param_4);
            if (iVar3 == 0) goto LAB_00175820;
            puts("ERROR: could not open cached file");
          }
        }
      }
      __snprintf_chk(__filename,0x400,1,0x400,"%s%cunzipped_rom.nds",param_2,0x2f);
    }
    iVar3 = strcasecmp(__s1,".zip");
    if (iVar3 == 0) {
      *(undefined4 *)(__ptr + 3) = 1;
      pvVar4 = (void *)unzip_file(param_1,&DAT_00220328,__ptr + 2,__filename);
      __ptr[1] = (long)pvVar4;
LAB_00175764:
      if ((pvVar4 != (void *)0x0) && (uVar10 = *(uint *)(__ptr + 2), uVar10 != 0)) {
        *(uint *)((long)__ptr + 0x14) = uVar10;
        *(undefined1 *)((long)__ptr + 0x1c) = 0;
        if (param_2 == 0) {
          if (param_3 != 0) {
            uVar1 = *(uint *)((long)pvVar4 + 0x80);
            *(uint *)((long)__ptr + 0x14) = uVar1;
            if (uVar1 == 0 || uVar10 < uVar1) {
              printf("WARNING: invalid trim size: %08x\n");
              *(int *)((long)__ptr + 0x14) = (int)__ptr[2];
            }
            else {
              pvVar4 = realloc(pvVar4,(ulong)uVar1);
              __ptr[1] = (long)pvVar4;
              printf("Trimmed ROM size to %d bytes. (compressed in RAM)\n",
                           *(undefined4 *)((long)__ptr + 0x14));
            }
          }
          goto LAB_00175820;
        }
        local_408[0] = '\0';
        local_408[1] = '\0';
        local_408[2] = '\0';
        local_408[3] = '\0';
        local_408[4] = '\0';
        local_408[5] = '\0';
        local_408[6] = '\0';
        local_408[7] = '\0';
        local_408[8] = '\0';
        local_408[9] = '\0';
        local_408[10] = '\0';
        local_408[0xb] = '\0';
        local_408[0xc] = '\0';
        local_408[0xd] = '\0';
        local_408[0xe] = '\0';
        local_408[0xf] = '\0';
        memset(local_408 + 0x10,0,0x3f0);
        snprintf(local_808,0x400,"%s%cunzipped_rom.nds",param_2,0x2f);
        snprintf(acStack_c08,0x400,"%s%ccache_info",param_2,0x2f);
        iVar3 = __xstat(0,local_808,acStack_c08);
        if ((iVar3 == 0) && (pFVar9 = fopen(local_808,"rb"), pFVar9 != (FILE *)0x0)) {
          fseek(pFVar9,0,2);
          local_1098 = ftell(pFVar9);
          fclose(pFVar9);
          if (local_1098 != uVar10) {
            puts("ERROR: can\'t write cache info: file size mismatch");
            goto LAB_001757f0;
          }
          pFVar9 = fopen(acStack_c08,"wb");
          if (pFVar9 == (FILE *)0x0) goto LAB_001757f0;
          local_1090 = acStack_c08[0].st_mtim.tv_sec;
          snprintf(local_408,0x3ff,"%s",param_1);
          local_9 = 0;
          fwrite(local_408,0x400,1,pFVar9);
          fwrite(&local_1090,8,1,pFVar9);
          fwrite(&local_1098,4,1,pFVar9);
          fclose(pFVar9);
          printf("Wrote %s to cache\n",param_1);
        }
        else {
LAB_001757f0:
          puts("ERROR: could not write cache info");
        }
        iVar3 = nds_file_open_cached(__ptr,param_2,param_3,param_4);
        if (iVar3 == 0) goto LAB_00175820;
      }
    }
    else {
      iVar3 = strcasecmp(__s1,".7z");
      if (iVar3 == 0) {
        *(undefined4 *)(__ptr + 3) = 2;
        pvVar4 = (void *)un7z_file(param_1,&DAT_00220328,__ptr + 2,__filename);
        __ptr[1] = (long)pvVar4;
        goto LAB_00175764;
      }
      iVar3 = strcasecmp(__s1,".rar");
      if (iVar3 == 0) {
        *(undefined4 *)(__ptr + 3) = 3;
        pvVar4 = (void *)unrar_file(param_1,&DAT_00220328,__ptr + 2,__filename);
        __ptr[1] = (long)pvVar4;
        goto LAB_00175764;
      }
    }
  }
  
  free(__ptr);
  __ptr = (long *)0x0;
  */
LAB_00175820:
  if (local_8 - __stack_chk_guard == 0) {
    return __ptr;
  }
                    
  __stack_chk_fail();
}

// --- 函数: nds_file_close ---

void nds_file_close(int *param_1)

{
  if (param_1 == (int *)0x0) {
    return;
  }
  if ((char)param_1[7] == '\0') {
    free(*(void **)(param_1 + 2));
    free(param_1);
    return;
  }
  munmap(*(void **)(param_1 + 2),(ulong)(uint)param_1[5]);
  close(*param_1);
  free(param_1);
  return;
}

// --- 函数: close ---

int _close(int __fd)

{
  int iVar1;
  
  iVar1 = close(__fd);
  return iVar1;
}

// --- 函数: lua_load_script ---

undefined4 lua_load_script(long param_1)

{

  return;
}

// --- 函数: fopen ---

//FILE * fopen(char *__filename,char *__modes)

//{
//  FILE *pFVar1;
  
//  pFVar1 = fopen(__filename,__modes);
//  return pFVar1;
//}

// --- 函数: lua_on_load_game ---

void lua_on_load_game(undefined8 param_1)

{
  return;
}

// --- 函数: fclose ---

int _fclose(FILE *__stream)

{
  int iVar1;
  
  iVar1 = fclose(__stream);
  return iVar1;
}

// --- 函数: strncpy ---

char * _strncpy(char *__dest,char *__src,size_t __n)

{
  char *pcVar1;
  
  pcVar1 = strncpy(__dest,__src,__n);
  return pcVar1;
}

// --- 函数: memcpy ---

void * _memcpy(void *__dest,void *__src,size_t __n)

{
  void *pvVar1;
  
  pvVar1 = memcpy(__dest,__src,__n);
  return pvVar1;
}

// --- 函数: strrchr ---

char * _strrchr(char *__s,int __c)

{
  char *pcVar1;
  
  pcVar1 = strrchr(__s,__c);
  return pcVar1;
}

// --- 函数: getcwd ---

char * _getcwd(char *__buf,size_t __size)

{
  char *pcVar1;
  
  pcVar1 = getcwd(__buf,__size);
  return pcVar1;
}

// --- 函数: fwrite ---

size_t _fwrite(void *__ptr,size_t __size,size_t __n,FILE *__s)

{
  size_t sVar1;
  
  sVar1 = fwrite(__ptr,__size,__n,__s);
  return sVar1;
}

// --- 函数: free ---

void _free(void *__ptr)

{
  free(__ptr);
  return;
}

// --- 函数: clear_screen ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void clear_screen(void)

{
  undefined4 local_28;
  undefined4 uStack_24;
  undefined1 auStack_20 [4];
  undefined4 local_1c;
  undefined4 uStack_18;
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  //SDL_GetCurrentDisplayMode(0,auStack_20,0);
  SDL_RenderGetLogicalSize(DAT_04031578,&local_28,&uStack_24);
  SDL_RenderSetLogicalSize(DAT_04031578,local_1c,uStack_18);
  SDL_SetRenderDrawColor(DAT_04031578,0,0,0,0xffffffff);
  SDL_RenderClear(DAT_04031578);
  SDL_RenderPresent(DAT_04031578);
  SDL_RenderClear(DAT_04031578);
  SDL_RenderPresent(DAT_04031578);
  SDL_RenderClear(DAT_04031578);
  SDL_RenderPresent(DAT_04031578);
  SDL_RenderClear(DAT_04031578);
  SDL_RenderPresent(DAT_04031578);
  SDL_RenderSetLogicalSize(DAT_04031578,local_28,uStack_24);
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}
*/

// --- 函数: set_screen_hires_mode ---

void set_screen_hires_mode(uint param_1,uint param_2)

{
  bool bVar1;
  undefined8 uVar2;
  void *pvVar3;
  int iVar4;
  long lVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  ulong uVar8;
  
  uVar8 = (ulong)(param_1 ^ (uint)DAT_040315cc);
  if ((byte)(&DAT_04031541)[uVar8 * 0x28] != param_2) {
    bVar1 = param_2 != 0;
    iVar4 = 0xc000;
    if (bVar1) {
      iVar4 = 0x30000;
    }
    uVar6 = 0x100;
    if (bVar1) {
      uVar6 = 0x200;
    }
    uVar7 = 0xc0;
    if (bVar1) {
      uVar7 = 0x180;
    }
    if ((&SDL_screen)[uVar8 * 5] != 0) {
      //SDL_DestroyTexture();
      return;
    }
    uVar2 = SDL_CreateTexture(DAT_04031578,DAT_040315a0,1,uVar6,uVar7);
    lVar5 = uVar8 * 0x28;
    (&SDL_screen)[uVar8 * 5] = uVar2;
    // 安全检查：防止异常大的内存分配
    ulong alloc_size = (ulong)(uint)(iVar4 * DAT_040315a8);
    if (alloc_size > 0x10000000) {  // 256MB 限制
      fprintf(stderr, "ERROR: set_screen_hires_mode: Invalid memory allocation size: %lu bytes (iVar4=%d, DAT_040315a8=%d)\n", 
              alloc_size, iVar4, DAT_040315a8);
      fflush(stderr);
      // 使用安全的默认值：假设 DAT_040315a8 应该是 2 或 4
      DAT_040315a8 = 2;  // 默认使用 2 字节每像素
      alloc_size = (ulong)(uint)(iVar4 * DAT_040315a8);
      fprintf(stderr, "WARNING: Using default DAT_040315a8=2, new allocation size: %lu bytes\n", alloc_size);
      fflush(stderr);
    }
    void *old_ptr = *(void **)(&DAT_04031528 + lVar5);
    // 安全检查：检查指针是否有效
    // 如果指针是 NULL 或看起来不像有效的堆指针（小于 0x100000），使用 malloc
    // 有效的堆指针通常在较高的地址（> 0x100000）
    if (old_ptr == (void *)0x0 || (ulong)old_ptr < 0x100000) {
      if (old_ptr != (void *)0x0 && (ulong)old_ptr < 0x100000) {
        fprintf(stderr, "WARNING: set_screen_hires_mode: Invalid pointer value 0x%lx at offset %ld, using malloc instead of realloc\n", 
                (ulong)old_ptr, lVar5);
        fflush(stderr);
      }
      pvVar3 = malloc(alloc_size);
      if (pvVar3 == (void *)0x0) {
        fprintf(stderr, "ERROR: set_screen_hires_mode: malloc failed for %lu bytes\n", alloc_size);
        fflush(stderr);
      }
      *(void **)(&DAT_04031528 + lVar5) = pvVar3;
    }
    else {
      pvVar3 = realloc(old_ptr, alloc_size);
      if (pvVar3 == (void *)0x0) {
        fprintf(stderr, "ERROR: set_screen_hires_mode: realloc failed for %lu bytes\n", alloc_size);
        fflush(stderr);
      }
      *(void **)(&DAT_04031528 + lVar5) = pvVar3;
    }
    (&DAT_04031541)[uVar8 * 0x28] = (char)param_2;
  }
  return;
}
/*
// --- 函数: SDL_CreateTexture ---

void SDL_CreateTexture(void)

{
  SDL_CreateTexture();
  return;
}

// --- 函数: SDL_SetRenderDrawBlendMode ---

void SDL_SetRenderDrawBlendMode(void)

{
  SDL_SetRenderDrawBlendMode();
  return;
}

// --- 函数: SDL_SetTextureBlendMode ---

void SDL_SetTextureBlendMode(void)

{
  SDL_SetTextureBlendMode();
  return;
}

// --- 函数: SDL_SetHint ---

void SDL_SetHint(void)

{
  SDL_SetHint();
  return;
}

// --- 函数: SDL_UpdateTexture ---

void SDL_UpdateTexture(void)

{
  SDL_UpdateTexture();
  return;
}

// --- 函数: SDL_CreateWindow ---

void SDL_CreateWindow(void)

{
  SDL_CreateWindow();
  return;
}

// --- 函数: SDL_CreateRenderer ---

void SDL_CreateRenderer(void)

{
  SDL_CreateRenderer();
  return;
}

// --- 函数: SDL_SetWindowFullscreen ---

void SDL_SetWindowFullscreen(void)

{
  SDL_SetWindowFullscreen();
  return;
}

// --- 函数: SDL_RenderSetLogicalSize ---

void SDL_RenderSetLogicalSize(void)

{
  SDL_RenderSetLogicalSize();
  return;
}

// --- 函数: SDL_GetCurrentDisplayMode ---

void SDL_GetCurrentDisplayMode(void)

{
  SDL_GetCurrentDisplayMode();
  return;
}

// --- 函数: SDL_SetWindowSize ---

void SDL_SetWindowSize(void)

{
  SDL_SetWindowSize();
  return;
}

*/

// --- 函数: event_force_task_switch_function ---

void event_force_task_switch_function(long param_1)

{
  long lVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  uint uVar5;
  
  lVar1 = param_1 + 0x78;
  uVar5 = 0x80 - ((uint)*(undefined8 *)(param_1 + 8) & 0x7f);
  if (*(uint **)(param_1 + 0x318) == (uint *)0x0) {
    *(uint *)(param_1 + 0x78) = uVar5;
    *(undefined8 *)(param_1 + 0x90) = 0;
    *(undefined8 *)(param_1 + 0x98) = 0;
    *(long *)(param_1 + 0x318) = lVar1;
    return;
  }
  puVar3 = (uint *)0x0;
  puVar2 = *(uint **)(param_1 + 0x318);
  do {
    puVar4 = puVar2;
    if (uVar5 <= *puVar4) {
      *(uint *)(param_1 + 0x78) = uVar5;
      *(uint **)(param_1 + 0x90) = puVar4;
      *(uint **)(param_1 + 0x98) = puVar3;
      if (puVar3 == (uint *)0x0) {
        *(long *)(param_1 + 0x318) = lVar1;
      }
      else {
        *(long *)(puVar3 + 6) = lVar1;
      }
      *(long *)(puVar4 + 8) = lVar1;
      *puVar4 = *puVar4 - uVar5;
      return;
    }
    uVar5 = uVar5 - *puVar4;
    puVar3 = puVar4;
    puVar2 = *(uint **)(puVar4 + 6);
  } while (*(uint **)(puVar4 + 6) != (uint *)0x0);
  *(uint *)(param_1 + 0x78) = uVar5;
  *(undefined8 *)(param_1 + 0x90) = 0;
  *(uint **)(param_1 + 0x98) = puVar4;
  *(long *)(puVar4 + 6) = lVar1;
  return;
}


void audio_reset_buffer(void *param_1)

{
  *(undefined8 *)((long)param_1 + 0x40008) = 0;
  memset(param_1,0,0x20000);
  return;
}

// --- 函数: reset_spu ---

void reset_spu(long param_1)

{
  long lVar1;
  int iVar2;
  long lVar3;
  undefined1 auStack_828 [2080];
  long local_8;
  
  lVar3 = *(long *)(param_1 + 0x40cf0);
  *(undefined8 *)(param_1 + 0x400a8) = 0;
  *(undefined4 *)(param_1 + 0x400b8) = 0;
  *(undefined8 *)(param_1 + 0x400c8) = 0;
  *(undefined4 *)(param_1 + 0x400d0) = 0;
  *(undefined4 *)(param_1 + 0x400dc) = 0;
  *(undefined2 *)(param_1 + 0x400e0) = 0;
  *(undefined2 *)(param_1 + 0x400e5) = 3;
  *(undefined1 *)(param_1 + 0x400e8) = 0;
  *(undefined8 *)(param_1 + 0x40170) = 0;
  *(undefined4 *)(param_1 + 0x40180) = 0;
  *(undefined8 *)(param_1 + 0x40190) = 0;
  *(undefined4 *)(param_1 + 0x40198) = 0;
  *(undefined4 *)(param_1 + 0x401a4) = 0;
  *(undefined2 *)(param_1 + 0x401a8) = 0;
  *(undefined1 *)(param_1 + 0x401b0) = 0;
  *(undefined2 *)(param_1 + 0x401ad) = 3;
  *(undefined8 *)(param_1 + 0x40238) = 0;
  *(undefined4 *)(param_1 + 0x40248) = 0;
  *(undefined8 *)(param_1 + 0x40258) = 0;
  *(undefined4 *)(param_1 + 0x40260) = 0;
  *(undefined4 *)(param_1 + 0x4026c) = 0;
  *(undefined2 *)(param_1 + 0x40270) = 0;
  *(undefined1 *)(param_1 + 0x40278) = 0;
  *(undefined2 *)(param_1 + 0x40275) = 3;
  *(undefined8 *)(param_1 + 0x40300) = 0;
  *(undefined4 *)(param_1 + 0x40310) = 0;
  *(undefined8 *)(param_1 + 0x40320) = 0;
  *(undefined4 *)(param_1 + 0x40328) = 0;
  *(undefined4 *)(param_1 + 0x40334) = 0;
  *(undefined2 *)(param_1 + 0x40338) = 0;
  *(undefined1 *)(param_1 + 0x40340) = 0;
  *(undefined2 *)(param_1 + 0x4033d) = 3;
  *(undefined8 *)(param_1 + 0x403c8) = 0;
  *(undefined4 *)(param_1 + 0x403d8) = 0;
  *(undefined8 *)(param_1 + 0x403e8) = 0;
  *(undefined4 *)(param_1 + 0x403f0) = 0;
  *(undefined4 *)(param_1 + 0x403fc) = 0;
  *(undefined2 *)(param_1 + 0x40400) = 0;
  *(undefined1 *)(param_1 + 0x40408) = 0;
  local_8 = (long)__stack_chk_guard;
  *(undefined2 *)(param_1 + 0x40405) = 3;
  *(undefined8 *)(param_1 + 0x40490) = 0;
  *(undefined4 *)(param_1 + 0x404a0) = 0;
  *(undefined8 *)(param_1 + 0x404b0) = 0;
  *(undefined4 *)(param_1 + 0x404b8) = 0;
  *(undefined4 *)(param_1 + 0x404c4) = 0;
  *(undefined2 *)(param_1 + 0x404c8) = 0;
  *(undefined1 *)(param_1 + 0x404d0) = 0;
  *(undefined2 *)(param_1 + 0x404cd) = 3;
  *(undefined8 *)(param_1 + 0x40558) = 0;
  *(undefined4 *)(param_1 + 0x40568) = 0;
  *(undefined8 *)(param_1 + 0x40578) = 0;
  *(undefined4 *)(param_1 + 0x40580) = 0;
  *(undefined4 *)(param_1 + 0x4058c) = 0;
  *(undefined2 *)(param_1 + 0x40590) = 0;
  *(undefined1 *)(param_1 + 0x40598) = 0;
  *(undefined2 *)(param_1 + 0x40595) = 3;
  *(undefined8 *)(param_1 + 0x40620) = 0;
  *(undefined4 *)(param_1 + 0x40630) = 0;
  *(undefined8 *)(param_1 + 0x40640) = 0;
  *(undefined4 *)(param_1 + 0x40648) = 0;
  *(undefined4 *)(param_1 + 0x40654) = 0;
  *(undefined2 *)(param_1 + 0x40658) = 0;
  *(undefined1 *)(param_1 + 0x40660) = 0;
  *(undefined2 *)(param_1 + 0x4065d) = 3;
  *(undefined8 *)(param_1 + 0x406e8) = 0;
  *(undefined4 *)(param_1 + 0x406f8) = 0;
  *(undefined8 *)(param_1 + 0x40708) = 0;
  *(undefined4 *)(param_1 + 0x40710) = 0;
  *(undefined4 *)(param_1 + 0x4071c) = 0;
  *(undefined2 *)(param_1 + 0x40720) = 0;
  *(undefined1 *)(param_1 + 0x40728) = 0;
  *(undefined2 *)(param_1 + 0x40725) = 3;
  *(undefined8 *)(param_1 + 0x407b0) = 0;
  *(undefined4 *)(param_1 + 0x407c0) = 0;
  *(undefined8 *)(param_1 + 0x407d0) = 0;
  *(undefined4 *)(param_1 + 0x407d8) = 0;
  *(undefined4 *)(param_1 + 0x407e4) = 0;
  *(undefined2 *)(param_1 + 0x407e8) = 0;
  *(undefined1 *)(param_1 + 0x407f0) = 0;
  *(undefined2 *)(param_1 + 0x407ed) = 3;
  *(undefined8 *)(param_1 + 0x40878) = 0;
  *(undefined4 *)(param_1 + 0x40888) = 0;
  *(undefined8 *)(param_1 + 0x40898) = 0;
  *(undefined4 *)(param_1 + 0x408a0) = 0;
  *(undefined4 *)(param_1 + 0x408ac) = 0;
  *(undefined2 *)(param_1 + 0x408b0) = 0;
  *(undefined1 *)(param_1 + 0x408b8) = 0;
  *(undefined2 *)(param_1 + 0x408b5) = 3;
  *(undefined8 *)(param_1 + 0x40940) = 0;
  *(undefined4 *)(param_1 + 0x40950) = 0;
  *(undefined8 *)(param_1 + 0x40960) = 0;
  *(undefined4 *)(param_1 + 0x40968) = 0;
  *(undefined4 *)(param_1 + 0x40974) = 0;
  *(undefined2 *)(param_1 + 0x40978) = 0;
  *(undefined1 *)(param_1 + 0x40980) = 0;
  *(undefined2 *)(param_1 + 0x4097d) = 3;
  *(undefined8 *)(param_1 + 0x40a08) = 0;
  *(undefined4 *)(param_1 + 0x40a18) = 0;
  *(undefined8 *)(param_1 + 0x40a28) = 0;
  *(undefined4 *)(param_1 + 0x40a30) = 0;
  *(undefined4 *)(param_1 + 0x40a3c) = 0;
  *(undefined2 *)(param_1 + 0x40a40) = 0;
  *(undefined1 *)(param_1 + 0x40a48) = 0;
  *(undefined2 *)(param_1 + 0x40a45) = 3;
  *(undefined8 *)(param_1 + 0x40ad0) = 0;
  *(undefined4 *)(param_1 + 0x40ae0) = 0;
  *(undefined8 *)(param_1 + 0x40af0) = 0;
  *(undefined4 *)(param_1 + 0x40af8) = 0;
  *(undefined4 *)(param_1 + 0x40b04) = 0;
  *(undefined2 *)(param_1 + 0x40b08) = 0;
  *(undefined1 *)(param_1 + 0x40b10) = 0;
  *(undefined2 *)(param_1 + 0x40b0d) = 3;
  *(undefined8 *)(param_1 + 0x40b98) = 0;
  *(undefined4 *)(param_1 + 0x40ba8) = 0;
  *(undefined8 *)(param_1 + 0x40bb8) = 0;
  *(undefined4 *)(param_1 + 0x40bc0) = 0;
  *(undefined4 *)(param_1 + 0x40bcc) = 0;
  *(undefined2 *)(param_1 + 0x40bd0) = 0;
  *(undefined1 *)(param_1 + 0x40bd8) = 0;
  *(undefined2 *)(param_1 + 0x40bd5) = 3;
  *(undefined8 *)(param_1 + 0x40c60) = 0;
  *(undefined4 *)(param_1 + 0x40c70) = 0;
  *(undefined8 *)(param_1 + 0x40c80) = 0;
  *(undefined4 *)(param_1 + 0x40c88) = 0;
  *(undefined4 *)(param_1 + 0x40c94) = 0;
  *(undefined2 *)(param_1 + 0x40c98) = 0;
  *(undefined1 *)(param_1 + 0x40ca0) = 0;
  *(undefined2 *)(param_1 + 0x40c9d) = 3;
  audio_reset_buffer(param_1);
  *(undefined1 *)(param_1 + 0x40cc4) = 0;
  *(undefined1 *)(param_1 + 0x40ce4) = 0;
  *(undefined8 *)(param_1 + 0x40d00) = 0;
  *(undefined4 *)(param_1 + 0x40d1c) = 0;
  *(undefined4 *)(param_1 + 0x40d24) = 0;
  *(undefined1 *)(param_1 + 0x40d40) = 0;
  if (*(void **)(param_1 + 0x40d30) != (void *)0x0) {
    free(*(void **)(param_1 + 0x40d30));
    *(undefined8 *)(param_1 + 0x40d30) = 0;
  }
  lVar1 = lVar3 + 0x8a780;
  snprintf((char*)auStack_828,0x820,"%s%cmicrophone%c%s.wav",(char*)lVar1,0x2f,0x2f,(char*)(lVar3 + 0x8b380));
  spu_load_fake_microphone_data(param_1,auStack_828);
  /*
  iVar2 = spu_load_fake_microphone_data(param_1,auStack_828);
  if (iVar2 == -1) {
    snprintf(auStack_828,0x820,"%s%cmicrophone%cmicrophone.wav",lVar1,0x2f);
    iVar2 = spu_load_fake_microphone_data(param_1,auStack_828);
    if (iVar2 == 0) {
      puts(" Loaded default fake microphone audio file.");
    }
  }
  else {
    puts(" Loaded game specific fake microphone audio file.");
  }
  */
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}


uint memory_region_block_memory_load
               (long param_1,undefined8 param_2,uint param_3,undefined4 *param_4,uint param_5)

{
  uint uVar1;
  long lVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  undefined8 uVar6;
  void *__dest;
  long lVar7;
  void *extraout_x12;
  undefined4 *puVar8;
  code *pcVar9;
  uint local_14;
  
  if (param_5 == 0) {
    return 0;
  }
  pcVar9 = (code *)0x0;
  local_14 = 0;
  do {
    while( true ) {
      lVar7 = (ulong)(param_3 >> 0x17) * 0x60;
      lVar2 = param_1 + lVar7;
      uVar5 = *(uint *)(param_1 + lVar7);
      lVar7 = (**(code3 **)(lVar2 + 0x48))(param_2,lVar2,param_3);
      uVar6 = (**(code3 **)(lVar2 + 0x50))(param_2,lVar2,param_3);
      cVar4 = *(char *)(lVar2 + 0x59);
      uVar1 = uVar5 + 1;
      if (param_5 < uVar5 + 1) {
        uVar1 = param_5;
      }
      param_5 = param_5 - uVar1;
      if (cVar4 == '\x01') break;
      /*
      if (cVar4 != '\x02') {
        if (cVar4 == '\0') {
          __dest = (void *)(*(long *)(lVar2 + 0x20) + (ulong)(uVar5 & param_3));
          goto LAB_001136f0;
        }
        printf(1,"can\'t get ptr @ %x type is %x!!!\n",param_3);
        goto LAB_001136b4;
      }
      pcVar9 = *(code **)(lVar2 + 0x30);
      if (pcVar9 != (code *)0x0) goto LAB_00113740;
LAB_001136b8:
      param_3 = param_3 + uVar1;
      param_4 = (undefined4 *)((long)param_4 + (ulong)uVar1);
joined_r0x00113770:
      if (param_5 == 0) {
        return local_14;
      }
      */
    }
    //__dest = (void *)(**(code2 **)(lVar2 + 0x20))(param_2,param_3);
    /*
LAB_001136f0:
    if (__dest == (void *)0x0) {
LAB_001136b4:
      if (pcVar9 == (code *)0x0) goto LAB_001136b8;
LAB_00113740:
      if (uVar1 != 0) {
        uVar1 = param_3 + uVar1;
        puVar8 = param_4;
        do {
          param_4 = puVar8 + 1;
          uVar3 = uVar5 & param_3;
          param_3 = param_3 + 4;
          //(*pcVar9)(param_2,uVar3,*puVar8);
          puVar8 = param_4;
        } while (param_3 != uVar1);
      }
      goto joined_r0x00113770;
    }
    */
    if (lVar7 != 0) {
      uVar5 = memory_check_code_region(lVar7,uVar6,param_3,uVar1);
      local_14 = local_14 | uVar5;
      __dest = extraout_x12;
    }
    param_3 = param_3 + uVar1;
    memcpy(__dest,param_4,(ulong)uVar1);
    param_4 = (undefined4 *)((long)param_4 + (ulong)uVar1);
    if (param_5 == 0) {
      return local_14;
    }
  } while( true );
}

// --- 函数: gamecard_load_program ---

undefined8 gamecard_load_program(long param_1)

{
  long lVar1;
  long lVar2;
  ulong uVar3;
  uint uVar4;
  uint uVar5;
  byte bVar6;
  char cVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  undefined4 uVar11;
  int iVar12;
  undefined8 *puVar13;
  void *pvVar14;
  char *__s;
  long lVar15;
  long lVar16;
  void *__src;
  uint local_a84;
  char acStack_a38 [12];
  undefined1 local_a2c;
  char acStack_a28 [0x200];  // 修复：从 12 字节改为 0x200 字节，因为 memcpy 需要复制 0x200 字节
  uint local_a1c;
  undefined2 local_a18;
  undefined1 local_a16;
  byte local_a14;
  undefined1 local_a0a;
  undefined1 local_a09;
  uint local_a08;
  undefined4 uStack_a04;
  uint local_a00;
  undefined4 uStack_9fc;
  uint local_9f8;
  undefined4 local_9f4;
  undefined4 uStack_9f0;
  undefined4 local_9ec;
  undefined4 local_9c8;
  ushort local_992;
  long local_988;
  char local_980;
  char acStack_97c [178];
  undefined2 local_8ca;
  undefined2 local_82a;
  undefined1 auStack_828 [2080];
  long local_8;
  
  lVar16 = *(long *)(param_1 + 0x918);
  // 修复：lVar16 是 nds_system 的绝对地址（从 libretro.c 设置），需要转换为相对于 nds_system 的偏移量
  // 因为代码中有些地方使用 lVar16 作为绝对地址（如 lVar16 + 0x85a5c），有些地方使用它作为偏移量（如 nds_system + lVar16 + 0x30d8930）
  // 我们需要统一处理：如果 lVar16 是绝对地址，转换为偏移量；否则直接使用
  long lVar16_offset;
  if (lVar16 >= (long)nds_system && lVar16 < (long)nds_system + 62042112) {
    // lVar16 是绝对地址，转换为偏移量
    lVar16_offset = (long)((char*)lVar16 - (char*)nds_system);
  } else {
    // lVar16 可能已经是偏移量，直接使用
    lVar16_offset = lVar16;
  }
  lVar15 = *(long *)(nds_system + lVar16_offset + 0x30d8930);
  local_a84 = *(uint *)(nds_system + lVar16_offset + 0x85a5c);
  __src = *(void **)(*(long *)(param_1 + 0x920) + 8);
  local_8 = (long)__stack_chk_guard;
  uVar4 = *(uint *)(*(long *)(param_1 + 0x920) + 0x10);
  memcpy(acStack_a28,__src,0x200);
  strncpy(acStack_a38,acStack_a28,0xc);
  uVar9 = local_a1c;
  uVar10 = 0x20000 << (ulong)(local_a14 & 0x1f);
  iVar12 = uVar10 - 1;
  local_a2c = 0;
  *(int *)(param_1 + 0x930) = iVar12;
  *(uint *)(param_1 + 0x938) = local_a1c;
  lVar1 = nds_system + lVar16_offset + 0x35d4930;
  if (uVar10 < uVar4) {
    do {
      iVar8 = iVar12 * 2;
      iVar12 = iVar8 + 1;
    } while (iVar8 + 2U < uVar4);
    *(int *)(param_1 + 0x930) = iVar12;
  }
  if (*(int *)(nds_system + lVar16_offset + 0x85a30) != 0) {
    *(int *)(param_1 + 0x930) = (1 << (ulong)(-(int)LZCOUNT(uVar4 + 1) & 0x1f)) + -1;
    printf("Ignoring gamecard header mask: using game card mask of %x\n");
  }
  printf("Gamecard title:  \'%s\'\n",acStack_a38);
  printf("Game code:       %08x (%c%c%c%c)\n",uVar9,uVar9 & 0xff,uVar9 >> 8 & 0xff,
               uVar9 >> 0x10 & 0xff,uVar9 >> 0x18);
  printf("Maker code:      %02x\n",local_a18);
  printf("Unit code:       %01x\n",local_a16);
  printf("Chip size:       %dKB\n",0x80 << (ulong)(local_a14 & 0x1f));
  printf("ROM version:     %d\n",local_a0a);
  printf("Autostart:       %d\n",local_a09);
  printf("ROMCTRL          %08x\n",local_9c8);
  if (uVar9 == 0x23232323) {
    gamecard_initialize_homebrew(param_1);
  }
  else {
    *(undefined4 *)(param_1 + 0x93c) = 0xffffffff;
  }
  *(undefined4 *)(param_1 + 0x940) = 0;
  uVar10 = game_database_generate_crc32_from_memory(acStack_a28,0x200);
  uVar3 = (ulong)local_a00;
  printf("ARM9 ROM offset: %04x\n",local_a08);
  printf("ARM9 entry PC:   %04x\n",uStack_a04);
  printf("ARM9 RAM offset: %04x\n",uVar3);
  printf("ARM9 size:       %04x\n",uStack_9fc);
  printf("ARM7 ROM offset: %04x\n",local_9f8);
  printf("ARM7 entry PC:   %04x\n",local_9f4);
  printf("ARM7 RAM offset: %04x\n",uStack_9f0);
  printf("ARM7 size:       %04x\n",local_9ec);
  // 安全检查：验证地址是否在有效范围内
  unsigned long nds_system_addr = (unsigned long)nds_system;
  unsigned long nds_system_end = nds_system_addr + 62042112;
  unsigned long addr1_offset = lVar16_offset + 0x31d43b8;
  unsigned long addr2_offset = lVar16_offset + 0x31d43c0;
  
  if (addr1_offset >= 62042112 || addr2_offset >= 62042112) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Offset out of range (lVar16_offset=0x%lx, addr1_offset=0x%lx, addr2_offset=0x%lx)\n",
            lVar16_offset, addr1_offset, addr2_offset);
    fflush(stderr);
    return 0xffffffff;
  }
  
  long *ptr1 = (long *)(nds_system + addr1_offset);
  long *ptr2 = (long *)(nds_system + addr2_offset);
  
  if ((unsigned long)ptr1 < nds_system_addr || (unsigned long)ptr1 + sizeof(long) > nds_system_end ||
      (unsigned long)ptr2 < nds_system_addr || (unsigned long)ptr2 + sizeof(long) > nds_system_end) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Pointer out of range (ptr1=0x%lx, ptr2=0x%lx, nds_system=0x%lx-0x%lx)\n",
            (unsigned long)ptr1, (unsigned long)ptr2, nds_system_addr, nds_system_end);
    fflush(stderr);
    return 0xffffffff;
  }
  
  long val1 = *ptr1;
  long val2 = *ptr2;
  
  // 如果指针值为 NULL，尝试从 ROM 头中读取 ARM9 和 ARM7 的 RAM 偏移量来初始化它们
  if (val1 == 0 || val2 == 0) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: WARNING: Pointer value is NULL (val1=0x%lx, val2=0x%lx), attempting to initialize from ROM header\n",
            (unsigned long)val1, (unsigned long)val2);
    fprintf(stderr, "[DRASTIC] gamecard_load_program: ROM header info - ARM9_RAM_offset=0x%x, ARM7_RAM_offset=0x%x, ARM7_size=0x%x\n",
            uVar3, uStack_9f0, local_9ec);
    fflush(stderr);
    
    // 从 ROM 头中读取的 ARM9 和 ARM7 RAM 偏移量
    // uVar3 是 ARM9 RAM offset (从 ROM 头偏移 0x28 读取)
    // uStack_9f0 是 ARM7 RAM offset (从 ROM 头偏移 0x30 读取)
    if (val1 == 0 && uVar3 != 0) {
      val1 = (long)uVar3;
      *ptr1 = val1;
      fprintf(stderr, "[DRASTIC] gamecard_load_program: Initialized val1 from ARM9 RAM offset: 0x%lx\n", (unsigned long)val1);
      fflush(stderr);
    }
    
    // 对于 ARM7，如果大小为 0，这可能是一个特殊的 ROM 格式（如某些自制程序）
    // 在这种情况下，我们尝试使用 ARM7 RAM offset，如果也为 0，则使用默认值
    if (val2 == 0) {
      if (uStack_9f0 != 0) {
        val2 = (long)uStack_9f0;
        *ptr2 = val2;
        fprintf(stderr, "[DRASTIC] gamecard_load_program: Initialized val2 from ARM7 RAM offset: 0x%lx\n", (unsigned long)val2);
        fflush(stderr);
      } else if (local_9ec == 0) {
        // ARM7 大小为 0，且 RAM offset 也为 0，这可能是一个特殊的 ROM
        // 尝试使用 ARM9 的 RAM offset 作为后备（某些自制程序可能只使用 ARM9）
        if (uVar3 != 0) {
          val2 = (long)uVar3;
          *ptr2 = val2;
          fprintf(stderr, "[DRASTIC] gamecard_load_program: WARNING: ARM7 size is 0, using ARM9 RAM offset as fallback: 0x%lx\n", (unsigned long)val2);
          fprintf(stderr, "[DRASTIC] gamecard_load_program: NOTE: This ROM may be incomplete or use an unusual format. Emulation may not work correctly.\n");
          fflush(stderr);
        } else {
          fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Cannot initialize val2 - ARM7 size is 0, ARM7 RAM offset is 0, and ARM9 RAM offset is also 0\n");
          fflush(stderr);
        }
      }
    }
    
    // 如果初始化后仍然为 0，返回错误（但允许 ARM7 为 0 如果 ROM 大小也为 0）
    if (val1 == 0) {
      fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Failed to initialize ARM9 pointer (val1=0x%lx, ARM9_RAM_offset=0x%x)\n",
              (unsigned long)val1, uVar3);
      fflush(stderr);
      return 0xffffffff;
    }
    
    // ARM7 指针为 0 只有在 ARM7 大小也为 0 时才允许
    if (val2 == 0 && local_9ec != 0) {
      fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Failed to initialize ARM7 pointer (val2=0x%lx, ARM7_RAM_offset=0x%x, ARM7_size=0x%x)\n",
              (unsigned long)val2, uStack_9f0, local_9ec);
      fflush(stderr);
      return 0xffffffff;
    }
    
    // 如果 ARM7 大小为 0 且 val2 也为 0，发出警告但继续
    if (val2 == 0 && local_9ec == 0) {
      fprintf(stderr, "[DRASTIC] gamecard_load_program: WARNING: ARM7 pointer is 0 and ARM7 size is 0 - this ROM may be incomplete\n");
      fprintf(stderr, "[DRASTIC] gamecard_load_program: NOTE: Attempting to continue with ARM9 only. Emulation may fail.\n");
      fflush(stderr);
    }
  }
  
  long final_addr1 = val1 + 0x1000000;
  long final_addr2 = val2 + 0x1000000;
  
  if (final_addr1 == 0) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Final ARM9 address is NULL (final_addr1=0x%lx)\n",
            (unsigned long)final_addr1);
    fflush(stderr);
    return 0xffffffff;
  }
  
  // 如果 ARM7 大小为 0，允许 final_addr2 为 0（某些特殊 ROM 可能只有 ARM9）
  if (final_addr2 == 0 && local_9ec != 0) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: ERROR: Final ARM7 address is NULL but ARM7 size is non-zero (final_addr2=0x%lx, ARM7_size=0x%x)\n",
            (unsigned long)final_addr2, local_9ec);
    fflush(stderr);
    return 0xffffffff;
  }
  
  // 加载 ARM9 程序
  memory_region_block_memory_load
            (*(undefined8 *)final_addr1, lVar1, uVar3,
             (long)__src + (ulong)local_a08, uStack_9fc);
  
  // 只有在 ARM7 大小不为 0 时才加载 ARM7 程序
  if (local_9ec != 0 && final_addr2 != 0) {
    memory_region_block_memory_load
              (*(undefined8 *)final_addr2, lVar1,
               uStack_9f0, (long)__src + (ulong)local_9f8, local_9ec);
  } else if (local_9ec == 0) {
    fprintf(stderr, "[DRASTIC] gamecard_load_program: NOTE: Skipping ARM7 program load (ARM7 size is 0)\n");
    fflush(stderr);
  }
  *(undefined2 *)(polygon_sort_list_13776 + lVar15 + 0x151c8) = local_8ca;
  memcpy((void *)(lVar15 + 0x3ffe00),acStack_a28,0x170);
  *(undefined4 *)(nds_system + lVar16_offset + 0x10ce10c) = uStack_a04;
  *(undefined4 *)(nds_system + lVar16_offset + 0x20d46fc) = local_9f4;
  *(undefined1 *)(param_1 + 0x2da6) = 0;
  if ((local_a08 == 0x4000) && (local_a00 + 0xfe000000 < 0x1000000)) {
    lVar2 = lVar15 + (uVar3 & 0x3fffff);
    iVar12 = *(int *)(lVar15 + (ulong)(local_a00 & 0x3fffff));
    if (iVar12 == -0x18002101) {
      if (*(int *)(lVar2 + 4) != -0x18002101) {
        if (uVar9 == 0x45355659) goto LAB_0016f978;
        goto LAB_0016f948;
      }
    }
    else if (uVar9 == 0x45355659) {
      if ((iVar12 == 0x14a191a) && (*(int *)(lVar2 + 4) == -0x5a3b8f47)) goto LAB_0016f690;
LAB_0016f978:
      puts("Decrypting secure region.");
      if ((nds_system[lVar16_offset + 0x31d5e42] & 1) == 0) {
        gamecard_decrypt_secure_region(lVar2,local_a1c,nds_system + lVar16_offset + 0x35e5980);
      }
      else {
        puts("Can\'t decrypt game, need original BIOS for this.");
        *(undefined1 *)(param_1 + 0x2da6) = 1;
      }
    }
    else {
LAB_0016f948:
      if (uVar9 == 0x50355659) {
        if ((iVar12 != -0x2f2b7499) || (*(int *)(lVar2 + 4) != 0x39392f23)) goto LAB_0016f978;
      }
      else if ((uVar9 != 0x4a355659 || iVar12 != 0x7829bc8d) || (*(int *)(lVar2 + 4) != -0x669710bc)
              ) goto LAB_0016f978;
    }
  }
LAB_0016f690:
  __snprintf_chk(auStack_828,0x820,1,0x820,"%s%cbackup%c%s.dsv",(char*)nds_system + lVar16_offset + 0x8ab80,0x2f,0x2f,
                 (char*)nds_system + lVar16_offset + 0x8b380);
  lVar15 = param_1 + 0x968;
  puVar13 = (undefined8 *)game_database_lookup_by_game_code(param_1,uVar9,acStack_a38);
  if (puVar13 == (undefined8 *)0x0) {
    puts("Couldn\'t find game entry by code + title, trying CRC32.");
    uVar11 = game_database_generate_crc32_from_memory
                       (__src,*(undefined4 *)(*(long *)(param_1 + 0x920) + 0x10));
    printf("Got game CRC32 %x\n",uVar11);
    puVar13 = (undefined8 *)game_database_lookup_by_crc32(param_1,uVar11);
    if (puVar13 != (undefined8 *)0x0) goto LAB_0016f6ec;
    __s = "Couldn\'t find game in database. Using 512KB flash just to be safe.";
LAB_0016fa30:
    puts(__s);
    pvVar14 = malloc(0x80000);
    *(void **)(param_1 + 0x2d90) = pvVar14;
    initialize_backup(lVar15,1,pvVar14,0x80000,auStack_828);
    *(undefined4 *)(param_1 + 0x2d80) = 0x204013;
LAB_0016f770:
    //load_custom_cheats(param_1 + 0x28,lVar16);
    iVar12 = *(int *)(param_1 + 0x45c);
  }
  else {
LAB_0016f6ec:
    uVar5 = *(uint *)(puVar13 + 4);
    printf("Found game in database: titled %s. ",*puVar13);
    bVar6 = *(byte *)(puVar13 + 6);
    if (bVar6 != 2) {
      if (bVar6 < 3) {
        if (bVar6 == 0) {
          __s = "No save backup. Using 512KB flash just to be safe.";
          goto LAB_0016fa30;
        }
        uVar11 = *(undefined4 *)(puVar13 + 5);
        printf("Flash backup: %x bytes, ID %08x\n",uVar5,uVar11);
        pvVar14 = malloc((ulong)uVar5);
        *(void **)(param_1 + 0x2d90) = pvVar14;
        initialize_backup(lVar15,1,pvVar14,uVar5,auStack_828);
        *(undefined4 *)(param_1 + 0x2d80) = uVar11;
      }
      else if (bVar6 == 3) {
        printf("NAND backup: %x bytes\n",uVar5);
        *(undefined2 *)(param_1 + 0x2da7) = 0;
        *(uint *)(param_1 + 0x2d9c) = (uint)local_992 << 0x11;
        goto LAB_0016f8ac;
      }
      goto LAB_0016f770;
    }
    printf("EEPROM backup: %x bytes\n",uVar5);
LAB_0016f8ac:
    pvVar14 = malloc((ulong)uVar5);
    *(void **)(param_1 + 0x2d90) = pvVar14;
    initialize_backup(lVar15,bVar6,pvVar14,uVar5,auStack_828);
    //load_custom_cheats(param_1 + 0x28,lVar16);
    iVar12 = *(int *)(param_1 + 0x45c);
  }
  if (iVar12 != 0) {
    //load_cheat_listing(param_1 + 0x28,uVar9,~uVar10);
  }
  lVar15 = *(long *)(nds_system + lVar16_offset + 0x30d8930);
  uVar10 = ((*(int *)(param_1 + 0x930) + 1U >> 0x14) - 1) * 0x100 | 0xc2;
  *(uint *)(param_1 + 0x958) = uVar10;
  *(uint *)(lVar15 + 0x3ff800) = uVar10;
  *(uint *)(polygon_sort_list_13776 + *(long *)(nds_system + lVar16_offset + 0x30d8930) + 0x151c4) = uVar10
  ;
  *(uint *)(*(long *)(nds_system + lVar16_offset + 0x30d8930) + 0x3ffc00) = uVar10;
  *(undefined2 *)(polygon_sort_list_13776 + *(long *)(nds_system + lVar16_offset + 0x30d8930) + 0x151c8) =
       local_82a;
  if (local_a84 < 2) {
    //iVar12 = gamecard_load_gba(param_1,(char*)nds_system + lVar16_offset + 0x8b380);
    //if (iVar12 == -1) {
      //iVar12 = gamecard_load_gba(param_1,"slot2_gamepak");
      //cVar7 = *(char *)(param_1 + 0x8e3);
      //local_a84 = (uint)(iVar12 != -1);
    //}
    //else {
      cVar7 = *(char *)(param_1 + 0x8e3);
      local_a84 = 1;
    //}
    if (((cVar7 != '\0') || (local_988 != 0x3131565f4d415253)) || (local_980 != '0'))
    goto LAB_0016f818;
    iVar12 = strncmp(acStack_97c,"PASS",4);
    if (local_a84 == 0 && iVar12 == 0) goto LAB_0016fb08;
  }
  else {
    if ((*(char *)(param_1 + 0x8e3) == '\0') && (local_a84 == 2)) {
LAB_0016fb08:
      if (uVar4 < 0x2000001) {
        iVar12 = memory_setup_slot2_ram(lVar1);
        if (iVar12 == -1) {
          local_a84 = 0;
          puts("Couldn\'t setup slot 2 RAM.");
        }
        else {
          puts("Auto-loading game to slot 2 RAM.");
          local_a84 = 2;
          memcpy(*(void **)(nds_system + lVar16_offset + 0x31d4368),__src,(ulong)uVar4);
        }
      }
      else {
        local_a84 = 0;
      }
      goto LAB_0016f818;
    }
    if (local_a84 == 3) {
      memory_setup_slot2_rumble(lVar1);
      goto LAB_0016f818;
    }
  }
  if (local_a84 == 4) {
    //memory_setup_slot2_motion(lVar1);
  }
  else if (local_a84 == 5) {
    //memory_setup_slot2_motion_hb(lVar1);
  }
LAB_0016f818:
  *(uint *)(nds_system + lVar16_offset + 0x85a5c) = local_a84;
  memory_copy_nintendo_logo(lVar1,__src);
  if (local_8 - __stack_chk_guard != 0) {
                    
    __stack_chk_fail();
  }
  return 0;
}

// --- 函数: reset_memory ---

void reset_memory(long *param_1)

{
  void *__start;
  uint uVar1;
  long lVar2;
  void *pvVar3;
  long lVar4;
  int extraout_w8;
  int iVar5;
  int iVar6;
  
  memset(param_1 + 0x360e,0,0x8000);
  memset(param_1 + 0x460e,0,0x8000);
  *(undefined1 *)((long)param_1 + 0x1b2b7) = 3;
  *(undefined2 *)(param_1 + 0x3634) = 0x3ff;
  *(undefined1 *)((long)param_1 + 0x232b1) = 3;
  *(undefined2 *)(param_1 + 0x4634) = 0x3ff;
  *(undefined4 *)((long)param_1 + 0x231a4) = 0x7f800f;
  *(undefined2 *)((long)param_1 + 0x1b212) = 0xff;
  *(undefined2 *)((long)param_1 + 0x1b374) = 1;
  *(undefined2 *)(param_1 + 0x466e) = 1;
  *(undefined2 *)(param_1 + 0x366e) = 1;
  iVar5 = 0x200000;
  iVar6 = 0;
  memset((void *)*param_1,0,0x400000);
  memset((void *)param_1[1],0,0x8000);
  memset((void *)param_1[2],0,0x8000);
  memset((void *)param_1[3],0,0x4000);
  memset(param_1 + 4,0,0x10000);
  memset((void *)param_1[0x2a04],0,0x20000);
  memset((void *)param_1[0x2a05],0,0x20000);
  memset((void *)param_1[0x2a06],0,0x20000);
  memset((void *)param_1[0x2a07],0,0x20000);
  memset((void *)param_1[0x2a08],0,0x10000);
  memset((void *)param_1[0x2a09],0,0x4000);
  memset((void *)param_1[0x2a0a],0,0x4000);
  memset((void *)param_1[0x2a0b],0,0x8000);
  memset((void *)param_1[0x2a0c],0,0x4000);
  memset(param_1 + 0xd60e,0,0x20000);
  memset((void *)param_1[0x2a0d],0,0x4000);
  memset(param_1 + 0x2c0e,0,0x800);
  memset(param_1 + 0x2d0e,0,0x800);
  memset(param_1 + 0x2a0e,0,0x800);
  memset(param_1 + 0x2b0e,0,0x800);
  *(undefined2 *)(param_1 + 0x1faa2) = 0;
  remap_wram(param_1);
  memory_clear_slot2(param_1);
  remap_palette_oam_deferred(param_1);
  puts("  Setting up ARM9 memory map.");
  lVar2 = param_1[0x1f751];
  do {
    map_memory_page_from_memory_map(lVar2,iVar6);
    iVar6 = iVar6 + 0x800;
    iVar5 = iVar5 + -1;
  } while (iVar5 < 0);
  puts("  Setting up ARM7 memory map.");
  lVar2 = param_1[0x1f752];
  iVar5 = 0x200000;
  iVar6 = 0;
  do {
    map_memory_page_from_memory_map(lVar2,iVar6);
    iVar6 = iVar6 + 0x800;
    iVar5 = iVar5 + -1;
  } while (iVar5 < 0);
  lVar2 = param_1[0x1f751];
  printf("Remapping ITCM limit from %x to %x\n",(int)param_1[0x1fa9b],0);
  iVar6 = 0;
  if ((int)param_1[0x1fa9b] != 0) {
    do {
      iVar5 = iVar6 + 0x800;
      map_memory_page_from_memory_map(lVar2,iVar6);
      iVar6 = iVar5;
    } while (extraout_w8 != iVar5);
    *(undefined4 *)(param_1 + 0x1fa9b) = 0;
  }
  remap_dtcm(param_1,0,0);
  reset_coprocessor(param_1 + 0x1faa3);
  reset_dma(param_1 + 0x1fa53);
  reset_dma(param_1 + 0x1fa69);
  reset_ipc(param_1 + 0x1fa7f);
  lVar2 = 0;
  reset_ipc(param_1 + 0x1fa8b);
  do {
    __start = (void *)(param_1[0x1faa0] + lVar2);
    if (*(char *)((long)param_1 + 0xfd513) == '\0') {
      munmap(__start,0x4000);
      pvVar3 = mmap(__start,0x4000,3,1,(int)param_1[0x1faa1],0xa4000);
      if (__start != pvVar3) {
        printf("ERROR: VRAM remap to %p didn\'t map to same location (got %p)\n",__start,
                     pvVar3);
      }
    }
    else {
      uVar1 = 0;
      if (*(uint *)((long)param_1 + 0xfd4dc) != 0) {
        uVar1 = 0xa4000 / *(uint *)((long)param_1 + 0xfd4dc);
      }
      remap_file_pages(__start,0x4000,0,(ulong)uVar1,0);
    }
    lVar2 = lVar2 + 0x4000;
  } while (lVar2 != 0x800000);
  patch_firmware_user_data(param_1[0x1f74d] + 0x855a8,param_1 + 0x560e);
  lVar4 = *param_1;
  // 修复：原始代码使用 polygon_sort_list_13776 + lVar4 + offset，但 lVar4 是绝对地址（内存映射缓冲区地址）
  // polygon_sort_list_13776 只有 790528 字节，而 lVar4 可能是 0x2000000 或更大的地址
  // 这会导致数组越界访问。正确的做法是将这些数据存储在 param_1 指向的结构中
  // 或者使用 lVar4 指向的内存映射缓冲区中的位置
  // 由于 lVar4 是内存映射缓冲区的地址，我们可以将数据存储在 lVar4 + offset 的位置
  lVar2 = param_1[0xd5cf];
  *(long *)(lVar4 + 0x15640) = param_1[0xd5ce];
  *(long *)(lVar4 + 0x15648) = lVar2;
  lVar2 = param_1[0xd5d1];
  *(long *)(lVar4 + 0x15650) = param_1[0xd5d0];
  *(long *)(lVar4 + 0x15658) = lVar2;
  lVar2 = param_1[0xd5d3];
  *(long *)(lVar4 + 0x15660) = param_1[0xd5d2];
  *(long *)(lVar4 + 0x15668) = lVar2;
  lVar2 = param_1[0xd5d5];
  *(long *)(lVar4 + 0x15670) = param_1[0xd5d4];
  *(long *)(lVar4 + 0x15678) = lVar2;
  lVar2 = param_1[0xd5d7];
  *(long *)(lVar4 + 0x15680) = param_1[0xd5d6];
  *(long *)(lVar4 + 0x15688) = lVar2;
  lVar2 = param_1[0xd5d9];
  *(long *)(lVar4 + 0x15690) = param_1[0xd5d8];
  *(long *)(lVar4 + 0x15698) = lVar2;
  lVar2 = param_1[0xd5db];
  *(long *)(lVar4 + 0x156a0) = param_1[0xd5da];
  *(long *)(lVar4 + 0x156a8) = lVar2;
  *(char *)(lVar4 + 0x15600) = 1;
  return;
}

// --- 函数: apply_cycle_adjustment_hacks ---

void apply_cycle_adjustment_hacks(long param_1)

{
  uint uVar1;
  
  // 修复：param_1 是绝对地址（nds_system 的地址），需要转换为相对于 nds_system 的偏移量
  long param_1_offset;
  if (param_1 >= (long)nds_system && param_1 < (long)nds_system + 62042112) {
    // param_1 是绝对地址，转换为偏移量
    param_1_offset = (long)((char*)param_1 - (char*)nds_system);
  } else {
    // param_1 可能已经是偏移量，直接使用
    param_1_offset = param_1;
  }
  
  printf("Checking cycle hacks for gamecode %08x\n",*(undefined4 *)(nds_system + param_1_offset + 0xc58));
  *(undefined8 *)(nds_system + param_1_offset + 0x362e988) = 0;
  *(undefined8 *)(nds_system + param_1_offset + 0x362e990) = 0;
  *(undefined8 *)(nds_system + param_1_offset + 0x362e998) = 0x100000000;
  nds_system[param_1_offset + 0x362e9a0] = 0;
  uVar1 = *(uint *)(nds_system + param_1_offset + 0xc58) & 0xffffff;
  if (uVar1 == 0x4c4542) {
    puts("Element Hunters: 1 cycle");
    *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
    return;
  }
  if (uVar1 < 0x4c4543) {
    if (uVar1 == 0x4a3542) {
      if (*(uint *)(nds_system + param_1_offset + 0xc58) >> 0x18 != 0x50) {
        return;
      }
      puts("Zhu Zhu Babies (E): 1 cycle");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
      return;
    }
    if (uVar1 < 0x4a3543) {
      if (uVar1 == 0x414156) {
        puts("Art Academy: Applying vcount 192 hack.");
        nds_system[param_1_offset + 0x362e99d] = 1;
        return;
      }
      if (0x414156 < uVar1) {
        if (uVar1 != 0x463242) {
          return;
        }
        puts("Florist Shop: force undeferred 2D");
        nds_system[param_1_offset + 0x362e9a0] = 1;
        return;
      }
      if (uVar1 == 0x344843) {
        puts("Imagine - Champion Rider: swap stalls geometry");
        nds_system[param_1_offset + 0x362e99f] = 1;
        return;
      }
      if (uVar1 != 0x385943) {
        return;
      }
LAB_0010fb94:
      puts("Adjustment for Yu-Gi-Oh! 5D\'s: 1 cycle + DMA CPU");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
      *(undefined4 *)(nds_system + param_1_offset + 0x362e998) = 1;
      nds_system[param_1_offset + 0x362e99e] = 1;
      return;
    }
    if (uVar1 == 0x4a4c43) {
      puts("Adjustment for Mario & Luigi: Bowser\'s Inside Story: 2 cycles + DMA 2x + geometry 4x");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 2;
      *(undefined8 *)(nds_system + param_1_offset + 0x362e994) = 0x200000004;
      return;
    }
    if (uVar1 == 0x4a5643) {
      puts("Adjustment for Ore ga Omae o Mamoru: 1 cycle");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
      return;
    }
    if (uVar1 != 0x4a4159) {
      return;
    }
  }
  else {
    if (uVar1 == 0x545059) {
      puts("Adjustment for Puppy Palace: 2 cycles");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 2;
      return;
    }
    if (0x545059 < uVar1) {
      if (uVar1 == 0x594b42) {
        puts("Adjustment for Legend of Kay: 1 cycle");
        *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
        return;
      }
      if (uVar1 < 0x594b43) {
        if (uVar1 == 0x574f43) {
          puts("Adjustment for Will o\' Wisp DS: 1 cycle");
          *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
          return;
        }
        if (uVar1 != 0x585942) {
          return;
        }
      }
      else if (uVar1 != 0x595942) {
        return;
      }
      goto LAB_0010fb94;
    }
    if (uVar1 == 0x4e5343) {
      puts("Adjustment for Sonic Chronicles: 1 cycle");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
      return;
    }
    if (uVar1 == 0x503342) {
      puts("Adjustment for Spider-Man Shattered Dimensions: 1 cycle");
      *(undefined4 *)(nds_system + param_1_offset + 0x362e988) = 1;
      return;
    }
    if (uVar1 != 0x4d4143) {
      return;
    }
  }
  puts("American Girl: swap stalls geometry");
  nds_system[param_1_offset + 0x362e99f] = 1;
  return;
}

// --- 函数: reset_gamecard ---

void reset_gamecard(long param_1)

{
  *(undefined8 *)(param_1 + 0x910) = 0;
  *(undefined8 *)(param_1 + 0x958) = 0;
  *(undefined4 *)(param_1 + 0x960) = 0;
  *(undefined1 *)(param_1 + 0x2da5) = 0;
  *(undefined1 *)(param_1 + 0x8e8) = 0;
  *(undefined2 *)(param_1 + 0x8ea) = 0x8080;
  *(undefined1 *)(param_1 + 0x8ec) = 0x80;
  *(undefined2 *)(param_1 + 0x8ee) = 0;
  *(undefined8 *)(param_1 + 0x8f0) = 0x6900800080008000;
  *(undefined2 *)(param_1 + 0x908) = 0;
  return;
}

// --- 函数: reset_spi_peripherals ---
void reset_backup(long param_1)

{
  *(undefined4 *)(param_1 + 0x2404) = 0;
  *(undefined4 *)(param_1 + 0x241c) = 0;
  *(undefined1 *)(param_1 + 0x2422) = 0;
  return;
}
void reset_spi_peripherals(long param_1)

{
  reset_backup(param_1);
  *(undefined8 *)(param_1 + 0x2428) = 0;
  *(undefined8 *)(param_1 + 0x2430) = 0;
  *(undefined1 *)(param_1 + 0x2440) = 0xf;
  *(undefined8 *)(param_1 + 0x2438) = 0x7f00000000;
  *(undefined2 *)(param_1 + 0x2450) = 0;
  return;
}

// --- 函数: reset_cpu_block ---

void reset_cpu_block(long param_1)

{
  void *__s;
  void *__s_00;
  long lVar1;
  long lVar2;
  void *__addr;
  long lVar3;
  
  __addr = *(void **)(param_1 + 0x2088);
  __s = (void *)((long)__addr + 0x14d8038);
  lVar2 = *(long *)(param_1 + 0x2260);
  lVar3 = *(long *)(param_1 + 0x22a0);
  memset((void *)((long)__addr + 0x1300000),0,0x100000);
  memset((void *)((long)__addr + 0x1380000),0,0x40000);
  __s_00 = *(void **)(param_1 + 0x2270);
  if (__s_00 < (void *)((long)__addr + 0x14da038U) && __s < (void *)((long)__s_00 + 0x8000U)) {
    lVar1 = 0;
    do {
      *(undefined4 *)((long)__s_00 + lVar1 * 4) = 0;
      *(undefined1 *)((long)__s + lVar1) = 0;
      lVar1 = lVar1 + 1;
    } while (lVar1 != 0x2000);
  }
  else {
    memset(__s_00,0,0x8000);
    memset(__s,0,0x2000);
  }
  memset(*(void **)(param_1 + 0x2278),0,0x10000);
  memset((void *)(param_1 + 0x80),0,0x2000);
  memset((void *)(lVar3 + 0x80),0,0x2000);
  memset((void *)(lVar2 + 0xaf070),0,0x100);
  *(undefined4 *)(lVar2 + 0xaf178) = 0;
  *(undefined4 *)(lVar2 + 0xaf17c) = 0;
  *(undefined4 *)(lVar2 + 0xaf180) = 0;
  *(undefined8 *)(lVar2 + 0xaf184) = 0;
  *(undefined8 *)(lVar2 + 0xaf18c) = 0;
  *(undefined8 *)(lVar2 + 0xaf194) = 0;
  *(undefined8 *)(lVar2 + 0xaf19c) = 0;
  *(undefined8 *)(lVar2 + 0xaf1a4) = 0;
  *(undefined4 *)(lVar2 + 0xaf1ac) = 0;
  memset((void *)(lVar2 + 0xaf1b0),0,0x40000);
  if (*(long *)(lVar2 + 0xef1b0) != 0) {
    memset(*(void **)(lVar2 + 0xaf170),0,0x800);
    memset(*(void **)(lVar2 + 0xef1b0),0,0x200000);
  }
  memset((void *)(lVar2 + 0xef1b8),0,0x800);
  memset((void *)(lVar2 + 0xef9b8),0,0x800);
  memset((void *)(lVar2 + 0xf01b8),0,0x1000);
  memset((void *)(lVar2 + 0xf11b8),0,0xa400);
  mprotect(__addr,0x1000000,7);
  mprotect((void *)((long)__addr + 0x1000000),0x100000,7);
  mprotect((void *)((long)__addr + 0x1100000),0x200000,7);
  memset(__s,0,0x2000);
  *(undefined4 *)((long)__addr + 0x14fa138) = 0;
  memset((void *)((long)__addr + 0x14da138),0,0x20000);
  return;
}

// --- 函数: cpu_block_lookup_base ---

long cpu_block_lookup_base(long param_1,ulong param_2)

{
  return 0;
/*
  long lVar1;
  long lVar2;
  ulong uVar3;
  long lVar4;
  undefined8 uVar5;
  ulong uVar6;
  long lVar7;
  ulong uVar8;
  ulong uVar9;
  uint uVar10;
  uint uVar11;
  ulong uVar12;
  ulong uVar13;
  ulong uVar14;
  uint *puVar15;
  ulong uVar16;
  uint uVar17;
  uint uVar18;
  long lVar19;
  ulong uVar20;
  uint *local_20;
  
  uVar18 = (uint)(param_2 >> 2);
  uVar17 = uVar18 & 0x3fffffff;
  uVar18 = uVar18 & 0x3ff;
  uVar14 = (ulong)uVar18;
  uVar10 = (uint)param_2;
  lVar1 = param_1 + 0x80;
  lVar19 = *(long *)(param_1 + 0x2088);
  if (uVar10 < 0x2000000) {
    if (*(int *)(param_1 + 0x210c) == 1) {
      if ((param_2 & 1) == 0) {
        uVar11 = *(uint *)(*(long *)(param_1 + 0x2270) + (param_2 >> 2 & 0x1fff) * 4);
      }
      else {
        uVar11 = *(uint *)(*(long *)(param_1 + 0x2278) + ((param_2 & 0xffffffff) >> 1 & 0x3fff) * 4)
        ;
      }
      uVar6 = (ulong)(uVar18 + 0x400);
      lVar4 = uVar14 << 2;
      puVar15 = (uint *)0x0;
      if (uVar11 != 0) goto LAB_0012e14c;
      goto LAB_0012dbb0;
    }
LAB_0012db54:
    lVar7 = lVar19 + 0x1380000;
    uVar11 = 0x1fff;
  }
  else {
    if (uVar10 >> 0x18 != 2) goto LAB_0012db54;
    uVar11 = 0x7fff;
    lVar7 = lVar19 + 0x1300000;
  }
  uVar12 = (ulong)(uVar17 & uVar11);
  uVar6 = (ulong)(uVar18 + 0x400);
  puVar15 = (uint *)(lVar7 + uVar12 * 0x10);
  lVar4 = uVar14 * 4;
  if (*(uint *)(lVar7 + uVar12 * 0x10) == uVar10) {
    uVar18 = puVar15[1];
    *(uint *)(lVar1 + uVar14 * 4) = uVar10;
    *(uint *)(lVar1 + uVar6 * 4) = uVar18;
    return lVar19 + (ulong)uVar18;
  }
  uVar11 = puVar15[3];
  if (puVar15[2] == uVar10) {
LAB_0012e14c:
    *(uint *)(lVar1 + uVar14 * 4) = uVar10;
    *(uint *)(lVar1 + uVar6 * 4) = uVar11;
    return lVar19 + (ulong)uVar11;
  }
  if (uVar11 != 0) {
    for (uVar18 = *(uint *)((ulong)*(uint *)(lVar19 + (ulong)uVar11 + -4) + lVar19); uVar18 != 0;
        uVar18 = *(uint *)(lVar19 + (ulong)uVar18)) {
      if (*(uint *)(lVar19 + (ulong)uVar18 + 4) == uVar10) {
        uVar18 = *(uint *)(lVar19 + (ulong)uVar18 + 8);
        *(uint *)(lVar1 + uVar14 * 4) = uVar10;
        *(uint *)(lVar1 + uVar6 * 4) = uVar18;
        return lVar19 + (ulong)uVar18;
      }
    }
  }
LAB_0012dbb0:
  local_20 = (uint *)(lVar1 + uVar6 * 4);
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24000);
  uVar9 = *(ulong *)(nds_system + lVar19 + 0xf24008);
  uVar12 = *(ulong *)(nds_system + lVar19 + 0xf24010);
  uVar3 = *(ulong *)(nds_system + lVar19 + 0xf24018);
  uVar6 = *(ulong *)(nds_system + lVar19 + 0xf24020);
  uVar20 = *(ulong *)(nds_system + lVar19 + 0xf24028);
  *(long *)(nds_system + lVar19 + 0xffe140) = lVar19 + 0x13a0000;
  *(undefined4 *)(nds_system + lVar19 + 0xfa4030) = 0;
  *(undefined4 *)(nds_system + lVar19 + 0xfc4034) = 0;
  lVar7 = cpu_block_create(param_1,uVar10 & 0xfffffffe);
  uVar18 = (int)lVar7 - (int)lVar19;
  if (lVar7 != 0) {
    if (puVar15 == (uint *)0x0) {
      if ((param_2 & 1) == 0) {
        uVar13 = (ulong)uVar17 & 0x1fff;
        lVar2 = lVar19 + uVar13;
        *(uint *)(*(long *)(param_1 + 0x2270) + uVar13 * 4) = uVar18;
        if (8 < (byte)nds_system[lVar2 + 0xfdc038]) {
          nds_system[lVar2 + 0xfdc038] = nds_system[lVar2 + 0xfdc038] | 0x80;
        }
      }
      else {
        *(uint *)(*(long *)(param_1 + 0x2278) + ((param_2 & 0xffffffff) >> 1 & 0x3fff) * 4) = uVar18
        ;
      }
    }
    else if (puVar15[1] == 0) {
      *puVar15 = uVar10;
      puVar15[1] = uVar18;
    }
    else if (puVar15[3] == 0) {
      puVar15[2] = uVar10;
      puVar15[3] = uVar18;
    }
    else {
      uVar13 = (ulong)*(uint *)(lVar19 + (ulong)puVar15[3] + -4);
      for (uVar17 = *(uint *)(lVar19 + uVar13); uVar17 != 0;
          uVar17 = *(uint *)(lVar19 + (ulong)uVar17)) {
        uVar13 = (ulong)uVar17;
      }
      *(undefined4 *)(lVar19 + uVar13) = *(undefined4 *)(lVar7 + -4);
    }
  }
  cpu_translate_link_delayed_targets(param_1);
  uVar13 = *(ulong *)(nds_system + lVar19 + 0xf24000);
  if (uVar13 != uVar14) {
    uVar5 = ctr_el0;
    uVar16 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar8 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar16 < icache_line_size_11647) {
      icache_line_size_11647 = uVar16;
    }
    if (uVar8 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar8;
    }
    for (uVar8 = uVar14 & -dcache_line_size_11648; uVar8 < uVar13;
        uVar8 = uVar8 + dcache_line_size_11648) {
      DC_CIVAC(uVar8);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar14 = uVar14 & -icache_line_size_11647; uVar14 < uVar13;
        uVar14 = uVar14 + icache_line_size_11647) {
      IC_IVAU(uVar14);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24008);
  if (uVar14 != uVar9) {
    uVar5 = ctr_el0;
    uVar8 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar13 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar8 < icache_line_size_11647) {
      icache_line_size_11647 = uVar8;
    }
    if (uVar13 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar13;
    }
    for (uVar13 = uVar14 & -dcache_line_size_11648; uVar13 < uVar9;
        uVar13 = uVar13 + dcache_line_size_11648) {
      DC_CIVAC(uVar13);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar14 = uVar14 & -icache_line_size_11647; uVar14 < uVar9;
        uVar14 = uVar14 + icache_line_size_11647) {
      IC_IVAU(uVar14);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24020);
  if (uVar14 != uVar6) {
    uVar5 = ctr_el0;
    uVar13 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar9 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar13 < icache_line_size_11647) {
      icache_line_size_11647 = uVar13;
    }
    if (uVar9 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar9;
    }
    for (uVar9 = uVar6 & -dcache_line_size_11648; uVar9 < uVar14;
        uVar9 = uVar9 + dcache_line_size_11648) {
      DC_CIVAC(uVar9);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar6 = uVar6 & -icache_line_size_11647; uVar6 < uVar14;
        uVar6 = uVar6 + icache_line_size_11647) {
      IC_IVAU(uVar6);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24028);
  if (uVar14 != uVar20) {
    uVar5 = ctr_el0;
    uVar9 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar6 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar9 < icache_line_size_11647) {
      icache_line_size_11647 = uVar9;
    }
    if (uVar6 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar6;
    }
    for (uVar6 = uVar14 & -dcache_line_size_11648; uVar6 < uVar20;
        uVar6 = uVar6 + dcache_line_size_11648) {
      DC_CIVAC(uVar6);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar14 = uVar14 & -icache_line_size_11647; uVar14 < uVar20;
        uVar14 = uVar14 + icache_line_size_11647) {
      IC_IVAU(uVar14);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24010);
  if (uVar14 != uVar12) {
    uVar5 = ctr_el0;
    uVar9 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar6 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar9 < icache_line_size_11647) {
      icache_line_size_11647 = uVar9;
    }
    if (uVar6 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar6;
    }
    for (uVar6 = uVar12 & -dcache_line_size_11648; uVar6 < uVar14;
        uVar6 = uVar6 + dcache_line_size_11648) {
      DC_CIVAC(uVar6);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar12 = uVar12 & -icache_line_size_11647; uVar12 < uVar14;
        uVar12 = uVar12 + icache_line_size_11647) {
      IC_IVAU(uVar12);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  uVar14 = *(ulong *)(nds_system + lVar19 + 0xf24018);
  if (uVar14 != uVar3) {
    uVar5 = ctr_el0;
    uVar12 = (ulong)(4 << (ulong)((uint)uVar5 & 0xf));
    uVar6 = (ulong)(4 << (ulong)((uint)uVar5 >> 0x10 & 0xf));
    if (uVar12 < icache_line_size_11647) {
      icache_line_size_11647 = uVar12;
    }
    if (uVar6 < dcache_line_size_11648) {
      dcache_line_size_11648 = uVar6;
    }
    for (uVar6 = uVar14 & -dcache_line_size_11648; uVar6 < uVar3;
        uVar6 = uVar6 + dcache_line_size_11648) {
      DC_CIVAC(uVar6);
    }
    DataSynchronizationBarrier(2,3,0);
    for (uVar14 = uVar14 & -icache_line_size_11647; uVar14 < uVar3;
        uVar14 = uVar14 + icache_line_size_11647) {
      IC_IVAU(uVar14);
    }
    DataSynchronizationBarrier(2,3,0);
    InstructionSynchronizationBarrier();
  }
  *(uint *)(lVar1 + lVar4) = uVar10;
  *local_20 = uVar18;
  return lVar7;
  */
}

// --- 函数: event_scanline_start_function ---

void event_scanline_start_function(long *param_1)

{
  long *plVar1;
  undefined1 uVar2;
  byte bVar3;
  undefined2 uVar4;
  uint *puVar5;
  byte bVar6;
  undefined4 uVar7;
  uint *puVar8;
  uint uVar9;
  uint *puVar10;
  long lVar11;
  uint uVar12;
  uint uVar13;
  
  uVar13 = *(ushort *)((long)param_1 + 0x14) + 1;
  uVar12 = uVar13;
  if (*(ushort *)((long)param_1 + 0x14) == 0x105) {
    uVar9 = *(byte *)((long)param_1 + 0x35ef9a4) & 0xfe;
    *(char *)((long)param_1 + 0x35ef9a4) = (char)uVar9;
    *(byte *)((long)param_1 + 0x35f79a4) = *(byte *)((long)param_1 + 0x35f79a4) & 0xfe;
  }
  else if (uVar13 == 0x107) {
    plVar1 = param_1 + 0x6da379;
    if (((int)param_1[0x6da37f] < 0) && (*(char *)((long)param_1 + 0x36d1bfc) == '\x03')) {
      dma_transfer(plVar1,param_1 + 0x6da37b);
    }
    if (((int)param_1[0x6da384] < 0) && (*(char *)((long)param_1 + 0x36d1c24) == '\x03')) {
      dma_transfer(plVar1,0);
    }
    if (((int)param_1[0x6da389] < 0) && (*(char *)((long)param_1 + 0x36d1c4c) == '\x03')) {
      dma_transfer(plVar1,param_1 + 0x6da385);
    }
    if (((int)param_1[0x6da38e] < 0) && (*(char *)((long)param_1 + 0x36d1c74) == '\x03')) {
      dma_transfer(plVar1,param_1 + 0x6da38a);
    }
    start_frame(param_1 + 0x6da3d8);
    uVar13 = 0;
    uVar9 = (uint)*(byte *)((long)param_1 + 0x35ef9a4);
    uVar12 = 0;
  }
  else if (uVar13 == 0xc0) {
    uVar7 = rumble_pack_update(param_1 + 0x184);
    bVar6 = *(byte *)((long)param_1 + 0x35ef9a4);
    *(byte *)((long)param_1 + 0x35ef9a4) = bVar6 | 1;
    if ((bVar6 >> 3 & 1) != 0) {
      lVar11 = param_1[0x2b93ba];
      uVar12 = *(uint *)(lVar11 + 0x214) | 1;
      *(uint *)(lVar11 + 0x214) = uVar12;
      if ((*(uint *)(param_1 + 0x2b93cc) & 6) == 0) {
        *(uint *)(param_1 + 0x2b93cb) =
             -*(int *)(lVar11 + 0x208) & uVar12 & *(uint *)(lVar11 + 0x210);
      }
    }
    bVar6 = *(byte *)((long)param_1 + 0x35f79a4);
    *(byte *)((long)param_1 + 0x35f79a4) = bVar6 | 1;
    if ((bVar6 >> 3 & 1) != 0) {
      lVar11 = param_1[0x4ba078];
      uVar12 = *(uint *)(lVar11 + 0x214) | 1;
      *(uint *)(lVar11 + 0x214) = uVar12;
      if ((*(uint *)(param_1 + 0x4ba08a) & 6) == 0) {
        *(uint *)(param_1 + 0x4ba089) =
             -*(int *)(lVar11 + 0x208) & uVar12 & *(uint *)(lVar11 + 0x210);
      }
    }
    update_frame(param_1 + 0x6da3d8);
    update_input(param_1 + 0xaaa);
    benchmark_step(param_1 + 0x11463);
    backup_auto_save_step(param_1 + 0x191);
    gba_backup_auto_save_step((char *)(param_1 + 0xf8));
    if ((((int)param_1[0x10b44] != 0) && ((*(uint *)(param_1 + 0x4ba089) & 1) != 0)) &&
       ((*(uint *)(param_1 + 0x4ba0e0) >> 7 & 1) == 0)) {
      //process_cheats(param_1,param_1 + 0x69,(int)*param_1);
    }
    platform_set_rumble_state(uVar7);
    param_1[0x2b8fa1] = param_1[1];
    if ((((*(uint *)((long)param_1 + 0x8a374) >> 6 & 1) == 0) &&
        (update_spu(param_1), *(int *)((long)param_1 + 0x85a04) == 0)) &&
       (*(int *)((long)param_1 + 0x85a34) == 0)) {
      audio_sync(param_1 + 0x2b0e00);
    }
    plVar1 = param_1 + 0x6da379;
    if (((int)param_1[0x6da37f] < 0) && (*(char *)((long)param_1 + 0x36d1bfc) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da37b);
    }
    if (((int)param_1[0x6da384] < 0) && (*(char *)((long)param_1 + 0x36d1c24) == '\x01')) {
      dma_transfer(plVar1,0);
    }
    if (((int)param_1[0x6da389] < 0) && (*(char *)((long)param_1 + 0x36d1c4c) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da385);
    }
    if (((int)param_1[0x6da38e] < 0) && (*(char *)((long)param_1 + 0x36d1c74) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da38a);
    }
    plVar1 = param_1 + 0x6da38f;
    if (((int)param_1[0x6da395] < 0) && (*(char *)((long)param_1 + 0x36d1cac) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da391);
    }
    if (((int)param_1[0x6da39a] < 0) && (*(char *)((long)param_1 + 0x36d1cd4) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da396);
    }
    if (((int)param_1[0x6da39f] < 0) && (*(char *)((long)param_1 + 0x36d1cfc) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da39b);
    }
    if (((int)param_1[0x6da3a4] < 0) && (*(char *)((long)param_1 + 0x36d1d24) == '\x01')) {
      dma_transfer(plVar1,param_1 + 0x6da3a0);
    }
    *param_1 = *param_1 + 1;
    uVar9 = (uint)*(byte *)((long)param_1 + 0x35ef9a4);
    uVar12 = 0xc0;
  }
  else if (uVar13 == 0xd7) {
    //system_frame_sync();
    uVar2 = (undefined1)param_1[0x6da9be];
    if ((*(uint *)((long)param_1 + 0x8a374) & 8) != 0) {
      uVar2 = 1;
    }
    if ((int)param_1[0x10b42] == 0) {
      //update_frame_3d_1x(param_1 + 0x6da3d8,uVar2);
    }
    else {
      video_3d_start_rendering(param_1 + 0x6da3d8, 0);
    }
    uVar9 = (uint)*(byte *)((long)param_1 + 0x35ef9a4);
  }
  else {
    uVar9 = (uint)*(byte *)((long)param_1 + 0x35ef9a4);
    uVar12 = uVar13 & 0xffff;
  }
  if (((uint)*(ushort *)((long)param_1 + 0x35ef9a4) * 2 & 0x100 |
      (uint)(*(ushort *)((long)param_1 + 0x35ef9a4) >> 8)) == uVar13) {
    *(byte *)((long)param_1 + 0x35ef9a4) = (byte)uVar9 | 4;
    if ((uVar9 >> 5 & 1) != 0) {
      lVar11 = param_1[0x2b93ba];
      uVar9 = *(uint *)(lVar11 + 0x214) | 4;
      *(uint *)(lVar11 + 0x214) = uVar9;
      if ((*(uint *)(param_1 + 0x2b93cc) & 6) == 0) {
        *(uint *)(param_1 + 0x2b93cb) =
             -*(int *)(lVar11 + 0x208) & uVar9 & *(uint *)(lVar11 + 0x210);
      }
    }
  }
  else {
    *(byte *)((long)param_1 + 0x35ef9a4) = (byte)uVar9 & 0xfb;
  }
  bVar3 = *(byte *)((long)param_1 + 0x35f79a4);
  bVar6 = bVar3 & 0xfb;
  if (((uint)*(ushort *)((long)param_1 + 0x35f79a4) * 2 & 0x100 |
      (uint)(*(ushort *)((long)param_1 + 0x35f79a4) >> 8)) == uVar13) {
    bVar6 = bVar3 | 4;
    *(byte *)((long)param_1 + 0x35f79a4) = bVar6;
    if ((bVar3 >> 5 & 1) != 0) {
      lVar11 = param_1[0x4ba078];
      uVar13 = *(uint *)(lVar11 + 0x214) | 4;
      *(uint *)(lVar11 + 0x214) = uVar13;
      if ((*(uint *)(param_1 + 0x4ba08a) & 6) == 0) {
        *(uint *)(param_1 + 0x4ba089) =
             -*(int *)(lVar11 + 0x208) & uVar13 & *(uint *)(lVar11 + 0x210);
      }
      bVar6 = *(byte *)((long)param_1 + 0x35f79a4);
    }
  }
  uVar4 = (undefined2)uVar12;
  *(undefined2 *)((long)param_1 + 0x35ef9a6) = uVar4;
  *(undefined2 *)((long)param_1 + 0x35f79a6) = uVar4;
  plVar1 = param_1 + 3;
  *(byte *)((long)param_1 + 0x35ef9a4) = *(byte *)((long)param_1 + 0x35ef9a4) & 0xfd;
  *(byte *)((long)param_1 + 0x35f79a4) = bVar6 & 0xfd;
  *(undefined2 *)((long)param_1 + 0x14) = uVar4;
  if ((uint *)param_1[99] == (uint *)0x0) {
    *(undefined4 *)(param_1 + 3) = 0xc00;
    param_1[6] = 0;
    param_1[7] = 0;
    param_1[99] = (long)plVar1;
  }
  else {
    uVar13 = 0xc00;
    puVar5 = (uint *)param_1[99];
    puVar10 = (uint *)0x0;
    while (puVar8 = puVar5, *puVar8 < uVar13) {
      uVar13 = uVar13 - *puVar8;
      puVar5 = *(uint **)(puVar8 + 6);
      puVar10 = puVar8;
      if (*(uint **)(puVar8 + 6) == (uint *)0x0) {
        *(uint *)(param_1 + 3) = uVar13;
        param_1[6] = 0;
        param_1[7] = (long)puVar8;
        *(long **)(puVar8 + 6) = plVar1;
        return;
      }
    }
    *(uint *)(param_1 + 3) = uVar13;
    param_1[6] = (long)puVar8;
    param_1[7] = (long)puVar10;
    if (puVar10 == (uint *)0x0) {
      param_1[99] = (long)plVar1;
    }
    else {
      *(long **)(puVar10 + 6) = plVar1;
    }
    *(long **)(puVar8 + 8) = plVar1;
    *puVar8 = *puVar8 - uVar13;
  }
  return;
}

// --- 函数: reset_video ---

void reset_video(long *param_1)

{
  byte bVar1;
  long lVar2;
  byte *pbVar3;
  long lVar4;
  long *plVar5;
  ulong uVar6;
  uint uVar7;
  uint uVar8;
  ulong uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  
  lVar2 = *param_1;
  uVar9 = 0;
  plVar5 = param_1;
  do {
    pbVar3 = (byte *)param_1[uVar9 + 0x41d];
    *(undefined4 *)(plVar5 + 2) = 0xb;
    bVar1 = *pbVar3;
    if ((uint)bVar1 != *(uint *)((long)plVar5 + 0x14)) {
      lVar4 = (param_1 + 0x414)[uVar9];
      *(undefined2 *)(param_1 + 0x5ce) = 0;
      //remap_vram_body(param_1,lVar4,uVar9 & 0xffffffff,bVar1,1);
      uVar7 = (uint)*(ushort *)(param_1 + 0x5ce);
      if (*(ushort *)(param_1 + 0x5ce) != 0) {
        uVar8 = 0;
        do {
          if ((uVar7 & 1) != 0) {
            uVar6 = (ulong)uVar8;
            *(undefined4 *)((long)param_1 + uVar6 * 0x10 + 0x14) = 0xffffffff;
            //remap_vram_body(param_1,param_1[uVar6 + 0x414],uVar8,
            //                *(undefined1 *)param_1[uVar6 + 0x41d],0);
          }
          uVar7 = uVar7 >> 1;
          uVar8 = uVar8 + 1;
        } while (uVar7 != 0);
      }
    }
    uVar9 = uVar9 + 1;
    plVar5 = plVar5 + 2;
  } while (uVar9 != 9);
  plVar5 = param_1 + 0x14;
  lVar4 = *(long *)(lVar2 + 0x15068);
  iVar10 = 0;
  iVar11 = 1;
  iVar12 = 2;
  iVar13 = 3;
  do {
    plVar5[1] = lVar4 - (ulong)(uint)(iVar11 << 0xe);
    *plVar5 = lVar4 - (ulong)(uint)(iVar10 << 0xe);
    plVar5[3] = lVar4 - (ulong)(uint)(iVar13 << 0xe);
    plVar5[2] = lVar4 - (ulong)(uint)(iVar12 << 0xe);
    plVar5 = plVar5 + 4;
    iVar10 = iVar10 + 4;
    iVar11 = iVar11 + 4;
    iVar12 = iVar12 + 4;
    iVar13 = iVar13 + 4;
  } while (plVar5 != param_1 + 0x414);
  memset((undefined4 *)((long)param_1 + 0x2224),0,0x800);
  lVar2 = lVar2 + 0x6b070;
  param_1[0x43c] = 0;
  param_1[0x43d] = 0;
  param_1[0x43e] = 0;
  param_1[0x43f] = 0;
  param_1[0x440] = 0;
  param_1[0x441] = 0;
  param_1[0x442] = 0;
  param_1[0x443] = 0;
  param_1[0x427] = 0;
  param_1[0x426] = 0;
  param_1[0x429] = 0;
  param_1[0x428] = 0;
  param_1[0x42b] = 0;
  param_1[0x42a] = 0;
  param_1[0x42d] = 0;
  param_1[0x42c] = 0;
  param_1[0x42f] = 0;
  param_1[0x42e] = 0;
  lVar4 = 0;
  param_1[0x431] = 0;
  param_1[0x430] = 0;
  param_1[0x433] = 0;
  param_1[0x432] = 0;
  *(undefined4 *)(param_1 + 0x444) = 0;
  *(undefined8 *)((long)param_1 + 0x2e24) = 0;
  *(undefined8 *)((long)param_1 + 0x2e2c) = 0;
  param_1[0x43b] = lVar2;
  param_1[0x43a] = lVar2;
  *(undefined4 *)((long)param_1 + 0x2e34) = 0;
  param_1[0x5c7] = 0;
  *(undefined4 *)((long)param_1 + 0x2e4c) = 0;
  param_1[0x434] = 0;
  param_1[0x435] = 0;
  param_1[0x436] = 0;
  param_1[0x437] = 0;
  param_1[0x438] = 0;
  param_1[0x439] = 0;
  param_1[0x5c8] = 0;
  *(undefined4 *)(param_1 + 0x5c9) = 0;
  *(undefined2 *)((long)param_1 + 0x458894) = 0;
  do {
    while ((void *)param_1[lVar4 + 0x8b104] == (void *)0x0) {
      *(undefined1 *)((long)param_1 + lVar4 + 0x458840) = 0;
      lVar4 = lVar4 + 1;
      if (lVar4 == 4) goto LAB_0013153c;
    }
    free((void *)param_1[lVar4 + 0x8b104]);
    param_1[lVar4 + 0x8b104] = 0;
    *(undefined1 *)((long)param_1 + lVar4 + 0x458840) = 0;
    lVar4 = lVar4 + 1;
  } while (lVar4 != 4);
LAB_0013153c:
  reset_video_2d(param_1 + 0x5cf);
  reset_video_2d(param_1 + 0x10853);
  reset_texture_cache(param_1 + 0x69d98);
  reset_geometry(param_1 + 0x6ad9e);
  reset_video_3d(param_1 + 0x20ad8);
  return;
}

// --- 函数: get_ticks_us ---

void get_ticks_us(long *param_1)

{
  bool bVar1;
  timeval local_18;
  long local_8;
  
  // 添加空指针检查
  if (param_1 == (long *)0x0) {
    fprintf(stderr, "ERROR: get_ticks_us called with NULL pointer\n");
    return;
  }
  
  local_8 = (long)__stack_chk_guard;
  if (gettimeofday(&local_18,(__timezone_ptr_t)0x0) != 0) {
    fprintf(stderr, "ERROR: gettimeofday failed\n");
    *param_1 = 0;
    return;
  }
  bVar1 = local_8 == __stack_chk_guard;
  *param_1 = (long)(local_18.tv_usec + local_18.tv_sec * 1000000L);
  if (bVar1) {
    return;
  }
                    
  __stack_chk_fail();
}

// --- 函数: reset_rtc ---

void reset_rtc(long param_1,int param_2,time_t param_3)

{
  char *pcVar1;
  time_t tVar2;
  time_t local_8;
  
  *(undefined1 *)(param_1 + 0x18) = 0;
  *(undefined2 *)(param_1 + 0x1a) = 2;
  *(undefined4 *)(param_1 + 0x1c) = 0;
  local_8 = param_3;
  if (param_2 != 0) {
    pcVar1 = ctime(&local_8);
    printf("Using custom RTC time: %s",pcVar1);
    *(time_t *)(param_1 + 8) = local_8;
    return;
  }
  tVar2 = time((time_t *)0x0);
  *(time_t *)(param_1 + 8) = tVar2;
  return;
}

// --- 函数: update_screens ---

void update_screens(void)

{
  uint uVar1;
  undefined8 local_18;
  undefined8 uStack_10;
  long local_8;
  int render_result;
  
  uVar1 = (uint)DAT_040315cc;
  local_8 = (long)__stack_chk_guard;
  
#ifdef DRASTIC_LIBRETRO
  // libretro 模式：不进行 SDL 渲染，只确保屏幕数据已准备好
  // RetroArch 会通过 video_cb 回调来处理实际的渲染
  // 屏幕数据应该已经在模拟器运行过程中更新了
  // 这里只需要处理一些状态更新，比如菜单状态
  
  // 处理菜单覆盖层倒计时
  if (DAT_040315ec != 0) {
    DAT_040315ec = DAT_040315ec + -1;
  }
  
  // 处理菜单状态
  if ((int)DAT_040315d4 != 0) {
    set_screen_menu_off();
  }
  
  // 在 libretro 模式下，屏幕数据通过 get_screen_ptr() 和 screen_copy16() 获取
  // 不需要更新 SDL 纹理，因为 RetroArch 会直接使用原始屏幕缓冲区数据
  
#else
  // 非 libretro 模式：使用 SDL 进行渲染
  
  // 先清除渲染目标，确保画面正确显示
  SDL_SetRenderDrawColor(DAT_04031578, 0, 0, 0, 255);
  render_result = SDL_RenderClear(DAT_04031578);
  if (render_result != 0) {
    fprintf(stderr, "WARNING: SDL_RenderClear failed: %s\n", SDL_GetError());
    fflush(stderr);
  }
  
  if (DAT_04031540 != '\0') {
    if (SDL_screen != NULL) {
      local_18 = DAT_04031530;
      uStack_10 = 0xc000000100;
      render_result = SDL_RenderCopy(DAT_04031578,SDL_screen,0,(SDL_Rect *)&local_18);
      if (render_result != 0) {
        fprintf(stderr, "WARNING: SDL_RenderCopy failed for SDL_screen: %s\n", SDL_GetError());
        fflush(stderr);
      }
    } else {
      fprintf(stderr, "WARNING: SDL_screen is NULL, cannot render\n");
      fflush(stderr);
    }
  }
  if (DAT_04031568 != '\0') {
    if (DAT_04031548 != NULL) {
      local_18 = DAT_04031558;
      uStack_10 = 0xc000000100;
      render_result = SDL_RenderCopy(DAT_04031578,DAT_04031548,0,(SDL_Rect *)&local_18);
      if (render_result != 0) {
        fprintf(stderr, "WARNING: SDL_RenderCopy failed for DAT_04031548: %s\n", SDL_GetError());
        fflush(stderr);
      }
    } else {
      fprintf(stderr, "WARNING: DAT_04031548 is NULL, cannot render\n");
      fflush(stderr);
    }
  }
  if (DAT_040315ec != 0) {
    DAT_040315ec = DAT_040315ec + -1;
    uStack_10 = CONCAT44((undefined4)DAT_040315e4,(undefined4)DAT_040315e4);
    local_18 = CONCAT44((int)((ulong)DAT_040315dc >> 0x20) +
                        (int)((ulong)(&DAT_04031530)[(ulong)(uVar1 ^ 1) * 5] >> 0x20),
                        (int)DAT_040315dc + (int)(&DAT_04031530)[(ulong)(uVar1 ^ 1) * 5]);
    if (*(int*)((char*)&DAT_040315e4 + 4) == 0) {
      if (DAT_04031588 != NULL) {
        render_result = SDL_RenderCopy(DAT_04031578,DAT_04031588,0,(SDL_Rect *)&local_18);
        if (render_result != 0) {
          fprintf(stderr, "WARNING: SDL_RenderCopy failed for DAT_04031588: %s\n", SDL_GetError());
          fflush(stderr);
        }
      }
    }
    else {
      if (DAT_04031590 != NULL) {
        render_result = SDL_RenderCopy(DAT_04031578,DAT_04031590,0,(SDL_Rect *)&local_18);
        if (render_result != 0) {
          fprintf(stderr, "WARNING: SDL_RenderCopy failed for DAT_04031590: %s\n", SDL_GetError());
          fflush(stderr);
        }
      }
    }
  }
  SDL_RenderPresent(DAT_04031578);
  if ((int)DAT_040315d4 != 0) {
    set_screen_menu_off();
  }
#endif // DRASTIC_LIBRETRO
  
  if (local_8 != (long)__stack_chk_guard) {
                    
    __stack_chk_fail();
  }
  return;
}

// --- 函数: reset_translation_cache ---

void reset_translation_cache(long param_1)

{
  // param_1 是绝对地址（例如 nds_system + 0x11800），不需要再加 nds_system
  *(long *)(param_1 + 0xf24000) = param_1;
  *(long *)(param_1 + 0xf24008) = param_1 + 0x1000000;
  *(long *)(param_1 + 0xf24010) = param_1 + 0x1000000;
  *(long *)(param_1 + 0xf24018) = param_1 + 0x1100000;
  *(long *)(param_1 + 0xf24020) = param_1 + 0x1100000;
  *(long *)(param_1 + 0xf24028) = param_1 + 0x1300000;
  return;
}

// --- 函数: set_font_narrow_small ---

void set_font_narrow_small(void)

{
  current_font = font_a;
  return;
}

// --- 函数: reset_input ---

void reset_input(long param_1)

{
  long *__filename;
  int iVar1;
  struct stat asStack_8a8 [15];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  //__filename = asStack_8a8[0].__unused + 1;
  //snprintf((char*)__filename,0x820,"%s%cinput_record%c%s.ir",(char*)(*(long *)(param_1 + 0x80008) + 0x8a780),
  //              0x2f,0x2f,(char*)(*(long *)(param_1 + 0x80008) + 0x8b380));
  //iVar1 = __xstat(0,(char *)__filename,asStack_8a8);
  //if (iVar1 == 0) {
  ///  input_log_playback(param_1,__filename);
  //}
  //*(long *)(param_1 + 0x80000) = param_1;
  //*(undefined8 *)(param_1 + 0x80010) = 0;
  //*(undefined4 *)(param_1 + 0x80018) = 0;
  //*(undefined2 *)(param_1 + 0x8001c) = 0;
  //*(undefined1 *)(param_1 + 0x8003c) = 0;
  platform_initialize_input();
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: reset_cpu ---

void reset_cpu(long param_1)

{
  *(undefined8 *)(param_1 + 0x2370) = 0;
  *(undefined8 *)(param_1 + 0x2378) = 0;
  *(undefined8 *)(param_1 + 0x2380) = 0;
  *(undefined8 *)(param_1 + 0x2388) = 0;
  *(undefined8 *)(param_1 + 0x2390) = 0;
  *(undefined8 *)(param_1 + 0x2398) = 0;
  *(undefined8 *)(param_1 + 0x23a0) = 0;
  *(undefined8 *)(param_1 + 0x23a8) = 0;
  *(undefined4 *)(param_1 + 0x2110) = 0;
  *(undefined4 *)(param_1 + 0x2290) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x23a4) = 0x2400000;
  *(undefined4 *)(param_1 + 0x23c0) = 0x1f;
  *(undefined8 *)(param_1 + 0x2104) = 0;
  *(undefined8 *)(param_1 + 0x14) = 0x10000;
  *(undefined1 *)(param_1 + 0x1c) = 0;
  *(undefined1 *)(param_1 + 0x1e) = 0;
  *(undefined8 *)(param_1 + 0x34) = 0x10000;
  *(undefined1 *)(param_1 + 0x3c) = 0;
  *(undefined1 *)(param_1 + 0x3e) = 0;
  *(undefined8 *)(param_1 + 0x54) = 0x10000;
  *(undefined1 *)(param_1 + 0x5c) = 0;
  *(undefined1 *)(param_1 + 0x5e) = 0;
  *(undefined8 *)(param_1 + 0x74) = 0x10000;
  *(undefined1 *)(param_1 + 0x7c) = 0;
  *(undefined1 *)(param_1 + 0x7e) = 0;
  *(undefined4 *)(param_1 + 0x22a8) = 0;
  *(undefined8 *)(param_1 + 0x23b8) = 0;
  reset_debug(param_1 + 0x2118);
  return;
}

// --- 函数: load_config_file ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void chomp_whitespace(char *param_1)

{
  int iVar1;
  size_t sVar2;
  ushort **ppuVar3;
  long lVar4;
  
  sVar2 = strlen(param_1);
  iVar1 = (int)sVar2 + -1;
  if (-1 < iVar1) {
    ppuVar3 = __ctype_b_loc();
    lVar4 = (long)iVar1;
    do {
      if (((*ppuVar3)[(byte)param_1[lVar4]] >> 0xd & 1) == 0) {
        return;
      }
      param_1[lVar4] = '\0';
      lVar4 = lVar4 + -1;
    } while (-1 < (int)lVar4);
  }
  return;
}
*/

undefined8 load_config_file(long param_1,undefined8 param_2,undefined4 param_3)

{
  int iVar1;
  FILE *__stream;
  char *pcVar2;
  char *__s1;
  long lVar3;
  undefined8 uVar4;
  int local_838;
  undefined1 auStack_834 [4];
  undefined1 auStack_830 [8];
  char acStack_828 [1024];
  const char acStack_428 [1056];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  snprintf(acStack_428,0x420,"%s%cconfig%c%s",param_1 + 0x8ab80,0x2f,0x2f,param_2);
  printf("Loading config file %s\n",acStack_428);
  __stream = fopen(acStack_428,"rb");
  if (__stream == (FILE *)0x0) {
    uVar4 = 0xffffffff;
    printf("Config file %s does not exist.\n",acStack_428);
  }
  else {
    fread(&local_838,4,1,__stream);
    fread(auStack_834,4,1,__stream);
    fread(auStack_830,8,1,__stream);
    if (local_838 == 0x46435344) {
      fclose(__stream);
      uVar4 = 0;
      iVar1 = load_config_file_binary(param_1,param_2,param_3);
      if (iVar1 == -1) {
        uVar4 = 0xffffffff;
      }
      else {
        printf(" Saving converted config file %s.\n",acStack_428);
        save_config_file(param_1,param_2,param_3);
      }
    }
    else {
      fseek(__stream,0,0);
      while (pcVar2 = fgets(acStack_828,0x400,__stream), pcVar2 != (char *)0x0) {
        pcVar2 = strchr(acStack_828,0x3d);
        if (pcVar2 != (char *)0x0) {
          *pcVar2 = '\0';
          __s1 = (char *)skip_whitespace(acStack_828);
          chomp_whitespace(param_1);
          pcVar2 = (char *)skip_whitespace(pcVar2 + 1);
          chomp_whitespace(param_1);
          iVar1 = strcasecmp(__s1,"frameskip_type");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x859e8) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"frameskip_value");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x859ec) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"safe_frameskip");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a54) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"show_frame_counter");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x859f0) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"screen_orientation");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x859f4) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"screen_swap");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x859fc) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"savestate_number");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a00) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"fast_forward");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a04) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"enable_sound");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a08) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"clock_speed");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a0c) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"threaded_3d");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a10) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"mirror_touch");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a14) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"compress_savestates");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a18) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"savestate_snapshot");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a1c) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"unzip_roms");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a24) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"preload_roms");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a28) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"backup_in_savestates");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a2c) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"ignore_gamecard_limit");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a30) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"frame_interval");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a34) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"trim_roms");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a3c) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"fix_main_2d_screen");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a40) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"disable_edge_marking");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a44) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"interframe_blend");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a58) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"hires_3d");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a48) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"use_rtc_custom_time");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a64) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"rtc_custom_time");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(long *)(param_1 + 0x85a68) = lVar3;
          }
          iVar1 = strcasecmp(__s1,"rtc_system_time");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a70) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"slot2_device_type");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a5c) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"rumble_frames");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a60) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"firmware.username");
          if (iVar1 == 0) {
            mbstowcs((wchar_t *)(param_1 + 0x855a8),pcVar2,10);
            *(undefined4 *)(param_1 + 0x855d0) = 0;
          }
          iVar1 = strcasecmp(__s1,"firmware.language");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x855d4) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"firmware.favorite_color");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x855d8) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"firmware.birthday_month");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x855dc) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"firmware.birthday_day");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x855e0) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"enable_cheats");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a20) = (int)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86274) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86276) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86278) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8627a) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_A]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8627c) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_B]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8627e) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_X]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86280) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_Y]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86282) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_L]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86284) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_R]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86286) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_START]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86288) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_SELECT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8628a) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_HINGE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8628c) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_TOUCH_CURSOR_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8628e) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_TOUCH_CURSOR_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86290) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_TOUCH_CURSOR_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86292) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_TOUCH_CURSOR_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86294) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_TOUCH_CURSOR_PRESS]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86296) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_MENU]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86298) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_SAVE_STATE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8629a) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_LOAD_STATE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8629c) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_FAST_FORWARD]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8629e) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_SWAP_SCREENS]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862a0) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_SWAP_ORIENTATION_A]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862a2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_SWAP_ORIENTATION_B]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862a4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_LOAD_GAME]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862a6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_QUIT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862a8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_FAKE_MICROPHONE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862aa) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862b2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862b4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862b6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862b8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_SELECT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ba) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_BACK]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862be) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_EXIT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862bc) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_PAGE_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862c0) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_PAGE_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862c2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_a[CONTROL_INDEX_UI_SWITCH]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862c4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862c6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862c8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ca) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862cc) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_A]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ce) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_B]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862d0) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_X]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862d2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_Y]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862d4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_L]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862d6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_R]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862d8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_START]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862da) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_SELECT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862dc) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_HINGE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862de) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_TOUCH_CURSOR_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862e0) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_TOUCH_CURSOR_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862e2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_TOUCH_CURSOR_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862e4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_TOUCH_CURSOR_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862e6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_TOUCH_CURSOR_PRESS]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862e8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_MENU]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ea) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_SAVE_STATE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ec) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_LOAD_STATE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862ee) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_FAST_FORWARD]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862f0) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_SWAP_SCREENS]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862f2) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_SWAP_ORIENTATION_A]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862f4) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_SWAP_ORIENTATION_B]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862f6) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_LOAD_GAME]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862f8) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_QUIT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862fa) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_FAKE_MICROPHONE]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x862fc) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86304) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86306) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_LEFT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86308) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_RIGHT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8630a) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_SELECT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8630c) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_BACK]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86310) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_EXIT]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x8630e) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_PAGE_UP]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86312) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_PAGE_DOWN]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86314) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"controls_b[CONTROL_INDEX_UI_SWITCH]");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(short *)(param_1 + 0x86316) = (short)lVar3;
          }
          iVar1 = strcasecmp(__s1,"batch_threads_3d_count");
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a38) = (int)lVar3;
            iVar1 = strcasecmp(__s1,"bypass_3d");
          }
          else {
            iVar1 = strcasecmp(__s1,"bypass_3d");
          }
          if (iVar1 == 0) {
            lVar3 = strtol(pcVar2,(char **)0x0,10);
            *(int *)(param_1 + 0x85a4c) = (int)lVar3;
          }
        }
      }
      uVar4 = 0;
      config_update_settings((wchar_t *)(param_1 + 0x855a8));
    }
  }
  if (local_8 - __stack_chk_guard == 0) {
    return uVar4;
  }
                    
  __stack_chk_fail();
}

// --- 函数: reset_event_list ---

void reset_event_list(long param_1)

{
  *(undefined8 *)(param_1 + 0x300) = 0;
  return;
}

// --- 函数: execute_events ---
// 修改函数，使用正确的函数指针类型
void execute_events(long param_1)
{
    typedef void (*EventCallback)(void*, uint64_t);  // 本地定义正确的类型
    
    EventCallback pcVar1;
    undefined8 uVar2;
    uint uVar3;
    uint uVar4;
    uint *puVar5;
    
    uVar3 = *(uint *)(param_1 + 0x10);
    puVar5 = *(uint **)(param_1 + 0x318);
    if (puVar5 == (uint *)0x0) {
        return;
    }
    uVar4 = *puVar5;
    *(ulong *)(param_1 + 8) = *(long *)(param_1 + 8) + (ulong)uVar3;
    
    if (uVar3 < uVar4) {
        *puVar5 = uVar4 - uVar3;
    }
    else {
        do {
            // 重要：puVar5是uint*，所以+2偏移是8字节（如果uint是4字节）
            pcVar1 = *(EventCallback *)((char*)puVar5 + 8);  // 偏移8字节
            uVar2 = *(undefined8 *)((char*)puVar5 + 16);     // 偏移16字节
            *(undefined8 *)(param_1 + 0x318) = *(undefined8 *)((char*)puVar5 + 24);
            
            // 调用函数指针
            pcVar1((void*)param_1, uVar2);
            
            puVar5 = *(uint **)(param_1 + 0x318);
            if (puVar5 == (uint *)0x0) {
                return;
            }
            puVar5[8] = 0;
            puVar5[9] = 0;
        } while (*puVar5 == 0);
    }
    return;
}
/*
void execute_events(long param_1)

{
  code *pcVar1;
  undefined8 uVar2;
  uint uVar3;
  uint uVar4;
  uint *puVar5;
  
  uVar3 = *(uint *)(param_1 + 0x10);
  puVar5 = *(uint **)(param_1 + 0x318);
  uVar4 = *puVar5;
  *(ulong *)(param_1 + 8) = *(long *)(param_1 + 8) + (ulong)uVar3;
  if (uVar3 < uVar4) {
    *puVar5 = uVar4 - uVar3;
  }
  else {
    do {
      pcVar1 = *(code2 **)(puVar5 + 2);
      uVar2 = *(undefined8 *)(puVar5 + 4);
      *(undefined8 *)(param_1 + 0x318) = *(undefined8 *)(puVar5 + 6);
      (*pcVar1)(param_1,uVar2);
      puVar5 = *(uint **)(param_1 + 0x318);
      if (puVar5 == (uint *)0x0) {
        return;
      }
      puVar5[8] = 0;
      puVar5[9] = 0;
    } while (*puVar5 == 0);
  }
  return;
}
*/
// --- 函数: execute_arm_raise_interrupt ---

void execute_arm_raise_interrupt(long param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  
  uVar3 = *(uint *)(param_1 + 0x23bc);
  uVar1 = *(uint *)(param_1 + 0x2104);
  if ((uVar3 & 1) == 0) {
    iVar2 = uVar3 + 4;
    if (uVar1 != 2) goto LAB_00125484;
    *(int *)(param_1 + 0x23a8) = iVar2;
LAB_00125508:
    uVar3 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + 0x20f0) = uVar3;
  }
  else {
    *(uint *)(param_1 + 0x23bc) = uVar3 & 0xfffffffe;
    iVar2 = (uVar3 & 0xfffffffe) + 4;
    if (uVar1 == 2) {
      *(int *)(param_1 + 0x23a8) = iVar2;
    }
    else {
LAB_00125484:
      *(undefined8 *)(param_1 + (ulong)uVar1 * 8 + 0x2090) = *(undefined8 *)(param_1 + 0x23a4);
      if (uVar1 == 1) {
        *(undefined8 *)(param_1 + 0x23a0) = *(undefined8 *)(param_1 + 0x20d8);
        *(undefined8 *)(param_1 + 0x2398) = *(undefined8 *)(param_1 + 0x20d0);
        *(undefined8 *)(param_1 + 0x2390) = *(undefined8 *)(param_1 + 0x20c8);
      }
      else {
        *(undefined4 *)(param_1 + 0x23a4) = *(undefined4 *)(param_1 + 0x20a0);
      }
      *(undefined4 *)(param_1 + 0x2104) = 2;
      *(int *)(param_1 + 0x23a8) = iVar2;
      if ((uVar3 & 1) == 0) goto LAB_00125508;
    }
    uVar3 = *(uint *)(param_1 + 0x23c0);
    *(uint *)(param_1 + 0x20f0) = uVar3 | 0x20;
  }
  iVar2 = 0x18;
  if (*(int *)(param_1 + 0x210c) == 1) {
    iVar2 = *(int *)(*(long *)(param_1 + 0x2250) + 0x10) + 0x18;
  }
  *(int *)(param_1 + 0x23bc) = iVar2;
  *(uint *)(param_1 + 0x23c0) = uVar3 & 0xffffffc0 | 0x92;
  return;
}

// --- 函数: strtol ---

long strtol(char *__nptr,char **__endptr,int __base)

{
  long lVar1;
  
  lVar1 = strtol(__nptr,__endptr,__base);
  return lVar1;
}

// --- 函数: input_log_record ---

void input_log_record(long param_1,const char *param_2)

{
  FILE *pFVar1;
  
  pFVar1 = fopen(param_2,"wb");
  *(FILE **)(param_1 + 0x80048) = pFVar1;
  if (pFVar1 != (FILE *)0x0) {
    printf("Recording input to %s.\n",param_2);
    *(undefined1 *)(param_1 + 0x80050) = 1;
    return;
  }
  printf("Couldn\'t open %s for input recording.\n",param_2);
  return;
}

// --- 函数: input_log_playback ---

void input_log_playback(void *param_1,const char *param_2)

{
  uint uVar1;
  FILE *__stream;
  long lVar2;
  size_t sVar3;
  
  __stream = fopen(param_2,"rb");
  if (__stream == (FILE *)0x0) {
    printf("Couldn\'t open %s for input playback.\n",param_2);
    return;
  }
  fseek(__stream,0,2);
  lVar2 = ftell(__stream);
  fseek(__stream,0,0);
  uVar1 = (uint)lVar2;
  if (0x7ffce < (uint)lVar2) {
    uVar1 = 0x7ffce;
  }
  sVar3 = fread(param_1,(ulong)uVar1,1,__stream);
  if (sVar3 != 1) {
    puts("ERROR: Couldn\'t read back from capture buffer.");
  }
  fclose(__stream);
  printf("Playing back input from %s (%d input events).\n",param_2,(ulong)uVar1 / 10);
  *(undefined4 *)((long)param_1 + (ulong)uVar1) = 0;
  *(undefined1 *)((long)param_1 + 0x80050) = 2;
  return;
}

// --- 函数: getopt_long ---

void getopt_long(void)

{
  getopt_long();
  return;
}

// --- 函数: set_debug_mode ---

void set_debug_mode(long param_1,undefined1 param_2)

{
  *(undefined1 *)(param_1 + 0x131) = param_2;
  *(undefined1 *)(param_1 + 0x132) = param_2;
  return;
}

// --- 函数: strtoull ---

ulonglong strtoull(char *__nptr,char **__endptr,int __base)

{
  ulonglong uVar1;
  
  uVar1 = strtoull(__nptr,__endptr,__base);
  return uVar1;
}

// --- 函数: config_default ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void config_default(undefined4 *param_1)

{
  uint uVar1;
  undefined4 *__s;
  ushort uVar2;
  long lVar3;
  ushort *puVar4;
  ulong uVar5;
  
  lVar3 = __stack_chk_guard;
  puVar4 = (ushort *)((long)param_1 + 0xd1e);
  puts("Setting default configuration.");
  *(undefined8 *)(param_1 + 0xd) = 0x100000001;
  *(undefined8 *)(param_1 + 0xb) = 1;
  *(undefined8 *)(param_1 + 0x112) = 0x100000001;
  *(undefined8 *)(param_1 + 0x110) = 0x400000002;
  *(undefined8 *)(param_1 + 0x114) = 0;
  *(undefined8 *)(param_1 + 0x119) = 0;
  *(undefined8 *)(param_1 + 0x117) = 0x100000000;
  *(undefined8 *)(param_1 + 0x11d) = 0x100000001;
  *(undefined8 *)(param_1 + 0x11b) = 0x100000000;
  *(undefined8 *)(param_1 + 0x121) = 1;
  *(undefined8 *)(param_1 + 0x11f) = 0;
  *(undefined8 *)(param_1 + 0x125) = 0;
  *(undefined8 *)(param_1 + 0x123) = 0x300000000;
  *(undefined8 *)(param_1 + 0x129) = 0x100000000;
  *(undefined8 *)(param_1 + 0x127) = 0;
  *(undefined8 *)(param_1 + 0x12d) = 0x300000000;
  *(undefined8 *)(param_1 + 299) = 0;
  *(undefined8 *)(param_1 + 0x12f) = 0;
  *(undefined8 *)(param_1 + 0x131) = 0;
  *param_1 = 0x44;
  param_1[1] = 0x72;
  param_1[2] = 0x20;
  param_1[3] = 0x44;
  param_1[4] = 0x72;
  param_1[5] = 0x61;
  param_1[6] = 0x53;
  param_1[7] = 0x74;
  param_1[8] = 0x69;
  param_1[9] = 99;
  param_1[10] = 0;
  *(undefined8 *)(param_1 + 0x333) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x335) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x337) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x339) = 0xffffffffffffffff;
  __s = param_1 + 0x35c;
  *(undefined8 *)(param_1 + 0x33b) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x33d) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x33f) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x341) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x343) = 0xffffffffffffffff;
  *(undefined8 *)(param_1 + 0x345) = 0xffffffffffffffff;
  *(undefined2 *)(param_1 + 0x347) = 0xffff;
  puVar4[0] = 0xffff;
  puVar4[1] = 0xffff;
  puVar4[2] = 0xffff;
  puVar4[3] = 0xffff;
  *(undefined8 *)((long)param_1 + 0xd26) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd2e) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd36) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd3e) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd46) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd4e) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd56) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd5e) = 0xffffffffffffffff;
  *(undefined8 *)((long)param_1 + 0xd66) = 0xffffffffffffffff;
  *(undefined2 *)((long)param_1 + 0xd6e) = 0xffff;
  platform_set_default_controls((undefined8 *)(param_1 + 0x333),(undefined8 *)puVar4);
  set_screen_orientation(param_1[0x113]);
  set_screen_swap(param_1[0x115]);
  memset(__s,0,0x4000);
  uVar5 = 0;
  do {
    uVar2 = puVar4[-0x29];
    if (uVar2 != 0xffff) {
      *(ulong *)(__s + (ulong)uVar2 * 2) = *(ulong *)(__s + (ulong)uVar2 * 2) | 1L << (uVar5 & 0x3f)
      ;
    }
    uVar2 = *puVar4;
    if (uVar2 != 0xffff) {
      *(ulong *)(__s + (ulong)uVar2 * 2) = *(ulong *)(__s + (ulong)uVar2 * 2) | 1L << (uVar5 & 0x3f)
      ;
    }
    uVar1 = (int)uVar5 + 1;
    uVar5 = (ulong)uVar1;
    puVar4 = puVar4 + 1;
  } while (uVar1 != 0x29);
  if (lVar3 - __stack_chk_guard == 0) {
    return;
  }
                    
  __stack_chk_fail();
}
*/

// --- 函数: initialize_system_directories ---

void initialize_system_directories(undefined8 param_1)

{
  initialize_system_directory(param_1,"backup");
  initialize_system_directory(param_1,"savestates");
  initialize_system_directory(param_1,"config");
  initialize_system_directory(param_1,"profiles");
  initialize_system_directory(param_1,"unzip_cache");
  initialize_system_directory(param_1,"system");
  initialize_system_directory(param_1,"input_record");
  initialize_system_directory(param_1,"cheats");
  initialize_system_directory(param_1,"slot2");
  initialize_system_directory(param_1,"microphone");
  initialize_system_directory(param_1,"scripts");
  return;
}

// --- 函数: quit ---

void quit(long param_1)

{
  double dVar1;
  double dVar2;
  
  if (nds_system[param_1 + 0x362e9a8] == '\0') {
    cpu_print_profiler_results();
  }
  /*
  printf(((double)(mini_hash_accesses - mini_hash_misses) * 100.0) /
               (double)mini_hash_accesses,1,"%lu mini hash hits out of %lu accesses (%lf%%)\n");
  printf("%lu hash accesses:\n",hash_accesses);
  dVar2 = (double)NEON_ucvtf(hash_accesses);
  dVar1 = (double)NEON_ucvtf(hash_hit_lengths);
  printf((dVar1 * 100.0) / dVar2,1," %lf%% hit in one hop\n");
  dVar1 = (double)NEON_ucvtf(DAT_00269018);
  dVar2 = (double)NEON_ucvtf(hash_accesses);
  printf((dVar1 * 100.0) / dVar2,1," %lf%% hit in two hops\n");
  dVar1 = (double)NEON_ucvtf(DAT_00269020);
  dVar2 = (double)NEON_ucvtf(hash_accesses);
  printf((dVar1 * 100.0) / dVar2,1," %lf%% hit in three hops\n");
  dVar2 = (double)NEON_ucvtf(DAT_00269028);
  dVar1 = (double)NEON_ucvtf(hash_accesses);
  printf((dVar2 * 100.0) / dVar1,1," %lf%% hit in four or more hops\n");
  */
  save_directory_config_file(param_1,"drastic.cf2");
  if (*(char *)(param_1 + 0x8b380) != '\0') {
    gamecard_close(param_1 + 800);
  }
  audio_exit(param_1 + 0x1587000);
  input_log_close(param_1 + 0x5550);
  uninitialize_memory((long)param_1 + 0x35d4930);
  platform_quit();
  fflush(stdout);
                    
  exit(0);
}

// --- 函数: initialize_input ---

void initialize_input(long param_1, undefined8 param_2)

{
  *(undefined8 *)(param_1 + 0x80008) = param_2;
  *(undefined8 *)(param_1 + 0x80048) = 0;
  *(undefined1 *)(param_1 + 0x80050) = 0;
  platform_initialize_input();
  return;
}

// --- 函数: initialize_spi_peripherals ---

void initialize_spi_peripherals(long param_1, long param_2)

{
  *(long *)(param_1 + 0x2448) = param_2;
  initialize_backup(param_1,1,param_2 + 0x35ff9a0,0x40000,0);
  *(undefined1 *)(param_1 + 0x2426) = 1;
  return;
}

// --- 函数: initialize_spu ---

undefined8 initialize_audio(long param_1)

{
  undefined4 uVar1;
  undefined4 local_48;
  undefined2 local_44;
  undefined1 local_42;
  undefined2 local_40;
  code *local_38;
  long lStack_30;
  undefined4 local_28 [8];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  printf("  Initializing audio with frequency %d.\n",*(undefined4 *)(param_1 + 0x40010),0);
  *(undefined4 *)(param_1 + 0x40018) = 0x1000;
  SDL_memset(&local_48,0,0x20);
  local_48 = *(undefined4 *)(param_1 + 0x40010);
  local_44 = 0x8010;
  local_42 = 2;
  local_40 = (undefined2)(*(uint *)(param_1 + 0x40018) >> 2);
  local_38 = (code *)audio_callback;
  lStack_30 = param_1;
  uVar1 = SDL_OpenAudio((SDL_AudioSpec*)&local_48,(SDL_AudioSpec*)local_28);
  *(undefined4 *)(param_1 + 0x40010) = local_28[0];
  printf("Got output device %d, frequency %d.\n",uVar1,local_28[0]);
  *(undefined1 *)(param_1 + 0x40027) = 0;
  SDL_PauseAudio(0);
  if (local_8 - __stack_chk_guard == 0) {
    return 0;
  }
                    // WARNING: Subroutine does not return
  __stack_chk_fail();
}


void initialize_spu(long param_1,long param_2)

{
  uint uVar1;
  ulong uVar2;
  int iVar3;
  uint uVar4;
  undefined1 *puVar5;
  undefined1 *puVar6;
  undefined1 uVar7;
  
  *(long *)(param_1 + 0x400c0) = param_2 + 0x35f7da0;
  *(undefined1 *)(param_1 + 0x400ea) = 0xff;
  *(long *)(param_1 + 0x40188) = param_2 + 0x35f7db0;
  *(undefined1 *)(param_1 + 0x401b2) = 0;
  *(long *)(param_1 + 0x40250) = param_2 + 0x35f7dc0;
  *(undefined1 *)(param_1 + 0x4027a) = 0xff;
  *(long *)(param_1 + 0x40318) = param_2 + 0x35f7dd0;
  *(long *)(param_1 + 0x403e0) = param_2 + 0x35f7de0;
  *(undefined1 *)(param_1 + 0x4040a) = 0xff;
  *(long *)(param_1 + 0x404a8) = param_2 + 0x35f7df0;
  *(undefined1 *)(param_1 + 0x404d2) = 0xff;
  *(long *)(param_1 + 0x40570) = param_2 + 0x35f7e00;
  *(undefined1 *)(param_1 + 0x4059a) = 0xff;
  *(long *)(param_1 + 0x40638) = param_2 + 0x35f7e10;
  *(undefined1 *)(param_1 + 0x40662) = 0xff;
  *(long *)(param_1 + 0x40700) = param_2 + 0x35f7e20;
  *(undefined1 *)(param_1 + 0x4072a) = 0xff;
  *(long *)(param_1 + 0x407c8) = param_2 + 0x35f7e30;
  *(undefined1 *)(param_1 + 0x407f2) = 0xff;
  *(long *)(param_1 + 0x40890) = param_2 + 0x35f7e40;
  *(undefined1 *)(param_1 + 0x408ba) = 0xff;
  *(long *)(param_1 + 0x40958) = param_2 + 0x35f7e50;
  *(undefined1 *)(param_1 + 0x40982) = 0xff;
  *(long *)(param_1 + 0x40a20) = param_2 + 0x35f7e60;
  *(undefined1 *)(param_1 + 0x40a4a) = 0xff;
  *(long *)(param_1 + 0x40ae8) = param_2 + 0x35f7e70;
  *(undefined1 *)(param_1 + 0x40b12) = 0xff;
  *(long *)(param_1 + 0x40bb0) = param_2 + 0x35f7e80;
  *(undefined1 *)(param_1 + 0x40bda) = 0xff;
  *(undefined1 *)(param_1 + 0x40ca2) = 0xff;
  *(undefined8 *)(param_1 + 0x40010) = 0xac440000ac44;
  *(long *)(param_1 + 0x40c78) = param_2 + 0x35f7e90;
  *(undefined2 *)(param_1 + 0x40020) = 0x101;
  *(long *)(param_1 + 0x40ce8) = param_2 + 0x35f7da0;
  *(undefined1 *)(param_1 + 0x40026) = 1;
  *(undefined1 *)(param_1 + 0x40342) = 1;
  *(long *)(param_1 + 0x40cf0) = param_2;
  *(long *)(param_1 + 0x40cf8) = param_2 + 0x35d4930;
  *(undefined8 *)(param_1 + 0x40d30) = 0;
  //initialize_audio(param_1,param_2 + 0x855a8);
  initialize_audio(param_1);
  uVar4 = *(uint *)(param_1 + 0x40010);
  uVar2 = 0;
  if ((ulong)uVar4 != 0) {
    uVar2 = 0x1006f43800 / (ulong)uVar4;
  }
  //iVar3 = SUB164((ZEXT416(uVar4) * (undefined1  [16])0x400000) / (undefined1  [16])0x401bd0e,0);
  iVar3 = SUB164((ZEXT416(uVar4) * 0x400000LL) / 0x401bd0eLL, 0);
  *(int *)(param_1 + 0x40d18) = (int)uVar2;
  *(int *)(param_1 + 0x40d20) = iVar3;
  //printf((double)(uVar2 & 0xffffffff) * 0.0009765625,(double)iVar3 * 2.384185791015625e-07,1,
  //             "%lf cycles per output sample (%lf samples per cycle)\n");
  uVar4 = 0x7fff;
  puVar5 = &noise_samples[0];
  do {
    uVar1 = uVar4 & 1;
    uVar7 = 0x80;
    uVar4 = uVar4 >> 1;
    if (uVar1 != 0) {
      uVar4 = uVar4 ^ 0x6000;
      uVar7 = 0x7f;
    }
    puVar6 = puVar5 + 1;
    *puVar5 = uVar7;
    puVar5 = puVar6;
  } while (puVar6 != &noise_samples[32768]);
  return;
}

// --- 函数: initialize_lua ---
/*
undefined8 initialize_lua(long param_1)

{
  undefined **ppuVar1;
  undefined8 uVar2;
  code *pcVar3;
  undefined **ppuVar4;
  
  DAT_004ec3e0 = param_1 + 0x5550;
  DAT_004ec3e8 = param_1 + 0x35d4930;
  _DAT_004ec3f8 = 0;
  lua_state = param_1;
  DAT_004ec3f0 = luaL_newstate();
  if (DAT_004ec3f0 == 0) {
    uVar2 = 0xffffffff;
  }
  else {
    pcVar3 = luaopen_base;
    ppuVar4 = &lua_load_libs;
    do {
      luaL_requiref(DAT_004ec3f0,*ppuVar4,pcVar3,1);
      lua_settop(DAT_004ec3f0,0xfffffffe);
      pcVar3 = (code *)ppuVar4[3];
      ppuVar4 = ppuVar4 + 2;
    } while (pcVar3 != (code *)0x0);
    luaL_checkversion_(0x43fb8000,DAT_004ec3f0,0x44);
    lua_createtable(DAT_004ec3f0,0,0x11);
    luaL_setfuncs(DAT_004ec3f0,&lua_drastic_module,0);
    lua_pushstring(DAT_004ec3f0,&DAT_00224b20);
    lua_createtable(DAT_004ec3f0,0xe,0);
    ppuVar4 = &lua_constants;
    do {
      ppuVar1 = ppuVar4 + 2;
      lua_pushstring(DAT_004ec3f0,*ppuVar4);
      lua_pushinteger(DAT_004ec3f0,*(undefined4 *)(ppuVar4 + 1));
      lua_settable(DAT_004ec3f0,0xfffffffd);
      ppuVar4 = ppuVar1;
    } while (ppuVar1 != (undefined **)&UNK_0025e018);
    lua_settable(DAT_004ec3f0,0xfffffffd);
    lua_setglobal(DAT_004ec3f0,"drastic");
    uVar2 = 0;
  }
  return uVar2;
}
*/
// --- 函数: platform_initialize ---

void platform_initialize()

{
  fprintf(stderr, "DEBUG: platform_initialize: calling SDL_Init\n");
  fflush(stderr);
  int result = SDL_Init(0x101230);
  if (result != 0) {
    fprintf(stderr, "ERROR: SDL_Init failed: %s\n", SDL_GetError());
    fflush(stderr);
  } else {
    fprintf(stderr, "DEBUG: platform_initialize: SDL_Init succeeded\n");
    fflush(stderr);
  }
  return;
}

// --- 函数: initialize_translation_cache ---

int initialize_translation_cache(void *param_1)

{
  int iVar1;
  
  *(undefined8 *)((long)param_1 + 0x14fa148) = 0;
  *(undefined8 *)((long)param_1 + 0x14fa150) = 0;
  mprotect(param_1,0x1000000,7);
  iVar1 = mprotect((void *)((long)param_1 + 0x1000000),0x100000,7);
  return iVar1;
}

// --- 函数: initialize_event_list ---

void event_hblank_start_function(long param_1)

{
  int iVar1;
  byte bVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  long lVar6;
  uint *puVar7;
  
  bVar2 = nds_system[param_1 + 0x30f39a4];
  nds_system[param_1 + 0x30f39a4] = bVar2 | 2;
  if ((bVar2 >> 4 & 1) != 0) {
    lVar6 = *(long *)(nds_system + param_1 + 0x10cddd0);
    uVar4 = *(uint *)(lVar6 + 0x214) | 2;
    *(uint *)(lVar6 + 0x214) = uVar4;
    if ((*(uint *)(nds_system + param_1 + 0x10cde60) & 6) == 0) {
      *(uint *)(nds_system + param_1 + 0x10cde58) =
           -*(int *)(lVar6 + 0x208) & uVar4 & *(uint *)(lVar6 + 0x210);
    }
  }
  bVar2 = nds_system[param_1 + 0x30fb9a4];
  nds_system[param_1 + 0x30fb9a4] = bVar2 | 2;
  if ((bVar2 >> 4 & 1) != 0) {
    lVar6 = *(long *)(nds_system + param_1 + 0x20d43c0);
    uVar4 = *(uint *)(lVar6 + 0x214) | 2;
    *(uint *)(lVar6 + 0x214) = uVar4;
    if ((*(uint *)(nds_system + param_1 + 0x20d4450) & 6) == 0) {
      *(uint *)(nds_system + param_1 + 0x20d4448) =
           -*(int *)(lVar6 + 0x208) & uVar4 & *(uint *)(lVar6 + 0x210);
    }
  }
  if (*(ushort *)(param_1 + 0x14) < 0xc0) {
    lVar6 = param_1 + 0x36d1bc8;
    if (((*(uint *)(nds_system + param_1 + 0x31d8dc8) >> 0x11 & 1) == 0) &&
       (nds_system[param_1 + 0x362e9a0] == '\0')) {
      iVar1 = *(int *)(nds_system + param_1 + 0x31d5bf8);
    }
    else {
      video_render_scanlines(param_1 + 0x36d1ec0, 0);
      iVar1 = *(int *)(nds_system + param_1 + 0x31d5bf8);
    }
    if ((iVar1 < 0) && (nds_system[param_1 + 0x31d5bfc] == '\x02')) {
      if (*(int *)(nds_system + param_1 + 0x31d5bf4) + 0xfa000000U < 0x800000) {
        video_render_scanlines(param_1 + 0x36d1ec0,*(undefined2 *)(param_1 + 0x14));
      }
      dma_transfer(lVar6,param_1 + 0x36d1bd8);
    }
    if ((*(int *)(nds_system + param_1 + 0x31d5c20) < 0) &&
       (nds_system[param_1 + 0x31d5c24] == '\x02')) {
      if (*(int *)(nds_system + param_1 + 0x31d5c1c) + 0xfa000000U < 0x800000) {
        video_render_scanlines(param_1 + 0x36d1ec0,*(undefined2 *)(param_1 + 0x14));
      }
      dma_transfer(lVar6,param_1 + 0x36d1c00);
    }
    if ((*(int *)(nds_system + param_1 + 0x31d5c48) < 0) &&
       (nds_system[param_1 + 0x31d5c4c] == '\x02')) {
      if (*(int *)(nds_system + param_1 + 0x31d5c44) + 0xfa000000U < 0x800000) {
        video_render_scanlines(param_1 + 0x36d1ec0,*(undefined2 *)(param_1 + 0x14));
      }
      dma_transfer(lVar6,param_1 + 0x36d1c28);
    }
    if ((*(int *)(nds_system + param_1 + 0x31d5c70) < 0) &&
       (nds_system[param_1 + 0x31d5c74] == '\x02')) {
      if (*(int *)(nds_system + param_1 + 0x31d5c6c) + 0xfa000000U < 0x800000) {
        video_render_scanlines(param_1 + 0x36d1ec0,*(undefined2 *)(param_1 + 0x14));
      }
      dma_transfer(lVar6,param_1 + 0x36d1c50);
    }
    if ((nds_system[param_1 + 0x362e99d] != '\0') && (*(short *)(param_1 + 0x14) == 0xbf)) {
      *(undefined2 *)(nds_system + param_1 + 0x30f39a6) = 0xc0;
      *(undefined2 *)(nds_system + param_1 + 0x30fb9a6) = 0xc0;
    }
  }
  lVar6 = param_1 + 0x48;
  if (*(uint **)(param_1 + 0x318) == (uint *)0x0) {
    *(undefined4 *)(param_1 + 0x48) = 0x4a4;
    *(undefined8 *)(param_1 + 0x60) = 0;
    *(undefined8 *)(param_1 + 0x68) = 0;
    *(long *)(param_1 + 0x318) = lVar6;
  }
  else {
    uVar4 = 0x4a4;
    puVar3 = *(uint **)(param_1 + 0x318);
    puVar7 = (uint *)0x0;
    while (puVar5 = puVar3, *puVar5 < uVar4) {
      uVar4 = uVar4 - *puVar5;
      puVar3 = *(uint **)(puVar5 + 6);
      puVar7 = puVar5;
      if (*(uint **)(puVar5 + 6) == (uint *)0x0) {
        *(uint *)(param_1 + 0x48) = uVar4;
        *(undefined8 *)(param_1 + 0x60) = 0;
        *(uint **)(param_1 + 0x68) = puVar5;
        *(long *)(puVar5 + 6) = lVar6;
        return;
      }
    }
    *(uint *)(param_1 + 0x48) = uVar4;
    *(uint **)(param_1 + 0x60) = puVar5;
    *(uint **)(param_1 + 0x68) = puVar7;
    if (puVar7 == (uint *)0x0) {
      *(long *)(param_1 + 0x318) = lVar6;
    }
    else {
      *(long *)(puVar7 + 6) = lVar6;
    }
    *(long *)(puVar5 + 8) = lVar6;
    *puVar5 = *puVar5 - uVar4;
  }
  return;
}

void event_gamecard_irq_function(undefined8 param_1,long param_2)

{
  uint uVar1;
  long lVar2;
  long lVar3;
  
  lVar2 = *(long *)(param_2 + 0x918);
  *(undefined1 *)(param_2 + 0x2da5) = 0;
  lVar3 = *(long *)(nds_system + lVar2 + 0x10cddd0);
  uVar1 = *(uint *)(lVar3 + 0x214) | 0x80000;
  *(uint *)(lVar3 + 0x214) = uVar1;
  if ((*(uint *)(nds_system + lVar2 + 0x10cde60) & 6) == 0) {
    *(uint *)(nds_system + lVar2 + 0x10cde58) =
         -*(int *)(lVar3 + 0x208) & uVar1 & *(uint *)(lVar3 + 0x210);
  }
  return;
}

void initialize_event_list(long param_1,long param_2)

{
  *(code1 *)(param_1 + 8) = event_hblank_start_function;
  *(long *)(param_1 + 0x10) = param_2;
  *(undefined1 *)(param_1 + 0x28) = 0;
  *(code1 *)(param_1 + 0x38) = reinterpret_cast<code1>(event_scanline_start_function);
  *(long *)(param_1 + 0x40) = param_2;
  *(undefined1 *)(param_1 + 0x58) = 1;
  *(code1 *)(param_1 + 0x68) = event_force_task_switch_function;
  *(undefined8 *)(param_1 + 0x70) = 0;
  *(undefined1 *)(param_1 + 0x88) = 2;
  *(code1 *)(param_1 + 0x218) = (code1)event_gamecard_irq_function;
  *(long *)(param_1 + 0x220) = param_2 + 800;
  *(undefined1 *)(param_1 + 0x238) = 0xb;
  *(undefined8 *)(param_1 + 0x300) = 0;
  *(undefined8 *)(param_1 + 0x318) = 0;
  return;
}

// --- 函数: initialize_gamecard ---

void initialize_gamecard(long param_1,long param_2)

{
  undefined1 auStack_428 [1056];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  //snprintf(auStack_428,0x420,"./game_database.xml");
  //initialize_game_database(param_1,auStack_428);
  //snprintf(auStack_428,0x420,"%s%cusrcheat.dat",param_2 + 0x8ab80,0x2f);
  //load_cheat_directory(param_1 + 0x28,auStack_428);
  *(undefined1 *)(param_1 + 0x8e9) = 0xff;
  *(undefined2 *)(param_1 + 0x8f8) = 0xf00f;
  *(long *)(param_1 + 0x900) = param_2 + 0x855a8;
  *(long *)(param_1 + 0x918) = param_2;
  *(undefined8 *)(param_1 + 0x920) = 0;
  *(long *)(param_1 + 0x928) = param_2 + 0x35ef9a0;
  *(undefined4 *)(param_1 + 0x93c) = 0xffffffff;
  *(long *)(param_1 + 0x948) = param_2 + 0x36d1bc8;
  *(long *)(param_1 + 0x950) = param_2 + 0x36d1c78;
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: load_directory_config_file ---

undefined8 load_directory_config_file(long param_1,undefined8 param_2)

{
  int iVar1;
  FILE *__stream;
  undefined8 uVar2;
  int local_438;
  uint local_434;
  undefined1 auStack_430 [8];
  char acStack_428 [1056];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  snprintf(acStack_428,0x420,"%s%cconfig%c%s",param_1 + 0x8ab80,0x2f,0x2f,param_2);
  printf("Loading directory config file %s\n",acStack_428);
  __stream = fopen(acStack_428,"rb");
  if (__stream == (FILE *)0x0) {
    uVar2 = 0xffffffff;
    printf("Directory config file %s does not exist.\n",acStack_428);
    goto LAB_001770d8;
  }
  fread(&local_438,4,1,__stream);
  fread(&local_434,4,1,__stream);
  fread(auStack_430,8,1,__stream);
  if ((local_438 == 0x32435344) && (uVar2 = 0, local_434 < 3)) {
    fread((char *)(param_1 + 0x855e4),0x400,1,__stream);
    if (1 < local_434) {
      fread((void *)(param_1 + 0x859e4),4,1,__stream);
    }
    iVar1 = chdir((char *)(param_1 + 0x855e4));
    if (iVar1 < 0) goto LAB_001770c0;
  }
  else {
LAB_001770c0:
    uVar2 = 0xffffffff;
    puts("ERROR: Directory config file could not be loaded.");
  }
  fclose(__stream);
LAB_001770d8:
  if (local_8 != (long)__stack_chk_guard) {
                    
    __stack_chk_fail();
  }
  return uVar2;
}

// --- 函数: menu_bios_warning ---

void menu_bios_warning(long param_1)

{
  int local_830 [2];
  undefined1 auStack_828 [2080];
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  set_font_narrow();
  set_screen_menu_on();
  snprintf(auStack_828,0x820,
                "Could not load system files.\n\nDraStic requires the following files in the system directory\n(%s%csystem):\n\nnds_bios_arm9.bin            4KB\nnds_bios_arm7.bin            16KB\n\nThese files can be extracted from a Nintendo DS, by using a\nflash cart with homebrew such as the following:\n\n"
                ,param_1 + 0x8a780,0x2f);
  print_string(auStack_828,0xffff,0,100,0);
  set_font_wide();
  print_string("http://library.dev-scene.com/index.php?dir=DS/Hardware (Firmware) 07/DSBF dump/",
               0xffff,0,100,0);
  update_screen_menu();
  update_screen_menu();
  update_screen_menu();
  do {
    get_gui_input(param_1 + 0x5550,local_830);
  } while (local_830[0] == 0xb);
  set_screen_menu_off();
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: initialize_memory ---

void initialize_memory(long *param_1, long param_2)

{
  // #region agent log
  debug_log("initialize_memory:entry", "Entering initialize_memory", "D", (void*)param_1, param_2, 0);
  // #endregion
  long lVar1;
  long lVar2;
  int iVar3;
  int iVar4;
  void *pvVar5;
  long lVar6;
  void *pvVar7;
  undefined8 uVar8;
  long lVar9;
  char local_108 [16];
  undefined8 local_f8;
  undefined1 uStack_f1;
  char local_f0 [2];
  undefined8 uStack_ee;
  long local_8;
  
  lVar1 = param_2 + 0x15c7d50;
  lVar2 = param_2 + 0x25ce340;
  *(undefined1 *)((long)param_1 + 0xfd513) = 1;
  param_1[0x1f74d] = param_2;
  param_1[0x1f74e] = param_2 + 0x36d1ec0;
  param_1[0x1f74f] = param_2 + 0x3a28bb0;
  param_1[0x1f750] = param_2 + 0x1587000;
  param_1[0x1f751] = param_2 + 0x15ca120;
  param_1[0x1f752] = param_2 + 0x25d0710;
  param_1[0x1fa97] = param_2 + 800;
  param_1[0x1fa98] = param_2 + 0x30d0;
  param_1[0x1fa99] = param_2 + 0x5528;
  // #region agent log
  debug_log("initialize_memory:before_first_pointer_assign", "Before first pointer assignments", "D", (void*)nds_system, param_2, 0);
  // #endregion
  *(long **)(param_2 + 0x20ce128) = param_1;
  *(long **)(param_2 + 0x30d4718) = param_1;
  *(long **)(param_2 + 0x20ce120) = param_1 + 0x1f753;
  *(long **)(param_2 + 0x30d4710) = param_1 + 0x1f8d3;
  *(long *)(param_2 + 0x20ce130) = lVar1;
  *(long *)(param_2 + 0x30d4720) = lVar2;
  local_8 = (long)__stack_chk_guard;
  iVar3 = getpagesize();
  *(int *)((long)param_1 + 0xfd4dc) = iVar3;
  // POSIX shared memory names must start with '/'
  snprintf(local_108, 16, "/memory.dat");
  iVar3 = shm_open(local_108,0x42,0x1ff);
  if (iVar3 < 0) {
    perror("shm_open failed");
    puts("ERROR: Memory map buffer failed.");
    exit(-1);
  }
  iVar4 = ftruncate(iVar3,0x414000);
  if (iVar4 < 0) {
    printf("Truncate of memory mapped file %s failed.\n",local_108);
    perror("ftruncate failed");
    close(iVar3);
    puts("ERROR: Memory map buffer failed.");
    exit(-1);
  }
  pvVar5 = mmap((void *)0x0,0x4000000,3,1,iVar3,0);
  shm_unlink(local_108);
  *(int *)(param_1 + 0x1fa9d) = iVar3;
  param_1[0x1fa9c] = (long)pvVar5;
  if (pvVar5 == (void *)0xffffffffffffffff) {
    puts("Memory map buffer failed. Trying large allocation.");
    close((int)param_1[0x1fa9d]);
    iVar3 = shm_open(local_108,0x42,0x1ff);
    if (iVar3 < 0) {
      perror("shm_open failed");
      puts("ERROR: Memory map buffer failed.");
      exit(-1);
    }
    iVar4 = ftruncate(iVar3,0x4000000);
    if (iVar4 < 0) {
      printf("Truncate of memory mapped file %s failed.\n",local_108);
      perror("ftruncate failed");
      close(iVar3);
      puts("ERROR: Memory map buffer failed.");
      exit(-1);
    }
    pvVar5 = mmap((void *)0x0,0x4000000,3,1,iVar3,0);
    shm_unlink(local_108);
    *(int *)(param_1 + 0x1fa9d) = iVar3;
    param_1[0x1fa9c] = (long)pvVar5;
    if (pvVar5 == (void *)0xffffffffffffffff) {
      puts("ERROR: Memory map buffer failed.");
      perror("Error is");
                    
      exit(-1);
    }
  }
  printf("Got memory mapped buffer at %p (%s)\n",pvVar5,local_108);
  pvVar5 = (void *)param_1[0x1fa9e];
  if (pvVar5 == (void *)0xffffffffffffffff) {
    pvVar5 = mmap((void *)0x0,0x4000000,3,1,(int)param_1[0x1fa9d],0);
    if (pvVar5 != (void *)0xffffffffffffffff) {
      printf("Got dynamic memory map low buffer @ %p (allocated 0x%x bytes)\n",pvVar5,
                   0x4000000);
      munmap(pvVar5,0x4000000);
      param_1[0x1fa9e] = (long)pvVar5;
      goto LAB_0011c044;
    }
LAB_0011c768:
    puts("ERROR: Dynamic memory map low buffer failed.");
LAB_0011c6d8:
    perror("Error is");
                    
    exit(-1);
  }
LAB_0011c044:
  pvVar5 = mmap(pvVar5,0x4000000,3,0x11,(int)param_1[0x1fa9d],0);
  if ((void *)param_1[0x1fa9e] != pvVar5) {
    pvVar5 = mmap((void *)0x0,0x4000000,3,1,(int)param_1[0x1fa9d],0);
    if (pvVar5 == (void *)0xffffffffffffffff) goto LAB_0011c768;
    printf("Got dynamic memory map low buffer @ %p\n",pvVar5);
    munmap(pvVar5,0x4000000);
    param_1[0x1fa9e] = (long)pvVar5;
    pvVar5 = mmap(pvVar5,0x4000000,3,0x11,(int)param_1[0x1fa9d],0);
    if ((void *)param_1[0x1fa9e] != pvVar5) {
      printf("ERROR: Memory map low buffer @ %08x to 0x4000000 failed.\n",0);
      goto LAB_0011c6d8;
    }
  }
  lVar6 = param_1[0x1fa9c];
  lVar9 = 0;
  *param_1 = lVar6;
  param_1[1] = lVar6 + 0x400000;
  param_1[2] = lVar6 + 0x408000;
  param_1[3] = lVar6 + 0x410000;
  printf("Using memory map offset %p\n");
  do {
    while( true ) {
      printf("Direct mapping main RAM to %x\n",(int)lVar9 + 0x2000000);
      pvVar5 = (void *)(lVar9 + param_1[0x1fa9e] + 0x2000000);
      lVar6 = 0;
      if (*(char *)((long)param_1 + 0xfd513) != '\0') break;
      do {
        munmap(pvVar5,0x4000);
        pvVar7 = mmap(pvVar5,0x4000,3,1,(int)param_1[0x1fa9d],lVar6);
        if (pvVar5 != pvVar7) {
          printf("ERROR: Low memory remap to %p didn\'t map to same location (got %p)\n",
                       pvVar5,pvVar7);
          goto LAB_0011c45c;
        }
        lVar6 = lVar6 + 0x4000;
        pvVar5 = (void *)((long)pvVar5 + 0x4000);
      } while (lVar6 != 0x400000);
joined_r0x0011c47c:
      lVar9 = lVar9 + 0x400000;
      if (lVar9 == 0x1000000) goto LAB_0011c1bc;
    }
    iVar3 = remap_file_pages(pvVar5,0x400000,0,0,0);
    if (iVar3 != 0) {
LAB_0011c45c:
      puts("Remap failed.");
      perror("Error is");
      goto joined_r0x0011c47c;
    }
    lVar9 = lVar9 + 0x400000;
  } while (lVar9 != 0x1000000);
LAB_0011c1bc:
  strncpy(local_108,"drastic_mapped_m",0x10);
  local_f8 = 0x765f79726f6d65;
  uStack_f1 = 0x72;
  local_f0[0] = 'a';
  local_f0[1] = 'm';
  uStack_ee = 0x7461642e;
  iVar3 = shm_open(local_108,0x42,0x1ff);
  iVar4 = ftruncate(iVar3,0xa8000);
  if (iVar4 < 0) {
    printf("Truncate of memory mapped file %s failed.\n",local_108);
  }
  shm_unlink(local_108);
  pvVar5 = mmap((void *)0x0,0x800000,3,1,iVar3,0);
  *(int *)(param_1 + 0x1faa1) = iVar3;
  param_1[0x1fa9f] = (long)pvVar5;
  if (pvVar5 == (void *)0xffffffffffffffff) {
    puts("Memory VRAM buffer map failed. Trying large allocation.");
    close((int)param_1[0x1faa1]);
    iVar3 = shm_open(local_108,0x42,0x1ff);
    iVar4 = ftruncate(iVar3,0x800000);
    if (iVar4 < 0) {
      printf("Truncate of memory mapped file %s failed.\n",local_108);
    }
    shm_unlink(local_108);
    pvVar5 = mmap((void *)0x0,0x800000,3,1,iVar3,0);
    *(int *)(param_1 + 0x1faa1) = iVar3;
    param_1[0x1fa9f] = (long)pvVar5;
    if (pvVar5 == (void *)0xffffffffffffffff) {
      puts("ERROR: Memory VRAM buffer map failed.");
      goto LAB_0011c6d8;
    }
  }
  pvVar5 = mmap((void *)0x0,0x800000,3,1,iVar3,0);
  param_1[0x1faa0] = (long)pvVar5;
  if (pvVar5 == (void *)0xffffffffffffffff) {
    puts("ERROR: Memory map VRAM failed.");
    goto LAB_0011c6d8;
  }
  lVar9 = param_1[0x1fa9f];
  param_1[0x2a04] = lVar9;
  param_1[0x2a05] = lVar9 + 0x20000;
  param_1[0x2a06] = lVar9 + 0x40000;
  param_1[0x2a07] = lVar9 + 0x60000;
  param_1[0x2a08] = lVar9 + 0x80000;
  param_1[0x2a09] = lVar9 + 0x90000;
  param_1[0x2a0a] = lVar9 + 0x94000;
  param_1[0x2a0b] = lVar9 + 0x98000;
  param_1[0x2a0c] = lVar9 + 0xa0000;
  param_1[0x2a0d] = lVar9 + 0xa4000;
  // #region agent log
  debug_log("initialize_memory:before_memory_map", "Before memory map initialization", "D", (void*)param_1, param_2, 0);
  // #endregion
  fprintf(stderr, "[DRASTIC] initialize_memory: About to call initialize_memory_map_arm9(param_1=0x%lx)\n", (unsigned long)param_1);
  fflush(stderr);
  initialize_memory_map_arm9(param_1);
  fprintf(stderr, "[DRASTIC] initialize_memory: Successfully returned from initialize_memory_map_arm9\n");
  fflush(stderr);
  fprintf(stderr, "[DRASTIC] initialize_memory: About to call initialize_memory_map_arm7(param_1=0x%lx)\n", (unsigned long)param_1);
  fflush(stderr);
  initialize_memory_map_arm7(param_1);
  fprintf(stderr, "[DRASTIC] initialize_memory: Successfully returned from initialize_memory_map_arm7\n");
  fflush(stderr);
  puts("  Initializing CP15.");
  // #region agent log
  debug_log("initialize_memory:before_coprocessor", "Before coprocessor initialization", "D", (void*)param_1, param_2 + 0x1faa3, 0);
  // #endregion
  initialize_coprocessor(param_1 + 0x1faa3,lVar1);
  // #region agent log
  debug_log("initialize_memory:before_pointer_assign", "Before pointer assignment", "D", (void*)nds_system, param_2 + 0x10cdfa0, 0);
  // #endregion
  // param_2 是 nds_system 的地址，不需要再加 nds_system
  *(long **)(param_2 + 0x10cdfa0) = param_1 + 0x1faa3;
  *(undefined8 *)(param_2 + 0x20d4590) = 0;
  *(long **)(param_2 + 0x10cddd0) = param_1 + 0x360e;
  *(long **)(param_2 + 0x20d43c0) = param_1 + 0x460e;
  puts("  Initializing DMA.");
  initialize_dma(param_1 + 0x1fa53,param_1,param_1 + 0x1f753,param_1 + 0x360e,lVar1);
  initialize_dma(param_1 + 0x1fa69,param_1,param_1 + 0x1f8d3,param_1 + 0x460e,lVar2);
  initialize_ipc(param_1 + 0x1fa7f,lVar1,param_1 + 0x1fa8b);
  initialize_ipc(param_1 + 0x1fa8b,lVar2,param_1 + 0x1fa7f);
  *(undefined1 *)((long)param_1 + 0xfd512) = 0;
  iVar3 = load_system_file(param_2,"nds_bios_arm9.bin",param_1 + 0x2004,0x1000);
  if (iVar3 < 0) {
    puts("Can\'t find Nintendo ARM9 BIOS. Trying free DraStic ARM9 BIOS.");
    iVar3 = load_system_file(param_2,"drastic_bios_arm9.bin",param_1 + 0x2004,0x1000);
    if (-1 < iVar3) {
      *(byte *)((long)param_1 + 0xfd512) = *(byte *)((long)param_1 + 0xfd512) | 2;
      iVar3 = load_system_file(param_2,"nds_bios_arm7.bin",param_1 + 0x2204,0x4000);
      goto joined_r0x0011c504;
    }
LAB_0011c74c:
    uVar8 = 0xffffffff;
  }
  else {
    iVar3 = load_system_file(param_2,"nds_bios_arm7.bin",param_1 + 0x2204,0x4000);
joined_r0x0011c504:
    if (iVar3 < 0) {
      puts("Can\'t find Nintendo ARM9 BIOS. Trying free DraStic ARM7 BIOS.");
      iVar3 = load_system_file(param_2,"drastic_bios_arm7.bin",param_1 + 0x2204,0x4000);
      if (iVar3 < 0) goto LAB_0011c74c;
      *(byte *)((long)param_1 + 0xfd512) = *(byte *)((long)param_1 + 0xfd512) | 1;
      iVar3 = load_system_file(param_2,"nds_firmware.bin",param_1 + 0x560e,0x40000);
    }
    else {
      iVar3 = load_system_file(param_2,"nds_firmware.bin",param_1 + 0x560e,0x40000);
    }
    if (iVar3 < 0) {
      memset(param_1 + 0x560e,0,0x40000);
      patch_firmware_header_data(param_1 + 0x560e);
    }
    param_1[0x1f747] = 0;
    param_1[0x15e2e] = 0;
    uVar8 = 0;
    param_1[0x1de36] = 0;
  }
  if (local_8 - __stack_chk_guard != 0) {
                    
    __stack_chk_fail();
  }
  return;
}

// --- 函数: initialize_cpu ---

void initialize_cpu(long *param_1,long param_2,int param_3,long param_4)

{
  int iVar1;
  long lVar2;
  long *plVar3;
  int iVar4;
  int iVar5;
  
  if (DAT_040270ff == '\0') {
    lVar2 = 1;
    bit_count = 0;
    do {
      (&bit_count)[lVar2] =
           POPCOUNT((char)lVar2) + POPCOUNT((char)((ulong)lVar2 >> 8)) +
           POPCOUNT((char)((ulong)lVar2 >> 0x10)) + POPCOUNT((char)((ulong)lVar2 >> 0x18));
      lVar2 = lVar2 + 1;
    } while (lVar2 != 0x100);
  }
  param_1[0x411] = param_2 + 0x8c000;
  *(int *)((long)param_1 + 0x210c) = param_3;
  param_1[1099] = param_2;
  param_1[0x44c] = param_2 + 0x35d4930;
  param_1[0x44d] = param_2 + 0x3a28bb0;
  param_1[0x454] = param_4;
  initialize_debug(param_1 + 0x423,param_1);
  iVar4 = 0;
  iVar5 = 8;
  plVar3 = param_1;
  do {
    *plVar3 = (long)param_1;
    *(int *)(plVar3 + 2) = iVar5;
    *(char *)((long)plVar3 + 0x1d) = (char)iVar4;
    iVar1 = param_3 * 4 + 3 + iVar4;
    iVar4 = iVar4 + 1;
    initialize_event(param_2 + 0x18,iVar1,event_timer_overflow_function,plVar3);
    iVar5 = iVar5 * 2;
    plVar3 = plVar3 + 4;
  } while (iVar4 != 4);
  if (param_3 == 1) {
    param_1[0x44e] = param_2 + 0x154c038;
    param_1[0x44f] = param_2 + 0x1554038;
  }
  return;
}

// --- 函数: malloc ---

void * _malloc(size_t __size)

{
  void *pvVar1;
  
  pvVar1 = malloc(__size);
  return pvVar1;
}

// --- 函数: CONCAT11 ---
/*
                                                  bVar26,CONCAT11(bVar26,bVar26))))))) &
       0x3f3f3f3f3f3f3f3f;
  auVar14[8] = bVar29;
  auVar14[9] = bVar29;
  auVar14[10] = bVar29;
  auVar14[0xb] = bVar29;
  auVar14[0xc] = bVar29;
  auVar14[0xd] = bVar29;
  auVar14[0xe] = bVar29;
  auVar14[0xf] = bVar29;
  auVar27._0_8_ =
       CONCAT17(bVar28,CONCAT16(bVar28,CONCAT15(bVar28,CONCAT14(bVar28,CONCAT13(bVar28,CONCAT12(
                                                  bVar28,CONCAT11(bVar28,bVar28))))))) &
       0x3f3f3f3f3f3f3f3f;
  auVar27[8] = bVar30;
  auVar27[9] = bVar30;
  auVar27[10] = bVar30;
  auVar27[0xb] = bVar30;
  auVar27[0xc] = bVar30;
  auVar27[0xd] = bVar30;
  auVar27[0xe] = bVar30;
  auVar27[0xf] = bVar30;
  auVar25[8] = 0x20;
  auVar25._0_8_ = 0x2020202020202020;
  auVar25[9] = 0x20;
  auVar25[10] = 0x20;
  auVar25[0xb] = 0x20;
  auVar25[0xc] = 0x20;
  auVar25[0xd] = 0x20;
  auVar25[0xe] = 0x20;
  auVar25[0xf] = 0x20;
  auVar27 = NEON_umin(auVar27,auVar25,1);
  iVar9 = 0x100;
  auVar11[8] = 0x20;
  auVar11._0_8_ = 0x2020202020202020;
  auVar11[9] = 0x20;
  auVar11[10] = 0x20;
  auVar11[0xb] = 0x20;
  auVar11[0xc] = 0x20;
  auVar11[0xd] = 0x20;
  auVar11[0xe] = 0x20;
  auVar11[0xf] = 0x20;
  auVar25 = NEON_umin(auVar14,auVar11,1);
  do {
    uVar1 = *param_4;
    auVar10._2_2_ = uVar1;
    auVar10._0_2_ = uVar1;
    auVar10._4_2_ = uVar1;
    auVar10._6_2_ = uVar1;
    auVar10._8_2_ = uVar1;
    auVar10._10_2_ = uVar1;
    auVar10._12_2_ = uVar1;
    auVar10._14_2_ = uVar1;
    uVar1 = param_4[1];
    auVar13._2_2_ = uVar1;
    auVar13._0_2_ = uVar1;
    auVar13._4_2_ = uVar1;
    auVar13._6_2_ = uVar1;
    auVar13._8_2_ = uVar1;
    auVar13._10_2_ = uVar1;
    auVar13._12_2_ = uVar1;
    auVar13._14_2_ = uVar1;
    uVar1 = param_4[2];
    auVar17._2_2_ = uVar1;
    auVar17._0_2_ = uVar1;
    auVar17._4_2_ = uVar1;
    auVar17._6_2_ = uVar1;
    auVar17._8_2_ = uVar1;
    auVar17._10_2_ = uVar1;
    auVar17._12_2_ = uVar1;
    auVar17._14_2_ = uVar1;
    uVar1 = param_4[3];
    auVar21._2_2_ = uVar1;
    auVar21._0_2_ = uVar1;
    auVar21._4_2_ = uVar1;
    auVar21._6_2_ = uVar1;
    auVar21._8_2_ = uVar1;
    auVar21._10_2_ = uVar1;
    auVar21._12_2_ = uVar1;
    auVar21._14_2_ = uVar1;
    param_4 = param_4 + 4;
    auVar5[9] = 1;
    auVar5._0_9_ = (unkuint9)1 << 0x40;
    auVar5[10] = 1;
    auVar5[0xb] = 1;
    auVar5[0xc] = 1;
    auVar5[0xd] = 1;
    auVar5[0xe] = 1;
    auVar5[0xf] = 1;
    auVar11 = a64_TBL(ZEXT816(0),auVar10,auVar5);
    auVar6[9] = 1;
    auVar6._0_9_ = (unkuint9)1 << 0x40;
    auVar6[10] = 1;
    auVar6[0xb] = 1;
    auVar6[0xc] = 1;
    auVar6[0xd] = 1;
    auVar6[0xe] = 1;
    auVar6[0xf] = 1;
    auVar14 = a64_TBL(ZEXT816(0),auVar13,auVar6);
    auVar7[9] = 1;
    auVar7._0_9_ = (unkuint9)1 << 0x40;
    auVar7[10] = 1;
    auVar7[0xb] = 1;
    auVar7[0xc] = 1;
    auVar7[0xd] = 1;
    auVar7[0xe] = 1;
    auVar7[0xf] = 1;
    auVar18 = a64_TBL(ZEXT816(0),auVar17,auVar7);
    auVar8[9] = 1;
    auVar8._0_9_ = (unkuint9)1 << 0x40;
    auVar8[10] = 1;
    auVar8[0xb] = 1;
    auVar8[0xc] = 1;
    auVar8[0xd] = 1;
    auVar8[0xe] = 1;
    auVar8[0xf] = 1;
    auVar22 = a64_TBL(ZEXT816(0),auVar21,auVar8);
    auVar12[0] = -((auVar11[0] & 1) != 0);
    auVar12[1] = -((auVar11[1] & 2) != 0);
    auVar12[2] = -((auVar11[2] & 4) != 0);
    auVar12[3] = -((auVar11[3] & 8) != 0);
    auVar12[4] = -((auVar11[4] & 0x10) != 0);
    auVar12[5] = -((auVar11[5] & 0x20) != 0);
    auVar12[6] = -((auVar11[6] & 0x40) != 0);
    auVar12[7] = -((auVar11[7] & 0x80) != 0);
    auVar12[8] = -((auVar11[8] & 1) != 0);
    auVar12[9] = -((auVar11[9] & 2) != 0);
    auVar12[10] = -((auVar11[10] & 4) != 0);
    auVar12[0xb] = -((auVar11[0xb] & 8) != 0);
    auVar12[0xc] = -((auVar11[0xc] & 0x10) != 0);
    auVar12[0xd] = -((auVar11[0xd] & 0x20) != 0);
    auVar12[0xe] = -((auVar11[0xe] & 0x40) != 0);
    auVar12[0xf] = -((auVar11[0xf] & 0x80) != 0);
    auVar15[0] = -((auVar14[0] & 1) != 0);
    auVar15[1] = -((auVar14[1] & 2) != 0);
    auVar15[2] = -((auVar14[2] & 4) != 0);
    auVar15[3] = -((auVar14[3] & 8) != 0);
    auVar15[4] = -((auVar14[4] & 0x10) != 0);
    auVar15[5] = -((auVar14[5] & 0x20) != 0);
    auVar15[6] = -((auVar14[6] & 0x40) != 0);
    auVar15[7] = -((auVar14[7] & 0x80) != 0);
    auVar15[8] = -((auVar14[8] & 1) != 0);
    auVar15[9] = -((auVar14[9] & 2) != 0);
    auVar15[10] = -((auVar14[10] & 4) != 0);
    auVar15[0xb] = -((auVar14[0xb] & 8) != 0);
    auVar15[0xc] = -((auVar14[0xc] & 0x10) != 0);
    auVar15[0xd] = -((auVar14[0xd] & 0x20) != 0);
    auVar15[0xe] = -((auVar14[0xe] & 0x40) != 0);
    auVar15[0xf] = -((auVar14[0xf] & 0x80) != 0);
    auVar19[0] = -((auVar18[0] & 1) != 0);
    auVar19[1] = -((auVar18[1] & 2) != 0);
    auVar19[2] = -((auVar18[2] & 4) != 0);
    auVar19[3] = -((auVar18[3] & 8) != 0);
    auVar19[4] = -((auVar18[4] & 0x10) != 0);
    auVar19[5] = -((auVar18[5] & 0x20) != 0);
    auVar19[6] = -((auVar18[6] & 0x40) != 0);
    auVar19[7] = -((auVar18[7] & 0x80) != 0);
    auVar19[8] = -((auVar18[8] & 1) != 0);
    auVar19[9] = -((auVar18[9] & 2) != 0);
    auVar19[10] = -((auVar18[10] & 4) != 0);
    auVar19[0xb] = -((auVar18[0xb] & 8) != 0);
    auVar19[0xc] = -((auVar18[0xc] & 0x10) != 0);
    auVar19[0xd] = -((auVar18[0xd] & 0x20) != 0);
    auVar19[0xe] = -((auVar18[0xe] & 0x40) != 0);
    auVar19[0xf] = -((auVar18[0xf] & 0x80) != 0);
    auVar23[0] = -((auVar22[0] & 1) != 0);
    auVar23[1] = -((auVar22[1] & 2) != 0);
    auVar23[2] = -((auVar22[2] & 4) != 0);
    auVar23[3] = -((auVar22[3] & 8) != 0);
    auVar23[4] = -((auVar22[4] & 0x10) != 0);
    auVar23[5] = -((auVar22[5] & 0x20) != 0);
    auVar23[6] = -((auVar22[6] & 0x40) != 0);
    auVar23[7] = -((auVar22[7] & 0x80) != 0);
    auVar23[8] = -((auVar22[8] & 1) != 0);
    auVar23[9] = -((auVar22[9] & 2) != 0);
    auVar23[10] = -((auVar22[10] & 4) != 0);
    auVar23[0xb] = -((auVar22[0xb] & 8) != 0);
    auVar23[0xc] = -((auVar22[0xc] & 0x10) != 0);
    auVar23[0xd] = -((auVar22[0xd] & 0x20) != 0);
    auVar23[0xe] = -((auVar22[0xe] & 0x40) != 0);
    auVar23[0xf] = -((auVar22[0xf] & 0x80) != 0);
    bVar26 = auVar27[0];
    bVar28 = auVar27[1];
    bVar29 = auVar27[2];
    bVar30 = auVar27[3];
    bVar31 = auVar27[4];
    bVar32 = auVar27[5];
    bVar33 = auVar27[6];
    bVar34 = auVar27[7];
    bVar35 = auVar27[8];
    bVar36 = auVar27[9];
    bVar37 = auVar27[10];
    bVar38 = auVar27[0xb];
    bVar39 = auVar27[0xc];
    bVar40 = auVar27[0xd];
    bVar41 = auVar27[0xe];
    bVar42 = auVar27[0xf];
    auVar18[8] = 0x20;
    auVar18._0_8_ = 0x2020202020202020;
    auVar18[9] = 0x20;
    auVar18[10] = 0x20;
    auVar18[0xb] = 0x20;
    auVar18[0xc] = 0x20;
    auVar18[0xd] = 0x20;
    auVar18[0xe] = 0x20;
    auVar18[0xf] = 0x20;
    auVar22[8] = 0x20;
    auVar22._0_8_ = 0x2020202020202020;
    auVar22[9] = 0x20;
    auVar22[10] = 0x20;
    auVar22[0xb] = 0x20;
    auVar22[0xc] = 0x20;
    auVar22[0xd] = 0x20;
    auVar22[0xe] = 0x20;
    auVar22[0xf] = 0x20;
    auVar22 = auVar22 ^ (auVar18 ^ auVar25) & auVar12;
    auVar2[8] = 0x20;
    auVar2._0_8_ = 0x2020202020202020;
    auVar2[9] = 0x20;
    auVar2[10] = 0x20;
    auVar2[0xb] = 0x20;
    auVar2[0xc] = 0x20;
    auVar2[0xd] = 0x20;
    auVar2[0xe] = 0x20;
    auVar2[0xf] = 0x20;
    auVar16[8] = 0x20;
    auVar16._0_8_ = 0x2020202020202020;
    auVar16[9] = 0x20;
    auVar16[10] = 0x20;
    auVar16[0xb] = 0x20;
    auVar16[0xc] = 0x20;
    auVar16[0xd] = 0x20;
    auVar16[0xe] = 0x20;
    auVar16[0xf] = 0x20;
    auVar16 = auVar16 ^ (auVar2 ^ auVar25) & auVar15;
    auVar3[8] = 0x20;
    auVar3._0_8_ = 0x2020202020202020;
    auVar3[9] = 0x20;
    auVar3[10] = 0x20;
    auVar3[0xb] = 0x20;
    auVar3[0xc] = 0x20;
    auVar3[0xd] = 0x20;
    auVar3[0xe] = 0x20;
    auVar3[0xf] = 0x20;
    auVar20[8] = 0x20;
    auVar20._0_8_ = 0x2020202020202020;
    auVar20[9] = 0x20;
    auVar20[10] = 0x20;
    auVar20[0xb] = 0x20;
    auVar20[0xc] = 0x20;
    auVar20[0xd] = 0x20;
    auVar20[0xe] = 0x20;
    auVar20[0xf] = 0x20;
    auVar20 = auVar20 ^ (auVar3 ^ auVar25) & auVar19;
    auVar4[8] = 0x20;
    auVar4._0_8_ = 0x2020202020202020;
    auVar4[9] = 0x20;
    auVar4[10] = 0x20;
    auVar4[0xb] = 0x20;
    auVar4[0xc] = 0x20;
    auVar4[0xd] = 0x20;
    auVar4[0xe] = 0x20;
    auVar4[0xf] = 0x20;
    auVar24[8] = 0x20;
    auVar24._0_8_ = 0x2020202020202020;
    auVar24[9] = 0x20;
    auVar24[10] = 0x20;
    auVar24[0xb] = 0x20;
    auVar24[0xc] = 0x20;
    auVar24[0xd] = 0x20;
    auVar24[0xe] = 0x20;
    auVar24[0xf] = 0x20;
    auVar24 = auVar24 ^ (auVar4 ^ auVar25) & auVar23;
    *param_3 = auVar12[0] & bVar26;
    param_3[1] = auVar12[1] & bVar28;
    param_3[2] = auVar12[2] & bVar29;
    param_3[3] = auVar12[3] & bVar30;
    param_3[4] = auVar12[4] & bVar31;
    param_3[5] = auVar12[5] & bVar32;
    param_3[6] = auVar12[6] & bVar33;
    param_3[7] = auVar12[7] & bVar34;
    param_3[8] = auVar12[8] & bVar35;
    param_3[9] = auVar12[9] & bVar36;
    param_3[10] = auVar12[10] & bVar37;
    param_3[0xb] = auVar12[0xb] & bVar38;
    param_3[0xc] = auVar12[0xc] & bVar39;
    param_3[0xd] = auVar12[0xd] & bVar40;
    param_3[0xe] = auVar12[0xe] & bVar41;
    param_3[0xf] = auVar12[0xf] & bVar42;
    param_3[0x10] = auVar15[0] & bVar26;
    param_3[0x11] = auVar15[1] & bVar28;
    param_3[0x12] = auVar15[2] & bVar29;
    param_3[0x13] = auVar15[3] & bVar30;
    param_3[0x14] = auVar15[4] & bVar31;
    param_3[0x15] = auVar15[5] & bVar32;
    param_3[0x16] = auVar15[6] & bVar33;
    param_3[0x17] = auVar15[7] & bVar34;
    param_3[0x18] = auVar15[8] & bVar35;
    param_3[0x19] = auVar15[9] & bVar36;
    param_3[0x1a] = auVar15[10] & bVar37;
    param_3[0x1b] = auVar15[0xb] & bVar38;
    param_3[0x1c] = auVar15[0xc] & bVar39;
    param_3[0x1d] = auVar15[0xd] & bVar40;
    param_3[0x1e] = auVar15[0xe] & bVar41;
    param_3[0x1f] = auVar15[0xf] & bVar42;
    param_3[0x20] = auVar19[0] & bVar26;
    param_3[0x21] = auVar19[1] & bVar28;
    param_3[0x22] = auVar19[2] & bVar29;
    param_3[0x23] = auVar19[3] & bVar30;
    param_3[0x24] = auVar19[4] & bVar31;
    param_3[0x25] = auVar19[5] & bVar32;
    param_3[0x26] = auVar19[6] & bVar33;
    param_3[0x27] = auVar19[7] & bVar34;
    param_3[0x28] = auVar19[8] & bVar35;
    param_3[0x29] = auVar19[9] & bVar36;
    param_3[0x2a] = auVar19[10] & bVar37;
    param_3[0x2b] = auVar19[0xb] & bVar38;
    param_3[0x2c] = auVar19[0xc] & bVar39;
    param_3[0x2d] = auVar19[0xd] & bVar40;
    param_3[0x2e] = auVar19[0xe] & bVar41;
    param_3[0x2f] = auVar19[0xf] & bVar42;
    param_3[0x30] = auVar23[0] & bVar26;
    param_3[0x31] = auVar23[1] & bVar28;
    param_3[0x32] = auVar23[2] & bVar29;
    param_3[0x33] = auVar23[3] & bVar30;
    param_3[0x34] = auVar23[4] & bVar31;
    param_3[0x35] = auVar23[5] & bVar32;
    param_3[0x36] = auVar23[6] & bVar33;
    param_3[0x37] = auVar23[7] & bVar34;
    param_3[0x38] = auVar23[8] & bVar35;
    param_3[0x39] = auVar23[9] & bVar36;
    param_3[0x3a] = auVar23[10] & bVar37;
    param_3[0x3b] = auVar23[0xb] & bVar38;
    param_3[0x3c] = auVar23[0xc] & bVar39;
    param_3[0x3d] = auVar23[0xd] & bVar40;
    param_3[0x3e] = auVar23[0xe] & bVar41;
    param_3[0x3f] = auVar23[0xf] & bVar42;
    param_3 = param_3 + 0x40;
    *param_2 = auVar22[0];
    param_2[1] = auVar22[1];
    param_2[2] = auVar22[2];
    param_2[3] = auVar22[3];
    param_2[4] = auVar22[4];
    param_2[5] = auVar22[5];
    param_2[6] = auVar22[6];
    param_2[7] = auVar22[7];
    param_2[8] = auVar22[8];
    param_2[9] = auVar22[9];
    param_2[10] = auVar22[10];
    param_2[0xb] = auVar22[0xb];
    param_2[0xc] = auVar22[0xc];
    param_2[0xd] = auVar22[0xd];
    param_2[0xe] = auVar22[0xe];
    param_2[0xf] = auVar22[0xf];
    param_2[0x10] = auVar16[0];
    param_2[0x11] = auVar16[1];
    param_2[0x12] = auVar16[2];
    param_2[0x13] = auVar16[3];
    param_2[0x14] = auVar16[4];
    param_2[0x15] = auVar16[5];
    param_2[0x16] = auVar16[6];
    param_2[0x17] = auVar16[7];
    param_2[0x18] = auVar16[8];
    param_2[0x19] = auVar16[9];
    param_2[0x1a] = auVar16[10];
    param_2[0x1b] = auVar16[0xb];
    param_2[0x1c] = auVar16[0xc];
    param_2[0x1d] = auVar16[0xd];
    param_2[0x1e] = auVar16[0xe];
    param_2[0x1f] = auVar16[0xf];
    param_2[0x20] = auVar20[0];
    param_2[0x21] = auVar20[1];
    param_2[0x22] = auVar20[2];
    param_2[0x23] = auVar20[3];
    param_2[0x24] = auVar20[4];
    param_2[0x25] = auVar20[5];
    param_2[0x26] = auVar20[6];
    param_2[0x27] = auVar20[7];
    param_2[0x28] = auVar20[8];
    param_2[0x29] = auVar20[9];
    param_2[0x2a] = auVar20[10];
    param_2[0x2b] = auVar20[0xb];
    param_2[0x2c] = auVar20[0xc];
    param_2[0x2d] = auVar20[0xd];
    param_2[0x2e] = auVar20[0xe];
    param_2[0x2f] = auVar20[0xf];
    param_2[0x30] = auVar24[0];
    param_2[0x31] = auVar24[1];
    param_2[0x32] = auVar24[2];
    param_2[0x33] = auVar24[3];
    param_2[0x34] = auVar24[4];
    param_2[0x35] = auVar24[5];
    param_2[0x36] = auVar24[6];
    param_2[0x37] = auVar24[7];
    param_2[0x38] = auVar24[8];
    param_2[0x39] = auVar24[9];
    param_2[0x3a] = auVar24[10];
    param_2[0x3b] = auVar24[0xb];
    param_2[0x3c] = auVar24[0xc];
    param_2[0x3d] = auVar24[0xd];
    param_2[0x3e] = auVar24[0xe];
    param_2[0x3f] = auVar24[0xf];
    param_2 = param_2 + 0x40;
    iVar9 = iVar9 + -0x40;
  } while (iVar9 != 0);
*/
// --- 函数: select_exit_current_menu ---
/*
void select_exit_current_menu(long *param_1,long param_2)

{
  long lVar1;
  
  lVar1 = param_1[2];
  if ((param_2 != 0) && (*(code1 **)(param_2 + 0x20) != (code *)0x0)) {
    (**(code1 **)(param_2 + 0x20))(param_1,param_2,1);
  }
  if (*(code1 **)(lVar1 + 8) != (code *)0x0) {
    (**(code1 **)(lVar1 + 8))(param_1,lVar1,1);
  }
  lVar1 = *(long *)(lVar1 + 0x28);
  if (lVar1 == 0) {
    if (*(char *)(*param_1 + 0x8b380) != '\0') {
      *(undefined4 *)(param_1 + 9) = 1;
      return;
    }
  }
  else {
    if (*(code1 **)(lVar1 + 8) != (code *)0x0) {
      (**(code1 **)(lVar1 + 8))(param_1,lVar1,0);
    }
    param_1[2] = lVar1;
  }
  return;
}
*/
void select_exit_current_menu(long *param_1, long param_2)
{
    long lVar1;
    
    lVar1 = param_1[2];
    
    // First callback - exit callback for current menu
    if ((param_2 != 0) && (*(code **)(param_2 + 0x20) != (code *)0x0)) {
        // Changed: Only 2 arguments instead of 3
        (**(code2 **)(param_2 + 0x20))(param_1, param_2);
    }
    
    // Second callback - exit callback for menu at param_1[2]
    if (*(code **)(lVar1 + 8) != (code *)0x0) {
        // Changed: Only 2 arguments instead of 3
        (**(code2 **)(lVar1 + 8))(param_1, lVar1);
    }
    
    lVar1 = *(long *)(lVar1 + 0x28);
    
    if (lVar1 == 0) {
        // No parent menu, set exit flag if needed
        if (*(char *)(*param_1 + 0x8b380) != '\0') {
            *(int *)(param_1 + 9) = 1;
            return;
        }
    }
    else {
        // Enter callback for parent menu
        if (*(code **)(lVar1 + 8) != (code *)0x0) {
            // Changed: Only 2 arguments instead of 3
            (**(code2 **)(lVar1 + 8))(param_1, lVar1);
        }
        param_1[2] = lVar1; // Set current menu to parent
    }
    
    return;
}
// --- 函数: get_gui_input ---

uint get_gui_input(long param_1,uint *param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  ulong uVar8;
  ulong uVar9;
  long lVar10;
  long local_48;
  // SDL_Event 通常需要 56 字节或更多，使用足够大的缓冲区
  char local_40 [128];
  uint local_38;
  byte local_34;
  byte local_33;
  short local_30;
  uint local_2c;
  ushort local_28;
  long local_8;
  
  uVar7 = 0xb;
  lVar10 = *(long *)(param_1 + 0x80008);
  local_8 = (long)__stack_chk_guard;
  delay_us(10000);
  lVar10 = lVar10 + 0x86318;
LAB_0018ba28:
  do {
    iVar4 = SDL_PollEvent((SDL_Event *)local_40);
    while( true ) {
      if (iVar4 == 0) {
        if (uVar7 == 0xb) {
          if (*(char *)((long)&gui_actions + (ulong)cursor_repeat_37536) != '\0') {
            get_ticks_us(&local_48);
            if (button_repeat_state_37535 == 1) {
              if (250000 < (ulong)(local_48 - button_repeat_timestamp_37534)) {
                button_repeat_timestamp_37534 = local_48;
                button_repeat_state_37535 = 2;
                uVar7 = cursor_repeat_37536;
              }
            }
            else if ((button_repeat_state_37535 == 2) &&
                    (50000 < (ulong)(local_48 - button_repeat_timestamp_37534))) {
              uVar7 = cursor_repeat_37536;
              button_repeat_timestamp_37534 = local_48;
            }
          }
        }
        else {
          get_ticks_us((long)&button_repeat_timestamp_37534);
          button_repeat_state_37535 = 1;
          cursor_repeat_37536 = uVar7;
        }
        lVar10 = local_8 - __stack_chk_guard;
        *param_2 = uVar7;
        if (lVar10 != 0) {
                    
          __stack_chk_fail();
        }
        return uVar7;
      }
      if (local_40[0] == 0x602) break;
      if (0x602 < local_40[0]) {
        if (local_40[0] == 0x603) {
          uVar9 = *(ulong *)(lVar10 + (long)(int)((local_38 & 3) << 8 | (uint)local_34 | 0x400) * 8)
          ;
          uVar7 = 0;
          if ((uVar9 & 0x80000000) == 0) {
            uVar7 = 0xb;
          }
          if ((uVar9 & 0x100000000) != 0) {
            uVar7 = 1;
          }
          if ((uVar9 & 0x200000000) != 0) {
            uVar7 = 2;
          }
          if ((uVar9 & 0x400000000) != 0) {
            uVar7 = 3;
          }
          if ((uVar9 & 0x800000000) != 0) {
            uVar7 = 4;
          }
          if ((uVar9 & 0x1000000000) != 0) {
            uVar7 = 5;
          }
          if ((uVar9 & 0x2000000000) != 0) {
            uVar7 = 6;
          }
          if ((uVar9 & 0x4000000000) != 0) {
            uVar7 = 7;
          }
          if ((uVar9 & 0x8000000000) != 0) {
            uVar7 = 8;
          }
          if ((uVar9 & 0x10000000000) != 0) {
            uVar7 = 9;
          }
          *(undefined1 *)((long)&gui_actions + (ulong)uVar7) = 1;
        }
        else if (local_40[0] == 0x604) {
          uVar9 = *(ulong *)(lVar10 + (long)(int)((local_38 & 3) << 8 | (uint)local_34 | 0x400) * 8)
          ;
          uVar6 = 0;
          if ((uVar9 & 0x80000000) == 0) {
            uVar6 = 0xb;
          }
          if ((uVar9 & 0x100000000) != 0) {
            uVar6 = 1;
          }
          if ((uVar9 & 0x200000000) != 0) {
            uVar6 = 2;
          }
          if ((uVar9 & 0x400000000) != 0) {
            uVar6 = 3;
          }
          if ((uVar9 & 0x800000000) != 0) {
            uVar6 = 4;
          }
          if ((uVar9 & 0x1000000000) != 0) {
            uVar6 = 5;
          }
          if ((uVar9 & 0x2000000000) != 0) {
            uVar6 = 6;
          }
          if ((uVar9 & 0x4000000000) != 0) {
            uVar6 = 7;
          }
          if ((uVar9 & 0x8000000000) != 0) {
            uVar6 = 8;
          }
LAB_0018bd50:
          if ((uVar9 & 0x10000000000) != 0) {
            uVar6 = 9;
          }
          *(undefined1 *)((long)&gui_actions + (ulong)uVar6) = 0;
        }
        goto LAB_0018ba28;
      }
      if (local_40[0] == 0x301) {
        uVar9 = *(ulong *)(lVar10 + (long)(int)(((int)local_2c >> 0x1e & 3U) << 8 | local_2c & 0xff)
                                    * 8);
        uVar6 = 0;
        if ((uVar9 & 0x80000000) == 0) {
          uVar6 = 0xb;
        }
        if ((uVar9 & 0x100000000) != 0) {
          uVar6 = 1;
        }
        if ((uVar9 & 0x200000000) != 0) {
          uVar6 = 2;
        }
        if ((uVar9 & 0x400000000) != 0) {
          uVar6 = 3;
        }
        if ((uVar9 & 0x800000000) != 0) {
          uVar6 = 4;
        }
        if ((uVar9 & 0x1000000000) != 0) {
          uVar6 = 5;
        }
        if ((uVar9 & 0x2000000000) != 0) {
          uVar6 = 6;
        }
        if ((uVar9 & 0x4000000000) != 0) {
          uVar6 = 7;
        }
        if ((uVar9 & 0x8000000000) != 0) {
          uVar6 = 8;
        }
        goto LAB_0018bd50;
      }
      if (local_40[0] != 0x600) {
        if (local_40[0] != 0x300) goto LAB_0018ba28;
        uVar9 = *(ulong *)(lVar10 + (long)(int)(((int)local_2c >> 0x1e & 3U) << 8 | local_2c & 0xff)
                                    * 8);
        uVar7 = 0;
        if ((uVar9 & 0x80000000) == 0) {
          uVar7 = 0xb;
        }
        if ((uVar9 & 0x100000000) != 0) {
          uVar7 = 1;
        }
        if ((uVar9 & 0x200000000) != 0) {
          uVar7 = 2;
        }
        if ((uVar9 & 0x400000000) != 0) {
          uVar7 = 3;
        }
        if ((uVar9 & 0x800000000) != 0) {
          uVar7 = 4;
        }
        if ((uVar9 & 0x1000000000) != 0) {
          uVar7 = 5;
        }
        if ((uVar9 & 0x2000000000) != 0) {
          uVar7 = 6;
        }
        uVar8 = uVar9 & 0x10000000000;
        if ((uVar9 >> 0x26 & 1) == 0) {
          if ((uVar9 >> 0x27 & 1) != 0) goto LAB_0018bc9c;
          if (uVar8 == 0) {
            if (uVar7 == 0xb) {
              uVar6 = local_2c;
              if ((local_28 & 3) != 0) {
                switch(local_2c) {
                case 0x27:
                  uVar6 = 0x22;
                  break;
                case 0x2c:
                  uVar6 = 0x3c;
                  break;
                case 0x2d:
                  uVar6 = 0x5f;
                  break;
                case 0x2e:
                  uVar6 = 0x3e;
                  break;
                case 0x2f:
                  uVar6 = 0x3f;
                  break;
                case 0x30:
                  uVar6 = 0x29;
                  break;
                case 0x31:
                  uVar6 = 0x21;
                  break;
                case 0x32:
                  uVar6 = 0x40;
                  break;
                case 0x33:
                  uVar6 = 0x23;
                  break;
                case 0x34:
                  uVar6 = 0x24;
                  break;
                case 0x35:
                  uVar6 = 0x25;
                  break;
                case 0x36:
                  uVar6 = 0x5e;
                  break;
                case 0x37:
                  uVar6 = 0x26;
                  break;
                case 0x38:
                  uVar6 = 0x2a;
                  break;
                case 0x39:
                  uVar6 = 0x28;
                  break;
                case 0x3b:
                  uVar6 = 0x3a;
                  break;
                case 0x3d:
                  uVar6 = 0x2b;
                  break;
                case 0x5b:
                  uVar6 = 0x7b;
                  break;
                case 0x5d:
                  uVar6 = 0x7d;
                  break;
                case 0x60:
                  uVar6 = 0x7e;
                  break;
                case 0x61:
                case 0x62:
                case 99:
                case 100:
                case 0x65:
                case 0x66:
                case 0x67:
                case 0x68:
                case 0x69:
                case 0x6a:
                case 0x6b:
                case 0x6c:
                case 0x6d:
                case 0x6e:
                case 0x6f:
                case 0x70:
                case 0x71:
                case 0x72:
                case 0x73:
                case 0x74:
                case 0x75:
                case 0x76:
                case 0x77:
                case 0x78:
                case 0x79:
                case 0x7a:
                  uVar6 = local_2c - 0x20;
                }
              }
              uVar7 = 10;
              param_2[1] = uVar6;
              goto LAB_0018ba28;
            }
          }
          else {
            uVar7 = 9;
          }
        }
        else if ((uVar9 >> 0x27 & 1) == 0) {
          uVar7 = 7;
          if (uVar8 != 0) {
            uVar7 = 9;
          }
        }
        else {
LAB_0018bc9c:
          uVar7 = (uVar8 != 0) + 8;
        }
        *(undefined1 *)((long)&gui_actions + (ulong)uVar7) = 1;
        goto LAB_0018ba28;
      }
      uVar3 = (uint)local_34 * 2;
      uVar6 = *(uint *)(SDL_input + (ulong)local_38 * 4) &
              (3 << (ulong)(uVar3 & 0x1f) ^ 0xffffffffU);
      if (local_30 < 0x2711) {
        uVar5 = uVar6 | 2 << (ulong)(uVar3 & 0x1f);
        if (-0x2711 < local_30) {
          uVar5 = uVar6;
        }
      }
      else {
        uVar5 = uVar6 | 1 << (ulong)(uVar3 & 0x1f);
      }
      uVar2 = (local_38 & 3) << 8 | (uint)local_34;
      uVar9 = *(ulong *)(lVar10 + (ulong)(uVar2 | 0x480) * 8);
      uVar6 = 0;
      if ((uVar9 & 0x80000000) == 0) {
        uVar6 = 0xb;
      }
      if ((uVar9 & 0x100000000) != 0) {
        uVar6 = 1;
      }
      if ((uVar9 & 0x200000000) != 0) {
        uVar6 = 2;
      }
      if ((uVar9 & 0x400000000) != 0) {
        uVar6 = 3;
      }
      if ((uVar9 & 0x800000000) != 0) {
        uVar6 = 4;
      }
      if ((uVar9 & 0x1000000000) != 0) {
        uVar6 = 5;
      }
      uVar8 = *(ulong *)(lVar10 + (ulong)(uVar2 | 0x4c0) * 8);
      if ((uVar9 & 0x2000000000) != 0) {
        uVar6 = 6;
      }
      if ((uVar9 & 0x4000000000) != 0) {
        uVar6 = 7;
      }
      if ((uVar9 & 0x8000000000) != 0) {
        uVar6 = 8;
      }
      if ((uVar9 & 0x10000000000) != 0) {
        uVar6 = 9;
      }
      uVar2 = 0;
      if ((uVar8 & 0x80000000) == 0) {
        uVar2 = 0xb;
      }
      if ((uVar8 & 0x100000000) != 0) {
        uVar2 = 1;
      }
      if ((uVar8 & 0x200000000) != 0) {
        uVar2 = 2;
      }
      if ((uVar8 & 0x400000000) != 0) {
        uVar2 = 3;
      }
      if ((uVar8 & 0x800000000) != 0) {
        uVar2 = 4;
      }
      if ((uVar8 & 0x1000000000) != 0) {
        uVar2 = 5;
      }
      if ((uVar8 & 0x2000000000) != 0) {
        uVar2 = 6;
      }
      if ((uVar8 & 0x4000000000) != 0) {
        uVar2 = 7;
      }
      if ((uVar8 & 0x8000000000) != 0) {
        uVar2 = 8;
      }
      if ((uVar8 & 0x10000000000) != 0) {
        uVar2 = 9;
      }
      if (*(uint *)(SDL_input + (ulong)local_38 * 4) == uVar5) goto LAB_0018ba28;
      if ((1 << (ulong)(uVar3 & 0x1f) & uVar5) != 0) {
        uVar7 = uVar6;
      }
      if ((2 << (ulong)(uVar3 & 0x1f) & uVar5) != 0) {
        uVar7 = uVar2;
      }
      *(undefined1 *)((long)&gui_actions + (ulong)uVar6) = 0;
      *(undefined1 *)((long)&gui_actions + (ulong)uVar2) = 0;
      *(undefined1 *)((long)&gui_actions + (ulong)uVar7) = 1;
      *(uint *)(SDL_input + (ulong)local_38 * 4) = uVar5;
      iVar4 = SDL_PollEvent((SDL_Event*)local_40);
    }
    uVar3 = (local_38 & 3) << 8;
    uVar9 = *(ulong *)(lVar10 + (long)(int)(uVar3 | 0x441) * 8);
    uVar5 = 0xb;
    uVar6 = 0;
    if ((uVar9 & 0x80000000) == 0) {
      uVar6 = uVar5;
    }
    if ((uVar9 & 0x100000000) != 0) {
      uVar6 = 1;
    }
    if ((uVar9 & 0x200000000) != 0) {
      uVar6 = 2;
    }
    if ((uVar9 & 0x400000000) != 0) {
      uVar6 = 3;
    }
    if ((uVar9 & 0x800000000) != 0) {
      uVar6 = 4;
    }
    if ((uVar9 & 0x1000000000) != 0) {
      uVar6 = 5;
    }
    uVar8 = *(ulong *)(lVar10 + (long)(int)(uVar3 | 0x444) * 8);
    if ((uVar9 & 0x2000000000) != 0) {
      uVar6 = 6;
    }
    if ((uVar9 & 0x4000000000) != 0) {
      uVar6 = 7;
    }
    if ((uVar9 & 0x8000000000) != 0) {
      uVar6 = 8;
    }
    if ((uVar9 & 0x10000000000) != 0) {
      uVar6 = 9;
    }
    uVar2 = 0;
    if ((uVar8 & 0x80000000) == 0) {
      uVar2 = uVar5;
    }
    if ((uVar8 & 0x100000000) != 0) {
      uVar2 = 1;
    }
    if ((uVar8 & 0x200000000) != 0) {
      uVar2 = 2;
    }
    if ((uVar8 & 0x400000000) != 0) {
      uVar2 = 3;
    }
    if ((uVar8 & 0x800000000) != 0) {
      uVar2 = 4;
    }
    if ((uVar8 & 0x1000000000) != 0) {
      uVar2 = 5;
    }
    uVar9 = *(ulong *)(lVar10 + (long)(int)(uVar3 | 0x448) * 8);
    if ((uVar8 & 0x2000000000) != 0) {
      uVar2 = 6;
    }
    if ((uVar8 & 0x4000000000) != 0) {
      uVar2 = 7;
    }
    if ((uVar8 & 0x8000000000) != 0) {
      uVar2 = 8;
    }
    if ((uVar8 & 0x10000000000) != 0) {
      uVar2 = 9;
    }
    uVar1 = 0;
    if ((uVar9 & 0x80000000) == 0) {
      uVar1 = uVar5;
    }
    if ((uVar9 & 0x100000000) != 0) {
      uVar1 = 1;
    }
    if ((uVar9 & 0x200000000) != 0) {
      uVar1 = 2;
    }
    if ((uVar9 & 0x400000000) != 0) {
      uVar1 = 3;
    }
    if ((uVar9 & 0x800000000) != 0) {
      uVar1 = 4;
    }
    if ((uVar9 & 0x1000000000) != 0) {
      uVar1 = 5;
    }
    uVar8 = *(ulong *)(lVar10 + (long)(int)(uVar3 | 0x442) * 8);
    if ((uVar9 & 0x2000000000) != 0) {
      uVar1 = 6;
    }
    if ((uVar9 & 0x4000000000) != 0) {
      uVar1 = 7;
    }
    if ((uVar9 & 0x8000000000) != 0) {
      uVar1 = 8;
    }
    if ((uVar9 & 0x10000000000) != 0) {
      uVar1 = 9;
    }
    uVar3 = 0;
    if ((uVar8 & 0x80000000) == 0) {
      uVar3 = uVar5;
    }
    if ((uVar8 & 0x100000000) != 0) {
      uVar3 = 1;
    }
    if ((uVar8 & 0x200000000) != 0) {
      uVar3 = 2;
    }
    if ((uVar8 & 0x400000000) != 0) {
      uVar3 = 3;
    }
    if ((uVar8 & 0x800000000) != 0) {
      uVar3 = 4;
    }
    if ((uVar8 & 0x1000000000) != 0) {
      uVar3 = 5;
    }
    if ((uVar8 & 0x2000000000) != 0) {
      uVar3 = 6;
    }
    if ((uVar8 & 0x4000000000) != 0) {
      uVar3 = 7;
    }
    if ((uVar8 & 0x8000000000) != 0) {
      uVar3 = 8;
    }
    if ((uVar8 & 0x10000000000) != 0) {
      uVar3 = 9;
    }
    if ((local_33 & 1) != 0) {
      uVar7 = uVar6;
    }
    if ((local_33 & 4) != 0) {
      uVar7 = uVar2;
    }
    if ((local_33 & 8) != 0) {
      uVar7 = uVar1;
    }
    *(undefined1 *)((long)&gui_actions + (ulong)uVar6) = 0;
    if ((local_33 & 2) != 0) {
      uVar7 = uVar3;
    }
    *(undefined1 *)((long)&gui_actions + (ulong)uVar2) = 0;
    *(undefined1 *)((long)&gui_actions + (ulong)uVar1) = 0;
    *(undefined1 *)((long)&gui_actions + (ulong)uVar3) = 0;
    *(undefined1 *)((long)&gui_actions + (ulong)uVar7) = 1;
  } while( true );
}

// --- 函数: load_logo ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void load_logo(long *param_1)

{
  time_t tVar1;
  FILE *__stream;
  void *__ptr;
  size_t sVar2;
  long lVar3;
  char acStack_428 [1056];
  long local_8;
  
  lVar3 = *param_1;
  local_8 = (long)__stack_chk_guard;
  tVar1 = time((time_t *)0x0);
  snprintf(acStack_428,0x420,"%s%cdrastic_logo_%d.raw",lVar3 + 0x8a780,0x2f,(uint)tVar1 & 1);
  __stream = fopen(acStack_428,"rb");
  if (__stream == (FILE *)0x0) {
    param_1[7] = 0;
  }
  else {
    __ptr = malloc(120000);
    param_1[7] = (long)__ptr;
    sVar2 = fread(__ptr,120000,1,__stream);
    if (sVar2 != 1) {
      free((void *)param_1[7]);
      param_1[7] = 0;
    }
    fclose(__stream);
  }
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: CONCAT17 ---
/*
       CONCAT17((byte)((ulong)uVar21 >> 0x38) & ~(byte)((ulong)uVar17 >> 0x38),
                CONCAT16((byte)((ulong)uVar21 >> 0x30) & ~(byte)((ulong)uVar17 >> 0x30),
                         CONCAT15((byte)((ulong)uVar21 >> 0x28) & ~(byte)((ulong)uVar17 >> 0x28),
                                  CONCAT14((byte)((ulong)uVar21 >> 0x20) &
                                           ~(byte)((ulong)uVar17 >> 0x20),
                                           CONCAT13((byte)((ulong)uVar21 >> 0x18) &
                                                    ~(byte)((ulong)uVar17 >> 0x18),
                                                    CONCAT12((byte)((ulong)uVar21 >> 0x10) &
                                                             ~(byte)((ulong)uVar17 >> 0x10),
                                                             CONCAT11((byte)((ulong)uVar21 >> 8) &
                                                                      ~(byte)((ulong)uVar17 >> 8),
                                                                      (byte)uVar21 & ~(byte)uVar17))
                                                   )))));
  *(ulong *)(param_3 + 0x1b40) =
       CONCAT17((byte)((ulong)uVar20 >> 0x38) & ~(byte)((ulong)uVar16 >> 0x38),
                CONCAT16((byte)((ulong)uVar20 >> 0x30) & ~(byte)((ulong)uVar16 >> 0x30),
                         CONCAT15((byte)((ulong)uVar20 >> 0x28) & ~(byte)((ulong)uVar16 >> 0x28),
                                  CONCAT14((byte)((ulong)uVar20 >> 0x20) &
                                           ~(byte)((ulong)uVar16 >> 0x20),
                                           CONCAT13((byte)((ulong)uVar20 >> 0x18) &
                                                    ~(byte)((ulong)uVar16 >> 0x18),
                                                    CONCAT12((byte)((ulong)uVar20 >> 0x10) &
                                                             ~(byte)((ulong)uVar16 >> 0x10),
                                                             CONCAT11((byte)((ulong)uVar20 >> 8) &
                                                                      ~(byte)((ulong)uVar16 >> 8),
                                                                      (byte)uVar20 & ~(byte)uVar16))
                                                   )))));
  *(ulong *)(param_3 + 7000) =
       CONCAT17((byte)((ulong)uVar19 >> 0x38) & ~(byte)((ulong)uVar15 >> 0x38),
                CONCAT16((byte)((ulong)uVar19 >> 0x30) & ~(byte)((ulong)uVar15 >> 0x30),
                         CONCAT15((byte)((ulong)uVar19 >> 0x28) & ~(byte)((ulong)uVar15 >> 0x28),
                                  CONCAT14((byte)((ulong)uVar19 >> 0x20) &
                                           ~(byte)((ulong)uVar15 >> 0x20),
                                           CONCAT13((byte)((ulong)uVar19 >> 0x18) &
                                                    ~(byte)((ulong)uVar15 >> 0x18),
                                                    CONCAT12((byte)((ulong)uVar19 >> 0x10) &
                                                             ~(byte)((ulong)uVar15 >> 0x10),
                                                             CONCAT11((byte)((ulong)uVar19 >> 8) &
                                                                      ~(byte)((ulong)uVar15 >> 8),
                                                                      (byte)uVar19 & ~(byte)uVar15))
                                                   )))));
  *(ulong *)(param_3 + 0x1b50) =
       CONCAT17((byte)((ulong)uVar18 >> 0x38) & ~(byte)((ulong)uVar14 >> 0x38),
                CONCAT16((byte)((ulong)uVar18 >> 0x30) & ~(byte)((ulong)uVar14 >> 0x30),
                         CONCAT15((byte)((ulong)uVar18 >> 0x28) & ~(byte)((ulong)uVar14 >> 0x28),
                                  CONCAT14((byte)((ulong)uVar18 >> 0x20) &
                                           ~(byte)((ulong)uVar14 >> 0x20),
                                           CONCAT13((byte)((ulong)uVar18 >> 0x18) &
                                                    ~(byte)((ulong)uVar14 >> 0x18),
                                                    CONCAT12((byte)((ulong)uVar18 >> 0x10) &
                                                             ~(byte)((ulong)uVar14 >> 0x10),
                                                             CONCAT11((byte)((ulong)uVar18 >> 8) &
                                                                      ~(byte)((ulong)uVar14 >> 8),
                                                                      (byte)uVar18 & ~(byte)uVar14))
                                                   )))));
  uVar9 = param_9 & 5;
  if (uVar9 == 4) {
    if ((param_3 + 0x1b4f) - uVar5 < 0x1f || (param_3 + 0x1b6f) - uVar5 < 0x1f) {
      *(ulong *)(param_3 + 0x1b88) = *(ulong *)(param_3 + 0x1b68) & *(ulong *)(param_3 + 0x1b48);
      *(ulong *)(param_3 + 0x1b80) = *(ulong *)(param_3 + 0x1b60) & *(ulong *)(param_3 + 0x1b40);
      *(ulong *)(param_3 + 0x1b98) = *(ulong *)(param_3 + 0x1b78) & *(ulong *)(param_3 + 7000);
      *(ulong *)(param_3 + 0x1b90) = *(ulong *)(param_3 + 0x1b70) & *(ulong *)(param_3 + 0x1b50);
    }
    else {
      uVar21 = *(undefined8 *)(param_3 + 0x1b48);
      uVar20 = *(undefined8 *)(param_3 + 0x1b40);
      uVar19 = *(undefined8 *)(param_3 + 7000);
      uVar18 = *(undefined8 *)(param_3 + 0x1b50);
      uVar17 = *(undefined8 *)(param_3 + 0x1b68);
      uVar16 = *(undefined8 *)(param_3 + 0x1b60);
      uVar15 = *(undefined8 *)(param_3 + 0x1b78);
      uVar14 = *(undefined8 *)(param_3 + 0x1b70);
      *(ulong *)(param_3 + 0x1b88) =
           CONCAT17((byte)((ulong)uVar17 >> 0x38) & (byte)((ulong)uVar21 >> 0x38),
                    CONCAT16((byte)((ulong)uVar17 >> 0x30) & (byte)((ulong)uVar21 >> 0x30),
                             CONCAT15((byte)((ulong)uVar17 >> 0x28) & (byte)((ulong)uVar21 >> 0x28),
                                      CONCAT14((byte)((ulong)uVar17 >> 0x20) &
                                               (byte)((ulong)uVar21 >> 0x20),
                                               CONCAT13((byte)((ulong)uVar17 >> 0x18) &
                                                        (byte)((ulong)uVar21 >> 0x18),
                                                        CONCAT12((byte)((ulong)uVar17 >> 0x10) &
                                                                 (byte)((ulong)uVar21 >> 0x10),
                                                                 CONCAT11((byte)((ulong)uVar17 >> 8)
                                                                          & (byte)((ulong)uVar21 >>
                                                                                  8),
                                                                          (byte)uVar17 &
                                                                          (byte)uVar21)))))));
      *(ulong *)(param_3 + 0x1b80) =
           CONCAT17((byte)((ulong)uVar16 >> 0x38) & (byte)((ulong)uVar20 >> 0x38),
                    CONCAT16((byte)((ulong)uVar16 >> 0x30) & (byte)((ulong)uVar20 >> 0x30),
                             CONCAT15((byte)((ulong)uVar16 >> 0x28) & (byte)((ulong)uVar20 >> 0x28),
                                      CONCAT14((byte)((ulong)uVar16 >> 0x20) &
                                               (byte)((ulong)uVar20 >> 0x20),
                                               CONCAT13((byte)((ulong)uVar16 >> 0x18) &
                                                        (byte)((ulong)uVar20 >> 0x18),
                                                        CONCAT12((byte)((ulong)uVar16 >> 0x10) &
                                                                 (byte)((ulong)uVar20 >> 0x10),
                                                                 CONCAT11((byte)((ulong)uVar16 >> 8)
                                                                          & (byte)((ulong)uVar20 >>
                                                                                  8),
                                                                          (byte)uVar16 &
                                                                          (byte)uVar20)))))));
      *(ulong *)(param_3 + 0x1b98) =
           CONCAT17((byte)((ulong)uVar15 >> 0x38) & (byte)((ulong)uVar19 >> 0x38),
                    CONCAT16((byte)((ulong)uVar15 >> 0x30) & (byte)((ulong)uVar19 >> 0x30),
                             CONCAT15((byte)((ulong)uVar15 >> 0x28) & (byte)((ulong)uVar19 >> 0x28),
                                      CONCAT14((byte)((ulong)uVar15 >> 0x20) &
                                               (byte)((ulong)uVar19 >> 0x20),
                                               CONCAT13((byte)((ulong)uVar15 >> 0x18) &
                                                        (byte)((ulong)uVar19 >> 0x18),
                                                        CONCAT12((byte)((ulong)uVar15 >> 0x10) &
                                                                 (byte)((ulong)uVar19 >> 0x10),
                                                                 CONCAT11((byte)((ulong)uVar15 >> 8)
                                                                          & (byte)((ulong)uVar19 >>
                                                                                  8),
                                                                          (byte)uVar15 &
                                                                          (byte)uVar19)))))));
      *(ulong *)(param_3 + 0x1b90) =
           CONCAT17((byte)((ulong)uVar14 >> 0x38) & (byte)((ulong)uVar18 >> 0x38),
                    CONCAT16((byte)((ulong)uVar14 >> 0x30) & (byte)((ulong)uVar18 >> 0x30),
                             CONCAT15((byte)((ulong)uVar14 >> 0x28) & (byte)((ulong)uVar18 >> 0x28),
                                      CONCAT14((byte)((ulong)uVar14 >> 0x20) &
                                               (byte)((ulong)uVar18 >> 0x20),
                                               CONCAT13((byte)((ulong)uVar14 >> 0x18) &
                                                        (byte)((ulong)uVar18 >> 0x18),
                                                        CONCAT12((byte)((ulong)uVar14 >> 0x10) &
                                                                 (byte)((ulong)uVar18 >> 0x10),
                                                                 CONCAT11((byte)((ulong)uVar14 >> 8)
                                                                          & (byte)((ulong)uVar18 >>
                                                                                  8),
                                                                          (byte)uVar14 &
                                                                          (byte)uVar18)))))));
    }
LAB_0013cc80:
    if ((param_9 >> 4 & 1) != 0) {
      if (uVar4 < param_3 + 0x1b90U && uVar5 < param_3 + 0x13d0U) {
        param_9 = param_9 & 0xffffffef;
        *(ulong *)(param_3 + 0x1b80) =
             *(ulong *)(param_3 + 0x1b80) & (*(ulong *)(param_3 + 0x13c0) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b88) =
             *(ulong *)(param_3 + 0x1b88) & (*(ulong *)(param_3 + 0x13c8) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b90) =
             *(ulong *)(param_3 + 0x1b90) & (*(ulong *)(param_3 + 0x13d0) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b98) =
             *(ulong *)(param_3 + 0x1b98) & (*(ulong *)(param_3 + 0x13d8) ^ 0xffffffffffffffff);
      }
      else {
        uVar17 = *(undefined8 *)(param_3 + 0x13c8);
        uVar16 = *(undefined8 *)(param_3 + 0x13c0);
        param_9 = param_9 & 0xffffffef;
        uVar15 = *(undefined8 *)(param_3 + 0x13d8);
        uVar14 = *(undefined8 *)(param_3 + 0x13d0);
        uVar21 = *(undefined8 *)(param_3 + 0x1b88);
        uVar20 = *(undefined8 *)(param_3 + 0x1b80);
        uVar19 = *(undefined8 *)(param_3 + 0x1b98);
        uVar18 = *(undefined8 *)(param_3 + 0x1b90);
        *(ulong *)(param_3 + 0x1b88) =
             CONCAT17((byte)((ulong)uVar21 >> 0x38) & ~(byte)((ulong)uVar17 >> 0x38),
                      CONCAT16((byte)((ulong)uVar21 >> 0x30) & ~(byte)((ulong)uVar17 >> 0x30),
                               CONCAT15((byte)((ulong)uVar21 >> 0x28) &
                                        ~(byte)((ulong)uVar17 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar21 >> 0x20) &
                                                 ~(byte)((ulong)uVar17 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar21 >> 0x18) &
                                                          ~(byte)((ulong)uVar17 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar21 >> 0x10) &
                                                                   ~(byte)((ulong)uVar17 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar21 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar17 >>
                                                                                   8),
                                                                            (byte)uVar21 &
                                                                            ~(byte)uVar17)))))));
        *(ulong *)(param_3 + 0x1b80) =
             CONCAT17((byte)((ulong)uVar20 >> 0x38) & ~(byte)((ulong)uVar16 >> 0x38),
                      CONCAT16((byte)((ulong)uVar20 >> 0x30) & ~(byte)((ulong)uVar16 >> 0x30),
                               CONCAT15((byte)((ulong)uVar20 >> 0x28) &
                                        ~(byte)((ulong)uVar16 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar20 >> 0x20) &
                                                 ~(byte)((ulong)uVar16 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar20 >> 0x18) &
                                                          ~(byte)((ulong)uVar16 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar20 >> 0x10) &
                                                                   ~(byte)((ulong)uVar16 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar20 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar16 >>
                                                                                   8),
                                                                            (byte)uVar20 &
                                                                            ~(byte)uVar16)))))));
        *(ulong *)(param_3 + 0x1b98) =
             CONCAT17((byte)((ulong)uVar19 >> 0x38) & ~(byte)((ulong)uVar15 >> 0x38),
                      CONCAT16((byte)((ulong)uVar19 >> 0x30) & ~(byte)((ulong)uVar15 >> 0x30),
                               CONCAT15((byte)((ulong)uVar19 >> 0x28) &
                                        ~(byte)((ulong)uVar15 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar19 >> 0x20) &
                                                 ~(byte)((ulong)uVar15 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar19 >> 0x18) &
                                                          ~(byte)((ulong)uVar15 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar19 >> 0x10) &
                                                                   ~(byte)((ulong)uVar15 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar19 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar15 >>
                                                                                   8),
                                                                            (byte)uVar19 &
                                                                            ~(byte)uVar15)))))));
        *(ulong *)(param_3 + 0x1b90) =
             CONCAT17((byte)((ulong)uVar18 >> 0x38) & ~(byte)((ulong)uVar14 >> 0x38),
                      CONCAT16((byte)((ulong)uVar18 >> 0x30) & ~(byte)((ulong)uVar14 >> 0x30),
                               CONCAT15((byte)((ulong)uVar18 >> 0x28) &
                                        ~(byte)((ulong)uVar14 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar18 >> 0x20) &
                                                 ~(byte)((ulong)uVar14 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar18 >> 0x18) &
                                                          ~(byte)((ulong)uVar14 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar18 >> 0x10) &
                                                                   ~(byte)((ulong)uVar14 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar18 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar14 >>
                                                                                   8),
                                                                            (byte)uVar18 &
                                                                            ~(byte)uVar14)))))));
      }
    }
    uVar10 = uVar5;
    if ((param_9 >> 5 & 1) == 0) {
      local_3c = param_9 & 8;
      uVar13 = param_9;
    }
    else {
      if (param_3 + 0x1440U < param_3 + 0x1b90U && uVar5 < param_3 + 0x1450U) {
        *(ulong *)(param_3 + 0x1b80) =
             *(ulong *)(param_3 + 0x1b80) & (*(ulong *)(param_3 + 0x1440) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b88) =
             *(ulong *)(param_3 + 0x1b88) & (*(ulong *)(param_3 + 0x1448) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b90) =
             *(ulong *)(param_3 + 0x1b90) & (*(ulong *)(param_3 + 0x1450) ^ 0xffffffffffffffff);
        *(ulong *)(param_3 + 0x1b98) =
             *(ulong *)(param_3 + 0x1b98) & (*(ulong *)(param_3 + 0x1458) ^ 0xffffffffffffffff);
      }
      else {
        uVar17 = *(undefined8 *)(param_3 + 0x1448);
        uVar16 = *(undefined8 *)(param_3 + 0x1440);
        uVar15 = *(undefined8 *)(param_3 + 0x1458);
        uVar14 = *(undefined8 *)(param_3 + 0x1450);
        uVar21 = *(undefined8 *)(param_3 + 0x1b88);
        uVar20 = *(undefined8 *)(param_3 + 0x1b80);
        uVar19 = *(undefined8 *)(param_3 + 0x1b98);
        uVar18 = *(undefined8 *)(param_3 + 0x1b90);
        *(ulong *)(param_3 + 0x1b88) =
             CONCAT17((byte)((ulong)uVar21 >> 0x38) & ~(byte)((ulong)uVar17 >> 0x38),
                      CONCAT16((byte)((ulong)uVar21 >> 0x30) & ~(byte)((ulong)uVar17 >> 0x30),
                               CONCAT15((byte)((ulong)uVar21 >> 0x28) &
                                        ~(byte)((ulong)uVar17 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar21 >> 0x20) &
                                                 ~(byte)((ulong)uVar17 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar21 >> 0x18) &
                                                          ~(byte)((ulong)uVar17 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar21 >> 0x10) &
                                                                   ~(byte)((ulong)uVar17 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar21 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar17 >>
                                                                                   8),
                                                                            (byte)uVar21 &
                                                                            ~(byte)uVar17)))))));
        *(ulong *)(param_3 + 0x1b80) =
             CONCAT17((byte)((ulong)uVar20 >> 0x38) & ~(byte)((ulong)uVar16 >> 0x38),
                      CONCAT16((byte)((ulong)uVar20 >> 0x30) & ~(byte)((ulong)uVar16 >> 0x30),
                               CONCAT15((byte)((ulong)uVar20 >> 0x28) &
                                        ~(byte)((ulong)uVar16 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar20 >> 0x20) &
                                                 ~(byte)((ulong)uVar16 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar20 >> 0x18) &
                                                          ~(byte)((ulong)uVar16 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar20 >> 0x10) &
                                                                   ~(byte)((ulong)uVar16 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar20 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar16 >>
                                                                                   8),
                                                                            (byte)uVar20 &
                                                                            ~(byte)uVar16)))))));
        *(ulong *)(param_3 + 0x1b98) =
             CONCAT17((byte)((ulong)uVar19 >> 0x38) & ~(byte)((ulong)uVar15 >> 0x38),
                      CONCAT16((byte)((ulong)uVar19 >> 0x30) & ~(byte)((ulong)uVar15 >> 0x30),
                               CONCAT15((byte)((ulong)uVar19 >> 0x28) &
                                        ~(byte)((ulong)uVar15 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar19 >> 0x20) &
                                                 ~(byte)((ulong)uVar15 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar19 >> 0x18) &
                                                          ~(byte)((ulong)uVar15 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar19 >> 0x10) &
                                                                   ~(byte)((ulong)uVar15 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar19 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar15 >>
                                                                                   8),
                                                                            (byte)uVar19 &
                                                                            ~(byte)uVar15)))))));
        *(ulong *)(param_3 + 0x1b90) =
             CONCAT17((byte)((ulong)uVar18 >> 0x38) & ~(byte)((ulong)uVar14 >> 0x38),
                      CONCAT16((byte)((ulong)uVar18 >> 0x30) & ~(byte)((ulong)uVar14 >> 0x30),
                               CONCAT15((byte)((ulong)uVar18 >> 0x28) &
                                        ~(byte)((ulong)uVar14 >> 0x28),
                                        CONCAT14((byte)((ulong)uVar18 >> 0x20) &
                                                 ~(byte)((ulong)uVar14 >> 0x20),
                                                 CONCAT13((byte)((ulong)uVar18 >> 0x18) &
                                                          ~(byte)((ulong)uVar14 >> 0x18),
                                                          CONCAT12((byte)((ulong)uVar18 >> 0x10) &
                                                                   ~(byte)((ulong)uVar14 >> 0x10),
                                                                   CONCAT11((byte)((ulong)uVar18 >>
                                                                                  8) &
                                                                            ~(byte)((ulong)uVar14 >>
                                                                                   8),
                                                                            (byte)uVar18 &
                                                                            ~(byte)uVar14)))))));
      }
      uVar13 = param_9 & 0xffffffdf;
      local_3c = param_9 & 8;
    }
  }
*/
// --- 函数: screen_copy16 ---

void screen_copy16(ushort *param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  long lVar4;
  ulong uVar5;
  ushort *puVar6;
  ushort *puVar7;
  ushort *puVar8;
  ulong uVar9;
  ushort *puVar10;
  
  lVar4 = get_screen_ptr(param_2);
  uVar5 = get_screen_pitch(param_2);
  iVar2 = get_screen_hires_mode(param_2);
  if (lVar4 != 0) {
    iVar3 = get_screen_bytes_per_pixel();
    puVar8 = param_1 + 0x100;
    if (iVar3 == 2) {
      puVar7 = param_1;
      puVar10 = puVar8;
      while( true ) {
        uVar9 = 0;
        do {
          puVar6 = puVar7 + 1;
          *puVar7 = *(ushort *)(lVar4 + uVar9 * 2);
          uVar9 = (ulong)(uint)((int)uVar9 + iVar2 + 1);
          puVar7 = puVar6;
        } while (puVar6 != puVar8);
        lVar4 = lVar4 + (uVar5 & 0xfffffffe);
        puVar8 = puVar8 + 0x100;
        if (puVar10 == param_1 + 0xc000) break;
        puVar7 = puVar10;
        puVar10 = puVar10 + 0x100;
      }
    }
    else {
      puVar7 = param_1;
      puVar10 = puVar8;
      while( true ) {
        uVar9 = 0;
        do {
          uVar1 = *(uint *)(lVar4 + uVar9 * 4);
          uVar9 = (ulong)(uint)((int)uVar9 + iVar2 + 1);
          puVar6 = puVar7 + 1;
          *puVar7 = (ushort)(uVar1 >> 3) & 0x1f | (ushort)((uVar1 >> 0x13) << 0xb) |
                    (ushort)((uVar1 >> 10 & 0x3f) << 5);
          puVar7 = puVar6;
        } while (puVar8 != puVar6);
        lVar4 = lVar4 + (uVar5 & 0xfffffffc);
        puVar8 = puVar8 + 0x100;
        if (puVar10 == param_1 + 0xc000) break;
        puVar7 = puVar10;
        puVar10 = puVar10 + 0x100;
      }
    }
    return;
  }
  memset(param_1,0,0x18000);
  return;
}

// --- 函数: audio_pause ---

char audio_pause(long param_1)

{
  char cVar1;
  
  cVar1 = *(char *)(param_1 + 0x40027);
  if (cVar1 != '\0') {
    *(undefined1 *)(param_1 + 0x40027) = 1;
    return cVar1;
  }
  SDL_PauseAudio(1);
  *(undefined1 *)(param_1 + 0x40027) = 1;
  return '\0';
}

// --- 函数: set_font_narrow ---

void set_font_narrow(void)

{
  current_font = (undefined *)&font_c;
  return;
}

// --- 函数: create_menu_main ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
undefined8 * create_menu_main(long param_1)

{
  undefined8 *puVar1;
  void *pvVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 *puVar6;
  undefined8 *puVar7;
  undefined8 *puVar8;
  undefined4 uVar9;
  undefined8 *puVar10;
  long lVar11;
  
  lVar11 = *(long *)(param_1 + 8);
  puVar1 = malloc(0x30);
  *puVar1 = draw_menu_main;
  puVar1[1] = 0;
  *(undefined8 *)((long)puVar1 + 0x14) = 10;
  puVar1[5] = 0;
  pvVar2 = malloc(0x50);
  puVar1[4] = pvVar2;
  uVar3 = create_menu_options(param_1,puVar1);
  uVar4 = create_menu_controls(param_1,puVar1);
  uVar5 = create_menu_firmware(param_1,puVar1);
  uVar9 = 0xb4;
  if (*(int *)(param_1 + 0x40) == 0) {
    uVar9 = 0x158;
  }
  *(undefined4 *)(puVar1 + 2) = uVar9;
  puVar10 = (undefined8 *)puVar1[4];
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Change Options";
  *(undefined4 *)(puVar8 + 1) = 0x23;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select_menu;
  puVar8[4] = 0;
  puVar8[5] = destroy_select_menu;
  puVar6[6] = uVar3;
  *puVar10 = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Configure Controls";
  *(undefined4 *)(puVar8 + 1) = 0x24;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select_menu;
  puVar8[4] = 0;
  puVar8[5] = destroy_select_menu;
  puVar6[6] = uVar4;
  puVar10[1] = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Configure Firmware";
  *(undefined4 *)(puVar8 + 1) = 0x25;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select_menu;
  puVar8[4] = 0;
  puVar8[5] = destroy_select_menu;
  puVar6[6] = uVar5;
  puVar10[2] = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Configure Cheats";
  *(undefined4 *)(puVar8 + 1) = 0x26;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select;
  puVar8[4] = 0;
  puVar8[5] = 0;
  lVar11 = lVar11 + 0x458;
  puVar6[6] = select_cheat_menu;
  puVar10[3] = puVar6;
  puVar7 = malloc(0x50);
  puVar8 = puVar7;
  puVar6 = puVar7;
  if (puVar7 == (undefined8 *)0x0) {
    puVar6 = malloc(0x40);
    puVar8 = puVar6;
    if (puVar6 == (undefined8 *)0x0) {
      puVar8 = malloc(0x30);
    }
  }
  *puVar8 = "Load state   ";
  *(undefined4 *)(puVar8 + 1) = 0x28;
  puVar8[2] = draw_numeric;
  puVar8[3] = action_numeric;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = lVar11;
  puVar6[7] = 0x900000000;
  puVar7[3] = action_numeric_select;
  puVar7[4] = focus_savestate;
  puVar7[8] = modify_snapshot_bg;
  puVar7[9] = select_load_state;
  puVar10[4] = puVar7;
  puVar7 = malloc(0x50);
  puVar8 = puVar7;
  puVar6 = puVar7;
  if (puVar7 == (undefined8 *)0x0) {
    puVar6 = malloc(0x40);
    puVar8 = puVar6;
    if (puVar6 == (undefined8 *)0x0) {
      puVar8 = malloc(0x30);
    }
  }
  *puVar8 = "Save state   ";
  *(undefined4 *)(puVar8 + 1) = 0x29;
  puVar8[2] = draw_numeric;
  puVar8[3] = action_numeric;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = lVar11;
  puVar6[7] = 0x900000000;
  puVar7[3] = action_numeric_select;
  puVar7[4] = focus_savestate;
  puVar7[8] = modify_snapshot_bg;
  puVar7[9] = select_save_state;
  puVar10[5] = puVar7;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Load new game ";
  *(undefined4 *)(puVar8 + 1) = 0x2b;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = select_load_game;
  puVar10[6] = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Restart game  ";
  *(undefined4 *)(puVar8 + 1) = 0x2c;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = select_restart;
  puVar10[7] = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Return to game";
  *(undefined4 *)(puVar8 + 1) = 0x2e;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = select_return;
  puVar10[8] = puVar6;
  puVar6 = malloc(0x38);
  puVar8 = puVar6;
  if (puVar6 == (undefined8 *)0x0) {
    puVar8 = malloc(0x30);
  }
  *puVar8 = "Exit DraStic  ";
  *(undefined4 *)(puVar8 + 1) = 0x30;
  puVar8[2] = draw_menu_option;
  puVar8[3] = action_select;
  puVar8[4] = 0;
  puVar8[5] = 0;
  puVar6[6] = select_quit;
  puVar10[9] = puVar6;
  return puVar1;
}
*/

// --- 函数: CONCAT12 ---
/*
          uVar50 = (uint)((uint3)((CONCAT12(uVar80,CONCAT11(uVar44,(char)uVar28)) & 0x1f1f1f) +
                                  0x1f1f1f >> 5) & 0x10101);
          uVar58 = (uint)((uint3)((CONCAT12(bVar71,CONCAT11(uVar48,uVar52)) & 0x1f1f1f) + 0x1f1f1f
                                 >> 5) & 0x10101);
          uVar56 = (uint)((uint3)((CONCAT12(uVar68,CONCAT11(uVar29,(char)uVar38)) & 0x1f1f1f) +
                                  0x1f1f1f >> 5) & 0x10101);
          uVar64 = (uint)((uint3)((CONCAT12(bVar67,CONCAT11(uVar34,uVar60)) & 0x1f1f1f) + 0x1f1f1f
                                 >> 5) & 0x10101);
          iVar18 = (uint)((uint3)(CONCAT16((char)((uint3)(uVar53 >> 0xd) >> 8),
                                           CONCAT15((char)(uVar54 >> 8),
                                                    CONCAT14((char)uVar54,uVar50))) >> 0x20) &
                         0x10101) + (uint)(CONCAT12(bVar70,CONCAT11(uVar46,uVar51)) & 0x1f1f1f) * 2;
          iVar7 = (uint)((uint3)(CONCAT16((char)((uint3)(uVar61 >> 0xd) >> 8),
                                          CONCAT15((char)(uVar62 >> 8),CONCAT14((char)uVar62,uVar58)
                                                  )) >> 0x20) & 0x10101) +
                  (uint)(CONCAT12(bVar72,CONCAT11(uVar49,uVar57)) & 0x1f1f1f) * 2;
          uVar25 = (uVar6 >> 5 & 0x1f) << 8 | (uVar6 >> 10 & 0x1f) << 0x10 | uVar6 & 0x1f;
          uVar53 = (uVar23 >> 5 & 0x1f) << 8 | (uVar23 >> 10 & 0x1f) << 0x10 | uVar23 & 0x1f;
          uVar54 = (uVar22 >> 5 & 0x1f) << 8 | (uVar22 >> 10 & 0x1f) << 0x10 | uVar22 & 0x1f;
          uVar61 = (uVar5 >> 5 & 0x1f) << 8 | (uVar5 >> 10 & 0x1f) << 0x10 | uVar5 & 0x1f;
          uVar62 = (uVar3 >> 5 & 0x1f) << 8 | (uVar3 >> 10 & 0x1f) << 0x10 | uVar3 & 0x1f;
          uVar55 = (uVar2 >> 5 & 0x1f) << 8 | (uVar2 >> 10 & 0x1f) << 0x10 | uVar2 & 0x1f;
          uVar63 = (uVar4 >> 5 & 0x1f) << 8 | (uVar4 >> 10 & 0x1f) << 0x10 | uVar4 & 0x1f;
          puVar17[9] = (uVar53 + 0x1f1f1f >> 5 & 0x10101) + uVar53 * 2 | 0x1f000000;
          puVar17[10] = (uVar55 + 0x1f1f1f >> 5 & 0x10101) + uVar55 * 2 | 0x1f000000;
          puVar17[0xb] = (uVar54 + 0x1f1f1f >> 5 & 0x10101) + uVar54 * 2 | 0x1f000000;
          puVar17[0xc] = (uVar61 + 0x1f1f1f >> 5 & 0x10101) + uVar61 * 2 | 0x1f000000;
          puVar17[0xd] = (uVar63 + 0x1f1f1f >> 5 & 0x10101) + uVar63 * 2 | 0x1f000000;
          puVar17[0xe] = (uVar62 + 0x1f1f1f >> 5 & 0x10101) + uVar62 * 2 | 0x1f000000;
          puVar17[0xf] = (uVar25 + 0x1f1f1f >> 5 & 0x10101) + uVar25 * 2 | 0x1f000000;
          *(ulong *)(puVar17 + 3) =
               (ulong)CONCAT16((char)((uint)iVar7 >> 0x10),
                               CONCAT15((char)((uint)iVar7 >> 8),
                                        CONCAT14((char)iVar7,
                                                 uVar58 + (uint)(CONCAT12(bVar71,CONCAT11(uVar48,
                                                  uVar52)) & 0x1f1f1f) * 2))) | 0x1f0000001f000000;
          *(ulong *)(puVar17 + 1) =
               (ulong)CONCAT16((char)((uint)iVar18 >> 0x10),
                               CONCAT15((char)((uint)iVar18 >> 8),
                                        CONCAT14((char)iVar18,
                                                 uVar50 + (uint)(CONCAT12(uVar80,CONCAT11(uVar44,(
                                                  char)uVar28)) & 0x1f1f1f) * 2))) |
               0x1f0000001f000000;
          lVar11 = lVar11 - __stack_chk_guard;
          sVar13 = 0;
          *(ulong *)(puVar17 + 7) =
               CONCAT44((uint)((uint3)(CONCAT16((char)((uint3)(uVar39 >> 0xd) >> 8),
                                                CONCAT15((char)(uVar40 >> 8),
                                                         CONCAT14((char)uVar40,uVar64))) >> 0x20) &
                              0x10101) +
                        (uint)(CONCAT12(bVar69,CONCAT11(uVar42,uVar65)) & 0x1f1f1f) * 2,
                        uVar64 + (uint)(CONCAT12(bVar67,CONCAT11(uVar34,uVar60)) & 0x1f1f1f) * 2) |
               0x1f0000001f000000;
          *(ulong *)(puVar17 + 5) =
               CONCAT44((uint)((uint3)(CONCAT16((char)((uint3)(uVar31 >> 0xd) >> 8),
                                                CONCAT15((char)(uVar32 >> 8),
                                                         CONCAT14((char)uVar32,uVar56))) >> 0x20) &
                              0x10101) +
                        (uint)(CONCAT12(bVar66,CONCAT11(uVar30,uVar59)) & 0x1f1f1f) * 2,
                        uVar56 + (uint)(CONCAT12(uVar68,CONCAT11(uVar29,(char)uVar38)) & 0x1f1f1f) *
                                 2) | 0x1f0000001f000000;
          if (lVar11 == 0) {
            texture_cache_convert_4bpp_asm(param_3,pbVar1,uVar19);
            return;
          }
*/
// --- 函数: draw_menu_bg ---
// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void draw_menu_bg(undefined8 *param_1)

{
  void *__s;
  void *__s_00;
  char *pcVar1;
  undefined4 uVar2;
  long local_10;
  long local_8;
  
  local_8 = (long)__stack_chk_guard;
  clear_screen_menu(0);
  if (param_1[7] != 0) {
    uVar2 = 0x24;
    if (*(int *)(param_1 + 8) == 0) {
      uVar2 = 200;
    }
    blit_screen_menu(param_1[7],uVar2,0x28,400,0x96);
  }
  uVar2 = 0x204;
  if (*(int *)(param_1 + 8) != 0) {
    uVar2 = 0x160;
  }
  set_font_narrow_small();
  print_string("Version r2.5.2.2",0xffff,0,uVar2,0);
  set_font_wide();
  if (*(int *)(param_1 + 8) != 0) {
    if ((*(long *)(param_1[2] + 0x28) == 0) && (*(int *)(param_1[2] + 0x18) == 5)) {
      local_10 = 0;
      //savestate_index_timestamp(*param_1,*(undefined4 *)(param_1[1] + 0x458));
      set_font_narrow_small();
      if (local_10 == 0) {
        print_string("(No savestate)",0xffff,0,0x220,0);
        set_font_wide();
      }
      else {
        __s = malloc(0x18000);
        __s_00 = malloc(0x18000);
        pcVar1 = ctime(&local_10);
        memset(__s,0,0x18000);
        memset(__s_00,0,0x18000);
        load_state_index(*param_1,*(undefined4 *)(param_1[1] + 0x458),__s,__s_00,1);
        blit_screen_menu(__s,0x1d8,0x30,0x100,0xc0);
        blit_screen_menu(__s_00,0x1d8,0xf0,0x100,0xc0);
        print_string(pcVar1,0xffff,0,0x1dc,0);
        free(__s);
        free(__s_00);
        set_font_wide();
      }
    }
    else {
      blit_screen_menu(param_1[5],0x1d8,0x30,0x100,0xc0);
      blit_screen_menu(param_1[6],0x1d8,0xf0,0x100,0xc0);
    }
  }
  if (local_8 != (long)__stack_chk_guard) {
    __stack_chk_fail();
  }
  return;
}

// --- 函数: audio_revert_pause_state ---

void audio_revert_pause_state(undefined8 param_1,int param_2)

{
  if (param_2 != 0) {
    return;
  }
  audio_unpause(param_1);
  return;
}

// --- 函数: update_screen_menu ---

void update_screen_menu(void)

{
  int render_result;
  
  if (DAT_04031578 == NULL) {
    fprintf(stderr, "ERROR: update_screen_menu: renderer is NULL\n");
    fflush(stderr);
    return;
  }
  if (DAT_04031580 == NULL) {
    fprintf(stderr, "ERROR: update_screen_menu: texture is NULL\n");
    fflush(stderr);
    return;
  }
  if (DAT_04031598 == NULL) {
    fprintf(stderr, "ERROR: update_screen_menu: pixel buffer is NULL\n");
    fflush(stderr);
    return;
  }
  
  // 先清除渲染目标
  SDL_SetRenderDrawColor(DAT_04031578, 0, 0, 0, 255);
  render_result = SDL_RenderClear(DAT_04031578);
  if (render_result != 0) {
    fprintf(stderr, "WARNING: update_screen_menu: SDL_RenderClear failed: %s\n", SDL_GetError());
    fflush(stderr);
  }
  
  // 更新纹理
  render_result = SDL_UpdateTexture(DAT_04031580,0,DAT_04031598,0x640);
  if (render_result != 0) {
    fprintf(stderr, "WARNING: update_screen_menu: SDL_UpdateTexture failed: %s\n", SDL_GetError());
    fflush(stderr);
  }
  
  // 渲染纹理
  render_result = SDL_RenderCopy(DAT_04031578,DAT_04031580,0,0);
  if (render_result != 0) {
    fprintf(stderr, "WARNING: update_screen_menu: SDL_RenderCopy failed: %s\n", SDL_GetError());
    fflush(stderr);
  }
  
  // 显示画面
  SDL_RenderPresent(DAT_04031578);
  return;
}

// --- 函数: clear_gui_actions ---
/*
void clear_gui_actions(void)

{
  gui_actions = 0;
  DAT_040315f8 = 0;
  return;
}
*/
// --- 函数: delay_us ---

void delay_us(ulong param_1)

{
  SDL_Delay((param_1 & 0xffffffff) / 1000);
  return;
}

// --- 函数: load_file ---

int load_file(long *param_1,long *param_2,char *param_3)

{
  uint uVar1;
  ushort uVar2;
  uint uVar3;
  bool bVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  void *pvVar8;
  char *pcVar9;
  FILE *__stream;
  FILE *__stream_00;
  DIR *__dirp;
  dirent *pdVar10;
  size_t sVar11;
  undefined8 *puVar12;
  char *pcVar13;
  undefined8 *puVar14;
  undefined4 uVar15;
  ushort *puVar16;
  char *pcVar17;
  void *pvVar18;
  ushort *puVar19;
  uint uVar20;
  long lVar21;
  ulong uVar22;
  uint uVar23;
  ulong uVar24;
  uint uVar25;
  uint uVar26;
  ulong __size;
  ulong uVar27;
  uint uVar28;
  ulong uVar29;
  uint uVar30;
  uint uVar31;
  long lVar32;
  uint uVar33;
  uint uVar34;
  size_t __size_00;
  size_t __size_01;
  int *piVar35;
  uint local_d25c;
  void *local_d258;
  FILE *local_d250;
  ulong local_d220;
  long local_d210;
  uint local_d148;
  long local_d140;
  long lStack_d138;
  uint local_d130;
  undefined4 local_d12c;
  long local_d128;
  void *local_d120;
  void *local_d118;
  uint local_d110;
  void *local_d108;
  void *local_d100;
  uint local_d0f0;
  undefined *local_d0e8 [3];
  undefined1 auStack_d0d0 [216];
  long local_cff8;
  ushort *local_cfd0 = (ushort*)malloc(24336 * sizeof(ushort));  // Changed from stack to heap to prevent stack overflow
  // Removed unused auStack_11b0 [2528] to reduce stack usage
  ushort local_7d0;
  char acStack_7ce [6];
  undefined1 auStack_7c8 [32];
  undefined1 auStack_7a8 [32];
  undefined1 auStack_788 [32];
  undefined1 auStack_768 [32];
  undefined8 local_748;
  undefined8 uStack_740;
  undefined1 uStack_737;
  undefined8 uStack_730;
  undefined8 local_728;
  undefined8 uStack_720;
  undefined8 local_718;
  undefined8 uStack_710;
  undefined1 local_708;
  char acStack_700 [88];
  undefined1 auStack_6a8 [128];
  undefined1 auStack_628 [128];
  undefined1 auStack_5a8 [128];
  undefined1 auStack_528 [128];
  char acStack_4a8 [51];
  char acStack_475 [77];
  char acStack_428 [1056];
  long local_8;
  
  lVar21 = *param_1;
  local_8 = (long)__stack_chk_guard;
  if (local_cfd0 == (ushort*)0x0) {
    fprintf(stderr, "ERROR: Failed to allocate local_cfd0 buffer (24336 * sizeof(ushort))\n");
    return -1;
  }
  local_d0e8[0] = PTR_s_filename_002603d8;
  local_d0e8[1] = PTR_s_title_002603e0;
  local_d0e8[2] = PTR_s_rom_title_002603e8;
  platform_print_code(auStack_7c8,*(undefined2 *)(lVar21 + 0x862ba));
  platform_print_code(auStack_7a8,*(undefined2 *)(lVar21 + 0x862bc));
  platform_print_code(auStack_788,*(undefined2 *)(lVar21 + 0x862be));
  platform_print_code(auStack_768,*(undefined2 *)(lVar21 + 0x862c4));
  fprintf(stderr, "DEBUG: load_file: clearing screen menu...\n");
  fflush(stderr);
  clear_screen_menu(0);
  fprintf(stderr, "DEBUG: load_file: screen cleared, entering main loop...\n");
  fflush(stderr);
  set_font_narrow();
  // 从系统配置中获取初始路径（但可能未初始化）
  strcpy(param_3,(char *)(*param_1 + 0x8af80));
  fprintf(stderr, "DEBUG: load_file: initial path from config (may be uninitialized): %s\n", param_3);
  fflush(stderr);
  uVar6 = 0;
  local_d210 = 0;
  local_d148 = 0;
LAB_0017b584:
  if (2 < *(uint *)(param_1[1] + 0x43c)) {
    *(undefined4 *)(param_1[1] + 0x43c) = 0;
  }
  snprintf(auStack_6a8,0x80,"%s: select",auStack_7c8);
  snprintf(auStack_628,0x80,"%s: return to main menu",auStack_7a8);
  snprintf(auStack_5a8,0x80,"%s: go up directory",auStack_788);
  snprintf(auStack_528,0x80,"%s: switch display (%s)",auStack_768,
                local_d0e8[*(uint *)(param_1[1] + 0x43c)]);
  get_ticks_us(&local_d140);
  clear_gui_actions();
  local_d258 = malloc(0x300);
  pvVar8 = malloc(0x100);
  // 直接使用getcwd获取当前工作目录，不依赖可能未初始化的配置路径
  // 先初始化缓冲区
  memset(acStack_428, 0, 0x400);
  fprintf(stderr, "DEBUG: load_file: calling getcwd...\n");
  fflush(stderr);
  pcVar9 = getcwd(acStack_428,0x400);
  if (pcVar9 == (char *)0x0) {
    fprintf(stderr, "ERROR: Couldn't get current path, using default '.'\n");
    fflush(stderr);
    // 如果getcwd失败，使用当前目录的默认值
    memset(acStack_428, 0, 0x400);
    strncpy(acStack_428, ".", 0x400 - 1);
    acStack_428[0x400 - 1] = '\0';
    pcVar9 = acStack_428;
  }
  fprintf(stderr, "DEBUG: load_file: getcwd returned: %p, path: '%s' (length: %zu)\n", 
          pcVar9, acStack_428, strlen(acStack_428));
  fflush(stderr);
  // 验证路径是否有效
  if (acStack_428[0] == '\0' || strlen(acStack_428) == 0) {
    fprintf(stderr, "WARNING: load_file: path is empty after getcwd, using '.'\n");
    fflush(stderr);
    memset(acStack_428, 0, 0x400);
    strncpy(acStack_428, ".", 0x400 - 1);
    acStack_428[0x400 - 1] = '\0';
  }
  // 保存路径到堆上，防止被后续函数覆盖栈缓冲区
  char* saved_path = (char*)malloc(0x400);
  if (saved_path == NULL) {
    fprintf(stderr, "ERROR: load_file: failed to allocate memory for saved_path\n");
    fflush(stderr);
    return -1;
  }
  strncpy(saved_path, acStack_428, 0x400 - 1);
  saved_path[0x400 - 1] = '\0';
  fprintf(stderr, "DEBUG: load_file: saved path to heap: '%s'\n", saved_path);
  fflush(stderr);
  iVar5 = __xstat(0,"drastic_file_info.txt",(struct stat *)(auStack_d0d0 + 0x80));
  if (iVar5 == 0) {
    puts("Renaming drastic_file_info.txt to .drastic_file_info.txt.");
    rename("drastic_file_info.txt",".drastic_file_info.txt");
  }
  iVar5 = __xstat(0,".drastic_file_info.txt",(struct stat *)(auStack_d0d0 + 0x80));
  if ((iVar5 == 0) && (local_cff8 < *(long *)(*param_1 + 0x338))) {
    puts("Found file info cache older than game database file. Deleting.");
    unlink(".drastic_file_info.txt");
  }
  file_info_cache_load((long int*)&local_d120);
  icon_cache_load((long int*)&local_d108);
  __stream = fopen(".drastic_file_info.txt","ab");
  __stream_00 = fopen(".drastic_icon_cache.bin","ab");
  // 从堆上恢复路径，防止被栈溢出覆盖
  strncpy(acStack_428, saved_path, 0x400 - 1);
  acStack_428[0x400 - 1] = '\0';
  free(saved_path);
  // 在opendir之前再次验证路径，如果路径无效则强制使用当前目录
  size_t path_len = strlen(acStack_428);
  fprintf(stderr, "DEBUG: load_file: before opendir, acStack_428 content: '%s' (length: %zu, first char: 0x%02x)\n", 
          acStack_428, path_len, path_len > 0 ? (unsigned char)acStack_428[0] : 0);
  fflush(stderr);
  // 如果路径无效（空、包含非打印字符、或第一个字符不是有效路径字符），强制使用当前目录
  if (path_len == 0 || acStack_428[0] == '\0' || 
      (acStack_428[0] < 32 && acStack_428[0] != '.') || 
      (acStack_428[0] > 126)) {
    fprintf(stderr, "WARNING: load_file: path appears invalid (len=%zu, first=0x%02x), forcing to '.'\n", 
            path_len, path_len > 0 ? (unsigned char)acStack_428[0] : 0);
    fflush(stderr);
    memset(acStack_428, 0, 0x400);
    strncpy(acStack_428, ".", 0x400 - 1);
    acStack_428[0x400 - 1] = '\0';
  }
  fprintf(stderr, "DEBUG: load_file: opening directory: '%s' (final path)\n", acStack_428);
  fflush(stderr);
  __dirp = opendir(acStack_428);
  uVar20 = local_d110;
  if (__dirp == (DIR *)0x0) {
    fprintf(stderr, "ERROR: load_file: failed to open directory: %s\n", acStack_428);
    fflush(stderr);
    __size_01 = 0;
    bVar4 = true;
    uVar29 = 0;
    __size_00 = 0;
    sVar11 = 0;
    uVar23 = 0;
    local_d25c = 0;
    local_d220 = 0;  // ._0_4_ removed
  }
  else {
    fprintf(stderr, "DEBUG: load_file: directory opened successfully, reading files...\n");
    fflush(stderr);
    uVar29 = 0x20;
    uVar23 = 0;
    local_d25c = 0;
    uVar26 = 0x20;
    local_d220 = 0;
LAB_0017b718:
    __size_01 = local_d220 << 3;
    pdVar10 = readdir(__dirp);
    if (pdVar10 != (dirent *)0x0) {
      do {
        pcVar9 = pdVar10->d_name;
        sVar11 = strlen(pcVar9);
        // 使用完整路径进行 stat，而不是相对路径
        {
          char full_path[0x800];
          snprintf(full_path, sizeof(full_path), "%s/%s", acStack_428, pcVar9);
          iVar5 = __xstat(0, full_path, (struct stat *)auStack_d0d0);
          if (iVar5 < 0) {
            fprintf(stderr, "DEBUG: load_file: __xstat failed for %s: %m\n", full_path);
            fflush(stderr);
            // 如果stat失败，跳过这个条目
            pdVar10 = readdir(__dirp);
            if (pdVar10 == (dirent *)0x0) break;
            continue;
          }
        }
        // 检查是否是目录：st_mode的高4位是文件类型，0x4000表示目录
        // 在64位系统上，struct stat的布局是：st_dev(8) + st_ino(8) + st_nlink(4) + st_mode(4)
        // 所以st_mode的偏移量是20字节（0x14），而不是16字节
        // 但为了兼容性，我们使用offsetof来获取正确的偏移量
        struct stat* st = (struct stat*)auStack_d0d0;
        uint st_mode = st->st_mode;
        int is_dir = S_ISDIR(st_mode);
        fprintf(stderr, "DEBUG: load_file: found entry: %s (stat_ok=%d, st_mode=0x%x, is_dir=%d, file_count=%u, dir_count=%u)\n", 
                pcVar9, iVar5 >= 0, st_mode, is_dir, uVar23, local_d25c);
        fflush(stderr);
        if ((-1 < iVar5) && ((pdVar10->d_name[0] != '.' || (pdVar10->d_name[1] == '.')))) {
          uVar25 = (uint)sVar11;
          if (S_ISDIR(st_mode)) {
            pcVar13 = malloc((ulong)(uVar25 + 1));
            *(char **)((long)pvVar8 + __size_01) = pcVar13;
            local_d25c = local_d25c + 1;
            local_d220 = (ulong)local_d25c;
            strcpy(pcVar13,pcVar9);
          }
          else if (3 < uVar25) {
            // 提取文件扩展名：从文件名末尾查找最后一个 '.'
            {
              char* dot_pos = strrchr(pcVar9, '.');
              if (dot_pos != NULL && dot_pos != pcVar9) {
                // 找到了扩展名，使用点后的部分
                pcVar13 = dot_pos + 1;  // 跳过 '.'
              } else {
                // 没有找到扩展名，使用整个文件名
                pcVar13 = pcVar9;
              }
            }
            // param_2 是 &nds_ext，nds_ext 是 (long*)nds_ext_array
            // 所以 *param_2 是 nds_ext 的值，也就是 nds_ext_array 的地址
            // 但是我们需要的是 nds_ext_array[0]，也就是指向 ".nds" 的指针
            // 所以我们需要先得到 nds_ext_array 的地址，然后读取第一个元素
            pcVar17 = (char *)*((long*)*param_2);  // 这是 nds_ext_array[0]，指向 ".nds"
            fprintf(stderr, "DEBUG: load_file: checking file extension for %s, ext=%s, param_2=%p, *param_2=0x%lx\n", 
                    pcVar9, pcVar13, param_2, *param_2);
            fflush(stderr);
            if (pcVar17 != (char *)0x0) {
              // 检查指针是否有效
              fprintf(stderr, "DEBUG: load_file: param_2[0]=%s (address=0x%p, first byte=0x%02x)\n", 
                      pcVar17, (void*)pcVar17, (unsigned char)pcVar17[0]);
              fflush(stderr);
              uVar31 = 0;
LAB_0017c4f8:
              iVar5 = strcasecmp(pcVar13,pcVar17);
              fprintf(stderr, "DEBUG: load_file: strcasecmp('%s', '%s') = %d\n", pcVar13, pcVar17, iVar5);
              fflush(stderr);
              pvVar18 = local_d118;
              if (iVar5 != 0) {
                // 扩展名不匹配，检查下一个扩展名
                fprintf(stderr, "DEBUG: load_file: extension '%s' does not match '%s', checking next...\n", pcVar13, pcVar17);
                fflush(stderr);
                goto LAB_0017c4e8;
              }
              fprintf(stderr, "DEBUG: load_file: extension '%s' matches '%s', adding file to list\n", pcVar13, pcVar17);
              fflush(stderr);
              __size = (ulong)(uVar25 + 1);
              puVar12 = (undefined8 *)((long)local_d258 + (ulong)uVar23 * 0x18);
              if (*(int *)(param_1[1] + 0x43c) == 0) goto LAB_0017c520;
              if (local_d120 != (void *)0x0) {
                uVar27 = 0;
                uVar22 = (ulong)local_d110;
                while (uVar24 = uVar22, uVar27 < uVar24) {
                  while( true ) {
                    uVar22 = uVar24 + uVar27 >> 1;
                    piVar35 = *(int **)((long)pvVar18 + uVar22 * 8);
                    iVar5 = strcmp(pcVar9,*(char **)(piVar35 + 6));
                    if (iVar5 < 0) break;
                    if (iVar5 == 0) {
                      if (*piVar35 == -1) goto LAB_0017c520;
                      puVar14 = (undefined8 *)
                                game_database_lookup_by_game_code
                                          (*param_1 + 800,*piVar35,(char *)piVar35 + 1);
                      goto joined_r0x0017c7cc;
                    }
                    uVar27 = uVar22 + 1;
                    if (uVar24 <= uVar27) goto LAB_0017c6d4;
                  }
                }
              }
LAB_0017c6d4:
              //puVar14 = (undefined8 *)1;
              gamecard_database_entry_for_file(*param_1 + 800,pcVar9);
              if (__stream == (FILE *)0x0) {
joined_r0x0017c7cc:
                if (puVar14 == (undefined8 *)0x0) goto LAB_0017c520;
                if (*(int *)(param_1[1] + 0x43c) == 1) {
                  pcVar13 = (char *)*puVar14;
                  pcVar17 = malloc(__size);
                  *puVar12 = pcVar17;
                  strcpy(pcVar17,pcVar9);
                  if (pcVar13 != (char *)0x0) goto LAB_0017c770;
                  puVar12[1] = 0;
                }
                else {
                  snprintf(acStack_4a8,0x80,
                                "%-12s                                        %08x",puVar14 + 1,
                                *(undefined4 *)((long)puVar14 + 0x24));
                  pcVar13 = malloc(__size);
                  *puVar12 = pcVar13;
                  strcpy(pcVar13,pcVar9);
                  pcVar13 = acStack_4a8;
LAB_0017c770:
                  sVar11 = strlen(pcVar13);
                  pcVar9 = malloc((ulong)((int)sVar11 + 1));
                  puVar12[1] = pcVar9;
                  strcpy(pcVar9,pcVar13);
                }
              }
              else {
                uVar20 = uVar20 + 1;
                if (puVar14 != (undefined8 *)0x0) {
                  escape_str((byte*)acStack_4a8,(byte*)puVar14 + 1);
                  fprintf(__stream,"%08x \'%s\' %s\n",*(undefined4 *)((long)puVar14 + 0x24),
                                acStack_4a8,pcVar9);
                  goto joined_r0x0017c7cc;
                }
                fprintf(__stream,"%08x \'\' %s\n",0xffffffff,pcVar9);
LAB_0017c520:
                pcVar13 = malloc(__size);
                *puVar12 = pcVar13;
                strcpy(pcVar13,pcVar9);
                puVar12[1] = 0;
              }
              uVar23 = uVar23 + 1;
            }
          }
        }
joined_r0x0017c544:
        if (uVar23 == uVar26) {
          uVar26 = uVar23 * 2;
          local_d258 = realloc(local_d258,(ulong)uVar23 * 0x30);
        }
        if ((uint)uVar29 != local_d25c) goto LAB_0017b718;
        pvVar8 = realloc(pvVar8,uVar29 << 4);
        uVar29 = (ulong)((uint)uVar29 * 2);
        __size_01 = local_d220 << 3;
        pdVar10 = readdir(__dirp);
        if (pdVar10 == (dirent *)0x0) break;
      } while( true );
    }
    uVar29 = local_d220;
    bVar4 = uVar23 == 0;
    __size_00 = (ulong)uVar23 * 0x18;
    sVar11 = (size_t)uVar23;
    local_d220 = !bVar4;  // ._0_4_ removed
  }
  if ((__stream != (FILE *)0x0) && (fclose(__stream), uVar20 == 0)) {
    puts("Removing empty file info file.");
    unlink(".drastic_file_info.txt");
  }
  local_d250 = __stream_00;
  if (__stream_00 != (FILE *)0x0 && bVar4) {
    puts("Removing empty icon cache file.");
    local_d250 = (FILE *)0x0;
    fclose(__stream_00);
    unlink(".drastic_icon_cache.bin");
  }
  puVar12 = realloc(local_d258,__size_00);
  pvVar8 = realloc(pvVar8,__size_01);
  fprintf(stderr, "DEBUG: load_file: after reading directory, file count: %u, dir count: %u\n", uVar23, local_d25c);
  fflush(stderr);
  qsort(puVar12,sVar11,0x18,compare_file_names);
  qsort(pvVar8,uVar29,8,compare_directory_names);
  closedir(__dirp);
  sVar11 = strlen(acStack_428);
  if (sVar11 < 0x51) {
    snprintf(acStack_700,0x50,"%s",acStack_428);
  }
  else {
    iVar5 = __snprintf_chk(acStack_700,0x50,1,0x51,"...%s",acStack_428 + (sVar11 - 0x4d));
    if (iVar5 < 0) {
      puts("sprintf error warrning to make GCC shut up");
    }
  }
  if (uVar23 == 0) {
    get_ticks_us(&lStack_d138);
    uVar26 = 0;
    uVar25 = 0;
    uVar6 = 1;
    uVar20 = 0;
    printf("%f", (double)(ulong)(lStack_d138 - local_d140) / 1000000.0, 
                 "Directory load took %lf seconds.\n");
  }
  else {
    get_ticks_us(&lStack_d138);
    printf("%f", (double)(ulong)(lStack_d138 - local_d140) / 1000000.0, 
                 "Directory load took %lf seconds.\n");
    if (*param_3 != '\0') {
      uVar20 = 0;
      puVar14 = puVar12;
      do {
        iVar5 = strcmp((char *)*puVar14,param_3);
        if (iVar5 == 0) {
          uVar25 = uVar20 - 0xe;
          uVar26 = 0xe;
          if ((int)uVar25 < 0) {
            uVar25 = 0;
            uVar26 = uVar20;
          }
          goto LAB_0017b9b8;
        }
        uVar20 = uVar20 + 1;
        puVar14 = puVar14 + 3;
      } while (uVar23 != uVar20);
    }
    uVar26 = 0;
    uVar25 = 0;
    uVar20 = 0;
  }
LAB_0017b9b8:
  uVar33 = uVar6 ^ 1;
  uVar30 = 0;
  uVar34 = 0;
  uVar31 = 0;
  *param_3 = '\0';
LAB_0017b9d4:
  fprintf(stderr, "DEBUG: load_file: LAB_0017b9d4: drawing file list, file count: %u, dir count: %u\n", uVar23, local_d25c);
  fflush(stderr);
  // 检查屏幕指针是否有效（使用临时变量，避免作用域问题）
  {
    void* screen_ptr = (void*)get_screen_ptr(0);
    fprintf(stderr, "DEBUG: load_file: screen_ptr = %p\n", screen_ptr);
    fflush(stderr);
    if (screen_ptr == NULL) {
      fprintf(stderr, "ERROR: load_file: get_screen_ptr(0) returned NULL, cannot draw text\n");
      fflush(stderr);
    }
  }
  // 注意：draw_menu_bg 期望的是一个菜单结构体指针，而不是 load_file 的 param_1
  // 在 load_file 中，我们不需要调用 draw_menu_bg，因为背景已经在 menu 函数中绘制了
  // draw_menu_bg(param_1);  // 注释掉，因为参数类型不匹配
  print_string(acStack_700,0xffff,0,6,0);
  print_string(auStack_6a8,0xffff,0,6,0);
  print_string(auStack_628,0xffff,0,0x16e,0);
  print_string(auStack_5a8,0xffff,0,6,0);
  print_string(auStack_528,0xffff,0,0x16e,0);
  iVar5 = 0x14;
  uVar28 = uVar25;
  do {
    if (uVar28 < uVar23) {
      pcVar9 = (char *)puVar12[(ulong)uVar28 * 3 + 1];
      if (pcVar9 == (char *)0x0) {
        pcVar9 = (char *)puVar12[(ulong)uVar28 * 3];
      }
      sVar11 = strlen(pcVar9);
      if (0x3f < sVar11) {
        local_748 = *(undefined8 *)pcVar9;
        uStack_740 = *(undefined8 *)(pcVar9 + 8);
        uStack_730 = *(undefined8 *)(pcVar9 + 0x18);
        uStack_737 = (undefined1)((ulong)*(undefined8 *)(pcVar9 + 0x10) >> 8);
        local_728 = *(undefined8 *)(pcVar9 + 0x20);
        uStack_720 = *(undefined8 *)(pcVar9 + 0x28);
        local_718 = *(undefined8 *)(pcVar9 + 0x30);
        uStack_710 = *(undefined8 *)(pcVar9 + 0x38);
        local_708 = 0;
        pcVar9 = (char *)&local_748;
      }
      uVar15 = 0x17;
      if ((uVar33 & 1) == 0 || uVar20 != uVar28) {
        uVar15 = 0;
      }
      print_string(pcVar9,0xffff,uVar15,10,0);
    }
    uVar1 = local_d0f0;
    pvVar18 = local_d100;
    iVar5 = iVar5 + 0xf;
    uVar28 = uVar28 + 1;
  } while (iVar5 != 0x1b8);
  uVar29 = (ulong)uVar20;
  if ((uint)local_d220 == 0 || (uVar33 & 1) == 0) {
LAB_0017bc90:
    fill_screen_menu(0,0x244,0x118,0xa0,0);
  }
  else {
    pcVar9 = (char *)puVar12[uVar29 * 3];
    if (uVar20 < local_d0f0) {
      iVar5 = strcmp(pcVar9,*(char **)((long)local_d100 + uVar29 * 8));
      if (iVar5 != 0) {
LAB_0017bb88:
        lVar21 = 0;
        do {
          iVar5 = strcmp(pcVar9,*(char **)((long)pvVar18 + lVar21 * 8));
          if (iVar5 == 0) {
            pvVar18 = (void *)((long)local_d108 + lVar21 * 800);
            goto LAB_0017bbf8;
          }
          lVar21 = lVar21 + 1;
        } while ((uint)lVar21 < uVar1);
        goto LAB_0017bc5c;
      }
      pvVar18 = (void *)((long)local_d108 + uVar29 * 800);
LAB_0017bbf8:
      if (pvVar18 == (void *)0x0) goto LAB_0017bc5c;
    }
    else {
      if (local_d0f0 != 0) goto LAB_0017bb88;
LAB_0017bc5c:
      printf("Loading NDS icon for %s (%p)\n",pcVar9,local_d250);
      iVar5 = nds_file_get_icon_data(pcVar9,(undefined4 *)local_cfd0);
      if (iVar5 != 0) goto LAB_0017bc90;
      pvVar18 = (void *)icon_cache_add((long *)&local_d108,local_d250,local_cfd0,pcVar9);
    }
    iVar5 = 0;
    *(undefined2 *)((long)pvVar18 + 0x200) = 0xffff;
    puVar19 = local_cfd0;
    do {
      uVar28 = 0;
      puVar16 = puVar19;
      do {
        uVar1 = uVar28 >> 1;
        uVar3 = uVar28 & 1;
        uVar28 = uVar28 + 1;
        uVar2 = *(ushort *)
                 ((long)pvVar18 +
                 (((ulong)(uint)((int)(uint)*(byte *)((long)pvVar18 + (ulong)(iVar5 + uVar1)) >>
                                (uVar3 << 2)) & 0xf) + 0x100) * 2);
        uVar2 = (uVar2 >> 5 & 0x1f) << 6 | uVar2 >> 10 & 0x1f | uVar2 << 0xb;
        *puVar16 = uVar2;
        puVar16[1] = uVar2;
        puVar16[2] = uVar2;
        puVar16[3] = uVar2;
        puVar16[4] = uVar2;
        puVar16[0xa0] = uVar2;
        puVar16[0xa1] = uVar2;
        puVar16[0xa2] = uVar2;
        puVar16[0xa3] = uVar2;
        puVar16[0xa4] = uVar2;
        puVar16[0x140] = uVar2;
        puVar16[0x141] = uVar2;
        puVar16[0x142] = uVar2;
        puVar16[0x143] = uVar2;
        puVar16[0x144] = uVar2;
        puVar16[0x1e0] = uVar2;
        puVar16[0x1e1] = uVar2;
        puVar16[0x1e2] = uVar2;
        puVar16[0x1e3] = uVar2;
        puVar16[0x1e4] = uVar2;
        puVar16[0x280] = uVar2;
        puVar16[0x281] = uVar2;
        puVar16[0x282] = uVar2;
        puVar16[0x283] = uVar2;
        puVar16[0x284] = uVar2;
        puVar16 = puVar16 + 5;
      } while (uVar28 != 0x20);
      puVar19 = puVar19 + 800;
      iVar5 = iVar5 + 0x10;
    } while (puVar19 != &local_7d0);
    blit_screen_menu(local_cfd0,0x244,0x118,0xa0,0);
  }
  iVar5 = 0x14;
  uVar28 = uVar34;
  do {
    if (uVar28 < local_d25c) {
      pcVar9 = *(char **)((long)pvVar8 + (ulong)uVar28 * 8);
      sVar11 = strlen(pcVar9);
      if (0x10 < sVar11) {
        local_748 = *(undefined8 *)pcVar9;
        uStack_740 = *(undefined8 *)(pcVar9 + 8);
        uStack_737 = 0;
        pcVar9 = (char *)&local_748;
      }
      uVar1 = uVar6 & 1;
      if (uVar31 != uVar28) {
        uVar1 = 0;
      }
      uVar15 = 0x17;
      if (uVar1 == 0) {
        uVar15 = 0;
      }
      print_string(pcVar9,0xffff,uVar15,0x24b,0);
    }
    iVar5 = iVar5 + 0xf;
    uVar28 = uVar28 + 1;
  } while (iVar5 != 0x113);
  fprintf(stderr, "DEBUG: load_file: about to update screen menu, file count: %u\n", local_d25c);
  fflush(stderr);
  update_screen_menu();
  update_screen_menu();
  update_screen_menu();
  fprintf(stderr, "DEBUG: load_file: screen updated, waiting for input...\n");
  fflush(stderr);
  delay_us(5000);
  lVar21 = *param_1;
  do {
    get_gui_input(lVar21 + 0x5550,&local_d130);
  } while (local_d130 == 0xb);
  if (local_d130 == 5) {
    iVar5 = -1;
LAB_0017bfe0:
    clear_screen_menu(0);
    if (uVar23 == 0) goto LAB_0017c024;
    goto LAB_0017bfec;
  }
  if (local_d130 < 6) {
    if (local_d130 == 2) {
      uVar33 = (uint)local_d220 | uVar33 & 1;
      uVar6 = uVar33 ^ 1;
      clear_screen_menu(0);
      goto LAB_0017b9d4;
    }
    if (local_d130 < 3) {
      if (local_d130 == 0) {
        if (uVar6 == 0) {
          if (uVar20 == 0) goto LAB_0017bdc8;
          uVar20 = uVar20 - 1;
          if (uVar26 == 0) {
            uVar25 = uVar25 - 1;
            uVar33 = 1;
            clear_screen_menu(0);
            uVar6 = uVar26;
          }
          else {
            uVar26 = uVar26 - 1;
            clear_screen_menu(0);
          }
          goto LAB_0017b9d4;
        }
        if (uVar31 != 0) {
          uVar31 = uVar31 - 1;
          if (uVar30 == 0) {
            uVar34 = uVar34 - 1;
            clear_screen_menu(0);
          }
          else {
            uVar30 = uVar30 - 1;
            clear_screen_menu(0);
          }
          goto LAB_0017b9d4;
        }
      }
      else {
        if (local_d130 != 1) goto LAB_0017bdcc;
        if (uVar6 == 0) {
          if (uVar20 < uVar23 - 1) {
            uVar20 = uVar20 + 1;
            if (uVar26 == 0x1b) {
              uVar25 = uVar25 + 1;
              clear_screen_menu(0);
            }
            else {
              uVar26 = uVar26 + 1;
              clear_screen_menu(0);
            }
            goto LAB_0017b9d4;
          }
        }
        else if (uVar31 < local_d25c - 1) {
          uVar31 = uVar31 + 1;
          if (uVar30 == 0x10) {
            uVar34 = uVar34 + 1;
            clear_screen_menu(0);
          }
          else {
            uVar30 = uVar30 + 1;
            clear_screen_menu(0);
          }
          goto LAB_0017b9d4;
        }
      }
    }
    else {
      if (local_d130 == 3) {
        uVar33 = (uint)(local_d25c == 0 && uVar6 == 0);
        clear_screen_menu(0);
        uVar6 = (uint)(local_d25c != 0 || uVar6 != 0);
        goto LAB_0017b9d4;
      }
      if (uVar6 == 1) {
        iVar7 = chdir(*(char **)((long)pvVar8 + (ulong)uVar31 * 8));
        iVar5 = 1;
        if (iVar7 < 0) {
          puts("Couldn\'t change directory.");
        }
        goto LAB_0017bfe0;
      }
      if (uVar23 != 0) {
        iVar5 = 0;
        strcpy(param_3,(char *)puVar12[(ulong)uVar20 * 3]);
        goto LAB_0017c19c;
      }
LAB_0017bdc8:
      uVar6 = 0;
      uVar33 = 1;
    }
  }
  else if (local_d130 == 8) {
    if (uVar6 == 0) {
      iVar5 = 0x1c;
      do {
        if (uVar20 < uVar23 - 1) {
          uVar20 = uVar20 + 1;
          if (uVar26 == 0x1b) {
            uVar25 = uVar25 + 1;
          }
          else {
            uVar26 = uVar26 + 1;
          }
        }
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      goto LAB_0017bdc8;
    }
    iVar5 = 0x1c;
    do {
      if (uVar31 < local_d25c - 1) {
        uVar31 = uVar31 + 1;
        if (uVar30 == 0x10) {
          uVar34 = uVar34 + 1;
        }
        else {
          uVar30 = uVar30 + 1;
        }
      }
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  else if (local_d130 < 9) {
    if (local_d130 == 6) {
      iVar5 = 1;
      iVar7 = chdir("..");
      if (iVar7 < 0) {
        puts("Couldn\'t move up directory.");
      }
      goto LAB_0017bfe0;
    }
    if (local_d130 == 7) {
      if (uVar6 != 0) {
        iVar5 = 0x1c;
        do {
          if (uVar31 != 0) {
            uVar31 = uVar31 - 1;
            if (uVar30 == 0) {
              uVar34 = uVar34 - 1;
            }
            else {
              uVar30 = uVar30 - 1;
            }
          }
          iVar5 = iVar5 + -1;
        } while (iVar5 != 0);
        clear_screen_menu(0);
        goto LAB_0017b9d4;
      }
      iVar5 = 0x1c;
      do {
        if (uVar20 != 0) {
          uVar20 = uVar20 - 1;
          if (uVar26 == 0) {
            uVar25 = uVar25 - 1;
          }
          else {
            uVar26 = uVar26 - 1;
          }
        }
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      uVar33 = 1;
      uVar6 = 0;
    }
  }
  else if (local_d130 == 9) {
    if (uVar23 != 0) goto code_r0x0017c164;
  }
  else if (local_d130 == 10) {
    get_ticks_us(&local_d128);
    lVar21 = local_d128;
    if ((ulong)(local_d128 - local_d210) < 0x7a121) {
      if (local_d148 < 6) {
        uVar28 = local_d148 + 2;
        uVar29 = (ulong)local_d148;
        local_d148 = local_d148 + 1;
        goto LAB_0017be38;
      }
    }
    else {
      uVar29 = 0;
      local_d148 = 1;
      uVar28 = 2;
LAB_0017be38:
      *(char *)((long)&local_7d0 + uVar29) = (char)local_d12c;
      *(char *)((long)&local_7d0 + (ulong)uVar28) = '\0';
      if (uVar6 == 1) {
        if (local_d25c != 0) {
          lVar32 = 0;
          do {
            iVar5 = strncasecmp(*(char **)((long)pvVar8 + lVar32 * 8),(char *)&local_7d0,
                                (ulong)local_d148);
            if (iVar5 == 0) {
              uVar31 = (uint)lVar32;
              uVar34 = uVar31 - 0xe;
              uVar30 = 0xe;
              if ((int)uVar34 < 0) {
                uVar34 = 0;
                uVar30 = uVar31;
              }
              break;
            }
            lVar32 = lVar32 + 1;
          } while ((uint)lVar32 < local_d25c);
        }
      }
      else {
        puVar14 = puVar12;
        uVar28 = uVar6;
        if (uVar23 != 0) {
          do {
            if ((((char *)puVar14[1] != (char *)0x0) &&
                (iVar5 = strncasecmp((char *)puVar14[1],(char *)&local_7d0,(ulong)local_d148),
                iVar5 == 0)) ||
               (iVar5 = strncasecmp((char *)*puVar14,(char *)&local_7d0,(ulong)local_d148),
               iVar5 == 0)) {
              uVar20 = uVar28;
              uVar25 = uVar28 - 0xe;
              uVar26 = 0xe;
              if ((int)(uVar28 - 0xe) < 0) {
                uVar25 = uVar6;
                uVar26 = uVar28;
              }
              break;
            }
            uVar28 = uVar28 + 1;
            puVar14 = puVar14 + 3;
          } while (uVar23 != uVar28);
        }
      }
    }
    local_d210 = lVar21;
    clear_screen_menu(0);
    goto LAB_0017b9d4;
  }
LAB_0017bdcc:
  clear_screen_menu(0);
  goto LAB_0017b9d4;
LAB_0017c4e8:
  uVar31 = uVar31 + 1;
  // param_2 是 &nds_ext，nds_ext 是 (long*)nds_ext_array
  // 所以 *param_2 是 nds_ext_array 的地址
  // 我们需要访问 nds_ext_array[uVar31]
  {
    long* nds_ext_array_ptr = (long*)*param_2;  // 这是 nds_ext_array 的地址
    pcVar17 = (char *)nds_ext_array_ptr[uVar31];  // 这是 nds_ext_array[uVar31]
  }
  if (pcVar17 == (char *)0x0) goto joined_r0x0017c544;
  goto LAB_0017c4f8;
code_r0x0017c164:
  iVar5 = 1;
  pcVar9 = (char *)puVar12[(ulong)uVar20 * 3];
  uVar20 = *(int *)(param_1[1] + 0x43c) + 1;
  if (2 < uVar20) {
    uVar20 = 0;
  }
  *(uint *)(param_1[1] + 0x43c) = uVar20;
  strcpy(param_3,pcVar9);
LAB_0017c19c:
  clear_screen_menu(0);
LAB_0017bfec:
  puVar14 = puVar12;
  do {
    free((void *)*puVar14);
    if ((void *)puVar14[1] != (void *)0x0) {
      free((void *)puVar14[1]);
    }
    puVar14 = puVar14 + 3;
  } while (puVar14 != puVar12 + (ulong)(uVar23 - 1) * 3 + 3);
LAB_0017c024:
  lVar21 = 0;
  if (local_cfd0 != (ushort*)0x0) {
    free(local_cfd0);
    local_cfd0 = (ushort*)0x0;
  }
  free(puVar12);
  if (local_d25c != 0) {
    do {
      lVar32 = lVar21 * 8;
      lVar21 = lVar21 + 1;
      free(*(void **)((long)pvVar8 + lVar32));
    } while ((uint)lVar21 < local_d25c);
  }
  free(pvVar8);
  if (local_d120 != (void *)0x0) {
    if (local_d110 != 0) {
      uVar29 = 0;
      do {
        lVar21 = uVar29 * 0x20;
        uVar20 = (int)uVar29 + 1;
        uVar29 = (ulong)uVar20;
        free(*(void **)((long)local_d120 + lVar21 + 0x18));
      } while (uVar20 < local_d110);
    }
    free(local_d120);
    free(local_d118);
    local_d120 = (void *)0x0;
    local_d118 = (void *)0x0;
    local_d110 = 0;
  }
  if (local_d108 != (void *)0x0) {
    free(local_d108);
  }
  uVar29 = 0;
  if (local_d0f0 != 0) {
    do {
      lVar21 = uVar29 * 8;
      uVar20 = (int)uVar29 + 1;
      uVar29 = (ulong)uVar20;
      free(*(void **)((long)local_d100 + lVar21));
    } while (uVar20 < local_d0f0);
  }
  free(local_d100);
  iVar7 = 0;
  if (local_d250 != (FILE *)0x0) {
    iVar7 = fclose(local_d250);
  }
  if (iVar5 != 1) {
    set_font_wide();
    clear_gui_actions();
    if (local_8 - __stack_chk_guard != 0) {
                    
      __stack_chk_fail();
    }
    return iVar5;
  }
  goto LAB_0017b584;
}

// 已在 drastic_functions.h 中定义，注释掉以避免重定义
/*
void select_load_game(long *param_1)

{
  undefined4 uVar1;
  int iVar2;
  long lVar3;
  undefined1 auStack_428 [1056];
  long local_8;
  
  local_8 = __stack_chk_guard;
  iVar2 = load_file(param_1,(long *)&nds_ext,auStack_428);
  if (iVar2 != -1) {
    lVar3 = *param_1;
    iVar2 = load_nds(lVar3 + 800,auStack_428);
    if (-1 < iVar2) {
      uVar1 = *(undefined4 *)(lVar3 + 0x859f4);
      *(undefined8 *)((long)param_1 + 0x44) = 0x100000001;
      *(undefined4 *)((long)param_1 + 0x4c) = 0;
      *(undefined4 *)(param_1 + 10) = uVar1;
    }
  }
  if (local_8 - __stack_chk_guard == 0) {
    return;
  }
                    // WARNING: Subroutine does not return
}
*/

// --- 函数: set_screen_menu_on ---

void set_screen_menu_on(void)

{
  if (DAT_04031570 == NULL || DAT_04031578 == NULL) {
    fprintf(stderr, "ERROR: set_screen_menu_on called but window or renderer is NULL\n");
    fflush(stderr);
    return;
  }
  SDL_SetWindowSize(DAT_04031570,800,0x1e0);
  SDL_RenderSetLogicalSize(DAT_04031578,800,0x1e0);
  if (DAT_04031580 == 0) {
    DAT_04031580 = SDL_CreateTexture(DAT_04031578,0x15151002,1,800,0x1e0);
    if (DAT_04031580 == NULL) {
      fprintf(stderr, "ERROR: SDL_CreateTexture failed in set_screen_menu_on: %s\n", SDL_GetError());
      fflush(stderr);
      return;
    } else {
      fprintf(stderr, "DEBUG: SDL_CreateTexture succeeded in set_screen_menu_on\n");
      fflush(stderr);
    }
    clear_screen();
  }
  else {
    clear_screen();
  }
  if (DAT_04031598 != (void *)0x0) {
    fprintf(stderr, "DEBUG: set_screen_menu_on: DAT_04031598 already allocated at %p\n", DAT_04031598);
    fflush(stderr);
    DAT_040315d4 = 1;  // ._4_4_ removed
    return;
  }
  DAT_04031598 = malloc(0xbb800);
  if (DAT_04031598 == NULL) {
    fprintf(stderr, "ERROR: malloc failed for DAT_04031598 in set_screen_menu_on\n");
    fflush(stderr);
    return;
  }
  fprintf(stderr, "DEBUG: set_screen_menu_on: allocated DAT_04031598 at %p, size 0xbb800\n", DAT_04031598);
  fflush(stderr);
  DAT_040315d4 = 1;  // ._4_4_ removed
  // 确保DAT_040315a8设置正确（用于get_screen_pitch）
  if (DAT_040315a8 == 0) {
    DAT_040315a8 = 2;  // 默认值，对应800像素宽度
    fprintf(stderr, "DEBUG: set_screen_menu_on: setting DAT_040315a8 to 2\n");
    fflush(stderr);
  }
  // 使用clear_screen_menu清除整个屏幕（填充为0，即黑色）
  // 这样可以确保整个缓冲区都被正确初始化
  clear_screen_menu(0);
  fprintf(stderr, "DEBUG: set_screen_menu_on: cleared screen menu\n");
  fflush(stderr);
  return;
}

// --- 函数: config_update_settings ---

void config_update_settings(long param_1)
{
  uint uVar1;
  void *__s;
  ushort uVar2;
  ulong uVar3;
  ulong uVar4;
  ushort *puVar5;
  
  __s = (void *)(param_1 + 0xd70);
  set_screen_orientation(*(undefined4 *)(param_1 + 0x44c));
  puVar5 = (ushort *)(param_1 + 0xd1e);
  set_screen_swap(*(undefined4 *)(param_1 + 0x454));
  memset(__s,0,0x4000);
  uVar3 = 0;
  do {
    uVar2 = puVar5[-0x29];
    uVar4 = 1L << (uVar3 & 0x3f);
    uVar1 = (int)uVar3 + 1;
    uVar3 = (ulong)uVar1;
    if (uVar2 != 0xffff) {
      *(ulong *)((long)__s + (ulong)uVar2 * 8) = *(ulong *)((long)__s + (ulong)uVar2 * 8) | uVar4;
    }
    uVar2 = *puVar5;
    puVar5 = puVar5 + 1;
    if (uVar2 != 0xffff) {
      *(ulong *)((long)__s + (ulong)uVar2 * 8) = *(ulong *)((long)__s + (ulong)uVar2 * 8) | uVar4;
    }
  } while (puVar5 < (ushort *)(param_1 + 0xd1e + 0x200));
}

