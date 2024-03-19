/*
===========================================================================
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

#ifndef FXSYSTEM_H
#define FXSYSTEM_H
#if !defined(CG_LOCAL_H_INC)
#include "cg_local.h"
#endif

#ifndef FX_SYSTEM_H_INC
#define FX_SYSTEM_H_INC

#include "qcommon/safe/gsl.h"

#define irand	Q_irand
#define flrand	Q_flrand

extern vmCvar_t fx_debug;
extern vmCvar_t fx_freeze;
extern vmCvar_t fx_optimizedParticles;

inline void Vector2Clear(vec2_t a)
{
	a[0] = 0.0f;
	a[1] = 0.0f;
}

inline void Vector2Set(vec2_t a, const float b, const float c)
{
	a[0] = b;
	a[1] = c;
}

inline void Vector2Copy(vec2_t src, vec2_t dst)
{
	dst[0] = src[0];
	dst[1] = src[1];
}

extern void CG_CalcEntityLerpPositions(centity_t*);

struct SFxHelper
{
	int mTime;
	int mFrameTime;
	float mFloatFrameTime;

	void Init();
	void AdjustTime(int frametime);

	// These functions are wrapped and used by the fx system in case it makes things a bit more portable
	static void Print(const char* msg, ...);

	// File handling
	static int OpenFile(const char* file, fileHandle_t* fh, int mode);
	static int ReadFile(void* data, int len, fileHandle_t fh);
	static void CloseFile(fileHandle_t fh);

	// Sound
	static void PlaySound(const vec3_t org, int entityNum, int entchannel, sfxHandle_t sfxHandle);
	static void PlayLocalSound(sfxHandle_t sfxHandle, int channelNum);
	static int RegisterSound(const gsl::cstring_view& sound);

	//G2
	static int GetOriginAxisFromBolt(const centity_t& cent, int modelNum, int boltNum, vec3_t /*out*/origin,
		vec3_t /*out*/* axis);

	// Physics/collision
	static void Trace(trace_t* tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, int skipEntNum, int flags);
	static void G2Trace(trace_t* tr, vec3_t start, vec3_t min, vec3_t max, vec3_t end, int skipEntNum, int flags);

	static void AddFxToScene(const refEntity_t* ent);
	static void AddLightToScene(vec3_t org, float radius, float red, float green, float blue);

	static int RegisterShader(const gsl::cstring_view& shader);
	static int RegisterModel(const gsl::cstring_view& model);

	static void AddPolyToScene(int shader, int count, const polyVert_t* verts);

	static void CameraShake(vec3_t origin, float intensity, int radius, int time);
};

extern SFxHelper theFxHelper;

#endif // FX_SYSTEM_H_INC
#endif // FXSYSTEM_H
