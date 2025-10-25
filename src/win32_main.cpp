#include <windows.h>
#include <cstdint>
#include <winuser.h>
#include <xinput.h>

// just a nice way of differentiating the uses of "static" keyword,
// which can mean different thing in different contexts
#define internal      static // a function that is internal to a file, not usable outside a source file (private-like)
#define local_persist static // a variable that is persistent inside a scope (keeps its value when program go out of the scope and return to it later)
#define global_var    static // a global variable... thats just it

#define GET_BIT(var, position) (var & (1 << position))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t bool32;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

/*
 * INFO: Since XInput API has some weird system requirements on different OSes,
 *       we need a way to retrieve the necessary functions of the api that 
 *       doesn't break our code if we link to a lib that does not exists.
 *       First, we define stubs that are placeholder functions just to not have
 *       empty pointers if we fail to provide the proper version of those functions
 *       provided by the OS.
 */
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
	return 0;
}
global_var x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
	return 0;
}
global_var x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void win32_load_xinput(void) {
	HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
	if (!xinput_library) {
		xinput_library = LoadLibraryA("xinput1_3.dll");
	}

	if (xinput_library) {
		XInputGetState = (x_input_get_state*)GetProcAddress(xinput_library, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xinput_library, "XInputSetState");
	}
}

/*
 * End of XInput thingies
 */

global_var bool running;
global_var win32_bmp_buffer backbuf;

internal win32_window_dimension win32_get_window_dimension(HWND window) {
	RECT client_rect;
	GetClientRect(window, &client_rect);

	return {
		.width = client_rect.right - client_rect.left,
		.height = client_rect.bottom - client_rect.top,
	};
}

internal void render_cool_gradient(win32_bmp_buffer* buf, int x_offset, int y_offset) {
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

internal void win32_display_buffer(HDC device_ctx, int width, int height, win32_bmp_buffer* buf) {
	// TODO: aspect my ratio
	StretchDIBits(
		device_ctx,
		0, 0, width, height, // source of the render - back buffer
		0, 0, buf->width, buf->height, // section of the window to fill - front buffer
		buf->memory,
		&buf->info,
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
			win32_display_buffer(hdc, dim.width, dim.height, &backbuf);

			EndPaint(window, &paint);
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			u32 vkcode = w_param;
			bool32 was_pressed = (l_param & (1 << 30)) != 0; // 30th bit indicates whether the key was pressed last call
			bool32 is_pressed = (l_param & (1 << 31)) != 0; // 31th bit indicates whether the key is pressed right now
			bool32 is_alt_pressed = GET_BIT(l_param, 29);

			if (
				(is_alt_pressed && vkcode == VK_F4) || // handle alt + f4 manually
				(vkcode == VK_ESCAPE)
			) {
				running = false;
			}
		} break;

		default: { // INFO: unhandled stuff
			// OutputDebugStringA("default\n");
			result = DefWindowProc(window, message, w_param, l_param);
		} break;
	}

	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
	win32_load_xinput();

	WNDCLASSA window_class = {
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

				for (DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; controller_index++) {
					XINPUT_STATE controller_state;

					if (XInputGetState(controller_index, &controller_state) == ERROR_DEVICE_NOT_CONNECTED) {
						continue;
					}

					XINPUT_GAMEPAD* pad = &controller_state.Gamepad;

					bool pad_up             = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
					bool pad_down           = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
					bool pad_left           = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
					bool pad_right          = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
					bool pad_start          = pad->wButtons & XINPUT_GAMEPAD_START;
					bool pad_back           = pad->wButtons & XINPUT_GAMEPAD_BACK;
					bool pad_left_shoulder  = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
					bool pad_right_shoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
					bool pad_a              = pad->wButtons & XINPUT_GAMEPAD_A;
					bool pad_b              = pad->wButtons & XINPUT_GAMEPAD_B;
					bool pad_x              = pad->wButtons & XINPUT_GAMEPAD_X;
					bool pad_y              = pad->wButtons & XINPUT_GAMEPAD_Y;

					bool pad_lthumb_x       = pad->sThumbLX;
					bool pad_lthumb_y       = pad->sThumbLY;
					bool pad_rthumb_x       = pad->sThumbRX;
					bool pad_rthumb_y       = pad->sThumbRY;

					XINPUT_VIBRATION vibration;
					vibration.wLeftMotorSpeed = 0;
					vibration.wRightMotorSpeed = 0;

					if (pad_left) {
						x_offset--;
						vibration.wLeftMotorSpeed = 60000;
					}
					else if (pad_right) {
						x_offset++;
						vibration.wRightMotorSpeed = 60000;
					}

					XInputSetState(1, &vibration);

					if (pad_up) y_offset--;
					else if (pad_down) y_offset++;
				}

				render_cool_gradient(&backbuf, x_offset, y_offset);

				HDC hdc = GetDC(window);
				win32_window_dimension dim = win32_get_window_dimension(window);
				win32_display_buffer(hdc, dim.width, dim.height, &backbuf);
				ReleaseDC(window, hdc);

				// x_offset++;
				// y_offset++;
			}
		}
	}

	return 0;
}
