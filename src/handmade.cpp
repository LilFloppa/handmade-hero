#include "handmade.h"


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

void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	RenderWeirdGradient(buffer, xOffset, yOffset);
}