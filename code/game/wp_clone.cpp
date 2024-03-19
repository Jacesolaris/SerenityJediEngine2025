/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
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

//---------------
//	DC-15 Carbine
//---------------

extern cvar_t* g_SerenityJediEngineMode;
//---------------------------------------------------------
void WP_FireCloneMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = alt_fire ? weaponData[WP_CLONECARBINE].altDamage : weaponData[WP_CLONECARBINE].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_CLONECARBINE;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);

//---------------------------------------------------------
void WP_FireClone(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
	{//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireCloneMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	DC-15 Rifle
//---------------

//---------------------------------------------------------
void WP_FireCloneRifleMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = CLONERIFLE_VELOCITY;
	int damage = alt_fire ? weaponData[WP_CLONERIFLE].altDamage : weaponData[WP_CLONERIFLE].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= CLONERIFLE_NPC_VEL_CUT;
			}
			else
			{
				velocity *= CLONERIFLE_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_CLONERIFLE;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_JANGO &&
		ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = CLONERIFLE_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = CLONERIFLE_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = CLONERIFLE_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_CLONERIFLE_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_CLONERIFLE;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireCloneRifle(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
	{//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireCloneRifleMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	DC-17
//---------------

//---------------------------------------------------------
void WP_FireCloneCommandoMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = CLONECOMMANDO_VELOCITY;
	int damage = alt_fire ? weaponData[WP_CLONECOMMANDO].altDamage : weaponData[WP_CLONECOMMANDO].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= CLONECOMMANDO_NPC_VEL_CUT;
			}
			else
			{
				velocity *= CLONECOMMANDO_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_CLONECOMMANDO;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_CLONECOMMANDO_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_CLONECOMMANDO;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireCloneCommando(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
	{//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.5f, 1.5f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
					}
				}
			}
			else
			{// add some slop to the alt-fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
			{
				if (PM_CrouchAnim(ent->client->ps.legsAnim))
				{// firing position
					angs[PITCH] += Q_flrand(-0.0f, 0.0f);
					angs[YAW] += Q_flrand(-0.0f, 0.0f);
				}
				else
				{
					if (PM_RunningAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
					{ // running or very fatigued
						angs[PITCH] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
						angs[YAW] += Q_flrand(-2.0f, 2.0f) * RUNNING_SPREAD;
					}
					else if (PM_WalkingAnim(ent->client->ps.legsAnim) || ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
					{//walking or fatigued a bit
						angs[PITCH] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
						angs[YAW] += Q_flrand(-1.1f, 1.1f) * WALKING_SPREAD;
					}
					else
					{// just standing
						angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
						angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
					}
				}
			}
			else
			{// add some slop to the fire direction for NPC,s
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireCloneCommandoMissile(ent, muzzle, dir, alt_fire);
}