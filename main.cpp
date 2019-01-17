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

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Height;
	int Width;
	int BytesPerPixel;
	int Pitch;
};

struct win32_window_dimensions
{
	int Width;
	int Height;
};

static bool Runnig = true;
static win32_offscreen_buffer GlobalBackBuffer;

void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width);
void Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions);
void RenderGradiant(win32_offscreen_buffer* Buffer, int XOffset, int YOffset);
win32_window_dimensions Win32GetWindowDimensions(HWND Window);
LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

// Entry point
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	// Initialize Window to 0
	WNDCLASS windowsClass = {};

	// Stack overflow exceptinos
	// uint8 memory[2 * 1024 * 1024] = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	// CS_HREDRAW|CS_VREDRAW redraw the entire window when it gets dragged, h => horizontal | v -> vertical
	windowsClass.style = CS_HREDRAW | CS_VREDRAW;
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

				RenderGradiant(&GlobalBackBuffer, xOffset++, 0);
				HDC deviceContext = GetDC(window);
				win32_window_dimensions dimensions = Win32GetWindowDimensions(window);
				Win32DisplayBufferInWindow(GlobalBackBuffer, deviceContext, dimensions);
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

		} break;

		case WM_CLOSE:
		{
			Runnig = false;
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle it as an error - try to re-create window
			Runnig = false;
		} break;

		case WM_ACTIVATEAPP:
		{
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC devicContext = BeginPaint(Window, &paint);
			win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);
			Win32DisplayBufferInWindow(GlobalBackBuffer, devicContext, dimensions);
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

void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width)
{
	if (Buffer->Memory)
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	// 8 bits as padding because we have 8 bits green, 8 bits red and 8 bits blue equal 3 bytes
	// so we need 8 more bits to keep performance the 8 bits wont be used at all
	int bitMapMemory = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, bitMapMemory, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

void Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions)
{
	// TODO: Fix aspect ratio
	StretchDIBits(DevicContext, 
		0, 0, WindowDimensions.Width, WindowDimensions.Height, // Distenation
		0, 0, Buffer.Width, Buffer.Height, // Source
		Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

void RenderGradiant(win32_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
	uint8* row = (uint8*)Buffer->Memory;

	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32* pixel = (uint32*)row;

		for (int x = 0; x < Buffer->Width; ++x)
		{
			uint8 blue = (uint8)x + XOffset;
			uint8 green = (uint8)y + YOffset;
			uint8 red = 100;
			uint8 alpha = 255;

			// Blue
			*pixel++ = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}

		row += Buffer->Pitch;
	}
}

win32_window_dimensions Win32GetWindowDimensions(HWND Window) {
	win32_window_dimensions dimensions;
	RECT clientRect;
	GetClientRect(Window, &clientRect);
	dimensions.Height = clientRect.bottom - clientRect.top;
	dimensions.Width = clientRect.right - clientRect.left;
	return dimensions;
}