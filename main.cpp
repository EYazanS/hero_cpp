#include<Windows.h>

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	WNDCLASS WindowsClass = {};

	//CS_HREDRAW|CS_VREDRAW redraw the window when it gets dragged h => horizontal | v -> vertical
	WindowsClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowsClass.hInstance = Instance;
	WindowsClass.lpszClassName = "Hero game";
	WindowsClass.lpfnWndProc = MainWindowCallBack;
	
	//WindowsClass.lpfnWndProc = MainWindowCallBack;
	// WindowsClass.hIcon = ;
	// WindowsClass.hCursor = 0;

	if (RegisterClass(&WindowsClass))
	{
		HWND WindowHandle = CreateWindowEx(
			0,
			WindowsClass.lpszClassName,
			"Hero window",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance, 0);

		if (WindowHandle)
		{
			// Extract messages from windows
			MSG Message;

			// Change for future
			for (; ;)
			{
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				
				if (MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
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

		default:
		{
			// return windows default behaviour
			result = DefWindowProc(Window, Message, WParam, LParam);
			// OutputDebugString("Everything else");
		} break;

	}

	return (result);
}