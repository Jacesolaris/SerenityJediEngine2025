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

// tr_shader.c -- this file deals with the parsing and definition of shaders

#include "tr_local.h"

static char* s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t shader;
static texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define FILE_HASH_SIZE		1024
static shader_t* hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char** shaderTextHashTable[MAX_SHADERTEXT_HASH] = { nullptr };

void KillTheShaderHashTable()
{
	memset(shaderTextHashTable, 0, sizeof shaderTextHashTable);
}

qboolean ShaderHashTableExists()
{
	if (shaderTextHashTable[0])
	{
		return qtrue;
	}
	return qfalse;
}

const int lightmapsNone[MAXLIGHTMAPS] =
{
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE,
	LIGHTMAP_NONE
};

const int lightmaps2d[MAXLIGHTMAPS] =
{
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D,
	LIGHTMAP_2D
};

const int lightmapsVertex[MAXLIGHTMAPS] =
{
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX,
	LIGHTMAP_BY_VERTEX
};

const int lightmapsFullBright[MAXLIGHTMAPS] =
{
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE,
	LIGHTMAP_WHITEIMAGE
};

const byte stylesDefault[MAXLIGHTMAPS] =
{
	LS_NORMAL,
	LS_LSNONE,
	LS_LSNONE,
	LS_LSNONE
};

static void ClearGlobalShader(void)
{
	memset(&shader, 0, sizeof shader);
	memset(&stages, 0, sizeof stages);
	for (int i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
		stages[i].mGLFogColorOverride = GLFOGOVERRIDE_NONE;
	}

	shader.contentFlags = CONTENTS_SOLID | CONTENTS_OPAQUE;
}

/*
================
return a hash value for the filename
================
*/
static long generateHashValue(const char* fname, const int size)
{
	long hash = 0;
	int i = 0;
	while (fname[i] != '\0')
	{
		char letter = tolower(static_cast<unsigned char>(fname[i]));
		if (letter == '.') break; // don't include extension
		if (letter == '\\') letter = '/'; // damn path names
		if (letter == PATH_SEP) letter = '/'; // damn path names
		hash += static_cast<long>(letter) * (i + 119);
		i++;
	}
	hash = hash ^ hash >> 10 ^ hash >> 20;
	hash &= size - 1;
	return hash;
}

void R_RemapShader(const char* shaderName, const char* newShaderName, const char* timeOffset)
{
	char stripped_name[MAX_QPATH];
	qhandle_t h;

	shader_t* sh = R_FindShaderByName(shaderName);
	if (sh == nullptr || sh == tr.defaultShader)
	{
		h = RE_RegisterShaderLightMap(shaderName, lightmapsNone, stylesDefault);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == nullptr || sh == tr.defaultShader)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: R_RemapShader: shader %s not found\n", shaderName);
		return;
	}

	shader_t* sh2 = R_FindShaderByName(newShaderName);
	if (sh2 == nullptr || sh2 == tr.defaultShader)
	{
		h = RE_RegisterShaderLightMap(newShaderName, lightmapsNone, stylesDefault);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == nullptr || sh2 == tr.defaultShader)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: R_RemapShader: new shader %s not found\n", newShaderName);
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, stripped_name, sizeof stripped_name);
	const int hash = generateHashValue(stripped_name, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		if (Q_stricmp(sh->name, stripped_name) == 0)
		{
			if (sh != sh2)
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = nullptr;
			}
		}
	}
	if (timeOffset)
	{
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector(const char** text, const int count, float* v)
{
	// FIXME: spaces are currently required after parens, should change parseext...
	const char* token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "(") != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	for (int i = 0; i < count; i++)
	{
		token = COM_ParseExt(text, qfalse);
		if (!token[0])
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing vector element in shader '%s'\n", shader.name);
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, ")") != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return qfalse;
	}

	return qtrue;
}

/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc(const char* funcname)
{
	if (!Q_stricmp(funcname, "GT0"))
	{
		return GLS_ATEST_GT_0;
	}
	if (!Q_stricmp(funcname, "LT128"))
	{
		return GLS_ATEST_LT_80;
	}
	if (!Q_stricmp(funcname, "GE128"))
	{
		return GLS_ATEST_GE_80;
	}
	if (!Q_stricmp(funcname, "GE192"))
	{
		return GLS_ATEST_GE_C0;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name);
	return 0;
}

/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode(const char* name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_SRCBLEND_ONE;
	}
	if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_SRCBLEND_ZERO;
	}
	if (!Q_stricmp(name, "GL_DST_COLOR"))
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name,
		shader.name);
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode(const char* name)
{
	if (!Q_stricmp(name, "GL_ONE"))
	{
		return GLS_DSTBLEND_ONE;
	}
	if (!Q_stricmp(name, "GL_ZERO"))
	{
		return GLS_DSTBLEND_ZERO;
	}
	if (!Q_stricmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	if (!Q_stricmp(name, "GL_DST_ALPHA"))
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	if (!Q_stricmp(name, "GL_SRC_COLOR"))
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name,
		shader.name);
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc(const char* funcname)
{
	if (!Q_stricmp(funcname, "sin"))
	{
		return GF_SIN;
	}
	if (!Q_stricmp(funcname, "square"))
	{
		return GF_SQUARE;
	}
	if (!Q_stricmp(funcname, "triangle"))
	{
		return GF_TRIANGLE;
	}
	if (!Q_stricmp(funcname, "sawtooth"))
	{
		return GF_SAWTOOTH;
	}
	if (!Q_stricmp(funcname, "inversesawtooth"))
	{
		return GF_INVERSE_SAWTOOTH;
	}
	if (!Q_stricmp(funcname, "noise"))
	{
		return GF_NOISE;
	}
	if (!Q_stricmp(funcname, "random"))
	{
		return GF_RAND;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm(const char** text, waveForm_t* wave)
{
	const char* token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = atof(token);

	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = atof(token);
}

/*
===================
ParseTexMod
===================
*/
static void ParseTexMod(const char* _text, shaderStage_t* stage)
{
	const char** text = &_text;

	if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
	{
		Com_Error(ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name);
	}

	texModInfo_t* tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	const char* token = COM_ParseExt(text, qfalse);

	//
	// turb
	//
	if (!Q_stricmp(token, "turb"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if (!Q_stricmp(token, "scale"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token); //scale unioned

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token); //scale unioned
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if (!Q_stricmp(token, "scroll"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token); //scroll unioned
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token); //scroll unioned
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if (!Q_stricmp(token, "stretch"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = atof(token);

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if (!Q_stricmp(token, "transform"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][1] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][1] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = atof(token);

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if (!Q_stricmp(token, "rotate"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = atof(token); //rotateSpeed unioned
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if (!Q_stricmp(token, "entityTranslate"))
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
	}
}

/*
/////===== Part of the VERTIGON system =====/////
===================
ParseSurfaceSprites
===================
*/
// surfaceSprites <type> <width> <height> <density> <fadedist>
//
// NOTE:  This parsing function used to be 12 pages long and very complex.  The new version of surfacesprites
// utilizes optional parameters parsed in ParseSurfaceSpriteOptional.
static void ParseSurfaceSprites(const char* _text, shaderStage_t* stage)
{
	const char** text = &_text;
	int sstype;

	//
	// spritetype
	//
	const char* token = COM_ParseExt(text, qfalse);

	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "vertical"))
	{
		sstype = SURFSPRITE_VERTICAL;
	}
	else if (!Q_stricmp(token, "oriented"))
	{
		sstype = SURFSPRITE_ORIENTED;
	}
	else if (!Q_stricmp(token, "effect"))
	{
		sstype = SURFSPRITE_EFFECT;
	}
	else if (!Q_stricmp(token, "flattened"))
	{
		sstype = SURFSPRITE_FLATTENED;
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid type in shader '%s'\n", shader.name);
		return;
	}

	//
	// width
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return;
	}
	const float width = atof(token);
	if (width <= 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width in shader '%s'\n", shader.name);
		return;
	}

	//
	// height
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return;
	}
	const float height = atof(token);
	if (height <= 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height in shader '%s'\n", shader.name);
		return;
	}

	//
	// density
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return;
	}
	const float density = atof(token);
	if (density <= 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid density in shader '%s'\n", shader.name);
		return;
	}

	//
	// fadedist
	//
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing surfaceSprites params in shader '%s'\n", shader.name);
		return;
	}
	const float fadedist = atof(token);
	if (fadedist < 32)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid fadedist (%f < 32) in shader '%s'\n", fadedist, shader.name);
		return;
	}

	if (!stage->ss)
	{
		stage->ss = static_cast<surfaceSprite_t*>(Hunk_Alloc(sizeof(surfaceSprite_t), h_low));
	}

	// These are all set by the command lines.
	stage->ss->surfaceSpriteType = sstype;
	stage->ss->width = width;
	stage->ss->height = height;
	stage->ss->density = density;
	stage->ss->fadeDist = fadedist;

	// These are defaults that can be overwritten.
	stage->ss->fadeMax = fadedist * 1.33;
	stage->ss->fadeScale = 0.0;
	stage->ss->wind = 0.0;
	stage->ss->windIdle = 0.0;
	stage->ss->variance[0] = 0.0;
	stage->ss->variance[1] = 0.0;
	stage->ss->facing = SURFSPRITE_FACING_NORMAL;

	// A vertical parameter that needs a default regardless
	stage->ss->vertSkew = 0.0f;

	// These are effect parameters that need defaults nonetheless.
	stage->ss->fxDuration = 1000; // 1 second
	stage->ss->fxGrow[0] = 0.0;
	stage->ss->fxGrow[1] = 0.0;
	stage->ss->fxAlphaStart = 1.0;
	stage->ss->fxAlphaEnd = 0.0;
}

/*
/////===== Part of the VERTIGON system =====/////
===========================
ParseSurfaceSpritesOptional
===========================
*/
//
// ssFademax <fademax>
// ssFadescale <fadescale>
// ssVariance <varwidth> <varheight>
// ssHangdown
// ssAnyangle
// ssFaceup
// ssWind <wind>
// ssWindIdle <windidle>
// ssVertSkew <skew>
// ssFXDuration <duration>
// ssFXGrow <growwidth> <growheight>
// ssFXAlphaRange <alphastart> <startend>
// ssFXWeather
//
// Optional parameters that will override the defaults set in the surfacesprites command above.
//
static void ParseSurfaceSpritesOptional(const char* param, const char* _text, shaderStage_t* stage)
{
	const char* token;
	const char** text = &_text;
	float value;

	if (!stage->ss)
	{
		stage->ss = static_cast<surfaceSprite_t*>(Hunk_Alloc(sizeof(surfaceSprite_t), h_low));
	}
	//
	// fademax
	//
	if (!Q_stricmp(param, "ssFademax"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite fademax in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value <= stage->ss->fadeDist)
		{
			Com_Printf(
				S_COLOR_YELLOW "WARNING: invalid surfacesprite fademax (%.2f <= fadeDist(%.2f)) in shader '%s'\n",
				value, stage->ss->fadeDist, shader.name);
			return;
		}
		stage->ss->fadeMax = value;
		return;
	}

	//
	// fadescale
	//
	if (!Q_stricmp(param, "ssFadescale"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite fadescale in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		stage->ss->fadeScale = value;
		return;
	}

	//
	// variance
	//
	if (!Q_stricmp(param, "ssVariance"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite variance width in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite variance width in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->variance[0] = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite variance height in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite variance height in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->variance[1] = value;
		return;
	}

	//
	// hangdown
	//
	if (!Q_stricmp(param, "ssHangdown"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Hangdown facing overrides previous facing in shader '%s'\n",
				shader.name);
			return;
		}
		stage->ss->facing = SURFSPRITE_FACING_DOWN;
		return;
	}

	//
	// anyangle
	//
	if (!Q_stricmp(param, "ssAnyangle"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Anyangle facing overrides previous facing in shader '%s'\n",
				shader.name);
			return;
		}
		stage->ss->facing = SURFSPRITE_FACING_ANY;
		return;
	}

	//
	// faceup
	//
	if (!Q_stricmp(param, "ssFaceup"))
	{
		if (stage->ss->facing != SURFSPRITE_FACING_NORMAL)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: Faceup facing overrides previous facing in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->facing = SURFSPRITE_FACING_UP;
		return;
	}

	//
	// wind
	//
	if (!Q_stricmp(param, "ssWind"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite wind in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite wind in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->wind = value;
		if (stage->ss->windIdle <= 0)
		{
			// Also override the windidle, it usually is the same as wind
			stage->ss->windIdle = value;
		}
		return;
	}

	//
	// windidle
	//
	if (!Q_stricmp(param, "ssWindidle"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite windidle in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite windidle in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->windIdle = value;
		return;
	}

	//
	// vertskew
	//
	if (!Q_stricmp(param, "ssVertskew"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite vertskew in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0.0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite vertskew in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->vertSkew = value;
		return;
	}

	//
	// fxduration
	//
	if (!Q_stricmp(param, "ssFXDuration"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite duration in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value <= 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite duration in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->fxDuration = value;
		return;
	}

	//
	// fxgrow
	//
	if (!Q_stricmp(param, "ssFXGrow"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite grow width in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite grow width in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->fxGrow[0] = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite grow height in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite grow height in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->fxGrow[1] = value;
		return;
	}

	//
	// fxalpharange
	//
	if (!Q_stricmp(param, "ssFXAlphaRange"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite fxalpha start in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite fxalpha start in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->fxAlphaStart = value;

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing surfacesprite fxalpha end in shader '%s'\n", shader.name);
			return;
		}
		value = atof(token);
		if (value < 0 || value > 1.0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid surfacesprite fxalpha end in shader '%s'\n", shader.name);
			return;
		}
		stage->ss->fxAlphaEnd = value;
		return;
	}

	//
	// fxweather
	//
	if (!Q_stricmp(param, "ssFXWeather"))
	{
		if (stage->ss->surfaceSpriteType != SURFSPRITE_EFFECT)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: weather applied to non-effect surfacesprite in shader '%s'\n",
				shader.name);
			return;
		}
		stage->ss->surfaceSpriteType = SURFSPRITE_WEATHERFX;
		return;
	}

	//
	// invalid ss command.
	//
	Com_Printf(S_COLOR_YELLOW "WARNING: invalid optional surfacesprite param '%s' in shader '%s'\n", param,
		shader.name);
}

/*
===================
ParseStage
===================
*/
static qboolean ParseStage(shaderStage_t* stage, const char** text)
{
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blend_dst_bits = 0, atest_bits = 0, depth_func_bits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while (true)
	{
		char* token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching '}' found\n");
			return qfalse;
		}

		if (token[0] == '}')
		{
			break;
		}
		//
		// map <name>
		//
		if (!Q_stricmp(token, "map"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "$whiteimage"))
			{
				stage->bundle[0].image = tr.whiteImage;
				continue;
			}
			if (!Q_stricmp(token, "$lightmap"))
			{
				stage->bundle[0].isLightmap = qtrue;
				if (shader.lightmapIndex[0] < 0 || shader.lightmapIndex[0] >= tr.numLightmaps)
				{
#ifndef FINAL_BUILD
					Com_Printf(S_COLOR_RED"Lightmap requested but none available for shader %s\n", shader.name);
#endif
					stage->bundle[0].image = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image = tr.lightmaps[shader.lightmapIndex[0]];
				}
				continue;
			}
			stage->bundle[0].image = nullptr;
			return qfalse;
		}
		//
		// clampmap <name>
		//
		if (!Q_stricmp(token, "clampmap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n",
					shader.name);
				return qfalse;
			}
			stage->bundle[0].image = nullptr;
			return qfalse;
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		if (!Q_stricmp(token, "animMap") || !Q_stricmp(token, "clampanimMap") || !Q_stricmp(token, "oneshotanimMap"))
		{
			constexpr auto max_image_animations = 64;
			image_t* images[max_image_animations]{};
			const bool b_clamp = !Q_stricmp(token, "clampanimMap");
			const bool one_shot = !Q_stricmp(token, "oneshotanimMap");

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for '%s' keyword in shader '%s'\n",
					b_clamp ? "animMap" : "clampanimMap", shader.name);
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof(token);
			stage->bundle[0].oneShotAnimMap = one_shot;

			// parse up to max_image_animations animations
			while (true)
			{
				token = COM_ParseExt(text, qfalse);
				if (!token[0])
				{
					break;
				}
				const int num = stage->bundle[0].numImageAnimations;
				if (num < max_image_animations)
				{
					stage->bundle[0].image = nullptr;
					return qfalse;
				}
			}
			// Copy image ptrs into an array of ptrs
			stage->bundle[0].image = static_cast<image_t*>(Hunk_Alloc(
				stage->bundle[0].numImageAnimations * sizeof(image_t*), h_low));
			memcpy(stage->bundle[0].image, images, stage->bundle[0].numImageAnimations * sizeof(image_t*));
		}
		else if (!Q_stricmp(token, "videoMap"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n",
					shader.name);
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = -1;
			if (stage->bundle[0].videoMapHandle != -1)
			{
				stage->bundle[0].isVideoMap = qtrue;
				assert(stage->bundle[0].videoMapHandle < NUM_SCRATCH_IMAGES);
				stage->bundle[0].image = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}

		//
		// alphafunc <func>
		//
		else if (!Q_stricmp(token, "alphaFunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n",
					shader.name);
				return qfalse;
			}

			atest_bits = NameToAFunc(token);
		}
		//
		// depthFunc <func>
		//
		else if (!Q_stricmp(token, "depthfunc"))
		{
			token = COM_ParseExt(text, qfalse);

			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n",
					shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "lequal"))
			{
				depth_func_bits = 0;
			}
			else if (!Q_stricmp(token, "equal"))
			{
				depth_func_bits = GLS_DEPTHFUNC_EQUAL;
			}
			else if (!Q_stricmp(token, "disable"))
			{
				depth_func_bits = GLS_DEPTHTEST_DISABLE;
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
			}
		}
		//
		// detail
		//
		else if (!Q_stricmp(token, "detail"))
		{
			stage->isDetail = qtrue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if (!Q_stricmp(token, "blendfunc"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}
			// check for "simple" blends first
			if (!Q_stricmp(token, "add"))
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blend_dst_bits = GLS_DSTBLEND_ONE;
			}
			else if (!Q_stricmp(token, "filter"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blend_dst_bits = GLS_DSTBLEND_ZERO;
			}
			else if (!Q_stricmp(token, "blend"))
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blend_dst_bits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}
				blend_dst_bits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if (!depthMaskExplicit)
			{
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if (!Q_stricmp(token, "rgbGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->rgbWave);
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				vec3_t color;

				ParseVector(text, 3, color);
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "identityLighting"))
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->rgbGen = CGEN_VERTEX;
				if (stage->alphaGen == 0)
				{
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if (!Q_stricmp(token, "exactVertex"))
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingDiffuse"))
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if (!Q_stricmp(token, "lightingDiffuseEntity"))
			{
				if (shader.lightmapIndex[0] != LIGHTMAP_NONE)
				{
					Com_Printf(S_COLOR_RED "ERROR: rgbGen lightingDiffuseEntity used on a misc_model! in shader '%s'\n",
						shader.name);
				}
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token,
					shader.name);
			}
		}
		//
		// alphaGen
		//
		else if (!Q_stricmp(token, "alphaGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->alphaWave);
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if (!Q_stricmp(token, "const"))
			{
				token = COM_ParseExt(text, qfalse);
				stage->constantColor[3] = 255 * atof(token);
				stage->alphaGen = AGEN_CONST;
			}
			else if (!Q_stricmp(token, "identity"))
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if (!Q_stricmp(token, "entity"))
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if (!Q_stricmp(token, "oneMinusEntity"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if (!Q_stricmp(token, "vertex"))
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if (!Q_stricmp(token, "lightingSpecular"))
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if (!Q_stricmp(token, "oneMinusVertex"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if (!Q_stricmp(token, "dot"))
			{
				stage->alphaGen = AGEN_DOT;
			}
			else if (!Q_stricmp(token, "oneMinusDot"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_DOT;
			}
			else if (!Q_stricmp(token, "portal"))
			{
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
				{
					shader.portalRange = 256;
					Com_Printf(
						S_COLOR_YELLOW
						"WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n",
						shader.name);
				}
				else
				{
					shader.portalRange = atof(token);
				}
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token,
					shader.name);
			}
		}
		//
		// tcGen <function>
		//
		else if (!Q_stricmp(token, "texgen") || !Q_stricmp(token, "tcGen"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing texgen parm in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "environment"))
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if (!Q_stricmp(token, "lightmap"))
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if (!Q_stricmp(token, "texture") || !Q_stricmp(token, "base"))
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if (!Q_stricmp(token, "vector"))
			{
				stage->bundle[0].tcGenVectors = static_cast<vec3_t*>(Hunk_Alloc(2 * sizeof(vec3_t), h_low));
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: unknown texgen parm in shader '%s'\n", shader.name);
			}
		}
		//
		// tcMod <type> <...>
		//
		else if (!Q_stricmp(token, "tcMod"))
		{
			char buffer[1024] = "";

			while (true)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof buffer, token);
				Q_strcat(buffer, sizeof buffer, " ");
			}

			ParseTexMod(buffer, stage);
		}
		//
		// depthmask
		//
		else if (!Q_stricmp(token, "depthwrite"))
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;
		}
		// If this stage has glow...	GLOWXXX
		else if (Q_stricmp(token, "glow") == 0)
		{
			stage->glow = true;
		}
		//
		// surfaceSprites <type> ...
		//
		else if (!Q_stricmp(token, "surfaceSprites"))
		{
			char buffer[1024] = "";

			while (true)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof buffer, token);
				Q_strcat(buffer, sizeof buffer, " ");
			}

			ParseSurfaceSprites(buffer, stage);
		}
		//
		// ssFademax <fademax>
		// ssFadescale <fadescale>
		// ssVariance <varwidth> <varheight>
		// ssHangdown
		// ssAnyangle
		// ssFaceup
		// ssWind <wind>
		// ssWindIdle <windidle>
		// ssDuration <duration>
		// ssGrow <growwidth> <growheight>
		// ssWeather
		//
		else if (!Q_stricmpn(token, "ss", 2)) // <--- NOTE ONLY COMPARING FIRST TWO LETTERS
		{
			char buffer[1024] = "";
			char param[128];
			strcpy(param, token);

			while (true)
			{
				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0)
					break;
				Q_strcat(buffer, sizeof buffer, token);
				Q_strcat(buffer, sizeof buffer, " ");
			}

			ParseSurfaceSpritesOptional(param, buffer, stage);
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if (stage->rgbGen == CGEN_BAD)
	{
		if ( //blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE ||
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA)
		{
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		else
		{
			stage->rgbGen = CGEN_IDENTITY;
		}
	}

	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if (blendSrcBits == GLS_SRCBLEND_ONE &&
		blend_dst_bits == GLS_DSTBLEND_ZERO)
	{
		blend_dst_bits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if (stage->alphaGen == AGEN_IDENTITY)
	{
		if (stage->rgbGen == CGEN_IDENTITY
			|| stage->rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
		blendSrcBits | blend_dst_bits |
		atest_bits |
		depth_func_bits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform(const char** text)
{
	char* token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if (shader.numDeforms == MAX_SHADER_DEFORMS)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	shader.deforms[shader.numDeforms] = static_cast<deformStage_t*>(Hunk_Alloc(sizeof(deformStage_t), h_low));

	deformStage_t* ds = shader.deforms[shader.numDeforms];
	shader.numDeforms++;

	if (!Q_stricmp(token, "projectionShadow"))
	{
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if (!Q_stricmp(token, "autosprite"))
	{
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if (!Q_stricmp(token, "autosprite2"))
	{
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if (!Q_stricmpn(token, "text", 4))
	{
		int n = token[4] - '0';
		if (n < 0 || n > 7)
		{
			n = 0;
		}
		ds->deformation = static_cast<deform_t>(DEFORM_TEXT0 + n);
		return;
	}

	if (!Q_stricmp(token, "bulge"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeWidth = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeHeight = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeSpeed = atof(token);

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if (!Q_stricmp(token, "wave"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}

		if (atof(token) != 0)
		{
			ds->deformationSpread = 1.0f / atof(token);
		}
		else
		{
			ds->deformationSpread = 100.0f;
			Com_Printf(S_COLOR_YELLOW "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n",
				shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if (!Q_stricmp(token, "normal"))
	{
		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.amplitude = atof(token);

		token = COM_ParseExt(text, qfalse);
		if (token[0] == 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.frequency = atof(token);

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if (!Q_stricmp(token, "move"))
	{
		for (float& i : ds->moveVector)
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
				return;
			}
			i = atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
		return;
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token,
		shader.name);
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms(const char** text)
{
	const char* suf[6] = { "rt", "lf", "bk", "ft", "up", "dn" };

	shader.sky = static_cast<skyParms_t*>(Hunk_Alloc(sizeof(skyParms_t), h_low));

	// outerbox
	char* token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (strcmp(token, "-") != 0)
	{
		for (int i = 0; i < 6; i++)
		{
			char pathname[MAX_QPATH];
			Com_sprintf(pathname, sizeof pathname, "%s_%s", token, suf[i]);
			shader.sky->outerbox[i] = nullptr;
		}
	}

	// cloudheight
	token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: 'skyParms' missing cloudheight in shader '%s'\n", shader.name);
		return;
	}
	shader.sky->cloudHeight = atof(token);
	if (!shader.sky->cloudHeight)
	{
		shader.sky->cloudHeight = 512;
	}

	// innerbox
	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "-") != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: in shader '%s' 'skyParms', innerbox is not supported!", shader.name);
	}
}

/*
=================
ParseSort
=================
*/
static void ParseSort(const char** text)
{
	const char* token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if (!Q_stricmp(token, "portal"))
	{
		shader.sort = SS_PORTAL;
	}
	else if (!Q_stricmp(token, "sky"))
	{
		shader.sort = SS_ENVIRONMENT;
	}
	else if (!Q_stricmp(token, "opaque"))
	{
		shader.sort = SS_OPAQUE;
	}
	else if (!Q_stricmp(token, "decal"))
	{
		shader.sort = SS_DECAL;
	}
	else if (!Q_stricmp(token, "seeThrough"))
	{
		shader.sort = SS_SEE_THROUGH;
	}
	else if (!Q_stricmp(token, "banner"))
	{
		shader.sort = SS_BANNER;
	}
	else if (!Q_stricmp(token, "additive"))
	{
		shader.sort = SS_BLEND1;
	}
	else if (!Q_stricmp(token, "nearest"))
	{
		shader.sort = SS_NEAREST;
	}
	else if (!Q_stricmp(token, "underwater"))
	{
		shader.sort = SS_UNDERWATER;
	}
	else if (!Q_stricmp(token, "inside"))
	{
		shader.sort = SS_INSIDE;
	}
	else if (!Q_stricmp(token, "mid_inside"))
	{
		shader.sort = SS_MID_INSIDE;
	}
	else if (!Q_stricmp(token, "middle"))
	{
		shader.sort = SS_MIDDLE;
	}
	else if (!Q_stricmp(token, "mid_outside"))
	{
		shader.sort = SS_MID_OUTSIDE;
	}
	else if (!Q_stricmp(token, "outside"))
	{
		shader.sort = SS_OUTSIDE;
	}
	else
	{
		shader.sort = atof(token);
	}
}

/*
=================
ParseMaterial
=================
*/
const char* materialNames[MATERIAL_LAST] =
{
	MATERIALS
};

static void ParseMaterial(const char** text)
{
	const char* token = COM_ParseExt(text, qfalse);
	if (token[0] == 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: missing material in shader '%s'\n", shader.name);
		return;
	}
	for (int i = 0; i < MATERIAL_LAST; i++)
	{
		if (!Q_stricmp(token, materialNames[i]))
		{
			shader.surfaceFlags |= i;
			break;
		}
	}
}

// this table is also present in q3map

using infoParm_t = struct infoParm_s
{
	const char* name;
	uint32_t clearSolid, surfaceFlags, contents;

	infoParm_s() = default;

	bool operator==(const infoParm_s& other) const
	{
		return false;
	}

	infoParm_s(const char* name, const uint32_t& clearSolid, const uint32_t& surfaceFlags, const uint32_t& contents)
		: name(name), clearSolid(clearSolid), surfaceFlags(surfaceFlags), contents(contents)
	{
	}
};

infoParm_t info_Parms[] = {
	// Game content Flags
	{"nonsolid", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_NONE}, // special hack to clear solid flag
	{"nonopaque", ~CONTENTS_OPAQUE, SURF_NONE, CONTENTS_NONE}, // special hack to clear opaque flag
	{"lava", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_LAVA}, // very damaging
	{"slime", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_SLIME}, // mildly damaging
	{"water", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_WATER}, //
	{"fog", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_FOG}, // carves surfaces entering
	{"shotclip", ~CONTENTS_SOLID, SURF_NONE, CONTENTS_SHOTCLIP}, // block shots, but not people
	{"playerclip", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_PLAYERCLIP}, // block only the player
	{"monsterclip", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_MONSTERCLIP}, //
	{"botclip", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_BOTCLIP}, // for bots
	{"trigger", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_TRIGGER}, //
	{"nodrop", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_NODROP},
	// don't drop items or leave bodies (death fog, lava, etc)
	{"terrain", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_TERRAIN}, // use special terrain collsion
	{"ladder", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_LADDER}, // climb up in it like water
	{"abseil", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_ABSEIL}, // can abseil down this brush
	{"outside", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_OUTSIDE},
	// volume is considered to be in the outside (i.e. not indoors)
	{"inside", ~(CONTENTS_SOLID | CONTENTS_OPAQUE), SURF_NONE, CONTENTS_INSIDE},
	// volume is considered to be inside (i.e. indoors)

	{"detail", CONTENTS_ALL, SURF_NONE, CONTENTS_DETAIL}, // don't include in structural bsp
	{"trans", CONTENTS_ALL, SURF_NONE, CONTENTS_TRANSLUCENT}, // surface has an alpha component

	/* Game surface flags */
	{"sky", CONTENTS_ALL, SURF_SKY, CONTENTS_NONE}, // emit light from an environment map
	{"slick", CONTENTS_ALL, SURF_SLICK, CONTENTS_NONE}, //

	{"nodamage", CONTENTS_ALL, SURF_NODAMAGE, CONTENTS_NONE}, //
	{"noimpact", CONTENTS_ALL, SURF_NOIMPACT, CONTENTS_NONE}, // don't make impact explosions or marks
	{"nomarks", CONTENTS_ALL, SURF_NOMARKS, CONTENTS_NONE}, // don't make impact marks, but still explode
	{"nodraw", CONTENTS_ALL, SURF_NODRAW, CONTENTS_NONE}, // don't generate a drawsurface (or a lightmap)
	{"nosteps", CONTENTS_ALL, SURF_NOSTEPS, CONTENTS_NONE}, //
	{"nodlight", CONTENTS_ALL, SURF_NODLIGHT, CONTENTS_NONE}, // don't ever add dynamic lights
	{"metalsteps", CONTENTS_ALL, SURF_METALSTEPS, CONTENTS_NONE}, //
	{"nomiscents", CONTENTS_ALL, SURF_NOMISCENTS, CONTENTS_NONE}, // No misc ents on this surface
	{"forcefield", CONTENTS_ALL, SURF_FORCEFIELD, CONTENTS_NONE}, //
	{"forcesight", CONTENTS_ALL, SURF_FORCESIGHT, CONTENTS_NONE}, // only visible with force sight
};

/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm(const char** text)
{
	const char* token = COM_ParseExt(text, qfalse);

	for (const auto& numInfoParms : info_Parms)
	{
		if (!Q_stricmp(token, numInfoParms.name))
		{
			shader.surfaceFlags |= numInfoParms.surfaceFlags;
			shader.contentFlags |= numInfoParms.contents;
			shader.contentFlags &= numInfoParms.clearSolid;
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader(const char** text)
{
	int s = 0;

	char* token = COM_ParseExt(text, qtrue);
	if (token[0] != '{')
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
		return qfalse;
	}

	while (true)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in shader %s\n", shader.name);
			return qfalse;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		// stage definition
		if (token[0] == '{')
		{
			if (s >= MAX_SHADER_STAGES)
			{
				ri->Printf(PRINT_WARNING, "WARNING: too many stages in shader %s (max is %i)\n", shader.name,
					MAX_SHADER_STAGES);
				return qfalse;
			}

			if (!ParseStage(&stages[s], text))
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			if (stages[s].glow)
			{
				shader.hasGlow = true;
			}
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		if (!Q_stricmpn(token, "qer", 3))
		{
			SkipRestOfLine(text);
			continue;
		}
		// material deprecated as of 11 Jan 01
		// material undeprecated as of 7 May 01 - q3map_material deprecated
		if (!Q_stricmp(token, "material") || !Q_stricmp(token, "q3map_material"))
		{
			ParseMaterial(text);
		}
		// sun parms
		else if (!Q_stricmp(token, "sun") || !Q_stricmp(token, "q3map_sun") || !Q_stricmp(token, "q3map_sunExt"))
		{
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[0] = atof(token);
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[1] = atof(token);
			token = COM_ParseExt(text, qfalse);
			tr.sunLight[2] = atof(token);

			VectorNormalize(tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			float a = atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt(text, qfalse);
			a = atof(token);
			a = a / 180 * M_PI;

			token = COM_ParseExt(text, qfalse);
			float b = atof(token);
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);

			SkipRestOfLine(text);
		}
		// q3map_surfacelight deprecated as of 16 Jul 01
		else if (!Q_stricmp(token, "surfacelight") || !Q_stricmp(token, "q3map_surfacelight"))
		{
			token = COM_ParseExt(text, qfalse);
			tr.sunSurfaceLight = atoi(token);
		}
		else if (!Q_stricmp(token, "lightColor"))
		{
			/*
			if ( !ParseVector( text, 3, tr.sunAmbient ) )
			{
				return qfalse;
			}
			*/
			//SP skips this so I'm skipping it here too.
			SkipRestOfLine(text);
		}
		else if (!Q_stricmp(token, "deformvertexes") || !Q_stricmp(token, "deform"))
		{
			ParseDeform(text);
		}
		else if (!Q_stricmp(token, "tesssize"))
		{
			SkipRestOfLine(text);
		}
		else if (!Q_stricmp(token, "clampTime"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0])
			{
				shader.clampTime = atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if (!Q_stricmpn(token, "q3map", 5))
		{
			SkipRestOfLine(text);
		}
		// skip stuff that only q3map or the server needs
		else if (!Q_stricmp(token, "surfaceParm"))
		{
			ParseSurfaceParm(text);
		}
		// no mip maps
		else if (!Q_stricmp(token, "nomipmaps"))
		{
			shader.noMipMaps = true;
			shader.noPicMip = true;
		}
		// no picmip adjustment
		else if (!Q_stricmp(token, "nopicmip"))
		{
			shader.noPicMip = true;
		}
		else if (!Q_stricmp(token, "noglfog"))
		{
			shader.fogPass = FP_NONE;
		}
		// polygonOffset
		else if (!Q_stricmp(token, "polygonOffset"))
		{
			shader.polygonOffset = true;
		}
		else if (!Q_stricmp(token, "noTC"))
		{
			shader.noTC = true;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if (!Q_stricmp(token, "entityMergable"))
		{
			shader.entityMergable = true;
		}
		// fogParms
		else if (!Q_stricmp(token, "fogParms"))
		{
			shader.fogParms = static_cast<fogParms_t*>(Hunk_Alloc(sizeof(fogParms_t), h_low));
			if (!ParseVector(text, 3, shader.fogParms->color))
			{
				return qfalse;
			}

			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.fogParms->depthForOpaque = atof(token);

			// skip any old gradient directions
			SkipRestOfLine(text);
		}
		// portal
		else if (!Q_stricmp(token, "portal"))
		{
			shader.sort = SS_PORTAL;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if (!Q_stricmp(token, "skyparms"))
		{
			ParseSkyParms(text);
		}
		// light <value> determines flaring in q3map, not needed here
		else if (!Q_stricmp(token, "light"))
		{
			token = COM_ParseExt(text, qfalse);
		}
		// cull <face>
		else if (!Q_stricmp(token, "cull"))
		{
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0)
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twosided") || !Q_stricmp(token, "disable"))
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if (!Q_stricmp(token, "back") || !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided"))
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name);
			}
		}
		// sort
		else if (!Q_stricmp(token, "sort"))
		{
			ParseSort(text);
		}
		else
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if (s == 0 && !shader.sky && !(shader.contentFlags & CONTENTS_FOG))
	{
		return qfalse;
	}

	shader.explicitlyDefined = true;

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

using collapse_t = struct collapse_s
{
	int blendA;
	int blendB;

	int multitextureEnv;
	int multitextureBlend;
};

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static qboolean CollapseMultitexture()
{
	return qfalse;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
extern bool gServerSkinHack;

static void FixRenderCommandList(const int newShader)
{
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader(void)
{
	int i;

	shader_t* newShader = tr.shaders[tr.numShaders - 1];
	const float sort = newShader->sort;

	for (i = tr.numShaders - 2; i >= 0; i--)
	{
		if (tr.sortedShaders[i]->sort <= sort)
		{
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList(i + 1);

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}

/*
====================
GeneratePermanentShader
====================
*/
static shader_t* GeneratePermanentShader(void)
{
	if (tr.numShaders == MAX_SHADERS)
	{
		//Com_Printf (S_COLOR_YELLOW  "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		Com_Printf("WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	const auto newShader = static_cast<shader_s*>(ri->Hunk_Alloc(sizeof(shader_t), h_low));

	*newShader = shader;

	if (shader.sort <= /*SS_OPAQUE*/SS_SEE_THROUGH)
	{
		newShader->fogPass = FP_EQUAL;
	}
	else if (shader.contentFlags & CONTENTS_FOG)
	{
		newShader->fogPass = FP_LE;
	}

	tr.shaders[tr.numShaders] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[tr.numShaders] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	int size = newShader->numUnfoggedPasses ? newShader->numUnfoggedPasses * sizeof stages[0] : sizeof stages[0];
	newShader->stages = static_cast<shaderStage_t*>(Hunk_Alloc(size, h_low));

	for (int i = 0; i < newShader->numUnfoggedPasses; i++)
	{
		if (!stages[i].active)
		{
			break;
		}
		newShader->stages[i] = stages[i];

		for (int b = 0; b < NUM_TEXTURE_BUNDLES; b++)
		{
			if (newShader->stages[i].bundle[b].numTexMods)
			{
				size = newShader->stages[i].bundle[b].numTexMods * sizeof(texModInfo_t);
				newShader->stages[i].bundle[b].texMods = static_cast<texModInfo_t*>(Hunk_Alloc(size, h_low));
				memcpy(newShader->stages[i].bundle[b].texMods, stages[i].bundle[b].texMods, size);
			}
			else
			{
				newShader->stages[i].bundle[b].texMods = nullptr; //clear the globabl ptr jic
			}
		}
	}

	SortNewShader();

	const int hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best approximate
what it is supposed to look like.

  OUTPUT:  Number of stages after the collapse (in the case of surfacesprites this isn't one).
=================
*/
//rww - no longer used, at least for now. destroys alpha shaders completely.
#if 0
static int VertexLightingCollapse(void) {
	int		stage, nextopenstage;
	shaderStage_t* bestStage;
	int		bestImageRank;
	int		rank;
	int		finalstagenum = 1;

	// if we aren't opaque, just use the first pass
	if (shader.sort == SS_OPAQUE) {
		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for (stage = 0; stage < MAX_SHADER_STAGES; stage++) {
			shaderStage_t* pStage = &stages[stage];

			if (!pStage->active) {
				break;
			}
			rank = 0;

			if (pStage->bundle[0].isLightmap) {
				rank -= 100;
			}
			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE) {
				rank -= 5;
			}
			if (pStage->bundle[0].numTexMods) {
				rank -= 5;
			}
			if (pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING) {
				rank -= 3;
			}

			// SurfaceSprites are most certainly NOT desireable as the collapsed surface texture.
			if (pStage->ss && pstage->ss->surfaceSpriteType)
			{
				rank -= 1000;
			}

			if (rank > bestImageRank) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if (shader.lightmapIndex[0] == LIGHTMAP_NONE) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		}
		else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	}
	else {
		// don't use a lightmap (tesla coils)
		if (stages[0].bundle[0].isLightmap) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if (stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH)
			&& (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH)) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH)
			&& (stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH)) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for (stage = 1, nextopenstage = 1; stage < MAX_SHADER_STAGES; stage++) {
		shaderStage_t* pStage = &stages[stage];

		if (!pStage->active) {
			break;
		}

		if (pStage->ss && pstage->ss->surfaceSpriteType)
		{
			// Copy this stage to the next open stage list (that is, we don't want any inactive stages before this one)
			if (nextopenstage != stage)
			{
				stages[nextopenstage] = *pStage;
				stages[nextopenstage].bundle[0] = pStage->bundle[0];
			}
			nextopenstage++;
			finalstagenum++;
			continue;
		}

		memset(pStage, 0, sizeof(*pStage));
	}

	return finalstagenum;
}
#endif

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t* FinishShader(void)
{
	int stage, lm_stage; //rwwRMG - stageIndex for AGEN_BLEND

	qboolean has_lightmap_stage = qfalse;

	//
	// set sky stuff appropriate
	//
	if (shader.sky)
	{
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if (shader.polygonOffset && !shader.sort)
	{
		shader.sort = SS_DECAL;
	}

	for (lm_stage = 0; lm_stage < MAX_SHADER_STAGES; lm_stage++)
	{
		const shaderStage_t* pStage = &stages[lm_stage];
		if (pStage->active && pStage->bundle[0].isLightmap)
		{
			break;
		}
	}

	if (lm_stage < MAX_SHADER_STAGES)
	{
		if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
		{
			if (lm_stage == 0) //< MAX_SHADER_STAGES-1)
			{
				//copy the rest down over the lightmap slot
				memmove(&stages[lm_stage], &stages[lm_stage + 1],
					sizeof(shaderStage_t) * (MAX_SHADER_STAGES - lm_stage - 1));
				memset(&stages[MAX_SHADER_STAGES - 1], 0, sizeof(shaderStage_t));
				//change blending on the moved down stage
				stages[lm_stage].stateBits = GLS_DEFAULT;
			}
			//change anything that was moved down (or the *white if LM is first) to use vertex color
			stages[lm_stage].rgbGen = CGEN_EXACT_VERTEX;
			stages[lm_stage].alphaGen = AGEN_SKIP;
			lm_stage = MAX_SHADER_STAGES; //skip the style checking below
		}
	}

	if (lm_stage < MAX_SHADER_STAGES) // && !r_fullbright->value)
	{
		int numStyles;
		int i;

		for (numStyles = 0; numStyles < MAXLIGHTMAPS; numStyles++)
		{
			if (shader.styles[numStyles] >= LS_UNUSED)
			{
				break;
			}
		}
		numStyles--;
		if (numStyles > 0)
		{
			for (i = MAX_SHADER_STAGES - 1; i > lm_stage + numStyles; i--)
			{
				stages[i] = stages[i - numStyles];
			}

			for (i = 0; i < numStyles; i++)
			{
				stages[lm_stage + i + 1] = stages[lm_stage];
				if (shader.lightmapIndex[i + 1] == LIGHTMAP_BY_VERTEX)
				{
					stages[lm_stage + i + 1].bundle[0].image = tr.whiteImage;
				}
				else if (shader.lightmapIndex[i + 1] < 0)
				{
					Com_Error(ERR_DROP, "FinishShader: light style with no light map or vertex color for shader %s",
						shader.name);
				}
				else
				{
					stages[lm_stage + i + 1].bundle[0].image = tr.lightmaps[shader.lightmapIndex[i + 1]];
					stages[lm_stage + i + 1].bundle[0].tcGen = static_cast<texCoordGen_t>(TCGEN_LIGHTMAP + i + 1);
				}
				stages[lm_stage + i + 1].rgbGen = CGEN_LIGHTMAPSTYLE;
				stages[lm_stage + i + 1].stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
				stages[lm_stage + i + 1].stateBits |= GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
			}
		}

		for (i = 0; i <= numStyles; i++)
		{
			stages[lm_stage + i].lightmapStyle = shader.styles[i];
		}
	}

	//
	// set appropriate stage information
	//
	int stage_index = 0; //rwwRMG - needed for AGEN_BLEND
	for (stage = 0; stage < MAX_SHADER_STAGES;)
	{
		shaderStage_t* pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		// check for a missing texture
		if (!pStage->bundle[0].image)
		{
			Com_Printf(S_COLOR_YELLOW "Shader %s has a stage with no image\n", shader.name);
			pStage->active = qfalse;
			stage++;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if (pStage->isDetail && !r_detailTextures->integer)
		{
			int index;

			for (index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if (!stages[index].active)
					break;
			}

			if (index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof * pStage * (index - stage));
			else
			{
				if (stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof * pStage * (index - stage - 1));

				Com_Memset(&stages[index - 1], 0, sizeof * stages);
			}

			continue;
		}

		pStage->index = stage_index; //rwwRMG - needed for AGEN_BLEND

		//
		// default texture coordinate generation
		//
		if (pStage->bundle[0].isLightmap)
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			has_lightmap_stage = qtrue;
		}
		else
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		// not a true lightmap but we want to leave existing
		// behaviour in place and not print out a warning
		//if (pStage->rgbGen == CGEN_VERTEX) {
		//  vertexLightmap = qtrue;
		//}

		//
		// determine sort order and fog color adjustment
		//
		if (pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS) &&
			stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			const int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			const int blend_dst_bits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (blendSrcBits == GLS_SRCBLEND_ONE && blend_dst_bits == GLS_DSTBLEND_ONE ||
				blendSrcBits == GLS_SRCBLEND_ZERO && blend_dst_bits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if (blendSrcBits == GLS_SRCBLEND_SRC_ALPHA && blend_dst_bits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if (blendSrcBits == GLS_SRCBLEND_ONE && blend_dst_bits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			else
			{
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if (!shader.sort)
			{
				// see through item, like a grill or grate
				if (pStage->stateBits & GLS_DEPTHMASK_TRUE)
				{
					shader.sort = SS_SEE_THROUGH;
				}
				else
				{
					/*
					if (( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ))
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND2;
					}
					else if (( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ))
					{ //rww - Pushed SS_BLEND1 up to SS_BLEND2, inserting this so that saber glow will render above water and things.
					  //Unfortunately it still affects other shaders with the same blend settings, but it seems more or less alright.
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
					*/
					if (blendSrcBits == GLS_SRCBLEND_ONE && blend_dst_bits == GLS_DSTBLEND_ONE)
					{
						// GL_ONE GL_ONE needs to come a bit later
						shader.sort = SS_BLEND1;
					}
					else
					{
						shader.sort = SS_BLEND0;
					}
				}
			}
		}

		//rww - begin hw fog
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_SRC_ALPHA |
			GLS_DSTBLEND_ONE) &&
			pStage->alphaGen == AGEN_LIGHTING_SPECULAR && stage)
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO |
			GLS_DSTBLEND_ZERO))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE |
			GLS_DSTBLEND_ZERO))
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == 0 && stage)
		{
			//
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == 0 && pStage->bundle[0].isLightmap &&
			stage < MAX_SHADER_STAGES - 1 &&
			stages[stage + 1].bundle[0].isLightmap)
		{
			// multiple light map blending
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_DST_COLOR |
			GLS_DSTBLEND_ZERO) && pStage->bundle[0].isLightmap)
		{
			//I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_WHITE;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_DST_COLOR |
			GLS_DSTBLEND_ZERO))
		{
			//I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE |
			GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))
		{
			//I don't know, it works. -rww
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_BLACK;
		}
		else
		{
			pStage->mGLFogColorOverride = GLFOGOVERRIDE_NONE;
		}
		//rww - end hw fog

		stage_index++; //rwwRMG - needed for AGEN_BLEND
		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if (!shader.sort)
	{
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if (stage > 1 && (r_vertexLight->integer && !r_uiFullScreen->integer))
	{
		//stage = VertexLightingCollapse();
		//rww - since this does bad things, I am commenting it out for now. If you want to attempt a fix, feel free.
		has_lightmap_stage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if (stage > 1 && CollapseMultitexture())
	{
		stage--;
	}

	if (shader.lightmapIndex[0] >= 0 && !has_lightmap_stage)
	{
		{
			Com_DPrintf("WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name);
			memcpy(shader.lightmapIndex, lightmapsNone, sizeof shader.lightmapIndex);
			memcpy(shader.styles, stylesDefault, sizeof shader.styles);
		}
	}

	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.sky)
	{
		shader.sort = SS_FOG;
	}

	for (stage = 1; stage < shader.numUnfoggedPasses; stage++)
	{
		// Make sure stage is non detail and active
		if (stages[stage].isDetail || !stages[stage].active)
		{
			break;
		}
		// MT lightmaps are always in bundle 1
		if (stages[stage].bundle[0].isLightmap)
		{
		}
	}

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char* FindShaderInShaderText(const char* shadername)
{
	char* token;
	const char* p;

	const int hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if (shaderTextHashTable[hash])
	{
		for (int i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);
			if (!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if (!p)
	{
		return nullptr;
	}

	// look for label
	while (true)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		if (!Q_stricmp(token, shadername))
		{
			return p;
		}
		// skip the definition
		SkipBracedSection(&p, 0);
	}

	return nullptr;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t* R_FindShaderByName(const char* name)
{
	char stripped_name[MAX_QPATH];

	if (name == nullptr || name[0] == 0)
	{
		return tr.defaultShader;
	}

	COM_StripExtension(name, stripped_name, sizeof stripped_name);

	const int hash = generateHashValue(stripped_name, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (shader_t* sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, stripped_name) == 0)
		{
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

inline static qboolean IsShader(const shader_t* sh, const char* name, const int* lightmapIndexes, const byte* styles)
{
	if (Q_stricmp(sh->name, name))
	{
		return qfalse;
	}

	if (!sh->defaultShader)
	{
		for (int i = 0; i < MAXLIGHTMAPS; i++)
		{
			if (sh->lightmapIndex[i] != lightmapIndexes[i])
			{
				return qfalse;
			}
			if (sh->styles[i] != styles[i])
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as appropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as appropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as appropriate for
most world construction surfaces.

===============
*/
shader_t* R_FindShader(const char* name, const int* lightmapIndexes, const byte* styles, const qboolean mipRawImage)
{
	char stripped_name[MAX_QPATH];
	char fileName[MAX_QPATH];
	const char* shader_text;
	shader_t* sh;

	if (name[0] == 0)
	{
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if (lightmapIndexes[0] >= 0 && lightmapIndexes[0] >= tr.numLightmaps)
	{
		lightmapIndexes = lightmapsVertex;
	}
	else if (lightmapIndexes[0] < LIGHTMAP_2D)
	{
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri->Printf(PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndexes[0]);
		lightmapIndexes = lightmapsVertex;
	}

	COM_StripExtension(name, stripped_name, sizeof stripped_name);

	const int hash = generateHashValue(stripped_name, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, stripped_name, lightmapIndexes, styles))
		{
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, stripped_name, sizeof shader.name);
	Com_Memcpy(shader.lightmapIndex, lightmapIndexes, sizeof shader.lightmapIndex);
	Com_Memcpy(shader.styles, styles, sizeof shader.styles);

	//
	// attempt to define shader from an explicit parameter file
	//
	shader_text = FindShaderInShaderText(stripped_name);
	if (shader_text)
	{
		if (!ParseShader(&shader_text))
		{
			// had errors, so use default shader
			shader.defaultShader = true;
		}
		sh = FinishShader();
		return sh;
	}

	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	COM_StripExtension(name, fileName, sizeof fileName);
	shader.defaultShader = qtrue;
	return FinishShader();
}

shader_t* R_FindServerShader(const char* name, const int* lightmapIndexes, const byte* styles)
{
	char stripped_name[MAX_QPATH];

	if (name[0] == 0)
	{
		return tr.defaultShader;
	}

	COM_StripExtension(name, stripped_name, sizeof stripped_name);

	const int hash = generateHashValue(stripped_name, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (shader_t* sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, stripped_name, lightmapIndexes, styles))
		{
			return sh;
		}
	}

	// clear the global shader
	ClearGlobalShader();
	Q_strncpyz(shader.name, stripped_name, sizeof shader.name);
	memcpy(shader.lightmapIndex, lightmapIndexes, sizeof shader.lightmapIndex);
	memcpy(shader.styles, styles, sizeof shader.styles);

	shader.defaultShader = true;
	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage(const char* name, const int* lightmapIndexes, const byte* styles, image_t* image)
{
	shader_t* sh;

	const int hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	// Doesn't actually ever get called in JA at all
	if (lightmapIndexes[0] >= tr.numLightmaps)
	{
		lightmapIndexes = const_cast<int*>(lightmapsFullBright);
	}

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (IsShader(sh, name, lightmapIndexes, styles))
		{
			return sh->index;
		}
	}

	// clear the global shader
	memset(&shader, 0, sizeof shader);
	memset(&stages, 0, sizeof stages);
	Q_strncpyz(shader.name, name, sizeof shader.name);
	memcpy(shader.lightmapIndex, lightmapIndexes, sizeof shader.lightmapIndex);
	memcpy(shader.styles, styles, sizeof shader.styles);

	for (int i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex[0] == LIGHTMAP_NONE)
	{
		// dynamic colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_BY_VERTEX)
	{
		// explicit colors at vertexes
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_2D)
	{
		// GUI elements
		stages[0].bundle[0].image = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			GLS_SRCBLEND_SRC_ALPHA |
			GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (shader.lightmapIndex[0] == LIGHTMAP_WHITEIMAGE)
	{
		// fullbright level
		stages[0].bundle[0].image = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	else
	{
		// two pass lightmap
		stages[0].bundle[0].image = tr.lightmaps[shader.lightmapIndex[0]];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY; // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}

/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap(const char* name, const int* lightmapIndexes, const byte* styles)
{
	if (strlen(name) >= MAX_QPATH)
	{
		Com_Printf("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	const shader_t* sh = R_FindShader(name, lightmapIndexes, styles, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader(const char* name)
{
	if (strlen(name) >= MAX_QPATH)
	{
		Com_Printf("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	const shader_t* sh = R_FindShader(name, lightmaps2d, stylesDefault, qtrue);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip(const char* name)
{
	if (strlen(name) >= MAX_QPATH)
	{
		Com_Printf("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	const shader_t* sh = R_FindShader(name, lightmaps2d, stylesDefault, qfalse);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

//added for ui -rww
const char* RE_ShaderNameFromIndex(const int index)
{
	assert(index >= 0 && index < tr.numShaders && tr.shaders[index]);
	return tr.shaders[index]->name;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t* R_GetShaderByHandle(const qhandle_t hShader)
{
	if (hShader < 0)
	{
		Com_Printf(S_COLOR_YELLOW "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	if (hShader >= tr.numShaders)
	{
		Com_Printf(S_COLOR_YELLOW "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders(const qboolean server)
{
	Com_Printf("Initializing Shaders\n");

	memset(hashTable, 0, sizeof hashTable);
}