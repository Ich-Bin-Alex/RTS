#include "helper.h"

char *hex(uint64_t x) {
	static char buffer[0x17];
	sprintf(buffer, "%lx", x);
	return buffer;
}
void $printBool(bool x) { printf("%s", x == true ? "true" : "false"); }
void $printInt(int64_t x) { printf("%ld", x); }
void $printUInt(uint64_t x) { printf("%lu", x); }
void $printFloat(float x) { printf("%g", x); }
void $printDouble(double x) { printf("%lg", x); }
void $printChar(char x) { printf("%c", x); }
void $printChars(char *x) { printf("%s", x); }
void $printPointer(void *x) { printf("%p", x); }