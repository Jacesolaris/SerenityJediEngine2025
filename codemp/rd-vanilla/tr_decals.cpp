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

#include "tr_local.h"

#define	MAX_VERTS_ON_DECAL_POLY	10
#define	MAX_DECAL_POLYS			500

using decalPoly_t = struct decalPoly_s
{
	int					time;
	int					fadetime;
	qhandle_t			shader;
	float				color[4];
	poly_t				poly;
	polyVert_t			verts[MAX_VERTS_ON_DECAL_POLY];
};

enum
{
	DECALPOLY_TYPE_NORMAL,
	DECALPOLY_TYPE_FADE,
	DECALPOLY_TYPE_MAX
};

#define		DECAL_FADE_TIME		1000

decalPoly_t* RE_AllocDecal(const int type);

static decalPoly_t	re_decalPolys[DECALPOLY_TYPE_MAX][MAX_DECAL_POLYS];

static int			re_decalPolyHead[DECALPOLY_TYPE_MAX];
static int			re_decalPolyTotal[DECALPOLY_TYPE_MAX];

/*
===================
RE_ClearDecals

This is called to remove all decals from the world
===================
*/
void RE_ClearDecals() {
	memset(re_decalPolys, 0, sizeof re_decalPolys);
	memset(re_decalPolyHead, 0, sizeof re_decalPolyHead);
	memset(re_decalPolyTotal, 0, sizeof re_decalPolyTotal);
}

void R_InitDecals() {
	RE_ClearDecals();
}

static void RE_FreeDecal(const int type, const int index) {
	if (!re_decalPolys[type][index].time)
		return;

	if (type == DECALPOLY_TYPE_NORMAL) {
		decalPoly_t* fade = RE_AllocDecal(DECALPOLY_TYPE_FADE);

		memcpy(fade, &re_decalPolys[type][index], sizeof(decalPoly_t));

		fade->time = tr.refdef.time;
		fade->fadetime = tr.refdef.time + DECAL_FADE_TIME;
	}

	re_decalPolys[type][index].time = 0;

	re_decalPolyTotal[type]--;
}

/*
===================
RE_AllocDecal

Will allways succeed, even if it requires freeing an old active mark
===================
*/
decalPoly_t* RE_AllocDecal(const int type) {
	// See if the cvar changed
	if (re_decalPolyTotal[type] > r_markcount->integer)
		RE_ClearDecals();

	decalPoly_t* le = &re_decalPolys[type][re_decalPolyHead[type]];

	// If it has no time its the first occasion its been used
	if (le->time) {
		if (le->time != tr.refdef.time) {
			int i = re_decalPolyHead[type];

			// since we are killing one that existed before, make sure we
			// kill all the other marks that belong to the group
			do {
				i++;
				if (i >= r_markcount->integer)
					i = 0;

				// Break out on the first one thats not part of the group
				if (re_decalPolys[type][i].time != le->time)
					break;

				RE_FreeDecal(type, i);
			} while (i != re_decalPolyHead[type]);

			RE_FreeDecal(type, re_decalPolyHead[type]);
		}
		else
			RE_FreeDecal(type, re_decalPolyHead[type]);
	}

	memset(le, 0, sizeof(decalPoly_t));
	le->time = tr.refdef.time;

	re_decalPolyTotal[type]++;

	// Move on to the next decal poly and wrap around if need be
	re_decalPolyHead[type]++;
	if (re_decalPolyHead[type] >= r_markcount->integer)
		re_decalPolyHead[type] = 0;

	return le;
}

/*
=================
RE_AddDecalToScene

origin should be a point within a unit of the plane
dir should be the plane normal

temporary marks will not be stored or randomly oriented, but immediately
passed to the renderer.
=================
*/
#define	MAX_DECAL_FRAGMENTS	128
#define	MAX_DECAL_POINTS		384

void RE_AddDecalToScene(const qhandle_t decalShader, const vec3_t origin, const vec3_t dir, const float orientation, const float red, const float green, const float blue, const float alpha, qboolean alphaFade, const float radius, const qboolean temporary)
{
	matrix3_t		axis{};
	vec3_t			original_points[4]{};
	byte			colors[4]{};
	int				i, j;
	markFragment_t	markFragments[MAX_DECAL_FRAGMENTS], * mf;
	vec3_t			markPoints[MAX_DECAL_POINTS]{};
	vec3_t			projection;

	assert(decalShader);

	if (r_markcount->integer <= 0 && !temporary)
		return;

	if (radius <= 0)
		Com_Error(ERR_FATAL, "RE_AddDecalToScene:  called with <= 0 radius");

	// create the texture axis
	VectorNormalize2(dir, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], orientation);
	CrossProduct(axis[0], axis[2], axis[1]);

	const float tex_coord_scale = 0.5 * 1.0 / radius;

	// create the full polygon
	for (i = 0; i < 3; i++)
	{
		original_points[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
		original_points[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
		original_points[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
		original_points[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	// get the fragments
	VectorScale(dir, -20, projection);
	const int numFragments = R_MarkFragments(4, original_points,
		projection, MAX_DECAL_POINTS, markPoints[0],
		MAX_DECAL_FRAGMENTS, markFragments);

	colors[0] = red * 255;
	colors[1] = green * 255;
	colors[2] = blue * 255;
	colors[3] = alpha * 255;

	for (i = 0, mf = markFragments; i < numFragments; i++, mf++)
	{
		polyVert_t* v;
		polyVert_t	verts[MAX_VERTS_ON_DECAL_POLY]{};

		// we have an upper limit on the complexity of polygons
		// that we store persistantly
		if (mf->numPoints > MAX_VERTS_ON_DECAL_POLY)
			mf->numPoints = MAX_VERTS_ON_DECAL_POLY;

		for (j = 0, v = verts; j < mf->numPoints; j++, v++)
		{
			vec3_t		delta;

			VectorCopy(markPoints[mf->firstPoint + j], v->xyz);

			VectorSubtract(v->xyz, origin, delta);
			v->st[0] = 0.5 + DotProduct(delta, axis[1]) * tex_coord_scale;
			v->st[1] = 0.5 + DotProduct(delta, axis[2]) * tex_coord_scale;

			for (int k = 0; k < 4; k++)
				v->modulate[k] = colors[k];
		}

		// if it is a temporary (shadow) mark, add it immediately and forget about it
		if (temporary)
		{
			RE_AddPolyToScene(decalShader, mf->numPoints, verts, 1);
			continue;
		}

		// otherwise save it persistantly
		decalPoly_t* decal = RE_AllocDecal(DECALPOLY_TYPE_NORMAL);
		decal->time = tr.refdef.time;
		decal->shader = decalShader;
		decal->poly.numVerts = mf->numPoints;
		decal->color[0] = red;
		decal->color[1] = green;
		decal->color[2] = blue;
		decal->color[3] = alpha;
		memcpy(decal->verts, verts, mf->numPoints * sizeof verts[0]);
	}
}

/*
===============
R_AddDecals
===============
*/
void R_AddDecals(void)
{
	static int  lastMarkCount = -1;

	if (r_markcount->integer != lastMarkCount) {
		if (lastMarkCount != -1)
			RE_ClearDecals();

		lastMarkCount = r_markcount->integer;
	}

	if (r_markcount->integer <= 0)
		return;

	for (int type = DECALPOLY_TYPE_NORMAL; type < DECALPOLY_TYPE_MAX; type++)
	{
		int decal_poly = re_decalPolyHead[type];

		do
		{
			decalPoly_t* p = &re_decalPolys[type][decal_poly];

			if (p->time)
			{
				if (p->fadetime)
				{
					// fade all marks out with time
					const int t = tr.refdef.time - p->time;
					if (t < DECAL_FADE_TIME)
					{
						const float fade = 255.0f * (1.0f - static_cast<float>(t) / DECAL_FADE_TIME);

						for (int j = 0; j < p->poly.numVerts; j++)
						{
							p->verts[j].modulate[3] = fade;
						}

						RE_AddPolyToScene(p->shader, p->poly.numVerts, p->verts, 1);
					}
					else
					{
						RE_FreeDecal(type, decal_poly);
					}
				}
				else
				{
					RE_AddPolyToScene(p->shader, p->poly.numVerts, p->verts, 1);
				}
			}

			decal_poly++;
			if (decal_poly >= r_markcount->integer)
			{
				decal_poly = 0;
			}
		} while (decal_poly != re_decalPolyHead[type]);
	}
}