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

// cmodel.c -- model loading
#include "cm_local.h"
#include "qcommon/qfiles.h"

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits(cplane_t* out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j = 0; j < 3; j++) {
		if (out->normal[j] < 0) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)

clipMap_t cmg; //rwwRMG - changed from cm
int c_pointcontents;
int c_traces, c_brush_traces, c_patch_traces;

byte* cmod_base;

#ifndef BSPC
cvar_t* cm_noAreas;
cvar_t* cm_noCurves;
cvar_t* cm_playerCurveClip;
cvar_t* cm_extraVerbose;
#endif

cmodel_t box_model;
cplane_t* box_planes;
cbrush_t* box_brush;

void CM_InitBoxHull();
void CM_FloodAreaConnections(clipMap_t& cm);

//rwwRMG - added:
clipMap_t SubBSP[MAX_SUB_BSP];
int NumSubBSP, TotalSubModels;

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

using MaterialReplace = struct
{
	const char* shader;
	const int materialNum;
};

static MaterialReplace replaceMaterials[] =
{
	{"textures/siege/siege2sand", MATERIAL_SAND},
	{"textures/siege1/terrain_0", MATERIAL_SNOW},
	{"textures/siege1/terrain_1", MATERIAL_SNOW},
	{"textures/siege1/terrain_0to1", MATERIAL_SNOW},
	{"textures/kor/terrain_0", MATERIAL_SAND},
	{"textures/kor/terrain_1", MATERIAL_SAND},
	{"textures/kor/terrain_2", MATERIAL_SAND},
	{"textures/kor/terrain_0to1", MATERIAL_SAND},
	{"textures/kor/terrain_0to2", MATERIAL_SAND},
	{"textures/kor/terrain_1to2", MATERIAL_SAND},
	{"textures/korriban1/terrain_0", MATERIAL_SAND},
	{"textures/korriban1/terrain_1", MATERIAL_SAND},
	{"textures/korriban1/terrain_2", MATERIAL_SAND},
	{"textures/korriban1/terrain_3", MATERIAL_SAND},
	{"textures/korriban1/terrain_0to1", MATERIAL_SAND},
	{"textures/korriban1/terrain_0to2", MATERIAL_SAND},
	{"textures/korriban1/terrain_0to3", MATERIAL_SAND},
	{"textures/korriban1/terrain_1to2", MATERIAL_SAND},
	{"textures/korriban1/terrain_1to3", MATERIAL_SAND},
	{"textures/korriban1/terrain_2to3", MATERIAL_SAND},
	{"textures/korriban1/terrain_0to1", MATERIAL_SAND},
	{"textures/korterra1/terrain_0", MATERIAL_SAND},
	{"textures/korterra1/terrain_1", MATERIAL_SAND},
	{"textures/korterra1/terrain_2", MATERIAL_SAND},
	{"textures/korterra1/terrain_0to1", MATERIAL_SAND},
	{"textures/korterra1/terrain_0to2", MATERIAL_SAND},
	{"textures/korterra1/terrain_1to2", MATERIAL_SAND},
	{"textures/terradesert/terrain_0", MATERIAL_SAND},
	{"textures/terradesert/terrain_1", MATERIAL_SAND},
	{"textures/terradesert/terrain_0to1", MATERIAL_SAND},
};

/*
=================
CMod_LoadShaders
=================
*/
static void CMod_LoadShaders(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dshader_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
	{
		Com_Error(ERR_DROP, "CMod_LoadShaders: funny lump size");
	}
	const int count = l->filelen / sizeof * in;

	if (count < 1)
	{
		Com_Error(ERR_DROP, "Map with no shaders");
	}
	cm.shaders = static_cast<CCMShader*>(Hunk_Alloc((1 + count) * sizeof * cm.shaders, h_high));
	cm.numShaders = count;

	CCMShader* out = cm.shaders;
	for (int i = 0; i < count; i++, in++, out++)
	{
		Q_strncpyz(out->shader, in->shader, MAX_QPATH);
		out->contentFlags = LittleLong in->contentFlags;
		out->surfaceFlags = LittleLong in->surfaceFlags;
		for (int j = 0; j <= ARRAY_LEN(replaceMaterials); j++)
		{
			if (!Q_stricmp(out->shader, replaceMaterials[j].shader))
			{
				out->surfaceFlags = replaceMaterials->materialNum;
			}
		}
	}
}

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels(const lump_t* l, clipMap_t& cm)
{
	cmodel_t* out = nullptr;
	int j;

	auto in = reinterpret_cast<dmodel_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	const int count = l->filelen / sizeof * in;

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no models");
	cm.cmodels = static_cast<cmodel_s*>(Hunk_Alloc(count * sizeof * cm.cmodels, h_high));
	cm.numSubModels = count;

	if (count > MAX_SUBMODELS)
	{
		Com_Error(ERR_DROP, "MAX_SUBMODELS exceeded");
	}

	for (int i = 0; i < count; i++, in++, out++)
	{
		out = &cm.cmodels[i];

		for (j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat in->mins[j] - 1;
			out->maxs[j] = LittleFloat in->maxs[j] + 1;
		}

		//rwwRMG - sof2 doesn't have to add this &cm == &cmg check.
		//Are they getting leaf data elsewhere? (the reason this needs to be done is
		//in sub bsp instances the first brush model isn't necessary a world model and might be
		//real architecture)
		if (i == 0 && &cm == &cmg)
		{
			out->firstNode = 0;
			continue; // world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->firstNode = -1;

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong in->numBrushes;
		auto indexes = static_cast<int*>(Hunk_Alloc(out->leaf.numLeafBrushes * 4, h_high));
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for (j = 0; j < out->leaf.numLeafBrushes; j++)
		{
			indexes[j] = LittleLong in->firstBrush + j;
		}

		out->leaf.numLeafSurfaces = LittleLong in->numSurfaces;
		indexes = static_cast<int*>(Hunk_Alloc(out->leaf.numLeafSurfaces * 4, h_high));
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for (j = 0; j < out->leaf.numLeafSurfaces; j++)
		{
			indexes[j] = LittleLong in->firstSurface + j;
		}
	}
}

/*
=================
CMod_LoadNodes

=================
*/
static void CMod_LoadNodes(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dnode_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadNodes: funny lump size");
	const int count = l->filelen / sizeof * in;

	if (count < 1)
		Com_Error(ERR_DROP, "Map has no nodes");
	cm.nodes = static_cast<cNode_t*>(Hunk_Alloc(count * sizeof * cm.nodes, h_high));
	cm.numNodes = count;

	cNode_t* out = cm.nodes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->plane = cm.planes + LittleLong in->planeNum;
		for (int j = 0; j < 2; j++)
		{
			const int child = in->children[j];
			out->children[j] = child;
		}
	}
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush(cbrush_t* b)
{
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}

/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dbrush_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
	{
		Com_Error(ERR_DROP, "CMod_LoadBrushes: funny lump size");
	}
	const int count = l->filelen / sizeof * in;

	cm.brushes = static_cast<cbrush_t*>(Hunk_Alloc((BOX_BRUSHES + count) * sizeof * cm.brushes, h_high));
	cm.numBrushes = count;

	cbrush_t* out = cm.brushes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->sides = cm.brushsides + LittleLong in->firstSide;
		out->numsides = LittleLong in->numSides;

		out->shaderNum = LittleLong in->shaderNum;
		if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders)
		{
			Com_Error(ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum);
		}
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush(out);
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dleaf_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadLeafs: funny lump size");
	const int count = l->filelen / sizeof * in;

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no leafs");

	cm.leafs = static_cast<cLeaf_t*>(Hunk_Alloc((BOX_LEAFS + count) * sizeof * cm.leafs, h_high));
	cm.numLeafs = count;

	cLeaf_t* out = cm.leafs;
	for (int i = 0; i < count; i++, in++, out++)
	{
		out->cluster = LittleLong in->cluster;
		out->area = LittleLong in->area;
		out->firstLeafBrush = LittleLong in->firstLeafBrush;
		out->numLeafBrushes = LittleLong in->numLeafBrushes;
		out->firstLeafSurface = LittleLong in->firstLeafSurface;
		out->numLeafSurfaces = LittleLong in->numLeafSurfaces;

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
		if (out->area >= cm.numAreas)
			cm.numAreas = out->area + 1;
	}

	cm.areas = static_cast<cArea_t*>(Hunk_Alloc(cm.numAreas * sizeof * cm.areas, h_high));
	cm.areaPortals = static_cast<int*>(Hunk_Alloc(cm.numAreas * cm.numAreas * sizeof * cm.areaPortals, h_high));
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dplane_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadPlanes: funny lump size");
	const int count = l->filelen / sizeof * in;

	if (count < 1)
		Com_Error(ERR_DROP, "Map with no planes");
	cm.planes = static_cast<cplane_s*>(Hunk_Alloc((BOX_PLANES + count) * sizeof * cm.planes, h_high));
	cm.numPlanes = count;

	cplane_t* out = cm.planes;

	for (int i = 0; i < count; i++, in++, out++)
	{
		int bits = 0;
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat in->normal[j];
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat in->dist;
		out->type = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<int*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadLeafBrushes: funny lump size");
	const int count = l->filelen / sizeof * in;

	cm.leafbrushes = static_cast<int*>(Hunk_Alloc((count + BOX_BRUSHES) * sizeof * cm.leafbrushes, h_high));
	cm.numLeafBrushes = count;

	int* out = cm.leafbrushes;

	for (int i = 0; i < count; i++, in++, out++)
	{
		*out = LittleLong * in;
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
static void CMod_LoadLeafSurfaces(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<int*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
		Com_Error(ERR_DROP, "CMod_LoadLeafSurfaces: funny lump size");
	const int count = l->filelen / sizeof * in;

	cm.leafsurfaces = static_cast<int*>(Hunk_Alloc(count * sizeof * cm.leafsurfaces, h_high));
	cm.numLeafSurfaces = count;

	int* out = cm.leafsurfaces;

	for (int i = 0; i < count; i++, in++, out++)
	{
		*out = LittleLong * in;
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides(const lump_t* l, clipMap_t& cm)
{
	auto in = reinterpret_cast<dbrushside_t*>(cmod_base + l->fileofs);
	if (l->filelen % sizeof * in)
	{
		Com_Error(ERR_DROP, "CMod_LoadBrushSides: funny lump size");
	}
	const int count = l->filelen / sizeof * in;

	cm.brushsides = static_cast<cbrushside_t*>(Hunk_Alloc((BOX_SIDES + count) * sizeof * cm.brushsides, h_high));
	cm.numBrushSides = count;

	cbrushside_t* out = cm.brushsides;

	for (int i = 0; i < count; i++, in++, out++)
	{
		const int num = in->planeNum;
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong in->shaderNum;
		if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders)
		{
			Com_Error(ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum);
		}
	}
}

/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString(const lump_t* l, clipMap_t& cm, const char* name)
{
	fileHandle_t h;
	char ent_name[MAX_QPATH];

	// Attempt to load entities from an external .ent file if available
	Q_strncpyz(ent_name, name, sizeof ent_name);
	const size_t ent_name_len = strlen(ent_name);
	ent_name[ent_name_len - 3] = 'e';
	ent_name[ent_name_len - 2] = 'n';
	ent_name[ent_name_len - 1] = 't';
	const int i_entity_file_len = FS_FOpenFileRead(ent_name, &h, qfalse);
	if (h)
	{
		cm.entityString = static_cast<char*>(Hunk_Alloc(i_entity_file_len + 1, h_high));
		cm.numEntityChars = i_entity_file_len + 1;
		FS_Read(cm.entityString, i_entity_file_len, h);
		FS_FCloseFile(h);
		cm.entityString[i_entity_file_len] = '\0';
		Com_Printf("Loaded entities from %s\n", ent_name);
		return;
	}

	cm.entityString = static_cast<char*>(Hunk_Alloc(l->filelen, h_high));
	cm.numEntityChars = l->filelen;
	Com_Memcpy(cm.entityString, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadVisibility
=================
*/
#define	VIS_HEADER	8

static void CMod_LoadVisibility(const lump_t* l, clipMap_t& cm)
{
	const int len = l->filelen;
	if (!len)
	{
		cm.clusterBytes = cm.numClusters + 31 & ~31;
		cm.visibility = static_cast<unsigned char*>(Hunk_Alloc(cm.clusterBytes, h_high));
		Com_Memset(cm.visibility, 255, cm.clusterBytes);
		return;
	}
	byte* buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility = static_cast<unsigned char*>(Hunk_Alloc(len, h_high));
	cm.numClusters = LittleLong reinterpret_cast<int*>(buf)[0];
	cm.clusterBytes = LittleLong reinterpret_cast<int*>(buf)[1];
	Com_Memcpy(cm.visibility, buf + VIS_HEADER, len - VIS_HEADER);
}

//==================================================================

/*
=================
CMod_LoadPatches
=================
*/
#define	MAX_PATCH_VERTS		1024

static void CMod_LoadPatches(const lump_t* surfs, const lump_t* verts, clipMap_t& cm)
{
	int count;
	cPatch_t* patch;
	vec3_t points[MAX_PATCH_VERTS]{};

	auto in = reinterpret_cast<dsurface_t*>(cmod_base + surfs->fileofs);
	if (surfs->filelen % sizeof * in)
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
	cm.numSurfaces = count = surfs->filelen / sizeof * in;
	cm.surfaces = static_cast<cPatch_t**>(Hunk_Alloc(cm.numSurfaces * sizeof cm.surfaces[0], h_high));

	const auto dv = reinterpret_cast<drawVert_t*>(cmod_base + verts->fileofs);
	if (verts->filelen % sizeof * dv)
		Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for (int i = 0; i < count; i++, in++)
	{
		if (LittleLong in->surfaceType != MST_PATCH)
		{
			continue; // ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cm.surfaces[i] = patch = static_cast<cPatch_t*>(Hunk_Alloc(sizeof * patch, h_high));

		// load the full drawverts onto the stack
		const int width = in->patchWidth;
		const int height = in->patchHeight;
		const int c = width * height;
		if (c > MAX_PATCH_VERTS)
		{
			Com_Error(ERR_DROP, "ParseMesh: MAX_PATCH_VERTS");
		}

		drawVert_t* dv_p = dv + LittleLong in->firstVert;
		for (int j = 0; j < c; j++, dv_p++)
		{
			points[j][0] = LittleFloat dv_p->xyz[0];
			points[j][1] = LittleFloat dv_p->xyz[1];
			points[j][2] = LittleFloat dv_p->xyz[2];
		}

		const int shaderNum = in->shaderNum;
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide(width, height, points);
	}
}

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void* gpvCachedMapDiskImage = nullptr;
char gsCachedMapDiskImage[MAX_QPATH];
qboolean gbUsingCachedMapDataRightNow = qfalse;
// if true, signifies that you can't delete this at the moment!! (used during z_malloc()-fail recovery attempt)

// called in response to a "devmapbsp blah" or "devmapall blah" command, do NOT use inside CM_Load unless you pass in qtrue
//
// new bool return used to see if anything was freed, used during z_malloc failure re-try
//
qboolean CM_DeleteCachedMap(const qboolean b_guaranteed_ok_to_delete)
{
	qboolean b_actually_freed_something = qfalse;

	if (b_guaranteed_ok_to_delete || !gbUsingCachedMapDataRightNow)
	{
		// dump cached disk image...
		//
		if (gpvCachedMapDiskImage)
		{
			Z_Free(gpvCachedMapDiskImage);
			gpvCachedMapDiskImage = nullptr;

			b_actually_freed_something = qtrue;
		}
		gsCachedMapDiskImage[0] = '\0';

		// force map loader to ignore cached internal BSP structures for next level CM_LoadMap() call...
		//
		cmg.name[0] = '\0';
	}

	return b_actually_freed_something;
}

static void CM_LoadMap_Actual(const char* name, const qboolean clientload, int* checksum, clipMap_t& cm)
{
	//rwwRMG - function needs heavy modification
	int* buf;
	dheader_t header;
	static unsigned last_checksum;
	char orig_name[MAX_OSPATH];

	if (!name || !name[0])
	{
		Com_Error(ERR_DROP, "CM_LoadMap: NULL name");
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get("cm_playerCurveClip", "1", CVAR_ARCHIVE_ND | CVAR_CHEAT);
	cm_extraVerbose = Cvar_Get("cm_extraVerbose", "0", CVAR_TEMP);
#endif
	Com_DPrintf("CM_LoadMap( %s, %i )\n", name, clientload);

	if (strcmp(cm.name, name) == 0 && clientload)
	{
		if (checksum)
			*checksum = last_checksum;
		return;
	}

	strcpy(orig_name, name);

	if (&cm == &cmg)
	{
		// free old stuff
		CM_ClearMap();
		CM_ClearLevelPatches();
	}

	// free old stuff
	Com_Memset(&cm, 0, sizeof cm);

	if (!name[0])
	{
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = static_cast<cmodel_s*>(Hunk_Alloc(sizeof * cm.cmodels, h_high));
		if (checksum)
			*checksum = 0;
		return;
	}

	//
	// load the file
	//
	//rww - Doesn't this sort of defeat the purpose? We're clearing it even if the map is the same as the last one!
	//Not touching it though in case I'm just overlooking something.
	if (gpvCachedMapDiskImage && &cm == &cmg) // MP code: this'll only be NZ if we got an ERR_DROP during last map load,
	{
		//	so it's really just a safety measure.
		Z_Free(gpvCachedMapDiskImage);
		gpvCachedMapDiskImage = nullptr;
	}

#ifndef BSPC
	//
	// load the file into a buffer that we either discard as usual at the bottom, or if we've got enough memory
	//	then keep it long enough to save the renderer re-loading it (if not dedicated server),
	//	then discard it after that...
	//
	buf = nullptr;
	fileHandle_t h;
	const int i_bsp_len = FS_FOpenFileRead(name, &h, qfalse);
	if (h)
	{
		void* new_buff = Z_Malloc(i_bsp_len, TAG_BSP_DISKIMAGE);
		FS_Read(new_buff, i_bsp_len, h);
		FS_FCloseFile(h);

		buf = static_cast<int*>(new_buff); // so the rest of the code works as normal
		if (&cm == &cmg)
		{
			gpvCachedMapDiskImage = new_buff;
			new_buff = nullptr;
		}

		// carry on as before...
		//
	}
#else
	const int i_bsp_len = LoadQuakeFile((quakefile_t*)name, (void**)&buf);
#endif

	if (!buf)
	{
		Com_Error(ERR_DROP, "Couldn't load %s", name);
	}

	last_checksum = LittleLong Com_BlockChecksum(buf, i_bsp_len);
	if (checksum)
		*checksum = last_checksum;

	header = *reinterpret_cast<dheader_t*>(buf);
	for (size_t i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		reinterpret_cast<int*>(&header)[i] = LittleLong reinterpret_cast<int*>(&header)[i];
	}

	if (header.version != BSP_VERSION)
	{
		Z_Free(gpvCachedMapDiskImage);
		gpvCachedMapDiskImage = nullptr;

		Com_Error(ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
			, name, header.version, BSP_VERSION);
	}

	cmod_base = reinterpret_cast<byte*>(buf);

	// load into heap
	CMod_LoadShaders(&header.lumps[LUMP_SHADERS], cm);
	CMod_LoadLeafs(&header.lumps[LUMP_LEAFS], cm);
	CMod_LoadLeafBrushes(&header.lumps[LUMP_LEAFBRUSHES], cm);
	CMod_LoadLeafSurfaces(&header.lumps[LUMP_LEAFSURFACES], cm);
	CMod_LoadPlanes(&header.lumps[LUMP_PLANES], cm);
	CMod_LoadBrushSides(&header.lumps[LUMP_BRUSHSIDES], cm);
	CMod_LoadBrushes(&header.lumps[LUMP_BRUSHES], cm);
	CMod_LoadSubmodels(&header.lumps[LUMP_MODELS], cm);
	CMod_LoadNodes(&header.lumps[LUMP_NODES], cm);
	CMod_LoadEntityString(&header.lumps[LUMP_ENTITIES], cm, name);
	CMod_LoadVisibility(&header.lumps[LUMP_VISIBILITY], cm);
	CMod_LoadPatches(&header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], cm);

	TotalSubModels += cm.numSubModels;

	if (&cm == &cmg)
	{
		// Load in the shader text - return instantly if already loaded
		CM_InitBoxHull();
	}

#ifndef BSPC	// I hope we can lose this crap soon
	//
	// if we've got enough memory, and it's not a dedicated-server, then keep the loaded map binary around
	//	for the renderer to chew on... (but not if this gets ported to a big-endian machine, because some of the
	//	map data will have been Little-Long'd, but some hasn't).
	//
	if (Sys_LowPhysicalMemory()
		|| com_dedicated->integer
		//		|| we're on a big-endian machine
		)
	{
		Z_Free(gpvCachedMapDiskImage);
		gpvCachedMapDiskImage = nullptr;
	}
	else
	{
		// ... do nothing, and let the renderer free it after it's finished playing with it...
		//
	}
#else
	FS_FreeFile(buf);
#endif

	CM_FloodAreaConnections(cm);

	// allow this to be cached if it is loaded by the server
	if (!clientload)
	{
		Q_strncpyz(cm.name, orig_name, sizeof cm.name);
	}
}

// need a wrapper function around this because of multiple returns, need to ensure bool is correct...
//
void CM_LoadMap(const char* name, const qboolean clientload, int* checksum)
{
	gbUsingCachedMapDataRightNow = qtrue; // !!!!!!!!!!!!!!!!!!

	CM_LoadMap_Actual(name, clientload, checksum, cmg);

	gbUsingCachedMapDataRightNow = qfalse; // !!!!!!!!!!!!!!!!!!
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
	Com_Memset(&cmg, 0, sizeof cmg);
	CM_ClearLevelPatches();

	for (int i = 0; i < NumSubBSP; i++)
	{
		memset(&SubBSP[i], 0, sizeof SubBSP[0]);
	}
	NumSubBSP = 0;
	TotalSubModels = 0;
}

/*
==================
CM_clip_handleToModel
==================
*/
cmodel_t* CM_clip_handleToModel(const clipHandle_t handle, clipMap_t** clip_map)
{
	if (handle < 0)
	{
		Com_Error(ERR_DROP, "CM_clip_handleToModel: bad handle %i", handle);
	}
	if (handle < cmg.numSubModels)
	{
		if (clip_map)
		{
			*clip_map = &cmg;
		}
		return &cmg.cmodels[handle];
	}
	if (handle == BOX_MODEL_HANDLE)
	{
		if (clip_map)
		{
			*clip_map = &cmg;
		}
		return &box_model;
	}

	int count = cmg.numSubModels;
	for (int i = 0; i < NumSubBSP; i++)
	{
		if (handle < count + SubBSP[i].numSubModels)
		{
			if (clip_map)
			{
				*clip_map = &SubBSP[i];
			}
			return &SubBSP[i].cmodels[handle - count];
		}
		count += SubBSP[i].numSubModels;
	}

	if (handle < MAX_SUBMODELS)
	{
		Com_Error(ERR_DROP, "CM_clip_handleToModel: bad handle (count: %i) < (handle: %i) < (max: %i)",
			cmg.numSubModels, handle, MAX_SUBMODELS);
	}
	Com_Error(ERR_DROP, "CM_clip_handleToModel: bad handle %i", handle + MAX_SUBMODELS);
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t CM_InlineModel(const int index)
{
	if (index < 0 || index >= TotalSubModels)
	{
		Com_Error(ERR_DROP, "CM_InlineModel: bad number: %d >= %d (may need to re-BSP map?)", index, TotalSubModels);
	}
	return index;
}

int CM_NumInlineModels()
{
	return cmg.numSubModels;
}

char* CM_EntityString()
{
	return cmg.entityString;
}

char* CM_SubBSPEntityString(const int index)
{
	return SubBSP[index].entityString;
}

int CM_LeafCluster(const int leafnum)
{
	if (leafnum < 0 || leafnum >= cmg.numLeafs)
	{
		Com_Error(ERR_DROP, "CM_LeafCluster: bad number");
	}
	return cmg.leafs[leafnum].cluster;
}

int CM_LeafArea(const int leafnum)
{
	if (leafnum < 0 || leafnum >= cmg.numLeafs)
	{
		Com_Error(ERR_DROP, "CM_LeafArea: bad number");
	}
	return cmg.leafs[leafnum].area;
}

//=======================================================================

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull()
{
	box_planes = &cmg.planes[cmg.numPlanes];

	box_brush = &cmg.brushes[cmg.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cmg.brushsides + cmg.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.firstNode = -1;
	box_model.leaf.numLeafBrushes = 1;
	//	box_model.leaf.firstLeafBrush = cmg.numBrushes;
	box_model.leaf.firstLeafBrush = cmg.numLeafBrushes;
	cmg.leafbrushes[cmg.numLeafBrushes] = cmg.numBrushes;

	for (int i = 0; i < 6; i++)
	{
		const int side = i & 1;

		// brush sides
		cbrushside_t* s = &cmg.brushsides[cmg.numBrushSides + i];
		s->plane = cmg.planes + (cmg.numPlanes + i * 2 + side);
		s->shaderNum = cmg.numShaders;

		// planes
		cplane_t* p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + (i >> 1);
		p->signbits = 0;
		VectorClear(p->normal);
		p->normal[i >> 1] = -1;

		SetPlaneSignbits(p);
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel(const vec3_t mins, const vec3_t maxs, const int capsule)
{
	VectorCopy(mins, box_model.mins);
	VectorCopy(maxs, box_model.maxs);

	if (capsule)
	{
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy(mins, box_brush->bounds[0]);
	VectorCopy(maxs, box_brush->bounds[1]);

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds(const clipHandle_t model, vec3_t mins, vec3_t maxs)
{
	const cmodel_t* cmod = CM_clip_handleToModel(model);
	VectorCopy(cmod->mins, mins);
	VectorCopy(cmod->maxs, maxs);
}

int CM_LoadSubBSP(const char* name, const qboolean clientload)
{
	int checksum;

	int count = cmg.numSubModels;
	for (int i = 0; i < NumSubBSP; i++)
	{
		if (!Q_stricmp(name, SubBSP[i].name))
		{
			return count;
		}
		count += SubBSP[i].numSubModels;
	}

	if (NumSubBSP == MAX_SUB_BSP)
	{
		Com_Error(ERR_DROP, "CM_LoadSubBSP: too many unique sub BSPs");
	}

	CM_LoadMap_Actual(name, clientload, &checksum, SubBSP[NumSubBSP]);
	NumSubBSP++;

	return count;
}

int CM_FindSubBSP(const int modelIndex)
{
	int count = cmg.numSubModels;
	if (modelIndex < count)
	{
		// belongs to the main bsp
		return -1;
	}

	for (int i = 0; i < NumSubBSP; i++)
	{
		count += SubBSP[i].numSubModels;
		if (modelIndex < count)
		{
			return i;
		}
	}
	return -1;
}

void CM_GetWorldBounds(vec3_t mins, vec3_t maxs)
{
	VectorCopy(cmg.cmodels[0].mins, mins);
	VectorCopy(cmg.cmodels[0].maxs, maxs);
}

int CM_ModelContents_Actual(const clipHandle_t model, clipMap_t* cm)
{
	int contents = 0;
	int i;

	if (!cm)
	{
		cm = &cmg;
	}

	const cmodel_t* cmod = CM_clip_handleToModel(model, &cm);

	//MCG ADDED - return the contents, too

	for (i = 0; i < cmod->leaf.numLeafBrushes; i++)
	{
		const int brush_num = cm->leafbrushes[cmod->leaf.firstLeafBrush + i];
		contents |= cm->brushes[brush_num].contents;
	}

	for (i = 0; i < cmod->leaf.numLeafSurfaces; i++)
	{
		const int surfaceNum = cm->leafsurfaces[cmod->leaf.firstLeafSurface + i];
		if (cm->surfaces[surfaceNum] != nullptr)
		{
			//HERNH?  How could we have a null surf within our cmod->leaf.numLeafSurfaces?
			contents |= cm->surfaces[surfaceNum]->contents;
		}
	}

	return contents;
}

int CM_ModelContents(const clipHandle_t model, const int sub_bsp_index)
{
	if (sub_bsp_index < 0)
	{
		return CM_ModelContents_Actual(model, nullptr);
	}

	return CM_ModelContents_Actual(model, &SubBSP[sub_bsp_index]);
}