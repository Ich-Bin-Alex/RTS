#ifndef H_MAP
#define H_MAP

#include "source/tools/helper.h"
#include "source/map.h"

#define MAP_SIZE 0x40

extern i32 CameraX, CameraY;

typedef struct tTile {
	u8 Move;
	u8 Bottom;
	u8 Top;
	u8 Animation;
	f32 Blood;
	f32 Frame;
	bool Seen;
} tTile;

extern tTile Map[MAP_SIZE][MAP_SIZE];

tTile getSafe(u32 x, u32 y);
i32 toMapX(f32 x);
i32 toMapY(f32 y);
void createMap(u32 seed);
void beginDrawMap();
void endDrawMap();

#endif