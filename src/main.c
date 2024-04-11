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

// uncomment this line out to draw directly to the screen
// #define NO_BUFFER

#define SAVE_VAR_NAME "HKMCHDAT"

#define SIMUL_FRAME_TIME 3200
#define ANIM_FRAME_TIME 800 // TODO

#define COLOR_BLACK 0
#define COLOR_WHITE 1
#define COLOR_RED 2
#define COLOR_DASH 3

#define TITLE_SPRITE_HOFFSET ((GFX_LCD_WIDTH - title_width) / 2)
#define TITLE_SPRITE_VOFFSET 36
#define TITLE_TEXT_HOFFSET 110
#define TITLE_TEXT_VOFFSET 180

#define NUM_COLS 6
#define MAX_ROWS 11
#define NUM_INITIAL_ROWS 3
#define NEW_ROW_INTERVAL 64
#define NUM_BLOCK_COLORS 5
#define EXA_START_COL 2
#define AMOUNT_TO_MATCH 4
#define BASE_BLOCK_VALUE 100
#define CHAIN_BLOCK_VALUE 200

#define BG_HOFFSET ((GFX_LCD_WIDTH - background_width) / 2)
#define BG_VOFFSET ((GFX_LCD_HEIGHT - background_height) / 2)

#define GRID_HOFFSET (BG_HOFFSET + 48)
#define GRID_VOFFSET (BG_VOFFSET + 23)
#define GRID_SIZE 16

#define EXA_HOFFSET (BG_HOFFSET + 44)
#define EXA_VOFFSET (BG_VOFFSET + 193)
#define EXA_HELD_HOFFSET 4
#define EXA_HELD_VOFFSET 6
#define DASH_HOFFSET 8
#define DASH_VOFFSET 3
#define DASH_INTERVAL 4

#define SCORE_HOFFSET (BG_HOFFSET + 160)
#define SCORE_VOFFSET (BG_VOFFSET + 41)
#define HIGH_SCORE_HOFFSET (BG_HOFFSET + 160)
#define HIGH_SCORE_VOFFSET (BG_VOFFSET + 97)
#define NUM_DISPLAY_DIGITS 6

#define FILE_EMPTY 0
#define FILE_RED 1
#define FILE_YELLOW 2
#define FILE_CYAN 3
#define FILE_BLUE 4
#define FILE_PURPLE 5
#define FILE_LOCKED 6

bool toExit;
bool gameOver;
unsigned char updateTracker;

unsigned int score;
unsigned int highScore;

unsigned char files[NUM_COLS][MAX_ROWS];
unsigned char exaCol;
bool isHoldingFile;
unsigned char heldFile;

const struct gfx_sprite_t *fileSprites[] = {
	0,
	file_red,
	file_yellow,
	file_cyan,
	file_blue,
	file_purple,
	file_locked
};

const struct gfx_sprite_t *digitSprites[] = {
	digit_0,
	digit_1,
	digit_2,
	digit_3,
	digit_4,
	digit_5,
	digit_6,
	digit_7,
	digit_8,
	digit_9
};

gfx_UninitedSprite(behindExa, exa_empty_width, exa_empty_height);

unsigned char prev2nd, prevAlpha;

void drawExa()
{
	const unsigned char exaX = EXA_HOFFSET + GRID_SIZE * exaCol;

	if (isHoldingFile)
	{
		gfx_Sprite_NoClip(fileSprites[heldFile], exaX + EXA_HELD_HOFFSET, EXA_VOFFSET + EXA_HELD_VOFFSET);
		gfx_RLETSprite_NoClip(exa_full, exaX, EXA_VOFFSET);
	}
	else
	{
		gfx_RLETSprite_NoClip(exa_empty, exaX, EXA_VOFFSET);
	}
}

bool getTargetedFile(unsigned char **output) // each of these returns false on failure
{
	for (unsigned char row = MAX_ROWS - 1; row < MAX_ROWS; row--)
	{
		if (files[exaCol][row] == FILE_EMPTY) continue;

		*output = &files[exaCol][row];
		return true;
	}

	return false;
}

bool getTargetedSpace(unsigned char **output)
{
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		if (files[exaCol][row] != FILE_EMPTY) continue;

		*output = &files[exaCol][row];
		return true;
	}

	return false;
}

bool grab() // all of these return true if something changed
{
	unsigned char *targetedFile;
	if (!getTargetedFile(&targetedFile)) return false; // column is empty

	isHoldingFile = true;
	heldFile = *targetedFile;
	*targetedFile = FILE_EMPTY;
	return true;
}

bool drop()
{
	unsigned char *targetedSpace;
	if (!getTargetedSpace(&targetedSpace)) return false;

	isHoldingFile = false;
	*targetedSpace = heldFile;
	return true;
}

bool swap()
{
	unsigned char *targetedFile;
	if (!getTargetedFile(&targetedFile)) return false;

	const unsigned char swapFile = *targetedFile;
	*targetedFile = heldFile;
	heldFile = swapFile;
	return true;
}

bool swapInPlace()
{
	// this is usually equivalent to just grab, then drop, then swap,
	// but doing it in each piece will make it easier to catch cases
	// where it doesn't work without stopping the process halfway
	// through (which would be inaccurate)

	unsigned char *topFile;
	if (!getTargetedFile(&topFile))
	{
		return false;
	}

	const unsigned char swapFile = *topFile;
	*topFile = FILE_EMPTY;

	unsigned char *bottomFile;
	if (!getTargetedFile(&bottomFile))
	{
		*topFile = swapFile;
		return false;
	}

	*topFile = *bottomFile;
	*bottomFile = swapFile;
	return true;
}

void drawCol(const unsigned char col, const unsigned int x)
{
	unsigned char y = GRID_VOFFSET;
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		if (files[col][row] != FILE_EMPTY)
		{
			// just draw a file
			gfx_Sprite_NoClip(fileSprites[files[col][row]], x, y);
		}
		else if (col == exaCol)
		{
			// draw the dashed line up from the exa
			const unsigned char xx = x + DASH_HOFFSET;
			for (unsigned char yy = y + DASH_VOFFSET; yy < y + GRID_SIZE; yy += DASH_INTERVAL)
			{
				gfx_SetPixel(xx, yy);
			}
		}
		y += GRID_SIZE;
	}

	if (col == exaCol) drawExa(); // else it will have been drawn over a bit
}

bool doInput()
{
	kb_Scan();

	const unsigned char oldExaCol = exaCol;

	if (kb_IsDown(kb_KeyRight) && exaCol < NUM_COLS - 1) exaCol++;
	if (kb_IsDown(kb_KeyLeft) && exaCol > 0) exaCol--;

	bool changedExaLook = false;

	// the three main operations
	if (kb_IsDown(kb_Key2nd) && !prev2nd)
	{
		if (isHoldingFile) changedExaLook = drop();
		else changedExaLook = grab();
	}
	if (kb_IsDown(kb_KeyAlpha) && !prevAlpha)
	{
		if (isHoldingFile) changedExaLook = swap();
		else swapInPlace(); // the exa will never change its look here
	}

	prev2nd = kb_IsDown(kb_Key2nd);
	prevAlpha = kb_IsDown(kb_KeyAlpha);

	if (exaCol != oldExaCol || changedExaLook) // redraw the exa
	{
		const unsigned char oldExaX = EXA_HOFFSET + GRID_SIZE * oldExaCol;
		gfx_Sprite(behindExa, oldExaX, EXA_VOFFSET);

		const unsigned char exaX = EXA_HOFFSET + GRID_SIZE * exaCol;
		gfx_GetSprite(behindExa, exaX, EXA_VOFFSET);

		drawExa();
	}

	return kb_IsDown(kb_KeyClear);
}

void findMatchRegion
(const unsigned char row, const unsigned char col, const unsigned char target, unsigned char *count)
{
	files[col][row] |= 0x80; // mark as visited

	// visited tiles won't == target
	if (col > 0 && files[col - 1][row] == target) findMatchRegion(row, col - 1, target, count);
	if (col < NUM_COLS - 1 && files[col + 1][row] == target) findMatchRegion(row, col + 1, target, count);
	if (row > 0 && files[col][row - 1] == target) findMatchRegion(row - 1, col, target, count);
	if (row < MAX_ROWS - 1 && files[col][row + 1] == target) findMatchRegion(row + 1, col, target, count);

	(*count)++;
}

void addNewRow()
{
	// check for game over
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		if (files[col][MAX_ROWS - 1] != FILE_EMPTY) gameOver = true;
	}

	unsigned char swapRow[NUM_COLS];
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		// save the old rows to move
		swapRow[col] = files[col][0];

		// make a new first row
		files[col][0] = rand() % NUM_BLOCK_COLORS + 1;
	}

	// move all the blocks down
	for (unsigned char row = 1; row < MAX_ROWS; row++)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			unsigned char swapFile = files[col][row];
			files[col][row] = swapRow[col];
			swapRow[col] = swapFile;
		}
	}
}

bool tryScoreGrid(bool inChain)
{
	bool didScore = false;

	// check for sets to pop and score
	for (unsigned char row = MAX_ROWS - 1; row < MAX_ROWS; row--)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			// skip if this one obviously isn't a member of a set
			if (files[col][row] == FILE_EMPTY) continue;

			// find all contiguous matching blocks and their count
			unsigned char numMatchingBlocks = 0;
			findMatchRegion(row, col, files[col][row], &numMatchingBlocks);

			if (numMatchingBlocks >= AMOUNT_TO_MATCH)
			{
				// calculate the score for the matching region
				if (!inChain)
				{
					score += BASE_BLOCK_VALUE * AMOUNT_TO_MATCH;
					numMatchingBlocks -= AMOUNT_TO_MATCH;
					inChain = true;
				}
				while (numMatchingBlocks > 0)
				{
					// done like this to make it more easily swappable to a
					// quadratic system if i decide to make that change
					score += CHAIN_BLOCK_VALUE;
					numMatchingBlocks--;
				}

				// remove the matching region
				for (unsigned char checkRow = 0; checkRow < MAX_ROWS; checkRow++)
				{
					for (unsigned char checkCol = 0; checkCol < NUM_COLS; checkCol++)
					{
						if (files[checkCol][checkRow] & 0x80) files[checkCol][checkRow] = FILE_EMPTY;
					}
				}

				didScore = true;
			}
			else
			{
				// unmark the matching region
				for (unsigned char checkRow = 0; checkRow < MAX_ROWS; checkRow++)
				{
					for (unsigned char checkCol = 0; checkCol < NUM_COLS; checkCol++)
					{
						files[checkCol][checkRow] &= 0x7f;
					}
				}	
			}
		}
	}

	return didScore;
}

void collapseGrid()
{
	// since this is a top-to-bottom search, we can do all the collapsing in one pass

	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		for (unsigned char row = 0; row < MAX_ROWS; row++)
		{
			// searching for empty files
			if (files[col][row] != FILE_EMPTY) continue;

			// search for the next file down (if any)
			for (unsigned char targetRow = row + 1; targetRow < MAX_ROWS; targetRow++)
			{
				if (files[col][targetRow] == FILE_EMPTY) continue;

				// move the file up
				files[col][row] = files[col][targetRow];
				files[col][targetRow] = FILE_EMPTY;
				break;
			}
		}
	}
}

void updateGrid()
{
	// score and collapse the grid until no more scoring can be done
	bool inChain = false;
	while (tryScoreGrid(inChain))
	{
		collapseGrid();
	};

	// move the grid down if it's time
	if (updateTracker % NEW_ROW_INTERVAL == 0)
	{
		addNewRow();
	}

	updateTracker++;
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
	// clear the old grid
	gfx_SetColor(COLOR_BLACK);
	gfx_FillRectangle_NoClip(GRID_HOFFSET, GRID_VOFFSET, NUM_COLS * GRID_SIZE, MAX_ROWS * GRID_SIZE);

	// draw the new grid
	gfx_SetColor(COLOR_DASH);
	unsigned char x = GRID_HOFFSET;
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		drawCol(col, x);
		x += GRID_SIZE;
	}

	// draw the score
	drawNumber(SCORE_HOFFSET, SCORE_VOFFSET, score);

#ifndef NO_BUFFER
	gfx_BlitBuffer();
#endif
}

void startGame()
{
	// draw the background
	gfx_FillScreen(COLOR_BLACK);
	gfx_RLETSprite_NoClip(background, BG_HOFFSET, BG_VOFFSET);

	// draw the high score
	drawNumber(HIGH_SCORE_HOFFSET, HIGH_SCORE_VOFFSET, highScore);

	score = 0;
	updateTracker = 0;
	gameOver = 0;

	srand(rtc_Time());

	// populate the initial grid
	for (unsigned char row = 0; row < NUM_INITIAL_ROWS - 1; row++)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			files[col][row] = rand() % NUM_BLOCK_COLORS + 1;
		}
	}

	// set and draw initial exa position
	exaCol = EXA_START_COL;
	gfx_GetSprite(behindExa, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);
	gfx_RLETSprite(exa_empty, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);
}

void endGame()
{
	if (score > highScore)
	{
		const unsigned char saveVar = ti_Open(SAVE_VAR_NAME, "w");
		ti_Write(&score, 3, 1, saveVar);
		ti_Close(saveVar);
	}

	toExit = true;
}

void init()
{
	toExit = false;

	// find the high score
	unsigned char saveVar = ti_Open(SAVE_VAR_NAME, "r");
	if (saveVar == 0)
	{
		// need to make a new file
		ti_Close(saveVar);
		saveVar = ti_Open(SAVE_VAR_NAME, "w");
		highScore = 0;
		ti_Write(&highScore, 3, 1, saveVar);
	}
	else
	{
		ti_Read(&highScore, 3, 1, saveVar);
	}
	ti_Close(saveVar);

	// initialize sprite storage
	behindExa->width = exa_empty_width;
	behindExa->height = exa_empty_height;
}

void titleScreen()
{
	gfx_FillScreen(COLOR_BLACK);
	gfx_RLETSprite_NoClip(title, TITLE_SPRITE_HOFFSET, TITLE_SPRITE_VOFFSET);

	gfx_SetTextFGColor(COLOR_RED);
	gfx_SetTextXY(TITLE_TEXT_HOFFSET, TITLE_TEXT_VOFFSET);
	gfx_PrintString("Press any key...");

#ifndef NO_BUFFER
	gfx_BlitBuffer();
#endif

	while (!os_GetCSC());
}

int main(void)
{
	gfx_Begin();
#ifdef NO_BUFFER
	gfx_SetDrawScreen();
#else
	gfx_SetDrawBuffer();
#endif
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

			updateGrid();
			drawFrame();

			if (gameOver) break;

			while (clock() - frameTimer < SIMUL_FRAME_TIME);
		}

		endGame();
	}

	gfx_End();

	return 0;
}
