#include "hero.h"

void GameUpdateAndRender(game_offscreen_buffer* RenderBuffer, int XOffset, int YOffset, game_sound_buffer* SoundBuffer)
{
	// TODO: Allow sample offset for more robust platform options
	RenderGradiant(RenderBuffer, XOffset, YOffset);
	OutputGameSound(SoundBuffer);
}

void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
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

void OutputGameSound(game_sound_buffer * buffer)
{
	static real32 tSine;
	int16  toneVolume = 3000;

	for (int i = 0; i < buffer->VoiceBufferSampleCount; i += 2)
	{
		buffer->BufferData[i] = sinf(i * 2 * Pi * buffer->Frequency / buffer->SampleRate);
		buffer->BufferData[i + 1] = sinf(i * 2 * Pi * (buffer->Frequency + 2) / buffer->SampleRate);
	}
}
