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

// tr_shade_calc.c

#include "tr_local.h"
#include "../rd-common/tr_common.h"

#define	WAVEVALUE( table, base, amplitude, phase, freq )  ((base) + table[ Q_ftol( ( ( (phase) + tess.shaderTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))

static float* TableForFunc(const genFunc_t func)
{
	switch (func)
	{
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	Com_Error(ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.shader->name);
	return NULL;
}

/*
** EvalWaveForm
**
** Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
*/
static float EvalWaveForm(const waveForm_t* wf)
{
	if (wf->func == GF_NOISE) {
		return  wf->base + R_NoiseGet4f(0, 0, 0, (backEnd.refdef.floatTime + wf->phase) * wf->frequency) * wf->amplitude;
	}
	if (wf->func == GF_RAND) {
		if (GetNoiseTime(backEnd.refdef.time + wf->phase) <= wf->frequency) {
			return wf->base + wf->amplitude;
		}
		return wf->base;
	}
	const float* table = TableForFunc(wf->func);

	return WAVEVALUE(table, wf->base, wf->amplitude, wf->phase, wf->frequency);
}

static float EvalWaveFormClamped(const waveForm_t* wf)
{
	const float glow = EvalWaveForm(wf);

	if (glow < 0)
	{
		return 0;
	}

	if (glow > 1)
	{
		return 1;
	}

	return glow;
}

/*
** RB_CalcStretchTexCoords
*/
void RB_CalcStretchTexCoords(const waveForm_t* wf, float* texCoords)
{
	texModInfo_t tmi{};

	const float p = 1.0f / EvalWaveForm(wf);

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords(&tmi, texCoords);
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
========================
RB_CalcDeformVertexes

========================
*/
static void RB_CalcDeformVertexes(const deformStage_t* ds)
{
	int i;
	vec3_t	offset;
	float	scale;
	auto* xyz = reinterpret_cast<float*>(tess.xyz);
	auto* normal = reinterpret_cast<float*>(tess.normal);

	if (ds->deformationWave.frequency == 0)
	{
		scale = EvalWaveForm(&ds->deformationWave);

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		const float* table = TableForFunc(ds->deformationWave.func);

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			const float off = (xyz[0] + xyz[1] + xyz[2]) * ds->deformationSpread;

			scale = WAVEVALUE(table, ds->deformationWave.base,
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency);

			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/*
=========================
RB_CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
static void RB_CalcDeformNormals(const deformStage_t* ds)
{
	float* xyz = (float*)tess.xyz;
	float* normal = (float*)tess.normal;

	for (int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4) {
		float scale = 0.98f;
		scale = R_NoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[0] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[1] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[2] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast(normal);
	}
}

/*
========================
RB_CalcBulgeVertexes

========================
*/
static void RB_CalcBulgeVertexes(const deformStage_t* ds)
{
	int		i;
	auto* xyz = reinterpret_cast<float*>(tess.xyz);
	auto* normal = reinterpret_cast<float*>(tess.normal);

	if (ds->bulgeSpeed == 0.0f && ds->bulgeWidth == 0.0f)
	{
		// We don't have a speed and width, so just use height to expand uniformly
		for (i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			xyz[0] += normal[0] * ds->bulgeHeight;
			xyz[1] += normal[1] * ds->bulgeHeight;
			xyz[2] += normal[2] * ds->bulgeHeight;
		}
	}
	else
	{
		// I guess do some extra dumb stuff..the fact that it uses ST seems bad though because skin pages may be set up in certain ways that can cause
		//	very noticeable seams on sufaces ( like on the huge ion_cannon ).
		const auto* st = reinterpret_cast<const float*>(tess.texCoords[0]);

		const float now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

		for (i = 0; i < tess.numVertexes; i++, xyz += 4, st += 2 * NUM_TEX_COORDS, normal += 4)
		{
			const int off = FUNCTABLE_SIZE / (M_PI * 2) * (st[0] * ds->bulgeWidth + now);

			const float scale = tr.sinTable[off & FUNCTABLE_MASK] * ds->bulgeHeight;

			xyz[0] += normal[0] * scale;
			xyz[1] += normal[1] * scale;
			xyz[2] += normal[2] * scale;
		}
	}
}

/*
======================
RB_CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
static void RB_CalcMoveVertexes(const deformStage_t* ds)
{
	vec3_t		offset;

	const float* table = TableForFunc(ds->deformationWave.func);

	const float scale = WAVEVALUE(table, ds->deformationWave.base,
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency);

	VectorScale(ds->moveVector, scale, offset);

	auto* xyz = reinterpret_cast<float*>(tess.xyz);
	for (int i = 0; i < tess.numVertexes; i++, xyz += 4) {
		VectorAdd(xyz, offset, xyz);
	}
}

/*
=============
DeformText

Change a polygon into a bunch of text polygons
=============
*/
static void DeformText(const char* text)
{
	int		i;
	vec3_t	origin, width, height{};
	byte	color[4]{};
	vec3_t	mid;

	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	CrossProduct(tess.normal[0], height, width);

	// find the midpoint of the box
	VectorClear(mid);
	float bottom = WORLD_SIZE;	//999999;	// WORLD_SIZE instead of MAX_WORLD_COORD so guaranteed to be...
	float top = -WORLD_SIZE;		//-999999;	// ... outside the legal range.
	for (i = 0; i < 4; i++) {
		VectorAdd(tess.xyz[i], mid, mid);
		if (tess.xyz[i][2] < bottom) {
			bottom = tess.xyz[i][2];
		}
		if (tess.xyz[i][2] > top) {
			top = tess.xyz[i][2];
		}
	}
	VectorScale(mid, 0.25f, origin);

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = (top - bottom) * 0.5f;

	VectorScale(width, height[2] * -0.75f, width);

	// determine the starting position
	const int len = strlen(text);
	VectorMA(origin, len - 1, width, origin);

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for (i = 0; i < len; i++) {
		int ch = text[i];
		ch &= 255;

		if (ch != ' ') {
			const int row = ch >> 4;
			const int col = ch & 15;

			const float frow = row * 0.0625f;
			const float fcol = col * 0.0625f;
			constexpr float size = 0.0625f;

			RB_AddQuadStampExt(origin, width, height, color, fcol, frow, fcol + size, frow + size);
		}
		VectorMA(origin, -2, width, origin);
	}
}

/*
==================
GlobalVectorToLocal
==================
*/
static void GlobalVectorToLocal(const vec3_t in, vec3_t out) {
	out[0] = DotProduct(in, backEnd.ori.axis[0]);
	out[1] = DotProduct(in, backEnd.ori.axis[1]);
	out[2] = DotProduct(in, backEnd.ori.axis[2]);
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independent
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform(void)
{
	vec3_t	mid{};
	vec3_t	left_dir, up_dir;

	if (tess.numVertexes & 3) {
		ri->Printf(PRINT_ALL, S_COLOR_YELLOW  "Autosprite shader %s had odd vertex count", tess.shader->name);
	}
	if (tess.numIndexes != (tess.numVertexes >> 2) * 6) {
		ri->Printf(PRINT_ALL, S_COLOR_YELLOW  "Autosprite shader %s had odd index count", tess.shader->name);
	}

	const int old_verts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	if (backEnd.currentEntity != &tr.worldEntity) {
		GlobalVectorToLocal(backEnd.viewParms.ori.axis[1], left_dir);
		GlobalVectorToLocal(backEnd.viewParms.ori.axis[2], up_dir);
	}
	else {
		VectorCopy(backEnd.viewParms.ori.axis[1], left_dir);
		VectorCopy(backEnd.viewParms.ori.axis[2], up_dir);
	}

	for (int i = 0; i < old_verts; i += 4) {
		vec3_t up;
		vec3_t left;
		vec3_t delta;
		// find the midpoint
		const float* xyz = tess.xyz[i];

		mid[0] = 0.25f * (xyz[0] + xyz[4] + xyz[8] + xyz[12]);
		mid[1] = 0.25f * (xyz[1] + xyz[5] + xyz[9] + xyz[13]);
		mid[2] = 0.25f * (xyz[2] + xyz[6] + xyz[10] + xyz[14]);

		VectorSubtract(xyz, mid, delta);
		const float radius = VectorLength(delta) * 0.707f;		// / sqrt(2)

		VectorScale(left_dir, radius, left);
		VectorScale(up_dir, radius, up);

		if (backEnd.viewParms.isMirror) {
			VectorSubtract(vec3_origin, left, left);
		}

		// compensate for scale in the axes if necessary
		if (backEnd.currentEntity->e.nonNormalizedAxes) {
			float axis_length = VectorLength(backEnd.currentEntity->e.axis[0]);
			if (!axis_length) {
				axis_length = 0;
			}
			else {
				axis_length = 1.0f / axis_length;
			}
			VectorScale(left, axis_length, left);
			VectorScale(up, axis_length, up);
		}

		RB_AddQuadStamp(mid, left, up, tess.vertexColors[i]);
	}
}

/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform(void)
{
	int		i, j, k;
	int		indexes;
	vec3_t	forward;

	if (tess.numVertexes & 3) {
		ri->Printf(PRINT_ALL, S_COLOR_YELLOW  "Autosprite2 shader %s had odd vertex count", tess.shader->name);
	}
	if (tess.numIndexes != (tess.numVertexes >> 2) * 6) {
		ri->Printf(PRINT_ALL, S_COLOR_YELLOW  "Autosprite2 shader %s had odd index count", tess.shader->name);
	}

	if (backEnd.currentEntity != &tr.worldEntity) {
		GlobalVectorToLocal(backEnd.viewParms.ori.axis[0], forward);
	}
	else {
		VectorCopy(backEnd.viewParms.ori.axis[0], forward);
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for (i = 0, indexes = 0; i < tess.numVertexes; i += 4, indexes += 6) {
		float	lengths[2]{};
		int		nums[2]{};
		vec3_t	mid[2]{};
		vec3_t	major, minor;
		float* v1, * v2;

		// find the midpoint
		float* xyz = tess.xyz[i];

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for (j = 0; j < 6; j++) {
			vec3_t	temp;

			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract(v1, v2, temp);

			const float l = DotProduct(temp, temp);
			if (l < lengths[0]) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			}
			else if (l < lengths[1]) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for (j = 0; j < 2; j++) {
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract(mid[1], mid[0], major);

		// cross this with the view direction to get minor axis
		CrossProduct(major, forward, minor);
		VectorNormalize(minor);

		// re-project the points
		for (j = 0; j < 2; j++) {
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			const float l = 0.5 * sqrt(lengths[j]);

			// we need to see which direction this edge
			// is used to determine direction of projection
			for (k = 0; k < 5; k++) {
				if (tess.indexes[indexes + k] == static_cast<unsigned>(i + edgeVerts[nums[j]][0])
					&& tess.indexes[indexes + k + 1] == static_cast<unsigned>(i + edgeVerts[nums[j]][1])) {
					break;
				}
			}

			if (k == 5) {
				VectorMA(mid[j], l, minor, v1);
				VectorMA(mid[j], -l, minor, v2);
			}
			else {
				VectorMA(mid[j], -l, minor, v1);
				VectorMA(mid[j], l, minor, v2);
			}
		}
	}
}

/*
=====================
RB_DeformTessGeometry

=====================
*/
void RB_DeformTessGeometry(void)
{
	for (int i = 0; i < tess.shader->numDeforms; i++)
	{
		const deformStage_t* ds = tess.shader->deforms[i];

		switch (ds->deformation)
		{
		case DEFORM_NONE:
			break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals(ds);
			break;
		case DEFORM_WAVE:
			RB_CalcDeformVertexes(ds);
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes(ds);
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes(ds);
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
			DeformText(backEnd.refdef.text[ds->deformation - DEFORM_TEXT0]);
			break;
		}
	}
}

/*
====================================================================

COLORS

====================================================================
*/

/*
** RB_CalcColorFromEntity
*/

void RB_CalcColorFromEntity(unsigned char* dst_colors)
{
	auto* p_colors = reinterpret_cast<int*>(dst_colors);

	if (!backEnd.currentEntity)
		return;

	const byteAlias_t* ba = reinterpret_cast<byteAlias_t*>(&backEnd.currentEntity->e.shaderRGBA);

	for (int i = 0; i < tess.numVertexes; i++) {
		*p_colors++ = ba->i;
	}
}

/*
** RB_CalcColorFromOneMinusEntity
*/
void RB_CalcColorFromOneMinusEntity(unsigned char* dst_colors)
{
	auto* p_colors = reinterpret_cast<int*>(dst_colors);
	unsigned char inv_modulate[4]{};

	if (!backEnd.currentEntity)
		return;

	inv_modulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	inv_modulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	inv_modulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	inv_modulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];	// this trashes alpha, but the AGEN block fixes it

	const byteAlias_t* ba = reinterpret_cast<byteAlias_t*>(&inv_modulate);

	for (int i = 0; i < tess.numVertexes; i++) {
		*p_colors++ = ba->i;
	}
}

/*
** RB_CalcAlphaFromEntity
*/
void RB_CalcAlphaFromEntity(unsigned char* dst_colors)
{
	if (!backEnd.currentEntity)
		return;

	dst_colors += 3;

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4)
	{
		*dst_colors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/*
** RB_CalcAlphaFromOneMinusEntity
*/
void RB_CalcAlphaFromOneMinusEntity(unsigned char* dst_colors)
{
	if (!backEnd.currentEntity)
		return;

	dst_colors += 3;

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4)
	{
		*dst_colors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/*
** RB_CalcWaveColor
*/
void RB_CalcWaveColor(const waveForm_t* wf, unsigned char* dst_colors)
{
	float glow;
	auto* colors = reinterpret_cast<int*>(dst_colors);
	byte	color[4]{};

	if (wf->func == GF_NOISE) {
		glow = wf->base + R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
	}
	else {
		glow = EvalWaveForm(wf) * tr.identityLight;
	}

	if (glow < 0) {
		glow = 0;
	}
	else if (glow > 1) {
		glow = 1;
	}

	const int v = Q_ftol(255 * glow);
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	const byteAlias_t* ba = reinterpret_cast<byteAlias_t*>(&color);

	for (int i = 0; i < tess.numVertexes; i++) {
		*colors++ = ba->i;
	}
}

/*
** RB_CalcWaveAlpha
*/
void RB_CalcWaveAlpha(const waveForm_t* wf, unsigned char* dst_colors)
{
	const float glow = EvalWaveFormClamped(wf);

	const int v = 255 * glow;

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4)
	{
		dst_colors[3] = v;
	}
}

/*
** RB_CalcModulateColorsByFog
*/
void RB_CalcModulateColorsByFog(unsigned char* dst_colors)
{
	float	texCoords[SHADER_MAX_VERTEXES][2]{};

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4)
	{
		const float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		dst_colors[0] *= f;
		dst_colors[1] *= f;
		dst_colors[2] *= f;
	}
}

/*
** RB_CalcModulateAlphasByFog
*/
void RB_CalcModulateAlphasByFog(unsigned char* dst_colors)
{
	float	texCoords[SHADER_MAX_VERTEXES][2]{};

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4) {
		const float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		dst_colors[3] *= f;
	}
}

/*
** RB_CalcModulateRGBAsByFog
*/
void RB_CalcModulateRGBAsByFog(unsigned char* dst_colors)
{
	float	texCoords[SHADER_MAX_VERTEXES][2]{};

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, dst_colors += 4) {
		const float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		dst_colors[0] *= f;
		dst_colors[1] *= f;
		dst_colors[2] *= f;
		dst_colors[3] *= f;
	}
}

/*
====================================================================

TEX COORDS

====================================================================
*/

/*
========================
RB_CalcFogTexCoords

To do the clipped fog plane really correctly, we should use
projected textures, but I don't trust the drivers and it
doesn't fit our shader data.
========================
*/
void RB_CalcFogTexCoords(float* dstTexCoords) {
	int			i;
	float* v;
	float		eye_t;
	qboolean	eye_outside;
	vec3_t		local_vec;
	vec4_t		fogDistanceVector{}, fogDepthVector{};

	const fog_t* fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	VectorSubtract(backEnd.ori.origin, backEnd.viewParms.ori.origin, local_vec);
	fogDistanceVector[0] = -backEnd.ori.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.ori.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.ori.modelMatrix[10];
	fogDistanceVector[3] = DotProduct(local_vec, backEnd.viewParms.ori.axis[0]);

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if (fog->hasSurface) {
		fogDepthVector[0] = fog->surface[0] * backEnd.ori.axis[0][0] +
			fog->surface[1] * backEnd.ori.axis[0][1] + fog->surface[2] * backEnd.ori.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.ori.axis[1][0] +
			fog->surface[1] * backEnd.ori.axis[1][1] + fog->surface[2] * backEnd.ori.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.ori.axis[2][0] +
			fog->surface[1] * backEnd.ori.axis[2][1] + fog->surface[2] * backEnd.ori.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct(backEnd.ori.origin, fog->surface);

		eye_t = DotProduct(backEnd.ori.viewOrigin, fogDepthVector) + fogDepthVector[3];
	}
	else {
		eye_t = 1;	// non-surface fog always has eye inside

		fogDepthVector[0] = fogDepthVector[1] = fogDepthVector[2] = 0.0f;
		fogDepthVector[3] = 1.0f;
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	if (eye_t < 0) {
		eye_outside = qtrue;
	}
	else {
		eye_outside = qfalse;
	}

	fogDistanceVector[3] += 1.0 / 512;

	// calculate density for each point
	for (i = 0, v = tess.xyz[0]; i < tess.numVertexes; i++, v += 4) {
		// calculate the length in fog
		const float s = DotProduct(v, fogDistanceVector) + fogDistanceVector[3];
		float t = DotProduct(v, fogDepthVector) + fogDepthVector[3];

		// partially clipped fogs use the T axis
		if (eye_outside) {
			if (t < 1.0) {
				t = 1.0 / 32;	// point is outside, so no fogging
			}
			else {
				t = 1.0 / 32 + 30.0 / 32 * t / (t - eye_t);	// cut the distance at the fog plane
			}
		}
		else {
			if (t < 0) {
				t = 1.0 / 32;	// point is outside, so no fogging
			}
			else {
				t = 31.0 / 32;
			}
		}

		dstTexCoords[0] = Q_isnan(s) ? 0.0f : s;
		dstTexCoords[1] = Q_isnan(s) ? 0.0f : t;
		dstTexCoords += 2;
	}
}

/*
** RB_CalcEnvironmentTexCoords
*/
void RB_CalcEnvironmentTexCoords(float* dstTexCoords)
{
	vec3_t reflected{};

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	for (int i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, dstTexCoords += 2)
	{
		vec3_t viewer;
		VectorSubtract(backEnd.ori.viewOrigin, v, viewer);
		VectorNormalizeFast(viewer);

		const float d = DotProduct(normal, viewer);

		reflected[0] = normal[0] * 2 * d - viewer[0];
		reflected[1] = normal[1] * 2 * d - viewer[1];
		reflected[2] = normal[2] * 2 * d - viewer[2];

		dstTexCoords[0] = 0.5 + reflected[1] * 0.5;
		dstTexCoords[1] = 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcTurbulentTexCoords
*/
void RB_CalcTurbulentTexCoords(const waveForm_t* wf, float* dstTexCoords)
{
	const float now = wf->phase + tess.shaderTime * wf->frequency;

	for (int i = 0; i < tess.numVertexes; i++, dstTexCoords += 2)
	{
		const float s = dstTexCoords[0];
		const float t = dstTexCoords[1];

		dstTexCoords[0] = s + tr.sinTable[static_cast<int>(((tess.xyz[i][0] + tess.xyz[i][2]) * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE) & FUNCTABLE_MASK] * wf->amplitude;
		dstTexCoords[1] = t + tr.sinTable[static_cast<int>((tess.xyz[i][1] * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE) & FUNCTABLE_MASK] * wf->amplitude;
	}
}

/*
** RB_CalcScaleTexCoords
*/
void RB_CalcScaleTexCoords(const float scale[2], float* dstTexCoords)
{
	for (int i = 0; i < tess.numVertexes; i++, dstTexCoords += 2)
	{
		dstTexCoords[0] *= scale[0];
		dstTexCoords[1] *= scale[1];
	}
}

/*
** RB_CalcScrollTexCoords
*/
void RB_CalcScrollTexCoords(const float scroll_speed[2], float* dstTexCoords)
{
	const float time_scale = tess.shaderTime;

	float adjusted_scroll_s = scroll_speed[0] * time_scale;
	float adjusted_scroll_t = scroll_speed[1] * time_scale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjusted_scroll_s = adjusted_scroll_s - floor(adjusted_scroll_s);
	adjusted_scroll_t = adjusted_scroll_t - floor(adjusted_scroll_t);

	for (int i = 0; i < tess.numVertexes; i++, dstTexCoords += 2)
	{
		dstTexCoords[0] += adjusted_scroll_s;
		dstTexCoords[1] += adjusted_scroll_t;
	}
}

/*
** RB_CalcTransformTexCoords
*/
void RB_CalcTransformTexCoords(const texModInfo_t* tmi, float* dstTexCoords)
{
	for (int i = 0; i < tess.numVertexes; i++, dstTexCoords += 2)
	{
		const float s = dstTexCoords[0];
		const float t = dstTexCoords[1];

		dstTexCoords[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		dstTexCoords[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

/*
** RB_CalcRotateTexCoords
*/
void RB_CalcRotateTexCoords(const float degs_per_second, float* dstTexCoords)
{
	const float time_scale = tess.shaderTime;
	texModInfo_t tmi{};

	const float degs = -degs_per_second * time_scale;
	const int index = degs * (FUNCTABLE_SIZE / 360.0f);

	const float sin_value = tr.sinTable[index & FUNCTABLE_MASK];
	const float cos_value = tr.sinTable[index + FUNCTABLE_SIZE / 4 & FUNCTABLE_MASK];

	tmi.matrix[0][0] = cos_value;
	tmi.matrix[1][0] = -sin_value;
	tmi.translate[0] = 0.5 - 0.5 * cos_value + 0.5 * sin_value;

	tmi.matrix[0][1] = sin_value;
	tmi.matrix[1][1] = cos_value;
	tmi.translate[1] = 0.5 - 0.5 * sin_value - 0.5 * cos_value;

	RB_CalcTransformTexCoords(&tmi, dstTexCoords);
}

/*
** RB_CalcSpecularAlpha
**
** Calculates specular coefficient and places it in the alpha channel
*/
vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

void RB_CalcSpecularAlpha(unsigned char* alphas)
{
	vec3_t reflected{};
	int			b;

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	alphas += 3;

	const int numVertexes = tess.numVertexes;
	for (int i = 0; i < numVertexes; i++, v += 4, normal += 4, alphas += 4) {
		vec3_t lightDir;
		vec3_t viewer;
		if (backEnd.currentEntity &&
			(backEnd.currentEntity->e.hModel || backEnd.currentEntity->e.ghoul2))	//this is a model so we can use world lights instead fake light
		{
			VectorCopy(backEnd.currentEntity->lightDir, lightDir);
		}
		else {
			VectorSubtract(lightOrigin, v, lightDir);
			VectorNormalizeFast(lightDir);
		}
		// calculate the specular color
		const float d = 2 * DotProduct(normal, lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0] * d - lightDir[0];
		reflected[1] = normal[1] * d - lightDir[1];
		reflected[2] = normal[2] * d - lightDir[2];

		VectorSubtract(backEnd.ori.viewOrigin, v, viewer);
		const float ilength = Q_rsqrt(DotProduct(viewer, viewer));
		float l = DotProduct(reflected, viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		}
		else {
			l = l * l;
			l = l * l;
			b = l * 255;
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc
*/
void RB_CalcDiffuseColor(unsigned char* colors)
{
	vec3_t			ambient_light;
	vec3_t			lightDir;
	vec3_t			directed_light;

	const trRefEntity_t* ent = backEnd.currentEntity;
	const int ambient_light_int = ent->ambientLightInt;
	VectorCopy(ent->ambientLight, ambient_light);
	VectorCopy(ent->directedLight, directed_light);
	VectorCopy(ent->lightDir, lightDir);

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	const int numVertexes = tess.numVertexes;
	for (int i = 0; i < numVertexes; i++, v += 4, normal += 4) {
		const float incoming = DotProduct(normal, lightDir);
		if (incoming <= 0) {
			*reinterpret_cast<int*>(&colors[i * 4]) = ambient_light_int;
			continue;
		}
		int j = Q_ftol(ambient_light[0] + incoming * directed_light[0]);
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 0] = j;

		j = Q_ftol(ambient_light[1] + incoming * directed_light[1]);
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 1] = j;

		j = Q_ftol(ambient_light[2] + incoming * directed_light[2]);
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 2] = j;

		colors[i * 4 + 3] = 255;
	}
}

/*
** RB_CalcDiffuseColorEntity
**
** The basic vertex lighting calc * Entity Color
*/
void RB_CalcDiffuseEntityColor(unsigned char* colors)
{
	int				ambient_light_int = 0;
	vec3_t			ambient_light;
	vec3_t			lightDir;
	vec3_t			directed_light;

	if (!backEnd.currentEntity)
	{//error, use the normal lighting
		RB_CalcDiffuseColor(colors);
	}

	const trRefEntity_t* ent = backEnd.currentEntity;
	VectorCopy(ent->ambientLight, ambient_light);
	VectorCopy(ent->directedLight, directed_light);
	VectorCopy(ent->lightDir, lightDir);

	const float r = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
	const float g = backEnd.currentEntity->e.shaderRGBA[1] / 255.0f;
	const float b = backEnd.currentEntity->e.shaderRGBA[2] / 255.0f;

	reinterpret_cast<byte*>(&ambient_light_int)[0] = Q_ftol(r * ent->ambientLight[0]);
	reinterpret_cast<byte*>(&ambient_light_int)[1] = Q_ftol(g * ent->ambientLight[1]);
	reinterpret_cast<byte*>(&ambient_light_int)[2] = Q_ftol(b * ent->ambientLight[2]);
	reinterpret_cast<byte*>(&ambient_light_int)[3] = backEnd.currentEntity->e.shaderRGBA[3];

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	const int numVertexes = tess.numVertexes;

	for (int i = 0; i < numVertexes; i++, v += 4, normal += 4)
	{
		const float incoming = DotProduct(normal, lightDir);
		if (incoming <= 0) {
			*reinterpret_cast<int*>(&colors[i * 4]) = ambient_light_int;
			continue;
		}
		float j = ambient_light[0] + incoming * directed_light[0];
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 0] = Q_ftol(j * r);

		j = ambient_light[1] + incoming * directed_light[1];
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 1] = Q_ftol(j * g);

		j = ambient_light[2] + incoming * directed_light[2];
		if (j > 255) {
			j = 255;
		}
		colors[i * 4 + 2] = Q_ftol(j * b);

		colors[i * 4 + 3] = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

//---------------------------------------------------------
void RB_CalcDisintegrateColors(unsigned char* colors, const colorGen_t rgbGen)
{
	int			i;
	float		dis;
	vec3_t		temp;

	const refEntity_t* ent = &backEnd.currentEntity->e;
	float* v = tess.xyz[0];

	// calculate the burn threshold at the given time, anything that passes the threshold will get burnt
	const float threshold = (backEnd.refdef.time - ent->endTime) * 0.045f; // endTime is really the start time, maybe I should just use a completely meaningless substitute?

	const int numVertexes = tess.numVertexes;

	if (ent->renderfx & RF_DISINTEGRATE1)
	{
		// this handles the blacken and fading out of the regular player model
		for (i = 0; i < numVertexes; i++, v += 4)
		{
			VectorSubtract(backEnd.currentEntity->e.oldorigin, v, temp);

			dis = VectorLengthSquared(temp);

			if (dis < threshold * threshold)
			{
				// completely disintegrated
				colors[i * 4 + 3] = 0x00;
			}
			else if (dis < threshold * threshold + 60)
			{
				// blacken before fading out
				colors[i * 4 + 0] = 0x0;
				colors[i * 4 + 1] = 0x0;
				colors[i * 4 + 2] = 0x0;
				colors[i * 4 + 3] = 0xff;
			}
			else if (dis < threshold * threshold + 150)
			{
				// darken more
				if (rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
				{
					colors[i * 4 + 0] = backEnd.currentEntity->e.shaderRGBA[0] * 0x6f / 255.0f;
					colors[i * 4 + 1] = backEnd.currentEntity->e.shaderRGBA[1] * 0x6f / 255.0f;
					colors[i * 4 + 2] = backEnd.currentEntity->e.shaderRGBA[2] * 0x6f / 255.0f;
				}
				else
				{
					colors[i * 4 + 0] = 0x6f;
					colors[i * 4 + 1] = 0x6f;
					colors[i * 4 + 2] = 0x6f;
				}
				colors[i * 4 + 3] = 0xff;
			}
			else if (dis < threshold * threshold + 180)
			{
				// darken at edge of burn
				if (rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
				{
					colors[i * 4 + 0] = backEnd.currentEntity->e.shaderRGBA[0] * 0xaf / 255.0f;
					colors[i * 4 + 1] = backEnd.currentEntity->e.shaderRGBA[1] * 0xaf / 255.0f;
					colors[i * 4 + 2] = backEnd.currentEntity->e.shaderRGBA[2] * 0xaf / 255.0f;
				}
				else
				{
					colors[i * 4 + 0] = 0xaf;
					colors[i * 4 + 1] = 0xaf;
					colors[i * 4 + 2] = 0xaf;
				}
				colors[i * 4 + 3] = 0xff;
			}
			else
			{
				// not burning at all yet
				if (rgbGen == CGEN_LIGHTING_DIFFUSE_ENTITY)
				{
					colors[i * 4 + 0] = backEnd.currentEntity->e.shaderRGBA[0];
					colors[i * 4 + 1] = backEnd.currentEntity->e.shaderRGBA[1];
					colors[i * 4 + 2] = backEnd.currentEntity->e.shaderRGBA[2];
				}
				else
				{
					colors[i * 4 + 0] = 0xff;
					colors[i * 4 + 1] = 0xff;
					colors[i * 4 + 2] = 0xff;
				}
				colors[i * 4 + 3] = 0xff;
			}
		}
	}
	else if (ent->renderfx & RF_DISINTEGRATE2)
	{
		// this handles the glowing, burning bit that scales away from the model
		for (i = 0; i < numVertexes; i++, v += 4)
		{
			VectorSubtract(backEnd.currentEntity->e.oldorigin, v, temp);

			dis = VectorLengthSquared(temp);

			if (dis < threshold * threshold)
			{
				// done burning
				colors[i * 4 + 0] = 0x00;
				colors[i * 4 + 1] = 0x00;
				colors[i * 4 + 2] = 0x00;
				colors[i * 4 + 3] = 0x00;
			}
			else
			{
				// still full burn
				colors[i * 4 + 0] = 0xff;
				colors[i * 4 + 1] = 0xff;
				colors[i * 4 + 2] = 0xff;
				colors[i * 4 + 3] = 0xff;
			}
		}
	}
}

//---------------------------------------------------------
void RB_CalcDisintegrateVertDeform()
{
	float* xyz = (float*)tess.xyz;
	float* normal = (float*)tess.normal;

	if (backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE2)
	{
		const float	threshold = (backEnd.refdef.time - backEnd.currentEntity->e.endTime) * 0.045f;

		for (int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			vec3_t temp;
			VectorSubtract(backEnd.currentEntity->e.oldorigin, xyz, temp);

			const float scale = VectorLengthSquared(temp);

			if (scale < threshold * threshold)
			{
				xyz[0] += normal[0] * 2.0f;
				xyz[1] += normal[1] * 2.0f;
				xyz[2] += normal[2] * 0.5f;
			}
			else if (scale < threshold * threshold + 50)
			{
				xyz[0] += normal[0] * 1.0f;
				xyz[1] += normal[1] * 1.0f;
			}
		}
	}
}