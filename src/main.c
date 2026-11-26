#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "hal/hal.h"

#define SIZE 5
#define PUZZLE_COUNT 3

/* =========================
 *   5x5 Sudoku Puzzles（合法且空格適中）
 * ========================= */

static int puzzles[PUZZLE_COUNT][SIZE][SIZE] = {

/* Puzzle 1 */
{
 {1, 0, 3, 4, 5},
 {0, 4, 0, 1, 2},
 {5, 1, 4, 0, 3},
 {4, 5, 2, 3, 0},
 {2, 0, 1, 5, 4}
},

/* Puzzle 2 */
{
 {0, 2, 3, 4, 0},
 {3, 4, 0, 1, 2},
 {5, 1, 4, 2, 3},
 {0, 5, 2, 3, 1},
 {2, 3, 1, 0, 4}
},

/* Puzzle 3 */
{
 {1, 2, 0, 4, 5},
 {3, 0, 5, 1, 2},
 {0, 1, 4, 2, 3},
 {4, 5, 2, 3, 1},
 {2, 3, 1, 5, 0}
}

};

/* =========================
 *   State
 * ========================= */

static int board[SIZE][SIZE];
static int fixed[SIZE][SIZE];
static lv_obj_t *cells[SIZE][SIZE];
static lv_obj_t *msg_label;

static int sel_r = -1, sel_c = -1;
static int current_puzzle = 0;
static int dark_mode = 0;
static int completed = 0;

/* =========================
 *   Logic
 * ========================= */

static int is_valid(int r, int c, int v) {
    for(int i = 0; i < SIZE; i++) {
        if(board[r][i] == v && i != c) return 0;
        if(board[i][c] == v && i != r) return 0;
    }
    return 1;
}

static int check_complete(void) {
    for(int i = 0; i < SIZE; i++)
        for(int j = 0; j < SIZE; j++) {
            if(board[i][j] == 0) return 0;
            if(!is_valid(i, j, board[i][j])) return 0;
        }
    return 1;
}

/* =========================
 *   UI Helpers
 * ========================= */

static void refresh_cell(int r, int c) {
    lv_obj_t *lbl = lv_obj_get_child(cells[r][c], 0);
    if(board[r][c] == 0)
        lv_label_set_text(lbl, "");
    else
        lv_label_set_text_fmt(lbl, "%d", board[r][c]);

    /* 標示選中格子 */
    if(r == sel_r && c == sel_c) {
        lv_obj_set_style_bg_color(cells[r][c], dark_mode ? lv_color_hex(0x005577) : lv_color_hex(0x99ccff), 0);
    } else {
        lv_obj_set_style_bg_color(cells[r][c], dark_mode ? lv_color_hex(0x333333) : lv_color_hex(0xffffff), 0);
    }

    /* 固定格子字體顏色不同 */
    if(fixed[r][c]) {
        lv_obj_set_style_text_color(cells[r][c], lv_color_hex(0xff6600), 0);
    } else {
        lv_obj_set_style_text_color(cells[r][c], dark_mode ? lv_color_hex(0xffffff) : lv_color_hex(0x000000), 0);
    }
}

static void apply_theme(void) {
    lv_color_t bg = dark_mode ? lv_color_hex(0x1e1e1e) : lv_color_hex(0xffffff);
    lv_color_t txt = dark_mode ? lv_color_hex(0xffffff) : lv_color_hex(0x000000);

    lv_obj_set_style_bg_color(lv_scr_act(), bg, 0);
    lv_obj_set_style_text_color(lv_scr_act(), txt, 0);

    /* 重新刷新所有格子顏色 */
    for(int i = 0; i < SIZE; i++)
        for(int j = 0; j < SIZE; j++)
            refresh_cell(i, j);

    /* 訊息顏色 */
    lv_obj_set_style_text_color(msg_label, txt, 0);
}

/* =========================
 *   Events
 * ========================= */

static void cell_cb(lv_event_t *e) {
    int id = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    sel_r = id / 10;
    sel_c = id % 10;
    refresh_cell(sel_r, sel_c);
}

static void num_cb(lv_event_t *e) {
    if(sel_r < 0 || fixed[sel_r][sel_c] || completed) return;

    int v = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    board[sel_r][sel_c] = v;
    refresh_cell(sel_r, sel_c);

    if(check_complete()) {
        lv_label_set_text(msg_label, "COMPLETED!");
        completed = 1;

        // 播放音效 (用 system() 呼叫 ffmpeg RTP 串流)
        system("ffmpeg -re -i 02.mp3 -acodec libmp3lame -ab 128k -f rtp rtp://10.5.128.227:1234 &");
    }
}

static void reset_puzzle(void) {
    for(int i = 0; i < SIZE; i++)
        for(int j = 0; j < SIZE; j++) {
            board[i][j] = puzzles[current_puzzle][i][j];
            fixed[i][j] = (board[i][j] != 0);
            refresh_cell(i, j);
        }
    lv_label_set_text(msg_label, "");
    sel_r = sel_c = -1;
    completed = 0;
}

static void new_game_cb(lv_event_t *e) {
    current_puzzle = (current_puzzle + 1) % PUZZLE_COUNT;
    reset_puzzle();
}

static void theme_cb(lv_event_t *e) {
    dark_mode = !dark_mode;
    apply_theme();
}

/* =========================
 *   UI Create
 * ========================= */

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();

    msg_label = lv_label_create(scr);
    lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, 250, 250);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    static lv_coord_t dsc[6] = {50,50,50,50,50, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, dsc, dsc);

    for(int i = 0; i < SIZE; i++)
        for(int j = 0; j < SIZE; j++) {
            lv_obj_t *b = lv_btn_create(grid);
            lv_obj_set_grid_cell(b,
                LV_GRID_ALIGN_STRETCH, j, 1,
                LV_GRID_ALIGN_STRETCH, i, 1);

            lv_obj_set_user_data(b, (void*)(intptr_t)(i * 10 + j));
            lv_obj_add_event_cb(b, cell_cb, LV_EVENT_CLICKED, NULL);

            lv_label_create(b);
            cells[i][j] = b;
        }

    for(int i = 1; i <= SIZE; i++) {
        lv_obj_t *b = lv_btn_create(scr);
        lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 20 + (i - 1) * 45, -60);
        lv_obj_set_user_data(b, (void*)(intptr_t)i);
        lv_obj_add_event_cb(b, num_cb, LV_EVENT_CLICKED, NULL);
        lv_label_set_text_fmt(lv_label_create(b), "%d", i);
    }

    lv_obj_t *btn_new = lv_btn_create(scr);
    lv_obj_align(btn_new, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_label_set_text(lv_label_create(btn_new), "New");
    lv_obj_add_event_cb(btn_new, new_game_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_theme = lv_btn_create(scr);
    lv_obj_align(btn_theme, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_label_set_text(lv_label_create(btn_theme), "Theme");
    lv_obj_add_event_cb(btn_theme, theme_cb, LV_EVENT_CLICKED, NULL);

    reset_puzzle();
    apply_theme();
}

/* =========================
 *   Main
 * ========================= */

int main(void) {
    lv_init();
    sdl_hal_init(320, 480);

    create_ui();

    while(1) {
        uint32_t t = lv_timer_handler();
        if(t == LV_NO_TIMER_READY) t = 10;
        usleep(t * 1000);
    }
}

