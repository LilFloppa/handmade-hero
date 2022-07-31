#ifndef HANDMADE_TILE_H
#include "handmade_math.h"

// TODO: Replace this with a v3 once we get to v3
struct tile_map_difference
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_position
{
    // NOTE: These are fixes point tile locations. The high bits are the tile chunk index,
    // and the low bits are the tile index in the chunk.
    int32 AbsTileX;
    int32 AbsTileY;
    int32 AbsTileZ;

    // NOTE: These are the offsets from the tile center
    v2 Offset;
};

struct tile_chunk
{
    uint32* Tiles;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;

    // TODO: Real sparseness;
    int32 TileChunkCountX;
    int32 TileChunkCountY;
    int32 TileChunkCountZ;

    tile_chunk* TileChunks;
};

#define HANDMADE_TILE_H
#endif
