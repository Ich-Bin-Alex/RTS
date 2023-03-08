#include "vector.h"
#include "math.h"

vec2 $vec2(f32 x, f32 y) {
	return (vec2){x, y};
}

vec2 $vec2_add_vv(vec2 v1, vec2 v2) {
	return $vec2(v1.x + v2.x, v1.y + v2.y);
}

vec2 $vec2_add_fv(f32 v1, vec2 v2) {
	return $vec2_add_vv($vec2(v1, v1), v2);
}

vec2 $vec2_add_vf(vec2 v1, f32 v2) {
	return $vec2_add_vv(v1, $vec2(v2, v2));
}

vec2 $vec2_sub_vv(vec2 v1, vec2 v2) {
	return $vec2(v1.x - v2.x, v1.y - v2.y);
}

vec2 $vec2_sub_fv(f32 v1, vec2 v2) {
	return $vec2_sub_vv($vec2(v1, v1), v2);
}

vec2 $vec2_sub_vf(vec2 v1, f32 v2) {
	return $vec2_sub_vv(v1, $vec2(v2, v2));
}

vec2 $vec2_mul_vv(vec2 v1, vec2 v2) {
	return $vec2(v1.x * v2.x, v1.y * v2.y);
}

vec2 $vec2_mul_fv(f32 v1, vec2 v2) {
	return $vec2_mul_vv($vec2(v1, v1), v2);
}

vec2 $vec2_mul_vf(vec2 v1, f32 v2) {
	return $vec2_mul_vv(v1, $vec2(v2, v2));
}

vec2 $vec2_div_vv(vec2 v1, vec2 v2) {
	return $vec2(v1.x / v2.x, v1.y / v2.y);
}

vec2 $vec2_div_fv(f32 v1, vec2 v2) {
	return $vec2_div_vv($vec2(v1, v1), v2);
}

vec2 $vec2_div_vf(vec2 v1, f32 v2) {
	return $vec2_div_vv(v1, $vec2(v2, v2));
}

vec2 vneg(vec2 v) {
	return $vec2(-v.x, -v.y);
}

vec2 vrotate(vec2 v, f32 angle) {
	return vec2(v.x*cos(angle) - v.y*sin(angle), v.x*sin(angle) + v.y*cos(angle));
}

f32 vlength(vec2 v) {
	return sqrt(v.x*v.x + v.y*v.y);
}

f32 vdot(vec2 v1, vec2 v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

f32 vdistance(vec2 v1, vec2 v2) {
	return sqrt((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));
}

vec2 vnormalize(vec2 v) {
	f32 len = vlength(v);
	if(len) return $vec2_mul_vf(v, 1.0 / len);
	return $vec2(0, 0);
}
