#ifndef HANDMANDE_H
#define HANDMANDE_H

#include <stdint.h>

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)

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

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// NOTE: Services that the platform layer provides to the game


// NOTE: Services that the game provides to the platform layer

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_buffer
{
	int SamplesPerSecond;
	int SamplesCount;
	int16* Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;

	f32 StartX;
	f32 StartY;

	f32 MinX;
	f32 MinY;

	f32 MaxX;
	f32 MaxY;

	f32 EndX;
	f32 EndY;
	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void* PermanentStorage;
};

void GameUpdateAndRender(game_memory* memory, game_input* input,  game_offscreen_buffer* buffer, game_sound_buffer* soundBuffer);


struct game_state
{
	int ToneHz;
	int BlueOffset;
	int GreenOffset;
};
#endif // !HANDMANDE_H
