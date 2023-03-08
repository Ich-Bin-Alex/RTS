#ifndef H_VECTOR
#define H_VECTOR

#include "helper.h"
#include "math.h"

#if defined(RAYMATH_H) || defined(RAYLIB_H)
	typedef Vector2 vec2;
#else
	typedef struct vec2 {
		f32 x, y;
	} vec2;
#endif

vec2 $vec2(f32 x, f32 y);
vec2 $vec2_add_vv(vec2 v1, vec2 v2);
vec2 $vec2_add_fv(f32 v1, vec2 v2);
vec2 $vec2_add_vf(vec2 v1, f32 v2);
vec2 $vec2_sub_vv(vec2 v1, vec2 v2);
vec2 $vec2_sub_fv(f32 v1, vec2 v2);
vec2 $vec2_sub_vf(vec2 v1, f32 v2);
vec2 $vec2_mul_vv(vec2 v1, vec2 v2);
vec2 $vec2_mul_fv(f32 v1, vec2 v2);
vec2 $vec2_mul_vf(vec2 v1, f32 v2);
vec2 $vec2_div_vv(vec2 v1, vec2 v2);
vec2 $vec2_div_fv(f32 v1, vec2 v2);
vec2 $vec2_div_vf(vec2 v1, f32 v2);
vec2 vneg(vec2 v);
vec2 vrotate(vec2 v, f32 angle);
f32 vlength(vec2 v);
f32 vdot(vec2 v1, vec2 v2);
f32 vdistance(vec2 v1, vec2 v2);
vec2 vnormalize(vec2 v);

#define $vec2_0() $vec2(0, 0)
#define $vec2_1(a) $vec2(a, a)
#define $vec2_2(a,b) $vec2(a, b)
#define $vec2_x(x,a,b,func, ...) func  
#define vec2(...) \
	$vec2_x(,##__VA_ARGS__,$vec2_2(__VA_ARGS__),$vec2_1(__VA_ARGS__),$vec2_0(__VA_ARGS__)) 

#define vadd(a,b) _Generic((a), \
vec2: _Generic((b), \
	vec2: $vec2_add_vv, \
	default: $vec2_add_vf), \
default: _Generic((b), \
	vec2: $vec2_add_fv, \
	default: $vec2_add_vf) ) (a,b)

#define vsub(a,b) _Generic((a), \
vec2: _Generic((b), \
	vec2: $vec2_sub_vv, \
	default: $vec2_sub_vf), \
default: _Generic((b), \
	vec2: $vec2_sub_fv, \
	default: $vec2_sub_vf) ) (a,b)

#define vmul(a,b) _Generic((a), \
vec2: _Generic((b), \
	vec2: $vec2_mul_vv, \
	default: $vec2_mul_vf), \
default: _Generic((b), \
	vec2: $vec2_mul_fv, \
	default: $vec2_mul_vf) ) (a,b)	

#define vdiv(a,b) _Generic((a), \
vec2: _Generic((b), \
	vec2: $vec2_div_vv, \
	default: $vec2_div_vf), \
default: _Generic((b), \
	vec2: $vec2_div_fv, \
	default: $vec2_div_vf) ) (a,b)

#endif