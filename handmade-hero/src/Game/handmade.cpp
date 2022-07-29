#include "handmade.h"

#include "handmade_intrinsics.h"
#include "handmade_tile.cpp"
#include "handmade_random.h"
#include "handmade_math.h"

internal void DrawRectangle(game_offscreen_buffer* Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
	int32 MinX = RoundReal32ToInt32(vMin.X);
	int32 MinY = RoundReal32ToInt32(vMin.Y);
	int32 MaxX = RoundReal32ToInt32(vMax.X);
	int32 MaxY = RoundReal32ToInt32(vMax.Y);

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

internal void DrawBitmap(game_offscreen_buffer* Buffer, loaded_bitmap* Bitmap, 
	real32 RealX, real32 RealY, int32 AlignX = 0, int32 AlignY = 0)
{
	RealX -= (real32)AlignX;
	RealY -= (real32)AlignY;

	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
	int32 MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

	int32 SourceOffsetX = 0;
	if (MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	int32 SourceOffsetY = 0;
	if (MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}

	if (MaxX > Buffer->Width)
		MaxX = Buffer->Width;

	if (MaxY > Buffer->Height)
		MaxY = Buffer->Height;

	uint32* SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
	uint8* DestRow = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

	for (int32 Y = MinY; Y < MaxY; Y++)
	{
		uint32* Dest = (uint32*)DestRow;
		uint32* Source = SourceRow;
		for (int32 X = MinX; X < MaxX; X++)
		{
			real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
			real32 SR = (real32)((*Source >> 16) & 0xFF);
			real32 SG = (real32)((*Source >> 8) & 0xFF);
			real32 SB = (real32)((*Source >> 0) & 0xFF);

			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);

			// TODO: Someday, we need to talk abount premultiplied alpha!
			// (this is not premultiplied alpha)
			real32 R = (1.0f - A) * DR + A * SR;
			real32 G = (1.0f - A) * DG + A * SG;
			real32 B = (1.0f - A) * DB + A * SB;

			*Dest = (
				((uint32)(R + 0.5f) << 16) |
				((uint32)(G + 0.5f) << 8) |
				((uint32)(B + 0.5f) << 0)
				);

			Dest++;
			Source++;
		}

		DestRow += Buffer->Pitch;
		SourceRow -= Bitmap->Width;
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
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
{
	loaded_bitmap Result = {};
	// NOTE: color bytes order is AA BB GG RR, bottom up
	// In little endian -> 0xRRGGBBAA

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0)
	{
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32 *)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		// NOTE: If you are using this generically for some reason,
		// please remember that BMP files CAN GO IN EITHER DIRECTION and
		// the height will be negative for top-down.
		// (Also, there can be compression, etc., etc.... DON'T think this
		// is complete BMP loading code because it isn't!!!)

		// NOTE: Byte order in memory is determined by the Header itself,
		// so we have to read out the masks and convert the pixels ourselves.
		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

		Assert(RedShift.Found);
		Assert(GreenShift.Found);
		Assert(BlueShift.Found);
		Assert(AlphaShift.Found);

		uint32* SourceDest = Pixels;
		for (int32 Y = 0; Y < Header->Width; Y++)
			for (int32 X = 0; X < Header->Height; X++)
			{
				uint32 C = *SourceDest;
				*SourceDest++ = 
					(((C >> AlphaShift.Index) & 0xFF) << 24 |
					((C >> RedShift.Index) & 0xFF) << 16 |
					((C >> GreenShift.Index) & 0xFF) << 8 |
					((C >> BlueShift.Index) & 0xFF) << 0);
			}
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
		GameState->Backdrop = 
			DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_background.bmp");


		hero_bitmaps* Bitmap = GameState->HeroBitmaps;
		Bitmap->Head = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		Bitmap++;

		Bitmap->Head = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		Bitmap++;

		Bitmap->Head = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		Bitmap++;

		Bitmap->Head = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(NULL, Memory->DEBUGPlatformReadEntireFile, (char*)"C:/repos/handmade-hero/assets/test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		Bitmap++;

		GameState->CameraPos.AbsTileX = 17 / 2;
		GameState->CameraPos.AbsTileY = 9 / 2;

		GameState->PlayerPos.AbsTileX = 3;
		GameState->PlayerPos.AbsTileY = 4;
		GameState->PlayerPos.Offset = { 0.0f, 0.0f };
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
			v2 dPlayer = {};
			if (Controller->MoveUp.EndedDown)
			{
				GameState->HeroFacingDirection = 1;
				dPlayer.Y = 1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				GameState->HeroFacingDirection = 3;
				dPlayer.Y = -1.0f;
			}
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->HeroFacingDirection = 2;
				dPlayer.X = -1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				GameState->HeroFacingDirection = 0;
				dPlayer.X = 1.0f;
			}
			real32 PlayerSpeed = 2.0f;
			if (Controller->ActionUp.EndedDown)
				PlayerSpeed = 20.0f;

			dPlayer *= PlayerSpeed;

			tile_map_position NewPlayerPos = GameState->PlayerPos;
			NewPlayerPos.Offset += Input->dtForFrame * dPlayer;
			NewPlayerPos = RecanonicalizePosition(TileMap, NewPlayerPos);

			tile_map_position PlayerLeft = NewPlayerPos;
			PlayerLeft.Offset.X -= 0.5f * PlayerWidth;
			PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
			tile_map_position PlayerRight = NewPlayerPos;
			PlayerRight.Offset.X += 0.5f * PlayerWidth;
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

			tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerPos, &GameState->CameraPos);
			if (Diff.dXY.X > 9.0f * TileMap->TileSideInMeters)
				GameState->CameraPos.AbsTileX += 17;
			if (Diff.dXY.X < -9.0f * TileMap->TileSideInMeters)
				GameState->CameraPos.AbsTileX -= 17;
			if (Diff.dXY.Y > 5.0f * TileMap->TileSideInMeters)
				GameState->CameraPos.AbsTileY += 9;
			if (Diff.dXY.Y < -5.0f * TileMap->TileSideInMeters)
				GameState->CameraPos.AbsTileY -= 9;

			GameState->CameraPos.AbsTileZ = GameState->PlayerPos.AbsTileZ;
		}
	}

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

	DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

	int32 Offset = 10;
	for (int32 RelRow = -Offset; RelRow < Offset; RelRow++)
	{
		for (int32 RelColumn = -2 * Offset; RelColumn < 2 * Offset; RelColumn++)
		{
			uint32 Column = RelColumn + GameState->CameraPos.AbsTileX;
			uint32 Row = RelRow + GameState->CameraPos.AbsTileY;

			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraPos.AbsTileZ);

			if (TileID > 1)
			{
				real32 Gray = 0.5f;
				if (TileID == 1)
					Gray = 0.2f;

				if (Column == GameState->CameraPos.AbsTileX && Row == GameState->CameraPos.AbsTileY)
					Gray = 0.0f;

				if (TileID > 2)
					Gray = 0.75;

				v2 TileSide = { 0.5f * TileSideInPixels, 0.5f * TileSideInPixels };
				v2 Cen = {
					ScreenCenterX - MetersToPixels * GameState->CameraPos.Offset.X + ((real32)RelColumn) * TileSideInPixels,
					ScreenCenterY + MetersToPixels * GameState->CameraPos.Offset.Y - ((real32)RelRow) * TileSideInPixels
				};
				v2 Min = Cen - TileSide;
				v2 Max = Cen + TileSide;

				DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
			}
		}
	}
	
	tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerPos, &GameState->CameraPos);

	real32 PlayerR = 1.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.0f;
	real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * Diff.dXY.X;
	real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * Diff.dXY.Y;
	v2 PlayerLeftTop = {
		PlayerGroundPointX - 0.5f * MetersToPixels * PlayerWidth,
		PlayerGroundPointY - MetersToPixels * PlayerHeight
	};


	v2 PlayerWidthHeight = { PlayerWidth, PlayerHeight };
	DrawRectangle(Buffer, 
		PlayerLeftTop,
		PlayerLeftTop + MetersToPixels * PlayerWidthHeight,
		PlayerR, PlayerG, PlayerB);

	hero_bitmaps* HeroBitmaps = &GameState->HeroBitmaps[GameState->HeroFacingDirection];
	DrawBitmap(Buffer, &HeroBitmaps->Torso, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
	DrawBitmap(Buffer, &HeroBitmaps->Cape, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
	DrawBitmap(Buffer, &HeroBitmaps->Head, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
}
