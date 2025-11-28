#if !defined(MAIN_H)

#include <stdint.h>

// just a nice way of differentiating the uses of "static" keyword,
// which can mean different thing in different contexts
#define internal      static // a function that is internal to a file, not usable outside a source file (private-like)
#define local_persist static // a variable that is persistent inside a scope (keeps its value when program go out of the scope and return to it later)
#define global_var    static // a global variable... thats just it

#define GET_BIT(var, position) (var & (1 << position))

#define PI_32 3.14159265359f

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t bool32;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

struct offscreen_buffer {
	void* memory;
	int width;
	int height;
	int pitch;
};

void game_update_and_render(offscreen_buffer* buf);
void render_cool_gradient(offscreen_buffer* buf, int x_offset, int y_offset);

#define MAIN_H
#endif
