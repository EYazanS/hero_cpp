#include<Windows.h>

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

// Entry point
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	WNDCLASS windowsClass = {};

	//CS_HREDRAW|CS_VREDRAW redraw the window when it gets dragged h => horizontal | v -> vertical
	windowsClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowsClass.hInstance = Instance;
	windowsClass.lpszClassName = "Hero game";
	windowsClass.lpfnWndProc = MainWindowCallBack;

	//WindowsClass.lpfnWndProc = MainWindowCallBack;
	// WindowsClass.hIcon = ;
	// WindowsClass.hCursor = 0;

	if (RegisterClass(&windowsClass))
	{
		HWND windowHandle = CreateWindowEx(
			0,
			windowsClass.lpszClassName,
			"Hero window",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance, 0);

		if (windowHandle)
		{
			// Extract messages from windows
			MSG message;

			// Change for future
			for (; ;)
			{
				BOOL messageResult = GetMessage(&message, 0, 0, 0);

				if (messageResult > 0)
				{
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			// Create window fails
			// TODO: Logging
		}
	}
	else
	{
		// register fails
		// TODO: Logging
	}

	auto box = MessageBox(0, "Heroes are self made", "Hero", MB_OKCANCEL | MB_ICONINFORMATION);

	return 0;
}

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
	LRESULT result = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
			OutputDebugString("Resize\n");
		} break;

		case WM_DESTROY:
		{
			OutputDebugString("Desatroyed\n");
		} break;

		case WM_CLOSE:
		{
			OutputDebugString("Closes\n");
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugString("activated\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC DevicContext = BeginPaint(Window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			PatBlt(DevicContext, x, y, width, height, WHITENESS);
			EndPaint(Window, &paint);
		}break;

		default:
		{
			// return window default behaviour
			result = DefWindowProc(Window, Message, WParam, LParam);

		} break;

	}

	return (result);
}