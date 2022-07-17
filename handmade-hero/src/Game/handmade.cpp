#include "handmade.h"
#include "handmade_intrinsics.h"

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


inline tile_map* GetTileMap(world* World, int32 TileMapX, int32 TileMapY)
{
    tile_map* TileMap = 0;
    if (TileMapX >= 0 && TileMapX < World->TileMapCountX && TileMapY >= 0 && TileMapY < World->TileMapCountY)
    {
        TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
    }

    return TileMap;
}

inline uint32 GetTileValueUnchecked(world* World, tile_map* TileMap, int32 TileX, int32 TileY)
{
    Assert(TileMap);
    Assert(TileX >= 0 && TileX < World->TileCountX && TileY >= 0 && TileY < World->TileCountY);

    return TileMap->Tiles[TileY * World->TileCountX + TileX];
}

internal bool32 IsTileMapPointEmpty(world *World, tile_map* TileMap, real32 TestTileX, real32 TestTileY)
{
    bool32 Empty = false;
    if (TileMap)
    {
        if (TestTileX >= 0 && TestTileX < World->TileCountX && TestTileY >= 0 && TestTileY < World->TileCountY)
        {
            uint32 TileMapValue = GetTileValueUnchecked(World ,TileMap, TestTileX, TestTileY);
            Empty = (TileMapValue == 0);
        }
    }

    return Empty;
}

internal canonical_position GetCanonicalPosition(world* World, raw_position Pos)
{
    canonical_position Result;

    Result.TileMapX = Pos.TileMapX;
    Result.TileMapY = Pos.TileMapY;

    real32 X = Pos.X - World->UpperLeftX;
    real32 Y = Pos.Y - World->UpperLeftY;
    Result.TileX = FloorReal32ToInt32(X / World->TileSideInPixels);
    Result.TileY = FloorReal32ToInt32(Y / World->TileSideInPixels);

    Result.TileRelX = Pos.X - Result.TileX * World->TileSideInPixels;
    Result.TileRelY = Pos.Y - Result.TileY * World->TileSideInPixels;

    Assert(Result.TileRelX >= 0);
    Assert(Result.TileRelY >= 0);
    Assert(Result.TileRelX < World->TileSideInPixels);
    Assert(Result.TileRelY < World->TileSideInPixels);

    if (Result.TileX < 0)
    {
        Result.TileX = World->TileCountX + Result.TileX;
        Result.TileMapX--;
    }

    if (Result.TileY < 0)
    {
        Result.TileY = World->TileCountY + Result.TileY;
        Result.TileMapY--;
    }

    if (Result.TileX >= World->TileCountX)
    {
        Result.TileX = Result.TileX - World->TileCountX;
        Result.TileMapX++;
    }

    if (Result.TileY >= World->TileCountY)
    {
        Result.TileY = Result.TileY - World->TileCountY;
        Result.TileMapY++;
    }

    return Result;
}

internal bool32 IsWorldPointEmpty(world* World, raw_position TestPos)
{
    bool32 Empty = false;

    canonical_position CanPos = GetCanonicalPosition(World, TestPos);
    tile_map* TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
    Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);

    return Empty;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerX = 200;
        GameState->PlayerY = 200;
        Memory->IsInitialized = true;
    }

#define TILEMAP_COUNT_X 16
#define TILEMAP_COUNT_Y 9
    real32 UpperLeftX = 0.0f;
    real32 UpperLeftY = 0.0f;

    uint32 Tiles00[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 1, 0, 0,  0, 0, 0, 1,  1, 1, 0, 1,  0, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  1, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
    };

    uint32 Tiles01[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    uint32 Tiles10[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
    };

    uint32 Tiles11[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    world World;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.TileCountX = TILEMAP_COUNT_X;
    World.TileCountY = TILEMAP_COUNT_Y;
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 75;
    World.UpperLeftX = UpperLeftX;
    World.UpperLeftY = UpperLeftY;

    tile_map TileMaps[2][2];

    TileMaps[0][0].Tiles = (uint32*)Tiles00;
    TileMaps[0][1].Tiles = (uint32*)Tiles10;
    TileMaps[1][0].Tiles = (uint32*)Tiles01;
    TileMaps[1][1].Tiles = (uint32*)Tiles11;

    World.TileMaps = (tile_map*)TileMaps;

    tile_map* TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
    Assert(TileMap);

    real32 PlayerWidth = 0.75f * World.TileSideInPixels;
    real32 PlayerHeight = World.TileSideInPixels;

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (!Controller->IsAnalog)
        {
            real32 dPlayerX = 0.0f;
            real32 dPlayerY = 0.0f;
            if (Controller->MoveUp.EndedDown)
                dPlayerY = -1.0f;
            if (Controller->MoveDown.EndedDown)
                dPlayerY = 1.0f;
            if (Controller->MoveLeft.EndedDown)
                dPlayerX = -1.0f;
            if (Controller->MoveRight.EndedDown)
                dPlayerX = 1.0f;

            real32 NewPlayerX = GameState->PlayerX + Input->dtForFrame * dPlayerX * 256.0f;
            real32 NewPlayerY = GameState->PlayerY + Input->dtForFrame * dPlayerY * 256.0f;

            raw_position PlayerPos = { GameState->PlayerTileMapX, GameState->PlayerTileMapY, NewPlayerX, NewPlayerY };
            raw_position PlayerLeft = PlayerPos;
            PlayerLeft.X -= 0.5f * PlayerWidth;
            raw_position PlayerRight = PlayerPos;
            PlayerRight.X += 0.5f * PlayerWidth;

            if (
                IsWorldPointEmpty(&World, PlayerPos) &&
                IsWorldPointEmpty(&World, PlayerLeft) &&
                IsWorldPointEmpty(&World, PlayerRight))
            { 
                canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);

                GameState->PlayerTileMapX = CanPos.TileMapX;
                GameState->PlayerTileMapY = CanPos.TileMapY;
                GameState->PlayerX = World.UpperLeftX + World.TileSideInPixels * CanPos.TileX + CanPos.TileRelX;
                GameState->PlayerY = World.UpperLeftY + World.TileSideInPixels * CanPos.TileY + CanPos.TileRelY;
                TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
            }
        }
    }

    for (int Row = 0; Row < World.TileCountY; Row++)
    {
        for (int Column = 0; Column < World.TileCountX; Column++)
        {
            uint32 TileID = GetTileValueUnchecked(&World, TileMap,Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
                Gray = 0.2f;

            real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileSideInPixels;
            real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY + World.TileSideInPixels;

            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
    real32 PlayerTop = GameState->PlayerY - PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
}
