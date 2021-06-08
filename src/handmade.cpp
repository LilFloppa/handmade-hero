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

internal void GameOutputSound(game_sound_buffer* soundBuffer)
{
	local_persist f32 t;
	int16 toneVolume = 600;
	int toneHz = 256;
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

void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffset, int yOffset, game_sound_buffer* soundBuffer)
{
	GameOutputSound(soundBuffer);
	RenderWeirdGradient(buffer, xOffset, yOffset);
}