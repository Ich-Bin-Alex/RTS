#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"
#include "source/interface.h"

i32 main() {
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(1024, 768, "RTS");
	SetTargetFPS(60);

	initGame();

	for(i32 i = 0; i < 40; i++) {
		Vector2 pos = {16.0 + GetRandomValue(-100, 100)/100.0,16.0 + GetRandomValue(-100, 100)/100.0};
		newUnit((tUnit){Type: &Peasent, Position: pos});
	}

	for(i32 i = 0; i < 30; i++) {
		Vector2 pos = {MAP_SIZE-16.0 + GetRandomValue(-100, 100)/100.0,MAP_SIZE-16.0 + GetRandomValue(-100, 100)/100.0};
		newUnit((tUnit){Type: &Peasent, Player: 7, Position: pos});
	}
	tMoveOrder *move = newMoveOrder((tMoveOrder){AttackOnSight: true, Target: {16.0, 16.0}});
	for(i32 i = 0; i < 10; i++) {
		Vector2 pos = {MAP_SIZE-16.0 + GetRandomValue(-100, 100)/100.0,MAP_SIZE-16.0 + GetRandomValue(-100, 100)/100.0};
		UnitHandle unit = newUnit((tUnit){Type: &Peasent, Player: 7, Position: pos});
		moveUnit(unit, move);
	}

	while(!WindowShouldClose()) {
		updateGame();
		updateInterface();
		updateUnits();

		BeginDrawing();
			ClearBackground(BLACK);
			beginDrawMap();
			beginDrawInterface();
			drawUnits();
			endDrawMap();
			endDrawInterface();
		EndDrawing();
	}

	return 0;
}