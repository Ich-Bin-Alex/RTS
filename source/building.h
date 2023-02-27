#ifndef H_BUILDING
#define H_BUILDING

#include "source/tools/helper.h"
#include "source/game.h"

typedef u32 BuildingHandle;

typedef struct tBuildingType {
	char Name[32];
	u32 MaxHealth;
	u32 FoodCost, WoodCost;
	i32 ViewDistance;
	bool BlockMovement;
	i32 SizeX, SizeY;
	u8 Tiles[3][3];
} tBuildingType;

extern tBuildingType Farm;

typedef struct tBuilding {
	bool Exists;
	tBuildingType *Type;
	PlayerHandle Player;
	i32 Health;
	union {
		struct {
			bool Occupied;
		} Farm;
	};
} tBuilding;

extern tBuilding *Buildings;
extern u32 AllocatedBuildings, BuildingPtr;

BuildingHandle newBuilding(tBuilding build, u32 x, u32 y);
void destroyBuilding(BuildingHandle build, i32 x, i32 y);

#endif