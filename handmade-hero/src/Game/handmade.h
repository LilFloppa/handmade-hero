#ifndef HANDMADE_H

#include "handmade_platform.h"

#include "handmade_tile.h"


#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))
struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;
};

internal void InitializeArena(memory_arena* Arena, memory_index Size, uint8* Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count) * sizeof(type))
internal void* PushSize_(memory_arena* Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

struct world
{
    tile_map* TileMap;
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32* Pixels;
};

struct hero_bitmaps
{
    int32 AlignX;
    int32 AlignY;
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct entity
{
    tile_map_position Pos;
    v2 dPos;
    bool32 Exists;
    uint32 FacingDirection;
    real32 Width, Height;
};

struct game_state
{
    memory_arena WorldArena;
    world* World;

    uint32 CameraFollowingEntityIndex;
    tile_map_position CameraPos;

    uint32 EntityCount;
    uint32 PlayerIndexForController[ArrayCount(((game_input*)0)->Controllers)];
    entity Entities[256];

    loaded_bitmap Backdrop;
    hero_bitmaps HeroBitmaps[4];
};

#define HANDMADE_H
#endif
