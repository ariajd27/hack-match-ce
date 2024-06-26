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
#include "variables.h"
#include "drawing.h"

#include <debug.h>

unsigned char global_palette[32 + sizeof_grid_palette];
unsigned char deathStage;
bool deathFlashOn;
unsigned long nextDeathFlash;

struct gfx_sprite_t *fileSprites[14];
struct gfx_sprite_t *fileMatchSprites[6];
struct gfx_sprite_t *digitSprites[10];
struct gfx_sprite_t *deathSprites[3];

gfx_UninitedSprite(behindExa, exa_empty_width, exa_empty_height);
gfx_UninitedSprite(behindGameOver, play_again_width, play_again_height);

void drawExa()
{
	const unsigned char exaX = EXA_HOFFSET + GRID_SIZE * exaCol;
	if (deathStage == 0 && isHoldingFile)
	{
		if (heldFile & 0x10)
		{
			gfx_Sprite_NoClip(file_locked, exaX + EXA_HELD_HOFFSET, EXA_VOFFSET + EXA_HELD_VOFFSET);
		}
		else
		{
			gfx_Sprite_NoClip(fileSprites[heldFile], exaX + EXA_HELD_HOFFSET, EXA_VOFFSET + EXA_HELD_VOFFSET);
		}
	}

	if (deathStage > 0) gfx_TransparentSprite_NoClip(deathSprites[deathStage], exaX, EXA_VOFFSET);
	else if (isHoldingFile)
	{
		if (heldFile & 0x08) gfx_TransparentSprite_NoClip(exa_star, exaX, EXA_VOFFSET);
		else gfx_TransparentSprite_NoClip(exa_file, exaX, EXA_VOFFSET);
	}
	else gfx_TransparentSprite_NoClip(exa_empty, exaX, EXA_VOFFSET);
}

void drawCol(const unsigned char col)
{
	const unsigned int x = GRID_HOFFSET + col * GRID_SIZE;

	unsigned char y = GRID_VOFFSET - gridMoveOffset;
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		if (files[col][row] != FILE_EMPTY)
		{
			// draw a file
			if (files[col][row] & 0x80)
			{
				if (files[col][row] & 0x08) gfx_Sprite(star_match, x, y);
				else gfx_Sprite(fileMatchSprites[files[col][row] & 0x7f], x, y);
			}
			else if (files[col][row] & 0x10) gfx_Sprite(file_locked, x, y);
			else gfx_Sprite(fileSprites[files[col][row]], x, y);
		}
		else
		{
			gfx_SetColor(COLOR_BLACK);
			gfx_FillRectangle(x, y, GRID_SIZE, GRID_SIZE);

			if (col == exaCol)
			{
				// draw the dashed line up from the exa
				gfx_SetColor(COLOR_DASH);
				const unsigned char xx = x + DASH_HOFFSET;
				for (unsigned char yy = y + DASH_VOFFSET; yy < y + GRID_SIZE; yy += DASH_INTERVAL)
				{
					gfx_FillRectangle(xx, yy, DASH_WIDTH, DASH_HEIGHT);
				}
			}
		}
		y += GRID_SIZE;
	}

	// fill in below the grid for partially-lowered files
	gfx_SetColor(COLOR_BLACK);
	gfx_FillRectangle(x, y, GRID_SIZE, gridMoveOffset);

	if (col == exaCol)
	{
		gfx_SetColor(COLOR_DASH);
		const unsigned char xx = x + DASH_HOFFSET;
		for (unsigned char yy = y + DASH_VOFFSET; yy < y + gridMoveOffset; yy += DASH_INTERVAL)
		{
			gfx_FillRectangle_NoClip(xx, yy, DASH_WIDTH, DASH_HEIGHT);
		}

		drawExa(); // else it will have been drawn over a bit
	}
}

void clearifySprites(const unsigned char step)
{
	unsigned int y = GRID_VOFFSET - gridMoveOffset;
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		unsigned char x = GRID_HOFFSET;
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			if (files[col][row] & 0x80)
			{
				if (files[col][row] & 0x08)
				{
					if (step == 0) gfx_Sprite(star_erase_1, x, y);
					else gfx_Sprite(star_erase_2, x, y);
				}
				else
				{
					if (step == 0) gfx_Sprite(file_erase_1, x, y);
					else gfx_Sprite(file_erase_2, x, y);
				}
			}

			x += GRID_SIZE;
		}

		y += GRID_SIZE;
	}
}

void animateClear() // relies on to-clears being flagged with bit 7
{
	clock_t refundTimer = clock();

	// get everything in position for the animation
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		drawCol(col);
	}

	for (unsigned char step = 0; step < 2; step++)
	{
		clock_t animationTimer = clock();

		clearifySprites(step);

		while (clock() - animationTimer < CLEAR_ANIMATION_FRAME_TIME);

		gfx_BlitBuffer();
	}

	nextMoveTime += clock() - refundTimer;
}

void drawNumber(const unsigned int x, const unsigned char y, const unsigned int toDraw)
{
	bool significant = false;
	unsigned int remainder = toDraw;
	unsigned int digitX = x;
	for (unsigned int divisor = 100000; divisor >= 1; divisor /= 10)
	{
		const unsigned char thisDigit = remainder / divisor;
		remainder -= divisor * thisDigit;
		if (thisDigit > 0) significant = true;

		if (!significant) gfx_Sprite_NoClip(digit_0_grey, digitX, y);
		else gfx_Sprite_NoClip(digitSprites[thisDigit], digitX, y);

		digitX += 8;
	}
}

void drawFrame()
{
	// erase the old exa
	gfx_Sprite_NoClip(behindExa, EXA_HOFFSET + GRID_SIZE * exaCol, EXA_VOFFSET);

	// draw the new grid and exa
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		drawCol(col);
	}

	// draw the score
	drawNumber(SCORE_HOFFSET, SCORE_VOFFSET, score);

#ifndef NO_BUFFER
	gfx_BlitBuffer();
#endif
}

bool titleScreen()
{
	gfx_FillScreen(COLOR_BLACK);
	gfx_RLETSprite_NoClip(title, TITLE_SPRITE_HOFFSET, TITLE_SPRITE_VOFFSET);

	gfx_SetTextFGColor(COLOR_RED);
	gfx_SetTextXY(TITLE_TEXT_HOFFSET, TITLE_TEXT_VOFFSET);
	gfx_PrintString("Press any key...");

#ifndef NO_BUFFER
	gfx_BlitBuffer();
#endif

	// wait for no keys to be pressed
	while (kb_AnyKey());

	// wait for a key to be pressed
	while (true)
	{
		kb_Scan();
		if (kb_IsDown(kb_KeyClear)) return true;
		else if (kb_AnyKey()) return false;
	}
}

void sleep(unsigned long time)
{
	clock_t sleepTimer = clock();
	while (clock() - sleepTimer < time);
}

void darken()
{
	for (unsigned char i = 16; i < 32; i++) gfx_palette[i] = gfx_Darken(gfx_palette[i], DEATH_DARKEN_LEVEL);
}

void animateDeath()
{
	// do the animation
	for (deathStage = 0; deathStage < 3; deathStage++)
	{
		darken();
		drawFrame();
		sleep(DEATH_ANIMATION_FRAME_TIME / 8);

		for (unsigned char i = 0; i < 7; i++)
		{
			darken();
			sleep(DEATH_ANIMATION_FRAME_TIME / 8);
		}
	}

	// say "GAME OVER"
	behindGameOver->width = play_again_width;
	behindGameOver->height = play_again_height;
	gfx_GetSprite(behindGameOver, PLAY_AGAIN_HOFFSET, PLAY_AGAIN_VOFFSET);
	gfx_RLETSprite_NoClip(play_again, PLAY_AGAIN_HOFFSET, PLAY_AGAIN_VOFFSET);
	gfx_BlitBuffer();
	deathFlashOn = true;
	nextDeathFlash = clock() + GAME_OVER_FLASH_TIME;
}

void deathPeriodic()
{
	if (clock() < nextDeathFlash) return;

	if (deathFlashOn) gfx_Sprite_NoClip(behindGameOver, PLAY_AGAIN_HOFFSET, PLAY_AGAIN_VOFFSET);
	else gfx_RLETSprite_NoClip(play_again, PLAY_AGAIN_HOFFSET, PLAY_AGAIN_VOFFSET);

	gfx_BlitBuffer();

	deathFlashOn = !deathFlashOn;
	nextDeathFlash = clock() + GAME_OVER_FLASH_TIME;
}
