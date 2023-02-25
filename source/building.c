#include "string.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/building.h"
#include "source/map.h"

u32 AllocatedBuildings, BuildingPtr;
tBuilding *Buildings;

tBuildingType Farm = {
	Name: "Farm",
	MaxHealth: 100,
	ViewDistance: 0,
	BlockMovement: false,
	SizeX: 2, SizeY: 2,
	Tiles: {{0xa0, 0xb0},
	        {0xa1, 0xb1}}
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
	if(!Buildings[ret].Health) Buildings[ret].Health = build.Type->MaxHealth;

	for(i32 xi = 0; xi < build.Type->SizeX; xi++) for(i32 yi = 0; yi < build.Type->SizeY; yi++)
		setSafe(x+xi, y+yi, (tTile){
			Bottom: build.Type->Tiles[xi][yi], 
			Seen: true, 
			Building: ret, 
			Move: build.Type->BlockMovement ? 0xff : 0});

	return ret;
}

void destroyBuilding(BuildingHandle build, i32 x, i32 y) {
	Buildings[build].Exists = false;
	for(i32 xi = -3; xi < 3; xi++) for(i32 yi = -3; yi < 3; yi++) {
		if(getSafe(x+xi, y+yi).Building == build) setSafe(x+xi, y+yi, (tTile){Bottom: 6});
	}
}
