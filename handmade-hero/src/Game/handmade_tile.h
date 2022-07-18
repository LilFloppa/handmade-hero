#ifndef HANDMADE_TILE_H

struct tile_map_position
{
    // NOTE: These are fixes point tile locations. The high bits are the tile chunk index,
    // and the low bits are the tile index in the chunk.
    int32 AbsTileX;
    int32 AbsTileY;

    real32 TileRelX;
    real32 TileRelY;
};

struct tile_chunk
{
    uint32* Tiles;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    int32 TileChunkCountX;
    int32 TileChunkCountY;

    tile_chunk* TileChunks;
};

#define HANDMADE_TILE_H
#endif
