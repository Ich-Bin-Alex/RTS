#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/vector.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"
#include "source/building.h"

u32 NumMoveOrders, NumUnits, AllocatedUnits, UnitPtr;
tUnit *Units;

tUnitType Peasent = {
	Name: "Peasent",
	FoodCost: 50,
	MaxHealth: 10,
	Attack: 1,
	Speed: 4.0,
	ViewDistance: 7,
	CanChop: true, CanFarm: true, CanBuild: true,
	Buildings: {&Farm, &House, NULL}
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

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		if(!order->OnlySeen) Grid[x][y] = Map[x][y].Move ? 0xffff : 0;
		else Grid[x][y] = Map[x][y].Move || !Map[x][y].Seen ? 0xffff : 0;
		order->Move[x][y] = Map[x][y].Move;
	}
	push(order->Target.x, order->Target.y, Grid[(u16)order->Target.x][(u16)order->Target.y] = 1);

	for(i32 i = 0; i < stackPtr; i++) {
		u16 x = VisitX[i], y = VisitY[i], distance = VisitDist[i] + 1;
		if(x > 0          && !Grid[x-1][y]) push(x-1, y  , Grid[x-1][y] = distance);
		if(y > 0          && !Grid[x][y-1]) push(x  , y-1, Grid[x][y-1] = distance);
		if(x < MAP_SIZE-1 && !Grid[x+1][y]) push(x+1, y  , Grid[x+1][y] = distance);
		if(y < MAP_SIZE-1 && !Grid[x][y+1]) push(x  , y+1, Grid[x][y+1] = distance);	
	}

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		if(Grid[x][y] == 0xffff || !Grid[x][y]) {
			order->Flow[x][y] = (tFlow){0,0};
			continue;
		}
		u16 cost = 0xffff;
		bool top  = y > 0 && Grid[x][y-1] != cost, bottom = y < MAP_SIZE-1 && Grid[x][y+1] != cost;
		bool left = x > 0 && Grid[x-1][y] != cost, right  = x < MAP_SIZE-1 && Grid[x+1][y] != cost;
		order->Flow[x][y] = (tFlow){0,0};
		if(top && Grid[x][y-1] < cost) {
			cost = Grid[x][y-1];
			order->Flow[x][y] = (tFlow){0,-1};
		}
		if(bottom && Grid[x][y+1] < cost) {
			cost = Grid[x][y+1];
			order->Flow[x][y] = (tFlow){0,+1};
		}
		if(left && Grid[x-1][y] < cost) {
			cost = Grid[x-1][y];
			order->Flow[x][y] = (tFlow){-1,0};
		}
		if(right && Grid[x+1][y] < cost) {
			cost = Grid[x+1][y];
			order->Flow[x][y] = (tFlow){+1,0};
		}
		if(top && left && Grid[x-1][y-1] < cost) {
			cost = Grid[x-1][y-1];
			order->Flow[x][y] = (tFlow){-1,-1};
		}
		if(top && right && Grid[x+1][y-1] < cost) {
			cost = Grid[x+1][y-1];
			order->Flow[x][y] = (tFlow){+1,-1};
		}
		if(bottom && left && Grid[x-1][y+1] < cost) {
			cost = Grid[x-1][y+1];
			order->Flow[x][y] = (tFlow){-1,+1};
		}
		if(bottom && right && Grid[x+1][y+1] < cost) {
			cost = Grid[x+1][y+1];
			order->Flow[x][y] = (tFlow){+1,+1};
		}
	}

	order->LastUpdate = FrameCount;
}

static bool collide(UnitHandle unit, vec2 move) {
	u32 x = round(Units[unit].Position.x + move.x), y = round(Units[unit].Position.y + move.y);
	if(getSafe(x, round(Units[unit].Position.y)).Move) return true;
	if(getSafe(round(Units[unit].Position.x), y).Move) return true;
	return false;
}

tMoveOrder *newMoveOrder(tMoveOrder order) {
	if((u32)order.Target.x >= MAP_SIZE-1 || (u32)order.Target.y >= MAP_SIZE-1) return NULL;

	tMoveOrder *ret = calloc(sizeof(tMoveOrder), 1);
	*ret = order;
	updateFlow(ret);
	NumMoveOrders++;

	return ret;
}

void freeMoveOrder(tMoveOrder *order) {
	free(order);
	NumMoveOrders--;
}

void updateMoveOrder(tMoveOrder *order, i32 time) {
	if(FrameCount - order->LastUpdate < time) return;
	order->Target = Units[order->Follow].Position;
	updateFlow(order);
}

void drawMoveOrder(tMoveOrder *order) {
	if(!order) return;
	DrawCircle(order->Target.x*8*DrawSize+4*DrawSize-CameraX,
		order->Target.y*8*DrawSize+4*DrawSize-CameraY, 4, RED);

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		DrawLine(x*8*DrawSize+4*DrawSize-CameraX, y*8*DrawSize+4*DrawSize-CameraY,
		x*8*DrawSize+4*DrawSize+order->Flow[x][y].X*4*DrawSize-CameraX,
		y*8*DrawSize+4*DrawSize+order->Flow[x][y].Y*4*DrawSize-CameraY, WHITE);
	}
}

UnitHandle newUnit(tUnit unit) {
	UnitHandle ret = 0;
	for(UnitHandle i = 1; i < UnitPtr; i++) if(!Units[i].Alive) {
		ret = i;
		break;
	}
	if(!ret) ret = ++UnitPtr;
	if(UnitPtr >= AllocatedUnits) {
		AllocatedUnits += 64;
		Units = realloc(Units, AllocatedUnits*sizeof(tUnit));
		memset(&Units[UnitPtr-1], 0, 64*sizeof(tUnit));
	}
	unit.Animation = GetRandomValue(0, 3);
	unit.IdleTimer = GetTime() + GetRandomValue(-4000,4000) / 1000.0;
	unit.Alive = true;
	if(!unit.Health) unit.Health = unit.Type->MaxHealth;
	Units[ret] = unit;
	Player[unit.Player].Population++;
	NumUnits++;
	return ret;
}

void killUnit(UnitHandle unit) {
	Units[unit].Alive = Units[unit].Selected = false;
	tTile *tile = refSafe(round(Units[unit].Position.x), round(Units[unit].Position.y));
	tile->Animation = 0xf0 + GetRandomValue(0, 1)*8; // Blood animation
	tile->Frame = GetRandomValue(0, 1);
	moveUnit(unit, NULL);
	unitAction(unit, 0, vec2(0), 0);
	Player[Units[unit].Player].Population--;
	NumUnits--;
}

void unitAction(UnitHandle unit, eAction action, vec2 target, UnitHandle enemy) {
	BuildingHandle oldFarm = Units[unit].Farm.Building, oldBuild = Units[unit].Build.Building;
	tTile tile = getSafe(target.x, target.y);

	if((Units[unit].Action == ACTION_CHOP || Units[unit].Action == ACTION_MOVE_AND_CHOP) &&
	   Units[unit].Chop.TreeX && getSafe(Units[unit].Chop.TreeX,Units[unit].Chop.TreeY).OccupiedTree 
	   && action != ACTION_CHOP)
		refSafe(Units[unit].Chop.TreeX, Units[unit].Chop.TreeY)->OccupiedTree--;
	else if(Units[unit].Action == ACTION_FARM || Units[unit].Action == ACTION_MOVE_AND_FARM)
		Buildings[Units[unit].Farm.Building].Farm.Occupier = 0;
	else if(Units[unit].Action == ACTION_BUILD || Units[unit].Action == ACTION_MOVE_AND_BUILD)
		Buildings[Units[unit].Build.Building].Build.Occupier = 0;

	Units[unit].Action = action;
	switch(action) {
	case ACTION_ATTACK:
		Units[unit].Attack.Unit = enemy;
		break;
	case ACTION_CHOP:
		Units[unit].Chop.TreeX = Units[unit].Chop.TreeY = 0;
		Units[unit].Chop.Timer = GetTime() + (f32)GetRandomValue(0, 500)/100.0;
		break;
	case ACTION_FARM:
		Units[unit].Farm.Timer = GetTime() + (f32)GetRandomValue(0, 500)/100.0;
		Units[unit].Farm.Target = target;
		Buildings[oldFarm].Farm.Occupier = unit;
		break;
	case ACTION_BUILD:
		Buildings[oldBuild].Build.Occupier = unit;
		break;
	case ACTION_MOVE_AND_CHOP:
		Units[unit].Chop.IgnoreTreeX = Units[unit].Chop.IgnoreTreeY = 0;
		Units[unit].Chop.SearchTreeX = target.x;
		Units[unit].Chop.SearchTreeY = target.y;
		break;
	case ACTION_MOVE_AND_FARM:
		Units[unit].Farm.Target = target;
		Units[unit].Farm.Building = tile.Building;
		Buildings[tile.Building].Farm.Occupier = unit;
		break;
	case ACTION_MOVE_AND_BUILD:
		Units[unit].Build.Target = target;
		Units[unit].Build.Building = tile.Building;
		Buildings[tile.Building].Build.Occupier = unit;
		break;
	case ACTION_MOVE_FREE:
		Units[unit].Build.Target = target;
		break;
	default: break;
	}
}

vec2 getUnitFlow(UnitHandle unit) {
	vec2 getFlow(u32 x, u32 y) {
		if(x >= MAP_SIZE || y >= MAP_SIZE) return vec2(0);
		return vec2(Units[unit].MoveOrder->Flow[x][y].X, Units[unit].MoveOrder->Flow[x][y].Y);
	}

	if(!Units[unit].MoveOrder) return vec2(0);

	u32 x = floor(Units[unit].Position.x), y = floor(Units[unit].Position.y);
	vec2 m = getFlow(x, y), s = getFlow(x, y+1), e = getFlow(x+1, y), se = getFlow(x+1, y+1);
	f32 xWeight = Units[unit].Position.x - (f32)x, yWeight = Units[unit].Position.y - (f32)y;

	vec2 top    = vadd(vmul(m, 1.0 - xWeight), vmul(e, xWeight));
	vec2 bottom = vadd(vmul(s, 1.0 - xWeight), vmul(se, xWeight));

	return vnormalize(vadd(vmul(top, 1.0 - yWeight), vmul(bottom, yWeight)));
}

void drawUnits(void) {
	forEachUnit(i) {
		if(Units[i].Player && !getSafe(Units[i].Position.x, Units[i].Position.y).Seen) continue;
		i32 anim = 0, offset = 0;
		switch(Units[i].Action) {
		case ACTION_MOVE: case ACTION_MOVE_AND_CHOP: case ACTION_MOVE_FREE:
		case ACTION_MOVE_AND_FARM: case ACTION_MOVE_AND_BUILD: lMove:
			if(Units[i].Speed.x || Units[i].Speed.y)
				anim = (Units[i].Animation + floor(GetTime()*vlength(Units[i].Speed)*2.0));
			break;
		case ACTION_ATTACK:
			anim = (Units[i].Animation + floor(GetTime() * 8.0));
			offset = 4;
			break;
		case ACTION_CHOP: // Reuse attack animation
			if(Units[i].Speed.x || Units[i].Speed.y) goto lMove;
			if(Units[i].Chop.TreeX) {
				tTile *tile = refSafe(Units[i].Chop.TreeX, Units[i].Chop.TreeY);
				anim = (Units[i].Animation + floor(GetTime() * 5.0));
				offset = 4;
				if(anim % 4 == 2 && (tile->Frame >= 8 || !tile->Frame)) {
					tile->Animation = 0xd0;
					tile->Frame = 0;
				}
			}
			break;
		case ACTION_FARM:
			if(Units[i].Speed.x || Units[i].Speed.y) goto lMove;
			anim = (Units[i].Animation + floor(GetTime() * 6.0));
			offset = 8;
			break;
		case ACTION_BUILD:
			if(Units[i].Speed.x || Units[i].Speed.y) goto lMove;
			anim = (Units[i].Animation + floor(GetTime() * 8.0));
			offset = 9;
			break;
		}
		drawSprite(Units[i].Position, Units[i].Player, anim % 4, offset + Units[i].Direction);
	}
}

void updateUnits(void) {
	forEachUnit(i) {
		u32 x = round(Units[i].Position.x), y = round(Units[i].Position.y);
		if(Units[i].Player == 0) { // Clear fog of war
			for(i32 yi = -Units[i].Type->ViewDistance; yi <= Units[i].Type->ViewDistance; yi++) 
			for(i32 xi = -Units[i].Type->ViewDistance; xi <= Units[i].Type->ViewDistance; xi++)
				if(xi*xi + yi*yi <= Units[i].Type->ViewDistance*Units[i].Type->ViewDistance)
					refSafe(x + xi, y + yi)->Seen = true;
		}
		if(Units[i].Health <= 0) {
			killUnit(i);
			continue;
		}
		
		bool bumbed = false;
		if(!Units[i].MoveOrder) Units[i].Speed = vec2(0);
		vec2 oldPos = Units[i].Position;
		UnitHandle nextEnemy = 0;
		forEachUnit(j) {
			if(i == j) continue;
			vec2 axis = vsub(Units[i].Position, Units[j].Position);
			f32 dist = vlength(axis);
			if(dist < 0.8) {
				vec2 delta = vmul(vdiv(axis, dist), 0.5*(0.8 - dist));
				if(Units[i].Action != ACTION_FARM && Units[i].Action != ACTION_BUILD && 
				  !collide(i, delta) && Units[i].Action != ACTION_CHOP)
					Units[i].Position = vadd(Units[i].Position, delta);
				if(Units[j].Action != ACTION_FARM && Units[j].Action != ACTION_BUILD && !Units[j].MoveOrder && 
				  !collide(j, vneg(delta)) && Units[j].Player == Units[i].Player)
					Units[j].Position = vsub(Units[j].Position, delta);
				bumbed = true;
			} else if(dist < 1.4 && Units[j].Player != Units[i].Player) {
				Units[i].Speed = vmul(Units[i].Speed, 0.75); // Slow down fighting units
				if(Units[i].Action != ACTION_ATTACK) unitAction(i, ACTION_ATTACK, vec2(0), j);
			} else if(Units[j].Player != Units[i].Player && dist < Units[i].Type->ViewDistance*0.75)
				nextEnemy = j;
		}

		bool attacked = false;
		if(Units[i].Action == ACTION_ATTACK) {
			tTile *tile = refSafe(round(Units[Units[i].Attack.Unit].Position.x),
			                      round(Units[Units[i].Attack.Unit].Position.y));
			if(!tile->Animation) { // Bloodsplatter animation
				tile->Animation = 0xf0 + GetRandomValue(0, 1)*8;
				tile->Frame = GetRandomValue(0, 1);
				tile->Blood = 1.0;
			}

			vec2 axis = vsub(Units[i].Position, Units[Units[i].Attack.Unit].Position);
			f32 dist = vlength(axis);
			f32 angle = fmod(2*PI + atan2(axis.y, axis.x), 2*PI); // Turn sprite to enemy unit
			if     (angle >= PI/2*3.5 || angle < PI/2*0.5) Units[i].Direction = 3;
			else if(angle >= PI/2*0.5 && angle < PI/2*1.5) Units[i].Direction = 2;
			else if(angle >= PI/2*1.5 && angle < PI/2*2.5) Units[i].Direction = 1;
			else if(angle >= PI/2*2.5 && angle < PI/2*3.5) Units[i].Direction = 0;
			if(dist >= 1.4 || !Units[Units[i].Attack.Unit].Alive) { // When enemy unit died
				Units[i].Attack.Unit = 0;
				unitAction(i, ACTION_MOVE, vec2(0), 0);
			} else {
				attacked = true;
				if(Units[i].Attack.Timer < GetTime() - 0.6) { // Hurt enemy unit
					Units[Units[i].Attack.Unit].Health -= Units[i].Type->Attack;
					Units[i].Attack.Timer = GetTime();
				}
			}
		} else if(Units[i].Action == ACTION_CHOP) {
			Player[Units[i].Player].WoodIncome++;
			i32 x2 = round(Units[i].Position.x), y2 = round(Units[i].Position.y);
			if(!Units[i].Chop.TreeX) {
				f32 closest = 1e30;
				for(i32 x3 = x2-1; x3 <= x2+1; x3++) for(i32 y3 = y2-1; y3 <= y2+1; y3++) {
					if(isTree(x3, y3) && getSafe(x3, y3).OccupiedTree < 2 && isReachable(x3, y3) &&
					   x3 != Units[i].Chop.IgnoreTreeX && y3 != Units[i].Chop.IgnoreTreeY) {
						f32 dist = vdistance(Units[i].Position, vec2(x3, y3));
						if(dist < closest) {
							closest = dist;
							Units[i].Chop.TreeX = x3;
							Units[i].Chop.TreeY = y3;
						}
					}
				}
				if(Units[i].Chop.TreeX)
					refSafe(Units[i].Chop.TreeX, Units[i].Chop.TreeY)->OccupiedTree++;
			}
			if(!Units[i].Chop.TreeX) {
				Units[i].Action = ACTION_MOVE_AND_CHOP;
				goto lSearchTree;
			}
			tTile *tile = refSafe(Units[i].Chop.TreeX, Units[i].Chop.TreeY);
			vec2 axis = vsub(Units[i].Position, vec2(Units[i].Chop.TreeX, Units[i].Chop.TreeY));
			f32 dist = vdistance(Units[i].Position, vec2(Units[i].Chop.TreeX, Units[i].Chop.TreeY));
			f32 angle = fmod(2*PI + atan2(axis.y, axis.x), 2*PI); // Turn sprite to tree
			if     (angle >= PI/2*3.5 || angle < PI/2*0.5) Units[i].Direction = 3;
			else if(angle >= PI/2*0.5 && angle < PI/2*1.5) Units[i].Direction = 2;
			else if(angle >= PI/2*1.5 && angle < PI/2*2.5) Units[i].Direction = 1;
			else if(angle >= PI/2*2.5 && angle < PI/2*3.5) Units[i].Direction = 0;
			if(Units[i].Chop.Timer < GetTime() - 5.0) {
				Player[Units[i].Player].Wood++;
				Units[i].Chop.Timer = GetTime();
			}

			if(dist > 0.8) {
				Units[i].Speed = vmul(vnormalize(vneg(axis)), Units[i].Type->Speed);
				Units[i].Unmoveable++;
				if(collide(i, vmul(Units[i].Speed, GetFrameTime())) || 
				  Units[i].Unmoveable > 30) {
					Units[i].Speed = vec2(0);
					if(tile->OccupiedTree) tile->OccupiedTree--;
					Units[i].Chop.IgnoreTreeX = Units[i].Chop.TreeX;
					Units[i].Chop.IgnoreTreeY = Units[i].Chop.TreeY;
					Units[i].Chop.TreeX = Units[i].Chop.TreeY = 0;
					Units[i].Action = ACTION_MOVE_AND_CHOP;
					goto lSearchTree;
				}
			} else {
				if(Units[i].Speed.x || Units[i].Speed.y) {
					tile->Animation = 0xd0;
					tile->Frame = 0;
				}
				Units[i].Speed = vec2(0);
			}
		} else if(Units[i].Action == ACTION_MOVE_AND_CHOP) {
			if(Units[i].Unmoveable > 160) {
				unitAction(i, ACTION_MOVE, vec2(0), 0);
				moveUnit(i, NULL);
			} else if(Units[i].Unmoveable > 10) {
				lSearchTree:;
				i32 treeX = 0, treeY = 0, di = 1, dj = 0, segment = 1, passed = 0;
				i32 x2 = Units[i].Chop.SearchTreeX + GetRandomValue(-2, 2);
				i32 y2 = Units[i].Chop.SearchTreeY + GetRandomValue(-2, 2);
				for(i32 j = 0; j < 128; j++) { // Search in a spiral for reachable trees
					x2 += di;
					y2 += dj;
					tTile tile = getSafe(x2, y2);
					if(isTree(x2, y2) && tile.OccupiedTree < tile.MaxOccupy && isReachable(x2, y2) &&
					   x2 != Units[i].Chop.IgnoreTreeX && y2 != Units[i].Chop.IgnoreTreeY) {
						treeX = x2;
						treeY = y2;
						break;
					}
					passed++;
					if(passed == segment) {
						passed = 0;
						i32 tmp = di;
						di = -dj;
						dj = tmp;
						if(dj == 0) segment++;
					}
				}
				if(treeX) {
					i32 oldUnmoveable = Units[i].Unmoveable;
					moveUnit(i, newMoveOrder(
						(tMoveOrder){Target: vec2(treeX, treeY), OnlySeen: true}));
					vec2 flow = getUnitFlow(i);
					if(!flow.x && !flow.y) {
						refSafe(treeX, treeY)->Unreachable = true;
						moveUnit(i, NULL);
						Units[i].Unmoveable = oldUnmoveable;
					}
				}
			}
			Units[i].Unmoveable += Units[i].MoveOrder == NULL;
		} else if(Units[i].Action == ACTION_FARM) {
			Player[Units[i].Player].FoodIncome++;
			if(vdistance(Units[i].Position, Units[i].Farm.Target) > 0.1) {
				vec2 axis = vsub(Units[i].Position, Units[i].Farm.Target);
				Units[i].Speed = vmul(vnormalize(vneg(axis)), Units[i].Type->Speed);
			} else {
				Units[i].Direction = 0;
				if(Units[i].Farm.Timer < GetTime() - 2.5) {
					Player[Units[i].Player].Food++;
					Units[i].Farm.Timer = GetTime();
				}
				Units[i].Speed = vec2(0);
			}
		} else if(Units[i].Action == ACTION_BUILD) {
			tBuilding *build = &Buildings[Units[i].Build.Building];
			f32 dist = build->Type->BlockMovement ? 1.0 : 0.1;
			if(vdistance(Units[i].Position, Units[i].Build.Target) > dist) {
				vec2 axis = vsub(Units[i].Position, Units[i].Build.Target);
				Units[i].Speed = vmul(vnormalize(vneg(axis)), Units[i].Type->Speed);
			} else {
				Units[i].Direction = 0;
				if(Units[i].Build.Timer < GetTime() - 0.1) {
					build->Health++;
					Units[i].Build.Timer = GetTime();
				}
				if(build->Health >= build->Type->MaxHealth) {
					build->Finished = true;
					build->Health = build->Type->MaxHealth;
					Player[build->Player].PopulationLimit += build->Type->Population;
					i32 x2 = build->FirstX, y2 = build->FirstY;
					for(i32 xi = 0; xi < build->Type->SizeX; xi++) 
					for(i32 yi = 0; yi < build->Type->SizeY; yi++) {
						refSafe(x2 + xi, y2 + yi)->Bottom = build->Type->BottomTiles[xi][yi];
						refSafe(x2 + xi, y2 + yi)->Top = build->Type->TopTiles[xi][yi];
					}
					if(build->Type == &Farm) unitAction(i, ACTION_FARM, 
						vec2(x2 + build->Type->SizeX/2 - 0.5, y2 + build->Type->SizeY/2 - 0.75), 0);
					else {
						for(i32 j = 0; j < 8; j++) {
							if(!getSafe(x2 + NX[j], y2 + NY[j]).Move) {
								x2 = x2 + NX[j];
								y2 = y2 + NY[j];
								break;
							}
						}
						unitAction(i, ACTION_MOVE_FREE, vec2(x2, y2), 0);
					}
				}
				Units[i].Speed = vec2(0);
			}
		} else if(Units[i].Action == ACTION_MOVE_FREE) {
			if(vdistance(Units[i].Position, Units[i].Build.Target) > 0.1) {
				vec2 axis = vsub(Units[i].Position, Units[i].Build.Target);
				Units[i].Speed = vmul(vnormalize(vneg(axis)), Units[i].Type->Speed);
			} else unitAction(i, ACTION_MOVE, vec2(0), 0);
		}
		
		Units[i].Speed = vmul(Units[i].Speed, GetFrameTime());
		u32 x2 = round(Units[i].Position.x + Units[i].Speed.x);
		u32 y2 = round(Units[i].Position.y + Units[i].Speed.y);
		if(Units[i].MoveOrder && getSafe(x2, y2).Move != Units[i].MoveOrder->Move[x2][y2])
			updateFlow(Units[i].MoveOrder); // Update pathfinding when Map changed
		else Units[i].Position = vadd(Units[i].Position, Units[i].Speed);
		vec2 movement = vsub(Units[i].Position, oldPos);

		if(Units[i].Action != ACTION_CHOP) {
			if(!attacked && (fabsf(movement.x) > 0.001 || fabsf(movement.y) > 0.001)) {
				f32 angle = fmod(2*PI + atan2(movement.y, movement.x), 2*PI);
				if     (angle >= PI/2*3.5 || angle < PI/2*0.5) Units[i].Direction = 1;
				else if(angle >= PI/2*0.5 && angle < PI/2*1.5) Units[i].Direction = 0;
				else if(angle >= PI/2*1.5 && angle < PI/2*2.5) Units[i].Direction = 3;
				else if(angle >= PI/2*2.5 && angle < PI/2*3.5) Units[i].Direction = 2;
			}
			if(fabsf(movement.x) > 0.02 || fabsf(movement.y) > 0.02) Units[i].Unmoveable = 0;
		}

		if(!Units[i].MoveOrder) { // Idle animation
			if(Units[i].Action != ACTION_MOVE_AND_CHOP && Units[i].Action != ACTION_CHOP)
				Units[i].Unmoveable = 0;
			if(!nextEnemy && Units[i].IdleTimer < GetTime() - 8.0) {
				if(Units[i].Action == ACTION_MOVE) Units[i].Direction = GetRandomValue(0,3);
				Units[i].IdleTimer = GetTime() + GetRandomValue(-4000, 4000) / 1000.0;
				continue;
			} else if(nextEnemy) {
				tMoveOrder *move = newMoveOrder((tMoveOrder){Follow: nextEnemy});
				moveUnit(i, move);
				unitAction(i, ACTION_MOVE, vec2(0), 0);
			} else continue;
		}
		if(Units[i].MoveOrder->Follow && !Units[Units[i].MoveOrder->Follow].Alive) {
			moveUnit(i, NULL);
			continue;
		}
		if(Units[i].Player && !Units[i].MoveOrder->Follow && nextEnemy) // Enemy units autoattack
			moveUnit(i, newMoveOrder((tMoveOrder){Follow: nextEnemy}));

		if(Units[i].MoveOrder->Follow) updateMoveOrder(Units[i].MoveOrder, 20);
		Units[i].Speed = vmul(getUnitFlow(i), Units[i].Type->Speed - bumbed);
		Units[i].Unmoveable += fabsf(movement.x) < 0.02 && fabsf(movement.y) < 0.02;
		f32 dist = 0.5;
		eAction action = Units[i].Action;
		if(Units[i].MoveOrder->Follow) dist = 0.3;
		else {
			if(bumbed) dist += 1.0;
			if(Units[i].Unmoveable > 20) dist += 4.0;
			if(action == ACTION_MOVE_AND_CHOP) dist = 1.0;
			else if(action == ACTION_MOVE_AND_FARM) dist = 0.5;
			else if(action == ACTION_MOVE_AND_BUILD) dist = 2.0;
		}

		if(vdistance(Units[i].Position, Units[i].MoveOrder->Target) < dist) {
			f32 friendDist = action == ACTION_MOVE_AND_CHOP ? 2.0 : 3.0;
			tMoveOrder *order = Units[i].MoveOrder;
			forEachUnit(j) if(vdistance(Units[i].Position, Units[j].Position) < friendDist &&
				order == Units[j].MoveOrder) moveUnit(j, NULL);
			if(action == ACTION_MOVE_AND_CHOP) unitAction(i, ACTION_CHOP, vec2(0), 0);
			if(action == ACTION_MOVE_AND_FARM) unitAction(i, ACTION_FARM, Units[i].Farm.Target, 0);
			if(action == ACTION_MOVE_AND_BUILD) unitAction(i, ACTION_BUILD, Units[i].Build.Target, 0);
		}
	}
}

void moveUnit(UnitHandle unit, tMoveOrder *order) {
	if(order) order->References++;
	if(Units[unit].MoveOrder && --Units[unit].MoveOrder->References <= 0) 
		freeMoveOrder(Units[unit].MoveOrder);
	Units[unit].Speed = vec2(0);
	Units[unit].MoveOrder = order;
	Units[unit].Unmoveable = 0;
}
