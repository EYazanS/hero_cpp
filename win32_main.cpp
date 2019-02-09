#include "hero.h"

#include <Windows.h>
#include <stdint.h>
#include <xinput.h>
#include <xaudio2.h>
#include <math.h>
#include <stdio.h>

/*
	TOOD:
		- Save game location
		- Getting handle of the exe
		- Threading
		- Asset loading path
		- Raw input
		- Sleep/timebeginperiod
		- ClipCursor() - for multi screen support
		- full screen support
		- control cursor visibilaty (WM_SETCURSOR)
		- Query canel auto play
		- WM_ACTIVEAPP (for when we are not the active application)
		- Blit speed improvement (BitBlt)
		- Hardware acceleration (Opengl, directx)
		- GetKeyboard layout (international WASD support)
*/

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
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
static x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_
#pragma endregion

static bool Runnig = true;
static win32_offscreen_buffer GlobalBackBuffer;
static IXAudio2* xAudio;
static IXAudio2SourceVoice* GlobalSourceVoice;
static XAUDIO2_BUFFER GlobalSoundBuffer = { 0 };

const int channels = 2;
const int SampleBits = 32;

void Win32LoadXInputModule();
void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions);
void Win32ProcessDigitalButtons(WORD XInputButtonState, DWORD ButtonBit, game_button_state* OldState, game_button_state* NewState);
void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width);


win32_window_dimensions Win32GetWindowDimensions(HWND Window);

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

HRESULT InitializeXAudio(int SampleRate);
HRESULT PlayGameSound(game_sound_buffer* SoundBuffer);

// Entry point
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCode)
{
	LARGE_INTEGER performanceCounterFrequencyResult;
	QueryPerformanceFrequency(&performanceCounterFrequencyResult);
	uint64 performanceCounterFrequency = performanceCounterFrequencyResult.QuadPart;
	uint64 lastCycleCount = __rdtsc();

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

	game_sound_buffer soundBuffer = {};

	game_memory gameMemory = {};
	gameMemory.PermenantStorageSpace = Megabytes(64);
	gameMemory.PermenantStorage = VirtualAlloc(0, gameMemory.PermenantStorageSpace, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	gameMemory.TransiateStorageSpace = Gigabytes((uint64)4);
	gameMemory.TransiateStorage = VirtualAlloc(0, gameMemory.TransiateStorageSpace, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	soundBuffer.Frequency = 60;
	soundBuffer.SampleRate = 48000;
	soundBuffer.VoiceBufferSampleCount = soundBuffer.SampleRate * 2;
	soundBuffer.WavePeriod = 1.f;
	soundBuffer.Time = 1.f;
	//WindowsClass.lpfnWndProc = MainWindowCallBack;
	// WindowsClass.hIcon = ;
	// WindowsClass.hCursor = 0;
	if (gameMemory.PermenantStorage)
	{
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

				// Sound test
				InitializeXAudio(soundBuffer.SampleRate);

				LARGE_INTEGER lastCountrer;
				QueryPerformanceCounter(&lastCountrer);
				// Win32InitXAudioSound(sampleBits, Channels, samplesPerSecond);

				game_input gameInput[2] = {};
				game_input* newInput = &gameInput[0];
				game_input* oldInput = &gameInput[1];

				int16 maxControlllersCount = (int16)XUSER_MAX_COUNT;

				if (maxControlllersCount > ArrayCount(newInput->Controllers))
					maxControlllersCount = ArrayCount(newInput->Controllers);

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

					for (int controllerIndex = 0; controllerIndex < maxControlllersCount; controllerIndex++)
					{
						game_controller_input* oldControllerState = &oldInput->Controllers[controllerIndex];
						game_controller_input* newControllerState = &newInput->Controllers[controllerIndex];

						XINPUT_STATE controllerState;

						if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
						{
							// Controller is plugged in
							XINPUT_GAMEPAD* pad = &controllerState.Gamepad;


							//TODO: end max macros
							newControllerState->MaxX = oldControllerState->MinX;
							newControllerState->MaxY = oldControllerState->MinY;

							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->DownButton, &newControllerState->DownButton);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_UP, &oldControllerState->UpButton, &newControllerState->UpButton);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, &oldControllerState->RightButton, &newControllerState->RightButton);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT, &oldControllerState->LeftButton, &newControllerState->LeftButton);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, &oldControllerState->RightShoulder, &newControllerState->RightShoulder);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, &oldControllerState->LeftShoulder, &newControllerState->LeftShoulder);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_START, &oldControllerState->Start, &newControllerState->Start);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_BACK, &oldControllerState->Back, &newControllerState->Back);

							real32 x = 0.f;

							if (pad->sThumbLX < 0)
								x = (real32)pad->sThumbLX / 32768;
							else
								x = (real32)pad->sThumbLX / 32767;

							real32 y = 0.f;

							if (pad->sThumbLY < 0)
								y = (real32)pad->sThumbLY / 32768;
							else
								y = (real32)pad->sThumbLY / 32767;

							newControllerState->IsAnalog = true;

							newControllerState->MinX = newControllerState->StartX = oldControllerState->EndX = x;
							newControllerState->MinY = newControllerState->StartY = oldControllerState->EndY = y;

							bool  aButton = pad->wButtons & XINPUT_GAMEPAD_A;
							bool  bButton = pad->wButtons & XINPUT_GAMEPAD_B;
							bool  yButton = pad->wButtons & XINPUT_GAMEPAD_Y;
							bool  xButton = pad->wButtons & XINPUT_GAMEPAD_X;

							int16  stickX = pad->sThumbLX;
							int16  stickY = pad->sThumbLY;

							/*	XINPUT_VIBRATION vibrations;

								vibrations.wLeftMotorSpeed = 0;
								vibrations.wRightMotorSpeed = 0;

								if (dPadUp)
								{
									vibrations.wLeftMotorSpeed = 60000;
								}

								if (dPadDown)
								{
									vibrations.wRightMotorSpeed = 60000;
								}

								if (dPadLeft)
								{
									vibrations.wLeftMotorSpeed = 60000;
								}

								if (dPadRight)
								{
									vibrations.wRightMotorSpeed = 60000;
								}

								XInputSetState(0, &vibrations);*/
						}
						else
						{
							// Controller is not available
						}
					}

					game_offscreen_buffer gameBuffer = {};

					gameBuffer.Height = GlobalBackBuffer.Height;
					gameBuffer.Width = GlobalBackBuffer.Width;
					gameBuffer.Memory = GlobalBackBuffer.Memory;
					gameBuffer.Pitch = GlobalBackBuffer.Pitch;

					GameUpdateAndRender(&gameMemory, &gameBuffer, &soundBuffer, newInput);

					// PlayGameSound(&soundBuffer);

					win32_window_dimensions dimensions = Win32GetWindowDimensions(window);
					Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, dimensions);

					LARGE_INTEGER endCountrer;
					QueryPerformanceCounter(&endCountrer);

					endCountrer.QuadPart;
					// ReleaseDC(window, deviceContext);
					uint64 endCycleCount = __rdtsc();

					uint64 cycleElapsed = endCycleCount - lastCycleCount;
					uint64 counterElapsed = endCountrer.QuadPart - lastCountrer.QuadPart;
					real64 milliseconds = (1000.f * (real32)counterElapsed) / (real32)performanceCounterFrequency;
					real64 fps = (real32)performanceCounterFrequency / (real32)counterElapsed;
					real64 mcpf = (real64)(cycleElapsed / (1000.f * 1000.f));

#if 0	
					char buffer[250];
					sprintf_s(buffer, "%.02fms, %.02fFPS, %.02f mc/f \n", milliseconds, fps, mcpf);
					OutputDebugString(buffer);
#endif // 0

					lastCountrer = endCountrer;
					lastCycleCount = cycleElapsed;

					//TODO: make a swap function
					game_input * temp = newInput;
					newInput = oldInput;
					oldInput = temp;
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
	}
	else
	{
		// memory allocation fails
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

void Win32ResizeDIBSection(win32_offscreen_buffer * Buffer, int Height, int Width)
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
			uint64 vkCode = WParam;
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
						if ((LParam& (1 << 29)) != 0)
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

void Win32DisplayBufferInWindow(win32_offscreen_buffer * Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions)
{
	// TODO: Fix aspect ratio
	StretchDIBits(DevicContext,
		0, 0, WindowDimensions.Width, WindowDimensions.Height, // Distenation
		0, 0, Buffer->Width, Buffer->Height, // Source
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

HRESULT InitializeXAudio(int SampleRate)
{
	// TODO: UncoInitialize on error.
	HRESULT result;

	if (FAILED(result = CoInitialize(NULL)))
		return result;

	if (FAILED(result = XAudio2Create(&xAudio)))
		return result;

	IXAudio2MasteringVoice * masteringVoice;

	if (FAILED(result = xAudio->CreateMasteringVoice(&masteringVoice)))
		return result;

	WAVEFORMATEX waveFormat = { 0 };
	waveFormat.wBitsPerSample = SampleBits;
	waveFormat.nAvgBytesPerSec = (SampleBits / 8) * channels * SampleRate;
	waveFormat.nChannels = channels;
	waveFormat.nBlockAlign = channels * (SampleBits / 8);
	waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	waveFormat.nSamplesPerSec = SampleRate;

	if (FAILED(result = xAudio->CreateSourceVoice(&GlobalSourceVoice, &waveFormat)))
		return result;


	CoUninitialize();

	return result;
}

HRESULT PlayGameSound(game_sound_buffer * SoundBuffer)
{
	HRESULT result;

	if (FAILED(result = CoInitialize(NULL)))
		return result;

	GlobalSoundBuffer.pAudioData = (BYTE*)SoundBuffer->BufferData;
	GlobalSoundBuffer.AudioBytes = SoundBuffer->VoiceBufferSampleCount * (SampleBits / 8);
	GlobalSoundBuffer.LoopCount = 0;

	if (FAILED(GlobalSourceVoice->SubmitSourceBuffer(&GlobalSoundBuffer)))
		return result;

	if (FAILED(GlobalSourceVoice->Start()))
		return result;

	return result;
}

void Win32ProcessDigitalButtons(WORD XInputButtonState, DWORD ButtonBit, game_button_state * OldState, game_button_state * NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}