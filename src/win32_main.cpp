#include <cstdint>
#include <windows.h>

// just a nice way of differentiating the uses of "static" keyword,
// which can mean different thing in different contexts
#define internal      static // a function that is internal to a file, not usable outside a source file (private-like)
#define local_persist static // a variable that is persistent inside a scope (keeps its value when program go out of the scope and return to it later)
#define global_var    static // a global variable... thats just it

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_bmp_buffer {
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
	int pixel_size_in_bytes;
};

struct win32_window_dimension {
	int width; int height;
};

global_var bool running;
global_var win32_bmp_buffer backbuf;

win32_window_dimension win32_get_window_dimension(HWND window) {
	RECT client_rect;
	GetClientRect(window, &client_rect);

	return {
		.width = client_rect.right - client_rect.left,
		.height = client_rect.bottom - client_rect.top,
	};
}

internal void render_cool_gradient(win32_bmp_buffer buf, int x_offset, int y_offset) {
	uint8* row = (uint8*)buf.memory;
	for (int y = 0; y < buf.height; y++) {
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buf.width; x++) {
			uint8 b = x + x_offset;
			uint8 g = y + y_offset;

			// 1. shift "g" left by 8 bits: 00 00 gg > 00 gg 00 
			// 2. bitwise or "g" and "b": 00 gg 00 | 00 00 bb = 00 gg bb
			// 3. post increment pixel pointer to next pixel
			*pixel++ = (g << 8) | b;
		}

		row += buf.pitch;
	}
}

/*
	DIB = device independant bitmap
	> "Things that you can write into as bitmaps that it (Windows) can then display using GDI" - Muratori, C
	GDI = windows graphics rendering thing
*/
internal void win32_resize_dbi_section(win32_bmp_buffer* buf, int w, int h) {
	if (buf->memory) {
		VirtualFree(buf->memory, 0, MEM_RELEASE);
	}

	buf->width = w;
	buf->height = h;
	buf->pixel_size_in_bytes = 4; // INFO: 4 bytes in order to align with biBitCount of 32 below
	buf->info = {
		.bmiHeader = {
			.biSize = sizeof(buf->info.bmiHeader),
			.biWidth = buf->width,
			.biHeight = -buf->height,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
		},
	};

	// 8b R + 8b G + 8b B + 8b ALIGNMENT PADDING = 32bits (4 bytes)
	int bitmap_memory_size = buf->width * buf->height * buf->pixel_size_in_bytes;
	buf->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
	buf->pitch = buf->width * buf->pixel_size_in_bytes;
}

internal void win32_display_buffer(HDC device_ctx, int width, int height, win32_bmp_buffer buf) {
	// TODO: aspect my ratio
	StretchDIBits(
		device_ctx,
		0, 0, width, height, // source of the render - back buffer
		0, 0, buf.width, buf.height, // section of the window to fill - front buffer
		buf.memory,
		&buf.info,
		DIB_RGB_COLORS,
		SRCCOPY
	);
}

LRESULT CALLBACK win32_main_window_callback(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
	LRESULT result = 0;

	switch (message) {

		case WM_SIZE: { // INFO: Window resize
			OutputDebugStringA("WM_SIZE\n");
		} break;

		case WM_DESTROY: { // INFO: window gets destroyed
			               // TODO: potential error - recreate window?
			OutputDebugStringA("WM_DESTROY\n");
			running = false;
		} break;
	
		case WM_CLOSE: { // INFO: window wants to close
			             // TODO: message user?
			OutputDebugStringA("WM_CLOSE\n");
			running = false;
		} break;
	
		case WM_ACTIVATEAPP: { // INFO: window get focused/active
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
	
		case WM_PAINT: { // INFO: painting on the window
			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(window, &paint);

			win32_window_dimension dim = win32_get_window_dimension(window);
			win32_display_buffer(hdc, dim.width, dim.height, backbuf);

			EndPaint(window, &paint);
		} break;

		default: { // INFO: unhandled stuff
			// OutputDebugStringA("default\n");
			result = DefWindowProc(window, message, w_param, l_param);
		} break;
	}

	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
	WNDCLASS window_class = {
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = win32_main_window_callback,
		.hInstance = instance,
		// .hIcon = 
		.lpszClassName = "handmade_hero_window_class",
	};

	if (RegisterClass(&window_class)) {
		HWND window = CreateWindowEx(
			0,
			window_class.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0
		);

		win32_resize_dbi_section(&backbuf, 1280, 720);

		if (window) {
			running = 1;

			int x_offset = 0;
			int y_offset = 0;

			while (running) {
				MSG message;

				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
					if (message.message == WM_QUIT) {
						running = false;
					}

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				render_cool_gradient(backbuf, x_offset, y_offset);

				HDC hdc = GetDC(window);
				win32_window_dimension dim = win32_get_window_dimension(window);
				win32_display_buffer(hdc, dim.width, dim.height, backbuf);
				ReleaseDC(window, hdc);

				x_offset++;
				y_offset++;
			}
		}
	}

	return 0;
}
