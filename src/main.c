#include <sys/rtc.h>
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "gfx/gfx.h"

#define SAVE_VAR_NAME "HKMCHDAT"

int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetPalette(&global_palette, sizeof_global_palette, 0);
	gfx_SetTransparentColor(3);

	kb_EnableOnLatch();

	while (true)
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
