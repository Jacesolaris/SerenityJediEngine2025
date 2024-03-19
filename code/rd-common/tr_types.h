/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#ifndef __TR_TYPES_H
#define __TR_TYPES_H

#include "../game/ghoul2_shared.h"
#include <qcommon\q_shared.h>

constexpr auto MAX_DLIGHTS = 32; // can't be increased, because bit flags are used on surfaces;
constexpr auto REFENTITYNUM_BITS = 11; // can't be increased without changing drawsurf bit packing;
#define	REFENTITYNUM_MASK	((1<<REFENTITYNUM_BITS) - 1)
// the last N-bit number (2^REFENTITYNUM_BITS - 1) is reserved for the special world refentity,
//  and this is reflected by the value of MAX_REFENTITIES (which therefore is not a power-of-2)
#define	MAX_REFENTITIES		((1<<REFENTITYNUM_BITS) - 1)
#define	REFENTITYNUM_WORLD	((1<<REFENTITYNUM_BITS) - 1)

#define	MAX_MINI_ENTITIES	1024

// renderfx flags
constexpr auto RF_MINLIGHT = 0x00001; // allways have some light (viewmodel, some items);
constexpr auto RF_THIRD_PERSON = 0x00002; // don't draw through eyes, only mirrors (player bodies, chat sprites);
constexpr auto RF_FIRST_PERSON = 0x00004; // only draw through eyes (view weapon, damage blood blob);
constexpr auto RF_DEPTHHACK = 0x00008; // for view weapon Z crunching;
constexpr auto RF_NODEPTH = 0x00010; // No depth at all (seeing through walls);

constexpr auto RF_VOLUMETRIC = 0x00020; // fake volumetric shading;

constexpr auto RF_NOSHADOW = 0x00040; // don't add stencil shadows;

constexpr auto RF_LIGHTING_ORIGIN = 0x00080; // use refEntity->lightingOrigin instead of refEntity->origin;
// for lighting.  This allows entities to sink into the floor
// with their origin going solid, and allows all parts of a
// player to get the same lighting
constexpr auto RF_SHADOW_PLANE = 0x00100; // use refEntity->shadowPlane;
constexpr auto RF_WRAP_FRAMES = 0x00200; // mod the model frames by the maxframes to allow continuous;
// animation without needing to know the frame count
constexpr auto RF_CAP_FRAMES = 0x00400; // cap the model frames by the maxframes for one shot anims;

constexpr auto RF_ALPHA_FADE = 0x00800; // hacks blend mode and uses whatever the set alpha is.;
constexpr auto RF_PULSATE = 0x01000; // for things like a dropped saber, where we want to add an extra visual clue;
constexpr auto RF_RGB_TINT = 0x02000; // overrides ent RGB color to the specified color;

constexpr auto RF_FORKED = 0x04000; // override lightning to have forks;
constexpr auto RF_TAPERED = 0x08000; // lightning tapers;
constexpr auto RF_GROW = 0x10000; // lightning grows from start to end during its life;

constexpr auto RF_SETANIMINDEX = 0x20000; //use backEnd.currentEntity->e.skinNum for R_BindAnimatedImage;

constexpr auto RF_DISINTEGRATE1 = 0x40000; // does a procedural hole-ripping thing.;
constexpr auto RF_DISINTEGRATE2 = 0x80000; // does a procedural hole-ripping thing with scaling at the ripping point;

constexpr auto RF_G2MINLOD = 0x100000; // force Lowest lod on g2;

constexpr auto RF_SHADOW_ONLY = 0x200000; //add surfs for shadowing but don't draw them normally -rww;

constexpr auto RF_DISTORTION = 0x400000; //area distortion effect -rww;

constexpr auto RF_FORCE_ENT_ALPHA = 0x800000; // override shader alpha settings;

constexpr auto RF_ALPHA_DEPTH = 0x1000000; //depth write on alpha model

// refdef flags
constexpr auto RDF_NOWORLDMODEL = 1; // used for player configuration screen;
constexpr auto RDF_HYPERSPACE = 4; // teleportation effect;

constexpr auto RDF_SKYBOXPORTAL = 8;
constexpr auto RDF_DRAWSKYBOX = 16;
// the above marks a scene as being a 'portal sky'.  this flag says to draw it or not;

constexpr auto RDF_doLAGoggles = 32; // Light Amp goggles;
constexpr auto RDF_doFullbright = 64; // Light Amp goggles;
constexpr auto RDF_ForceSightOn = 128; // using force sight;

using rgbTintsNew_t = enum
{
	TINT_NEW_ENT,
	TINT_HILT1,
	TINT_HILT2,
	MAX_CVAR_TINT,
	TINT_BLADE1 = MAX_CVAR_TINT,
	TINT_BLADE2,
	MAX_NEW_ENT_RGB
};

extern int skyboxportal;
extern int drawskyboxportal;

using color4ub_t = byte[4];

using polyVert_t = struct
{
	vec3_t xyz;
	float st[2];
	byte modulate[4];
};

using poly_t = struct poly_s
{
	qhandle_t hShader;
	int numVerts;
	polyVert_t* verts;
};

using refEntityType_t = enum
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_ORIENTED_QUAD,
	RT_LINE,
	RT_ORIENTEDLINE,
	RT_ELECTRICITY,
	RT_CYLINDER,
	RT_LATHE,
	RT_BEAM,
	RT_SABER_GLOW,
	RT_PORTALSURFACE,
	// doesn't draw anything, just info for portals
	RT_CLOUDS,
	RT_LIGHTNING,

	RT_MAX_REF_ENTITY_TYPE
};

using refEntity_t = struct
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel; // opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin; // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane; // projection shadows go here, stencils go slightly lower

	vec3_t axis[3]; // rotation vectors
	qboolean nonNormalizedAxes; // axis are not normalized, i.e. they have scale
	float origin[3]; // also used as MODEL_BEAM's "from"
	int frame; // also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	float oldorigin[3]; // also used as MODEL_BEAM's "to"
	int oldframe;
	float backlerp; // 0.0 = current, 1.0 = old

	// texturing
	int skinNum; // inline skin index

	qhandle_t customSkin; // NULL for default skin
	qhandle_t customShader; // use one image for the entire thing

	// misc
	byte shaderRGBA[4]; // colors used by colorSrc=vertex shaders
	float shaderTexCoord[2]; // texture coordinates used by tcMod=vertex modifiers
	float shaderTime; // subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;

	// texturing
	union
	{
		//		int			skinNum;		// inline skin index
		//		ivec3_t		terxelCoords;	// coords of patch for RT_TERXELS
		struct
		{
			int miniStart;
			int miniCount;
		} uMini;
	} uRefEnt;

	// extra sprite information
	union
	{
		struct
		{
			float rotation;
			float radius;
			byte vertRGBA[4][4];
		} sprite;

		struct
		{
			float width;
			float width2;
			float stscale;
		} line;

		struct
			// that whole put-the-opening-brace-on-the-same-line-as-the-beginning-of-the-definition coding style is fecal
		{
			float width;
			vec3_t control1;
			vec3_t control2;
		} bezier;

		struct
		{
			float width;
			float width2;
			float stscale;
			float height;
			float bias;
			qboolean wrap;
		} cylinder;

		struct
		{
			float width;
			float deviation;
			float stscale;
			qboolean wrap;
			qboolean taper;
		} electricity;
	} data;

	// This doesn't have to be unioned, but it does make for more meaningful variable names :)
	union
	{
		float rotation;
		float endTime;
		float saberLength;
	};

	/*
	Ghoul2 Insert Start
	*/
	vec3_t angles; // rotation angles - used for Ghoul2

	vec3_t modelScale; // axis scale for models
	CGhoul2Info_v* ghoul2; // has to be at the end of the ref-ent in order for it to be created properly
	/*
	Ghoul2 Insert End
	*/
	byte newShaderRGBA[MAX_NEW_ENT_RGB][4]; // colors used by colorSrc=vertex shaders
};

constexpr auto MAX_RENDER_STRINGS = 8;
constexpr auto MAX_RENDER_STRING_LENGTH = 32;

using refdef_t = struct
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewangles;
	vec3_t viewaxis[3]; // transformation matrix
	int viewContents; // world contents at vieworg

	// time in milliseconds for shader effects and other time dependent rendering issues
	int time;

	int rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
};

using stereoFrame_t = enum
{
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
};

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
using textureCompression_t = enum
{
	TC_NONE,
	TC_S3TC,
	TC_S3TC_DXT,
	TC_S3TC_ARB
};

using glconfig_t = struct glconfig_s
{
	const char* renderer_string;
	const char* vendor_string;
	const char* version_string;
	const char* extensions_string;
	int maxTextureSize; // queried from GL
	int maxActiveTextures; // multitexture ability
	float maxTextureFilterAnisotropy;

	int colorBits, depthBits, stencilBits;

	qboolean deviceSupportsGamma;
	textureCompression_t textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean textureFilterAnisotropicAvailable;
	qboolean clampToEdgeAvailable;

	int vidWidth, vidHeight;

	int displayFrequency;

	qboolean				doStencilShadowsInOneDrawcall;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean isFullscreen;
	qboolean stereoEnabled;
};

#endif	// __TR_TYPES_H
