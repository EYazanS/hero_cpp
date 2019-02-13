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

static bool32 Running = true;
static win32_offscreen_buffer GlobalBackBuffer;
static IXAudio2* xAudio;
static IXAudio2SourceVoice* GlobalSourceVoice;
static XAUDIO2_BUFFER GlobalSoundBuffer = { 0 };

const int channels = 2;
const int SampleBits = 32;

internal void Win32LoadXInputModule(void);
internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DevicContext, win32_window_dimensions WindowDimensions);
internal void Win32ProcessDigitalButtons(WORD XInputButtonState, DWORD ButtonBit, game_button_state* OldState, game_button_state* NewState);
internal void Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 isDown);
internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Height, int Width);
internal void Win32ProcessMessageQueue(game_controller_input* KeyboardControllerState);

internal real32 Win32ProcessXInputStickValues(real32 value, SHORT deadZoneThreshold);

internal win32_window_dimensions Win32GetWindowDimensions(HWND Window);

internal LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

internal HRESULT InitializeXAudio(int SampleRate);
// internal HRESULT PlayGameSound(game_sound_buffer* SoundBuffer);

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

	if (gameMemory.PermenantStorage)
	{
		if (RegisterClassA(&windowsClass))
		{
			HWND window = CreateWindowExA(
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

				// Initialize XAudio
				InitializeXAudio(soundBuffer.SampleRate);

				LARGE_INTEGER lastCounter;

				QueryPerformanceCounter(&lastCounter);
				// Win32InitXAudioSound(sampleBits, Channels, samplesPerSecond);

				game_input gameInput[2] = {};
				game_input* newInput = &gameInput[0];
				game_input* oldInput = &gameInput[1];


				uint16 maxControlllersCount = XUSER_MAX_COUNT;

				if (maxControlllersCount > (ArrayCount(newInput->Controllers) - 1))
					maxControlllersCount = (ArrayCount(newInput->Controllers) - 1);

				game_controller_input * oldKeyboardControllerState = GetController(oldInput, 0);
				game_controller_input * newKeyboardControllerState = GetController(newInput, 0);

				*newKeyboardControllerState = {};

				for (size_t buttonIndex = 0; buttonIndex < ArrayCount(newKeyboardControllerState->Buttons); buttonIndex++)
				{
					newKeyboardControllerState->Buttons[buttonIndex].EndedDown = oldKeyboardControllerState->Buttons[buttonIndex].EndedDown;
				}

				// Change for future
				while (Running)
				{
					// Extract messages from windows
					Win32ProcessMessageQueue(newKeyboardControllerState);

					for (int controllerIndex = 0; controllerIndex < maxControlllersCount; controllerIndex++)
					{
						DWORD ourControllerIndex = controllerIndex + 1;
						game_controller_input* oldControllerState = GetController(oldInput, ourControllerIndex);
						game_controller_input* newControllerState = GetController(newInput, ourControllerIndex);

						XINPUT_STATE controllerState;

						if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
						{
							newControllerState->IsConnected = true;

							// Controller is plugged in
							XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

							// Check for the dead zone
							newControllerState->StickAverageX = Win32ProcessXInputStickValues((real32)pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							newControllerState->StickAverageY = Win32ProcessXInputStickValues((real32)pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if (newControllerState->StickAverageY != 0.0f || newControllerState->StickAverageX != 0.0f)
							{
								newControllerState->IsAnalog = true;
							}

							real32 moveThreashhold = 0.5f;

							Win32ProcessDigitalButtons((newControllerState->StickAverageX < -moveThreashhold) ? 1 : 0,
								XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->MoveLeft, &newControllerState->MoveLeft);

							Win32ProcessDigitalButtons((newControllerState->StickAverageX > moveThreashhold) ? 1 : 0,
								XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->MoveLeft, &newControllerState->MoveRight);

							Win32ProcessDigitalButtons((newControllerState->StickAverageY < -moveThreashhold) ? 1 : 0,
								XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->MoveLeft, &newControllerState->MoveDown);

							Win32ProcessDigitalButtons((newControllerState->StickAverageY > moveThreashhold) ? 1 : 0,
								XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->MoveLeft, &newControllerState->MoveUp);

							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN, &oldControllerState->ActionDown, &newControllerState->ActionDown);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_UP, &oldControllerState->ActionUp, &newControllerState->ActionUp);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, &oldControllerState->ActionRight, &newControllerState->ActionRight);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT, &oldControllerState->ActionLeft, &newControllerState->ActionLeft);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, &oldControllerState->RightShoulder, &newControllerState->RightShoulder);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, &oldControllerState->LeftShoulder, &newControllerState->LeftShoulder);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_START, &oldControllerState->Start, &newControllerState->Start);
							Win32ProcessDigitalButtons(pad->wButtons, XINPUT_GAMEPAD_BACK, &oldControllerState->Back, &newControllerState->Back);

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
							newControllerState->IsConnected = false;
						}
					}

					game_offscreen_buffer gameBuffer = {};

					gameBuffer.Height = GlobalBackBuffer.Height;
					gameBuffer.Width = GlobalBackBuffer.Width;
					gameBuffer.Memory = GlobalBackBuffer.Memory;
					gameBuffer.Pitch = GlobalBackBuffer.Pitch;

					GameUpdateAndRender(&gameMemory, newInput, &gameBuffer, &soundBuffer);

					// PlayGameSound(&soundBuffer);

					win32_window_dimensions dimensions = Win32GetWindowDimensions(window);
					Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, dimensions);

					LARGE_INTEGER endCountrer;
					QueryPerformanceCounter(&endCountrer);

					endCountrer.QuadPart;
					// ReleaseDC(window, deviceContext);
					uint64 endCycleCount = __rdtsc();

					uint64 cycleElapsed = endCycleCount - lastCycleCount;
					uint64 counterElapsed = endCountrer.QuadPart - lastCounter.QuadPart;
					real64 milliseconds = (1000.f * (real32)counterElapsed) / (real32)performanceCounterFrequency;
					real64 fps = (real32)performanceCounterFrequency / (real32)counterElapsed;
					real64 mcpf = (real64)(cycleElapsed / (1000.f * 1000.f));

					char buffer[250];
					sprintf_s(buffer, "%.02fms, %.02fFPS, %.02f mc/f \n", milliseconds, fps, mcpf);
					OutputDebugStringA(buffer);

					lastCounter = endCountrer;
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
			result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;

	}

	return (result);
}

void Win32LoadXInputModule(void)
{
	HMODULE xInputModule = LoadLibraryA("xinput1_4.dll");

	// Show which version is loaded
	if (!xInputModule)
		xInputModule = LoadLibraryA("xinput1_3.dll");

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

/*HRESULT PlayGameSound(game_sound_buffer * SoundBuffer)
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
}*/

void Win32ProcessDigitalButtons(WORD XInputButtonState, DWORD ButtonBit, game_button_state * OldState, game_button_state * NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

void Win32ProcessKeyboardMessage(game_button_state * NewState, bool32 isDown)
{
	Assert(NewState->EndedDown != isDown);
	NewState->EndedDown = isDown;
	NewState->HalfTransitionCount++;
}

debug_read_file_result DEBUGPlatformReadEntireFile(const char* FileName)
{
	debug_read_file_result result = {};

	HANDLE fileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

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

bool32 DEBUGPlatformWriteEntireFile(const char* FileName, uint32 MemorySize, void* Memory)
{
	bool32 result = 0;

	HANDLE fileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

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

void Win32ProcessMessageQueue(game_controller_input * KeyboardControllerState)
{
	MSG message;

	while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			case WM_KEYUP:
			case WM_KEYDOWN:
			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			{
				uint64 virtualKeyCode = message.wParam;
				bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
				bool32 isDown = ((message.lParam & (1 << 31)) == 0);

				if (isDown != wasDown)
					switch (virtualKeyCode)
					{
						case 'W':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->MoveUp, isDown);
							break;

						case 'A':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->MoveLeft, isDown);
							break;

						case 'S':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->MoveDown, isDown);
							break;

						case 'D':
							Win32ProcessKeyboardMessage(&KeyboardControllerState->MoveRight, isDown);
							break;

						case 'E':
							OutputDebugStringA("E");
							break;

						case 'Q':
							OutputDebugStringA("Q");
							break;

						case VK_SPACE:
							OutputDebugStringA("Space");
							break;

						case VK_UP:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->ActionUp, isDown);
							break;

						case VK_LEFT:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->ActionLeft, isDown);
							break;

						case VK_RIGHT:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->ActionRight, isDown);
							break;

						case VK_DOWN:
							Win32ProcessKeyboardMessage(&KeyboardControllerState->ActionDown, isDown);
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

real32 Win32ProcessXInputStickValues(real32 value, SHORT deadZoneThreshold)
{
	real32 result = 0.f;

	if (value < -deadZoneThreshold)
		result = (real32)(value + deadZoneThreshold) / (32768.f - deadZoneThreshold);
	else if (value > deadZoneThreshold)
		result = (real32)(value + deadZoneThreshold) / (32767.f - deadZoneThreshold);

	return result;
}