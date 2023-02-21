#ifndef H_HELPER
#define H_HELPER

#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"

#ifndef u8
	#define u8 uint8_t
#endif
#ifndef u16
	#define u16 uint16_t
#endif
#ifndef u32
	#define u32 uint32_t
#endif
#ifndef u64
	#define u64 uint64_t
#endif

#ifndef i8
	#define i8 int8_t
#endif
#ifndef i16
	#define i16 int16_t
#endif
#ifndef i32
	#define i32 int32_t
#endif
#ifndef i64
	#define i64 int64_t
#endif

#ifndef f32
	#define f32 float
#endif
#ifndef f64
	#define f64 double
#endif

#define ANSI_BLACK "\e[30m"
#define ANSI_RED "\e[31m"
#define ANSI_GREEN "\e[32m"
#define ANSI_YELLOW "\e[33m"
#define ANSI_BLUE "\e[34m"
#define ANSI_PURPLE "\e[35m"
#define ANSI_CYAN "\e[36m"
#define ANSI_GRAY "\e[90m"
#define ANSI_LGRAY "\e[37m"
#define ANSI_LRED "\e[91m"
#define ANSI_LGREEN "\e[92m"
#define ANSI_LYELLOW "\e[93m"
#define ANSI_LBLUE "\e[94m"
#define ANSI_LPURPLE "\e[95m"
#define ANSI_LCYAN "\e[96m"
#define ANSI_WHITE "\e[97m"
#define ANSI_BACK_RED "\e[41m"
#define ANSI_BACK_GRAY "\e[100m"
#define ANSI_BACK_YELLOW "\e[43m"
#define ANSI_BACK_LGRAY "\e[49m"
#define ANSI_BACK_LRED "\e[101m"
#define ANSI_BACK_LGREEN "\e[102m"
#define ANSI_BACK_LYELLOW "\e[103m"
#define ANSI_BACK_WHITE "\e[47m"
#define ANSI_RESET "\e[0m"
#define ANSI_COLV "\e[50D\e["
#define ANSI_COL(x) "\e[50D\e["#x"C"

#define isPowerOf2(x) ((x) && (!((x)&((x)-1))))

#define min(x,y) ({ __typeof__ (x) _x = (x); \
                    __typeof__ (y) _y = (y); \
                    _x < _y ? _x : _y; })

#define max(x,y) ({ __typeof__ (x) _x = (x); \
                    __typeof__ (y) _y = (y); \
                    _x > _y ? _x : _y; })

#define TOSTRING_INNER(s) #s
#define TOSTRING(s) TOSTRING_INNER(s)

char *hex(uint64_t x);
void $printBool(bool x);
void $printInt(int64_t x);
void $printUInt(uint64_t x);
void $printFloat(float x);
void $printDouble(double x);
void $printChar(char x);
void $printChars(char *x);
void $printPointer(void *x);

#define $print_inner(x) _Generic((x), \
	int8_t:  $printInt, uint8_t:  $printUInt, \
	int16_t: $printInt, uint16_t: $printUInt, \
	int32_t: $printInt, uint32_t: $printUInt, \
	int64_t: $printInt, uint64_t: $printUInt, \
	float: $printFloat, double: $printDouble, \
	char:   $printChar, char*:   $printChars, \
	bool:   $printBool, \
	default: $printPointer)(x)

#define print(...) $print_args(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
#define $print_args(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, number, ...) {\
	if(number > 0) { $print_inner(a0);              } \
	if(number > 1) { printf(","); $print_inner(a1); } \
	if(number > 2) { printf(","); $print_inner(a2); } \
	if(number > 3) { printf(","); $print_inner(a3); } \
	if(number > 4) { printf(","); $print_inner(a4); } \
	if(number > 5) { printf(","); $print_inner(a5); } \
	if(number > 6) { printf(","); $print_inner(a6); } \
	if(number > 7) { printf(","); $print_inner(a7); } \
	if(number > 8) { printf(","); $print_inner(a8); } \
	if(number > 9) { printf(","); $print_inner(a9); } \
	puts(""); }

#endif