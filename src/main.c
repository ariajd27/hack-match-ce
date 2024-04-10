#include <sys/rtc.h>
#include <ti/getcsc.h>
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "gfx/gfx.h"

#define SAVE_VAR_NAME "HKMCHDAT"

#define FRAME_TIME 1600

#define COLOR_BLACK 0
#define COLOR_WHITE 1
#define COLOR_RED 2

#define TITLE_SPRITE_HOFFSET ((GFX_LCD_WIDTH - title_width) / 2)
#define TITLE_SPRITE_VOFFSET 36
#define TITLE_TEXT_HOFFSET 110
#define TITLE_TEXT_VOFFSET 180

#define NUM_COLS 6
#define MAX_ROWS 11
#define NUM_INITIAL_ROWS 3
#define NUM_BLOCK_COLORS 5

#define BG_HOFFSET ((GFX_LCD_WIDTH - background_width) / 2)
#define BG_VOFFSET ((GFX_LCD_HEIGHT - background_height) / 2)
#define GRID_HOFFSET (BG_HOFFSET + 48)
#define GRID_VOFFSET (BG_VOFFSET + 23)
#define GRID_SIZE 16

bool toExit;

enum file_t {
	EMPTY,
	RED,
	YELLOW,
	CYAN,
	BLUE,
	PURPLE,
	LOCKED
};

const struct gfx_sprite_t *fileSprites[] = {
	0,
	file_red,
	file_yellow,
	file_cyan,
	file_blue,
	file_purple,
	file_locked
};

unsigned char files[NUM_COLS][MAX_ROWS];

bool doInput()
{
	return os_GetCSC();
}

void drawFrame()
{
	// draw the grid
	gfx_SetColor(COLOR_BLACK); // to fill in empty cells
	unsigned char y = GRID_VOFFSET;
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		unsigned int x = GRID_HOFFSET;
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			if (files[col][row] == EMPTY) gfx_FillRectangle(x, y, GRID_SIZE, GRID_SIZE);
			else gfx_Sprite(fileSprites[files[col][row]], x, y);
			x += GRID_SIZE;
		}
		y += GRID_SIZE;
	}
	
	gfx_BlitBuffer();
}

void startGame()
{
	// draw the background
	gfx_FillScreen(COLOR_BLACK);
	gfx_Sprite(background, BG_HOFFSET, BG_VOFFSET);

	srand(rtc_Time());

	// populate the initial grid
	for (unsigned char row = 0; row < NUM_INITIAL_ROWS; row++)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			files[col][row] = rand() % NUM_BLOCK_COLORS + 1;
		}
	}

	// get the first screen out there
	drawFrame();
}

void endGame()
{
	toExit = true;
}

void init()
{
	toExit = false;
}

void titleScreen()
{
	gfx_FillScreen(COLOR_BLACK);
	gfx_Sprite(title, TITLE_SPRITE_HOFFSET, TITLE_SPRITE_VOFFSET);

	gfx_SetTextFGColor(COLOR_RED);
	gfx_SetTextXY(TITLE_TEXT_HOFFSET, TITLE_TEXT_VOFFSET);
	gfx_PrintString("Press any key...");

	gfx_BlitBuffer();

	while (!os_GetCSC());
}

int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetPalette(global_palette, sizeof_global_palette, 0);

	init();
	titleScreen();

	while (!toExit)
	{
		startGame();

		while (true)
		{
			clock_t frameTimer = clock();

			if (doInput()) break;

			drawFrame();

			while (clock() - frameTimer < FRAME_TIME);
		}

		endGame();
	}

	gfx_End();

	return 0;
}
