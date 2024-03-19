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

#include "../cgame/cg_local.h"
#include "g_functions.h"

#include "b_local.h"

constexpr auto AMMO_POD_HEALTH = 1;
constexpr auto TURN_OFF = 0x00000100;

constexpr auto VELOCITY_DECAY = 0.25;
constexpr auto MAX_DISTANCE = 256;
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )
constexpr auto MIN_DISTANCE = 24;
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

extern gitem_t* FindItemForAmmo(ammo_t ammo);

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_DROPPINGDOWN,
	LSTATE_DOWN,
	LSTATE_RISINGUP,
};

gentity_t* CreateMissile(vec3_t org, vec3_t dir, float vel, int life, gentity_t* owner, qboolean alt_fire = qfalse);

void NPC_Mark2_Precache()
{
	G_SoundIndex("sound/chars/mark2/misc/mark2_explo"); // blows up on death
	G_SoundIndex("sound/chars/mark2/misc/mark2_pain");
	G_SoundIndex("sound/chars/mark2/misc/mark2_fire");
	G_SoundIndex("sound/chars/mark2/misc/mark2_move_lp");

	G_EffectIndex("explosions/droidexplosion1");
	G_EffectIndex("env/med_explode2");
	G_EffectIndex("blaster/smoke_bolton");
	G_EffectIndex("bryar_old/muzzle_flash");

	register_item(FindItemForWeapon(WP_BRYAR_PISTOL));
	register_item(FindItemForAmmo(AMMO_METAL_BOLTS));
	register_item(FindItemForAmmo(AMMO_POWERCELL));
	register_item(FindItemForAmmo(AMMO_BLASTER));
}

/*
-------------------------
NPC_Mark2_Part_Explode
-------------------------
*/
static void NPC_Mark2_Part_Explode(gentity_t* self, const int bolt)
{
	if (bolt >= 0)
	{
		mdxaBone_t boltMatrix;
		vec3_t org, dir;

		gi.G2API_GetBoltMatrix(self->ghoul2, self->playerModel,
			bolt,
			&boltMatrix, self->currentAngles, self->currentOrigin, cg.time ? cg.time : level.time,
			nullptr, self->s.modelScale);

		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org);
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Y, dir);

		G_PlayEffect("env/med_explode2", org, dir);
		G_PlayEffect(G_EffectIndex("blaster/smoke_bolton"), self->playerModel, bolt, self->s.number, org);
	}

	self->count++; // Count of pods blown off
}

/*
-------------------------
NPC_Mark2_Pain
- look at what was hit and see if it should be removed from the model.
-------------------------
*/
void NPC_Mark2_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* other, const vec3_t point, const int damage,
	const int mod,
	const int hit_loc)
{
	NPC_Pain(self, inflictor, other, point, damage, mod);

	for (int i = 0; i < 3; i++)
	{
		if (hit_loc == HL_GENERIC1 + i && self->locationDamage[HL_GENERIC1 + i] > AMMO_POD_HEALTH) // Blow it up?
		{
			if (self->locationDamage[hit_loc] >= AMMO_POD_HEALTH)
			{
				const int new_bolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], va("torso_canister%d", i + 1));
				if (new_bolt != -1)
				{
					NPC_Mark2_Part_Explode(self, new_bolt);
				}
				gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], va("torso_canister%d", i + 1), TURN_OFF);
				break;
			}
		}
	}

	G_Sound(self, G_SoundIndex("sound/chars/mark2/misc/mark2_pain"));

	// If any pods were blown off, kill him
	if (self->count > 0)
	{
		G_Damage(self, nullptr, nullptr, nullptr, nullptr, self->health, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
	}
}

/*
-------------------------
Mark2_Hunt
-------------------------
*/
static void Mark2_Hunt()
{
	if (NPCInfo->goalEntity == nullptr)
	{
		NPCInfo->goalEntity = NPC->enemy;
	}

	// Turn toward him before moving towards him.
	NPC_FaceEnemy(qtrue);

	NPCInfo->combatMove = qtrue;
	NPC_MoveToGoal(qtrue);
}

/*
-------------------------
Mark2_FireBlaster
-------------------------
*/
static void Mark2_FireBlaster()
{
	vec3_t muzzle1;
	static vec3_t forward, vright, up;
	mdxaBone_t boltMatrix;

	gi.G2API_GetBoltMatrix(NPC->ghoul2, NPC->playerModel,
		NPC->genericBolt1,
		&boltMatrix, NPC->currentAngles, NPC->currentOrigin, cg.time ? cg.time : level.time,
		nullptr, NPC->s.modelScale);

	gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, muzzle1);

	if (NPC->health)
	{
		vec3_t angle_to_enemy1;
		vec3_t delta1;
		vec3_t enemy_org1;
		CalcEntitySpot(NPC->enemy, SPOT_HEAD, enemy_org1);
		VectorSubtract(enemy_org1, muzzle1, delta1);
		vectoangles(delta1, angle_to_enemy1);
		AngleVectors(angle_to_enemy1, forward, vright, up);
	}
	else
	{
		AngleVectors(NPC->currentAngles, forward, vright, up);
	}

	G_PlayEffect("bryar_old/muzzle_flash", muzzle1, forward);

	G_Sound(NPC, G_SoundIndex("sound/chars/mark2/misc/mark2_fire"));

	gentity_t* missile = CreateMissile(muzzle1, forward, 1600, 10000, NPC);

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = 1;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT;
}

/*
-------------------------
Mark2_BlasterAttack
-------------------------
*/
static void Mark2_BlasterAttack(const qboolean advance)
{
	if (TIMER_Done(NPC, "attackDelay")) // Attack?
	{
		if (NPCInfo->localState == LSTATE_NONE) // He's up so shoot less often.
		{
			TIMER_Set(NPC, "attackDelay", Q_irand(500, 2000));
		}
		else
		{
			TIMER_Set(NPC, "attackDelay", Q_irand(100, 500));
		}
		Mark2_FireBlaster();
		return;
	}
	if (advance)
	{
		Mark2_Hunt();
	}
}

/*
-------------------------
Mark2_AttackDecision
-------------------------
*/
static void Mark2_AttackDecision()
{
	NPC_FaceEnemy(qtrue);

	const float distance = static_cast<int>(DistanceHorizontalSquared(NPC->currentOrigin, NPC->enemy->currentOrigin));
	const qboolean visible = NPC_ClearLOS(NPC->enemy);
	const auto advance = static_cast<qboolean>(distance > MIN_DISTANCE_SQR);

	// He's been ordered to get up
	if (NPCInfo->localState == LSTATE_RISINGUP)
	{
		NPC->flags &= ~FL_SHIELDED;
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_RUN1START, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
		if (NPC->client->ps.legsAnimTimer == 0 &&
			NPC->client->ps.torsoAnim == BOTH_RUN1START)
		{
			NPCInfo->localState = LSTATE_NONE; // He's up again.
		}
		return;
	}

	// If we cannot see our target, move to see it
	if (!visible || !NPC_FaceEnemy(qtrue))
	{
		// If he's going down or is down, make him get up
		if (NPCInfo->localState == LSTATE_DOWN || NPCInfo->localState == LSTATE_DROPPINGDOWN)
		{
			if (TIMER_Done(NPC, "downTime"))
				// Down being down?? (The delay is so he doesn't pop up and down when the player goes in and out of range)
			{
				NPCInfo->localState = LSTATE_RISINGUP;
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
				TIMER_Set(NPC, "runTime", Q_irand(3000, 8000));
				// So he runs for a while before testing to see if he should drop down.
			}
		}
		else
		{
			Mark2_Hunt();
		}
		return;
	}

	// He's down but he could advance if he wants to.
	if (advance && TIMER_Done(NPC, "downTime") && NPCInfo->localState == LSTATE_DOWN)
	{
		NPCInfo->localState = LSTATE_RISINGUP;
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
		TIMER_Set(NPC, "runTime", Q_irand(3000, 8000));
		// So he runs for a while before testing to see if he should drop down.
	}

	NPC_FaceEnemy(qtrue);

	// Dropping down to shoot
	if (NPCInfo->localState == LSTATE_DROPPINGDOWN)
	{
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_RUN1STOP, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
		TIMER_Set(NPC, "downTime", Q_irand(3000, 9000));

		if (NPC->client->ps.legsAnimTimer == 0 && NPC->client->ps.torsoAnim == BOTH_RUN1STOP)
		{
			NPC->flags |= FL_SHIELDED;
			NPCInfo->localState = LSTATE_DOWN;
		}
	}
	// He's down and shooting
	else if (NPCInfo->localState == LSTATE_DOWN)
	{
		//		NPC->flags |= FL_SHIELDED;//only damagable by lightsabers and missiles

		Mark2_BlasterAttack(qfalse);
	}
	else if (TIMER_Done(NPC, "runTime")) // Lowering down to attack. But only if he's done running at you.
	{
		NPCInfo->localState = LSTATE_DROPPINGDOWN;
	}
	else if (advance)
	{
		// We can see enemy so shoot him if timer lets you.
		Mark2_BlasterAttack(advance);
	}
}

/*
-------------------------
Mark2_Patrol
-------------------------
*/
static void Mark2_Patrol()
{
	if (NPC_CheckPlayerTeamStealth())
	{
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}

	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		if (UpdateGoal())
		{
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal(qtrue);
			NPC_UpdateAngles(qtrue, qtrue);
		}

		//randomly talk
		if (TIMER_Done(NPC, "patrolNoise"))
		{
			TIMER_Set(NPC, "patrolNoise", Q_irand(2000, 4000));
		}
	}
}

/*
-------------------------
Mark2_Idle
-------------------------
*/
static void Mark2_Idle()
{
	NPC_BSIdle();
}

/*
-------------------------
NPC_BSMark2_Default
-------------------------
*/
void NPC_BSMark2_Default()
{
	if (NPC->enemy)
	{
		NPCInfo->goalEntity = NPC->enemy;
		Mark2_AttackDecision();
	}
	else if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
	{
		Mark2_Patrol();
	}
	else
	{
		Mark2_Idle();
	}
}