#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"

static u32 MoveX, MoveY;
static f32 MoveAnim;
static bool RectSelect = false;
static UnitHandle MoveTarget, UnitUnderMouse;
static Vector2 Select1, Select2;

void updateInterface() {
	static bool Selected = true;

	if(IsKeyDown(KEY_RIGHT) || GetMouseX() >= GetScreenWidth() - 50) CameraX += 15;
	else if(IsKeyDown(KEY_LEFT) || GetMouseX() <= 50) CameraX -= 15;
	if(IsKeyDown(KEY_DOWN) || GetMouseY() >= GetScreenHeight() - 50) CameraY += 15;
	else if(IsKeyDown(KEY_UP) || GetMouseY() <= 50) CameraY -= 15;
	if(CameraX > MAP_SIZE*8*DRAW_SIZE-GetScreenWidth()) 
		CameraX = MAP_SIZE*8*DRAW_SIZE-GetScreenWidth();
	if(CameraX < 0) CameraX = 0;
	if(CameraY < 0) CameraY = 0;
	if(CameraY > MAP_SIZE*8*DRAW_SIZE-GetScreenHeight()) 
		CameraY = MAP_SIZE*8*DRAW_SIZE-GetScreenHeight();

	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		Selected = RectSelect = false;
		i32 x = mouse.x/DRAW_SIZE, y = mouse.y/DRAW_SIZE;
		for(UnitHandle i = 1; i < MAX_UNITS; i++) {
			if(!Units[i].Alive) continue;
			Units[i].Selected = false;
			if(x > Units[i].Position.x*8 && x < Units[i].Position.x*8+8 &&
			   y > Units[i].Position.y*8 && y < Units[i].Position.y*8+8 && !Selected && !Units[i].Player)
				Units[i].Selected = Selected = true;
		}
	} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		if(RectSelect && abs(Select1.x - Select2.x) > 1 && abs(Select1.y - Select2.y) > 1) {
			i32 x1 = ((Select1.x < Select2.x ? Select1.x : Select2.x) + CameraX) / DRAW_SIZE;
			i32 y1 = ((Select1.y < Select2.y ? Select1.y : Select2.y) + CameraY) / DRAW_SIZE;
			i32 x2 = ((Select1.x < Select2.x ? Select2.x : Select1.x) + CameraX) / DRAW_SIZE;
			i32 y2 = ((Select1.y < Select2.y ? Select2.y : Select1.y) + CameraY) / DRAW_SIZE;
			for(UnitHandle i = 1; i < MAX_UNITS; i++) {
				if(!Units[i].Alive) continue;
				Units[i].Selected = false;
				if(Units[i].Position.x*8+8 > x1 && Units[i].Position.x*8 < x2 &&
				   Units[i].Position.y*8+8 > y1 && Units[i].Position.y*8 < y2 && !Units[i].Player)
					Units[i].Selected = Selected = true;
			}
			RectSelect = false;
		}
	} else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		if(!RectSelect) Select1 = Select2 = GetMousePosition();
		else Select2 = GetMousePosition();
		RectSelect = true;
	} else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Selected) {
		RectSelect = false;
		MoveX = mouse.x/DRAW_SIZE/8;
		MoveY = mouse.y/DRAW_SIZE/8;
		Vector2 target = {round((f32)MoveX-0.5), round((f32)MoveY-0.5)};
		tMoveOrder *move;
		MoveTarget = 0;
		if(UnitUnderMouse && Units[UnitUnderMouse].Player) {
			MoveTarget = UnitUnderMouse;
			move = newMoveOrder((tMoveOrder){Target: target, Follow: MoveTarget});
		} else
			move = newMoveOrder((tMoveOrder){Target: target});
		i32 numSelected = 0, numUnmoveable = 0;
		Vector2 mid = {0};
		for(UnitHandle i = 1; i < MAX_UNITS; i++) {
			if(!Units[i].Alive) continue;
			if(Units[i].Selected) {
				mid = Vector2Add(mid, Units[i].Position);
				moveUnit(i, move);
				numSelected++;
				Vector2 flow = getUnitFlow(i);
				if(!flow.x && !flow.y) numUnmoveable++;
			}
		}
		mid = Vector2Divide(mid, (Vector2){numSelected, numSelected});
		Vector2 dir = Vector2Normalize(Vector2Subtract(mid, target));

		// Search for valid position, in case the desired position is unreachable
		while(numSelected == numUnmoveable || getSafe(MoveX, MoveY).Move == 0xff) {
			numUnmoveable = 0;
			target = Vector2Add(target, dir);
			move = newMoveOrder((tMoveOrder){Target: target});
			for(UnitHandle i = 1; i < MAX_UNITS; i++) {
				if(!Units[i].Alive) continue;
				if(Units[i].Selected) {
					moveUnit(i, move);
					Vector2 flow = getUnitFlow(i);
					if(!flow.x && !flow.y) numUnmoveable++;
				}
			}
			MoveX = target.x;
			MoveY = target.y;
			if(MoveX >= MAP_SIZE || MoveY >= MAP_SIZE) {
				MoveX = MoveY = 0;
				break;
			}
		}

		if(MoveX) MoveAnim = 4;
	}
}

void beginDrawInterface() {
	//i32 anim = (i32)(GetTime() * 10.0) % 8;
	//if(anim >= 4) anim = 7-anim;
	//
	//Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};
	//mouse.x = (i32)mouse.x/DRAW_SIZE/8;
	//mouse.y = (i32)mouse.y/DRAW_SIZE/8;
	//DrawTexturePro(Interface, (Rectangle){anim*8,0,8,8}, 
	//	toMapR(mouse.x, mouse.y), (Vector2){0}, 0.0, WHITE);

	if(MoveAnim > 0 && !MoveTarget)
		DrawTexturePro(Tileset, (Rectangle){128+(4-ceil(MoveAnim))*8,248,8,8}, 
			toMapR(MoveX, MoveY), (Vector2){0}, 0.0, WHITE);
}

void endDrawInterface() {
	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};
	i32 x = mouse.x/DRAW_SIZE, y = mouse.y/DRAW_SIZE;
	UnitUnderMouse = 0;
	for(UnitHandle i = 1; i < MAX_UNITS; i++) {
		if(!Units[i].Alive) continue;
		if(!UnitUnderMouse && x > Units[i].Position.x*8 && x < Units[i].Position.x*8+8 &&
		   y > Units[i].Position.y*8 && y < Units[i].Position.y*8+8) UnitUnderMouse = i;
		if(Units[i].Selected || UnitUnderMouse == i) {
			i32 health = ((f32)Units[i].Health / (f32)Units[i].Type->MaxHealth) * 6.0;
			i32 x = toMapX(Units[i].Position.x*8), y = toMapY(Units[i].Position.y*8-2);
			Color color = GetColor(0x4ebe1eff);
			if(health < 4.0) color = GetColor(0xbda81cff);
			if(health < 2.0) color = GetColor(0xbd3c1cff);
			DrawRectangle(x+3, y, 6*DRAW_SIZE, DRAW_SIZE, GetColor(0x00000080));
			DrawRectangle(x+3, y, health*DRAW_SIZE, DRAW_SIZE, color);
		}
	}

	if(MoveAnim > 0 && MoveTarget)
		DrawTexturePro(Tileset, (Rectangle){160+(4-ceil(MoveAnim))*8,248,8,8}, 
			toMapR(Units[MoveTarget].Position.x, Units[MoveTarget].Position.y), (Vector2){0}, 0.0, WHITE);

	if(RectSelect) {
		i32 x = Select1.x < Select2.x ? Select1.x : Select2.x;
		i32 y = Select1.y < Select2.y ? Select1.y : Select2.y;
		DrawRectangleLinesEx((Rectangle)
			{x, y, abs(Select2.x-Select1.x), abs(Select2.y-Select1.y)}, 3, WHITE);
	}

	if(MoveAnim > 0) MoveAnim -= GetFrameTime() * 10.0;
}
