#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

LRESULT CALLBACK main_window_callback(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
	LRESULT result = 0;

	switch (message) {

		case WM_SIZE: {
			OutputDebugStringA("WM_SIZE\n");
		} break;

		case WM_DESTROY: {
			OutputDebugStringA("WM_DESTROY\n");
		} break;
	
		case WM_CLOSE: {
			OutputDebugStringA("WM_CLOSE\n");
			PostQuitMessage(1);
		} break;
	
		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
	
		case WM_PAINT: {
			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(window, &paint);

			int w = paint.rcPaint.right - paint.rcPaint.left;
			int h = paint.rcPaint.bottom - paint.rcPaint.top;
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;

			PatBlt(hdc, x, y, w, h, WHITENESS);

			SetPixel(hdc, 100, 100, RGB(255, 0, 0));
			SetPixel(hdc, 101, 100, RGB(255, 0, 0));
			SetPixel(hdc, 102, 100, RGB(255, 0, 0));
			SetPixel(hdc, 103, 100, RGB(255, 0, 0));
			SetPixel(hdc, 104, 100, RGB(255, 0, 0));
			SetPixel(hdc, 105, 100, RGB(255, 0, 0));
			SetPixel(hdc, 106, 100, RGB(255, 0, 0));
			SetPixel(hdc, 107, 100, RGB(255, 0, 0));
			SetPixel(hdc, 108, 100, RGB(255, 0, 0));

			EndPaint(window, &paint);
		} break;

		default: {
			// OutputDebugStringA("default\n");
			result = DefWindowProc(window, message, w_param, l_param);
		} break;
	}

	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
	WNDCLASS window_class = {
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = main_window_callback,
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

			for (;;) {
				BOOL msg_result = GetMessage(&message, 0, 0, 0);

				if (msg_result <= 0) break;

				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
	}

	return 0;
}
