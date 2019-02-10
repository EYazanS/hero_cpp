#pragma once

#define ArrayCount(Array) (sizeof((Array)) / sizeof((Array)[0]))

#define Killobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Killobytes(Value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)
#define Assert(Expression) \
	if (!(Expression)) { *(int*)0 = 0; }

#include <cstdint>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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
	real32 BufferData[96000];
	int VoiceBufferSampleCount;
	int SampleRate;
	int Frequency;
	real32 WavePeriod;
	real32 Time;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	bool IsAnalog;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state UpButton;
			game_button_state DownButton;
			game_button_state RightButton;
			game_button_state LeftButton;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
			game_button_state Start;
			game_button_state Back;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};

struct game_state
{
	int ToneHz;
	int YOffset;
	int XOffset;
};

struct game_memory
{
	bool IsInitialized;
	uint64 PermenantStorageSize;
	void* PermenantStorage;
	uint64 TransiateStorageSize;
	void* TransiateStorage;
};

struct debug_read_file_result
{
	void* Content;
	uint32 ContentSize;
};

// Utility functions 
inline uint32 SafeTruncateUint64(uint64 Value)
{
	Assert(Value < 0xFFFFFFFF);
	uint32 newUint32Value = (uint32)Value;
	return newUint32Value;
}

// Services that the game provide for the platform
void DEBUGPlatformReleaseFileMemory(void* Memory);
bool DEBUGPlatformWriteEntireFile(const char* FileName, int32 MemorySize, void* Memory);
debug_read_file_result DEBUGPlatformReadEntireFile(const char* FileName);

// Need to take the use input, the bitmap buffer to use, the sound buffer to use and the timing
void GameUpdateAndRender(game_memory* Memory, game_offscreen_buffer* RenderBuffer, game_sound_buffer* SoundBuffer, game_input* Input);
void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset);
void OutputGameSound(game_sound_buffer* buffer);