#ifndef variables_include_file
#define variables_include_file

#define SAVE_VAR_NAME "HKMCHDAT"

#define COLOR_BLACK 0
#define COLOR_WHITE 1
#define COLOR_RED 2
#define COLOR_DASH 17
#define COLOR_DOT 18
#define COLOR_METAL 3

#define TITLE_SPRITE_HOFFSET ((GFX_LCD_WIDTH - title_width) / 2)
#define TITLE_SPRITE_VOFFSET 36
#define TITLE_TEXT_HOFFSET 110
#define TITLE_TEXT_VOFFSET 180

#define NUM_COLS 6
#define MAX_ROWS 11
#define NUM_INITIAL_ROWS 3
#define NUM_BLOCK_COLORS 5
#define EXA_START_COL 2
#define AMOUNT_FILES_TO_MATCH 4
#define STAR_CHANCE 40 // <- the actual probability is the reciprocal of this number
#define BASE_BLOCK_VALUE 100
#define CHAIN_BLOCK_BONUS 50
#define MAX_NEW_ROW_INTERVAL 150000ul
#define MIN_NEW_ROW_INTERVAL 50000ul
#define ROW_INTERVAL_SCALE_START 0ul
#define ROW_INTERVAL_SCALE_END 50000ul
#define ROW_INTERVAL_SCALE_FACTOR (MAX_NEW_ROW_INTERVAL - MIN_NEW_ROW_INTERVAL) \
								/ (ROW_INTERVAL_SCALE_END - ROW_INTERVAL_SCALE_START)
#define MATCH_TIME 25000

#define FILE_EMPTY 0
#define FILE_RED 1
#define FILE_YELLOW 2
#define FILE_CYAN 3
#define FILE_BLUE 4
#define FILE_PURPLE 5
#define FILE_LOCKED 6

#define MOVE_ANIMATION_FRAME_TIME 300
#define CLEAR_ANIMATION_FRAME_TIME 1600
#define DEATH_ANIMATION_FRAME_TIME 16000
#define DEATH_DARKEN_LEVEL 232
#define GAME_OVER_FLASH_TIME 32000

#define BG_HOFFSET ((GFX_LCD_WIDTH - background_width) / 2)
#define BG_VOFFSET ((GFX_LCD_HEIGHT - background_height) / 2)
#define GRID_HOFFSET (BG_HOFFSET + 48)
#define GRID_VOFFSET (BG_VOFFSET + 23)
#define GRID_SIZE 16
#define GRID_MOVE_STEPS 4
#define CLIP_VOFFSET (BG_VOFFSET + 22)
#define EXA_HOFFSET (BG_HOFFSET + 44)
#define EXA_VOFFSET (BG_VOFFSET + 193)
#define EXA_HELD_HOFFSET 4
#define EXA_HELD_VOFFSET 6
#define DASH_HOFFSET 8
#define DASH_VOFFSET 3
#define DASH_INTERVAL 4
#define DASH_WIDTH 1
#define DASH_HEIGHT 1
#define SCORE_HOFFSET (BG_HOFFSET + 160)
#define SCORE_VOFFSET (BG_VOFFSET + 41)
#define HIGH_SCORE_HOFFSET (BG_HOFFSET + 160)
#define HIGH_SCORE_VOFFSET (BG_VOFFSET + 97)
#define NUM_DISPLAY_DIGITS 6
#define PLAY_AGAIN_HOFFSET (2 + GRID_HOFFSET + (NUM_COLS * GRID_SIZE - play_again_width) / 2)
#define PLAY_AGAIN_VOFFSET (GRID_VOFFSET + 42)
#define DOTS_HOFFSET (BG_HOFFSET + 48)
#define DOTS_VOFFSET 202
#define NUM_DOTS 49
#define BLK_RECT_1_X (BG_HOFFSET + 46)
#define BLK_RECT_1_Y (BG_VOFFSET + 22)
#define BLK_RECT_1_W 100
#define BLK_RECT_1_H 196
#define BLK_RECT_2_X (BG_HOFFSET + 149)
#define BLK_RECT_2_Y (BG_VOFFSET + 22)
#define BLK_RECT_2_W 61
#define BLK_RECT_2_H 35
#define BLK_RECT_3_X (BG_HOFFSET + 150)
#define BLK_RECT_3_Y (BG_VOFFSET + 78)
#define BLK_RECT_3_W 60
#define BLK_RECT_3_H 34

extern bool toExit;
extern bool gameOver;

extern unsigned int score;
extern unsigned int highScore;

extern unsigned char files[NUM_COLS][MAX_ROWS];
extern unsigned char exaCol;
extern bool isHoldingFile;
extern unsigned char heldFile;
extern unsigned char gridMoveOffset;

extern clock_t nextMoveTime;

extern unsigned char prevRight, prevLeft, prev2nd, prevAlpha;

#endif
