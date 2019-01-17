#include<Windows.h>
#include<stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// TODO: change from global variable
#pragma region globals_variables
static bool Runnig = true;
static BITMAPINFO BitMapInfo;
static void *BitMapMemory;
static int BitMapHeight = 0;
static int BitMapWidth = 0;
const int BytesPerPixed = 4;

#pragma endregion

void Win32ResizeDIBSection(int Height, int Width);
void Win32UpdateWindow(HDC DevicContext, RECT* WindowRect);
void RenderGradiant(int XOffset, int YOffset);
LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

// Entry point
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	WNDCLASS windowsClass = {};

	//CS_HREDRAW|CS_VREDRAW redraw the window when it gets dragged h => horizontal | v -> vertical
	windowsClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowsClass.hInstance = Instance;
	windowsClass.lpszClassName = "Class Hero game";
	windowsClass.lpfnWndProc = MainWindowCallBack;

	//WindowsClass.lpfnWndProc = MainWindowCallBack;
	// WindowsClass.hIcon = ;
	// WindowsClass.hCursor = 0;

	if (RegisterClass(&windowsClass))
	{
		HWND window = CreateWindowEx(
			0,
			windowsClass.lpszClassName,
			"Hero window",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0, 0, Instance, 0);

		if (window)
		{
			// Extract messages from windows
			MSG message;
			int xOffset = 0;
			// Change for future
			while (Runnig)
			{
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
						Runnig = false;

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				RenderGradiant(xOffset++, 0);
				HDC deviceContext = GetDC(window);
				RECT clientRect;
				GetClientRect(window, &clientRect);
				Win32UpdateWindow(deviceContext, &clientRect);
				ReleaseDC(window, deviceContext);
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
			HDC devicContext = BeginPaint(Window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			RECT clientRect;
			GetClientRect(Window, &clientRect);
			Win32UpdateWindow(devicContext, &clientRect);
			//PatBlt(devicContext, x, y, width, height, WHITENESS);
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

void Win32ResizeDIBSection(int Height, int Width)
{
	if (BitMapMemory)
		VirtualFree(BitMapMemory, 0, MEM_RELEASE);

	BitMapWidth = Width;
	BitMapHeight = Height;

	BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = BitMapWidth;
	BitMapInfo.bmiHeader.biHeight = -BitMapHeight;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;

	// 8 bits as padding because we have 8 bits green, 8 bits red and 8 bits blue equal 3 bytes
	// so we need 8 more bits to keep performance the 8 bits wont be used at all
	int bitMapMemory = (BitMapWidth * BitMapHeight) * BytesPerPixed;
	BitMapMemory = VirtualAlloc(0, bitMapMemory, MEM_COMMIT, PAGE_READWRITE);
}

void Win32UpdateWindow(HDC DevicContext, RECT* WindowRect)
{
	int windowWidth = WindowRect->right - WindowRect->left;
	int windowHeight = WindowRect->bottom - WindowRect->top;
	int result = StretchDIBits(DevicContext, 0, 0, BitMapWidth, BitMapHeight, 0, 0, windowWidth, windowHeight, BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}

void RenderGradiant(int XOffset, int YOffset)
{
	int pitch = BitMapWidth * BytesPerPixed;
	uint8* row = (uint8*)BitMapMemory;

	for (int y = 0; y < BitMapHeight; ++y)
	{
		uint32* pixel = (uint32*)row;

		for (int x = 0; x < BitMapWidth; ++x)
		{
			uint8 blue = (uint8)x + XOffset;
			uint8 green = (uint8)y + YOffset;
			uint8 red = 100;
			uint8 padding = 0;

			// Blue
			*pixel++ = (red << 16)| (green << 8) | blue;
		}

		row += pitch;
	}
}