#ifndef H_UNIT
#define H_UNIT

#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/building.h"

#define forEachUnit(x) for(UnitHandle x = 1; x < UnitPtr; x++) if(Units[x].Alive)

typedef u32 UnitHandle;

typedef struct tFlow {
	i8 X : 4;
	i8 Y : 4;
} tFlow;

typedef struct tMoveOrder {
	i32 References;
	Vector2 Target;
	UnitHandle Follow;
	bool OnlySeen;
	bool AttackOnSight;
	u32 LastUpdate;
	u8 Move[MAP_SIZE][MAP_SIZE];
	tFlow Flow[MAP_SIZE][MAP_SIZE];
} tMoveOrder;

extern u32 NumMoveOrders;

tMoveOrder *newMoveOrder(tMoveOrder order);
void freeMoveOrder(tMoveOrder *order);
void updateMoveOrder(tMoveOrder *order, i32 time);
void drawMoveOrder(tMoveOrder *order);

typedef struct tUnitType {
	char Name[32];
	u32 MaxHealth;
	u32 Attack;
	i32 ViewDistance;
	f32 Speed;
	u32 FoodCost, WoodCost;
	bool CanChop, CanFarm, CanBuild;
	tBuildingType *Buildings[];
} tUnitType;

typedef enum eAction {
	ACTION_MOVE,
	ACTION_ATTACK,
	ACTION_CHOP, ACTION_MOVE_AND_CHOP,
	ACTION_FARM, ACTION_MOVE_AND_FARM,
	ACTION_BUILD, ACTION_MOVE_AND_BUILD,
	ACTION_MOVE_FREE,
} eAction;

extern tUnitType Peasent;

typedef struct tUnit {
	bool Alive;
	tUnitType *Type;
	eAction Action;
	PlayerHandle Player;
	Vector2 Position, Speed;
	bool Selected;
	i32 Health;
	tMoveOrder *MoveOrder;
	f32 IdleTimer;
	union {
		struct {
			f32 Timer;
			UnitHandle Unit;
		} Attack;
		struct {
			f32 Timer;
			u32 TreeX, TreeY, IgnoreTreeX, IgnoreTreeY, SearchTreeX, SearchTreeY;
		} Chop;
		struct {
			f32 Timer;
			Vector2 Target;
			BuildingHandle Building;
		} Farm;
		struct {
			f32 Timer;
			Vector2 Target;
			BuildingHandle Building;
		} Build;
	};
	u8 Direction, Animation;
	u32 Unmoveable; // How many frames the unit didn't move. Used to prevent it from getting stuck
} tUnit;

extern tUnit *Units;
extern u32 NumUnits, AllocatedUnits, UnitPtr;

UnitHandle newUnit(tUnit unit);
void killUnit(UnitHandle unit);
void unitAction(UnitHandle unit, eAction action, Vector2 target, UnitHandle enemy);
Vector2 getUnitFlow(UnitHandle unit);
void drawUnits(void);
void updateUnits(void);
void moveUnit(UnitHandle unit, tMoveOrder *order);

#endif