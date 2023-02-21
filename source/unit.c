#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"

tUnitType Peasent = {
	Name: "Peasent",
	MaxHealth: 10,
	Attack: 1,
	Speed: 4.0,
	ViewDistance: 7,
};

static void updateFlow(tMoveOrder *order) {
	static u16 VisitX[MAP_SIZE*MAP_SIZE], VisitY[MAP_SIZE*MAP_SIZE], VisitDist[MAP_SIZE*MAP_SIZE];
	static u16 Grid[MAP_SIZE][MAP_SIZE];
	i32 stackPtr = 0;

	void push(u16 x, u16 y, u16 distance) {
		VisitX[stackPtr] = x;
		VisitY[stackPtr] = y;
		VisitDist[stackPtr] = distance;
		stackPtr++;
	}

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++)
		Grid[x][y] = Map[x][y].Move ? 0xffff : 0;
	push(order->Target.x, order->Target.y, Grid[(u16)order->Target.x][(u16)order->Target.y] = 1);

	for(i32 i = 0; i < stackPtr; i++) {
		u16 x = VisitX[i], y = VisitY[i], distance = VisitDist[i] + 1;
		if(x > 0 && !Grid[x-1][y]) push(x-1, y, Grid[x-1][y] = distance);
		if(y > 0 && !Grid[x][y-1]) push(x, y-1, Grid[x][y-1] = distance);
		if(x < MAP_SIZE-1 && !Grid[x+1][y]) push(x+1, y, Grid[x+1][y] = distance);
		if(y < MAP_SIZE-1 && !Grid[x][y+1]) push(x, y+1, Grid[x][y+1] = distance);	
	}

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		if(Grid[x][y] == 0xffff || !Grid[x][y]) {
			order->Flow[x][y] = (tFlow){0,0};
			continue;
		}
		bool topValid = y < MAP_SIZE-1 && Grid[x][y-1] != 0xffff;
		bool bottomValid = y < MAP_SIZE-1 && Grid[x][y+1] != 0xffff;
		bool leftValid = x < MAP_SIZE-1 && Grid[x-1][y] != 0xffff;
		bool rightValid = x < MAP_SIZE-1 && Grid[x+1][y] != 0xffff;
		i32 cost = 0xffff;
		order->Flow[x][y] = (tFlow){0,0};
		if(topValid && Grid[x][y-1] < cost) {
			cost = Grid[x][y-1];
			order->Flow[x][y] = (tFlow){0,-1};
		}
		if(bottomValid && Grid[x][y+1] < cost) {
			cost = Grid[x][y+1];
			order->Flow[x][y] = (tFlow){0,+1};
		}
		if(leftValid && Grid[x-1][y] < cost) {
			cost = Grid[x-1][y];
			order->Flow[x][y] = (tFlow){-1,0};
		}
		if(rightValid && Grid[x+1][y] < cost) {
			cost = Grid[x+1][y];
			order->Flow[x][y] = (tFlow){+1,0};
		}
		if(topValid && leftValid && Grid[x-1][y-1] < cost) {
			cost = Grid[x-1][y-1];
			order->Flow[x][y] = (tFlow){-1,-1};
		}
		if(topValid && rightValid && Grid[x+1][y-1] < cost) {
			cost = Grid[x+1][y-1];
			order->Flow[x][y] = (tFlow){+1,-1};
		}
		if(bottomValid && leftValid && Grid[x-1][y+1] < cost) {
			cost = Grid[x-1][y+1];
			order->Flow[x][y] = (tFlow){-1,+1};
		}
		if(bottomValid && rightValid && Grid[x+1][y+1] < cost) {
			cost = Grid[x+1][y+1];
			order->Flow[x][y] = (tFlow){+1,+1};
		}
	}
}

tMoveOrder *newMoveOrder(tMoveOrder order) {
	if((u32)order.Target.x >= MAP_SIZE-1 || (u32)order.Target.y >= MAP_SIZE-1) return NULL;

	tMoveOrder *ret = calloc(sizeof(tMoveOrder), 1);
	*ret = order;
	ret->LastUpdate = FrameCount;
	updateFlow(ret);

	return ret;
}

void updateMoveOrder(tMoveOrder *order) {
	if(!order->Follow || order->LastUpdate == FrameCount) return;

	order->Target = Units[order->Follow].Position;
	updateFlow(order);
}

void drawMoveOrder(tMoveOrder *order) {
	if(!order) return;
	DrawCircle(order->Target.x*8*DRAW_SIZE+4*DRAW_SIZE-CameraX, 
		order->Target.y*8*DRAW_SIZE+4*DRAW_SIZE-CameraY, 4, RED);

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		DrawLine(x*8*DRAW_SIZE+4*DRAW_SIZE-CameraX, y*8*DRAW_SIZE+4*DRAW_SIZE-CameraY, 
		x*8*DRAW_SIZE+4*DRAW_SIZE+order->Flow[x][y].X*4*DRAW_SIZE-CameraX, 
		y*8*DRAW_SIZE+4*DRAW_SIZE+order->Flow[x][y].Y*4*DRAW_SIZE-CameraY, WHITE);
	}
}

tUnit Units[MAX_UNITS];

UnitHandle newUnit(tUnit unit) {
	UnitHandle ret = 0;
	for(UnitHandle i = 1; i < MAX_UNITS; i++)
		if(!Units[i].Alive) ret = i;
	Units[ret] = unit;
	Units[ret].Animation = GetRandomValue(0, 3);
	Units[ret].IdleTimer = GetTime() + GetRandomValue(-4000,4000) / 1000.0;
	Units[ret].Alive = true;
	if(!Units[ret].Health) Units[ret].Health = unit.Type->MaxHealth;
	return ret;
}

void removeUnit(UnitHandle unit) {
	Units[unit].Alive = false;
}

Vector2 getUnitFlow(UnitHandle unit) {
	Vector2 getFlow(u32 x, u32 y) {
		if(x >= MAP_SIZE || y >= MAP_SIZE) return (Vector2){0};
		return (Vector2){Units[unit].MoveOrder->Flow[x][y].X, Units[unit].MoveOrder->Flow[x][y].Y};
	}

	if(!Units[unit].MoveOrder) return (Vector2){0,0};

	u32 x = floor(Units[unit].Position.x), y = floor(Units[unit].Position.y);
	Vector2 m = getFlow(x,y), s = getFlow(x,y+1), e = getFlow(x+1,y), se = getFlow(x+1,y+1);
	f32 xWeight = Units[unit].Position.x - (f32)x, yWeight = Units[unit].Position.y - (f32)y;

	Vector2 top = Vector2Add(Vector2Scale(m, 1.0 - xWeight), Vector2Scale(e, xWeight));
	Vector2 bottom = Vector2Add(Vector2Scale(s, 1.0 - xWeight), Vector2Scale(se, xWeight));

	Vector2 move = Vector2Normalize
		(Vector2Add(Vector2Scale(top, 1.0 - yWeight), Vector2Scale(bottom, yWeight)));
	return move;
}

void drawUnits() {	
	for(UnitHandle i = 1; i < MAX_UNITS; i++) {
		if(!Units[i].Alive) continue;
		if(Units[i].Player && !getSafe(Units[i].Position.x, Units[i].Position.y).Seen) continue;
		switch(Units[i].Action) {
		case ACTION_MOVE: {
			i32 move = (Units[i].Animation + (i32)(GetTime() * Vector2Length(Units[i].Speed)*2.0)) % 4;
			i32 anim = Units[i].Speed.x || Units[i].Speed.y ? move : 0;
			DrawTexturePro(Sprites[Player[Units[i].Player].Color], 
				(Rectangle){anim*8,Units[i].Direction*8,8,8}, 
				toMapR(Units[i].Position.x, Units[i].Position.y), (Vector2){0}, 0.0, WHITE);
			} break;
		case ACTION_ATTACK: {
			i32 anim = (Units[i].Animation + (i32)(GetTime() * 8.0)) % 4;
			DrawTexturePro(Sprites[Player[Units[i].Player].Color], 
				(Rectangle){anim*8,32 + Units[i].Direction*8,8,8}, 
				toMapR(Units[i].Position.x, Units[i].Position.y), (Vector2){0}, 0.0, WHITE);
			} break;
		}
		//i32 anim2 = (unit->Animation + (i32)(GetTime() * 8.0)) % 4;
		//DrawTexturePro(UnitSprites, (Rectangle){anim2*8,7*8,8,8}, 
		//	toMapR(unit->Position.x, unit->Position.y), (Vector2){0}, 0.0, WHITE);
	}
}

void updateUnits() {
	for(UnitHandle i = 1; i < MAX_UNITS; i++) {
		if(!Units[i].Alive) continue;
		u32 x = round(Units[i].Position.x), y = round(Units[i].Position.y);
		if(Units[i].Player == 0) {
			for(i32 yi = -Units[i].Type->ViewDistance; yi <= Units[i].Type->ViewDistance; yi++) 
			for(i32 xi = -Units[i].Type->ViewDistance; xi <= Units[i].Type->ViewDistance; xi++)
				if(xi*xi + yi*yi <= Units[i].Type->ViewDistance*Units[i].Type->ViewDistance) {
					if((u32)(x+xi) < MAP_SIZE && (u32)(y+yi) < MAP_SIZE) Map[x+xi][y+yi].Seen = true;
				}
		}
		if(Units[i].Health <= 0) {
			Map[x][y].Animation = 0xf0 + GetRandomValue(0, 1)*8;
			Map[x][y].Frame = GetRandomValue(0, 1);
			removeUnit(i);
			continue;
		}
		
		bool bumbed = false;
		if(!Units[i].MoveOrder) Units[i].Speed = (Vector2){0};
		Vector2 oldPos = Units[i].Position;
		UnitHandle NextEnemy = 0;
		for(UnitHandle j = 1; j < MAX_UNITS; j++) {
			if(!Units[j].Alive || i == j) continue;
			Vector2 axis = Vector2Subtract(Units[i].Position, Units[j].Position);
			f32 dist = sqrtf((axis.x*axis.x) + (axis.y*axis.y));
			if(dist < 0.8) {
				//if(unit2->Player != unit->Player) unit->Speed = unit2->Speed = (Vector2){0};
				Vector2 delta = Vector2Scale((Vector2){axis.x/dist,axis.y/dist}, 0.5*(0.8 - dist));
				u32 x2 = round(Units[i].Position.x+delta.x), y2 = round(Units[i].Position.y+delta.y);
				if(!getSafe(x2, y2).Move)
					Units[i].Position = Vector2Add(Units[i].Position, delta);
				u32 x3 = round(Units[j].Position.x-delta.x), y3 = round(Units[j].Position.y-delta.y);
				if(!getSafe(x3, y3).Move && !Units[j].MoveOrder && Units[j].Player == Units[i].Player)
					Units[j].Position = Vector2Subtract(Units[j].Position, delta);
				bumbed = true;
			} else if(dist < 1.4) {
				if(Units[j].Player != Units[i].Player && !Units[i].Attack) {
					Units[i].Speed = Vector2Scale(Units[i].Speed, 0.025);
					Units[j].Speed = Vector2Scale(Units[j].Speed, 0.025);
					Units[i].Attack = j;
					Units[i].Action = ACTION_ATTACK;
				}
			} else if(Units[j].Player != Units[i].Player && dist < Units[i].Type->ViewDistance) 
				NextEnemy = j;
		}

		if(Units[i].Attack) {
			u32 x2 = round(Units[Units[i].Attack].Position.x);
			u32 y2 = round(Units[Units[i].Attack].Position.y);
			if(x2 < MAP_SIZE && y2 < MAP_SIZE && !Map[x2][y2].Animation) { // Bloodsplatter animation
				Map[x2][y2].Animation = 0xf0 + GetRandomValue(0, 1)*8;
				Map[x2][y2].Frame = GetRandomValue(0, 1);
				Map[x2][y2].Blood = 1.0;
			}

			Vector2 axis = Vector2Subtract(Units[i].Position, Units[Units[i].Attack].Position);
			f32 dist = sqrtf((axis.x*axis.x) + (axis.y*axis.y));
			f32 angle = fmod(2*PI + atan2(axis.y, axis.x), 2*PI);
			if     (angle >= PI/2*3.5 || angle < PI/2*0.5) Units[i].Direction = 3;
			else if(angle >= PI/2*0.5 && angle < PI/2*1.5) Units[i].Direction = 2;
			else if(angle >= PI/2*1.5 && angle < PI/2*2.5) Units[i].Direction = 1;
			else if(angle >= PI/2*2.5 && angle < PI/2*3.5) Units[i].Direction = 0;
			if(dist >= 1.4 || !Units[Units[i].Attack].Alive) {
				Units[i].Attack = 0;
				Units[i].Action = ACTION_MOVE;
			} else {
				if(Units[i].AttackTimer < GetTime() - 0.6) {
					Units[Units[i].Attack].Health -= Units[i].Type->Attack;
					Units[i].AttackTimer = GetTime();
				}
			}
		}
		
		Units[i].Speed = Vector2Scale(Units[i].Speed, GetFrameTime());
		Units[i].Position = Vector2Add(Units[i].Position, Units[i].Speed);
		Vector2 movement = Vector2Subtract(Units[i].Position, oldPos);

		if(fabsf(movement.x + movement.y) > 0.001) {
			f32 angle = fmod(2*PI + atan2(movement.y, movement.x), 2*PI);
			if     (angle >= PI/2*3.5 || angle < PI/2*0.5) Units[i].Direction = 1;
			else if(angle >= PI/2*0.5 && angle < PI/2*1.5) Units[i].Direction = 0;
			else if(angle >= PI/2*1.5 && angle < PI/2*2.5) Units[i].Direction = 3;
			else if(angle >= PI/2*2.5 && angle < PI/2*3.5) Units[i].Direction = 2;
		}

		if(!Units[i].MoveOrder) { // Idle animation
			if(!NextEnemy && Units[i].IdleTimer < GetTime() - 8.0) {
				Units[i].Direction = GetRandomValue(0,3);
				Units[i].IdleTimer = GetTime() + GetRandomValue(-4000, 4000) / 1000.0;
				continue;
			} else if(NextEnemy) {
				tMoveOrder *move = newMoveOrder((tMoveOrder){Follow: NextEnemy});
				moveUnit(i, move);
			} else continue;
		}
		if(Units[i].MoveOrder->Follow && !Units[Units[i].MoveOrder->Follow].Alive) {
			moveUnit(i, NULL);
			continue;
		}
		if(Units[i].Player && !Units[i].MoveOrder->Follow && NextEnemy) {
			tMoveOrder *move = newMoveOrder((tMoveOrder){Follow: NextEnemy});
			moveUnit(i, move);
		}

		updateMoveOrder(Units[i].MoveOrder);
		Units[i].Speed = Vector2Scale(getUnitFlow(i), Units[i].Type->Speed - bumbed);

		f32 dist = Units[i].MoveOrder->Follow ? 0.3 : 0.5 + bumbed;

		if(Vector2Distance(Units[i].Position, Units[i].MoveOrder->Target) < dist) {
			for(UnitHandle j = 1; j < MAX_UNITS; j++) {
				if(!Units[j].Alive) continue;
				if(Vector2Distance(Units[i].Position, Units[j].Position) < 3.0) {
					if(i != j && Units[i].MoveOrder == Units[j].MoveOrder) moveUnit(j, NULL);
				}
			}
			moveUnit(i, NULL);
		}
	}
}

void moveUnit(UnitHandle unit, tMoveOrder *order) {
	if(order) order->References++;
	if(Units[unit].MoveOrder)
		if(--Units[unit].MoveOrder->References <= 0) free(Units[unit].MoveOrder);
	Units[unit].Speed = (Vector2){0};
	Units[unit].MoveOrder = order;
}
