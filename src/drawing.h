#ifndef drawing_include_file
#define drawing_include_file

extern struct gfx_sprite_t *behindExa;

void drawExa();
void drawCol(const unsigned char col);
void clearifySprites(const unsigned char step);
void animateClear();
void drawNumber(const unsigned int x, const unsigned char y, const unsigned int toDraw);
void drawFrame();
void titleScreen();

#endif
