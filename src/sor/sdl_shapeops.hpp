#pragma once

#include <SDL3/SDL_rect.h>

namespace JanSordid::SDL
{
	// Care: I've really gone a bit to wild in here, some of those operators will be removed in the near future

	constexpr const SDL_Rect  * EntireRect  = nullptr;
	constexpr const SDL_FRect * EntireFRect = nullptr;

	/*struct Index : SDL_Point
	{

	};
	struct Vec2 : SDL_FPoint
	{

	};
	struct Size
	{
		float w;
		float h;
	};*/

	// Define Y_IS_UP if you want to use +Y as up (by default +Y means down, as SDL uses it),
	// but beware you need to do more than just this to make it work
	namespace PointLiterals
	{
#ifdef Y_IS_UP
		constexpr SDL_Point   operator"" _up    (unsigned long long n)  { return { 0,  (int)n }; }
		constexpr SDL_Point   operator"" _down  (unsigned long long n)  { return { 0, -(int)n }; }
		constexpr SDL_FPoint  operator"" _up    (long double n)         { return { 0.0f,  (float)n }; }
		constexpr SDL_FPoint  operator"" _down  (long double n)         { return { 0.0f, -(float)n }; }
#else
		constexpr SDL_Point   operator"" _up   ( unsigned long long n )  { return { 0, -(int)n }; }
		constexpr SDL_Point   operator"" _down ( unsigned long long n )  { return { 0,  (int)n }; }
		constexpr SDL_FPoint  operator"" _up   ( long double n )         { return { 0.0f, -(float)n }; }
		constexpr SDL_FPoint  operator"" _down ( long double n )         { return { 0.0f,  (float)n }; }
#endif
		constexpr SDL_Point   operator"" _left ( unsigned long long n )  { return { -(int)n, 0 }; }
		constexpr SDL_Point   operator"" _right( unsigned long long n )  { return {  (int)n, 0 }; }
		constexpr SDL_FPoint  operator"" _left ( long double n )         { return { -(float)n, 0.0f }; }
		constexpr SDL_FPoint  operator"" _right( long double n )         { return {  (float)n, 0.0f }; }
	}


	/// Point <-> Point => Point
	constexpr SDL_Point    operator+ ( const SDL_Point lhs, const SDL_Point rhs ) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
	constexpr SDL_Point    operator- ( const SDL_Point lhs, const SDL_Point rhs ) { return { lhs.x - rhs.x, lhs.y - rhs.y }; }
	constexpr SDL_Point    operator* ( const SDL_Point lhs, const SDL_Point rhs ) { return { lhs.x * rhs.x, lhs.y * rhs.y }; }
	constexpr SDL_Point    operator/ ( const SDL_Point lhs, const SDL_Point rhs ) { return { lhs.x / rhs.x, lhs.y / rhs.y }; }
	constexpr SDL_Point &  operator+=( SDL_Point & lhs, const SDL_Point rhs ) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_Point &  operator-=( SDL_Point & lhs, const SDL_Point rhs ) { lhs = lhs - rhs; return lhs; }
	constexpr SDL_Point &  operator*=( SDL_Point & lhs, const SDL_Point rhs ) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_Point &  operator/=( SDL_Point & lhs, const SDL_Point rhs ) { lhs = lhs / rhs; return lhs; }

	/// Point <-> Scalar => Point
	constexpr SDL_Point    operator* ( const SDL_Point lhs, const int rhs ) { return { lhs.x * rhs, lhs.y * rhs }; }
	constexpr SDL_Point    operator/ ( const SDL_Point lhs, const int rhs ) { return { lhs.x / rhs, lhs.y / rhs }; }
	constexpr SDL_Point &  operator*=( SDL_Point & lhs, const int rhs ) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_Point &  operator/=( SDL_Point & lhs, const int rhs ) { lhs = lhs / rhs; return lhs; }

	/// Rect <-> Rect => Rect
	constexpr SDL_Rect     operator+ ( const SDL_Rect lhs, const SDL_Rect rhs ) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.w + rhs.w, lhs.h + rhs.h }; }
	constexpr SDL_Rect     operator- ( const SDL_Rect lhs, const SDL_Rect rhs ) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.w - rhs.w, lhs.h - rhs.h }; }
	constexpr SDL_Rect &   operator+=( SDL_Rect & lhs, const SDL_Rect rhs ) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_Rect &   operator-=( SDL_Rect & lhs, const SDL_Rect rhs ) { lhs = lhs - rhs; return lhs; }


	/// FPoint <-> FPoint => FPoint
	constexpr SDL_FPoint   operator+ ( const SDL_FPoint lhs, const SDL_FPoint rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
	constexpr SDL_FPoint   operator- ( const SDL_FPoint lhs, const SDL_FPoint rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y }; }
	constexpr SDL_FPoint   operator* ( const SDL_FPoint lhs, const SDL_FPoint rhs) { return { lhs.x * rhs.x, lhs.y * rhs.y }; }
	constexpr SDL_FPoint   operator/ ( const SDL_FPoint lhs, const SDL_FPoint rhs) { return { lhs.x / rhs.x, lhs.y / rhs.y }; }
	constexpr SDL_FPoint & operator+=( SDL_FPoint & lhs, const SDL_FPoint rhs) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_FPoint & operator-=( SDL_FPoint & lhs, const SDL_FPoint rhs) { lhs = lhs - rhs; return lhs; }
	constexpr SDL_FPoint & operator*=( SDL_FPoint & lhs, const SDL_FPoint rhs) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_FPoint & operator/=( SDL_FPoint & lhs, const SDL_FPoint rhs) { lhs = lhs / rhs; return lhs; }

	/// FPoint <-> FScalar => FPoint
	constexpr SDL_FPoint   operator* ( const SDL_FPoint lhs, const float rhs) { return { lhs.x * rhs, lhs.y * rhs }; }
	constexpr SDL_FPoint   operator/ ( const SDL_FPoint lhs, const float rhs) { return { lhs.x / rhs, lhs.y / rhs }; }
	constexpr SDL_FPoint & operator*=( SDL_FPoint & lhs, const float rhs) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_FPoint & operator/=( SDL_FPoint & lhs, const float rhs) { lhs = lhs / rhs; return lhs; }

	/// FRect <-> FRect
	constexpr SDL_FRect    operator+ ( const SDL_FRect lhs, const SDL_FRect rhs ) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.w + rhs.w, lhs.h + rhs.h }; }
	constexpr SDL_FRect    operator- ( const SDL_FRect lhs, const SDL_FRect rhs ) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.w - rhs.w, lhs.h - rhs.h }; }
	constexpr SDL_FRect    operator* ( const SDL_FRect lhs, const SDL_FRect rhs ) { return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.w * rhs.w, lhs.h * rhs.h }; }
	constexpr SDL_FRect    operator/ ( const SDL_FRect lhs, const SDL_FRect rhs ) { return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.w / rhs.w, lhs.h / rhs.h }; }
	constexpr SDL_FRect &  operator+=( SDL_FRect & lhs, const SDL_FRect rhs ) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_FRect &  operator-=( SDL_FRect & lhs, const SDL_FRect rhs ) { lhs = lhs - rhs; return lhs; }
	constexpr SDL_FRect &  operator*=( SDL_FRect & lhs, const SDL_FRect rhs ) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_FRect &  operator/=( SDL_FRect & lhs, const SDL_FRect rhs ) { lhs = lhs / rhs; return lhs; }


	// TODO: All of the Operators below need to be revisited and evaluated if they do what one might expect
	/// Point <-> FScalar => FPoint
	constexpr SDL_FPoint  operator*  (const SDL_Point lhs, const float rhs) { return { lhs.x * rhs, lhs.y * rhs }; }
	constexpr SDL_FPoint  operator/  (const SDL_Point lhs, const float rhs) { return { lhs.x / rhs, lhs.y / rhs }; }

	/// Point <-> Rect => Rect
	constexpr SDL_Rect    operator+  (const SDL_Rect lhs, const SDL_Point rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_Rect    operator-  (const SDL_Rect lhs, const SDL_Point rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_Rect    operator*  (const SDL_Rect lhs, const SDL_Point rhs) { return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_Rect    operator/  (const SDL_Rect lhs, const SDL_Point rhs) { return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_Rect &  operator+= (SDL_Rect & lhs, const SDL_Point rhs) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_Rect &  operator-= (SDL_Rect & lhs, const SDL_Point rhs) { lhs = lhs - rhs; return lhs; }
	constexpr SDL_Rect &  operator*= (SDL_Rect & lhs, const SDL_Point rhs) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_Rect &  operator/= (SDL_Rect & lhs, const SDL_Point rhs) { lhs = lhs / rhs; return lhs; }

//	constexpr SDL_Rect    operator+  (const SDL_Point lhs, const SDL_Rect rhs) { return rhs + lhs; }
//	constexpr SDL_Rect    operator*  (const SDL_Point lhs, const SDL_Rect rhs) { return rhs * lhs; }

	/// Rect <-> Scalar => Rect
	constexpr SDL_Rect    operator*  (const SDL_Rect lhs, const int rhs) { return { lhs.x, lhs.y, lhs.w * rhs, lhs.h * rhs }; }
	constexpr SDL_Rect    operator/  (const SDL_Rect lhs, const int rhs) { return { lhs.x, lhs.y, lhs.w / rhs, lhs.h / rhs }; }
	constexpr SDL_Rect &  operator*= (SDL_Rect & lhs, const int rhs) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_Rect &  operator/= (SDL_Rect & lhs, const int rhs) { lhs = lhs / rhs; return lhs; }

	/// FPoint <-> FRect => FRect
	constexpr SDL_FRect   operator+  (const SDL_FRect lhs, const SDL_FPoint rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_FRect   operator-  (const SDL_FRect lhs, const SDL_FPoint rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.w, lhs.h }; }
	constexpr SDL_FRect & operator+= (SDL_FRect & lhs, const SDL_FPoint rhs) { lhs = lhs + rhs; return lhs; }
	constexpr SDL_FRect & operator-= (SDL_FRect & lhs, const SDL_FPoint rhs) { lhs = lhs - rhs; return lhs; }

	constexpr SDL_FRect   operator+  (const SDL_FPoint lhs, const SDL_FRect rhs) { return rhs + lhs; }

	/// Rect <-> Scalar
	constexpr SDL_FRect   operator*  (const SDL_FRect lhs, const float rhs) { return { lhs.x, lhs.y, lhs.w * rhs, lhs.h * rhs }; }
	constexpr SDL_FRect   operator/  (const SDL_FRect lhs, const float rhs) { return { lhs.x, lhs.y, lhs.w / rhs, lhs.h / rhs }; }
	constexpr SDL_FRect&  operator*= (SDL_FRect& lhs, const float rhs) { lhs = lhs * rhs; return lhs; }
	constexpr SDL_FRect&  operator/= (SDL_FRect& lhs, const float rhs) { lhs = lhs / rhs; return lhs; }


	// Conversion F -> I, I -> F
	constexpr  Point toI( const FPoint n ) { return  Point { (int)   n.x, (int)   n.y }; }
	constexpr FPoint toF( const  Point n ) { return FPoint { (float) n.x, (float) n.y }; }
	constexpr  Rect  toI( const FRect  n ) { return  Rect  { (int)   n.x, (int)   n.y, (int)   n.w, (int)   n.h }; }
	constexpr FRect  toF( const  Rect  n ) { return FRect  { (float) n.x, (float) n.y, (float) n.w, (float) n.h }; }

	// Spreading out data
	constexpr  Rect toXY( const  Point n, const int   r = 0 ) { return  Rect { n.x, n.y, r,   r   }; }
	constexpr FRect toXY( const FPoint n, const float r = 0 ) { return FRect { n.x, n.y, r,   r   }; }
	constexpr  Rect toWH( const  Point n, const int   r = 0 ) { return  Rect { r,   r,   n.x, n.y }; }
	constexpr FRect toWH( const FPoint n, const float r = 0 ) { return FRect { r,   r,   n.x, n.y }; }
	constexpr  Rect toXYWH( const  Point n ) { return  Rect { n.x, n.y, n.x, n.y }; }
	constexpr FRect toXYWH( const FPoint n ) { return FRect { n.x, n.y, n.x, n.y }; }
	constexpr  Rect toRect(  const Point  xy, const Point  wh ) { return  Rect { xy.x, xy.y, wh.x, wh.y }; }
	constexpr FRect toFRect( const FPoint xy, const FPoint wh ) { return FRect { xy.x, xy.y, wh.x, wh.y }; }

	inline /*constexpr*/ int    calcLengthSq( const Point  n ) { return n.x * n.x + n.y * n.y; }
	inline /*constexpr*/ f32    calcLength(   const Point  n ) { return sqrt( calcLengthSq( n ) ); } // Care: Returns f32 as sqrt(int) will very likely not return an int
	inline /*constexpr*/ FPoint normalize(    const Point  n ) { return n / calcLength( n ); }       // Care: Returns FPoint

	inline /*constexpr*/ f32    calcLengthSq( const FPoint n ) { return n.x * n.x + n.y * n.y; }
	inline /*constexpr*/ f32    calcLength(   const FPoint n ) { return sqrt( calcLengthSq( n ) ); }
	inline /*constexpr*/ FPoint normalize(    const FPoint n ) { return n / calcLength( n ); }

	inline bool hasIntersection( const Rect  && lhs, const Rect  && rhs ) { return SDL_HasRectIntersection( &lhs, &rhs ); }
	inline bool hasIntersection( const FRect && lhs, const FRect && rhs ) { return SDL_HasRectIntersectionFloat( &lhs, &rhs ); }
}
