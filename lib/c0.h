#ifndef C0_H
#define C0_H

#include <stdint.h>

#define FORCE_INLINE __attribute__((always_inline)) inline

typedef signed char i8;
typedef char u8;
/* typedef unsigned char u8; */
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;
typedef __int128_t i128;
typedef __uint128_t u128;
typedef unsigned short f16;
typedef float f32;
typedef double f64;
typedef void* ptr;
typedef int bool;


/* --- Read --- */

FORCE_INLINE i8 rdi8(i8 *src) { return *src; }
FORCE_INLINE i16 rdi16(i16 *src) { return *src; }
FORCE_INLINE i32 rdi32(i32 *src) { return *src; }
FORCE_INLINE i64 rdi64(i64 *src) { return *src; }
FORCE_INLINE i128 rdi128(i128 *src) { return *src; }
FORCE_INLINE u8 rdu8(u8 *src) { return *src; }
FORCE_INLINE u16 rdu16(u16 *src) { return *src; }
FORCE_INLINE u32 rdu32(u32 *src) { return *src; }
FORCE_INLINE u64 rdu64(u64 *src) { return *src; }
FORCE_INLINE u128 rdu128(u128 *src) { return *src; }


/* --- Write --- */

FORCE_INLINE void wri8(i8 *dst, i8 src) { *dst = src; }
FORCE_INLINE void wri16(i16 *dst, i16 src) { *dst = src; }
FORCE_INLINE void wri32(i32 *dst, i32 src) { *dst = src; }
FORCE_INLINE void wri64(i64 *dst, i64 src) { *dst = src; }
FORCE_INLINE void wri128(i128 *dst, i128 src) { *dst = src; }
FORCE_INLINE void wru8(u8 *dst, u8 src) { *dst = src; }
FORCE_INLINE void wru16(u16 *dst, u16 src) { *dst = src; }
FORCE_INLINE void wru32(u32 *dst, u32 src) { *dst = src; }
FORCE_INLINE void wru64(u64 *dst, u64 src) { *dst = src; }
FORCE_INLINE void wru128(u128 *dst, u128 src) { *dst = src; }


/* --- Addition --- */

FORCE_INLINE u8 addu8(u8 a, u8 b) { return a + b; }
FORCE_INLINE u16 addu16(u16 a, u16 b) { return a + b; }
FORCE_INLINE u32 addu32(u32 a, u32 b) { return a + b; }
FORCE_INLINE u64 addu64(u64 a, u64 b) { return a + b; }
FORCE_INLINE u128 addu128(u128 a, u128 b) { return a + b; }

FORCE_INLINE i8
addi8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a + b;
	return (i8)s;
}

FORCE_INLINE i16
addi16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a + b;
	return (i16)s;
}

FORCE_INLINE i32
addi32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a + b;
	return (i32)s;
}

FORCE_INLINE i64
addi64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a + b;
	return (i64)s;
}


/* --- Subtraction --- */

FORCE_INLINE u8 subu8(u8 a, u8 b) { return a - b; }
FORCE_INLINE u16 subu16(u16 a, u16 b) { return a - b; }
FORCE_INLINE u32 subu32(u32 a, u32 b) { return a - b; }
FORCE_INLINE u64 subu64(u64 a, u64 b) { return a - b; }
FORCE_INLINE u128 subu128(u128 a, u128 b) { return a - b; }

FORCE_INLINE i8
subi8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a - b;
	return (i8)s;
}

FORCE_INLINE i16
subi16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a - b;
	return (i16)s;
}

FORCE_INLINE i32
subi32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a - b;
	return (i32)s;
}

FORCE_INLINE i64
subi64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a - b;
	return (i64)s;
}

FORCE_INLINE i64
subi128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a - b;
	return (i128)s;
}


/* --- Multiplication --- */

FORCE_INLINE u8 mulu8(u8 a, u8 b) { return a * b; }
FORCE_INLINE u16 mulu16(u16 a, u16 b) { return a * b; }
FORCE_INLINE u32 mulu32(u32 a, u32 b) { return a * b; }
FORCE_INLINE u64 mulu64(u64 a, u64 b) { return a * b; }
FORCE_INLINE u128 mulu128(u128 a, u128 b) { return a * b; }

FORCE_INLINE i8
muli8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a * b;
	return (i8)s;
}

FORCE_INLINE i16
muli16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a * b;
	return (i16)s;
}

FORCE_INLINE i32
muli32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a * b;
	return (i32)s;
}

FORCE_INLINE i64
muli64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a * b;
	return (i64)s;
}

FORCE_INLINE i128
muli128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a * b;
	return (i128)s;
}


/* --- Quotient --- */

FORCE_INLINE u8 quou8(u8 a, u8 b) { return a / b; }
FORCE_INLINE u16 quou16(u16 a, u16 b) { return a / b; }
FORCE_INLINE u32 quou32(u32 a, u32 b) { return a / b; }
FORCE_INLINE u64 quou64(u64 a, u64 b) { return a / b; }
FORCE_INLINE u128 quou128(u128 a, u128 b) { return a / b; }

FORCE_INLINE i8
quoi8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a / b;
	return (i8)s;
}

FORCE_INLINE i16
quoi16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a / b;
	return (i16)s;
}

FORCE_INLINE i32
quoi32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a / b;
	return (i32)s;
}

FORCE_INLINE i64
quoi64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a / b;
	return (i64)s;
}

FORCE_INLINE i128
quoi128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a / b;
	return (i128)s;
}


/* --- Remainder --- */

FORCE_INLINE u8 remu8(u8 a, u8 b) { return a % b; }
FORCE_INLINE u16 remu16(u16 a, u16 b) { return a % b; }
FORCE_INLINE u32 remu32(u32 a, u32 b) { return a % b; }
FORCE_INLINE u64 remu64(u64 a, u64 b) { return a % b; }
FORCE_INLINE u128 remu128(u128 a, u128 b) { return a % b; }

FORCE_INLINE i8
remi8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a % b;
	return (i8)s;
}

FORCE_INLINE i16
remi16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a % b;
	return (i16)s;
}

FORCE_INLINE i32
remi32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a % b;
	return (i32)s;
}

FORCE_INLINE i64
remi64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a % b;
	return (i64)s;
}

FORCE_INLINE i128
remi128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a % b;
	return (i128)s;
}


/* --- Left shift --- */

FORCE_INLINE u8 shlu8(u8 a, u8 b) { return a << b; }
FORCE_INLINE u16 shlu16(u16 a, u16 b) { return a << b; }
FORCE_INLINE u32 shlu32(u32 a, u32 b) { return a << b; }
FORCE_INLINE u64 shlu64(u64 a, u64 b) { return a << b; }
FORCE_INLINE u128 shlu128(u128 a, u128 b) { return a << b; }

FORCE_INLINE i8
shli8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a << b;
	return (i8)s;
}

FORCE_INLINE i16
shli16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a << b;
	return (i16)s;
}

FORCE_INLINE i32
shli32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a << b;
	return (i32)s;
}

FORCE_INLINE i64
shli64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a << b;
	return (i64)s;
}

FORCE_INLINE i128
shli128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a << b;
	return (i128)s;
}


/* --- Right shift --- */

FORCE_INLINE u8 shru8(u8 a, u8 b) { return a >> b; }
FORCE_INLINE u16 shru16(u16 a, u16 b) { return a >> b; }
FORCE_INLINE u32 shru32(u32 a, u32 b) { return a >> b; }
FORCE_INLINE u64 shru64(u64 a, u64 b) { return a >> b; }
FORCE_INLINE u128 shru128(u128 a, u128 b) { return a >> b; }

FORCE_INLINE i8
shri8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a >> b;
	return (i8)s;
}

FORCE_INLINE i16
shri16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a >> b;
	return (i16)s;
}

FORCE_INLINE i32
shri32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a >> b;
	return (i32)s;
}

FORCE_INLINE i64
shri64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a >> b;
	return (i64)s;
}

FORCE_INLINE i128
shri128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a >> b;
	return (i128)s;
}

/* --- Bitwise and --- */

FORCE_INLINE u8 andu8(u8 a, u8 b) { return a && b; }
FORCE_INLINE u16 andu16(u16 a, u16 b) { return a && b; }
FORCE_INLINE u32 andu32(u32 a, u32 b) { return a && b; }
FORCE_INLINE u64 andu64(u64 a, u64 b) { return a && b; }
FORCE_INLINE u128 andu128(u128 a, u128 b) { return a && b; }

FORCE_INLINE i8
andi8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a && b;
	return (i8)s;
}

FORCE_INLINE i16
andi16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a && b;
	return (i16)s;
}

FORCE_INLINE i32
andi32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a && b;
	return (i32)s;
}

FORCE_INLINE i64
andi64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a && b;
	return (i64)s;
}

FORCE_INLINE i128
andi128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a && b;
	return (i128)s;
}


/* --- Bitwise or --- */

FORCE_INLINE u8 oru8(u8 a, u8 b) { return a | b; }
FORCE_INLINE u16 oru16(u16 a, u16 b) { return a | b; }
FORCE_INLINE u32 oru32(u32 a, u32 b) { return a | b; }
FORCE_INLINE u64 oru64(u64 a, u64 b) { return a | b; }
FORCE_INLINE u128 oru128(u128 a, u128 b) { return a | b; }

FORCE_INLINE i8
ori8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a | b;
	return (i8)s;
}

FORCE_INLINE i16
ori16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a | b;
	return (i16)s;
}

FORCE_INLINE i32
ori32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a | b;
	return (i32)s;
}

FORCE_INLINE i64
ori64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a | b;
	return (i64)s;
}

FORCE_INLINE i128
ori128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a | b;
	return (i128)s;
}


/* --- Bitwise xor --- */

FORCE_INLINE u8 xoru8(u8 a, u8 b) { return a ^ b; }
FORCE_INLINE u16 xoru16(u16 a, u16 b) { return a ^ b; }
FORCE_INLINE u32 xoru32(u32 a, u32 b) { return a ^ b; }
FORCE_INLINE u64 xoru64(u64 a, u64 b) { return a ^ b; }
FORCE_INLINE u128 xoru128(u128 a, u128 b) { return a ^ b; }

FORCE_INLINE i8
xori8(i8 lhs, i8 rhs)
{
	const u8 a = lhs;
	const u8 b = rhs;
	const u8 s = a ^ b;
	return (i8)s;
}

FORCE_INLINE i16
xori16(i16 lhs, i16 rhs)
{
	const u16 a = lhs;
	const u16 b = rhs;
	const u16 s = a ^ b;
	return (i16)s;
}

FORCE_INLINE i32
xori32(i32 lhs, i32 rhs)
{
	const u32 a = lhs;
	const u32 b = rhs;
	const u32 s = a ^ b;
	return (i32)s;
}

FORCE_INLINE i64
xori64(i64 lhs, i64 rhs)
{
	const u64 a = lhs;
	const u64 b = rhs;
	const u64 s = a ^ b;
	return (i64)s;
}

FORCE_INLINE i128
xori128(i128 lhs, i128 rhs)
{
	const u128 a = lhs;
	const u128 b = rhs;
	const u128 s = a ^ b;
	return (i128)s;
}


/* --- Equal --- */

FORCE_INLINE bool eqi8(i8 a, i8 b) { return a == b; }
FORCE_INLINE bool eqi16(i16 a, i16 b) { return a == b; }
FORCE_INLINE bool eqi32(i32 a, i32 b) { return a == b; }
FORCE_INLINE bool eqi64(i64 a, i64 b) { return a == b; }
FORCE_INLINE bool eqi128(i128 a, i128 b) { return a == b; }
FORCE_INLINE bool equ8(u8 a, u8 b) { return a == b; }
FORCE_INLINE bool equ16(u16 a, u16 b) { return a == b; }
FORCE_INLINE bool equ32(u32 a, u32 b) { return a == b; }
FORCE_INLINE bool equ64(u64 a, u64 b) { return a == b; }
FORCE_INLINE bool equ128(u128 a, u128 b) { return a == b; }


/* --- Not-equal --- */

FORCE_INLINE bool neqi8(i8 a, i8 b) { return a != b; }
FORCE_INLINE bool neqi16(i16 a, i16 b) { return a != b; }
FORCE_INLINE bool neqi32(i32 a, i32 b) { return a != b; }
FORCE_INLINE bool neqi64(i64 a, i64 b) { return a != b; }
FORCE_INLINE bool neqi128(i128 a, i128 b) { return a != b; }
FORCE_INLINE bool nequ8(u8 a, u8 b) { return a != b; }
FORCE_INLINE bool nequ16(u16 a, u16 b) { return a != b; }
FORCE_INLINE bool nequ32(u32 a, u32 b) { return a != b; }
FORCE_INLINE bool nequ64(u64 a, u64 b) { return a != b; }
FORCE_INLINE bool nequ128(u128 a, u128 b) { return a != b; }


/* --- Less-than --- */

FORCE_INLINE bool lti8(i8 a, i8 b) { return a < b; }
FORCE_INLINE bool lti16(i16 a, i16 b) { return a < b; }
FORCE_INLINE bool lti32(i32 a, i32 b) { return a < b; }
FORCE_INLINE bool lti64(i64 a, i64 b) { return a < b; }
FORCE_INLINE bool lti128(i128 a, i128 b) { return a < b; }
FORCE_INLINE bool ltu8(u8 a, u8 b) { return a < b; }
FORCE_INLINE bool ltu16(u16 a, u16 b) { return a < b; }
FORCE_INLINE bool ltu32(u32 a, u32 b) { return a < b; }
FORCE_INLINE bool ltu64(u64 a, u64 b) { return a < b; }
FORCE_INLINE bool ltu128(u128 a, u128 b) { return a < b; }


/* --- Greater-than --- */

FORCE_INLINE bool gti8(i8 a, i8 b) { return a > b; }
FORCE_INLINE bool gti16(i16 a, i16 b) { return a > b; }
FORCE_INLINE bool gti32(i32 a, i32 b) { return a > b; }
FORCE_INLINE bool gti64(i64 a, i64 b) { return a > b; }
FORCE_INLINE bool gti128(i128 a, i128 b) { return a > b; }
FORCE_INLINE bool gtu8(u8 a, u8 b) { return a > b; }
FORCE_INLINE bool gtu16(u16 a, u16 b) { return a > b; }
FORCE_INLINE bool gtu32(u32 a, u32 b) { return a > b; }
FORCE_INLINE bool gtu64(u64 a, u64 b) { return a > b; }
FORCE_INLINE bool gtu128(u128 a, u128 b) { return a > b; }


/* --- Less-than-equal --- */

FORCE_INLINE bool lteqi8(i8 a, i8 b) { return a <= b; }
FORCE_INLINE bool lteqi16(i16 a, i16 b) { return a <= b; }
FORCE_INLINE bool lteqi32(i32 a, i32 b) { return a <= b; }
FORCE_INLINE bool lteqi64(i64 a, i64 b) { return a <= b; }
FORCE_INLINE bool lteqi128(i128 a, i128 b) { return a <= b; }
FORCE_INLINE bool ltequ8(u8 a, u8 b) { return a <= b; }
FORCE_INLINE bool ltequ16(u16 a, u16 b) { return a <= b; }
FORCE_INLINE bool ltequ32(u32 a, u32 b) { return a <= b; }
FORCE_INLINE bool ltequ64(u64 a, u64 b) { return a <= b; }
FORCE_INLINE bool ltequ128(u128 a, u128 b) { return a <= b; }


/* --- Greater-than-equal --- */

FORCE_INLINE bool gteqi8(i8 a, i8 b) { return a >= b; }
FORCE_INLINE bool gteqi16(i16 a, i16 b) { return a >= b; }
FORCE_INLINE bool gteqi32(i32 a, i32 b) { return a >= b; }
FORCE_INLINE bool gteqi64(i64 a, i64 b) { return a >= b; }
FORCE_INLINE bool gteqi128(i128 a, i128 b) { return a >= b; }
FORCE_INLINE bool gtequ8(u8 a, u8 b) { return a >= b; }
FORCE_INLINE bool gtequ16(u16 a, u16 b) { return a >= b; }
FORCE_INLINE bool gtequ32(u32 a, u32 b) { return a >= b; }
FORCE_INLINE bool gtequ64(u64 a, u64 b) { return a >= b; }
FORCE_INLINE bool gtequ128(u128 a, u128 b) { return a >= b; }


/* --- Float Addition --- */

FORCE_INLINE f64 addf64(f64 a, f64 b) { return a + b; }

FORCE_INLINE f32
addf32(f32 lhs, f32 rhs)
{
	const f64 a = lhs;
	const f64 b = rhs;
	const f64 s = a + b;
	return (f32)s;
}


/* --- Float Subtraction --- */

FORCE_INLINE f64 subf64(f64 a, f64 b) { return a - b; }

FORCE_INLINE f32
subf32(f32 lhs, f32 rhs)
{
	const f64 a = lhs;
	const f64 b = rhs;
	const f64 s = a - b;
	return (f32)s;
}


/* --- Float Multiplication --- */

FORCE_INLINE f64 mulf64(f64 a, f64 b) { return a * b; }

FORCE_INLINE f32
mulf32(f32 lhs, f32 rhs)
{
	const f64 a = lhs;
	const f64 b = rhs;
	const f64 s = a * b;
	return (f32)s;
}


/* --- Float Division --- */

FORCE_INLINE f64 divf64(f64 a, f64 b) { return a / b; }

FORCE_INLINE f32
divf32(f32 lhs, f32 rhs)
{
	const f64 a = lhs;
	const f64 b = rhs;
	const f64 s = a / b;
	return (f32)s;
}


/* --- Float Equal --- */
FORCE_INLINE bool eqf32(f32 a, f32 b) { return a == b; }
FORCE_INLINE bool eqf64(f64 a, f64 b) { return a == b; }

/* --- Float Not-equal --- */
FORCE_INLINE bool neqf32(f32 a, f32 b) { return a != b; }
FORCE_INLINE bool neqf64(f64 a, f64 b) { return a != b; }

/* --- Float Less-than --- */
FORCE_INLINE bool ltf32(f32 a, f32 b) { return a < b; }
FORCE_INLINE bool ltf64(f64 a, f64 b) { return a < b; }

/* --- Float Greater-than --- */
FORCE_INLINE bool gtf32(f32 a, f32 b) { return a > b; }
FORCE_INLINE bool gtf64(f64 a, f64 b) { return a > b; }

/* --- Float Less-than-equal --- */
FORCE_INLINE bool lteqf32(f32 a, f32 b) { return a <= b; }
FORCE_INLINE bool lteqf64(f64 a, f64 b) { return a <= b; }

/* --- Float Greater-than-equal --- */
FORCE_INLINE bool gteqf32(f32 a, f32 b) { return a >= b; }
FORCE_INLINE bool gteqf64(f64 a, f64 b) { return a >= b; }

#endif
