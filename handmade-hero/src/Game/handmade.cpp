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

inline tile_chunk* GetTileChunk(world* World, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk* TileChunk = 0;
    if (TileChunkX >= 0 && TileChunkX < World->TileChunkCountX && TileChunkY >= 0 && TileChunkY < World->TileChunkCountY)
    {
        TileChunk = &World->TileChunks[TileChunkY * World->TileChunkCountX + TileChunkX];
    }

    return TileChunk;
}

inline uint32 GetTileValueUnchecked(world* World, tile_chunk* TileChunk, uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < World->ChunkDim);
    Assert(TileY < World->ChunkDim);

    return TileChunk->Tiles[TileY * World->ChunkDim + TileX];
}

internal uint32 GetTileValue(world* World, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;
    if (TileChunk)
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);

    return TileChunkValue;
}

inline void RecanonicalizeCoord(world* World, int32* Tile, real32* TileRel)
{
    int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset * World->TileSideInMeters;

    Assert(*Tile >= 0);
    Assert(*TileRel < World->TileSideInMeters);
}

internal world_position RecanonicalizePosition(world* World, world_position Pos)
{
    world_position Result = Pos;
    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

inline tile_chunk_position GetChunkPositionFor(world* World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;
    Result.TileChunkX = AbsTileX >> World->ChunkShift;
    Result.TileChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;

    return Result;
}

internal uint32 GetTileValue(world* World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
    tile_chunk* TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return TileChunkValue;
}

internal bool32 IsWorldPointEmpty(world* World, world_position CanPos)
{
    uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 0);

    return Empty;
}


GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerPos.AbsTileX = 3;
        GameState->PlayerPos.AbsTileY = 3;
        GameState->PlayerPos.TileRelX = 0.0f;
        GameState->PlayerPos.TileRelY = 0.0f;

        Memory->IsInitialized = true;
    }

#define TILEMAP_COUNT_X 256
#define TILEMAP_COUNT_Y 256
    uint32 Tiles[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 1, 0, 0,  0, 0, 0, 1,  1, 1, 0, 1,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  1, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 1, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    world World;
    World.ChunkShift = 8;
    World.ChunkMask = 0xFF;
    World.ChunkDim = 256;

    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 75;
    World.MetersToPixels = (real32)World.TileSideInPixels / (real32)World.TileSideInMeters;

    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;

    real32 LowerLeftX = -(real32)World.TileSideInPixels / 2.0f;
    real32 LowerLeftY = Buffer->Height;

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32*)Tiles;
    World.TileChunks = &TileChunk;

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

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

            dPlayerX *= 5.0f;
            dPlayerY *= 5.0f;
            world_position NewPlayerPos = GameState->PlayerPos;
            NewPlayerPos.TileRelX += Input->dtForFrame * dPlayerX;
            NewPlayerPos.TileRelY += Input->dtForFrame * dPlayerY;
            NewPlayerPos = RecanonicalizePosition(&World, NewPlayerPos);

            world_position PlayerLeft = NewPlayerPos;
            PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);
            world_position PlayerRight = NewPlayerPos;
            PlayerRight.TileRelX += 0.5f * PlayerWidth;
            PlayerRight = RecanonicalizePosition(&World, PlayerRight);


            if (
                IsWorldPointEmpty(&World, NewPlayerPos) &&
                IsWorldPointEmpty(&World, PlayerLeft) &&
                IsWorldPointEmpty(&World, PlayerRight))
            { 
                GameState->PlayerPos = NewPlayerPos;
            }
        }
    }

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 0.1f);

    for (uint32 Row = 0; Row < World.ChunkDim; Row++)
    {
        for (uint32 Column = 0; Column < World.ChunkDim; Column++)
        {
            uint32 TileID = GetTileValue(&World, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
                Gray = 0.2f;

            if (Column == GameState->PlayerPos.AbsTileX && Row == GameState->PlayerPos.AbsTileY)
                Gray = 0.0f;

            real32 MinX = LowerLeftX + ((real32)Column) * World.TileSideInPixels;
            real32 MinY = LowerLeftY - ((real32)Row) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY - World.TileSideInPixels;

            DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = LowerLeftX + GameState->PlayerPos.AbsTileX * World.TileSideInPixels +
        World.MetersToPixels * GameState->PlayerPos.TileRelX - 0.5f * World.MetersToPixels * PlayerWidth;
    real32 PlayerTop = LowerLeftY - GameState->PlayerPos.AbsTileY * World.TileSideInPixels -
        World.MetersToPixels * GameState->PlayerPos.TileRelY - World.MetersToPixels * PlayerHeight;

    DrawRectangle(Buffer, 
        PlayerLeft, 
        PlayerTop, 
        PlayerLeft + PlayerWidth * World.MetersToPixels,
        PlayerTop + PlayerHeight * World.MetersToPixels, 
        PlayerR, PlayerG, PlayerB);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
}
