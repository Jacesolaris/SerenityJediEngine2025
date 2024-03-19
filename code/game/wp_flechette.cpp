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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//-----------------------
//	Golan Arms Flechette
//-----------------------

extern qboolean walk_check(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
//---------------------------------------------------------
static void WP_FlechetteMainFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t angs, start;
	float damage = weaponData[WP_FLECHETTE].damage, vel = FLECHETTE_VEL;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	// If we aren't the player, we will cut the velocity and damage of the shots
	if (ent->s.number)
	{
		damage *= 0.75f;
		vel *= 0.5f;
	}

	for (int i = 0; i < FLECHETTE_SHOTS; i++)
	{
		vec3_t fwd;
		vectoangles(forward_vec, angs);

		if (i == 0 && ent->s.number == 0)
		{
			// do nothing on the first shot for the player, this one will hit the crosshairs
		}
		else
		{
			if (NPC_IsNotHavingEnoughForceSight(ent))
			{
				//force sight 2+ gives perfect aim
				if (ent->NPC && ent->NPC->currentAim < 5)
				{
					if (ent->client && ent->NPC &&
						(ent->client->NPC_class == CLASS_STORMTROOPER ||
							ent->client->NPC_class == CLASS_CLONETROOPER ||
							ent->client->NPC_class == CLASS_STORMCOMMANDO ||
							ent->client->NPC_class == CLASS_SWAMPTROOPER ||
							ent->client->NPC_class == CLASS_DROIDEKA ||
							ent->client->NPC_class == CLASS_SBD ||
							ent->client->NPC_class == CLASS_IMPWORKER ||
							ent->client->NPC_class == CLASS_REBEL ||
							ent->client->NPC_class == CLASS_WOOKIE ||
							ent->client->NPC_class == CLASS_BATTLEDROID))
					{
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) *0.25f); //was 0.5f
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
						//was 0.5
					}
				}
				else if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
					//if running aim is shit
				{
					angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
					angs[YAW] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
				}
				else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_ELEVEN)
				{
					// add some slop to the fire direction
					angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
					angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				}
				else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
				{
					// add some slop to the fire direction
					angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
					angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				}
				else if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{
					//
				}
				else
				{
					//
				}
			}
		}

		AngleVectors(angs, fwd, nullptr, nullptr);

		WP_MissileTargetHint(ent, start, fwd);

		gentity_t* missile = CreateMissile(start, fwd, vel, 10000, ent);

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;

		VectorSet(missile->maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE);
		VectorScale(missile->maxs, -1, missile->mins);

		missile->damage = damage;

		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(1, 2);

		missile->s.eFlags |= EF_BOUNCE_SHRAPNEL;
		ent->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
void prox_mine_think(gentity_t* ent)
//---------------------------------------------------------
{
	qboolean blow = qfalse;

	// if it isn't time to auto-explode, do a small proximity check
	if (ent->delay > level.time)
	{
		const int count = G_RadiusList(ent->currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list);

		for (int i = 0; i < count; i++)
		{
			if (ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->
				activator->s.number)
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if (blow)
	{
		//		G_Sound( ent, G_SoundIndex( "sound/weapons/flechette/warning.wav" ));
		ent->e_ThinkFunc = thinkF_WP_Explode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//---------------------------------------------------------
void prox_mine_stick(gentity_t* self, gentity_t* other, const trace_t* trace)
//---------------------------------------------------------
{
	// turn us into a generic entity so we aren't running missile code
	self->s.eType = ET_GENERAL;

	self->s.modelindex = G_ModelIndex("models/weapons2/golan_arms/prox_mine.md3");
	self->e_TouchFunc = touchF_NULL;

	self->contents = CONTENTS_SOLID;
	self->takedamage = qtrue;
	self->health = 5;
	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet(self->maxs, 5, 5, 5);
	VectorScale(self->maxs, -1, self->mins);

	self->activator = self->owner;
	self->owner = nullptr;

	WP_Stick(self, trace);

	self->e_ThinkFunc = thinkF_prox_mine_think;
	self->nextthink = level.time + 450;

	// sticks for twenty seconds, then auto blows.
	self->delay = level.time + 20000;

	gi.linkentity(self);
}

//----------------------------------------------
void wp_flechette_alt_blow(gentity_t* ent)
//----------------------------------------------
{
	EvaluateTrajectory(&ent->s.pos, level.time, ent->currentOrigin);
	// Not sure if this is even necessary, but correct origins are cool?

	G_RadiusDamage(ent->currentOrigin, ent->owner, ent->splashDamage, ent->splashRadius, nullptr, MOD_EXPLOSIVE_SPLASH);
	G_PlayEffect("flechette/alt_blow", ent->currentOrigin);

	G_FreeEntity(ent);
}

//------------------------------------------------------------------------------
static void WP_CreateFlechetteBouncyThing(vec3_t start, vec3_t fwd, gentity_t* self)
//------------------------------------------------------------------------------
{
	gentity_t* missile = CreateMissile(start, fwd, 950 + Q_flrand(0.0f, 1.0f) * 700,
		1500 + Q_flrand(0.0f, 1.0f) * 2000, self, qtrue);

	missile->e_ThinkFunc = thinkF_wp_flechette_alt_blow;

	missile->s.weapon = WP_FLECHETTE;
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet(missile->mins, -3.0f, -3.0f, -3.0f);
	VectorSet(missile->maxs, 3.0f, 3.0f, 3.0f);
	missile->clipmask = MASK_SHOT;
	missile->clipmask &= ~CONTENTS_CORPSE;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_BOUNCE_HALF;

	missile->damage = weaponData[WP_FLECHETTE].altDamage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->splashDamage = weaponData[WP_FLECHETTE].altSplashDamage;
	missile->splashRadius = weaponData[WP_FLECHETTE].altSplashRadius;

	missile->svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT;

	VectorCopy(start, missile->pos2);
}

//---------------------------------------------------------
static void WP_FlechetteAltFire(gentity_t* ent)
//---------------------------------------------------------
{
	vec3_t dir, start, angs;

	vectoangles(forward_vec, angs);
	VectorCopy(muzzle, start);

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_HALF)
	{
		NPC_SetAnim(ent, SETANIM_BOTH, BOTH_H1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	for (int i = 0; i < 2; i++)
	{
		vec3_t fwd;
		VectorCopy(angs, dir);

		dir[PITCH] -= Q_flrand(0.0f, 1.0f) * 4 + 8; // make it fly upwards
		dir[YAW] += Q_flrand(-1.0f, 1.0f) * 2;
		AngleVectors(dir, fwd, nullptr, nullptr);

		WP_CreateFlechetteBouncyThing(start, fwd, ent);
		ent->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
void WP_FireFlechette(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	if (alt_fire)
	{
		WP_FlechetteAltFire(ent);
	}
	else
	{
		WP_FlechetteMainFire(ent);
	}
}