#include "handmade.h"
#include "handmade_intrinsics.h"

#include "handmade_tile.cpp"
#include "handmade_random.h"

internal void DrawRectangle(game_offscreen_buffer* Buffer,
	real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
	real32 R, real32 G, real32 B)
{
	// TODO(casey): Floating point color tomorrow!!!!!!

	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);

	if (MinX < 0)
		MinX = 0;
	if (MinY < 0)
		MinY = 0;

	if (MaxX > Buffer->Width)
		MaxX = Buffer->Width;

	if (MaxY > Buffer->Height)
		MaxY = Buffer->Height;

	// Bit pattern: 0x AA RR GG BB
	uint32 Color = (
		(RoundReal32ToInt32(R * 255.0f) << 16) |
		(RoundReal32ToInt32(G * 255.0f) << 8) |
		(RoundReal32ToInt32(B * 255.0f) << 0));

	uint8* Row = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; Y++)
	{
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; X++)
		{
			*Pixel++ = Color;
		}

		Row += Buffer->Pitch;
	}
}

#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
};
#pragma pack(pop)

internal uint32* DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
{
	uint32* Result = 0;
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0)
	{
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32 *)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result = Pixels;
	}

	return Result;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	real32 PlayerHeight = 1.4f;
	real32 PlayerWidth = 0.75f * PlayerHeight;

	int32 TileSideInPixels;
	real32 MetersToPixels;

	game_state* GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		GameState->PixelPointer = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_background.bmp");
		GameState->PlayerPos.AbsTileX = 3;
		GameState->PlayerPos.AbsTileY = 4;
		GameState->PlayerPos.OffsetX = 0.0f;
		GameState->PlayerPos.OffsetY = 0.0f;
		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8*)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world* World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

		tile_map* TileMap = World->TileMap;

		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);

		TileMap->TileSideInMeters = 1.4f;

		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(
			&GameState->WorldArena,
			TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ,
			tile_chunk);

		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenX = 0;
		uint32 ScreenY = 0;

		uint32 AbsTileZ = 0;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 100; ScreenIndex++)
		{
			// TODO: Random Number Generator!
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice;
			if (DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			else
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			}

			bool32 CreatedZDoor = false;
			if (RandomChoice == 2)
			{
				CreatedZDoor = true;
				if (AbsTileZ == 0)
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
			}
			else if (RandomChoice == 1)
				DoorRight = true;
			else
				DoorTop = true;

			for (uint32 TileY = 0; TileY < TilesPerHeight; TileY++)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; TileX++)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					uint32 TileValue = 1;
					if (TileX == 0 && (!DoorLeft || TileY != (TilesPerHeight / 2)))
					{
						TileValue = 2;
					}
					if (TileX == (TilesPerWidth - 1) && (!DoorRight || TileY != (TilesPerHeight / 2)))
					{
						TileValue = 2;
					}
					if (TileY == 0 && (!DoorBottom || TileX != (TilesPerWidth / 2)))
					{
						TileValue = 2;
					}
					if (TileY == (TilesPerHeight - 1) && (!DoorTop || TileX != (TilesPerWidth / 2)))
					{
						TileValue = 2;
					}

					if (TileX == 10 && TileY == 6)
					{
						if (DoorUp)
							TileValue = 3;

						if (DoorDown)
							TileValue = 4;
					}

					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
				}
			}

			DoorLeft = DoorRight;
			DoorBottom = DoorTop;

			if (CreatedZDoor)
			{
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}

			DoorRight = false;
			DoorTop = false;

			if (RandomChoice == 2)
			{
				if (AbsTileZ == 0)
				{
					AbsTileZ = 1;
				}
				else
				{
					AbsTileZ = 0;
				}
			}
			else if (RandomChoice == 1)
				ScreenX += 1;
			else
				ScreenY += 1;
		}

		Memory->IsInitialized = true;
	}

	world* World = GameState->World;
	tile_map* TileMap = World->TileMap;

	TileSideInPixels = 60;
	MetersToPixels = (real32)TileSideInPixels / (real32)TileMap->TileSideInMeters;

	real32 LowerLeftX = -(real32)TileSideInPixels / 2.0f;
	real32 LowerLeftY = Buffer->Height;

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		if (!Controller->IsAnalog)
		{
			real32 dPlayerX = 0.0f;
			real32 dPlayerY = 0.0f;
			if (Controller->MoveUp.EndedDown)
				dPlayerY = 1.0f;
			if (Controller->MoveDown.EndedDown)
				dPlayerY = -1.0f;
			if (Controller->MoveLeft.EndedDown)
				dPlayerX = -1.0f;
			if (Controller->MoveRight.EndedDown)
				dPlayerX = 1.0f;
			real32 PlayerSpeed = 2.0f;
			if (Controller->ActionUp.EndedDown)
				PlayerSpeed = 20.0f;

			dPlayerX *= PlayerSpeed;
			dPlayerY *= PlayerSpeed;

			tile_map_position NewPlayerPos = GameState->PlayerPos;
			NewPlayerPos.OffsetX += Input->dtForFrame * dPlayerX;
			NewPlayerPos.OffsetY += Input->dtForFrame * dPlayerY;
			NewPlayerPos = RecanonicalizePosition(TileMap, NewPlayerPos);

			tile_map_position PlayerLeft = NewPlayerPos;
			PlayerLeft.OffsetX -= 0.5f * PlayerWidth;
			PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
			tile_map_position PlayerRight = NewPlayerPos;
			PlayerRight.OffsetX += 0.5f * PlayerWidth;
			PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);


			if (
				IsWorldPointEmpty(TileMap, NewPlayerPos) &&
				IsWorldPointEmpty(TileMap, PlayerLeft) &&
				IsWorldPointEmpty(TileMap, PlayerRight))
			{
				if (!AreOnSameTile(&GameState->PlayerPos, &NewPlayerPos))
				{
					uint32 NewTileValue = GetTileValue(TileMap, NewPlayerPos.AbsTileX, NewPlayerPos.AbsTileY, NewPlayerPos.AbsTileZ);
					if (NewTileValue == 3)
					{
						NewPlayerPos.AbsTileZ++;
					}
					else if (NewTileValue == 4)
					{
						NewPlayerPos.AbsTileZ--;
					}
				}
				GameState->PlayerPos = NewPlayerPos;
			}
		}
	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 0.1f);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

	int32 Offset = 10;
	for (int32 RelRow = -Offset; RelRow < Offset; RelRow++)
	{
		for (int32 RelColumn = -2 * Offset; RelColumn < 2 * Offset; RelColumn++)
		{
			uint32 Column = RelColumn + GameState->PlayerPos.AbsTileX;
			uint32 Row = RelRow + GameState->PlayerPos.AbsTileY;

			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerPos.AbsTileZ);

			if (TileID > 0)
			{
				real32 Gray = 0.5f;
				if (TileID == 1)
					Gray = 0.2f;

				if (Column == GameState->PlayerPos.AbsTileX && Row == GameState->PlayerPos.AbsTileY)
					Gray = 0.0f;

				if (TileID > 2)
					Gray = 0.75;

				real32 CenX = ScreenCenterX - MetersToPixels * GameState->PlayerPos.OffsetX + ((real32)RelColumn) * TileSideInPixels;
				real32 CenY = ScreenCenterY + MetersToPixels * GameState->PlayerPos.OffsetY - ((real32)RelRow) * TileSideInPixels;
				real32 MinX = CenX - 0.5f * TileSideInPixels;
				real32 MinY = CenY - 0.5f * TileSideInPixels;
				real32 MaxX = CenX + 0.5f * TileSideInPixels;
				real32 MaxY = CenY + 0.5f * TileSideInPixels;

				DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
			}
		}
	}

	real32 PlayerR = 1.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.0f;

	real32 PlayerLeft = ScreenCenterX - 0.5f * MetersToPixels * PlayerWidth;
	real32 PlayerTop = ScreenCenterY - MetersToPixels * PlayerHeight;

	DrawRectangle(Buffer,
		PlayerLeft,
		PlayerTop,
		PlayerLeft + PlayerWidth * MetersToPixels,
		PlayerTop + PlayerHeight * MetersToPixels,
		PlayerR, PlayerG, PlayerB);

	uint32* Source = GameState->PixelPointer;
	uint32* Dest = (uint32 *)Buffer->Memory;
	int32 index = 0;
	for (int32 Y = 0; Y < Buffer->Height; Y++)
		for (int32 X = 0; X < Buffer->Width; X++)
			if (index < 500000)
			{
				*Dest++ = *Source++;
				index++;
			}
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
}
