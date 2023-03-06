#ifndef H_BUILDING
#define H_BUILDING

#include "source/tools/helper.h"
#include "source/game.h"

typedef u32 BuildingHandle;

typedef struct tBuildingType {
	char Name[32];
	u32 MaxHealth;
	u32 FoodCost, WoodCost;
	i32 Population;
	i32 ViewDistance;
	bool BlockMovement;
	i32 SizeX, SizeY;
	u8 Icon;
	u8 BottomTiles[3][3], TopTiles[3][3], RubbleTiles[3][3], ConstructTiles[3][3];
} tBuildingType;

extern tBuildingType Farm, House;

typedef struct tBuilding {
	bool Exists, Finished;
	tBuildingType *Type;
	PlayerHandle Player;
	i32 Health, FirstX, FirstY;
	union {
		struct {
			u32 Occupier;
		} Farm;
		struct {
			u32 Occupier;
		} Build;
	};
} tBuilding;

extern tBuilding *Buildings;
extern u32 AllocatedBuildings, BuildingPtr;

BuildingHandle newBuilding(tBuilding build, u32 x, u32 y);
void destroyBuilding(BuildingHandle build);

#endif