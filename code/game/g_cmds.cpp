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

#include "g_local.h"
#include "objectives.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "g_functions.h"
#include "../cgame/cg_local.h"
#include "b_local.h"

extern bool in_camera;
extern stringID_table_t SaberStyleTable[];
extern cvar_t* com_outcast;

extern void ForceHeal(gentity_t* self);
extern void ForceGripAdvanced(gentity_t* self);
extern void ForceGripBasic(gentity_t* self);
extern void ForceTelepathy(gentity_t* self);
extern void ForceRage(gentity_t* self);
extern void ForceProtect(gentity_t* self);
extern void ForceAbsorb(gentity_t* self);
extern void ForceSeeing(gentity_t* self);
extern void g_create_g2_attached_weapon_model(gentity_t* ent, const char* ps_weapon_model, int bolt_num,
	int weapon_num);
extern void G_StartMatrixEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void ItemUse_Bacta(gentity_t* ent);
extern gentity_t* G_GetSelfForPlayerCmd();
extern void force_stasis(gentity_t* self);
extern void ForceDestruction(gentity_t* self);
extern void ItemUse_UseCloak(gentity_t* ent);
extern void ItemUse_Barrier(gentity_t* ent);
extern void ItemUse_Barrier_with_saber(gentity_t* ent);
extern void ItemUse_Jetpack(const gentity_t* ent);
extern qboolean PM_SaberInAttack(int move);
extern cvar_t* g_sex;
extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);
extern void G_SpeechEvent(const gentity_t* self, int event);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean PM_RunningAnim(int anim);
extern qboolean BG_IsAlreadyinTauntAnim(int anim);
extern cvar_t* g_sex;
extern void G_PilotXWing(gentity_t* ent);
extern void G_DriveATST(gentity_t* pEnt, gentity_t* atst);
extern qboolean IsHoldingReloadableGun(const gentity_t* ent);
extern void WP_ReloadGun(gentity_t* ent);
extern void RemoveBarrier(gentity_t* ent);
extern void CancelReload(gentity_t* ent);
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern void TurnBarrierOff(gentity_t* ent);

extern void ForceRepulse(gentity_t* self);
extern void ForceGrasp(gentity_t* self);
extern void ForceFear(gentity_t* self);
extern void ForceLightningStrike(gentity_t* self);
extern void ForceDeadlySight(gentity_t* self);
extern void ForceBlast(gentity_t* self);
extern void ForceInsanity(gentity_t* self);
extern void ForceBlinding(gentity_t* self);

/*
==================
CheatsOk
==================
*/
static qboolean CheatsOk(const gentity_t* ent)
{
	if (!g_cheats->integer)
	{
		gi.SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	if (ent->health <= 0)
	{
		gi.SendServerCommand(ent - g_entities, "print \"You must be alive to use this command.\n\"");
		return qfalse;
	}
	return qtrue;
}

/*
==================
ConcatArgs
==================
*/
char* ConcatArgs(const int start)
{
	static char line[MAX_STRING_CHARS];

	int len = 0;
	const int c = gi.argc();
	for (int i = start; i < c; i++)
	{
		const char* arg = gi.argv(i);
		const int tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1)
		{
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(char* in, char* out)
{
	while (*in)
	{
		if (*in == 94)
		{
			in += 2; // skip color code
			continue;
		}
		if (*in < 32)
		{
			in++;
			continue;
		}
		*out++ = tolower(*in++);
	}

	*out = 0;
}

/*
==================
clientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int clientNumberFromString(const gentity_t* to, char* s)
{
	gclient_t* cl;
	int idnum;
	char s2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi(s);
		if (idnum < 0 || idnum >= level.maxclients)
		{
			gi.SendServerCommand(to - g_entities, "print \"Bad client slot: %i\n\"", idnum);
			return -1;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED)
		{
			gi.SendServerCommand(to - g_entities, "print \"Client %i is not active\n\"", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString(s, s2);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++)
	{
		char n2[MAX_STRING_CHARS];
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		SanitizeString(cl->pers.netname, n2);
		if (strcmp(n2, s2) == 0)
		{
			return idnum;
		}
	}

	gi.SendServerCommand(to - g_entities, "print \"User %s is not on the server\n\"", s);
	return -1;
}

extern qboolean he_is_jedi(const gentity_t* ent);

static void G_Give(gentity_t* ent, const char* name, const char* args, const int argc)
{
	qboolean give_all = qfalse;

	if (!Q_stricmp(name, "all"))
		give_all = qtrue;

	if (give_all || !Q_stricmp(name, "health"))
	{
		if (argc == 3)
			ent->health = Com_Clampi(1, ent->client->ps.stats[STAT_MAX_HEALTH], atoi(args));
		else
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "inventory") == 0)
	{
		// Huh?  Was doing a INV_MAX+1 which was wrong because then you'd actually have every inventory item including INV_MAX
		ent->client->ps.stats[STAT_ITEMS] = (1 << INV_MAX) - (1 << INV_ELECTROBINOCULARS);

		if (com_outcast->integer == 1 || com_outcast->integer == 4) //playing outcast
		{
			ent->client->ps.inventory[INV_LIGHTAMP_GOGGLES] = 1;
		}
		else
		{
			ent->client->ps.inventory[INV_ELECTROBINOCULARS] = 1;
		}
		ent->client->ps.inventory[INV_BACTA_CANISTER] = 5;
		ent->client->ps.inventory[INV_GOODIE_KEY] = 5;
		ent->client->ps.inventory[INV_SECURITY_KEY] = 5;

		if (he_is_jedi(ent))
		{
			ent->client->ps.inventory[INV_CLOAK] = 1;
			ent->client->ps.inventory[INV_SEEKER] = 2;
		}

		if (!he_is_jedi(ent))
		{
			if (ent->client->NPC_class == CLASS_DROIDEKA)
			{
				ent->client->ps.inventory[INV_BARRIER] = 1;

				ent->client->ps.inventory[INV_CLOAK] = 0;
				ent->client->ps.inventory[INV_SEEKER] = 0;
				ent->client->ps.inventory[INV_ELECTROBINOCULARS] = 0;
				ent->client->ps.inventory[INV_LIGHTAMP_GOGGLES] = 0;
				ent->client->ps.inventory[INV_CLOAK] = 0;
				ent->client->ps.inventory[INV_SEEKER] = 0;
				ent->client->ps.inventory[INV_BACTA_CANISTER] = 0;
				ent->client->ps.inventory[INV_SENTRY] = 0;

				if (ent->client->ps.powerups[PW_GALAK_SHIELD] || ent->flags & FL_SHIELDED)
				{
					TurnBarrierOff(ent);
				}
			}
			else
			{
				ent->client->ps.inventory[INV_BARRIER] = 1;
				ent->client->ps.inventory[INV_SENTRY] = 2;
			}
		}

		if (!give_all)
		{
			return;
		}
	}

	if (give_all || Q_stricmp(name, "security") == 0)
	{
		// Huh?  Was doing a INV_MAX+1 which was wrong because then you'd actually have every inventory item including INV_MAX
		ent->client->ps.stats[STAT_ITEMS] = (1 << INV_MAX) - (1 << INV_ELECTROBINOCULARS);

		ent->client->ps.inventory[INV_GOODIE_KEY] = 1;
		ent->client->ps.inventory[INV_SECURITY_KEY] = 1;

		if (!give_all)
		{
			return;
		}
	}

	if (give_all || !Q_stricmp(name, "armor") || !Q_stricmp(name, "shield"))
	{
		if (argc == 3)
			ent->client->ps.stats[STAT_ARMOR] = Com_Clampi(0, ent->client->ps.stats[STAT_MAX_HEALTH], atoi(args));
		else
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];

		if (ent->client->ps.stats[STAT_ARMOR] > 0)
			ent->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;
		else
			ent->client->ps.powerups[PW_BATTLESUIT] = 0;

		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "force"))
	{
		if (argc == 3)
			ent->client->ps.forcePower = Com_Clampi(0, ent->client->ps.forcePowerMax, atoi(args));
		else
			ent->client->ps.forcePower = ent->client->ps.forcePowerMax;

		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "weapons"))
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << MAX_PLAYER_WEAPONS) - (1 << WP_NONE);
		if (!give_all)
			return;
	}

	if (!give_all && !Q_stricmp(name, "weaponnum"))
	{
		ent->client->ps.stats[STAT_WEAPONS] |= 1 << atoi(args);
		return;
	}

	if (give_all || !Q_stricmp(name, "ammo"))
	{
		int num = 500;
		if (argc == 3)
			num = Com_Clampi(-1, 500, atoi(args));
		for (int i = AMMO_BLASTER; i < AMMO_MAX; i++)
			ent->client->ps.ammo[i] = num != -1 ? num : ammoData[i].max;
		if (!give_all)
			return;
	}

	if (give_all || !Q_stricmp(name, "batteries"))
	{
		if (argc == 3)
			ent->client->ps.batteryCharge = Com_Clampi(0, MAX_BATTERIES, atoi(args));
		else
			ent->client->ps.batteryCharge = MAX_BATTERIES;

		if (!give_all)
			return;
	}

	// spawn a specific item right on the player
	if (!give_all)
	{
		trace_t trace;
		gitem_t* it = FindItem(args);
		if (!it)
		{
			it = FindItem(name);
			if (!it)
			{
				gi.SendServerCommand(ent - g_entities, "print \"unknown item\n\"");
				return;
			}
		}

		gentity_t* it_ent = G_Spawn();
		VectorCopy(ent->currentOrigin, it_ent->s.origin);
		it_ent->classname = G_NewString(it->classname);
		G_SpawnItem(it_ent, it);
		FinishSpawningItem(it_ent);
		memset(&trace, 0, sizeof trace);
		Touch_Item(it_ent, ent, &trace);
		if (it_ent->inuse)
		{
			G_FreeEntity(it_ent);
		}
	}
}

static void Cmd_Give_f(gentity_t* ent)
{
	if (ent->client->NPC_class == CLASS_DROIDEKA
		&& ent->s.weapon == WP_DROIDEKA)
	{
		return;
	}
	G_Give(ent, gi.argv(1), ConcatArgs(2), gi.argc());
}

extern void SP_fx_runner(gentity_t* ent);
//------------------
static void Cmd_Fx(const gentity_t* ent)
{
	gentity_t* fx_ent = nullptr;

	if (Q_stricmp(gi.argv(1), "play") == 0)
	{
		if (gi.argc() == 3)
		{
			vec3_t dir;
			// I guess, only allow one active at a time
			while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
			{
				G_FreeEntity(fx_ent);
			}

			fx_ent = G_Spawn();

			fx_ent->fxFile = gi.argv(2);

			// Move out in front of the person spawning the effect
			AngleVectors(ent->currentAngles, dir, nullptr, nullptr);
			VectorMA(ent->currentOrigin, 32, dir, fx_ent->s.origin);

			SP_fx_runner(fx_ent);
			fx_ent->delay = 2000; // adjusting delay
			fx_ent->classname = "cmd_fx"; //	and classname

			return;
		}
	}
	else if (Q_stricmp(gi.argv(1), "stop") == 0)
	{
		while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
		{
			G_FreeEntity(fx_ent);
		}

		return;
	}
	else if (Q_stricmp(gi.argv(1), "delay") == 0)
	{
		while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
		{
			if (gi.argc() == 3)
			{
				fx_ent->delay = atoi(gi.argv(2));
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"FX: current delay is: %i\n", fx_ent->delay);
			}

			return;
		}
	}
	else if (Q_stricmp(gi.argv(1), "random") == 0)
	{
		while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
		{
			if (gi.argc() == 3)
			{
				fx_ent->random = atoi(gi.argv(2));
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"FX: current random is: %6.2f\n", fx_ent->random);
			}

			return;
		}
	}
	else if (Q_stricmp(gi.argv(1), "origin") == 0)
	{
		while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
		{
			if (gi.argc() == 5)
			{
				fx_ent->s.origin[0] = atof(gi.argv(2));
				fx_ent->s.origin[1] = atof(gi.argv(3));
				fx_ent->s.origin[2] = atof(gi.argv(4));

				G_SetOrigin(fx_ent, fx_ent->s.origin);
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"FX: current origin is: <%6.2f %6.2f %6.2f>\n",
					fx_ent->currentOrigin[0], fx_ent->currentOrigin[1], fx_ent->currentOrigin[2]);
			}

			return;
		}
	}
	else if (Q_stricmp(gi.argv(1), "dir") == 0)
	{
		while ((fx_ent = G_Find(fx_ent, FOFS(classname), "cmd_fx")) != nullptr)
		{
			if (gi.argc() == 5)
			{
				fx_ent->s.angles[0] = atof(gi.argv(2));
				fx_ent->s.angles[1] = atof(gi.argv(3));
				fx_ent->s.angles[2] = atof(gi.argv(4));

				if (!VectorNormalize(fx_ent->s.angles))
				{
					// must have been zero length
					fx_ent->s.angles[2] = 1;
				}
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"FX: current dir is: <%6.2f %6.2f %6.2f>\n",
					fx_ent->s.angles[0], fx_ent->s.angles[1], fx_ent->s.angles[2]);
			}

			return;
		}
	}

	gi.Printf(S_COLOR_CYAN"Fx--------------------------------------------------------\n");
	gi.Printf(S_COLOR_CYAN"commands:              sample usage:\n");
	gi.Printf(S_COLOR_CYAN"----------------------------------------------------------\n");
	gi.Printf(S_COLOR_CYAN"fx play <filename>     fx play sparks, fx play env/fire\n");
	gi.Printf(S_COLOR_CYAN"fx stop                fx stop\n");
	gi.Printf(S_COLOR_CYAN"fx delay <#>           fx delay 1000\n");
	gi.Printf(S_COLOR_CYAN"fx random <#>          fx random 200\n");
	gi.Printf(S_COLOR_CYAN"fx origin <#><#><#>    fx origin 10 20 30\n");
	gi.Printf(S_COLOR_CYAN"fx dir <#><#><#>       fx dir 0 0 -1\n\n");
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(gentity_t* ent)
{
	const char* msg;

	if (!CheatsOk(ent))
	{
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.SendServerCommand(ent - g_entities, "print \"%s\"", msg);
}

/*
==================
Cmd_Undying_f

Sets client to undead mode

argv(0) undying
==================
*/
static void Cmd_Undying_f(gentity_t* ent)
{
	const char* msg;

	if (!CheatsOk(ent))
	{
		return;
	}

	ent->flags ^= FL_UNDYING;
	if (!(ent->flags & FL_UNDYING))
	{
		msg = "undead mode OFF\n";
	}
	else
	{
		int max;

		const char* cmd = gi.argv(1);
		if (cmd && atoi(cmd))
		{
			max = atoi(cmd);
		}
		else
		{
			max = 999;
		}

		ent->health = ent->max_health = max;

		msg = "undead mode ON\n";

		if (ent->client)
		{
			ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH] = 999;
		}
	}

	gi.SendServerCommand(ent - g_entities, "print \"%s\"", msg);
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(gentity_t* ent)
{
	const char* msg;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.SendServerCommand(ent - g_entities, "print \"%s\"", msg);
}

static void Cmd_Noforce_f(gentity_t* ent)
{
	const char* msg;

	ent->flags ^= FL_NOFORCE;
	if (!(ent->flags & FL_NOFORCE))
		msg = "No Force OFF\n";
	else
		msg = "No Force ON\n";

	gi.SendServerCommand(ent - g_entities, "print \"%s\"", msg);
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(gentity_t* ent)
{
	const char* msg;

	if (ent->client->noclip)
	{
		msg = "noclip OFF\n";
	}
	else
	{
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;
	ent->flags ^= FL_NOFORCE;

	gi.SendServerCommand(ent - g_entities, "print \"%s\"", msg);
}

/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
static void Cmd_LevelShot_f(const gentity_t* ent)
{
	gi.SendServerCommand(ent - g_entities, "clientLevelShot");
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(gentity_t* ent)
{
	if (level.time - ent->client->respawnTime < 5000)
	{
		gi.SendServerCommand(ent - g_entities, "cp @SP_INGAME_ONE_KILL_PER_5_SECONDS");
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
==================
Cmd_Where_f
==================
*/
static void Cmd_Where_f(const gentity_t* ent)
{
	const char* s = gi.argv(1);
	const int len = strlen(s);

	if (gi.argc() < 2)
	{
		gi.Printf("usage: where classname\n");
		return;
	}
	for (int i = 0; i < globals.num_entities; i++)
	{
		if (!PInUse(i))
			continue;
		const gentity_t* check = &g_entities[i];
		if (!Q_stricmpn(s, check->classname, len))
		{
			gi.SendServerCommand(ent - g_entities, "print \"%s %s\n\"", check->classname, vtos(check->s.pos.trBase));
		}
	}
}

/*
-------------------------
UserSpawn
-------------------------
*/

extern qboolean G_CallSpawn(gentity_t* ent);

static void UserSpawn(const gentity_t* ent, const char* name)
{
	vec3_t origin;
	vec3_t vf;
	vec3_t angles;

	//Spawn the ent
	gentity_t* ent2 = G_Spawn();
	ent2->classname = G_NewString(name);

	//Spawn the entity and place it there
	VectorSet(angles, 0, ent->s.apos.trBase[YAW], 0);
	AngleVectors(angles, vf, nullptr, nullptr);
	VectorMA(ent->s.pos.trBase, 96, vf, origin); //FIXME: Find the radius size of the object, and push out 32 + radius

	origin[2] += 8;
	VectorCopy(origin, ent2->s.pos.trBase);
	VectorCopy(origin, ent2->s.origin);
	VectorCopy(ent->s.apos.trBase, ent2->s.angles);

	gi.linkentity(ent2);

	//Find a valid spawning spot
	if (G_CallSpawn(ent2) == qfalse)
	{
		gi.SendServerCommand(ent - g_entities, "print \"Failed to spawn '%s'\n\"", name);
		G_FreeEntity(ent2);
	}
}

/*
-------------------------
Cmd_Spawn
-------------------------
*/

static void Cmd_Spawn(const gentity_t* ent)
{
	char* name = ConcatArgs(1);

	gi.SendServerCommand(ent - g_entities, "print \"Spawning '%s'\n\"", name);

	UserSpawn(ent, name);
}

/*
=================
Cmd_SetViewpos_f
=================
*/
static void Cmd_SetViewpos_f(gentity_t* ent)
{
	vec3_t origin{}, angles;

	if (!g_cheats->integer)
	{
		gi.SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if (gi.argc() != 5)
	{
		gi.SendServerCommand(ent - g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear(angles);
	for (int i = 0; i < 3; i++)
	{
		origin[i] = atof(gi.argv(i + 1));
	}
	origin[2] -= 25; //acount for eye height from viewpos cmd

	angles[YAW] = atof(gi.argv(4));

	TeleportPlayer(ent, origin, angles);
}

/*
=================
Cmd_SetObjective_f
=================
*/
qboolean g_check_player_dark_side();

static void Cmd_SetObjective_f(const gentity_t* ent)
{
	int objective_i;

	if (gi.argc() == 2)
	{
		objective_i = atoi(gi.argv(1));
		gi.Printf("objective #%d  display status=%d, status=%d\n", objective_i,
			ent->client->sess.mission_objectives[objective_i].display,
			ent->client->sess.mission_objectives[objective_i].status
		);
		return;
	}
	if (gi.argc() != 4)
	{
		gi.SendServerCommand(ent - g_entities,
			va("print \"usage: setobjective <objective #>  <display status> <status>\n\""));
		return;
	}

	objective_i = atoi(gi.argv(1));
	const int display_status = atoi(gi.argv(2));
	const int status = atoi(gi.argv(3));

	ent->client->sess.mission_objectives[objective_i].display = static_cast<qboolean>(display_status != 0);
	ent->client->sess.mission_objectives[objective_i].status = status;
	g_check_player_dark_side();
}

/*
=================
Cmd_ViewObjective_f
=================
*/
static void Cmd_ViewObjective_f(const gentity_t* ent)
{
	if (gi.argc() != 2)
	{
		gi.SendServerCommand(ent - g_entities, va("print \"usage: viewobjective <objective #>\n\""));
		return;
	}

	const int objectiveI = atoi(gi.argv(1));

	gi.SendServerCommand(ent - g_entities, va("print \"Objective %d   Display Status(1=show): %d  Status:%d\n\"",
		objectiveI, ent->client->sess.mission_objectives[objectiveI].display,
		ent->client->sess.mission_objectives[objectiveI].status));
}

/*
================
Cmd_UseElectrobinoculars_f
================
*/
static void Cmd_UseElectrobinoculars_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	if (ent->client->ps.inventory[INV_ELECTROBINOCULARS] <= 0)
	{
		// have none to place...play sound?
		return;
	}

	G_AddEvent(ent, EV_USE_INV_BINOCULARS, 0);
}

/*
================
Cmd_UseBacta_f
================
*/
static void Cmd_UseBacta_f(gentity_t* ent)
{
	if (G_IsRidingVehicle(ent))
	{
		return;
	}
	if (ent->health < 1 || in_camera)
	{
		return;
	}

	ItemUse_Bacta(ent);
}

/*
================
Cmd_UseCloak_f
================
*/
static void Cmd_UseCloak_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	ItemUse_UseCloak(ent);
}

/*
================
Cmd_UseJetpack_f
================
*/
static void Cmd_UseJetpack_f(const gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	if (ent->client->ps.jetpackFuel > 10)
	{
		//too low on fuel to start it up
		ItemUse_Jetpack(ent);
	}
}

//----------------------------------------------------------------------------------
static qboolean pick_seeker_spawn_point(vec3_t org, vec3_t fwd, vec3_t right, const int skip, vec3_t spot)
{
	vec3_t mins{}, maxs, forward, end;
	trace_t tr;

	VectorSet(maxs, -8, -8, -24); // ?? size
	VectorSet(maxs, 8, 8, 8);

	VectorCopy(fwd, forward);

	// to the front and side a bit
	forward[2] = 0.3f; // start up a bit

	VectorMA(org, 80, forward, end);
	VectorMA(end, -8, right, end);

	gi.trace(&tr, org, mins, maxs, end, skip, MASK_PLAYERSOLID, static_cast<EG2_Collision>(0), 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f)
	{
		VectorCopy(tr.endpos, spot);
		return qtrue;
	}

	// side
	VectorMA(org, 80, right, end);

	gi.trace(&tr, org, mins, maxs, end, skip, MASK_PLAYERSOLID, static_cast<EG2_Collision>(0), 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f)
	{
		VectorCopy(tr.endpos, spot);
		return qtrue;
	}

	// other side
	VectorMA(org, -80, right, end);

	gi.trace(&tr, org, mins, maxs, end, skip, MASK_PLAYERSOLID, static_cast<EG2_Collision>(0), 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f)
	{
		VectorCopy(tr.endpos, spot);
		return qtrue;
	}

	// behind
	VectorMA(org, -80, fwd, end);

	gi.trace(&tr, org, mins, maxs, end, skip, MASK_PLAYERSOLID, static_cast<EG2_Collision>(0), 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction >= 1.0f)
	{
		VectorCopy(tr.endpos, spot);
		return qtrue;
	}

	return qfalse;
}

extern void SP_NPC_Droid_Seeker(gentity_t* ent);

/*
================
Cmd_UseSeeker_f
================
*/
static void Cmd_UseSeeker_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	// don't use them if we don't have any...also don't use them if one is already going
	if (ent->client && ent->client->ps.inventory[INV_SEEKER] > 0 && level.time > ent->client->ps.powerups[PW_SEEKER])
	{
		gentity_t* tent = G_Spawn();

		if (tent)
		{
			vec3_t fwd, right, spot;

			AngleVectors(ent->client->ps.viewangles, fwd, right, nullptr);

			VectorCopy(ent->currentOrigin, spot); // does nothing really, just initialize the goods...

			if (pick_seeker_spawn_point(ent->currentOrigin, fwd, right, ent->s.number, spot))
			{
				VectorCopy(spot, tent->s.origin);
				G_SetOrigin(tent, spot);
				G_SetAngles(tent, ent->currentAngles);
				//SP_NPC_Droid_Remote(tent);
				SP_NPC_Droid_Seeker(tent);
				G_Sound(tent, G_SoundIndex("sound/chars/seeker/misc/hiss"));

				// make sure that we even have some
				ent->client->ps.inventory[INV_SEEKER]--;
				ent->client->ps.powerups[PW_SEEKER] = level.time + 1000;
				// can only drop one every second..maybe this is annoying?
			}
		}
		NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
}

/*
================
Cmd_UseGoggles_f
================
*/
static void Cmd_UseGoggles_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	if (ent->client && ent->client->ps.inventory[INV_LIGHTAMP_GOGGLES] > 0)
	{
		G_AddEvent(ent, EV_USE_INV_LIGHTAMP_GOGGLES, 0);
	}
}

/*
================
Cmd_UseSentry_f
================
*/
qboolean place_portable_assault_sentry(gentity_t* self, vec3_t origin, vec3_t dir);

static void Cmd_UseSentry_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}

	if (G_IsRidingVehicle(ent))
	{
		return;
	}
	if (ent->client->ps.inventory[INV_SENTRY] <= 0)
	{
		// have none to place...play sound?
		return;
	}

	if (place_portable_assault_sentry(ent, ent->currentOrigin, ent->client->ps.viewangles))
	{
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		ent->client->ps.inventory[INV_SENTRY]--;
		G_AddEvent(ent, EV_USE_INV_SENTRY, 0);
	}
	else
	{
		// couldn't be placed....play a notification sound!!
	}
}

extern void CG_ChangeWeapon(int num);

static void Cmd_UseBarrier_f(gentity_t* ent)
{
	if (ent->health < 1 || in_camera)
	{
		return;
	}
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	if (ent->s.weapon == WP_SABER)
	{
		if (ent->client->ps.SaberActive())
		{
			ent->client->ps.SaberDeactivate();
			G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
		}
		CG_ChangeWeapon(WP_MELEE);

		const int actual_time = cg.time ? cg.time : level.time;
		qboolean clear_to_activate_barrier = qtrue;

		if (actual_time < 1500)
		{
			clear_to_activate_barrier = qfalse;
		}
		if (clear_to_activate_barrier)
		{
			ItemUse_Barrier_with_saber(ent);
		}
	}
	else
	{
		ItemUse_Barrier(ent);
	}
}

static void Cmd_StopBarrier_f(gentity_t* ent)
{
	RemoveBarrier(ent);
}

/*
================
Cmd_UseInventory_f
================
*/
static void Cmd_UseInventory_f(gentity_t* ent)
{
	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	switch (cg.inventorySelect)
	{
	case INV_ELECTROBINOCULARS:
		Cmd_UseElectrobinoculars_f(ent);
		return;
	case INV_BACTA_CANISTER:
		Cmd_UseBacta_f(ent);
		return;
	case INV_SEEKER:
		Cmd_UseSeeker_f(ent);
		return;
	case INV_LIGHTAMP_GOGGLES:
		Cmd_UseGoggles_f(ent);
		return;
	case INV_SENTRY:
		Cmd_UseSentry_f(ent);
		return;
	case INV_CLOAK:
		Cmd_UseCloak_f(ent);
		return;
	case INV_BARRIER:
		Cmd_UseBarrier_f(ent);
		return;
	default:
		return;
	}
}

static void Cmd_FlushCamFile_f(gentity_t* ent)
{
	gi.FlushCamFile();
}

static void G_Taunt(gentity_t* ent)
{
	if (ent->client)
	{
		if (ent->client->ps.weapon == WP_SABER
			&& (ent->client->ps.saberAnimLevel == SS_STAFF
				|| ent->client->ps.dualSabers))
		{
			ent->client->ps.taunting = level.time + 100;
			//make sure all sabers are on
			ent->client->ps.SaberActivate();
			if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
			{
				G_RemoveHolsterModels(ent);
				wp_saber_add_g2_saber_models(ent, qtrue);
			}
		}
		else
		{
			ent->client->ps.taunting = level.time + 100;
		}
	}
}

static void G_Victory(const gentity_t* ent)
{
	if (ent->health > 0)
	{
		//say something and put away saber
		G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/kyle/misc/taunt1.wav");
		if (ent->client)
		{
			ent->client->ps.SaberDeactivate();
		}
	}
}

enum
{
	TAUNT_TAUNT = 0,
	TAUNT_BOW,
	TAUNT_MEDITATE,
	TAUNT_FLOURISH,
	TAUNT_GLOAT,
	TAUNT_SURRENDER,
	TAUNT_RELOAD
};

static void G_TauntSound(gentity_t* ent, int taunt)
{
	if (BG_IsAlreadyinTauntAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	switch (taunt)
	{
	case TAUNT_TAUNT:
	default:
		if (Q_irand(0, 1))
		{
			G_SpeechEvent(ent, Q_irand(EV_ANGER1, EV_ANGER3));
		}
		else
		{
			G_SpeechEvent(ent, Q_irand(EV_TAUNT1, EV_TAUNT3));
		}
		break;
	case TAUNT_BOW:
		break;
	case TAUNT_MEDITATE:
		//G_AddVoiceEvent(ent, EV_PUSHFAIL, 0);
		break;
	case TAUNT_FLOURISH:
		if (Q_irand(0, 1))
		{
			G_SpeechEvent(ent, Q_irand(EV_DEFLECT1, EV_DEFLECT3));
		}
		else
		{
			G_SpeechEvent(ent, Q_irand(EV_GLOAT1, EV_GLOAT3));
		}
		break;
	case TAUNT_GLOAT:
		G_SpeechEvent(ent, Q_irand(EV_VICTORY1, EV_VICTORY3));
		break;
	case TAUNT_SURRENDER:
		if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
		{
			G_SpeechEvent(ent, Q_irand(EV_GLOAT1, EV_GLOAT3));
		}
		else
		{
			if (Q_irand(0, 1))
			{
				G_SpeechEvent(ent, Q_irand(EV_CONFUSE1, EV_CONFUSE3));
			}
			else
			{
				G_SpeechEvent(ent, Q_irand(EV_GIVEUP1, EV_GIVEUP4));
			}
		}
		break;
	case TAUNT_RELOAD:
		break;
	}
}

extern qboolean PM_CrouchAnim(int anim);
extern qboolean manual_saberblocking(const gentity_t* defender);
extern qboolean Block_Button_Held(const gentity_t* defender);
extern qboolean NPC_IsMando(const gentity_t* self);
extern qboolean PM_RestAnim(int anim);

void G_SetTauntAnim(gentity_t* ent, const int taunt)
{
	const qboolean is_holding_block_button = ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->painDebounceTime > level.time)
	{
		return;
	}

	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (BG_IsAlreadyinTauntAnim(ent->client->ps.torsoAnim))
	{
		return;
	}

	if (taunt != TAUNT_RELOAD)
	{
		if (PM_CrouchAnim(ent->client->ps.legsAnim) ||
			PM_CrouchAnim(ent->client->ps.torsoAnim) ||
			PM_RestAnim(ent->client->ps.legsAnim) ||
			PM_RestAnim(ent->client->ps.torsoAnim))
		{
			return;
		}
	}

	if (is_holding_block_button || Block_Button_Held(ent))
	{
		return;
	}

	if (manual_saberblocking(ent))
	{
		return;
	}

	if (ent->client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
	{
		return;
	}

	if (G_IsRidingVehicle(ent))
	{
		return;
	}

	if (ent->client->ps.weaponTime <= 0 && ent->client->ps.saberLockTime < level.time)
	{
		int anim = -1;

		switch (taunt)
		{
		case TAUNT_TAUNT:
			G_TauntSound(ent, TAUNT_TAUNT);
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
			}
			else if (ent->client->ps.saber[0].tauntAnim != -1)
			{
				anim = ent->client->ps.saber[0].tauntAnim;
			}
			else if (ent->client->ps.dualSabers
				&& ent->client->ps.saber[1].tauntAnim != -1)
			{
				anim = ent->client->ps.saber[1].tauntAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saber[1].Active())
						{
							//turn off second saber
							G_Sound(ent, ent->client->ps.saber[1].soundOff);
						}
						else if (ent->client->ps.saber[0].Active())
						{
							//turn off first
							G_Sound(ent, ent->client->ps.saber[0].soundOff);
						}
						ent->client->ps.SaberDeactivate();
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (ent->client->ps.saber[0].type == SABER_VADER) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_DUAL:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						if (ent->client->ps.saber[0].type == SABER_GRIE || ent->client->ps.saber[0].type == SABER_GRIE4)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STAFF:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (ent->client->ps.saber[1].Active())
						{
							//turn off second saber
							G_Sound(ent, ent->client->ps.saber[1].soundOff);
						}
						else if (ent->client->ps.saber[0].Active())
						{
							//turn off first
							G_Sound(ent, ent->client->ps.saber[0].soundOff);
						}
						ent->client->ps.SaberDeactivate();
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (ent->client->ps.saber[0].type == SABER_VADER) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_DUAL:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						if (ent->client->ps.saber[0].type == SABER_GRIE || ent->client->ps.saber[0].type == SABER_GRIE4)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STAFF:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
			}
			break;
		case TAUNT_BOW:
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else if (ent->client->ps.saber[0].bowAnim != -1)
			{
				anim = ent->client->ps.saber[0].bowAnim;
			}
			else if (ent->client->ps.dualSabers
				&& ent->client->ps.saber[1].bowAnim != -1)
			{
				anim = ent->client->ps.saber[1].bowAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->ps.saber[1].Active())
					{
						//turn off second saber
						G_Sound(ent, ent->client->ps.saber[1].soundOff);
					}
					else if (ent->client->ps.saber[0].Active())
					{
						//turn off first
						G_Sound(ent, ent->client->ps.saber[0].soundOff);
					}
					ent->client->ps.SaberDeactivate();
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (ent->client->ps.saber[1].Active())
					{
						//turn off second saber
						G_Sound(ent, ent->client->ps.saber[1].soundOff);
					}
					else if (ent->client->ps.saber[0].Active())
					{
						//turn off first
						G_Sound(ent, ent->client->ps.saber[0].soundOff);
					}
					ent->client->ps.SaberDeactivate();
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			break;
		case TAUNT_MEDITATE:
			//G_TauntSound(ent, TAUNT_MEDITATE);
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						anim = BOTH_MEDITATE1;
					}
				}
			}
			else if (ent->client->ps.saber[0].meditateAnim != -1)
			{
				anim = ent->client->ps.saber[0].meditateAnim;
			}
			else if (ent->client->ps.dualSabers
				&& ent->client->ps.saber[1].meditateAnim != -1)
			{
				anim = ent->client->ps.saber[1].meditateAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					if (ent->client->ps.saber[1].Active())
					{
						//turn off second saber
						G_Sound(ent, ent->client->ps.saber[1].soundOff);
					}
					else if (ent->client->ps.saber[0].Active())
					{
						//turn off first
						G_Sound(ent, ent->client->ps.saber[0].soundOff);
					}
					ent->client->ps.SaberDeactivate();
					anim = BOTH_MEDITATE;
				}
			}
			break;
		case TAUNT_FLOURISH:
			G_TauntSound(ent, TAUNT_FLOURISH);
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL2,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL2,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
			}
			else if (ent->client->ps.saber[0].flourishAnim != -1)
			{
				anim = ent->client->ps.saber[0].flourishAnim;
			}
			else if (ent->client->ps.dualSabers && ent->client->ps.saber[1].flourishAnim != -1)
			{
				anim = ent->client->ps.saber[1].flourishAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_MEDIUM,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_MEDIUM,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
				ent->client->ps.SaberActivate();
				if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
				{
					G_RemoveHolsterModels(ent);
					wp_saber_add_g2_saber_models(ent, qtrue);
				}
			}
			break;
		case TAUNT_GLOAT:
			G_TauntSound(ent, TAUNT_GLOAT);
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
			}
			else if (ent->client->ps.saber[0].gloatAnim != -1)
			{
				anim = ent->client->ps.saber[0].gloatAnim;
			}
			else if (ent->client->ps.dualSabers
				&& ent->client->ps.saber[1].gloatAnim != -1)
			{
				anim = ent->client->ps.saber[1].gloatAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						break;
					case SS_MEDIUM:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						break;
					case SS_STRONG:
					case SS_DESANN:
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						break;
					case SS_DUAL:
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						break;
					case SS_STAFF:
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						break;
					default:;
					}
				}
				else
				{
					switch (ent->client->ps.saberAnimLevel)
					{
					case SS_FAST:
					case SS_TAVION:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_MEDIUM:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_MEDIUM, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STRONG:
					case SS_DESANN:
						if (!ent->client->ps.SaberActive())
						{
							//turn on the saber if it's not on
							ent->client->ps.SaberActivate();
						}
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_DUAL:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VICTORY_DUAL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					case SS_STAFF:
						ent->client->ps.SaberActivate();
						if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
						{
							G_RemoveHolsterModels(ent);
							wp_saber_add_g2_saber_models(ent, qtrue);
						}
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_STAFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						break;
					default:;
					}
				}
			}
			break;
		case TAUNT_SURRENDER:
			G_TauntSound(ent, TAUNT_SURRENDER);
			if (ent->client->ps.weapon != WP_SABER) //SP
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.weapon == WP_DISRUPTOR)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_TUSKENTAUNT1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.torsoAnim != BOTH_COWER1 && ent->client->ps.torsoAnim != BOTH_COWER1_START)
						{
							anim = BOTH_COWER1_START;
						}
						else if (ent->client->ps.torsoAnim == BOTH_COWER1_START)
						{
							anim = BOTH_COWER1;
						}
					}
				}
			}
			else if (ent->client->ps.saber[0].surrenderAnim != -1)
			{
				anim = ent->client->ps.saber[0].surrenderAnim;
			}
			else if (ent->client->ps.dualSabers
				&& ent->client->ps.saber[1].surrenderAnim != -1)
			{
				anim = ent->client->ps.saber[1].surrenderAnim;
			}
			else
			{
				if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
				{
					//TORSO ONLY
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else
				{
					if (ent->client->NPC_class == CLASS_VADER || ent->client->NPC_class == CLASS_DESANN)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						if (ent->client->ps.saber[1].Active())
						{
							//turn off second saber
							G_Sound(ent, ent->client->ps.saber[1].soundOff);
						}
						else if (ent->client->ps.saber[0].Active())
						{
							//turn off first
							G_Sound(ent, ent->client->ps.saber[0].soundOff);
						}
						ent->client->ps.SaberDeactivate();
						anim = PLAYER_SURRENDER_START;
					}
				}
			}
			break;
		case TAUNT_RELOAD:
			if (IsHoldingReloadableGun(ent)) //SP
			{
				if (ent->reloadTime > 0)
				{
					CancelReload(ent);
				}
				else
				{
					WP_ReloadGun(ent);
				}
				break;
			}
			if (PM_WalkingAnim(ent->client->ps.legsAnim) || PM_RunningAnim(ent->client->ps.legsAnim))
			{
				//TORSO ONLY
				switch (ent->client->ps.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					if (ent->client->ps.saber[1].Active())
					{
						//turn off second saber
						G_Sound(ent, ent->client->ps.saber[1].soundOff);
					}
					else if (ent->client->ps.saber[0].Active())
					{
						//turn off first
						G_Sound(ent, ent->client->ps.saber[0].soundOff);
					}
					ent->client->ps.SaberDeactivate();
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STRONG:
				case SS_DESANN:
					if (ent->client->ps.saber[0].type == SABER_VADER) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_DUAL:
					ent->client->ps.SaberActivate();
					if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
					{
						G_RemoveHolsterModels(ent);
						wp_saber_add_g2_saber_models(ent, qtrue);
					}
					if (ent->client->ps.saber[0].type == SABER_GRIE || ent->client->ps.saber[0].type == SABER_GRIE4)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STAFF:
					ent->client->ps.SaberActivate();
					if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
					{
						G_RemoveHolsterModels(ent);
						wp_saber_add_g2_saber_models(ent, qtrue);
					}
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}
			else
			{
				switch (ent->client->ps.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
					if (ent->client->ps.saber[1].Active())
					{
						//turn off second saber
						G_Sound(ent, ent->client->ps.saber[1].soundOff);
					}
					else if (ent->client->ps.saber[0].Active())
					{
						//turn off first
						G_Sound(ent, ent->client->ps.saber[0].soundOff);
					}
					ent->client->ps.SaberDeactivate();
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_GESTURE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					if (ent->client->ps.saber[0].type == SABER_OBIWAN) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_SHOWOFF_OBI,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STRONG:
				case SS_DESANN:
					if (ent->client->ps.saber[0].type == SABER_VADER) //saber kylo
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_VADERTAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ENGAGETAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_DUAL:
					ent->client->ps.SaberActivate();
					if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
					{
						G_RemoveHolsterModels(ent);
						wp_saber_add_g2_saber_models(ent, qtrue);
					}
					if (ent->client->ps.saber[0].type == SABER_GRIE || ent->client->ps.saber[0].type == SABER_GRIE4)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					break;
				case SS_STAFF:
					ent->client->ps.SaberActivate();
					if (ent->client->ps.dualSabers && ent->weaponModel[1] == -1)
					{
						G_RemoveHolsterModels(ent);
						wp_saber_add_g2_saber_models(ent, qtrue);
					}
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
			}
			break;
		default:;
		}

		if (anim != -1)
		{
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE || NPC_IsMando(ent))
			{
				int parts = SETANIM_TORSO;

				if (anim != BOTH_ENGAGETAUNT)
				{
					parts = SETANIM_BOTH;
					VectorClear(ent->client->ps.velocity);
				}
				NPC_SetAnim(ent, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

void G_SetsaberdownorAnim(gentity_t* ent)
{
	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}
	if (ent->client->ps.weapon == WP_SABER)
	{
		// put away saber
		if (ent->client->ps.SaberActive())
		{
			if (ent->client->ps.saber[1].Active())
			{
				//turn off second saber
				G_Sound(ent, ent->client->ps.saber[1].soundOff);
			}
			else if (ent->client->ps.saber[0].Active())
			{
				//turn off first
				G_Sound(ent, ent->client->ps.saber[0].soundOff);
			}
			ent->client->ps.SaberDeactivate();
		}
		else
		{
			if (!ent->client->ps.SaberActive())
			{
				//turn on the saber if it's not on
				ent->client->ps.SaberActivate();
			}
		}
		ent->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
		ent->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	}
	else
	{
		if (IsHoldingReloadableGun(ent)) //SP
		{
			if (ent->reloadTime > 0)
			{
				CancelReload(ent);
			}
			else
			{
				WP_ReloadGun(ent);
			}
		}
		else
		{
			G_SetTauntAnim(ent, TAUNT_TAUNT);
		}
	}
}

extern cvar_t* g_saberPickuppableDroppedSabers;
extern void WP_RemoveSaber(gentity_t* ent, int saberNum);
extern void CG_ChangeWeapon(int num);
extern void ChangeWeapon(const gentity_t* ent, int new_weapon);

static void Cmd_SaberDrop_f(gentity_t* ent, const int saberNum)
{
	if (saberNum < 0)
	{
		return;
	}
	if (saberNum > 1)
	{
		return;
	}
	if (!ent || !ent->client)
	{
		return;
	}
	if (ent->weaponModel[saberNum] <= 0)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	if (ent->client->ps.weaponTime > 0)
	{
		return;
	}

	if (ent->client->ps.saber_move != LS_READY
		&& ent->client->ps.saber_move != LS_PUTAWAY
		&& ent->client->ps.saber_move != LS_DRAW
		&& ent->client->ps.saber_move != LS_DRAW2
		&& ent->client->ps.saber_move != LS_DRAW3
		&& ent->client->ps.saber_move != LS_NONE)
	{
		return;
	}

	if (!g_saberPickuppableDroppedSabers->integer)
	{
		return;
	}

	if (!ent->client->ps.saber[saberNum].name
		|| !ent->client->ps.saber[saberNum].name[0])
	{
		return;
	}

	//have a valid string to use for saberType

	//turn it into a pick-uppable item!
	if (G_DropSaberItem(ent->client->ps.saber[saberNum].name,
		ent->client->ps.saber[saberNum].blade[0].color,
		saberNum == 0 ? ent->client->renderInfo.handRPoint : ent->client->renderInfo.handLPoint,
		ent->client->ps.velocity,
		ent->currentAngles)
		!= nullptr)
	{
		//dropped it
		WP_RemoveSaber(ent, saberNum);
	}

	if (ent->weaponModel[0] <= 0
		&& ent->weaponModel[1] <= 0)
	{
		//no sabers left!
		//remove saber from inventory
		ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
		//change weapons
		if (ent->s.number < MAX_CLIENTS)
		{
			//player
			CG_ChangeWeapon(WP_NONE);
		}
		else
		{
			ChangeWeapon(ent, WP_NONE);
		}
		ent->client->ps.weapon = WP_NONE;
	}
}

void G_RemoveWeather()
{
	gi.SendConsoleCommand(va("execq Weather/clear.cfg"));
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(const int clientNum)
{
	gentity_t* ent = g_entities + clientNum;
	if (!ent->client)
	{
		return; // not fully in game yet
	}

	const char* cmd = gi.argv(0);

	if (Q_stricmp(cmd, "spawn") == 0)
	{
		Cmd_Spawn(ent);
		return;
	}

	if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "undying") == 0)
		Cmd_Undying_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noforce") == 0)
		Cmd_Noforce_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
	{
		Cmd_Noclip_f(ent);
	}
	else if (Q_stricmp(cmd, "kill") == 0)
	{
		if (!CheatsOk(ent))
		{
			return;
		}
		Cmd_Kill_f(ent);
	}
	else if (Q_stricmp(cmd, "levelshot") == 0)
		Cmd_LevelShot_f(ent);
	else if (Q_stricmp(cmd, "where") == 0)
		Cmd_Where_f(ent);
	else if (Q_stricmp(cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f(ent);
	else if (Q_stricmp(cmd, "setobjective") == 0)
		Cmd_SetObjective_f(ent);
	else if (Q_stricmp(cmd, "viewobjective") == 0)
		Cmd_ViewObjective_f(ent);
	else if (Q_stricmp(cmd, "force_throw") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceThrow(ent, qfalse);
	}
	else if (Q_stricmp(cmd, "force_pull") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceThrow(ent, qtrue);
	}
	else if (Q_stricmp(cmd, "force_speed") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceSpeed(ent);
	}
	else if (Q_stricmp(cmd, "force_heal") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceHeal(ent);
	}
	else if (Q_stricmp(cmd, "force_grip") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		if (ent->client->ps.forcePower < 90)
		{
			ForceGripBasic(ent);
		}
		else
		{
			ForceGripAdvanced(ent);
		}
	}
	else if (Q_stricmp(cmd, "force_distract") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceTelepathy(ent);
	}
	else if (Q_stricmp(cmd, "force_rage") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceRage(ent);
	}
	else if (Q_stricmp(cmd, "force_protect") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceProtect(ent);
	}
	else if (Q_stricmp(cmd, "force_absorb") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceAbsorb(ent);
	}
	else if (Q_stricmp(cmd, "force_sight") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceSeeing(ent);
	}
	else if (Q_stricmp(cmd, "force_stasis") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		force_stasis(ent);
	}
	else if (Q_stricmp(cmd, "force_deadlysight") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceDeadlySight(ent);
	}
	else if (Q_stricmp(cmd, "force_destruction") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceDestruction(ent);
	}
	else if (Q_stricmp(cmd, "force_grasp") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceGrasp(ent);
	}
	else if (Q_stricmp(cmd, "force_insanity") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceInsanity(ent);
	}
	else if (Q_stricmp(cmd, "force_blinding") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceBlinding(ent);
	}
	else if (Q_stricmp(cmd, "force_blast") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceBlast(ent);
	}
	else if (Q_stricmp(cmd, "force_repulse") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceRepulse(ent);
		ent->client->ps.powerups[PW_INVINCIBLE] = level.time + ent->client->ps.torsoAnimTimer + 2000;
	}
	else if (Q_stricmp(cmd, "force_strike") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceLightningStrike(ent);
	}
	else if (Q_stricmp(cmd, "force_fear") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		ForceFear(ent);
	}
	else if (Q_stricmp(cmd, "addsaberstyle") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		if (!ent || !ent->client)
		{
			//wtf?
			return;
		}
		if (gi.argc() < 2)
		{
			gi.SendServerCommand(ent - g_entities, va("print \"usage: addsaberstyle <saber style>\n\""));
			gi.SendServerCommand(ent - g_entities,
				va(
					"print \"Valid styles: SS_FAST, SS_MEDIUM, SS_STRONG, SS_DESANN, SS_TAVION, SS_DUAL and SS_STAFF\n\""));
			return;
		}

		const int addStyle = GetIDForString(SaberStyleTable, gi.argv(1));
		if (addStyle > SS_NONE && addStyle < SS_STAFF)
		{
			ent->client->ps.saberStylesKnown |= 1 << addStyle;
		}
	}
	else if (Q_stricmp(cmd, "setsaberstyle") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		if (!ent || !ent->client)
		{
			//wtf?
			return;
		}
		if (gi.argc() < 2)
		{
			gi.SendServerCommand(ent - g_entities, va("print \"usage: setsaberstyle <saber style>\n\""));
			gi.SendServerCommand(ent - g_entities,
				va(
					"print \"Valid styles: SS_FAST, SS_MEDIUM, SS_STRONG, SS_DESANN, SS_TAVION, SS_DUAL and SS_STAFF\n\""));
			return;
		}

		const int setStyle = GetIDForString(SaberStyleTable, gi.argv(1));
		if (setStyle > SS_NONE && setStyle < SS_STAFF)
		{
			ent->client->ps.saberStylesKnown = 1 << setStyle;
			cg.saber_anim_levelPending = ent->client->ps.saberAnimLevel = setStyle;
		}
	}
	else if (Q_stricmp(cmd, "saberdown") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetsaberdownorAnim(ent);
	}
	else if (Q_stricmp(cmd, "taunt") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_TAUNT);
	}
	else if (Q_stricmp(cmd, "bow") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_BOW);
	}
	else if (Q_stricmp(cmd, "meditate") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_MEDITATE);
	}
	else if (Q_stricmp(cmd, "flourish") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_FLOURISH);
	}
	else if (Q_stricmp(cmd, "gloat") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_GLOAT);
	}
	else if (Q_stricmp(cmd, "surrender") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_SURRENDER);
	}
	else if (Q_stricmp(cmd, "reload") == 0)
	{
		ent = G_GetSelfForPlayerCmd();
		G_SetTauntAnim(ent, TAUNT_RELOAD);
	}
	else if (Q_stricmp(cmd, "NPCdrive") == 0)
	{
		if (!CheatsOk(ent))
		{
			return;
		}
		if (gi.argc() < 3)
		{
			gi.SendServerCommand(ent - g_entities, va("print \"usage: drive <NPC_targetname> <vehicle name>\n\""));
			gi.SendServerCommand(ent - g_entities,
				va("print \"Vehicles will be in vehicles.cfg, try using 'speeder' for now\n\""));
			return;
		}
		const gentity_t* found = G_Find(nullptr, FOFS(targetname), gi.argv(1));
		if (found && found->health > 0 && found->client)
		{
			// TEMPORARY! BRING BACK LATER!!!
			//G_DriveVehicle( found, NULL, gi.argv(2) );
		}
	}
	else if (Q_stricmp(cmd, "victory") == 0)
		G_Victory(ent);
	else if (Q_stricmp(cmd, "fly_xwing") == 0)
		G_PilotXWing(ent);
	else if (Q_stricmp(cmd, "drive_atst") == 0)
	{
		G_DriveATST(ent, nullptr);
	}
	else if (Q_stricmp(cmd, "thereisnospoon") == 0)
		G_StartMatrixEffect(ent);
	else if (Q_stricmp(cmd, "fx") == 0)
		Cmd_Fx(ent);
	else if (Q_stricmp(cmd, "use_electrobinoculars") == 0)
	{
		Cmd_UseElectrobinoculars_f(ent);
	}
	else if (Q_stricmp(cmd, "use_bacta") == 0)
	{
		Cmd_UseBacta_f(ent);
	}
	else if (Q_stricmp(cmd, "use_seeker") == 0)
	{
		Cmd_UseSeeker_f(ent);
	}
	else if (Q_stricmp(cmd, "use_lightamp_goggles") == 0)
	{
		Cmd_UseGoggles_f(ent);
	}
	else if (Q_stricmp(cmd, "use_sentry") == 0)
	{
		Cmd_UseSentry_f(ent);
	}
	else if (Q_stricmp(cmd, "use_cloak") == 0)
	{
		Cmd_UseCloak_f(ent);
	}
	else if (Q_stricmp(cmd, "use_barrier") == 0)
	{
		Cmd_UseBarrier_f(ent);
	}
	else if (Q_stricmp(cmd, "stop_barrier") == 0)
	{
		Cmd_StopBarrier_f(ent);
	}
	else if (Q_stricmp(cmd, "invuse") == 0)
	{
		Cmd_UseInventory_f(ent);
	}
	else if (Q_stricmp(cmd, "playmusic") == 0)
	{
		const char* cmd2 = gi.argv(1);
		if (cmd2)
		{
			gi.SetConfigstring(CS_MUSIC, cmd2);
		}
	}
	else if (Q_stricmp(cmd, "flushcam") == 0)
	{
		Cmd_FlushCamFile_f(ent);
	}
	else if (Q_stricmp(cmd, "dropsaber") == 0)
	{
		const char* cmd2 = gi.argv(1);
		int saberNum = 2; //by default, drop both
		if (cmd2 && cmd2[0])
		{
			saberNum = atoi(cmd2);
		}
		if (saberNum > 1)
		{
			//drop both
			Cmd_SaberDrop_f(ent, 1);
			Cmd_SaberDrop_f(ent, 0);
		}
		else
		{
			//drop either left or right
			Cmd_SaberDrop_f(ent, saberNum);
		}
	}
	else if ((Q_stricmp(cmd, "weather") == 0) || (Q_stricmp(cmd, "r_weather") == 0))
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
	else if (TryWorkshopCommand(ent))
	{
		//
	}
	else
	{
		gi.SendServerCommand(clientNum, va("print \"Unknown command %s\n\"", cmd));
	}
}