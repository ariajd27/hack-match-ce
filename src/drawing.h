#ifndef drawing_include_file
#define drawing_include_file

extern unsigned char global_palette[32 + sizeof_grid_palette];
extern unsigned char deathStage;
extern struct gfx_sprite_t *behindExa;

extern struct gfx_sprite_t *fileSprites[14];
extern struct gfx_sprite_t *fileMatchSprites[6];
extern struct gfx_sprite_t *digitSprites[10];
extern struct gfx_rletsprite_t *deathSprites[3];

void drawExa();
void drawCol(const unsigned char col);
void clearifySprites(const unsigned char step);
void animateClear();
void drawNumber(const unsigned int x, const unsigned char y, const unsigned int toDraw);
void drawFrame();
void titleScreen();
void animateDeath();

#endif
