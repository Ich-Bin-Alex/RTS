#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"

i32 CharSizes[0x80] = {[' '] = 4};

static f32 MoveAnim, CursorOffset;
static bool Selected = false, RectSelect = false;
static UnitHandle MoveTarget, UnitUnderMouse;
static BuildingHandle SelectedBuild;
static Vector2 Select1, Select2, MovePos;
static bool ShowDebug = false;
static i32 UIWidth, UIHeight;

static void drawText(char *text, i32 x, i32 y, Color color) {
	for(i32 i = 0; text[i]; i++) {
		u32 tx = (text[i] - ' ') % 16, ty = 16 + (text[i] - ' ') / 16;
		for(i32 j = 0; j < 8; j++) drawTileFixed(x+NX[j], y+NY[j], tx, ty, BLACK, FontSize); // Shadow
		drawTileFixed(x, y, tx, ty, color, FontSize);
		x += CharSizes[(u32)text[i]] * FontSize;
	}
}

static void drawTextAnimated(char *text, i32 x, i32 y, Color color[5]) {
	i32 anim = (i32)(GetTime() * 10) % 10;
	if(anim >= 5) anim = 9 - anim;
	for(i32 i = 0; text[i]; i++) {
		u32 tx = (text[i] - ' ') % 16, ty = 16 + (text[i] - ' ') / 16;
		for(i32 j = 0; j < 8; j++) drawTileFixed(x+NX[j], y+NY[j], tx, ty, BLACK, FontSize); // Shadow
		drawTileFixed(x, y, tx, ty, color[anim], FontSize);
		x += CharSizes[(u32)text[i]] * FontSize;
	}
}

static bool drawActionButton(i32 x, i32 y, u32 tx, u32 ty) {
	i32 mx = GetMouseX(), my = GetMouseY();
	drawTileFixed(x, y, 20, 27, WHITE, DrawSize);
	drawTileFixed(x + 8*DrawSize, y, 21, 27, WHITE, DrawSize);
	drawTileFixed(x, y + 8*DrawSize, 20, 28, WHITE, DrawSize);
	drawTileFixed(x + 8*DrawSize, y + 8*DrawSize, 21, 28, WHITE, DrawSize);
	drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, WHITE, DrawSize);
	if(mx > x && mx < x + 16*DrawSize && my > y && my < y + 16*DrawSize) {
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			drawTileFixed(x, y, 22, 27, WHITE, DrawSize);
			drawTileFixed(x + 8*DrawSize, y, 23, 27, WHITE, DrawSize);
			drawTileFixed(x, y + 8*DrawSize, 22, 28, WHITE, DrawSize);
			drawTileFixed(x + 8*DrawSize, y + 8*DrawSize, 23, 28, WHITE, DrawSize);
			drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, WHITE, DrawSize);
			drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, GetColor(0x00000080), DrawSize);
		} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return true;
	}
	return false;
}

static i32 measureText(char *text) {
	i32 length = 0;
	for(i32 i = 0; text[i]; i++) length += CharSizes[(u32)text[i]] * FontSize;
	return length;
}

static void drawHealthBar(i32 x, i32 y, UnitHandle unit, i32 scale) {
	i32 health = ((f32)Units[unit].Health / (f32)Units[unit].Type->MaxHealth) * (f32)scale;
	Color color = HealthColor1;
	if(health < scale/3.0) color = HealthColor3;
	else if(health < scale/1.5) color = HealthColor2;
	DrawRectangle(x, y, scale*DrawSize, DrawSize, GetColor(0x00000080));
	DrawRectangle(x, y, health*DrawSize, DrawSize, color);
}

static void drawBuildingHealthBar(i32 x, i32 y, BuildingHandle build, i32 scale) {
	i32 health = ((f32)Buildings[build].Health / (f32)Buildings[build].Type->MaxHealth) * (f32)scale;
	Color color = HealthColor1;
	if(health < scale/3.0) color = HealthColor3;
	else if(health < scale/1.5) color = HealthColor2;
	DrawRectangle(x, y, scale*DrawSize, DrawSize, GetColor(0x00000080));
	DrawRectangle(x, y, health*DrawSize, DrawSize, color);
}

void updateInterface(void) {
	i32 width = GetScreenWidth(), height = GetScreenHeight();
	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};
	bool inUnitUI = (GetMouseY() > height - 3 - 16*DrawSize && GetMouseX() <= UIWidth) ||
	                (GetMouseX() <= 3 + 16*DrawSize && GetMouseY() >= height - UIHeight);

	if(IsKeyDown(KEY_RIGHT) || GetMouseX() >= width - 15) CameraX += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_LEFT) || (GetMouseX() <= 15 && !inUnitUI))
		CameraX -= round(1000.0 * GetFrameTime());
	if(IsKeyDown(KEY_DOWN) || (GetMouseY() >= height - 15 && !inUnitUI))
		CameraY += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_UP) || GetMouseY() <= 15) CameraY -= round(1000.0 * GetFrameTime());
	if(CameraX > MAP_SIZE*8*DrawSize-width) CameraX = MAP_SIZE*8*DrawSize-width;
	if(CameraX < 0) CameraX = 0;
	if(CameraY > MAP_SIZE*8*DrawSize-height) CameraY = MAP_SIZE*8*DrawSize-height;
	if(CameraY < 0) CameraY = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inUnitUI) {
		Selected = RectSelect = false;
		SelectedBuild = 0;
		i32 x = mouse.x/DrawSize, y = mouse.y/DrawSize;
		forEachUnit(i) {
			Units[i].Selected = false;
			if(x > Units[i].Position.x*8 && x < Units[i].Position.x*8+8 && !Units[i].Player &&
			   y > Units[i].Position.y*8 && y < Units[i].Position.y*8+8 && !Selected)
				Units[i].Selected = Selected = true;
		}
		if(!Selected) SelectedBuild = getBuilding(x / 8, y / 8);
	} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !inUnitUI) {
		if(RectSelect && abs(Select1.x - Select2.x) > 1 && abs(Select1.y - Select2.y) > 1) {
			i32 x1 = (min(Select1.x, Select2.x) + CameraX) / DrawSize;
			i32 y1 = (min(Select1.y, Select2.y) + CameraY) / DrawSize;
			i32 x2 = (max(Select1.x, Select2.x) + CameraX) / DrawSize;
			i32 y2 = (max(Select1.y, Select2.y) + CameraY) / DrawSize;
			forEachUnit(i) {
				Units[i].Selected = false;
				if(Units[i].Position.x*8+4 > x1 && Units[i].Position.x*8 < x2 &&
				   Units[i].Position.y*8+4 > y1 && Units[i].Position.y*8 < y2 && !Units[i].Player)
					Units[i].Selected = Selected = true;
			}
			RectSelect = false;
		}
	} else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !inUnitUI) {
		if(!RectSelect) Select1 = GetMousePosition();
		Select2 = GetMousePosition();
		RectSelect = true;
	} else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Selected) {
		RectSelect = false;
		MovePos = (Vector2){mouse.x / DrawSize / 8.0, mouse.y / DrawSize / 8.0};
		tMoveOrder *move;
		MoveTarget = 0;
		bool canChop = isTree(MovePos.x, MovePos.y) && getSafe(MovePos.x, MovePos.y).Seen;
		bool canFarm = isFarm(MovePos.x, MovePos.y) && getSafe(MovePos.x, MovePos.y).Seen &&
			!Buildings[getSafe(MovePos.x, MovePos.y).Building].Player &&
			!Buildings[getSafe(MovePos.x, MovePos.y).Building].Farm.Occupied;
		bool canBuild = true;
		UnitHandle farmer = 0;
		if(UnitUnderMouse && Units[UnitUnderMouse].Player) {
			MoveTarget = UnitUnderMouse;
			move = newMoveOrder((tMoveOrder){Target: MovePos, Follow: MoveTarget});
		} else {
			if(canFarm) {
				MovePos = Buildings[getBuilding(MovePos.x, MovePos.y)].Position;
				MovePos.y -= 0.25;
			}
			move = newMoveOrder((tMoveOrder){Target: (Vector2){round(MovePos.x), round(MovePos.y)}});
		}
		i32 numSelected = 0, numUnmoveable = 0;
		Vector2 mid = {0};
		forEachUnit(i) if(Units[i].Selected) {
			if(!Units[i].Type->CanChop) canChop = false;
			if(!Units[i].Type->CanFarm) canFarm = false;
			if(!Units[i].Type->CanBuild) canBuild = false;
		}
		forEachUnit(i) if(Units[i].Selected) {
			mid = Vector2Add(mid, Units[i].Position);
			moveUnit(i, move);
			Units[i].Action = ACTION_MOVE;
			numSelected++;
			Vector2 flow = getUnitFlow(i);
			if(!flow.x && !flow.y) numUnmoveable++;
			if(canFarm && !farmer) {
				farmer = i;
				break;
			}
		}
		if(!numSelected) {
			Selected = false;
			return;
		}
		mid = Vector2Divide(mid, (Vector2){numSelected, numSelected});
		Vector2 dir = Vector2Normalize(Vector2Subtract(mid, MovePos));

		// Search for valid position, in case the desired position is unreachable
		while(numSelected == numUnmoveable || getSafe(MovePos.x, MovePos.y).Move == 0xff) {
			canFarm = false;
			numUnmoveable = 0;
			MovePos = Vector2Add(MovePos, dir);
			move = newMoveOrder((tMoveOrder){Target: (Vector2){round(MovePos.x), round(MovePos.y)}});
			forEachUnit(i) if(Units[i].Selected) {
				moveUnit(i, move);
				Vector2 flow = getUnitFlow(i);
				if(!flow.x && !flow.y) numUnmoveable++;
			}
			if((u32)MovePos.x >= MAP_SIZE || (u32)MovePos.y >= MAP_SIZE) {
				MovePos = (Vector2){0};
				break;
			}
		}

		CursorOffset = 0.5;
		if(canChop) {
			forEachUnit(i) if(Units[i].Selected) {
				Units[i].Action = ACTION_MOVE_AND_CHOP;
				Units[i].Chop.IgnoreTreeX = Units[i].Chop.IgnoreTreeY = 0;
				Units[i].Chop.SearchTreeX = MovePos.x;
				Units[i].Chop.SearchTreeY = MovePos.y;
			}
		} else if(canFarm && farmer) {
			tTile tile = getSafe(MovePos.x, MovePos.y);
			if(!Buildings[tile.Building].Farm.Occupied) {
				Units[farmer].Action = ACTION_MOVE_AND_FARM;
				Units[farmer].Farm.Target = MovePos;
				Units[farmer].Farm.Building = tile.Building;
				Buildings[tile.Building].Farm.Occupied = true;
				CursorOffset = 0.0;
			} else moveUnit(farmer, NULL);
		}

		if(MovePos.x) MoveAnim = 4;
		else if(move && move->References <= 0) freeMoveOrder(move);
	}

	if(IsKeyPressed(KEY_F3)) ShowDebug = 1 - ShowDebug;
}

void beginDrawInterface(void) {
	if(MoveAnim > 0 && !MoveTarget) drawTileFree(
		(Vector2){MovePos.x - CursorOffset, MovePos.y - CursorOffset}, 20 - ceil(MoveAnim), 31);
}

void endDrawInterface(void) {
	Vector2 mouse = (Vector2){GetMouseX() + CameraX, GetMouseY() + CameraY};
	i32 x = mouse.x/DrawSize, y = mouse.y/DrawSize, numSelected = 0;
	u32 width = GetScreenWidth(), height = GetScreenHeight();
	UnitUnderMouse = 0;
	UnitHandle firstSelected = 0;
	bool canChop = Selected && isTree(x / 8, y / 8) && getSafe(x / 8 , y / 8).Seen;
	bool canFarm = Selected && isFarm(x / 8, y / 8) && getSafe(x / 8 , y / 8).Seen &&
		!Buildings[getSafe(x / 8, y / 8).Building].Farm.Occupied;
	bool canBuild = true;
	forEachUnit(i) {
		f32 x2 = Units[i].Position.x, y2 = Units[i].Position.y;
		if(!UnitUnderMouse && x > x2*8 && x < x2*8+8 && y > y2*8 && y < y2*8+8) {
			UnitUnderMouse = i;
			if(Units[i].Player && !getSafe(x2, y2).Seen) UnitUnderMouse = 0;
		}
		if(Units[i].Selected) {
			if(!Units[i].Type->CanChop) canChop = false;
			if(!Units[i].Type->CanFarm) canFarm = false;
			if(!Units[i].Type->CanBuild) canBuild = false;
			numSelected++;
			if(!firstSelected) firstSelected = i;
		}
		if(Units[i].Selected || UnitUnderMouse == i)
			drawHealthBar(toMapX(Units[i].Position.x*8)+DrawSize, toMapY(Units[i].Position.y*8-2), i, 6);
	}
	BuildingHandle build = SelectedBuild ? SelectedBuild : getBuilding(x / 8, y / 8);
	if(!numSelected && !UnitUnderMouse && build) {
		tBuildingType *type = Buildings[build].Type;
		drawBuildingHealthBar(toMapX(Buildings[build].Position.x*8-type->SizeX*2)-1, 
			toMapY(Buildings[build].Position.y*8-type->SizeY*2)-DrawSize*3, build, type->SizeX*8);
	}

	if(MoveAnim > 0 && MoveTarget) drawTileFree(Units[MoveTarget].Position, 24-ceil(MoveAnim), 31);

	if(RectSelect)
		DrawRectangleLinesEx((Rectangle){min(Select1.x, Select2.x), min(Select1.y, Select2.y),
			abs(Select2.x-Select1.x), abs(Select2.y-Select1.y)}, DrawSize, WHITE);

	if(MoveAnim > 0) MoveAnim -= GetFrameTime() * 10.0;

	i32 offset = 8*DrawSize + 9;
	char *text = TextFormat("%d/%d", Player[0].Population, Player[0].PopulationLimit);
	drawText(text, width - measureText(text) - offset, 6, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 3, 16, 29, WHITE, DrawSize);

	text = TextFormat("%d", Player[0].Food);
	offset = 8*DrawSize + 9;
	if(Player[0].FoodIncome) {
		char *inc = TextFormat("+%d", Player[0].FoodIncome);
		offset += measureText(inc);
		drawText(inc, width - offset, 8 + 8*DrawSize, GoodColor);
	}
	drawText(text, width - measureText(text) - offset, 8 + 8*DrawSize, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 5 + 8*DrawSize, 17, 29, WHITE, DrawSize);

	text = TextFormat("%d", Player[0].Wood);
	offset = 8*DrawSize + 9;
	if(Player[0].WoodIncome) {
		char *inc = TextFormat("+%d", Player[0].WoodIncome);
		offset += measureText(inc);
		drawText(inc, width - offset, 10 + 16*DrawSize, GoodColor);
	}
	drawText(text, width - measureText(text) - offset, 10 + 16*DrawSize, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 7 + 16*DrawSize, 18, 29, WHITE, DrawSize);

	if(ShowDebug) {
		i32 fps = GetFPS();
		drawText(TextFormat("%d/%d Units, %d Orders", NumUnits, AllocatedUnits, NumMoveOrders),
			3, 3, TextColor);
		drawText(TextFormat("%d FPS", fps), 3, 8*DrawSize - 1, fps < 30 ? BadColor : GoodColor);
	}

	if(numSelected || SelectedBuild) {
		DrawRectangle(3, height - 3 - 16*DrawSize, 16*DrawSize, 16*DrawSize, Player[0].Color2);
		drawTileFixed(3, height - 3 - 16*DrawSize, 16, 27, WHITE, DrawSize);
		drawTileFixed(3 + 8*DrawSize, height - 3 - 16*DrawSize, 17, 27, WHITE, DrawSize);
		drawTileFixed(3, height - 3 - 8*DrawSize, 16, 28, WHITE, DrawSize);
		drawTileFixed(3 + 8*DrawSize, height - 3 - 8*DrawSize, 17, 28, WHITE, DrawSize);
		if(numSelected) {
			i32 anim = (i32)(GetTime() * 4.0);
			drawSpriteFixed(3 + 4*DrawSize, height-3-13*DrawSize, 0, anim % 4, 0);
			if(numSelected > 1) {
				char *text = TextFormat("%d", numSelected);
				drawText(text, 3 + 8*DrawSize - measureText(text)/2, height - 8*DrawSize, WHITE);
			} else drawHealthBar(3 + 3*DrawSize, height - 3 - 4*DrawSize, firstSelected, 10);
	
			if(drawActionButton(3 + 16*DrawSize, height - 3 - 16*DrawSize, 21, 29))
				killUnit(firstSelected);
		} else if(SelectedBuild) {
			tBuildingType *type = Buildings[SelectedBuild].Type;
			u32 tx = 16 + (type->Icon & 0x0f), ty = 16 + (type->Icon >> 4);
			drawTileFixed(4 + 4*DrawSize, height-3-13*DrawSize, tx, ty, WHITE, DrawSize);
			drawBuildingHealthBar(3 + 3*DrawSize, height - 3 - 4*DrawSize, SelectedBuild, 10);
			if(drawActionButton(3 + 16*DrawSize, height - 3 - 16*DrawSize, 21, 29)) {
				destroyBuilding(SelectedBuild);
				SelectedBuild = 0;
			}
		}
		UIWidth = 3 + 32*DrawSize;
		UIHeight = 3 + 16*DrawSize;
	} else UIWidth = UIHeight = 0;

	if(canChop) {
		i32 anim = GetTime() * 5;
		drawTileFixed(GetMouseX() - 3, GetMouseY() - 3, 21 + anim % 3, 30, WHITE, DrawSize);
	} else if(canFarm) {
		i32 anim = GetTime() * 5;
		drawTileFixed(GetMouseX() - 3, GetMouseY() - 3, 24 + anim % 3, 30, WHITE, DrawSize);
	} else drawTileFixed(GetMouseX() - 3, GetMouseY() - 3, 20, 30, WHITE, DrawSize);
}
