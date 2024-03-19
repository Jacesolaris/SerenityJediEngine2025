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
#include "ghoul2/G2.h"
#include "qcommon/q_shared.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn appropriately.
*/

#define	RESPAWN_ARMOR		20
#define	RESPAWN_TEAM_WEAPON	30
#define	RESPAWN_HEALTH		30
#define	RESPAWN_AMMO		40
#define	RESPAWN_HOLDABLE	60
#define	RESPAWN_MEGAHEALTH	120
#define	RESPAWN_POWERUP		120

// Item Spawn flags
#define ITMSF_SUSPEND		1
#define ITMSF_NOPLAYER		2
#define ITMSF_ALLOWNPC		4
#define ITMSF_NOTSOLID		8
#define ITMSF_VERTICAL		16
#define ITMSF_INVISIBLE		32

extern gentity_t* droppedRedFlag;
extern gentity_t* droppedBlueFlag;
qboolean AllForceDisabled(int force);
extern void Player_CheckBurn(const gentity_t* self);
extern void player_Burn(const gentity_t* self);

//======================================================================
#define MAX_MEDPACK_HEAL_AMOUNT		25
#define MAX_MEDPACK_BIG_HEAL_AMOUNT	50
#define MAX_SENTRY_DISTANCE			256
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);

// For more than four players, adjust the respawn times, up to 1/4.
static int adjustRespawnTime(const float preRespawnTime, const int itemType, const int itemTag)
{
	float respawnTime = preRespawnTime;

	if (itemType == IT_WEAPON)
	{
		if (itemTag == WP_THERMAL ||
			itemTag == WP_TRIP_MINE ||
			itemTag == WP_DET_PACK)
		{
			//special case for these, use ammo respawn rate
			respawnTime = RESPAWN_AMMO;
		}
	}

	if (!g_adaptRespawn.integer)
	{
		return (int)respawnTime;
	}

	if (level.numPlayingClients > 4)
	{
		// Start scaling the respawn times.
		if (level.numPlayingClients > 32)
		{
			// 1/4 time minimum.
			respawnTime *= 0.25;
		}
		else if (level.numPlayingClients > 12)
		{
			// From 12-32, scale from 0.5 to 0.25;
			respawnTime *= 20.0 / (float)(level.numPlayingClients + 8);
		}
		else
		{
			// From 4-12, scale from 1.0 to 0.5;
			respawnTime *= 8.0f / (float)(level.numPlayingClients + 4);
		}
	}

	if (respawnTime < 1.0)
	{
		// No matter what, don't go lower than 1 second, or the pickups become very noisy!
		respawnTime = 1.0;
	}

	return (int)respawnTime;
}

#define SHIELD_HEALTH				250
#define SHIELD_HEALTH_DEC			10		// 25 seconds
#define MAX_SHIELD_HEIGHT			254
#define MAX_SHIELD_HALFWIDTH		255
#define SHIELD_HALFTHICKNESS		4
#define SHIELD_PLACEDIST			64

#define SHIELD_SIEGE_HEALTH			2000
#define SHIELD_SIEGE_HEALTH_DEC		(SHIELD_SIEGE_HEALTH/25)	// still 25 seconds.

static qhandle_t shieldLoopSound = 0;
static qhandle_t shieldAttachSound = 0;
static qhandle_t shieldActivateSound = 0;
static qhandle_t shieldDeactivateSound = 0;
static qhandle_t shieldDamageSound = 0;

static void ShieldRemove(gentity_t* self)
{
	self->parent->forceFieldThink = level.time + 30000;
	self->think = G_FreeEntity;
	self->nextthink = level.time + 100;

	// Play kill sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDeactivateSound);
	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;
}

// Count down the health of the shield.
static void ShieldThink(gentity_t* self)
{
	self->s.trickedentindex = 0;

	if (level.gametype == GT_SIEGE || level.gametype == GT_CTF)
	{
		self->health -= SHIELD_SIEGE_HEALTH_DEC;
	}
	else
	{
		self->health -= SHIELD_HEALTH_DEC;
	}
	self->nextthink = level.time + 1000;

	if (self->health <= 0)
	{
		ShieldRemove(self);
	}
}

// The shield was damaged to below zero health.
static void ShieldDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	ShieldRemove(self);
}

// The shield had damage done to it.  Make it flicker.
static void ShieldPain(gentity_t* self, gentity_t* attacker, int damage)
{
	// Set the itemplaceholder flag to indicate the the shield drawing that the shield pain should be drawn.
	self->think = ShieldThink;
	self->nextthink = level.time + 400;

	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	self->s.trickedentindex = 1;
}

// Try to turn the shield back on after a delay.
static void ShieldGoSolid(gentity_t* self)
{
	trace_t tr;

	// see if we're valid
	self->health--;
	if (self->health <= 0)
	{
		ShieldRemove(self);
		return;
	}

	trap->Trace(&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, self->r.currentOrigin, self->s.number,
		CONTENTS_BODY, qfalse, 0, 0);
	if (tr.startsolid)
	{
		// gah, we can't activate yet
		self->nextthink = level.time + 200;
		self->think = ShieldGoSolid;
		trap->LinkEntity((sharedEntity_t*)self);
	}
	else
	{
		// get hard... huh-huh...
		self->s.eFlags &= ~EF_NODRAW;

		self->r.contents = CONTENTS_SOLID;
		self->nextthink = level.time + 1000;
		self->think = ShieldThink;
		self->takedamage = qtrue;
		trap->LinkEntity((sharedEntity_t*)self);

		// Play raising sound...
		G_AddEvent(self, EV_GENERAL_SOUND, shieldActivateSound);
		self->s.loopSound = shieldLoopSound;
		self->s.loopIsSoundset = qfalse;
	}
}

// Turn the shield off to allow a friend to pass through.
static void ShieldGoNotSolid(gentity_t* self)
{
	// make the shield non-solid very briefly
	self->r.contents = 0;
	self->s.eFlags |= EF_NODRAW;
	// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
	self->nextthink = level.time + 200;
	self->think = ShieldGoSolid;
	self->takedamage = qfalse;
	trap->LinkEntity((sharedEntity_t*)self);

	// Play kill sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDeactivateSound);
	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;
}

// tests if ent has other as ally
qboolean sje_is_ally(const gentity_t* ent, const gentity_t* other)
{
	if (ent && other && !ent->NPC && !other->NPC && ent != other && ent->client && other->client && other->client->pers.
		connected == CON_CONNECTED)
	{
		if (other->s.number > 15 && ent->client->sess.ally2 & 1 << (other->s.number - 16))
		{
			return qtrue;
		}
		if (ent->client->sess.ally1 & 1 << other->s.number)
		{
			return qtrue;
		}
	}

	return qfalse;
}

// counts how many allies this player has
int sje_number_of_allies(const gentity_t* ent)
{
	int number_of_allies = 0;

	for (int i = 0; i < level.maxclients; i++)
	{
		const gentity_t* allied_player = &g_entities[i];

		if (sje_is_ally(ent, allied_player) == qtrue && allied_player->client->sess.sessionTeam != TEAM_SPECTATOR)
			number_of_allies++;
	}

	return number_of_allies;
}

extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);
// Somebody (a player) has touched the shield.  See if it is a "friend".
static void ShieldTouch(gentity_t* self, gentity_t* other, trace_t* trace)
{
	vec3_t tempDir, stunDir;

	if (level.gametype >= GT_TEAM)
	{
		// let teammates through
		// compare the parent's team to the "other's" team
		if (self->parent && self->parent->client && other->client)
		{
			if (OnSameTeam(self->parent, other))
			{
				ShieldGoNotSolid(self);
				return;
			}
			if (self->parent && self->parent->client && sje_is_ally(self->parent, other) == qtrue)
			{
				// allies can pass through the force field
				ShieldGoNotSolid(self);
				return;
			}
		}
	}
	else
	{
		//let the person who dropped the shield through
		if (self->parent && self->parent->s.number == other->s.number)
		{
			ShieldGoNotSolid(self);
			return;
		}
	}

	if (!other->client)
	{
		//can't knock over non-clients
		return;
	}

	//player touched the shield, knock them on their ass.
	if (self->s.time2 & 1 << 24)
	{
		//shield on xaxis
		VectorSet(stunDir, 0, 1, 0);
	}
	else
	{
		VectorSet(stunDir, 1, 0, 0);
	}

	VectorSubtract(self->r.currentOrigin, other->client->ps.origin, tempDir);

	if (DotProduct(tempDir, stunDir) > 0)
	{
		//stun the player away from the shield
		VectorScale(stunDir, -1, stunDir);
	}

	stunDir[2] = 0.5; //bump the kickback up into the air a bit.
	g_throw(other, stunDir, 75);
	G_Knockdown(other, self, stunDir, 300, qtrue);
}

// After a short delay, create the shield by expanding in all directions.
static void CreateShield(gentity_t* ent)
{
	trace_t tr;
	vec3_t mins, maxs, end, posTraceEnd, negTraceEnd, start;
	qboolean xaxis;

	// trace upward to find height of shield
	VectorCopy(ent->r.currentOrigin, end);
	end[2] += MAX_SHIELD_HEIGHT;
	trap->Trace(&tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT, qfalse, 0, 0);
	const int height = (int)(MAX_SHIELD_HEIGHT * tr.fraction);

	// use angles to find the proper axis along which to align the shield
	VectorSet(mins, -SHIELD_HALFTHICKNESS, -SHIELD_HALFTHICKNESS, 0);
	VectorSet(maxs, SHIELD_HALFTHICKNESS, SHIELD_HALFTHICKNESS, height);
	VectorCopy(ent->r.currentOrigin, posTraceEnd);
	VectorCopy(ent->r.currentOrigin, negTraceEnd);

	if ((int)ent->s.angles[YAW] == 0) // shield runs along y-axis
	{
		posTraceEnd[1] += MAX_SHIELD_HALFWIDTH;
		negTraceEnd[1] -= MAX_SHIELD_HALFWIDTH;
		xaxis = qfalse;
	}
	else // shield runs along x-axis
	{
		posTraceEnd[0] += MAX_SHIELD_HALFWIDTH;
		negTraceEnd[0] -= MAX_SHIELD_HALFWIDTH;
		xaxis = qtrue;
	}

	// trace horizontally to find extend of shield
	// positive trace
	VectorCopy(ent->r.currentOrigin, start);
	start[2] += height >> 1;
	trap->Trace(&tr, start, 0, 0, posTraceEnd, ent->s.number, MASK_SHOT, qfalse, 0, 0);
	const int posWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;
	// negative trace
	trap->Trace(&tr, start, 0, 0, negTraceEnd, ent->s.number, MASK_SHOT, qfalse, 0, 0);
	const int negWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;

	// kef -- monkey with dimensions and place origin in center
	const int half_width = (posWidth + negWidth) >> 1;
	if (xaxis)
	{
		ent->r.currentOrigin[0] = ent->r.currentOrigin[0] - negWidth + half_width;
	}
	else
	{
		ent->r.currentOrigin[1] = ent->r.currentOrigin[1] - negWidth + half_width;
	}
	ent->r.currentOrigin[2] += height >> 1;

	// set entity's mins and maxs to new values, make it solid, and link it
	if (xaxis)
	{
		VectorSet(ent->r.mins, -half_width, -SHIELD_HALFTHICKNESS, -(height >> 1));
		VectorSet(ent->r.maxs, half_width, SHIELD_HALFTHICKNESS, height >> 1);
	}
	else
	{
		VectorSet(ent->r.mins, -SHIELD_HALFTHICKNESS, -half_width, -(height >> 1));
		VectorSet(ent->r.maxs, SHIELD_HALFTHICKNESS, half_width, height);
	}
	ent->clipmask = MASK_SHOT;

	// Information for shield rendering.

	//	xaxis - 1 bit
	//	height - 0-254 8 bits
	//	posWidth - 0-255 8 bits
	//  negWidth - 0 - 255 8 bits

	const int paramData = xaxis << 24 | height << 16 | posWidth << 8 | negWidth;
	ent->s.time2 = paramData;

	if (level.gametype == GT_SIEGE || level.gametype == GT_CTF)
	{
		ent->health = ceil((float)(SHIELD_SIEGE_HEALTH * 1));
	}
	else
	{
		ent->health = ceil((float)(SHIELD_HEALTH * 1));
	}

	ent->s.time = ent->health; //???
	ent->pain = ShieldPain;
	ent->die = ShieldDie;
	ent->touch = ShieldTouch;

	// see if we're valid
	trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, CONTENTS_BODY,
		qfalse, 0, 0);

	if (tr.startsolid)
	{
		// Something in the way!
		// make the shield non-solid very briefly
		ent->r.contents = 0;
		ent->s.eFlags |= EF_NODRAW;
		// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
		ent->nextthink = level.time + 200;
		ent->think = ShieldGoSolid;
		ent->takedamage = qfalse;
		trap->LinkEntity((sharedEntity_t*)ent);
	}
	else
	{
		// Get solid.
		ent->r.contents = CONTENTS_PLAYERCLIP | CONTENTS_SHOTCLIP; //CONTENTS_SOLID;

		ent->nextthink = level.time;
		ent->think = ShieldThink;

		ent->takedamage = qtrue;
		trap->LinkEntity((sharedEntity_t*)ent);

		// Play raising sound...
		G_AddEvent(ent, EV_GENERAL_SOUND, shieldActivateSound);
		ent->s.loopSound = shieldLoopSound;
		ent->s.loopIsSoundset = qfalse;
	}

	ShieldGoSolid(ent);
}

static qboolean PlaceShield(gentity_t* playerent)
{
	static const gitem_t* shieldItem = NULL;
	trace_t tr;
	vec3_t fwd, dest;
	const vec3_t maxs = { 4, 4, 4 };
	const vec3_t mins = { -4, -4, 0 };
	static qboolean registered = qfalse;

	if (!registered)
	{
		shieldLoopSound = G_SoundIndex("sound/movers/doors/forcefield_lp.wav");
		shieldAttachSound = G_SoundIndex("sound/weapons/detpack/stick.wav");
		shieldActivateSound = G_SoundIndex("sound/movers/doors/forcefield_on.wav");
		shieldDeactivateSound = G_SoundIndex("sound/movers/doors/forcefield_off.wav");
		shieldDamageSound = G_SoundIndex("sound/effects/bumpfield.wav");
		shieldItem = BG_FindItemForHoldable(HI_SHIELD);
		registered = qtrue;
	}

	// can we place this in front of us?
	AngleVectors(playerent->client->ps.viewangles, fwd, NULL, NULL);
	fwd[2] = 0;
	VectorMA(playerent->client->ps.origin, SHIELD_PLACEDIST, fwd, dest);
	trap->Trace(&tr, playerent->client->ps.origin, mins, maxs, dest, playerent->s.number, MASK_SHOT, qfalse, 0, 0);
	if (tr.fraction > 0.9)
	{
		vec3_t pos;
		//room in front
		VectorCopy(tr.endpos, pos);
		// drop to floor
		VectorSet(dest, pos[0], pos[1], pos[2] - 4096);
		trap->Trace(&tr, pos, mins, maxs, dest, playerent->s.number, MASK_SOLID, qfalse, 0, 0);
		if (!tr.startsolid && !tr.allsolid)
		{
			// got enough room so place the portable shield
			gentity_t* shield = G_Spawn();

			// Figure out what direction the shield is facing.
			if (fabs(fwd[0]) > fabs(fwd[1]))
			{
				// shield is north/south, facing east.
				shield->s.angles[YAW] = 0;
			}
			else
			{
				// shield is along the east/west axis, facing north
				shield->s.angles[YAW] = 90;
			}
			shield->think = CreateShield;
			shield->nextthink = level.time + 500; // power up after .5 seconds
			shield->parent = playerent;

			// Set team number.
			shield->s.otherentity_num2 = playerent->client->sess.sessionTeam;

			shield->s.eType = ET_SPECIAL;
			shield->s.modelIndex = HI_SHIELD; // this'll be used in CG_Useable() for rendering.
			shield->classname = shieldItem->classname;

			shield->r.contents = CONTENTS_TRIGGER;

			shield->touch = 0;
			// using an item causes it to respawn
			shield->use = 0; //Use_Item;

			// allow to ride movers
			shield->s.groundEntityNum = tr.entityNum;

			G_SetOrigin(shield, tr.endpos);

			shield->s.eFlags &= ~EF_NODRAW;
			shield->r.svFlags &= ~SVF_NOCLIENT;
			shield->s.otherentity_num = playerent->s.number; //mark owner info for duel

			trap->LinkEntity((sharedEntity_t*)shield);

			shield->s.owner = playerent->s.number;
			shield->s.shouldtarget = qtrue;
			if (level.gametype >= GT_TEAM)
			{
				shield->s.teamowner = playerent->client->sess.sessionTeam;
			}
			else
			{
				shield->s.teamowner = 16;
			}

			// Play placing sound...
			G_AddEvent(shield, EV_GENERAL_SOUND, shieldAttachSound);
			NPC_SetAnim(playerent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			return qtrue;
		}
	}
	// no room
	return qfalse;
}

void ItemUse_Binoculars(const gentity_t* ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->client->ps.weaponstate != WEAPON_READY)
	{
		//So we can't fool it and reactivate while switching to the saber or something.
		return;
	}

	if (PM_InLedgeMove(ent->client->ps.legsAnim))
	{
		//No binocs while hanging!
		return;
	}

	if (ent->client->ps.zoomMode == 0) // not zoomed or currently zoomed with the disruptor
	{
		ent->client->ps.zoomMode = 2;
		ent->client->ps.zoomLocked = qfalse;
		ent->client->ps.zoomFov = 40.0f;
	}
	else if (ent->client->ps.zoomMode == 2)
	{
		ent->client->ps.zoomMode = 0;
		ent->client->ps.zoomTime = level.time;
	}
}

void ItemUse_Shield(gentity_t* ent)
{
	PlaceShield(ent);
}

//--------------------------
// PERSONAL ASSAULT SENTRY
//--------------------------

#define PAS_DAMAGE	2

static void SentryTouch(gentity_t* ent, gentity_t* other, trace_t* trace)
{
}

extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);

//----------------------------------------------------------------
static void pas_fire(gentity_t* ent)
//----------------------------------------------------------------
{
	vec3_t fwd, myOrg, enOrg;

	VectorCopy(ent->r.currentOrigin, myOrg);
	myOrg[2] += 24;

	VectorCopy(ent->enemy->client->ps.origin, enOrg);
	enOrg[2] += 24;

	VectorSubtract(enOrg, myOrg, fwd);
	VectorNormalize(fwd);

	myOrg[0] += fwd[0] * 16;
	myOrg[1] += fwd[1] * 16;

	if (PM_CrouchAnim(ent->enemy->s.legsAnim))
	{
		myOrg[2] += fwd[2] - 10;
	}
	else if (PM_InKnockDown(ent->enemy->playerState))
	{
	}
	else
	{
		myOrg[2] += fwd[2] * 16;
	}

	WP_FireTurretMissile(&g_entities[ent->genericValue3], myOrg, fwd, qfalse, 10, 2300, MOD_SENTRY, ent);

	G_RunObject(ent);
}

#define TURRET_RADIUS 800

//-----------------------------------------------------
static qboolean pas_find_enemies(gentity_t* self)
//-----------------------------------------------------
{
	qboolean found = qfalse;
	float bestDist = TURRET_RADIUS * TURRET_RADIUS;
	vec3_t org2;
	gentity_t* entity_list[MAX_GENTITIES];
	trace_t tr;

	if (self->aimDebounceTime > level.time) // time since we've been shut off
	{
		// We were active and alert, i.e. had an enemy in the last 3 secs
		if (self->painDebounceTime < level.time)
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/chars/turret/ping.wav"));
			self->painDebounceTime = level.time + 1000;
		}
	}

	VectorCopy(self->s.pos.trBase, org2);

	const int count = G_RadiusList(org2, TURRET_RADIUS, self, qtrue, entity_list);

	for (int i = 0; i < count; i++)
	{
		vec3_t org;
		gentity_t* target = entity_list[i];

		if (!target->client)
		{
			continue;
		}
		if (target == self || !target->takedamage || target->health <= 0 || target->flags & FL_NOTARGET)
		{
			continue;
		}
		if (self->alliedTeam && target->client->sess.sessionTeam == self->alliedTeam)
		{
			continue;
		}
		if (self->genericValue3 == target->s.number)
		{
			continue;
		}
		if (self->s.NPC_class == CLASS_SEEKER)
		{
			continue;
		}
		//don't allow sentry gun to attack our own stuff (specifically our seeker item)
		if (target->r.ownerNum == self->genericValue3)
		{
			continue;
		}
		if (!trap->InPVS(org2, target->r.currentOrigin))
		{
			continue;
		}

		if (target->s.eType == ET_NPC &&
			target->s.NPC_class == CLASS_VEHICLE)
		{
			//don't get mad at vehicles, silly.
			continue;
		}
		if (self->parent && sje_is_ally(self->parent, target) == qtrue)
		{
			continue;
		}

		if (target->client)
		{
			if (target->NPC && target->client->playerTeam == NPCTEAM_PLAYER && target->client->enemyTeam ==
				NPCTEAM_ENEMY)
			{
				// dont attack allied npcs
				continue;
			}
			VectorCopy(target->client->ps.origin, org);
		}
		else
		{
			VectorCopy(target->r.currentOrigin, org);
		}

		trap->Trace(&tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, qfalse, 0, 0);

		if (!tr.allsolid && !tr.startsolid && (tr.fraction == 1.0 || tr.entityNum == target->s.number))
		{
			vec3_t enemyDir;
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract(target->r.currentOrigin, self->r.currentOrigin, enemyDir);
			const float enemyDist = VectorLengthSquared(enemyDir);

			if (enemyDist < bestDist) // all things equal, keep current
			{
				if (self->attackDebounceTime + 100 < level.time)
				{
					// We haven't fired or acquired an enemy in the last 2 seconds-start-up sound
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/chars/turret/startup.wav"));

					// Wind up turrets for a bit
					self->attackDebounceTime = level.time + 900 + Q_flrand(0.0f, 1.0f) * 200;
				}

				G_SetEnemy(self, target);
				bestDist = enemyDist;
				found = qtrue;
			}
		}
	}

	return found;
}

//---------------------------------
static void pas_adjust_enemy(gentity_t* ent)
//---------------------------------
{
	trace_t tr;
	qboolean keep = qtrue;

	if (ent->enemy->health <= 0)
	{
		keep = qfalse;
	}
	else
	{
		vec3_t org, org2;

		VectorCopy(ent->s.pos.trBase, org2);

		if (ent->enemy->client)
		{
			VectorCopy(ent->enemy->client->ps.origin, org);
			org[2] -= 15;
		}
		else
		{
			VectorCopy(ent->enemy->r.currentOrigin, org);
		}

		trap->Trace(&tr, org2, NULL, NULL, org, ent->s.number, MASK_SHOT, qfalse, 0, 0);

		if (tr.allsolid || tr.startsolid || tr.fraction < 0.9f || tr.entityNum == ent->s.number)
		{
			if (tr.entityNum != ent->enemy->s.number)
			{
				// trace failed
				keep = qfalse;
			}
		}
	}

	if (keep)
	{
		//ent->bounceCount = level.time + 500 + Q_flrand(0.0f, 1.0f) * 150;
	}
	else if (ent->bounceCount < level.time && ent->enemy) // don't ping pong on and off
	{
		ent->enemy = NULL;
		// shut-down sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/turret/shutdown.wav"));

		ent->bounceCount = level.time + 500 + Q_flrand(0.0f, 1.0f) * 150;

		// make turret play ping sound for 5 seconds
		ent->aimDebounceTime = level.time + 5000;
	}
}

#define TURRET_DEATH_DELAY 2000
#define TURRET_LIFETIME 100000

void turret_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod);

void sentryExpire(gentity_t* self)
{
	turret_die(self, self, self, 1000, MOD_UNKNOWN);
}

//---------------------------------
void pas_think(gentity_t* ent)
//---------------------------------
{
	vec3_t frontAngles, backAngles;
	int iEntityList[MAX_GENTITIES];
	int i = 0;
	qboolean clTrapped = qfalse;
	vec3_t testMins, testMaxs;

	testMins[0] = ent->r.currentOrigin[0] + ent->r.mins[0] + 4;
	testMins[1] = ent->r.currentOrigin[1] + ent->r.mins[1] + 4;
	testMins[2] = ent->r.currentOrigin[2] + ent->r.mins[2] + 4;

	testMaxs[0] = ent->r.currentOrigin[0] + ent->r.maxs[0] - 4;
	testMaxs[1] = ent->r.currentOrigin[1] + ent->r.maxs[1] - 4;
	testMaxs[2] = ent->r.currentOrigin[2] + ent->r.maxs[2] - 4;

	int num_listed_entities = trap->EntitiesInBox(testMins, testMaxs, iEntityList, MAX_GENTITIES);

	while (i < num_listed_entities)
	{
		if (iEntityList[i] < MAX_CLIENTS)
		{
			//client stuck inside me. go nonsolid.
			const int clNum = iEntityList[i];

			num_listed_entities = trap->EntitiesInBox(g_entities[clNum].r.absmin, g_entities[clNum].r.absmax,
				iEntityList,
				MAX_GENTITIES);

			i = 0;
			while (i < num_listed_entities)
			{
				if (iEntityList[i] == ent->s.number)
				{
					clTrapped = qtrue;
					break;
				}
				i++;
			}
			break;
		}

		i++;
	}

	if (clTrapped)
	{
		ent->r.contents = 0;
		ent->s.fireflag = 0;
		ent->nextthink = level.time + FRAMETIME;
		return;
	}
	ent->r.contents = CONTENTS_SOLID;

	if (!g_entities[ent->genericValue3].inuse || !g_entities[ent->genericValue3].client ||
		g_entities[ent->genericValue3].client->sess.sessionTeam != ent->genericValue2)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	//	G_RunObject(ent);

	if (!ent->damage)
	{
		ent->damage = 1;
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	if (ent->genericValue8 + TURRET_LIFETIME < level.time)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/turret/shutdown.wav"));
		ent->s.bolt2 = ENTITYNUM_NONE;
		ent->s.fireflag = 2;

		ent->think = sentryExpire;
		ent->nextthink = level.time + TURRET_DEATH_DELAY;
		return;
	}

	ent->nextthink = level.time + FRAMETIME;

	if (ent->enemy)
	{
		// make sure that the enemy is still valid
		pas_adjust_enemy(ent);
	}

	if (ent->enemy)
	{
		if (!ent->enemy->client)
		{
			ent->enemy = NULL;
		}
		else if (ent->enemy->s.number == ent->s.number)
		{
			ent->enemy = NULL;
		}
		else if (ent->enemy->health < 1)
		{
			ent->enemy = NULL;
		}
	}

	if (!ent->enemy)
	{
		pas_find_enemies(ent);
	}

	if (ent->enemy)
	{
		ent->s.bolt2 = ent->enemy->s.number;
	}
	else
	{
		ent->s.bolt2 = ENTITYNUM_NONE;
	}

	qboolean moved = qfalse;
	float diff_yaw;
	float diffPitch = 0.0f;

	ent->speed = AngleNormalize360(ent->speed);
	ent->random = AngleNormalize360(ent->random);

	if (ent->enemy)
	{
		vec3_t desiredAngles;
		vec3_t org;
		vec3_t enemyDir;
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		if (ent->enemy->client)
		{
			VectorCopy(ent->enemy->client->ps.origin, org);
		}
		else
		{
			VectorCopy(ent->enemy->r.currentOrigin, org);
		}

		VectorSubtract(org, ent->r.currentOrigin, enemyDir);
		vectoangles(enemyDir, desiredAngles);

		diff_yaw = AngleSubtract(ent->speed, desiredAngles[YAW]);
		diffPitch = AngleSubtract(ent->random, desiredAngles[PITCH]);
	}
	else
	{
		// no enemy, so make us slowly sweep back and forth as if searching for a new one
		diff_yaw = sin(level.time * 0.0001f + ent->count) * 2.0f;
	}

	if (fabs(diff_yaw) > 0.25f)
	{
		moved = qtrue;

		if (fabs(diff_yaw) > 10.0f)
		{
			// cap max speed
			ent->speed += diff_yaw > 0.0f ? -10.0f : 10.0f;
		}
		else
		{
			// small enough
			ent->speed -= diff_yaw;
		}
	}

	if (fabs(diffPitch) > 0.25f)
	{
		moved = qtrue;

		if (fabs(diffPitch) > 4.0f)
		{
			// cap max speed
			ent->random += diffPitch > 0.0f ? -4.0f : 4.0f;
		}
		else
		{
			// small enough
			ent->random -= diffPitch;
		}
	}

	// the bone axes are messed up, so hence some dumbness here
	VectorSet(frontAngles, -ent->random, 0.0f, 0.0f);
	VectorSet(backAngles, 0.0f, 0.0f, ent->speed);

	if (moved)
	{
		//ent->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );
	}
	else
	{
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
	}

	if (ent->enemy && ent->attackDebounceTime < level.time)
	{
		ent->count--;

		if (ent->count)
		{
			pas_fire(ent);
			ent->s.fireflag = 1;
			ent->attackDebounceTime = level.time + 200;
		}
		else
		{
			//ent->nextthink = 0;
			G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/turret/shutdown.wav"));
			ent->s.bolt2 = ENTITYNUM_NONE;
			ent->s.fireflag = 2;

			ent->think = sentryExpire;
			ent->nextthink = level.time + TURRET_DEATH_DELAY;
		}
	}
	else
	{
		ent->s.fireflag = 0;
	}
}

//------------------------------------------------------------------------------------------------------------
void turret_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
//------------------------------------------------------------------------------------------------------------
{
	gentity_t* owner = &g_entities[self->genericValue3];
	self->think = 0; //NULL;
	self->use = 0; //NULL;

	if (self->target)
	{
		G_UseTargets(self, attacker);
	}

	if (!g_entities[self->genericValue3].inuse || !g_entities[self->genericValue3].client)
	{
		G_FreeEntity(self);
		return;
	}

	// clear my data
	self->die = 0; //NULL;
	self->takedamage = qfalse;
	self->health = 0;

	// hack the effect angle so that explode death can orient the effect properly
	VectorSet(self->s.angles, 0, 0, 1);

	G_PlayEffect(EFFECT_EXPLOSION_PAS, self->s.pos.trBase, self->s.angles);
	g_radius_damage(self->s.pos.trBase, &g_entities[self->genericValue3], 30, 256, self, self, MOD_UNKNOWN);
	G_FreeEntity(self);
	if (owner->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_0)
	{
		owner->sentryDeadThink = level.time + 30000;
	}
}

static void turret_free(gentity_t* self)
{
	if (!g_entities[self->genericValue3].inuse || !g_entities[self->genericValue3].client)
	{
		G_FreeEntity(self);
		return;
	}

	g_entities[self->genericValue3].client->ps.fd.sentryDeployed = qfalse;

	G_FreeEntity(self);
}

#define TURRET_AMMO_COUNT 40

//---------------------------------
void SP_PAS(gentity_t* base)
//---------------------------------
{
	if (base->count == 0)
	{
		// give ammo
		base->count = TURRET_AMMO_COUNT;
	}

	base->s.bolt1 = 1; //This is a sort of hack to indicate that this model needs special turret things done to it
	base->s.bolt2 = ENTITYNUM_NONE; //store our current enemy index

	base->damage = 0; // start animation flag

	VectorSet(base->r.mins, -8, -8, 0);
	VectorSet(base->r.maxs, 8, 8, 24);

	G_RunObject(base);

	base->think = pas_think;
	base->nextthink = level.time + FRAMETIME;

	if (!base->health)
	{
		base->health = 50;
	}

	base->takedamage = qtrue;
	base->die = turret_die;

	base->physicsObject = qtrue;

	G_Sound(base, CHAN_BODY, G_SoundIndex("sound/chars/turret/startup.wav"));
}

//------------------------------------------------------------------------
void ItemUse_Sentry(gentity_t* ent)
//------------------------------------------------------------------------
{
	vec3_t fwd, fwdorg;
	vec3_t yawonly;
	vec3_t mins, maxs;

	if (!ent || !ent->client)
	{
		return;
	}

	VectorSet(mins, -8, -8, 0);
	VectorSet(maxs, 8, 8, 24);

	yawonly[ROLL] = 0;
	yawonly[PITCH] = 0;
	yawonly[YAW] = ent->client->ps.viewangles[YAW];

	AngleVectors(yawonly, fwd, NULL, NULL);

	fwdorg[0] = ent->client->ps.origin[0] + fwd[0] * 64;
	fwdorg[1] = ent->client->ps.origin[1] + fwd[1] * 64;
	fwdorg[2] = ent->client->ps.origin[2] + fwd[2] * 64;

	gentity_t* sentry = G_Spawn();

	sentry->classname = "sentryGun";
	sentry->s.modelIndex = G_model_index("models/items/psgun.glm"); //replace ASAP

	sentry->s.g2radius = 30.0f;
	sentry->s.modelGhoul2 = 1;

	G_SetOrigin(sentry, fwdorg);
	sentry->parent = ent;
	sentry->r.contents = CONTENTS_SOLID;
	sentry->s.solid = 2;
	sentry->clipmask = MASK_SOLID;
	VectorCopy(mins, sentry->r.mins);
	VectorCopy(maxs, sentry->r.maxs);
	sentry->genericValue3 = ent->s.number;
	sentry->genericValue2 = ent->client->sess.sessionTeam; //so we can remove ourself if our owner changes teams
	sentry->genericValue15 = HI_SENTRY_GUN;
	sentry->r.absmin[0] = sentry->s.pos.trBase[0] + sentry->r.mins[0];
	sentry->r.absmin[1] = sentry->s.pos.trBase[1] + sentry->r.mins[1];
	sentry->r.absmin[2] = sentry->s.pos.trBase[2] + sentry->r.mins[2];
	sentry->r.absmax[0] = sentry->s.pos.trBase[0] + sentry->r.maxs[0];
	sentry->r.absmax[1] = sentry->s.pos.trBase[1] + sentry->r.maxs[1];
	sentry->r.absmax[2] = sentry->s.pos.trBase[2] + sentry->r.maxs[2];
	sentry->s.eType = ET_GENERAL;
	sentry->s.pos.trType = TR_GRAVITY; //STATIONARY;
	sentry->s.pos.trTime = level.time;
	sentry->touch = SentryTouch;
	sentry->nextthink = level.time;
	sentry->genericValue4 = ENTITYNUM_NONE; //genericValue4 used as enemy index

	sentry->genericValue5 = 1000;

	sentry->genericValue8 = level.time;

	sentry->alliedTeam = ent->client->sess.sessionTeam;

	ent->client->ps.fd.sentryDeployed = qtrue;

	trap->LinkEntity((sharedEntity_t*)sentry);

	sentry->s.owner = ent->s.number;
	sentry->s.otherentity_num = ent->s.number; // mark owner info for duel
	sentry->s.weapon = WP_TURRET; //so I can identify the entity as sentry gun client side
	sentry->s.shouldtarget = qtrue;
	if (level.gametype >= GT_TEAM)
	{
		sentry->s.teamowner = ent->client->sess.sessionTeam;
	}
	else
	{
		sentry->s.teamowner = 16;
	}
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	SP_PAS(sentry);
}

//------------------------------------------------------------------------
void ItemUse_Sentry2(gentity_t* ent)
//------------------------------------------------------------------------
{
	vec3_t fwd, fwdorg;
	vec3_t yawonly;
	vec3_t mins, maxs;

	if (!ent || !ent->client)
	{
		return;
	}

	VectorSet(mins, -8, -8, 0);
	VectorSet(maxs, 8, 8, 24);

	yawonly[ROLL] = 0;
	yawonly[PITCH] = 0;
	yawonly[YAW] = ent->client->ps.viewangles[YAW];

	AngleVectors(yawonly, fwd, NULL, NULL);

	fwdorg[0] = ent->client->ps.origin[0] + fwd[0] * 64;
	fwdorg[1] = ent->client->ps.origin[1] + fwd[1] * 64;
	fwdorg[2] = ent->client->ps.origin[2] + fwd[2] * 64;

	gentity_t* sentry = G_Spawn();

	sentry->classname = "sentryGun";
	sentry->s.modelIndex = G_model_index("models/items/psgun.glm"); //replace ASAP

	sentry->s.g2radius = 30.0f;
	sentry->s.modelGhoul2 = 1;

	G_SetOrigin(sentry, fwdorg);
	sentry->parent = ent;
	sentry->r.contents = CONTENTS_SOLID;
	sentry->s.solid = 2;
	sentry->clipmask = MASK_SOLID;
	VectorCopy(mins, sentry->r.mins);
	VectorCopy(maxs, sentry->r.maxs);
	sentry->genericValue3 = ent->s.number;
	sentry->genericValue2 = ent->client->sess.sessionTeam; //so we can remove ourself if our owner changes teams
	sentry->genericValue15 = HI_SENTRY_GUN;
	sentry->r.absmin[0] = sentry->s.pos.trBase[0] + sentry->r.mins[0];
	sentry->r.absmin[1] = sentry->s.pos.trBase[1] + sentry->r.mins[1];
	sentry->r.absmin[2] = sentry->s.pos.trBase[2] + sentry->r.mins[2];
	sentry->r.absmax[0] = sentry->s.pos.trBase[0] + sentry->r.maxs[0];
	sentry->r.absmax[1] = sentry->s.pos.trBase[1] + sentry->r.maxs[1];
	sentry->r.absmax[2] = sentry->s.pos.trBase[2] + sentry->r.maxs[2];
	sentry->s.eType = ET_GENERAL;
	sentry->s.pos.trType = TR_GRAVITY; //STATIONARY;
	sentry->s.pos.trTime = level.time;
	sentry->touch = SentryTouch;
	sentry->nextthink = level.time;
	sentry->genericValue4 = ENTITYNUM_NONE; //genericValue4 used as enemy index

	sentry->genericValue5 = 1000;

	sentry->genericValue8 = level.time;

	sentry->alliedTeam = ent->client->sess.sessionTeam;

	trap->LinkEntity((sharedEntity_t*)sentry);

	sentry->parent->sentryDeadThink = 0;

	sentry->s.owner = ent->s.number;
	sentry->s.shouldtarget = qtrue;
	if (level.gametype >= GT_TEAM)
	{
		sentry->s.teamowner = ent->client->sess.sessionTeam;
	}
	else
	{
		sentry->s.teamowner = 16;
	}

	SP_PAS(sentry);
}

extern gentity_t* NPC_SpawnType(const gentity_t* ent, const char* npc_type, const char* targetname, qboolean isVehicle);
extern void NPC_SetMoveGoal(const gentity_t* ent, vec3_t point, int radius, qboolean isNavGoal, int combatPoint, gentity_t* targetEnt);
gentity_t* ViewTarget(gentity_t* ent, int length, vec3_t* target, cplane_t* plane);

void ItemUse_Seeker(gentity_t* ent)
{
	if (level.gametype == GT_SIEGE && d_siegeSeekerNPC.integer)
	{
		//actualy spawn a remote NPC
		gentity_t* remote = NPC_SpawnType(ent, "remote", NULL, qfalse);

		if (remote && remote->client)
		{
			//set it to my team
			remote->s.owner = remote->r.ownerNum = ent->s.number;
			remote->activator = ent;
			if (ent->client->sess.sessionTeam == TEAM_BLUE)
			{
				remote->client->playerTeam = NPCTEAM_PLAYER;
			}
			else if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				remote->client->playerTeam = NPCTEAM_ENEMY;
			}
			else
			{
				remote->client->playerTeam = NPCTEAM_NEUTRAL;
			}
		}
	}
	else
	{
		ent->client->ps.eFlags |= EF_SEEKERDRONE;
		ent->client->ps.droneExistTime = level.time + 30000;
		ent->client->ps.droneFireTime = level.time + 1500;
		NPC_SetAnim(ent, SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
}

void ItemUse_ATST(gentity_t* ent)
{
	gentity_t* decca = NPC_SpawnType(ent, "ATST_vehicle", NULL, qtrue);

	if (decca && decca->client)
	{
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/atst/atst_hatch_open.mp3"));
	}
}

void ItemUse_Decca(gentity_t* ent)
{
	gentity_t* decca = NPC_SpawnType(ent, "droideka_veh", NULL, qtrue);

	if (decca && decca->client)
	{
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/chars/droideka/foldout.mp3"));
	}
}

void ItemUse_Swoop(gentity_t* ent)
{
	gentity_t* swoop = NPC_SpawnType(ent, "swoop_mp", NULL, qtrue);

	if (swoop && swoop->client)
	{
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_INV_USE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_Sound(ent, CHAN_BODY, G_SoundIndex("sound/vehicles/swoop/on.mp3"));
	}
}

static void MedPackGive(gentity_t* ent, const int amount)
{
	if (!ent || !ent->client)
	{
		return;
	}
	if (ent->client->ps.duelInProgress)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->ps.eFlags & EF_DEAD)
	{
		return;
	}

	if (ent->health >= ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		return;
	}

	ent->health += amount;
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCEHEAL_QUICK, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
	}
}

void ItemUse_MedPack_Big(gentity_t* ent)
{
	MedPackGive(ent, MAX_MEDPACK_BIG_HEAL_AMOUNT);
}

void ItemUse_MedPack(gentity_t* ent)
{
	MedPackGive(ent, MAX_MEDPACK_HEAL_AMOUNT);
}

#define JETPACK_TOGGLE_TIME			1000

void Jetpack_Off(const gentity_t* ent)
{
	//create effects?
	assert(ent && ent->client);

	if (!ent->client->jetPackOn)
	{
		//aready off
		return;
	}
	if (!PM_InKnockDown(&ent->client->ps))
	{
		if (ent->client->ps.eFlags & EF3_DUAL_WEAPONS)
		{
			ent->client->ps.torsoAnim = WeaponReadyAnim2[ent->client->ps.weapon];
			ent->client->ps.legsAnim = WeaponReadyAnim2[ent->client->ps.weapon];
		}
		else
		{
			ent->client->ps.torsoAnim = WeaponReadyAnim[ent->client->ps.weapon];
			ent->client->ps.legsAnim = WeaponReadyAnim[ent->client->ps.weapon];
		}
	}

	ent->client->jetPackOn = qfalse;
}

void Jetpack_On(gentity_t* ent)
{
	//create effects?
	assert(ent && ent->client);

	if (ent->client->jetPackOn)
	{
		//aready on
		return;
	}

	if (ent->client->ps.fd.forceGripBeingGripped >= level.time)
	{
		//can't turn on during grip interval
		return;
	}

	if (ent->client->ps.fallingToDeath)
	{
		//too late!
		return;
	}

	G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/jetpack/ignite.wav"));

	ent->client->jetPackOn = qtrue;
}

#define FLAMETHROWER_RADIUS 200
#define FLAMETHROWER_DAMAGE	2

void Flamethrower_Fire(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	vec3_t center, mins, maxs, v;

	const float radius = FLAMETHROWER_RADIUS;
	gentity_t* entity_list[MAX_GENTITIES];
	int iEntityList[MAX_GENTITIES];
	int i;

	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	VectorCopy(self->client->ps.origin, center);
	for (i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}
	const int num_listed_entities = trap->EntitiesInBox(mins, maxs, iEntityList, MAX_GENTITIES);

	i = 0;
	while (i < num_listed_entities)
	{
		entity_list[i] = &g_entities[iEntityList[i]];

		i++;
	}

	for (int e = 0; e < num_listed_entities; e++)
	{
		vec3_t size;
		vec3_t ent_org;
		vec3_t dir;
		float dot;
		gentity_t* traceEnt = entity_list[e];

		if (!traceEnt)
			continue;
		if (traceEnt == self)
			continue;
		if (!traceEnt->inuse)
			continue;
		if (!traceEnt->takedamage)
			continue;
		if (!g_friendlyFire.integer && OnSameTeam(self, traceEnt))
			continue;
		for (i = 0; i < 3; i++)
		{
			if (center[i] < traceEnt->r.absmin[i])
			{
				v[i] = traceEnt->r.absmin[i] - center[i];
			}
			else if (center[i] > traceEnt->r.absmax[i])
			{
				v[i] = center[i] - traceEnt->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}

		VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
		VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

		//see if they're in front of me
		//must be within the forward cone
		VectorSubtract(ent_org, center, dir);
		VectorNormalize(dir);
		if ((dot = DotProduct(dir, forward)) < 0.5)
			continue;

		//must be close enough
		const float dist = VectorLength(v);
		if (dist >= radius)
		{
			continue;
		}

		//in PVS?
		if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
		{
			//must be in PVS
			continue;
		}

		//Now check and see if we can actually hit it
		trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);

		if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
		{
			//must have clear LOS
			continue;
		}

		if (tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage)
		{
			const int damage = FLAMETHROWER_DAMAGE;

			G_Damage(traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_IGNORE_TEAM, MOD_BURNING);

			if (traceEnt->health > 0)
			{
				g_throw(traceEnt, dir, 30);

				if (traceEnt->client)
				{
					G_PlayBoltedEffect(G_EffectIndex("flamethrower/flame_impact"), traceEnt, "thoracic");

					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_FACEPROTECT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				player_Burn(traceEnt);
			}
		}
		else
		{
			Player_CheckBurn(traceEnt);
		}
	}
}

extern qboolean G_ControlledByPlayer(gentity_t* self);
void ItemUse_FlameThrower(const gentity_t* ent)
{
	if (ent->client->ps.jetpackFuel < 15)
	{//can't use flamethrower
		return;
	}

	if (PM_InLedgeMove(ent->client->ps.legsAnim))
	{
		//can't use flamethrower while in ledgegrab
		return;
	}

	if (ent->client->ps.weapon == WP_SABER)
	{
		//can't use flamethrower with saber
		return;
	}

	ent->client->flameTime = level.time + 300;
}

void ItemUse_Jetpack(gentity_t* ent)
{
	assert(ent && ent->client);

	if (ent->client->jetPackToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->ps.eFlags & EF_DEAD ||
		ent->client->ps.pm_type == PM_DEAD)
	{
		//can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->jetPackOn &&
		ent->client->ps.jetpackFuel < 5)
	{
		//too low on fuel to start it up
		return;
	}

	if (ent->client->jetPackOn)
	{
		Jetpack_Off(ent);
	}
	else
	{
		Jetpack_On(ent);
	}

	ent->client->jetPackToggleTime = level.time + JETPACK_TOGGLE_TIME;
}

#define CLOAK_TOGGLE_TIME			1000
extern void Jedi_Cloak(gentity_t* self);
extern void player_Cloak(gentity_t* self);
extern void Jedi_Decloak(gentity_t* self);

void ItemUse_UseCloak(gentity_t* ent)
{
	assert(ent && ent->client);

	if (ent->client->cloakToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		ent->client->ps.eFlags & EF_DEAD ||
		ent->client->ps.pm_type == PM_DEAD)
	{
		//can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.powerups[PW_CLOAKED] &&
		ent->client->ps.cloakFuel < 5)
	{
		//too low on fuel to start it up
		return;
	}

	if (ent->client->ps.powerups[PW_CLOAKED])
	{
		//decloak
		Jedi_Decloak(ent);
	}
	else
	{
		//cloak
		if (ent->client->ps.cloakFuel > 15)
		{
			//cloak
			if (ent->r.svFlags & SVF_BOT)
			{
				if (ent->health > 0
					&& ent->painDebounceTime < level.time
					&& !ent->client->ps.saberInFlight
					&& !PM_SaberInAttack(ent->client->ps.saber_move)
					&& ent->client->ps.fd.forceGripBeingGripped < level.time
					&& !(ent->client->ps.communicatingflags & 1 << CLOAK_CHARGE_RESTRICTION))
				{
					Jedi_Cloak(ent);
				}
			}
			else
			{
				player_Cloak(ent);
			}
		}
	}

	ent->client->cloakToggleTime = level.time + CLOAK_TOGGLE_TIME;
}

#define SPHERESHIELD_TOGGLE_TIME			1000
#define OVERLOAD_TOGGLE_TIME			1000

extern void Sphereshield_On(gentity_t* self);
extern void Sphereshield_Off(gentity_t* self);
void ItemUse_UseSphereshield(gentity_t* ent)
{
	assert(ent && ent->client);

	if (ent->client->sphereshieldToggleTime >= level.time)
	{
		return;
	}

	if (ent->health <= 0 ||
		ent->client->ps.stats[STAT_HEALTH] <= 0 ||
		(ent->client->ps.eFlags & EF_DEAD) ||
		ent->client->ps.pm_type == PM_DEAD)
	{ //can't use it when dead under any circumstances.
		return;
	}

	if (!ent->client->ps.powerups[PW_SPHERESHIELDED] &&
		ent->client->ps.cloakFuel < 5)
	{ //too low on fuel to start it up
		return;
	}

	if (ent->client->ps.powerups[PW_SPHERESHIELDED])
	{//decloak
		Sphereshield_Off(ent);
	}
	else
	{//cloak
		Sphereshield_On(ent);
	}

	ent->client->sphereshieldToggleTime = level.time + SPHERESHIELD_TOGGLE_TIME;
}

#define TOSSED_ITEM_STAY_PERIOD			20000
#define TOSSED_ITEM_OWNER_NOTOUCH_DUR	1000

static void SpecialItemThink(gentity_t* ent)
{
	const float gravity = 3.0f;
	const float mass = 0.09f;
	const float bounce = 1.1f;

	if (ent->genericValue5 < level.time)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	G_RunExPhys(ent, gravity, mass, bounce, qfalse, NULL, 0);
	VectorCopy(ent->r.currentOrigin, ent->s.origin);
	ent->nextthink = level.time + 50;
}

static void G_SpecialSpawnItem(gentity_t* ent, gitem_t* item)
{
	register_item(item);
	ent->item = item;

	//go away if no one wants me
	ent->genericValue5 = level.time + TOSSED_ITEM_STAY_PERIOD;
	ent->think = SpecialItemThink;
	ent->nextthink = level.time + 50;
	ent->clipmask = MASK_SOLID;

	ent->physicsBounce = 0.50f; // items are bouncy
	VectorSet(ent->r.mins, -8, -8, -0);
	VectorSet(ent->r.maxs, 8, 8, 16);

	ent->s.eType = ET_ITEM;
	ent->s.modelIndex = ent->item - bg_itemlist; // store item number in modelIndex

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;

	//can't touch owner for x seconds
	ent->genericValue11 = ent->r.ownerNum;
	ent->genericValue10 = level.time + TOSSED_ITEM_OWNER_NOTOUCH_DUR;

	//so we know to remove when picked up, not respawn
	ent->genericValue9 = 1;

	//kind of a lame value to use, but oh well. This means don't
	//pick up this item clientside with prediction, because we
	//aren't sending over all the data necessary for the player
	//to know if he can.
	ent->s.brokenLimbs = 1;

	//since it uses my server-only physics
	ent->s.eFlags |= EF_CLIENTSMOOTH;
}

#define DISP_HEALTH_ITEM		"item_medpak_instant"
#define DISP_AMMO_ITEM			"ammo_all"

void G_PrecacheDispensers(void)
{
	const gitem_t* item = BG_FindItem(DISP_HEALTH_ITEM);
	if (item)
	{
		register_item(item);
	}

	item = BG_FindItem(DISP_AMMO_ITEM);
	if (item)
	{
		register_item(item);
	}
}

void G_LoadDispensers(void)
{
	const gitem_t* item = BG_FindItem(DISP_HEALTH_ITEM);
	if (item)
	{
		register_item(item);
	}

	item = BG_FindItem(DISP_AMMO_ITEM);
	if (item)
	{
		register_item(item);
	}
}

void ItemUse_UseDisp(gentity_t* ent, int type)
{
	gitem_t* item;

	if (!ent->client ||
		ent->client->tossableItemDebounce > level.time)
	{
		//can't use it again yet
		return;
	}

	if (ent->client->ps.weaponTime > 0 ||
		ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//busy doing something else
		return;
	}

	ent->client->tossableItemDebounce = level.time + TOSS_DEBOUNCE_TIME;

	if (type == HI_HEALTHDISP)
	{
		item = BG_FindItem(DISP_HEALTH_ITEM);
	}
	else
	{
		item = BG_FindItem(DISP_AMMO_ITEM);
	}

	if (item)
	{
		vec3_t fwd, pos;

		gentity_t* eItem = G_Spawn();
		eItem->r.ownerNum = ent->s.number;
		eItem->classname = item->classname;

		VectorCopy(ent->client->ps.origin, pos);
		pos[2] += ent->client->ps.viewheight;

		G_SetOrigin(eItem, pos);
		VectorCopy(eItem->r.currentOrigin, eItem->s.origin);
		trap->LinkEntity((sharedEntity_t*)eItem);

		G_SpecialSpawnItem(eItem, item);

		AngleVectors(ent->client->ps.viewangles, fwd, NULL, NULL);
		VectorScale(fwd, 128.0f, eItem->epVelocity);
		eItem->epVelocity[2] = 16.0f;

		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_BUTTON2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

		gentity_t* te = G_TempEntity(ent->client->ps.origin, EV_LOCALTIMER);
		te->s.time = level.time;
		te->s.time2 = TOSS_DEBOUNCE_TIME;
		te->s.owner = ent->client->ps.clientNum;
	}
}

//===============================================
//Portable E-Web -rww
//===============================================
//put the e-web away/remove it from the owner
static void EWebDisattach(const gentity_t* owner, gentity_t* eweb)
{
	owner->client->ewebIndex = 0;
	owner->client->ps.emplacedIndex = 0;
	if (owner->health > 0)
	{
		owner->client->ps.stats[STAT_WEAPONS] = eweb->genericValue11;
	}
	else
	{
		owner->client->ps.stats[STAT_WEAPONS] = 0;
	}
	eweb->think = G_FreeEntity;
	eweb->nextthink = level.time;
}

//precache misc e-web assets
void EWebPrecache(void)
{
	register_item(BG_FindItemForWeapon(WP_TURRET));
	G_EffectIndex("detpack/explosion.efx");
	G_EffectIndex("turret/muzzle_flash.efx");
}

//e-web death
#define EWEB_DEATH_RADIUS		128
#define EWEB_DEATH_DMG			90

extern void BG_CycleInven(playerState_t* ps, int direction); //bg_misc.c

static void EWebDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int mod)
{
	vec3_t fx_dir;

	g_radius_damage(self->r.currentOrigin, self, EWEB_DEATH_DMG, EWEB_DEATH_RADIUS, self, self, MOD_SUICIDE);

	VectorSet(fx_dir, 1.0f, 0.0f, 0.0f);
	G_PlayEffect(EFFECT_EXPLOSION_DETPACK, self->r.currentOrigin, fx_dir);

	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		const gentity_t* owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			EWebDisattach(owner, self);

			//make sure it resets next time we spawn one in case we someone obtain one before death
			owner->client->ewebHealth = -1;

			//take it away from him, it is gone forever.
			owner->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_EWEB);

			if (owner->client->ps.stats[STAT_HOLDABLE_ITEM] > 0 &&
				bg_itemlist[owner->client->ps.stats[STAT_HOLDABLE_ITEM]].giType == IT_HOLDABLE &&
				bg_itemlist[owner->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_EWEB)
			{
				//he has it selected so deselect it and select the first thing available
				owner->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				BG_CycleInven(&owner->client->ps, 1);
			}
		}
	}
}

//e-web pain
static void EWebPain(const gentity_t* self, gentity_t* attacker, int damage)
{
	//update the owner's health status of me
	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		const gentity_t* owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			owner->client->ewebHealth = self->health;
		}
	}
}

//special routine for tracking angles between client and server
static void EWeb_SetBoneAngles(gentity_t* ent, const char* bone, vec3_t angles)
{
	int* thebone = &ent->s.boneIndex1;
	int* firstFree = NULL;
	int i = 0;
	const int boneIndex = G_BoneIndex(bone);
	vec3_t* boneVector = &ent->s.boneAngles1;
	vec3_t* freeBoneVec = NULL;

	while (thebone)
	{
		if (!*thebone && !firstFree)
		{
			//if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			firstFree = thebone;
			freeBoneVec = boneVector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{
				//this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			boneVector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			boneVector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			boneVector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			boneVector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{
		//didn't find it, create it
		if (!firstFree)
		{
			//no free bones.. can't do a thing then.
			Com_Printf("WARNING: E-Web has no free bone indexes\n");
			return;
		}

		thebone = firstFree;

		*thebone = boneIndex;
		boneVector = freeBoneVec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *boneVector);

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	const int flags = BONE_ANGLES_POSTMULT;
	const int up = POSITIVE_Y;
	const int right = NEGATIVE_Z;
	const int forward = NEGATIVE_X;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = forward | right << 3 | up << 6;

	trap->G2API_SetBoneAngles(ent->ghoul2,
		0,
		bone,
		angles,
		flags,
		up,
		right,
		forward,
		NULL,
		100,
		level.time);
}

//start an animation on model_root both server side and client side
static void EWeb_SetBoneAnim(gentity_t* eweb, const int startFrame, const int endFrame)
{
	//set info on the entity so it knows to start the anim on the client next snapshot.
	eweb->s.eFlags |= EF_G2ANIMATING;

	if (eweb->s.torsoAnim == startFrame && eweb->s.legsAnim == endFrame)
	{
		//already playing this anim, let's flag it to restart
		eweb->s.torsoFlip = !eweb->s.torsoFlip;
	}
	else
	{
		eweb->s.torsoAnim = startFrame;
		eweb->s.legsAnim = endFrame;
	}

	//now set the animation on the server ghoul2 instance.
	assert(eweb->ghoul2);
	trap->G2API_SetBoneAnim(eweb->ghoul2, 0, "model_root", startFrame, endFrame,
		BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, 1.0f, level.time, -1, 100);
}

//fire a shot off
#define EWEB_MISSILE_DAMAGE			20

static void EWebFire(gentity_t* owner, gentity_t* eweb)
{
	mdxaBone_t boltMatrix;
	vec3_t p, d, bPoint;

	if (eweb->genericValue10 == -1)
	{
		//oh no
		assert(!"Bad e-web bolt");
		return;
	}

	//get the muzzle point
	trap->G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue10, &boltMatrix, eweb->s.apos.trBase,
		eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, d);

	//Start the thing backwards into the bounding box so it can't start inside other solid things
	VectorMA(p, -16.0f, d, bPoint);

	//create the missile
	gentity_t* missile = CreateMissile(bPoint, d, 1200.0f, 10000, owner, qfalse);

	missile->classname = "generic_proj";
	missile->s.weapon = WP_TURRET;

	missile->damage = EWEB_MISSILE_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_TURBLAST;
	missile->clipmask = MASK_SHOT;

	//ignore the e-web entity
	missile->passThroughNum = eweb->s.number + 1;

	//times it can bounce before it dies
	missile->bounceCount = 8;

	//play the muzzle flash
	vectoangles(d, d);
	G_PlayEffectID(G_EffectIndex("turret/muzzle_flash.efx"), p, d);
}

//lock the owner into place relative to the cannon pos
static void EWebPositionUser(gentity_t* owner, gentity_t* eweb)
{
	mdxaBone_t boltMatrix;
	vec3_t p, d;
	trace_t tr;

	trap->G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue9, &boltMatrix, eweb->s.apos.trBase,
		eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d);

	VectorMA(p, 32.0f, d, p);
	p[2] = eweb->r.currentOrigin[2];

	p[2] += 4.0f;

	trap->Trace(&tr, owner->client->ps.origin, owner->r.mins, owner->r.maxs, p, owner->s.number, MASK_PLAYERSOLID,
		qfalse, 0, 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction == 1.0f)
	{
		//all clear, we can move there
		vec3_t pDown;

		VectorCopy(p, pDown);
		pDown[2] -= 7.0f;
		trap->Trace(&tr, p, owner->r.mins, owner->r.maxs, pDown, owner->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (!tr.startsolid && !tr.allsolid)
		{
			VectorSubtract(owner->client->ps.origin, tr.endpos, d);
			if (VectorLength(d) > 1.0f)
			{
				//we moved, do some animating
				vec3_t dAng;
				int aFlags = SETANIM_FLAG_HOLD;

				vectoangles(d, dAng);
				dAng[YAW] = AngleSubtract(owner->client->ps.viewangles[YAW], dAng[YAW]);
				if (dAng[YAW] > 0.0f)
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1)
					{
						//reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}
					G_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_LEFT1, aFlags, 0);
				}
				else
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_LEFT1)
					{
						//reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}
					G_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_RIGHT1, aFlags, 0);
				}
			}
			else if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1 || owner->client->ps.legsAnim ==
				BOTH_STRAFE_LEFT1)
			{
				//don't keep animating in place
				owner->client->ps.legsTimer = 0;
			}

			G_SetOrigin(owner, tr.endpos);
			VectorCopy(tr.endpos, owner->client->ps.origin);
		}
	}
	else
	{
		//can't move here.. stop using the thing I guess
		EWebDisattach(owner, eweb);
	}
}

//keep the bone angles updated based on the owner's look dir
static void EWebUpdateBoneAngles(gentity_t* owner, gentity_t* eweb)
{
	vec3_t yAng;
	const float turnCap = 4.0f; //max degrees we can turn per update

	VectorClear(yAng);
	const float ideal = AngleSubtract(owner->client->ps.viewangles[YAW], eweb->s.angles[YAW]);
	float incr = AngleSubtract(ideal, eweb->angle);

	if (incr > turnCap)
	{
		incr = turnCap;
	}
	else if (incr < -turnCap)
	{
		incr = -turnCap;
	}

	eweb->angle += incr;

	yAng[0] = eweb->angle;
	EWeb_SetBoneAngles(eweb, "cannon_Yrot", yAng);

	EWebPositionUser(owner, eweb);
	if (!owner->client->ewebIndex)
	{
		//was removed during position function
		return;
	}

	VectorClear(yAng);
	yAng[2] = AngleSubtract(owner->client->ps.viewangles[PITCH], eweb->s.angles[PITCH]) * 0.8f;
	EWeb_SetBoneAngles(eweb, "cannon_Xrot", yAng);
}

//keep it updated
extern int BG_EmplacedView(vec3_t base_angles, vec3_t angles, float* new_yaw, float constraint); //bg_misc.c

static void EWebThink(gentity_t* self)
{
	qboolean killMe = qfalse;
	const float gravity = 3.0f;
	const float mass = 0.09f;
	const float bounce = 1.1f;

	if (self->r.ownerNum == ENTITYNUM_NONE)
	{
		killMe = qtrue;
	}
	else
	{
		gentity_t* owner = &g_entities[self->r.ownerNum];

		if (!owner->inuse || !owner->client || owner->client->pers.connected != CON_CONNECTED ||
			owner->client->ewebIndex != self->s.number || owner->health < 1)
		{
			killMe = qtrue;
		}
		else if (owner->client->ps.emplacedIndex != self->s.number)
		{
			//just go back to the inventory then
			EWebDisattach(owner, self);
			return;
		}

		if (!killMe)
		{
			float yaw;

			if (BG_EmplacedView(owner->client->ps.viewangles, self->s.angles, &yaw, self->s.origin2[0]))
			{
				owner->client->ps.viewangles[YAW] = yaw;
			}
			owner->client->ps.weapon = WP_EMPLACED_GUN;
			owner->client->ps.stats[STAT_WEAPONS] = WP_EMPLACED_GUN;

			if (self->genericValue8 < level.time)
			{
				//make sure the anim timer is done
				EWebUpdateBoneAngles(owner, self);
				if (!owner->client->ewebIndex)
				{
					//was removed during position function
					return;
				}

				if (owner->client->pers.cmd.buttons & BUTTON_ATTACK)
				{
					if (self->genericValue5 < level.time)
					{
						//we can fire another shot off
						EWebFire(owner, self);

						//cheap firing anim
						EWeb_SetBoneAnim(self, 2, 4);
						self->genericValue3 = 1;

						//set fire debounce time
						self->genericValue5 = level.time + 100;
					}
				}
				else if (self->genericValue5 < level.time && self->genericValue3)
				{
					//reset the anim back to non-firing
					EWeb_SetBoneAnim(self, 0, 1);
					self->genericValue3 = 0;
				}
			}
		}
	}

	if (killMe)
	{
		//something happened to the owner, let's explode
		EWebDie(self, self, self, 999, MOD_SUICIDE);
		return;
	}

	//run some physics on it real quick so it falls and stuff properly
	G_RunExPhys(self, gravity, mass, bounce, qfalse, NULL, 0);

	self->nextthink = level.time;
}

#define EWEB_HEALTH			200

//spawn and set up an e-web ent
static gentity_t* EWeb_Create(gentity_t* spawner)
{
	const char* modelName = "models/map_objects/hoth/eweb_model.glm";
	const int failSound = G_SoundIndex("sound/interface/shieldcon_empty");
	trace_t tr;
	vec3_t fAng, fwd, pos, downPos, s;
	vec3_t mins, maxs;

	VectorSet(mins, -32, -32, -24);
	VectorSet(maxs, 32, 32, 24);

	VectorSet(fAng, 0, spawner->client->ps.viewangles[1], 0);
	AngleVectors(fAng, fwd, 0, 0);

	VectorCopy(spawner->client->ps.origin, s);
	//allow some fudge
	s[2] += 12.0f;

	VectorMA(s, 48.0f, fwd, pos);

	trap->Trace(&tr, s, mins, maxs, pos, spawner->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.allsolid || tr.startsolid || tr.fraction != 1.0f)
	{
		//can't spawn here, we are in solid
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	gentity_t* ent = G_Spawn();

	ent->clipmask = MASK_PLAYERSOLID;
	ent->r.contents = MASK_PLAYERSOLID;

	ent->physicsObject = qtrue;

	//for the sake of being able to differentiate client-side between this and an emplaced gun
	ent->s.weapon = WP_NONE;

	VectorCopy(pos, downPos);
	downPos[2] -= 18.0f;
	trap->Trace(&tr, pos, mins, maxs, downPos, spawner->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.startsolid || tr.allsolid || tr.fraction == 1.0f || tr.entityNum < ENTITYNUM_WORLD)
	{
		//didn't hit ground.
		G_FreeEntity(ent);
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	VectorCopy(tr.endpos, pos);

	G_SetOrigin(ent, pos);

	VectorCopy(fAng, ent->s.apos.trBase);
	VectorCopy(fAng, ent->r.currentAngles);

	ent->s.owner = spawner->s.number;
	ent->s.teamowner = spawner->client->sess.sessionTeam;

	ent->takedamage = qtrue;

	if (spawner->client->ewebHealth <= 0)
	{
		//refresh the owner's e-web health if its last e-web did not exist or was killed
		spawner->client->ewebHealth = EWEB_HEALTH;
	}

	//resume health of last deployment
	ent->maxHealth = EWEB_HEALTH;
	ent->health = spawner->client->ewebHealth;
	G_ScaleNetHealth(ent);

	ent->die = EWebDie;
	ent->pain = EWebPain;

	ent->think = EWebThink;
	ent->nextthink = level.time;

	//set up the g2 model info
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 128;
	ent->s.modelIndex = G_model_index((char*)modelName);

	trap->G2API_InitGhoul2Model(&ent->ghoul2, modelName, 0, 0, 0, 0, 0);

	if (!ent->ghoul2)
	{
		//should not happen, but just to be safe.
		G_FreeEntity(ent);
		return NULL;
	}

	//initialize bone angles
	EWeb_SetBoneAngles(ent, "cannon_Yrot", vec3_origin);
	EWeb_SetBoneAngles(ent, "cannon_Xrot", vec3_origin);

	ent->genericValue10 = trap->G2API_AddBolt(ent->ghoul2, 0, "*cannonflash"); //muzzle bolt
	ent->genericValue9 = trap->G2API_AddBolt(ent->ghoul2, 0, "cannon_Yrot");
	//for placing the owner relative to rotation

	//set the constraints for this guy as an emplaced weapon, and his constraint angles
	ent->s.origin2[0] = 360.0f; //360 degrees in either direction

	VectorCopy(fAng, ent->s.angles); //consider "angle 0" for constraint

	//angle of y rot bone
	ent->angle = 0.0f;

	ent->r.ownerNum = spawner->s.number;
	trap->LinkEntity((sharedEntity_t*)ent);

	//store off the owner's current weapons, we will be forcing him to use the "emplaced" weapon
	ent->genericValue11 = spawner->client->ps.stats[STAT_WEAPONS];

	//start the "unfolding" anim
	EWeb_SetBoneAnim(ent, 4, 20);
	//don't allow use until the anim is done playing (rough time estimate)
	ent->genericValue8 = level.time + 500;

	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);

	return ent;
}

#define EWEB_USE_DEBOUNCE		1000
//use the e-web
void ItemUse_UseEWeb(gentity_t* ent)
{
	if (ent->client->ewebTime > level.time)
	{
		//can't use again yet
		return;
	}

	if (ent->client->ps.weaponTime > 0 ||
		ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		//busy doing something else
		return;
	}

	if (ent->client->ps.emplacedIndex && !ent->client->ewebIndex)
	{
		//using an emplaced gun already that isn't our own e-web
		return;
	}

	if (ent->client->ewebIndex)
	{
		//put it away
		EWebDisattach(ent, &g_entities[ent->client->ewebIndex]);
	}
	else
	{
		//create it
		const gentity_t* eweb = EWeb_Create(ent);

		if (eweb)
		{
			//if it's null the thing couldn't spawn (probably no room)
			ent->client->ewebIndex = eweb->s.number;
			ent->client->ps.emplacedIndex = eweb->s.number;
		}
	}

	ent->client->ewebTime = level.time + EWEB_USE_DEBOUNCE;
}

//===============================================
//End E-Web
//===============================================

static int Pickup_Powerup(const gentity_t* ent, const gentity_t* other)
{
	int quantity;

	if (!other->client->ps.powerups[ent->item->giTag])
	{
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->giTag] =
			level.time - level.time % 1000;

		G_LogWeaponPowerup(other->s.number, ent->item->giTag);
	}

	if (ent->count)
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	if (ent->item->giTag == PW_YSALAMIRI)
	{
		other->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] = 0;
		other->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] = 0;
		other->client->ps.powerups[PW_FORCE_BOON] = 0;
	}

	// give any nearby players a "denied" anti-reward
	for (int i = 0; i < level.maxclients; i++)
	{
		vec3_t delta;
		vec3_t forward;
		trace_t tr;

		gclient_t* client = &level.clients[i];
		if (client == other->client)
		{
			continue;
		}
		if (client->pers.connected == CON_DISCONNECTED)
		{
			continue;
		}
		if (client->ps.stats[STAT_HEALTH] <= 0)
		{
			continue;
		}

		// if same team in team game, no sound
		// cannot use OnSameTeam as it expects to g_entities, not clients
		if (level.gametype >= GT_TEAM && other->client->sess.sessionTeam == client->sess.sessionTeam)
		{
			continue;
		}

		// if too far away, no sound
		VectorSubtract(ent->s.pos.trBase, client->ps.origin, delta);
		const float len = VectorNormalize(delta);
		if (len > 192)
		{
			continue;
		}

		// if not facing, no sound
		AngleVectors(client->ps.viewangles, forward, NULL, NULL);
		if (DotProduct(delta, forward) < 0.4)
		{
			continue;
		}

		// if not line of sight, no sound
		trap->Trace(&tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID, qfalse, 0,
			0);
		if (tr.fraction != 1.0)
		{
			continue;
		}

		// anti-reward
		client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
	}
	return RESPAWN_POWERUP;
}

//======================================================================

static int Pickup_Holdable(const gentity_t* ent, const gentity_t* other)
{
	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	other->client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << ent->item->giTag;

	G_LogWeaponItem(other->s.number, ent->item->giTag);

	return adjustRespawnTime(RESPAWN_HOLDABLE, ent->item->giType, ent->item->giTag);
}

//======================================================================

void Add_Ammo3(const gentity_t* ent, const int weapon, const int count, int* stop, qboolean* gaveSome)
{
	// weapon is actually ammotype
	if (ent->client->ps.eFlags & EF_DOUBLE_AMMO)
	{
		if (ent->client->ps.ammo[weapon] < ammoData[weapon].max * 2)
		{
			*gaveSome = qtrue;
			ent->client->ps.ammo[weapon] += count;
			if (ent->client->ps.ammo[weapon] >= ammoData[weapon].max * 2)
			{
				ent->client->ps.ammo[weapon] = ammoData[weapon].max * 2;
			}
			else
			{
				*stop = 0;
			}
		}
	}
	else
	{
		if (ent->client->ps.ammo[weapon] < ammoData[weapon].max)
		{
			*gaveSome = qtrue;
			ent->client->ps.ammo[weapon] += count;
			if (ent->client->ps.ammo[weapon] >= ammoData[weapon].max)
			{
				ent->client->ps.ammo[weapon] = ammoData[weapon].max;
			}
			else
			{
				*stop = 0;
			}
		}
	}
}

void Add_Ammo(const gentity_t* ent, const int weapon, const int count)
{
	if (ent->client->ps.eFlags & EF_DOUBLE_AMMO)
	{
		if (ent->client->ps.ammo[weapon] < ammoData[weapon].max * 2)
		{
			ent->client->ps.ammo[weapon] += count;
			if (ent->client->ps.ammo[weapon] >= ammoData[weapon].max * 2)
			{
				ent->client->ps.ammo[weapon] = ammoData[weapon].max * 2;
			}
		}
	}
	else
	{
		if (ent->client->ps.ammo[weapon] < ammoData[weapon].max)
		{
			ent->client->ps.ammo[weapon] += count;
			if (ent->client->ps.ammo[weapon] >= ammoData[weapon].max)
			{
				ent->client->ps.ammo[weapon] = ammoData[weapon].max;
			}
		}
	}
}

//======================================================================
void Add_Batteries(gentity_t* ent, int* count)
{
	if (ent->client && ent->client->ps.batteryCharge < MAX_BATTERIES && *count)
	{
		if (*count + ent->client->ps.batteryCharge > MAX_BATTERIES)
		{
			// steal what we need, then leave the rest for later
			*count -= MAX_BATTERIES - ent->client->ps.batteryCharge;
			ent->client->ps.batteryCharge = MAX_BATTERIES;
		}
		else
		{
			// just drain all of the batteries
			ent->client->ps.batteryCharge += *count;
			*count = 0;
		}

		G_AddEvent(ent, EV_BATTERIES_CHARGED, 0);
	}
}

//-------------------------------------------------------
static int Pickup_Battery(const gentity_t* ent, gentity_t* other)
{
	int quantity;

	if (ent->count)
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	// There may be some left over in quantity if the player is close to full, but with pickup items, this amount will just be lost
	Add_Batteries(other, &quantity);

	return 30;
}

static int Pickup_Ammo(const gentity_t* ent, const gentity_t* other)
{
	int quantity;

	if (ent->count)
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	if (ent->item->giTag == -1)
	{
		//an ammo_all, give them a bit of everything
		if (level.gametype == GT_SIEGE)
			// complaints that siege tech's not giving enough ammo.  Does anything else use ammo all?
		{
			Add_Ammo(other, AMMO_BLASTER, 100);
			Add_Ammo(other, AMMO_POWERCELL, 100);
			Add_Ammo(other, AMMO_METAL_BOLTS, 100);
			Add_Ammo(other, AMMO_ROCKETS, 5);
			if (other->client->ps.stats[STAT_WEAPONS] & 1 << WP_DET_PACK)
			{
				Add_Ammo(other, AMMO_DETPACK, 2);
			}
			if (other->client->ps.stats[STAT_WEAPONS] & 1 << WP_THERMAL)
			{
				Add_Ammo(other, AMMO_THERMAL, 2);
			}
			if (other->client->ps.stats[STAT_WEAPONS] & 1 << WP_TRIP_MINE)
			{
				Add_Ammo(other, AMMO_TRIPMINE, 2);
			}
		}
		else
		{
			Add_Ammo(other, AMMO_BLASTER, 50);
			Add_Ammo(other, AMMO_POWERCELL, 50);
			Add_Ammo(other, AMMO_METAL_BOLTS, 50);
			Add_Ammo(other, AMMO_ROCKETS, 2);
		}
	}
	else
	{
		Add_Ammo(other, ent->item->giTag, quantity);
	}

	return adjustRespawnTime(RESPAWN_AMMO, ent->item->giType, ent->item->giTag);
}

//======================================================================

static int Pickup_Weapon(const gentity_t* ent, gentity_t* other)
{
	int quantity;

	if (ent->count < 0)
	{
		quantity = 0; // None for you, sir!
	}
	else
	{
		if (ent->count)
		{
			quantity = ent->count;
		}
		else
		{
			quantity = ent->item->quantity;
		}

		// dropped items and teamplay weapons always have full ammo
		if (!(ent->flags & FL_DROPPED_ITEM) && level.gametype != GT_TEAM)
		{
			// respawning rules

			// New method:  If the player has less than half the minimum, give them the minimum, else add 1/2 the min.

			// drop the quantity if the already have over the minimum
			if (other->client->ps.ammo[ent->item->giTag] < quantity * 0.5)
			{
				quantity = quantity - other->client->ps.ammo[ent->item->giTag];
			}
			else
			{
				quantity = quantity * 0.5; // only add half the value.
			}
		}
	}

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= 1 << ent->item->giTag;

	G_AddEvent(other, EV_WEAPINVCHANGE, other->client->ps.stats[STAT_WEAPONS]);

	//Add_Ammo( other, ent->item->giTag, quantity );
	Add_Ammo(other, weaponData[ent->item->giTag].ammoIndex, quantity);

	G_LogWeaponPickup(other->s.number, ent->item->giTag);

	// team deathmatch has slow weapon respawns
	if (level.gametype == GT_TEAM)
	{
		return adjustRespawnTime(RESPAWN_TEAM_WEAPON, ent->item->giType, ent->item->giTag);
	}

	return adjustRespawnTime(g_weaponRespawn.integer, ent->item->giType, ent->item->giTag);
}

//======================================================================

static int Pickup_Health(const gentity_t* ent, gentity_t* other)
{
	int max;
	int quantity;

	// small and mega healths will go over the max
	if (ent->item->quantity != 5 && ent->item->quantity != 100)
	{
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	else
	{
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if (ent->count)
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max)
	{
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if (ent->item->quantity == 100)
	{
		// mega health respawns slow
		return RESPAWN_MEGAHEALTH;
	}

	return adjustRespawnTime(RESPAWN_HEALTH, ent->item->giType, ent->item->giTag);
}

//======================================================================

static int Pickup_Armor(const gentity_t* ent, const gentity_t* other)
{
	// make sure that the shield effect is on
	other->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;

	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
	if (other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_HEALTH] * ent->item->giTag)
	{
		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_HEALTH] * ent->item->giTag;
	}

	return adjustRespawnTime(RESPAWN_ARMOR, ent->item->giType, ent->item->giTag);
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem(gentity_t* ent)
{
	// randomly select from teamed entities
	if (ent->team)
	{
		int count;

		if (!ent->teammaster)
		{
			trap->Error(ERR_DROP, "RespawnItem: bad teammaster");
		}
		gentity_t* master = ent->teammaster;
		for (count = 0, ent = master; ent; ent = ent->teamchain, count++);

		const int choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++);
	}

	if (g_pushitems.integer)
	{
		VectorCopy(ent->origOrigin, ent->s.origin);
		VectorCopy(ent->origOrigin, ent->s.pos.trBase);
		VectorCopy(ent->origOrigin, ent->s.apos.trBase);
		VectorCopy(ent->origOrigin, ent->r.currentOrigin);
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~(EF_NODRAW | EF_ITEMPLACEHOLDER);
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap->LinkEntity((sharedEntity_t*)ent);

	if (ent->item->giType == IT_POWERUP)
	{
		// play powerup spawn sound to all clients
		gentity_t* te;

		// if the powerup respawn sound should Not be global
		if (ent->speed)
		{
			te = G_TempEntity(ent->s.pos.trBase, EV_GENERAL_SOUND);
		}
		else
		{
			te = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_SOUND);
		}
		te->s.eventParm = G_SoundIndex("sound/items/respawn1");
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent(ent, EV_ITEM_RESPAWN, 0);

	ent->nextthink = 0;
}

qboolean CheckItemCanBePickedUpByNPC(const gentity_t* item, const gentity_t* pickerupper)
{
	if (!item->item)
	{
		return qfalse;
	}
	if (item->item->giType == IT_HOLDABLE)
	{
		return qfalse;
	}
	if (item->flags & FL_DROPPED_ITEM
		&& item->activator != &g_entities[0]
		&& pickerupper->s.number
		&& pickerupper->s.weapon == WP_NONE
		&& pickerupper->enemy
		&& pickerupper->painDebounceTime < level.time
		&& pickerupper->NPC && pickerupper->NPC->surrenderTime < level.time //not surrendering
		&& !(pickerupper->NPC->scriptFlags & SCF_FORCED_MARCH)) //not being forced to march
	{
		//non-player, in combat, picking up a dropped item that does NOT belong to the player and it *not* a security key
		if (level.time - item->s.time < 3000) //was 5000
		{
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}

void ResetItem(gentity_t* ent)
{
	VectorCopy(ent->origOrigin, ent->s.origin);
	VectorCopy(ent->origOrigin, ent->s.pos.trBase);
	VectorCopy(ent->origOrigin, ent->s.apos.trBase);
	VectorCopy(ent->origOrigin, ent->r.currentOrigin);

	trap->LinkEntity((sharedEntity_t*)ent);
}

qboolean G_CanPickUpWeapons(const gentity_t* other)
{
	if (!other || !other->client)
	{
		return qfalse;
	}
	switch (other->client->NPC_class)
	{
	case CLASS_ATST:
	case CLASS_GONK:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_MOUSE:
	case CLASS_PROBE:
	case CLASS_PROTOCOL:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_RANCOR:
	case CLASS_WAMPA:
	case CLASS_JAWA: //FIXME: in some cases it's okay?
	case CLASS_UGNAUGHT: //FIXME: in some cases it's okay?
	case CLASS_SENTRY:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/*
===============
Touch_Item
===============
*/

void Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace)
{
	int respawn;

	if (ent->genericValue10 > level.time &&
		other &&
		other->s.number == ent->genericValue11)
	{
		//this is the ent that we don't want to be able to touch us for x seconds
		return;
	}

	//if (!(other->client->pers.cmd.buttons & BUTTON_USE) && (!other->s.number))
	//{//not holding use?
	//	return;
	//}

	if (ent->s.eFlags & EF_ITEMPLACEHOLDER)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (ent->item->giType == IT_WEAPON &&
		ent->s.powerups &&
		ent->s.powerups < level.time)
	{
		ent->s.generic1 = 0;
		ent->s.powerups = 0;
	}

	if (!other->client)
		return;
	if (other->health < 1)
		return; // dead people can't pickup

	if (ent->item->giType == IT_POWERUP &&
		(ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT || ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK))
	{
		if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT)
		{
			if (other->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				return;
			}
		}
		else
		{
			if (other->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				return;
			}
		}
	}

	// the same pickup rules are used for client side and server side
	if (!BG_CanItemBeGrabbed(level.gametype, &ent->s, &other->client->ps))
	{
		return;
	}

	if (other->client->NPC_class == CLASS_ATST ||
		other->client->NPC_class == CLASS_GONK ||
		other->client->NPC_class == CLASS_MARK1 ||
		other->client->NPC_class == CLASS_MARK2 ||
		other->client->NPC_class == CLASS_MOUSE ||
		other->client->NPC_class == CLASS_PROBE ||
		other->client->NPC_class == CLASS_PROTOCOL ||
		other->client->NPC_class == CLASS_R2D2 ||
		other->client->NPC_class == CLASS_R5D2 ||
		other->client->NPC_class == CLASS_SEEKER ||
		other->client->NPC_class == CLASS_REMOTE ||
		other->client->NPC_class == CLASS_RANCOR ||
		other->client->NPC_class == CLASS_WAMPA ||
		other->client->NPC_class == CLASS_SBD || //FIXME: in some cases it's okay?
		other->client->NPC_class == CLASS_UGNAUGHT || //FIXME: in some cases it's okay?
		other->client->NPC_class == CLASS_SENTRY)
	{
		//FIXME: some flag would be better
		//droids can't pick up items/weapons!
		return;
	}

	if (CheckItemCanBePickedUpByNPC(ent, other))
	{
		if (other->NPC && other->NPC->goalEntity && other->NPC->goalEntity->enemy == ent)
		{
			//they were running to pick me up, they did, so clear goal
			other->NPC->goalEntity = NULL;
			other->NPC->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	else if (!(ent->spawnflags & ITMSF_ALLOWNPC))
	{
		// NPCs cannot pick it up
		if (other->s.eType == ET_NPC)
		{
			// Not the player?
			qboolean dontGo = qfalse;
			if (ent->item->giType == IT_AMMO &&
				ent->item->giTag == -1 &&
				other->s.NPC_class == CLASS_VEHICLE &&
				other->m_pVehicle &&
				other->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)
			{
				//yeah, uh, atst gets healed by these things
				if (other->maxHealth &&
					other->health < other->maxHealth)
				{
					other->health += 80;
					if (other->health > other->maxHealth)
					{
						other->health = other->maxHealth;
					}
					G_ScaleNetHealth(other);
					dontGo = qtrue;
				}
			}

			if (!dontGo)
			{
				return;
			}
		}
	}

	G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);

	qboolean predict = other->client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch (ent->item->giType)
	{
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
		//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
		if (ent->item->giTag == AMMO_THERMAL || ent->item->giTag == AMMO_TRIPMINE || ent->item->giTag == AMMO_DETPACK)
		{
			int weap_for_ammo;

			if (ent->item->giTag == AMMO_THERMAL)
			{
				weap_for_ammo = WP_THERMAL;
			}
			else if (ent->item->giTag == AMMO_TRIPMINE)
			{
				weap_for_ammo = WP_TRIP_MINE;
			}
			else
			{
				weap_for_ammo = WP_DET_PACK;
			}

			if (other && other->client && other->client->ps.ammo[weaponData[weap_for_ammo].ammoIndex] > 0)
			{
				other->client->ps.stats[STAT_WEAPONS] |= 1 << weap_for_ammo;
				G_AddEvent(other, EV_WEAPINVCHANGE, other->client->ps.stats[STAT_WEAPONS]);
			}
		}
		//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		//		predict = qfalse;
		predict = qtrue;
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		predict = qtrue;
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	case IT_BATTERY:
		respawn = Pickup_Battery(ent, other);
		break;
	default:
		return;
	}

	if (!respawn)
	{
		return;
	}

	// play the normal pickup sound
	if (predict)
	{
		if (other && other->client)
		{
			BG_AddPredictableEventToPlayerstate(EV_ITEM_PICKUP, ent->s.number, &other->client->ps);
		}
		else
		{
			G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.number);
		}
		NPC_SetAnim(other, SETANIM_TORSO, BOTH_BUTTON2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		G_AddEvent(other, EV_ITEM_PICKUP, ent->s.number);
		NPC_SetAnim(other, SETANIM_TORSO, BOTH_BUTTON2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	// powerup pickups are global broadcasts
	if (/*ent->item->giType == IT_POWERUP ||*/ ent->item->giType == IT_TEAM)
	{
		// if we want the global sound to play
		if (!ent->speed)
		{
			gentity_t* te = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelIndex;
			te->r.svFlags |= SVF_BROADCAST;
		}
		else
		{
			gentity_t* te = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP);
			te->s.eventParm = ent->s.modelIndex;
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	// fire item targets
	G_UseTargets(ent, other);

	// wait of -1 will not respawn
	if (ent->wait == -1)
	{
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if (ent->wait)
	{
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if (ent->random)
	{
		respawn += Q_flrand(-1.0f, 1.0f) * ent->random;
		if (respawn < 1)
		{
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if (ent->flags & FL_DROPPED_ITEM)
	{
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	if (!(ent->flags & FL_DROPPED_ITEM) && (ent->item->giType == IT_WEAPON || ent->item->giType == IT_POWERUP))
	{
		ent->s.eFlags |= EF_ITEMPLACEHOLDER;
		ent->s.eFlags &= ~EF_NODRAW;
	}
	else
	{
		ent->s.eFlags |= EF_NODRAW;
		ent->r.svFlags |= SVF_NOCLIENT;
	}
	ent->r.contents = 0;

	if (ent->genericValue9)
	{
		//dropped item, should be removed when picked up
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if (respawn <= 0)
	{
		ent->nextthink = 0;
		ent->think = 0;
	}
	else
	{
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap->LinkEntity((sharedEntity_t*)ent);
}

//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t* LaunchItem(gitem_t* item, vec3_t origin, vec3_t velocity)
{
	gentity_t* dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelIndex = item - bg_itemlist; // store item number in modelIndex
	if (dropped->s.modelIndex < 0)
	{
		dropped->s.modelIndex = 0;
	}
	dropped->s.model_index2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet(dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet(dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);

	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin(dropped, origin);
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy(velocity, dropped->s.pos.trDelta);

	dropped->flags |= FL_BOUNCE_HALF;
	if ((level.gametype == GT_CTF || level.gametype == GT_CTY) && item->giType == IT_TEAM)
	{
		// Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem(dropped);

		//rww - so bots know
		if (dropped->item->giTag == PW_REDFLAG)
		{
			droppedRedFlag = dropped;
		}
		else if (dropped->item->giTag == PW_BLUEFLAG)
		{
			droppedBlueFlag = dropped;
		}
	}
	else
	{
		// auto-remove after 30 seconds
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	if (item->giType == IT_WEAPON || item->giType == IT_POWERUP)
	{
		dropped->s.eFlags |= EF_DROPPEDWEAPON;
	}

	vectoangles(velocity, dropped->s.angles);
	dropped->s.angles[PITCH] = 0;

	if (item->giTag == WP_TRIP_MINE ||
		item->giTag == WP_DET_PACK)
	{
		dropped->s.angles[PITCH] = -90;
	}

	if (item->giTag != WP_BOWCASTER &&
		item->giTag != WP_DET_PACK &&
		item->giTag != WP_THERMAL)
	{
		dropped->s.angles[ROLL] = -90;
	}

	dropped->physicsObject = qtrue;

	trap->LinkEntity((sharedEntity_t*)dropped);

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t* Drop_Item(gentity_t* ent, gitem_t* item, const float angle)
{
	vec3_t velocity;
	vec3_t angles;

	VectorCopy(ent->s.apos.trBase, angles);
	angles[YAW] += angle;
	angles[PITCH] = 0; // always forward

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 150, velocity);
	velocity[2] += 200 + Q_flrand(-1.0f, 1.0f) * 50;

	return LaunchItem(item, ent->s.pos.trBase, velocity);
}

/*
================
Use_Item

Respawn the item
================
*/
void Use_Item(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
	RespawnItem(ent);
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem(gentity_t* ent)
{
	trace_t tr;

	if (level.gametype == GT_SIEGE)
	{
		//in siege remove all powerups
		if (ent->item->giType == IT_POWERUP)
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype != GT_JEDIMASTER)
	{
		if (HasSetSaberOnly())
		{
			if (ent->item->giType == IT_AMMO)
			{
				G_FreeEntity(ent);
				return;
			}

			if (ent->item->giType == IT_HOLDABLE)
			{
				if (ent->item->giTag == HI_SEEKER ||
					ent->item->giTag == HI_SHIELD ||
					ent->item->giTag == HI_SENTRY_GUN)
				{
					G_FreeEntity(ent);
					return;
				}
			}
		}
	}
	else
	{
		//no powerups in jedi master
		if (ent->item->giType == IT_POWERUP)
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype == GT_HOLOCRON)
	{
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	if (AllForceDisabled(g_forcePowerDisable.integer))
	{
		//if force powers disabled, don't add force powerups
		if (ent->item->giType == IT_POWERUP)
		{
			if (ent->item->giTag == PW_FORCE_ENLIGHTENED_LIGHT ||
				ent->item->giTag == PW_FORCE_ENLIGHTENED_DARK ||
				ent->item->giTag == PW_FORCE_BOON)
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		if (ent->item->giType == IT_ARMOR ||
			ent->item->giType == IT_HEALTH ||
			ent->item->giType == IT_HOLDABLE && (ent->item->giTag == HI_MEDPAC || ent->item->giTag == HI_MEDPAC_BIG))
		{
			G_FreeEntity(ent);
			return;
		}
	}

	if (level.gametype != GT_CTF &&
		level.gametype != GT_CTY &&
		ent->item->giType == IT_TEAM)
	{
		int killMe = 0;

		switch (ent->item->giTag)
		{
		case PW_REDFLAG:
			killMe = 1;
			break;
		case PW_BLUEFLAG:
			killMe = 1;
			break;
		case PW_NEUTRALFLAG:
			killMe = 1;
			break;
		default:
			break;
		}

		if (killMe)
		{
			G_FreeEntity(ent);
			return;
		}
	}

	VectorSet(ent->r.mins, -8, -8, -0);
	VectorSet(ent->r.maxs, 8, 8, 16);

	ent->s.eType = ET_ITEM;
	ent->s.modelIndex = ent->item - bg_itemlist; // store item number in modelIndex
	ent->s.model_index2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;

	// create a Ghoul2 model if the world model is a glm
	/*	item = &bg_itemlist[ ent->s.modelIndex ];
		if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
		{
			trap->G2API_InitGhoul2Model(&ent->s, item->world_model[0], G_model_index(item->world_model[0] ), 0, 0, 0, 0);
			ent->s.radius = 60;
		}
	*/
	if (ent->spawnflags & ITMSF_SUSPEND)
	{
		// suspended
		G_SetOrigin(ent, ent->s.origin);
	}
	else
	{
		vec3_t dest;
		// drop to floor

		//if it is directly even with the floor it will return startsolid, so raise up by 0.1
		//and temporarily subtract 0.1 from the z maxs so that going up doesn't push into the ceiling
		ent->s.origin[2] += 0.1f;
		ent->r.maxs[2] -= 0.1f;

		VectorSet(dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096);
		trap->Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID, qfalse, 0, 0);
		if (tr.startsolid)
		{
			trap->Print("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity(ent);
			return;
		}

		//add the 0.1 back after the trace
		ent->r.maxs[2] += 0.1f;

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin(ent, tr.endpos);
	}

	if (ent->spawnflags & ITMSF_VERTICAL)
	{
		ent->s.angles[PITCH] += 75;
		G_SetAngles(ent, ent->s.angles);
	}

	// team slaves and targeted items aren't present at start
	if (ent->flags & FL_TEAMSLAVE || ent->targetname)
	{
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// powerups don't spawn in for a while
	/*
	if ( ent->item->giType == IT_POWERUP ) {
		float	respawn;

		respawn = 45 + Q_flrand(-1.0f, 1.0f) * 15;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}
	*/

	trap->LinkEntity((sharedEntity_t*)ent);
}

qboolean itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems(void)
{
	// Set up team stuff
	Team_InitGame();

	if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		// check for the two flags
		const gitem_t* item = BG_FindItem("team_CTF_redflag");
		if (!item || !itemRegistered[item - bg_itemlist])
		{
			trap->Print(S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n");
		}
		item = BG_FindItem("team_CTF_blueflag");
		if (!item || !itemRegistered[item - bg_itemlist])
		{
			trap->Print(S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n");
		}
	}
}

/*
==============
ClearRegisteredItems
==============
*/
void clear_registered_items(void)
{
	memset(itemRegistered, 0, sizeof itemRegistered);

	// players always start with the base weapon
	register_item(BG_FindItemForWeapon(WP_BRYAR_PISTOL));
	register_item(BG_FindItemForWeapon(WP_STUN_BATON));
	register_item(BG_FindItemForWeapon(WP_MELEE));
	register_item(BG_FindItemForWeapon(WP_SABER));
	//addition possible starting weapons
	register_item(BG_FindItemForWeapon(WP_BLASTER));
	register_item(BG_FindItemForWeapon(WP_THERMAL));
	register_item(BG_FindItemForWeapon(WP_ROCKET_LAUNCHER));
	register_item(BG_FindItemForWeapon(WP_BOWCASTER));
	register_item(BG_FindItemForWeapon(WP_REPEATER));
	register_item(BG_FindItemForWeapon(WP_DISRUPTOR));
	register_item(BG_FindItemForWeapon(WP_CONCUSSION));

	if (level.gametype == GT_SIEGE)
	{//kind of cheesy, maybe check if siege class with disp's is gonna be on this map too
		G_PrecacheDispensers();
	}
	else
	{
		G_LoadDispensers();
	}
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void register_item(const gitem_t* item)
{
	if (!item)
	{
		trap->Error(ERR_DROP, "register_item: NULL");
	}
	itemRegistered[item - bg_itemlist] = qtrue;
}

/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void save_registered_items(void)
{
	char string[MAX_ITEMS + 1];

	int count = 0;
	for (int i = 0; i < bg_numItems; i++)
	{
		if (itemRegistered[i])
		{
			count++;
			string[i] = '1';
		}
		else
		{
			string[i] = '0';
		}
	}
	string[bg_numItems] = 0;
	trap->SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
static int G_ItemDisabled(const gitem_t* item)
{
	char name[128];

	Com_sprintf(name, sizeof name, "disable_%s", item->classname);
	return trap->Cvar_VariableIntegerValue(name);
}

//This function checks an ammo type against the disabled weapons list.
static qboolean G_AmmoDisabled(const int wDisable, const gitem_t* item)
{
	if (item->giType != IT_AMMO)
	{
		Com_Printf("Error: G_AmmoDisabled run for an non-ammo item.\n");
		return qfalse;
	}

	switch (item->giTag)
	{
	case AMMO_FORCE:
	case AMMO_EMPLACED:
		//never disable these since they aren't actually associated with a weapon
		return qfalse;
	case AMMO_BLASTER:
	{
		if (!(wDisable & 1 << WP_BRYAR_PISTOL)
			|| !(wDisable & 1 << WP_BLASTER)
			|| !(wDisable & 1 << WP_BRYAR_OLD))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_POWERCELL:
	{
		if (!(wDisable & 1 << WP_DISRUPTOR)
			|| !(wDisable & 1 << WP_BOWCASTER)
			|| !(wDisable & 1 << WP_DEMP2))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_METAL_BOLTS:
	{
		if (!(wDisable & 1 << WP_REPEATER)
			|| !(wDisable & 1 << WP_FLECHETTE))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_ROCKETS:
	{
		if (!(wDisable & 1 << WP_ROCKET_LAUNCHER))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_THERMAL:
	{
		if (!(wDisable & 1 << WP_THERMAL))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_TRIPMINE:
	{
		if (!(wDisable & 1 << WP_THERMAL))
		{
			return qfalse;
		}
		return qtrue;
	}
	case AMMO_DETPACK:
	{
		if (!(wDisable & 1 << WP_THERMAL))
		{
			return qfalse;
		}
		return qtrue;
	}
	default:
		Com_Printf("Error: G_AmmoDisabled couldn't find ammo type.\n");
		return qfalse;
	}
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem(gentity_t* ent, gitem_t* item)
{
	int w_disable;

	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		w_disable = g_duelWeaponDisable.integer;
	}
	else
	{
		w_disable = g_weaponDisable.integer;
	}

	if (item->giType == IT_WEAPON &&
		w_disable &&
		w_disable & 1 << item->giTag)
	{
		G_FreeEntity(ent);
		return;
	}
	if (item->giType == IT_AMMO)
	{
		if (G_AmmoDisabled(w_disable, item))
		{
			//This ammo is disabled.
			G_FreeEntity(ent);
			return;
		}
	}

	register_item(item);
	if (G_ItemDisabled(item))
		return;

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50f; // items are bouncy

	if (item->giType == IT_POWERUP)
	{
		G_SoundIndex("sound/items/respawn1");
		G_SpawnFloat("noglobalsound", "0", &ent->speed);
	}
}

/*
================
G_BounceItem

================
*/
static void G_BounceItem(gentity_t* ent, trace_t* trace)
{
	vec3_t velocity;

	// reflect the velocity on the trace plane
	const int hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	const float dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	// cut the velocity to keep from bouncing forever
	VectorScale(ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta);

	if (ent->s.weapon == WP_DET_PACK && ent->s.eType == ET_GENERAL && ent->physicsObject)
	{
		//detpacks only
		if (ent->touch)
		{
			ent->touch(ent, &g_entities[trace->entityNum], trace);
			return;
		}
	}

	// check for stop
	if (trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 || trace->startsolid)
	{
		trace->endpos[2] += 1.0; // make sure it is off ground
		SnapVector(trace->endpos);
		G_SetOrigin(ent, trace->endpos);
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;

	if (ent->s.eType == ET_HOLOCRON ||
		ent->s.shouldtarget && ent->s.eType == ET_GENERAL && ent->physicsObject)
	{
		//holocrons and sentry guns
		if (ent->touch)
		{
			ent->touch(ent, &g_entities[trace->entityNum], trace);
		}
	}
}

/*
================
G_RunItem

================
*/

static void G_RemoveWeaponEffect(gentity_t* ent)
{
	vec3_t effectPos, defaultDir;

	VectorSet(defaultDir, 0, 0, 1);

	if (ent->TimeOfWeaponDrop > level.time)
	{
		return;
	}
	VectorCopy(ent->r.currentOrigin, effectPos);
	G_PlayEffectID(G_EffectIndex("weapon/remove"), effectPos, defaultDir);

	if (level.time >= 1000)
	{
		G_FreeEntity(ent);
	}
}

void G_RunItem(gentity_t* ent)
{
	vec3_t origin;
	trace_t tr;
	int mask;

	// if groundentity has been set to ENTITYNUM_NONE, it may have been pushed off an edge
	if (ent->s.groundEntityNum == ENTITYNUM_NONE)
	{
		if (ent->s.pos.trType != TR_GRAVITY)
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if (ent->s.pos.trType == TR_STATIONARY)
	{
		// check think function
		G_RunThink(ent);

		if (ent->flags & FL_DROPPED_ITEM && g_remove_unused_weapons.integer
			&& ent->item
			&& ent->item->giType == IT_WEAPON
			&& ent->item->giType != IT_AMMO
			&& ent->item->giTag != WP_SABER)
		{
			//a dropped weapon item,
			G_RemoveWeaponEffect(ent);
		}
		return;
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// trace a line from the previous position to the current position
	if (ent->clipmask)
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY; //MASK_SOLID;
	}
	trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, ent->r.ownerNum, mask, qfalse, 0, 0);

	VectorCopy(tr.endpos, ent->r.currentOrigin);

	if (tr.startsolid)
	{
		tr.fraction = 0;
	}

	trap->LinkEntity((sharedEntity_t*)ent); // FIXME: avoid this for stationary?

	// check think function
	G_RunThink(ent);

	if (tr.fraction == 1)
	{
		return;
	}

	// if it is in a nodrop volume, remove it
	const int contents = trap->PointContents(ent->r.currentOrigin, -1);
	if (contents & CONTENTS_NODROP)
	{
		if (ent->item && ent->item->giType == IT_TEAM)
		{
			Team_FreeEntity(ent);
		}
		else if (ent->genericValue15 == HI_SENTRY_GUN)
		{
			turret_free(ent);
		}
		else
		{
			G_FreeEntity(ent);
		}
		return;
	}

	G_BounceItem(ent, &tr);
}

void IT_LoadWeatherParms(void)
{
	char serverinfo[MAX_INFO_STRING] = { 0 };
	char sje_mapname[128] = { 0 };
	vmCvar_t mapname;// getting mapname

	Q_strncpyz(sje_mapname, Info_ValueForKey(serverinfo, "mapname"), sizeof sje_mapname);
	strcpy(level.sjemapname, sje_mapname);

	if (com_rend2.integer == 1 &&
		(Q_stricmp(sje_mapname, "yavin1") == 0)
		|| (Q_stricmp(sje_mapname, "demo") == 0)
		|| (Q_stricmp(sje_mapname, "01nar") == 0)
		|| (Q_stricmp(sje_mapname, "md2_bd_ch") == 0)
		|| (Q_stricmp(sje_mapname, "md_sn_intro_jedi") == 0)
		|| (Q_stricmp(sje_mapname, "md_ch_battledroids") == 0)
		|| (Q_stricmp(sje_mapname, "md_ep4_intro") == 0)
		|| (Q_stricmp(sje_mapname, "secbase") == 0)
		|| (Q_stricmp(sje_mapname, "level0") == 0)
		|| (Q_stricmp(sje_mapname, "JKG_ImperialBase") == 0)
		|| (Q_stricmp(sje_mapname, "kejim_post") == 0))
	{
		return;
	}

	if (g_AllowWeather.integer != 1)
	{
		return;
	}

	trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	trap->SendConsoleCommand(EXEC_INSERT, va("execq Weather/%s", mapname.string, mapname.string, mapname.string));
}