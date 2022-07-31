#ifndef HANDMADE_MATH
#include "handmade_platform.h"

union v2
{
	struct
	{
		real32 X, Y;
	};
	real32 E[2];
};

inline v2 V2(real32 X, real32 Y)
{
	v2 Result;
	Result.X = X;
	Result.Y = Y;
	return Result;
}

inline v2 operator*(real32 A, v2 B)
{
	v2 Result;
	Result.X = A * B.X;
	Result.Y = A * B.Y;
	return Result;
}

inline v2& operator*=(v2& A, real32 B)
{
	A = B * A;
	return A;
}

inline v2 operator*(v2 A, real32 B)
{
	v2 Result;
	Result.X = B * A.X;
	Result.Y = B * A.Y;
	return Result;
}

inline v2 operator+(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	return Result;
}

inline v2& operator+=(v2& A, v2 B)
{
	A = A + B;
	return A;
}

inline v2 operator-(v2 A)
{
	v2 Result;
	Result.X = -A.X;
	Result.Y = -A.Y;
	return Result;
}

inline v2 operator-(v2 A, v2 B)
{
	v2 Result;
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	return Result;
}

inline v2& operator-=(v2& A, v2& B)
{
	A = A - B;
	return A;
}

inline real32 Inner(v2 A, v2 B)
{
	return A.X * B.X + A.Y * B.Y;
}

inline real32 LengthSq(v2 A)
{
	real32 Result = Inner(A, A);
	return Result;
}

#define HANDMADE_MATH
#endif