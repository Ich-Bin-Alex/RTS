#include "source/tools/raylib.h"
#include "source/tools/helper.h"
#include "source/map.h"
#include "source/game.h"

u32 FrameCount = 0;

tPlayer Player[8] = {
	{Color: 0},
	{Color: 1},
	{Color: 2},
	{Color: 3},
	{Color: 4},
	{Color: 5},
	{Color: 6},
	{Color: 7}
};
Texture Tileset, Sprites[8];

void initGame() {
	Image img = LoadImage(TextFormat("%s/data/tileset.png", GetApplicationDirectory()));
	Tileset = LoadTextureFromImage(img);

	const u32 PlayerColors[8][2] = {
		{0x1e548dff, 0x1b306aff},
		{0x8d1e1eff, 0x6a1b1bff},
		{0x1e8d1eff, 0x1b6a1bff},
		{0x8d861eff, 0x6a651bff},
		{0x4a1e8dff, 0x3b1b6aff},
		{0x8d431eff, 0x6a351bff},
		{0x1e8d8dff, 0x1b6a6aff},
		{0x4d4d4dff, 0x3c3c3cff}
	};

	for(i32 i = 0; i < 8; i++) {
		Image sprites = ImageFromImage(img, (Rectangle){128,0,128,128});
		ImageColorReplace(&sprites, GetColor(0xb8b8b8ff), GetColor(PlayerColors[i][0]));
		ImageColorReplace(&sprites, GetColor(0x858585ff), GetColor(PlayerColors[i][1]));
		Sprites[i] = LoadTextureFromImage(sprites);
		UnloadImage(sprites);
	}

	createMap(GetTime()*1000.0);

	UnloadImage(img);
}

void updateGame() {
	FrameCount++;
}

void drawTile(i32 x, i32 y, u32 tx, u32 ty, f32 alpha) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8}, 
		(Rectangle){x*8*DRAW_SIZE-CameraX,y*8*DRAW_SIZE-CameraY,8*DRAW_SIZE,8*DRAW_SIZE}, 
		(Vector2){0}, 0, (Color){255,255,255,255.0*alpha});
}

void drawTileFree(Vector2 pos, u32 tx, u32 ty) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8}, 
		(Rectangle){pos.x*8*DRAW_SIZE-CameraX,pos.y*8*DRAW_SIZE-CameraY,8*DRAW_SIZE,8*DRAW_SIZE}, 
		(Vector2){0}, 0, WHITE);
}

void drawSprite(Vector2 pos, PlayerHandle player, u32 tx, u32 ty) {
	DrawTexturePro(Sprites[Player[player].Color], (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){pos.x*8*DRAW_SIZE-CameraX,pos.y*8*DRAW_SIZE-CameraY,8*DRAW_SIZE,8*DRAW_SIZE}, 
		(Vector2){0}, 0, WHITE);
}