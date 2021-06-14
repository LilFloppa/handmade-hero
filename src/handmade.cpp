#include "handmade.h"

#include <math.h>


void RenderWeirdGradient(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	// TODO(nikita): Let's see what the optimizer does
	uint8* row = (uint8*)buffer->Memory;
	for (int y = 0; y < buffer->Height; y++)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer->Width; x++)
		{
			uint8 Blue = (x + xOffset);
			uint8 Green = (y + yOffset);
			uint8 Red = 128;
			*pixel++ = (Blue | (Green << 8) | (Red << 16));
		}

		row += buffer->Pitch;
	}
}

internal void GameOutputSound(game_sound_buffer* soundBuffer, int toneHz)
{
	local_persist f32 t;
	int16 toneVolume = 600;
	int wavePeriod = soundBuffer->SamplesPerSecond / toneHz;

	int16* sampleOut = soundBuffer->Samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->SamplesCount; sampleIndex++)
	{
		f32 sineValue = sinf(t);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		t += 2.0f * PI32 * 1.0f / (f32)wavePeriod;
	}
}

void GameUpdateAndRender(game_memory* memory, game_input* input, game_offscreen_buffer* buffer, game_sound_buffer* soundBuffer)
{
	Assert(sizeof(game_state) <= memory->PermanentStorageSize);

	game_state* gameState = (game_state*)memory->PermanentStorage;
	if (!memory->IsInitialized)
	{
		const char* filename = __FILE__;
		debug_read_file_result file = DEBUGPlatformReadEntireFile(filename);

		if (file.Content)
		{
			DEBUGPlatformWriteEntireFile("test.out", file.ContentSize, file.Content);
			DEBUGPlatformFreeFileMemory(file.Content);
		}

		gameState->ToneHz = 256;
		gameState->GreenOffset = 0;
		gameState->BlueOffset = 0;

		// TODO: This may be more appropritate to do in the platform layer
		memory->IsInitialized = true;
	}

	game_controller_input* input0 = &(input->Controllers[0]);

	if (input0->Down.EndedDown)
	{
		gameState->GreenOffset += 1;
	}

	gameState->ToneHz = 256 + (int)(128.0f * input0->EndY);
	gameState->BlueOffset += (int)4.0f * (input0->EndX);

	GameOutputSound(soundBuffer, gameState->ToneHz);
	RenderWeirdGradient(buffer, gameState->BlueOffset, gameState->GreenOffset);
}