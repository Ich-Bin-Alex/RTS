#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"

i32 CharSizes[0x80] = {[' '] = 4};

static f32 MoveAnim;
static bool RectSelect = false;
static UnitHandle MoveTarget, UnitUnderMouse;
static Vector2 Select1, Select2, MovePos;
static bool ShowDebug = true;

static void drawText(char *text, i32 x, i32 y, Color color) {
	const i32 NX[8] = {-1,-1,0,1,1,1,0,-1}, NY[8] = {0,-1,-1,-1,0,1,1,1};
	for(i32 i = 0; text[i]; i++) {
		u32 tx = (text[i] - ' ') % 16, ty = 16 + (text[i] - ' ') / 16;
		for(i32 j = 0; j < 8; j++) drawTileFixed(x+NX[j], y+NY[j], tx, ty, BLACK, 2); // Shadow
		drawTileFixed(x, y, tx, ty, color, 2);
		x += CharSizes[(u32)text[i]] * 2;
	}
}

static i32 measureText(char *text) {
	i32 length = 0;
	for(i32 i = 0; text[i]; i++) length += CharSizes[(u32)text[i]] * 2;
	return length;
}

void updateInterface() {
	static bool Selected = true;

	i32 width = GetScreenWidth(), height = GetScreenHeight();
	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};

	if(IsKeyDown(KEY_RIGHT) || GetMouseX() >= width - 50) CameraX += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_LEFT) || GetMouseX() <= 50) CameraX -= round(1000.0 * GetFrameTime());
	if(IsKeyDown(KEY_DOWN) || GetMouseY() >= height - 50) CameraY += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_UP) || GetMouseY() <= 50) CameraY -= round(1000.0 * GetFrameTime());
	if(CameraX > MAP_SIZE*8*DRAW_SIZE-width) CameraX = MAP_SIZE*8*DRAW_SIZE-width;
	if(CameraX < 0) CameraX = 0;
	if(CameraY > MAP_SIZE*8*DRAW_SIZE-height) CameraY = MAP_SIZE*8*DRAW_SIZE-height;
	if(CameraY < 0) CameraY = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		Selected = RectSelect = false;
		i32 x = mouse.x/DRAW_SIZE, y = mouse.y/DRAW_SIZE;
		for(UnitHandle i = 1; i < MAX_UNITS; i++) {
			if(!Units[i].Alive) continue;
			Units[i].Selected = false;
			if(x > Units[i].Position.x*8 && x < Units[i].Position.x*8+8 && !Units[i].Player &&
			   y > Units[i].Position.y*8 && y < Units[i].Position.y*8+8 && !Selected)
				Units[i].Selected = Selected = true;
		}
	} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		if(RectSelect && abs(Select1.x - Select2.x) > 1 && abs(Select1.y - Select2.y) > 1) {
			i32 x1 = (min(Select1.x, Select2.x) + CameraX) / DRAW_SIZE;
			i32 y1 = (min(Select1.y, Select2.y) + CameraY) / DRAW_SIZE;
			i32 x2 = (max(Select1.x, Select2.x) + CameraX) / DRAW_SIZE;
			i32 y2 = (max(Select1.y, Select2.y) + CameraY) / DRAW_SIZE;
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
		if(!RectSelect) Select1 = GetMousePosition();
		Select2 = GetMousePosition();
		RectSelect = true;
	} else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Selected) {
		RectSelect = false;
		MovePos = (Vector2){mouse.x / DRAW_SIZE / 8.0, mouse.y / DRAW_SIZE / 8.0};
		tMoveOrder *move;
		MoveTarget = 0;
		if(UnitUnderMouse && Units[UnitUnderMouse].Player) {
			MoveTarget = UnitUnderMouse;
			move = newMoveOrder((tMoveOrder){Target: MovePos, Follow: MoveTarget});
		} else
			move = newMoveOrder((tMoveOrder){Target: MovePos});
		i32 numSelected = 0, numUnmoveable = 0;
		Vector2 mid = {0};
		bool canChop = true;
		for(UnitHandle i = 1; i < MAX_UNITS; i++) {
			if(!Units[i].Alive) continue;
			if(Units[i].Selected) {
				mid = Vector2Add(mid, Units[i].Position);
				if(!Units[i].Type->CanChop) canChop = false;
				moveUnit(i, move);
				numSelected++;
				Vector2 flow = getUnitFlow(i);
				if(!flow.x && !flow.y) numUnmoveable++;
			}
		}
		if(!numSelected) {
			Selected = false;
			return;
		}
		mid = Vector2Divide(mid, (Vector2){numSelected, numSelected});
		Vector2 dir = Vector2Normalize(Vector2Subtract(mid, MovePos));

		canChop = canChop && isTree(MovePos.x, MovePos.y);
		// Search for valid position, in case the desired position is unreachable
		while(numSelected == numUnmoveable || getSafe(MovePos.x, MovePos.y).Move == 0xff) {
			numUnmoveable = 0;
			MovePos = Vector2Add(MovePos, dir);
			move = newMoveOrder((tMoveOrder){Target: MovePos});
			for(UnitHandle i = 1; i < MAX_UNITS; i++) {
				if(!Units[i].Alive) continue;
				if(Units[i].Selected) {
					moveUnit(i, move);
					Vector2 flow = getUnitFlow(i);
					if(!flow.x && !flow.y) numUnmoveable++;
				}
			}
			if(MovePos.x >= MAP_SIZE || MovePos.y >= MAP_SIZE) {
				MovePos = (Vector2){0};
				break;
			}
		}

		if(MovePos.x) MoveAnim = 4;
		else if(move && move->References <= 0) freeMoveOrder(move);
	}

	if(IsKeyPressed(KEY_F3)) ShowDebug = 1 - ShowDebug;
}

void beginDrawInterface() {
	if(MoveAnim > 0 && !MoveTarget)
		drawTileFree((Vector2){MovePos.x-0.5, MovePos.y-0.5}, 20-ceil(MoveAnim), 31);
}

void endDrawInterface() {
	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};
	i32 x = mouse.x/DRAW_SIZE, y = mouse.y/DRAW_SIZE;
	UnitUnderMouse = 0;
	for(UnitHandle i = 1; i < MAX_UNITS; i++) {
		if(!Units[i].Alive) continue;
		f32 x2 = Units[i].Position.x, y2 = Units[i].Position.y;
		if(!UnitUnderMouse && x > x2*8 && x < x2*8+8 && y > y2*8 && y < y2*8+8) {
			UnitUnderMouse = i;
			if(Units[i].Player && !Map[(u32)x2][(u32)y2].Seen) UnitUnderMouse = 0;
		}
		if(Units[i].Selected || UnitUnderMouse == i) {
			i32 health = ((f32)Units[i].Health / (f32)Units[i].Type->MaxHealth) * 6.0;
			i32 x3 = toMapX(Units[i].Position.x*8), y3 = toMapY(Units[i].Position.y*8-2);
			Color color = GetColor(0x4ebe1eff);
			if(health < 2.0) color = GetColor(0xbd3c1cff);
			else if(health < 4.0) color = GetColor(0xbda81cff);
			DrawRectangle(x3+3, y3, 6*DRAW_SIZE, DRAW_SIZE, GetColor(0x00000080));
			DrawRectangle(x3+3, y3, health*DRAW_SIZE, DRAW_SIZE, color);
		}
	}

	if(MoveAnim > 0 && MoveTarget) drawTileFree(Units[MoveTarget].Position, 24-ceil(MoveAnim), 31);

	if(RectSelect)
		DrawRectangleLinesEx((Rectangle){min(Select1.x, Select2.x), min(Select1.y, Select2.y), 
			abs(Select2.x-Select1.x), abs(Select2.y-Select1.y)}, DRAW_SIZE, WHITE);

	if(MoveAnim > 0) MoveAnim -= GetFrameTime() * 10.0;

	char *text = TextFormat("%d/%d", Player[0].Population, Player[0].PopulationLimit);
	drawText(text, GetScreenWidth() - measureText(text) - 33, 6, GetColor(0xefefefff));
	drawTileFixed(GetScreenWidth() - 27, 3, 16, 29, WHITE, DRAW_SIZE);

	text = TextFormat("%d", Player[0].Food);
	drawText(text, GetScreenWidth() - measureText(text) - 33, 32, GetColor(0xefefefff));
	drawTileFixed(GetScreenWidth() - 27, 29, 17, 29, WHITE, DRAW_SIZE);

	text = TextFormat("%d", Player[0].Wood);
	drawText(text, GetScreenWidth() - measureText(text) - 33, 58, GetColor(0xefefefff));
	drawTileFixed(GetScreenWidth() - 27, 55, 18, 29, WHITE, DRAW_SIZE);

	if(ShowDebug) {
		drawText(TextFormat("%d Units, %d Orders", NumUnits, NumMoveOrders), 
			3, 3, GetColor(0xefefefff));
		drawText(TextFormat("%d FPS", GetFPS()), 3, 23, GetColor(0xb2d37dff));
	}

	drawTileFixed(GetMouseX() - 3, GetMouseY() - 3, 20, 30, WHITE, DRAW_SIZE);
}
