#include <sys/rtc.h>
#include <ti/screen.h>
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "gfx/gfx.h"
#include "drawing.h"
#include "variables.h"

#include <debug.h>

bool gameOver;

unsigned int score;
unsigned int highScore;

unsigned char files[NUM_COLS][MAX_ROWS];
unsigned char exaCol;
bool isHoldingFile;
unsigned char heldFile;

unsigned char gridMoveOffset;
clock_t nextMoveTime;

unsigned char prevRight, prevLeft, prev2nd, prevAlpha;

bool matched;
unsigned char starMatches;
clock_t clearTime;
unsigned int nextValue;

bool getTargetedFile(unsigned char *output) // each of these returns false on failure
{
	for (unsigned char row = MAX_ROWS - 1; row < MAX_ROWS; row--)
	{
		if (files[exaCol][row] == FILE_EMPTY) continue;
		if (files[exaCol][row] & 0x80) return false; // no moving matched files!

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

bool grab() // all of these return true if something changed
{
	unsigned char targetedRow;
	if (!getTargetedFile(&targetedRow)) return false; // column is empty

	const unsigned char fileValue = files[exaCol][targetedRow];

	clock_t refundTimer = clock();

	// these animations are always drawn unbuffered. YEAH!
#ifndef NO_BUFFER
	gfx_SetDrawScreen();
#endif
	for (unsigned char row = targetedRow; row < MAX_ROWS - 1; row++)
	{
		clock_t animationTimer = clock();

		files[exaCol][row] = FILE_EMPTY;
		files[exaCol][row + 1] = fileValue;
		drawCol(exaCol);

		while (clock() - animationTimer < MOVE_ANIMATION_FRAME_TIME);
	}

	files[exaCol][MAX_ROWS - 1] = FILE_EMPTY;
	isHoldingFile = true;
	heldFile = fileValue;
#ifndef NO_BUFFER
	gfx_SetDrawBuffer();
#endif

	nextMoveTime += clock() - refundTimer;

	return true;
}

bool drop()
{
	unsigned char targetedRow;
	if (!getTargetedSpace(&targetedRow)) return false;

	clock_t refundTimer = clock();
	clock_t animationTimer = clock();

#ifndef NO_BUFFER
	gfx_SetDrawScreen();
#endif
	const unsigned char fileValue = heldFile;
	isHoldingFile = false;
	files[exaCol][MAX_ROWS - 1] = fileValue;
	drawCol(exaCol);

	for (unsigned char row = MAX_ROWS - 1; row > targetedRow; row--)
	{
		files[exaCol][row] = FILE_EMPTY;
		files[exaCol][row - 1] = fileValue;

		while (clock() - animationTimer < MOVE_ANIMATION_FRAME_TIME);

		drawCol(exaCol);

		animationTimer = clock();
	}
#ifndef NO_BUFFER
	gfx_SetDrawBuffer();
#endif

	nextMoveTime += clock() - refundTimer;

	return true;
}

bool swap()
{
	unsigned char topRow;
	if (!getTargetedFile(&topRow)) return false;
	if (topRow == 0) return false; // nothing under the top file to swap
	if (files[exaCol][topRow - 1] & 0x80) return false; // no moving matched files!

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

	return !kb_On;
}

void findMatchRegion
(const unsigned char row, const unsigned char col, const unsigned char target, unsigned char *count)
{
	files[col][row] |= 0xc0; // mark as visited

	// visited tiles won't == target
	if (col > 0 && (files[col - 1][row] & 0x7f) == target)
	{
		findMatchRegion(row, col - 1, target, count);
	}
	if (col < NUM_COLS - 1 && (files[col + 1][row] & 0x7f) == target)
	{
		findMatchRegion(row, col + 1, target, count);
	}
	if (row > 0 && (files[col][row - 1] & 0x7f) == target)
	{
		findMatchRegion(row - 1, col, target, count);
	}
	if (row < MAX_ROWS - 1 && (files[col][row + 1] & 0x7f) == target) 
	{
		findMatchRegion(row + 1, col, target, count);
	}

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
			if (files[cleaningCol][cleaningRow] & 0x40) files[cleaningCol][cleaningRow] &= 0x3f;
		}
	}
}

void getNextMoveTime()
{
	if (score < ROW_INTERVAL_SCALE_END)
	{
		nextMoveTime = clock() + MAX_NEW_ROW_INTERVAL / 4;

		if (score > ROW_INTERVAL_SCALE_START)
		{
			nextMoveTime -= (score - ROW_INTERVAL_SCALE_START) * ROW_INTERVAL_SCALE_FACTOR / 4;
		}
	}
	else
	{
		nextMoveTime = clock() + MIN_NEW_ROW_INTERVAL / 4;
	}
}

void addNewRow()
{
	// check for game over
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		if (files[col][MAX_ROWS - 1] == FILE_EMPTY) continue;

		// if we make it here, there was something in the lowest row
		gameOver = true;
		return;
	}

	unsigned char swapRow[NUM_COLS];
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		// save the old rows to move
		swapRow[col] = files[col][0];

		// overwrite with zeroes so it doesn't mess with checks while making a new row later
		files[col][0] = FILE_EMPTY;
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

	// make a new first row, regenerating each block until it doesn't make a set
	for (unsigned char col = 0; col < NUM_COLS; col++)
	{
		while (true)
		{
			files[col][0] = rand() % NUM_BLOCK_COLORS + 1;
			if (rand() % STAR_CHANCE == 0) files[col][0] |= 0x08; // set it to be a star

			unsigned char numInGroup = 0;
			findMatchRegionClean(0, col, files[col][0], &numInGroup);

			if ((files[col][0] & 0x08) && numInGroup >= 2) continue;
			if (numInGroup >= AMOUNT_FILES_TO_MATCH) continue;

			break;
		}
	}

	// move all the blocks back up so it's not too jarring
	gridMoveOffset += GRID_SIZE;
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

void setClearTime()
{
	if (matched) return;

	matched = true;
	clearTime = clock() + MATCH_TIME;
}

void clearMarks(const bool toMark)
{
	for (unsigned char checkRow = 0; checkRow < MAX_ROWS; checkRow++)
	{
		for (unsigned char checkCol = 0; checkCol < NUM_COLS; checkCol++)
		{
			if (toMark) files[checkCol][checkRow] &= 0xbf;
			else if (files[checkCol][checkRow] & 0x40) files[checkCol][checkRow] &= 0x3f;
		}
	}	
}

void checkForMatch()
{
	// check for sets to pop and score
	for (unsigned char row = MAX_ROWS - 1; row < MAX_ROWS; row--)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			if (files[col][row] == FILE_EMPTY) continue;
			if (files[col][row] & 0x80) continue;

			if (files[col][row] & 0x08)
			{
				// it's a star... does it have a match?
				unsigned char numMatchingStars = 0;
				findMatchRegion(row, col, files[col][row], &numMatchingStars);
				if (numMatchingStars < 2)
				{
					// if not, we don't care about it, bc it can't form a set
					clearMarks(false);
				}
				else
				{
					starMatches += numMatchingStars;

					// if we get here, it does have a match!
					const unsigned char target = files[col][row] & 0x07;
					for (unsigned char starRow = 0; starRow < MAX_ROWS; starRow++)
					{
						for (unsigned char starCol = 0; starCol < NUM_COLS; starCol++)
						{
							if (files[starCol][starRow] & 0x40) files[starCol][starRow] &= 0xbf;
							if (files[starCol][starRow] != target) continue;
							
							files[starCol][starRow] |= 0x80;
							starMatches++; // keep track so we don't award points for them later
						}
					}

					setClearTime();
				}
			}
			else
			{
				// find all contiguous matching blocks and their count
				unsigned char numMatchingBlocks = 0;
				findMatchRegion(row, col, files[col][row], &numMatchingBlocks);

				// unmark or cement the matching region
				const bool toMark = numMatchingBlocks >= AMOUNT_FILES_TO_MATCH;
				clearMarks(toMark);	
				if (toMark) setClearTime();	
			}
		}
	}
}

void scoreGrid()
{
	animateClear();

	// remove the matching region and count how many blocks
	unsigned char numMatchingBlocks = 0;	
	for (unsigned char checkRow = 0; checkRow < MAX_ROWS; checkRow++)
	{
		for (unsigned char checkCol = 0; checkCol < NUM_COLS; checkCol++)
		{
			if (files[checkCol][checkRow] & 0x80)
			{
				files[checkCol][checkRow] = FILE_EMPTY;
				numMatchingBlocks++;
			}
		}
	}

	// calculate the score for the matching region
	unsigned char atBase = 0;
	if (nextValue == BASE_BLOCK_VALUE) atBase = AMOUNT_FILES_TO_MATCH;
	numMatchingBlocks -= starMatches;
	while (numMatchingBlocks > 0)
	{
		if (atBase > 0) (atBase)--;
		else nextValue += CHAIN_BLOCK_BONUS;
					
		score += nextValue;
		numMatchingBlocks--;

		dbg_printf("added block with value %u\n", nextValue);
	}

	dbg_printf("total score: %u\n", score);

	// tidy up
	matched = false;
	starMatches = 0;
	collapseGrid();
}

void updateGrid()
{
	clock_t refundTimer = clock();

	// remove the matched blocks and collapse the grid if it's time
	if (matched && clock() > clearTime)
	{
		scoreGrid();
	}

	// check to see if there are any matches now
	checkForMatch();

	// reset the score if not
	if (!matched)
	{
		nextValue = BASE_BLOCK_VALUE;

		// ...and move the grid down if it's time
		if (clock() > nextMoveTime)
		{
			gridMoveOffset -= GRID_SIZE / GRID_MOVE_STEPS;
			if (gridMoveOffset == 0) addNewRow();
			getNextMoveTime();
		}
	}

	// if there is a match, don't move the grid down
	else 
	{
		nextMoveTime += clock() - refundTimer;
	}
}

void startGame()
{
	// restore the palette
	gfx_SetPalette(global_palette, 32 + sizeof_grid_palette, 0);

	// draw the background
	gfx_FillScreen(COLOR_BLACK);
	gfx_SetColor(COLOR_METAL);
	gfx_FillRectangle_NoClip(BG_HOFFSET, BG_VOFFSET, background_width, background_height);
	gfx_SetColor(COLOR_BLACK);
	gfx_FillRectangle_NoClip(BLK_RECT_1_X, BLK_RECT_1_Y, BLK_RECT_1_W, BLK_RECT_1_H);
	gfx_FillRectangle_NoClip(BLK_RECT_2_X, BLK_RECT_2_Y, BLK_RECT_2_W, BLK_RECT_2_H);
	gfx_FillRectangle_NoClip(BLK_RECT_3_X, BLK_RECT_3_Y, BLK_RECT_3_W, BLK_RECT_3_H);
	gfx_RLETSprite_NoClip(background, BG_HOFFSET, BG_VOFFSET);
	gfx_SetColor(COLOR_DOT);
	for (unsigned int x = DOTS_HOFFSET; x <= DOTS_HOFFSET + 2 * NUM_DOTS; x += 2)
	{
		gfx_SetPixel(x, DOTS_VOFFSET);
	}

	// draw the high score
	drawNumber(HIGH_SCORE_HOFFSET, HIGH_SCORE_VOFFSET, highScore);

	score = 0;
	gameOver = 0;
	deathStage = 0; // reset animation from last game
	matched = false;
	nextValue = BASE_BLOCK_VALUE;

	srand(rtc_Time());

	// clear the grid from last game
	for (unsigned char row = 0; row < MAX_ROWS; row++)
	{
		for (unsigned char col = 0; col < NUM_COLS; col++)
		{
			files[col][row] = FILE_EMPTY;
		}
	}

	// populate the initial grid
	for (unsigned char i = 0; i <= NUM_INITIAL_ROWS; i++)
	{
		addNewRow();
	}

	gridMoveOffset = GRID_SIZE;

	// set and draw initial exa position
	exaCol = EXA_START_COL;
	gfx_GetSprite(behindExa, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);
	gfx_TransparentSprite_NoClip(exa_empty, EXA_HOFFSET + EXA_START_COL * GRID_SIZE, EXA_VOFFSET);

	nextMoveTime = clock() + MIN_NEW_ROW_INTERVAL;
}

bool endGame()
{
	if (score > highScore)
	{
		const unsigned char saveVar = ti_Open(SAVE_VAR_NAME, "w");
		ti_Write(&score, 3, 1, saveVar);
		ti_Close(saveVar);
		highScore = score;
	}

	if (gameOver)
	{
		isHoldingFile = false;
		animateDeath();

		// wait for input to either exit or restart
		while (true)
		{
			deathPeriodic();

			kb_Scan();

			if (kb_IsDown(kb_KeyClear)) return false;
			if (kb_IsDown(kb_Key2nd)) return true;
		}
	}

	return false;
}

unsigned char init()
{
	kb_EnableOnLatch();
	kb_ClearOnLatch();

	// initialize the sprites	
	if(!HKMCHGFX_init()) return 1;

	fileSprites[0] = 0;
	fileSprites[1] = file_red;
	fileSprites[2] = file_yellow;
	fileSprites[3] = file_cyan;
	fileSprites[4] = file_blue;
	fileSprites[5] = file_purple;
	fileSprites[6] = 0;
	fileSprites[7] = 0;
	fileSprites[8] = 0;
	fileSprites[9] = star_red;
	fileSprites[10] = star_yellow;
	fileSprites[11] = star_cyan;
	fileSprites[12] = star_blue;
	fileSprites[13] = star_purple;

	fileMatchSprites[0] = 0;
	fileMatchSprites[1] = file_match_red;
	fileMatchSprites[2] = file_match_yellow;
	fileMatchSprites[3] = file_match_cyan;
	fileMatchSprites[4] = file_match_blue;
	fileMatchSprites[5] = file_match_purple;

	digitSprites[0] = digit_0;
	digitSprites[1] = digit_1;
	digitSprites[2] = digit_2;
	digitSprites[3] = digit_3;
	digitSprites[4] = digit_4;
	digitSprites[5] = digit_5;
	digitSprites[6] = digit_6;
	digitSprites[7] = digit_7;
	digitSprites[8] = digit_8;
	digitSprites[9] = digit_9;

	deathSprites[0] = 0;
	deathSprites[1] = exa_dying_1;
	deathSprites[2] = exa_dying_2;

	// basic graphics display settings
	gfx_Begin();
#ifdef NO_BUFFER
	gfx_SetDrawScreen();
#else
	gfx_SetDrawBuffer();
#endif

	// set the clip area so that the grid can be offset
	gfx_SetClipRegion(0, CLIP_VOFFSET, GFX_LCD_WIDTH, GFX_LCD_HEIGHT);

	// prepare the sketchy combination palette
	for (unsigned char i = 0; i < sizeof_fixed_palette; i++) global_palette[i] = fixed_palette[i];
	for (unsigned char i = 0; i < sizeof_grid_palette; i++) global_palette[i + 32] = grid_palette[i];
	gfx_SetPalette(global_palette, 32 + sizeof_grid_palette, 0);
	gfx_SetTransparentColor(16);

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

	return 0;
}

int main(void)
{
	if (init())
	{
		os_ClrHome();
		os_PutStrFull("Missing gfx var!");

		while (!kb_AnyKey());

		return 0;
	}

	do
	{
		if (titleScreen()) break;

		// wait for keys to be reset
		while (kb_AnyKey());

		startGame();

		while (doInput())
		{
			updateGrid();
			drawFrame();

			if (gameOver) break;
		}
	}
	while (endGame());

	gfx_End();

	kb_ClearOnLatch();

	return 0;
}
