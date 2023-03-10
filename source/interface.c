#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/helper.h"
#include "source/tools/vector.h"
#include "source/game.h"
#include "source/map.h"
#include "source/unit.h"

i32 CharSizes[0x80] = {[' '] = 4};

typedef struct tTooltip {
	char *Text;
	bool Builder;
	tUnitType *Unit;
	tBuildingType *Build;
	KeyboardKey Hotkey;
} tTooltip;

static tTooltip Tooltip;

static f32 MoveAnim, CursorOffset;
static bool Selected = false, RectSelect = false;
static UnitHandle MoveTarget, UnitUnderMouse;
static BuildingHandle SelectedBuild;
static vec2 Select1, Select2, MovePos;
static bool ShowDebug = false;
static i32 UIWidth, UIHeight;
static tBuildingType *BuildLock = NULL;
static bool BuildOk[3][3] = {false};

static const u32 IconTY = 29, PopIconTX = 16, FoodIconTX = 17, WoodIconTX = 18;

static void drawText(char *text, i32 x, i32 y, Color color) {
	for(i32 i = 0; text[i]; i++) {
		u32 tx = (text[i] - ' ') % 16, ty = 16 + (text[i] - ' ') / 16;
		for(i32 j = 0; j < 8; j++) drawTileFixed(x+NX[j], y+NY[j], tx, ty, BLACK, FontSize); // Shadow
		drawTileFixed(x, y, tx, ty, color, FontSize);
		x += CharSizes[(u32)text[i]] * FontSize;
	}
}

static void drawTextAnimated(char *text, i32 x, i32 y, Color color[5]) {
	i32 anim = (i32)floor(GetTime() * 10) % 10;
	if(anim >= 5) anim = 9 - anim;
	for(i32 i = 0; text[i]; i++) {
		u32 tx = (text[i] - ' ') % 16, ty = 16 + (text[i] - ' ') / 16;
		for(i32 j = 0; j < 8; j++) drawTileFixed(x+NX[j], y+NY[j], tx, ty, BLACK, FontSize); // Shadow
		drawTileFixed(x, y, tx, ty, color[anim], FontSize);
		x += CharSizes[(u32)text[i]] * FontSize;
	}
}

static i32 measureText(char *text) {
	i32 length = 0;
	for(i32 i = 0; text[i]; i++) length += CharSizes[(u32)text[i]] * FontSize;
	return length;
}

static void drawTooltip(tTooltip tooltip) {
	i32 x = GetMouseX(), y = GetMouseY(), width = 4*DrawSize, height = 6*FontSize+2*DrawSize;
	if(tooltip.Text) width += measureText(tooltip.Text);
	else if(tooltip.Unit) width += measureText(tooltip.Unit->Name);
	else if(tooltip.Build) width += measureText(tooltip.Build->Name);
	if(tooltip.Builder) {
		height += 12*FontSize;
		y -= 12*FontSize;
	}
	if(x + width > GetScreenWidth()) x -= width;
	if(y - height - DrawSize*3 < 0) y += height + DrawSize*3;
	if(tooltip.Text) {
		DrawRectangle(x-2*DrawSize, y-DrawSize*8, width, height, GetColor(0x00000080));
		drawText(tooltip.Text, x, y-DrawSize*8, WHITE);
	} else if(tooltip.Unit && !tooltip.Builder) {
		DrawRectangle(x-2*DrawSize, y-DrawSize*8, width, height, GetColor(0x00000080));
		drawText(tooltip.Unit->Name, x, y-DrawSize*8, WHITE);
	} else if(tooltip.Build && !tooltip.Builder) {
		DrawRectangle(x-2*DrawSize, y-DrawSize*8, width, height, GetColor(0x00000080));
		drawText(tooltip.Build->Name, x, y-DrawSize*8, WHITE);
	} else if(tooltip.Build && tooltip.Builder) {
		DrawRectangle(x-2*DrawSize, y-DrawSize*8, width, height, GetColor(0x00000080));
		drawText(tooltip.Build->Name, x, y-DrawSize*8, WHITE);
		if(tooltip.Build->WoodCost) {
			char *text = TextFormat("%d", tooltip.Build->WoodCost);
			drawTileFixed(x + measureText(text) + FontSize, y-DrawSize*8+FontSize*10, 
				WoodIconTX, IconTY, WHITE, DrawSize);
			drawText(text, x, y-DrawSize*8+FontSize*11, 
				Player[0].Wood >= tooltip.Build->WoodCost ? WHITE : BadColor);
		}
	}
}

static bool drawActionButton(i32 x, i32 y, u32 tx, u32 ty, tTooltip tooltip) {
	i32 mx = GetMouseX(), my = GetMouseY();
	drawTileFixed(x, y, 18, 27, WHITE, DrawSize);
	drawTileFixed(x + 8*DrawSize, y, 19, 27, WHITE, DrawSize);
	drawTileFixed(x, y + 8*DrawSize, 18, 28, WHITE, DrawSize);
	drawTileFixed(x + 8*DrawSize, y + 8*DrawSize, 19, 28, WHITE, DrawSize);
	drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, WHITE, DrawSize);
	if(mx > x && mx < x + 16*DrawSize && my > y && my < y + 16*DrawSize) {
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			drawTileFixed(x, y, 20, 27, WHITE, DrawSize);
			drawTileFixed(x + 8*DrawSize, y, 21, 27, WHITE, DrawSize);
			drawTileFixed(x, y + 8*DrawSize, 20, 28, WHITE, DrawSize);
			drawTileFixed(x + 8*DrawSize, y + 8*DrawSize, 21, 28, WHITE, DrawSize);
			drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, WHITE, DrawSize);
			drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, GetColor(0x00000080), DrawSize);
		} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return true;
		else Tooltip = tooltip;
	}
	return false;
}

static bool drawDisabledButton(i32 x, i32 y, u32 tx, u32 ty, tTooltip tooltip) {
	i32 mx = GetMouseX(), my = GetMouseY();
	drawTileFixed(x, y, 18, 27, GRAY, DrawSize);
	drawTileFixed(x + 8*DrawSize, y, 19, 27, GRAY, DrawSize);
	drawTileFixed(x, y + 8*DrawSize, 18, 28, GRAY, DrawSize);
	drawTileFixed(x + 8*DrawSize, y + 8*DrawSize, 19, 28, GRAY, DrawSize);
	drawTileFixed(x + 4*DrawSize, y + 4*DrawSize, tx, ty, GRAY, DrawSize);
	if(mx > x && mx < x + 16*DrawSize && my > y && my < y + 16*DrawSize) Tooltip = tooltip;
	return false;
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
	Tooltip = (tTooltip){NULL};
	i32 width = GetScreenWidth(), height = GetScreenHeight();
	i32 mx = GetMouseX(), my = GetMouseY();
	i32 x = (mx + CameraX)/DrawSize, y = (my + CameraY)/DrawSize;
	bool inUI = (my > height - 3 - 16*DrawSize && mx <= UIWidth) ||
	            (mx <= 3 + 16*DrawSize && my >= height - UIHeight);

	if(IsKeyDown(KEY_RIGHT) || mx >= width - 15) CameraX += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_LEFT) || (mx <= 15 && !inUI))
		CameraX -= round(1000.0 * GetFrameTime());
	if(IsKeyDown(KEY_DOWN) || (my >= height - 15 && !inUI))
		CameraY += round(1000.0 * GetFrameTime());
	else if(IsKeyDown(KEY_UP) || my <= 15) CameraY -= round(1000.0 * GetFrameTime());
	if(CameraX > MAP_SIZE*8*DrawSize-width) CameraX = MAP_SIZE*8*DrawSize-width;
	if(CameraX < 0) CameraX = 0;
	if(CameraY > MAP_SIZE*8*DrawSize-height) CameraY = MAP_SIZE*8*DrawSize-height;
	if(CameraY < 0) CameraY = 0;

	if(IsKeyDown(KEY_LEFT_SHIFT)) { // Cheats
		static i32 Magellan = 0, Kroisos = 0, Adam = 0;
		if(IsKeyPressed(KEY_M) && Magellan == 0) Magellan++;
		else if(IsKeyPressed(KEY_A) && Magellan == 1) Magellan++;
		else if(IsKeyPressed(KEY_G) && Magellan == 2) Magellan++;
		else if(IsKeyPressed(KEY_E) && Magellan == 3) Magellan++;
		else if(IsKeyPressed(KEY_L) && Magellan == 4) Magellan++;
		else if(IsKeyPressed(KEY_L) && Magellan == 5) Magellan++;
		else if(IsKeyPressed(KEY_A) && Magellan == 6) Magellan++;
		else if(IsKeyPressed(KEY_N) && Magellan == 7) Magellan++;
		else if(GetKeyPressed()) Magellan = 0;

		if(IsKeyPressed(KEY_K) && Kroisos == 0) Kroisos++;
		else if(IsKeyPressed(KEY_R) && Kroisos == 1) Kroisos++;
		else if(IsKeyPressed(KEY_O) && Kroisos == 2) Kroisos++;
		else if(IsKeyPressed(KEY_I) && Kroisos == 3) Kroisos++;
		else if(IsKeyPressed(KEY_S) && Kroisos == 4) Kroisos++;
		else if(IsKeyPressed(KEY_O) && Kroisos == 5) Kroisos++;
		else if(IsKeyPressed(KEY_S) && Kroisos == 6) Kroisos++;
		else if(GetKeyPressed()) Kroisos = 0;

		if(IsKeyPressed(KEY_A) && Adam == 0) Adam++;
		else if(IsKeyPressed(KEY_D) && Adam == 1) Adam++;
		else if(IsKeyPressed(KEY_A) && Adam == 2) Adam++;
		else if(IsKeyPressed(KEY_M) && Adam == 3) Adam++;
		else if(GetKeyPressed()) Adam = 0;

		if(Magellan == 8) {
			for(u32 x2 = 0; x2 < MAP_SIZE; x2++) for(u32 y2 = 0; y2 < MAP_SIZE; y2++) 
				Map[x2][y2].Seen = true;
			Magellan = Kroisos = Adam = 0;
		} else if(Kroisos == 7) {
			Player[0].Food += 200;
			Player[0].Wood += 200;
			Magellan = Kroisos = Adam = 0;
		} else if(Adam == 4) {
			vec2 pos = vec2(16.0 + GetRandomValue(-100, 100)/100.0,
			                16.0 + GetRandomValue(-100, 100)/100.0);
			newUnit((tUnit){Type: &Peasent, Position: pos});
			Magellan = Kroisos = Adam = 0;
		}
	}

	if(BuildLock) {
		if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) BuildLock = NULL;
		else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			bool ok = true;
			for(i32 xi = 0; xi < BuildLock->SizeX; xi++) for(i32 yi = 0; yi < BuildLock->SizeY; yi++)
				if(!BuildOk[xi][yi]) ok = false;
			if(ok && Player[0].Food >= BuildLock->FoodCost && Player[0].Wood >= BuildLock->WoodCost) {
				Player[0].Food -= BuildLock->FoodCost;
				Player[0].Wood -= BuildLock->WoodCost;
				forEachUnit(i) if(Units[i].Selected) {
					tMoveOrder *move = newMoveOrder((tMoveOrder){Target: vec2(x/8, y/8)});
					moveUnit(i, move);
					unitAction(i, ACTION_MOVE_AND_BUILD, vec2(x/8, y/8), 0);
					Units[i].Build.Building = newBuilding(
						(tBuilding){Type: BuildLock, Player: 0, Finished: false}, x/8, y/8);
					break;
				}
				BuildLock = NULL;
			}
		}
		return;
	}

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inUI) {
		Selected = RectSelect = false;
		SelectedBuild = 0;
		forEachUnit(i) {
			Units[i].Selected = false;
			if(x > Units[i].Position.x*8 && x < Units[i].Position.x*8+8 && !Units[i].Player &&
			   y > Units[i].Position.y*8 && y < Units[i].Position.y*8+8 && !Selected)
				Units[i].Selected = Selected = true;
		}
		BuildingHandle build = getBuilding(x / 8, y / 8);
		if(!Selected && !Buildings[build].Player && !Selected) SelectedBuild = build;
	} else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !inUI) {
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
			if(Selected) SelectedBuild = 0;
		}
		RectSelect = false;
	} else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !inUI) {
		if(!RectSelect) Select1 = GetMousePosition();
		Select2 = GetMousePosition();
		RectSelect = true;
	} else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Selected) {
		RectSelect = false;
		MovePos = vec2(x/8.0, y/8.0);
		tMoveOrder *move;
		MoveTarget = 0;
		bool canChop = isTree(MovePos.x, MovePos.y) && getSafe(MovePos.x, MovePos.y).Seen;
		bool canFarm = isFarm(MovePos.x, MovePos.y) && getSafe(MovePos.x, MovePos.y).Seen &&
			Buildings[getSafe(MovePos.x, MovePos.y).Building].Finished &&
			!Buildings[getSafe(MovePos.x, MovePos.y).Building].Player &&
			!Buildings[getSafe(MovePos.x, MovePos.y).Building].Farm.Occupier;
		BuildingHandle build = getBuilding(MovePos.x, MovePos.y);
		bool canBuild = build != 0 && !Buildings[build].Finished && !Buildings[build].Build.Occupier;
		UnitHandle farmer = 0, builder = 0;
		if(UnitUnderMouse && Units[UnitUnderMouse].Player) {
			MoveTarget = UnitUnderMouse;
			move = newMoveOrder((tMoveOrder){Target: MovePos, Follow: MoveTarget});
		} else {
			if(canFarm) {
				BuildingHandle build = getBuilding(MovePos.x, MovePos.y);
				MovePos = vec2(Buildings[build].FirstX + Buildings[build].Type->SizeX/2 - 0.5,
				               Buildings[build].FirstY + Buildings[build].Type->SizeY/2 - 0.75);
			}
			move = newMoveOrder((tMoveOrder){Target: vec2(round(MovePos.x), round(MovePos.y))});
		}
		i32 numSelected = 0, numUnmoveable = 0;
		vec2 mid = vec2(0);
		forEachUnit(i) if(Units[i].Selected) {
			if(!Units[i].Type->CanChop) canChop = false;
			if(!Units[i].Type->CanFarm) canFarm = false;
			if(!Units[i].Type->CanBuild) canBuild = false;
		}
		forEachUnit(i) if(Units[i].Selected) {
			mid = vadd(mid, Units[i].Position);
			moveUnit(i, move);
			unitAction(i, ACTION_MOVE, MovePos, 0);
			numSelected++;
			vec2 flow = getUnitFlow(i);
			if(!flow.x && !flow.y) numUnmoveable++;
			if(canFarm && !farmer) {
				farmer = i;
				break;
			} else if(canBuild && !builder) {
				builder = i;
				break;
			}
		}
		if(!numSelected) {
			Selected = false;
			return;
		}
		vec2 dir = vnormalize(vsub(vdiv(mid, numSelected), MovePos));

		// Search for valid position, in case the desired position is unreachable
		while(numSelected == numUnmoveable || getSafe(MovePos.x, MovePos.y).Move == 0xff) {
			if(canBuild && builder) break;
			canFarm = false;
			numUnmoveable = 0;
			MovePos = vadd(MovePos, dir);
			move = newMoveOrder((tMoveOrder){Target: vec2(round(MovePos.x), round(MovePos.y))});
			forEachUnit(i) if(Units[i].Selected) {
				moveUnit(i, move);
				vec2 flow = getUnitFlow(i);
				if(!flow.x && !flow.y) numUnmoveable++;
			}
			if((u32)MovePos.x >= MAP_SIZE || (u32)MovePos.y >= MAP_SIZE) {
				MovePos = vec2(0);
				break;
			}
		}

		CursorOffset = 0.5;
		if(canChop) {
			forEachUnit(i) if(Units[i].Selected) unitAction(i, ACTION_MOVE_AND_CHOP, MovePos, 0);
		} else if(canFarm && farmer) {
			tTile tile = getSafe(MovePos.x, MovePos.y);
			if(!Buildings[tile.Building].Farm.Occupier) {
				unitAction(farmer, ACTION_MOVE_AND_FARM, MovePos, 0);
				CursorOffset = 0.0;
			} else moveUnit(farmer, NULL);
		} else if(canBuild && builder) {
			tTile tile = getSafe(MovePos.x, MovePos.y);
			if(!Buildings[tile.Building].Build.Occupier)
				unitAction(builder, ACTION_MOVE_AND_BUILD, MovePos, 0);
			else moveUnit(builder, NULL);
		}

		if(MovePos.x) MoveAnim = 4;
		else if(move && move->References <= 0) freeMoveOrder(move);
	}

	if(IsKeyPressed(KEY_F3)) ShowDebug = 1 - ShowDebug;
}

void beginDrawInterface(void) {
	if(MoveAnim > 0 && !MoveTarget) drawTileFree(
		vec2(MovePos.x - CursorOffset, MovePos.y - CursorOffset), 20 - ceil(MoveAnim), 31);
}

void endDrawInterface(void) {
	i32 mx = GetMouseX(), my = GetMouseY(), width = GetScreenWidth(), height = GetScreenHeight();
	i32 x = (mx + CameraX)/DrawSize, y = (my + CameraY)/DrawSize, numSelected = 0;
	UnitUnderMouse = 0;
	UnitHandle firstSelected = 0;
	bool canChop = Selected && isTree(x / 8, y / 8) && getSafe(x / 8 , y / 8).Seen;
	bool canFarm = Selected && isFarm(x / 8, y / 8) && getSafe(x / 8 , y / 8).Seen &&
		Buildings[getSafe(x / 8, y / 8).Building].Finished &&
		!Buildings[getSafe(x / 8, y / 8).Building].Farm.Occupier;
	BuildingHandle build = getBuilding(x / 8, y / 8);
	bool canBuild = Selected && build && !Buildings[build].Finished && !Buildings[build].Build.Occupier;
	bool inUI = (my > height - 3 - 16*DrawSize && mx <= UIWidth) ||
	            (mx <= 3 + 16*DrawSize && my >= height - UIHeight);

	if(BuildLock) { // Check if location is valid for buildings
		for(i32 xi = 0; xi < BuildLock->SizeX; xi++) for(i32 yi = 0; yi < BuildLock->SizeY; yi++) {
			tTile tile = getSafe(x/8 + xi, y/8 + yi);
			BuildOk[xi][yi] = !tile.Move && tile.Seen && !getBuilding(x/8 + xi, y/8 + yi);
			forEachUnit(i) {
				if(!BuildOk[xi][yi]) break;
				if(round(Units[i].Position.x) == x/8 + xi && round(Units[i].Position.y) == y/8 + yi) 
					BuildOk[xi][yi] = false;
			}
			drawTile(x/8 + xi, y/8 + yi, 22, 27 + !BuildOk[xi][yi], 10);
		}
	}

	forEachUnit(i) {
		f32 x2 = Units[i].Position.x, y2 = Units[i].Position.y;
		if(!UnitUnderMouse && !inUI && x > x2*8 && x < x2*8+8 && y > y2*8 && y < y2*8+8) {
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
	if(SelectedBuild)
		drawBuildingHealthBar(toMapX(Buildings[SelectedBuild].FirstX*8), 
			toMapY(Buildings[SelectedBuild].FirstY*8)-DrawSize*3, SelectedBuild, 
			Buildings[SelectedBuild].Type->SizeX*8);
	if(build && !UnitUnderMouse && !inUI)
		drawBuildingHealthBar(toMapX(Buildings[build].FirstX*8), 
			toMapY(Buildings[build].FirstY*8)-DrawSize*3, build, Buildings[build].Type->SizeX*8);

	if(MoveAnim > 0 && MoveTarget) drawTileFree(Units[MoveTarget].Position, 24-ceil(MoveAnim), 31);

	if(RectSelect)
		DrawRectangleLinesEx((Rectangle){min(Select1.x, Select2.x), min(Select1.y, Select2.y),
			abs(Select2.x-Select1.x), abs(Select2.y-Select1.y)}, DrawSize, WHITE);

	if(MoveAnim > 0) MoveAnim -= GetFrameTime() * 10.0;

	i32 offset = 8*DrawSize + 9;
	char *text = TextFormat("%d/%d", Player[0].Population, Player[0].PopulationLimit);
	drawText(text, width - measureText(text) - offset, 6, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 3, PopIconTX, IconTY, WHITE, DrawSize);

	text = TextFormat("%d", Player[0].Food);
	offset = 8*DrawSize + 9;
	if(Player[0].FoodIncome) {
		char *inc = TextFormat("+%d", Player[0].FoodIncome);
		offset += measureText(inc);
		drawText(inc, width - offset, 8 + 8*DrawSize, GoodColor);
	}
	drawText(text, width - measureText(text) - offset, 8 + 8*DrawSize, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 5 + 8*DrawSize, FoodIconTX, IconTY, WHITE, DrawSize);

	text = TextFormat("%d", Player[0].Wood);
	offset = 8*DrawSize + 9;
	if(Player[0].WoodIncome) {
		char *inc = TextFormat("+%d", Player[0].WoodIncome);
		offset += measureText(inc);
		drawText(inc, width - offset, 10 + 16*DrawSize, GoodColor);
	}
	drawText(text, width - measureText(text) - offset, 10 + 16*DrawSize, TextColor);
	drawTileFixed(width - 3 - 8*DrawSize, 7 + 16*DrawSize, WoodIconTX, IconTY, WHITE, DrawSize);
	if(mx > width - 3 - 8*DrawSize) {
		if(my < 3+8*DrawSize) Tooltip = (tTooltip){Text: "Population"};
		else if(my < 5+16*DrawSize) Tooltip = (tTooltip){Text: "Food"};
		else if(my < 7+24*DrawSize) Tooltip = (tTooltip){Text: "Wood"};
	}

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
			UIHeight = 3 + (16 + (Units[firstSelected].Type->Buildings[0] != 0))*DrawSize;
			i32 anim =  floor(GetTime() * 4.0);
			drawSpriteFixed(3 + 4*DrawSize, height-3-13*DrawSize, 0, anim % 4, 0);
			if(numSelected > 1) {
				char *text = TextFormat("%d", numSelected);
				drawText(text, 3 + 8*DrawSize - measureText(text)/2, height - 8*DrawSize, WHITE);
			} else drawHealthBar(3 + 3*DrawSize, height - 3 - 4*DrawSize, firstSelected, 10);

			for(u32 i = 0; Units[firstSelected].Type->Buildings[i]; i++) {
				tBuildingType *build = Units[firstSelected].Type->Buildings[i];
				u32 tx = 16 + (build->Icon & 0x0f), ty = 16 + (build->Icon >> 4);
				UIHeight += 14*DrawSize;
				if(Player[0].Wood < build->WoodCost || Player[0].Food < build->FoodCost)
					drawDisabledButton(6, height - UIHeight, tx, ty, 
					(tTooltip){Build: build, Builder: true});
				else if(drawActionButton(6, height - UIHeight, tx, ty, 
					(tTooltip){Build: build, Builder: true})) BuildLock = build;
			}
	
			if(drawActionButton(3 + 16*DrawSize, height - 3 - 16*DrawSize, 21, IconTY, 
			  (tTooltip){Text: "Kill unit"})) {
				killUnit(firstSelected);
				firstSelected = 0;
				if(--numSelected <= 0) {
					Select1 = Select2 = vec2(0);
					Selected = RectSelect = false;
				}
			}

			if(mx < 3 + 16*DrawSize && my >= height - 3 - 16*DrawSize) 
				Tooltip = (tTooltip){Unit: Units[firstSelected].Type, Builder: false};
		} else if(SelectedBuild) {
			UIHeight = 3 + 16*DrawSize;
			tBuildingType *type = Buildings[SelectedBuild].Type;
			u32 tx = 16 + (type->Icon & 0x0f), ty = 16 + (type->Icon >> 4);
			drawTileFixed(4 + 4*DrawSize, height-3-13*DrawSize, tx, ty, WHITE, DrawSize);
			drawBuildingHealthBar(3 + 3*DrawSize, height - 3 - 4*DrawSize, SelectedBuild, 10);

			if(drawActionButton(3 + 16*DrawSize, height - 3 - 16*DrawSize, 21, IconTY, 
			  (tTooltip){Text: "Destroy building"})) {
				destroyBuilding(SelectedBuild);
				SelectedBuild = 0;
				Select1 = Select2 = vec2(0);
				Selected = RectSelect = false;
			}

			if(mx < 3 + 16*DrawSize && my >= height - 3 - 16*DrawSize) 
				Tooltip = (tTooltip){Build: Buildings[SelectedBuild].Type, Builder: false};
		}
		UIWidth = 3 + 32*DrawSize;
	} else UIWidth = UIHeight = 0;

	if(Tooltip.Text || Tooltip.Unit || Tooltip.Build) drawTooltip(Tooltip);

	i32 anim = GetTime() * 5;
	if(canChop && !inUI && !BuildLock)
		drawTileFixed(mx - 3, my - 3, 21 + anim % 3, 30, WHITE, DrawSize);
	else if((canFarm || canBuild) && !inUI && !BuildLock)
		drawTileFixed(mx - 3, my - 3, 24 + anim % 3, 30, WHITE, DrawSize);
	else drawTileFixed(mx - 3, my - 3, 20, 30, WHITE, DrawSize);
}
