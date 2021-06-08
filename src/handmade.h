#ifndef HANDMANDE_H
#define HANDMANDE_H

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float f32;
typedef double f64;

// NOTE: Services that the platform layer provides to the game


// NOTE: Services that the game provides to the platform layer

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

void GameUpdateAndRender(game_offscreen_buffer* buffer, int xOffset, int yOffset);

#endif // !HANDMANDE_H
