#ifndef H_UNIT
#define H_UNIT

#include "source/tools/helper.h"
#include "source/map.h"

#define MAX_UNITS 200

typedef u32 UnitHandle;

typedef struct tFlow {
	i8 X : 4;
	i8 Y : 4;
} tFlow;

typedef struct tMoveOrder {
	i32 References;
	Vector2 Target;
	UnitHandle Follow;
	bool AttackOnSight;
	u32 LastUpdate;
	tFlow Flow[MAP_SIZE][MAP_SIZE];
} tMoveOrder;

tMoveOrder *newMoveOrder(tMoveOrder order);
void updateMoveOrder(tMoveOrder *order);
void drawMoveOrder(tMoveOrder *order);

typedef struct tUnitType {
	char Name[32];
	u8 MaxHealth;
	u8 Attack;
	f32 Speed;
	i32 ViewDistance;
} tUnitType;

typedef enum eAction {
	ACTION_MOVE,
	ACTION_ATTACK
} eAction;

extern tUnitType Peasent;

typedef struct tUnit {
	bool Alive;
	tUnitType *Type;
	eAction Action;
	PlayerHandle Player;
	UnitHandle Attack;
	Vector2 Position, Speed;
	bool Selected;
	i32 Health;
	tMoveOrder *MoveOrder;
	f32 IdleTimer, AttackTimer;
	u8 Direction, Animation;
} tUnit;

extern tUnit Units[MAX_UNITS];

UnitHandle newUnit(tUnit unit);
void removeUnit(UnitHandle unit);
Vector2 getUnitFlow(UnitHandle unit);
void drawUnits();
void updateUnits();
void moveUnit(UnitHandle unit, tMoveOrder *order);

#endif