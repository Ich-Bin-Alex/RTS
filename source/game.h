#ifndef H_GAME
#define H_GAME

#include "source/tools/helper.h"

typedef struct tPlayer {
	u8 Color;
} tPlayer;

typedef u8 PlayerHandle;

extern u32 FrameCount;
extern tPlayer Player[8];
extern Texture Tileset, Sprites[8];

void initGame();
void updateGame();

#endif