#include "source/tools/raylib.h"
#include "source/tools/helper.h"
#include "source/map.h"
#include "source/interface.h"
#include "source/game.h"

u32 FrameCount = 0;
tPlayer Player[8];
Texture Tileset;
i32 DrawSize = 3, FontSize = 2;
Color TextColor, GoodColor, BadColor, HealthColor1, HealthColor2, HealthColor3;

void initGame(void) {
	Image img = LoadImage(TextFormat("%s/data/tileset.png", GetApplicationDirectory()));
	Tileset = LoadTextureFromImage(img);

	TextColor = GetImageColor(img, 248, 248);
	GoodColor = GetImageColor(img, 248, 249);
	BadColor = GetImageColor(img, 248, 250);
	HealthColor1 = GetImageColor(img, 249, 248);
	HealthColor2 = GetImageColor(img, 249, 249);
	HealthColor3 = GetImageColor(img, 249, 250);

	for(i32 i = 0; i < 8; i++) {
		Image sprites = ImageFromImage(img, (Rectangle){128,0,128,128});
		Player[i].Color = GetImageColor(img, 254, 248 + i);
		Player[i].Color2 = GetImageColor(img, 255, 248 + i);
		ImageColorReplace(&sprites, GetColor(0xb8b8b8ff), Player[i].Color);
		ImageColorReplace(&sprites, GetColor(0x858585ff), Player[i].Color2);
		Player[i].Sprites = LoadTextureFromImage(sprites);
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

	HideCursor();
	UnloadImage(img);
}

void updateGame(void) {
	FrameCount++;
	for(i32 i = 0; i < 8; i++) Player[i].FoodIncome = Player[i].WoodIncome = 0;
	if(IsWindowResized()) {
		DrawSize = max(3, (GetScreenWidth() * GetScreenHeight()) / 262144);
		FontSize = DrawSize - 1;
	}
}

void drawTile(i32 x, i32 y, u32 tx, u32 ty, f32 alpha) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){x*8*DrawSize-CameraX, y*8*DrawSize-CameraY, 8*DrawSize, 8*DrawSize},
		(Vector2){0}, 0, (Color){255,255,255,255.0*alpha});
}

void drawTileFixed(i32 x, i32 y, u32 tx, u32 ty, Color color, i32 scale) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){x, y, 8*scale, 8*scale}, (Vector2){0}, 0, color);
}

void drawTileFree(Vector2 pos, u32 tx, u32 ty) {
	DrawTexturePro(Tileset, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){pos.x*8*DrawSize-CameraX, pos.y*8*DrawSize-CameraY, 8*DrawSize, 8*DrawSize},
		(Vector2){0}, 0, WHITE);
}

void drawSprite(Vector2 pos, PlayerHandle player, u32 tx, u32 ty) {
	DrawTexturePro(Player[player].Sprites, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){pos.x*8*DrawSize-CameraX, pos.y*8*DrawSize-CameraY, 8*DrawSize, 8*DrawSize},
		(Vector2){0}, 0, WHITE);
}

void drawSpriteFixed(i32 x, i32 y, PlayerHandle player, u32 tx, u32 ty) {
	DrawTexturePro(Player[player].Sprites, (Rectangle){tx*8, ty*8, 8, 8},
		(Rectangle){x, y, 8*DrawSize, 8*DrawSize}, (Vector2){0}, 0, WHITE);
}

i32 choice(i32 x, i32 y) {
	return GetRandomValue(0, 1) == 0 ? x : y;
}