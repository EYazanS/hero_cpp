#pragma once

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

constexpr real32 Pi = 3.14;

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


// Services that the game provide for the platform

// Need to take the use input, the bitmap buffer to use, the sound buffer to use and the timing
void GameUpdateAndRender(game_offscreen_buffer* RenderBuffer, game_sound_buffer* SoundBuffer, game_input* Input);
void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset);
void OutputGameSound(game_sound_buffer* buffer);
