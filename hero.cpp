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

	for (int i = 0; i < Buffer->VoiceBufferSampleCount; i += 2)
	{
		auto sineValue = Buffer->wavePeriod * 2 * Pi * (Buffer->Frequency + 2) / Buffer->SampleRate;

		Buffer->BufferData[i] = sinf(sineValue);
		Buffer->BufferData[i + 1] = sinf(sineValue);

		if (Buffer->wavePeriod <= 0)
		{
			Buffer->wavePeriod += 2;
		}
		else
		{
			Buffer->wavePeriod -= 2;
		}
	}
}
