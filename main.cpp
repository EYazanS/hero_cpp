#include<Windows.h>
#include<stdint.h>
#include<xinput.h>
#include<dsound.h>

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

#pragma region DefineXInput
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_
#pragma endregion

#pragma region DirectSound

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#pragma endregion

static bool Runnig = true;
static win32_offscreen_buffer GlobalBackBuffer;
static LPDIRECTSOUNDBUFFER secondaryBuffer;

void Win32LoadXInputModule();
win32_window_dimensions Win32GetWindowDimensions(HWND Window);
void RenderGradiant(win32_offscreen_buffer* Buffer, int XOffset, int YOffset);
void Win32InitDirectSound(HWND Window, int32 BufferSize, int32 SamplesPerSecond);
void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width);
LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions);
void PlayTestSound(int SamplesPerSecond, int BytesPerSample, int SecondaryBufferSize);

// Entry point
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	// Initialize Window to 0
	WNDCLASS windowsClass = {};

	// Initialize XInput library
	Win32LoadXInputModule();

	// Stack overflow exceptinos
	// uint8 memory[2 * 1024 * 1024] = {};

	//Initialize first bitmap
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
			HDC deviceContext = GetDC(window);


			// Extract messages from windows
			MSG message;

			// Graphic test
			int xOffset = 0;
			int yOffset = 0;

			// Sound test
			int samplesPerSecond = 48000;
			int bytesPerSample = sizeof(int16) * 2;
			int secondaryBufferSize = samplesPerSecond * bytesPerSample;

			Win32InitDirectSound(window, samplesPerSecond, secondaryBufferSize);

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

				for (int controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
				{
					XINPUT_STATE controllerState;

					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						// Controller is plugged in
						XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

						bool  dPadUp = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
						bool  dPadDown = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
						bool  dPadLeft = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
						bool  dPadRight = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

						bool  start = pad->wButtons & XINPUT_GAMEPAD_START;
						bool  back = pad->wButtons & XINPUT_GAMEPAD_BACK;

						bool  leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
						bool  rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

						bool  aButton = pad->wButtons & XINPUT_GAMEPAD_A;
						bool  bButton = pad->wButtons & XINPUT_GAMEPAD_B;
						bool  yButton = pad->wButtons & XINPUT_GAMEPAD_Y;
						bool  xButton = pad->wButtons & XINPUT_GAMEPAD_X;

						int16  stickX = pad->sThumbLX;
						int16  stickY = pad->sThumbLY;

						xOffset = stickX >> 8;
						yOffset = -stickY >> 8;

						XINPUT_VIBRATION vibrations;

						vibrations.wLeftMotorSpeed = 0;
						vibrations.wRightMotorSpeed = 0;

						if (dPadUp)
						{
							yOffset += 2;
							vibrations.wLeftMotorSpeed = 60000;
						}

						if (dPadDown)
						{
							yOffset -= 2;
							vibrations.wRightMotorSpeed = 60000;
						}

						if (dPadLeft)
						{
							xOffset += 2;
							vibrations.wLeftMotorSpeed = 60000;
						}

						if (dPadRight)
						{
							xOffset -= 2;
							vibrations.wRightMotorSpeed = 60000;
						}

						XInputSetState(0, &vibrations);
					}
					else
					{
						// Controller is not available
					}
				}

				RenderGradiant(&GlobalBackBuffer, xOffset, yOffset);

				PlayTestSound(samplesPerSecond, bytesPerSample, secondaryBufferSize);


				win32_window_dimensions dimensions = Win32GetWindowDimensions(window);
				Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, dimensions);
				// ReleaseDC(window, deviceContext);
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


void Win32LoadXInputModule()
{
	HMODULE xInputModule = LoadLibrary("xinput1_4.dll");

	// Show which version is loaded
	if (!xInputModule)
		xInputModule = LoadLibrary("xinput1_3.dll");

	if (xInputModule)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(xInputModule, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xInputModule, "XInputSetState");
	}
	else
	{
		// TODO: Log x input load error
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
	Buffer->Memory = VirtualAlloc(0, bitMapMemory, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
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

		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			uint32 vkCode = WParam;
			bool wasDown = ((LParam & (1 << 30)) != 0);
			bool isDown = ((LParam & (1 << 31)) == 0);

			if (isDown != wasDown)
				switch (vkCode)
				{
					case 'W':
						OutputDebugString("W");
						break;

					case 'A':
						OutputDebugString("A");
						break;

					case 'S':
						OutputDebugString("S");
						break;

					case 'D':
						OutputDebugString("D");
						break;

					case 'E':
						OutputDebugString("E");
						break;

					case 'Q':
						OutputDebugString("Q");
						break;

					case VK_SPACE:
						OutputDebugString("Space");
						break;

					case VK_UP:
						OutputDebugString("Up");
						break;

					case VK_LEFT:
						OutputDebugString("Left");
						break;

					case VK_RIGHT:
						OutputDebugString("Right");
						break;

					case VK_DOWN:
						OutputDebugString("Down");
						break;

					case VK_ESCAPE:
						OutputDebugString("Esacpe: ");
						if (isDown)
						{
							OutputDebugString("is Down ");
							// Runnig = false;
						}

						if (wasDown)
						{
							OutputDebugString("was Down ");
						}
						OutputDebugString("\n");
						break;

					case VK_F4:
					{
						// Is alt button held down
						if ((LParam & (1 << 29)) != 0)
							Runnig = false;

					} break;

					default:
						break;
				}
		}break;

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
			Win32DisplayBufferInWindow(&GlobalBackBuffer, devicContext, dimensions);
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

void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions)
{
	// TODO: Fix aspect ratio
	StretchDIBits(DevicContext,
		0, 0, WindowDimensions.Width, WindowDimensions.Height, // Distenation
		0, 0, Buffer->Width, Buffer->Height, // Source
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

void Win32InitDirectSound(HWND Window, int32 BufferSize, int32 SamplesPerSecond)
{
	// Load Direct sound library
	HMODULE dSoundModule = LoadLibraryA("dsound.dll");

	if (dSoundModule)
	{
		// Get Direct sound object
		direct_sound_create* directSoundCreate = (direct_sound_create*)GetProcAddress(dSoundModule, "DirectSoundCreate");
		LPDIRECTSOUND directSound;

		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = SamplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(directSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				// Create primary buffer, it gets a hold to sound device and configure it
				// to play sounds in the correct format we supply
				// the second buffer will hold the sound we will play
				DSBUFFERDESC bufferDescription = { };
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER primaryBuffer;

				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					OutputDebugString("Set primary buffer format\n");

					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						// Got format
						OutputDebugString("Set primary buffer format\n");
					}
					else
					{
						// TODO: Log cant set format
					}
				}
				else
				{
					// TODO: Log cant create buffer
				}
			}
			else
			{
				// TODO: Log sound cant set sound level error
			}

			// Create secondary buffer
			DSBUFFERDESC bufferDescription = { };
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = BufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;

			if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0)))
			{
				// Can start playing
				OutputDebugString("Secondary buffer created\n");
			}
			else
			{
				// TODO: Log cant create buffer
			}
		}
		else
		{
			// TODO: Log sound function load error
		}
	}
	else
	{
		// TODO: Log sound load error
	}
}

void PlayTestSound(int SamplesPerSecond, int BytesPerSample, int SecondaryBufferSize)
{
	int toneHz = 256;
	int squareWavePeriod = SamplesPerSecond / toneHz;
	int halfSquareWavePeriod = squareWavePeriod / 2;
	int squareWaveCounter = 0;
	int16 volume = 6000;
	uint32 runningSampleIndex = 1;

	// Test sound
	DWORD playCursor;
	DWORD writeCursor;

	if (SUCCEEDED(secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
	{
		DWORD byteToLock = runningSampleIndex * BytesPerSample % SecondaryBufferSize;
		DWORD bytesToWrite;

		if (byteToLock > playCursor)
		{
			bytesToWrite = SecondaryBufferSize - byteToLock;
			bytesToWrite += playCursor;
		}
		else
		{
			bytesToWrite = playCursor - byteToLock;
		}

		void* regionOne;
		DWORD regionOneSize;
		void* regionTwo;
		DWORD regionTwoSize;

		if (secondaryBuffer->Lock(byteToLock, bytesToWrite, &regionOne, &regionOneSize, &regionTwo, &regionTwoSize, 0))
		{
			int16* sampleOut = (int16*)regionOne;

			for (DWORD sampleIndex = 0; sampleIndex < regionOneSize; ++sampleIndex)
			{
				if (squareWaveCounter)
					squareWaveCounter = squareWavePeriod;

				int16 sampleValue = (runningSampleIndex / halfSquareWavePeriod) % 2 ? volume : -volume;

				*sampleOut++ = sampleValue;
				*sampleOut++ = sampleValue;
				--squareWaveCounter;
				++runningSampleIndex;
			}

			sampleOut = (int16*)regionTwo;

			for (DWORD sampleIndex = 0; sampleIndex < regionTwoSize; ++sampleIndex)
			{
				if (squareWaveCounter)
					squareWaveCounter = squareWavePeriod;

				int16 sampleValue = (runningSampleIndex / halfSquareWavePeriod) % 2 ? volume : -volume;

				*sampleOut++ = sampleValue;
				*sampleOut++ = sampleValue;
				--squareWaveCounter;
				++runningSampleIndex;
			}

			secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			secondaryBuffer->Unlock(regionOne, regionOneSize, regionTwo, regionTwoSize);
		}
	}
}