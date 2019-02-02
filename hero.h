#pragma once

#include <cstdint>
#include <math.h>

constexpr float Pi = 3.14;

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
	float BufferData[96000];
	int VoiceBufferSampleCount;
	int SampleRate;
	int Frequency;
};

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

// Services that the game provide for the platform

// Need to take the use input, the bitmap buffer to use, the sound buffer to use and the timing
void GameUpdateAndRender(game_offscreen_buffer* RenderBuffer, int XOffset, int YOffset, game_sound_buffer* SoundBuffer);
void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset);
void OutputGameSound(game_sound_buffer* buffer);
