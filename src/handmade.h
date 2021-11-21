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

#if _DEBUG
#define Assert(Expression) if (!(Expression)) { *(int*)0 = 0; }
#else
#define Assert(Expression)
#endif

#define Kilobytes(value) (((uint64)value) * 1024)
#define Megabytes(value) (Kilobytes((uint64)value) * 1024)
#define Gigabytes(value) (Megabytes((uint64)value) * 1024)
#define Terabytes(value) (Gigabytes((uint64)value) * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateSize32(uint64 value)
{
	Assert(value <= 0xFFFFFFFF);
	uint32 result = (uint32)value;
	return result;
}
// NOTE: Services that the platform layer provides to the game


#if _DEBUG
struct debug_read_file_result
{
	uint32 ContentSize;
	void* Content;
};

debug_read_file_result DEBUGPlatformReadEntireFile(const char* filename);
void DEBUGPlatformFreeFileMemory(void* memory);
bool32 DEBUGPlatformWriteEntireFile(const char* filename, uint32 memorySize, void* memory);
#endif
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
	bool32 IsConnected;
	bool32 IsAnalog;
	f32 StickAverageX;
	f32 StickAverageY;

	union
	{
		game_button_state Buttons[10];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Back;
			game_button_state Start;
		};
	};
};

struct game_input
{
	// TODO: Insert clock values here.
	game_controller_input Controllers[5];
};

inline game_controller_input* GetControllers(game_input* input, int controllerIndex)
{
	Assert(controllerIndex < ArrayCount(input->Controllers));
	game_controller_input* result = &input->Controllers[controllerIndex];
	return result;
}

struct game_memory
{
	bool32 IsInitialized;
	
	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;
};

struct game_clocks
{
	f32 SecondsElapsed;
};

void GameUpdateAndRender(game_memory* memory, game_input* input, game_offscreen_buffer* buffer, game_sound_buffer* soundBuffer);

struct game_state
{
	int ToneHz;
	int BlueOffset;
	int GreenOffset;
};

#endif // !HANDMANDE_H
