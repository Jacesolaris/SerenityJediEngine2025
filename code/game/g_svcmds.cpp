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

#include "../cgame/cg_local.h"
#include "Q3_Interface.h"

#include "g_local.h"
#include "wp_saber.h"
#include "g_functions.h"

extern void G_NextTestAxes();
extern void G_ChangePlayerModel(gentity_t* ent, const char* newModel);
extern void G_InitPlayerFromCvars(gentity_t* ent);
extern void Q3_SetViewEntity(int entID, const char* name);
extern qboolean G_ClearViewEntity(gentity_t* ent);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength, qboolean break_saber_lock);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern void WP_SetSaber(gentity_t* ent, int saberNum, const char* saber_name);
extern void WP_RemoveSaber(gentity_t* ent, int saberNum);
extern saber_colors_t TranslateSaberColor(const char* name);
extern qboolean WP_SaberBladeUseSecondBladeStyle(const saberInfo_t* saber, int blade_num);
extern qboolean WP_UseFirstValidSaberStyle(const gentity_t* ent, int* saberAnimLevel);
extern void G_RemoveWeather();
extern void RemoveBarrier(gentity_t* ent);

extern void G_SetWeapon(gentity_t* self, int wp);
extern stringID_table_t WPTable[];

extern cvar_t* g_char_model;
extern cvar_t* g_char_skin_head;
extern cvar_t* g_char_skin_torso;
extern cvar_t* g_char_skin_legs;
extern cvar_t* g_char_color_red;
extern cvar_t* g_char_color_green;
extern cvar_t* g_char_color_blue;
extern cvar_t* g_saber;
extern cvar_t* g_saber2;
extern cvar_t* g_saber_color;
extern cvar_t* g_saber2_color;

/*
===================
Svcmd_EntityList_f
===================
*/
static void Svcmd_EntityList_f()
{
	gentity_t* check = g_entities;
	for (int e = 0; e < globals.num_entities; e++, check++)
	{
		if (!check->inuse)
		{
			continue;
		}
		gi.Printf("%3i:", e);
		switch (check->s.eType)
		{
		case ET_GENERAL:
			gi.Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			gi.Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			gi.Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			gi.Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			gi.Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			gi.Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			gi.Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			gi.Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			gi.Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			gi.Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			gi.Printf("ET_INVISIBLE        ");
			break;
		case ET_THINKER:
			gi.Printf("ET_THINKER          ");
			break;
		case ET_CLOUD:
			gi.Printf("ET_CLOUD            ");
			break;
		case ET_TERRAIN:
			gi.Printf("ET_TERRAIN          ");
			break;
		case ET_GRAPPLE:
			gi.Printf("ET_GRAPPLE          ");
			break;
		default:
			gi.Printf("%-3i                ", check->s.eType);
			break;
		}

		if (check->classname)
		{
			gi.Printf("%s", check->classname);
		}
		gi.Printf("\n");
	}
}

//---------------------------
extern void G_StopCinematicSkip();
extern void G_StartCinematicSkip();
extern void ExitEmplacedWeapon(gentity_t* ent);

static void Svcmd_ExitView_f()
{
	static int exit_view_debounce = 0;
	if (exit_view_debounce > level.time)
	{
		return;
	}
	exit_view_debounce = level.time + 500;
	if (in_camera)
	{
		extern cvar_t* g_skippingcin;
		//see if we need to exit an in-game cinematic
		if (g_skippingcin->integer) // already doing cinematic skip?
		{
			// yes...   so stop skipping...
			G_StopCinematicSkip();
		}
		else
		{
			// no... so start skipping...
			G_StartCinematicSkip();
		}
	}
	else if (!G_ClearViewEntity(player))
	{
		//didn't exit control of a droid or turret
		//okay, now try exiting emplaced guns or AT-ST's
		if (player->s.eFlags & EF_LOCKED_TO_WEAPON)
		{
			//get out of emplaced gun
			ExitEmplacedWeapon(player);
		}
		else if (player->client && player->client->NPC_class == CLASS_ATST)
		{
			//a player trying to get out of his ATST
			GEntity_UseFunc(player->activator, player, player);
		}
	}
}

gentity_t* G_GetSelfForPlayerCmd()
{
	if (g_entities[0].client->ps.viewEntity > 0
		&& g_entities[0].client->ps.viewEntity < ENTITYNUM_WORLD
		&& g_entities[g_entities[0].client->ps.viewEntity].client
		&& g_entities[g_entities[0].client->ps.viewEntity].s.weapon == WP_SABER)
	{
		//you're controlling another NPC
		return &g_entities[g_entities[0].client->ps.viewEntity];
	}
	return &g_entities[0];
}

static void Svcmd_Saber_f()
{
	const char* saber = gi.argv(1);
	const char* saber2 = gi.argv(2);
	char name[MAX_CVAR_VALUE_STRING] = { 0 };

	if (gi.argc() < 2)
	{
		gi.Printf("Usage: saber <saber1> <saber2>\n");
		gi.Cvar_VariableStringBuffer("g_saber", name, sizeof name);
		gi.Printf("g_saber is set to %s\n", name);
		gi.Cvar_VariableStringBuffer("g_saber2", name, sizeof name);
		if (name[0])
			gi.Printf("g_saber2 is set to %s\n", name);
		return;
	}

	if (!g_entities[0].client || !saber || !saber[0])
	{
		return;
	}

	gi.cvar_set("g_saber", saber);
	WP_SetSaber(&g_entities[0], 0, saber);
	if (saber2 && saber2[0] && !(g_entities[0].client->ps.saber[0].saberFlags & SFL_TWO_HANDED))
	{
		//want to use a second saber and first one is not twoHanded
		gi.cvar_set("g_saber2", saber2);
		WP_SetSaber(&g_entities[0], 1, saber2);
	}
	else
	{
		gi.cvar_set("g_saber2", "");
		WP_RemoveSaber(&g_entities[0], 1);
	}
}

static void Svcmd_SaberBlade_f()
{
	if (gi.argc() < 2)
	{
		gi.Printf("USAGE: saberblade <saberNum> <blade_num> [0 = off, 1 = on, no arg = toggle]\n");
		return;
	}
	if (g_entities[0].client == nullptr)
	{
		return;
	}
	const int saberNum = atoi(gi.argv(1)) - 1;
	if (saberNum < 0 || saberNum > 1)
	{
		return;
	}
	if (saberNum > 0 && !g_entities[0].client->ps.dualSabers)
	{
		return;
	}
	//FIXME: what if don't even have a single saber at all?
	const int blade_num = atoi(gi.argv(2)) - 1;
	if (blade_num < 0 || blade_num >= g_entities[0].client->ps.saber[saberNum].numBlades)
	{
		return;
	}
	qboolean turnOn;
	if (gi.argc() > 2)
	{
		//explicit
		turnOn = static_cast<qboolean>(atoi(gi.argv(3)) != 0);
	}
	else
	{
		//toggle
		turnOn = static_cast<qboolean>(!g_entities[0].client->ps.saber[saberNum].blade[blade_num].active);
	}

	g_entities[0].client->ps.SaberBladeActivate(saberNum, blade_num, turnOn);
}

static void Svcmd_SaberColor_f()
{
	//FIXME: just list the colors, each additional listing sets that blade
	int saberNum = atoi(gi.argv(1));
	const char* color[MAX_BLADES]{};
	int blade_num;

	for (blade_num = 0; blade_num < MAX_BLADES; blade_num++)
	{
		color[blade_num] = gi.argv(2 + blade_num);
	}

	if (saberNum < 1 || saberNum > 2 || gi.argc() < 3)
	{
		gi.Printf("Usage:  saberColor <saberNum> <blade1 color> <blade2 color> ... <blade8 color>\n");
		gi.Printf("valid saber_nums:  1 or 2\n");
		gi.Printf("valid colors:  red, orange, yellow, green, blue, and purple\n");

		return;
	}
	saberNum--;

	const gentity_t* self = G_GetSelfForPlayerCmd();

	for (blade_num = 0; blade_num < MAX_BLADES; blade_num++)
	{
		if (!color[blade_num] || !color[blade_num][0])
		{
			break;
		}
		self->client->ps.saber[saberNum].blade[blade_num].color = TranslateSaberColor(color[blade_num]);
	}

	if (saberNum == 0)
	{
		gi.cvar_set("g_saber_color", color[0]);
	}
	else if (saberNum == 1)
	{
		gi.cvar_set("g_saber2_color", color[0]);
	}
}

struct SetForceCmd
{
	const char* desc;
	const char* cmdname;
	const int maxlevel;
};

SetForceCmd SetForceTable[NUM_FORCE_POWERS] = {
	{"forceHeal", "setForceHeal", FORCE_LEVEL_3},
	{"forceJump", "setForceJump", FORCE_LEVEL_3},
	{"forceSpeed", "setForceSpeed", FORCE_LEVEL_3},
	{"forcePush", "setForcePush", FORCE_LEVEL_3},
	{"forcePull", "setForcePull", FORCE_LEVEL_3},
	{"forceMindTrick", "setForceMindTrick", FORCE_LEVEL_4},
	{"forceGrip", "setForceGrip", FORCE_LEVEL_3},
	{"forceLightning", "setForceLightning", FORCE_LEVEL_3},
	{"saberThrow", "setSaberThrow", FORCE_LEVEL_3},
	{"saberDefense", "setSaberDefense", FORCE_LEVEL_3},
	{"saberOffense", "setSaberOffense", SS_NUM_SABER_STYLES - 1},
	{"forceRage", "setForceRage", FORCE_LEVEL_3},
	{"forceProtect", "setForceProtect", FORCE_LEVEL_3},
	{"forceAbsorb", "setForceAbsorb", FORCE_LEVEL_3},
	{"forceDrain", "setForceDrain", FORCE_LEVEL_3},
	{"forceSight", "setForceSight", FORCE_LEVEL_3},
	{"forceStasis", "setForceStasis", FORCE_LEVEL_3},
	{"forceDestruction", "setForceDestruction", FORCE_LEVEL_3},
	{"forceGrasp", "setForceGrasp", FORCE_LEVEL_3},
	{"forceRepulse", "setForceRepulse", FORCE_LEVEL_3},
	{"forceLightningStrike", "setForceLightningStrike", FORCE_LEVEL_3},
	{"forceFear", "setForceFear", FORCE_LEVEL_3},
	{"forceDeadlySight", "setForceDeadlySight", FORCE_LEVEL_3},
	{"forceblast", "setForceBlast", FORCE_LEVEL_3},
	{"forceInsanity", "setForceInsanity", FORCE_LEVEL_3},
	{"forceBlinding", "setForceBlinding", FORCE_LEVEL_3},
};

static void Svcmd_ForceSetLevel_f(const int forcePower)
{
	if (!g_entities[0].client)
	{
		return;
	}
	if (g_entities[0].client->NPC_class == CLASS_DROIDEKA
		&& player->s.weapon == WP_DROIDEKA)
	{
		return;
	}
	const char* new_val = gi.argv(1);
	if (!VALIDSTRING(new_val))
	{
		gi.Printf("Current %s level is %d\n", SetForceTable[forcePower].desc,
			g_entities[0].client->ps.forcePowerLevel[forcePower]);
		gi.Printf("Usage:  %s <level> (0 - %i)\n", SetForceTable[forcePower].cmdname,
			SetForceTable[forcePower].maxlevel);
		return;
	}
	const int val = atoi(new_val);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << forcePower;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << forcePower);
	}
	g_entities[0].client->ps.forcePowerLevel[forcePower] = val;
	if (g_entities[0].client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[forcePower] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[forcePower] > SetForceTable[forcePower].maxlevel)
	{
		g_entities[0].client->ps.forcePowerLevel[forcePower] = SetForceTable[forcePower].maxlevel;
	}
}

extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInTransition(int move);
extern qboolean PM_SaberInAttack(int move);
extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);
extern qboolean PM_ControlledByPlayer();
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags,
	int i_blend = SETANIM_BLEND_DEFAULT);

static void Svcmd_SaberAttackCycle_f()
{
	if (!g_entities[0].client)
	{
		return;
	}

	gentity_t* self = G_GetSelfForPlayerCmd();

	if (self->s.weapon != WP_SABER)
	{
		CG_PrevWeapon_f();
		return;
	}

	if (self->client->ps.saber[0].type == SABER_BACKHAND && self->client->ps.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->client->ps.saber[0].type == SABER_ASBACKHAND && self->client->ps.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->client->ps.saber[0].type == SABER_ELECTROSTAFF && self->client->ps.saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->s.clientNum >= MAX_CLIENTS //not the player
		&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF && self->client->ps.saberAnimLevel
		== SS_STAFF)
	{
		return;
	}

	if (self->s.clientNum >= MAX_CLIENTS //not the player
		&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_ELECTROSTAFF && self->client->ps.
		saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->s.clientNum >= MAX_CLIENTS //not the player
		&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF_UNSTABLE && self->client->ps.
		saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->s.clientNum >= MAX_CLIENTS //not the player
		&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF_MAUL && self->client->ps.
		saberAnimLevel == SS_STAFF)
	{
		return;
	}

	if (self->s.clientNum >= MAX_CLIENTS //not the player
		&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_SINGLE && self->client->ps.saberAnimLevel
		== SS_DUAL)
	{
		return;
	}

	if (self->client->ps.dualSabers)
	{
		//can't cycle styles with dualSabers, so just toggle second saber on/off
		if (WP_SaberCanTurnOffSomeBlades(&self->client->ps.saber[1]))
		{
			//can turn second saber off
			if (self->client->ps.saber[1].ActiveManualOnly())
			{
				//turn it off
				for (int blade_num = 0; blade_num < self->client->ps.saber[1].numBlades; blade_num++)
				{
					qboolean skipThisBlade = qfalse;
					if (WP_SaberBladeUseSecondBladeStyle(&self->client->ps.saber[1], blade_num))
					{
						//check to see if we should check the secondary style's flags
						if (self->client->ps.saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2)
						{
							skipThisBlade = qtrue;
						}
					}
					else
					{
						//use the primary style's flags
						if (self->client->ps.saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE)
						{
							skipThisBlade = qtrue;
						}
					}
					if (!skipThisBlade)
					{
						self->client->ps.saber[1].BladeActivate(blade_num, qfalse);
						G_SoundIndexOnEnt(self, CHAN_WEAPON, self->client->ps.saber[1].soundOff);
					}
				}
				if (!self->client->ps.saber[1].Active())
				{
					G_RemoveWeaponModels(self);
					G_RemoveHolsterModels(self);
					if (!self->client->ps.saberInFlight)
					{
						wp_saber_add_g2_saber_models(self, qfalse);
					}
					wp_saber_add_holstered_g2_saber_models(self, qtrue);
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_S6_S1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else if (!self->client->ps.saber[0].ActiveManualOnly())
			{
				//first one is off, too, so just turn that one on
				if (!self->client->ps.saberInFlight)
				{
					//but only if it's in your hand!
					if (!self->client->ps.saber[1].Active())
					{
						G_RemoveWeaponModels(self);
						G_RemoveHolsterModels(self);
						if (!self->client->ps.saberInFlight)
						{
							wp_saber_add_g2_saber_models(self, qfalse);
						}
						wp_saber_add_holstered_g2_saber_models(self, qtrue);
					}
					self->client->ps.saber[0].Activate();
				}
			}
			else
			{
				//turn on the second one
				G_RemoveHolsterModels(self);
				wp_saber_add_g2_saber_models(self, qtrue);
				self->client->ps.saber[1].Activate();
			}
			return;
		}
	}
	else if (self->client->ps.saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&self->client->ps.saber[0]))
	{
		//can't cycle styles with saberstaff, so just toggles saber blades on/off
		if (self->client->ps.saberInFlight)
		{
			//can't turn second blade back on if it's in the air, you naughty boy!
			return;
		}
		qboolean playedSound = qfalse;
		if (!self->client->ps.saber[0].blade[0].active)
		{
			//first one is not even on
			//turn only it on
			self->client->ps.SaberBladeActivate(0, 0, qtrue);
			return;
		}

		for (int blade_num = 1; blade_num < self->client->ps.saber[0].numBlades; blade_num++)
		{
			if (!self->client->ps.saber[0].blade[blade_num].active)
			{
				//extra is off, turn it on
				self->client->ps.saber[0].BladeActivate(blade_num, qtrue);
			}
			else
			{
				//turn extra off
				qboolean skipThisBlade = qfalse;
				if (WP_SaberBladeUseSecondBladeStyle(&self->client->ps.saber[1], blade_num))
				{
					//check to see if we should check the secondary style's flags
					if (self->client->ps.saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2)
					{
						skipThisBlade = qtrue;
					}
				}
				else
				{
					//use the primary style's flags
					if (self->client->ps.saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE)
					{
						skipThisBlade = qtrue;
					}
				}
				if (!skipThisBlade)
				{
					self->client->ps.saber[0].BladeActivate(blade_num, qfalse);
					if (!playedSound)
					{
						G_SoundIndexOnEnt(self, CHAN_WEAPON, self->client->ps.saber[0].soundOff);
						playedSound = qtrue;
					}
				}
			}
		}
		return;
	}

	int allowedStyles = self->client->ps.saberStylesKnown;
	if (self->client->ps.dualSabers
		&& self->client->ps.saber[0].Active()
		&& self->client->ps.saber[1].Active())
	{
		allowedStyles |= 1 << SS_DUAL;

		for (int styleNum = SS_NONE + 1; styleNum < SS_NUM_SABER_STYLES; styleNum++)
		{
			if (styleNum == SS_TAVION
				&& (self->client->ps.saber[0].stylesLearned & 1 << SS_TAVION || self->client->ps.saber[1].stylesLearned & 1 << SS_TAVION) //was given this style by one of my sabers
				&& !(self->client->ps.saber[0].stylesForbidden & 1 << SS_TAVION)
				&& !(self->client->ps.saber[1].stylesForbidden & 1 << SS_TAVION))
			{
				//if have both sabers on, allow tavion only if one of our sabers specifically wanted to use it... (unless specifically forbidden)
			}
			else if (styleNum == SS_DUAL
				&& !(self->client->ps.saber[0].stylesForbidden & 1 << SS_DUAL)
				&& !(self->client->ps.saber[1].stylesForbidden & 1 << SS_DUAL))
			{
				//if have both sabers on, only dual style is allowed (unless specifically forbidden)
			}
			else
			{
				allowedStyles &= ~(1 << styleNum);
			}
		}
	}

	if (!allowedStyles)
	{
		return;
	}

	int saberAnimLevel;
	if (!self->s.number)
	{
		saberAnimLevel = cg.saber_anim_levelPending;
	}
	else
	{
		saberAnimLevel = self->client->ps.saberAnimLevel;
	}
	saberAnimLevel++;
	int sanityCheck = 0;
	while (self->client->ps.saberAnimLevel != saberAnimLevel
		&& !(allowedStyles & 1 << saberAnimLevel)
		&& sanityCheck < SS_NUM_SABER_STYLES + 1)
	{
		saberAnimLevel++;
		if (saberAnimLevel > SS_STAFF)
		{
			if (self->s.clientNum >= MAX_CLIENTS //not the player
				&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF && self->client->ps.
				saberAnimLevel == SS_STAFF)
			{
				saberAnimLevel = SS_STAFF;
			}
			else if (self->s.clientNum >= MAX_CLIENTS //not the player
				&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF_UNSTABLE && self->client->
				ps.saberAnimLevel == SS_STAFF)
			{
				saberAnimLevel = SS_STAFF;
			}
			else if (self->s.clientNum >= MAX_CLIENTS //not the player
				&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_STAFF_MAUL && self->client->ps.
				saberAnimLevel == SS_STAFF)
			{
				saberAnimLevel = SS_STAFF;
			}
			else if (self->s.clientNum >= MAX_CLIENTS //not the player
				&& !PM_ControlledByPlayer() && self->client->ps.saber[0].type == SABER_SINGLE && self->client->ps.
				saberAnimLevel == SS_DUAL)
			{
				saberAnimLevel = SS_DUAL;
			}
			else if (self->client->ps.saber[0].type == SABER_BACKHAND
				|| self->client->ps.saber[0].type == SABER_ASBACKHAND) //saber backhand
			{
				saberAnimLevel = SS_STAFF;
			}
			else
			{
				saberAnimLevel = SS_FAST;
			}
		}
		sanityCheck++;
	}

	if (!(allowedStyles & 1 << saberAnimLevel))
	{
		return;
	}

	WP_UseFirstValidSaberStyle(self, &saberAnimLevel);
	if (!self->s.number)
	{
		cg.saber_anim_levelPending = saberAnimLevel;

		if (!self->client->ps.saberInFlight)
		{
			if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK) && !PM_SaberInAttack(self->client->ps.saber_move) && self->client->ps.saberLockTime < level.time) // lets do a movement when changing styles // need better anims for this
			{
				if (saberAnimLevel == SS_DUAL)
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else if (saberAnimLevel == SS_STAFF)
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_STAND2TO1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) //only player
				{
					if (self->client->ps.SaberActive())
					{
						G_Sound(self, G_SoundIndex(va("sound/weapons/saber/lowswing%d", Q_irand(1, 7))));
					}
					else
					{
						G_Sound(self, G_SoundIndex("sound/weapons/saber/saber_catch.mp3"));
					}
				}
			}
		}
	}
	else
	{
		self->client->ps.saberAnimLevel = saberAnimLevel;
	}

#ifndef FINAL_BUILD
	switch (saberAnimLevel)
	{
	case SS_FAST:
		gi.Printf(S_COLOR_BLUE"Lightsaber Combat Style: Fast\n");
		//LIGHTSABERCOMBATSTYLE_FAST
		break;
	case SS_MEDIUM:
		gi.Printf(S_COLOR_YELLOW"Lightsaber Combat Style: Medium\n");
		//LIGHTSABERCOMBATSTYLE_MEDIUM
		break;
	case SS_STRONG:
		gi.Printf(S_COLOR_RED"Lightsaber Combat Style: Strong\n");
		//LIGHTSABERCOMBATSTYLE_STRONG
		break;
	case SS_DESANN:
		gi.Printf(S_COLOR_CYAN"Lightsaber Combat Style: Desann\n");
		//LIGHTSABERCOMBATSTYLE_DESANN
		break;
	case SS_TAVION:
		gi.Printf(S_COLOR_MAGENTA"Lightsaber Combat Style: Tavion\n");
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	case SS_DUAL:
		gi.Printf(S_COLOR_MAGENTA"Lightsaber Combat Style: Dual\n");
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	case SS_STAFF:
		gi.Printf(S_COLOR_MAGENTA"Lightsaber Combat Style: Staff\n");
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	}
	//gi.Printf("\n");
#endif
}

qboolean G_ReleaseEntity(const gentity_t* grabber)
{
	if (grabber && grabber->client && grabber->client->ps.heldClient < ENTITYNUM_WORLD)
	{
		gentity_t* heldClient = &g_entities[grabber->client->ps.heldClient];
		grabber->client->ps.heldClient = ENTITYNUM_NONE;
		if (heldClient && heldClient->client)
		{
			heldClient->client->ps.heldByClient = ENTITYNUM_NONE;

			heldClient->owner = nullptr;
		}
		return qtrue;
	}
	return qfalse;
}

static void G_GrabEntity(gentity_t* grabber, const char* target)
{
	if (!grabber || !grabber->client)
	{
		return;
	}
	gentity_t* held_client = G_Find(nullptr, FOFS(targetname), target);
	if (held_client && held_client->client && held_client != grabber) //don't grab yourself, it's not polite
	{
		//found him
		grabber->client->ps.heldClient = held_client->s.number;
		held_client->client->ps.heldByClient = grabber->s.number;

		held_client->owner = grabber;
	}
}

static void Svcmd_ICARUS_f()
{
	Quake3Game()->Svcmd();
}

template <int32_t power>
static void Svcmd_ForceSetLevel_f()
{
	Svcmd_ForceSetLevel_f(power);
}

static void Svcmd_SetForceAll_f()
{
	for (int i = FP_HEAL; i < NUM_FORCE_POWERS; i++)
	{
		Svcmd_ForceSetLevel_f(i);
	}

	if (gi.argc() > 1)
	{
		for (int i = SS_NONE + 1; i < SS_NUM_SABER_STYLES; i++)
		{
			g_entities[0].client->ps.saberStylesKnown |= 1 << i;
		}
	}
}

static void Svcmd_ForceJump_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current forceJump level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION]);
		gi.Printf("Usage:  setForceJump <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_LEVITATION;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_LEVITATION);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
	}
}

static void Svcmd_SaberThrow_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current saberThrow level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW]);
		gi.Printf("Usage:  setSaberThrow <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_SABERTHROW;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_SABERTHROW);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceHeal_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current forceHeal level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_HEAL]);
		gi.Printf("Usage:  forceHeal <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_HEAL;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_HEAL);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_HEAL] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForcePush_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current forcePush level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_PUSH]);
		gi.Printf("Usage:  forcePush <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_PUSH;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_PUSH);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_PUSH] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForcePull_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current forcePull level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_PULL]);
		gi.Printf("Usage:  forcePull <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_PULL;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_PULL);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_PULL] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceSpeed_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current forceSpeed level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_SPEED]);
		gi.Printf("Usage:  forceSpeed <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_SPEED;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_SPEED);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_SPEED] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_SPEED] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_SPEED] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceMindTrick_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current mindTrick level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY]);
		gi.Printf("Usage:  mindTrick <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_TELEPATHY;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_TELEPATHY);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_4)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_4;
	}
}

static void Svcmd_SaberDefense_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current saberDefense level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE]);
		gi.Printf("Usage:  saberDefense <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_SABER_DEFENSE;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_SABER_DEFENSE);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
	}
}

static void Svcmd_SaberOffense_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current saberOffense level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE]);
		gi.Printf("Usage:  saberOffense <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_SABER_OFFENSE;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_SABER_OFFENSE);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE] >= SS_NUM_SABER_STYLES)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_SABER_OFFENSE] = SS_NUM_SABER_STYLES - 1;
	}
}

static void Svcmd_ForceStasis_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current ForceStasis level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_STASIS]);
		gi.Printf("Usage:  ForceStasis <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_STASIS;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_STASIS);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_STASIS] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_STASIS] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_STASIS] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_STASIS] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_STASIS] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceStrike_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current ForceStrike level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE]);
		gi.Printf("Usage:  ForceStrike <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_LIGHTNING_STRIKE;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_LIGHTNING_STRIKE);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceBlast_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current ForceBlast level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_BLAST]);
		gi.Printf("Usage:  ForceBlast <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_BLAST;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_BLAST);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_BLAST] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_BLAST] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_BLAST] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_BLAST] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_BLAST] = FORCE_LEVEL_3;
	}
}

static void Svcmd_ForceBlinding_f()
{
	if (!g_entities[0].client)
	{
		return;
	}
	const char* newVal = gi.argv(1);
	if (!VALIDSTRING(newVal))
	{
		gi.Printf("Current ForceBlast level is %d\n", g_entities[0].client->ps.forcePowerLevel[FP_BLINDING]);
		gi.Printf("Usage:  ForceBlast <level> (1 - 3)\n");
		return;
	}
	const int val = atoi(newVal);
	if (val > FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << FP_BLINDING;
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~(1 << FP_BLINDING);
	}
	g_entities[0].client->ps.forcePowerLevel[FP_BLINDING] = val;
	if (g_entities[0].client->ps.forcePowerLevel[FP_BLINDING] < FORCE_LEVEL_0)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_BLINDING] = FORCE_LEVEL_0;
	}
	else if (g_entities[0].client->ps.forcePowerLevel[FP_BLINDING] > FORCE_LEVEL_3)
	{
		g_entities[0].client->ps.forcePowerLevel[FP_BLINDING] = FORCE_LEVEL_3;
	}
}

static void Svcmd_GiveForceAll_f()
{
	Svcmd_ForceJump_f();
	Svcmd_SaberThrow_f();
	Svcmd_ForceHeal_f();
	Svcmd_ForcePush_f();
	Svcmd_ForcePull_f();
	Svcmd_ForceSpeed_f();
	Svcmd_ForceMindTrick_f();
	Svcmd_ForceStasis_f();
	Svcmd_ForceStrike_f();
	Svcmd_ForceBlast_f();
	Svcmd_ForceBlinding_f();
	Svcmd_SaberDefense_f();
	Svcmd_SaberOffense_f();
	Svcmd_ForceSetLevel_f(FP_ABSORB);
	Svcmd_ForceSetLevel_f(FP_SEE);

	if (gi.argc() > 1)
	{
		for (int i = SS_NONE + 1; i < SS_NUM_SABER_STYLES; i++)
		{
			g_entities[0].client->ps.saberStylesKnown |= 1 << i;
		}
	}
}

static void Svcmd_SetSaberAll_f()
{
	Svcmd_ForceSetLevel_f(FP_SABERTHROW);
	Svcmd_ForceSetLevel_f(FP_SABER_DEFENSE);
	Svcmd_ForceSetLevel_f(FP_SABER_OFFENSE);
	for (int i = SS_NONE + 1; i < SS_NUM_SABER_STYLES; i++)
	{
		g_entities[0].client->ps.saberStylesKnown |= 1 << i;
	}
}

static void Svcmd_Scale_f()
{
	if (gi.argc() == 1)
	{
		gi.Printf(S_COLOR_RED "USAGE: scale <30-150>" S_COLOR_WHITE "\n");
	}
	else if (gi.argc() == 2)
	{
		try
		{
			const int value = std::stoi(gi.argv(1));

			if (value < 30 || value > 150)
			{
				gi.Printf(S_COLOR_RED "ERROR: Scale must be between 30 and 150. You put %i." S_COLOR_WHITE "\n", value);
			}
			else
			{
				player->s.modelScale[0] = player->s.modelScale[1] = player->s.modelScale[2] = value / 100.0f;
			}
		}
		catch (...)
		{
			gi.Printf(S_COLOR_RED "ERROR: You just tried to crash the game! Shame on you!" S_COLOR_WHITE "\n");
		}
	}
}

static void Svcmd_RunScript_f()
{
	const char* cmd2 = gi.argv(1);

	if (cmd2 && cmd2[0])
	{
		const char* cmd3 = gi.argv(2);
		if (cmd3 && cmd3[0])
		{
			const gentity_t* found;
			if ((found = G_Find(nullptr, FOFS(targetname), cmd2)) != nullptr)
			{
				Quake3Game()->RunScript(found, cmd3);
			}
			else
			{
				//can't find cmd2
				gi.Printf(S_COLOR_RED "runscript: can't find targetname %s\n", cmd2);
			}
		}
		else
		{
			Quake3Game()->RunScript(&g_entities[0], cmd2);
		}
	}
	else
	{
		gi.Printf(S_COLOR_RED "usage: runscript <ent targetname> scriptname\n");
	}
}

static void Svcmd_Weather_f(void)
{
	char arg1[MAX_STRING_CHARS]{};
	int num;
	CG_Argv(1);

	if ((Q_stricmp(arg1, "snow") == 0) || (Q_stricmp(arg1, "1") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*snow");
	}
	else if ((Q_stricmp(arg1, "lava") == 0) || (Q_stricmp(arg1, "3") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*lava");
	}
	else if ((Q_stricmp(arg1, "rain") == 0) || (Q_stricmp(arg1, "2") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*rain 800");
	}
	else if ((Q_stricmp(arg1, "sandstorm") == 0) || (Q_stricmp(arg1, "sand") == 0) || (Q_stricmp(arg1, "4") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*wind");
		G_EffectIndex("*sand");
	}
	else if ((Q_stricmp(arg1, "fog") == 0) || (Q_stricmp(arg1, "6") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*heavyrainfog");
	}
	else if ((Q_stricmp(arg1, "spacedust") == 0) || (Q_stricmp(arg1, "9") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*spacedust 9000");
	}
	else if ((Q_stricmp(arg1, "acidrain") == 0) || (Q_stricmp(arg1, "7") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
		G_EffectIndex("*acidrain 500");
	}
	if ((Q_stricmp(arg1, "clear") == 0) || (Q_stricmp(arg1, "0") == 0))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		gi.SetConfigstring(CS_EFFECTS + num, "");
	}
}

static void Svcmd_PlayerTeam_f()
{
	const char* cmd2 = gi.argv(1);

	if (!*cmd2 || !cmd2[0])
	{
		gi.Printf(S_COLOR_RED "'playerteam' - change player team, requires a team name!\n");
		gi.Printf(S_COLOR_RED "Current team is: %s\n", GetStringForID(TeamTable, g_entities[0].client->playerTeam));
		gi.Printf(S_COLOR_RED "Valid team names are:\n");
		for (int n = TEAM_FREE + 1; n < TEAM_NUM_TEAMS; n++)
		{
			gi.Printf(S_COLOR_RED "%s\n", GetStringForID(TeamTable, n));
		}
	}
	else
	{
		const auto team = static_cast<team_t>(GetIDForString(TeamTable, cmd2));
		if (team == static_cast<team_t>(-1))
		{
			gi.Printf(S_COLOR_RED "'playerteam' unrecognized team name %s!\n", cmd2);
			gi.Printf(S_COLOR_RED "Current team is: %s\n", GetStringForID(TeamTable, g_entities[0].client->playerTeam));
			gi.Printf(S_COLOR_RED "Valid team names are:\n");
			for (int n = TEAM_FREE; n < TEAM_NUM_TEAMS; n++)
			{
				gi.Printf(S_COLOR_RED "%s\n", GetStringForID(TeamTable, n));
			}
		}
		else
		{
			g_entities[0].client->playerTeam = team;
			//FIXME: convert Imperial, Malon, Hirogen and Klingon to Scavenger?
		}
	}
}

static void Svcmd_Control_f()
{
	const char* cmd2 = gi.argv(1);
	if (!*cmd2 || !cmd2[0])
	{
		if (!G_ClearViewEntity(&g_entities[0]))
		{
			gi.Printf(S_COLOR_RED "control <NPC_targetname>\n", cmd2);
		}
	}
	else
	{
		Q3_SetViewEntity(0, cmd2);
	}
}

static void Svcmd_Grab_f()
{
	const char* cmd2 = gi.argv(1);
	if (!*cmd2 || !cmd2[0])
	{
		if (!G_ReleaseEntity(&g_entities[0]))
		{
			gi.Printf(S_COLOR_RED "grab <NPC_targetname>\n", cmd2);
		}
	}
	else
	{
		G_GrabEntity(&g_entities[0], cmd2);
	}
}

static void Svcmd_Knockdown_f()
{
	if (g_entities[0].s.groundEntityNum == ENTITYNUM_NONE)
	{
	}
	else
	{
		G_Knockdown(&g_entities[0], &g_entities[0], vec3_origin, 300, qtrue);
	}
}

static void Svcmd_PlayerModel_f()
{
	if (g_entities[0].s.groundEntityNum == ENTITYNUM_NONE && PM_InLedgeMove(g_entities[0].client->ps.legsAnim))
	{
		g_entities[0].client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
		g_entities[0].client->ps.legsAnimTimer = 0;
		g_entities[0].client->ps.torsoAnimTimer = 0;

		if (gi.argc() == 1)
		{
			gi.Printf(
				S_COLOR_RED
				"USAGE: playerModel <NPC Name>\n       playerModel <g2model> <skinhead> <skintorso> <skinlower>\n       playerModel player (builds player from customized menu settings)"
				S_COLOR_WHITE "\n");
			gi.Printf("playerModel = %s ", va("%s %s %s %s\n", g_char_model->string, g_char_skin_head->string,
				g_char_skin_torso->string, g_char_skin_legs->string));
		}
		else if (gi.argc() == 2)
		{
			G_ChangePlayerModel(&g_entities[0], gi.argv(1));
		}
		else if (gi.argc() == 5)
		{
			gi.cvar_set("g_char_model", gi.argv(1));
			gi.cvar_set("g_char_skin_head", gi.argv(2));
			gi.cvar_set("g_char_skin_torso", gi.argv(3));
			gi.cvar_set("g_char_skin_legs", gi.argv(4));
			G_InitPlayerFromCvars(&g_entities[0]);
		}
	}
	else if (g_entities[0].client->ps.powerups[PW_GALAK_SHIELD])
	{
		RemoveBarrier(&g_entities[0]);

		if (gi.argc() == 1)
		{
			gi.Printf(
				S_COLOR_RED
				"USAGE: playerModel <NPC Name>\n       playerModel <g2model> <skinhead> <skintorso> <skinlower>\n       playerModel player (builds player from customized menu settings)"
				S_COLOR_WHITE "\n");
			gi.Printf("playerModel = %s ", va("%s %s %s %s\n", g_char_model->string, g_char_skin_head->string,
				g_char_skin_torso->string, g_char_skin_legs->string));
		}
		else if (gi.argc() == 2)
		{
			G_ChangePlayerModel(&g_entities[0], gi.argv(1));
		}
		else if (gi.argc() == 5)
		{
			gi.cvar_set("g_char_model", gi.argv(1));
			gi.cvar_set("g_char_skin_head", gi.argv(2));
			gi.cvar_set("g_char_skin_torso", gi.argv(3));
			gi.cvar_set("g_char_skin_legs", gi.argv(4));
			G_InitPlayerFromCvars(&g_entities[0]);
		}
	}
	else
	{
		if (gi.argc() == 1)
		{
			gi.Printf(
				S_COLOR_RED
				"USAGE: playerModel <NPC Name>\n       playerModel <g2model> <skinhead> <skintorso> <skinlower>\n       playerModel player (builds player from customized menu settings)"
				S_COLOR_WHITE "\n");
			gi.Printf("playerModel = %s ", va("%s %s %s %s\n", g_char_model->string, g_char_skin_head->string,
				g_char_skin_torso->string, g_char_skin_legs->string));
		}
		else if (gi.argc() == 2)
		{
			G_ChangePlayerModel(&g_entities[0], gi.argv(1));
		}
		else if (gi.argc() == 5)
		{
			gi.cvar_set("g_char_model", gi.argv(1));
			gi.cvar_set("g_char_skin_head", gi.argv(2));
			gi.cvar_set("g_char_skin_torso", gi.argv(3));
			gi.cvar_set("g_char_skin_legs", gi.argv(4));
			G_InitPlayerFromCvars(&g_entities[0]);
		}
	}
}

static void Svcmd_PlayerTint_f()
{
	if (gi.argc() == 4)
	{
		g_entities[0].client->renderInfo.customRGBA[0] = atoi(gi.argv(1));
		g_entities[0].client->renderInfo.customRGBA[1] = atoi(gi.argv(2));
		g_entities[0].client->renderInfo.customRGBA[2] = atoi(gi.argv(3));
		gi.cvar_set("g_char_color_red", gi.argv(1));
		gi.cvar_set("g_char_color_green", gi.argv(2));
		gi.cvar_set("g_char_color_blue", gi.argv(3));
	}
	else
	{
		gi.Printf(S_COLOR_RED "USAGE: playerTint <red 0 - 255> <green 0 - 255> <blue 0 - 255>\n");
		gi.Printf("playerTint = %s\n", va("%d %d %d", g_char_color_red->integer, g_char_color_green->integer,
			g_char_color_blue->integer));
	}
}

static void Svcmd_IKnowKungfu_f()
{
	G_SetWeapon(&g_entities[0], WP_MELEE);
	for (int i = FP_FIRST; i < NUM_FORCE_POWERS; i++)
	{
		g_entities[0].client->ps.forcePowersKnown |= 1 << i;
		if (i == FP_TELEPATHY)
		{
			g_entities[0].client->ps.forcePowerLevel[i] = FORCE_LEVEL_4;
		}
		else
		{
			g_entities[0].client->ps.forcePowerLevel[i] = FORCE_LEVEL_3;
		}
	}
}

static void Svcmd_Secrets_f()
{
	const gentity_t* pl = &g_entities[0];
	if (pl->client->sess.missionStats.totalSecrets < 1)
	{
		gi.Printf("There are" S_COLOR_RED " NO " S_COLOR_WHITE "secrets on this map!\n");
	}
	else if (pl->client->sess.missionStats.secretsFound == pl->client->sess.missionStats.totalSecrets)
	{
		gi.Printf("You've found all " S_COLOR_GREEN "%i" S_COLOR_WHITE " secrets on this map!\n",
			pl->client->sess.missionStats.secretsFound);
	}
	else
	{
		gi.Printf(
			"You've found " S_COLOR_GREEN "%i" S_COLOR_WHITE " out of " S_COLOR_GREEN "%i" S_COLOR_WHITE " secrets!\n",
			pl->client->sess.missionStats.secretsFound, pl->client->sess.missionStats.totalSecrets);
	}
}

// PADAWAN - g_spskill 0 + cg_crosshairForceHint 1 + handicap 100
// JEDI - g_spskill 1 + cg_crosshairForceHint 1 + handicap 100
// JEDI KNIGHT - g_spskill 2 + cg_crosshairForceHint 0 + handicap 100
// JEDI MASTER - g_spskill 2 + cg_crosshairForceHint 0 + handicap 50

extern cvar_t* g_spskill;

static void Svcmd_Difficulty_f()
{
	if (gi.argc() == 1)
	{
		if (g_spskill->integer == 0)
		{
			gi.Printf(S_COLOR_GREEN "Current Difficulty: Padawan" S_COLOR_WHITE "\n");
		}
		else if (g_spskill->integer == 1)
		{
			gi.Printf(S_COLOR_GREEN "Current Difficulty: Jedi" S_COLOR_WHITE "\n");
		}
		else if (g_spskill->integer == 2)
		{
			const int crosshairHint = gi.Cvar_VariableIntegerValue("cg_crosshairForceHint");
			const int handicap = gi.Cvar_VariableIntegerValue("handicap");
			if (handicap == 100 && crosshairHint == 0)
			{
				gi.Printf(S_COLOR_GREEN "Current Difficulty: Jedi Knight" S_COLOR_WHITE "\n");
			}
			else if (handicap == 50 && crosshairHint == 0)
			{
				gi.Printf(S_COLOR_GREEN "Current Difficulty: Jedi Master" S_COLOR_WHITE "\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN "Current Difficulty: Jedi Knight (Custom)" S_COLOR_WHITE "\n");
				gi.Printf(S_COLOR_GREEN "Crosshair Force Hint: %i" S_COLOR_WHITE "\n", crosshairHint != 0 ? 1 : 0);
				gi.Printf(S_COLOR_GREEN "Handicap: %i" S_COLOR_WHITE "\n", handicap);
			}
		}
		else
		{
			gi.Printf(
				S_COLOR_RED "Invalid difficulty cvar set! g_spskill (%i) [0-2] is valid range only" S_COLOR_WHITE "\n",
				g_spskill->integer);
		}
	}
}

constexpr auto CMD_NONE = 0x00000000u;
constexpr auto CMD_CHEAT = 0x00000001u;
constexpr auto CMD_ALIVE = 0x00000002u;

using svcmd_t = struct svcmd_s
{
	const char* name;
	void (*func)();
	uint32_t flags;
};

static int svcmdcmp(const void* a, const void* b)
{
	return Q_stricmp(static_cast<const char*>(a), ((svcmd_t*)b)->name);
}

// FIXME some of these should be made CMD_ALIVE too!
static svcmd_t svcmds[] = {
	{"entitylist", Svcmd_EntityList_f, CMD_NONE},
	{"game_memory", Svcmd_GameMem_f, CMD_NONE},

	{"nav", Svcmd_Nav_f, CMD_CHEAT},
	{"npc", Svcmd_NPC_f, CMD_CHEAT},
	{"use", Svcmd_Use_f, CMD_CHEAT},
	{"ICARUS", Svcmd_ICARUS_f, CMD_CHEAT},

	{"saberColor", Svcmd_SaberColor_f, CMD_CHEAT},
	{"saber", Svcmd_Saber_f, CMD_CHEAT},
	{"saberBlade", Svcmd_SaberBlade_f, CMD_CHEAT},

	{"setForceJump", Svcmd_ForceSetLevel_f<FP_LEVITATION>, CMD_CHEAT},
	{"setSaberThrow", Svcmd_ForceSetLevel_f<FP_SABERTHROW>, CMD_CHEAT},
	{"setForceHeal", Svcmd_ForceSetLevel_f<FP_HEAL>, CMD_CHEAT},
	{"setForcePush", Svcmd_ForceSetLevel_f<FP_PUSH>, CMD_CHEAT},
	{"setForcePull", Svcmd_ForceSetLevel_f<FP_PULL>, CMD_CHEAT},
	{"setForceSpeed", Svcmd_ForceSetLevel_f<FP_SPEED>, CMD_CHEAT},
	{"setForceGrip", Svcmd_ForceSetLevel_f<FP_GRIP>, CMD_CHEAT},
	{"setForceLightning", Svcmd_ForceSetLevel_f<FP_LIGHTNING>, CMD_CHEAT},
	{"setMindTrick", Svcmd_ForceSetLevel_f<FP_TELEPATHY>, CMD_CHEAT},
	{"setSaberDefense", Svcmd_ForceSetLevel_f<FP_SABER_DEFENSE>, CMD_CHEAT},
	{"setSaberOffense", Svcmd_ForceSetLevel_f<FP_SABER_OFFENSE>, CMD_CHEAT},
	{"setForceRage", Svcmd_ForceSetLevel_f<FP_RAGE>, CMD_CHEAT},
	{"setForceDrain", Svcmd_ForceSetLevel_f<FP_DRAIN>, CMD_CHEAT},
	{"setForceProtect", Svcmd_ForceSetLevel_f<FP_PROTECT>, CMD_CHEAT},
	{"setForceAbsorb", Svcmd_ForceSetLevel_f<FP_ABSORB>, CMD_CHEAT},
	{"setForceSight", Svcmd_ForceSetLevel_f<FP_SEE>, CMD_CHEAT},
	{"setForceStasis", Svcmd_ForceSetLevel_f<FP_STASIS>, CMD_CHEAT},
	{"setForceDestruction", Svcmd_ForceSetLevel_f<FP_DESTRUCTION>, CMD_CHEAT},
	{"setForceGrasp", Svcmd_ForceSetLevel_f<FP_GRASP>, CMD_CHEAT},
	{"setForceRepulse", Svcmd_ForceSetLevel_f<FP_REPULSE>, CMD_CHEAT},
	{"setForceLightningStrike", Svcmd_ForceSetLevel_f<FP_LIGHTNING_STRIKE>, CMD_CHEAT},
	{"setForceFear", Svcmd_ForceSetLevel_f<FP_FEAR>, CMD_CHEAT},
	{"setForceDeadlySight", Svcmd_ForceSetLevel_f<FP_DEADLYSIGHT>, CMD_CHEAT},
	{"setForceBlast", Svcmd_ForceSetLevel_f<FP_BLAST>, CMD_CHEAT},
	{"setForceInsanity", Svcmd_ForceSetLevel_f<FP_INSANITY>, CMD_CHEAT},
	{"setForceBlinding", Svcmd_ForceSetLevel_f<FP_BLINDING>, CMD_CHEAT},
	{"setForceAll", Svcmd_SetForceAll_f, CMD_CHEAT},
	{"setSaberAll", Svcmd_SetSaberAll_f, CMD_CHEAT},

	{"saberAttackCycle", Svcmd_SaberAttackCycle_f, CMD_NONE},

	{"runscript", Svcmd_RunScript_f, CMD_CHEAT},

	{"playerTeam", Svcmd_PlayerTeam_f, CMD_CHEAT},

	{"control", Svcmd_Control_f, CMD_CHEAT},
	{"grab", Svcmd_Grab_f, CMD_CHEAT},
	{"knockdown", Svcmd_Knockdown_f, CMD_CHEAT},

	{"playerModel", Svcmd_PlayerModel_f, CMD_NONE},
	{"playerTint", Svcmd_PlayerTint_f, CMD_NONE},

	{"nexttestaxes", G_NextTestAxes, CMD_NONE},

	{"exitview", Svcmd_ExitView_f, CMD_NONE},

	{"iknowkungfu", Svcmd_IKnowKungfu_f, CMD_CHEAT},

	{"secrets", Svcmd_Secrets_f, CMD_NONE},
	{"difficulty", Svcmd_Difficulty_f, CMD_NONE},

	{"giveForceAll", Svcmd_GiveForceAll_f, CMD_NONE},

	{"scale", Svcmd_Scale_f, CMD_NONE},
};
static constexpr size_t numsvcmds = std::size(svcmds);

/*
=================
ConsoleCommand
=================
*/
qboolean ConsoleCommand()
{
	const char* cmd = gi.argv(0);
	const auto command = static_cast<const svcmd_t*>(
		Q_LinearSearch(cmd, svcmds, numsvcmds, sizeof svcmds[0], svcmdcmp));

	if (!command)
		return qfalse;

	if ((Q_stricmp(cmd, "r_weather") == 0) || (Q_stricmp(cmd, "weather") == 0))
	{
		Svcmd_Weather_f();
		return qtrue;
	}

	if (command->flags & CMD_CHEAT
		&& !g_cheats->integer)
	{
		gi.Printf("Cheats are not enabled on this server.\n");
		return qtrue;
	}
	if (command->flags & CMD_ALIVE
		&& g_entities[0].health <= 0)
	{
		gi.Printf("You must be alive to use this command.\n");
		return qtrue;
	}
	command->func();
	return qtrue;
}