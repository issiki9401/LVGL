/**
 * @file lv_example_pc_monitor_1.c
 * @brief 電腦狀態監測面板 (PC Status Monitor Panel)
 *        班級：資工二甲 | 姓名：曾俊睿 | 學號：11411133
 *
 * 結合 LVGL get_started 與 widgets 範例功能：
 * 1. lv_example_get_started_1: 基礎標籤 (lv_label) 與主畫面背景設定
 * 2. lv_example_get_started_2: 互動按鈕 (lv_button) 與點擊事件回呼 (LV_EVENT_CLICKED)
 * 3. lv_example_get_started_3: 自訂樣式 (lv_style_t)、圓角卡片與漸層背景設計
 * 4. lv_example_get_started_4: 滑桿控制 (lv_slider) 與數值連動更新 (LV_EVENT_VALUE_CHANGED)
 * 5. lv_example_freetype_1   : FreeType 繁體中文字型渲染
 * 6. lv_arc / lv_bar / timer : 雙圓弧動態儀表板、硬體資源進度條與定時器自動刷新
 */

#include "lvgl/lvgl.h"
#include "lv_example_pc_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


/* 全域或靜態 UI 物件指標，供定時器與事件回呼更新數值 */
static lv_obj_t * time_label;
static lv_obj_t * date_label;
static lv_obj_t * weather_label;
static lv_obj_t * temp_hum_label;

static lv_obj_t * cpu_arc;
static lv_obj_t * cpu_val_label;
static lv_obj_t * cpu_temp_label;

static lv_obj_t * gpu_arc;
static lv_obj_t * gpu_val_label;
static lv_obj_t * gpu_temp_label;

static lv_obj_t * ram_bar;
static lv_obj_t * ram_val_label;
static lv_obj_t * ssd_bar;
static lv_obj_t * ssd_val_label;
static lv_obj_t * net_label;

static lv_obj_t * fan_slider;
static lv_obj_t * fan_label;
static lv_obj_t * mode_btn;
static lv_obj_t * mode_btn_label;

/* 自訂樣式變數 (參考 lv_example_get_started_3.c) */
static lv_style_t style_card;
static lv_style_t style_btn_mode;
static lv_style_t style_btn_refresh;
static lv_style_t style_btn_pressed;
static lv_style_t style_cn_12;
static lv_style_t style_cn_14;

/* 系統狀態與模式變數 */
static int current_mode = 0; /* 0: 平衡模式, 1: 極速電競, 2: 靜音節能 */
static int32_t sim_cpu_usage = 42;
static int32_t sim_gpu_usage = 65;
static int32_t sim_cpu_temp  = 53;
static int32_t sim_gpu_temp  = 61;
static int32_t sim_ram_pct   = 76;
static int32_t sim_ssd_pct   = 72;
static int32_t tick_counter  = 0;

/* 尋找可用之繁體中文字型路徑 */
static const char * find_font_path(void)
{
    static const char * paths[] = {
        "./project/pc_monitor/NotoSerifCJK-Regular.ttc",
        "../project/pc_monitor/NotoSerifCJK-Regular.ttc",
        "/home/sshuser/LVGL/project/pc_monitor/NotoSerifCJK-Regular.ttc"
    };
    for(size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        if(access(paths[i], R_OK) == 0) {
            return paths[i];
        }
    }
    return paths[0];
}

/* 更新畫面上的硬體監測數值與顏色 */
static void update_monitor_ui(void)
{
    /* 更新 CPU 圓弧與文字 */
    lv_arc_set_value(cpu_arc, sim_cpu_usage);
    lv_label_set_text_fmt(cpu_val_label, "%d%%", (int)sim_cpu_usage);
    int cpu_freq_ghz_x10 = (current_mode == 1) ? 48 : (current_mode == 2 ? 32 : 42);
    lv_label_set_text_fmt(cpu_temp_label, "%dC | %d.%dGHz",
                          (int)sim_cpu_temp, cpu_freq_ghz_x10 / 10, cpu_freq_ghz_x10 % 10);

    if(sim_cpu_usage >= 80) {
        lv_obj_set_style_arc_color(cpu_arc, lv_color_hex(0xFF4D6D), LV_PART_INDICATOR);
    } else if(sim_cpu_usage >= 60) {
        lv_obj_set_style_arc_color(cpu_arc, lv_color_hex(0xFFD166), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_arc_color(cpu_arc, lv_color_hex(0x00BBF9), LV_PART_INDICATOR);
    }

    /* 更新 GPU 圓弧與文字 */
    lv_arc_set_value(gpu_arc, sim_gpu_usage);
    lv_label_set_text_fmt(gpu_val_label, "%d%%", (int)sim_gpu_usage);
    int gpu_freq_ghz_x10 = (current_mode == 1) ? 24 : (current_mode == 2 ? 14 : 19);
    lv_label_set_text_fmt(gpu_temp_label, "%dC | %d.%dGHz",
                          (int)sim_gpu_temp, gpu_freq_ghz_x10 / 10, gpu_freq_ghz_x10 % 10);

    if(sim_gpu_usage >= 80) {
        lv_obj_set_style_arc_color(gpu_arc, lv_color_hex(0xFF4D6D), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_arc_color(gpu_arc, lv_color_hex(0x9B5DE5), LV_PART_INDICATOR);
    }

    /* 更新 RAM 與 SSD 進度條 */
    lv_bar_set_value(ram_bar, sim_ram_pct, LV_ANIM_ON);
    int ram_used_x10 = (sim_ram_pct * 160) / 100;
    lv_label_set_text_fmt(ram_val_label, "%d.%d/16GB (%d%%)",
                          ram_used_x10 / 10, ram_used_x10 % 10, (int)sim_ram_pct);

    lv_bar_set_value(ssd_bar, sim_ssd_pct, LV_ANIM_ON);
    int ssd_used = (sim_ssd_pct * 512) / 100;
    lv_label_set_text_fmt(ssd_val_label, "%d/512GB (%d%%)", ssd_used, (int)sim_ssd_pct);

    /* 更新網路上下行速率 */
    int dl_x10 = 124 + (tick_counter * 17) % 65;
    int ul_x10 = 32 + (tick_counter * 9) % 24;
    lv_label_set_text_fmt(net_label, "網路速率  ↓ %d.%d MB/s   ↑ %d.%d MB/s",
                          dl_x10 / 10, dl_x10 % 10, ul_x10 / 10, ul_x10 % 10);
}

/* 滑桿事件回呼函式 (參考 lv_example_get_started_4.c) */
static void fan_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target_obj(e);
    int32_t pct = lv_slider_get_value(slider);
    int32_t rpm = 800 + (pct * 22);
    lv_label_set_text_fmt(fan_label, "風扇轉速控制: %d%% (%d RPM)", (int)pct, (int)rpm);
}

/* 模式切換按鈕事件回呼函式 (參考 lv_example_get_started_2.c) */
static void mode_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        current_mode = (current_mode + 1) % 3;
        if(current_mode == 0) {
            lv_label_set_text(mode_btn_label, "模式: 平衡模式");
            lv_obj_set_style_bg_color(mode_btn, lv_color_hex(0x0077B6), 0);
            lv_slider_set_value(fan_slider, 65, LV_ANIM_ON);
            sim_cpu_usage = 42; sim_gpu_usage = 65;
            sim_cpu_temp  = 53; sim_gpu_temp  = 61;
        } else if(current_mode == 1) {
            lv_label_set_text(mode_btn_label, "模式: 極速電競");
            lv_obj_set_style_bg_color(mode_btn, lv_color_hex(0xE63946), 0);
            lv_slider_set_value(fan_slider, 95, LV_ANIM_ON);
            sim_cpu_usage = 86; sim_gpu_usage = 92;
            sim_cpu_temp  = 74; sim_gpu_temp  = 79;
        } else {
            lv_label_set_text(mode_btn_label, "模式: 靜音節能");
            lv_obj_set_style_bg_color(mode_btn, lv_color_hex(0x2A9D8F), 0);
            lv_slider_set_value(fan_slider, 35, LV_ANIM_ON);
            sim_cpu_usage = 18; sim_gpu_usage = 22;
            sim_cpu_temp  = 41; sim_gpu_temp  = 44;
        }
        /* 同步更新滑桿標籤與監測儀表 */
        int32_t pct = lv_slider_get_value(fan_slider);
        lv_label_set_text_fmt(fan_label, "風扇轉速控制: %d%% (%d RPM)", (int)pct, (int)(800 + pct * 22));
        update_monitor_ui();
    }
}

/* 立即刷新按鈕事件回呼函式 */
static void refresh_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        tick_counter++;
        int delta = (tick_counter % 5) - 2;
        sim_cpu_usage = LV_CLAMP(15, sim_cpu_usage + delta * 3, 98);
        sim_gpu_usage = LV_CLAMP(20, sim_gpu_usage - delta * 2, 99);
        sim_cpu_temp  = 38 + (sim_cpu_usage * 42) / 100;
        sim_gpu_temp  = 40 + (sim_gpu_usage * 45) / 100;
        update_monitor_ui();
    }
}

/* 定時器回呼：每秒更新時鐘與微調系統負載數據 */
static void monitor_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    tick_counter++;

    /* 取得系統時間並更新時鐘 */
    time_t now = time(NULL);
    struct tm * t = localtime(&now);
    if(t != NULL) {
        lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        static const char * wday_cn[] = {"週日", "週一", "週二", "週三", "週四", "週五", "週六"};
        lv_label_set_text_fmt(date_label, "%04d/%02d/%02d %s",
                              t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                              wday_cn[t->tm_wday % 7]);
    }

    /* 產生平滑的即時波動效果 */
    int wave = (tick_counter % 3 == 0) ? 2 : ((tick_counter % 3 == 1) ? -1 : 1);
    sim_cpu_usage = LV_CLAMP(10, sim_cpu_usage + wave, 99);
    sim_gpu_usage = LV_CLAMP(15, sim_gpu_usage - wave, 99);
    sim_cpu_temp  = 38 + (sim_cpu_usage * 40) / 100;
    sim_gpu_temp  = 42 + (sim_gpu_usage * 42) / 100;
    update_monitor_ui();
}

/* 初始化自訂樣式 (參考 lv_example_get_started_3.c) */
static void init_styles(void)
{
    /* 卡片容器樣式 */
    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, 10);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_bg_color(&style_card, lv_color_hex(0x16213E));
    lv_style_set_bg_grad_color(&style_card, lv_color_hex(0x1A1A2E));
    lv_style_set_bg_grad_dir(&style_card, LV_GRAD_DIR_VER);
    lv_style_set_border_color(&style_card, lv_color_hex(0x2E4374));
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_pad_all(&style_card, 0);

    /* 模式按鈕樣式 */
    lv_style_init(&style_btn_mode);
    lv_style_set_radius(&style_btn_mode, 8);
    lv_style_set_bg_opa(&style_btn_mode, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_mode, lv_color_hex(0x0077B6));
    lv_style_set_border_color(&style_btn_mode, lv_color_hex(0x48CAE4));
    lv_style_set_border_width(&style_btn_mode, 1);
    lv_style_set_text_color(&style_btn_mode, lv_color_hex(0xFFFFFF));

    /* 刷新按鈕樣式 */
    lv_style_init(&style_btn_refresh);
    lv_style_set_radius(&style_btn_refresh, 8);
    lv_style_set_bg_opa(&style_btn_refresh, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_refresh, lv_color_hex(0x5A189A));
    lv_style_set_border_color(&style_btn_refresh, lv_color_hex(0x9D4EDD));
    lv_style_set_border_width(&style_btn_refresh, 1);
    lv_style_set_text_color(&style_btn_refresh, lv_color_hex(0xFFFFFF));

    /* 按鈕按壓回饋樣式 */
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x00F5D4));
    lv_style_set_text_color(&style_btn_pressed, lv_color_hex(0x0B132B));

    /* 載入 FreeType 繁體中文字型 (參考 lv_example_freetype_1.c) */
    lv_style_init(&style_cn_12);
    lv_style_init(&style_cn_14);
#if LV_USE_FREETYPE
    const char * font_path = find_font_path();
    lv_font_t * font_12 = lv_freetype_font_create(font_path,
                                                  LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                  13,
                                                  LV_FREETYPE_FONT_STYLE_NORMAL);
    lv_font_t * font_14 = lv_freetype_font_create(font_path,
                                                  LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                  15,
                                                  LV_FREETYPE_FONT_STYLE_NORMAL);
    if(font_12) lv_style_set_text_font(&style_cn_12, font_12);
    if(font_14) lv_style_set_text_font(&style_cn_14, font_14);
#endif
}

/**
 * @brief 建立電腦狀態監測面板主畫面 (解析度 280 x 420)
 */
void lv_example_pc_monitor_1(void)
{
    init_styles();

    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B132B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* =====================================================================
     * 0. 頂部標題列 (x=0, y=3, 顯示題目與姓名學號)
     * ===================================================================== */
    lv_obj_t * header_title = lv_label_create(scr);
    lv_obj_add_style(header_title, &style_cn_12, 0);
    lv_label_set_text(header_title, "電腦狀態監測面板 | 資工二甲 11411133 曾俊睿");
    lv_obj_set_style_text_color(header_title, lv_color_hex(0x64DFDF), 0);
    lv_obj_align(header_title, LV_ALIGN_TOP_MID, 0, 4);

    /* =====================================================================
     * 1. 時鐘與天氣預報卡片 (x=10, y=24, w=260, h=64)
     * ===================================================================== */
    lv_obj_t * card_top = lv_obj_create(scr);
    lv_obj_remove_style_all(card_top);
    lv_obj_add_style(card_top, &style_card, 0);
    lv_obj_set_pos(card_top, 10, 24);
    lv_obj_set_size(card_top, 260, 64);

    time_label = lv_label_create(card_top);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00F5D4), 0);
    lv_label_set_text(time_label, "19:08:30");
    lv_obj_set_pos(time_label, 10, 6);

    date_label = lv_label_create(card_top);
    lv_obj_add_style(date_label, &style_cn_12, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xA9BCD0), 0);
    lv_label_set_text(date_label, "2026/11/12 週四");
    lv_obj_set_pos(date_label, 10, 38);

    weather_label = lv_label_create(card_top);
    lv_obj_add_style(weather_label, &style_cn_14, 0);
    lv_obj_set_style_text_color(weather_label, lv_color_hex(0xFFD166), 0);
    lv_label_set_text(weather_label, "台東市 晴時多雲");
    lv_obj_align(weather_label, LV_ALIGN_TOP_RIGHT, -10, 8);

    temp_hum_label = lv_label_create(card_top);
    lv_obj_add_style(temp_hum_label, &style_cn_12, 0);
    lv_obj_set_style_text_color(temp_hum_label, lv_color_hex(0xE0FBFC), 0);
    lv_label_set_text(temp_hum_label, "氣溫 25°C | 濕度 64%");
    lv_obj_align(temp_hum_label, LV_ALIGN_TOP_RIGHT, -10, 38);

    /* =====================================================================
     * 2. CPU 與 GPU 雙圓弧監測卡片 (y=93, h=120)
     * ===================================================================== */
    /* 左卡: CPU */
    lv_obj_t * card_cpu = lv_obj_create(scr);
    lv_obj_remove_style_all(card_cpu);
    lv_obj_add_style(card_cpu, &style_card, 0);
    lv_obj_set_pos(card_cpu, 10, 93);
    lv_obj_set_size(card_cpu, 126, 120);

    lv_obj_t * cpu_title = lv_label_create(card_cpu);
    lv_obj_add_style(cpu_title, &style_cn_14, 0);
    lv_obj_set_style_text_color(cpu_title, lv_color_hex(0x90E0EF), 0);
    lv_label_set_text(cpu_title, "CPU 處理器");
    lv_obj_align(cpu_title, LV_ALIGN_TOP_MID, 0, 4);

    cpu_arc = lv_arc_create(card_cpu);
    lv_obj_set_size(cpu_arc, 72, 72);
    lv_arc_set_rotation(cpu_arc, 135);
    lv_arc_set_bg_angles(cpu_arc, 0, 270);
    lv_arc_set_range(cpu_arc, 0, 100);
    lv_arc_set_value(cpu_arc, sim_cpu_usage);
    lv_obj_set_style_arc_width(cpu_arc, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(cpu_arc, lv_color_hex(0x2B3A55), LV_PART_MAIN);
    lv_obj_set_style_arc_width(cpu_arc, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(cpu_arc, lv_color_hex(0x00BBF9), LV_PART_INDICATOR);
    lv_obj_remove_style(cpu_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(cpu_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(cpu_arc, LV_ALIGN_CENTER, 0, 2);

    cpu_val_label = lv_label_create(card_cpu);
    lv_obj_set_style_text_font(cpu_val_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(cpu_val_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(cpu_val_label, LV_ALIGN_CENTER, 0, 2);

    cpu_temp_label = lv_label_create(card_cpu);
    lv_obj_set_style_text_font(cpu_temp_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(cpu_temp_label, lv_color_hex(0x00F5D4), 0);
    lv_obj_align(cpu_temp_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    /* 右卡: GPU */
    lv_obj_t * card_gpu = lv_obj_create(scr);
    lv_obj_remove_style_all(card_gpu);
    lv_obj_add_style(card_gpu, &style_card, 0);
    lv_obj_set_pos(card_gpu, 144, 93);
    lv_obj_set_size(card_gpu, 126, 120);

    lv_obj_t * gpu_title = lv_label_create(card_gpu);
    lv_obj_add_style(gpu_title, &style_cn_14, 0);
    lv_obj_set_style_text_color(gpu_title, lv_color_hex(0xE0AAFF), 0);
    lv_label_set_text(gpu_title, "GPU 繪圖核心");
    lv_obj_align(gpu_title, LV_ALIGN_TOP_MID, 0, 4);

    gpu_arc = lv_arc_create(card_gpu);
    lv_obj_set_size(gpu_arc, 72, 72);
    lv_arc_set_rotation(gpu_arc, 135);
    lv_arc_set_bg_angles(gpu_arc, 0, 270);
    lv_arc_set_range(gpu_arc, 0, 100);
    lv_arc_set_value(gpu_arc, sim_gpu_usage);
    lv_obj_set_style_arc_width(gpu_arc, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(gpu_arc, lv_color_hex(0x2B3A55), LV_PART_MAIN);
    lv_obj_set_style_arc_width(gpu_arc, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(gpu_arc, lv_color_hex(0x9B5DE5), LV_PART_INDICATOR);
    lv_obj_remove_style(gpu_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(gpu_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(gpu_arc, LV_ALIGN_CENTER, 0, 2);

    gpu_val_label = lv_label_create(card_gpu);
    lv_obj_set_style_text_font(gpu_val_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(gpu_val_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(gpu_val_label, LV_ALIGN_CENTER, 0, 2);

    gpu_temp_label = lv_label_create(card_gpu);
    lv_obj_set_style_text_font(gpu_temp_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(gpu_temp_label, lv_color_hex(0xF15BB5), 0);
    lv_obj_align(gpu_temp_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    /* =====================================================================
     * 3. 記憶體、硬碟與網路速率卡片 (x=10, y=218, w=260, h=92)
     * ===================================================================== */
    lv_obj_t * card_mem = lv_obj_create(scr);
    lv_obj_remove_style_all(card_mem);
    lv_obj_add_style(card_mem, &style_card, 0);
    lv_obj_set_pos(card_mem, 10, 218);
    lv_obj_set_size(card_mem, 260, 92);

    /* RAM 標籤與進度條 */
    lv_obj_t * ram_title = lv_label_create(card_mem);
    lv_obj_add_style(ram_title, &style_cn_12, 0);
    lv_obj_set_style_text_color(ram_title, lv_color_hex(0xE0FBFC), 0);
    lv_label_set_text(ram_title, "記憶體 RAM");
    lv_obj_set_pos(ram_title, 10, 5);

    ram_val_label = lv_label_create(card_mem);
    lv_obj_set_style_text_font(ram_val_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ram_val_label, lv_color_hex(0x00F5D4), 0);
    lv_obj_align(ram_val_label, LV_ALIGN_TOP_RIGHT, -10, 7);

    ram_bar = lv_bar_create(card_mem);
    lv_obj_set_pos(ram_bar, 10, 24);
    lv_obj_set_size(ram_bar, 240, 8);
    lv_bar_set_range(ram_bar, 0, 100);
    lv_obj_set_style_bg_color(ram_bar, lv_color_hex(0x2B3A55), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ram_bar, lv_color_hex(0x00F5D4), LV_PART_INDICATOR);

    /* SSD 標籤與進度條 */
    lv_obj_t * ssd_title = lv_label_create(card_mem);
    lv_obj_add_style(ssd_title, &style_cn_12, 0);
    lv_obj_set_style_text_color(ssd_title, lv_color_hex(0xE0FBFC), 0);
    lv_label_set_text(ssd_title, "系統碟 SSD");
    lv_obj_set_pos(ssd_title, 10, 36);

    ssd_val_label = lv_label_create(card_mem);
    lv_obj_set_style_text_font(ssd_val_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ssd_val_label, lv_color_hex(0xFFD166), 0);
    lv_obj_align(ssd_val_label, LV_ALIGN_TOP_RIGHT, -10, 38);

    ssd_bar = lv_bar_create(card_mem);
    lv_obj_set_pos(ssd_bar, 10, 55);
    lv_obj_set_size(ssd_bar, 240, 8);
    lv_bar_set_range(ssd_bar, 0, 100);
    lv_obj_set_style_bg_color(ssd_bar, lv_color_hex(0x2B3A55), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ssd_bar, lv_color_hex(0xFFD166), LV_PART_INDICATOR);

    /* 網路流量標籤 */
    net_label = lv_label_create(card_mem);
    lv_obj_add_style(net_label, &style_cn_12, 0);
    lv_obj_set_style_text_color(net_label, lv_color_hex(0x80ED99), 0);
    lv_obj_set_pos(net_label, 10, 68);

    /* =====================================================================
     * 4. 風扇滑桿與模式切換互動卡片 (x=10, y=315, w=260, h=98)
     * ===================================================================== */
    lv_obj_t * card_ctrl = lv_obj_create(scr);
    lv_obj_remove_style_all(card_ctrl);
    lv_obj_add_style(card_ctrl, &style_card, 0);
    lv_obj_set_pos(card_ctrl, 10, 315);
    lv_obj_set_size(card_ctrl, 260, 98);

    fan_label = lv_label_create(card_ctrl);
    lv_obj_add_style(fan_label, &style_cn_12, 0);
    lv_obj_set_style_text_color(fan_label, lv_color_hex(0xE0FBFC), 0);
    lv_label_set_text(fan_label, "風扇轉速控制: 65% (2230 RPM)");
    lv_obj_set_pos(fan_label, 12, 5);

    fan_slider = lv_slider_create(card_ctrl);
    lv_obj_set_pos(fan_slider, 14, 28);
    lv_obj_set_size(fan_slider, 232, 8);
    lv_slider_set_range(fan_slider, 20, 100);
    lv_slider_set_value(fan_slider, 65, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(fan_slider, lv_color_hex(0x2B3A55), LV_PART_MAIN);
    lv_obj_set_style_bg_color(fan_slider, lv_color_hex(0x48CAE4), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(fan_slider, lv_color_hex(0x00F5D4), LV_PART_KNOB);
    lv_obj_set_style_pad_all(fan_slider, 3, LV_PART_KNOB);
    lv_obj_add_event_cb(fan_slider, fan_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 模式切換按鈕 (參考 lv_example_get_started_2.c & 3.c) */
    mode_btn = lv_button_create(card_ctrl);
    lv_obj_remove_style_all(mode_btn);
    lv_obj_add_style(mode_btn, &style_btn_mode, 0);
    lv_obj_add_style(mode_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_pos(mode_btn, 10, 46);
    lv_obj_set_size(mode_btn, 146, 42);
    lv_obj_add_event_cb(mode_btn, mode_btn_event_cb, LV_EVENT_CLICKED, NULL);

    mode_btn_label = lv_label_create(mode_btn);
    lv_obj_add_style(mode_btn_label, &style_cn_14, 0);
    lv_label_set_text(mode_btn_label, "模式: 平衡模式");
    lv_obj_center(mode_btn_label);

    /* 立即刷新按鈕 */
    lv_obj_t * refresh_btn = lv_button_create(card_ctrl);
    lv_obj_remove_style_all(refresh_btn);
    lv_obj_add_style(refresh_btn, &style_btn_refresh, 0);
    lv_obj_add_style(refresh_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_pos(refresh_btn, 164, 46);
    lv_obj_set_size(refresh_btn, 86, 42);
    lv_obj_add_event_cb(refresh_btn, refresh_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * refresh_label = lv_label_create(refresh_btn);
    lv_obj_add_style(refresh_label, &style_cn_14, 0);
    lv_label_set_text(refresh_label, "立即刷新");
    lv_obj_center(refresh_label);

    /* 初始更新一次數值並啟動 1000ms 定時器 */
    update_monitor_ui();
    monitor_timer_cb(NULL);
    lv_timer_create(monitor_timer_cb, 1000, NULL);
}

void lv_example_pc_monitor_next_mode(void)
{
    if(mode_btn != NULL) {
        lv_obj_send_event(mode_btn, LV_EVENT_CLICKED, NULL);
    }
}

