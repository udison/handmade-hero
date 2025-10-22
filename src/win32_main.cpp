#include <cstdint>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

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

// TODO: global for now, struct in the future, when? dunno
global_var bool running;
global_var BITMAPINFO bitmap_info;
global_var void* bitmap_memory;
global_var int bitmap_width;
global_var int bitmap_height;
global_var const int PIXEL_SIZE_IN_BYTES = 4;

internal void render_cool_gradient(int x_offset, int y_offset) {
	int pitch = bitmap_width * PIXEL_SIZE_IN_BYTES;
	uint8* row = (uint8*)bitmap_memory;
	for (int y = 0; y < bitmap_height; y++) {
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < bitmap_width; x++) {
			uint8 b = x + x_offset;
			uint8 g = y + y_offset;

			*pixel++ = (g << 8) | b;
		}

		row += pitch;
	}
}

/*
	DIB = device independant bitmap
	> "Things that you can write into as bitmaps that it (Windows) can then display using GDI" - Muratori, C
	GDI = windows graphics rendering thing
*/
internal void win32_resize_dbi_section(int w, int h) {

	if (bitmap_memory) {
		VirtualFree(bitmap_memory, 0, MEM_RELEASE);
	}

	bitmap_width = w;
	bitmap_height = h;

	bitmap_info = {
		.bmiHeader = {
			.biSize = sizeof(bitmap_info.bmiHeader),
			.biWidth = bitmap_width,
			.biHeight = -bitmap_height,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
		},
	};

	// INFO: 4 bytes in order to align with biBitCount of 32 above
	// 8b R + 8b G + 8b B + 8b ALIGNMENT PADDING = 32bits (4 bytes)
	int bitmap_memory_size = bitmap_width * bitmap_height * PIXEL_SIZE_IN_BYTES;
	bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

internal void win32_update_window(HDC device_ctx, RECT *window_rect, int x, int y, int w, int h) {
	int window_width = window_rect->right - window_rect->left;
	int window_height = window_rect->bottom - window_rect->top;

	StretchDIBits(
		device_ctx,
		// x, y, w, h, // section of the window to fill - front buffer
		// x, y, w, h, // source of the render - back buffer
		0, 0, bitmap_width, bitmap_height,
		0, 0, window_width, window_height,
		bitmap_memory,
		&bitmap_info,
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

			RECT client_rect;
			GetClientRect(window, &client_rect);
			int w = client_rect.right - client_rect.left;
			int h = client_rect.bottom - client_rect.top;
			win32_resize_dbi_section(w, h);
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

			int w = paint.rcPaint.right - paint.rcPaint.left;
			int h = paint.rcPaint.bottom - paint.rcPaint.top;
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;

			RECT client_rect;
			GetClientRect(window, &client_rect);
			win32_update_window(hdc, &client_rect, x, y, w, h);

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
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
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

		if (window) {
			MSG message;
			running = 1;

			int x_offset = 0;
			int y_offset = 0;

			while (running) {
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
					if (message.message == WM_QUIT) {
						running = false;
					}

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				render_cool_gradient(x_offset, y_offset);

				HDC hdc = GetDC(window);
				RECT client_rect;
				GetClientRect(window, &client_rect);
				int w = client_rect.right - client_rect.left;
				int h = client_rect.bottom - client_rect.top;
				win32_update_window(hdc, &client_rect, 0, 0, w, h);
				ReleaseDC(window, hdc);

				x_offset++;
				y_offset++;
			}
		}
	}

	return 0;
}
