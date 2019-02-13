#include "hero.h"

void RenderGradiant(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
	uint8* row = (uint8*)Buffer->Memory;

	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32* pixel = (uint32*)row;

		for (int x = 0; x < Buffer->Width; ++x)
		{
			uint8 blue = (uint8)(x + XOffset);
			uint8 green = (uint8)(y + YOffset);
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

void GameUpdateAndRender(game_memory * Memory, game_input * Input, game_offscreen_buffer * Buffer, game_sound_buffer * SoundBuffer)
{
	Assert(&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0] == (ArrayCount(Input->Controllers[0].Buttons)));
	Assert(sizeof(game_state) <= Memory->PermenantStorageSize);

	game_state * gameState = (game_state*)Memory->PermenantStorage;

	if (!Memory->IsInitialized)
	{
		/*	const char* fileName = "tempFile.txt";

			debug_read_file_result file = DEBUGPlatformReadEntireFile(__FILE__);

			if (file.Content)
			{
				DEBUGPlatformWriteEntireFile(fileName, file.ContentSize, file.Content);
				DEBUGPlatformReleaseFileMemory(file.Content);
			}*/

		gameState->ToneHz = 256;
		Memory->IsInitialized = true;
	}

	for (size_t controllerIndex = 0; controllerIndex < ArrayCount(Input->Controllers); controllerIndex++)
	{
		game_controller_input* controller = GetController(Input, controllerIndex);

		if (controller->IsAnalog)
		{
			gameState->XOffset += (int32)(4.f * (controller->StickAverageX));
			gameState->YOffset -= (int32)(4.f * (controller->StickAverageY));
		}
		else
		{
			uint16 howMuchToMove = 10;

			//Use digital movement system
			if (controller->MoveUp.EndedDown)
			{
				gameState->YOffset += howMuchToMove;
			}

			if (controller->MoveDown.EndedDown)
			{
				gameState->YOffset -= howMuchToMove;
			}

			if (controller->MoveRight.EndedDown)
			{
				gameState->XOffset -= howMuchToMove;
			}

			if (controller->MoveLeft.EndedDown)
			{
				gameState->XOffset += howMuchToMove;
			}
		}
	}

	// TODO: Allow sample offset for more robust platform options
	RenderGradiant(Buffer, gameState->XOffset, gameState->YOffset);
	OutputGameSound(SoundBuffer);
}