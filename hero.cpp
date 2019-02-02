#include "hero.h"

void GameUpdateAndRender(game_offscreen_buffer* RenderBuffer, int XOffset, int YOffset, game_sound_buffer* SoundBuffer)
{
	// TODO: Allow sample offset for more robust platform options
	RenderGradiant(RenderBuffer, XOffset, YOffset);
	// OutputGameSound(SoundBuffer);
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

void OutputGameSound(game_sound_buffer * Buffer)
{
	for (size_t i = 0; i < Buffer->VoiceBufferSampleCount; ++i)
	{
		Buffer->BufferData[i] = sinf((Buffer->Time * 2 * Pi) / Buffer->WavePeriod);
		Buffer->Time += (1.0f / Buffer->Frequency);             // move time forward one sample-tick

		if (Buffer->Time > Buffer->WavePeriod)
			Buffer->Time -= Buffer->WavePeriod;    // if we're beyond the period of the sine, skip time back by one period
	}
}
