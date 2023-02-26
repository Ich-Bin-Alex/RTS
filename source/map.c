#include "string.h"
#include "source/tools/raylib.h"
#include "source/tools/raymath.h"
#include "source/tools/helper.h"
#include "source/game.h"
#include "source/map.h"

enum eTileset {
	T_FLOOR = 0, T_FLOOR_END = 5,
	T_FLOOR_FLAT = 0, T_FLOOR_FLAT_END = 3,
	T_FLOOR_GROWN = 3, T_FLOOR_GROWN_END = 5,
	T_TREE = 16, T_TREE2 = 32,
	T_FOREST = 34,
	T_WATER = 44,
	T_MOUNTAIN = 64,
};

i32 CameraX, CameraY;
tTile Map[MAP_SIZE][MAP_SIZE];

static i32 StartDrawX, StartDrawY, EndDrawX, EndDrawY;
const i32 NX[8] = {-1,-1,0,1,1,1,0,-1}, NY[8] = {0,-1,-1,-1,0,1,1,1};
static const i32 DNX[4] = {-1,0,1,0}, DNY[4] = {0,-1,0,1};

void setSafe(u32 x, u32 y, tTile tile) {
	if(x < MAP_SIZE && y < MAP_SIZE) Map[x][y] = tile;
}

static tTile DummyTile = {Move: 0xff, Seen: false};

tTile getSafe(u32 x, u32 y) {
	if(x >= MAP_SIZE || y >= MAP_SIZE) return DummyTile;
	return Map[x][y];
}

tTile *refSafe(u32 x, u32 y) {
	if(x >= MAP_SIZE || y >= MAP_SIZE) return &DummyTile;
	return &Map[x][y];
}

bool isTree(u32 x, u32 y) {
	tTile tile = getSafe(x, y);
	return (tile.Top == T_TREE || tile.Top == T_TREE2) || (tile.Top >= 16 && tile.Top <= 21) ||
	       (tile.Top >= 32 && tile.Top <= 37) || (tile.Bottom >= 48 && tile.Bottom <= 53);
}

bool isFarm(u32 x, u32 y) {
	tTile tile = getSafe(x, y);
	return (tile.Bottom == 0xa0 || tile.Bottom == 0xa1) ||
	       (tile.Bottom == 0xb0 || tile.Bottom == 0xb1);
}

bool isReachable(u32 x, u32 y) {
	if(getSafe(x, y).Unreachable) return false;
	for(i32 i = 0; i < 8; i++)
		if(!getSafe((i32)x + NX[i],(i32)y + NY[i]).Move) return true;
	return false;
}

i32 toMapX(f32 x) {
	return x*DRAW_SIZE-CameraX;
}

i32 toMapY(f32 y) {
	return y*DRAW_SIZE-CameraY;
}

void createMap(u32 seed) {
	#define IS_FLOOR(x) (x.Bottom >= 0 && x.Bottom <= 15)
	#define IS_TREE(x) (x.Top == T_TREE || x.Top == T_TREE2)
	#define IS_FOREST(x) ((x.Bottom >= 16 && x.Bottom <= 63) || (x.Top >= 16 && x.Top <= 63))
	#define IS_MOUNTAIN(x) ((x.Bottom >= 64 && x.Bottom <= 79) || (x.Top >= 64 && x.Top <= 79))

	f32 perlin(f32 x, f32 y, u32 seed, i32 octaves) {
		static u8 hash[] =
		{208,34,231,213,32,248,233,56,161,78,24,140,
		71,48,140,254,245,255,247,247,40,185,248,251,245,28,124,204,
		204,76,36,1,107,28,234,163,202,224,245,128,167,204,9,92,217,
		54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,
		204,8,81,70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,
		169,112,32,97,53,195,13,203,9,47,104,125,117,114,124,165,203,
		181,235,193,206,70,180,174,0,167,181,41,164,30,116,127,198,245,
		146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,228,108,
		245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,
		237,55,89,232,50,217,64,244,157,199,121,252,90,17,212,203,149,
		152,140,187,234,177,73,174,193,100,192,143,97,53,145,135,19,
		103,13,90,135,151,199,91,239,247,33,39,145,101,120,99,3,186,86,
		99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,135,
		176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,
		137,139,255,114,20,218,113,154,27,127,246,250,1,8,198,250,209,
		92,222,173,21,88,102,219};
	
		i32 noise(i32 x, i32 y, u8 *hash, u32 seed) {
			static u32 tmp;
			tmp = hash[(y + seed) % 256];
			return hash[(tmp + x) % 256];
		}
		
		f32 inter(f32 x, f32 y, f32 s) {
			return x + s * s * (3 - 2 * s) * (y - x);
		}
		
		f32 amp = 1.0, fin = 0, div = 0.0;
	
		for(i32 i = 0; i < octaves; i++) {
			div += 256 * amp;
			
			i32 x_int = x, y_int = y;
			f32 x_frac = x - x_int, y_frac = y - y_int;
			i32 s = noise(x_int, y_int, hash, seed);
			i32 t = noise(x_int + 1, y_int,     hash, seed);
			i32 u = noise(x_int,     y_int + 1, hash, seed);
			i32 v = noise(x_int + 1, y_int + 1, hash, seed);
			f32 low  = inter(s, t, x_frac);
			f32 high = inter(u, v, x_frac);
			
			fin += inter(low, high, y_frac) * amp;
			amp /= 2;
			x *= 2;
			y *= 2;
		}
	
		return fin/div;
	}

	void placePatch(u32 x, u32 y, tTile tile, i32 scale) {
		for(i32 i = 0; i < scale; i++) {
			setSafe(x, y, tile);
			for(i32 j = 0; j < 8; j++)
				if(GetRandomValue(0, 100) < 50) setSafe((i32)x+NX[j], (i32)y+NY[j], tile);
			x = GetRandomValue(x-1, x+1);
			y = GetRandomValue(y-1, y+1);
		}
	}

	void placeCircle(u32 x, u32 y, i32 radius, tTile tile) {
		for(i32 yi = -radius; yi <= radius; yi++) for(i32 xi = -radius; xi <= radius; xi++)
			if(xi*xi + yi*yi <= radius*radius) setSafe(x+xi, y+yi, tile);
	}

	void placePath(u32 x1, u32 y1, u32 x2, u32 y2, i32 radius) {
		Vector2 dir = Vector2Normalize((Vector2){x2-x1, y2-y1}), pos = {x1, y1};
		for(i32 i = 0; i < 512; i++) {
			if(i % 2 == 0) dir = Vector2Rotate(dir, GetRandomValue(-1000, 1000) / 1000.0 * PI/4);
			else dir = Vector2Normalize((Vector2){x2-pos.x, y2-pos.y});

			for(i32 j = 0; j <= 8.0; j++) {
				placeCircle(pos.x, pos.y, radius, (tTile){Bottom: T_FLOOR});
				pos = Vector2Add(pos, dir);
				if(Vector2Distance(pos, (Vector2){x2,y2}) < 1.0) return;
			}
		}
	}

	bool compare(tTile tile1, tTile tile2) {
		return tile1.Top == tile2.Top && tile1.Bottom == tile2.Bottom;
	}

	void placeBorders(u32 x, u32 y, tTile border[15]) {
		bool n = compare(getSafe(x,y-1), border[0]), s = compare(getSafe(x,y+1), border[0]);
		bool w = compare(getSafe(x-1,y), border[0]), e = compare(getSafe(x+1,y), border[0]);
		bool nw = compare(getSafe(x-1,y-1), border[0]), ne = compare(getSafe(x+1,y-1), border[0]);
		bool sw = compare(getSafe(x-1,y+1), border[0]), se = compare(getSafe(x+1,y+1), border[0]);
		if(      n && !s && !w && !e) setSafe(x, y, border[4]);
		else if(!n &&  s && !w && !e) setSafe(x, y, border[3]);
		else if(!n && !s &&  w && !e) setSafe(x, y, border[2]);
		else if(!n && !s && !w &&  e) setSafe(x, y, border[1]);
		else if( n && !s &&  w && !e) setSafe(x, y, border[14]);
		else if( n && !s && !w &&  e) setSafe(x, y, border[13]);
		else if(!n &&  s &&  w && !e) setSafe(x, y, border[12]);
		else if(!n &&  s && !w &&  e) setSafe(x, y, border[11]);

		if     (se && !n && !s && !w && !e) setSafe(x, y, border[5]);
		else if(sw && !n && !s && !w && !e) setSafe(x, y, border[6]);
		else if(ne && !n && !s && !w && !e) setSafe(x, y, border[7]);
		else if(nw && !n && !s && !w && !e) setSafe(x, y, border[8]);

		if     (n && se && !s && !w && !e) setSafe(x, y, border[13]);
		else if(s && nw && !n && !w && !e) setSafe(x, y, border[12]);
		else if(s && ne && !n && !w && !e) setSafe(x, y, border[11]);
		else if(e && nw && !n && !w && !s) setSafe(x, y, border[13]);
		else if(w && se && !n && !e && !s) setSafe(x, y, border[12]);
		else if(e && sw && !n && !w && !s) setSafe(x, y, border[11]);
		else if(w && ne && !n && !e && !s) setSafe(x, y, border[14]);
		else if(n && sw && !s && !w && !e) setSafe(x, y, border[14]);

		if     (ne && sw && !n && !s && !w && !e) setSafe(x, y, border[9]);
		else if(nw && se && !n && !s && !w && !e) setSafe(x, y, border[10]);

		if     ( n &&  s && !w && !e) setSafe(x, y, border[0]);
		else if(!n && !s &&  w &&  e) setSafe(x, y, border[0]);
		else if( n &&  s &&  w && !e) setSafe(x, y, border[0]);
		else if( n &&  s && !w &&  e) setSafe(x, y, border[0]);
		else if( n && !s &&  w &&  e) setSafe(x, y, border[0]);
		else if(!n &&  s &&  w &&  e) setSafe(x, y, border[0]);
		else if( n &&  s &&  w &&  e) setSafe(x, y, border[0]);
		else if( n &&  w && se) setSafe(x, y, border[0]);
		else if( n &&  e && sw) setSafe(x, y, border[0]);
		else if( s &&  w && ne) setSafe(x, y, border[0]);
		else if( s &&  e && nw) setSafe(x, y, border[0]);
	}

	SetRandomSeed(seed);

	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		Map[x][y].Bottom = 0;
		Map[x][y].Seen = false;
		f32 p = perlin(x*0.2, y*0.2, seed, 4);
		if(p > 0.65) setSafe(x, y, (tTile){Top: T_MOUNTAIN, Move: 0xff});
		else if(p > 0.6) setSafe(x, y, (tTile){Top: T_FOREST, Move: 0xff});
		else if(p > 0.55) setSafe(x, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
	}

	const u32 waterX[4] = {1,16,MAP_SIZE-16,MAP_SIZE-1}, waterY[4] = {1,MAP_SIZE-16,16,MAP_SIZE-1};
	for(i32 i = 0; i < 4; i++) // Generate lakes
		placePatch(waterX[i] + GetRandomValue(-16, 16), waterY[i] + GetRandomValue(-16, 16), 
			(tTile){Bottom: T_WATER, Move: 0xff}, GetRandomValue(20, 40));
	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		if(Map[x][y].Bottom == T_WATER) for(i32 i = 1; i < 3; i++) for(i32 j = 0; j < 8; j++) {
			if(getSafe(x+NX[j]*i, y+NY[j]*i).Bottom != T_WATER) // Remove water/forest borders
				setSafe(x+NX[j]*i, y+NY[j]*i, (tTile){Bottom: T_FLOOR});
		}
	}

	placeCircle(16, 16, 8, (tTile){Bottom: T_FLOOR});
	placeCircle(MAP_SIZE-16, MAP_SIZE-16, 8, (tTile){Bottom: T_FLOOR});

	for(i32 x = 0; x < MAP_SIZE/8; x++) for(i32 y = 0; y < MAP_SIZE/8; y++) // Random small trees
		setSafe(x*8 + GetRandomValue(-3, 3), y*8 + GetRandomValue(-3, 3), 
			(tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});

	placePath(8, 8, MAP_SIZE-8, MAP_SIZE-8, 3);

	for(i32 i = 0; i < 2; i++) for(u32 x = 0; x < MAP_SIZE; x++) for(u32 y = 0; y < MAP_SIZE; y++) {
		if(Map[x][y].Top != T_FOREST) placeBorders(x, y, (tTile[15]){
			{Top: T_FOREST, Move: 0xff},
			{Top:    33, Move: 0xff}, // Left
			{Top:    35, Move: 0xff}, // Right
			{Top:    18, Move: 0xff}, // Top
			{Bottom: 50, Move: 0xff}, // Bottom
			{Top:    17, Move: 0xff}, // Top Left (outer corner)
			{Top:    19, Move: 0xff}, // Top Right (outer corner)
			{Bottom: 49, Move: 0xff}, // Bottom Left (outer corner)
			{Bottom: 51, Move: 0xff}, // Bottom Right (outer corner)
			{Bottom: 52, Move: 0xff}, // Bottom Left & Top Right (diagonal)
			{Bottom: 53, Move: 0xff}, // Bottom Right & Top Left (diagonal)
			{Top:    20, Move: 0xff}, // Right & Bottom (inner corner)
			{Top:    21, Move: 0xff}, // Left & Bottom (inner corner)
			{Top:    36, Move: 0xff}, // Right & Top (inner corner)
			{Top:    37, Move: 0xff}, // Left & Top (inner corner)
		});
		if(Map[x][y].Bottom != T_WATER) placeBorders(x, y, (tTile[15]){
			{Bottom: T_WATER,       Move: 0xff},
			{Bottom: choice(57,43), Move: 0xff}, // Left
			{Bottom: choice(58,45), Move: 0xff}, // Right
			{Bottom: choice(26,28), Move: 0xff}, // Top
			{Bottom: choice(42,60), Move: 0xff}, // Bottom
			{Bottom: 27,            Move: 0xff}, // Top Left (outer corner)
			{Bottom: 29,            Move: 0xff}, // Top Right (outer corner)
			{Bottom: 59,            Move: 0xff}, // Bottom Left (outer corner)
			{Bottom: 61,            Move: 0xff}, // Bottom Right (outer corner)
			{Bottom: 62,            Move: 0xff}, // Bottom Left & Top Right (diagonal)
			{Bottom: 63,            Move: 0xff}, // Bottom Right & Top Left (diagonal)
			{Bottom: 30,            Move: 0xff}, // Right & Bottom (inner corner)
			{Bottom: 31,            Move: 0xff}, // Left & Bottom (inner corner)
			{Bottom: 46,            Move: 0xff}, // Right & Top (inner corner)
			{Bottom: 47,            Move: 0xff}, // Left & Top (inner corner)
		});
	}

	for(u32 x = 0; x < MAP_SIZE; x++) for(u32 y = 0; y < MAP_SIZE; y++) {
		if(getSafe(x,y).Top == T_MOUNTAIN) { // Mountains
			bool n = IS_MOUNTAIN(getSafe(x, y-1)), s = IS_MOUNTAIN(getSafe(x, y+1));
			if     ( n &&  s) setSafe(x, y, (tTile){Bottom: 67, Move: 0xff});
			else if( n && !s) setSafe(x, y, (tTile){Bottom: GetRandomValue(68, 69), Move: 0xff});
			else if(!n &&  s) setSafe(x, y, (tTile){Top: GetRandomValue(65, 66), Move: 0xff});
		}

		if(getSafe(x,y).Bottom == 0) { // Floor and small trees
			tTile nt = getSafe(x,y-1), st = getSafe(x,y+1), wt = getSafe(x-1,y), et = getSafe(x+1,y);
			bool n = IS_FOREST(nt), s = IS_FOREST(st), w = IS_FOREST(wt), e = IS_FOREST(et);
			bool ns = IS_TREE(nt), ss = IS_TREE(st), ws = IS_TREE(wt), es = IS_TREE(et);
			if(n || s || w || e) {
				if(!getSafe(x,y).Top && !ns && !ss && !ws && !es && GetRandomValue(0,4) == 0)
					setSafe(x, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
				else Map[x][y].Bottom = GetRandomValue(T_FLOOR_FLAT, T_FLOOR_FLAT_END);
			} else Map[x][y].Bottom = GetRandomValue(T_FLOOR_GROWN, T_FLOOR_GROWN_END);
		}

		if(isTree(x, y)) {
			Map[x][y].MaxOccupy = 0;
			for(i32 i = 0; i < 4; i++) {
				if(!getSafe((i32)x+DNX[i], (i32)y+DNY[i]).Move) {
					if(++Map[x][y].MaxOccupy >= 2) break;
				}
			}
		}
	}

	for(u32 x = 1; x < MAP_SIZE-1; x++) for(u32 y = 1; y < MAP_SIZE-1; y++) {
		if(Map[x][y].Move == 0xff) { // Fix diagonal holes to prevent pathfinding glitches
			if(Map[x+1][y+1].Move == 0xff && !Map[x+1][y].Move && !Map[x][y+1].Move)
				setSafe(x+1, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
			if(Map[x-1][y+1].Move == 0xff && !Map[x-1][y].Move && !Map[x][y+1].Move)
				setSafe(x-1, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
			if(Map[x+1][y-1].Move == 0xff && !Map[x+1][y].Move && !Map[x][y-1].Move)
				setSafe(x+1, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
			if(Map[x-1][y-1].Move == 0xff && !Map[x-1][y].Move && !Map[x][y-1].Move)
				setSafe(x-1, y, (tTile){Top: choice(T_TREE, T_TREE2), Move: 0xff});
		}
		//Map[x][y].Seen = true;
	}

	for(i32 i = 0; i < MAP_SIZE; i++) // Prevent walking beyond the map borders
		Map[i][0].Move = Map[i][MAP_SIZE-1].Move = Map[0][i].Move = Map[MAP_SIZE-1][i].Move = 0xff;
}

void beginDrawMap(void) {
	u32 hash(u32 x, u32 y) {
		u32 qx = 1103515245 * ((x >> 1) ^ y), qy = 1103515245 * ((y >> 1) ^ x);
		return 1103515245 * (qx ^ (qy >> 3));
	}

	StartDrawX = max(0, CameraX/DRAW_SIZE/8);
	StartDrawY = max(0, CameraY/DRAW_SIZE/8);
	EndDrawX = min(MAP_SIZE, (GetScreenWidth() + CameraX)/DRAW_SIZE/8 + 1);
	EndDrawY = min(MAP_SIZE, (GetScreenHeight() + CameraY)/DRAW_SIZE/8 + 1);

	i32 anim = (i32)(GetTime() * 4.0) % 4;
	for(i32 x = StartDrawX; x < EndDrawX; x++) for(i32 y = StartDrawY; y < EndDrawY; y++) {
		if(!Map[x][y].Seen) continue;
		u32 tx = Map[x][y].Bottom & 0x0f, ty = Map[x][y].Bottom >> 4;
		if(tx >= 9 && ty >= 1 && ty <= 3) {
			ty += anim * 3; // Water animation
			Map[x][y].Blood = 0; // Disable blood in water
		}
		drawTile(x, y, tx, ty, 1.0);
		if(Map[x][y].Blood) drawTile(x, y, hash(x, y) % 12, 14, Map[x][y].Blood);
	}
	for(i32 x = 0; x < MAP_SIZE; x++) for(i32 y = 0; y < MAP_SIZE; y++) {
		if(Map[x][y].Blood) { // Fade out blood on the floor
			Map[x][y].Blood -= GetFrameTime()*0.0125;
			if(Map[x][y].Blood < 0) Map[x][y].Blood = 0;
		}
	}
}

void endDrawMap(void) {
	for(i32 x = StartDrawX; x < EndDrawX; x++) for(i32 y = StartDrawY; y < EndDrawY; y++) {
		if(Map[x][y].Seen) {
			if(Map[x][y].Top) drawTile(x, y, Map[x][y].Top & 0x0f, Map[x][y].Top >> 4, 1.0);
			if(Map[x][y].Animation) {
				u32 tx = (Map[x][y].Animation & 0x0f) + Map[x][y].Frame, ty = Map[x][y].Animation >> 4;
				drawTile(x, y, tx, ty, 1.0);
				Map[x][y].Frame += GetFrameTime() * (Map[x][y].Animation <= 0xef ? 16.0 : 24.0);
				if(Map[x][y].Frame >= 8) Map[x][y].Animation = 0;
			}
		}

		for(i32 i = 1; i < 3; i++) for(i32 j = 0; j < 8; j++) { // Fog of war
			if(!getSafe(x+NX[j]*i, y+NY[j]*i).Seen) {
				DrawRectangle(x*8*DRAW_SIZE-CameraX, y*8*DRAW_SIZE-CameraY, 
					8*DRAW_SIZE, 8*DRAW_SIZE, (Color){0,0,0,92});
				break;
			}
		}
	}
}
