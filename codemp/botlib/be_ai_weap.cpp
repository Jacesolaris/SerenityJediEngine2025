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

/*****************************************************************************
 * name:		be_ai_weap.c
 *
 * desc:		weapon AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_weap.c $
 * $Author: Ttimo $
 * $Revision: 6 $
 * $Modtime: 4/13/01 4:45p $
 * $Date: 4/13/01 4:45p $
 *
 *****************************************************************************/

#include "qcommon/q_shared.h"
#include "l_libvar.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h"		//fuzzy weights
#include "be_ai_weap.h"

 //#define DEBUG_AI_WEAP

 //structure field offsets
#define WEAPON_OFS(x) offsetof(weaponinfo_t, x)
#define PROJECTILE_OFS(x) offsetof(projectileinfo_t, x)

//weapon definition // bk001212 - static
static fielddef_t weaponinfo_fields[] =
{
{"number", WEAPON_OFS(number), FT_INT},						//weapon number
{"name", WEAPON_OFS(name), FT_STRING},							//name of the weapon
{"level", WEAPON_OFS(level), FT_INT},
{"model", WEAPON_OFS(model), FT_STRING},						//model of the weapon
{"weapon_index", WEAPON_OFS(weapon_index), FT_INT},			//index of weapon in inventory
{"flags", WEAPON_OFS(flags), FT_INT},							//special flags
{"projectile", WEAPON_OFS(projectile), FT_STRING},			//projectile used by the weapon
{"numprojectiles", WEAPON_OFS(numprojectiles), FT_INT},	//number of projectiles
{"hspread", WEAPON_OFS(hspread), FT_FLOAT},					//horizontal spread of projectiles (degrees from middle)
{"vspread", WEAPON_OFS(vspread), FT_FLOAT},					//vertical spread of projectiles (degrees from middle)
{"speed", WEAPON_OFS(speed), FT_FLOAT},						//speed of the projectile (0 = instant hit)
{"acceleration", WEAPON_OFS(acceleration), FT_FLOAT},		//"acceleration" * time (in seconds) + "speed" = projectile speed
{"recoil", WEAPON_OFS(recoil), FT_FLOAT | FT_ARRAY, 3},		//amount of recoil the player gets from the weapon
{"offset", WEAPON_OFS(offset), FT_FLOAT | FT_ARRAY, 3},		//projectile start offset relative to eye and view angles
{"angleoffset", WEAPON_OFS(angleoffset), FT_FLOAT | FT_ARRAY, 3},//offset of the shoot angles relative to the view angles
{"extrazvelocity", WEAPON_OFS(extrazvelocity), FT_FLOAT},//extra z velocity the projectile gets
{"ammoamount", WEAPON_OFS(ammoamount), FT_INT},				//ammo amount used per shot
{"ammoindex", WEAPON_OFS(ammoindex), FT_INT},				//index of ammo in inventory
{"activate", WEAPON_OFS(activate), FT_FLOAT},				//time it takes to select the weapon
{"reload", WEAPON_OFS(reload), FT_FLOAT},						//time it takes to reload the weapon
{"spinup", WEAPON_OFS(spinup), FT_FLOAT},						//time it takes before first shot
{"spindown", WEAPON_OFS(spindown), FT_FLOAT},				//time it takes before weapon stops firing
{nullptr, 0, 0, 0}
};

//projectile definition
static fielddef_t projectileinfo_fields[] =
{
{"name", PROJECTILE_OFS(name), FT_STRING},					//name of the projectile
{"model", PROJECTILE_OFS(model), FT_STRING},					//model of the projectile
{"flags", PROJECTILE_OFS(flags), FT_INT},						//special flags
{"gravity", PROJECTILE_OFS(gravity), FT_FLOAT},				//amount of gravity applied to the projectile [0,1]
{"damage", PROJECTILE_OFS(damage), FT_INT},					//damage of the projectile
{"radius", PROJECTILE_OFS(radius), FT_FLOAT},				//radius of damage
{"visdamage", PROJECTILE_OFS(visdamage), FT_INT},			//damage of the projectile to visible entities
{"damagetype", PROJECTILE_OFS(damagetype), FT_INT},		//type of damage (combination of the DAMAGETYPE_? flags)
{"healthinc", PROJECTILE_OFS(healthinc), FT_INT},			//health increase the owner gets
{"push", PROJECTILE_OFS(push), FT_FLOAT},						//amount a player is pushed away from the projectile impact
{"detonation", PROJECTILE_OFS(detonation), FT_FLOAT},		//time before projectile explodes after fire pressed
{"bounce", PROJECTILE_OFS(bounce), FT_FLOAT},				//amount the projectile bounces
{"bouncefric", PROJECTILE_OFS(bouncefric), FT_FLOAT}, 	//amount the bounce decreases per bounce
{"bouncestop", PROJECTILE_OFS(bouncestop), FT_FLOAT},		//minimum bounce value before bouncing stops
//recurive projectile definition??
{nullptr, 0, 0, 0}
};

static structdef_t weaponinfo_struct =
{
	sizeof(weaponinfo_t), weaponinfo_fields
};
static structdef_t projectileinfo_struct =
{
	sizeof(projectileinfo_t), projectileinfo_fields
};

//weapon configuration: set of weapons with projectiles
using weaponconfig_t = struct weaponconfig_s
{
	int numweapons;
	int numprojectiles;
	projectileinfo_t* projectileinfo;
	weaponinfo_t* weaponinfo;
};

//the bot weapon state
using bot_weaponstate_t = struct bot_weaponstate_s
{
	weightconfig_s* weaponweightconfig;		//weapon weight configuration
	int* weaponweightindex;							//weapon weight index
};

static bot_weaponstate_t* botweaponstates[MAX_CLIENTS + 1];
static weaponconfig_t* weaponconfig;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotValidWeaponNumber(const int weaponnum)
{
	if (weaponnum <= 0 || weaponnum > weaponconfig->numweapons)
	{
		botimport.Print(PRT_ERROR, "weapon number out of range\n");
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidWeaponNumber
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
bot_weaponstate_t* BotWeaponStateFromHandle(const int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "weapon state handle %d out of range\n", handle);
		return nullptr;
	} //end if
	if (!botweaponstates[handle])
	{
		botimport.Print(PRT_FATAL, "invalid weapon state %d\n", handle);
		return nullptr;
	} //end if
	return botweaponstates[handle];
} //end of the function BotWeaponStateFromHandle
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef DEBUG_AI_WEAP
void DumpWeaponConfig(weaponconfig_t* wc)
{
	FILE* fp;
	int i;

	fp = Log_FileStruct();
	if (!fp) return;
	for (i = 0; i < wc->numprojectiles; i++)
	{
		WriteStructure(fp, &projectileinfo_struct, (char*)&wc->projectileinfo[i]);
		Log_Flush();
	} //end for
	for (i = 0; i < wc->numweapons; i++)
	{
		WriteStructure(fp, &weaponinfo_struct, (char*)&wc->weaponinfo[i]);
		Log_Flush();
	} //end for
} //end of the function DumpWeaponConfig
#endif //DEBUG_AI_WEAP
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
weaponconfig_t* LoadWeaponConfig(char* filename)
{
	token_t token;
	char path[MAX_PATH];
	int j;
	weaponinfo_t weaponinfo;

	int max_weaponinfo = static_cast<int>(LibVarValue("max_weaponinfo", "32"));
	if (max_weaponinfo < 0)
	{
		botimport.Print(PRT_ERROR, "max_weaponinfo = %d\n", max_weaponinfo);
		max_weaponinfo = 32;
		LibVarSet("max_weaponinfo", "32");
	} //end if
	int max_projectileinfo = static_cast<int>(LibVarValue("max_projectileinfo", "32"));
	if (max_projectileinfo < 0)
	{
		botimport.Print(PRT_ERROR, "max_projectileinfo = %d\n", max_projectileinfo);
		max_projectileinfo = 32;
		LibVarSet("max_projectileinfo", "32");
	} //end if
	strncpy(path, filename, MAX_PATH);
	PC_SetBaseFolder(BOTFILESBASEFOLDER);
	source_t* source = LoadSourceFile(path);
	if (!source)
	{
		botimport.Print(PRT_ERROR, "counldn't load %s\n", path);
		return nullptr;
	} //end if
	//initialize weapon config
	const auto wc = static_cast<weaponconfig_t*>(GetClearedHunkMemory(sizeof(weaponconfig_t) +
		max_weaponinfo * sizeof(weaponinfo_t) +
		max_projectileinfo * sizeof(projectileinfo_t)));
	wc->weaponinfo = reinterpret_cast<weaponinfo_t*>(reinterpret_cast<char*>(wc) + sizeof(weaponconfig_t));
	wc->projectileinfo = reinterpret_cast<projectileinfo_t*>(reinterpret_cast<char*>(wc->weaponinfo) +
		max_weaponinfo * sizeof(weaponinfo_t));
	wc->numweapons = max_weaponinfo;
	wc->numprojectiles = 0;
	//parse the source file
	while (PC_ReadToken(source, &token))
	{
		if (strcmp(token.string, "weaponinfo") == 0)
		{
			Com_Memset(&weaponinfo, 0, sizeof(weaponinfo_t));
			if (!ReadStructure(source, &weaponinfo_struct, reinterpret_cast<char*>(&weaponinfo)))
			{
				FreeMemory(wc);
				FreeSource(source);
				return nullptr;
			} //end if
			if (weaponinfo.number < 0 || weaponinfo.number >= max_weaponinfo)
			{
				botimport.Print(PRT_ERROR, "weapon info number %d out of range in %s\n", weaponinfo.number, path);
				FreeMemory(wc);
				FreeSource(source);
				return nullptr;
			} //end if
			Com_Memcpy(&wc->weaponinfo[weaponinfo.number], &weaponinfo, sizeof(weaponinfo_t));
			wc->weaponinfo[weaponinfo.number].valid = qtrue;
		} //end if
		else if (strcmp(token.string, "projectileinfo") == 0)
		{
			if (wc->numprojectiles >= max_projectileinfo)
			{
				botimport.Print(PRT_ERROR, "more than %d projectiles defined in %s\n", max_projectileinfo, path);
				FreeMemory(wc);
				FreeSource(source);
				return nullptr;
			} //end if
			Com_Memset(&wc->projectileinfo[wc->numprojectiles], 0, sizeof(projectileinfo_t));
			if (!ReadStructure(source, &projectileinfo_struct, reinterpret_cast<char*>(&wc->projectileinfo[wc->numprojectiles])))
			{
				FreeMemory(wc);
				FreeSource(source);
				return nullptr;
			} //end if
			wc->numprojectiles++;
		} //end if
		else
		{
			botimport.Print(PRT_ERROR, "unknown definition %s in %s\n", token.string, path);
			FreeMemory(wc);
			FreeSource(source);
			return nullptr;
		} //end else
	} //end while
	FreeSource(source);
	//fix up weapons
	for (int i = 0; i < wc->numweapons; i++)
	{
		if (!wc->weaponinfo[i].valid) continue;
		if (!wc->weaponinfo[i].name[0])
		{
			botimport.Print(PRT_ERROR, "weapon %d has no name in %s\n", i, path);
			FreeMemory(wc);
			return nullptr;
		} //end if
		if (!wc->weaponinfo[i].projectile[0])
		{
			botimport.Print(PRT_ERROR, "weapon %s has no projectile in %s\n", wc->weaponinfo[i].name, path);
			FreeMemory(wc);
			return nullptr;
		} //end if
		//find the projectile info and copy it to the weapon info
		for (j = 0; j < wc->numprojectiles; j++)
		{
			if (strcmp(wc->projectileinfo[j].name, wc->weaponinfo[i].projectile) == 0)
			{
				Com_Memcpy(&wc->weaponinfo[i].proj, &wc->projectileinfo[j], sizeof(projectileinfo_t));
				break;
			} //end if
		} //end for
		if (j == wc->numprojectiles)
		{
			botimport.Print(PRT_ERROR, "weapon %s uses undefined projectile in %s\n", wc->weaponinfo[i].name, path);
			FreeMemory(wc);
			return nullptr;
		} //end if
	} //end for
	if (!wc->numweapons) botimport.Print(PRT_WARNING, "no weapon info loaded\n");
	botimport.Print(PRT_MESSAGE, "loaded %s\n", path);
	return wc;
} //end of the function LoadWeaponConfig
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int* WeaponWeightIndex(weightconfig_t* wwc, weaponconfig_t* wc)
{
	//initialize item weight index
	const auto index = static_cast<int*>(GetClearedMemory(sizeof(int) * wc->numweapons));

	for (int i = 0; i < wc->numweapons; i++)
	{
		index[i] = FindFuzzyWeight(wwc, wc->weaponinfo[i].name);
	} //end for
	return index;
} //end of the function WeaponWeightIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeWeaponWeights(const int weaponstate)
{
	const bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws) return;
	if (ws->weaponweightconfig) FreeWeightConfig(ws->weaponweightconfig);
	if (ws->weaponweightindex) FreeMemory(ws->weaponweightindex);
} //end of the function BotFreeWeaponWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadWeaponWeights(const int weaponstate, char* filename)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws) return BLERR_CANNOTLOADWEAPONWEIGHTS;
	BotFreeWeaponWeights(weaponstate);
	//
	ws->weaponweightconfig = ReadWeightConfig(filename);
	if (!ws->weaponweightconfig)
	{
		botimport.Print(PRT_FATAL, "couldn't load weapon config %s\n", filename);
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	} //end if
	if (!weaponconfig) return BLERR_CANNOTLOADWEAPONCONFIG;
	ws->weaponweightindex = WeaponWeightIndex(ws->weaponweightconfig, weaponconfig);
	return BLERR_NOERROR;
} //end of the function BotLoadWeaponWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotGetWeaponInfo(const int weaponstate, const int weapon, weaponinfo_t* weaponinfo)
{
	if (!BotValidWeaponNumber(weapon)) return;
	const bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws) return;
	if (!weaponconfig) return;
	Com_Memcpy(weaponinfo, &weaponconfig->weaponinfo[weapon], sizeof(weaponinfo_t));
} //end of the function BotGetWeaponInfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseBestFightWeapon(const int weaponstate, int* inventory)
{
	const bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws) return 0;
	const weaponconfig_t* wc = weaponconfig;
	if (!weaponconfig) return 0;

	//if the bot has no weapon weight configuration
	if (!ws->weaponweightconfig) return 0;

	float bestweight = 0;
	int bestweapon = 0;
	for (int i = 0; i < wc->numweapons; i++)
	{
		if (!wc->weaponinfo[i].valid) continue;
		const int index = ws->weaponweightindex[i];
		if (index < 0) continue;
		const float weight = FuzzyWeight(inventory, ws->weaponweightconfig, index);
		if (weight > bestweight)
		{
			bestweight = weight;
			bestweapon = i;
		} //end if
	} //end for
	return bestweapon;
} //end of the function BotChooseBestFightWeapon
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetWeaponState(const int weaponstate)
{
	bot_weaponstate_t* ws = BotWeaponStateFromHandle(weaponstate);
	if (!ws) return;
	weightconfig_s* weaponweightconfig = ws->weaponweightconfig;
	int* weaponweightindex = ws->weaponweightindex;

	//Com_Memset(ws, 0, sizeof(bot_weaponstate_t));
	ws->weaponweightconfig = weaponweightconfig;
	ws->weaponweightindex = weaponweightindex;
} //end of the function BotResetWeaponState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotAllocWeaponState(void)
{
	for (int i = 1; i <= MAX_CLIENTS; i++)
	{
		if (!botweaponstates[i])
		{
			botweaponstates[i] = static_cast<bot_weaponstate_s*>(GetClearedMemory(sizeof(bot_weaponstate_t)));
			return i;
		} //end if
	} //end for
	return 0;
} //end of the function BotAllocWeaponState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeWeaponState(const int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "weapon state handle %d out of range\n", handle);
		return;
	} //end if
	if (!botweaponstates[handle])
	{
		botimport.Print(PRT_FATAL, "invalid weapon state %d\n", handle);
		return;
	} //end if
	BotFreeWeaponWeights(handle);
	FreeMemory(botweaponstates[handle]);
	botweaponstates[handle] = nullptr;
} //end of the function BotFreeWeaponState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupWeaponAI(void)
{
	char* file = LibVarString("weaponconfig", "weapons.c");
	weaponconfig = LoadWeaponConfig(file);
	if (!weaponconfig)
	{
		botimport.Print(PRT_FATAL, "couldn't load the weapon config\n");
		return BLERR_CANNOTLOADWEAPONCONFIG;
	} //end if

#ifdef DEBUG_AI_WEAP
	DumpWeaponConfig(weaponconfig);
#endif //DEBUG_AI_WEAP
	//
	return BLERR_NOERROR;
} //end of the function BotSetupWeaponAI
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotShutdownWeaponAI(void)
{
	if (weaponconfig) FreeMemory(weaponconfig);
	weaponconfig = nullptr;

	for (int i = 1; i <= MAX_CLIENTS; i++)
	{
		if (botweaponstates[i])
		{
			BotFreeWeaponState(i);
		} //end if
	} //end for
} //end of the function BotShutdownWeaponAI