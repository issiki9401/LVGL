/**
 * @file pc_monitor_main.c
 * @brief 電腦狀態監測面板獨立進入點 (PC Status Monitor Panel)
 */

#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE /* needed for usleep() */
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif
#include "lvgl/lvgl.h"
#include "project/pc_monitor/lv_example_pc_monitor.h"
#include <SDL.h>

#include "hal/hal.h"

/* 若設定 LVGL_AUTO_SCREENSHOT，將目前 SDL 視窗畫面儲存為 BMP 圖檔供驗證用 */
static void save_sdl_screenshot(lv_display_t * disp, const char * filename)
{
  SDL_Renderer * renderer = (SDL_Renderer *)lv_sdl_window_get_renderer(disp);
  if(renderer == NULL) return;

  int w = 280;
  int h = 420;
  SDL_GetRendererOutputSize(renderer, &w, &h);

  SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
  if(surface == NULL) return;

  if(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch) == 0) {
    SDL_SaveBMP(surface, filename);
  }
  SDL_FreeSurface(surface);
}

#if LV_USE_OS != LV_OS_FREERTOS

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /* 初始化 LVGL */
  lv_init();

  /* 初始化 SDL 顯示與輸入裝置 (280 x 420 直立螢幕) */
  lv_display_t * disp = sdl_hal_init(280, 420);

  /* 啟動電腦狀態監測面板 */
  lv_example_pc_monitor_1();

  int frame_count = 0;
  const char * auto_exit = getenv("LVGL_AUTO_SCREENSHOT");

  while(1) {
    uint32_t sleep_time_ms = lv_timer_handler();
    if(sleep_time_ms == LV_NO_TIMER_READY) {
      sleep_time_ms = LV_DEF_REFR_PERIOD;
    }

    if(auto_exit != NULL) {
      frame_count++;
      if(frame_count == 15) {
        lv_refr_now(disp);
        save_sdl_screenshot(disp, "pc_monitor_verify.bmp");
        break;
      }
    }

#ifdef _MSC_VER
    Sleep(sleep_time_ms);
#else
    usleep(sleep_time_ms * 1000);
#endif
  }

  return 0;
}

#endif
