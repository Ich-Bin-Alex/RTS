#ifndef H_UNIT
#define H_UNIT

#include "source/tools/helper.h"
#include "source/map.h"

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
	tFlow Flow[MAP_SIZE][MAP_SIZE];
} tMoveOrder;

extern u32 NumMoveOrders;

tMoveOrder *newMoveOrder(tMoveOrder order);
void freeMoveOrder(tMoveOrder *order);
void updateMoveOrder(tMoveOrder *order);
void drawMoveOrder(tMoveOrder *order);

typedef struct tUnitType {
	char Name[32];
	u8 MaxHealth;
	u8 Attack;
	f32 Speed;
	i32 ViewDistance;
	bool CanChop, CanFarm, CanBuild;
} tUnitType;

typedef enum eAction {
	ACTION_MOVE,
	ACTION_ATTACK,
	ACTION_CHOP_TREE, ACTION_MOVE_AND_CHOP,
	ACTION_FARM,
	ACTION_BUILD
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
			f32 Timer, Distance;
			u32 TreeX, TreeY, IgnoreTreeX, IgnoreTreeY, SearchTreeX, SearchTreeY;
		} Chop;
	};
	u8 Direction, Animation;
	u32 Unmoveable; // How many frames the unit didn't move. Used to prevent it from getting stuck
} tUnit;

extern tUnit *Units;
extern u32 NumUnits, AllocatedUnits, UnitPtr;

UnitHandle newUnit(tUnit unit);
Vector2 getUnitFlow(UnitHandle unit);
void drawUnits(void);
void updateUnits(void);
void moveUnit(UnitHandle unit, tMoveOrder *order);

#endif