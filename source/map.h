#ifndef H_MAP
#define H_MAP

#include "source/tools/helper.h"
#include "source/map.h"
#include "source/building.h"

#define MAP_SIZE 0x40

extern i32 CameraX, CameraY;
extern const i32 NX[8], NY[8];

typedef struct tTile {
	u8 Move; // Movement costs, used by the pathfinding
	u8 Bottom, Top;
	u8 Animation;
	f32 Frame;
	f32 Blood;
	bool Seen; // Fog of war
	union {
		struct {
			u8 OccupiedTree, MaxOccupy; // Used to prevent too many units chopping the same tree
		};
		struct {
			BuildingHandle Building;
		};
	};
	bool Unreachable;
} tTile;

extern tTile Map[MAP_SIZE][MAP_SIZE];

void setSafe(u32 x, u32 y, tTile tile);
tTile getSafe(u32 x, u32 y);
tTile *refSafe(u32 x, u32 y);
bool isTree(u32 x, u32 y);
bool isFarm(u32 x, u32 y);
bool isReachable(u32 x, u32 y);
i32 toMapX(f32 x);
i32 toMapY(f32 y);
void createMap(u32 seed);
void beginDrawMap(void);
void endDrawMap(void);

#endif