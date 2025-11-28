#include "main.h"

void render_cool_gradient(offscreen_buffer* buf, int x_offset, int y_offset) {
	u8* row = (u8*)buf->memory;
	for (int y = 0; y < buf->height; y++) {
		u32* pixel = (u32*)row;
		for (int x = 0; x < buf->width; x++) {
			u8 b = x + x_offset;
			u8 g = y + y_offset;

			// 1. shift "g" left by 8 bits: 00 00 gg > 00 gg 00 
			// 2. bitwise or "g" and "b": 00 gg 00 | 00 00 bb = 00 gg bb
			// 3. post increment pixel pointer to next pixel
			*pixel++ = (g << 8) | b;
		}

		row += buf->pitch;
	}
}

void game_update_and_render(offscreen_buffer* buf) {
	render_cool_gradient(buf, 0, 0);
}
