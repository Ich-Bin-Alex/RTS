#include "source/tools/raylib.h"
#include "source/tools/helper.h"
#include "source/map.h"
#include "source/interface.h"
#include "source/game.h"

u32 FrameCount = 0;

tPlayer Player[8];
Texture Tileset;

void initGame() {
	Image img = LoadImage(TextFormat("%s/data/tileset.png", GetApplicationDirectory()));
	Tileset = LoadTextureFromImage(img);

	for(i32 i = 0; i < 8; i++) {
		Image sprites = ImageFromImage(img, (Rectangle){128,0,128,128});
		ImageColorReplace(&sprites, GetColor(0xb8b8b8ff), GetImageColor(img, 254, 248 + i));
		ImageColorReplace(&sprites, GetColor(0x858585ff), GetImageColor(img, 255, 248 + i));
		Player[i].Sprites = LoadTextureFromImage(sprites);
		Player[i].Color = GetImageColor(img, 254, 248 + i);
		UnloadImage(sprites);
	}
	for(u32 i = '\"'; i <= '~'; i++) {
		i32 count = 0; // Search for the first two (count == 2) empty pixel columns in the tile
		for(CharSizes[i] = 0; CharSizes[i] < 9; CharSizes[i]++) {
			i32 height = 0, x = ((i - ' ') % 16) * 8, y = (16 + (i - ' ') / 16) * 8;
			for(i32 j = 0; j < 8; j++) 
				if(!GetImageColor(img, x + CharSizes[i] + 1, y + j).a) height++;
			count += height == 8;
			if(count >= 2) break;
		}
	}

	createMap(GetTime()*1000.0);

	UnloadImage(img);
}

void updateGame() {
	FrameCount++;
}

void drawTile(i32 x, i32 y, u32 tx, u32 ty, f32 alpha) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8}, 
		(Rectangle){x*8*DRAW_SIZE-CameraX, y*8*DRAW_SIZE-CameraY, 8*DRAW_SIZE, 8*DRAW_SIZE}, 
		(Vector2){0}, 0, (Color){255,255,255,255.0*alpha});
}

void drawTileFixed(i32 x, i32 y, u32 tx, u32 ty, Color color, i32 scale) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8}, 
		(Rectangle){x, y, 8*scale, 8*scale}, (Vector2){0}, 0, color);
}

void drawTileFree(Vector2 pos, u32 tx, u32 ty) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8}, 
		(Rectangle){pos.x*8*DRAW_SIZE-CameraX, pos.y*8*DRAW_SIZE-CameraY, 8*DRAW_SIZE, 8*DRAW_SIZE}, 
		(Vector2){0}, 0, WHITE);
}

void drawSprite(Vector2 pos, PlayerHandle player, u32 tx, u32 ty) {
	DrawTexturePro(Player[player].Sprites, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){pos.x*8*DRAW_SIZE-CameraX, pos.y*8*DRAW_SIZE-CameraY, 8*DRAW_SIZE, 8*DRAW_SIZE}, 
		(Vector2){0}, 0, WHITE);
}