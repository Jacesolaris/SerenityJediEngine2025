/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2025 contributors

This file is part of the SerenityJediEngine2025 source code.

SerenityJediEngine2025 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "q_math.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#define Q4_INFINITE			16777216

///////////////////////////////////////////////////////////////////////////
//
//      DIRECTION ENCODING
//
///////////////////////////////////////////////////////////////////////////
#define NUMVERTEXNORMALS 162
static const vec3_t bytedirs[NUMVERTEXNORMALS] =
{
	{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f},
	{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f},
	{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f},
	{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f},
	{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f},
	{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f},
	{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f},
	{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f},
	{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f},
	{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f},
	{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f},
	{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f},
	{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f},
	{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f},
	{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f},
	{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f},
	{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f},
	{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f},
	{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f},
	{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f},
	{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f},
	{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f},
	{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f},
	{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f},
	{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f},
	{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f},
	{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f},
	{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f},
	{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f},
	{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f},
	{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f},
	{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f},
	{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f},
	{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f},
	{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f},
	{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f},
	{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f},
	{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f},
	{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f},
	{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f},
	{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f},
	{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f},
	{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f},
	{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f},
	{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f},
	{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f},
	{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f},
	{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f},
	{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f},
	{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f},
	{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f},
	{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f},
	{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f},
	{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f},
	{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f},
	{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f},
	{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f},
	{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f},
	{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f},
	{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f},
	{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f},
	{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f},
	{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f},
	{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f},
	{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f},
	{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f},
	{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f},
	{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f},
	{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f},
	{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f},
	{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f},
	{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f},
	{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f},
	{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f},
	{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f},
	{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f},
	{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f},
	{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f},
	{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f},
	{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir)
{
	if (!dir) {
		return 0;
	}

	float bestd = 0;
	int best = 0;
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		const float d = DotProduct(dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir(const int b, vec3_t dir)
{
	if (b < 0 || b >= NUMVERTEXNORMALS) {
		VectorCopy(vec3_origin, dir);
		return;
	}
	VectorCopy(bytedirs[b], dir);
}

/*
** NormalToLatLong
**
** We use two byte encoded normals in some space critical applications.
** Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
** Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
**
*/
//rwwRMG - added
void NormalToLatLong(const vec3_t normal, byte bytes[2])
{
	// check for singularities
	if (!normal[0] && !normal[1])
	{
		if (normal[2] > 0.0f)
		{
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		}
		else
		{
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	}
	else
	{
		int a = (int)(RAD2DEG((float)atan2(normal[1], normal[0])) * (255.0f / 360.0f));
		a &= 0xff;

		int b = (int)(RAD2DEG((float)acos(normal[2])) * (255.0f / 360.0f));
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      RANDOM NUMBER GENERATION
//
///////////////////////////////////////////////////////////////////////////
int Q_rand(int* seed)
{
	*seed = 69069 * *seed + 1;
	return *seed;
}

float Q_random(int* seed)
{
	return (Q_rand(seed) & 0xffff) / (float)0x10000;
}

float Q_crandom(int* seed)
{
	return 2.0f * (Q_random(seed) - 0.5f);
}

static uint32_t	holdrand = 0x89abcdef;

void Rand_Init(const int seed)
{
	holdrand = seed;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)
float flrand(const float min, const float max)
{
	holdrand = holdrand * 214013L + 2531011L;
	float result = (float)(holdrand >> 17); // 0 - 32767 range
	result = result * (max - min) / (float)QRAND_MAX + min;

	return result;
}

float Q_flrand(const float min, const float max)
{
	return flrand(min, max);
}

// Returns an integer min <= x <= max (ie inclusive)
int irand(const int min, int max)
{
	assert(max - min < QRAND_MAX);

	max++;
	holdrand = holdrand * 214013L + 2531011L;
	int result = holdrand >> 17;
	result = (result * (max - min) >> 15) + min;

	return result;
}

int Q_irand(const int value1, const int value2)
{
	return irand(value1, value2);
}

int Q_irand2(const int min, const int max)
{
	return rand() % (max - min + 1) + min;
}

/*
erandom

This function produces a random number with a exponential
distribution and the specified mean value.
*/
float erandom(const float mean)
{
	float	r;

	do {
		r = Q_flrand(0.0f, 1.0f);
	} while (r == 0.0);

	return -mean * logf(r);
}

///////////////////////////////////////////////////////////////////////////
//
//      MATH UTILITIES
//
///////////////////////////////////////////////////////////////////////////
signed char ClampChar(const int i)
{
	if (i < -128) {
		return -128;
	}
	if (i > 127) {
		return 127;
	}
	return i;
}

signed char ClampCharMove(const int i)
{
	if (i < -127) {
		return -127;
	}
	if (i > 127) {
		return 127;
	}
	return i;
}

signed short ClampShort(const int i)
{
	if (i < -32768) {
		return -32768;
	}
	if (i > 0x7fff) {
		return 0x7fff;
	}
	return i;
}

int Com_Clampi(const int min, const int max, const int value)
{
	if (value < min)
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}

float Com_Clamp(const float min, const float max, const float value) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

int Com_AbsClampi(const int min, const int max, const int value)
{
	if (value < 0)
	{
		return Com_Clampi(-max, -min, value);
	}
	return Com_Clampi(min, max, value);
}

float Com_AbsClamp(const float min, const float max, const float value)
{
	if (value < 0.0f)
	{
		return Com_Clamp(-max, -min, value);
	}
	return Com_Clamp(min, max, value);
}

float Q_rsqrt(const float number)
{
	byteAlias_t t;
	const float threehalfs = 1.5F;

	const float x2 = number * 0.5F;
	t.f = number;
	t.i = 0x5f3759df - (t.i >> 1);               // what the fuck?
	float y = t.f;
	y = y * (threehalfs - x2 * y * y);   // 1st iteration
	//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	assert(!Q_isnan(y));
	return y;
}

float Q_fabs(const float f)
{
	byteAlias_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

This should go in q_math but it is too late to add new traps
to game and ui
=====================
*/
float Q_acos(const float c) {
	const float angle = acosf(c);

	if (angle > M_PI) {
		return M_PI;
	}
	if (angle < -M_PI) {
		return M_PI;
	}
	return angle;
}

float Q_asin(const float c)
{
	const float angle = asinf(c);

	if (angle > M_PI) {
		return M_PI;
	}
	if (angle < -M_PI) {
		return M_PI;
	}
	return angle;
}

float Q_powf(const float x, int y)
{
	float r = x;
	for (y--; y > 0; y--)
		r *= x;
	return r;
}

qboolean Q_isnan(float f)
{
#ifdef _MSC_VER
	return _isnan(f) != 0;
#else
	return (qboolean)(isnan(f) != 0);
#endif
}

int Q_log2(int val)
{
	int answer = 0;
	while ((val >>= 1) != 0) {
		answer++;
	}
	return answer;
}

float LerpAngle(const float from, float to, const float frac)
{
	if (to - from > 180) {
		to -= 360;
	}
	if (to - from < -180) {
		to += 360;
	}
	const float a = from + frac * (to - from);

	return a;
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract(const float a1, const float a2) {
	float a = a1 - a2;
	a = fmodf(a, 360);//chop it down quickly, then level it out
	while (a > 180) {
		a -= 360;
	}
	while (a < -180) {
		a += 360;
	}
	return a;
}

void AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3) {
	v3[0] = AngleSubtract(v1[0], v2[0]);
	v3[1] = AngleSubtract(v1[1], v2[1]);
	v3[2] = AngleSubtract(v1[2], v2[2]);
}

float AngleMod(float a) {
	a = 360.0f / 65536 * ((int)(a * (65536 / 360.0f)) & 65535);
	return a;
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360(const float angle) {
	return 360.0f / 65536 * ((int)(angle * (65536 / 360.0f)) & 65535);
}

/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180(float angle) {
	angle = AngleNormalize360(angle);
	if (angle > 180.0) {
		angle -= 360.0;
	}
	return angle;
}

/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta(const float angle1, const float angle2) {
	return AngleNormalize180(angle1 - angle2);
}

///////////////////////////////////////////////////////////////////////////
//
//      GEOMETRIC UTILITIES
//
///////////////////////////////////////////////////////////////////////////
/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c) {
	vec3_t	d1, d2;

	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);
	CrossProduct(d2, d1, plane);
	if (VectorNormalize(plane) == 0) {
		return qfalse;
	}

	plane[3] = DotProduct(a, plane);
	return qtrue;
}

/*
===============
RotatePointAroundVector

From q3mme
===============
*/
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees) {
	float   m[3][3];

	degrees = -DEG2RAD(degrees);
	const float s = sinf(degrees);
	const float c = cosf(degrees);
	const float t = 1 - c;

	m[0][0] = t * dir[0] * dir[0] + c;
	m[0][1] = t * dir[0] * dir[1] + s * dir[2];
	m[0][2] = t * dir[0] * dir[2] - s * dir[1];

	m[1][0] = t * dir[0] * dir[1] - s * dir[2];
	m[1][1] = t * dir[1] * dir[1] + c;
	m[1][2] = t * dir[1] * dir[2] + s * dir[0];

	m[2][0] = t * dir[0] * dir[2] + s * dir[1];
	m[2][1] = t * dir[1] * dir[2] - s * dir[0];
	m[2][2] = t * dir[2] * dir[2] + c;
	VectorRotate(point, m, dst);
}

void RotateAroundDirection(matrix3_t axis, const float yaw) {
	// create an arbitrary axis[1]
	PerpendicularVector(axis[1], axis[0]);

	// rotate it around axis[0] by yaw
	if (yaw) {
		vec3_t	temp;

		VectorCopy(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	// cross to get axis[2]
	CrossProduct(axis[0], axis[1], axis[2]);
}

void vectoangles(const vec3_t value1, vec3_t angles) {
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0) {
		yaw = 0;
		if (value1[2] > 0) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if (value1[0]) {
			yaw = atan2f(value1[1], value1[0]) * 180 / M_PI;
		}
		else if (value1[1] > 0) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}

		const float forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = atan2f(value1[2], forward) * 180 / M_PI;
		if (pitch < 0) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

vec_t GetYawForDirection(const vec3_t p1, const vec3_t p2) {
	vec3_t v, angles;

	VectorSubtract(p2, p1, v);
	vectoangles(v, angles);

	return angles[YAW];
}

void GetAnglesForDirection(const vec3_t p1, const vec3_t p2, vec3_t out) {
	vec3_t v;

	VectorSubtract(p2, p1, v);
	vectoangles(v, out);
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	vec3_t n;

	float inv_denom = DotProduct(normal, normal);
	assert(Q_fabs(inv_denom) != 0.0f);
	inv_denom = 1.0f / inv_denom;

	const float d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

qboolean G_FindClosestPointOnLineSegment(const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result)
{
	vec3_t	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract(from, start, vecStart2From);
	VectorSubtract(end, start, vecStart2End);

	float dot = DotProductNormalize(vecStart2From, vecStart2End);

	if (dot <= 0)
	{
		//The perpendicular would be beyond or through the start point
		VectorCopy(start, result);
		return qfalse;
	}

	if (dot == 1)
	{
		//parallel, closer of 2 points will be the target
		if (VectorLengthSquared(vecStart2From) < VectorLengthSquared(vecStart2End))
		{
			VectorCopy(from, result);
		}
		else
		{
			VectorCopy(end, result);
		}
		return qfalse;
	}

	//Try other end
	VectorSubtract(from, end, vecEnd2From);
	VectorSubtract(start, end, vecEnd2Start);

	dot = DotProductNormalize(vecEnd2From, vecEnd2Start);

	if (dot <= 0)
	{//The perpendicular would be beyond or through the start point
		VectorCopy(end, result);
		return qfalse;
	}

	if (dot == 1)
	{//parallel, closer of 2 points will be the target
		if (VectorLengthSquared(vecEnd2From) < VectorLengthSquared(vecEnd2Start))
		{
			VectorCopy(from, result);
		}
		else
		{
			VectorCopy(end, result);
		}
		return qfalse;
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	const float theta = 90 * (1 - dot);//theta

	//Get length of side from End2Result using sine of theta
	const float distEnd2From = VectorLength(vecEnd2From);//c
	const float cos_theta = cosf(DEG2RAD(theta));//cos(theta)
	const float distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize(vecEnd2Start);
	VectorMA(end, distEnd2Result, vecEnd2Start, result);

	//perpendicular intersection is between the 2 endpoints
	return qtrue;
}

float G_PointDistFromLineSegment(const vec3_t start, const vec3_t end, const vec3_t from)
{
	vec3_t	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From, intersection;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract(from, start, vecStart2From);
	VectorSubtract(end, start, vecStart2End);
	VectorSubtract(from, end, vecEnd2From);
	VectorSubtract(start, end, vecEnd2Start);

	float dot = DotProductNormalize(vecStart2From, vecStart2End);

	const float distStart2From = Distance(start, from);
	const float distEnd2From = Distance(end, from);

	if (dot <= 0)
	{
		//The perpendicular would be beyond or through the start point
		return distStart2From;
	}

	if (dot == 1)
	{
		//parallel, closer of 2 points will be the target
		return distStart2From < distEnd2From ? distStart2From : distEnd2From;
	}

	//Try other end

	dot = DotProductNormalize(vecEnd2From, vecEnd2Start);

	if (dot <= 0)
	{//The perpendicular would be beyond or through the end point
		return distEnd2From;
	}

	if (dot == 1)
	{//parallel, closer of 2 points will be the target
		return distStart2From < distEnd2From ? distStart2From : distEnd2From;
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	const float theta = 90 * (1 - dot);//theta

	//Get length of side from End2Result using sine of theta
	const float cos_theta = cosf(DEG2RAD(theta));//cos(theta)
	const float distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize(vecEnd2Start);
	VectorMA(end, distEnd2Result, vecEnd2Start, intersection);

	//perpendicular intersection is between the 2 endpoints, return dist to it from from
	return Distance(intersection, from);
}

float ShortestLineSegBewteen2LineSegs(vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2)
{//racc - finds the shorts line between two line segments.  I moved this to q_math.c to make it available for the saber lock effects on the cgame side.
	float	current_dist;
	vec3_t	new_pnt;

	//compute some temporaries:
	//vec start_dif = start2 - start1
	vec3_t	start_dif;
	vec3_t	v1;
	vec3_t	v2;

	VectorSubtract(start2, start1, start_dif);
	//vec v1 = end1 - start1
	VectorSubtract(end1, start1, v1);
	//vec v2 = end2 - start2
	VectorSubtract(end2, start2, v2);
	//
	const float v1v1 = DotProduct(v1, v1);
	const float v2v2 = DotProduct(v2, v2);
	const float v1v2 = DotProduct(v1, v2);

	//the main computation

	const float denom = v1v2 * v1v2 - v1v1 * v2v2;

	//if denom is small, then skip all this and jump to the section marked below
	if (fabs(denom) > 0.001f)
	{
		float s = -(v2v2 * DotProduct(v1, start_dif) - v1v2 * DotProduct(v2, start_dif)) / denom;
		float t = (v1v1 * DotProduct(v2, start_dif) - v1v2 * DotProduct(v1, start_dif)) / denom;
		qboolean done = qtrue;

		if (s < 0)
		{
			done = qfalse;
			s = 0;// and see note below
		}

		if (s > 1)
		{
			done = qfalse;
			s = 1;// and see note below
		}

		if (t < 0)
		{
			done = qfalse;
			t = 0;// and see note below
		}

		if (t > 1)
		{
			done = qfalse;
			t = 1;// and see note below
		}

		//vec close_pnt1 = start1 + s * v1
		VectorMA(start1, s, v1, close_pnt1);
		//vec close_pnt2 = start2 + t * v2
		VectorMA(start2, t, v2, close_pnt2);

		current_dist = Distance(close_pnt1, close_pnt2);
		//now, if none of those if's fired, you are done.
		if (done)
		{
			return current_dist;
		}
	}
	else
	{
		//******start here for paralell lines with current_dist = infinity****
		current_dist = Q4_INFINITE;
	}
	//test all the endpoints
	float new_dist = Distance(start1, start2);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(start1, close_pnt1);
		VectorCopy(start2, close_pnt2);
		current_dist = new_dist;
	}

	new_dist = Distance(start1, end2);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(start1, close_pnt1);
		VectorCopy(end2, close_pnt2);
		current_dist = new_dist;
	}

	new_dist = Distance(end1, start2);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(end1, close_pnt1);
		VectorCopy(start2, close_pnt2);
		current_dist = new_dist;
	}

	new_dist = Distance(end1, end2);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(end1, close_pnt1);
		VectorCopy(end2, close_pnt2);
		current_dist = new_dist;
	}

	//Then we have 4 more point / segment tests

	G_FindClosestPointOnLineSegment(start2, end2, start1, new_pnt);
	new_dist = Distance(start1, new_pnt);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(start1, close_pnt1);
		VectorCopy(new_pnt, close_pnt2);
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment(start2, end2, end1, new_pnt);
	new_dist = Distance(end1, new_pnt);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(end1, close_pnt1);
		VectorCopy(new_pnt, close_pnt2);
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment(start1, end1, start2, new_pnt);
	new_dist = Distance(start2, new_pnt);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(new_pnt, close_pnt1);
		VectorCopy(start2, close_pnt2);
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment(start1, end1, end2, new_pnt);
	new_dist = Distance(end2, new_pnt);
	if (new_dist < current_dist)
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy(new_pnt, close_pnt1);
		VectorCopy(end2, close_pnt2);
		current_dist = new_dist;
	}

	return current_dist;
}

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}

///////////////////////////////////////////////////////////////////////////
//
//      BOUNDING BOX
//
///////////////////////////////////////////////////////////////////////////
float RadiusFromBounds(const vec3_t mins, const vec3_t maxs) {
	vec3_t	corner;

	for (int i = 0; i < 3; i++) {
		const float a = fabsf(mins[i]);
		const float b = fabsf(maxs[i]);
		corner[i] = a > b ? a : b;
	}

	return VectorLength(corner);
}

void ClearBounds(vec3_t mins, vec3_t maxs) {
	mins[0] = mins[1] = mins[2] = 100000;
	maxs[0] = maxs[1] = maxs[2] = -100000;
}

void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs) {
	if (v[0] < mins[0]) {
		mins[0] = v[0];
	}
	if (v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if (v[1] < mins[1]) {
		mins[1] = v[1];
	}
	if (v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if (v[2] < mins[2]) {
		mins[2] = v[2];
	}
	if (v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      PLANE
//
///////////////////////////////////////////////////////////////////////////
void SetPlaneSignbits(cplane_t* out)
{
	// for fast box on planeside test
	int bits = 0;
	for (int j = 0; j < 3; j++) {
		if (out->normal[j] < 0) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}

int	PlaneTypeForNormal(vec3_t normal)
{
	if (normal[0] == 1.0)
		return PLANE_X;
	if (normal[1] == 1.0)
		return PLANE_Y;
	if (normal[2] == 1.0)
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, const cplane_t* p)
{
	float	dist[2];

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (int i = 0; i < 3; i++)
		{
			const int b = p->signbits >> i & 1;
			dist[b] += p->normal[i] * emaxs[i];
			dist[!b] += p->normal[i] * emins[i];
		}
	}

	int sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;

	return sides;
}

///////////////////////////////////////////////////////////////////////////
//
//      AXIS
//
///////////////////////////////////////////////////////////////////////////
matrix3_t	axisDefault = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

void AxisClear(matrix3_t axis) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy(matrix3_t in, matrix3_t out) {
	VectorCopy(in[0], out[0]);
	VectorCopy(in[1], out[1]);
	VectorCopy(in[2], out[2]);
}

void AnglesToAxis(const vec3_t angles, matrix3_t axis) {
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors(angles, axis[0], right, axis[2]);
	VectorSubtract(vec3_origin, right, axis[1]);
}

///////////////////////////////////////////////////////////////////////////
//
//      VEC2
//
///////////////////////////////////////////////////////////////////////////
vec2_t		vec2_zero = { 0,0 };

void VectorAdd2(const vec2_t vec1, const vec2_t vec2, vec2_t vecOut)
{
	vecOut[0] = vec1[0] + vec2[0];
	vecOut[1] = vec1[1] + vec2[1];
}

void VectorSubtract2(const vec2_t vec1, const vec2_t vec2, vec2_t vecOut)
{
	vecOut[0] = vec1[0] - vec2[0];
	vecOut[1] = vec1[1] - vec2[1];
}

void VectorScale2(const vec2_t vecIn, const float scale, vec2_t vecOut)
{
	vecOut[0] = vecIn[0] * scale;
	vecOut[1] = vecIn[1] * scale;
}

void VectorMA2(const vec2_t vec1, const float scale, const vec2_t vec2, vec2_t vecOut)
{
	vecOut[0] = vec1[0] + scale * vec2[0];
	vecOut[1] = vec1[1] + scale * vec2[1];
}

void VectorSet2(vec2_t vec, const float x, const float y)
{
	vec[0] = x; vec[1] = y;
}

void VectorClear2(vec2_t vec)
{
	vec[0] = vec[1] = 0.0f;
}

void VectorCopy2(const vec2_t vecIn, vec2_t vecOut)
{
	vecOut[0] = vecIn[0];
	vecOut[1] = vecIn[1];
}

///////////////////////////////////////////////////////////////////////////
//
//      VEC3
//
///////////////////////////////////////////////////////////////////////////
vec3_t		vec3_origin = { 0,0,0 };

void VectorAdd(const vec3_t vec1, const vec3_t vec2, vec3_t vecOut)
{
	vecOut[0] = vec1[0] + vec2[0];
	vecOut[1] = vec1[1] + vec2[1];
	vecOut[2] = vec1[2] + vec2[2];
}

void VectorSubtract(const vec3_t vec1, const vec3_t vec2, vec3_t vecOut)
{
	vecOut[0] = vec1[0] - vec2[0];
	vecOut[1] = vec1[1] - vec2[1];
	vecOut[2] = vec1[2] - vec2[2];
}

void VectorScale(const vec3_t vecIn, const float scale, vec3_t vecOut)
{
	vecOut[0] = vecIn[0] * scale;
	vecOut[1] = vecIn[1] * scale;
	vecOut[2] = vecIn[2] * scale;
}

void VectorMA(const vec3_t vec1, const float scale, const vec3_t vec2, vec3_t vecOut)
{
	vecOut[0] = vec1[0] + scale * vec2[0];
	vecOut[1] = vec1[1] + scale * vec2[1];
	vecOut[2] = vec1[2] + scale * vec2[2];
}

void VectorSet(vec3_t vec, const float x, const float y, const float z)
{
	vec[0] = x; vec[1] = y; vec[2] = z;
}

void VectorClear(vec3_t vec)
{
	vec[0] = vec[1] = vec[2] = 0.0f;
}

void VectorCopy(const vec3_t vecIn, vec3_t vecOut)
{
	vecOut[0] = vecIn[0];
	vecOut[1] = vecIn[1];
	vecOut[2] = vecIn[2];
}

float VectorLength(const vec3_t vec)
{
	return (float)sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

float VectorLengthSquared(const vec3_t vec)
{
	return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
}

float Distance(const vec3_t p1, const vec3_t p2)
{
	vec3_t	v;

	VectorSubtract(p2, p1, v);
	return VectorLength(v);
}

float DistanceSquared(const vec3_t p1, const vec3_t p2)
{
	vec3_t	v;

	VectorSubtract(p2, p1, v);
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void VectorNormalizeFast(vec3_t vec)
{
	const float ilength = Q_rsqrt(DotProduct(vec, vec));

	vec[0] *= ilength;
	vec[1] *= ilength;
	vec[2] *= ilength;
}

float VectorNormalize(vec3_t vec)
{
	float length = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
	length = sqrtf(length);

	if (length) {
		const float ilength = 1 / length;
		vec[0] *= ilength;
		vec[1] *= ilength;
		vec[2] *= ilength;
	}

	return length;
}

float VectorNormalize2(const vec3_t vec, vec3_t vecOut)
{
	float length = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
	length = sqrtf(length);

	if (length) {
		const float ilength = 1 / length;
		vecOut[0] = vec[0] * ilength;
		vecOut[1] = vec[1] * ilength;
		vecOut[2] = vec[2] * ilength;
	}
	else
		VectorClear(vecOut);

	return length;
}

void VectorAdvance(const vec3_t veca, const float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * (vecb[0] - veca[0]);
	vecc[1] = veca[1] + scale * (vecb[1] - veca[1]);
	vecc[2] = veca[2] + scale * (vecb[2] - veca[2]);
}

void VectorInc(vec3_t vec) {
	vec[0] += 1.0f; vec[1] += 1.0f; vec[2] += 1.0f;
}

void VectorDec(vec3_t vec) {
	vec[0] -= 1.0f; vec[1] -= 1.0f; vec[2] -= 1.0f;
}

void VectorInverse(vec3_t vec) {
	vec[0] = -vec[0]; vec[1] = -vec[1]; vec[2] = -vec[2];
}

void CrossProduct(const vec3_t vec1, const vec3_t vec2, vec3_t vec_out) {
	vec_out[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
	vec_out[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
	vec_out[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
}

float DotProduct(const vec3_t vec1, const vec3_t vec2) {
	return vec1[0] * vec2[0] + vec1[1] * vec2[1] + vec1[2] * vec2[2];
}

qboolean VectorCompare(const vec3_t vec1, const vec3_t vec2)
{
	return vec1[0] == vec2[0] && vec1[1] == vec2[1] && vec1[2] == vec2[2];
}

qboolean VectorCompare2(const vec3_t v1, const vec3_t v2)
{
	if (v1[0] > v2[0] + 0.0001f || v1[0] < v2[0] - 0.0001f ||
		v1[1] > v2[1] + 0.0001f || v1[1] < v2[1] + 0.0001f ||
		v1[2] > v2[2] + 0.0001f || v1[2] < v2[2] + 0.0001f)
	{
		return qfalse;
	}

	return qtrue;
}

void SnapVector(float* v)
{
#if defined(_MSC_VER) && !defined(idx64)
	// pitiful attempt to reduce _ftol2 calls -rww
	static int i;
	static float f;

	f = *v;
	__asm fld f
	__asm fistp	i
	* v = (float)i;
	v++;
	f = *v;
	__asm fld f
	__asm fistp i
	* v = (float)i;
	v++;
	f = *v;
	__asm fld f
	__asm fistp i
	* v = (float)i;
#else // mac, linux, mingw
	v[0] = (int)v[0];
	v[1] = (int)v[1];
	v[2] = (int)v[2];
#endif
}

float DistanceHorizontal(const vec3_t p1, const vec3_t p2)
{
	vec3_t	v;

	VectorSubtract(p2, p1, v);
	return sqrtf(v[0] * v[0] + v[1] * v[1]); //Leave off the z component
}

float DistanceHorizontalSquared(const vec3_t p1, const vec3_t p2)
{
	vec3_t	v;

	VectorSubtract(p2, p1, v);
	return v[0] * v[0] + v[1] * v[1];	//Leave off the z component
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up) {
	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	const float d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

void VectorRotate(const vec3_t in, matrix3_t matrix, vec3_t out)
{
	out[0] = DotProduct(in, matrix[0]);
	out[1] = DotProduct(in, matrix[1]);
	out[2] = DotProduct(in, matrix[2]);
}

void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	float angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sinf(angle);
	cr = cosf(angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
		right[1] = -1 * sr * sp * sy + -1 * cr * cy;
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = cr * sp * cy + -sr * -sy;
		up[1] = cr * sp * sy + -sr * cy;
		up[2] = cr * cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int	pos;
	int i;
	float minelem = 1.0f;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabsf(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0f;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}

float DotProductNormalize(const vec3_t inVec1, const vec3_t inVec2)
{
	vec3_t	v1, v2;

	VectorNormalize2(inVec1, v1);
	VectorNormalize2(inVec2, v2);

	return DotProduct(v1, v2);
}

///////////////////////////////////////////////////////////////////////////
//
//      VEC4
//
///////////////////////////////////////////////////////////////////////////
void VectorScale4(const vec4_t vecIn, const float scale, vec4_t vecOut)
{
	vecOut[0] = vecIn[0] * scale;
	vecOut[1] = vecIn[1] * scale;
	vecOut[2] = vecIn[2] * scale;
	vecOut[3] = vecIn[3] * scale;
}

void VectorCopy4(const vec4_t vecIn, vec4_t vecOut)
{
	vecOut[0] = vecIn[0];
	vecOut[1] = vecIn[1];
	vecOut[2] = vecIn[2];
	vecOut[3] = vecIn[3];
}

void VectorSet4(vec4_t vec, const float x, const float y, const float z, const float w)
{
	vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w;
}

void VectorClear4(vec4_t vec)
{
	vec[0] = vec[1] = vec[2] = vec[3] = 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      VEC5
//
///////////////////////////////////////////////////////////////////////////
void VectorSet5(vec5_t vec, const float x, const float y, const float z, const float w, const float u) {
	vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; vec[4] = u;
}

float VectorDistance(vec3_t v1, vec3_t v2)
{//returns the distance between the two points.
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

float Distance2(const vec3_t p1, const vec3_t p2)
{
	vec3_t	v;

	VectorSubtract(p2, p1, v);
	return VectorLength(v);
}