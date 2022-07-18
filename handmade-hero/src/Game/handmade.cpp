#include "handmade.h"

#include "handmade_intrinsics.h"
#include "handmade_tile.h"

#include "handmade_tile.cpp"

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

    tile_map TileMap;
    TileMap.ChunkShift = 8;
    TileMap.ChunkMask = 0xFF;
    TileMap.ChunkDim = 256;

    TileMap.TileSideInMeters = 1.4f;
    TileMap.TileSideInPixels = 75;
    TileMap.MetersToPixels = (real32)TileMap.TileSideInPixels / (real32)TileMap.TileSideInMeters;

    TileMap.TileChunkCountX = 1;
    TileMap.TileChunkCountY = 1;

    real32 LowerLeftX = -(real32)TileMap.TileSideInPixels / 2.0f;
    real32 LowerLeftY = Buffer->Height;

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32*)Tiles;
    TileMap.TileChunks = &TileChunk;

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
            real32 PlayerSpeed = 2.0f;
            if (Controller->ActionUp.EndedDown)
                PlayerSpeed = 10.0f;

            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;

            tile_map_position NewPlayerPos = GameState->PlayerPos;
            NewPlayerPos.TileRelX += Input->dtForFrame * dPlayerX;
            NewPlayerPos.TileRelY += Input->dtForFrame * dPlayerY;
            NewPlayerPos = RecanonicalizePosition(&TileMap, NewPlayerPos);

            tile_map_position PlayerLeft = NewPlayerPos;
            PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&TileMap, PlayerLeft);
            tile_map_position PlayerRight = NewPlayerPos;
            PlayerRight.TileRelX += 0.5f * PlayerWidth;
            PlayerRight = RecanonicalizePosition(&TileMap, PlayerRight);


            if (
                IsWorldPointEmpty(&TileMap, NewPlayerPos) &&
                IsWorldPointEmpty(&TileMap, PlayerLeft) &&
                IsWorldPointEmpty(&TileMap, PlayerRight))
            { 
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

            uint32 TileID = GetTileValue(&TileMap, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
                Gray = 0.2f;

            if (Column == GameState->PlayerPos.AbsTileX && Row == GameState->PlayerPos.AbsTileY)
                Gray = 0.0f;

            real32 CenX = ScreenCenterX - TileMap.MetersToPixels * GameState->PlayerPos.TileRelX + ((real32)RelColumn) * TileMap.TileSideInPixels;
            real32 CenY = ScreenCenterY + TileMap.MetersToPixels * GameState->PlayerPos.TileRelY - ((real32)RelRow) * TileMap.TileSideInPixels;
            real32 MinX = CenX - 0.5f * TileMap.TileSideInPixels;
            real32 MinY = CenY - 0.5f * TileMap.TileSideInPixels;
            real32 MaxX = CenX + 0.5f * TileMap.TileSideInPixels;
            real32 MaxY = CenY + 0.5f * TileMap.TileSideInPixels;

            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;

    real32 PlayerLeft = ScreenCenterX - 0.5f * TileMap.MetersToPixels * PlayerWidth;
    real32 PlayerTop = ScreenCenterY - TileMap.MetersToPixels * PlayerHeight;

    DrawRectangle(Buffer, 
        PlayerLeft, 
        PlayerTop, 
        PlayerLeft + PlayerWidth * TileMap.MetersToPixels,
        PlayerTop + PlayerHeight * TileMap.MetersToPixels, 
        PlayerR, PlayerG, PlayerB);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
}
