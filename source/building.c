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
	MaxHealth: 50,
	WoodCost: 50,
	ViewDistance: 0,
	BlockMovement: false,
	SizeX: 2, SizeY: 2,
	Icon: 0xd6,
	BottomTiles: {{0x70, 0x80},
	              {0x71, 0x81}},
	RubbleTiles: {{0x50, 0x60},
	              {0x51, 0x61}},
	ConstructTiles: {{0x90, 0xa0},
	                 {0x91, 0xa1}}
}, House = {
	Name: "House",
	MaxHealth: 150,
	WoodCost: 100,
	Population: +5,
	ViewDistance: 1,
	BlockMovement: true,
	SizeX: 2, SizeY: 2,
	Icon: 0xd7,
	BottomTiles: {{0, 0x82},
	              {0, 0x83}},
	TopTiles: {{0x72, 0},
	           {0x73, 0}},
	RubbleTiles: {{0x52, 0x62},
	              {0x53, 0x63}},
	ConstructTiles: {{0x92, 0xa2},
	                 {0x93, 0xa3}}
};;

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
	build.Exists = true;
	build.FirstX = x;
	build.FirstY = y;
	if(!build.Health && build.Finished) build.Health = build.Type->MaxHealth;
	Buildings[ret] = build;

	for(i32 xi = 0; xi < build.Type->SizeX; xi++) for(i32 yi = 0; yi < build.Type->SizeY; yi++)
		setSafe(x+xi, y+yi, (tTile){
			Bottom: build.Finished ? build.Type->BottomTiles[xi][yi] : build.Type->ConstructTiles[xi][yi],
			Top: build.Finished ? build.Type->TopTiles[xi][yi] : 0,
			Seen: getSafe(x+xi, y+yi).Seen,
			Building: ret,
			Move: build.Type->BlockMovement ? 0xff : 0});

	if(build.Finished) Player[build.Player].PopulationLimit += build.Type->Population; 

	return ret;
}

void destroyBuilding(BuildingHandle build) {
	i32 x = Buildings[build].FirstX, y = Buildings[build].FirstY;
	tBuildingType *type = Buildings[build].Type;
	Buildings[build].Exists = false;
	if(Buildings[build].Finished) {
		if(Buildings[build].Type == &Farm) Units[Buildings[build].Farm.Occupier].Action = ACTION_MOVE;
	} else Units[Buildings[build].Build.Occupier].Action = ACTION_MOVE;
	for(i32 xi = 0; xi < type->SizeX; xi++) for(i32 yi = 0; yi < type->SizeY; yi++)
		setSafe(x+xi, y+yi, (tTile){Bottom: type->RubbleTiles[xi][yi], 
		                            Seen: getSafe(x+xi, y+yi).Seen});
}
