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

static bool Running = true;
static win32_offscreen_buffer GlobalBackBuffer;
static IXAudio2* xAudio;
static IXAudio2SourceVoice* GlobalSourceVoice;
static XAUDIO2_BUFFER GlobalSoundBuffer = { 0 };

const int channels = 2;
const int SampleBits = 32;

void Win32LoadXInputModule();
void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions);
void Win32ProcessDigitalButtons(WORD XInputButtonState, DWORD ButtonBit, game_button_state* OldState, game_button_state* NewState);
void Win32ProcessKeyboardMessage(game_button_state* NewState, bool isDown);
void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width);
void ProccessMessageQueue(game_controller_input* KeyboardControllerState);

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

	gameMemory.PermenantStorageSize = Megabytes(64);
	gameMemory.TransiateStorageSize = Gigabytes((uint64)4);

	// TODO: Remove before production :D
	LPVOID baseAddress = (LPVOID)Terabytes(2);
	uint64 totalSize = gameMemory.PermenantStorageSize + gameMemory.TransiateStorageSize;

	gameMemory.PermenantStorage = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	gameMemory.TransiateStorage = ((uint8_t*)gameMemory.PermenantStorage + gameMemory.PermenantStorageSize);

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

				game_controller_input * keyboardControllerState = &newInput->Controllers[0];
				game_controller_input zeroController = {};
				*keyboardControllerState = zeroController;

				// Change for future
				while (Running)
				{
					ProccessMessageQueue(keyboardControllerState);

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

					char buffer[250];
					sprintf_s(buffer, "%.02fms, %.02fFPS, %.02f mc/f \n", milliseconds, fps, mcpf);
					OutputDebugString(buffer);

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
			Running = false;
		} break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			Assert(false);
		}break;

		case WM_DESTROY:
		{
			// TODO: Handle it as an error - try to re-create window
			Running = false;
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

void Win32ProcessKeyboardMessage(game_button_state * NewState, bool isDown)
{
	NewState->EndedDown = isDown;
	NewState->HalfTransitionCount++;
}

debug_read_file_result DEBUGPlatformReadEntireFile(const char* FileName)
{
	debug_read_file_result result = {};

	HANDLE fileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;

		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			Assert(fileSize.QuadPart <= 0xFFFFFFFF);
			uint32 fileSize32 = SafeTruncateUint64(fileSize.QuadPart);
			result.Content = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if (result.Content)
			{
				DWORD bytesRead;

				if (ReadFile(fileHandle, result.Content, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
				{
					// File read successfully
					result.ContentSize = fileSize32;
				}
				else
				{
					// TODO: LOG ERROR COULDNT READ FILE
					DEBUGPlatformReleaseFileMemory(result.Content);
					result.Content = nullptr;
				}
			}
			else
			{
				// TODO: LOG ERROR COULDNT COMMITE MEMORY
			}
		}

		CloseHandle(fileHandle);
	}
	else
	{
		// TODO: LOG ERROR COULDNT OPEN FILE
	}

	return result;
}

void DEBUGPlatformReleaseFileMemory(void* Memory)
{
	if (Memory)
	{
		VirtualFree(&Memory, 0, MEM_RELEASE);
	}
}

bool DEBUGPlatformWriteEntireFile(const char* FileName, uint32 MemorySize, void* Memory)
{
	bool result = false;

	HANDLE fileHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if (fileHandle != INVALID_HANDLE_VALUE)
	{

		DWORD bytesWritten;

		if (WriteFile(fileHandle, Memory, MemorySize, &bytesWritten, 0))
		{
			result = bytesWritten == MemorySize;
		}
		else
		{
			// TODO: Log error couldn't write the file
		}

		CloseHandle(fileHandle);
	}
	else
	{
		// TODO: Log error couldn't create file
	}

	return result;
}


void ProccessMessageQueue(game_controller_input * KeyboardControllerState)
{
	MSG message;

	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			case WM_KEYUP:
			case WM_KEYDOWN:
			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			{
				uint64 virtualKeyCode = message.wParam;
				bool wasDown = ((message.lParam & (1 << 30)) != 0);
				bool isDown = ((message.lParam & (1 << 31)) == 0);

				if (isDown != wasDown)
					switch (virtualKeyCode)
					{
						case 'W':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->UpButton, isDown);
							break;

						case 'A':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->LeftButton, isDown);
							break;

						case 'S':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->DownButton, isDown);
							break;

						case 'D':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->RightButton, isDown);
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
							Win32ProcessKeyboardMessage(&KeyboardControllerState->UpButton, isDown);
							break;

						case VK_LEFT:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->LeftButton, isDown);
							break;

						case VK_RIGHT:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->RightButton, isDown);
							break;

						case VK_DOWN:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->DownButton, isDown);
							break;

						case VK_ESCAPE:
							Running = false;
							break;

						case VK_F4:
						{
							// Is alt button held down
							if ((message.lParam& (1 << 29)) != 0)
								Running = false;

						} break;

						default:
							break;
					}
			} break;

			case WM_QUIT:
			{
				Running = false;

			} break;

			default:
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			} break;
		}
	}
}