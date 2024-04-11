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

#include <debug.h>

// uncomment this line out to draw directly to the screen
// #define NO_BUFFER

#define SAVE_VAR_NAME "HKMCHDAT"

#define ANIM_FRAME_TIME 800

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
#define NUM_BLOCK_COLORS 5
#define EXA_START_COL 2
#define AMOUNT_TO_MATCH 4
#define BASE_BLOCK_VALUE 100
#define CHAIN_BLOCK_BONUS 50
#define MAX_NEW_ROW_INTERVAL 200000ul
#define MIN_NEW_ROW_INTERVAL 50000ul
#define ROW_INTERVAL_SCALE_START 0ul
#define ROW_INTERVAL_SCALE_END 100000ul
#define ROW_INTERVAL_SCALE_FACTOR (MAX_NEW_ROW_INTERVAL - MIN_NEW_ROW_INTERVAL) \
								/ (ROW_INTERVAL_SCALE_END - ROW_INTERVAL_SCALE_START)

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

unsigned int score;
unsigned int highScore;

unsigned char files[NUM_COLS][MAX_ROWS];
unsigned char exaCol;
bool isHoldingFile;
unsigned char heldFile;

clock_t nextLineTime;

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

unsigned char prevRight, prevLeft, prev2nd, prevAlpha;

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

bool getTargetedFile(unsigned char *output) // each of these returns false on failure
{
	for (unsigned char row = MAX_ROWS - 1; row < MAX_ROWS; row--)
	{
		if (files[exaCol][row] == FILE_EMPTY) continue;

		*output = row;
		return true;
	}

	return false;
}

bool getTargetedSpace(unsigned char *output)
{
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		if (files[exaCol][row] != FILE_EMPTY) continue;

		*output = row;
		return true;
	}

	return false;
}

void drawCol(const unsigned char col)
{
	const unsigned int x = GRID_HOFFSET + col * GRID_SIZE;

	unsigned char y = GRID_VOFFSET;
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		if (files[col][row] != FILE_EMPTY)
		{
			// just draw a file
			gfx_Sprite_NoClip(fileSprites[files[col][row]], x, y);
		}
		else
		{
			gfx_SetColor(COLOR_BLACK);
			gfx_FillRectangle_NoClip(x, y, GRID_SIZE, GRID_SIZE);

			if (col == exaCol)
			{
				// draw the dashed line up from the exa
				gfx_SetColor(COLOR_DASH);
				const unsigned char xx = x + DASH_HOFFSET;
				for (unsigned char yy = y + DASH_VOFFSET; yy < y + GRID_SIZE; yy += DASH_INTERVAL)
				{
					gfx_SetPixel(xx, yy);
				}
			}
		}
		y += GRID_SIZE;
	}

	if (col == exaCol) drawExa(); // else it will have been drawn over a bit
}

bool grab() // all of these return true if something changed
{
	unsigned char targetedRow;
	if (!getTargetedFile(&targetedRow)) return false; // column is empty

	const unsigned char fileValue = files[exaCol][targetedRow];

	// these animations are always drawn unbuffered. YEAH!
#ifndef NO_BUFFER
	gfx_SetDrawScreen();
#endif
	for (unsigned char row = targetedRow; row < MAX_ROWS - 1; row++)
	{
		files[exaCol][row] = FILE_EMPTY;
		files[exaCol][row + 1] = fileValue;
		drawCol(exaCol);
	}

	files[exaCol][MAX_ROWS - 1] = FILE_EMPTY;
	isHoldingFile = true;
	heldFile = fileValue;
#ifndef NO_BUFFER
	gfx_SetDrawBuffer();
#endif

	return true;
}

bool drop()
{
	unsigned char targetedRow;
	if (!getTargetedSpace(&targetedRow)) return false;

#ifndef NO_BUFFER
	gfx_SetDrawScreen();
#endif
	const unsigned char fileValue = heldFile;
	isHoldingFile = false;
	drawExa(); // we'll have to draw it again later for the buffer, but... myeh.
	files[exaCol][MAX_ROWS - 1] = fileValue;
	drawCol(exaCol);

	for (unsigned char row = MAX_ROWS - 1; row > targetedRow; row--)
	{
		files[exaCol][row] = FILE_EMPTY;
		files[exaCol][row - 1] = fileValue;
		drawCol(exaCol);
	}
#ifndef NO_BUFFER
	gfx_SetDrawBuffer();
#endif

	return true;
}

bool swap()
{
	unsigned char topRow;
	if (!getTargetedFile(&topRow)) return false;
	if (topRow == 0) return false; // nothing under the top file to swap

	const unsigned char swapFile = files[exaCol][topRow];

	files[exaCol][topRow] = files[exaCol][topRow - 1];
	files[exaCol][topRow - 1] = swapFile;
	return true;
}

bool doInput()
{
	kb_Scan();

	const unsigned char oldExaCol = exaCol;

	if (kb_IsDown(kb_KeyRight) && !prevRight && exaCol < NUM_COLS - 1) exaCol++;
	if (kb_IsDown(kb_KeyLeft) && !prevLeft && exaCol > 0) exaCol--;

	bool changedExaLook = false;

	// the three main operations
	if (kb_IsDown(kb_Key2nd) && !prev2nd)
	{
		if (isHoldingFile) changedExaLook = drop();
		else changedExaLook = grab();
	}
	if (kb_IsDown(kb_KeyAlpha) && !prevAlpha) swap();

	prevLeft = kb_IsDown(kb_KeyLeft);
	prevRight = kb_IsDown(kb_KeyRight);
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

void findMatchRegionClean
(const unsigned char row, const unsigned char col, const unsigned char target, unsigned char *count)
{
	findMatchRegion(row, col, target, count);

	for (unsigned char cleaningRow = 0; cleaningRow < MAX_ROWS; cleaningRow++)
	{
		for (unsigned char cleaningCol = 0; cleaningCol < NUM_COLS; cleaningCol++)
		{
			files[cleaningCol][cleaningRow] &= 0x7f;
		}
	}
}

void getNextLineTime()
{
	if (score < ROW_INTERVAL_SCALE_END)
	{
		nextLineTime = clock() + MAX_NEW_ROW_INTERVAL;

		if (score > ROW_INTERVAL_SCALE_START)
		{
			nextLineTime -= (score - ROW_INTERVAL_SCALE_START) * ROW_INTERVAL_SCALE_FACTOR;
		}
	}
	else
	{
		nextLineTime = clock() + MIN_NEW_ROW_INTERVAL;
	}
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

		// make a new first row -- avoiding completing a set
		unsigned char numInGroup;
		findMatchRegionClean(0, col, swapRow[col], &numInGroup);
		do
		{
			files[col][0] = rand() % NUM_BLOCK_COLORS + 1;
		}
		while (files[col][0] == swapRow[col] && numInGroup >= AMOUNT_TO_MATCH - 1);
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

bool tryScoreGrid(unsigned char *atBase, unsigned int *nextValue)
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
				while (numMatchingBlocks > 0)
				{
					if (*atBase > 0) (*atBase)--;
					else *nextValue += CHAIN_BLOCK_BONUS;
					
					score += *nextValue;
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
	unsigned char atBase = AMOUNT_TO_MATCH;
	unsigned int nextValue = BASE_BLOCK_VALUE;
	while (tryScoreGrid(&atBase, &nextValue))
	{
		collapseGrid();
	}

	// move the grid down if it's time
	if (clock() > nextLineTime)
	{
		addNewRow();
		getNextLineTime();
	}
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
	// draw the new grid
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

void startGame()
{
	// draw the background
	gfx_FillScreen(COLOR_BLACK);
	gfx_RLETSprite_NoClip(background, BG_HOFFSET, BG_VOFFSET);

	// draw the high score
	drawNumber(HIGH_SCORE_HOFFSET, HIGH_SCORE_VOFFSET, highScore);

	score = 0;
	gameOver = 0;

	srand(rtc_Time());

	// populate the initial grid
	for (unsigned char i = 0; i < NUM_INITIAL_ROWS; i++)
	{
		addNewRow();
	}

	// set and draw initial exa position
	exaCol = EXA_START_COL;
	gfx_GetSprite(behindExa, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);
	gfx_RLETSprite(exa_empty, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);

	nextLineTime = clock() + MIN_NEW_ROW_INTERVAL;
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
			if (doInput()) break;

			updateGrid();
			drawFrame();

			if (gameOver) break;
		}

		endGame();
	}

	gfx_End();

	return 0;
}
