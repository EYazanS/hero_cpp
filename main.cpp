#include<Windows.h>

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

// TODO: change from global variable
#pragma region globals_variables
static bool Runnig = true;
static BITMAPINFO BitMapInfo;
static void *BitMapMemory;
static  HBITMAP BitMapHandle;
static HDC BitMapDeviceContext;
#pragma endregion

void Win32ResizeDIBSection(int height, int width);
void Win32UpdateWindow(HDC DevicContext, int x, int y, int height, int width);

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
			while (Runnig)
			{
				BOOL messageResult = GetMessage(&message, 0, 0, 0);
				if (messageResult > 0)
				{
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else
				{
					// Exit loop
					Runnig = false;
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

	// int box = MessageBox(0, "Heroes are self made", "Hero", MB_OKCANCEL | MB_ICONINFORMATION);

	return 0;
}

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT result = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
			RECT clientRect;
			GetClientRect(Window, &clientRect);
			int height = clientRect.bottom - clientRect.top;
			int width = clientRect.right - clientRect.left;
			Win32ResizeDIBSection(height, width);
			OutputDebugString("Resize\n");
		} break;

		case WM_CLOSE:
		{
			Runnig = false;
		} break;

		case WM_DESTROY:
		{
			// TODO: Hand as error - try to create window
			Runnig = false;
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
			Win32UpdateWindow(DevicContext, x, y, height, width);

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

void Win32ResizeDIBSection(int height, int width)
{
	if (BitMapHandle)
		DeleteObject(BitMapHandle);
	
	if(!BitMapDeviceContext)
		BitMapDeviceContext = CreateCompatibleDC(0);

	BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = width;
	BitMapInfo.bmiHeader.biHeight = height;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;
	BitMapInfo.bmiHeader.biSizeImage = 0;
	BitMapInfo.bmiHeader.biXPelsPerMeter = 0;
	BitMapInfo.bmiHeader.biYPelsPerMeter = 0;
	BitMapInfo.bmiHeader.biClrUsed = 0;
	BitMapInfo.bmiHeader.biClrImportant = 0;

	HBITMAP section = CreateDIBSection(BitMapDeviceContext, &BitMapInfo, DIB_RGB_COLORS, &BitMapMemory, 0, 0);
}

void Win32UpdateWindow(HDC DevicContext, int x, int y, int height, int width)
{
	int result = StretchDIBits(DevicContext, x, y, width, height, x, y, width, height, BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}
