#include <windows.h>

// just a nice way of differentiating the uses of "static" keyword,
// which can mean different thing in different contexts
#define internal      static // a function that is internal to a file, not usable outside a source file (private-like)
#define local_persist static // a variable that is persistent inside a scope (keeps its value when program go out of the scope and return to it later)
#define global_var    static // a global variable... thats just it

// TODO: global for now, struct in the future, when? dunno
global_var bool running;

global_var BITMAPINFO bitmap_info;
global_var void* bitmap_memory;
global_var HBITMAP bitmap_handle;
global_var HDC bitmap_device_ctx;

/*
	DIB = device independant bitmap
	> "Things that you can write into as bitmaps that it (Windows) can then display using GDI" - Muratori, C
	GDI = windows graphics rendering thing
*/
internal void win32_resize_dbi_section(int w, int h) {

	if (bitmap_handle) {
		DeleteObject(bitmap_handle);
	}

	if (!bitmap_device_ctx) {
		bitmap_device_ctx = CreateCompatibleDC(0);
	}

	bitmap_info = {
		.bmiHeader = {
			.biSize = sizeof(bitmap_info.bmiHeader),
			.biWidth = w,
			.biHeight = h,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
		},
	};


	bitmap_handle = CreateDIBSection(
		bitmap_device_ctx,
		&bitmap_info,
		DIB_RGB_COLORS,
		&bitmap_memory,
		0, 0
	);
}

internal void win32_update_window(HDC device_ctx, int x, int y, int w, int h) {
	StretchDIBits(
		device_ctx,
		x, y, w, h, // section of the window to fill - front buffer
		x, y, w, h, // source of the render - back buffer
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

			win32_update_window(hdc, x, y, w, h);

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
		HWND window_handle = CreateWindowEx(
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

		if (window_handle) {
			MSG message;
			running = 1;

			while (running) {
				BOOL msg_result = GetMessage(&message, 0, 0, 0);

				if (msg_result <= 0) break;

				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
	}

	return 0;
}
