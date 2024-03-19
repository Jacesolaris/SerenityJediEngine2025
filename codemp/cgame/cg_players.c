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

// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"
#include "ghoul2/G2.h"
#include "game/bg_saga.h"

extern void CheckCameraLocation(vec3_t oldeye_origin);
extern vmCvar_t cg_thirdPersonAlpha;
extern int cgSiegeTeam1PlShader;
extern int cgSiegeTeam2PlShader;
extern void CG_AddRadarEnt(const centity_t* cent); //cg_ents.c
extern void CG_AddBracketedEnt(const centity_t* cent); //cg_ents.c
extern qboolean CG_InFighter(void);
extern qboolean WP_SaberBladeUseSecondBladeStyle(const saberInfo_t* saber, int blade_num);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean manual_saberreadyanim(int anim);
extern qboolean PM_SaberStanceAnim(int anim);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WindAnim(int anim);
extern qboolean PM_InKataAnim(int anim);
extern qboolean PM_StandingAtReadyAnim(int anim);
extern qboolean PM_WalkingOrRunningAnim(int anim);
extern qboolean pm_saber_innonblockable_attack(int anim);

#define MIN_SABERBLADE_DRAW_LENGTH 0.5f

//for g2 surface routines
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100

extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

char* cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1",
	"*death2",
	"*death3",
	"*jump1",
	"*pain25",
	"*pain50",
	"*pain75",
	"*pain100",
	"*falling1",
	"*choke1",
	"*choke2",
	"*choke3",
	"*gasp",
	"*land1",
	"*taunt",
	"*pushed1", //Say when force-pushed
	"*pushed2",
	"*pushed3",
	"*anger1", //Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1", //Say when killed an enemy
	"*victory2",
	"*victory3",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	NULL
};

//NPC sounds:
//Used as a supplement to the basic set for enemies and hazard team
// (keep numbers in ascending order in order for variant-capping to work)
const char* cg_customCombatSoundNames[MAX_CUSTOM_COMBAT_SOUNDS] =
{
	"*anger1", //Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1", //Say when killed an enemy
	"*victory2",
	"*victory3",
	"*confuse1", //Say when confused
	"*confuse2",
	"*confuse3",
	"*choke1",
	"*choke2",
	"*choke3",
	"*ffwarn",
	"*ffturn",
	NULL
};

//Used as a supplement to the basic set for stormtroopers
// (keep numbers in ascending order in order for variant-capping to work)
const char* cg_customExtraSoundNames[MAX_CUSTOM_EXTRA_SOUNDS] =
{
	"*chase1",
	"*chase2",
	"*chase3",
	"*cover1",
	"*cover2",
	"*cover3",
	"*cover4",
	"*cover5",
	"*detected1",
	"*detected2",
	"*detected3",
	"*detected4",
	"*detected5",
	"*lost1",
	"*outflank1",
	"*outflank2",
	"*escaping1",
	"*escaping2",
	"*escaping3",
	"*giveup1",
	"*giveup2",
	"*giveup3",
	"*giveup4",
	"*look1",
	"*look2",
	"*sight1",
	"*sight2",
	"*sight3",
	"*sound1",
	"*sound2",
	"*sound3",
	"*suspicious1",
	"*suspicious2",
	"*suspicious3",
	"*suspicious4",
	"*suspicious5",
	NULL
};

//Used as a supplement to the basic set for jedi
// (keep numbers in ascending order in order for variant-capping to work)
const char* cg_customJediSoundNames[MAX_CUSTOM_JEDI_SOUNDS] =
{
	"*combat1",
	"*combat2",
	"*combat3",
	"*jdetected1",
	"*jdetected2",
	"*jdetected3",
	"*taunt",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*jchase1",
	"*jchase2",
	"*jchase3",
	"*jlost1",
	"*jlost2",
	"*jlost3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	"*pushfail",
	NULL
};

//Used for DUEL taunts
const char* cg_customDuelSoundNames[MAX_CUSTOM_DUEL_SOUNDS] =
{
	"*anger1", //Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1", //Say when killed an enemy
	"*victory2",
	"*victory3",
	"*taunt",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	NULL
};

// Callouts
const char* cg_customCalloutSoundNames[MAX_CUSTOM_CALLOUT_SOUNDS] =
{
	"*confuse1.wav", //Say when confused
	"*confuse2.wav",
	"*confuse3.wav",
	"*escaping1.wav",
	"*escaping2.wav",
	"*escaping3.wav",
	"*sight1.wav",
	"*sight2.wav",
	"*sight3.wav",
	NULL
};

void CG_Disintegration(centity_t* cent, refEntity_t* ent);

/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound(int clientNum, const char* sound_name)
{
	clientInfo_t* ci;
	int i;
	int num_c_sounds = 0;
	int num_c_com_sounds = 0;
	int num_c_ex_sounds = 0;
	int num_c_jedi_sounds = 0;
	int num_c_siege_sounds = 0;
	int num_c_duel_sounds = 0;
	int num_c_call_sounds = 0;
	char l_sound_name[MAX_QPATH];

	if (sound_name[0] != '*')
	{
		return trap->S_RegisterSound(sound_name);
	}

	COM_StripExtension(sound_name, l_sound_name, sizeof l_sound_name);

	if (clientNum < 0)
	{
		clientNum = 0;
	}

	if (clientNum >= MAX_CLIENTS)
	{
		ci = cg_entities[clientNum].npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[clientNum];
	}

	if (!ci)
	{
		return 0;
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		if (!cg_customSoundNames[i])
		{
			num_c_sounds = i;
			break;
		}
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		if (!cg_customCalloutSoundNames[i])
		{
			num_c_call_sounds = i;
			break;
		}
	}

	if (clientNum >= MAX_CLIENTS)
	{
		//these are only for npc's
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customCombatSoundNames[i])
			{
				num_c_com_sounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customExtraSoundNames[i])
			{
				num_c_ex_sounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customJediSoundNames[i])
			{
				num_c_jedi_sounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customCalloutSoundNames[i])
			{
				num_c_call_sounds = i;
				break;
			}
		}
	}

	if (cgs.gametype >= GT_TEAM || com_buildScript.integer)
	{
		//siege only
		for (i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				num_c_siege_sounds = i;
				break;
			}
		}
	}

	if (cgs.gametype >= GT_FFA || com_buildScript.integer)
	{
		//Duel only
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customDuelSoundNames[i])
			{
				num_c_duel_sounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customCombatSoundNames[i])
			{
				num_c_com_sounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customJediSoundNames[i])
			{
				num_c_jedi_sounds = i;
				break;
			}
		}
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		if (i < num_c_sounds && strcmp(l_sound_name, cg_customSoundNames[i]) == 0)
		{
			return ci->sounds[i];
		}
		if (i < num_c_siege_sounds && strcmp(l_sound_name, bg_customSiegeSoundNames[i]) == 0)
		{
			//siege only
			return ci->siegeSounds[i];
		}
		if (i < num_c_duel_sounds && strcmp(l_sound_name, cg_customDuelSoundNames[i]) == 0)
		{
			//siege only
			return ci->duelSounds[i];
		}
		if (i < num_c_com_sounds && strcmp(l_sound_name, cg_customCombatSoundNames[i]) == 0)
		{
			//npc only
			return ci->combatSounds[i];
		}
		if (clientNum >= MAX_CLIENTS && i < num_c_ex_sounds && strcmp(l_sound_name, cg_customExtraSoundNames[i]) == 0)
		{
			//npc only
			return ci->extraSounds[i];
		}
		if (i < num_c_jedi_sounds && strcmp(l_sound_name, cg_customJediSoundNames[i]) == 0)
		{
			//npc only
			return ci->jediSounds[i];
		}
		if (clientNum >= MAX_CLIENTS && i < num_c_call_sounds && strcmp(l_sound_name, cg_customCalloutSoundNames[i]) ==
			0)
		{
			//npc only
			return ci->calloutSounds[i];
		}
	}

#ifndef FINAL_BUILD
	Com_Printf("Unknown custom sound: %s\n", lSoundName);
#endif
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/
#define MAX_SURF_LIST_SIZE	1024

qboolean CG_ParseSurfsFile(const char* model_name, const char* skin_name, char* surf_off, char* surf_on)
{
	const char* text_p;
	const char* value;
	char text[20000];
	char sfilename[MAX_QPATH];
	fileHandle_t f;
	int i = 0;

	while (skin_name && skin_name[i])
	{
		if (skin_name[i] == '|')
		{
			//this is a multi-part skin, said skins do not support .surf files
			return qfalse;
		}

		i++;
	}

	// Load and parse .surf file
	Com_sprintf(sfilename, sizeof sfilename, "models/players/%s/model_%s.surf", model_name, skin_name);

	// load the file
	const int len = trap->FS_Open(sfilename, &f, FS_READ);
	if (len <= 0)
	{
		//no file
		return qfalse;
	}
	if (len >= sizeof text - 1)
	{
		Com_Printf("File %s too long\n", sfilename);
		trap->FS_Close(f);
		return qfalse;
	}

	trap->FS_Read(text, len, f);
	text[len] = 0;
	trap->FS_Close(f);

	// parse the text
	text_p = text;

	surf_off[0] = '\0';
	surf_on[0] = '\0';

	COM_BeginParseSession("CG_ParseSurfsFile");

	// read information for surfOff and surfOn
	while (1)
	{
		const char* token = COM_ParseExt(&text_p, qtrue);
		if (!token || !token[0])
		{
			break;
		}

		// surfOff
		if (!Q_stricmp(token, "surfOff"))
		{
			if (COM_ParseString(&text_p, &value))
			{
				continue;
			}
			if (surf_off && surf_off[0])
			{
				Q_strcat(surf_off, MAX_SURF_LIST_SIZE, ",");
				Q_strcat(surf_off, MAX_SURF_LIST_SIZE, value);
			}
			else
			{
				Q_strncpyz(surf_off, value, MAX_SURF_LIST_SIZE);
			}
			continue;
		}

		// surfOn
		if (!Q_stricmp(token, "surfOn"))
		{
			if (COM_ParseString(&text_p, &value))
			{
				continue;
			}
			if (surf_on && surf_on[0])
			{
				Q_strcat(surf_on, MAX_SURF_LIST_SIZE, ",");
				Q_strcat(surf_on, MAX_SURF_LIST_SIZE, value);
			}
			else
			{
				Q_strncpyz(surf_on, value, MAX_SURF_LIST_SIZE);
			}
		}
	}
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
qboolean BG_IsValidCharacterModel(const char* model_name, const char* skin_name);
qboolean BG_ValidateSkinForTeam(const char* model_name, char* skin_name, int team, float* colors);

static qboolean CG_RegisterClientModelname(clientInfo_t* ci, const char* model_name, const char* skin_name,
	const char* team_name, const int clientNum)
{
	char afilename[MAX_QPATH];
	char gla_name[MAX_QPATH];
	const vec3_t temp_vec = { 0, 0, 0 };
	qboolean bad_model = qfalse;
	char surf_off[MAX_SURF_LIST_SIZE];
	char surf_on[MAX_SURF_LIST_SIZE];
	char* use_skin_name;

retryModel:
	if (bad_model)
	{
		if (model_name && model_name[0])
		{
			Com_Printf(
				"WARNING: Attempted to load an unsupported multi player model %s! (bad or missing bone, or missing animation sequence)\n",
				model_name);
		}

		model_name = DEFAULT_MODEL;
		skin_name = "default";

		bad_model = qfalse;
	}

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
	}

	if (!BG_IsValidCharacterModel(model_name, skin_name))
	{
		model_name = DEFAULT_MODEL;
		skin_name = "default";
	}

	if (cgs.gametype >= GT_TEAM && !cgs.jediVmerc && cgs.gametype != GT_SIEGE)
	{
		//We won't force colors for siege.
		BG_ValidateSkinForTeam(ci->modelName, ci->skinName, ci->team, ci->colorOverride);
		skin_name = ci->skinName;
	}
	else
	{
		ci->colorOverride[0] = ci->colorOverride[1] = ci->colorOverride[2] = 0.0f;
	}

	// fix for transparent custom skin parts
	if (strchr(skin_name, '|')
		&& strstr(skin_name, "head")
		&& strstr(skin_name, "torso")
		&& strstr(skin_name, "lower"))
	{
		//three part skin
		use_skin_name = va("models/players/%s/|%s", model_name, skin_name);
	}
	else
	{
		use_skin_name = va("models/players/%s/model_%s.skin", model_name, skin_name);
	}

	const int check_skin = trap->R_RegisterSkin(use_skin_name);

	if (check_skin)
	{
		ci->torsoSkin = check_skin;
	}
	else
	{
		//fallback to the default skin
		ci->torsoSkin = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", model_name, skin_name));
	}
	Com_sprintf(afilename, sizeof afilename, "models/players/%s/model.glm", model_name);
	const int handle = trap->G2API_InitGhoul2Model(&ci->ghoul2Model, afilename, 0, ci->torsoSkin, 0, 0, 0);

	if (handle < 0)
	{
		return qfalse;
	}

	// The model is now loaded.

	trap->G2API_SetSkin(ci->ghoul2Model, 0, ci->torsoSkin, ci->torsoSkin);

	gla_name[0] = 0;

	trap->G2API_GetGLAName(ci->ghoul2Model, 0, gla_name);
	if (gla_name[0] != 0)
	{
		if (!strstr(gla_name, "players/_humanoid/")) //only allow rockettrooper in siege
		{
			//Bad!
			bad_model = qtrue;
			goto retryModel;
		}
	}

	if (!bgpa_ftext_loaded)
	{
		if (gla_name[0] == 0/*gla_name == NULL*/)
		{
			bad_model = qtrue;
			goto retryModel;
		}
		Q_strncpyz(afilename, gla_name, sizeof afilename);
		char* slash = Q_strrchr(afilename, '/');
		if (slash)
		{
			strcpy(slash, "/animation.cfg");
		} // Now afilename holds just the path to the animation.cfg
		else
		{
			// Didn't find any slashes, this is a raw filename right in base (whish isn't a good thing)
			return qfalse;
		}

		//rww - All player models must use humanoid, no matter what.
		if (Q_stricmp(afilename, "models/players/_humanoid/animation.cfg"))
		{
			Com_Printf("Model does not use supported animation config.\n");
			return qfalse;
		}
		if (bg_parse_animation_file("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf("Failed to load animation file models/players/_humanoid/animation.cfg\n");
			return qfalse;
		}

		BG_ParseAnimationEvtFile("models/players/_humanoid/", 0, -1); //get the sounds for the humanoid anims
	}
	else if (!bgAllEvents[0].eventsParsed)
	{
		//make sure the player anim sounds are loaded even if the anims already are
		BG_ParseAnimationEvtFile("models/players/_humanoid/", 0, -1);
	}

	if (CG_ParseSurfsFile(model_name, skin_name, surf_off, surf_on))
	{
		//turn on/off any surfs
		const char* token;
		const char* p;

		//Now turn on/off any surfaces
		if (surf_off[0])
		{
			p = surf_off;
			COM_BeginParseSession("CG_RegisterClientModelname: surfOff");
			while (1)
			{
				token = COM_ParseExt(&p, qtrue);
				if (!token[0])
				{
					//reached end of list
					break;
				}
				//turn off this surf
				trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, token, 0x00000002/*G2SURFACEFLAG_OFF*/);
			}
		}
		if (surf_on[0])
		{
			p = surf_on;
			COM_BeginParseSession("CG_RegisterClientModelname: surfOn");
			while (1)
			{
				token = COM_ParseExt(&p, qtrue);
				if (!token[0])
				{
					//reached end of list
					break;
				}
				//turn on this surf
				trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, token, 0);
			}
		}
	}

	ci->bolt_rhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");

	if (!trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1,
		-1))
	{
		bad_model = qtrue;
	}

	if (!trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", temp_vec, BONE_ANGLES_POSTMULT, POSITIVE_X,
		NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time))
	{
		bad_model = qtrue;
	}

	if (!trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", temp_vec, BONE_ANGLES_POSTMULT, POSITIVE_Z,
		NEGATIVE_Y,
		POSITIVE_X, NULL, 0, cg.time))
	{
		bad_model = qtrue;
	}

	ci->bolt_lhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

	//rhand must always be first bolt. lhand always second. Whichever you want the
	//jetpack bolted to must always be third.
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

	//claw bolts
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

	ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
	if (ci->bolt_head == -1)
	{
		ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
	}

	ci->bolt_motion = trap->G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

	//We need a lower lumbar bolt for footsteps
	ci->bolt_llumbar = trap->G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");

	//Initialize the holster bolts
	ci->holster_saber = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_saber");
	ci->saberHolstered = qfalse;

	ci->holster_saber2 = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_saber2");
	ci->saber2_holstered = qfalse;

	ci->holster_staff = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_staff");
	ci->staff_holstered = qfalse;

	ci->holster_blaster = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_blaster");
	ci->blaster_holstered = 0;

	ci->holster_blaster2 = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_blaster2");
	ci->blaster2_holstered = 0;

	ci->holster_golan = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_golan");
	ci->golan_holstered = qfalse;

	ci->holster_launcher = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*holster_launcher");
	ci->launcher_holstered = 0;

	//offset holster bolts
	ci->bolt_rfemurYZ = trap->G2API_AddBolt(ci->ghoul2Model, 0, "rfemurYZ");
	ci->bolt_lfemurYZ = trap->G2API_AddBolt(ci->ghoul2Model, 0, "lfemurYZ");

	if (ci->bolt_rhand == -1 || ci->bolt_lhand == -1 || ci->bolt_head == -1 || ci->bolt_motion == -1 || ci->bolt_llumbar
		== -1)
	{
		bad_model = qtrue;
	}

	if (bad_model)
	{
		goto retryModel;
	}

	if (!Q_stricmp(model_name, "boba_fett"))
	{
		//special case, turn off the jetpack surfs
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_rjet", TURN_OFF);
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_cjet", TURN_OFF);
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_ljet", TURN_OFF);
	}

	if (clientNum != -1)
	{
		cg_entities[clientNum].ghoul2weapon = NULL;
	}

	Q_strncpyz(ci->teamName, team_name, sizeof ci->teamName);

	// Model icon for drawing the portrait on screen
	ci->modelIcon = trap->R_RegisterShaderNoMip(va("models/players/%s/icon_%s", model_name, skin_name));
	if (!ci->modelIcon)
	{
		int i = 0;
		char icon_name[2048];
		strcpy(icon_name, "icon_");
		int j = strlen(icon_name);
		while (skin_name[i] && skin_name[i] != '|' && j < 1024)
		{
			icon_name[j] = skin_name[i];
			j++;
			i++;
		}
		icon_name[j] = 0;
		if (skin_name[i] == '|')
		{
			//looks like it actually may be a custom model skin, let's try getting the icon...
			ci->modelIcon = trap->R_RegisterShaderNoMip(va("models/players/%s/%s", model_name, icon_name));
		}
	}
	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString(const char* v, vec3_t color)
{
	VectorClear(color);

	const int val = atoi(v);

	if (val < 1 || val > 7)
	{
		VectorSet(color, 1, 1, 1);
		return;
	}

	if (val & 1)
	{
		color[2] = 1.0f;
	}
	if (val & 2)
	{
		color[1] = 1.0f;
	}
	if (val & 4)
	{
		color[0] = 1.0f;
	}
}

/*
====================
CG_ColorFromInt
====================
*/
static void CG_ColorFromInt(const int val, vec3_t color)
{
	VectorClear(color);

	if (val < 1 || val > 7)
	{
		VectorSet(color, 1, 1, 1);
		return;
	}

	if (val & 1)
	{
		color[2] = 1.0f;
	}
	if (val & 2)
	{
		color[1] = 1.0f;
	}
	if (val & 4)
	{
		color[0] = 1.0f;
	}
}

//load anim info
int CG_G2SkelForModel(void* g2)
{
	int anim_index = -1;
	char gla_name[MAX_QPATH];

	gla_name[0] = 0;
	trap->G2API_GetGLAName(g2, 0, gla_name);

	char* slash = Q_strrchr(gla_name, '/');
	if (slash)
	{
		strcpy(slash, "/animation.cfg");

		anim_index = bg_parse_animation_file(gla_name, NULL, qfalse);
	}

	return anim_index;
}

//get the appropriate anim events file index
int CG_G2EvIndexForModel(void* g2, const int anim_index)
{
	int evt_index = -1;
	char gla_name[MAX_QPATH];

	if (anim_index == -1)
	{
		assert(!"shouldn't happen, bad animIndex");
		return -1;
	}

	gla_name[0] = 0;
	trap->G2API_GetGLAName(g2, 0, gla_name);

	char* slash = Q_strrchr(gla_name, '/');
	if (slash)
	{
		slash++;
		*slash = 0;

		evt_index = BG_ParseAnimationEvtFile(gla_name, anim_index, bgNumAnimEvents);
	}

	return evt_index;
}

#define DEFAULT_FEMALE_sound_path "chars/mp_generic_female/misc"//"chars/tavion/misc"
#define DEFAULT_MALE_sound_path "chars/mp_generic_male/misc"//"chars/kyle/misc"

void CG_LoadCISounds(clientInfo_t* ci, const qboolean modelloaded)
{
	fileHandle_t f;
	qboolean is_female = qfalse;
	int i;
	int f_len;
	char sound_path[MAX_QPATH];
	char sound_name[1024];
	const char* s;

	const char* dir = ci->modelName;

	if (!ci->skinName[0] || !Q_stricmp("default", ci->skinName))
	{
		//try default sounds.cfg first
		f_len = trap->FS_Open(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		if (!f)
		{
			//no?  Look for _default sounds.cfg
			f_len = trap->FS_Open(va("models/players/%s/sounds_default.cfg", dir), &f, FS_READ);
		}
	}
	else
	{
		//use the .skin associated with this skin
		f_len = trap->FS_Open(va("models/players/%s/sounds_%s.cfg", dir, ci->skinName), &f, FS_READ);
		if (!f)
		{
			//fall back to default sounds
			f_len = trap->FS_Open(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		}
	}

	sound_path[0] = 0;

	if (f)
	{
		trap->FS_Read(sound_path, f_len, f);
		sound_path[f_len] = 0;

		i = f_len;

		while (i >= 0 && sound_path[i] != '\n')
		{
			if (sound_path[i] == 'f')
			{
				is_female = qtrue;
				sound_path[i] = 0;
			}
			i--;
		}

		i = 0;

		while (sound_path[i] && sound_path[i] != '\r' && sound_path[i] != '\n')
		{
			i++;
		}
		sound_path[i] = 0;

		trap->FS_Close(f);

		if (is_female)
		{
			ci->gender = GENDER_FEMALE;
		}
		else
		{
			ci->gender = GENDER_MALE;
		}
	}
	else
	{
		if (cgs.gametype != GT_SIEGE)
			is_female = ci->gender == GENDER_FEMALE;
		else
			is_female = qfalse;
	}

	trap->S_Shutup(qtrue);

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		s = cg_customSoundNames[i];
		if (!s)
		{
			break;
		}

		Com_sprintf(sound_name, sizeof sound_name, "%s", s + 1);
		COM_StripExtension(sound_name, sound_name, sizeof sound_name);
		//strip the extension because we might want .mp3's

		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (sound_path[0])
		{
			ci->sounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", sound_path, sound_name));
		}
		else
		{
			if (modelloaded)
			{
				ci->sounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", dir, sound_name));
			}
		}

		if (!ci->sounds[i])
		{
			//failed the load, try one out of the generic path
			if (is_female)
			{
				ci->sounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_FEMALE_sound_path, sound_name));
			}
			else
			{
				ci->sounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_MALE_sound_path, sound_name));
			}
		}
	}

	if (cgs.gametype >= GT_TEAM || com_buildScript.integer)
	{
		//load the siege sounds then
		for (i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++)
		{
			s = bg_customSiegeSoundNames[i];
			if (!s)
			{
				break;
			}

			Com_sprintf(sound_name, sizeof sound_name, "%s", s + 1);
			COM_StripExtension(sound_name, sound_name, sizeof sound_name);
			//strip the extension because we might want .mp3's

			ci->siegeSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (sound_path[0])
			{
				ci->siegeSounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", sound_path, sound_name));
				if (!ci->siegeSounds[i])
					ci->siegeSounds[i] = trap->S_RegisterSound(va("sound/%s/%s", sound_path, sound_name));
			}
			else
			{
				if (modelloaded)
				{
					ci->siegeSounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", dir, sound_name));
				}
			}

			if (!ci->siegeSounds[i])
			{
				//failed the load, try one out of the generic path
				if (is_female)
				{
					ci->siegeSounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_FEMALE_sound_path, sound_name));
				}
				else
				{
					ci->siegeSounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_MALE_sound_path, sound_name));
				}
			}
		}
	}

	if (cgs.gametype >= GT_FFA || com_buildScript.integer)
	{
		//load the Duel sounds then
		for (i = 0; i < MAX_CUSTOM_DUEL_SOUNDS; i++)
		{
			s = cg_customDuelSoundNames[i];
			if (!s)
			{
				break;
			}

			Com_sprintf(sound_name, sizeof sound_name, "%s", s + 1);
			COM_StripExtension(sound_name, sound_name, sizeof sound_name);
			//strip the extension because we might want .mp3's

			ci->duelSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (sound_path[0])
			{
				ci->duelSounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", sound_path, sound_name));
			}
			else
			{
				if (modelloaded)
				{
					ci->duelSounds[i] = trap->S_RegisterSound(va("sound/chars/%s/misc/%s", dir, sound_name));
				}
			}

			if (!ci->duelSounds[i])
			{
				//failed the load, try one out of the generic path
				if (is_female)
				{
					ci->duelSounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_FEMALE_sound_path, sound_name));
				}
				else
				{
					ci->duelSounds[i] = trap->S_RegisterSound(va("sound/%s/%s", DEFAULT_MALE_sound_path, sound_name));
				}
			}
		}
	}

	trap->S_Shutup(qfalse);
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
void CG_LoadHolsterData(clientInfo_t* ci);

void CG_LoadClientInfo(clientInfo_t* ci)
{
	char teamname[MAX_QPATH];
	char* fallback_model = DEFAULT_MODEL;

	if (ci->gender == GENDER_FEMALE)
		fallback_model = DEFAULT_MODEL_FEMALE;

	int clientNum = ci - cgs.clientinfo;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = -1;
	}

	ci->deferred = qfalse;

	teamname[0] = 0;
	if (cgs.gametype >= GT_TEAM)
	{
		if (ci->team == TEAM_BLUE)
		{
			Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof teamname);
		}
		else
		{
			Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof teamname);
		}
	}
	if (teamname[0])
	{
		strcat(teamname, "/");
	}
	qboolean modelloaded = qtrue;
	if (cgs.gametype == GT_SIEGE &&
		(ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{
		//yeah.. kind of a hack I guess. Don't care until they are actually ingame with a valid class.
		if (!CG_RegisterClientModelname(ci, fallback_model, "default", teamname, -1))
		{
			trap->Error(ERR_DROP, "DEFAULT_MODEL (%s) failed to register", fallback_model);
		}
	}
	else
	{
		if (!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, teamname, clientNum))
		{
			//trap->Error( ERR_DROP, "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
			//rww - DO NOT error out here! Someone could just type in a nonsense model name and crash everyone's client.
			//Give it a chance to load default model for this client instead.

			// fall back to default team name
			if (cgs.gametype >= GT_TEAM)
			{
				// keep skin name
				if (ci->team == TEAM_BLUE)
				{
					Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof teamname);
				}
				else
				{
					Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof teamname);
				}
				if (!CG_RegisterClientModelname(ci, fallback_model, ci->skinName, teamname, -1))
				{
					trap->Error(ERR_DROP, "DEFAULT_MODEL / skin (%s/%s) failed to register", fallback_model,
						ci->skinName);
				}
			}
			else
			{
				if (!CG_RegisterClientModelname(ci, fallback_model, "default", teamname, -1))
				{
					trap->Error(ERR_DROP, "DEFAULT_MODEL (%s) failed to register", fallback_model);
				}
			}
			modelloaded = qfalse;
		}
	}

	if (clientNum != -1)
	{
		trap->G2API_ClearAttachedInstance(clientNum);
	}

	if (clientNum != -1 && ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		if (cg_entities[clientNum].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		//Attach the instance to this entity num so we can make use of client-server
		//shared operations if possible.
		trap->G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);

		if (trap->G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{
			//check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2,
			cg_entities[clientNum].localAnimIndex);
	}

	ci->newAnims = qfalse;
	if (ci->torsoModel)
	{
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if (trap->R_LerpTag(&tag, ci->torsoModel, 0, 0, 1, "tag_flag"))
		{
			ci->newAnims = qtrue;
		}
	}

	// sounds
	if (cgs.gametype == GT_SIEGE && (ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{
		//don't need to load sounds
	}
	else
	{
		CG_LoadCISounds(ci, modelloaded);
		CG_LoadHolsterData(ci); //initialize our manual holster offset data.
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		if (cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER)
		{
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

//Take care of initializing all the ghoul2 saber stuff based on clientinfo data. -rww
static void CG_InitG2SaberData(const int saberNum, clientInfo_t* ci)
{
	trap->G2API_InitGhoul2Model(&ci->ghoul2Weapons[saberNum], ci->saber[saberNum].model, 0, ci->saber[saberNum].skin,
		0,
		0, 0);

	if (ci->ghoul2Weapons[saberNum])
	{
		int k = 0;

		if (ci->saber[saberNum].skin)
		{
			trap->G2API_SetSkin(ci->ghoul2Weapons[saberNum], 0, ci->saber[saberNum].skin, ci->saber[saberNum].skin);
		}

		if (ci->saber[saberNum].saberFlags & SFL_BOLT_TO_WRIST)
		{
			trap->G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, 3 + saberNum);
		}
		else
		{
			trap->G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, saberNum);
		}

		while (k < ci->saber[saberNum].numBlades)
		{
			const char* tag_name = va("*blade%i", k + 1);
			int tag_bolt = trap->G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, tag_name);

			if (tag_bolt == -1)
			{
				if (k == 0)
				{
					//guess this is an 0ldsk3wl saber
					tag_bolt = trap->G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, "*flash");

					if (tag_bolt == -1)
					{
						assert(0);
					}
					break;
				}

				if (tag_bolt == -1)
				{
					assert(0);
					break;
				}
			}

			k++;
		}
	}
	//init the holster model at the same time
	trap->G2API_InitGhoul2Model(&ci->ghoul2HolsterWeapons[saberNum], ci->saber[saberNum].model, 0,
		ci->saber[saberNum].skin, 0, 0, 0);

	if (ci->ghoul2HolsterWeapons[saberNum])
	{
		if (ci->saber[saberNum].skin)
		{
			trap->G2API_SetSkin(ci->ghoul2HolsterWeapons[saberNum], 0, ci->saber[saberNum].skin,
				ci->saber[saberNum].skin);
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel(const clientInfo_t* from, clientInfo_t* to)
{
	VectorCopy(from->headOffset, to->headOffset);
	to->gender = from->gender;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	assert(to->ghoul2Model != from->ghoul2Model);

	if (to->ghoul2Model && trap->G2_HaveWeGhoul2Models(to->ghoul2Model))
	{
		trap->G2API_CleanGhoul2Models(&to->ghoul2Model);
	}
	if (from->ghoul2Model && trap->G2_HaveWeGhoul2Models(from->ghoul2Model))
	{
		trap->G2API_DuplicateGhoul2Instance(from->ghoul2Model, &to->ghoul2Model);
	}

	to->bolt_head = from->bolt_head;
	to->bolt_lhand = from->bolt_lhand;
	to->bolt_rhand = from->bolt_rhand;
	to->bolt_motion = from->bolt_motion;
	to->bolt_llumbar = from->bolt_llumbar;

	to->siegeIndex = from->siegeIndex;

	to->holster_saber = from->holster_saber;
	to->saberHolstered = from->saberHolstered;

	to->holster_saber2 = from->holster_saber2;
	to->saber2_holstered = from->saber2_holstered;

	to->holster_staff = from->holster_staff;
	to->staff_holstered = from->staff_holstered;

	to->holster_blaster = from->holster_blaster;
	to->blaster_holstered = from->blaster_holstered;

	to->holster_blaster2 = from->holster_blaster2;
	to->blaster2_holstered = from->blaster2_holstered;

	to->holster_golan = from->holster_golan;
	to->golan_holstered = from->golan_holstered;

	to->holster_launcher = from->holster_launcher;
	to->launcher_holstered = from->launcher_holstered;

	//offset bolts
	to->bolt_rfemurYZ = from->bolt_rfemurYZ;
	to->bolt_lfemurYZ = from->bolt_lfemurYZ;

	memcpy(to->holsterData, from->holsterData, sizeof to->holsterData);
	memcpy(to->sounds, from->sounds, sizeof to->sounds);
	memcpy(to->siegeSounds, from->siegeSounds, sizeof to->siegeSounds);
	memcpy(to->duelSounds, from->duelSounds, sizeof to->duelSounds);
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo(clientInfo_t* ci, const int clientNum)
{
	for (int i = 0; i < cgs.maxclients; i++)
	{
		const clientInfo_t* match = &cgs.clientinfo[i];
		if (!match->infoValid)
		{
			continue;
		}
		if (match->deferred)
		{
			continue;
		}
		if (!Q_stricmp(ci->modelName, match->modelName)
			&& !Q_stricmp(ci->skinName, match->skinName)
			&& !Q_stricmp(ci->saber_name, match->saber_name)
			&& !Q_stricmp(ci->saber2Name, match->saber2Name)
			&& (cgs.gametype < GT_TEAM || ci->team == match->team)
			&& ci->siegeIndex == match->siegeIndex
			&& match->ghoul2Model
			&& match->bolt_head) //if the bolts haven't been initialized, this "match" is useless to us
		{
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			//rww - Filthy hack. If this is actually the info already belonging to us, just reassign the pointer.
			//Switching instances when not necessary produces small animation glitches.
			//Actually, before, were we even freeing the instance attached to the old clientinfo before copying
			//this new clientinfo over it? Could be a nasty leak possibility. (though this should remedy it in theory)
			if (clientNum == i)
			{
				if (match->ghoul2Model && trap->G2_HaveWeGhoul2Models(match->ghoul2Model))
				{
					//The match has a valid instance (if it didn't, we'd probably already be fudged (^_^) at this state)
					if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
					{
						//First kill the copy we have if we have one. (but it should be null)
						trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
					}

					VectorCopy(match->headOffset, ci->headOffset);
					//					ci->footsteps = match->footsteps;
					ci->gender = match->gender;

					ci->legsModel = match->legsModel;
					ci->legsSkin = match->legsSkin;
					ci->torsoModel = match->torsoModel;
					ci->torsoSkin = match->torsoSkin;
					ci->modelIcon = match->modelIcon;

					ci->newAnims = match->newAnims;

					ci->bolt_head = match->bolt_head;
					ci->bolt_lhand = match->bolt_lhand;
					ci->bolt_rhand = match->bolt_rhand;
					ci->bolt_motion = match->bolt_motion;
					ci->bolt_llumbar = match->bolt_llumbar;
					ci->siegeIndex = match->siegeIndex;

					ci->holster_saber = match->holster_saber;
					ci->saberHolstered = match->saberHolstered;

					ci->holster_saber2 = match->holster_saber2;
					ci->saber2_holstered = match->saber2_holstered;

					ci->holster_staff = match->holster_staff;
					ci->staff_holstered = match->staff_holstered;

					ci->holster_blaster = match->holster_blaster;
					ci->blaster_holstered = match->blaster_holstered;

					ci->holster_blaster2 = match->holster_blaster2;
					ci->blaster2_holstered = match->blaster2_holstered;

					ci->holster_golan = match->holster_golan;
					ci->golan_holstered = match->golan_holstered;

					ci->holster_launcher = match->holster_launcher;
					ci->launcher_holstered = match->launcher_holstered;

					ci->bolt_rfemurYZ = match->bolt_rfemurYZ;
					ci->bolt_lfemurYZ = match->bolt_lfemurYZ;

					memcpy(ci->holsterData, match->holsterData, sizeof ci->holsterData);
					memcpy(ci->sounds, match->sounds, sizeof ci->sounds);
					memcpy(ci->siegeSounds, match->siegeSounds, sizeof ci->siegeSounds);
					memcpy(ci->duelSounds, match->duelSounds, sizeof ci->duelSounds);

					//We can share this pointer, because it already belongs to this client.
					//The pointer itself and the ghoul2 instance is never actually changed, just passed between
					//clientinfo structures.
					ci->ghoul2Model = match->ghoul2Model;

					//Don't need to do this I guess, whenever this function is called the saber stuff should
					//already be taken care of in the new info.
					/*
					while (k < MAX_SABERS)
					{
						if (match->ghoul2Weapons[k] && match->ghoul2Weapons[k] != ci->ghoul2Weapons[k])
						{
							if (ci->ghoul2Weapons[k])
							{
								trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
							}
							ci->ghoul2Weapons[k] = match->ghoul2Weapons[k];
						}
						k++;
					}
					*/
				}
			}
			else
			{
				CG_CopyClientInfoModel(match, ci);
			}

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo(clientInfo_t* ci)
{
	int i;
	clientInfo_t* match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for (i = 0; i < cgs.maxclients; i++)
	{
		match = &cgs.clientinfo[i];
		if (!match->infoValid || match->deferred)
		{
			continue;
		}
		if (Q_stricmp(ci->skinName, match->skinName) ||
			Q_stricmp(ci->modelName, match->modelName) ||
			//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
			//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			cgs.gametype >= GT_TEAM && ci->team != match->team && ci->team != TEAM_SPECTATOR)
		{
			continue;
		}

		/*
	   if (Q_stricmp(ci->saber_name, match->saber_name) ||
		   Q_stricmp(ci->saber2Name, match->saber2Name))
	   {
		   continue;
	   }
	   */

	   // just load the real info cause it uses the same models and skins
		CG_LoadClientInfo(ci);
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if (cgs.gametype >= GT_TEAM)
	{
		for (i = 0; i < cgs.maxclients; i++)
		{
			match = &cgs.clientinfo[i];
			if (!match->infoValid || match->deferred)
			{
				continue;
			}
			if (ci->team != TEAM_SPECTATOR &&
				(Q_stricmp(ci->skinName, match->skinName) ||
					cgs.gametype >= GT_TEAM && ci->team != match->team))
			{
				continue;
			}

			/*
			if (Q_stricmp(ci->saber_name, match->saber_name) ||
				Q_stricmp(ci->saber2Name, match->saber2Name))
			{
				continue;
			}
			*/

			ci->deferred = qtrue;
			CG_CopyClientInfoModel(match, ci);
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo(ci);
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for (i = 0; i < cgs.maxclients; i++)
	{
		match = &cgs.clientinfo[i];
		if (!match->infoValid)
		{
			continue;
		}

		/*
		if (Q_stricmp(ci->saber_name, match->saber_name) ||
			Q_stricmp(ci->saber2Name, match->saber2Name))
		{
			continue;
		}
		*/

		if (match->deferred)
		{
			//no deferring off of deferred info. Because I said so.
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel(match, ci);
		return;
	}

	// we should never get here...
	//trap->Print( "CG_SetDeferredClientInfo: no valid clients!\n" );
	//Actually it is possible now because of the unique sabers.

	CG_LoadClientInfo(ci);
}

/*
======================
CG_NewClientInfo
======================
*/
void ParseRGBSaber(char* str, vec3_t c);
void CG_ParseScriptedSaber(char* script, clientInfo_t* ci, int snum);
void WP_SetSaber(int entNum, saberInfo_t* sabers, int saberNum, const char* saber_name);
extern void* CG_G2WeaponInstance2(const centity_t* cent, int weapon);

void CG_NewClientInfo(int clientNum, qboolean entities_initialized)
{
	clientInfo_t* ci;
	clientInfo_t new_info;
	const char* configstring;
	const char* v;
	char* slash;
	char* yo, * yo2;
	void* old_ghoul2;
	void* old_g2_weapons[MAX_SABERS];
	void* old_g2_holstered_weapons[MAX_SABERS];
	int i = 0;
	int k = 0;
	qboolean saber_update[MAX_SABERS];

	ci = &cgs.clientinfo[clientNum];

	old_ghoul2 = ci->ghoul2Model;

	while (k < MAX_SABERS)
	{
		old_g2_weapons[k] = ci->ghoul2Weapons[k];
		old_g2_holstered_weapons[k] = ci->ghoul2HolsterWeapons[k];
		k++;
	}

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!configstring[0])
	{
		if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			//clean this stuff up first
			trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
		}
		k = 0;
		while (k < MAX_SABERS)
		{
			if (ci->ghoul2Weapons[k] && trap->G2_HaveWeGhoul2Models(ci->ghoul2Weapons[k]))
			{
				trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
			}
			//racc - kill holster saber ghoul 2 instances
			if (ci->ghoul2HolsterWeapons[k] && trap->G2_HaveWeGhoul2Models(ci->ghoul2HolsterWeapons[k]))
			{
				trap->G2API_CleanGhoul2Models(&ci->ghoul2HolsterWeapons[k]);
			}
			k++;
		}

		memset(ci, 0, sizeof * ci);
		return; // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&new_info, 0, sizeof new_info);

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(new_info.name, v, sizeof new_info.name);
	Q_strncpyz(new_info.cleanname, v, sizeof new_info.cleanname);
	Q_StripColor(new_info.cleanname);

	// colors
	v = Info_ValueForKey(configstring, "c1");
	CG_ColorFromString(v, new_info.color1);

	new_info.icolor1 = atoi(v);

	v = Info_ValueForKey(configstring, "c2");
	CG_ColorFromString(v, new_info.color2);

	new_info.icolor2 = atoi(v);

	// bot skill
	v = Info_ValueForKey(configstring, "skill");
	// players have -1 skill so you can determine the bots from the scoreboard code
	if (v && v[0])
		new_info.botSkill = atoi(v);
	else
		new_info.botSkill = -1;

	// handicap
	v = Info_ValueForKey(configstring, "hc");
	new_info.handicap = atoi(v);

	// wins
	v = Info_ValueForKey(configstring, "w");
	new_info.wins = atoi(v);

	// losses
	v = Info_ValueForKey(configstring, "l");
	new_info.losses = atoi(v);

	// team
	v = Info_ValueForKey(configstring, "t");
	new_info.team = atoi(v);

	//copy team info out to menu
	if (clientNum == cg.clientNum) //this is me
	{
		trap->Cvar_Set("ui_team", v);
	}

	// Gender hints
	if ((v = Info_ValueForKey(configstring, "ds")))
	{
		if (*v == 'f')
		{
			new_info.gender = GENDER_FEMALE;
		}
		else
		{
			new_info.gender = GENDER_MALE;
		}
	}

	yo = Info_ValueForKey(configstring, "tc1");
	ParseRGBSaber(yo, new_info.rgb1);

	yo = Info_ValueForKey(configstring, "tc2");
	ParseRGBSaber(yo, new_info.rgb2);

	yo = Info_ValueForKey(configstring, "ss1");
	if (yo[0] != ':')
	{
		CG_ParseScriptedSaber(":255,0,255:500:0,0,255:500", &new_info, 0);
	}
	else
	{
		CG_ParseScriptedSaber(yo, &new_info, 0);
	}

	yo2 = Info_ValueForKey(configstring, "ss2");
	if (yo2[0] != ':')
	{
		CG_ParseScriptedSaber(":0,255,255:500:0,0,255:500", &new_info, 1);
	}
	else
	{
		CG_ParseScriptedSaber(yo2, &new_info, 1);
	}

	// team task
	v = Info_ValueForKey(configstring, "tt");
	new_info.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey(configstring, "tl");
	new_info.teamLeader = atoi(v);

	// model
	v = Info_ValueForKey(configstring, "model");
	if (cg_forceModel.integer)
	{
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char model_str[MAX_QPATH];
		char* skin;

		trap->Cvar_VariableStringBuffer("model", model_str, sizeof model_str);
		if ((skin = strchr(model_str, '/')) == NULL)
		{
			skin = "default";
		}
		else
		{
			*skin++ = 0;
		}
		Q_strncpyz(new_info.skinName, skin, sizeof new_info.skinName);
		Q_strncpyz(new_info.modelName, model_str, sizeof new_info.modelName);

		if (cgs.gametype >= GT_TEAM)
		{
			// keep skin name
			slash = strchr(v, '/');
			if (slash)
			{
				Q_strncpyz(new_info.skinName, slash + 1, sizeof new_info.skinName);
			}
		}
	}
	else
	{
		Q_strncpyz(new_info.modelName, v, sizeof new_info.modelName);

		slash = strchr(new_info.modelName, '/');
		if (!slash)
		{
			// modelName didn not include a skin name
			Q_strncpyz(new_info.skinName, "default", sizeof new_info.skinName);
		}
		else
		{
			Q_strncpyz(new_info.skinName, slash + 1, sizeof new_info.skinName);
			// truncate modelName
			*slash = 0;
		}
	}

	if (cgs.gametype == GT_SIEGE)
	{
		//entries only sent in siege mode
		//siege desired team
		v = Info_ValueForKey(configstring, "sdt");
		if (v && v[0])
		{
			new_info.siegeDesiredTeam = atoi(v);
		}
		else
		{
			new_info.siegeDesiredTeam = 0;
		}

		//siege classname
		v = Info_ValueForKey(configstring, "siegeclass");
		new_info.siegeIndex = -1;

		if (v)
		{
			siegeClass_t* siege_class = BG_SiegeFindClassByName(v);

			if (siege_class)
			{
				//See if this class forces a model, if so, then use it. Same for skin.
				new_info.siegeIndex = BG_SiegeFindClassIndexByName(v);

				if (siege_class->forcedModel[0])
				{
					Q_strncpyz(new_info.modelName, siege_class->forcedModel, sizeof new_info.modelName);
				}

				if (siege_class->forcedSkin[0])
				{
					Q_strncpyz(new_info.skinName, siege_class->forcedSkin, sizeof new_info.skinName);
				}

				if (siege_class->hasForcedSaberColor)
				{
					new_info.icolor1 = siege_class->forcedSaberColor;

					CG_ColorFromInt(new_info.icolor1, new_info.color1);
				}
				if (siege_class->hasForcedSaber2Color)
				{
					new_info.icolor2 = siege_class->forcedSaber2Color;

					CG_ColorFromInt(new_info.icolor2, new_info.color2);
				}
			}
		}
	}

	saber_update[0] = qfalse;
	saber_update[1] = qfalse;

	//saber being used
	v = Info_ValueForKey(configstring, "st");

	if (v && Q_stricmp(v, ci->saber_name))
	{
		Q_strncpyz(new_info.saber_name, v, 64);
		WP_SetSaber(clientNum, new_info.saber, 0, new_info.saber_name);
		saber_update[0] = qtrue;
	}
	else
	{
		Q_strncpyz(new_info.saber_name, ci->saber_name, 64);
		memcpy(&new_info.saber[0], &ci->saber[0], sizeof new_info.saber[0]);
		new_info.ghoul2Weapons[0] = ci->ghoul2Weapons[0];
		//copy our first ghoul2 holstered saber to the new info file.
		new_info.ghoul2HolsterWeapons[0] = ci->ghoul2HolsterWeapons[0];
	}

	v = Info_ValueForKey(configstring, "st2");

	if (v && Q_stricmp(v, ci->saber2Name))
	{
		Q_strncpyz(new_info.saber2Name, v, 64);
		WP_SetSaber(clientNum, new_info.saber, 1, new_info.saber2Name);
		saber_update[1] = qtrue;
	}
	else
	{
		Q_strncpyz(new_info.saber2Name, ci->saber2Name, 64);
		memcpy(&new_info.saber[1], &ci->saber[1], sizeof new_info.saber[1]);
		new_info.ghoul2Weapons[1] = ci->ghoul2Weapons[1];
		//copy our first ghoul2 holstered saber to the new info file.
		new_info.ghoul2HolsterWeapons[1] = ci->ghoul2HolsterWeapons[1];
	}

	if (saber_update[0] || saber_update[1])
	{
		int j = 0;

		while (j < MAX_SABERS)
		{
			if (saber_update[j])
			{
				if (new_info.saber[j].model[0])
				{
					if (old_g2_weapons[j])
					{
						//free the old instance(s)
						trap->G2API_CleanGhoul2Models(&old_g2_weapons[j]);
						old_g2_weapons[j] = 0;
					}
					if (old_g2_holstered_weapons[j])
					{
						//free the old instance(s)
						trap->G2API_CleanGhoul2Models(&old_g2_holstered_weapons[j]);
						old_g2_holstered_weapons[j] = 0;
					}
					CG_InitG2SaberData(j, &new_info);
				}
				else
				{
					if (old_g2_weapons[j])
					{
						//free the old instance(s)
						trap->G2API_CleanGhoul2Models(&old_g2_weapons[j]);
						old_g2_weapons[j] = 0;
					}
					//racc - delete the old holstered saber instance since we don't have this saber anymore.
					if (old_g2_holstered_weapons[j])
					{
						//free the old instance(s)
						trap->G2API_CleanGhoul2Models(&old_g2_holstered_weapons[j]);
						old_g2_holstered_weapons[j] = 0;
					}
				}
				cg_entities[clientNum].weapon = 0;
				cg_entities[clientNum].ghoul2weapon = NULL; //force a refresh
			}
			j++;
		}
	}

	//Check for any sabers that didn't get set again, if they didn't, then reassign the pointers for the new ci
	k = 0;
	while (k < MAX_SABERS)
	{
		if (old_g2_weapons[k])
		{
			new_info.ghoul2Weapons[k] = old_g2_weapons[k];
			new_info.ghoul2HolsterWeapons[k] = old_g2_holstered_weapons[k];
		}
		k++;
	}

	//duel team
	v = Info_ValueForKey(configstring, "dt");

	if (v)
	{
		new_info.duelTeam = atoi(v);
	}
	else
	{
		new_info.duelTeam = 0;
	}

	// force powers
	v = Info_ValueForKey(configstring, "forcepowers");
	Q_strncpyz(new_info.forcePowers, v, sizeof new_info.forcePowers);

	if (cgs.gametype >= GT_TEAM && !cgs.jediVmerc && cgs.gametype != GT_SIEGE)
	{
		//We won't force colors for siege.
		BG_ValidateSkinForTeam(new_info.modelName, new_info.skinName, new_info.team, new_info.colorOverride);
	}
	else
	{
		new_info.colorOverride[0] = new_info.colorOverride[1] = new_info.colorOverride[2] = 0.0f;
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if (!CG_ScanForExistingClientInfo(&new_info, clientNum))
	{
		// if we are defering loads, just have it pick the first valid
		if (cg.snap && cg.snap->ps.clientNum == clientNum)
		{
			//rww - don't defer your own client info ever
			CG_LoadClientInfo(&new_info);
		}
		else if (cg_deferPlayers.integer && cgs.gametype != GT_SIEGE && !com_buildScript.integer && !cg.loading)
		{
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo(&new_info);
		}
		else
		{
			CG_LoadClientInfo(&new_info);
		}
	}

	if (cg.snap && cg.snap->ps.clientNum == clientNum)
	{
		//adjust our True View position for this new model since this model is for this client
		//Set the eye position based on the trueviewmodel.cfg if it has a position for this model.
		CG_AdjustEyePos(new_info.modelName);
	}

	// replace whatever was there with the new one
	new_info.infoValid = qtrue;
	if (ci->ghoul2Model &&
		ci->ghoul2Model != new_info.ghoul2Model &&
		trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		//We must kill this instance before we remove our only pointer to it from the cgame.
		//Otherwise we will end up with extra instances all over the place, I think.
		trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
	}
	*ci = new_info;

	//force a weapon change anyway, for all clients being rendered to the current client
	while (i < MAX_CLIENTS)
	{
		cg_entities[i].ghoul2weapon = NULL;
		i++;
	}

	if (clientNum != -1)
	{
		//don't want it using an invalid pointer to share
		trap->G2API_ClearAttachedInstance(clientNum);
	}

	// Check if the ghoul2 model changed in any way.  This is safer than assuming we have a legal cent shile loading info.
	if (entities_initialized && ci->ghoul2Model && old_ghoul2 != ci->ghoul2Model)
	{
		// Copy the new ghoul2 model to the centity.
		animation_t* anim;
		centity_t* cent = &cg_entities[clientNum];

		anim = &bgHumanoidAnimations[cg_entities[clientNum].currentState.legsAnim];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int first_frame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= anim->firstFrame + anim->numFrames)
			{
				setFrame = cent->pe.legs.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", first_frame, anim->firstFrame + anim->numFrames,
				flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.legsAnim = 0.0f;
		}

		anim = &bgHumanoidAnimations[cg_entities[clientNum].currentState.torsoAnim];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int first_frame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= anim->firstFrame + anim->numFrames)
			{
				setFrame = cent->pe.torso.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", first_frame, anim->firstFrame + anim->numFrames,
				flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.torsoAnim = 0;
		}

		if (cg_entities[clientNum].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		if (clientNum != -1)
		{
			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);
		}

		if (trap->G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{
			//check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2,
			cg_entities[clientNum].localAnimIndex);

		if (cg_entities[clientNum].currentState.number != cg.predictedPlayerState.clientNum &&
			cg_entities[clientNum].currentState.weapon == WP_SABER)
		{
			cg_entities[clientNum].weapon = cg_entities[clientNum].currentState.weapon;
			if (cg_entities[clientNum].ghoul2 && ci->ghoul2Model)
			{
				CG_CopyG2WeaponInstance(&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon,
					cg_entities[clientNum].ghoul2);
				cg_entities[clientNum].ghoul2weapon = CG_G2WeaponInstance(
					&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon);

				if (cg_entities[clientNum].currentState.eFlags & EF3_DUAL_WEAPONS &&
					cg_entities[clientNum].currentState.weapon == WP_BRYAR_PISTOL)
				{
					cg_entities[clientNum].ghoul2weapon2 = CG_G2WeaponInstance2(
						&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon);
				}
				else
				{
					cg_entities[clientNum].ghoul2weapon2 = NULL;
				}
			}
			if (!cg_entities[clientNum].currentState.saberHolstered)
			{
				//if not holstered set length and desired length for both blades to full right now.
				int j;
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

				i = 0;
				while (i < MAX_SABERS)
				{
					j = 0;
					while (j < ci->saber[i].numBlades)
					{
						ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
						j++;
					}
					i++;
				}
			}
		}
	}
}

qboolean cgQueueLoad = qfalse;
/*
======================
CG_ActualLoadDeferredPlayers

Called at the beginning of CG_Player if cgQueueLoad is set.
======================
*/
void CG_ActualLoadDeferredPlayers(void)
{
	int i;
	clientInfo_t* ci;

	// scan for a deferred player to load
	for (i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++)
	{
		if (ci->infoValid && ci->deferred)
		{
			CG_LoadClientInfo(ci);
			//			break;
		}
	}
}

/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers(void)
{
	cgQueueLoad = qtrue;
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

#define	FOOTSTEP_DISTANCE	32

static void player_foot_step(const vec3_t origin,
	const float orientation,
	const float radius,
	const centity_t* cent, const footstepType_t foot_step_type)
{
	vec3_t end;
	const vec3_t maxs = { 7, 7, 2 };
	const vec3_t mins = { -7, -7, 0 };
	trace_t trace;
	footstep_t sound_type;
	qboolean b_mark = qfalse;
	qhandle_t foot_mark_shader;
	int effect_id = -1;
	//float		alpha;

	// send a trace down from the player to the ground
	VectorCopy(origin, end);
	end[2] -= FOOTSTEP_DISTANCE;

	trap->CM_Trace(&trace, origin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0);

	// no shadow if too high
	if (trace.fraction >= 1.0f)
	{
		return;
	}

	if (foot_step_type == FOOTSTEP_HEAVY_SBD_R || foot_step_type == FOOTSTEP_HEAVY_SBD_L)
	{
		sound_type = FOOTSTEP_SBDRUN;
	}
	else if (foot_step_type == FOOTSTEP_SBD_R || foot_step_type == FOOTSTEP_SBD_L)
	{
		sound_type = FOOTSTEP_SBDWALK;
	}
	else
	{
		//check for foot-steppable surface flag
		switch (trace.surfaceFlags & MATERIAL_MASK)
		{
		case MATERIAL_MUD:
			b_mark = qtrue;
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_MUDRUN;
			}
			else
			{
				sound_type = FOOTSTEP_MUDWALK;
			}
			effect_id = cgs.effects.footstepMud;
			break;
		case MATERIAL_DIRT:
			b_mark = qtrue;
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_DIRTRUN;
			}
			else
			{
				sound_type = FOOTSTEP_DIRTWALK;
			}
			effect_id = cgs.effects.footstepSand;
			break;
		case MATERIAL_SAND:
			b_mark = qtrue;
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_SANDRUN;
			}
			else
			{
				sound_type = FOOTSTEP_SANDWALK;
			}
			effect_id = cgs.effects.footstepSand;
			break;
		case MATERIAL_SNOW:
			b_mark = qtrue;
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_SNOWRUN;
			}
			else
			{
				sound_type = FOOTSTEP_SNOWWALK;
			}
			effect_id = cgs.effects.footstepSnow;
			break;
		case MATERIAL_SHORTGRASS:
		case MATERIAL_LONGGRASS:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_GRASSRUN;
			}
			else
			{
				sound_type = FOOTSTEP_GRASSWALK;
			}
			break;
		case MATERIAL_SOLIDMETAL:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_METALRUN;
			}
			else
			{
				sound_type = FOOTSTEP_METALWALK;
			}
			break;
		case MATERIAL_HOLLOWMETAL:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_PIPERUN;
			}
			else
			{
				sound_type = FOOTSTEP_PIPEWALK;
			}
			break;
		case MATERIAL_GRAVEL:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_GRAVELRUN;
			}
			else
			{
				sound_type = FOOTSTEP_GRAVELWALK;
			}
			effect_id = cgs.effects.footstepGravel;
			break;
		case MATERIAL_LAVA:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_DIRTRUN;
			}
			else
			{
				sound_type = FOOTSTEP_DIRTWALK;
			}
			effect_id = cgs.effects.footstepSand;
			break;
		case MATERIAL_CARPET:
		case MATERIAL_FABRIC:
		case MATERIAL_CANVAS:
		case MATERIAL_RUBBER:
		case MATERIAL_PLASTIC:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_RUGRUN;
			}
			else
			{
				sound_type = FOOTSTEP_RUGWALK;
			}
			break;
		case MATERIAL_SOLIDWOOD:
		case MATERIAL_HOLLOWWOOD:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_WOODRUN;
			}
			else
			{
				sound_type = FOOTSTEP_WOODWALK;
			}
			break;

		default:
			//fall through
		case MATERIAL_GLASS:
		case MATERIAL_WATER:
		case MATERIAL_FLESH:
		case MATERIAL_BPGLASS:
		case MATERIAL_DRYLEAVES:
		case MATERIAL_GREENLEAVES:
		case MATERIAL_TILES:
		case MATERIAL_PLASTER:
		case MATERIAL_SHATTERGLASS:
		case MATERIAL_ARMOR:
		case MATERIAL_COMPUTER:
		case MATERIAL_CONCRETE:
		case MATERIAL_ROCK:
		case MATERIAL_ICE:
		case MATERIAL_MARBLE:
			if (foot_step_type == FOOTSTEP_HEAVY_R || foot_step_type == FOOTSTEP_HEAVY_L)
			{
				sound_type = FOOTSTEP_STONERUN;
			}
			else
			{
				sound_type = FOOTSTEP_STONEWALK;
			}
			break;
		}
	}
	if (sound_type < FOOTSTEP_TOTAL)
	{
		trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_BODY, cgs.media.footsteps[sound_type][rand() & 3]);
	}

	if (cg_footsteps.integer < 4)
	{
		//debugging - 4 always does footstep effect
		if (cg_footsteps.integer < 2) //1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

	if (effect_id != -1)
	{
		trap->FX_PlayEffectID(effect_id, trace.endpos, trace.plane.normal, -1, -1, qfalse);
	}

	if (cg_footsteps.integer < 4)
	{
		//debugging - 4 always does footstep effect
		if (!b_mark || cg_footsteps.integer < 3) //1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

	switch (foot_step_type)
	{
	case FOOTSTEP_HEAVY_R:
	case FOOTSTEP_HEAVY_SBD_R:
		foot_mark_shader = cgs.media.fshrMarkShader;
		break;
	case FOOTSTEP_HEAVY_L:
	case FOOTSTEP_HEAVY_SBD_L:
		foot_mark_shader = cgs.media.fshlMarkShader;
		break;
	case FOOTSTEP_R:
	case FOOTSTEP_SBD_R:
		foot_mark_shader = cgs.media.fsrMarkShader;
		break;
	default:
	case FOOTSTEP_L:
	case FOOTSTEP_SBD_L:
		foot_mark_shader = cgs.media.fslMarkShader;
		break;
	}

	// fade the shadow out with height
	//	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	if (trace.plane.normal[0] || trace.plane.normal[1] || trace.plane.normal[2])
	{
		CG_ImpactMark(foot_mark_shader, trace.endpos, trace.plane.normal,
			orientation, 1, 1, 1, 1.0f, qfalse, radius, qfalse);
	}
}

static void CG_PlayerFootsteps(centity_t* cent, const footstepType_t foot_step_type)
{
	if (!cg_footsteps.integer)
	{
		return;
	}

	//FIXME: make this a feature of NPCs in the NPCs.cfg? Specify a footstep shader, if any?
	if (cent->currentState.NPC_class != CLASS_ATST
		&& cent->currentState.NPC_class != CLASS_CLAW
		&& cent->currentState.NPC_class != CLASS_FISH
		&& cent->currentState.NPC_class != CLASS_FLIER2
		&& cent->currentState.NPC_class != CLASS_GLIDER
		&& cent->currentState.NPC_class != CLASS_INTERROGATOR
		&& cent->currentState.NPC_class != CLASS_MURJJ
		&& cent->currentState.NPC_class != CLASS_PROBE
		&& cent->currentState.NPC_class != CLASS_R2D2
		&& cent->currentState.NPC_class != CLASS_R5D2
		&& cent->currentState.NPC_class != CLASS_REMOTE
		&& cent->currentState.NPC_class != CLASS_SEEKER
		&& cent->currentState.NPC_class != CLASS_SENTRY
		&& cent->currentState.NPC_class != CLASS_SWAMP)
	{
		mdxaBone_t boltMatrix;
		vec3_t temp_angles, side_origin;
		int foot_bolt;

		temp_angles[PITCH] = 0;
		temp_angles[YAW] = cent->pe.legs.yawAngle;
		temp_angles[ROLL] = 0;

		switch (foot_step_type)
		{
		case FOOTSTEP_R:
		case FOOTSTEP_HEAVY_R:
		case FOOTSTEP_SBD_R:
		case FOOTSTEP_HEAVY_SBD_R:
			foot_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot"); //cent->gent->footRBolt;
			break;
		case FOOTSTEP_L:
		case FOOTSTEP_HEAVY_L:
		case FOOTSTEP_SBD_L:
		case FOOTSTEP_HEAVY_SBD_L:
		default:
			foot_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot"); //cent->gent->footLBolt;
			break;
		}

		//FIXME: get yaw orientation of the foot and use on decal
		trap->G2API_GetBoltMatrix(cent->ghoul2, 0, foot_bolt, &boltMatrix, temp_angles, cent->lerpOrigin,
			cg.time, cgs.game_models, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, side_origin);
		side_origin[2] += 15; //fudge up a bit for coplanar
		player_foot_step(side_origin, cent->pe.legs.yawAngle, 6, cent, foot_step_type);
	}
}

void CG_PlayerAnimEventDo(centity_t* cent, animevent_t* anim_event)
{
	soundChannel_t channel = CHAN_AUTO;
	const clientInfo_t* client;
	qhandle_t swing_sound;
	qhandle_t spin_sound;

	if (!cent || !anim_event)
	{
		return;
	}

	switch (anim_event->eventType)
	{
	case AEV_SOUNDCHAN:
	case AEV_AMBIENT:
		channel = (soundChannel_t)anim_event->eventData[AED_SOUNDCHANNEL];
	case AEV_SOUND:
	{
		// are there variations on the sound?
		if (anim_event->eventData[AED_SOUNDINDEX_START] == 0)
		{
			const int n = Q_irand(anim_event->eventData[AED_CSOUND_RANDSTART],
				anim_event->eventData[AED_CSOUND_RANDSTART] + anim_event->eventData[
					AED_SOUND_NUMRANDOMSNDS]);
			trap->S_StartSound(NULL, cent->currentState.number, channel,
				CG_CustomSound(cent->currentState.number, va(anim_event->stringData, n)));
		}
		else
		{
			const int hold_snd = anim_event->eventData[AED_SOUNDINDEX_START + Q_irand(
				0, anim_event->eventData[AED_SOUND_NUMRANDOMSNDS])];
			if (hold_snd > 0)
			{
				trap->S_StartSound(NULL, cent->currentState.number, channel, hold_snd);
			}
		}
	}
	break;
	case AEV_SABER_SWING:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if (client && client->infoValid && client->saber[anim_event->eventData[AED_SABER_SWING_saber_num]].swingSound[
			0])
		{
			//custom swing sound
			swing_sound = client->saber[0].swingSound[Q_irand(0, 2)];
		}
		else
		{
			int random_swing;
			switch (anim_event->eventData[AED_SABER_SWING_TYPE])
			{
			default:
			case 0: //SWING_FAST
				random_swing = Q_irand(1, 3);
				break;
			case 1: //SWING_MEDIUM
				random_swing = Q_irand(4, 6);
				break;
			case 2: //SWING_STRONG
				random_swing = Q_irand(7, 9);
				break;
			}
			swing_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", random_swing));
		}
		trap->S_StartSound(cent->currentState.pos.trBase, cent->currentState.number, CHAN_AUTO, swing_sound);
		break;
	case AEV_SABER_SPIN:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if (client
			&& client->infoValid
			&& client->saber[AED_SABER_SPIN_saber_num].spinSound)
		{
			//use override
			spin_sound = client->saber[AED_SABER_SPIN_saber_num].spinSound;
		}
		else
		{
			switch (anim_event->eventData[AED_SABER_SPIN_TYPE])
			{
			case 0: //saberspinoff
				spin_sound = trap->S_RegisterSound("sound/weapons/saber/saberspinoff.wav");
				break;
			case 1: //saberspin
				spin_sound = trap->S_RegisterSound("sound/weapons/saber/saberspin.wav");
				break;
			case 2: //saberspin1
				spin_sound = trap->S_RegisterSound("sound/weapons/saber/saberspin1.wav");
				break;
			case 3: //saberspin2
				spin_sound = trap->S_RegisterSound("sound/weapons/saber/saberspin2.wav");
				break;
			case 4: //saberspin3
				spin_sound = trap->S_RegisterSound("sound/weapons/saber/saberspin3.wav");
				break;
			default: //random saberspin1-3
				spin_sound = trap->S_RegisterSound(va("sound/weapons/saber/saberspin%d.wav", Q_irand(1, 3)));
				break;
			}
		}
		if (spin_sound)
		{
			trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_AUTO, spin_sound);
		}
		break;
	case AEV_FOOTSTEP:
		CG_PlayerFootsteps(cent, anim_event->eventData[AED_FOOTSTEP_TYPE]);
		break;
	case AEV_EFFECT:
#if 0 //SP method
		//add bolt, play effect
		if (animEvent->stringData != NULL && cent && cent->gent && cent->gent->ghoul2.size())
		{//have a bolt name we want to use
			animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt(&cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData);
			animEvent->stringData = NULL;//so we don't try to do this again
		}
		if (animEvent->eventData[AED_BOLTINDEX] != -1)
		{//have a bolt we want to play the effect on
			G_PlayEffect(animEvent->eventData[AED_EFFECTINDEX], cent->gent->playerModel, animEvent->eventData[AED_BOLTINDEX], cent->currentState.clientNum);
		}
		else
		{//play at origin?  FIXME: maybe allow a fwd/rt/up offset?
			theFxScheduler.PlayEffect(animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, qfalse);
		}
#else //my method
		if (anim_event->stringData && anim_event->stringData[0] && cent && cent->ghoul2)
		{
			anim_event->eventData[AED_model_index] = 0;
			if (Q_stricmpn("*blade", anim_event->stringData, 6) == 0
				|| Q_stricmp("*flash", anim_event->stringData) == 0)
			{
				//must be a weapon, try weapon 0?
				anim_event->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt(cent->ghoul2, 1, anim_event->stringData);
				if (anim_event->eventData[AED_BOLTINDEX] != -1)
				{
					//found it!
					anim_event->eventData[AED_model_index] = 1;
				}
				else
				{
					//hmm, just try on the player model, then?
					anim_event->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt(cent->ghoul2, 0, anim_event->stringData);
				}
			}
			else
			{
				anim_event->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt(cent->ghoul2, 0, anim_event->stringData);
			}
			anim_event->stringData[0] = 0;
		}
		if (anim_event->eventData[AED_BOLTINDEX] != -1)
		{
			vec3_t l_angles;
			vec3_t b_point, bAngle;
			mdxaBone_t matrix;

			VectorSet(l_angles, 0, cent->lerpAngles[YAW], 0);

			trap->G2API_GetBoltMatrix(cent->ghoul2, anim_event->eventData[AED_model_index],
				anim_event->eventData[AED_BOLTINDEX], &matrix, l_angles,
				cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, b_point);
			VectorSet(bAngle, 0, 1, 0);

			trap->FX_PlayEffectID(anim_event->eventData[AED_EFFECTINDEX], b_point, bAngle, -1, -1, qfalse);
		}
		else
		{
			vec3_t bAngle;

			VectorSet(bAngle, 0, 1, 0);
			trap->FX_PlayEffectID(anim_event->eventData[AED_EFFECTINDEX], cent->lerpOrigin, bAngle, -1, -1, qfalse);
		}
#endif
		break;
		//Would have to keep track of this on server to for these, it's not worth it.
	case AEV_FIRE:
	case AEV_MOVE:
		break;
	default:
		return;
	}
}

/*
void CG_PlayerAnimEvents( int animFileIndex, int eventFileIndex, qboolean torso, int oldFrame, int frame, const vec3_t org, int entNum )

play any keyframed sounds - only when start a new frame
This func is called once for legs and once for torso
*/
void CG_PlayerAnimEvents(const int animFileIndex, const int eventFileIndex, const qboolean torso,
	const int old_frame, const int frame, const int entNum)
{
	int first_frame = 0, last_frame = 0;
	qboolean do_event = qfalse, in_same_anim = qfalse, loop_anim = qfalse, anim_backward = qfalse;
	animevent_t* anim_events;

	if (torso)
	{
		anim_events = bgAllEvents[eventFileIndex].torsoAnimEvents;
	}
	else
	{
		anim_events = bgAllEvents[eventFileIndex].legsAnimEvents;
	}
	if (fabs((float)(old_frame - frame)) > 1.0f)
	{
		//given a range, see if keyFrame falls in that range
		int old_anim, anim;
		if (torso)
		{
			old_anim = cg_entities[entNum].currentState.torsoAnim;
			anim = cg_entities[entNum].nextState.torsoAnim;
		}
		else
		{
			old_anim = cg_entities[entNum].currentState.legsAnim;
			anim = cg_entities[entNum].nextState.legsAnim;
		}
		if (anim != old_anim)
		{
			//not in same anim
			in_same_anim = qfalse;
			//FIXME: we *could* see if the oldFrame was *just about* to play the keyframed sound...
		}
		else
		{
			//still in same anim, check for looping anim
			in_same_anim = qtrue;
			const animation_t* animation = &bgAllAnims[animFileIndex].anims[anim];
			anim_backward = animation->frameLerp < 0;
			if (animation->loopFrames != -1)
			{
				//a looping anim!
				loop_anim = qtrue;
				first_frame = animation->firstFrame;
				last_frame = animation->firstFrame + animation->numFrames;
			}
		}
	}

	// Check for anim sound
	for (int i = 0; i < MAX_ANIM_EVENTS; ++i)
	{
		if (anim_events[i].eventType == AEV_NONE) // No event, end of list
		{
			break;
		}

		qboolean match = qfalse;
		if (anim_events[i].keyFrame == frame)
		{
			//exact match
			match = qtrue;
		}
		else if (fabs((float)(old_frame - frame)) > 1.0f)
		{
			//given a range, see if keyFrame falls in that range
			if (in_same_anim)
			{
				//if changed anims altogether, sorry, the sound is lost
				if (fabs((float)(old_frame - anim_events[i].keyFrame)) <= 3.0f
					|| fabs((float)(frame - anim_events[i].keyFrame)) <= 3.0f)
				{
					//must be at least close to the keyframe
					if (anim_backward)
					{
						//animation plays backwards
						if (old_frame > anim_events[i].keyFrame && frame < anim_events[i].keyFrame)
						{
							//old to new passed through keyframe
							match = qtrue;
						}
						else if (loop_anim)
						{
							//hmm, didn't pass through it linearally, see if we looped
							if (anim_events[i].keyFrame >= first_frame && anim_events[i].keyFrame < last_frame)
							{
								//keyframe is in this anim
								if (old_frame > anim_events[i].keyFrame
									&& frame > old_frame)
								{
									//old to new passed through keyframe
									match = qtrue;
								}
							}
						}
					}
					else
					{
						//anim plays forwards
						if (old_frame < anim_events[i].keyFrame && frame > anim_events[i].keyFrame)
						{
							//old to new passed through keyframe
							match = qtrue;
						}
						else if (loop_anim)
						{
							//hmm, didn't pass through it linearally, see if we looped
							if (anim_events[i].keyFrame >= first_frame && anim_events[i].keyFrame < last_frame)
							{
								//keyframe is in this anim
								if (old_frame < anim_events[i].keyFrame
									&& frame < old_frame)
								{
									//old to new passed through keyframe
									match = qtrue;
								}
							}
						}
					}
				}
			}
		}
		if (match)
		{
			switch (anim_events[i].eventType)
			{
			case AEV_SOUND:
			case AEV_SOUNDCHAN:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_SOUND_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_SOUND_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_SABER_SWING:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_SABER_SWING_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_SABER_SWING_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_SABER_SPIN:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_SABER_SPIN_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_SABER_SPIN_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_FOOTSTEP:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_FOOTSTEP_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_FOOTSTEP_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_EFFECT:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_EFFECT_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_EFFECT_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_FIRE:
				// Determine probability of playing sound
				if (!anim_events[i].eventData[AED_FIRE_PROBABILITY]) // 100%
				{
					do_event = qtrue;
				}
				else if (anim_events[i].eventData[AED_FIRE_PROBABILITY] > Q_irand(0, 99))
				{
					do_event = qtrue;
				}
				break;
			case AEV_MOVE:
				do_event = qtrue;
				break;
			case AEV_AMBIENT:
				break;
			default:
				//doEvent = qfalse;//implicit
				break;
			}
			// do event
			if (do_event)
			{
				CG_PlayerAnimEventDo(&cg_entities[entNum], &anim_events[i]);
			}
		}
	}
}

extern int CheckAnimFrameForEventType(const animevent_t* anim_events, int key_frame, animEventType_t event_type);
//Checks for and plays Ambient model sounds
void CG_PlayerAmbientEvents(centity_t* cent)
{
	animevent_t* anim_events;
	int event_num;

	if (cent->currentState.eFlags & EF_DEAD || cent->currentState.eType == ET_PLAYER &&
		(cg.predictedPlayerState.pm_type == PM_INTERMISSION || cgs.clientinfo[cent->currentState.clientNum].team ==
			TEAM_SPECTATOR))
	{
		//Shouldn't play ambient sounds when dead; in intermission; or a spectator.
		return;
	}

	//Torso Ambient
	if (cg.time > cent->AmbTorsoTimer)
	{
		anim_events = bgAllEvents[cent->eventAnimIndex].torsoAnimEvents;
		event_num = CheckAnimFrameForEventType(anim_events, 0, AEV_AMBIENT);
		if (event_num != -1)
		{
			//Find the Ambient event.
			if (!anim_events[event_num].eventData[AED_SOUND_PROBABILITY] || anim_events[event_num].eventData[
				AED_SOUND_PROBABILITY] > Q_irand(0, 99))
			{
				CG_PlayerAnimEventDo(cent, &anim_events[event_num]);
			}

			//Ok, now reset the timer.
			cent->AmbTorsoTimer = cg.time + (anim_events[event_num].ambtime - anim_events[event_num].ambrandom) +
				Q_irand(
					0, anim_events[event_num].ambrandom + anim_events[event_num].ambrandom);
		}
	}
	if (cg.time > cent->AmbLegsTimer)
	{
		anim_events = bgAllEvents[cent->eventAnimIndex].legsAnimEvents;
		event_num = CheckAnimFrameForEventType(anim_events, 0, AEV_AMBIENT);
		if (event_num != -1)
		{
			//Find the Ambient event.
			if (!anim_events[event_num].eventData[AED_SOUND_PROBABILITY] || anim_events[event_num].eventData[
				AED_SOUND_PROBABILITY] > Q_irand(0, 99))
			{
				CG_PlayerAnimEventDo(cent, &anim_events[event_num]);
			}
			//Ok, now reset the timer.
			cent->AmbLegsTimer = cg.time + (anim_events[event_num].ambtime - anim_events[event_num].ambrandom) +
				Q_irand(
					0, anim_events[event_num].ambrandom + anim_events[event_num].ambrandom);
		}
	}
}

void CG_TriggerAnimSounds(centity_t* cent)
{
	//this also sets the lerp frames, so I suggest you keep calling it regardless of if you want anim sounds.
	int cur_frame = 0;
	float currentFrame = 0;

	assert(cent->localAnimIndex >= 0);

	const int s_file_index = cent->eventAnimIndex;

	if (trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &currentFrame, cgs.game_models, 0))
	{
		// the above may have failed, not sure what to do about it, current frame will be zero in that case
		cur_frame = floor(currentFrame);
	}
	if (cur_frame != cent->pe.legs.frame)
	{
		CG_PlayerAnimEvents(cent->localAnimIndex, s_file_index, qfalse, cent->pe.legs.frame, cur_frame,
			cent->currentState.number);
	}
	cent->pe.legs.oldFrame = cent->pe.legs.frame;
	cent->pe.legs.frame = cur_frame;

	if (cent->noLumbar)
	{
		//probably a droid or something.
		cent->pe.torso.oldFrame = cent->pe.legs.oldFrame;
		cent->pe.torso.frame = cent->pe.legs.frame;
		return;
	}

	if (trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.game_models, 0))
	{
		cur_frame = floor(currentFrame);
	}
	if (cur_frame != cent->pe.torso.frame)
	{
		CG_PlayerAnimEvents(cent->localAnimIndex, s_file_index, qtrue, cent->pe.torso.frame, cur_frame,
			cent->currentState.number);
	}
	cent->pe.torso.oldFrame = cent->pe.torso.frame;
	cent->pe.torso.frame = cur_frame;
	cent->pe.torso.backlerp = 1.0f - (currentFrame - (float)cur_frame);
	CG_PlayerAmbientEvents(cent);
}

static qboolean CG_FirstAnimFrame(const lerpFrame_t* lf, qboolean torso_only, float speed_scale);

qboolean CG_InRoll(const centity_t* cent)
{
	switch (cent->currentState.legsAnim)
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if (cent->pe.legs.animationTime > cg.time)
		{
			return qtrue;
		}
		break;
	default:;
	}
	return qfalse;
}

qboolean CG_InRollAnim(const centity_t* cent)
{
	switch (cent->currentState.legsAnim)
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

/*
===============
CG_SetLerpFrameAnimation
===============
*/
static void CG_SetLerpFrameAnimation(centity_t* cent, clientInfo_t* ci, lerpFrame_t* lf, const int new_animation,
	const float anim_speed_mult, const qboolean torso_only, const qboolean flip_state)
{
	float animSpeed;
	int flags = BONE_ANIM_OVERRIDE_FREEZE;
	const float old_speed = lf->animationSpeed;

	const qboolean is_holding_block_button = cent->currentState.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	const qboolean is_holding_block_button_and_attack = cent->currentState.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
		? qtrue
		: qfalse;

	if (cent->localAnimIndex > 0)
	{
		//rockettroopers can't have broken arms, nor can anything else but humanoids
		ci->brokenLimbs = cent->currentState.brokenLimbs;
	}

	const int old_anim = lf->animationNumber;

	lf->animationNumber = new_animation;

	if (new_animation < 0 || new_animation >= MAX_TOTALANIMATIONS)
	{
		trap->Error(ERR_DROP, "Bad animation number: %i", new_animation);
	}

	animation_t* anim = &bgAllAnims[cent->localAnimIndex].anims[new_animation];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + abs(anim->frameLerp);

	if (cent->localAnimIndex > 1 &&
		anim->firstFrame == 0 &&
		anim->numFrames == 0)
	{
		//We'll allow this for non-humanoids.
		return;
	}

	if (cg_debugAnim.integer && (cg_debugAnim.integer < 0 || cg_debugAnim.integer == cent->currentState.clientNum))
	{
		if (lf == &cent->pe.legs)
		{
			trap->Print("%d: %d TORSO Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, new_animation,
				GetStringForID(animTable, new_animation));
		}
		else
		{
			trap->Print("%d: %d LEGS Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, new_animation,
				GetStringForID(animTable, new_animation));
		}
	}

	if (cent->ghoul2)
	{
		int blendTime = 100;
		qboolean resume_frame = qfalse;
		int begin_frame = -1;
		int first_frame;
		int last_frame;
#if 0 //disabled for now
		float unused;
#endif

		animSpeed = 50.0f / anim->frameLerp;
		if (lf->animation->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (animSpeed < 0)
		{
			last_frame = anim->firstFrame;
			first_frame = anim->firstFrame + anim->numFrames;
		}
		else
		{
			first_frame = anim->firstFrame;
			last_frame = anim->firstFrame + anim->numFrames;
		}

		if (cg_animBlend.integer)
		{
			flags |= BONE_ANIM_BLEND;
		}

		if (BG_InDeathAnim(new_animation))
		{
			flags &= ~BONE_ANIM_BLEND;
		}
		else if (old_anim != -1 &&
			BG_InDeathAnim(old_anim))
		{
			flags &= ~BONE_ANIM_BLEND;
		}

		if (flags & BONE_ANIM_BLEND)
		{
			if (PM_FlippingAnim(new_animation))
			{
				blendTime = 200;
			}
			else if (old_anim != -1 &&
				PM_FlippingAnim(old_anim))
			{
				blendTime = 200;
			}
		}

		if (!PM_WalkingOrRunningAnim(cent->currentState.legsAnim) && !PM_InKataAnim(cent->currentState.torsoAnim))
		{
			//make smooth animations
			if ((is_holding_block_button_and_attack || is_holding_block_button) && cent->currentState.saber_move == LS_READY)
			{
				blendTime *= 1.8f;
			}
		}

		animSpeed *= anim_speed_mult;

		pm_saber_start_trans_anim(cent->currentState.number, cent->currentState.fireflag, cent->currentState.weapon,
			new_animation, &animSpeed, cent->currentState.userInt3);

		if (torso_only)
		{
			if (lf->animationTorsoSpeed != anim_speed_mult && new_animation == old_anim &&
				flip_state == lf->lastFlip)
			{
				//same animation, but changing speed, so we will want to resume off the frame we're on.
				resume_frame = qtrue;
			}
			lf->animationTorsoSpeed = anim_speed_mult;
			if ((is_holding_block_button_and_attack || is_holding_block_button) && cent->currentState.saber_move == LS_READY)
			{
				blendTime *= 1.8f;
			}
		}
		else
		{
			if (lf->animationSpeed != anim_speed_mult && new_animation == old_anim &&
				flip_state == lf->lastFlip)
			{
				//same animation, but changing speed, so we will want to resume off the frame we're on.
				resume_frame = qtrue;
			}
			lf->animationSpeed = anim_speed_mult;
		}

		//vehicles may have torso etc but we only want to animate the root bone
		if (cent->currentState.NPC_class == CLASS_VEHICLE)
		{
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", first_frame, last_frame, flags, animSpeed, cg.time,
				begin_frame, blendTime);
			return;
		}

		if (torso_only && !cent->noLumbar)
		{
			//rww - The guesswork based on the lerp frame figures is usually BS, so I've resorted to a call to get the frame of the bone directly.
			float gb_ac_frame = 0;
			if (resume_frame)
			{
				//we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &gb_ac_frame, NULL, 0);
				begin_frame = gb_ac_frame;
			}

			//even if resuming, also be sure to check if we are running the same frame on the legs. If so, we want to use their frame no matter what.
			trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &gb_ac_frame, NULL, 0);

			if (cent->currentState.torsoAnim == cent->currentState.legsAnim && gb_ac_frame >= anim->firstFrame &&
				gb_ac_frame <= anim->firstFrame + anim->numFrames)
			{
				//if the legs are already running this anim, pick up on the exact same frame to avoid the "wobbly spine" problem.
				begin_frame = gb_ac_frame;
			}

			if (first_frame > last_frame || ci->torsoAnim == new_animation)
			{
				//don't resume on backwards playing animations.. I guess.
				begin_frame = -1;
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", first_frame, last_frame, flags, animSpeed,
				cg.time,
				begin_frame, blendTime);

			// Update the torso frame with the new animation
			cent->pe.torso.frame = first_frame;

			if (ci)
			{
				ci->torsoAnim = new_animation;
			}
		}
		else
		{
			if (resume_frame)
			{
				//we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				float gb_ac_frame = 0;
				trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &gb_ac_frame, NULL, 0);
				begin_frame = gb_ac_frame;
			}

			if (begin_frame < first_frame || begin_frame > last_frame)
			{
				//out of range, don't use it then.
				begin_frame = -1;
			}

			if (cent->currentState.torsoAnim == cent->currentState.legsAnim &&
				(ci->legsAnim != new_animation || old_speed != animSpeed))
			{
				//alright, we are starting an anim on the legs, and that same anim is already playing on the toro, so pick up the frame.
				float gb_ac_frame = 0;
				const int old_begin_frame = begin_frame;

				trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &gb_ac_frame, NULL, 0);
				begin_frame = gb_ac_frame;
				if (begin_frame < first_frame || begin_frame > last_frame)
				{
					//out of range, don't use it then.
					begin_frame = old_begin_frame;
				}
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", first_frame, last_frame, flags, animSpeed, cg.time,
				begin_frame, blendTime);

			if (ci)
			{
				ci->legsAnim = new_animation;
			}
		}

		if (cent->localAnimIndex <= 1 && cent->currentState.torsoAnim == new_animation && !cent->noLumbar)
		{
			//make sure we're humanoid before we access the motion bone
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", first_frame, last_frame, flags, animSpeed, cg.time,
				begin_frame, blendTime);
		}

#if 0 //disabled for now
		if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char* brokenBone = "lhumerus";
			animation_t* armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[cent->localAnimIndex].anims[BOTH_DEAD21];
			ci->brokenLimbs = cent->currentState.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame + armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = BONE_ANIM_OVERRIDE_LOOP;

			if (cg_animBlend.integer)
			{
				armFlags |= BONE_ANIM_BLEND;
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, blendTime);
		}
		else if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char* brokenBone = "rhumerus";
			char* supportBone = "lhumerus";

			ci->brokenLimbs = cent->currentState.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//cent->currentState.weapon == WP_MELEE ||
				cent->currentState.weapon != WP_SABER ||
				PM_SaberStanceAnim(newAnimation) ||
				PM_RunningAnim(newAnimation)) &&
				cent->currentState.torsoAnim == newAnimation &&
				(!ci->saber[1].model[0] || cent->currentState.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t* armAnim;

				if (cent->currentState.weapon == WP_MELEE || cent->currentState.weapon == WP_SABER)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[BOTH_ATTACK2];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame + armAnim->numFrames;
					armLastFrame = armAnim->firstFrame + armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					//No blend on the broken arm

					trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 0);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}

				if (newAnimation != BOTH_MELEE1 &&
					newAnimation != BOTH_MELEE2 &&
					newAnimation != BOTH_MELEE3 &&
					newAnimation != BOTH_MELEE4 &&
					newAnimation != BOTH_MELEE5 &&
					newAnimation != BOTH_MELEE6 &&
					newAnimation != BOTH_MELEE_L &&
					newAnimation != BOTH_MELEE_R &&
					newAnimation != BOTH_MELEEUP &&
					newAnimation != BOTH_WOOKIE_SLAP &&
					(newAnimation == TORSO_WEAPONREADY2 || newAnimation == BOTH_ATTACK2 || cent->currentState.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[BOTH_STAND2];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame + armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					if (cg_animBlend.integer)
					{
						armFlags |= BONE_ANIM_BLEND;
					}

					trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}
			}
			else if (cent->currentState.torsoAnim == newAnimation)
			{ //otherwise, keep it set to the same as the torso
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
			}
		}
		else if (ci &&
			(ci->brokenLimbs ||
				trap->G2API_GetBoneFrame(cent->ghoul2, "lhumerus", cg.time, &unused, cgs.game_models, 0) ||
				trap->G2API_GetBoneFrame(cent->ghoul2, "rhumerus", cg.time, &unused, cgs.game_models, 0)))
			//rwwFIXMEFIXME: brokenLimbs gets stomped sometimes, but it shouldn't.
		{ //remove the bone now so it can be set again
			char* brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (ci->brokenLimbs & (1 << BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1 << BROKENLIMB_LARM);
			}
			else if (ci->brokenLimbs & (1 << BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1 << BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				if (!trap->G2API_RemoveBone(cent->ghoul2, "lhumerus", 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove lhumerus\n");
				}
			}

			if (!brokenBone)
			{
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "rhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap->G2API_RemoveBone(cent->ghoul2, "lhumerus", 0);
				trap->G2API_RemoveBone(cent->ghoul2, "rhumerus", 0);
				ci->brokenLimbs = 0;
			}
			else
			{
				//Set the flags and stuff to 0, so that the remove will succeed
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, 0, 1, 0, 0, cg.time, -1, 0);

				//Now remove it
				if (!trap->G2API_RemoveBone(cent->ghoul2, brokenBone, 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove %s\n", brokenBone);
				}
				ci->brokenLimbs &= ~broken;
			}
		}
#endif
	}
}

/*
===============
CG_FirstAnimFrame

Returns true if the lerpframe is on its first frame of animation.
Otherwise false.

This is used to scale an animation into higher-speed without restarting
the animation before it completes at normal speed, in the case of a looping
animation (such as the leg running anim).
===============
*/
static qboolean CG_FirstAnimFrame(const lerpFrame_t* lf, const qboolean torso_only, const float speed_scale)
{
	if (torso_only)
	{
		if (lf->animationTorsoSpeed == speed_scale)
		{
			return qfalse;
		}
	}
	else
	{
		if (lf->animationSpeed == speed_scale)
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(centity_t* cent, clientInfo_t* ci, lerpFrame_t* lf, const qboolean flip_state,
	const int new_animation,
	const float speed_scale, const qboolean torso_only)
{
	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (cent->currentState.forceFrame)
	{
		if (lf->lastForcedFrame != cent->currentState.forceFrame)
		{
			const int flags = BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND;
			const float animSpeed = 1.0f;
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", cent->currentState.forceFrame,
				cent->currentState.forceFrame + 1, flags, animSpeed, cg.time, -1, 150);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", cent->currentState.forceFrame,
				cent->currentState.forceFrame + 1, flags, animSpeed, cg.time, -1, 150);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", cent->currentState.forceFrame,
				cent->currentState.forceFrame + 1, flags, animSpeed, cg.time, -1, 150);
		}

		lf->lastForcedFrame = cent->currentState.forceFrame;

		lf->animationNumber = 0;
	}
	else
	{
		lf->lastForcedFrame = -1;

		if (new_animation != lf->animationNumber || cent->currentState.brokenLimbs != ci->brokenLimbs || lf->lastFlip !=
			flip_state || !lf->animation || CG_FirstAnimFrame(lf, torso_only, speed_scale))
		{
			CG_SetLerpFrameAnimation(cent, ci, lf, new_animation, speed_scale, torso_only, flip_state);
		}
	}

	lf->lastFlip = flip_state;

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}

	if (lf->frameTime)
	{
		// calculate current lerp value
		if (lf->frameTime == lf->oldFrameTime)
			lf->backlerp = 0.0f;
		else
			lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(centity_t* cent, clientInfo_t* ci, lerpFrame_t* lf, const int animation_number,
	const qboolean torso_only)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(cent, ci, lf, animation_number, 1, torso_only, qfalse);

	if (lf->animation->frameLerp < 0)
	{
		//Plays backwards
		lf->oldFrame = lf->frame = lf->animation->firstFrame + lf->animation->numFrames;
	}
	else
	{
		lf->oldFrame = lf->frame = lf->animation->firstFrame;
	}
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t* cent, int* legs_old, int* legs, float* legs_back_lerp, int* torso_old,
	int* torso,
	float* torso_back_lerp)
{
	clientInfo_t* ci;
	float speed_scale;

	const int clientNum = cent->currentState.clientNum;

	const qboolean is_holding_block_button = cent->currentState.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	const qboolean is_holding_block_button_and_attack = cent->currentState.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
		? qtrue
		: qfalse;

	if (cg_noPlayerAnims.integer)
	{
		*legs_old = *legs = *torso_old = *torso = 0;
		return;
	}

	if (!PM_RunningAnim(cent->currentState.legsAnim) &&
		!PM_WalkingAnim(cent->currentState.legsAnim))
	{
		//if legs are not in a walking/running anim then just animate at standard speed
		speed_scale = 1.0f;
	}
	else if (cent->currentState.forcePowersActive & 1 << FP_SPEED)
	{
		speed_scale = 1.7f;
	}
	else
	{
		speed_scale = 1.0f;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[clientNum];
	}

	CG_RunLerpFrame(cent, ci, &cent->pe.legs, cent->currentState.legsFlip, cent->currentState.legsAnim, speed_scale,
		qfalse);

	if (!(cent->currentState.forcePowersActive & 1 << FP_RAGE))
	{
		//don't affect torso anim speed unless raged
		speed_scale = 1.0f;
	}
	else
	{
		//speedScale = 1.7f;
		speed_scale = 1.0f;
	}

	*legs_old = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legs_back_lerp = cent->pe.legs.backlerp;

	// If this is not a vehicle, you may lerp the frame (since vehicles never have a torso anim). -AReis
	if (cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		if (!PM_WalkingOrRunningAnim(cent->currentState.legsAnim) && !PM_InKataAnim(cent->currentState.torsoAnim))
		{
			//make smooth animations
			if ((is_holding_block_button_and_attack || is_holding_block_button) && cent->currentState.saber_move == LS_READY)
			{
				speed_scale *= 0.3f;
			}
		}

		CG_RunLerpFrame(cent, ci, &cent->pe.torso, cent->currentState.torsoFlip, cent->currentState.torsoAnim,
			speed_scale, qtrue);

		*torso_old = cent->pe.torso.oldFrame;
		*torso = cent->pe.torso.frame;
		*torso_back_lerp = cent->pe.torso.backlerp;
	}
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

#if 0
typedef struct boneAngleParms_s {
	void* ghoul2;
	int modelIndex;
	char* boneName;
	vec3_t angles;
	int flags;
	int up;
	int right;
	int forward;
	qhandle_t* modelList;
	int blendTime;
	int currentTime;

	qboolean refreshSet;
} boneAngleParms_t;

boneAngleParms_t cgBoneAnglePostSet;
#endif

void CG_G2SetBoneAngles(void* ghoul2, int modelIndex, const char* boneName, const vec3_t angles, const int flags,
	const int up, const int right, const int forward, qhandle_t* modelList,
	const int blendTime, const int currentTime)
{
	//we want to hold off on setting the bone angles until the end of the frame, because every time we set
	//them the entire skeleton has to be reconstructed.
#if 0
	//This function should ONLY be called from CG_Player() or a function that is called only within CG_Player().
	//At the end of the frame we will check to use this information to call SetBoneAngles
	memset(&cgBoneAnglePostSet, 0, sizeof(cgBoneAnglePostSet));
	cgBoneAnglePostSet.ghoul2 = ghoul2;
	cgBoneAnglePostSet.modelIndex = modelIndex;
	cgBoneAnglePostSet.boneName = (char*)boneName;

	cgBoneAnglePostSet.angles[0] = angles[0];
	cgBoneAnglePostSet.angles[1] = angles[1];
	cgBoneAnglePostSet.angles[2] = angles[2];

	cgBoneAnglePostSet.flags = flags;
	cgBoneAnglePostSet.up = up;
	cgBoneAnglePostSet.right = right;
	cgBoneAnglePostSet.forward = forward;
	cgBoneAnglePostSet.modelList = modelList;
	cgBoneAnglePostSet.blendTime = blendTime;
	cgBoneAnglePostSet.currentTime = currentTime;

	cgBoneAnglePostSet.refreshSet = qtrue;
#endif
	//We don't want to go with the delayed approach, we want out bolt points and everything to be updated in realtime.
	//We'll just take the reconstructs and live with them.
	trap->G2API_SetBoneAngles(ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList,
		blendTime, currentTime);
}

/*
================
CG_Rag_Trace

Variant on CG_Trace. Doesn't trace for ents because ragdoll engine trace code has no entity
trace access. Maybe correct this sometime, so bmodel col. at least works with ragdoll.
But I don't want to slow it down..
================
*/
void CG_Rag_Trace(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	const int mask)
{
	trap->CM_Trace(result, start, end, mins, maxs, 0, mask, 0);
	result->entityNum = result->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

//#define _RAG_BOLT_TESTING

#ifdef _RAG_BOLT_TESTING
void CG_TempTestFunction(centity_t* cent, vec3_t forcedAngles)
{
	mdxaBone_t boltMatrix;
	vec3_t tAngles;
	vec3_t bOrg;
	vec3_t bDir;
	vec3_t uOrg;

	VectorSet(tAngles, 0, cent->lerpAngles[YAW], 0);

	trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, tAngles, cent->lerpOrigin,
		cg.time, cgs.game_models, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bOrg);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, bDir);

	VectorMA(bOrg, 40, bDir, uOrg);

	CG_TestLine(bOrg, uOrg, 50, 0x0000ff, 1);

	cent->turAngles[YAW] = forcedAngles[YAW];
}
#endif

//list of valid ragdoll effectors
static const char* cg_effectorStringTable[] =
{
	//commented out the ones I don't want dragging to affect
	//	"thoracic",
	//	"rhand",
	"lhand",
	"rtibia",
	"ltibia",
	"rtalus",
	"ltalus",
	//	"rradiusX",
	"lradiusX",
	"rfemurX",
	"lfemurX",
	//	"ceyebrow",
	NULL //always terminate
};

//we want to see which way the pelvis is facing to get a relatively oriented base settling frame
//this is to avoid the arms stretching in opposite directions on the body trying to reach the base
//pose if the pelvis is flipped opposite of the base pose or something -rww
static int CG_RagAnimForPositioning(centity_t* cent)
{
	vec3_t dir;
	mdxaBone_t matrix;

	assert(cent->ghoul2);
	const int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "pelvis");
	assert(bolt > -1);

	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &matrix, cent->turAngles, cent->lerpOrigin,
		cg.time, cgs.game_models, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Z, dir);

	if (dir[2] > 0.0f)
	{
		//facing up
		return BOTH_DEADFLOP2;
	}
	//facing down
	return BOTH_DEADFLOP1;
}

//rww - cgame interface for the ragdoll stuff.
//Returns qtrue if the entity is now in a ragdoll state, otherwise qfalse.
qboolean CG_RagDoll(centity_t* cent, vec3_t forced_angles)
{
	vec3_t used_org;
	qboolean in_something = qfalse;

	if (!broadsword.integer || cent->currentState.botclass == BCLASS_SBD || cent->currentState.botclass ==
		BCLASS_DROIDEKA)
	{
		return qfalse;
	}

	if (cent->localAnimIndex)
	{
		//don't rag non-humanoids
		return qfalse;
	}

	VectorCopy(cent->lerpOrigin, used_org);

	if (!cent->isRagging)
	{
		//If we're not in a ragdoll state, perform the checks.
		if (cent->currentState.eFlags & EF_RAG)
		{
			//want to go into it no matter what then
			in_something = qtrue;
		}
		else if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
		{
			vec3_t c_vel;

			VectorCopy(cent->currentState.pos.trDelta, c_vel);

			if (VectorNormalize(c_vel) > 400)
			{
				//if he's flying through the air at a good enough speed, switch into ragdoll
				in_something = qtrue;
			}
		}

		if (cent->currentState.eType == ET_BODY)
		{
			//just rag bodies immediately if their own was ragging on respawn
			if (cent->ownerRagging)
			{
				cent->isRagging = qtrue;
				return qfalse;
			}
		}

		if (broadsword.integer > 1 && cent->currentState.botclass != BCLASS_SBD && cent->currentState.botclass !=
			BCLASS_DROIDEKA)
		{
			in_something = qtrue;
		}

		if (!in_something)
		{
			int anim = cent->currentState.legsAnim;
			int dur = (bgAllAnims[cent->localAnimIndex].anims[anim].numFrames - 1) * fabs(
				bgAllAnims[cent->localAnimIndex].anims[anim].frameLerp);
			int i = 0;
			int bolt_checks[5];
			vec3_t bolt_points[5];
			vec3_t t_ang;
			qboolean death_done = qfalse;
			trace_t tr;
			mdxaBone_t boltMatrix;

			VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

			if (cent->pe.legs.animationTime > 50 && cg.time - cent->pe.legs.animationTime > dur)
			{
				//Looks like the death anim is done playing
				death_done = qtrue;
			}

			if (death_done)
			{
				//only trace from the hands if the death anim is already done.
				bolt_checks[0] = trap->G2API_AddBolt(cent->ghoul2, 0, "rhand");
				bolt_checks[1] = trap->G2API_AddBolt(cent->ghoul2, 0, "lhand");
			}
			else
			{
				//otherwise start the trace loop at the cranium.
				i = 2;
			}
			bolt_checks[2] = trap->G2API_AddBolt(cent->ghoul2, 0, "cranium");
			bolt_checks[3] = trap->G2API_AddBolt(cent->ghoul2, 0, "rtalus");
			bolt_checks[4] = trap->G2API_AddBolt(cent->ghoul2, 0, "ltalus");

			//This may seem bad, but since we have a bone cache now it should manage to not be too disgustingly slow.
			//Do the head first, because the hands reference it anyway.
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt_checks[2], &boltMatrix, t_ang, cent->lerpOrigin, cg.time,
				cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_points[2]);

			while (i < 5)
			{
				vec3_t tr_end;
				vec3_t tr_start;
				if (i < 2)
				{
					//when doing hands, trace to the head instead of origin
					trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt_checks[i], &boltMatrix, t_ang, cent->lerpOrigin,
						cg.time, cgs.game_models, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_points[i]);
					VectorCopy(bolt_points[i], tr_start);
					VectorCopy(bolt_points[2], tr_end);
				}
				else
				{
					if (i > 2)
					{
						//2 is the head, which already has the bolt point.
						trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt_checks[i], &boltMatrix, t_ang,
							cent->lerpOrigin,
							cg.time, cgs.game_models, cent->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_points[i]);
					}
					VectorCopy(bolt_points[i], tr_start);
					VectorCopy(cent->lerpOrigin, tr_end);
				}

				//Now that we have all that sorted out, trace between the two points we desire.
				CG_Rag_Trace(&tr, tr_start, NULL, NULL, tr_end, MASK_SOLID);

				if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid)
				{
					//Hit something or start in solid, so flag it and break.
					//This is a slight hack, but if we aren't done with the death anim, we don't really want to
					//go into ragdoll unless our body has a relatively "flat" pitch.
#if 0
					vec3_t vSub;

					//Check the pitch from the head to the right foot (should be reasonable)
					VectorSubtract(boltPoints[2], boltPoints[3], vSub);
					VectorNormalize(vSub);
					vectoangles(vSub, vSub);

					if (deathDone || (vSub[PITCH] < 50 && vSub[PITCH] > -50))
					{
						inSomething = qtrue;
					}
#else
					in_something = qtrue;
#endif
					break;
				}

				i++;
			}
		}

		if (in_something)
		{
			cent->isRagging = qtrue;
#if 0
			VectorClear(cent->lerpOriginOffset);
#endif
		}
	}

	if (cent->isRagging)
	{
		int rag_anim;
		//We're in a ragdoll state, so make the call to keep our positions updated and whatnot.
		sharedRagDollParams_t t_parms;
		sharedRagDollUpdateParams_t tu_parms;

		rag_anim = CG_RagAnimForPositioning(cent);

		if (cent->ikStatus)
		{
			//ik must be reset before ragdoll is started, or you'll get some interesting results.
			trap->G2API_SetBoneIKState(cent->ghoul2, cg.time, NULL, IKS_NONE, NULL);
			cent->ikStatus = qfalse;
		}

		//these will be used as "base" frames for the ragoll settling.
		t_parms.startFrame = bgAllAnims[cent->localAnimIndex].anims[rag_anim].firstFrame;
		// + bgAllAnims[cent->localAnimIndex].anims[ragAnim].numFrames;
		t_parms.endFrame = bgAllAnims[cent->localAnimIndex].anims[rag_anim].firstFrame + bgAllAnims[cent->
			localAnimIndex].
			anims[rag_anim].numFrames;
#if 0
		{
			float animSpeed = 0;
			int blendTime = 600;
			int flags = 0;//BONE_ANIM_OVERRIDE_FREEZE;

			if (bgAllAnims[cent->localAnimIndex].anims[ragAnim].loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			/*
			if (cg_animBlend.integer)
			{
				flags |= BONE_ANIM_BLEND;
			}
			*/

			animSpeed = 50.0f / bgAllAnims[cent->localAnimIndex].anims[ragAnim].frameLerp;
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
		}
#elif 1 //with my new method of doing things I want it to continue the anim
		{
			float currentFrame;
			int startFrame, endFrame;
			int flags;
			float animSpeed;

			if (trap->G2API_GetBoneAnim(cent->ghoul2, "model_root", cg.time, &currentFrame, &startFrame, &endFrame,
				&flags, &animSpeed, cgs.game_models, 0))
			{
				//lock the anim on the current frame.
				int blendTime = 500;
				animation_t* cur_anim = &bgAllAnims[cent->localAnimIndex].anims[cent->currentState.legsAnim];

				if (currentFrame >= cur_anim->firstFrame + cur_anim->numFrames - 1)
				{
					//this is sort of silly but it works for now.
					currentFrame = cur_anim->firstFrame + cur_anim->numFrames - 2;
				}

				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", currentFrame, currentFrame + 1, flags,
					animSpeed, cg.time, currentFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", currentFrame, currentFrame + 1, flags,
					animSpeed,
					cg.time, currentFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", currentFrame, currentFrame + 1, flags, animSpeed,
					cg.time, currentFrame, blendTime);
			}
		}
#endif
		CG_G2SetBoneAngles(cent->ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, cgs.game_models, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, cgs.game_models, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, cgs.game_models, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, cgs.game_models, 0, cg.time);

		VectorCopy(forced_angles, t_parms.angles);
		VectorCopy(used_org, t_parms.position);
		VectorCopy(cent->modelScale, t_parms.scale);
		t_parms.me = cent->currentState.number;

		t_parms.collisionType = 1;
		t_parms.RagPhase = RP_DEATH_COLLISION;
		t_parms.fShotStrength = 4;

		trap->G2API_SetRagDoll(cent->ghoul2, &t_parms);

		VectorCopy(forced_angles, tu_parms.angles);
		VectorCopy(used_org, tu_parms.position);
		VectorCopy(cent->modelScale, tu_parms.scale);
		tu_parms.me = cent->currentState.number;
		tu_parms.settleFrame = t_parms.endFrame - 1;

		if (cent->currentState.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(tu_parms.velocity);
		}
		else
		{
			VectorScale(cent->currentState.pos.trDelta, 2.0f, tu_parms.velocity);
		}

		trap->G2API_AnimateG2Models(cent->ghoul2, cg.time, &tu_parms);

		//So if we try to get a bolt point it's still correct
		cent->turAngles[YAW] =
			cent->lerpAngles[YAW] =
			cent->pe.torso.yawAngle =
			cent->pe.legs.yawAngle = forced_angles[YAW];

		if (cent->currentState.ragAttach &&
			(cent->currentState.eType != ET_NPC || cent->currentState.NPC_class != CLASS_VEHICLE))
		{
			centity_t* grabEnt;

			if (cent->currentState.ragAttach == ENTITYNUM_NONE)
			{
				//switch cl 0 and entity_num_none, so we can operate on the "if non-0" concept
				grabEnt = &cg_entities[0];
			}
			else
			{
				grabEnt = &cg_entities[cent->currentState.ragAttach];
			}

			if (grabEnt->ghoul2)
			{
				mdxaBone_t matrix;
				vec3_t b_org;
				vec3_t this_hand;
				vec3_t hands;
				vec3_t pcj_min, pcj_max;
				vec3_t p_dif;
				vec3_t thor_point;
				int thor_bolt;

				//Get the person who is holding our hand's hand location
				trap->G2API_GetBoltMatrix(grabEnt->ghoul2, 0, 0, &matrix, grabEnt->turAngles, grabEnt->lerpOrigin,
					cg.time, cgs.game_models, grabEnt->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, b_org);

				//Get our hand's location
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, 0, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.game_models, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, this_hand);

				//Get the position of the thoracic bone for hinting its velocity later on
				thor_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "thoracic");
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, thor_bolt, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.game_models, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thor_point);

				VectorSubtract(b_org, this_hand, hands);

				if (VectorLength(hands) < 3.0f)
				{
					trap->G2API_RagForceSolve(cent->ghoul2, qfalse);
				}
				else
				{
					trap->G2API_RagForceSolve(cent->ghoul2, qtrue);
				}

				//got the hand pos of him, now we want to make our hand go to it
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhand", b_org);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rradius", b_org);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", b_org);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", b_org);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", b_org);

				//Make these two solve quickly so we can update decently
				trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 1.5f);
				trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 1.5f);

				//Break the constraints on them I suppose
				VectorSet(pcj_min, -999, -999, -999);
				VectorSet(pcj_max, 999, 999, 999);
				trap->G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcj_min, pcj_max);
				trap->G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcj_min, pcj_max);

				cent->overridingBones = cg.time + 2000;

				//hit the thoracic velocity to the hand point
				VectorSubtract(b_org, thor_point, hands);
				VectorNormalize(hands);
				VectorScale(hands, 2048.0f, hands);
				trap->G2API_RagEffectorKick(cent->ghoul2, "thoracic", hands);
				trap->G2API_RagEffectorKick(cent->ghoul2, "ceyebrow", hands);

				VectorSubtract(cent->ragLastOrigin, cent->lerpOrigin, p_dif);
				VectorCopy(cent->lerpOrigin, cent->ragLastOrigin);

				if (cent->ragLastOriginTime >= cg.time && cent->currentState.groundEntityNum != ENTITYNUM_NONE)
				{
					float dif_len;
					//make sure it's reasonably updated
					dif_len = VectorLength(p_dif);
					if (dif_len > 0.0f)
					{
						//if we're being dragged, then kick all the bones around a bit
						vec3_t d_vel;
						int i = 0;

						if (dif_len < 12.0f)
						{
							VectorScale(p_dif, 12.0f / dif_len, p_dif);
							dif_len = 12.0f;
						}

						while (cg_effectorStringTable[i])
						{
							vec3_t r_vel;
							VectorCopy(p_dif, d_vel);
							d_vel[2] = 0;

							//Factor in a random velocity
							VectorSet(r_vel, flrand(-0.1f, 0.1f), flrand(-0.1f, 0.1f), flrand(0.1f, 0.5));
							VectorScale(r_vel, 8.0f, r_vel);

							VectorAdd(d_vel, r_vel, d_vel);
							VectorScale(d_vel, 10.0f, d_vel);

							trap->G2API_RagEffectorKick(cent->ghoul2, cg_effectorStringTable[i], d_vel);

#if 0
							{
								mdxaBone_t bm;
								vec3_t borg;
								vec3_t vorg;
								int b = trap->G2API_AddBolt(cent->ghoul2, 0, cg_effectorStringTable[i]);

								trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &bm, cent->turAngles, cent->lerpOrigin, cg.time,
									cgs.game_models, cent->modelScale);
								BG_GiveMeVectorFromMatrix(&bm, ORIGIN, borg);

								VectorMA(borg, 1.0f, dVel, vorg);

								CG_TestLine(borg, vorg, 50, 0x0000ff, 1);
							}
#endif

							i++;
						}
					}
				}
				cent->ragLastOriginTime = cg.time + 1000;
			}
		}
		else if (cent->overridingBones)
		{
			//reset things to their normal rag state
			vec3_t pcj_min, pcj_max;
			vec3_t d_vel;

			//got the hand pos of him, now we want to make our hand go to it
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhand", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rradius", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", NULL);

			VectorSet(d_vel, 0.0f, 0.0f, -64.0f);
			trap->G2API_RagEffectorKick(cent->ghoul2, "rhand", d_vel);

			trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 0.0f);
			trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 0.0f);

			VectorSet(pcj_min, -100.0f, -40.0f, -15.0f);
			VectorSet(pcj_max, -15.0f, 80.0f, 15.0f);
			trap->G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcj_min, pcj_max);

			VectorSet(pcj_min, -25.0f, -20.0f, -20.0f);
			VectorSet(pcj_max, 90.0f, 20.0f, -20.0f);
			trap->G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcj_min, pcj_max);

			if (cent->overridingBones < cg.time)
			{
				trap->G2API_RagForceSolve(cent->ghoul2, qfalse);
				cent->overridingBones = 0;
			}
			else
			{
				trap->G2API_RagForceSolve(cent->ghoul2, qtrue);
			}
		}

		return qtrue;
	}

	return qfalse;
}

//set the bone angles of this client entity based on data from the server -rww
void CG_G2ServerBoneAngles(const centity_t* cent)
{
	int i = 0;
	int bone = cent->currentState.boneIndex1;
	vec3_t bone_angles;

	VectorCopy(cent->currentState.boneAngles1, bone_angles);

	while (i < 4)
	{
		//cycle through the 4 bone index values on the entstate
		if (bone)
		{
			//if it's non-0 then it could have something in it.
			const char* boneName = CG_ConfigString(CS_G2BONES + bone);

			if (boneName && boneName[0])
			{
				//got the bone, now set the angles from the corresponding entitystate boneangles value.
				const int flags = BONE_ANGLES_POSTMULT;

				//get the orientation out of our bit field
				const int forward = cent->currentState.boneOrient & 7; //3 bits from bit 0
				const int right = cent->currentState.boneOrient >> 3 & 7; //3 bits from bit 3
				const int up = cent->currentState.boneOrient >> 6 & 7; //3 bits from bit 6

				trap->G2API_SetBoneAngles(cent->ghoul2, 0, boneName, bone_angles, flags, up, right, forward,
					cgs.game_models, 100, cg.time);
			}
		}

		switch (i)
		{
		case 0:
			bone = cent->currentState.boneIndex2;
			VectorCopy(cent->currentState.boneAngles2, bone_angles);
			break;
		case 1:
			bone = cent->currentState.boneIndex3;
			VectorCopy(cent->currentState.boneAngles3, bone_angles);
			break;
		case 2:
			bone = cent->currentState.boneIndex4;
			VectorCopy(cent->currentState.boneAngles4, bone_angles);
			break;
		default:
			break;
		}

		i++;
	}
}

/*
-------------------------
CG_G2SetHeadBlink
-------------------------
*/
static void CG_G2SetHeadBlink(const centity_t* cent, const qboolean b_start)
{
	vec3_t desired_angles;
	int blendTime = 80;
	qboolean b_wink = qfalse;
	const int h_reye = trap->G2API_AddBolt(cent->ghoul2, 0, "reye");
	const int h_leye = trap->G2API_AddBolt(cent->ghoul2, 0, "leye");

	if (h_leye == -1)
	{
		return;
	}

	VectorClear(desired_angles);

	if (b_start)
	{
		desired_angles[YAW] = -50;
		if (Q_flrand(0.0f, 1.0f) > 0.95f)
		{
			b_wink = qtrue;
			blendTime /= 3;
		}
	}
	trap->G2API_SetBoneAngles(cent->ghoul2, 0, "leye", desired_angles,
		BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time);

	if (h_reye == -1)
	{
		return;
	}

	if (!b_wink)
	{
		trap->G2API_SetBoneAngles(cent->ghoul2, 0, "reye", desired_angles,
			BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time);
	}
}

/*
-------------------------
CG_G2SetHeadAnims
-------------------------
*/
static void CG_G2SetHeadAnim(const centity_t* cent, const int anim)
{
	const animation_t* animations = bgAllAnims[cent->localAnimIndex].anims;
	int anim_flags = BONE_ANIM_OVERRIDE;
	const float time_scale_mod = timescale.value ? 1.0 / timescale.value : 1.0;
	const float animSpeed = 50.0f / animations[anim].frameLerp * time_scale_mod;
	int first_frame;
	int last_frame;

	if (animations[anim].numFrames <= 0)
	{
		return;
	}
	if (anim == FACE_DEAD)
	{
		anim_flags |= BONE_ANIM_OVERRIDE_FREEZE;
	}
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
	if (animSpeed < 0)
	{
		//play anim backwards
		last_frame = animations[anim].firstFrame - 1;
		first_frame = animations[anim].numFrames - 1 + animations[anim].firstFrame;
	}
	else
	{
		first_frame = animations[anim].firstFrame;
		last_frame = animations[anim].numFrames + animations[anim].firstFrame;
	}

	// first decide if we are doing an animation on the head already
	//	int startFrame, endFrame;
	//	const qboolean animatingHead =  gi.G2API_GetAnimRangeIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone, &startFrame, &endFrame);

	//	if (!animatingHead || ( animations[anim].firstFrame != startFrame ) )// only set the anim if we aren't going to do the same animation again
	{
		const int blendTime = 50;
		//	gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone,
		//		firstFrame, lastFrame, animFlags, animSpeed, cg.time, -1, blendTime);
		trap->G2API_SetBoneAnim(cent->ghoul2, 0, "face", first_frame, last_frame, anim_flags, animSpeed,
			cg.time, -1, blendTime);
	}
}

qboolean CG_G2PlayerHeadAnims(const centity_t* cent)
{
	clientInfo_t* ci;
	int anim = -1;

	if (cent->localAnimIndex > 1)
	{
		//only do this for humanoids
		return qfalse;
	}

	if (cent->noFace)
	{
		// i don't have a face
		return qfalse;
	}

	if (cent->currentState.number < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}
	else
	{
		ci = cent->npcClient;
	}

	if (!ci)
	{
		return qfalse;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//Dead people close their eyes and don't make faces!
		anim = FACE_DEAD;
		ci->facial_blink = -1;
	}
	else
	{
		if (!ci->facial_blink)
		{
			// set the timers
			ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
			ci->facial_frown = cg.time + flrand(6000.0, 10000.0);
			ci->facial_aux = cg.time + flrand(6000.0, 10000.0);
		}

		//are we blinking?
		if (ci->facial_blink < 0)
		{
			// yes, check if we are we done blinking ?
			if (-ci->facial_blink < cg.time)
			{
				// yes, so reset blink timer
				ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
				CG_G2SetHeadBlink(cent, qfalse); //stop the blink
			}
		}
		else // no we aren't blinking
		{
			if (ci->facial_blink < cg.time) // but should we start ?
			{
				CG_G2SetHeadBlink(cent, qtrue);
				if (ci->facial_blink == 1)
				{
					//requested to stay shut by SET_FACEEYESCLOSED
					ci->facial_blink = -(cg.time + 99999999.0f); // set blink timer
				}
				else
				{
					ci->facial_blink = -(cg.time + 300.0f); // set blink timer
				}
			}
		}

		const int voice_volume = trap->S_GetVoiceVolume(cent->currentState.number);

		if (voice_volume > 0) // if we aren't talking, then it will be 0, -1 for talking but paused
		{
			anim = FACE_TALK1 + voice_volume - 1;
		}
		else if (voice_volume == 0) //don't do aux if in a slient part of speech
		{
			//not talking
			if (ci->facial_aux < 0) // are we auxing ?
			{
				//yes
				if (-ci->facial_aux < cg.time) // are we done auxing ?
				{
					// yes, reset aux timer
					ci->facial_aux = cg.time + flrand(7000.0, 10000.0);
				}
				else
				{
					// not yet, so choose aux
					anim = FACE_ALERT;
				}
			}
			else // no we aren't auxing
			{
				// but should we start ?
				if (ci->facial_aux < cg.time)
				{
					//yes
					anim = FACE_ALERT;
					// set aux timer
					ci->facial_aux = -(cg.time + 2000.0);
				}
			}

			if (anim != -1) //we we are auxing, see if we should override with a frown
			{
				if (ci->facial_frown < 0) // are we frowning ?
				{
					// yes,
					if (-ci->facial_frown < cg.time) //are we done frowning ?
					{
						// yes, reset frown timer
						ci->facial_frown = cg.time + flrand(7000.0, 10000.0);
					}
					else
					{
						// not yet, so choose frown
						anim = FACE_FROWN;
					}
				}
				else // no we aren't frowning
				{
					// but should we start ?
					if (ci->facial_frown < cg.time)
					{
						anim = FACE_FROWN;
						// set frown timer
						ci->facial_frown = -(cg.time + 2000.0);
					}
				}
			}
		} //talking
	} //dead
	if (anim != -1)
	{
		CG_G2SetHeadAnim(cent, anim);
		return qtrue;
	}
	return qfalse;
}

static void CG_G2PlayerAngles(centity_t* cent, matrix3_t legs, vec3_t legs_angles)
{
	clientInfo_t* ci;

	//rww - now do ragdoll stuff
	if (cent->currentState.eFlags & EF_DEAD || cent->currentState.eFlags & EF_RAG)
	{
		vec3_t forced_angles;

		VectorClear(forced_angles);
		forced_angles[YAW] = cent->lerpAngles[YAW];

		if (CG_RagDoll(cent, forced_angles))
		{
			//if we managed to go into the rag state, give our ent axis the forced angles and return.
			AnglesToAxis(forced_angles, legs);
			VectorCopy(forced_angles, legs_angles);
			return;
		}
	}
	else if (cent->isRagging)
	{
		cent->isRagging = qfalse;
		trap->G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	//rww - Quite possibly the most arguments for a function ever.
	if (cent->localAnimIndex <= 1)
	{
		//don't do these things on non-humanoids
		vec3_t look_angles;
		const entityState_t* emplaced = NULL;

		if (cent->currentState.hasLookTarget)
		{
			VectorSubtract(cg_entities[cent->currentState.lookTarget].lerpOrigin, cent->lerpOrigin, look_angles);
			vectoangles(look_angles, look_angles);
			ci->lookTime = cg.time + 1000;
		}
		else
		{
			VectorCopy(cent->lerpAngles, look_angles);
		}
		look_angles[PITCH] = 0;

		if (cent->currentState.otherentity_num2)
		{
			emplaced = &cg_entities[cent->currentState.otherentity_num2].currentState;
		}

		BG_G2PlayerAngles(cent->ghoul2, ci->bolt_motion, &cent->currentState, cg.time,
			cent->lerpOrigin, cent->lerpAngles, legs, legs_angles, &cent->pe.torso.yawing,
			&cent->pe.torso.pitching,
			&cent->pe.legs.yawing, &cent->pe.torso.yawAngle, &cent->pe.torso.pitchAngle,
			&cent->pe.legs.yawAngle,
			cg.frametime, cent->turAngles, cent->modelScale, ci->legsAnim, ci->torsoAnim, &ci->corrTime,
			look_angles, ci->lastHeadAngles, ci->lookTime, emplaced, &ci->superSmoothTime,
			cent->currentState.ManualBlockingFlags);

		if (cent->currentState.heldByClient && cent->currentState.heldByClient <= MAX_CLIENTS)
		{
			//then put our arm in this client's hand
			//is index+1 because index 0 is valid.
			const int held_by_index = cent->currentState.heldByClient - 1;
			centity_t* other = &cg_entities[held_by_index];

			if (other && other->ghoul2 && ci->bolt_lhand)
			{
				mdxaBone_t boltMatrix;
				vec3_t bolt_org;

				trap->G2API_GetBoltMatrix(other->ghoul2, 0, ci->bolt_lhand, &boltMatrix, other->turAngles,
					other->lerpOrigin, cg.time, cgs.game_models, other->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_org);

				BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
					cent->currentState.torsoAnim/*BOTH_DEAD1*/, bolt_org, &cent->ikStatus, cent->lerpOrigin,
					cent->lerpAngles, cent->modelScale, 500, qfalse);
			}
		}
		else if (cent->ikStatus)
		{
			//make sure we aren't IKing if we don't have anyone to hold onto us.
			BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
				cent->currentState.torsoAnim/*BOTH_DEAD1*/, vec3_origin, &cent->ikStatus, cent->lerpOrigin,
				cent->lerpAngles, cent->modelScale, 500, qtrue);
		}
	}
	else if (cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
	{
		vec3_t look_angles;

		VectorCopy(cent->lerpAngles, legs_angles);
		legs_angles[PITCH] = 0;
		AnglesToAxis(legs_angles, legs);

		VectorCopy(cent->lerpAngles, look_angles);
		look_angles[YAW] = look_angles[ROLL] = 0;

		BG_G2ATSTAngles(cent->ghoul2, cg.time, look_angles);
	}
	else
	{
		if (cent->currentState.eType == ET_NPC &&
			cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//fighters actually want to take pitch and roll into account for the axial angles
			VectorCopy(cent->lerpAngles, legs_angles);
			AnglesToAxis(legs_angles, legs);
		}
		else if (cent->currentState.eType == ET_NPC &&
			cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE)
		{
			//an NPC bolted to a vehicle should use the full angles
			VectorCopy(cent->lerpAngles, legs_angles);
			AnglesToAxis(legs_angles, legs);
		}
		else
		{
			vec3_t nh_angles;

			if (cent->currentState.eType == ET_NPC &&
				cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle &&
				cent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{
				//yeah, a hack, sorry.
				VectorSet(nh_angles, 0, cent->lerpAngles[YAW], cent->lerpAngles[ROLL]);
			}
			else
			{
				VectorSet(nh_angles, 0, cent->lerpAngles[YAW], 0);
			}
			AnglesToAxis(nh_angles, legs);
		}
	}

	//See if we have any bone angles sent from the server
	CG_G2ServerBoneAngles(cent);
}

//==========================================================================

/*
===============
CG_TrailItem
===============
*/
#if 0
static void CG_TrailItem(centity_t* cent, qhandle_t hModel) {
	refEntity_t		ent;
	vec3_t			angles;
	matrix3_t		axis;

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap->R_AddRefEntityToScene(&ent);
}
#endif

/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag(centity_t* cent, const qhandle_t h_model)
{
	refEntity_t ent;
	vec3_t angles;
	matrix3_t axis;
	vec3_t bolt_org, t_ang, get_ang, right;
	mdxaBone_t boltMatrix;
	clientInfo_t* ci;

	if (cent->currentState.number == cg.snap->ps.clientNum
		&& !cg.renderingThirdPerson && !cg_trueguns.integer
		&& cg.snap->ps.weapon != WP_SABER)
	{
		return;
	}

	if (!cent->ghoul2)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_llumbar, &boltMatrix, t_ang, cent->lerpOrigin, cg.time,
		cgs.game_models, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_org);

	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, t_ang);
	vectoangles(t_ang, t_ang);

	VectorCopy(cent->lerpAngles, angles);

	bolt_org[2] -= 12;
	VectorSet(get_ang, 0, cent->lerpAngles[1], 0);
	AngleVectors(get_ang, 0, right, 0);
	bolt_org[0] += right[0] * 8;
	bolt_org[1] += right[1] * 8;
	bolt_org[2] += right[2] * 8;

	angles[PITCH] = -cent->lerpAngles[PITCH] / 2 - 30;
	angles[YAW] = t_ang[YAW] + 270;

	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof ent);
	VectorMA(bolt_org, 24, axis[0], ent.origin);

	angles[ROLL] += 20;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = h_model;

	ent.modelScale[0] = 0.5;
	ent.modelScale[1] = 0.5;
	ent.modelScale[2] = 0.5;
	ScaleModelAxis(&ent);

	/*
	if (cent->currentState.number == cg.snap->ps.clientNum)
	{ //If we're the current client (in third person), render the flag on our back transparently
		ent.renderfx |= RF_FORCE_ENT_ALPHA;
		ent.shaderRGBA[3] = 100;
	}
	*/
	//FIXME: Not doing this at the moment because sorting totally messes up

	trap->R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerPowerups
===============
*/
#define ARMOR_EFFECT_TIMES	500

static void CG_PlayerPowerups(centity_t* cent)
{
	const int powerups = cent->currentState.powerups;
	const int health = cg.snap->ps.stats[STAT_HEALTH];

	if (!powerups)
	{
		return;
	}

	if (health < 1)
	{
		return;
	}
	// quad gives a dlight
	if (powerups & 1 << PW_MEDITATE)
	{
		if (cg.snap->ps.fd.forcePower < 50)
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1, 0.2f, 0.2f);
		}
		else
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1);
		}
	}

	if (cent->currentState.eType == ET_NPC)
		assert(cent->npcClient);

	// redflag
	if (powerups & 1 << PW_REDFLAG)
	{
		CG_PlayerFlag(cent, cgs.media.redFlagModel);
		trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 0.2f, 0.2f);
	}

	// blueflag
	if (powerups & 1 << PW_BLUEFLAG)
	{
		CG_PlayerFlag(cent, cgs.media.blueFlagModel);
		trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1.0);
	}

	// neutralflag
	if (powerups & 1 << PW_NEUTRALFLAG)
	{
		trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 1.0, 1.0);
	}

	// Personal Shields
	//------------------------
	if (powerups & 1 << PW_BATTLESUIT)
	{
		//float diff = cent->ps.powerups[PW_BATTLESUIT] - cg.time;
		//float t;
		//refEntity_t		ent;

		//memset(&ent, 0, sizeof(ent));

		//if (diff > 0)
		//{
		//	t = 1.0f - (diff / (ARMOR_EFFECT_TIMES * 2.0f));
		//	// Only display when we have damage
		//	if (t < 0.0f || t > 1.0f)
		//	{
		//	}
		//	else
		//	{
		//		ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = 255.0f * t;
		//		ent->shaderRGBA[3] = 255;
		//		ent->renderfx &= ~RF_ALPHA_FADE;
		//		ent->renderfx |= RF_RGB_TINT;
		//		ent->customShader = cgs.media.playerShieldDamage;

		//		cgi_R_AddRefEntityToScene(ent);
		//	}
		//}
	}

	// Galak Mech shield bubble
	//------------------------------------------------------
	if (powerups & 1 << PW_GALAK_SHIELD)
	{
		refEntity_t tent;

		assert(cent->ghoul2);
		const int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "thoracic");
		//assert(bolt > -1);

		memset(&tent, 0, sizeof(refEntity_t));

		tent.reType = RT_LATHE;

		// Setting up the 2d control points, these get swept around to make a 3D lathed model
		VectorSet2(tent.axis[0], 0.5, 0); // start point of curve
		VectorSet2(tent.axis[1], 50, 85); // control point 1
		VectorSet2(tent.axis[2], 135, -100); // control point 2
		VectorSet2(tent.oldorigin, 0, -90); // end point of curve

		/*if (cent->currentState.poisonTime && cent->poisonTime + 1000 > cg.time)
		{
			VectorCopy(cent->pos4, tent.lightingOrigin);
			tent.frame = cent->client->poisonTime;
		}*/

		mdxaBone_t boltMatrix;
		const vec3_t angles = { 0, 0, 0 };

		trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, angles, cent->lerpOrigin, cg.time,
			cgs.game_models,
			cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tent.origin); // pass in the emitter origin here

		tent.endTime = cent->fx_time + 1000;
		// if you want the shell to build around the guy, pass in a time that is 1000ms after the start of the turn-on-effect
		tent.customShader = trap->R_RegisterShader("gfx/effects/irid_shield");

		trap->R_AddRefEntityToScene(&tent);
	}
	if (powerups & 1 << PW_FORCE_PUSH)
	{
		if (cg.snap->ps.fd.forcePower < 50)
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1, 0.2f, 0.2f);
		}
		else
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1);
		}
	}
	if (powerups & 1 << PW_PULL)
	{
		if (cg.snap->ps.fd.forcePower < 50)
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1, 0.2f, 0.2f);
		}
		else
		{
			trap->R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1);
		}
	}

	if (powerups & 1 << PW_FORCE_PROJECTILE)
	{
		trap->R_AddLightToScene(cent->lerpOrigin, 125, 1.0f, 0.25f, 0.75f);
	}
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite(const centity_t* cent, const qhandle_t shader)
{
	int rf;
	refEntity_t ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		rf = RF_THIRD_PERSON; // only show in mirrors
	}
	else
	{
		rf = 0;
	}

	memset(&ent, 0, sizeof ent);
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap->R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerFloatSprite

Same as above but allows custom RGBA values
===============
*/
#if 0
static void CG_PlayerFloatSpriteRGBA(centity_t* cent, qhandle_t shader, vec4_t rgba) {
	int				rf;
	refEntity_t		ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	}
	else {
		rf = 0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = rgba[0];
	ent.shaderRGBA[1] = rgba[1];
	ent.shaderRGBA[2] = rgba[2];
	ent.shaderRGBA[3] = rgba[3];
	trap->R_AddRefEntityToScene(&ent);
}
#endif

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(const centity_t* cent)
{
	//	int		team;

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	if (cent->currentState.eFlags & EF_CONNECTION)
	{
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		return;
	}

	if (cent->vChatTime > cg.time)
	{
		CG_PlayerFloatSprite(cent, cgs.media.vchatShader);
	}
	else if (cent->currentState.eType != ET_NPC && //don't draw talk balloons on NPCs
		cent->currentState.eFlags & EF_TALK)
	{
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader);
	}

	/*if (cent->currentState.eFlags & EF_AWARD_IMPRESSIVE)
	{
		CG_PlayerFloatSprite(cent, cgs.media.medalImpressive);
		return;
	}*/

	/*if (cent->currentState.eFlags & EF_AWARD_EXCELLENT)
	{
		CG_PlayerFloatSprite(cent, cgs.media.medalExcellent);
		return;
	}*/
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128

static qboolean CG_PlayerShadow(const centity_t* cent, float* shadowPlane)
{
	vec3_t end;
	const vec3_t maxs = { 15, 15, 2 };
	const vec3_t mins = { -15, -15, 0 };
	trace_t trace;
	float radius = 24.0f;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0)
	{
		return qfalse;
	}

	// no shadows when cloaked
	if (cent->currentState.powerups & 1 << PW_CLOAKED)
	{
		return qfalse;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		return qfalse;
	}

	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return qfalse; //this entity is mind-tricking the current client, so don't render it
	}
	if (cent->currentState.NPC_class == CLASS_SAND_CREATURE)
	{
		//sand creatures have no shadow
		return qfalse;
	}

	if (cg_shadows.integer == 1)
	{
		//dropshadow
		if (cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE)
		{
			//riding a vehicle, no dropshadow
			return qfalse;
		}
	}
	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	if (cg_shadows.integer == 2)
	{
		//stencil
		end[2] -= 4096.0f;

		trap->CM_Trace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0);

		if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
		{
			trace.endpos[2] = cent->lerpOrigin[2] - 25.0f;
		}
	}
	else
	{
		end[2] -= SHADOW_DISTANCE;

		trap->CM_Trace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0);

		// no shadow if too high
		if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
		{
			return qfalse;
		}
	}

	if (cg_shadows.integer == 2)
	{
		//stencil shadows need plane to be on ground
		*shadowPlane = trace.endpos[2];
	}
	else
	{
		*shadowPlane = trace.endpos[2] + 1;
	}

	if (cg_shadows.integer != 1)
	{
		// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	const float alpha = 1.0 - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	if (cent->currentState.NPC_class == CLASS_REMOTE
		|| cent->currentState.NPC_class == CLASS_SEEKER)
	{
		radius = 8.0f;
	}
	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		cent->pe.legs.yawAngle, alpha, alpha, alpha, 1, qfalse, radius, qtrue);

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash(const centity_t* cent)
{
	vec3_t start, end;
	trace_t trace;
	polyVert_t verts[4];

	if (!cg_shadows.integer)
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	int contents = CG_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}

	// trace down to find the surface
	trap->CM_Trace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA, 0);

	if (trace.fraction == 1.0)
	{
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap->R_AddPolysToScene(cgs.media.wakeMarkShader, 4, verts, 1);
}

#define REFRACT_EFFECT_DURATION		500

static void CG_ForcePushBlur(vec3_t org, centity_t* cent)
{
	if (com_outcast.integer == 2) // NO EFFECT
	{
		//no effect
		return;
	}

	if (!cent || !cg_renderToTextureFX.integer)
	{
		localEntity_t* ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale(cg.refdef.viewaxis[1], 55, ex->pos.trDelta);

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");

		ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->refEntity.rotation = 180.0f;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale(cg.refdef.viewaxis[1], -55, ex->pos.trDelta);

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");
	}
	else if (com_outcast.integer == 1)
	{
		localEntity_t* ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;

		if (cent->currentState.powerups & 1 << PW_FORCE_PUSH_RHAND)
		{
			ex->radius = 8.0f;
		}
		else
		{
			ex->radius = 2.0f;
		}
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;

		if (cent->currentState.powerups & 1 << PW_FORCE_PUSH_RHAND)
		{
			VectorScale(cg.refdef.viewaxis[1], 255, ex->pos.trDelta);
		}
		else
		{
			VectorScale(cg.refdef.viewaxis[1], 55, ex->pos.trDelta);
		}

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");

		ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->refEntity.rotation = 180.0f;

		if (cent->currentState.powerups & 1 << PW_FORCE_PUSH_RHAND)
		{
			ex->radius = 8.0f;
		}
		else
		{
			ex->radius = 2.0f;
		}
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;

		if (cent->currentState.powerups & 1 << PW_FORCE_PUSH_RHAND)
		{
			VectorScale(cg.refdef.viewaxis[1], -255, ex->pos.trDelta);
		}
		else
		{
			VectorScale(cg.refdef.viewaxis[1], -55, ex->pos.trDelta);
		}

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");
	}
	else
	{
		//superkewl "refraction" (well sort of) effect -rww
		refEntity_t ent;
		vec3_t ang;
		float scale;

		if (!cent->bodyFadeTime)
		{
			//the duration for the expansion and fade
			cent->bodyFadeTime = cg.time + REFRACT_EFFECT_DURATION;
		}

		//closer tDif is to 0, the closer we are to
		//being "done"
		const int t_dif = cent->bodyFadeTime - cg.time;

		if (REFRACT_EFFECT_DURATION - t_dif < 200)
		{
			//stop following the hand after a little and stay in a fixed spot
			//save the initial spot of the effect
			VectorCopy(org, cent->pushEffectOrigin);
		}

		//scale from 1.0f to 0.1f then hold at 0.1 for the rest of the duration
		if (cent->currentState.powerups & 1 << PW_PULL)
		{
			scale = (float)(REFRACT_EFFECT_DURATION - t_dif) * 0.003f;
		}
		else
		{
			scale = (float)t_dif * 0.003f;
		}

		if (scale > 1.0f)
		{
			scale = 1.0f;
		}
		else if (scale < 0.2f)
		{
			scale = 0.2f;
		}

		//start alpha at 244, fade to 10
		float alpha = (float)t_dif * 0.488f;

		if (alpha > 244.0f)
		{
			alpha = 244.0f;
		}
		else if (alpha < 0.0f)
		{
			alpha = 0.0f;
		}

		memset(&ent, 0, sizeof ent);
		ent.shaderTime = (cent->bodyFadeTime - REFRACT_EFFECT_DURATION) / 1000.0f;

		VectorCopy(cent->pushEffectOrigin, ent.origin);

		VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
		const float v_len = VectorLength(ent.axis[0]);
		if (v_len <= 0.1f)
		{
			// Entity is right on vieworg.  quit.
			return;
		}

		vectoangles(ent.axis[0], ang);
		ang[ROLL] += 180.0f;
		AnglesToAxis(ang, ent.axis);

		//radius must be a power of 2, and is the actual captured texture size
		if (v_len < 128)
		{
			ent.radius = 256;
		}
		else if (v_len < 256)
		{
			ent.radius = 128;
		}
		else if (v_len < 512)
		{
			ent.radius = 64;
		}
		else
		{
			ent.radius = 32;
		}

		VectorScale(ent.axis[0], scale, ent.axis[0]);
		VectorScale(ent.axis[1], scale, ent.axis[1]);
		VectorScale(ent.axis[2], scale, ent.axis[2]);

		ent.hModel = cgs.media.halfShieldModel;
		ent.customShader = cgs.media.refractionShader; //cgs.media.cloakedShader;
		ent.nonNormalizedAxes = qtrue;

		//make it partially transparent so it blends with the background
		ent.renderfx = RF_DISTORTION | RF_FORCE_ENT_ALPHA;
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = alpha;

		trap->R_AddRefEntityToScene(&ent);
	}
}

static void CG_ForceRepulseRefraction(vec3_t org, centity_t* cent, vec3_t colour)
{
	if (!cent || !cg_renderToTextureFX.integer)
	{
		localEntity_t* ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale(cg.refdef.viewaxis[1], 55, ex->pos.trDelta);

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");

		ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->refEntity.rotation = 180.0f;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy(org, ex->pos.trBase);
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale(cg.refdef.viewaxis[1], -55, ex->pos.trDelta);

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");
	}
	else
	{
		//superkewl "refraction" (well sort of) effect -rww
		refEntity_t ent;
		vec3_t ang;
		float scale;
		float alpha;

		if (!cent->bodyFadeTime)
		{
			//the duration for the expansion and fade
			cent->bodyFadeTime = cg.time + REFRACT_EFFECT_DURATION;
		}

		//closer tDif is to 0, the closer we are to
		//being "done"
		const int t_dif = cent->bodyFadeTime - cg.time;

		if (REFRACT_EFFECT_DURATION - t_dif < 200)
		{
			//stop following the hand after a little and stay in a fixed spot
			//save the initial spot of the effect
			VectorCopy(org, cent->pushEffectOrigin);
		}

		//scale from 1.0f to 0.1f then hold at 0.1 for the rest of the duration
		if ((cent->currentState.weapon == WP_NONE || cent->currentState.weapon == WP_MELEE)
			&& cent->currentState.forcePowersActive & 1 << FP_PUSH && cent->currentState.groundEntityNum ==
			ENTITYNUM_NONE)
		{
			scale = 1.0f;
		}
		else if (cent->currentState.powerups & 1 << PW_PULL)
		{
			scale = (float)(REFRACT_EFFECT_DURATION - t_dif) * 0.003f;
		}
		else
		{
			scale = (float)t_dif * 0.003f;
		}

		if (scale > 1.0f)
		{
			scale = 1.0f;
		}
		else if (scale < 0.2f)
		{
			scale = 0.2f;
		}

		//start alpha at 244, fade to 10
		if ((cent->currentState.weapon == WP_NONE || cent->currentState.weapon == WP_MELEE)
			&& cent->currentState.forcePowersActive & 1 << FP_PUSH && cent->currentState.groundEntityNum ==
			ENTITYNUM_NONE)
		{
			alpha = 244.0f;
		}
		else
		{
			alpha = (float)t_dif * 0.488f;
		}

		if (alpha > 244.0f)
		{
			alpha = 244.0f;
		}
		else if (alpha < 0.0f)
		{
			alpha = 0.0f;
		}

		memset(&ent, 0, sizeof ent);
		ent.shaderTime = (cent->bodyFadeTime - REFRACT_EFFECT_DURATION) / 1000.0f;

		VectorCopy(cent->pushEffectOrigin, ent.origin);

		VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
		const float v_len = VectorLength(ent.axis[0]);
		if (v_len <= 0.1f)
		{
			// Entity is right on vieworg.  quit.
			return;
		}

		vectoangles(ent.axis[0], ang);
		ang[ROLL] += 180.0f;
		AnglesToAxis(ang, ent.axis);

		//radius must be a power of 2, and is the actual captured texture size
		if (v_len < 128)
		{
			ent.radius = 256;
		}
		else if (v_len < 256)
		{
			ent.radius = 128;
		}
		else if (v_len < 512)
		{
			ent.radius = 64;
		}
		else
		{
			ent.radius = 32;
		}

		VectorScale(ent.axis[0], scale, ent.axis[0]);
		VectorScale(ent.axis[1], scale, ent.axis[1]);
		VectorScale(ent.axis[2], scale, ent.axis[2]);

		ent.hModel = cgs.media.halfShieldModel;
		ent.customShader = cgs.media.refractionShader; //cgs.media.cloakedShader;
		ent.nonNormalizedAxes = qtrue;

		//make it partially transparent so it blends with the background
		ent.renderfx = RF_DISTORTION | RF_FORCE_ENT_ALPHA;
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = alpha;

		if (colour)
		{
			ent.shaderRGBA[0] = colour[0];
			ent.shaderRGBA[1] = colour[1];
			ent.shaderRGBA[2] = colour[2];
		}

		trap->R_AddRefEntityToScene(&ent);
	}
}

static const char* cg_pushBoneNames[] =
{
	"cranium",
	"lower_lumbar",
	"rhand",
	"lhand",
	"ltibia",
	"rtibia",
	"lradius",
	"rradius",
	NULL
};

static void CG_ForcePushBodyBlur(centity_t* cent)
{
	mdxaBone_t boltMatrix;

	if (cent->localAnimIndex > 1)
	{
		//Sorry, the humanoid IS IN ANOTHER CASTLE.
		return;
	}

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	assert(cent->ghoul2);

	for (int i = 0; cg_pushBoneNames[i]; i++)
	{
		vec3_t fx_org;
		//go through all the bones we want to put a blur effect on
		const int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, cg_pushBoneNames[i]);

		if (cent->currentState.NPC_class == CLASS_HAZARD_TROOPER
			|| cent->currentState.NPC_class == CLASS_ROCKETTROOPER
			|| cent->currentState.NPC_class == CLASS_SABER_DROID
			|| cent->currentState.NPC_class == CLASS_WAMPA
			|| cent->currentState.NPC_class == CLASS_ASSASSIN_DROID
			|| cent->currentState.NPC_class == CLASS_RANCOR)
		{
			if (bolt == -1)
			{
				//don't use bones
			}
		}
		else
		{
			if (bolt == -1)
			{
				assert(!"You've got an invalid bone/bolt name in cg_pushBoneNames");
				continue;
			}
		}

		trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time,
			cgs.game_models, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, fx_org);

		if ((cent->currentState.weapon == WP_NONE || cent->currentState.weapon == WP_MELEE)
			&& cent->currentState.forcePowersActive & 1 << FP_PUSH && cent->currentState.groundEntityNum ==
			ENTITYNUM_NONE)
		{
			vec3_t blue;
			VectorScale(colorTable[CT_LTBLUE1], 255.0f, blue);
			CG_ForceRepulseRefraction(fx_org, NULL, blue);
		}
		else
		{
			if (com_outcast.integer == 0) //JKA
			{
				//standard effect, don't be refractive (for now)
				CG_ForcePushBlur(fx_org, NULL);
			}
			else if (com_outcast.integer == 1) //JKO
			{
				//standard effect, don't be refractive (for now)
				CG_ForcePushBlur(fx_org, NULL);
			}
			else if (com_outcast.integer == 2) // NO EFFECT
			{
				//no effect
			}
			else // BACKUP
			{
				//standard effect, don't be refractive (for now)
				CG_ForcePushBlur(fx_org, NULL);
			}
		}
	}
}

static void CG_ForceGripEffect(vec3_t org)
{
	const float wv = sin(cg.time * 0.004f) * 0.08f + 0.1f;

	localEntity_t* ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy(org, ex->pos.trBase);
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale(cg.refdef.viewaxis[1], 55, ex->pos.trDelta);

	ex->color[0] = 200 + wv * 255;
	if (ex->color[0] > 255)
	{
		ex->color[0] = 255;
	}
	ex->color[1] = 0;
	ex->color[2] = 0;
	ex->refEntity.customShader = trap->R_RegisterShader("gfx/effects/forcePush");

	ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = 180.0f;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy(org, ex->pos.trBase);
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale(cg.refdef.viewaxis[1], -55, ex->pos.trDelta);
	ex->color[0] = 255;
	ex->color[1] = 255;
	ex->color[2] = 255;
	ex->refEntity.customShader = cgs.media.redSaberGlowShader;
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups(const refEntity_t* ent, const entityState_t* state)
{
	if (CG_IsMindTricked(state->trickedentindex,
		state->trickedentindex2,
		state->trickedentindex3,
		state->trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	trap->R_AddRefEntityToScene(ent);
}

#define MAX_SHIELD_TIME	2000.0
#define MIN_SHIELD_TIME	2000.0

void CG_PlayerShieldHit(const int entityNum, vec3_t dir, const int amount)
{
	int time;

	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
	{
		return;
	}

	centity_t* cent = &cg_entities[entityNum];

	if (cent->currentState.weapon == WP_SABER)
	{
		return;
	}

	if (amount > 100)
	{
		time = cg.time + MAX_SHIELD_TIME; // 2 sec.
	}
	else
	{
		time = cg.time + 500 + amount * 15;
	}

	if (time > cent->damageTime)
	{
		cent->damageTime = time;
		VectorScale(dir, -1, dir);
		vectoangles(dir, cent->damageAngles);
	}
	cent->shieldHitTime = cg.time + 250;
}

void CG_PlayerShieldRecharging(const int entityNum)
{
	if (entityNum < 0 || entityNum >= MAX_GENTITIES)
	{
		return;
	}

	centity_t* cent = &cg_entities[entityNum];
	cent->shieldRechargeTime = cg.time + 1000;
}

void CG_DrawPlayerShield(const centity_t* cent, vec3_t origin)
{
	refEntity_t ent;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset(&ent, 0, sizeof ent);

	VectorCopy(origin, ent.origin);
	ent.origin[2] += 10.0;
	AnglesToAxis(cent->damageAngles, ent.axis);

	int alpha = 255.0f * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + Q_flrand(0.1f, 1.0f) * 16;
	if (alpha > 255)
		alpha = 255;

	// Make it bigger, but tighter if more solid
	const float scale = 1.4 - (float)alpha * (0.4 / 255.0); // Range from 1.0 to 1.4
	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.halfShieldShader;
	ent.shaderRGBA[0] = alpha;
	ent.shaderRGBA[1] = alpha;
	ent.shaderRGBA[2] = alpha;
	ent.shaderRGBA[3] = 255;
	trap->R_AddRefEntityToScene(&ent);
}

void CG_PlayerHitFX(centity_t* cent)
{
	// only do the below fx if the cent in question is...uh...me, and it's first person.
	if (cent->currentState.clientNum != cg.predictedPlayerState.clientNum || cg.renderingThirdPerson)
	{
		if (cent->damageTime > cg.time && cent->currentState.NPC_class != CLASS_VEHICLE
			&& cent->currentState.NPC_class != CLASS_SEEKER
			&& cent->currentState.NPC_class != CLASS_REMOTE)
		{
			CG_DrawPlayerShield(cent, cent->lerpOrigin);
		}
	}
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts(vec3_t normal, const int numVerts, polyVert_t* verts)
{
	vec3_t ambient_light;
	vec3_t lightDir;
	vec3_t directed_light;

	trap->R_LightForPoint(verts[0].xyz, ambient_light, directed_light, lightDir);

	for (int i = 0; i < numVerts; i++)
	{
		const float incoming = DotProduct(normal, lightDir);
		if (incoming <= 0)
		{
			verts[i].modulate[0] = ambient_light[0];
			verts[i].modulate[1] = ambient_light[1];
			verts[i].modulate[2] = ambient_light[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		int j = ambient_light[0] + incoming * directed_light[0];
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ambient_light[1] + incoming * directed_light[1];
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ambient_light[2] + incoming * directed_light[2];
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

void RGB_LerpColor(vec3_t from, vec3_t to, const float frac, vec3_t out)
{
	vec3_t diff;

	VectorSubtract(to, from, diff);

	VectorCopy(from, out);

	for (int i = 0; i < 3; i++)
	{
		out[i] += diff[i] * frac;
	}
}

int getint(char** buf)
{
	const double temp = strtod(*buf, buf);
	return (int)temp;
}

void ParseRGBSaber(char* str, vec3_t c)
{
	char* p = str;

	for (int i = 0; i < 3; i++)
	{
		c[i] = getint(&p);
		p++;
	}
}

void CG_ParseScriptedSaber(char* script, clientInfo_t* ci, const int snum)
{
	int n = 0;
	char* p = script;

	const int l = strlen(p);
	p++; //skip the 1st ':'

	while (p[0] && p - script < l && n < 10)
	{
		ParseRGBSaber(p, ci->ScriptedColors[n][snum]);
		while (p[0] != ':')
			p++;
		p++; //skipped 1st point

		ci->ScriptedTimes[n][snum] = getint(&p);

		p++;
		n++;
	}
	ci->ScriptedNum[snum] = n;
}

void rgb_adjust_scipted_saber_color(clientInfo_t* ci, vec3_t color, const int n)
{
	int actual;

	if (!ci->ScriptedStartTime[n])
	{
		ci->ScriptedActualNum[n] = 0;
		ci->ScriptedStartTime[n] = cg.time;
		ci->ScriptedEndTime[n] = cg.time + ci->ScriptedTimes[0][n];
	}
	else if (ci->ScriptedEndTime[n] < cg.time)
	{
		ci->ScriptedActualNum[n] = (ci->ScriptedActualNum[n] + 1) % ci->ScriptedNum[n];
		actual = ci->ScriptedActualNum[n];
		ci->ScriptedStartTime[n] = cg.time;
		ci->ScriptedEndTime[n] = cg.time + ci->ScriptedTimes[actual][n];
	}

	actual = ci->ScriptedActualNum[n];

	const float frac = (float)(cg.time - ci->ScriptedStartTime[n]) / (float)(ci->ScriptedEndTime[n] - ci->
		ScriptedStartTime[
			n]);

	if (actual + 1 != ci->ScriptedNum[n])
		RGB_LerpColor(ci->ScriptedColors[actual][n], ci->ScriptedColors[actual + 1][n], frac, color);
	else
		RGB_LerpColor(ci->ScriptedColors[actual][n], ci->ScriptedColors[0][n], frac, color);
}

#define PIMP_MIN_INTESITY 120

void RGB_RandomRGB(vec3_t c)
{
	int i;
	for (i = 0; i < 3; i++)
		c[i] = 0;

	while (c[0] + c[1] + c[2] < PIMP_MIN_INTESITY)
		for (i = 0; i < 3; i++)
			c[i] = rand() % 255;
}

void RGB_AdjustPimpSaberColor(clientInfo_t* ci, vec3_t color, const int n)
{
	int time;

	if (!ci->PimpStartTime[n])
	{
		ci->PimpStartTime[n] = cg.time;
		RGB_RandomRGB(ci->PimpColorFrom[n]);
		RGB_RandomRGB(ci->PimpColorTo[n]);
		time = 250 + rand() % 250;
		//		time = 500 + rand()%250;
		ci->PimpEndTime[n] = cg.time + time;
	}
	else if (ci->PimpEndTime[n] < cg.time)
	{
		VectorCopy(ci->PimpColorTo[n], ci->PimpColorFrom[n]);
		RGB_RandomRGB(ci->PimpColorTo[n]);
		time = 250 + rand() % 250;
		ci->PimpStartTime[n] = cg.time;
		ci->PimpEndTime[n] = cg.time + time;
	}

	const float frac = (float)(cg.time - ci->PimpStartTime[n]) / (float)(ci->PimpEndTime[n] - ci->PimpStartTime[n]);

	//	Com_Printf("frac : %f\n",frac);

	RGB_LerpColor(ci->PimpColorFrom[n], ci->PimpColorTo[n], frac, color);
}

static void CG_RGBForSaberColor(const saber_colors_t color, vec3_t rgb, const int cnum, const int bnum)
{
	switch (color)
	{
	case SABER_RED:
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (cnum < MAX_CLIENTS)
		{
			const clientInfo_t* ci = &cgs.clientinfo[cnum];

			if (bnum == 0)
				VectorCopy(ci->rgb1, rgb);
			else
				VectorCopy(ci->rgb2, rgb);
			for (int i = 0; i < 3; i++)
				rgb[i] /= 255;
		}
		else
			VectorSet(rgb, 0.2f, 0.4f, 1.0f);
	}
	break;
	case SABER_PIMP:
	{
		if (cnum < MAX_CLIENTS)
		{
			clientInfo_t* ci = &cgs.clientinfo[cnum];

			RGB_AdjustPimpSaberColor(ci, rgb, bnum);

			for (int i = 0; i < 3; i++)
				rgb[i] /= 255;
		}
		else
			VectorSet(rgb, 0.2f, 0.4f, 1.0f);
	}
	break;
	case SABER_SCRIPTED:
	{
		if (cnum < MAX_CLIENTS)
		{
			clientInfo_t* ci = &cgs.clientinfo[cnum];

			rgb_adjust_scipted_saber_color(ci, rgb, bnum);

			for (int i = 0; i < 3; i++)
				rgb[i] /= 255;
		}
		else
			VectorSet(rgb, 0.2f, 0.4f, 1.0f);
	}
	break;
	case SABER_BLACK:
		VectorSet(rgb, 0.0f, 0.0f, 0.0f);
		break;
	case SABER_WHITE:
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	default:;
	}
}

void CG_DoSaberLight(const saberInfo_t* saber, const int cnum, const int bnum)
{
	vec3_t positions[MAX_BLADES * 2], mid = { 0 }, rgbs[MAX_BLADES * 2], rgb = { 0 };
	float lengths[MAX_BLADES * 2] = { 0 }, totallength = 0, numpositions = 0, diameter = 0;
	int i;

	//RGB combine all the colors of the sabers you're using into one averaged color!
	if (!saber)
	{
		return;
	}

	if (saber->saberFlags2 & SFL2_NO_DLIGHT)
	{
		//no dlight!
		return;
	}

	for (i = 0; i < saber->numBlades; i++)
	{
		if (saber->blade[i].length >= 0.5f)
		{
			//FIXME: make RGB sabers
			CG_RGBForSaberColor(saber->blade[i].color, rgbs[i], cnum, bnum);
			lengths[i] = saber->blade[i].length;
			if (saber->blade[i].length * 2.0f > diameter)
			{
				diameter = saber->blade[i].length * 2.0f;
			}
			totallength += saber->blade[i].length;
			VectorMA(saber->blade[i].muzzlePoint, saber->blade[i].length, saber->blade[i].muzzleDir, positions[i]);
			if (!numpositions)
			{
				//first blade, store middle of that as midpoint
				VectorMA(saber->blade[i].muzzlePoint, saber->blade[i].length * 0.5, saber->blade[i].muzzleDir, mid);
				VectorCopy(rgbs[i], rgb);
			}
			numpositions++;
		}
	}

	if (totallength)
	{
		//actually have something to do
		if (numpositions == 1)
		{
			//only 1 blade, midpoint is already set (halfway between the start and end of that blade), rgb is already set, so it diameter
		}
		else
		{
			//multiple blades, calc averages
			VectorClear(mid);
			VectorClear(rgb);
			//now go through all the data and get the average RGB and middle position and the radius
			for (i = 0; i < MAX_BLADES * 2; i++)
			{
				if (lengths[i])
				{
					VectorMA(rgb, lengths[i], rgbs[i], rgb);
					VectorAdd(mid, positions[i], mid);
				}
			}

			//get middle rgb
			VectorScale(rgb, 1 / totallength, rgb); //get the average, normalized RGB
			//get mid position
			VectorScale(mid, 1 / numpositions, mid);
			//find the farthest distance between the blade tips, this will be our diameter
			for (i = 0; i < MAX_BLADES * 2; i++)
			{
				if (lengths[i])
				{
					for (int j = 0; j < MAX_BLADES * 2; j++)
					{
						if (lengths[j])
						{
							const float dist = Distance(positions[i], positions[j]);
							if (dist > diameter)
							{
								diameter = dist;
							}
						}
					}
				}
			}
		}

		trap->R_AddLightToScene(mid, diameter + randoms() * 8.0f, rgb[0], rgb[1], rgb[2]);
	}
}

void CG_DoCustomSaber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum,
	int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeCSM.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeCSM.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoCloakedSaber(vec3_t origin, vec3_t dir, float length, float length_max, float radius, saber_colors_t color,
	int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t mid;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radius_range;
	float radius_start;
	qhandle_t ignite = 0;
	float ignite_len, ignite_radius;
	vec3_t rgb = { 1, 1, 1 };
	int i;

	if (length < MIN_SABERBLADE_DRAW_LENGTH)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_ORANGE:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_YELLOW:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_GREEN:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_BLUE:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_PURPLE:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_LIME:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_BLACK:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	default:
		glow = cgs.media.cloakedShader;
		blade = cgs.media.cloakedShader;
		ignite = cgs.media.cloakedShader;
		break;
	}

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, length * 1.4f + Q_flrand(0.0f, 1.0f) * 3.0f, rgb[0], rgb[1], rgb[2]);
	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < length_max)
	{
		radiusmult = 1.0 + 2.0 / length; // Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		//draw the blade as a post-render so it doesn't get in the cap...
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	radius_range = radius * 0.075f;
	radius_start = radius - radius_range;

	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	ignite_len = length_max * 0.30f;
	ignite_radius = saber.radius * saber.radius * 1.5f;
	ignite_radius -= length;
	ignite_radius *= 2.2f;

	VectorCopy(origin, saber.origin);
	VectorCopy(dir, saber.axis[0]);
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;

	saber.shaderRGBA[0] = 0xff;
	saber.shaderRGBA[1] = 0xff;
	saber.shaderRGBA[2] = 0xff;
	saber.shaderRGBA[3] = 0xff;

	if (color >= SABER_RGB || color >= SABER_PIMP || color >= SABER_WHITE || color >= SABER_SCRIPTED)
	{
		for (i = 0; i < 3; i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}

	saber.renderfx = rfx;

	trap->R_AddRefEntityToScene(&saber);

	// Do the hot core
	VectorMA(origin, length, dir, saber.origin);
	VectorMA(origin, -1, dir, saber.oldorigin);

	saber.customShader = blade;
	saber.reType = RT_LINE;
	radius_start = radius / 3.0f;
	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene(&saber);

	//Ignition Flare
	//--------------------
	//GR - Do the flares

	if (length <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(origin, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;

		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}

		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoSaber(vec3_t origin, vec3_t dir, float length, float length_max, float radius, saber_colors_t color, int rfx,
	qboolean do_light, int cnum, int bnum)
{
	vec3_t mid;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radius_range;
	float radius_start;
	qhandle_t ignite = 0;
	float ignite_len, ignite_radius;
	vec3_t rgb = { 1, 1, 1 };
	int i;

	if (length < MIN_SABERBLADE_DRAW_LENGTH)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		blade = cgs.media.redSaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		blade = cgs.media.orangeSaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		blade = cgs.media.yellowSaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		blade = cgs.media.greenSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		blade = cgs.media.blueSaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		blade = cgs.media.purpleSaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.limeSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.customSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, length * 1.4f + Q_flrand(0.0f, 1.0f) * 3.0f, rgb[0], rgb[1], rgb[2]);
	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < length_max)
	{
		radiusmult = 1.0 + 2.0 / length; // Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		//draw the blade as a post-render so it doesn't get in the cap...
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	radius_range = radius * 0.075f;
	radius_start = radius - radius_range;

	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	ignite_len = length_max * 0.30f;
	ignite_radius = saber.radius * saber.radius * 1.5f;
	ignite_radius -= length;
	ignite_radius *= 2.2f;

	VectorCopy(origin, saber.origin);
	VectorCopy(dir, saber.axis[0]);
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;

	saber.shaderRGBA[0] = 0xff;
	saber.shaderRGBA[1] = 0xff;
	saber.shaderRGBA[2] = 0xff;
	saber.shaderRGBA[3] = 0xff;

	if (color >= SABER_RGB || color >= SABER_PIMP || color >= SABER_WHITE || color >= SABER_SCRIPTED)
	{
		for (i = 0; i < 3; i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}

	saber.renderfx = rfx;

	trap->R_AddRefEntityToScene(&saber);

	// Do the hot core
	VectorMA(origin, length, dir, saber.origin);
	VectorMA(origin, -1, dir, saber.oldorigin);

	saber.customShader = blade;
	saber.reType = RT_LINE;
	radius_start = radius / 3.0f;
	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene(&saber);

	//Ignition Flare
	//--------------------
	//GR - Do the flares

	if (length <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(origin, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;

		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}

		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoTFASaber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbTFASaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeTFA.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeTFA.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = cgs.media.sfxSaberBlade2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoSaberUnstable(vec3_t origin, vec3_t dir, float length, float length_max, float radius, saber_colors_t color,
	int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t mid;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radius_range;
	float radius_start;
	qhandle_t ignite = 0;
	float ignite_len, ignite_radius;
	vec3_t rgb = { 1, 1, 1 };
	int i;

	if (length < MIN_SABERBLADE_DRAW_LENGTH)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.unstableRedSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, length * 1.4f + Q_flrand(0.0f, 1.0f) * 3.0f, rgb[0], rgb[1], rgb[2]);
	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < length_max)
	{
		radiusmult = 1.0 + 2.0 / length; // Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		//draw the blade as a post-render so it doesn't get in the cap...
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	radius_range = radius * 0.075f;
	radius_start = radius - radius_range;

	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	ignite_len = length_max * 0.30f;
	ignite_radius = saber.radius * saber.radius * 1.5f;
	ignite_radius -= length;
	ignite_radius *= 2.2f;

	VectorCopy(origin, saber.origin);
	VectorCopy(dir, saber.axis[0]);
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;

	saber.shaderRGBA[0] = 0xff;
	saber.shaderRGBA[1] = 0xff;
	saber.shaderRGBA[2] = 0xff;
	saber.shaderRGBA[3] = 0xff;

	if (color >= SABER_RGB || color >= SABER_PIMP || color >= SABER_WHITE || color >= SABER_SCRIPTED)
	{
		for (i = 0; i < 3; i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}

	saber.renderfx = rfx;

	trap->R_AddRefEntityToScene(&saber);

	// Do the hot core
	VectorMA(origin, length, dir, saber.origin);
	VectorMA(origin, -1, dir, saber.oldorigin);

	saber.customShader = blade;
	saber.reType = RT_LINE;
	radius_start = radius / 3.0f;
	saber.radius = (radius_start + Q_flrand(-1.0f, 1.0f) * radius_range) * radiusmult;

	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene(&saber);

	//Ignition Flare
	//--------------------
	//GR - Do the flares

	if (length <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(origin, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;

		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}

		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoBattlefrontSaber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum,
	int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t blade = 0, glow = 0, ignite = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		ignite = cgs.media.redIgniteFlare;
		blade = cgs.media.redSaberCoreShader;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		ignite = cgs.media.orangeIgniteFlare;
		blade = cgs.media.orangeSaberCoreShader;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		ignite = cgs.media.yellowIgniteFlare;
		blade = cgs.media.yellowSaberCoreShader;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		ignite = cgs.media.greenIgniteFlare;
		blade = cgs.media.greenSaberCoreShader;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		ignite = cgs.media.blueIgniteFlare;
		blade = cgs.media.blueSaberCoreShader;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		ignite = cgs.media.purpleIgniteFlare;
		blade = cgs.media.purpleSaberCoreShader;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.limeSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		ignite = cgs.media.whiteIgniteFlare02;
		blade = cgs.media.rgbSaberCoreShader;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.rgbSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeBF2.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeBF2.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoEp1Saber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueEp1GlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep1SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeEP1.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeEP1.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = cgs.media.sfxSaberBlade2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoEp2Saber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueEp2GlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep2SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f)) * radiusmult * cg_SFXSabersGlowSizeEP2.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f)) * radiusmult * cg_SFXSabersCoreSizeEP2.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoEp3Saber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueEp3GlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.ep3SaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeEP3.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeEP3.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoOTSaber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius,
	saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	float dist;
	int count;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueOTGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.customSaberGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.otSaberCoreShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(0.3f, 1.8f)) * radiusmult * cg_SFXSabersGlowSizeOT.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f)) * radiusmult * cg_SFXSabersCoreSizeOT.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	coreradius *= 0.5f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	//Main Blade
	//--------------------
	{
		saber.radius = effectradius * angle_scale;

		dist = saber.radius / 4.0f;

		count = (int)(blade_len / dist);

		saber.renderfx = rfx;

		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = glow;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		trap->R_AddRefEntityToScene(&saber);

		for (i = 0; i < count; i++)
		{
			VectorMA(saber.origin, dist, blade_dir, saber.origin);
			effectradius = (radius * 1.6f * v1 + Q_flrand(0.3f, 1.8f) * 0.1f) * radiusmult * cg_SFXSabersGlowSize.value;
			saber.radius = effectradius * angle_scale;
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			sbak.shaderRGBA[0] = 0xff;
			sbak.shaderRGBA[1] = 0xff;
			sbak.shaderRGBA[2] = 0xff;
			sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius * 3.0f;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	//Trail Edge
	//--------------------
	//GR - Adding this condition cuz when you look side to side quickly it looks a little funny
	if (end_len > 1)
	{
		saber.radius = effectradius * angle_scale;

		dist = saber.radius / 4.0f;

		count = (int)(trail_len / dist);

		saber.renderfx = rfx;

		VectorCopy(trail_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = glow;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}

		trap->R_AddRefEntityToScene(&saber);

		for (i = 0; i < count; i++)
		{
			VectorMA(saber.origin, dist, trail_dir, saber.origin);
			effectradius = (radius * 1.6f * v1 + Q_flrand(0.3f, 1.8f) * 0.1f) * radiusmult * cg_SFXSabersGlowSize.value;
			saber.radius = effectradius * angle_scale;
			trap->R_AddRefEntityToScene(&sbak);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			sbak.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius * 1.14f;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	//Fan Base
	//--------------------
	if (end_len > 1)
	{
		saber.radius = effectradius * angle_scale;

		dist = saber.radius / 4.0f;

		count = (int)(base_len / dist);

		saber.renderfx = rfx;

		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = glow;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}

		trap->R_AddRefEntityToScene(&saber);

		for (i = 0; i < count; i++)
		{
			VectorMA(saber.origin, dist, base_dir, saber.origin);
			effectradius = (radius * 1.6f * v1 + flrand(0.3f, 1.8f)) * radiusmult;
			saber.radius = effectradius * angle_scale;
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			sbak.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius * 1.14f;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	//Fan Top
	//--------------------
	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			sbak.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius * angle_scale;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

void CG_DoSFXSaber(vec3_t blade_muz, vec3_t blade_tip, vec3_t trail_tip, vec3_t trail_muz, float length_max,
	float radius, saber_colors_t color, int rfx, qboolean do_light, int cnum, int bnum)
{
	vec3_t dif, mid, blade_dir, end_dir, trail_dir, base_dir;
	float radiusmult, effectradius, coreradius, effectalpha, angle_scale;
	float blade_len, end_len, trail_len, base_len, dis_tip, dis_muz, dis_dif;
	float v1, v2;
	vec3_t rgb = { 1, 1, 1 };
	int i;
	qhandle_t ignite = 0;
	qhandle_t blade = 0, glow = 0;
	refEntity_t saber, sbak;
	float ignite_len, ignite_radius;

	VectorSubtract(blade_tip, blade_muz, blade_dir);
	VectorSubtract(trail_tip, trail_muz, trail_dir);
	blade_len = VectorLength(blade_dir);
	trail_len = VectorLength(trail_dir);
	VectorNormalize(blade_dir);
	VectorNormalize(trail_dir);

	if (blade_len < MIN_SABERBLADE_DRAW_LENGTH)
	{
		return;
	}

	VectorSubtract(trail_tip, blade_tip, end_dir);
	VectorSubtract(trail_muz, blade_muz, base_dir);
	end_len = VectorLength(end_dir);
	base_len = VectorLength(base_dir);
	VectorNormalize(end_dir);
	VectorNormalize(base_dir);

	switch (color)
	{
	case SABER_RED:
		glow = cgs.media.redSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.redIgniteFlare;
		break;
	case SABER_ORANGE:
		glow = cgs.media.orangeSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.orangeIgniteFlare;
		break;
	case SABER_YELLOW:
		glow = cgs.media.yellowSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.yellowIgniteFlare;
		break;
	case SABER_GREEN:
		glow = cgs.media.greenSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_PURPLE:
		glow = cgs.media.purpleSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.purpleIgniteFlare;
		break;
	case SABER_BLUE:
		glow = cgs.media.blueSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.blueIgniteFlare;
		break;
	case SABER_LIME:
		glow = cgs.media.limeSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.greenIgniteFlare;
		break;
	case SABER_SCRIPTED:
	case SABER_WHITE:
	case SABER_PIMP:
	case SABER_RGB:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	case SABER_BLACK:
		glow = cgs.media.blackSaberGlowShader;
		blade = cgs.media.blackSaberCoreShader;
		ignite = cgs.media.blackIgniteFlare;
		break;
	case SABER_CUSTOM:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.whiteIgniteFlare02;
		break;
	default:
		glow = cgs.media.rgbSaberGlowShader;
		blade = cgs.media.sfxSaberBladeShader;
		ignite = cgs.media.whiteIgniteFlare;
		break;
	}

	VectorMA(blade_muz, blade_len * 0.5f, blade_dir, mid);

	if (do_light)
	{
		CG_RGBForSaberColor(color, rgb, cnum, bnum);
		VectorScale(rgb, 0.66f, rgb);
		trap->R_AddLightToScene(mid, blade_len * 2.0f + randoms() * 10.0f, rgb[0], rgb[1], rgb[2]);
	}

	//Distance Scale
	{
		float len;
		float glowscale = 0.5;
		VectorSubtract(mid, cg.refdef.vieworg, dif);
		len = VectorLength(dif);
		if (len > 4000)
		{
			len = 4000;
		}
		else if (len < 1)
		{
			len = 1;
		}

		v1 = (len + 400) / 400;
		v2 = (len + 4000) / 4000;

		if (end_len > 1 || base_len > 1)
		{
			if (end_len > base_len)
				glowscale = (end_len + 4) * 0.1;
			else
				glowscale = (base_len + 4) * 0.1;

			if (glowscale > 1.0)
				glowscale = 1.0;
		}
		effectalpha = glowscale;
	}

	//Angle Scale
	{
		VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
		dis_tip = VectorLength(dif);

		VectorSubtract(blade_muz, cg.refdef.vieworg, dif);
		dis_muz = VectorLength(dif);

		if (dis_tip > dis_muz)
		{
			dis_dif = dis_tip - dis_muz;
		}
		else if (dis_tip < dis_muz)
		{
			dis_dif = dis_muz - dis_tip;
		}
		else
		{
			dis_dif = 0;
		}

		angle_scale = 1.2f - dis_dif / blade_len * (dis_dif / blade_len);

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.2f)
			angle_scale = 0.2f;

		effectalpha *= angle_scale;

		angle_scale += 0.3f;

		if (angle_scale > 1.0f)
			angle_scale = 1.0f;
		if (angle_scale < 0.4f)
			angle_scale = 0.4f;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	if (blade_len < length_max)
	{
		radiusmult = 0.5 + blade_len / length_max / 2;
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = (radius * 1.6f * v1 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersGlowSizeSFX.value;
	coreradius = (radius * 0.4f * v2 + Q_flrand(-1.0f, 1.0f) * 0.1f) * radiusmult * cg_SFXSabersCoreSizeSFX.value;

	ignite_len = length_max * 0.30f;
	ignite_radius = effectradius * effectradius * 1.5f;
	ignite_radius -= blade_len;
	ignite_radius *= 2.2f;

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{
		rfx |= RF_FORCEPOST;
	}

	for (i = 0; i < 3; i++)
		rgb[i] *= 255;

	{
		saber.renderfx = rfx;
		if (blade_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = blade_len - saber.radius / 2;
			VectorCopy(blade_muz, saber.origin);
			VectorCopy(blade_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, blade_len, blade_dir, saber.origin);
		VectorMA(blade_muz, -1, blade_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	{
		saber.renderfx = rfx;
		if (trail_len - effectradius * angle_scale / 2 > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = trail_len - saber.radius / 2;
			VectorCopy(trail_muz, saber.origin);
			VectorCopy(trail_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(trail_muz, trail_len, trail_dir, saber.origin);
		VectorMA(trail_muz, -1, trail_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	VectorMA(blade_muz, blade_len - 0.5, blade_dir, blade_tip);
	VectorMA(trail_muz, trail_len - 0.5, trail_dir, trail_tip);

	if (base_len > 2)
	{
		saber.renderfx = rfx;
		if (base_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = base_len - effectradius * angle_scale;
			VectorMA(blade_muz, effectradius * angle_scale / 2, base_dir, saber.origin);
			VectorCopy(base_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_muz, base_len, base_dir, saber.origin);
		VectorMA(blade_muz, -0.1, base_dir, saber.oldorigin);

		saber.customShader = blade;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;

		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			// Add the saber surface that provides color.
			sbak.customShader = blade;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (end_len > 1)
	{
		{
			VectorSubtract(blade_tip, cg.refdef.vieworg, dif);
			dis_tip = VectorLength(dif);

			VectorSubtract(trail_tip, cg.refdef.vieworg, dif);
			dis_muz = VectorLength(dif);

			if (dis_tip > dis_muz)
			{
				dis_dif = dis_tip - dis_muz;
			}
			else if (dis_tip < dis_muz)
			{
				dis_dif = dis_muz - dis_tip;
			}
			else
			{
				dis_dif = 0;
			}

			if (dis_dif > end_len * 0.9)
			{
				effectalpha *= 0.3;
			}
			else if (dis_dif > end_len * 0.8)
			{
				effectalpha *= 0.5;
			}
			else if (dis_dif > end_len * 0.7)
			{
				effectalpha *= 0.7;
			}
		}

		saber.renderfx = rfx;
		if (end_len - effectradius * angle_scale > 0)
		{
			saber.radius = effectradius * angle_scale;
			saber.saberLength = end_len - effectradius * angle_scale;
			VectorMA(blade_tip, effectradius * angle_scale / 2, end_dir, saber.origin);
			VectorCopy(end_dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff *
				effectalpha;
			else
			{
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(blade_tip, end_len, end_dir, saber.origin);
		VectorMA(blade_tip, -0.1, end_dir, saber.oldorigin);

		saber.customShader = cgs.media.sfxSaberEndShader;
		saber.reType = RT_LINE;

		if (end_len > 9)
		{
			angle_scale = 5;
		}
		else if (end_len < 3)
		{
			angle_scale = 1;
		}
		else
		{
			angle_scale = end_len / 5;
		}

		{
			angle_scale -= dis_dif / end_len * (dis_dif / end_len) * angle_scale;

			if (angle_scale < 0.8)
				angle_scale = 0.8;
		}

		saber.radius = coreradius * angle_scale;
		saber.saberLength = end_len;

		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		if (color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for (i = 0; i < 3; i++)
				saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB || color == SABER_PIMP || color == SABER_WHITE || color == SABER_SCRIPTED)
		{
			sbak.customShader = cgs.media.sfxSaberEnd2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius * angle_scale;
			saber.saberLength = end_len;
			trap->R_AddRefEntityToScene(&sbak);
		}
	}

	if (blade_len <= ignite_len)
	{
		saber.renderfx = rfx;
		saber.radius = ignite_radius;
		VectorCopy(blade_muz, saber.origin);
		saber.reType = RT_SPRITE;
		saber.customShader = ignite;
		for (i = 0; i < 3; i++)
		{
			saber.shaderRGBA[i] = rgb[i];
			saber.shaderRGBA[3] = 255;
		}
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		if (color == SABER_BLACK)
		{
			saber.customShader = cgs.media.blackIgniteFlare;
		}
		else
		{
			saber.customShader = ignite;
			trap->R_AddRefEntityToScene(&saber);
		}
		saber.customShader = ignite;
		saber.radius = ignite_radius * 0.25f;
		saber.shaderRGBA[0] = 0xff;
		saber.shaderRGBA[1] = 0xff;
		saber.shaderRGBA[2] = 0xff;
		saber.shaderRGBA[3] = 0xff;
		trap->R_AddRefEntityToScene(&saber);

		if (color == SABER_RGB ||
			color == SABER_PIMP ||
			color == SABER_WHITE ||
			color == SABER_SCRIPTED)
		{
			saber.customShader = cgs.media.whiteIgniteFlare;
			saber.radius = ignite_radius * 0.25f;
			saber.shaderRGBA[0] = 0xff;
			saber.shaderRGBA[1] = 0xff;
			saber.shaderRGBA[2] = 0xff;
			saber.shaderRGBA[3] = 0xff;
			trap->R_AddRefEntityToScene(&saber);
		}
	}
}

//--------------------------------------------------------------
// CG_GetTagWorldPosition
//
// Can pass in NULL for the axis
//--------------------------------------------------------------
void CG_GetTagWorldPosition(refEntity_t* model, const char* tag, vec3_t pos, matrix3_t axis)
{
	orientation_t orientation;

	// Get the requested tag
	trap->R_LerpTag(&orientation, model->hModel, model->oldframe, model->frame,
		1.0f - model->backlerp, tag);

	VectorCopy(model->origin, pos);
	for (int i = 0; i < 3; i++)
	{
		VectorMA(pos, orientation.origin[i], model->axis[i], pos);
	}

	if (axis)
	{
		MatrixMultiply(orientation.axis, model->axis, axis);
	}
}

#define	MAX_MARK_FRAGMENTS	128
#define	MAX_MARK_POINTS		384
extern markPoly_t* CG_AllocMark();

void CG_CreateSaberMarks(vec3_t start, vec3_t end, vec3_t normal)
{
	//	byte			colors[4];
	int i, j;
	matrix3_t axis;
	vec3_t original_points[4];
	vec3_t markPoints[MAX_MARK_POINTS], projection;
	polyVert_t* v;
	markFragment_t markFragments[MAX_MARK_FRAGMENTS], * mf;

	if (!cg_marks.integer)
	{
		return;
	}

	VectorSubtract(end, start, axis[1]);
	VectorNormalize(axis[1]);

	// create the texture axis
	VectorCopy(normal, axis[0]);
	CrossProduct(axis[1], axis[0], axis[2]);

	// create the full polygon that we'll project
	for (i = 0; i < 3; i++)
	{
		const float radius = 0.65f;
		// stretch a bit more in the direction that we are traveling in...  debateable as to whether this makes things better or worse
		original_points[0][i] = start[i] - radius * axis[1][i] - radius * axis[2][i];
		original_points[1][i] = end[i] + radius * axis[1][i] - radius * axis[2][i];
		original_points[2][i] = end[i] + radius * axis[1][i] + radius * axis[2][i];
		original_points[3][i] = start[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	VectorScale(normal, -1, projection);

	// get the fragments
	const int numFragments = trap->R_MarkFragments(4, (const float(*)[3])original_points,
		projection, MAX_MARK_POINTS, markPoints[0], MAX_MARK_FRAGMENTS, markFragments);

	for (i = 0, mf = markFragments; i < numFragments; i++, mf++)
	{
		polyVert_t verts[MAX_VERTS_ON_POLY];
		// we have an upper limit on the complexity of polygons that we store persistantly
		if (mf->numPoints > MAX_VERTS_ON_POLY)
		{
			mf->numPoints = MAX_VERTS_ON_POLY;
		}

		for (j = 0, v = verts; j < mf->numPoints; j++, v++)
		{
			vec3_t mid;
			vec3_t delta;

			// Set up our texture coords, this may need some work
			VectorCopy(markPoints[mf->firstPoint + j], v->xyz);
			VectorAdd(end, start, mid);
			VectorScale(mid, 0.5f, mid);
			VectorSubtract(v->xyz, mid, delta);

			v->st[0] = 0.5f + DotProduct(delta, axis[1]) * (0.05f + Q_flrand(0.0f, 1.0f) * 0.03f);
			v->st[1] = 0.5f + DotProduct(delta, axis[2]) * (0.15f + Q_flrand(0.0f, 1.0f) * 0.05f);
		}

		if (cg_saberDynamicMarks.integer)
		{
			int i1 = 0;
			int i_2 = 0;
			addpolyArgStruct_t apArgs;
			vec3_t x;

			memset(&apArgs, 0, sizeof apArgs);

			while (i1 < 4)
			{
				while (i_2 < 3)
				{
					apArgs.p[i1][i_2] = verts[i1].xyz[i_2];

					i_2++;
				}

				i_2 = 0;
				i1++;
			}

			i1 = 0;
			i_2 = 0;

			while (i1 < 4)
			{
				while (i_2 < 2)
				{
					apArgs.ev[i1][i_2] = verts[i1].st[i_2];

					i_2++;
				}

				i_2 = 0;
				i1++;
			}

			//When using addpoly, having a situation like this tends to cause bad results.
			//(I assume it doesn't like trying to draw a polygon over two planes and extends
			//the vertex out to some odd value)
			VectorSubtract(apArgs.p[0], apArgs.p[3], x);
			if (VectorLength(x) > 3.0f)
			{
				return;
			}

			apArgs.numVerts = mf->numPoints;
			VectorCopy(vec3_origin, apArgs.vel);
			VectorCopy(vec3_origin, apArgs.accel);

			apArgs.alpha1 = 1.0f;
			apArgs.alpha2 = 0.0f;
			apArgs.alphaParm = 255.0f;

			VectorSet(apArgs.rgb1, 0.0f, 0.0f, 0.0f);
			VectorSet(apArgs.rgb2, 0.0f, 0.0f, 0.0f);

			apArgs.rgbParm = 0.0f;

			apArgs.bounce = 0;
			apArgs.motionDelay = 0;
			apArgs.killTime = cg_saberDynamicMarkTime.integer;
			apArgs.shader = cgs.media.rivetMarkShader;
			apArgs.flags = 0x08000000 | 0x00000004;

			trap->FX_AddPoly(&apArgs);

			apArgs.shader = cgs.media.mSaberDamageGlow;
			apArgs.rgb1[0] = 215 + Q_flrand(0.0f, 1.0f) * 40.0f;
			apArgs.rgb1[1] = 96 + Q_flrand(0.0f, 1.0f) * 32.0f;
			apArgs.rgb1[2] = apArgs.alphaParm = Q_flrand(0.0f, 1.0f) * 15.0f;

			apArgs.rgb1[0] /= 255;
			apArgs.rgb1[1] /= 255;
			apArgs.rgb1[2] /= 255;
			VectorCopy(apArgs.rgb1, apArgs.rgb2);

			apArgs.killTime = 100;

			trap->FX_AddPoly(&apArgs);
		}
		else
		{
			// save it persistantly, do burn first
			markPoly_t* mark = CG_AllocMark();
			mark->time = cg.time;
			mark->alphaFade = qtrue;
			mark->markShader = cgs.media.rivetMarkShader;
			mark->poly.numVerts = mf->numPoints;
			mark->color[0] = mark->color[1] = mark->color[2] = mark->color[3] = 255;
			memcpy(mark->verts, verts, mf->numPoints * sizeof verts[0]);

			// And now do a glow pass
			// by moving the start time back, we can hack it to fade out way before the burn does
			mark = CG_AllocMark();
			mark->time = cg.time - 8500;
			mark->alphaFade = qfalse;
			mark->markShader = cgs.media.mSaberDamageGlow;
			mark->poly.numVerts = mf->numPoints;
			mark->color[0] = 215 + Q_flrand(0.0f, 1.0f) * 40.0f;
			mark->color[1] = 96 + Q_flrand(0.0f, 1.0f) * 32.0f;
			mark->color[2] = mark->color[3] = Q_flrand(0.0f, 1.0f) * 15.0f;
			memcpy(mark->verts, verts, mf->numPoints * sizeof verts[0]);
		}
	}
}

qboolean CG_G2TraceCollide(trace_t* tr, const vec3_t mins, const vec3_t maxs, const vec3_t last_valid_start,
	const vec3_t last_valid_end)
{
	G2Trace_t g2_trace;
	int t_n = 0;
	float f_radius = 0.0f;

	if (mins && maxs &&
		(mins[0] || maxs[0]))
	{
		f_radius = (maxs[0] - mins[0]) / 2.0f;
	}

	memset(&g2_trace, 0, sizeof g2_trace);

	while (t_n < MAX_G2_COLLISIONS)
	{
		g2_trace[t_n].mEntityNum = -1;
		t_n++;
	}
	centity_t* g2_hit = &cg_entities[tr->entityNum];

	if (g2_hit && g2_hit->ghoul2)
	{
		vec3_t angles;
		angles[ROLL] = angles[PITCH] = 0;
		angles[YAW] = g2_hit->lerpAngles[YAW];

		if (com_optvehtrace.integer &&
			g2_hit->currentState.eType == ET_NPC &&
			g2_hit->currentState.NPC_class == CLASS_VEHICLE &&
			g2_hit->m_pVehicle)
		{
			trap->G2API_CollisionDetectCache(g2_trace, g2_hit->ghoul2, angles, g2_hit->lerpOrigin, cg.time,
				g2_hit->currentState.number, (float*)last_valid_start,
				(float*)last_valid_end,
				g2_hit->modelScale, 0, cg_g2TraceLod.integer, f_radius);
		}
		else
		{
			trap->G2API_CollisionDetect(g2_trace, g2_hit->ghoul2, angles, g2_hit->lerpOrigin, cg.time,
				g2_hit->currentState.number, (float*)last_valid_start, (float*)last_valid_end,
				g2_hit->modelScale, 0, cg_g2TraceLod.integer, f_radius);
		}

		if (g2_trace[0].mEntityNum != g2_hit->currentState.number)
		{
			tr->fraction = 1.0f;
			tr->entityNum = ENTITYNUM_NONE;
			tr->startsolid = 0;
			tr->allsolid = 0;
			return qfalse;
		}
		//Yay!
		VectorCopy(g2_trace[0].mCollisionPosition, tr->endpos);
		VectorCopy(g2_trace[0].mCollisionNormal, tr->plane.normal);
		return qtrue;
	}

	return qfalse;
}

void CG_G2SaberEffects(vec3_t start, vec3_t end, const centity_t* owner)
{
	trace_t trace;
	qboolean back_wards = qfalse;
	qboolean done_with_traces = qfalse;

	while (!done_with_traces)
	{
		vec3_t end_tr;
		vec3_t start_tr;
		if (!back_wards)
		{
			VectorCopy(start, start_tr);
			VectorCopy(end, end_tr);
		}
		else
		{
			VectorCopy(end, start_tr);
			VectorCopy(start, end_tr);
		}

		CG_Trace(&trace, start_tr, NULL, NULL, end_tr, owner->currentState.number, MASK_PLAYERSOLID);

		if (trace.entityNum < MAX_CLIENTS || cg_entities[trace.entityNum].currentState.eType == ET_NPC)
		{
			//hit a client..
			CG_G2TraceCollide(&trace, NULL, NULL, start_tr, end_tr);

			if (trace.entityNum != ENTITYNUM_NONE)
			{
				//it succeeded with the ghoul2 trace
				trap->FX_PlayEffectID(cgs.effects.mSaberBloodSparks, trace.endpos, trace.plane.normal, -1, -1, qfalse);
				trap->S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap->S_RegisterSound(va("sound/weapons/saber/saberhit%i.mp3", Q_irand(1, 15))));
			}
		}

		if (!back_wards)
		{
			back_wards = qtrue;
		}
		else
		{
			done_with_traces = qtrue;
		}
	}
}

#define CG_MAX_SABER_COMP_TIME 400 //last registered saber entity hit must match within this many ms for the client effect to take place.

void CG_AddGhoul2Mark(const int shader, const float size, vec3_t start, vec3_t end, const int entnum,
	vec3_t entposition, const float entangle, void* ghoul2, vec3_t scale, const int life_time)
{
	SSkinGoreData gore_skin;

	assert(ghoul2);

	memset(&gore_skin, 0, sizeof gore_skin);

	if (trap->G2API_GetNumGoreMarks(ghoul2, 0) >= cg_ghoul2Marks.integer)
	{
		//you've got too many marks already
		return;
	}

	gore_skin.growDuration = -1; // default expandy time
	gore_skin.goreScaleStartFraction = 1.0; // default start scale
	gore_skin.frontFaces = qtrue;
	gore_skin.backFaces = qtrue;
	gore_skin.lifeTime = life_time; //last randomly 10-20 seconds
	/*
	if (lifeTime)
	{
		goreSkin.fadeOutTime = lifeTime*0.1; //default fade duration is relative to lifetime.
	}
	goreSkin.fadeRGB = qtrue; //fade on RGB instead of alpha (this depends on the shader really, modify if needed)
	*/
	//rwwFIXMEFIXME: fade has sorting issues with other non-fading decals, disabled until fixed

	gore_skin.baseModelOnly = qfalse;

	gore_skin.currentTime = cg.time;
	gore_skin.entNum = entnum;
	gore_skin.SSize = size;
	gore_skin.TSize = size;
	gore_skin.theta = flrand(0.0f, 6.28f);
	gore_skin.shader = shader;

	if (!scale[0] && !scale[1] && !scale[2])
	{
		VectorSet(gore_skin.scale, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		VectorCopy(gore_skin.scale, scale);
	}

	VectorCopy(start, gore_skin.hitLocation);

	VectorSubtract(end, start, gore_skin.rayDirection);
	if (VectorNormalize(gore_skin.rayDirection) < .1f)
	{
		return;
	}

	VectorCopy(entposition, gore_skin.position);
	gore_skin.angles[YAW] = entangle;

	trap->G2API_AddSkinGore(ghoul2, &gore_skin);
}

void CG_SaberCompWork(vec3_t start, vec3_t end, centity_t* owner, const int saberNum, const int blade_num)
{
	trace_t trace;
	const qboolean back_wards = qfalse;
	qboolean done_with_traces = qfalse;
	qboolean do_effect = qfalse;
	clientInfo_t* client;

	if (cg.time - owner->serverSaberHitTime > CG_MAX_SABER_COMP_TIME)
	{
		return;
	}

	if (cg.time == owner->serverSaberHitTime)
	{
		//don't want to do it the same frame as the server hit, to avoid burst effect concentrations every x ms.
		return;
	}

	while (!done_with_traces)
	{
		vec3_t end_tr;
		vec3_t start_tr;
		if (!back_wards)
		{
			VectorCopy(start, start_tr);
			VectorCopy(end, end_tr);
		}

		CG_Trace(&trace, start_tr, NULL, NULL, end_tr, owner->currentState.number, MASK_PLAYERSOLID);

		if (trace.entityNum == owner->serverSaberHitIndex)
		{
			//this is the guy the server says we last hit, so continue.
			if (cg_entities[trace.entityNum].ghoul2)
			{
				//If it has a g2 instance, do the proper ghoul2 checks
				CG_G2TraceCollide(&trace, NULL, NULL, start_tr, end_tr);

				if (trace.entityNum != ENTITYNUM_NONE)
				{
					//it succeeded with the ghoul2 trace
					do_effect = qtrue;

					if (cg_ghoul2Marks.integer)
					{
						centity_t* tr_ent = &cg_entities[trace.entityNum];

						if (tr_ent->ghoul2)
						{
							if (tr_ent->currentState.eType != ET_NPC ||
								tr_ent->currentState.NPC_class != CLASS_VEHICLE ||
								!tr_ent->m_pVehicle ||
								tr_ent->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
							{
								vec3_t e_pos;
								//don't do on fighters cause they have crazy full axial angles
								int weapon_mark_shader = 0, markShader = cgs.media.bdecal_saberglow;

								VectorSubtract(end_tr, trace.endpos, e_pos);
								VectorNormalize(e_pos);
								VectorMA(trace.endpos, 4.0f, e_pos, e_pos);

								if (owner->currentState.eType == ET_NPC)
								{
									client = owner->npcClient;
								}
								else
								{
									client = &cgs.clientinfo[owner->currentState.clientNum];
								}
								if (client
									&& client->infoValid)
								{
									if (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num))
									{
										if (client->saber[saberNum].g2MarksShader2)
										{
											//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader2;
										}
										if (client->saber[saberNum].g2WeaponMarkShader2)
										{
											//we have a shader to use as a splashback onto the weapon model
											weapon_mark_shader = client->saber[saberNum].g2WeaponMarkShader2;
										}
									}
									else
									{
										if (client->saber[saberNum].g2MarksShader)
										{
											//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader;
										}
										if (client->saber[saberNum].g2WeaponMarkShader)
										{
											//we have a shader to use as a splashback onto the weapon model
											weapon_mark_shader = client->saber[saberNum].g2WeaponMarkShader;
										}
									}
								}
								//ROP VEHICLE_IMP START
								//Cannot be marked if we are a vehicle which - well - can't be marked ;)
								if (!(tr_ent->currentState.NPC_class == CLASS_VEHICLE
									&& tr_ent->m_pVehicle && tr_ent->m_pVehicle->m_pVehicleInfo->ResistsMarking))
								{
									CG_AddGhoul2Mark(markShader, flrand(3.0f, 4.0f),
										trace.endpos, e_pos, trace.entityNum, tr_ent->lerpOrigin,
										tr_ent->lerpAngles[YAW],
										tr_ent->ghoul2, tr_ent->modelScale, Q_irand(5000, 10000));
									if (weapon_mark_shader)
									{
										vec3_t splash_back_dir;
										VectorScale(e_pos, -1, splash_back_dir);
										CG_AddGhoul2Mark(weapon_mark_shader, flrand(0.5f, 2.0f),
											trace.endpos, splash_back_dir, owner->currentState.clientNum,
											owner->lerpOrigin, owner->lerpAngles[YAW],
											owner->ghoul2, owner->modelScale, Q_irand(5000, 10000));
									}

									if (client && client->infoValid)
									{// Also do blood sparks here...
										trap->FX_PlayEffectID(cgs.effects.mSaberBloodSparks, trace.endpos, trace.plane.normal, -1, -1, qfalse);
										trap->S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap->S_RegisterSound(va("sound/weapons/saber/saberhit%i.mp3", Q_irand(1, 15))));
									}
								}
							}
						}
					}
				}
			}
			else
			{
				//otherwise, we're all set.
				do_effect = qtrue;
			}

			if (do_effect)
			{
				int hit_person_fx_id = cgs.effects.mSaberBloodSparks;
				const int hit_other_fx_id2 = cgs.effects.mSaberBodyHit;

				if (owner->currentState.eType == ET_NPC)
				{
					client = owner->npcClient;
				}
				else
				{
					client = &cgs.clientinfo[owner->currentState.clientNum];
				}
				if (client && client->infoValid)
				{
					if (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num))
					{
						//use second blade style values
						if (client->saber[saberNum].hitPersonEffect2)
						{
							hit_person_fx_id = client->saber[saberNum].hitPersonEffect2;
						}
						if (client->saber[saberNum].hitOtherEffect2)
						{
							//custom hit other effect
							client->saber[saberNum].hitOtherEffect2;
						}
					}
					else
					{
						//use first blade style values
						if (client->saber[saberNum].hitPersonEffect)
						{
							hit_person_fx_id = client->saber[saberNum].hitPersonEffect;
						}
						if (client->saber[saberNum].hitOtherEffect)
						{
							//custom hit other effect
							client->saber[saberNum].hitOtherEffect;
						}
					}
				}
				if (!trace.plane.normal[0] && !trace.plane.normal[1] && !trace.plane.normal[2])
				{
					//who cares, just shoot it somewhere.
					trace.plane.normal[1] = 1;
				}

				if (owner->serverSaberFleshImpact)
				{
					//do standard player/live ent hit sparks
					trap->FX_PlayEffectID(hit_person_fx_id, trace.endpos, trace.plane.normal, -1, -1, qfalse);
				}
				else
				{
					//do the cut effect
					trap->FX_PlayEffectID(hit_other_fx_id2, trace.endpos, trace.plane.normal, -1, -1, qfalse);
				}
				do_effect = qfalse;
			}
		}
		done_with_traces = qtrue; //disabling backwards tr for now, sometimes it just makes too many effects.
	}
}

#define SABER_TRAIL_TIME	40.0f
#define FX_USE_ALPHA		0x08000000

qboolean PM_SuperBreakWinAnim(int anim);

void CG_AddSaberBlade(centity_t* cent, centity_t* scent, int renderfx, int saberNum, int blade_num, vec3_t origin,
	vec3_t angles, qboolean from_saber, qboolean dont_draw)
{
	vec3_t org, end,
		axis[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // shut the compiler up
	vec3_t rgb1 = { 255.0f, 255.0f, 255.0f };
	trace_t trace;
	float saber_len;
	clientInfo_t* client;
	centity_t* saberEnt;
	saberTrail_t* saber_trail;
	mdxaBone_t boltMatrix;
	vec3_t future_angles;
	effectTrailArgStruct_t fx;
	int scolor = 0;
	int use_model_index = 0;

	if (cent->currentState.eType == ET_NPC)
	{
		client = cent->npcClient;
		assert(client);
	}
	else
	{
		client = &cgs.clientinfo[cent->currentState.number];
	}

	saberEnt = &cg_entities[cent->currentState.saberEntityNum];
	saber_len = client->saber[saberNum].blade[blade_num].length;

	if (saber_len <= 0 && !dont_draw)
	{
		//don't bother then.
		return;
	}

	future_angles[YAW] = angles[YAW];
	future_angles[PITCH] = angles[PITCH];
	future_angles[ROLL] = angles[ROLL];

	if (from_saber)
	{
		use_model_index = 0;
	}
	else
	{
		use_model_index = saberNum + 1;
	}
	//Assume blade_num is equal to the bolt index because bolts should be added in order of the blades.
	//if there is an effect on this blade, play it
	if (!WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num)
		&& client->saber[saberNum].bladeEffect)
	{
		trap->FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect, scent->lerpOrigin,
			scent->ghoul2, blade_num, scent->currentState.number, use_model_index, -1, qfalse);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num)
		&& client->saber[saberNum].bladeEffect2)
	{
		trap->FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect2, scent->lerpOrigin,
			scent->ghoul2, blade_num, scent->currentState.number, use_model_index, -1, qfalse);
	}
	//get the boltMatrix
	trap->G2API_GetBoltMatrix(scent->ghoul2, use_model_index, blade_num, &boltMatrix, future_angles, origin, cg.time,
		cgs.game_models, scent->modelScale);

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, org);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis[0]);

	if (!from_saber && saberEnt && !cent->currentState.saberInFlight)
	{
		VectorCopy(org, saberEnt->currentState.pos.trBase);

		VectorCopy(axis[0], saberEnt->currentState.apos.trBase);
	}

	VectorMA(org, saber_len, axis[0], end);

	VectorAdd(end, axis[0], end);

	if (cent->currentState.eType == ET_NPC)
	{
		scolor = client->saber[saberNum].blade[blade_num].color;
	}
	else
	{
		if (saberNum == 0)
		{
			scolor = client->icolor1;
		}
		else
		{
			scolor = client->icolor2;
		}
	}

	if ((team_sabercolours.integer < 1
		|| team_sabercolours.integer == 1 && cg.snap && cg.snap->ps.clientNum != cent->currentState.number) &&
		cgs.gametype >= GT_TEAM &&
		cgs.gametype != GT_SIEGE &&
		!cgs.jediVmerc &&
		cent->currentState.eType != ET_NPC)
	{
		//racc - in a team game where we want to override the saber colors.
		if (client->team == TEAM_RED)
		{
			scolor = SABER_RED;
		}
		else if (client->team == TEAM_BLUE)
		{
			scolor = SABER_BLUE;
		}
	}

	if (!cg_saberContact.integer)
	{
		//if we don't have saber contact enabled, just add the blade and don't care what it's touching
		goto CheckTrail;
	}

	if (!dont_draw)
	{
		int i = 0;
		if (cg_saberModelTraceEffect.integer)
		{
			CG_G2SaberEffects(org, end, cent);
		}
		else if (cg_saberClientVisualCompensation.integer)
		{
			CG_Trace(&trace, org, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID);

			if (trace.fraction != 1)
			{
				//nudge the endpos a very small amount from the beginning to the end, so the comp trace hits at the end.
				//I'm only bothering with this because I want to do a backwards trace too in the comp trace, so if the
				//blade is sticking through a player or something the standard trace doesn't it, it will make sparks
				//on each side.
				vec3_t se_dif;

				VectorSubtract(trace.endpos, org, se_dif);
				VectorNormalize(se_dif);
				trace.endpos[0] += se_dif[0] * 0.1f;
				trace.endpos[1] += se_dif[1] * 0.1f;
				trace.endpos[2] += se_dif[2] * 0.1f;
			}

			if (client->saber[saberNum].blade[blade_num].storageTime < cg.time)
			{
				//debounce it in case our framerate is absurdly high. Using storageTime since it's not used for anything else in the client.
				CG_SaberCompWork(org, trace.endpos, cent, saberNum, blade_num);

				client->saber[saberNum].blade[blade_num].storageTime = cg.time + 5;
			}
		}

		for (i = 0; i < 1; i++)
			//was 2 because it would go through architecture and leave saber trails on either side of the brush - but still looks bad if we hit a corner, blade is still 8 longer than hit
		{
			if (i)
			{
				//tracing from end to base
				CG_Trace(&trace, end, NULL, NULL, org, ENTITYNUM_NONE, MASK_SOLID);
			}
			else
			{
				//tracing from base to end
				CG_Trace(&trace, org, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID);
			}

			if (trace.fraction < 1.0f)
			{
				vec3_t v;
				vec3_t tr_dir;
				int doit = 1; //consider NonX

				VectorCopy(trace.plane.normal, tr_dir);
				if (!tr_dir[0] && !tr_dir[1] && !tr_dir[2])
				{
					tr_dir[1] = 1;
				}

				if (cg.snap->ps.duelInProgress)
				{ // this client is dueling
					if (cent->currentState.number >= 0 && cent->currentState.number < MAX_CLIENTS && cent->currentState.number != cg.snap->ps.clientNum && cent->currentState.number != cg.snap->ps.duelIndex)
					{	// sound originated from a client, but not one of the duelers
						doit = 0;
					}
					else if (cent->currentState.number >= MAX_CLIENTS && cent->currentState.otherentity_num != ENTITYNUM_NONE &&
						cent->currentState.otherentity_num != cg.snap->ps.clientNum &&
						cent->currentState.otherentity_num != cg.snap->ps.duelIndex)
					{  // sound generated by an temp entity in a snap, the otherentity_num should be the orinating
					// client number (by hack!). If otherentity_num is ENTITYNUM_NONE, then it is one of the many sounds
					// I don't bother filtering, so play it. Otherwise, if not from the duelers, supress it
						doit = 0;
					}
				}
				if (doit)
				{
					if (client->saber[saberNum].saberFlags2 & SFL2_NO_WALL_MARKS)
					{
						//don't actually draw the marks/impact effects
					}
					else
					{
						if (!(trace.surfaceFlags & SURF_NOIMPACT)) // never spark on sky
						{
							trap->FX_PlayEffectID(cgs.effects.mSparks, trace.endpos, tr_dir, -1, -1, qfalse);
						}
						else
						{
							if (trace.contents & CONTENTS_WATER | CONTENTS_SLIME)
							{
								if (Q_irand(1, client->saber[saberNum].numBlades) == 1)
								{
									trap->FX_PlayEffectID(cgs.effects.mboil, trace.endpos, tr_dir, -1, -1, qfalse);
								}
							}
						}
					}
				}

				//Stop saber? (it wouldn't look right if it was stuck through a thin wall and unable to hurt players on the other side)
				VectorSubtract(org, trace.endpos, v);
				saber_len = VectorLength(v);

				VectorCopy(trace.endpos, end);

				if (client->saber[saberNum].saberFlags2 & SFL2_NO_WALL_MARKS)
				{
					//don't actually draw the marks
				}
				else
				{
					//draw marks if we hit a wall
					// All I need is a bool to mark whether I have a previous point to work with.
					//....come up with something better..
					if (client->saber[saberNum].blade[blade_num].trail.haveOldPos[i])
					{
						if (trace.entityNum == ENTITYNUM_WORLD || cg_entities[trace.entityNum].currentState.eType ==
							ET_TERRAIN || cg_entities[trace.entityNum].currentState.eFlags & EF_PERMANENT)
						{
							//only put marks on architecture
							// Let's do some cool burn/glowing mark bits!!!
							CG_CreateSaberMarks(client->saber[saberNum].blade[blade_num].trail.oldPos[i], trace.endpos,
								trace.plane.normal);

							//make a sound
							if (cg.time - client->saber[saberNum].blade[blade_num].hitWallDebounceTime >= 100)
							{
								//ugh, need to have a real sound debouncer... or do this game-side
								client->saber[saberNum].blade[blade_num].hitWallDebounceTime = cg.time;
								if (PM_SaberInAttack(cent->currentState.saber_move)
									|| pm_saber_in_special_attack(cent->currentState.torsoAnim))
								{
									trap->S_StartSound(trace.endpos, -1, CHAN_WEAPON,
										trap->S_RegisterSound(va(
											"sound/weapons/saber/saberstrikewall%i", Q_irand(1, 17))));
								}
								else
								{
									trap->S_StartSound(trace.endpos, -1, CHAN_WEAPON,
										trap->S_RegisterSound(va(
											"sound/weapons/saber/saberhitwall%i", Q_irand(1, 3))));
								}
							}
						}
					}
					else
					{
						// if we impact next frame, we'll mark a slash mark
						client->saber[saberNum].blade[blade_num].trail.haveOldPos[i] = qtrue;
					}
				}

				// stash point so we can connect-the-dots later
				VectorCopy(trace.endpos, client->saber[saberNum].blade[blade_num].trail.oldPos[i]);
				VectorCopy(trace.plane.normal, client->saber[saberNum].blade[blade_num].trail.oldNormal[i]);
			}
			else
			{
				if (client->saber[saberNum].blade[blade_num].trail.haveOldPos[i])
				{
					// Hmmm, no impact this frame, but we have an old point
					// Let's put the mark there, we should use an endcap mark to close the line, but we
					//	can probably just get away with a round mark
					//					CG_ImpactMark( cgs.media.rivetMarkShader, client->saber[saberNum].blade[blade_num].trail.oldPos[i], client->saber[saberNum].blade[blade_num].trail.oldNormal[i],
					//							0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
				}

				// we aren't impacting, so turn off our mark tracking mechanism
				client->saber[saberNum].blade[blade_num].trail.haveOldPos[i] = qfalse;
			}
		}
	}
CheckTrail:

	if (!cg_saberTrail.integer)
	{
		//don't do the trail in this case
		goto JustDoIt;
	}

	if (!WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num) && client->saber[saberNum].trailStyle >
		1
		|| WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num) && client->saber[saberNum].
		trailStyle2 >
		1)
	{
		//don't actually draw the trail at all
		goto JustDoIt;
	}

	//FIXME: if trailStyle is 1, use the motion blur instead

	saber_trail = &client->saber[saberNum].blade[blade_num].trail;
	if (cg_SFXSabers.integer == 0)
	{
		int trail_dur;
		// Use Raven's superior sabers.
		saber_trail->duration = saber_moveData[cent->currentState.saber_move].trailLength;

		if (cent->currentState.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attack faking, have a longer saber trail
			saber_trail->duration *= 2;
		}

		if (cent->currentState.userInt3 & 1 << FLAG_FATIGUED)
		{
			//fatigued players have slightly shorter saber trails since they're moving slower.
			saber_trail->duration *= .5;
		}

		trail_dur = saber_trail->duration / 5.0f;

		if (!trail_dur)
		{
			//hmm.. ok, default
			if (PM_SuperBreakWinAnim(cent->currentState.torsoAnim))
			{
				trail_dur = 150;
			}
			else
			{
				trail_dur = SABER_TRAIL_TIME;
			}
		}

		// if we happen to be timescaled or running in a high framerate situation, we don't want to flood
		//	the system with very small trail slices...but perhaps doing it by distance would yield better results?
		if (cg.time > saber_trail->lastTime + 2 || cg_saberTrail.integer == 2) // 2ms
		{
			if (!dont_draw)
			{
				if (client->saber[saberNum].type == SABER_SITH_SWORD
					|| (PM_SuperBreakWinAnim(cent->currentState.torsoAnim)
						|| saber_moveData[cent->currentState.saber_move].trailLength > 0
						|| cent->currentState.powerups & 1 << PW_SPEED && cg_speedTrail.integer
						|| cent->currentState.saberInFlight && saberNum == 0)
					&& cg.time < saber_trail->lastTime + 2000)
					// if we have a stale segment, don't draw until we have a fresh one
				{
#if 0
					if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
					{
						polyVert_t	verts[4];

						VectorCopy(org_, verts[0].xyz);
						VectorMA(end, 3.0f, axis_[0], verts[1].xyz);
						VectorCopy(saberTrail->tip, verts[2].xyz);
						VectorCopy(saberTrail->base, verts[3].xyz);

						//tc doesn't even matter since we're just gonna stencil an outline, but whatever.
						verts[0].st[0] = 0;
						verts[0].st[1] = 0;
						verts[0].modulate[0] = 255;
						verts[0].modulate[1] = 255;
						verts[0].modulate[2] = 255;
						verts[0].modulate[3] = 255;

						verts[1].st[0] = 0;
						verts[1].st[1] = 1;
						verts[1].modulate[0] = 255;
						verts[1].modulate[1] = 255;
						verts[1].modulate[2] = 255;
						verts[1].modulate[3] = 255;

						verts[2].st[0] = 1;
						verts[2].st[1] = 1;
						verts[2].modulate[0] = 255;
						verts[2].modulate[1] = 255;
						verts[2].modulate[2] = 255;
						verts[2].modulate[3] = 255;

						verts[3].st[0] = 1;
						verts[3].st[1] = 0;
						verts[3].modulate[0] = 255;
						verts[3].modulate[1] = 255;
						verts[3].modulate[2] = 255;
						verts[3].modulate[3] = 255;

						//don't capture postrender objects (now we'll postrender the saber so it doesn't get in the capture)
						trap_R_SetRefractProp(1.0f, 0.0f, qtrue, qtrue);

						//shader 2 is always the crazy refractive shader.
						trap_R_AddPolyToScene(2, 4, verts);
					}
					else
#endif
					{
						float diff;
						switch (scolor)
						{
						case SABER_RED:
							VectorSet(rgb1, 255.0f, 0.0f, 0.0f);
							break;
						case SABER_ORANGE:
							VectorSet(rgb1, 255.0f, 64.0f, 0.0f);
							break;
						case SABER_YELLOW:
							VectorSet(rgb1, 255.0f, 255.0f, 0.0f);
							break;
						case SABER_GREEN:
							VectorSet(rgb1, 0.0f, 255.0f, 0.0f);
							break;
						case SABER_BLUE:
							VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
							break;
						case SABER_PURPLE:
							VectorSet(rgb1, 220.0f, 0.0f, 255.0f);
							break;
						case SABER_LIME:
							VectorSet(rgb1, 0.0f, 255.0f, 0.0f);
							break;
						case SABER_RGB:
						{
							int cnum = cent->currentState.clientNum;
							if (cnum < MAX_CLIENTS)
							{
								clientInfo_t* ci = &cgs.clientinfo[cnum];

								if (saberNum == 0)
									VectorCopy(ci->rgb1, rgb1);
								else
									VectorCopy(ci->rgb2, rgb1);
							}
							else
								VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
						}
						break;
						case SABER_PIMP:
						{
							int cnum = cent->currentState.clientNum;
							if (cnum < MAX_CLIENTS)
							{
								clientInfo_t* ci = &cgs.clientinfo[cnum];
								RGB_AdjustPimpSaberColor(ci, rgb1, saberNum);
							}
							else
								VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
						}
						break;
						case SABER_SCRIPTED:
						{
							int cnum = cent->currentState.clientNum;
							if (cnum < MAX_CLIENTS)
							{
								clientInfo_t* ci = &cgs.clientinfo[cnum];
								rgb_adjust_scipted_saber_color(ci, rgb1, saberNum);
							}
							else
								VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
						}
						break;
						case SABER_BLACK:
							VectorSet(rgb1, 1.0f, 1.0f, 1.0f);
							break;
						case SABER_WHITE:
							VectorSet(rgb1, 1.0f, 1.0f, 1.0f);
							break;
						default:
							VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
							break;
						}

						//Here we will use the happy process of filling a struct in with arguments and passing it to a trap function
						//so that we can take the struct and fill in an actual CTrail type using the data within it once we get it
						//into the effects area

						// Go from new muzzle to new end...then to old end...back down to old muzzle...finally
						//	connect back to the new muzzle...this is our trail quad
						VectorCopy(org, fx.mVerts[0].origin);
						VectorMA(end, 3.0f, axis[0], fx.mVerts[1].origin);

						VectorCopy(saber_trail->tip, fx.mVerts[2].origin);
						VectorCopy(saber_trail->base, fx.mVerts[3].origin);

						diff = cg.time - saber_trail->lastTime;

						if (diff <= 10000)
						{
							//don't draw it if the last time is way out of date
							float old_alpha = 1.0f - diff / trail_dur;

							if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
							{
								//does other stuff below
							}
							else
							{
								if (client->saber[saberNum].type == SABER_SITH_SWORD
									|| !WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num) && client
									->
									saber[saberNum].trailStyle == 1
									|| WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], blade_num) && client
									->
									saber[saberNum].trailStyle2 == 1)
								{
									//motion trail
									fx.mShader = cgs.media.swordTrailShader;
									VectorSet(rgb1, 32.0f, 32.0f, 32.0f); // make the sith sword trail pretty faint
									trail_dur *= 2.0f; // stay around twice as long?
								}
								else if (client->saber[saberNum].type == SABER_UNSTABLE
									|| client->saber[saberNum].type == SABER_STAFF_UNSTABLE
									|| client->saber[saberNum].type == SABER_ELECTROSTAFF)
								{
									fx.mShader = cgs.media.unstableBlurShader;
								}
								else
								{
									switch (scolor)
									{
									case SABER_RED:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_ORANGE:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_YELLOW:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_GREEN:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_BLUE:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_PURPLE:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_LIME:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_RGB:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_PIMP:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_SCRIPTED:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									case SABER_BLACK:
										fx.mShader = cgs.media.blackSaberTrail;
										break;
									case SABER_WHITE:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									default:
										fx.mShader = cgs.media.saberBlurShader;
										break;
									}
								}
								fx.mKillTime = trail_dur;
								fx.mSetFlags = FX_USE_ALPHA;
							}

							// New muzzle
							VectorCopy(rgb1, fx.mVerts[0].rgb);
							fx.mVerts[0].alpha = 255.0f;

							fx.mVerts[0].ST[0] = 0.0f;
							fx.mVerts[0].ST[1] = 1.0f;
							fx.mVerts[0].destST[0] = 1.0f;
							fx.mVerts[0].destST[1] = 1.0f;

							// new tip
							VectorCopy(rgb1, fx.mVerts[1].rgb);
							fx.mVerts[1].alpha = 255.0f;

							fx.mVerts[1].ST[0] = 0.0f;
							fx.mVerts[1].ST[1] = 0.0f;
							fx.mVerts[1].destST[0] = 1.0f;
							fx.mVerts[1].destST[1] = 0.0f;

							// old tip
							VectorCopy(rgb1, fx.mVerts[2].rgb);
							fx.mVerts[2].alpha = 255.0f;

							fx.mVerts[2].ST[0] = 1.0f - old_alpha;
							// NOTE: this just happens to contain the value I want
							fx.mVerts[2].ST[1] = 0.0f;
							fx.mVerts[2].destST[0] = 1.0f + fx.mVerts[2].ST[0];
							fx.mVerts[2].destST[1] = 0.0f;

							// old muzzle
							VectorCopy(rgb1, fx.mVerts[3].rgb);
							fx.mVerts[3].alpha = 255.0f;

							fx.mVerts[3].ST[0] = 1.0f - old_alpha;
							// NOTE: this just happens to contain the value I want
							fx.mVerts[3].ST[1] = 1.0f;
							fx.mVerts[3].destST[0] = 1.0f + fx.mVerts[2].ST[0];
							fx.mVerts[3].destST[1] = 1.0f;

							if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
							{
								trap->R_SetRefractionProperties(1.0f, 0.0f, qtrue, qtrue);
								//don't need to do this every frame.. but..

								if (PM_SaberInAttack(cent->currentState.saber_move)
									|| PM_SuperBreakWinAnim(cent->currentState.torsoAnim))
								{
									//in attack, strong trail
									fx.mKillTime = 300;
								}
								else
								{
									//faded trail
									fx.mKillTime = 40;
								}
								fx.mShader = 2; //2 is always refractive shader
								fx.mSetFlags = FX_USE_ALPHA;
							}
							if (scolor == SABER_BLACK)
							{
								fx.mShader = cgs.media.blackSaberTrail;
							}

							trap->FX_AddPrimitive(&fx);
						}
					}
				}
			}

			// we must always do this, even if we aren't active..otherwise we won't know where to pick up from
			VectorCopy(org, saber_trail->base);
			VectorMA(end, 3.0f, axis[0], saber_trail->tip);
			saber_trail->lastTime = cg.time;
		}
	}
	else
	{
		// Use the supremely hacky SFX Sabers.
		saber_trail->duration = 0;

		if (!saber_trail->base || !saber_trail->tip || !saber_trail->dualbase || !saber_trail->dualtip || !saber_trail->
			lastTime || !saber_trail->inAction)
		{
			VectorCopy(org, saber_trail->base);
			VectorMA(end, -1.5f, axis[0], saber_trail->tip);
			VectorCopy(saber_trail->base, saber_trail->dualbase);
			VectorCopy(saber_trail->tip, saber_trail->dualtip);
			saber_trail->lastTime = cg.time;
			saber_trail->inAction = cg.time;
			return;
		}
		if (cg.time > saber_trail->lastTime)
		{
			float dirlen0, dirlen1, dirlen2, lagscale;
			vec3_t dir0, dir1, dir2;

			VectorCopy(saber_trail->base, saber_trail->dualbase);
			VectorCopy(saber_trail->tip, saber_trail->dualtip);

			VectorCopy(org, saber_trail->base);
			VectorMA(end, -1.5f, axis[0], saber_trail->tip);

			VectorSubtract(saber_trail->dualtip, saber_trail->tip, dir0);
			VectorSubtract(saber_trail->dualbase, saber_trail->base, dir1);
			VectorSubtract(saber_trail->dualtip, saber_trail->dualbase, dir2);

			dirlen0 = VectorLength(dir0);
			dirlen1 = VectorLength(dir1);
			dirlen2 = VectorLength(dir2);

			if (saber_moveData[cent->currentState.saber_move].trailLength == 0)
			{
				dirlen0 *= 0.5;
				dirlen1 *= 0.3;
			}
			else
			{
				dirlen0 *= 1.0;
				dirlen1 *= 0.5;
			}

			lagscale = cg.time - saber_trail->lastTime;
			lagscale = 1 - lagscale * 3 / 200;

			if (lagscale < 0.1)
				lagscale = 0.1;

			VectorNormalize(dir0);
			VectorNormalize(dir1);

			VectorMA(saber_trail->tip, dirlen0 * lagscale, dir0, saber_trail->dualtip);
			VectorMA(saber_trail->base, dirlen1 * lagscale, dir1, saber_trail->dualbase);
			VectorSubtract(saber_trail->dualtip, saber_trail->dualbase, dir1);
			VectorNormalize(dir1);

			VectorMA(saber_trail->dualbase, dirlen2, dir1, saber_trail->dualtip);

			saber_trail->lastTime = cg.time;
		}

		if (!dont_draw)
		{
			switch (scolor)
			{
			case SABER_RED:
				VectorSet(rgb1, 255.0f, 0.0f, 0.0f);
				break;
			case SABER_ORANGE:
				VectorSet(rgb1, 255.0f, 64.0f, 0.0f);
				break;
			case SABER_YELLOW:
				VectorSet(rgb1, 255.0f, 255.0f, 0.0f);
				break;
			case SABER_GREEN:
				VectorSet(rgb1, 0.0f, 255.0f, 0.0f);
				break;
			case SABER_BLUE:
				VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
				break;
			case SABER_PURPLE:
				VectorSet(rgb1, 220.0f, 0.0f, 255.0f);
				break;
			case SABER_LIME:
				VectorSet(rgb1, 0.0f, 255.0f, 0.0f);
				break;
			case SABER_RGB:
			{
				int cnum = cent->currentState.clientNum;
				if (cnum < MAX_CLIENTS)
				{
					clientInfo_t* ci = &cgs.clientinfo[cnum];

					if (saberNum == 0)
						VectorCopy(ci->rgb1, rgb1);
					else
						VectorCopy(ci->rgb2, rgb1);
				}
				else
					VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
			}
			break;
			case SABER_PIMP:
			{
				int cnum = cent->currentState.clientNum;
				if (cnum < MAX_CLIENTS)
				{
					clientInfo_t* ci = &cgs.clientinfo[cnum];
					RGB_AdjustPimpSaberColor(ci, rgb1, saberNum);
				}
				else
					VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
			}
			break;
			case SABER_SCRIPTED:
			{
				int cnum = cent->currentState.clientNum;
				if (cnum < MAX_CLIENTS)
				{
					clientInfo_t* ci = &cgs.clientinfo[cnum];
					rgb_adjust_scipted_saber_color(ci, rgb1, saberNum);
				}
				else
					VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
			}
			break;
			case SABER_BLACK:
				VectorSet(rgb1, 1.0f, 1.0f, 1.0f);
				break;
			case SABER_WHITE:
				VectorSet(rgb1, 1.0f, 1.0f, 1.0f);
				break;
			default:
				VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
				break;
			}

			VectorCopy(saber_trail->base, fx.mVerts[0].origin);
			VectorCopy(saber_trail->tip, fx.mVerts[1].origin);
			VectorCopy(saber_trail->dualtip, fx.mVerts[2].origin);
			VectorCopy(saber_trail->dualbase, fx.mVerts[3].origin);
		}
	}

JustDoIt:

	if (dont_draw)
	{
		return;
	}

	if (client->saber[saberNum].saberFlags2 & SFL2_NO_BLADE || client->saber[saberNum].type == SABER_SITH_SWORD)
	{
		//don't actually draw the blade at all
		if (client->saber[saberNum].numBlades < 3
			&& !(client->saber[saberNum].saberFlags2 & SFL2_NO_DLIGHT))
		{
			//hmm, but still add the dlight
			CG_DoSaberLight(&client->saber[saberNum], cent->currentState.clientNum, saberNum);
		}
		return;
	}

	if (cg_SFXSabers.integer < 1)
	{
		// Draw the Raven blade.
		if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE ||
			client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
			SABER_ELECTROSTAFF)
		{
			CG_DoSaberUnstable(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
				client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
				client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
					SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
		}
		else if (cent->currentState.powerups & 1 << PW_CLOAKED)
		{
			CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
				client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
				client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
					SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
		}
		else
		{
			CG_DoSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
				client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
				client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
					SFL2_NO_DLIGHT),
				cent->currentState.clientNum, saberNum);
		}
	}
	else
	{
		switch (cg_SFXSabers.integer)
		{
		case 1:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 2:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 3:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoEp1Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 4:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoEp2Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 5:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 6:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoOTSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 7:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		case 8:
			if (cent->currentState.botclass == BCLASS_UNSTABLESABER || client->saber[saberNum].type == SABER_UNSTABLE
				||
				client->saber[saberNum].type == SABER_STAFF_UNSTABLE || client->saber[saberNum].type ==
				SABER_ELECTROSTAFF)
			{
				CG_DoTFASaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_THIN || client->saber[saberNum].type == SABER_STAFF_THIN)
			{
				CG_DoEp3Saber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_SFX || client->saber[saberNum].type == SABER_STAFF_SFX)
			{
				CG_DoSFXSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_CUSTOMSFX)
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				CG_DoCloakedSaber(org, axis[0], saber_len, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else if (client->saber[saberNum].type == SABER_GRIE || client->saber[saberNum].type == SABER_GRIE4)
			{
				CG_DoBattlefrontSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin,
					fx.mVerts[3].origin, client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			else
			{
				CG_DoCustomSaber(fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin,
					client->saber[saberNum].blade[blade_num].lengthMax,
					client->saber[saberNum].blade[blade_num].radius, scolor, renderfx,
					client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2 &
						SFL2_NO_DLIGHT), cent->currentState.clientNum, saberNum);
			}
			break;
		default:;
		}

		if (cg.time > saber_trail->inAction)
		{
			vec3_t draw_dir;
			float draw_len;
			saber_trail->inAction = cg.time;

			//Just tweaking this a little, cuz it looks funny when you turn around slowly
			VectorSubtract(fx.mVerts[2].origin, fx.mVerts[1].origin, draw_dir);
			draw_len = VectorNormalize(draw_dir);

			if (draw_len > 2)
			{
				fx.mShader = cgs.media.sfxSaberTrailShader;
				fx.mKillTime = 0;
				fx.mSetFlags = FX_USE_ALPHA;

				// New muzzle
				VectorCopy(rgb1, fx.mVerts[0].rgb);
				fx.mVerts[0].alpha = 255.0f;

				fx.mVerts[0].ST[0] = 0.0f;
				fx.mVerts[0].ST[1] = 4.0f;
				fx.mVerts[0].destST[0] = 4.0f;
				fx.mVerts[0].destST[1] = 4.0f;

				// new tip
				VectorCopy(rgb1, fx.mVerts[1].rgb);
				fx.mVerts[1].alpha = 255.0f;

				fx.mVerts[1].ST[0] = 0.0f;
				fx.mVerts[1].ST[1] = 0.0f;
				fx.mVerts[1].destST[0] = 4.0f;
				fx.mVerts[1].destST[1] = 0.0f;

				// old tip
				VectorCopy(rgb1, fx.mVerts[2].rgb);
				fx.mVerts[2].alpha = 255.0f;

				fx.mVerts[2].ST[0] = 4.0f;
				fx.mVerts[2].ST[1] = 0.0f;
				fx.mVerts[2].destST[0] = 4.0f;
				fx.mVerts[2].destST[1] = 0.0f;

				// old muzzle
				VectorCopy(rgb1, fx.mVerts[3].rgb);
				fx.mVerts[3].alpha = 255.0f;

				fx.mVerts[3].ST[0] = 4.0f;
				fx.mVerts[3].ST[1] = 4.0f;
				fx.mVerts[3].destST[0] = 4.0f;
				fx.mVerts[3].destST[1] = 4.0f;

				trap->FX_AddPrimitive(&fx);
			}
		}
	}
}

int CG_IsMindTricked(const int trick_index1, const int trick_index2, const int trick_index3, const int trick_index4,
	const int client)
{
	int check_in;
	int sub = 0;

	if (cg_entities[client].currentState.forcePowersActive & 1 << FP_SEE)
	{
		return 0;
	}

	if (client > 47)
	{
		check_in = trick_index4;
		sub = 48;
	}
	else if (client > 31)
	{
		check_in = trick_index3;
		sub = 32;
	}
	else if (client > 15)
	{
		check_in = trick_index2;
		sub = 16;
	}
	else
	{
		check_in = trick_index1;
	}

	if (check_in & (1 << (client - sub)))
	{
		return 1;
	}

	return 0;
}

#define SPEED_TRAIL_DISTANCE 6

void CG_DrawPlayerSphere(const centity_t* cent, vec3_t origin, const float scale, const int shader)
{
	refEntity_t ent;
	vec3_t ang;
	vec3_t view_dir;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset(&ent, 0, sizeof ent);

	VectorCopy(origin, ent.origin);
	ent.origin[2] += 9.0;

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	const float v_len = VectorLength(ent.axis[0]);
	if (v_len <= 0.1f)
	{
		// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(ent.axis[0], view_dir);
	VectorInverse(view_dir);
	VectorNormalize(view_dir);

	vectoangles(ent.axis[0], ang);
	ang[ROLL] += 180.0f;
	ang[PITCH] += 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);

	ent.nonNormalizedAxes = qtrue;

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;

	trap->R_AddRefEntityToScene(&ent);

	if (!cg.renderingThirdPerson && cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		//don't do the rest then
		return;
	}
	if (!cg_renderToTextureFX.integer)
	{
		return;
	}

	ang[PITCH] -= 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale * 0.5f, ent.axis[0]);
	VectorScale(ent.axis[1], scale * 0.5f, ent.axis[1]);
	VectorScale(ent.axis[2], scale * 0.5f, ent.axis[2]);

	ent.renderfx = RF_DISTORTION | RF_FORCE_ENT_ALPHA;
	if (shader == cgs.media.invulnerabilityShader)
	{
		//ok, ok, this is a little hacky. sorry!
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.ysalimariShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.endarkenmentShader)
	{
		ent.shaderRGBA[0] = 100;
		ent.shaderRGBA[1] = 0;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 20;
	}
	else if (shader == cgs.media.enlightenmentShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 20;
	}
	else if (shader == cgs.media.ysaliredShader)
	{
		//  added the red ysal shader
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 20;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 80;
	}
	else if (shader == cgs.media.ysaliblueShader)
	{
		// added the blue ysal shader
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 20;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 80;
	}
	else if (shader == cgs.media.galakShader)
	{
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else
	{
		//ysal red/blue, boon
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = 20;
	}

	ent.radius = 256;

	VectorMA(ent.origin, 40.0f, view_dir, ent.origin);

	ent.customShader = trap->R_RegisterShader("effects/refract_2");
	trap->R_AddRefEntityToScene(&ent);
}

void CG_AddLightningBeam(vec3_t start, vec3_t end)
{
	vec3_t dir, chaos,
		c1, c2,
		v1, v2;

	addbezierArgStruct_t b;

	VectorCopy(start, b.start);
	VectorCopy(end, b.end);

	VectorSubtract(b.end, b.start, dir);
	const float len = VectorNormalize(dir);

	// Get the base control points, we'll work from there
	VectorMA(b.start, 0.3333f * len, dir, c1);
	VectorMA(b.start, 0.6666f * len, dir, c2);

	// get some chaos values that really aren't very chaotic :)
	const float s1 = sin(cg.time * 0.005f) * 2.0f + Q_flrand(-1.0f, 1.0f) * 0.2f;
	const float s2 = sin(cg.time * 0.001f);
	const float s3 = sin(cg.time * 0.011f);

	VectorSet(chaos, len * 0.01f * s1,
		len * 0.02f * s2,
		len * 0.04f * (s1 + s2 + s3));

	VectorAdd(c1, chaos, c1);
	VectorScale(chaos, 4.0f, v1);

	VectorSet(chaos, -len * 0.02f * s3,
		len * 0.01f * (s1 * s2),
		-len * 0.02f * (s1 + s2 * s3));

	VectorAdd(c2, chaos, c2);
	VectorScale(chaos, 2.0f, v2);

	VectorSet(chaos, 1.0f, 1.0f, 1.0f);

	VectorCopy(c1, b.control1);
	VectorCopy(vec3_origin, b.control1Vel);
	VectorCopy(c2, b.control2);
	VectorCopy(vec3_origin, b.control2Vel);

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;

	/*
	VectorCopy(WHITE, b.sRGB);
	VectorCopy(WHITE, b.eRGB);
	*/

	b.sRGB[0] = 255;
	b.sRGB[1] = 255;
	b.sRGB[2] = 255;
	VectorCopy(b.sRGB, b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 50;
	b.shader = trap->R_RegisterShader("gfx/misc/electric2");
	b.flags = 0x00000001; //FX_ALPHA_LINEAR

	trap->FX_AddBezier(&b);
}

void CG_AddRandomLightning(vec3_t start, vec3_t end)
{
	vec3_t in_org, out_org;

	VectorCopy(start, in_org);
	VectorCopy(end, out_org);

	if (rand() & 1)
	{
		out_org[0] += Q_irand(0, 24);
		in_org[0] += Q_irand(0, 8);
	}
	else
	{
		out_org[0] -= Q_irand(0, 24);
		in_org[0] -= Q_irand(0, 8);
	}

	if (rand() & 1)
	{
		out_org[1] += Q_irand(0, 24);
		in_org[1] += Q_irand(0, 8);
	}
	else
	{
		out_org[1] -= Q_irand(0, 24);
		in_org[1] -= Q_irand(0, 8);
	}

	if (rand() & 1)
	{
		out_org[2] += Q_irand(0, 50);
		in_org[2] += Q_irand(0, 40);
	}
	else
	{
		out_org[2] -= Q_irand(0, 64);
		in_org[2] -= Q_irand(0, 40);
	}

	CG_AddLightningBeam(in_org, out_org);
}

extern char* forceHolocronModels[];

qboolean CG_ThereIsAMaster(void)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		const centity_t* cent = &cg_entities[i];

		if (cent && cent->currentState.isJediMaster)
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

#if 0
void CG_DrawNoForceSphere(centity_t* cent, vec3_t origin, float scale, int shader)
{
	refEntity_t ent;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset(&ent, 0, sizeof(ent));

	VectorCopy(origin, ent.origin);
	ent.origin[2] += 9.0;

	VectorSubtract(cg.refdef.vieworg, ent.origin, ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(cg.refdef.viewaxis[2], ent.axis[2]);
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.shaderRGBA[3] = (cent->currentState.genericenemyindex - cg.time) / 8;
	ent.renderfx |= RF_RGB_TINT;
	if (ent.shaderRGBA[3] > 200)
	{
		ent.shaderRGBA[3] = 200;
	}
	if (ent.shaderRGBA[3] < 1)
	{
		ent.shaderRGBA[3] = 1;
	}

	ent.shaderRGBA[2] = 0;
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[3];

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;

	trap->R_AddRefEntityToScene(&ent);
}
#endif

//Checks to see if the model string has a * appended with a custom skin name after.
//If so, it terminates the model string correctly, parses the skin name out, and returns
//the handle of the registered skin.
int CG_HandleAppendedSkin(const char* model_name)
{
	qhandle_t skin_id = 0;

	//see if it has a skin name
	char* p = Q_strrchr(model_name, '*');

	if (p)
	{
		int i = 0;
		char skin_name[MAX_QPATH];
		//found a *, we should have a model name before it and a skin name after it.
		*p = 0; //terminate the modelName string at this point, then go ahead and parse to the next 0 for the skin.
		p++;

		while (p && *p)
		{
			skin_name[i] = *p;
			i++;
			p++;
		}
		skin_name[i] = 0;

		if (skin_name[0])
		{
			//got it, register the skin under the model path.
			char base_folder[MAX_QPATH];

			strcpy(base_folder, model_name);
			p = Q_strrchr(base_folder, '/'); //go back to the first /, should be the path point

			if (p)
			{
				//got it.. terminate at the slash and register.
				char* use_skin_name;

				*p = 0;

				if (strchr(skin_name, '|'))
				{
					//three part skin
					use_skin_name = va("%s/|%s", base_folder, skin_name);
				}
				else
				{
					use_skin_name = va("%s/model_%s.skin", base_folder, skin_name);
				}

				skin_id = trap->R_RegisterSkin(use_skin_name);
			}
		}
	}

	return skin_id;
}

//Create a temporary ghoul2 instance and get the gla name so we can try loading animation data and sounds.
void BG_GetVehicleModelName(char* modelName, const char* vehicleName, size_t len);
void BG_GetVehicleSkinName(char* skinname, int len);

void CG_CacheG2AnimInfo(const char* model_name)
{
	void* g2 = NULL;
	char use_model[MAX_QPATH] = { 0 };
	char use_skin[MAX_QPATH] = { 0 };

	Q_strncpyz(use_model, model_name, sizeof use_model);
	Q_strncpyz(use_skin, model_name, sizeof use_skin);

	if (model_name[0] == '$')
	{
		//it's a vehicle name actually, let's precache the whole vehicle
		BG_GetVehicleModelName(use_model, use_model, sizeof use_model);
		BG_GetVehicleSkinName(use_skin, sizeof use_skin);
		if (use_skin[0])
		{
			//use a custom skin
			trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", use_model, use_skin));
		}
		else
		{
			trap->R_RegisterSkin(va("models/players/%s/model_default.skin", use_model));
		}
		Q_strncpyz(use_model, va("models/players/%s/model.glm", use_model), sizeof use_model);
	}

	trap->G2API_InitGhoul2Model(&g2, use_model, 0, 0, 0, 0, 0);

	if (g2)
	{
		char gla_name[MAX_QPATH];
		char original_model_name[MAX_QPATH];

		int anim_index = -1;

		gla_name[0] = 0;
		trap->G2API_GetGLAName(g2, 0, gla_name);

		Q_strncpyz(original_model_name, use_model, sizeof original_model_name);

		char* slash = Q_strrchr(gla_name, '/');
		if (slash)
		{
			strcpy(slash, "/animation.cfg");

			anim_index = bg_parse_animation_file(gla_name, NULL, qfalse);
		}

		if (anim_index != -1)
		{
			slash = Q_strrchr(original_model_name, '/');
			if (slash)
			{
				slash++;
				*slash = 0;
			}

			BG_ParseAnimationEvtFile(original_model_name, anim_index, bgNumAnimEvents);
		}

		//Now free the temp instance
		trap->G2API_CleanGhoul2Models(&g2);
	}
}

static void CG_RegisterVehicleAssets(Vehicle_t* p_veh)
{
	/*
	if ( p_veh->m_pVehicleInfo->exhaustFX )
	{
		p_veh->m_pVehicleInfo->iExhaustFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->exhaustFX );
	}
	if ( p_veh->m_pVehicleInfo->trailFX )
	{
		p_veh->m_pVehicleInfo->iTrailFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->trailFX );
	}
	if ( p_veh->m_pVehicleInfo->impactFX )
	{
		p_veh->m_pVehicleInfo->iImpactFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->impactFX );
	}
	if ( p_veh->m_pVehicleInfo->explodeFX )
	{
		p_veh->m_pVehicleInfo->iExplodeFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->explodeFX );
	}
	if ( p_veh->m_pVehicleInfo->wakeFX )
	{
		p_veh->m_pVehicleInfo->iWakeFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->wakeFX );
	}

	if ( p_veh->m_pVehicleInfo->dmgFX )
	{
		p_veh->m_pVehicleInfo->iDmgFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->dmgFX );
	}
	if ( p_veh->m_pVehicleInfo->wpn1FX )
	{
		p_veh->m_pVehicleInfo->iWpn1FX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->wpn1FX );
	}
	if ( p_veh->m_pVehicleInfo->wpn2FX )
	{
		p_veh->m_pVehicleInfo->iWpn2FX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->wpn2FX );
	}
	if ( p_veh->m_pVehicleInfo->wpn1FireFX )
	{
		p_veh->m_pVehicleInfo->iWpn1FireFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->wpn1FireFX );
	}
	if ( p_veh->m_pVehicleInfo->wpn2FireFX )
	{
		p_veh->m_pVehicleInfo->iWpn2FireFX = trap->FX_RegisterEffect( p_veh->m_pVehicleInfo->wpn2FireFX );
	}
	*/
}

extern void CG_HandleNPCSounds(const centity_t* cent);

extern void G_CreateAnimalNPC(Vehicle_t** p_veh, const char* strAnimalType);
extern void G_CreateSpeederNPC(Vehicle_t** p_veh, const char* strType);
extern void G_CreateWalkerNPC(Vehicle_t** p_veh, const char* strAnimalType);
extern void G_CreateFighterNPC(Vehicle_t** p_veh, const char* strType);

extern playerState_t* cgSendPS[MAX_GENTITIES];

void CG_G2AnimEntModelLoad(centity_t* cent)
{
	const char* c_model_name = CG_ConfigString(CS_MODELS + cent->currentState.modelIndex);

	if (!cent->npcClient)
	{
		//have not init'd client yet
		return;
	}

	if (c_model_name && c_model_name[0])
	{
		char model_name[MAX_QPATH];
		int skin_id;

		strcpy(model_name, c_model_name);

		if (cent->currentState.NPC_class == CLASS_VEHICLE && model_name[0] == '$')
		{
			//vehicles pass their veh names over as model names, then we get the model name from the veh type
			//create a vehicle object clientside for this type
			const char* vehType = &model_name[1];
			const int iVehIndex = BG_VehicleGetIndex(vehType);

			switch (g_vehicleInfo[iVehIndex].type)
			{
			case VH_ANIMAL:
				// Create the animal (making sure all it's data is initialized).
				G_CreateAnimalNPC(&cent->m_pVehicle, vehType);
				break;
			case VH_SPEEDER:
				// Create the speeder (making sure all it's data is initialized).
				G_CreateSpeederNPC(&cent->m_pVehicle, vehType);
				break;
			case VH_FIGHTER:
				// Create the fighter (making sure all it's data is initialized).
				G_CreateFighterNPC(&cent->m_pVehicle, vehType);
				break;
			case VH_WALKER:
				// Create the walker (making sure all it's data is initialized).
				G_CreateWalkerNPC(&cent->m_pVehicle, vehType);
				break;

			default:
				assert(!"vehicle with an unknown type - couldn't create vehicle_t");
				break;
			}

			//set up my happy prediction hack
			cent->m_pVehicle->m_vOrientation = &cgSendPS[cent->currentState.number]->vehOrientation[0];

			cent->m_pVehicle->m_pParentEntity = (bgEntity_t*)cent;

			//attach the handles for fx cgame-side
			CG_RegisterVehicleAssets(cent->m_pVehicle);

			BG_GetVehicleModelName(model_name, model_name, sizeof model_name);
			if (cent->m_pVehicle->m_pVehicleInfo->skin &&
				cent->m_pVehicle->m_pVehicleInfo->skin[0])
			{
				//use a custom skin
				skin_id = trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", model_name,
					cent->m_pVehicle->m_pVehicleInfo->skin));
			}
			else
			{
				skin_id = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", model_name));
			}
			strcpy(model_name, va("models/players/%s/model.glm", model_name));

			//this sound is *only* used for vehicles now
			cgs.media.noAmmoSound = trap->S_RegisterSound("sound/weapons/noammo.wav");
		}
		else
		{
			skin_id = CG_HandleAppendedSkin(model_name); //get the skin if there is one.
		}

		if (cent->ghoul2)
		{
			//clean it first!
			trap->G2API_CleanGhoul2Models(&cent->ghoul2);
		}

		trap->G2API_InitGhoul2Model(&cent->ghoul2, model_name, 0, skin_id, 0, 0, 0);

		if (cent->ghoul2)
		{
			char* slash;
			char gla_name[MAX_QPATH];
			char original_model_name[MAX_QPATH];
			char* saber;

			if (cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{
				//do special vehicle stuff
				char strTemp[128];

				// Setup the default first bolt
				int i;

				// Setup the droid unit.
				cent->m_pVehicle->m_iDroidUnitTag = trap->G2API_AddBolt(cent->ghoul2, 0, "*droidunit");

				// Setup the Exhausts.
				for (i = 0; i < MAX_VEHICLE_EXHAUSTS; i++)
				{
					Com_sprintf(strTemp, 128, "*exhaust%i", i + 1);
					cent->m_pVehicle->m_iExhaustTag[i] = trap->G2API_AddBolt(cent->ghoul2, 0, strTemp);
				}

				// Setup the Muzzles.
				for (i = 0; i < MAX_VEHICLE_MUZZLES; i++)
				{
					Com_sprintf(strTemp, 128, "*muzzle%i", i + 1);
					cent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt(cent->ghoul2, 0, strTemp);
					if (cent->m_pVehicle->m_iMuzzleTag[i] == -1)
					{
						//ergh, try *flash?
						Com_sprintf(strTemp, 128, "*flash%i", i + 1);
						cent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt(cent->ghoul2, 0, strTemp);
					}
				}

				// Setup the Turrets.
				for (i = 0; i < MAX_VEHICLE_TURRETS; i++)
				{
					if (cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag)
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = trap->G2API_AddBolt(
							cent->ghoul2, 0, cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag);
					}
					else
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = -1;
					}
				}
			}

			if (cent->currentState.npcSaber1)
			{
				saber = (char*)CG_ConfigString(CS_MODELS + cent->currentState.npcSaber1);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 0, saber);
				}
			}
			if (cent->currentState.npcSaber2)
			{
				saber = (char*)CG_ConfigString(CS_MODELS + cent->currentState.npcSaber2);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 1, saber);
				}
			}

			// If this is a not vehicle, give it saber stuff...
			if (cent->currentState.NPC_class != CLASS_VEHICLE)
			{
				int j = 0;
				while (j < MAX_SABERS)
				{
					if (cent->npcClient->saber[j].model[0])
					{
						if (cent->npcClient->ghoul2Weapons[j])
						{
							//free the old instance(s)
							trap->G2API_CleanGhoul2Models(&cent->npcClient->ghoul2Weapons[j]);
							cent->npcClient->ghoul2Weapons[j] = 0;
						}
						//racc - delete the current ghoul2holsterWeapons so we can load new ones
						if (cent->npcClient->ghoul2HolsterWeapons[j])
						{
							//free the old instance(s)
							trap->G2API_CleanGhoul2Models(&cent->npcClient->ghoul2HolsterWeapons[j]);
							cent->npcClient->ghoul2HolsterWeapons[j] = 0;
						}

						CG_InitG2SaberData(j, cent->npcClient);
					}
					j++;
				}
			}

			trap->G2API_SetSkin(cent->ghoul2, 0, skin_id, skin_id);

			cent->localAnimIndex = -1;

			gla_name[0] = 0;
			trap->G2API_GetGLAName(cent->ghoul2, 0, gla_name);

			strcpy(original_model_name, model_name);

			if (gla_name[0] &&
				!strstr(gla_name, "players/_humanoid/"))
			{
				//it doesn't use humanoid anims.
				slash = Q_strrchr(gla_name, '/');
				if (slash)
				{
					strcpy(slash, "/animation.cfg");

					cent->localAnimIndex = bg_parse_animation_file(gla_name, NULL, qfalse);
				}
			}
			else
			{
				//humanoid index.
				trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
				trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap->G2API_AddBolt(cent->ghoul2, 0, "*chestg");

				//claw bolts
				trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand_cap_r_arm");
				trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand_cap_l_arm");

				if (strstr(gla_name, "players/rockettrooper/"))
				{
					cent->localAnimIndex = 1;
				}
				else
				{
					cent->localAnimIndex = 0;
				}

				if (trap->G2API_AddBolt(cent->ghoul2, 0, "*head_top") == -1)
				{
					trap->G2API_AddBolt(cent->ghoul2, 0, "ceyebrow");
				}
				trap->G2API_AddBolt(cent->ghoul2, 0, "Motion");
			}

			// If this is a not vehicle...
			if (cent->currentState.NPC_class != CLASS_VEHICLE)
			{
				if (trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar") == -1)
				{
					//check now to see if we have this bone for setting anims and such
					cent->noLumbar = qtrue;
				}

				if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
				{
					//check now to see if we have this bone for setting anims and such
					cent->noFace = qtrue;
				}
			}
			else
			{
				cent->noLumbar = qtrue;
				cent->noFace = qtrue;
			}

			if (cent->localAnimIndex != -1)
			{
				slash = Q_strrchr(original_model_name, '/');
				if (slash)
				{
					slash++;
					*slash = 0;
				}

				cent->eventAnimIndex = BG_ParseAnimationEvtFile(original_model_name, cent->localAnimIndex,
					bgNumAnimEvents);
			}
		}
	}

	trap->S_Shutup(qtrue);
	CG_HandleNPCSounds(cent); //handle sound loading here as well.
	trap->S_Shutup(qfalse);
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceDebris(centity_t* cent, const int surfNum, const int fx_id, const qboolean throw_part)
{
	int lost_part_fx = 0;
	int b = -1;
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char* surfName = NULL;

	if (surfNum > 0)
	{
		surfName = bgToggleableSurfaces[surfNum];
	}

	if (!cent->ghoul2)
	{
		//oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if (bgToggleableSurfaceDebris[surfNum] == 3)
	{
		//right wing flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if (throw_part
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo)
		{
			lost_part_fx = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 4)
	{
		//left wing flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if (throw_part
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo)
		{
			lost_part_fx = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 5)
	{
		//right wing flame 2
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if (throw_part
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo)
		{
			lost_part_fx = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 6)
	{
		//left wing flame 2
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if (throw_part
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo)
		{
			lost_part_fx = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 7)
	{
		//nose flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*nosedamage");
		if (cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo)
		{
			lost_part_fx = cent->m_pVehicle->m_pVehicleInfo->iNoseFX;
		}
	}
	else if (surfName)
	{
		b = trap->G2API_AddBolt(cent->ghoul2, 0, surfName);
	}

	if (b == -1 || surfNum == -1)
	{
		//couldn't find this surface apparently, so play on origin?
		VectorCopy(cent->lerpOrigin, v);
		AngleVectors(cent->lerpAngles, d, NULL, NULL);
		VectorNormalize(d);
	}
	else
	{
		//now let's get the position and direction of this surface and make a big explosion
		trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
			cgs.game_models, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
		BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);
	}

	trap->FX_PlayEffectID(fx_id, v, d, -1, -1, qfalse);
	if (throw_part && lost_part_fx)
	{
		//throw off a ship part, too
		vec3_t fxFwd;
		AngleVectors(cent->lerpAngles, fxFwd, NULL, NULL);
		trap->FX_PlayEffectID(lost_part_fx, v, fxFwd, -1, -1, qfalse);
	}
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceSmoke(centity_t* cent, const int ship_surf, const int fx_id)
{
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char* surfName;

	if (!cent->ghoul2)
	{
		//oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if (ship_surf == SHIPSURF_FRONT)
	{
		//front flame/smoke
		surfName = "*nosedamage";
	}
	else if (ship_surf == SHIPSURF_BACK)
	{
		//back flame/smoke
		surfName = "*exhaust1"; //FIXME: random?  Some point in-between?
	}
	else if (ship_surf == SHIPSURF_RIGHT)
	{
		//right wing flame/smoke
		surfName = "*r_wingdamage";
	}
	else if (ship_surf == SHIPSURF_LEFT)
	{
		//left wing flame/smoke
		surfName = "*l_wingdamage";
	}
	else
	{
		//unknown surf!
		return;
	}
	const int b = trap->G2API_AddBolt(cent->ghoul2, 0, surfName);
	if (b == -1)
	{
		//couldn't find this surface apparently
		return;
	}

	//now let's get the position and direction of this surface and make a big explosion
	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
		cgs.game_models, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);

	trap->FX_PlayEffectID(fx_id, v, d, -1, -1, qfalse);
}

#define SMOOTH_G2ANIM_LERPANGLES

qboolean CG_VehicleShouldDrawShields(const centity_t* veh_cent)
{
	if (veh_cent->damageTime > cg.time //ship shields currently taking damage
		&& veh_cent->currentState.NPC_class == CLASS_VEHICLE
		&& veh_cent->m_pVehicle
		&& veh_cent->m_pVehicle->m_pVehicleInfo)
	{
		return qtrue;
	}
	return qfalse;
}

/*
extern	vmCvar_t		cg_showVehBounds;
extern void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vec3_t origin, vec3_t mins, vec3_t maxs,
										int clientNum, int tracemask,
										void (*localTrace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int pass_entity_num, int content_mask)); // bg_pmove.c
*/
qboolean CG_VehicleAttachDroidUnit(centity_t* droid_cent)
{
	if (droid_cent
		&& droid_cent->currentState.owner
		&& droid_cent->currentState.clientNum >= MAX_CLIENTS)
	{
		//the only NPCs that can ride a vehicle are droids...???
		centity_t* veh_cent = &cg_entities[droid_cent->currentState.owner];
		if (veh_cent
			&& veh_cent->m_pVehicle
			&& veh_cent->ghoul2
			&& veh_cent->m_pVehicle->m_iDroidUnitTag != -1)
		{
			mdxaBone_t boltMatrix;
			vec3_t fwd, rt, temp_ang;

			trap->G2API_GetBoltMatrix(veh_cent->ghoul2, 0, veh_cent->m_pVehicle->m_iDroidUnitTag, &boltMatrix,
				veh_cent->lerpAngles, veh_cent->lerpOrigin, cg.time,
				cgs.game_models, veh_cent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droid_cent->lerpOrigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, fwd); //WTF???
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, rt); //WTF???
			vectoangles(fwd, droid_cent->lerpAngles);
			vectoangles(rt, temp_ang);
			droid_cent->lerpAngles[ROLL] = temp_ang[PITCH];

			return qtrue;
		}
	}
	return qfalse;
}

void CG_G2Animated(centity_t* cent)
{
#ifdef SMOOTH_G2ANIM_LERPANGLES
	const float angSmoothFactor = 0.7f;
#endif

	if (!cent->ghoul2)
	{
		//Initialize this g2 anim ent, then return (will start rendering next frame)
		CG_G2AnimEntModelLoad(cent);
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
		return;
	}

	if (cent->npcLocalSurfOff != cent->currentState.surfacesOff ||
		cent->npcLocalSurfOn != cent->currentState.surfacesOn)
	{
		//looks like it's time for an update.
		int i = 0;

		while (i < BG_NUM_TOGGLEABLE_SURFACES && bgToggleableSurfaces[i])
		{
			if (!(cent->npcLocalSurfOff & 1 << i) &&
				cent->currentState.surfacesOff & 1 << i)
			{
				//it wasn't off before but it's off now, so reflect this change in the g2 instance.
				if (bgToggleableSurfaceDebris[i] > 0)
				{
					//make some local debris of this thing?
					//FIXME: throw off the proper model effect, too
					CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestDestroyed, qtrue);
				}

				trap->G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_OFF);
			}

			if (!(cent->npcLocalSurfOn & 1 << i) &&
				cent->currentState.surfacesOn & 1 << i)
			{
				//same as above, but on instead of off.
				trap->G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_ON);
			}

			i++;
		}

		cent->npcLocalSurfOff = cent->currentState.surfacesOff;
		cent->npcLocalSurfOn = cent->currentState.surfacesOn;
	}

	/*
	if (cent->currentState.weapon &&
		!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1) &&
		!(cent->currentState.eFlags & EF_DEAD))
	{ //if the server says we have a weapon and we haven't copied one onto ourselves yet, then do so.
		trap->G2API_CopySpecificGhoul2Model(g2WeaponInstances[cent->currentState.weapon], 0, cent->ghoul2, 1);

		if (cent->currentState.weapon == WP_SABER)
		{
			trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
		}
	}
	*/

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{
		//he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if ((cent->currentState.eFlags & EF_DEAD || cent->currentState.eFlags & EF_RAG) && !cent->localAnimIndex)
	{
		vec3_t forced_angles;

		VectorClear(forced_angles);
		forced_angles[YAW] = cent->lerpAngles[YAW];

		CG_RagDoll(cent, forced_angles);
	}

#ifdef SMOOTH_G2ANIM_LERPANGLES
	if (cent->lerpAngles[YAW] > 0 && cent->smoothYaw < 0 ||
		cent->lerpAngles[YAW] < 0 && cent->smoothYaw > 0)
	{
		//keep it from snapping around on the threshold
		cent->smoothYaw = -cent->smoothYaw;
	}
	cent->lerpAngles[YAW] = cent->smoothYaw + (cent->lerpAngles[YAW] - cent->smoothYaw) * angSmoothFactor;
	cent->smoothYaw = cent->lerpAngles[YAW];
#endif

	//now just render as a player
	CG_Player(cent);

	/*
	if ( cg_showVehBounds.integer )
	{//show vehicle bboxes
		if ( cent->currentState.clientNum >= MAX_CLIENTS
			&& cent->currentState.NPC_class == CLASS_VEHICLE
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->currentState.clientNum != cg.predictedVehicleState.clientNum )
		{//not the predicted vehicle
			vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
			vec3_t absmin, absmax;
			vec3_t bmins, bmaxs;
			float *old = cent->m_pVehicle->m_vOrientation;
			cent->m_pVehicle->m_vOrientation = &cent->lerpAngles[0];

			BG_VehicleAdjustBBoxForOrientation( cent->m_pVehicle, cent->lerpOrigin, bmins, bmaxs,
										cent->currentState.number, MASK_PLAYERSOLID, NULL );
			cent->m_pVehicle->m_vOrientation = old;

			VectorAdd( cent->lerpOrigin, bmins, absmin );
			VectorAdd( cent->lerpOrigin, bmaxs, absmax );
			CG_Cube( absmin, absmax, NPCDEBUG_RED, 0.25 );
		}
	}
	*/
}

//rww - here ends the majority of my g2animent stuff.

//Disabled for now, I'm too lazy to keep it working with all the stuff changing around.
#if 0
int cgFPLSState = 0;

void CG_ForceFPLSPlayerModel(centity_t* cent, clientInfo_t* ci)
{
	animation_t* anim;

	if (cg_fpls.integer && !cg.renderingThirdPerson)
	{
		int				skinHandle;

		skinHandle = trap->R_RegisterSkin("models/players/kyle/model_fpls2.skin");

		trap->G2API_CleanGhoul2Models(&(ci->ghoul2Model));

		ci->torsoSkin = skinHandle;
		trap->G2API_InitGhoul2Model(&ci->ghoul2Model, "models/players/kyle/model.glm", 0, ci->torsoSkin, 0, 0, 0);

		ci->bolt_rhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1, -1);
		trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time);
		trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, cg.time);

		ci->bolt_lhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

		//claw bolts
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

		ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
		if (ci->bolt_head == -1)
		{
			ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
		}

		ci->bolt_motion = trap->G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

		//We need a lower lumbar bolt for footsteps
		ci->bolt_llumbar = trap->G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");

		CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, ci->ghoul2Model);
	}
	else
	{
		CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->teamName, cent->currentState.number);
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[cent->currentState.legsAnim];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.legs.frame;
		}

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.legsAnim = 0;
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[cent->currentState.torsoAnim];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.torso.frame;
		}

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.torsoAnim = 0;
	}

	trap->G2API_CleanGhoul2Models(&(cent->ghoul2));
	trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);
}
#endif

//find the gender for a given modelname.  This is used to set up the NPCClient's gender correct
//to make jaden's dialog play in the correct gender
//based off the approprate sections of CG_LoadCISounds
int FindGender(const char* model_path, const centity_t* cent)
{
	fileHandle_t f;
	qboolean is_female = qfalse;
	char model_name[MAX_QPATH];

	if (cent->currentState.NPC_class != CLASS_PLAYER)
	{
		//we don't care about the gender of models that aren't going to be doing Jaden
		//voice scripting.
		return GENDER_MALE;
	}

	strcpy(model_name, model_path);

	char* temp = Q_strrchr(model_name, '/');

	if (!temp)
	{
		//bad player model
		Com_Printf("Bad modelPath passed to FindGender.\n");
		return GENDER_MALE;
	}

	*temp = '\0';

	const int f_len = trap->FS_Open(va("%s/sounds.cfg", model_name), &f, FS_READ);

	if (f)
	{
		char sound_path[MAX_QPATH];
		trap->FS_Read(sound_path, f_len, f);
		sound_path[f_len] = 0;

		int i = f_len;

		while (i >= 0 && sound_path[i] != '\n')
		{
			if (sound_path[i] == 'f')
			{
				is_female = qtrue;
				sound_path[i] = 0;
			}

			i--;
		}

		trap->FS_Close(f);
	}
	else
	{
		Com_Printf("FindGender Error: Couldnt find sounds.cfg for %s\n", model_name);
	}

	if (is_female)
	{
		return GENDER_FEMALE;
	}
	return GENDER_MALE;
}

//for allocating and freeing npc clientinfo structures.
//Remember to free this before game shutdown no matter what
//and don't stomp over it, as it is dynamic memory from the
//exe.
void CG_CreateNPCClient(clientInfo_t** ci)
{
	//trap->TrueMalloc((void **)ci, sizeof(clientInfo_t));
	*ci = (clientInfo_t*)BG_Alloc(sizeof(clientInfo_t));
}

void CG_DestroyNPCClient(clientInfo_t** ci)
{
	memset(*ci, 0, sizeof(clientInfo_t));
	//trap->TrueFree((void **)ci);
}

static void CG_ForceElectrocution(centity_t* cent, const vec3_t origin, vec3_t temp_angles, const qhandle_t shader,
	const qboolean always_do)
{
	// Undoing for now, at least this code should compile if I ( or anyone else ) decides to work on this effect
	qboolean found = qfalse;
	vec3_t fx_org, fx_org2, dir;
	vec3_t rgb;
	mdxaBone_t boltMatrix;
	trace_t tr;
	int bolt = -1;
	int iter = 0;
	int torso_bolt = -1;
	int elbow_l_bolt = -1;
	int elbow_r_bolt = -1;
	int hand_l_bolt = -1;
	int hand_r_bolt = -1;
	int foot_l_bolt = -1;
	int foot_r_bolt = -1;

	VectorSet(rgb, 1, 1, 1);

	if (cent->localAnimIndex <= 1)
	{
		//humanoid
		torso_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		elbow_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_arm_elbow");
		elbow_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_arm_elbow");
		hand_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand");
		hand_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
		foot_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot");
		foot_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot");
	}
	else if (cent->currentState.NPC_class == CLASS_PROTOCOL)
	{
		//any others that can use these bolts too?
		torso_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		elbow_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*bicep_lg");
		elbow_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*bicep_rg");
		hand_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*hand_l");
		hand_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*weapon");
		foot_l_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*foot_lg");
		foot_r_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*foot_rg");
	}

	// Pick a random start point
	while (bolt < 0)
	{
		int test;
		if (iter > 5)
		{
			test = iter - 5;
		}
		else
		{
			test = Q_irand(0, 6);
		}
		switch (test)
		{
		case 0:
			// Right Elbow
			bolt = elbow_r_bolt;
			break;
		case 1:
			// Left Hand
			bolt = hand_l_bolt;
			break;
		case 2:
			// Right hand
			bolt = hand_r_bolt;
			break;
		case 3:
			// Left Foot
			bolt = foot_l_bolt;
			break;
		case 4:
			// Right foot
			bolt = foot_r_bolt;
			break;
		case 5:
			// Torso
			bolt = torso_bolt;
			break;
		case 6:
		default:
			// Left Elbow
			bolt = elbow_l_bolt;
			break;
		}
		if (++iter == 20)
			break;
	}
	if (bolt >= 0)
	{
		found = trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt,
			&boltMatrix, temp_angles, origin, cg.time,
			cgs.game_models, cent->modelScale);
	}

	// Make sure that it's safe to even try and get these values out of the Matrix, otherwise the values could be garbage
	if (found)
	{
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, fx_org);
		if (Q_flrand(0.0f, 1.0f) > 0.5f)
		{
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, dir);
		}
		else
		{
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, dir);
		}

		// Add some fudge, makes us not normalized, but that isn't really important
		dir[0] += Q_flrand(-1.0f, 1.0f) * 0.4f;
		dir[1] += Q_flrand(-1.0f, 1.0f) * 0.4f;
		dir[2] += Q_flrand(-1.0f, 1.0f) * 0.4f;
	}
	else
	{
		// Just use the lerp Origin and a random direction
		VectorCopy(cent->lerpOrigin, fx_org);
		VectorSet(dir, Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f));
		// Not normalized, but who cares.
		switch (cent->currentState.NPC_class)
		{
		case CLASS_PROBE:
			fx_org[2] += 50;
			break;
		case CLASS_MARK1:
			fx_org[2] += 50;
			break;
		case CLASS_ATST:
			fx_org[2] += 120;
			break;
		default:
			break;
		}
	}

	VectorMA(fx_org, Q_flrand(0.0f, 1.0f) * 40 + 40, dir, fx_org2);

	CG_Trace(&tr, fx_org, NULL, NULL, fx_org2, -1, CONTENTS_SOLID);

	if (tr.fraction < 1.0f || Q_flrand(0.0f, 1.0f) > 0.94f || always_do)
	{
		addElectricityArgStruct_t p;

		VectorCopy(fx_org, p.start);
		VectorCopy(tr.endpos, p.end);
		p.size1 = 1.5f;
		p.size2 = 4.0f;
		p.sizeParm = 0.0f;
		p.alpha1 = 1.0f;
		p.alpha2 = 0.5f;
		p.alphaParm = 0.0f;
		VectorCopy(rgb, p.sRGB);
		VectorCopy(rgb, p.eRGB);
		p.rgbParm = 0.0f;
		p.chaos = 5.0f;
		p.killTime = Q_flrand(0.0f, 1.0f) * 50 + 100;
		p.shader = shader;
		p.flags = 0x00000001 | 0x00000100 | 0x02000000 | 0x04000000 | 0x01000000;

		trap->FX_AddElectricity(&p);

		//In other words:
		/*
		FX_AddElectricity( fxOrg, tr.endpos,
			1.5f, 4.0f, 0.0f,
			1.0f, 0.5f, 0.0f,
			rgb, rgb, 0.0f,
			5.5f, Q_flrand(0.0f, 1.0f) * 50 + 100, shader, FX_ALPHA_LINEAR | FX_SIZE_LINEAR | FX_BRANCH | FX_GROW | FX_TAPER );
		*/
	}
}

void* cg_g2JetpackInstance = NULL;

#define JETPACK_MODEL "models/weapons2/jetpack/model.glm"

void CG_InitJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		assert(!"Tried to init jetpack inst, already init'd");
		return;
	}

	trap->G2API_InitGhoul2Model(&cg_g2JetpackInstance, JETPACK_MODEL, 0, 0, 0, 0, 0);

	assert(cg_g2JetpackInstance);

	//Indicate which bolt on the player we will be attached to
	//In this case bolt 0 is rhand, 1 is lhand, and 2 is the bolt
	//for the jetpack (*chestg)
	trap->G2API_SetBoltInfo(cg_g2JetpackInstance, 0, 2);

	//Add the bolts jet effects will be played from
	trap->G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_ljet");
	trap->G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_rjet");
}

void CG_CleanJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		trap->G2API_CleanGhoul2Models(&cg_g2JetpackInstance);
		cg_g2JetpackInstance = NULL;
	}
}

#define RARMBIT			(1 << (G2_MODELPART_RARM-10))
#define RHANDBIT		(1 << (G2_MODELPART_RHAND-10))
#define WAISTBIT		(1 << (G2_MODELPART_WAIST-10))

#if 0
static void CG_VehicleHeatEffect(vec3_t org, centity_t* cent)
{
	refEntity_t ent;
	vec3_t ang;
	float scale;
	float vLen;
	float alpha;

	if (!cg_renderToTextureFX.integer)
	{
		return;
	}
	scale = 0.1f;

	alpha = 200.0f;

	memset(&ent, 0, sizeof(ent));

	VectorCopy(org, ent.origin);

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	vectoangles(ent.axis[0], ang);
	AnglesToAxis(ang, ent.axis);

	//radius must be a power of 2, and is the actual captured texture size
	ent.radius = 32;

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.cloakedShader;

	//make it partially transparent so it blends with the background
	ent.renderfx = (RF_DISTORTION | RF_FORCE_ENT_ALPHA);
	ent.shaderRGBA[0] = 255.0f;
	ent.shaderRGBA[1] = 255.0f;
	ent.shaderRGBA[2] = 255.0f;
	ent.shaderRGBA[3] = alpha;

	trap->R_AddRefEntityToScene(&ent);
}
#endif

static int lastFlyBySound[MAX_GENTITIES] = { 0 };
#define	FLYBYSOUNDTIME 2000
int cg_lastHyperSpaceEffectTime = 0;

static QINLINE void CG_VehicleEffects(centity_t* cent)
{
	if (cent->currentState.eType != ET_NPC ||
		cent->currentState.NPC_class != CLASS_VEHICLE ||
		!cent->m_pVehicle)
	{
		return;
	}

	Vehicle_t* p_veh_npc = cent->m_pVehicle;

	if (cent->currentState.clientNum == cg.predictedPlayerState.m_iVehicleNum //my vehicle
		&& cent->currentState.eFlags2 & EF2_HYPERSPACE) //hyperspacing
	{
		//in hyperspace!
		if (cg.predictedVehicleState.hyperSpaceTime
			&& cg.time - cg.predictedVehicleState.hyperSpaceTime < HYPERSPACE_TIME)
		{
			if (!cg_lastHyperSpaceEffectTime
				|| cg.time - cg_lastHyperSpaceEffectTime > HYPERSPACE_TIME + 500)
			{
				//can't be from the last time we were in hyperspace, so play the effect!
				trap->FX_PlayBoltedEffectID(cgs.effects.mHyperspaceStars, cent->lerpOrigin, cent->ghoul2, 0,
					cent->currentState.number, 0, 0, qtrue);
				cg_lastHyperSpaceEffectTime = cg.time;
			}
		}
	}

	//FLYBY sound
	if (cent->currentState.clientNum != cg.predictedPlayerState.m_iVehicleNum
		&& (p_veh_npc->m_pVehicleInfo->soundFlyBy || p_veh_npc->m_pVehicleInfo->soundFlyBy2))
	{
		//not my vehicle
		if (cent->currentState.speed && cg.predictedPlayerState.speed + cent->currentState.speed > 500)
		{
			//he's moving and between the two of us, we're moving fast
			vec3_t diff;
			VectorSubtract(cent->lerpOrigin, cg.predictedPlayerState.origin, diff);
			if (VectorLength(diff) < 2048)
			{
				//close
				vec3_t my_fwd, their_fwd;
				AngleVectors(cg.predictedPlayerState.viewangles, my_fwd, NULL, NULL);
				VectorScale(my_fwd, cg.predictedPlayerState.speed, my_fwd);
				AngleVectors(cent->lerpAngles, their_fwd, NULL, NULL);
				VectorScale(their_fwd, cent->currentState.speed, their_fwd);
				if (lastFlyBySound[cent->currentState.clientNum] + FLYBYSOUNDTIME < cg.time)
				{
					//okay to do a flyby sound on this vehicle
					if (DotProduct(my_fwd, their_fwd) < 500)
					{
						int fly_by_sound;
						if (p_veh_npc->m_pVehicleInfo->soundFlyBy && p_veh_npc->m_pVehicleInfo->soundFlyBy2)
						{
							fly_by_sound = Q_irand(0, 1)
								? p_veh_npc->m_pVehicleInfo->soundFlyBy
								: p_veh_npc->m_pVehicleInfo->soundFlyBy2;
						}
						else if (p_veh_npc->m_pVehicleInfo->soundFlyBy)
						{
							fly_by_sound = p_veh_npc->m_pVehicleInfo->soundFlyBy;
						}
						else //if ( pVehNPC->m_pVehicleInfo->soundFlyBy2 )
						{
							fly_by_sound = p_veh_npc->m_pVehicleInfo->soundFlyBy2;
						}
						trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN, fly_by_sound);
						lastFlyBySound[cent->currentState.clientNum] = cg.time;
					}
				}
			}
		}
	}

	if (!cent->currentState.speed //was stopped
		&& cent->nextState.speed > 0 //now moving forward
		&& cent->m_pVehicle->m_pVehicleInfo->soundEngineStart)
	{
		//engines rev up for the first time
		trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN,
			cent->m_pVehicle->m_pVehicleInfo->soundEngineStart);
	}
	// Animals don't exude any effects...
	if (p_veh_npc->m_pVehicleInfo->type != VH_ANIMAL)
	{
		qboolean did_fire_trail = qfalse;
		if (p_veh_npc->m_pVehicleInfo->surfDestruction && cent->ghoul2)
		{
			//see if anything has been blown off
			int i = 0;
			qboolean surf_dmg = qfalse;

			while (i < BG_NUM_TOGGLEABLE_SURFACES)
			{
				if (bgToggleableSurfaceDebris[i] > 1)
				{
					//this is decidedly a destroyable surface, let's check its status
					const int surf_test = trap->G2API_GetSurfaceRenderStatus(cent->ghoul2, 0, bgToggleableSurfaces[i]);

					if (surf_test != -1
						&& surf_test & TURN_OFF)
					{
						//it exists, but it's off...
						surf_dmg = qtrue;

						//create some flames
						CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestBurning, qfalse);
						did_fire_trail = qtrue;
					}
				}

				i++;
			}

			if (surf_dmg)
			{
				//if any surface are damaged, neglect exhaust etc effects (so we don't have exhaust trails coming out of invisible surfaces)
				return;
			}
		}

		if (!did_fire_trail && cent->currentState.eFlags & EF_DEAD)
		{
			//spiralling out of control anyway
			CG_CreateSurfaceDebris(cent, -1, cgs.effects.mShipDestBurning, qfalse);
		}

		if (p_veh_npc->m_iLastFXTime <= cg.time)
		{
			//until we attach it, we need to debounce this
			vec3_t fwd, rt, up;
			vec3_t flat;
			const float next_fx_delay = 50;
			VectorSet(flat, 0, cent->lerpAngles[1], cent->lerpAngles[2]);
			AngleVectors(flat, fwd, rt, up);
			if (cent->currentState.speed > 0)
			{
				//FIXME: only do this when accelerator is being pressed! (must have a driver?)
				vec3_t org;
				qboolean doExhaust;
				VectorMA(cent->lerpOrigin, -16, up, org);
				VectorMA(org, -42, fwd, org);
				// Play damage effects.
				//if ( pVehNPC->m_iArmor <= 75 )
				if (p_veh_npc->m_pVehicleInfo->iTrailFX)
				{
					//okay, do normal trail
					trap->FX_PlayEffectID(p_veh_npc->m_pVehicleInfo->iTrailFX, org, fwd, -1, -1, qfalse);
				}
				//=====================================================================
				//EXHAUST FX
				//=====================================================================
				//do exhaust
				if (cent->currentState.eFlags & EF_JETPACK_ACTIVE)
				{
					//cheap way of telling us the vehicle is in "turbo" mode
					doExhaust = p_veh_npc->m_pVehicleInfo->iTurboFX != 0;
				}
				else
				{
					doExhaust = p_veh_npc->m_pVehicleInfo->iExhaustFX != 0;
				}
				if (doExhaust && cent->ghoul2 && !(cent->currentState.powerups & 1 << PW_CLOAKED))
				{
					int fx;

					for (int i = 0; i < MAX_VEHICLE_EXHAUSTS; i++)
					{
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if (p_veh_npc->m_iExhaustTag[i] == -1)
						{
							break;
						}

						if (cent->currentState.brokenLimbs & 1 << SHIPSURF_DAMAGE_BACK_HEAVY)
						{
							//engine has taken heavy damage
							if (!Q_irand(0, 1))
							{
								//50% chance of not drawing this engine glow this frame
								continue;
							}
						}
						else if (cent->currentState.brokenLimbs & 1 << SHIPSURF_DAMAGE_BACK_LIGHT)
						{
							//engine has taken light damage
							if (!Q_irand(0, 4))
							{
								//20% chance of not drawing this engine glow this frame
								continue;
							}
						}

						if (cent->currentState.eFlags & EF_JETPACK_ACTIVE
							//cheap way of telling us the vehicle is in "turbo" mode
							&& p_veh_npc->m_pVehicleInfo->iTurboFX) //they have a valid turbo exhaust effect to play
						{
							fx = p_veh_npc->m_pVehicleInfo->iTurboFX;
						}
						else
						{
							//play the normal one
							fx = p_veh_npc->m_pVehicleInfo->iExhaustFX;
						}

						if (p_veh_npc->m_pVehicleInfo->type == VH_FIGHTER)
						{
							trap->FX_PlayBoltedEffectID(fx, cent->lerpOrigin, cent->ghoul2, p_veh_npc->m_iExhaustTag[i],
								cent->currentState.number, 0, 0, qtrue);
						}
						else
						{
							//fixme: bolt these too
							mdxaBone_t boltMatrix;
							vec3_t bolt_org, bolt_dir;

							trap->G2API_GetBoltMatrix(cent->ghoul2, 0, p_veh_npc->m_iExhaustTag[i], &boltMatrix, flat,
								cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);

							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_org);
							VectorCopy(fwd, bolt_dir); //fixme?

							trap->FX_PlayEffectID(fx, bolt_org, bolt_dir, -1, -1, qfalse);
						}
					}
				}
				//=====================================================================
				//WING TRAIL FX
				//=====================================================================
				//do trail
				//FIXME: not in space!!!
				if (p_veh_npc->m_pVehicleInfo->iTrailFX != 0 && cent->ghoul2)
				{
					mdxaBone_t boltMatrix;
					vec3_t get_bolt_angles;

					VectorCopy(cent->lerpAngles, get_bolt_angles);
					if (p_veh_npc->m_pVehicleInfo->type != VH_FIGHTER)
					{
						//only fighters use pitch/roll in refent axis
						get_bolt_angles[PITCH] = get_bolt_angles[ROLL] = 0.0f;
					}

					for (int i = 1; i < 5; i++)
					{
						vec3_t bolt_dir;
						vec3_t bolt_org;
						const int trail_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, va("*trail%d", i));
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if (trail_bolt == -1)
						{
							break;
						}

						trap->G2API_GetBoltMatrix(cent->ghoul2, 0, trail_bolt, &boltMatrix, get_bolt_angles,
							cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);

						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_org);
						VectorCopy(fwd, bolt_dir); //fixme?

						trap->FX_PlayEffectID(p_veh_npc->m_pVehicleInfo->iTrailFX, bolt_org, bolt_dir, -1, -1, qfalse);
					}
				}
			}
			//FIXME armor needs to be sent over network
			{
				if (cent->currentState.eFlags & EF_DEAD)
				{
					//just plain dead, use flames
					vec3_t up_1 = { 0, 0, 1 };
					vec3_t bolt_org;

					//if ( pVehNPC->m_iDriverTag == -1 )
					{
						//doh!  no tag
						VectorCopy(cent->lerpOrigin, bolt_org);
					}
					trap->FX_PlayEffectID(cgs.effects.mShipDestBurning, bolt_org, up_1, -1, -1, qfalse);
				}
			}
			if (cent->currentState.brokenLimbs)
			{
				if (!Q_irand(0, 5))
				{
					for (int i = SHIPSURF_FRONT; i <= SHIPSURF_LEFT; i++)
					{
						if ((cent->currentState.brokenLimbs & (1 << ((i - SHIPSURF_FRONT) +
							SHIPSURF_DAMAGE_FRONT_HEAVY))))
						{
							//heavy damage, do both effects
							if (p_veh_npc->m_pVehicleInfo->iInjureFX)
							{
								CG_CreateSurfaceSmoke(cent, i, p_veh_npc->m_pVehicleInfo->iInjureFX);
							}
							if (p_veh_npc->m_pVehicleInfo->iDmgFX)
							{
								CG_CreateSurfaceSmoke(cent, i, p_veh_npc->m_pVehicleInfo->iDmgFX);
							}
						}
						else if ((cent->currentState.brokenLimbs & (1 << ((i - SHIPSURF_FRONT) +
							SHIPSURF_DAMAGE_FRONT_LIGHT))))
						{
							//only light damage
							if (p_veh_npc->m_pVehicleInfo->iInjureFX)
							{
								CG_CreateSurfaceSmoke(cent, i, p_veh_npc->m_pVehicleInfo->iInjureFX);
							}
						}
					}
				}
			}
			p_veh_npc->m_iLastFXTime = cg.time + next_fx_delay;
		}
	}
}

/*
===============
CG_Player
===============
*/
int BG_EmplacedView(vec3_t base_angles, vec3_t angles, float* new_yaw, float constraint);

float CG_RadiusForCent(const centity_t* cent)
{
	if (cent->currentState.eType == ET_NPC)
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->g2radius)
		{
			//has override
			return cent->m_pVehicle->m_pVehicleInfo->g2radius;
		}
		if (cent->currentState.g2radius)
		{
			return cent->currentState.g2radius;
		}
	}
	else if (cent->currentState.g2radius)
	{
		return cent->currentState.g2radius;
	}
	return 64.0f;
}

static float cg_vehThirdPersonAlpha = 1.0f;
extern vec3_t cg_crosshairPos;
extern vec3_t cameraCurLoc;

void CG_CheckThirdPersonAlpha(const centity_t* cent, refEntity_t* legs)
{
	float alpha = 1.0f;
	int set_flags = 0;

	if (cent->m_pVehicle)
	{
		//a vehicle
		if (cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum //not mine
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
			&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha) //it has alpha
		{
			//make sure it's not using any alpha
			legs->renderfx |= RF_FORCE_ENT_ALPHA;
			legs->shaderRGBA[3] = 255;
			return;
		}
	}

	if (!cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.predictedPlayerState.m_iVehicleNum)
	{
		//in a vehicle
		if (cg.predictedPlayerState.m_iVehicleNum == cent->currentState.clientNum)
		{
			//this is my vehicle
			if (cent->m_pVehicle
				&& cent->m_pVehicle->m_pVehicleInfo
				&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
				&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha)
			{
				//vehicle has auto third-person alpha on
				trace_t trace;
				vec3_t dir2_crosshair, end;
				VectorSubtract(cg_crosshairPos, cameraCurLoc, dir2_crosshair);
				VectorNormalize(dir2_crosshair);
				VectorMA(cameraCurLoc, cent->m_pVehicle->m_pVehicleInfo->cameraRange * 2.0f, dir2_crosshair, end);
				CG_G2Trace(&trace, cameraCurLoc, vec3_origin, vec3_origin, end, ENTITYNUM_NONE, CONTENTS_BODY);
				if (trace.entityNum == cent->currentState.clientNum
					|| trace.entityNum == cg.predictedPlayerState.clientNum)
				{
					//hit me or the vehicle I'm in
					cg_vehThirdPersonAlpha -= 0.1f * cg.frametime / 50.0f;
					if (cg_vehThirdPersonAlpha < cent->m_pVehicle->m_pVehicleInfo->cameraAlpha)
					{
						cg_vehThirdPersonAlpha = cent->m_pVehicle->m_pVehicleInfo->cameraAlpha;
					}
				}
				else
				{
					cg_vehThirdPersonAlpha += 0.1f * cg.frametime / 50.0f;
					if (cg_vehThirdPersonAlpha > 1.0f)
					{
						cg_vehThirdPersonAlpha = 1.0f;
					}
				}
				alpha = cg_vehThirdPersonAlpha;
			}
			else
			{
				//use the cvar
				//reset this
				cg_vehThirdPersonAlpha = 1.0f;
				//use the cvar
				alpha = cg_thirdPersonAlpha.value;
			}
		}
	}
	else if (cg.predictedPlayerState.clientNum == cent->currentState.clientNum)
	{
		//it's me
		//reset this
		cg_vehThirdPersonAlpha = 1.0f;
		//use the cvar
		set_flags = RF_FORCE_ENT_ALPHA;
		alpha = cg_thirdPersonAlpha.value;
	}

	if (alpha < 1.0f)
	{
		legs->renderfx |= set_flags;
		legs->shaderRGBA[3] = (unsigned char)(alpha * 255.0f);
	}
}

/*
================
GetSelfLegAnimPoint

Based On:  G_GetAnimPoint
================
*/
//Get the point in the leg animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
float GetSelfLegAnimPoint()
{
	return BG_GetLegsAnimPoint(&cg.predictedPlayerState,
		cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex);
}

/*
================
GetSelfTorsoAnimPoint

Based On:  G_GetAnimPoint
================
*/
//Get the point in the torso animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
float GetSelfTorsoAnimPoint()
{
	return bg_get_torso_anim_point(&cg.predictedPlayerState,
		cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex);
}

/*
===============
SmoothTrueView

Purpose:  Uses the currently setup model-based First Person View to calculation the final viewangles.  Features the
following:
1.  Simulates allowable eye movement by makes a deadzone around the inputed viewangles vs the desired
viewangles of cg.refdef.viewangles
2.  Prevents the sudden view flipping during moves where your camera is suppose to flip 360 on the pitch (x)
pitch (x) axis.
===============
*/

void SmoothTrueView(vec3_t eye_angles)
{
	const float leg_anim_point = GetSelfLegAnimPoint();
	const float torso_anim_point = GetSelfTorsoAnimPoint();

	qboolean eye_range = qtrue;
	qboolean use_ref_def = qfalse;
	qboolean did_special = qfalse;

	//Rolls
	if (cg_trueroll.integer)
	{
		if (cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_STOP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_STOP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_LEFT
			|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_RIGHT)
		{
			//Roll moves that look good with eye range
			eye_range = qtrue;
			did_special = qtrue;
		}
		else if (cg_trueroll.integer == 1)
		{
			//Use simple roll for the more complicated rolls
			if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_L
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_L)
			{
				//Left rolls
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[2] += AngleNormalize180(360 * leg_anim_point);
				AngleNormalize180(eye_angles[2]);
				eye_range = qfalse;
				did_special = qtrue;
			}
			else if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_R
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_R)
			{
				//Right rolls
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[2] += AngleNormalize180(360 - 360 * leg_anim_point);
				AngleNormalize180(eye_angles[2]);
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
		else
		{
			//You're here because you're using cg_trueroll.integer == 2
			if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_L
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_L
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_R
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_R)
			{
				//Roll animation, lock the eyemovement
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
	}
	else if (cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_STOP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_STOP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_LEFT
		|| cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_RIGHT
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_L
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_L
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_R
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_R)
	{
		//you don't want rolling so use cg.refdef.viewangles as the view
		use_ref_def = qtrue;
	}

	//Flips
	if (cg_trueflip.integer)
	{
		if (cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_BACK1)
		{
			//Flip moves that look good with the eyemovement locked
			eye_range = qfalse;
			did_special = qtrue;
		}
		else if (cg_trueflip.integer == 1)
		{
			//Use simple flip for the more complicated flips
			if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_F
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_F2
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F1
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F2)
			{
				//forward flips
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[0] += AngleNormalize180(360 - 360 * leg_anim_point);
				AngleNormalize180(eye_angles[0]);
				eye_range = qfalse;
				did_special = qtrue;
			}
			else if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_B
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_B
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK2
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK3)
			{
				//back flips
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[0] += AngleNormalize180(360 * leg_anim_point);
				AngleNormalize180(eye_angles[0]);
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
		else
		{
			//You're here because you're using cg_trueflip.integer = 2
			if (cg.predictedPlayerState.legsAnim == BOTH_FLIP_F
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_F2
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F1
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F2
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_B
				|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_B
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK2
				|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK3)
			{
				//Flip animation and using cg_trueflip.integer = 2, lock the eyemovement
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
	}
	else if (cg.predictedPlayerState.legsAnim == BOTH_WALL_FLIP_BACK1
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_F
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_F2
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F1
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_F2
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_B
		|| cg.predictedPlayerState.legsAnim == BOTH_ROLL_B
		|| cg.predictedPlayerState.legsAnim == BOTH_FLIP_BACK1)
	{
		//you don't want flipping so use cg.refdef.viewangles as the view
		use_ref_def = qtrue;
	}

	if (cg_truespin.integer)
	{
		if (cg_truespin.integer == 1)
		{
			//Do a simulated Spin for the more complicated spins
			if (cg.predictedPlayerState.torsoAnim == BOTH_T1_TL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__L__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BL_TR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_TL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__L_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__L__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BL_BR
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BL__R
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BL_TR
				|| cg.predictedPlayerState.torsoAnim == BOTH_ATTACK_BACK
				|| cg.predictedPlayerState.torsoAnim == BOTH_CROUCHATTACKBACK1
				|| cg.predictedPlayerState.torsoAnim == BOTH_BUTTERFLY_LEFT
				//This technically has 2 spins and seems to have been labeled wrong
				|| cg.predictedPlayerState.legsAnim == BOTH_FJSS_TR_BL)
			{
				//Left Spins
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[1] += AngleNormalize180(360 - 360 * torso_anim_point);
				AngleNormalize180(eye_angles[1]);
				eye_range = qfalse;
				did_special = qtrue;
			}
			else if (cg.predictedPlayerState.torsoAnim == BOTH_T1_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__R__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_TR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BR_TL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T1_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T2__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T3__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BR__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T4__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__R__L
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5__R_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_TR_BL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BR_TL
				|| cg.predictedPlayerState.torsoAnim == BOTH_T5_BR__L
				//This technically has 2 spins
				|| cg.predictedPlayerState.legsAnim == BOTH_BUTTERFLY_RIGHT
				//This technically has 2 spins and seems to have been labeled wrong
				|| cg.predictedPlayerState.legsAnim == BOTH_FJSS_TL_BR)
			{
				//Right Spins
				VectorCopy(cg.refdef.viewangles, eye_angles);
				eye_angles[1] += AngleNormalize180(360 * torso_anim_point);
				AngleNormalize180(eye_angles[1]);
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
		else
		{
			//You're here because you're using cg_truespin.integer == 2
			if (PM_SpinningSaberAnim(cg.predictedPlayerState.torsoAnim)
				&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSLASHDOWN1
				&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSTABDOWN)
			{
				//Flip animation and using cg_trueflip.integer = 2, lock the eyemovement
				eye_range = qfalse;
				did_special = qtrue;
			}
		}
	}
	else if (PM_SpinningSaberAnim(cg.predictedPlayerState.torsoAnim)
		&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSLASHDOWN1
		&& cg.predictedPlayerState.torsoAnim != BOTH_JUMPFLIPSTABDOWN)
	{
		//you don't want spinning so use cg.refdef.viewangles as the view
		use_ref_def = qtrue;
	}
	else if (cg.predictedPlayerState.legsAnim == BOTH_JUMPATTACK6)
	{
		use_ref_def = qtrue;
	}
	else if (cg.predictedPlayerState.legsAnim == BOTH_GRIEVOUS_LUNGE)
	{
		use_ref_def = qtrue;
	}

	//Prevent camera flicker while landing.
	if (cg.predictedPlayerState.legsAnim == BOTH_LAND1
		|| cg.predictedPlayerState.legsAnim == BOTH_LAND2
		|| cg.predictedPlayerState.legsAnim == BOTH_LAND3
		|| cg.predictedPlayerState.legsAnim == BOTH_LANDBACK1
		|| cg.predictedPlayerState.legsAnim == BOTH_LANDLEFT1
		|| cg.predictedPlayerState.legsAnim == BOTH_LANDRIGHT1)
	{
		use_ref_def = qtrue;
	}

	//Prevent the camera flicker while switching to the saber.
	if (cg.predictedPlayerState.torsoAnim == BOTH_STAND2TO1
		|| cg.predictedPlayerState.torsoAnim == BOTH_STAND1TO2
		|| cg.predictedPlayerState.torsoAnim == BOTH_S1_S7
		|| cg.predictedPlayerState.torsoAnim == BOTH_S7_S1
		|| cg.predictedPlayerState.torsoAnim == BOTH_S1_S6
		|| cg.predictedPlayerState.torsoAnim == BOTH_S6_S1
		|| cg.predictedPlayerState.torsoAnim == BOTH_DOOKU_FULLDRAW
		|| cg.predictedPlayerState.torsoAnim == BOTH_DOOKU_SMALLDRAW
		|| cg.predictedPlayerState.torsoAnim == BOTH_SABER_IGNITION
		|| cg.predictedPlayerState.torsoAnim == BOTH_SABER_IGNITION_JFA
		|| cg.predictedPlayerState.torsoAnim == BOTH_SABER_BACKHAND_IGNITION
		|| cg.predictedPlayerState.torsoAnim == BOTH_GRIEVOUS_SABERON)
	{
		use_ref_def = qtrue;
	}

	//special camera view for blue backstab
	if (cg.predictedPlayerState.torsoAnim == BOTH_A2_STABBACK1 || cg.predictedPlayerState.torsoAnim ==
		BOTH_A2_STABBACK1B)
	{
		eye_range = qfalse;
		did_special = qtrue;
	}

	if (cg.predictedPlayerState.torsoAnim == BOTH_JUMPFLIPSLASHDOWN1
		|| cg.predictedPlayerState.torsoAnim == BOTH_JUMPFLIPSTABDOWN)
	{
		eye_range = qfalse;
		did_special = qtrue;
	}

	if (use_ref_def)
	{
		VectorCopy(cg.refdef.viewangles, eye_angles);
	}
	else
	{
		//Movement Roll dampener
		if (!did_special)
		{
			if (!cg_truemoveroll.integer)
			{
				eye_angles[2] = cg.refdef.viewangles[2];
			}
			else if (cg_truemoveroll.integer == 1)
			{
				//dampen the movement leaning
				eye_angles[2] *= .5;
			}
		}

		//eye movement
		if (eye_range)
		{
			//allow eye motion
			for (int i = 0; i < 2; i++)
			{
				int fov;
				if (cg_truefov.integer)
				{
					fov = cg_truefov.value;
				}
				else if (cg_saberlockfov.integer)
				{
					fov = cg_saberlockfov.value;
				}
				else if (cg_oversizedview.integer)
				{
					fov = cg_oversizedview.value;
				}
				else if (cg_undersizedview.integer)
				{
					fov = cg_undersizedview.value;
				}
				else
				{
					fov = cg_fov.value;
				}

				float ang_diff = eye_angles[i] - cg.refdef.viewangles[i];

				ang_diff = AngleNormalize180(ang_diff);
				if (fabs(ang_diff) > fov)
				{
					if (ang_diff < 0)
					{
						eye_angles[i] += fov;
					}
					else
					{
						eye_angles[i] -= fov;
					}
				}
				else
				{
					eye_angles[i] = cg.refdef.viewangles[i];
				}
				AngleNormalize180(eye_angles[i]);
			}
		}
	}
}

extern void* g2HolsterWeaponInstances[MAX_WEAPONS];
void* CG_G2HolsterWeaponInstance(const centity_t* cent, int weapon, qboolean second_saber);

void ApplyAxisRotation(vec3_t axis[3], const int rot_type, const float value)
{
	//apply matrix rotation to this axis.
	//rotType = type of rotation (PITCH, YAW, ROLL)
	//value = size of rotation in degrees, no action if == 0
	vec3_t result[3]; //The resulting axis
	vec3_t rotation[3]; //rotation matrix

	if (value == 0)
	{
		//no rotation, just return.
		return;
	}

	//init rotation matrix
	switch (rot_type)
	{
	case ROLL: //R_X
		rotation[0][0] = 1;
		rotation[0][1] = 0;
		rotation[0][2] = 0;

		rotation[1][0] = 0;
		rotation[1][1] = cos(value / 360 * (2 * M_PI));
		rotation[1][2] = -sin(value / 360 * (2 * M_PI));

		rotation[2][0] = 0;
		rotation[2][1] = sin(value / 360 * (2 * M_PI));
		rotation[2][2] = cos(value / 360 * (2 * M_PI));
		break;

	case PITCH: //R_Y
		rotation[0][0] = cos(value / 360 * (2 * M_PI));
		rotation[0][1] = 0;
		rotation[0][2] = sin(value / 360 * (2 * M_PI));

		rotation[1][0] = 0;
		rotation[1][1] = 1;
		rotation[1][2] = 0;

		rotation[2][0] = -sin(value / 360 * (2 * M_PI));
		rotation[2][1] = 0;
		rotation[2][2] = cos(value / 360 * (2 * M_PI));
		break;

	case YAW: //R_Z
		rotation[0][0] = cos(value / 360 * (2 * M_PI));
		rotation[0][1] = -sin(value / 360 * (2 * M_PI));
		rotation[0][2] = 0;

		rotation[1][0] = sin(value / 360 * (2 * M_PI));
		rotation[1][1] = cos(value / 360 * (2 * M_PI));
		rotation[1][2] = 0;

		rotation[2][0] = 0;
		rotation[2][1] = 0;
		rotation[2][2] = 1;
		break;

	default:
		Com_Printf("Error:  Bad rotType %i given to ApplyAxisRotation\n", rot_type);
		break;
	}

	//apply rotation
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			result[i][j] = rotation[i][0] * axis[0][j] + rotation[i][1] * axis[1][j] + rotation[i][2] * axis[2][j];
		}
	}

	//copy result
	AxisCopy(result, axis);
}

extern stringID_table_t holsterTypeTable[];

void CG_HolsteredWeaponRender(centity_t* cent, const clientInfo_t* ci, const int holster_type)
{
	refEntity_t ent;
	vec3_t axis[3];
	vec3_t bolt_org;
	mdxaBone_t boltMatrix;
	vec3_t pos_offset;
	vec3_t ang_offset;
	qhandle_t offset_bolt;
	int weapon_type;
	qboolean second_weap = qfalse;
	int boneIndex;

	if (cent->currentState.number == cg.snap->ps.clientNum &&
		!cg.renderingThirdPerson && !cg_trueguns.integer && cg.snap->ps.weapon != WP_SABER)
	{
		//don't render when in first person and not in True View.
		return;
	}

	if (!cent->ghoul2)
	{
		//no ghoul2 instance on player
		return;
	}

	//debugger override for editting the holster positions
	if (cg_holsterdebug.integer == holster_type)
	{
		//debug has been set for this holsterType, use the debug overrides
		boneIndex = cg_holsterdebug_boneindex.integer;
		sscanf(cg_holsterdebug_posoffset.string, "%f %f %f", &pos_offset[0], &pos_offset[1], &pos_offset[2]);
		sscanf(cg_holsterdebug_angoffset.string, "%f %f %f", &ang_offset[0], &ang_offset[1], &ang_offset[2]);
	}
	else
	{
		//use the model's loaded data for this holsterType
		boneIndex = ci->holsterData[holster_type].boneIndex;
		VectorCopy(ci->holsterData[holster_type].posOffset, pos_offset);
		VectorCopy(ci->holsterData[holster_type].angOffset, ang_offset);
	}

	if (boneIndex == HOLSTER_NONE)
	{
		//this weapon isn't set up to be rendered on this player model.
		return;
	}

	switch (holster_type)
	{
	case HLR_SINGLESABER_1:
		weapon_type = WP_SABER;
		break;
	case HLR_SINGLESABER_2:
		weapon_type = WP_SABER;
		second_weap = qtrue;
		break;
	case HLR_STAFFSABER:
		weapon_type = WP_SABER;
		break;
	case HLR_PISTOL_L:
	case HLR_PISTOL_R:
		weapon_type = WP_BRYAR_PISTOL;
		break;
	case HLR_BLASTER_L:
	case HLR_BLASTER_R:
		weapon_type = WP_BLASTER;
		break;
	case HLR_BRYARPISTOL_L:
	case HLR_BRYARPISTOL_R:
		weapon_type = WP_BRYAR_OLD;
		break;
	case HLR_BOWCASTER:
		weapon_type = WP_BOWCASTER;
		break;
	case HLR_ROCKET_LAUNCHER:
		weapon_type = WP_ROCKET_LAUNCHER;
		break;
	case HLR_DEMP2:
		weapon_type = WP_DEMP2;
		break;
	case HLR_CONCUSSION:
		weapon_type = WP_CONCUSSION;
		break;
	case HLR_REPEATER:
		weapon_type = WP_REPEATER;
		break;
	case HLR_FLECHETTE:
		weapon_type = WP_FLECHETTE;
		break;
	case HLR_DISRUPTOR:
		weapon_type = WP_DISRUPTOR;
		break;
	default:
		Com_Printf("Unknown weaponType for holsterType %i in CG_HolsteredWeaponRender.\n", holster_type);
		return;
	}

	//set offsetBolt
	switch (boneIndex)
	{
	case HOLSTER_UPPERBACK:
		offset_bolt = 2; //2 = jetpack tag position
		break;
	case HOLSTER_LOWERBACK:
		offset_bolt = ci->bolt_llumbar; //use lower lumbar bone
		break;
	case HOLSTER_LEFTHIP:
		offset_bolt = ci->bolt_lfemurYZ; //use left hip bone
		break;
	case HOLSTER_RIGHTHIP:
		offset_bolt = ci->bolt_rfemurYZ; //use right hip bone
		break;
	default:
		Com_Printf("Unknown offsetBolt for boneIndex %i in CG_HolsteredWeaponRender.\n", boneIndex);
		return;
	}

	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, offset_bolt, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time,
		cgs.game_models, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bolt_org);

	//find bolt rotation unit vectors
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, axis[0]); //left/right
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Z, axis[1]); //fwd/back
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Y, axis[2]); //up/down?!

	memset(&ent, 0, sizeof ent);

	//positional transitions
	VectorMA(bolt_org, pos_offset[0], axis[0], bolt_org);
	VectorMA(bolt_org, pos_offset[1], axis[1], bolt_org);
	VectorMA(bolt_org, pos_offset[2], axis[2], bolt_org);

	//rotational transitions
	//configure the initial rotational axis
	VectorCopy(axis[1], ent.axis[0]);
	VectorScale(axis[0], -1, ent.axis[1]); //reversed since this is a right hand rule system.
	VectorCopy(axis[2], ent.axis[2]);

	//debug/config rotation statement.

	ApplyAxisRotation(ent.axis, PITCH, ang_offset[PITCH]);
	ApplyAxisRotation(ent.axis, YAW, ang_offset[YAW]);
	ApplyAxisRotation(ent.axis, ROLL, ang_offset[ROLL]);

	VectorCopy(bolt_org, ent.origin);
	//AnglesToAxis( angles, ent.axis );

	//attach the ghoul2 weapon instance to the render entity.
	ent.ghoul2 = CG_G2HolsterWeaponInstance(cent, weapon_type, second_weap);

	if (cent->currentState.powerups & 1 << PW_CLOAKED)
	{
		ent.customShader = cgs.media.cloakedShader;
	}

	trap->R_AddRefEntityToScene(&ent);
}

//Defines for the ghoul2 model indexes
#define		G2MODEL_SABER_HOLSTERED			4
#define		G2MODEL_SABER2_HOLSTERED		5
#define		G2MODEL_STAFF_HOLSTERED			6
#define		G2MODEL_BLASTER_HOLSTERED		7
#define		G2MODEL_BLASTER2_HOLSTERED		8
#define		G2MODEL_LAUNCHER_HOLSTERED		9
#define		G2MODEL_GOLAN_HOLSTERED			10

void CG_VisualWeaponsUpdate(centity_t* cent, clientInfo_t* ci)
{
	//renders holstered weapons on players.
	//flag to indicate that
	//used to make sure that
	qboolean back_in_use = cent->currentState.eFlags & EF_JETPACK ? qtrue : qfalse;
	int weap_inv;

	if (cg.snap && cg.snap->ps.clientNum == cent->currentState.number)
	{
		//this cent is the client, use playerstate data
		if (cg_holsteredweapons.integer < 1)
		{
			//no holstered weapon rendering
			return;
		}

		weap_inv = cg.snap->ps.stats[STAT_WEAPONS];
	}
	else
	{
		//use event generated data.  Note that this data won't be updated if not all of the clients have the OJP client plugin.
		if (cg_holsteredweapons.integer < 2)
		{
			//no rendering holstered weapons on other players.
			return;
		}
		if (CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.predictedPlayerState.clientNum))
		{
			//this player has mind tricked the client, don't render weapons on this dude.
			return;
		}

		weap_inv = cent->weapons;
	}

	if (cent->ghoul2 &&
		(cent->currentState.eType != ET_NPC
			|| cent->currentState.NPC_class != CLASS_VEHICLE
			&& cent->currentState.NPC_class != CLASS_REMOTE
			&& cent->currentState.NPC_class != CLASS_SEEKER)
		//don't add weapon models to NPCs that have no bolt for them!
		&& !(cent->currentState.eFlags & EF_DEAD) //dead players don't have holstered weapons.
		&& !cent->torsoBolt
		&& cg.snap)
	{
		int left_hip_in_use = 0;
		int right_hip_in_use = 0;
		//this player can have holstered weapons
		//sabers
		if (weap_inv & 1 << WP_SABER)
		{
			//have saber in inventory
			if (cent->currentState.weapon != WP_SABER)
			{
				//saber is holstered. render as nessicary.
				if (!(ci->saber[0].saberFlags & SFL_TWO_HANDED))
				{
					//using single saber(s)
					if (cent->currentState.saberEntityNum && !cent->currentState.saberInFlight)
					{
						//only have holstered saber if we current have our saber in our posession.
						if (ci->holster_saber != -1)
						{
							//have specialized holster surface tag for single sabers
							if (!ci->saberHolstered)
							{
								//we haven't bolted the saber to the model yet. Do it now.
								trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0,
									cent->ghoul2, G2MODEL_SABER_HOLSTERED);
								trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_SABER_HOLSTERED, ci->holster_saber);
								ci->saberHolstered = qtrue;
							}
						}
						else
						{
							//use offset method
							CG_HolsteredWeaponRender(cent, ci, HLR_SINGLESABER_1);
						}
					}

					//secondary saber
					if (ci->saber[1].model && ci->ghoul2Weapons[1])
					{
						//have a second saber
						if (ci->holster_saber2 != -1)
						{
							//use specialized bolt.
							trap->G2API_CopySpecificGhoul2Model(ci->ghoul2Weapons[1], 0, cent->ghoul2,
								G2MODEL_SABER2_HOLSTERED);
							trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_SABER2_HOLSTERED, ci->holster_saber2);
							ci->saber2_holstered = qtrue;
						}
						else
						{
							//use offset method
							CG_HolsteredWeaponRender(cent, ci, HLR_SINGLESABER_2);
						}
					}
				}
				else
				{
					//using staff saber
					if (cent->currentState.saberEntityNum && !cent->currentState.saberInFlight)
					{
						//only have holstered saber if we current have our saber in our posession.
						if (!back_in_use)
						{
							//only use if you're not wearing a jetpack.
							if (ci->holster_staff != -1)
							{
								//have specialized holster surface tag for single sabers
								if (!ci->saberHolstered)
								{
									//we haven't bolted the saber to the model yet. Do it now.
									trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0,
										cent->ghoul2, G2MODEL_STAFF_HOLSTERED);
									trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_STAFF_HOLSTERED, ci->holster_staff);
									ci->staff_holstered = qtrue;
								}
							}
							else
							{
								//use offset method
								CG_HolsteredWeaponRender(cent, ci, HLR_STAFFSABER);
							}
							//let the system know that we're using the back holster position at the moment.
							back_in_use = qtrue;
						}
					}
				}
			}
			else
			{
				//sabers in use, remove models from bolts if being used.
				if (ci->holster_saber)
				{
					//single saber in use and is bolted, remove
					if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_SABER_HOLSTERED))
					{
						trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_SABER_HOLSTERED);
					}
					ci->saberHolstered = qfalse;
				}

				if (ci->staff_holstered)
				{
					//staff saber in use and is bolted, remove
					if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_STAFF_HOLSTERED))
					{
						trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_STAFF_HOLSTERED);
					}
					ci->staff_holstered = qfalse;
				}

				if (ci->saber2_holstered)
				{
					//second saber in use and is bolted, remove.
					if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_SABER2_HOLSTERED))
					{
						trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_SABER2_HOLSTERED);
					}
					ci->saber2_holstered = qfalse;
				}
			}
		}
		else
		{
			//don't have any sabers, remove them from bolts if we have them.
			if (ci->holster_saber)
			{
				//single saber in use and is bolted, remove
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_SABER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_SABER_HOLSTERED);
				}
				ci->saberHolstered = qfalse;
			}

			if (ci->staff_holstered)
			{
				//staff saber in use and is bolted, remove
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_STAFF_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_STAFF_HOLSTERED);
				}
				ci->staff_holstered = qfalse;
			}

			if (ci->saber2_holstered)
			{
				//second saber in use and is bolted, remove.
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_SABER2_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_SABER2_HOLSTERED);
				}
				ci->saber2_holstered = qfalse;
			}
		}

		//Handle Blaster Holster on right hip
		if (!(weap_inv & 1 << WP_BLASTER) //don't have blaster
			|| cent->currentState.weapon == WP_BLASTER) //or are currently using blaster
		{
			//don't need holstered blaster rendered
			if (ci->holster_blaster != -1 && ci->blaster_holstered == WP_BLASTER)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{
			//need holstered blaster to be rendered
			if (ci->holster_blaster != -1)
			{
				//have specialized bolt
				if (ci->blaster_holstered != WP_BLASTER)
				{
					//don't already have the blaster bolted.
					if (ci->blaster_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the blaster
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BLASTER), 0, cent->ghoul2,
						G2MODEL_BLASTER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BLASTER;
				}
			}
			else
			{
				//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_BLASTER_R);
			}
			right_hip_in_use = WP_BLASTER;
		}

		//handle rendering WP_BRYAR_PISTOL on right hip holster
		if (right_hip_in_use //hip in use already
			|| !(weap_inv & 1 << WP_BRYAR_PISTOL) //don't have the WP_BRYAR_PISTOL
			|| cent->currentState.weapon == WP_BRYAR_PISTOL) //currently using WP_BRYAR_PISTOL
		{
			//don't render WP_BRYAR_PISTOL on right hip.
			if (ci->holster_blaster != -1 && ci->blaster_holstered == WP_BRYAR_PISTOL)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{
			//render WP_BRYAR_PISTOL on right hip
			if (ci->holster_blaster != -1)
			{
				//have specialized bolt
				if (ci->blaster_holstered != WP_BRYAR_PISTOL)
				{
					//don't already have the WP_BRYAR_PISTOL bolted.
					if (ci->blaster_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the WP_BRYAR_PISTOL
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_PISTOL), 0, cent->ghoul2,
						G2MODEL_BLASTER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BRYAR_PISTOL;
				}
			}
			else
			{
				//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_PISTOL_R);
			}
			right_hip_in_use = WP_BRYAR_PISTOL;
		}

		//handle old school pistol
		if (right_hip_in_use //hip in use already
			|| !(weap_inv & 1 << WP_BRYAR_OLD) //don't have the WP_BRYAR_OLD
			|| cent->currentState.weapon == WP_BRYAR_OLD) //currently using WP_BRYAR_OLD
		{
			//don't render WP_BRYAR_OLD on right hip.
			if (ci->holster_blaster != -1 && ci->blaster_holstered == WP_BRYAR_OLD)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{
			//render WP_BRYAR_OLD on right hip
			if (ci->holster_blaster != -1)
			{
				//have specialized bolt
				if (ci->blaster_holstered != WP_BRYAR_OLD)
				{
					//don't already have the WP_BRYAR_OLD bolted.
					if (ci->blaster_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the WP_BRYAR_OLD
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_OLD), 0, cent->ghoul2,
						G2MODEL_BLASTER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BRYAR_OLD;
				}
			}
			else
			{
				//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_BRYARPISTOL_R);
			}
			right_hip_in_use = WP_BRYAR_OLD;
		}

		/*============================
		* Start Left Hip Holster code
		*============================
		*/
		//Handle Blaster Holster on left hip
		if (!(weap_inv & 1 << WP_BLASTER) //don't have blaster
			|| cent->currentState.weapon == WP_BLASTER //or are currently using blaster
			|| right_hip_in_use == WP_BLASTER) //or the blaster is already on the right hip.

		{
			//don't need holstered blaster on left hip rendered
			if (ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BLASTER)
			{
				//remove bolted holster instance.
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{
			//need holstered blaster to be rendered
			if (ci->holster_blaster2 != -1)
			{
				//have specialized bolt
				if (ci->blaster2_holstered != WP_BLASTER)
				{
					//don't already have the blaster bolted.
					if (ci->blaster2_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BLASTER), 0, cent->ghoul2,
						G2MODEL_BLASTER2_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BLASTER;
				}
			}
			else
			{
				//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_BLASTER_L);
			}
			left_hip_in_use = WP_BLASTER;
		}

		//Handle pistol Holster on left hip
		if (left_hip_in_use
			|| !(weap_inv & 1 << WP_BRYAR_PISTOL) //don't have pistol
			|| cent->currentState.weapon == WP_BRYAR_PISTOL //or are currently using pistol
			|| right_hip_in_use == WP_BRYAR_PISTOL) //or the pistol is already on the right hip.
		{
			//don't need holstered pistol on left hip rendered
			if (ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BRYAR_PISTOL)
			{
				//remove bolted holster instance.
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{
			//need holstered pistol to be rendered
			if (ci->holster_blaster2 != -1)
			{
				//have specialized bolt
				if (ci->blaster2_holstered != WP_BRYAR_PISTOL)
				{
					//don't already have the pistol bolted.
					if (ci->blaster2_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_PISTOL), 0, cent->ghoul2,
						G2MODEL_BLASTER2_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BRYAR_PISTOL;
				}
			}
			else
			{
				//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_PISTOL_L);
			}
			left_hip_in_use = WP_BRYAR_PISTOL;
		}

		//Handle pistol Holster on left hip
		if (left_hip_in_use
			|| !(weap_inv & 1 << WP_BRYAR_OLD) //don't have pistol
			|| cent->currentState.weapon == WP_BRYAR_OLD //or are currently using pistol
			|| right_hip_in_use == WP_BRYAR_OLD) //or the pistol is already on the right hip.
		{
			//don't need holstered pistol on left hip rendered
			if (ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BRYAR_OLD)
			{
				//remove bolted holster instance.
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{
			//need holstered pistol to be rendered
			if (ci->holster_blaster2 != -1)
			{
				//have specialized bolt
				if (ci->blaster2_holstered != WP_BRYAR_OLD)
				{
					//don't already have the pistol bolted.
					if (ci->blaster2_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_OLD), 0, cent->ghoul2,
						G2MODEL_BLASTER2_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BRYAR_OLD;
				}
			}
			else
			{
				//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_BRYARPISTOL_L);
			}
		}

		/*============================
		* End Left Hip Holster code
		*============================
		*/

		/*============================
		* Start Back Gun Holster code
		*============================
		*/

		//Golan Rocket Launcher
		if (weap_inv & 1 << WP_ROCKET_LAUNCHER)
		{
			//have launcher
			if (cent->currentState.weapon != WP_ROCKET_LAUNCHER && !back_in_use)
			{
				//need to render holstered rocket launcher
				if (ci->holster_golan != -1 && !ci->golan_holstered)
				{
					//we have a valid holster bolt for this weapon and we haven't bolted
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_ROCKET_LAUNCHER), 0, cent->ghoul2,
						G2MODEL_GOLAN_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_GOLAN_HOLSTERED, ci->holster_golan);
					ci->golan_holstered = qtrue;
				}
				else
				{
					//manual offset method
					CG_HolsteredWeaponRender(cent, ci, HLR_ROCKET_LAUNCHER);
				}
				back_in_use = qtrue;
			}
			else if (ci->golan_holstered)
			{
				//remove holstered instace if exists.
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_GOLAN_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_GOLAN_HOLSTERED);
				}
				ci->golan_holstered = qfalse;
			}
		}
		else if (ci->golan_holstered)
		{
			//remove holstered instace if exists.
			if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_GOLAN_HOLSTERED))
			{
				trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_GOLAN_HOLSTERED);
			}
			ci->golan_holstered = qfalse;
		}

		//handle concussion on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_CONCUSSION) //don't have the concussion
			|| cent->currentState.weapon == WP_CONCUSSION) //currently using concussion
		{
			//don't render weapon on back
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_CONCUSSION)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render concussion on right hip
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_CONCUSSION)
				{
					//don't already have the concussion bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now add the concussion
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_CONCUSSION), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_CONCUSSION;
				}
			}
			else
			{
				//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_CONCUSSION);
			}
			back_in_use = qtrue;
		}

		//handle repeater on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_REPEATER) //don't have weapon
			|| cent->currentState.weapon == WP_REPEATER) //currently using weapon
		{
			//don't render weapon on back
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_REPEATER)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render weapon on back
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_REPEATER)
				{
					//don't already have the concussion bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_REPEATER), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_REPEATER;
				}
			}
			else
			{
				//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_REPEATER);
			}
			back_in_use = qtrue;
		}

		//handle flechette on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_FLECHETTE) //don't have weapon
			|| cent->currentState.weapon == WP_FLECHETTE) //currently using weapon
		{
			//don't render weapon on back
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_FLECHETTE)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render weapon on back
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_FLECHETTE)
				{
					//don't already have the weapon bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_FLECHETTE), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_FLECHETTE;
				}
			}
			else
			{
				//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_FLECHETTE);
			}
			back_in_use = qtrue;
		}

		//handle disruptor on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_DISRUPTOR) //don't have weapon
			|| cent->currentState.weapon == WP_DISRUPTOR) //currently using weapon
		{
			//don't render weapon on back
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_DISRUPTOR)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render weapon on back
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_DISRUPTOR)
				{
					//don't already have the weapon bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_DISRUPTOR), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_DISRUPTOR;
				}
			}
			else
			{
				//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_DISRUPTOR);
			}
			back_in_use = qtrue;
		}

		//handle bowcaster on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_BOWCASTER) //don't have weapon
			|| cent->currentState.weapon == WP_BOWCASTER) //currently using weapon
		{
			//don't render weapon on back
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_BOWCASTER)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render weapon on back
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_BOWCASTER)
				{
					//don't already have the weapon bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BOWCASTER), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_BOWCASTER;
				}
			}
			else
			{
				//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_BOWCASTER);
			}
			back_in_use = qtrue;
		}

		//handle demp 2 on back
		if (back_in_use //back in use already
			|| !(weap_inv & 1 << WP_DEMP2) //don't have the demp2
			|| cent->currentState.weapon == WP_DEMP2) //currently using Demp2
		{
			//don't render Demp2 on right hip.
			if (ci->holster_launcher != -1 && ci->launcher_holstered == WP_DEMP2)
			{
				if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{
			//render demp 2 on right hip
			if (ci->holster_launcher != -1)
			{
				//have specialized bolt
				if (ci->launcher_holstered != WP_DEMP2)
				{
					//don't already have the demp2 bolted.
					if (ci->launcher_holstered != 0)
					{
						//we have something else bolted there, remove it first.
						if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap->G2API_RemoveGhoul2Model(&cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now add the demp2
					trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_DEMP2), 0, cent->ghoul2,
						G2MODEL_LAUNCHER_HOLSTERED);
					trap->G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_DEMP2;
				}
			}
			else
			{
				//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_DEMP2);
			}
		}

		/*============================
		* End Back Gun Holster code
		*============================
		*/
	}
}

extern void CG_CubeOutline(vec3_t mins, vec3_t maxs, int time, unsigned int color);

void CG_Player(centity_t* cent)
{
	clientInfo_t* ci;
	refEntity_t legs;
	refEntity_t torso;
	int renderfx;
	qboolean shadow = qfalse;
	float shadowPlane = 0;
	vec3_t root_angles;
	float angle;
	vec3_t angles, dir, elevated;
	int iwantout = 0;
	int team;
	mdxaBone_t boltMatrix, l_hand_matrix;
	int do_alpha = 0;
	qboolean got_l_hand_matrix = qfalse;
	qboolean g2_has_weapon = qfalse;
	qboolean draw_player_saber = qfalse;
	qboolean check_droid_shields = qfalse;
	int health;

	//first if we are not an npc and we are using an emplaced gun then make sure our
	//angles are visually capped to the constraints (otherwise it's possible to lerp
	//a little outside and look kind of twitchy)
	if (cent->currentState.weapon == WP_EMPLACED_GUN &&
		cent->currentState.otherentity_num2)
	{
		float emp_yaw;

		if (BG_EmplacedView(cent->lerpAngles, cg_entities[cent->currentState.otherentity_num2].currentState.angles,
			&emp_yaw, cg_entities[cent->currentState.otherentity_num2].currentState.origin2[0]))
		{
			cent->lerpAngles[YAW] = emp_yaw;
		}
	}

	if (cent->currentState.iModelScale)
	{
		//if the server says we have a custom scale then set it now.
		cent->modelScale[0] = cent->modelScale[1] = cent->modelScale[2] = cent->currentState.iModelScale / 100.0f;
		if (cent->currentState.NPC_class != CLASS_VEHICLE)
		{
			if (cent->modelScale[2] && cent->modelScale[2] != 1.0f)
			{
				cent->lerpOrigin[2] += 24 * (cent->modelScale[2] - 1);
			}
		}
	}
	else
	{
		VectorClear(cent->modelScale);
	}

	if ((cg_smoothClients.integer || cent->currentState.heldByClient) && (cent->currentState.groundEntityNum >=
		ENTITYNUM_WORLD || cent->currentState.eType == ET_TERRAIN) &&
		!(cent->currentState.eFlags2 & EF2_HYPERSPACE) && cg.predictedPlayerState.m_iVehicleNum != cent->currentState.
		number)
	{
		//always smooth when being thrown
		vec3_t pos_dif;
		float smooth_factor;
		int k = 0;
		float f_tolerance = 20000.0f;

		if (cent->currentState.heldByClient)
		{
			//smooth the origin more when in this state, because movement is origin-based on server.
			smooth_factor = 0.2f;
		}
		else if (cent->currentState.powerups & 1 << PW_SPEED)
		{
			//we're moving fast so don't smooth as much
			smooth_factor = 0.6f;
		}
		else if (cent->currentState.eType == ET_NPC && cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//greater smoothing for flying vehicles, since they move so fast
			f_tolerance = 6000000.0f; //500000.0f; //yeah, this is so wrong..but..
			smooth_factor = 0.5f;
		}
		else
		{
			smooth_factor = 0.5f;
		}

		if (DistanceSquared(cent->beamEnd, cent->lerpOrigin) > smooth_factor * f_tolerance) //10000
		{
			VectorCopy(cent->lerpOrigin, cent->beamEnd);
		}

		VectorSubtract(cent->lerpOrigin, cent->beamEnd, pos_dif);

		for (k = 0; k < 3; k++)
		{
			cent->beamEnd[k] = cent->beamEnd[k] + pos_dif[k] * smooth_factor;
			cent->lerpOrigin[k] = cent->beamEnd[k];
		}
	}
	else
	{
		VectorCopy(cent->lerpOrigin, cent->beamEnd);
	}

	if (cent->currentState.m_iVehicleNum &&
		cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		//this player is riding a vehicle
		centity_t* veh = &cg_entities[cent->currentState.m_iVehicleNum];

		cent->lerpAngles[YAW] = veh->lerpAngles[YAW];

		//Attach ourself to the vehicle
		if (veh->m_pVehicle &&
			cent->playerState &&
			veh->playerState &&
			cent->ghoul2 &&
			veh->ghoul2)
		{
			if (veh->currentState.owner != cent->currentState.clientNum)
			{
				//FIXME: what about visible passengers?
				if (CG_VehicleAttachDroidUnit(cent))
				{
					check_droid_shields = qtrue;
				}
			}
			// fix for screen blinking when spectating person on vehicle and then
			// switching to someone else, often happens on siege
			else if (veh->currentState.owner != ENTITYNUM_NONE &&
				cent->playerState->clientNum != cg.snap->ps.clientNum)
			{
				//has a pilot...???
				vec3_t old_ps_org;

				//make sure it has its pilot and parent set
				veh->m_pVehicle->m_pPilot = (bgEntity_t*)&cg_entities[veh->currentState.owner];
				veh->m_pVehicle->m_pParentEntity = (bgEntity_t*)veh;

				VectorCopy(veh->playerState->origin, old_ps_org);

				//update the veh's playerstate org for getting the bolt
				VectorCopy(veh->lerpOrigin, veh->playerState->origin);
				VectorCopy(cent->lerpOrigin, cent->playerState->origin);

				//Now do the attach
				VectorCopy(veh->lerpAngles, veh->playerState->viewangles);
				veh->m_pVehicle->m_pVehicleInfo->AttachRiders(veh->m_pVehicle);

				//copy the "playerstate origin" to the lerpOrigin since that's what we use to display
				VectorCopy(cent->playerState->origin, cent->lerpOrigin);

				VectorCopy(old_ps_org, veh->playerState->origin);
			}
		}
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	if (cent->currentState.eType != ET_NPC)
	{
		int clientNum;
		clientNum = cent->currentState.clientNum;
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			trap->Error(ERR_DROP, "Bad clientNum on player entity");
		}
		ci = &cgs.clientinfo[clientNum];
	}
	else
	{
		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
			cent->npcClient->gender = FindGender(CG_ConfigString(CS_MODELS + cent->currentState.modelIndex), cent);
		}

		assert(cent->npcClient);

		if (cent->npcClient->ghoul2Model != cent->ghoul2 && cent->ghoul2)
		{
			cent->npcClient->ghoul2Model = cent->ghoul2;
			if (cent->localAnimIndex <= 1)
			{
				cent->npcClient->bolt_rhand = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand");
				cent->npcClient->bolt_lhand = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*chestg");

				//claw bolts
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand_cap_r_arm");
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand_cap_l_arm");

				cent->npcClient->bolt_head = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*head_top");
				if (cent->npcClient->bolt_head == -1)
				{
					cent->npcClient->bolt_head = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "ceyebrow");
				}
				cent->npcClient->bolt_motion = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "Motion");
				cent->npcClient->bolt_llumbar = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "lower_lumbar");
			}
			else
			{
				cent->npcClient->bolt_rhand = -1;
				cent->npcClient->bolt_lhand = -1;
				cent->npcClient->bolt_head = -1;
				cent->npcClient->bolt_motion = -1;
				cent->npcClient->bolt_llumbar = -1;
			}
			cent->npcClient->team = TEAM_FREE;
			cent->npcClient->infoValid = qtrue;
		}
		ci = cent->npcClient;
	}

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if (!ci->infoValid)
	{
		return;
	}

	// if in a duel, don't render players outside of duel
	if (cg.snap->ps.duelInProgress &&
		cent->currentState.clientNum != cg.snap->ps.clientNum && cent->currentState.clientNum != cg.snap->ps.
		duelIndex)
	{
		// duel in progress, don't render any player not part of the duel
		return;
	}

	// Add the player to the radar if on the same team and its a team game
	CG_AddRadarEnt(cent);

	if (cent->currentState.eType == ET_NPC)
	{
		//add vehicles
		CG_AddRadarEnt(cent);
		if (cent->currentState.NPC_class == CLASS_VEHICLE && CG_InFighter())
		{
			//this is a vehicle, bracket it
			if (cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum)
			{
				//don't add the vehicle I'm in... :)
				CG_AddBracketedEnt(cent);
			}
		}
	}

	if (!cent->ghoul2)
	{
		//not ready yet?
#ifdef _DEBUG
		Com_Printf("WARNING: Client %i has a null ghoul2 instance\n", cent->currentState.number);
#endif
		trap->G2API_ClearAttachedInstance(cent->currentState.number);

		if (ci->ghoul2Model &&
			trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
#ifdef _DEBUG
			Com_Printf("Clientinfo instance was valid, duplicating for cent\n");
#endif
			trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{
				//check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);
		}
		return;
	}

	if (ci->superSmoothTime)
	{
		//do crazy smoothing
		if (ci->superSmoothTime > cg.time)
		{
			//do it
			trap->G2API_AbsurdSmoothing(cent->ghoul2, qtrue);
		}
		else
		{
			//turn it off
			ci->superSmoothTime = 0;
			trap->G2API_AbsurdSmoothing(cent->ghoul2, qfalse);
		}
	}

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		//don't show all this shit during intermission
		if (cent->currentState.eType == ET_NPC
			&& cent->currentState.NPC_class != CLASS_VEHICLE)
		{
			//NPC in intermission
		}
		else
		{
			//don't render players or vehicles in intermissions, allow other NPCs for scripts
			return;
		}
	}

	CG_VehicleEffects(cent);

	if (cent->currentState.eFlags & EF_JETPACK && !(cent->currentState.eFlags & EF_DEAD) &&
		cg_g2JetpackInstance)
	{
		//should have a jetpack attached
		//1 is rhand weap, 2 is lhand weap (akimbo sabs), 3 is jetpack
		if (!trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, 3))
		{
			trap->G2API_CopySpecificGhoul2Model(cg_g2JetpackInstance, 0, cent->ghoul2, 3);
		}

		if (cent->currentState.eFlags & EF_JETPACK_ACTIVE)
		{
			mdxaBone_t mat;
			int n = 0;

			while (n < 2)
			{
				vec3_t flame_dir;
				vec3_t flame_pos;
				//Get the position/dir of the flame bolt on the jetpack model bolted to the player
				trap->G2API_GetBoltMatrix(cent->ghoul2, 3, n, &mat, cent->turAngles, cent->lerpOrigin, cg.time,
					cgs.game_models, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flame_pos);

				if (n == 0)
				{
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flame_dir);
					VectorMA(flame_pos, -9.5f, flame_dir, flame_pos);
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flame_dir);
					VectorMA(flame_pos, -13.5f, flame_dir, flame_pos);
				}
				else
				{
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flame_dir);
					VectorMA(flame_pos, -9.5f, flame_dir, flame_pos);
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flame_dir);
					VectorMA(flame_pos, -13.5f, flame_dir, flame_pos);
				}

				if (cent->currentState.eFlags & EF_JETPACK_ACTIVE
					|| cent->currentState.eFlags & EF_JETPACK_FLAMING
					|| cent->currentState.eFlags & EF3_JETPACK_HOVER)
				{
					//create effects
					//Play the effect
					trap->FX_PlayEffectID(cgs.effects.mBobaJet, flame_pos, flame_dir, -1, -1, qfalse);
					trap->FX_PlayEffectID(cgs.effects.mBobaJet, flame_pos, flame_dir, -1, -1, qfalse);

					//Keep the jet fire sound looping
					trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
						trap->S_RegisterSound("sound/jetpack/thrust"));
				}
				else
				{
					//just idling
					trap->FX_PlayEffectID(cgs.effects.mBlueJet, flame_pos, flame_dir, -1, -1, qfalse);
				}

				n++;
			}

			trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
				trap->S_RegisterSound("sound/jetpack/idle.wav"));
		}
	}
	else if (trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, 3))
	{
		//fixme: would be good if this could be done not every frame
		trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 3);
	}

	if ((cent->currentState.botclass == BCLASS_BOBAFETT ||
		cent->currentState.NPC_class == CLASS_BOBAFETT ||
		cent->currentState.NPC_class == CLASS_ROCKETTROOPER ||
		cent->currentState.botclass == BCLASS_MANDOLORIAN ||
		cent->currentState.botclass == BCLASS_MANDOLORIAN1 ||
		cent->currentState.botclass == BCLASS_MANDOLORIAN2) && cent->currentState.eFlags2 & EF2_FLYING && !(cent->
			currentState.eFlags & EF_DEAD))
	{
		mdxaBone_t mat;
		vec3_t flame_pos, flame_dir;
		qboolean wj = qfalse;
		fxHandle_t flame_effect = cent->currentState.botclass == BCLASS_BOBAFETT ||
			cent->currentState.NPC_class == CLASS_BOBAFETT ||
			cent->currentState.botclass == BCLASS_MANDOLORIAN ||
			cent->currentState.botclass == BCLASS_MANDOLORIAN1 ||
			cent->currentState.botclass == BCLASS_MANDOLORIAN2
			? cgs.effects.mBobaJet
			: cgs.effects.mBobaJet;

		if (cent->bobaInit == qfalse)
		{
			cent->bolt_jet1 = trap->G2API_AddBolt(cent->ghoul2, 0, "*jet1");
			cent->bolt_jet2 = trap->G2API_AddBolt(cent->ghoul2, 0, "*jet2");

			if (cent->bolt_jet1 == -1 || cent->bolt_jet2 == -1)
			{
				cent->bolt_jet1 = trap->G2API_AddBolt(cent->ghoul2, 0, "*hip_br");
				cent->bolt_jet2 = trap->G2API_AddBolt(cent->ghoul2, 0, "*hip_bl");

				wj = qtrue;
			}
		}

		if (!wj)
		{
			//1st flamme
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt_jet1, &mat, cent->turAngles, cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flame_pos);

			BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_X, flame_dir);
			VectorMA(flame_pos, -13.5f, flame_dir, flame_pos);
			trap->FX_PlayEffectID(flame_effect, flame_pos, flame_dir, -1, -1, qfalse);

			//2nde flamme
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt_jet2, &mat, cent->turAngles, cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flame_pos);

			BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_X, flame_dir);
			VectorMA(flame_pos, -13.5f, flame_dir, flame_pos);

			trap->FX_PlayEffectID(flame_effect, flame_pos, flame_dir, -1, -1, qfalse);
		}
		else
		{
			//1st flamme
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt_jet1, &mat, cent->turAngles, cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flame_pos);

			BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flame_dir);
			VectorMA(flame_pos, 3.5f, flame_dir, flame_pos);
			BG_GiveMeVectorFromMatrix(&mat, POSITIVE_Y, flame_dir);
			VectorMA(flame_pos, 1.0f, flame_dir, flame_pos);
			BG_GiveMeVectorFromMatrix(&mat, POSITIVE_Z, flame_dir);
			trap->FX_PlayEffectID(flame_effect, flame_pos, flame_dir, -1, -1, qfalse);

			//2nde flamme
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, cent->bolt_jet2, &mat, cent->turAngles, cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flame_pos);

			BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flame_dir);
			VectorMA(flame_pos, 3.5f, flame_dir, flame_pos);
			BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flame_dir);
			VectorMA(flame_pos, 1.0f, flame_dir, flame_pos);
			BG_GiveMeVectorFromMatrix(&mat, POSITIVE_Z, flame_dir);

			trap->FX_PlayEffectID(flame_effect, flame_pos, flame_dir, -1, -1, qfalse);
		}

		trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
			trap->S_RegisterSound("sound/chars/boba/jethover"));
	}

	g2_has_weapon = trap->G2API_HasGhoul2ModelOnIndex(&cent->ghoul2, 1);

	if (!g2_has_weapon && !cent->currentState.saberInFlight)
	{
		//force a redup of the weapon instance onto the client instance
		cent->ghoul2weapon = NULL;
		cent->weapon = 0;
	}

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{
		//he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if (cent->isRagging && !(cent->currentState.eFlags & EF_DEAD) && !(cent->currentState.eFlags & EF_RAG))
	{
		//make sure we don't ragdoll ever while alive unless directly told to with eFlags
		cent->isRagging = qfalse;
		trap->G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->ghoul2 && cent->torsoBolt && (cent->torsoBolt & RARMBIT || cent->torsoBolt & RHANDBIT || cent->torsoBolt &
		WAISTBIT) && g2_has_weapon)
	{
		//kill the weapon if the limb holding it is no longer on the model
		trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 1);
		g2_has_weapon = qfalse;
	}

	if (!cent->trickAlphaTime || cg.time - cent->trickAlphaTime > 1000)
	{
		//things got out of sync, perhaps a new client is trying to fill in this slot
		cent->trickAlpha = 255;
		cent->trickAlphaTime = cg.time;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		//If nodraw, return here
		return;
	}
	if (cent->currentState.eFlags2 & EF2_SHIP_DEATH)
	{
		//died in ship, don't draw, we were "obliterated"
		return;
	}

	if (g_DebugSaberCombat.integer)
	{
		vec3_t bmins = { -15, -15, DEFAULT_MINS_2 }, bmaxs = { 15, 15, DEFAULT_MAXS_2 };

		if (pm && pm->ps && cent->currentState.clientNum == pm->ps->clientNum)
		{
			VectorCopy(pm->mins, bmins);
			VectorCopy(pm->maxs, bmaxs);
		}
		else if (cent->currentState.solid)
		{
			int zu;
			int zd;
			int x;
			x = cent->currentState.solid & 255;
			zd = cent->currentState.solid >> 8 & 255;
			zu = (cent->currentState.solid >> 16 & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;
		}

		if (!CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4, cg.snap->ps.clientNum))
		{
			vec3_t absmax;
			vec3_t absmin;
			VectorAdd(cent->lerpOrigin, bmins, absmin);
			VectorAdd(cent->lerpOrigin, bmaxs, absmax);
			CG_CubeOutline(absmin, absmax, 1, COLOR_RED);
		}
	}

	//If this client has tricked you.
	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		if (cent->trickAlpha > 1)
		{
			cent->trickAlpha -= (cg.time - cent->trickAlphaTime) * 0.5f;
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha < 0)
			{
				cent->trickAlpha = 0;
			}

			do_alpha = 1;
		}
		else
		{
			do_alpha = 1;
			cent->trickAlpha = 1;
			cent->trickAlphaTime = cg.time;
			iwantout = 1;
		}
	}
	else
	{
		if (cent->trickAlpha < 255)
		{
			cent->trickAlpha += cg.time - cent->trickAlphaTime;
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha > 255)
			{
				cent->trickAlpha = 255;
			}

			do_alpha = 1;
		}
		else
		{
			cent->trickAlpha = 255;
			cent->trickAlphaTime = cg.time;
		}
	}

	// get the player model information
	renderfx = 0;

	if (cent->currentState.number == cg.snap->ps.clientNum)
	{
		if (!cg.renderingThirdPerson)
		{
			if (!cg_trueguns.integer && cg.predictedPlayerState.weapon != WP_SABER
				&& cg.predictedPlayerState.weapon != WP_MELEE
				|| cg.predictedPlayerState.weapon == WP_SABER && cg_truesaberonly.integer
				|| cg.predictedPlayerState.zoomMode)
			{
				renderfx = RF_THIRD_PERSON; // only draw in mirrors
			}
		}
		else
		{
			if (com_cameraMode.integer)
			{
				iwantout = 1;
				return;
			}
		}
	}

	// Update the player's client entity information regarding weapons.
	// Explanation:  The entitystate has a weapond defined on it.  The cliententity does as well.
	// The cliententity's weapon tells us what the ghoul2 instance on the cliententity has bolted to it.
	// If the entitystate and cliententity weapons differ, then the state's needs to be copied to the client.
	// Save the old weapon, to verify that it is or is not the same as the new weapon.
	// rww - Make sure weapons don't get set BEFORE cent->ghoul2 is initialized or else we'll have no
	// weapon bolted on

	if (cent->ghoul2 &&
		(cent->currentState.eType != ET_NPC || cent->currentState.NPC_class != CLASS_VEHICLE &&
			cent->currentState.NPC_class != CLASS_REMOTE && cent->currentState.NPC_class != CLASS_SEEKER) &&
		(cent->ghoul2weapon != CG_G2WeaponInstance(cent, cent->currentState.weapon) ||
			cent->ghoul2weapon2 != CG_G2WeaponInstance2(cent, cent->currentState.weapon) &&
			cent->currentState.eFlags & EF3_DUAL_WEAPONS ||
			cent->ghoul2weapon2 != NULL && !(cent->currentState.eFlags & EF3_DUAL_WEAPONS)) &&
		!(cent->currentState.eFlags & EF_DEAD) && !cent->torsoBolt &&
		cg.snap && (cent->currentState.number != cg.snap->ps.clientNum || cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		if (ci->team == TEAM_SPECTATOR)
		{
			cent->ghoul2weapon = NULL;
			cent->weapon = 0;
		}
		else
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);

			if (cent->currentState.eType != ET_NPC)
			{
				if (cent->weapon == WP_SABER
					&& cent->weapon != cent->currentState.weapon
					&& !cent->currentState.saberHolstered)
				{
					//switching away from the saber
					if (ci->saber[0].soundOff
						&& !cent->currentState.saberHolstered)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
							ci->saber[0].soundOff);
					}

					if (ci->saber[1].soundOff &&
						ci->saber[1].model[0] &&
						!cent->currentState.saberHolstered)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
							ci->saber[1].soundOff);
					}
				}
				else if (cent->currentState.weapon == WP_SABER
					&& cent->weapon != cent->currentState.weapon
					&& !cent->saberWasInFlight)
				{
					//switching to the saber
					if (ci->saber[0].soundOn)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
							ci->saber[0].soundOn);
					}

					if (ci->saber[1].soundOn)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
							ci->saber[1].soundOn);
					}

					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
			}

			cent->weapon = cent->currentState.weapon;
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
			if (cent->currentState.eFlags & EF3_DUAL_WEAPONS && cent->currentState.weapon == WP_BRYAR_PISTOL)
			{
				cent->ghoul2weapon2 = CG_G2WeaponInstance2(cent, cent->currentState.weapon);
			}
			else
			{
				cent->ghoul2weapon2 = NULL;
			}
		}
	}
	else if (cent->currentState.eFlags & EF_DEAD || cent->torsoBolt)
	{
		cent->ghoul2weapon = NULL; //be sure to update after respawning/getting limb regrown
	}

	CG_VisualWeaponsUpdate(cent, ci);

	if (cent->saberWasInFlight && g2_has_weapon && cent->ghoul2weapon == CG_G2WeaponInstance(cent, WP_SABER))
	{
		cent->saberWasInFlight = qfalse;
	}

	memset(&legs, 0, sizeof legs);

	CG_SetGhoul2Info(&legs, cent);

	VectorCopy(cent->modelScale, legs.modelScale);
	legs.radius = CG_RadiusForCent(cent);
	VectorClear(legs.angles);

	if (ci->colorOverride[0] != 0.0f ||
		ci->colorOverride[1] != 0.0f ||
		ci->colorOverride[2] != 0.0f)
	{
		legs.shaderRGBA[0] = ci->colorOverride[0] * 255.0f;
		legs.shaderRGBA[1] = ci->colorOverride[1] * 255.0f;
		legs.shaderRGBA[2] = ci->colorOverride[2] * 255.0f;
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}
	else
	{
		legs.shaderRGBA[0] = cent->currentState.customRGBA[0];
		legs.shaderRGBA[1] = cent->currentState.customRGBA[1];
		legs.shaderRGBA[2] = cent->currentState.customRGBA[2];
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}

	// minimal_add:

	team = ci->team;

	if (cgs.gametype >= GT_TEAM && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum &&
		cent->currentState.eType != ET_NPC)
	{
		// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (cgs.gametype == GT_SIEGE)
			{
				//check for per-map team shaders
				if (team == SIEGETEAM_TEAM1)
				{
					if (cgSiegeTeam1PlShader)
					{
						CG_PlayerFloatSprite(cent, cgSiegeTeam1PlShader);
					}
					else
					{
						//if there isn't one fallback to default
						CG_PlayerFloatSprite(cent, cgs.media.teamRedShader);
					}
				}
				else
				{
					if (cgSiegeTeam2PlShader)
					{
						CG_PlayerFloatSprite(cent, cgSiegeTeam2PlShader);
					}
					else
					{
						//if there isn't one fallback to default
						CG_PlayerFloatSprite(cent, cgs.media.teamBlueShader);
					}
				}
			}
			else
			{
				//generic teamplay
				if (team == TEAM_RED)
				{
					CG_PlayerFloatSprite(cent, cgs.media.teamRedShader);
				}
				else // if (team == TEAM_BLUE)
				{
					CG_PlayerFloatSprite(cent, cgs.media.teamBlueShader);
				}
			}
		}
	}
	else if (cgs.gametype == GT_POWERDUEL && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)
	{
		if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci &&
			cgs.clientinfo[cg.snap->ps.clientNum].duelTeam == ci->duelTeam)
		{
			//ally in powerduel, so draw the icon
			CG_PlayerFloatSprite(cent, cgs.media.powerDuelAllyShader);
		}
		else if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci->duelTeam == DUELTEAM_DOUBLE)
		{
			CG_PlayerFloatSprite(cent, cgs.media.powerDuelAllyShader);
		}
	}

	if (cgs.gametype == GT_JEDIMASTER && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)
		// Don't show a sprite above a player's own head in 3rd person.
	{
		// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (CG_ThereIsAMaster())
			{
				if (!cg.snap->ps.isJediMaster)
				{
					if (!cent->currentState.isJediMaster)
					{
						CG_PlayerFloatSprite(cent, cgs.media.teamRedShader);
					}
				}
			}
		}
	}

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	if ((cent->currentState.eFlags & EF_SEEKERDRONE || cent->currentState.genericenemyindex != -1) && cent->currentState
		.eType != ET_NPC)
	{
		vec3_t seekorg;
		int successchange = 0;
		refEntity_t seeker = { 0 };

		VectorCopy(cent->lerpOrigin, elevated);
		elevated[2] += 40;

		VectorCopy(elevated, seeker.lightingOrigin);
		seeker.shadowPlane = shadowPlane;
		seeker.renderfx = 0; //renderfx;
		//don't show in first person?

		angle = (cg.time / 12 & 255) * (M_PI * 2) / 255;
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, seeker.origin);

		VectorCopy(seeker.origin, seekorg);

		if (cent->currentState.genericenemyindex > MAX_GENTITIES)
		{
			float prefig = (cent->currentState.genericenemyindex - cg.time) / 80;

			if (prefig > 55)
			{
				prefig = 55;
			}
			else if (prefig < 1)
			{
				prefig = 1;
			}

			elevated[2] -= 55 - prefig;

			angle = (cg.time / 12 & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 5;
			VectorAdd(elevated, dir, seeker.origin);
		}
		else if (cent->currentState.genericenemyindex != ENTITYNUM_NONE && cent->currentState.genericenemyindex != -1)
		{
			centity_t* enent = &cg_entities[cent->currentState.genericenemyindex];

			if (enent)
			{
				vec3_t enang;
				VectorSubtract(enent->lerpOrigin, seekorg, enang);
				VectorNormalize(enang);
				vectoangles(enang, angles);
				successchange = 1;
			}
		}

		if (!successchange)
		{
			angles[0] = sin(angle) * 30;
			angles[1] = angle * 180 / M_PI + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
		}

		AnglesToAxis(angles, seeker.axis);

		seeker.hModel = trap->R_RegisterModel("models/items/remote.md3");
		trap->R_AddRefEntityToScene(&seeker);
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if ((cg_shadows.integer == 3 || cg_shadows.integer == 2) && shadow)
	{
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

	// if we've been hit, display proper fullscreen fx
	//CG_PlayerHitFX(cent);

	VectorCopy(cent->lerpOrigin, legs.origin);

	VectorCopy(cent->lerpOrigin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	if (cg_shadows.integer == 2 && renderfx & RF_THIRD_PERSON)
	{
		//can see own shadow
		legs.renderfx |= RF_SHADOW_ONLY;
	}
	VectorCopy(legs.origin, legs.oldorigin); // don't positionally lerp at all

	CG_G2PlayerAngles(cent, legs.axis, root_angles);
	CG_G2PlayerHeadAnims(cent);

	if (cent->currentState.eFlags2 & EF2_HELD_BY_MONSTER
		&& cent->currentState.hasLookTarget)
		//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
	{
		centity_t* rancor = &cg_entities[cent->currentState.lookTarget];
		if (rancor)
		{
			BG_AttachToRancor(rancor->ghoul2, //ghoul2 info
				rancor->lerpAngles[YAW],
				rancor->lerpOrigin,
				cg.time,
				cgs.game_models,
				rancor->modelScale,
				rancor->currentState.eFlags2 & EF2_GENERIC_NPC_FLAG,
				legs.origin,
				legs.angles,
				NULL);

			if (cent->isRagging)
			{
				//hack, ragdoll has you way at bottom of bounding box
				VectorMA(legs.origin, 32, legs.axis[2], legs.origin);
			}
			VectorCopy(legs.origin, legs.oldorigin);
			VectorCopy(legs.origin, legs.lightingOrigin);

			VectorCopy(legs.angles, cent->lerpAngles);
			VectorCopy(cent->lerpAngles, root_angles); //??? tempAngles );//tempAngles is needed a lot below
			VectorCopy(cent->lerpAngles, cent->turAngles);
			VectorCopy(legs.origin, cent->lerpOrigin);
		}
	}
	//This call is mainly just to reconstruct the skeleton. But we'll get the left hand matrix while we're at it.
	//If we don't reconstruct the skeleton after setting the bone angles, we will get bad bolt points on the model
	//(e.g. the weapon model bolt will look "lagged") if there's no other GetBoltMatrix call for the rest of the
	//frame. Yes, this is stupid and needs to be fixed properly.
	//The current solution is to force it not to reconstruct the skeleton for the first GBM call in G2PlayerAngles.
	//It works and we end up only reconstructing it once, but it doesn't seem like the best solution.
	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles, cent->lerpOrigin,
		cg.time,
		cgs.game_models, cent->modelScale);
	got_l_hand_matrix = qtrue;

	//Restrict True View Model changes to the player and do the True View camera view work.
	if (cg.snap && cent->currentState.number == cg.snap->ps.clientNum && cg_truebobbing.integer)
	{
		if (!cg.renderingThirdPerson && (cg_trueguns.integer || cent->currentState.weapon == WP_SABER
			|| cent->currentState.weapon == WP_MELEE) && !cg.predictedPlayerState.zoomMode)
		{
			//<True View varibles
			mdxaBone_t eye_matrix;
			vec3_t eye_angles;
			vec3_t eye_axis[3];
			vec3_t oldeye_origin;
			qhandle_t eyes_bolt;
			qboolean bone_based = qfalse;

			//make the player's be based on the ghoul2 model

			//grab the location data for the "*head_eyes" tag surface
			eyes_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*head_eyes");
			if (!trap->G2API_GetBoltMatrix(cent->ghoul2, 0, eyes_bolt, &eye_matrix, cent->turAngles, cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale))
			{
				//Something prevented you from getting the "*head_eyes" information.  The model probably doesn't have a
				//*head_eyes tag surface.  Try using *head_front instead

				eyes_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*head_front");
				if (!trap->G2API_GetBoltMatrix(cent->ghoul2, 0, eyes_bolt, &eye_matrix, cent->turAngles,
					cent->lerpOrigin,
					cg.time, cgs.game_models, cent->modelScale))
				{
					eyes_bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "reye");
					bone_based = qtrue;
					if (!trap->G2API_GetBoltMatrix(cent->ghoul2, 0, eyes_bolt, &eye_matrix, cent->turAngles,
						cent->lerpOrigin,
						cg.time, cgs.game_models, cent->modelScale))
					{
						goto SkipTrueView;
					}
				}
			}

			//Set the original eye Origin
			VectorCopy(cg.refdef.vieworg, oldeye_origin);

			//set the player's view origin
			BG_GiveMeVectorFromMatrix(&eye_matrix, ORIGIN, cg.refdef.vieworg);

			//Find the orientation of the eye tag surface
			//I based this on coordsys.h that I found at http://www.xs4all.nl/~hkuiper/cwmtx/html/coordsys_8h-source.html
			//According to the file, Harry Kuiper, Will DeVore deserve credit for making that file that I based this on.

			if (bone_based)
			{
				//the eye bone has different default axis orientation than the tag surfaces.
				eye_axis[0][0] = eye_matrix.matrix[0][1];
				eye_axis[1][0] = eye_matrix.matrix[1][1];
				eye_axis[2][0] = eye_matrix.matrix[2][1];

				eye_axis[0][1] = eye_matrix.matrix[0][0];
				eye_axis[1][1] = eye_matrix.matrix[1][0];
				eye_axis[2][1] = eye_matrix.matrix[2][0];

				eye_axis[0][2] = -eye_matrix.matrix[0][2];
				eye_axis[1][2] = -eye_matrix.matrix[1][2];
				eye_axis[2][2] = -eye_matrix.matrix[2][2];
			}
			else
			{
				eye_axis[0][0] = eye_matrix.matrix[0][0];
				eye_axis[1][0] = eye_matrix.matrix[1][0];
				eye_axis[2][0] = eye_matrix.matrix[2][0];

				eye_axis[0][1] = eye_matrix.matrix[0][1];
				eye_axis[1][1] = eye_matrix.matrix[1][1];
				eye_axis[2][1] = eye_matrix.matrix[2][1];

				eye_axis[0][2] = eye_matrix.matrix[0][2];
				eye_axis[1][2] = eye_matrix.matrix[1][2];
				eye_axis[2][2] = eye_matrix.matrix[2][2];
			}

			eye_angles[YAW] = atan2(eye_axis[1][0], eye_axis[0][0]) * 180 / M_PI;

			//I want asin but it's not setup in the libraries so I'm useing the statement asin x = (M_PI / 2) - acos x
			eye_angles[PITCH] = (M_PI / 2 - acos(-eye_axis[2][0])) * 180 / M_PI;
			eye_angles[ROLL] = atan2(eye_axis[2][1], eye_axis[2][2]) * 180 / M_PI;

			//END Find the orientation of the eye tag surface

			//Shift the camera origin by cg_trueeyeposition
			AngleVectors(eye_angles, eye_axis[0], NULL, NULL);
			VectorMA(cg.refdef.vieworg, cg_trueeyeposition.value, eye_axis[0], cg.refdef.vieworg);

			//Trace to see if the bolt eye origin is ok to move to.  If it's not, place it at the last safe position.
			CheckCameraLocation(oldeye_origin);

			//Do all the Eye "movement" and simplified moves here.
			SmoothTrueView(eye_angles);

			//set the player view angles
			VectorCopy(eye_angles, cg.refdef.viewangles);

			//set the player view axis
			AnglesToAxis(cg.refdef.viewangles, cg.refdef.viewaxis);
		}
	}

SkipTrueView:

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//dead = qtrue;
		//rww - since our angles are fixed when we're dead this shouldn't be an issue anyway
		//we need to render the dying/dead player because we are now spawning the body on respawn instead of death
		//return;
	}

	ScaleModelAxis(&legs);

	memset(&torso, 0, sizeof torso);

	//rww - force speed "trail" effect
	if (!(cent->currentState.powerups & 1 << PW_SPEED) || do_alpha || !cg_speedTrail.integer)
	{
		cent->frame_minus1_refreshed = 0;
		cent->frame_minus2_refreshed = 0;
	}

	if (cent->frame_minus1_refreshed ||
		cent->frame_minus2_refreshed)
	{
		vec3_t t_dir;
		int dist_vel_base;

		VectorCopy(cent->currentState.pos.trDelta, t_dir);
		dist_vel_base = SPEED_TRAIL_DISTANCE * (VectorNormalize(t_dir) * 0.004);

		if (cent->frame_minus1_refreshed)
		{
			refEntity_t reframe_minus1 = legs;
			reframe_minus1.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus1.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus1.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus1.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus1.shaderRGBA[3] = 100;

			//rww - if the client gets a bad framerate we will only receive frame positions
			//once per frame anyway, so we might end up with speed trails very spread out.
			//in order to avoid that, we'll get the direction of the last trail from the player
			//and place the trail refent a set distance from the player location this frame
			VectorSubtract(cent->frame_minus1, legs.origin, t_dir);
			VectorNormalize(t_dir);

			cent->frame_minus1[0] = legs.origin[0] + t_dir[0] * dist_vel_base;
			cent->frame_minus1[1] = legs.origin[1] + t_dir[1] * dist_vel_base;
			cent->frame_minus1[2] = legs.origin[2] + t_dir[2] * dist_vel_base;

			VectorCopy(cent->frame_minus1, reframe_minus1.origin);

			//reframe_minus1.customShader = 2;

			trap->R_AddRefEntityToScene(&reframe_minus1);
		}

		if (cent->frame_minus2_refreshed)
		{
			refEntity_t reframe_minus2 = legs;

			reframe_minus2.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus2.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus2.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus2.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus2.shaderRGBA[3] = 50;

			//Same as above but do it between trail points instead of the player and first trail entry
			VectorSubtract(cent->frame_minus2, cent->frame_minus1, t_dir);
			VectorNormalize(t_dir);

			cent->frame_minus2[0] = cent->frame_minus1[0] + t_dir[0] * dist_vel_base;
			cent->frame_minus2[1] = cent->frame_minus1[1] + t_dir[1] * dist_vel_base;
			cent->frame_minus2[2] = cent->frame_minus1[2] + t_dir[2] * dist_vel_base;

			VectorCopy(cent->frame_minus2, reframe_minus2.origin);

			//reframe_minus2.customShader = 2;

			trap->R_AddRefEntityToScene(&reframe_minus2);
		}
	}

	//trigger animation-based sounds, done before next lerp frame.
	CG_TriggerAnimSounds(cent);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		&torso.oldframe, &torso.frame, &torso.backlerp);

	// add the talk baloon or disconnect icon
	CG_PlayerSprites(cent);

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//keep track of death anim frame for when we copy off the bodyqueue
		ci->frame = cent->pe.torso.frame;
	}

	if (cent->currentState.activeForcePass > FORCE_LEVEL_3
		&& cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		matrix3_t axis;
		vec3_t t_ang, f_ang, fx_dir;
		vec3_t ef_org;

		int real_force_lev = cent->currentState.activeForcePass - FORCE_LEVEL_3;

		VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

		VectorSet(f_ang, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0);

		AngleVectors(f_ang, fx_dir, NULL, NULL);

		if ((cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW)
			&& Q_irand(0, 1))
		{
			//alternate back and forth between left and right
			mdxaBone_t r_hand_matrix;
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &r_hand_matrix, cent->turAngles,
				cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			ef_org[0] = r_hand_matrix.matrix[0][3];
			ef_org[1] = r_hand_matrix.matrix[1][3];
			ef_org[2] = r_hand_matrix.matrix[2][3];
		}
		else
		{
			if (!got_l_hand_matrix)
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles,
					cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);
				got_l_hand_matrix = qtrue;
			}
			ef_org[0] = l_hand_matrix.matrix[0][3];
			ef_org[1] = l_hand_matrix.matrix[1][3];
			ef_org[2] = l_hand_matrix.matrix[2][3];
		}

		AnglesToAxis(f_ang, axis);

		if (real_force_lev > FORCE_LEVEL_2)
		{
			//arc
			trap->FX_PlayEntityEffectID(cgs.effects.forceDrainWide, ef_org, axis, -1, -1, -1, -1);
		}
		else
		{
			//line
			trap->FX_PlayEntityEffectID(cgs.effects.forceDrain, ef_org, axis, -1, -1, -1, -1);
		}
	}
	else if (cent->currentState.activeForcePass && cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		//doing the electrocuting
		vec3_t axis[3];
		vec3_t t_ang, f_ang, fx_dir;
		vec3_t ef_org_l; //origin left hand
		vec3_t ef_org_r; //origin right hand

		VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

		VectorSet(f_ang, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0);

		AngleVectors(f_ang, fx_dir, NULL, NULL);

		if (cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
			|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
		{
			mdxaBone_t r_hand_matrix;
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &r_hand_matrix, cent->turAngles,
				cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			ef_org_r[0] = r_hand_matrix.matrix[0][3]; //right hand matrix -> efOrgR
			ef_org_r[1] = r_hand_matrix.matrix[1][3];
			ef_org_r[2] = r_hand_matrix.matrix[2][3];

			if (!got_l_hand_matrix)
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles,
					cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);
				got_l_hand_matrix = qtrue;
			}
			ef_org_l[0] = l_hand_matrix.matrix[0][3]; //left hand matrix -> efOrgL
			ef_org_l[1] = l_hand_matrix.matrix[1][3];
			ef_org_l[2] = l_hand_matrix.matrix[2][3];
		}
		else
		{
			if (!got_l_hand_matrix)
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles,
					cent->lerpOrigin, cg.time, cgs.game_models, cent->modelScale);
				got_l_hand_matrix = qtrue;
			}
			ef_org_l[0] = l_hand_matrix.matrix[0][3]; //just for the simple lightning from the left hand
			ef_org_l[1] = l_hand_matrix.matrix[1][3];
			ef_org_l[2] = l_hand_matrix.matrix[2][3];
		}

		AnglesToAxis(f_ang, axis);

		if (cent->currentState.activeForcePass > FORCE_LEVEL_2)
		{
			//arc
			if (cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
			{
				if (cent->currentState.botclass == BCLASS_DESANN)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.redlightningwide, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_ALORA)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.purplelightningwide, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_TAVION)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.purplelightningwide, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_KYLE)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.yellowlightningwide, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_SHADOWTROOPER)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.greenlightningwide, ef_org_r, axis, -1, -1, -1, -1);
				}
				else
				{
					trap->FX_PlayEntityEffectID(cgs.effects.forceLightningWide, ef_org_r, axis, -1, -1, -1, -1);
				}
			}
			if (cent->currentState.botclass == BCLASS_DESANN)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.redlightningwide, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_ALORA)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.purplelightningwide, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_TAVION)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.purplelightningwide, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_KYLE)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.yellowlightningwide, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_SHADOWTROOPER)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.greenlightningwide, ef_org_l, axis, -1, -1, -1, -1);
			}
			else
			{
				trap->FX_PlayEntityEffectID(cgs.effects.forceLightningWide, ef_org_l, axis, -1, -1, -1, -1);
			}
		}
		else
		{
			//arc
			if (cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
				|| cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
			{
				if (cent->currentState.botclass == BCLASS_DESANN)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.redlightning, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_ALORA)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.purplelightning, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_TAVION)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.purplelightning, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_KYLE)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.yellowlightning, ef_org_r, axis, -1, -1, -1, -1);
				}
				else if (cent->currentState.botclass == BCLASS_SHADOWTROOPER)
				{
					trap->FX_PlayEntityEffectID(cgs.effects.greenlightning, ef_org_r, axis, -1, -1, -1, -1);
				}
				else
				{
					trap->FX_PlayEntityEffectID(cgs.effects.forceLightning, ef_org_r, axis, -1, -1, -1, -1);
				}
			}
			if (cent->currentState.botclass == BCLASS_DESANN)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.redlightning, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_ALORA)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.purplelightning, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_TAVION)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.purplelightning, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_KYLE)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.yellowlightning, ef_org_l, axis, -1, -1, -1, -1);
			}
			else if (cent->currentState.botclass == BCLASS_SHADOWTROOPER)
			{
				trap->FX_PlayEntityEffectID(cgs.effects.greenlightning, ef_org_l, axis, -1, -1, -1, -1);
			}
			else
			{
				trap->FX_PlayEntityEffectID(cgs.effects.forceLightning, ef_org_l, axis, -1, -1, -1, -1);
			}
		}
	}

	if (cent->currentState.PlayerEffectFlags & 1 << PEF_FLAMING/* && cg.snap->ps.forceHandExtend == HANDEXTEND_FLAMETHROWER_HOLD*/)
	{
		//player is firing flamethrower, render effect.
		vec3_t axis[3];
		vec3_t t_ang, f_ang, fx_dir;
		vec3_t ef_org;

		VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

		VectorSet(f_ang, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0);

		AngleVectors(f_ang, fx_dir, NULL, NULL);

		if (!got_l_hand_matrix)
		{
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles,
				cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			got_l_hand_matrix = qtrue;
		}

		ef_org[0] = l_hand_matrix.matrix[0][3];
		ef_org[1] = l_hand_matrix.matrix[1][3];
		ef_org[2] = l_hand_matrix.matrix[2][3];

		AnglesToAxis(f_ang, axis);

		trap->FX_PlayEntityEffectID(cgs.effects.flamethrower, ef_org, axis, -1, -1, -1, -1);
	}

	//fullbody push effect
	if (cent->currentState.eFlags & EF_BODYPUSH)
	{
		if (com_outcast.integer == 0) //JKA
		{
			CG_ForcePushBodyBlur(cent);
		}
		else if (com_outcast.integer == 1) //JKO
		{
			CG_ForcePushBodyBlur(cent);
		}
		else if (com_outcast.integer == 2) // NO EFFECT
		{
			//no effect
		}
		else // BACKUP
		{
			CG_ForcePushBodyBlur(cent);
		}
	}

	if (cent->currentState.powerups & 1 << PW_DISINT_4)
	{
		vec3_t t_ang;
		vec3_t ef_org;

		VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

		if (!got_l_hand_matrix)
		{
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &l_hand_matrix, cent->turAngles,
				cent->lerpOrigin,
				cg.time, cgs.game_models, cent->modelScale);
			got_l_hand_matrix = qtrue;
		}

		ef_org[0] = l_hand_matrix.matrix[0][3];
		ef_org[1] = l_hand_matrix.matrix[1][3];
		ef_org[2] = l_hand_matrix.matrix[2][3];

		if (cent->currentState.forcePowersActive & 1 << FP_GRIP &&
			(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum))
		{
			vec3_t bolt_dir;
			vec3_t orig_bolt;
			VectorCopy(ef_org, orig_bolt);
			BG_GiveMeVectorFromMatrix(&l_hand_matrix, NEGATIVE_Y, bolt_dir);

			CG_ForceGripEffect(ef_org);
		}
		else if (!(cent->currentState.forcePowersActive & 1 << FP_GRIP))
		{
			//use refractive effect
			CG_ForcePushBlur(ef_org, cent);
		}
	}
	else if (cent->bodyFadeTime)
	{
		//reset the counter for keeping track of push refraction effect state
		cent->bodyFadeTime = 0;
	}

	if (cent->currentState.powerups & (1 << PW_SPHERESHIELDED))
	{
		trap->S_AddLoopingSound(cent->currentState.number, cg.refdef.vieworg, vec3_origin, trap->S_RegisterSound("sound/barrier/barrier_loop.wav"));
	}

	if (cent->currentState.weapon == WP_STUN_BATON && cent->currentState.number == cg.snap->ps.clientNum)
	{
		if (cent->currentState.weapon == WP_STUN_BATON && cent->currentState.eFlags & EF_ALT_FIRING)
		{
			trap->S_AddLoopingSound(cent->currentState.number, cg.refdef.vieworg, vec3_origin,
				trap->S_RegisterSound("sound/weapons/concussion/idle_lp.wav"));
		}
		else
		{
			trap->S_AddLoopingSound(cent->currentState.number, cg.refdef.vieworg, vec3_origin,
				trap->S_RegisterSound("sound/weapons/baton/idle.wav"));
		}
	}

	//NOTE: All effects that should be visible during mindtrick should go above here

	if (iwantout)
	{
		goto stillDoSaber;
		//return;
	}
	if (do_alpha)
	{
		legs.renderfx |= RF_FORCE_ENT_ALPHA;
		legs.shaderRGBA[3] = cent->trickAlpha;

		if (legs.shaderRGBA[3] < 1)
		{
			//don't cancel it out even if it's < 1
			legs.shaderRGBA[3] = 1;
		}
	}

	if (cent->teamPowerEffectTime > cg.time)
	{
		if (cent->teamPowerType == 3)
		{
			//absorb is a somewhat different effect entirely
			//Guess I'll take care of it where it's always been, just checking these values instead.
		}
		else
		{
			vec4_t pre_col;
			int pre_rfx;

			pre_rfx = legs.renderfx;

			legs.renderfx |= RF_RGB_TINT;
			legs.renderfx |= RF_FORCE_ENT_ALPHA;

			pre_col[0] = legs.shaderRGBA[0];
			pre_col[1] = legs.shaderRGBA[1];
			pre_col[2] = legs.shaderRGBA[2];
			pre_col[3] = legs.shaderRGBA[3];

			if (cent->teamPowerType == 1)
			{
				//heal
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if (cent->teamPowerType == 0)
			{
				//regen
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 255;
			}
			else
			{
				//drain
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 0;
			}

			legs.shaderRGBA[3] = (cent->teamPowerEffectTime - cg.time) / 8;

			legs.customShader = trap->R_RegisterShader("powerups/ysalimarishell");
			trap->R_AddRefEntityToScene(&legs);

			legs.customShader = 0;
			legs.renderfx = pre_rfx;
			legs.shaderRGBA[0] = pre_col[0];
			legs.shaderRGBA[1] = pre_col[1];
			legs.shaderRGBA[2] = pre_col[2];
			legs.shaderRGBA[3] = pre_col[3];
		}
	}

	//If you've tricked this client.
	if (CG_IsMindTricked(cg.snap->ps.fd.forceMindtrickTargetIndex,
		cg.snap->ps.fd.forceMindtrickTargetIndex2,
		cg.snap->ps.fd.forceMindtrickTargetIndex3,
		cg.snap->ps.fd.forceMindtrickTargetIndex4,
		cent->currentState.number))
	{
		if (cent->ghoul2)
		{
			vec3_t ef_org;
			vec3_t t_ang, fx_ang;
			matrix3_t axis;

			//VectorSet( tAng, 0, cent->pe.torso.yawAngle, 0 );
			VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &boltMatrix, t_ang, cent->lerpOrigin, cg.time,
				cgs.game_models, cent->modelScale);

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, ef_org);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fx_ang);

			axis[0][0] = boltMatrix.matrix[0][0];
			axis[0][1] = boltMatrix.matrix[1][0];
			axis[0][2] = boltMatrix.matrix[2][0];

			axis[1][0] = boltMatrix.matrix[0][1];
			axis[1][1] = boltMatrix.matrix[1][1];
			axis[1][2] = boltMatrix.matrix[2][1];

			axis[2][0] = boltMatrix.matrix[0][2];
			axis[2][1] = boltMatrix.matrix[1][2];
			axis[2][2] = boltMatrix.matrix[2][2];

			trap->FX_PlayEntityEffectID(cgs.effects.mForceConfustionOld, ef_org, axis, -1, -1, -1, -1);
		}
	}

	if (cgs.gametype == GT_HOLOCRON && cent->currentState.time2 &&
		(cg.renderingThirdPerson || cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER || cg.
			predictedPlayerState.weapon == WP_MELEE
			|| cg.snap->ps.clientNum != cent->currentState.number))
	{
		int i = 0;
		int rendered_holos = 0;
		refEntity_t holo_ref;

		while (i < NUM_FORCE_POWERS && rendered_holos < 3)
		{
			if (cent->currentState.time2 & 1 << i)
			{
				memset(&holo_ref, 0, sizeof holo_ref);

				VectorCopy(cent->lerpOrigin, elevated);
				elevated[2] += 8;

				VectorCopy(elevated, holo_ref.lightingOrigin);
				holo_ref.shadowPlane = shadowPlane;
				holo_ref.renderfx = 0; //RF_THIRD_PERSON;

				if (rendered_holos == 0)
				{
					angle = (cg.time / 8 & 255) * (M_PI * 2) / 255;
					dir[0] = cos(angle) * 20;
					dir[1] = sin(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holo_ref.origin);

					angles[0] = sin(angle) * 30;
					angles[1] = angle * 180 / M_PI + 90;
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis(angles, holo_ref.axis);
				}
				else if (rendered_holos == 1)
				{
					angle = (cg.time / 8 & 255) * (M_PI * 2) / 255 + M_PI;
					if (angle > M_PI * 2)
						angle -= M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holo_ref.origin);

					angles[0] = cos(angle - 0.5 * M_PI) * 30;
					angles[1] = 360 - angle * 180 / M_PI;
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis(angles, holo_ref.axis);
				}
				else
				{
					angle = (cg.time / 6 & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
					if (angle > M_PI * 2)
						angle -= M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = 0;
					VectorAdd(elevated, dir, holo_ref.origin);

					VectorCopy(dir, holo_ref.axis[1]);
					VectorNormalize(holo_ref.axis[1]);
					VectorSet(holo_ref.axis[2], 0, 0, 1);
					CrossProduct(holo_ref.axis[1], holo_ref.axis[2], holo_ref.axis[0]);
				}

				holo_ref.modelScale[0] = 0.5;
				holo_ref.modelScale[1] = 0.5;
				holo_ref.modelScale[2] = 0.5;
				ScaleModelAxis(&holo_ref);

				{
					float wv;
					addspriteArgStruct_t fx_s_args;
					vec3_t holo_center;

					holo_center[0] = holo_ref.origin[0] + holo_ref.axis[2][0] * 18;
					holo_center[1] = holo_ref.origin[1] + holo_ref.axis[2][1] * 18;
					holo_center[2] = holo_ref.origin[2] + holo_ref.axis[2][2] * 18;

					wv = sin(cg.time * 0.004f) * 0.08f + 0.1f;

					VectorCopy(holo_center, fx_s_args.origin);
					VectorClear(fx_s_args.vel);
					VectorClear(fx_s_args.accel);
					fx_s_args.scale = wv * 60;
					fx_s_args.dscale = wv * 60;
					fx_s_args.sAlpha = wv * 12;
					fx_s_args.eAlpha = wv * 12;
					fx_s_args.rotation = 0.0f;
					fx_s_args.bounce = 0.0f;
					fx_s_args.life = 1.0f;

					fx_s_args.flags = 0x08000000 | 0x00000001;

					if (force_power_dark_light[i] == FORCE_DARKSIDE)
					{
						//dark
						fx_s_args.sAlpha *= 3;
						fx_s_args.eAlpha *= 3;
						fx_s_args.shader = cgs.media.redSaberGlowShader;
						trap->FX_AddSprite(&fx_s_args);
					}
					else if (force_power_dark_light[i] == FORCE_LIGHTSIDE)
					{
						//light
						fx_s_args.sAlpha *= 1.5;
						fx_s_args.eAlpha *= 1.5;
						fx_s_args.shader = cgs.media.redSaberGlowShader;
						trap->FX_AddSprite(&fx_s_args);
						fx_s_args.shader = cgs.media.greenSaberGlowShader;
						trap->FX_AddSprite(&fx_s_args);
						fx_s_args.shader = cgs.media.blueSaberGlowShader;
						trap->FX_AddSprite(&fx_s_args);
					}
					else
					{
						//neutral
						if (i == FP_SABER_OFFENSE ||
							i == FP_SABER_DEFENSE ||
							i == FP_SABERTHROW)
						{
							//saber power
							fx_s_args.sAlpha *= 1.5;
							fx_s_args.eAlpha *= 1.5;
							fx_s_args.shader = cgs.media.greenSaberGlowShader;
							trap->FX_AddSprite(&fx_s_args);
						}
						else
						{
							fx_s_args.sAlpha *= 0.5;
							fx_s_args.eAlpha *= 0.5;
							fx_s_args.shader = cgs.media.greenSaberGlowShader;
							trap->FX_AddSprite(&fx_s_args);
							fx_s_args.shader = cgs.media.blueSaberGlowShader;
							trap->FX_AddSprite(&fx_s_args);
						}
					}
				}

				holo_ref.hModel = trap->R_RegisterModel(forceHolocronModels[i]);
				trap->R_AddRefEntityToScene(&holo_ref);

				rendered_holos++;
			}
			i++;
		}
	}

	if (cent->currentState.powerups & 1 << PW_YSALAMIRI ||
		cgs.gametype == GT_CTY && (cent->currentState.powerups & 1 << PW_REDFLAG ||
			cent->currentState.powerups & 1 << PW_BLUEFLAG))
	{
		if (cgs.gametype == GT_CTY && cent->currentState.powerups & 1 << PW_REDFLAG)
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliredShader);
		}
		else if (cgs.gametype == GT_CTY && cent->currentState.powerups & 1 << PW_BLUEFLAG)
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliblueShader);
		}
		else
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysalimariShader);
		}
	}

	if (cent->currentState.powerups & 1 << PW_GALAK_SHIELD)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.galakShader);
	}

	if (cent->currentState.powerups & 1 << PW_FORCE_BOON)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.boonShader);
	}

	if (cent->currentState.powerups & 1 << PW_FORCE_ENLIGHTENED_DARK)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.endarkenmentShader);
	}
	else if (cent->currentState.powerups & 1 << PW_FORCE_ENLIGHTENED_LIGHT)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.enlightenmentShader);
	}

	if (cent->currentState.eFlags & EF_INVULNERABLE)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.0f, cgs.media.invulnerabilityShader);
	}
	health = cg.snap->ps.stats[STAT_HEALTH];

	if (cent->currentState.powerups & 1 << PW_INVINCIBLE
		&& cent->currentState.groundEntityNum != ENTITYNUM_NONE
		&& health > 1)
	{
		if (cent->ghoul2)
		{
			vec3_t ef_org;
			vec3_t t_ang, fx_ang;
			matrix3_t axis;

			VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &boltMatrix, t_ang, cent->lerpOrigin, cg.time,
				cgs.game_models, cent->modelScale);

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, ef_org);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fx_ang);

			axis[0][0] = boltMatrix.matrix[0][0];
			axis[0][1] = boltMatrix.matrix[1][0];
			axis[0][2] = boltMatrix.matrix[2][0];

			axis[1][0] = boltMatrix.matrix[0][1];
			axis[1][1] = boltMatrix.matrix[1][1];
			axis[1][2] = boltMatrix.matrix[2][1];

			axis[2][0] = boltMatrix.matrix[0][2];
			axis[2][1] = boltMatrix.matrix[1][2];
			axis[2][2] = boltMatrix.matrix[2][2];

			trap->FX_PlayEntityEffectID(cgs.effects.forceInvincibility, ef_org, axis, -1, -1, -1, -1);
		}
	}

	if (cent->currentState.powerups & 1 << PW_MEDITATE
		&& health > 1)
	{
		if (cent->ghoul2)
		{
			vec3_t ef_org;
			vec3_t t_ang, fx_ang;
			matrix3_t axis;

			VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &boltMatrix, t_ang, cent->lerpOrigin, cg.time,
				cgs.game_models, cent->modelScale);

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, ef_org);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fx_ang);

			axis[0][0] = boltMatrix.matrix[0][0];
			axis[0][1] = boltMatrix.matrix[1][0];
			axis[0][2] = boltMatrix.matrix[2][0];

			axis[1][0] = boltMatrix.matrix[0][1];
			axis[1][1] = boltMatrix.matrix[1][1];
			axis[1][2] = boltMatrix.matrix[2][1];

			axis[2][0] = boltMatrix.matrix[0][2];
			axis[2][1] = boltMatrix.matrix[1][2];
			axis[2][2] = boltMatrix.matrix[2][2];

			trap->FX_PlayEntityEffectID(cgs.effects.forceInvincibility, ef_org, axis, -1, -1, -1, -1);
		}
	}

stillDoSaber:
	if (cent->currentState.eFlags & EF_DEAD && cent->currentState.weapon == WP_SABER)
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		draw_player_saber = qtrue;
	}
	else if (cent->currentState.weapon == WP_SABER)
	{
		if (cent->currentState.saberHolstered < 2 &&
			(!cent->currentState.saberInFlight //saber not in flight
				|| ci->saber[1].soundLoop) //??? //racc - or we have dual sabers
			&& !(cent->currentState.eFlags & EF_DEAD)) //still alive
		{
			vec3_t sound_spot;
			qboolean did_first_sound = qfalse;

			if (cg.snap->ps.clientNum == cent->currentState.number)
			{
				VectorCopy(cg.refdef.vieworg, sound_spot);
			}
			else
			{
				VectorCopy(cent->lerpOrigin, sound_spot);
			}

			if (ci->saber[0].model[0]
				&& ci->saber[0].soundLoop
				&& !cent->currentState.saberInFlight)
			{
				int i = 0;
				qboolean has_len = qfalse;

				while (i < ci->saber[0].numBlades)
				{
					if (ci->saber[0].blade[i].length)
					{
						has_len = qtrue;
						break;
					}
					i++;
				}

				if (has_len)
				{
					trap->S_AddLoopingSound(cent->currentState.number, sound_spot, vec3_origin,
						ci->saber[0].soundLoop);
					did_first_sound = qtrue;
				}
			}
			if (ci->saber[1].model[0]
				&& ci->saber[1].soundLoop
				&& (!did_first_sound || ci->saber[0].soundLoop != ci->saber[1].soundLoop))
			{
				int i = 0;
				qboolean has_len = qfalse;

				while (i < ci->saber[1].numBlades)
				{
					if (ci->saber[1].blade[i].length)
					{
						has_len = qtrue;
						break;
					}
					i++;
				}

				if (has_len)
				{
					trap->S_AddLoopingSound(cent->currentState.number, sound_spot, vec3_origin,
						ci->saber[1].soundLoop);
				}
			}
		}

		if (iwantout
			&& !cent->currentState.saberInFlight)
		{
			if (cent->currentState.eFlags & EF_DEAD)
			{
				if (cent->ghoul2
					&& cent->currentState.saberInFlight
					&& g2_has_weapon)
				{
					//special case, kill the saber on a freshly dead player if another source says to.
					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 1);
					g2_has_weapon = qfalse;
				}
			}
			return;
			//goto endOfCall;
		}

		if (g2_has_weapon
			&& cent->currentState.saberInFlight)
		{
			//keep this set, so we don't re-unholster the thing when we get it back, even if it's knocked away.
			cent->saberWasInFlight = qtrue;
		}

		if (cent->currentState.saberInFlight
			&& cent->currentState.saberEntityNum)
		{
			centity_t* saberEnt;

			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (g2_has_weapon || !cent->bolt3 ||
				saberEnt->serverSaberHitIndex != saberEnt->currentState.modelIndex)
			{
				//saber is in flight, do not have it as a standard weapon model
				qboolean add_bolts = qfalse;
				mdxaBone_t bolt_mat;

				if (g2_has_weapon)
				{
					//ah well, just stick it over the right hand right now.
					trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &bolt_mat, cent->turAngles,
						cent->lerpOrigin,
						cg.time, cgs.game_models, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&bolt_mat, ORIGIN, saberEnt->currentState.pos.trBase);

					trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 1);
					g2_has_weapon = qfalse;
				}

				saberEnt->currentState.pos.trTime = cg.time;
				saberEnt->currentState.apos.trTime = cg.time;

				VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
				VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);

				cent->bolt3 = saberEnt->currentState.apos.trBase[0];
				if (!cent->bolt3)
				{
					cent->bolt3 = 1;
				}
				cent->bolt2 = 0;

				saberEnt->currentState.bolt2 = 123;

				if (saberEnt->ghoul2 &&
					saberEnt->serverSaberHitIndex == saberEnt->currentState.modelIndex)
				{
					// now set up the gun bolt on it
					add_bolts = qtrue;
				}
				else
				{
					const char* saber_model = CG_ConfigString(CS_MODELS + saberEnt->currentState.modelIndex);

					saberEnt->serverSaberHitIndex = saberEnt->currentState.modelIndex;

					if (saberEnt->ghoul2)
					{
						//clean if we already have one (because server changed model string index)
						trap->G2API_CleanGhoul2Models(&saberEnt->ghoul2);
						saberEnt->ghoul2 = 0;
					}

					if (saber_model && saber_model[0])
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, saber_model, 0, 0, 0, 0, 0);
					}
					else if (ci->saber[0].model[0])
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, ci->saber[0].model, 0, 0, 0, 0, 0);
					}
					else
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, DEFAULT_SABER_MODEL, 0, 0, 0, 0, 0);
					}

					if (saberEnt->ghoul2)
					{
						add_bolts = qtrue;

						VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
						VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);
						saberEnt->currentState.pos.trTime = cg.time;
						saberEnt->currentState.apos.trTime = cg.time;
					}
				}

				if (add_bolts)
				{
					int m = 0;
					int tag_bolt;
					char* tag_name;

					while (m < ci->saber[0].numBlades)
					{
						tag_name = va("*blade%i", m + 1);
						tag_bolt = trap->G2API_AddBolt(saberEnt->ghoul2, 0, tag_name);

						if (tag_bolt == -1)
						{
							if (m == 0)
							{
								//guess this is an 0ldsk3wl saber
								tag_bolt = trap->G2API_AddBolt(saberEnt->ghoul2, 0, "*flash");

								if (tag_bolt == -1)
								{
									assert(0);
								}
								break;
							}

							if (tag_bolt == -1)
							{
								assert(0);
								break;
							}
						}

						m++;
					}
				}
			}

			if (saberEnt && saberEnt->ghoul2)
			{
				vec3_t blade_angles;
				vec3_t t_ang;
				vec3_t ef_org;
				float wv;
				addspriteArgStruct_t fx_s_args;

				if (!cent->bolt2)
				{
					cent->bolt2 = cg.time;
				}

				if (cent->bolt3 != 90)
				{
					if (cent->bolt3 < 90)
					{
						cent->bolt3 += (cg.time - cent->bolt2) * 0.5f;

						if (cent->bolt3 > 90)
						{
							cent->bolt3 = 90;
						}
					}
					else if (cent->bolt3 > 90)
					{
						cent->bolt3 -= (cg.time - cent->bolt2) * 0.5f;

						if (cent->bolt3 < 90)
						{
							cent->bolt3 = 90;
						}
					}
				}

				cent->bolt2 = cg.time;

				saberEnt->currentState.apos.trBase[0] = cent->bolt3;
				saberEnt->lerpAngles[0] = cent->bolt3;

				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{
					//owner is pulling is back
					if (!(ci->saber[0].saberFlags & SFL_RETURN_DAMAGE)
						|| cent->currentState.saberHolstered)
					{
						vec3_t owndir;

						VectorSubtract(saberEnt->lerpOrigin, cent->lerpOrigin, owndir);
						VectorNormalize(owndir);

						vectoangles(owndir, owndir);

						owndir[0] += 90;

						if (!(saberEnt->currentState.eFlags & EF_MISSILE_STICK))
						{
							//As long as your not stuck orient towards the player
							VectorCopy(owndir, saberEnt->currentState.apos.trBase);
							VectorCopy(owndir, saberEnt->lerpAngles);
							VectorClear(saberEnt->currentState.apos.trDelta);
						}
					}
				}

				//We don't actually want to rely entirely on server updates to render the position of the saber, because we actually know generally where
				//it's going to be before the first position update even gets here, and it needs to start getting rendered the instant the saber model is
				//removed from the player hand. So we'll just render it manually and let normal rendering for the entity be ignored.
				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{
					//tell it that we're a saber and to render the glow around our handle because we're being pulled back
					saberEnt->bolt3 = 999;
				}

				saberEnt->currentState.modelGhoul2 = 1;
				CG_ManualEntityRender(saberEnt);
				saberEnt->bolt3 = 0;
				saberEnt->currentState.modelGhoul2 = 127;

				VectorCopy(saberEnt->lerpAngles, blade_angles);
				blade_angles[ROLL] = 0;

				//racc - set blade length of first saber for rendering
				if (cent->currentState.saberHolstered == 0)
				{
					BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				}

				// basejka code
				if (ci->saber[0].numBlades > 1 //staff
					&& cent->currentState.saberHolstered == 1) //extra blades off
				{
					//only first blade should be on
					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
				}

				//racc - set render for the second saber's blade as needed
				if (ci->saber[1].model //dual sabers
					&& cent->currentState.saberHolstered == 1) //second one off
				{
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
				}
				if (cent->currentState.saberHolstered < 2)
				{
					int l = 0;
					int k = 0;
					//racc - saber blades are active, render them for thrown sabers.
					//Only want to do for the first saber actually, it's the one in flight.
					while (l < 1)
					{
						if (!ci->saber[l].model[0])
						{
							break;
						}

						k = 0;
						while (k < ci->saber[l].numBlades)
						{
							if (l == 0 //first saber
								&& cent->currentState.saberHolstered == 1 //extra blades should be off
								&& k > 0) //this is an extra blade
							{
								//extra blades off
								//don't draw them
								CG_AddSaberBlade(cent, saberEnt, 0, l, k, saberEnt->lerpOrigin, blade_angles, qtrue,
									qtrue);
							}
							else
							{
								CG_AddSaberBlade(cent, saberEnt, 0, l, k, saberEnt->lerpOrigin, blade_angles, qtrue,
									qfalse);
							}

							k++;
						}
						if (ci->saber[l].numBlades > 2)
						{
							//add a single glow for the saber based on all the blade colors combined
							CG_DoSaberLight(&ci->saber[l], cent->currentState.clientNum, l);
						}

						l++;
					}
				}

				//Make the player's hand glow while guiding the saber
				VectorSet(t_ang, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL]);

				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMatrix, t_ang, cent->lerpOrigin,
					cg.time,
					cgs.game_models, cent->modelScale);

				ef_org[0] = boltMatrix.matrix[0][3];
				ef_org[1] = boltMatrix.matrix[1][3];
				ef_org[2] = boltMatrix.matrix[2][3];

				wv = sin(cg.time * 0.003f) * 0.08f + 0.1f;

				VectorCopy(ef_org, fx_s_args.origin);
				VectorClear(fx_s_args.vel);
				VectorClear(fx_s_args.accel);
				fx_s_args.scale = 8.0f;
				fx_s_args.dscale = 8.0f;
				fx_s_args.sAlpha = wv;
				fx_s_args.eAlpha = wv;
				fx_s_args.rotation = 0.0f;
				fx_s_args.bounce = 0.0f;
				fx_s_args.life = 1.0f;
				fx_s_args.shader = cgs.media.yellowDroppedSaberShader;
				fx_s_args.flags = 0x08000000;
				trap->FX_AddSprite(&fx_s_args);
			}
		}
		else
		{
			//saber is not in a throw.
			if (cent->currentState.saberHolstered == 2)
			{
				//all blades off
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
			}
			else
			{
				//racc - at least some of our blades are active.
				if (ci->saber[0].numBlades > 1 //staff
					&& cent->currentState.saberHolstered == 1) //extra blades off
				{
					//only first blade should be on
					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
				}
				if (ci->saber[1].model //dual sabers
					&& cent->currentState.saberHolstered == 1) //second one off
				{
					if (cent->currentState.saberInFlight && !cent->currentState.saberEntityNum)
					{
						BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
						BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
					}
					else
					{
						BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
					}
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
				}
			}
		}
		//Leaving right arm on, at least for now.
		if (cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM))
		{
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}
		draw_player_saber = qtrue;
	}
	else
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		BG_SI_SetLength(&ci->saber[0], 0);
		BG_SI_SetLength(&ci->saber[1], 0);
	}

#ifdef _RAG_BOLT_TESTING
	if (cent->currentState.eFlags & EF_RAG)
	{
		CG_TempTestFunction(cent, cent->turAngles);
	}
#endif

	if (cent->currentState.weapon == WP_SABER)
	{
		BG_SI_SetLengthGradual(&ci->saber[0], cg.time);
		BG_SI_SetLengthGradual(&ci->saber[1], cg.time);
	}

	if (draw_player_saber)
	{
		centity_t* saberEnt;
		int k = 0;
		int l = 0;

		if (!cent->currentState.saberEntityNum)
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}
		else if (!cent->currentState.saberInFlight)
		{
			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (!g2_has_weapon)
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0, cent->ghoul2, 1);

				if (saberEnt && saberEnt->ghoul2)
				{
					trap->G2API_CleanGhoul2Models(&saberEnt->ghoul2);
				}

				saberEnt->currentState.modelIndex = 0;
				saberEnt->ghoul2 = NULL;
				VectorClear(saberEnt->currentState.pos.trBase);
			}

			cent->bolt3 = 0;
			cent->bolt2 = 0;
		}
		else
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}

		while (l < MAX_SABERS)
		{
			k = 0;

			if (!ci->saber[l].model[0])
			{
				break;
			}

			if (cent->currentState.eFlags2 & EF2_HELD_BY_MONSTER)
			{
#if 0
				if (cent->currentState.hasLookTarget)//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
				{
					centity_t* rancor = &cg_entities[cent->currentState.lookTarget];
					if (rancor && rancor->ghoul2)
					{
						BG_AttachToRancor(rancor->ghoul2, //ghoul2 info
							rancor->lerpAngles[YAW],
							rancor->lerpOrigin,
							cg.time,
							cgs.game_models,
							rancor->modelScale,
							(rancor->currentState.eFlags2 & EF2_GENERIC_NPC_FLAG),
							legs.origin,
							rootAngles,
							NULL);
					}
				}
#else
				vectoangles(legs.axis[0], root_angles);
#endif
			}

			while (k < ci->saber[l].numBlades)
			{
				if (cent->currentState.saberHolstered == 1 //extra blades should be off
					&& k > 0 //this is an extra blade
					&& ci->saber[l].blade[k].length <= 0) //it's completely off
				{
					//extra blades off
					//don't draw them
					CG_AddSaberBlade(cent, cent, 0, l, k, legs.origin, root_angles, qfalse, qtrue);
				}
				else if (ci->saber[1].model[0] //we have a second saber
					&& cent->currentState.saberHolstered == 1 //it should be off
					&& l > 0 //and this is the second one
					&& ci->saber[l].blade[k].length <= 0) //it's completely off
				{
					//second saber is turned off and this blade is done with turning off
					CG_AddSaberBlade(cent, cent, 0, l, k, legs.origin, root_angles, qfalse, qtrue);
				}
				else
				{
					CG_AddSaberBlade(cent, cent, 0, l, k, legs.origin, root_angles, qfalse, qfalse);
				}

				k++;
			}
			if (ci->saber[l].numBlades > 2)
			{
				//add a single glow for the saber based on all the blade colors combined
				CG_DoSaberLight(&ci->saber[l], cent->currentState.clientNum, l);
			}

			l++;
		}
	}

	if (cent->currentState.saberInFlight && !cent->currentState.saberEntityNum)
	{
		//reset the length if the saber is knocked away
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		if (g2_has_weapon && cent->ghoul2weapon == CG_G2WeaponInstance(cent, WP_SABER))
		{
			//and remember to kill the bolton model in case we didn't get a thrown saber update first
			trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 1);
			g2_has_weapon = qfalse;
		}
		cent->bolt3 = 0;
		cent->bolt2 = 0;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		if (cent->ghoul2 && cent->currentState.saberInFlight && g2_has_weapon)
		{
			//special case, kill the saber on a freshly dead player if another source says to.
			trap->G2API_RemoveGhoul2Model(&cent->ghoul2, 1);
			g2_has_weapon = qfalse;
		}
	}

	if (iwantout)
	{
		return;
		//goto endOfCall;
	}

	if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 0;
		legs.renderfx |= RF_MINLIGHT;
	}

	//if (cg.snap->ps.duelInProgress /*&& cent->currentState.number != cg.snap->ps.clientNum*/)
	//{ //I guess go ahead and glow your own client too in a duel
	//	if (cent->currentState.number != cg.snap->ps.duelIndex &&
	//		cent->currentState.number != cg.snap->ps.clientNum)
	//	{ //everyone not involved in the duel is drawn very dark
	//		legs.shaderRGBA[0] /= 5.0f;
	//		legs.shaderRGBA[1] /= 5.0f;
	//		legs.shaderRGBA[2] /= 5.0f;
	//		legs.renderfx |= RF_RGB_TINT;
	//	}
	//	else
	//	{ //adjust the glow by how far away you are from your dueling partner
	//		centity_t *duelEnt;

	//		duelEnt = &cg_entities[cg.snap->ps.duelIndex];

	//		if (duelEnt)
	//		{
	//			vec3_t vecSub;
	//			float subLen = 0;

	//			VectorSubtract(duelEnt->lerpOrigin, cg.snap->ps.origin, vecSub);
	//			subLen = VectorLength(vecSub);

	//			if (subLen < 1)
	//			{
	//				subLen = 1;
	//			}

	//			if (subLen > 1020)
	//			{
	//				subLen = 1020;
	//			}

	//			{
	//				unsigned char savRGBA[3];
	//				savRGBA[0] = legs.shaderRGBA[0];
	//				savRGBA[1] = legs.shaderRGBA[1];
	//				savRGBA[2] = legs.shaderRGBA[2];
	//				legs.shaderRGBA[0] = Q_max(255-subLen/4,1);
	//				legs.shaderRGBA[1] = Q_max(255-subLen/4,1);
	//				legs.shaderRGBA[2] = Q_max(255-subLen/4,1);

	//				legs.renderfx &= ~RF_RGB_TINT;
	//				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
	//				legs.customShader = cgs.media.forceShell;

	//				trap->R_AddRefEntityToScene( &legs );	//draw the shell

	//				legs.customShader = 0;	//reset to player model

	//				legs.shaderRGBA[0] = Q_max(savRGBA[0]-subLen/8,1);
	//				legs.shaderRGBA[1] = Q_max(savRGBA[1]-subLen/8,1);
	//				legs.shaderRGBA[2] = Q_max(savRGBA[2]-subLen/8,1);
	//			}

	//			if (subLen <= 1024)
	//			{
	//				legs.renderfx |= RF_RGB_TINT;
	//			}
	//		}
	//	}
	//}
	//else
	//{
	//	if (cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->currentState.number))
	//	{
	//		legs.shaderRGBA[0] = 50;
	//		legs.shaderRGBA[1] = 50;
	//		legs.shaderRGBA[2] = 50;
	//		legs.renderfx |= RF_RGB_TINT;
	//	}
	//}

	if (cent->currentState.eFlags & EF_DISINTEGRATION)
	{
		if (!cent->dustTrailTime)
		{
			cent->dustTrailTime = cg.time;
			cent->miscTime = legs.frame;
		}

		if (cg.time - cent->dustTrailTime > 1500)
		{
			//avoid rendering the entity after disintegration has finished anyway
			//goto endOfCall;
			return;
		}

		trap->G2API_SetBoneAnim(legs.ghoul2, 0, "model_root", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE,
			1.0f, cg.time, cent->miscTime, -1);

		if (!cent->noLumbar)
		{
			trap->G2API_SetBoneAnim(legs.ghoul2, 0, "lower_lumbar", cent->miscTime, cent->miscTime,
				BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);

			if (cent->localAnimIndex <= 1)
			{
				trap->G2API_SetBoneAnim(legs.ghoul2, 0, "Motion", cent->miscTime, cent->miscTime,
					BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);
			}
		}

		CG_Disintegration(cent, &legs);

		//goto endOfCall;
		return;
	}
	cent->dustTrailTime = 0;
	cent->miscTime = 0;

	if (cent->currentState.powerups & 1 << PW_CLOAKED)
	{
		if (!cent->cloaked)
		{
			cent->cloaked = qtrue;
			cent->uncloaking = cg.time + 2000;
		}
	}
	else if (cent->cloaked)
	{
		cent->cloaked = qfalse;
		cent->uncloaking = cg.time + 2000;
	}

	if (cent->uncloaking > cg.time)
	{
		//in the middle of cloaking
		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{
			//just draw him
			trap->R_AddRefEntityToScene(&legs);
		}
		else
		{
			float perc = (float)(cent->uncloaking - cg.time) / 2000.0f;
			if (cent->currentState.powerups & 1 << PW_CLOAKED)
			{
				//actually cloaking, so reverse it
				perc = 1.0f - perc;
			}

			if (perc >= 0.0f && perc <= 1.0f)
			{
				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
				legs.renderfx |= RF_RGB_TINT;
				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255.0f * perc;
				legs.shaderRGBA[3] = 0;
				legs.customShader = cgs.media.cloakedShader;
				trap->R_AddRefEntityToScene(&legs);

				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255;
				legs.shaderRGBA[3] = 255 * (1.0f - perc); // let model alpha in
				legs.customShader = 0; // use regular skin
				legs.renderfx &= ~RF_RGB_TINT;
				legs.renderfx |= RF_FORCE_ENT_ALPHA;
				trap->R_AddRefEntityToScene(&legs);
			}
		}
	}
	else if (cent->currentState.powerups & 1 << PW_CLOAKED)
	{
		//fully cloaked
		if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{
			//just draw him
			trap->R_AddRefEntityToScene(&legs);
		}
		else
		{
			if (cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
			{
				if (cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4 && cg_renderToTextureFX.integer)
				{
					trap->R_SetRefractionProperties(1.0f, 0.0f, qfalse, qfalse);
					//don't need to do this every frame.. but..
					legs.customShader = 2; //crazy "refractive" shader
					trap->R_AddRefEntityToScene(&legs);
					legs.customShader = 0;
				}
				else
				{
					//stencil buffer's in use, sorry
					legs.renderfx = 0; //&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
					legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = legs.shaderRGBA[3] = 255;
					legs.customShader = cgs.media.cloakedShader;
					trap->R_AddRefEntityToScene(&legs);
					legs.customShader = 0;
				}
			}
		}
	}

	if (!(cent->currentState.powerups & 1 << PW_CLOAKED))
	{
		//don't add the normal model if cloaked
		CG_CheckThirdPersonAlpha(cent, &legs);
		trap->R_AddRefEntityToScene(&legs);
	}

	//cent->frame_minus2 = cent->frame_minus1;
	VectorCopy(cent->frame_minus1, cent->frame_minus2);

	if (cent->frame_minus1_refreshed)
	{
		cent->frame_minus2_refreshed = 1;
	}

	//cent->frame_minus1 = legs;
	VectorCopy(legs.origin, cent->frame_minus1);

	cent->frame_minus1_refreshed = 1;

	if (!cent->frame_hold_refreshed && cent->currentState.powerups & 1 << PW_SPEEDBURST)
	{
		cent->frame_hold_time = cg.time + 254;
	}

	if (cent->frame_hold_time >= cg.time)
	{
		refEntity_t reframe_hold;

		if (!cent->frame_hold_refreshed)
		{
			//We're taking the ghoul2 instance from the original refent and duplicating it onto our refent alias so that we can then freeze the frame and fade it for the effect
			if (cent->frame_hold && trap->G2_HaveWeGhoul2Models(cent->frame_hold) &&
				cent->frame_hold != cent->ghoul2)
			{
				trap->G2API_CleanGhoul2Models(&cent->frame_hold);
			}
			reframe_hold = legs;
			cent->frame_hold_refreshed = 1;
			reframe_hold.ghoul2 = NULL;

			trap->G2API_DuplicateGhoul2Instance(cent->ghoul2, &cent->frame_hold);

			//Set the animation to the current frame and freeze on end
			//trap->G2API_SetBoneAnim(cent->frame_hold.ghoul2, 0, "model_root", cent->frame_hold.frame, cent->frame_hold.frame, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->frame_hold.frame, -1);
			trap->G2API_SetBoneAnim(cent->frame_hold, 0, "model_root", legs.frame, legs.frame, 0, 1.0f, cg.time,
				legs.frame, -1);
		}
		else
		{
			reframe_hold = legs;
			reframe_hold.ghoul2 = cent->frame_hold;
		}

		reframe_hold.renderfx |= RF_FORCE_ENT_ALPHA;
		reframe_hold.shaderRGBA[3] = cent->frame_hold_time - cg.time;
		if (reframe_hold.shaderRGBA[3] > 254)
		{
			reframe_hold.shaderRGBA[3] = 254;
		}
		if (reframe_hold.shaderRGBA[3] < 1)
		{
			reframe_hold.shaderRGBA[3] = 1;
		}

		reframe_hold.ghoul2 = cent->frame_hold;
		trap->R_AddRefEntityToScene(&reframe_hold);
	}
	else
	{
		cent->frame_hold_refreshed = 0;
	}

	if (cent->currentState.powerups & (1 << PW_SPHERESHIELDED))
	{
		if (!cent->sphereshielded)
		{
			cent->sphereshielded = qtrue;
			cent->unsphereshielding = cg.time + 2000;
		}
	}
	else if (cent->sphereshielded)
	{
		cent->sphereshielded = qfalse;
		cent->unsphereshielding = cg.time + 2000;
	}

	if (cent->unsphereshielding > cg.time)
	{//in the middle of cloaking
		float perc2 = (float)(cent->unsphereshielding - cg.time) / 2000.0f;
		if ((cent->currentState.powerups & (1 << PW_SPHERESHIELDED)))
		{//actually cloaking, so reverse it
			perc2 = 1.0f - perc2;
		}

		if (perc2 >= 0.0f && perc2 <= 1.0f)
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.25f, cgs.media.ysaliblueShader);
		}
	}
	else if ((cent->currentState.powerups & (1 << PW_SPHERESHIELDED)))
	{//fully cloaked
		if (cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.25f, cgs.media.ysaliblueShader);
		}
	}

	//
	// add the gun / barrel / flash
	//
	if (cent->currentState.weapon != WP_EMPLACED_GUN)
	{
		if (cent->currentState.eFlags & EF3_DUAL_WEAPONS && cent->currentState.weapon == WP_BRYAR_PISTOL)
		{
			cg_add_player_weaponduals(&legs, NULL, cent, root_angles, qtrue, qtrue);
		}
		else
		{
			CG_AddPlayerWeapon(&legs, NULL, cent, root_angles, qtrue);
		}
	}
	// add powerups floating behind the player
	CG_PlayerPowerups(cent);

	if (cent->currentState.forcePowersActive & 1 << FP_RAGE &&
		(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum
			|| cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER
			|| cg.predictedPlayerState.weapon == WP_MELEE))
	{
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx &= ~RF_MINLIGHT;

		legs.renderfx |= RF_RGB_TINT;
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = legs.shaderRGBA[2] = 0;
		legs.shaderRGBA[3] = 255;

		if (rand() & 1)
		{
			legs.customShader = cgs.media.electricBodyShader;
		}
		else
		{
			legs.customShader = cgs.media.electricBody2Shader;
		}

		trap->R_AddRefEntityToScene(&legs);
	}

	if (cent->currentState.number != cg.snap->ps.clientNum)
	{
		if (cg_SaberInnonblockableAttackWarning.integer)
		{
			if (pm_saber_innonblockable_attack(cent->currentState.torsoAnim) && !(cent->currentState.powerups & 1 <<
				PW_CLOAKED))
			{
				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
				legs.renderfx &= ~RF_MINLIGHT;

				legs.renderfx |= RF_RGB_TINT;
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = legs.shaderRGBA[2] = 0;
				legs.shaderRGBA[3] = 255;

				trap->R_AddRefEntityToScene(&legs);
			}
		}
	}

	if (!cg.snap->ps.duelInProgress && cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->
		currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->
			currentState.number))
	{
		legs.shaderRGBA[0] = 50;
		legs.shaderRGBA[1] = 50;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.forceSightBubble;

		trap->R_AddRefEntityToScene(&legs);
	}

	if (CG_VehicleShouldDrawShields(cent) //vehicle
		|| check_droid_shields && CG_VehicleShouldDrawShields(&cg_entities[cent->currentState.m_iVehicleNum]))
		//droid in vehicle
	{
		//Vehicles have form-fitting shields
		Vehicle_t* p_veh = cent->m_pVehicle;
		if (check_droid_shields)
		{
			p_veh = cg_entities[cent->currentState.m_iVehicleNum].m_pVehicle;
		}
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 255;
		legs.shaderRGBA[3] = 10.0f + sin((float)(cg.time / 4)) * 128.0f;
		//112.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + Q_flrand(0.0f, 1.0f)*16;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;

		if (p_veh
			&& p_veh->m_pVehicleInfo
			&& p_veh->m_pVehicleInfo->shieldShaderHandle)
		{
			//use the vehicle-specific shader
			legs.customShader = p_veh->m_pVehicleInfo->shieldShaderHandle;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}

		trap->R_AddRefEntityToScene(&legs);
	}

	//For now, these two are using the old shield shader. This is just so that you
	//can tell it apart from the JM/duel shaders, but it's still very obvious.
	if (cent->currentState.forcePowersActive & 1 << FP_PROTECT
		&& cent->currentState.forcePowersActive & 1 << FP_ABSORB)
	{
		//using both at once, save ourselves some rendering
		//protect+absorb is represented by cyan..

		if (cg.snap->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1
			|| cg.snap->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1)
		{
			legs.shaderRGBA[0] = 0;
			legs.shaderRGBA[1] = 255;
			legs.shaderRGBA[2] = 255;
			legs.shaderRGBA[3] = 254;
		}
		else
		{
			legs.shaderRGBA[0] = 0;
			legs.shaderRGBA[1] = 0;
			legs.shaderRGBA[2] = 255;
			legs.shaderRGBA[3] = 254;
		}

		legs.renderfx &= ~RF_RGB_TINT;

		if (cg.snap->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1
			|| cg.snap->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1)
		{
			legs.customShader = cgs.media.forceShell;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}
		trap->R_AddRefEntityToScene(&legs);
	}
	else if (cent->currentState.forcePowersActive & 1 << FP_PROTECT)
	{
		//protect is represented by green..
		if (cg.snap->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_2) // if i have fp 3 i get this color
		{
			legs.shaderRGBA[0] = 120;
			legs.shaderRGBA[1] = 190;
			legs.shaderRGBA[2] = 0;
			legs.shaderRGBA[3] = 254;
		}
		else //if i have less than FP 3 i get this color
		{
			legs.shaderRGBA[0] = 80;
			legs.shaderRGBA[1] = 75;
			legs.shaderRGBA[2] = 140;
			legs.shaderRGBA[3] = 254;
		}

		legs.renderfx &= ~RF_RGB_TINT;

		if (cg.snap->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1)
		{
			legs.customShader = cgs.media.forceShell;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}

		trap->R_AddRefEntityToScene(&legs);
	}
	else if (cent->currentState.forcePowersActive & 1 << FP_ABSORB)
	{
		//aborb is represented by blue..
		if (cg.snap->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2) // if i have fp 3 i get this color
		{
			legs.shaderRGBA[0] = 90;
			legs.shaderRGBA[1] = 30;
			legs.shaderRGBA[2] = 185;
			legs.shaderRGBA[3] = 254;
		}
		else //if i have less than FP 3 i get this color
		{
			legs.shaderRGBA[0] = 255;
			legs.shaderRGBA[1] = 115;
			legs.shaderRGBA[2] = 0;
			legs.shaderRGBA[3] = 254;
		}

		legs.renderfx &= ~RF_RGB_TINT;

		if (cg.snap->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1)
		{
			legs.customShader = cgs.media.forceShell;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}

		trap->R_AddRefEntityToScene(&legs);
	}

	// Render the shield effect if they've been hit recently
	if (cent->shieldHitTime > cg.time)
	{
		float t = (cent->shieldHitTime - cg.time) / 250.0f;
		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = t * 255.0f;
		legs.shaderRGBA[3] = 255.0f;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.playerShieldDamage;
		trap->R_AddRefEntityToScene(&legs); //draw the shell
	}

	// ...or have started recharging their shield
	if (cent->shieldRechargeTime > cg.time)
	{
		float t = 1.0f - (cent->shieldRechargeTime - cg.time) / 1000.0f;
		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = t * 255.0f;
		legs.shaderRGBA[3] = 255.0f;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.playerShieldDamage;
		trap->R_AddRefEntityToScene(&legs); //draw the shell
	}

	if (cent->currentState.isJediMaster && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 100;
		legs.shaderRGBA[1] = 100;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx |= RF_NODEPTH;
		legs.customShader = cgs.media.forceShell;

		trap->R_AddRefEntityToScene(&legs);

		legs.renderfx &= ~RF_NODEPTH;
	}

	if (cg.snap->ps.fd.forcePowersActive & 1 << FP_SEE && cg.snap->ps.clientNum != cent->currentState.number &&
		cg_auraShell.integer)
	{
		if (cent->currentState.NPC_class == CLASS_IMPERIAL && cent->currentState.generic1 == 100)
		{
			legs.shaderRGBA[0] = 255;
			legs.shaderRGBA[1] = 255;
			legs.shaderRGBA[2] = 0;
		}
		else if (cgs.gametype == GT_SIEGE)
		{
			// A team game
			if (ci->team == TEAM_SPECTATOR || ci->team == TEAM_FREE)
			{
				//yellow
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if (ci->team != cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				//red
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
			}
			else
			{
				//green
				legs.shaderRGBA[0] = 50;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 50;
			}
		}
		else if (cgs.gametype >= GT_TEAM)
		{
			// A team game
			switch (ci->team)
			{
			case TEAM_RED:
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
				break;
			case TEAM_BLUE:
				legs.shaderRGBA[0] = 75;
				legs.shaderRGBA[1] = 75;
				legs.shaderRGBA[2] = 255;
				break;
			default:;
			}
		}
		else if (cgs.gametype == GT_FFA)
		{
			// Not a team game
			if (cent->currentState.botclass == BCLASS_DESANN
				|| cent->currentState.botclass == BCLASS_UNSTABLESABER
				|| cent->currentState.botclass == BCLASS_SHADOWTROOPER
				|| cent->currentState.botclass == BCLASS_TAVION
				|| cent->currentState.botclass == BCLASS_ALORA
				|| cent->currentState.botclass == BCLASS_REBORN)
			{
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 120;
				legs.shaderRGBA[2] = 255;
			}
			else if (cent->currentState.botclass == BCLASS_SBD
				|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
				|| cent->currentState.botclass == BCLASS_BATTLEDROID
				|| cent->currentState.botclass == BCLASS_DROIDEKA
				|| cent->currentState.botclass == BCLASS_SABER_DROID)
			{
				legs.shaderRGBA[0] = 100;
				legs.shaderRGBA[1] = 90;
				legs.shaderRGBA[2] = 160;
			}
			else if (cent->currentState.botclass == BCLASS_JEDI
				|| cent->currentState.botclass == BCLASS_JEDITRAINER
				|| cent->currentState.botclass == BCLASS_JEDIMASTER
				|| cent->currentState.botclass == BCLASS_LUKE
				|| cent->currentState.botclass == BCLASS_KYLE)
			{
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 120;
				legs.shaderRGBA[2] = 0;
			}
			else if (cent->currentState.botclass == BCLASS_R2D2
				|| cent->currentState.botclass == BCLASS_R5D2
				|| cent->currentState.botclass == BCLASS_PROTOCOL)
			{
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 230;
			}
			else if (cent->currentState.botclass == BCLASS_JAN)
			{
				legs.shaderRGBA[0] = 125;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 230;
			}
			else if (cent->currentState.botclass == BCLASS_BOBAFETT
				|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
				|| cent->currentState.botclass == BCLASS_MANDOLORIAN
				|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
				|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
			{
				legs.shaderRGBA[0] = 85;
				legs.shaderRGBA[1] = 155;
				legs.shaderRGBA[2] = 210;
			}
			else
			{
				legs.shaderRGBA[0] = 80;
				legs.shaderRGBA[1] = 80;
				legs.shaderRGBA[2] = 200;
			}
		}
		else
		{
			// Not a team game
			if (cg.snap->ps.zoomMode == 2)
			{
				// binoculars see orange
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 128;
				legs.shaderRGBA[2] = 0;
			}
			else
			{
				if (cent->currentState.botclass == BCLASS_DESANN
					|| cent->currentState.botclass == BCLASS_UNSTABLESABER
					|| cent->currentState.botclass == BCLASS_SHADOWTROOPER
					|| cent->currentState.botclass == BCLASS_TAVION
					|| cent->currentState.botclass == BCLASS_ALORA
					|| cent->currentState.botclass == BCLASS_REBORN)
				{
					legs.shaderRGBA[0] = 255;
					legs.shaderRGBA[1] = 120;
					legs.shaderRGBA[2] = 255;
				}
				else if (cent->currentState.botclass == BCLASS_SBD
					|| cent->currentState.botclass == BCLASS_ASSASSIN_DROID
					|| cent->currentState.botclass == BCLASS_BATTLEDROID
					|| cent->currentState.botclass == BCLASS_DROIDEKA
					|| cent->currentState.botclass == BCLASS_SABER_DROID)
				{
					legs.shaderRGBA[0] = 100;
					legs.shaderRGBA[1] = 90;
					legs.shaderRGBA[2] = 160;
				}
				else if (cent->currentState.botclass == BCLASS_JEDI
					|| cent->currentState.botclass == BCLASS_JEDITRAINER
					|| cent->currentState.botclass == BCLASS_JEDIMASTER
					|| cent->currentState.botclass == BCLASS_LUKE
					|| cent->currentState.botclass == BCLASS_KYLE)
				{
					legs.shaderRGBA[0] = 0;
					legs.shaderRGBA[1] = 120;
					legs.shaderRGBA[2] = 0;
				}
				else if (cent->currentState.botclass == BCLASS_R2D2
					|| cent->currentState.botclass == BCLASS_R5D2
					|| cent->currentState.botclass == BCLASS_PROTOCOL)
				{
					legs.shaderRGBA[0] = 255;
					legs.shaderRGBA[1] = 255;
					legs.shaderRGBA[2] = 230;
				}
				else if (cent->currentState.botclass == BCLASS_JAN)
				{
					legs.shaderRGBA[0] = 125;
					legs.shaderRGBA[1] = 255;
					legs.shaderRGBA[2] = 230;
				}
				else if (cent->currentState.botclass == BCLASS_BOBAFETT
					|| cent->currentState.botclass == BCLASS_ROCKETTROOPER
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN1
					|| cent->currentState.botclass == BCLASS_MANDOLORIAN2)
				{
					legs.shaderRGBA[0] = 85;
					legs.shaderRGBA[1] = 155;
					legs.shaderRGBA[2] = 210;
				}
				else
				{
					legs.shaderRGBA[0] = 80;
					legs.shaderRGBA[1] = 80;
					legs.shaderRGBA[2] = 200;
				}
			}
		}

		legs.renderfx |= RF_MINLIGHT | RF_NODEPTH;

		if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
		{
			//only level 2+ can see players through walls
			legs.renderfx &= ~RF_NODEPTH;
		}

		if (cg.snap->ps.zoomMode == 2)
		{
			legs.renderfx |= RF_NODEPTH;
		}

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.sightShell;

		trap->R_AddRefEntityToScene(&legs);
	}

	// Electricity
	//------------------------------------------------
	if (cent->currentState.emplacedOwner > cg.time)
	{
		int dif = cent->currentState.emplacedOwner - cg.time;
		vec3_t temp_angles;

		if (dif > 0 && Q_flrand(0.0f, 1.0f) > 0.4f)
		{
			// fade out over the last 500 ms
			int brightness = 255;

			if (dif < 500)
			{
				brightness = floor((dif - 500.0f) / 500.0f * 255.0f);
			}

			legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
			legs.renderfx &= ~RF_MINLIGHT;

			legs.renderfx |= RF_RGB_TINT;
			legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = brightness;
			legs.shaderRGBA[3] = 255;

			if (rand() & 1)
			{
				legs.customShader = cgs.media.electricBodyShader;
			}
			else
			{
				legs.customShader = cgs.media.electricBody2Shader;
			}

			trap->R_AddRefEntityToScene(&legs);

			if (Q_flrand(0.0f, 1.0f) > 0.9f)
				trap->S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, cgs.media.crackleSound);
		}

		VectorSet(temp_angles, 0, cent->lerpAngles[YAW], 0);
		CG_ForceElectrocution(cent, legs.origin, temp_angles, cgs.media.boltShader, qfalse);
	}

#if 0
	endOfCall:

	if (cgBoneAnglePostSet.refreshSet)
	{
		trap->G2API_SetBoneAngles(cgBoneAnglePostSet.ghoul2, cgBoneAnglePostSet.modelIndex, cgBoneAnglePostSet.boneName,
			cgBoneAnglePostSet.angles, cgBoneAnglePostSet.flags, cgBoneAnglePostSet.up, cgBoneAnglePostSet.right,
			cgBoneAnglePostSet.forward, cgBoneAnglePostSet.modelList, cgBoneAnglePostSet.blendTime, cgBoneAnglePostSet.currentTime);

		cgBoneAnglePostSet.refreshSet = qfalse;
	}
#endif
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t* cent)
{
	clientInfo_t* ci;
	int i = 0;
	int j;

	if (cent->currentState.eType == ET_NPC)
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
			cg.predictedPlayerState.m_iVehicleNum &&
			cent->currentState.number == cg.predictedPlayerState.m_iVehicleNum)
		{
			//holy hackery, batman!
			//I don't think this will break anything. But really, do I ever?
			return;
		}

		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
			cent->npcClient->gender = FindGender(CG_ConfigString(CS_MODELS + cent->currentState.modelIndex), cent);
		}

		ci = cent->npcClient;

		assert(ci);

		//just force these guys to be set again, it won't hurt anything if they're
		//already set.
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.clientNum];
	}

	while (i < MAX_SABERS)
	{
		j = 0;
		while (j < ci->saber[i].numBlades)
		{
			ci->saber[i].blade[j].trail.lastTime = -20000;
			j++;
		}
		i++;
	}

	ci->facial_blink = -1;
	ci->facial_frown = 0;
	ci->facial_aux = 0;
	ci->superSmoothTime = 0;

	//reset lerp origin smooth point
	VectorCopy(cent->lerpOrigin, cent->beamEnd);

	if (cent->currentState.eType != ET_NPC ||
		!(cent->currentState.eFlags & EF_DEAD))
	{
		CG_ClearLerpFrame(cent, ci, &cent->pe.legs, cent->currentState.legsAnim, qfalse);
		CG_ClearLerpFrame(cent, ci, &cent->pe.torso, cent->currentState.torsoAnim, qtrue);

		BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
		BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

		VectorCopy(cent->lerpAngles, cent->rawAngles);

		memset(&cent->pe.legs, 0, sizeof cent->pe.legs);
		cent->pe.legs.yawAngle = cent->rawAngles[YAW];
		cent->pe.legs.yawing = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching = qfalse;

		memset(&cent->pe.torso, 0, sizeof cent->pe.torso);
		cent->pe.torso.yawAngle = cent->rawAngles[YAW];
		cent->pe.torso.yawing = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching = qfalse;

		if (cent->currentState.eType == ET_NPC)
		{
			//just start them off at 0 pitch
			cent->pe.torso.pitchAngle = 0;
		}

		if (cent->ghoul2 == NULL && ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);
			cent->weapon = 0;
			cent->ghoul2weapon = NULL;

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{
				//check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);
		}
	}

	//do this to prevent us from making a saber unholster sound the first time we enter the pvs
	if (cent->currentState.number != cg.predictedPlayerState.clientNum &&
		cent->currentState.weapon == WP_SABER &&
		cent->weapon != cent->currentState.weapon)
	{
		cent->weapon = cent->currentState.weapon;
		if (cent->ghoul2 && ci->ghoul2Model)
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
		}
		if (!cent->currentState.saberHolstered)
		{
			//if not holstered set length and desired length for both blades to full right now.
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

			i = 0;
			while (i < MAX_SABERS)
			{
				j = 0;
				while (j < ci->saber[i].numBlades)
				{
					ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
					j++;
				}
				i++;
			}
		}
	}

	if (cg_debugPosition.integer)
	{
		trap->Print("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle);
	}
}