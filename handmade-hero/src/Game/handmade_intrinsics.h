#ifndef HANDMADE_INTRINSICS_H

#include "handmade.h"

#include <math.h>

// TODO: HOW TO IMPLEMENT THESE MATH FUNCTIONS!!!

inline int32 RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    // TODO(casey): Intrinsic????
    return Result;
}

inline uint32 RoundReal32ToUInt32(real32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    // TODO(casey): Intrinsic????
    return Result;
}

inline int32 TruncateReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)(Real32);
    return Result;
}

inline uint32 AbsoluteInt32ToUInt32(int32 Int32)
{

}
inline int32 FloorReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)floorf(Real32);
    return Result;
}

inline real32 Sin(real32 Angle)
{
    return sinf(Angle);
}

inline real32 Cos(real32 Angle)
{
    return cosf(Angle);
}

inline real32 ATan2(real32 Y, real32 X)
{
    return atan2f(Y, X);
}

#define HANDMADE_INTRINSICS_H
#endif