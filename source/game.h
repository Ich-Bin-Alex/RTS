#ifndef H_GAME
#define H_GAME

#include "source/tools/raylib.h"
#include "source/tools/helper.h"
#include "source/tools/vector.h"

typedef struct tPlayer {
	Texture Sprites, Buildings;
	Color Color, Color2;
	i32 Population, PopulationLimit;
	i32 Food, Wood, FoodIncome, WoodIncome;
} tPlayer;

typedef u8 PlayerHandle;

extern u32 FrameCount;
extern tPlayer Player[8];
extern Texture Tileset;
extern i32 DrawSize, FontSize;
extern Color TextColor, GoodColor, BadColor, HealthColor1, HealthColor2, HealthColor3;

void initGame(void);
void updateGame(void);
void drawTile(i32 x, i32 y, u32 tx, u32 ty, f32 alpha);
void drawBuilding(i32 x, i32 y, u32 tx, u32 ty, PlayerHandle player);
void drawTileFixed(i32 x, i32 y, u32 tx, u32 ty, Color color, i32 scale);
void drawTileFree(vec2 pos, u32 tx, u32 ty);
void drawSprite(vec2 pos, PlayerHandle player, u32 tx, u32 ty);
void drawSpriteFixed(i32 x, i32 y, PlayerHandle player, u32 tx, u32 ty);
i32 choice(i32 x, i32 y);

#endif