#pragma once


#define ArrayCount(Array) (sizeof((Array)) / sizeof((Array)[0]))

#define Killobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Killobytes(Value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)
#define Assert(Expression) \
	if (!(Expression)) { *(int*)0 = 0; }

#define internal static 

#include <cstdint>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

constexpr real32 Pi = 3.14f;

struct game_offscreen_buffer
{
	void* Memory;
	int Height;
	int Width;
	int BytesPerPixel;
	int Pitch;
};

struct game_sound_buffer
{
	uint32 VoiceBufferSampleCount;
	int SampleRate;
	int Frequency;
	real32 WavePeriod;
	real32 Time;
	real32 BufferData[96000];
};

struct game_button_state
{
	bool32 EndedDown;
	int HalfTransitionCount;
};

struct game_controller_input
{
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state Buttons[12];

		struct
		{
			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionRight;
			game_button_state ActionLeft;

			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveRight;
			game_button_state MoveLeft;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;

			// Add button before this line so the assertion about the buttons array == the struct can hit properly 
			game_button_state Terminator;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[5];
};

struct game_state
{
	int ToneHz;
	int YOffset;
	int XOffset;
};

struct game_memory
{
	uint64 PermenantStorageSize;
	uint64 TransiateStorageSize;
	void* PermenantStorage;
	void* TransiateStorage;
	bool32 IsInitialized;
};

struct debug_read_file_result
{
	void* Content;
	uint32 ContentSize;
};

// Utility functions 
inline game_controller_input* GetController(game_input* Input, size_t ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input* result = &Input->Controllers[ControllerIndex];
	return result;
}

inline uint32 SafeTruncateUint64(uint64 Value)
{
	Assert(Value < 0xFFFFFFFF);
	uint32 newUint32Value = (uint32)Value;
	return newUint32Value;
}

// Services that the game provide for the platform
void DEBUGPlatformReleaseFileMemory(void* Memory);
bool32 DEBUGPlatformWriteEntireFile(const char* FileName, uint32 MemorySize, void* Memory);
debug_read_file_result DEBUGPlatformReadEntireFile(const char* FileName);

// Need to take the use input, the bitmap buffer to use, the sound buffer to use and the timing

void GameUpdateAndRender(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_buffer* SoundBuffer);
void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset);
void OutputGameSound(game_sound_buffer* buffer);