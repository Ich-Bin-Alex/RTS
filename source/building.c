#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/building.h"
#include "source/unit.h"
#include "source/map.h"

u32 AllocatedBuildings, BuildingPtr;
tBuilding *Buildings;

tBuildingType Farm = {
	Name: "Farm",
	MaxHealth: 100,
	WoodCost: 50,
	ViewDistance: 0,
	BlockMovement: false,
	SizeX: 2, SizeY: 2,
	Icon: 0xd6,
	Tiles: {{0x70, 0x80},
	        {0x71, 0x81}},
	RubbleTiles: {{0x50, 0x60},
	              {0x51, 0x61}}
};

BuildingHandle newBuilding(tBuilding build, u32 x, u32 y) {
	BuildingHandle ret = 0;
	for(BuildingHandle i = 1; i < BuildingPtr; i++) if(!Buildings[i].Exists) {
		ret = i;
		break;
	}
	if(!ret) ret = ++BuildingPtr;
	if(BuildingPtr >= AllocatedBuildings) {
		AllocatedBuildings += 64;
		Buildings = realloc(Buildings, AllocatedBuildings*sizeof(tBuilding));
		memset(&Buildings[BuildingPtr-1], 0, 64*sizeof(tBuilding));
	}
	Buildings[ret] = build;
	Buildings[ret].Exists = true;
	Buildings[ret].FirstX = x;
	Buildings[ret].FirstY = y;
	if(!Buildings[ret].Health) Buildings[ret].Health = build.Type->MaxHealth;

	for(i32 xi = 0; xi < build.Type->SizeX; xi++) for(i32 yi = 0; yi < build.Type->SizeY; yi++)
		setSafe(x+xi, y+yi, (tTile){
			Bottom: build.Type->Tiles[xi][yi], 
			Seen: getSafe(x+xi, y+yi).Seen, 
			Building: ret, 
			Move: build.Type->BlockMovement ? 0xff : 0});

	return ret;
}

void destroyBuilding(BuildingHandle build) {
	i32 x = Buildings[build].FirstX, y = Buildings[build].FirstY;
	tBuildingType *type = Buildings[build].Type;
	Buildings[build].Exists = false;
	if(Buildings[build].Type == &Farm) Units[Buildings[build].Farm.Occupier].Action = ACTION_MOVE;
	for(i32 xi = 0; xi < type->SizeX; xi++) for(i32 yi = 0; yi < type->SizeY; yi++)
		setSafe(x+xi, y+yi, (tTile){Bottom: type->RubbleTiles[xi][yi], 
		                            Seen: getSafe(x+xi, y+yi).Seen});
}
