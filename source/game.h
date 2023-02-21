#ifndef H_GAME
#define H_GAME

#include "source/tools/helper.h"

#define DRAW_SIZE 3

typedef struct tPlayer {
	u8 Color;
} tPlayer;

typedef u8 PlayerHandle;

extern u32 FrameCount;
extern tPlayer Player[8];
extern Texture Tileset, Sprites[8];

void initGame();
void updateGame();
void drawTile(i32 x, i32 y, u32 tx, u32 ty, f32 alpha);
void drawTileFree(Vector2 pos, u32 tx, u32 ty);
void drawSprite(Vector2 pos, PlayerHandle player, u32 tx, u32 ty);

#endif