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

#include "b_local.h"
#include "g_navigator.h"
#include "g_functions.h"

extern void CG_DrawAlert(vec3_t origin, float rating);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean G_ExpandPointToBBox(vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask);
extern qboolean FlyingCreature(const gentity_t* ent);
extern void Saboteur_Cloak(gentity_t* self);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean NPC_IsGunner(const gentity_t* self);
extern void NPC_AngerSound();
extern void npc_check_speak(gentity_t* speaker_npc);

constexpr auto SPF_NO_HIDE = 2;

constexpr auto MAX_VIEW_DIST = 1024;
constexpr auto MAX_VIEW_SPEED = 250;
constexpr auto MAX_LIGHT_INTENSITY = 255;
constexpr auto MIN_LIGHT_THRESHOLD = 0.1;

constexpr auto DISTANCE_SCALE = 0.25f;
constexpr auto DISTANCE_THRESHOLD = 0.075f;
constexpr auto SPEED_SCALE = 0.25f;
constexpr auto FOV_SCALE = 0.5f;
constexpr auto LIGHT_SCALE = 0.25f;

constexpr auto REALIZE_THRESHOLD = 0.6f;
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

extern void NPC_Tusken_Taunt();
qboolean NPC_CheckPlayerTeamStealth();

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean faceEnemy;
static qboolean doMove;
static qboolean shoot;
static float enemyDist;

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

static void Sniper_ClearTimers(const gentity_t* ent)
{
	TIMER_Set(ent, "chatter", 0);
	TIMER_Set(ent, "duck", 0);
	TIMER_Set(ent, "stand", 0);
	TIMER_Set(ent, "shuffleTime", 0);
	TIMER_Set(ent, "sleepTime", 0);
	TIMER_Set(ent, "enemyLastVisible", 0);
	TIMER_Set(ent, "roamTime", 0);
	TIMER_Set(ent, "hideTime", 0);
	TIMER_Set(ent, "attackDelay", 0); //FIXME: Slant for difficulty levels
	TIMER_Set(ent, "stick", 0);
	TIMER_Set(ent, "scoutTime", 0);
	TIMER_Set(ent, "flee", 0);
	TIMER_Set(ent, "taunting", 0);
}

static void NPC_Sniper_PlayConfusionSound(gentity_t* self)
{
	//FIXME: make this a custom sound in sound set
	if (self->health > 0)
	{
		G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
	}
	//reset him to be totally unaware again
	TIMER_Set(self, "enemyLastVisible", 0);
	TIMER_Set(self, "flee", 0);
	self->NPC->squadState = SQUAD_IDLE;
	self->NPC->tempBehavior = BS_DEFAULT;
	G_ClearEnemy(self); //FIXME: or just self->enemy = NULL;?

	self->NPC->investigateCount = 0;
}

/*
-------------------------
NPC_ST_Pain
-------------------------
*/

static void NPC_Sniper_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* other, vec3_t point, const int damage,
	const int mod)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	if (self->client->NPC_class == CLASS_SABOTEUR)
	{
		Saboteur_Decloak(self);
	}
	TIMER_Set(self, "duck", -1);
	TIMER_Set(self, "stand", 2000);

	NPC_Pain(self, inflictor, other, point, damage, mod);

	if (self && self->health > 0 && !damage)
	{
		G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
	}
}

/*
-------------------------
ST_HoldPosition
-------------------------
*/

static void Sniper_HoldPosition()
{
	NPC_FreeCombatPoint(NPCInfo->combatPoint, qtrue);
	NPCInfo->goalEntity = nullptr;
}

/*
-------------------------
ST_Move
-------------------------
*/
static qboolean Sniper_Move()
{
	NPCInfo->combatMove = qtrue; //always doMove straight toward our goal

	const qboolean moved = NPC_MoveToGoal(qtrue);
	navInfo_t info;

	//Get the doMove info
	NAV_GetLastMove(info);

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if (info.flags & NIF_COLLISION)
	{
		if (info.blocker == NPC->enemy)
		{
			Sniper_HoldPosition();
		}
	}

	//If our doMove failed, then reset
	if (moved == qfalse)
	{
		//couldn't get to enemy
		if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES && NPCInfo->goalEntity && NPCInfo->goalEntity == NPC->enemy)
		{
			//we were running after enemy
			//Try to find a combat point that can hit the enemy
			int cp_flags = CP_CLEAR | CP_HAS_ROUTE;
			if (NPCInfo->scriptFlags & SCF_USE_CP_NEAREST)
			{
				cp_flags &= ~(CP_FLANK | CP_APPROACH_ENEMY | CP_CLOSEST);
				cp_flags |= CP_NEAREST;
			}
			int cp = NPC_FindCombatPoint(NPC->currentOrigin, NPC->currentOrigin, NPC->currentOrigin, cp_flags, 32);
			if (cp == -1 && !(NPCInfo->scriptFlags & SCF_USE_CP_NEAREST))
			{
				//okay, try one by the enemy
				cp = NPC_FindCombatPoint(NPC->currentOrigin, NPC->currentOrigin, NPC->enemy->currentOrigin,
					CP_CLEAR | CP_HAS_ROUTE | CP_HORZ_DIST_COLL, 32);
			}
			//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
			if (cp != -1)
			{
				//found a combat point that has a clear shot to enemy
				NPC_SetCombatPoint(cp);
				NPC_SetMoveGoal(NPC, level.combatPoints[cp].origin, 8, qtrue, cp);
				return moved;
			}
		}
		//just hang here
		Sniper_HoldPosition();
	}

	return moved;
}

/*
-------------------------
NPC_BSSniper_Patrol
-------------------------
*/

static void NPC_BSSniper_Patrol()
{
	//FIXME: pick up on bodies of dead buddies?
	NPC->count = 0;

	if (NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time)
	{
		//Look for any enemies
		if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			if (NPC_CheckPlayerTeamStealth())
			{
				NPC_AngerSound();
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}

		if (!(NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
		{
			//Is there danger nearby
			const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_SUSPICIOUS);
			if (NPC_CheckForDanger(alert_event))
			{
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
			//check for other alert events
			//There is an event to look at
			if (alert_event >= 0) //&& level.alertEvents[alert_event].ID != NPCInfo->lastAlertID )
			{
				//NPCInfo->lastAlertID = level.alertEvents[alert_event].ID;
				if (level.alertEvents[alert_event].level == AEL_DISCOVERED)
				{
					if (level.alertEvents[alert_event].owner &&
						level.alertEvents[alert_event].owner->client &&
						level.alertEvents[alert_event].owner->health >= 0 &&
						level.alertEvents[alert_event].owner->client->playerTeam == NPC->client->enemyTeam)
					{
						//an enemy
						G_SetEnemy(NPC, level.alertEvents[alert_event].owner);
						//NPCInfo->enemyLastSeenTime = level.time;
						TIMER_Set(NPC, "attackDelay",
							Q_irand((6 - NPCInfo->stats.aim) * 100, (6 - NPCInfo->stats.aim) * 500));
					}
				}
				else
				{
					//FIXME: get more suspicious over time?
					//Save the position for movement (if necessary)
					//FIXME: sound?
					VectorCopy(level.alertEvents[alert_event].position, NPCInfo->investigateGoal);
					NPCInfo->investigateDebounceTime = level.time + Q_irand(500, 1000);
					if (level.alertEvents[alert_event].level == AEL_SUSPICIOUS)
					{
						//suspicious looks longer
						NPCInfo->investigateDebounceTime += Q_irand(500, 2500);
					}
				}
			}

			if (NPCInfo->investigateDebounceTime > level.time)
			{
				//FIXME: walk over to it, maybe?  Not if not chase enemies flag
				//NOTE: stops walking or doing anything else below
				vec3_t dir, angles;

				VectorSubtract(NPCInfo->investigateGoal, NPC->client->renderInfo.eyePoint, dir);
				vectoangles(dir, angles);

				const float o_yaw = NPCInfo->desiredYaw;
				const float o_pitch = NPCInfo->desiredPitch;
				NPCInfo->desiredYaw = angles[YAW];
				NPCInfo->desiredPitch = angles[PITCH];

				NPC_UpdateAngles(qtrue, qtrue);

				NPCInfo->desiredYaw = o_yaw;
				NPCInfo->desiredPitch = o_pitch;
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}

	NPC_UpdateAngles(qtrue, qtrue);
}

/*
-------------------------
NPC_BSSniper_Idle
-------------------------
*/
/*
void NPC_BSSniper_Idle( void )
{
	//reset our shotcount
	NPC->count = 0;

	//FIXME: check for other alert events?

	//Is there danger nearby?
	if ( NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue ) ) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	TIMER_Set( NPC, "roamTime", 2000 + Q_irand( 1000, 2000 ) );

	NPC_UpdateAngles( qtrue, qtrue );
}
*/
/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void Sniper_CheckMoveState()
{
	//See if we're a scout
	if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)) //NPCInfo->behaviorState == BS_STAND_AND_SHOOT )
	{
		if (NPCInfo->goalEntity == NPC->enemy)
		{
			doMove = qfalse;
			return;
		}
	}
	//See if we're running away
	else if (NPCInfo->squadState == SQUAD_RETREAT)
	{
		if (TIMER_Done(NPC, "flee"))
		{
			NPCInfo->squadState = SQUAD_IDLE;
		}
		else
		{
			faceEnemy = qfalse;
		}
	}
	else if (NPCInfo->squadState == SQUAD_IDLE)
	{
		if (!NPCInfo->goalEntity)
		{
			doMove = qfalse;
			return;
		}
	}

	if (!TIMER_Done(NPC, "taunting"))
	{
		//no doMove while taunting
		doMove = qfalse;
		return;
	}

	//See if we're moving towards a goal, not the enemy
	if (NPCInfo->goalEntity != NPC->enemy && NPCInfo->goalEntity != nullptr)
	{
		//Did we make it?
		if (STEER::Reached(NPC, NPCInfo->goalEntity, 16, !!FlyingCreature(NPC)) ||
			NPCInfo->squadState == SQUAD_SCOUT && enemyLOS && enemyDist <= 10000)
		{
			//int	newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch (NPCInfo->squadState)
			{
			case SQUAD_RETREAT: //was running away
				if (NPC->client->NPC_class == CLASS_SABOTEUR)
				{
					Saboteur_Cloak(NPC);
				}
				TIMER_Set(NPC, "duck", (NPC->max_health - NPC->health) * 100);
				TIMER_Set(NPC, "hideTime", Q_irand(3000, 7000));
				//newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION: //was heading for a combat point
				TIMER_Set(NPC, "hideTime", Q_irand(2000, 4000));
				break;
			case SQUAD_SCOUT: //was running after player
				break;
			default:
				break;
			}
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set(NPC, "attackDelay", Q_irand((6 - NPCInfo->stats.aim) * 50, (6 - NPCInfo->stats.aim) * 100));
			//FIXME: Slant for difficulty levels, too?
			//don't do something else just yet
			TIMER_Set(NPC, "roamTime", Q_irand(1000, 4000));
			//stop fleeing
			if (NPCInfo->squadState == SQUAD_RETREAT)
			{
				TIMER_Set(NPC, "flee", -level.time);
				NPCInfo->squadState = SQUAD_IDLE;
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set(NPC, "roamTime", Q_irand(4000, 8000));
	}
}

static void Sniper_ResolveBlockedShot()
{
	if (TIMER_Done(NPC, "duck"))
	{
		//we're not ducking
		if (TIMER_Done(NPC, "roamTime"))
		{
			//not roaming
			//FIXME: try to find another spot from which to hit the enemy
			if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES && (!NPCInfo->goalEntity || NPCInfo->goalEntity == NPC->enemy))
			{
				//we were running after enemy
				//Try to find a combat point that can hit the enemy
				int cp_flags = CP_CLEAR | CP_HAS_ROUTE;
				if (NPCInfo->scriptFlags & SCF_USE_CP_NEAREST)
				{
					cp_flags &= ~(CP_FLANK | CP_APPROACH_ENEMY | CP_CLOSEST);
					cp_flags |= CP_NEAREST;
				}
				int cp = NPC_FindCombatPoint(NPC->currentOrigin, NPC->currentOrigin, NPC->currentOrigin, cp_flags, 32);
				if (cp == -1 && !(NPCInfo->scriptFlags & SCF_USE_CP_NEAREST))
				{
					//okay, try one by the enemy
					cp = NPC_FindCombatPoint(NPC->currentOrigin, NPC->currentOrigin, NPC->enemy->currentOrigin,
						CP_CLEAR | CP_HAS_ROUTE | CP_HORZ_DIST_COLL, 32);
				}
				//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
				if (cp != -1)
				{
					//found a combat point that has a clear shot to enemy
					NPC_SetCombatPoint(cp);
					NPC_SetMoveGoal(NPC, level.combatPoints[cp].origin, 8, qtrue, cp);
					TIMER_Set(NPC, "duck", -1);
					if (NPC->client->NPC_class == CLASS_SABOTEUR)
					{
						Saboteur_Decloak(NPC);
					}
					TIMER_Set(NPC, "attackDelay", Q_irand(1000, 3000));
				}
			}
		}
	}
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void Sniper_CheckFireState()
{
	if (enemyCS)
	{
		//if have a clear shot, always try
		return;
	}

	if (NPCInfo->squadState == SQUAD_RETREAT || NPCInfo->squadState == SQUAD_TRANSITION || NPCInfo->squadState ==
		SQUAD_SCOUT)
	{
		//runners never try to fire at the last pos
		return;
	}

	if (!VectorCompare(NPC->client->ps.velocity, vec3_origin))
	{
		//if moving at all, don't do this
		return;
	}

	if (!TIMER_Done(NPC, "taunting"))
	{
		//no shoot while taunting
		return;
	}

	//continue to fire on their last position
	if (!Q_irand(0, 1)
		&& NPCInfo->enemyLastSeenTime
		&& level.time - NPCInfo->enemyLastSeenTime < (5 - NPCInfo->stats.aim) * 1000) //FIXME: incorporate skill too?
	{
		if (!VectorCompare(vec3_origin, NPCInfo->enemyLastSeenLocation))
		{
			//Fire on the last known position
			vec3_t muzzle, dir, angles;

			CalcEntitySpot(NPC, SPOT_WEAPON, muzzle);
			VectorSubtract(NPCInfo->enemyLastSeenLocation, muzzle, dir);

			VectorNormalize(dir);

			vectoangles(dir, angles);

			NPCInfo->desiredYaw = angles[YAW];
			NPCInfo->desiredPitch = angles[PITCH];

			shoot = qtrue;
			//faceEnemy = qfalse;
		}
		return;
	}
	if (level.time - NPCInfo->enemyLastSeenTime > 10000)
	{
		//next time we see him, we'll miss few times first
		NPC->count = 0;
	}
}

static qboolean Sniper_EvaluateShot(const int hit)
{
	if (!NPC->enemy)
	{
		return qfalse;
	}

	const gentity_t* hit_ent = &g_entities[hit];
	if (hit == NPC->enemy->s.number
		|| hit_ent && hit_ent->client && hit_ent->client->playerTeam == NPC->client->enemyTeam
		|| hit_ent && hit_ent->takedamage && (hit_ent->svFlags & SVF_GLASS_BRUSH || hit_ent->health < 40 || NPC->s.weapon ==
			WP_EMPLACED_GUN)
		|| hit_ent && hit_ent->svFlags & SVF_GLASS_BRUSH)
	{
		//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

static void Sniper_FaceEnemy()
{
	if (NPC->enemy)
	{
		vec3_t muzzle, target, angles, forward, right, up;
		//Get the positions
		AngleVectors(NPC->client->ps.viewangles, forward, right, up);
		calcmuzzlePoint(NPC, forward, muzzle, 0);
		//CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		CalcEntitySpot(NPC->enemy, SPOT_ORIGIN, target);

		if (enemyDist > 65536 && NPCInfo->stats.aim < 5) //is 256 squared, was 16384 (128*128)
		{
			if (NPC->count < 5 - NPCInfo->stats.aim)
			{
				//miss a few times first
				if (shoot && TIMER_Done(NPC, "attackDelay") && level.time >= NPCInfo->shotTime)
				{
					//ready to fire again
					qboolean aim_error = qfalse;
					qboolean hit = qtrue;
					int try_miss_count = 0;
					trace_t trace;

					GetAnglesForDirection(muzzle, target, angles);
					AngleVectors(angles, forward, right, up);

					while (hit && try_miss_count < 10)
					{
						try_miss_count++;
						if (!Q_irand(0, 1))
						{
							aim_error = qtrue;
							if (!Q_irand(0, 1))
							{
								VectorMA(target, NPC->enemy->maxs[2] * Q_flrand(1.5, 4), right, target);
							}
							else
							{
								VectorMA(target, NPC->enemy->mins[2] * Q_flrand(1.5, 4), right, target);
							}
						}
						if (!aim_error || !Q_irand(0, 1))
						{
							if (!Q_irand(0, 1))
							{
								VectorMA(target, NPC->enemy->maxs[2] * Q_flrand(1.5, 4), up, target);
							}
							else
							{
								VectorMA(target, NPC->enemy->mins[2] * Q_flrand(1.5, 4), up, target);
							}
						}
						gi.trace(&trace, muzzle, vec3_origin, vec3_origin, target, NPC->s.number, MASK_SHOT,
							static_cast<EG2_Collision>(0), 0);
						hit = Sniper_EvaluateShot(trace.entityNum);
					}
					NPC->count++;
				}
				else
				{
					if (!enemyLOS)
					{
						NPC_UpdateAngles(qtrue, qtrue);
						return;
					}
				}
			}
			else
			{
				//based on distance, aim value, difficulty and enemy movement, miss
				//FIXME: incorporate distance as a factor?
				int miss_factor = 8 - (NPCInfo->stats.aim + g_spskill->integer) * 3;
				if (miss_factor > ENEMY_POS_LAG_STEPS)
				{
					miss_factor = ENEMY_POS_LAG_STEPS;
				}
				else if (miss_factor < 0)
				{
					//???
					miss_factor = 0;
				}
				VectorCopy(NPCInfo->enemyLaggedPos[miss_factor], target);
			}
			GetAnglesForDirection(muzzle, target, angles);
		}
		else
		{
			target[2] += Q_flrand(0, NPC->enemy->maxs[2]);
			//CalcEntitySpot( NPC->enemy, SPOT_HEAD_LEAN, target );
			GetAnglesForDirection(muzzle, target, angles);
		}

		NPCInfo->desiredYaw = AngleNormalize360(angles[YAW]);
		NPCInfo->desiredPitch = AngleNormalize360(angles[PITCH]);
	}
	NPC_UpdateAngles(qtrue, qtrue);
}

static void Sniper_UpdateEnemyPos()
{
	for (int i = MAX_ENEMY_POS_LAG - ENEMY_POS_LAG_INTERVAL; i >= 0; i -= ENEMY_POS_LAG_INTERVAL)
	{
		const int index = i / ENEMY_POS_LAG_INTERVAL;
		if (!index)
		{
			CalcEntitySpot(NPC->enemy, SPOT_HEAD_LEAN, NPCInfo->enemyLaggedPos[index]);
			NPCInfo->enemyLaggedPos[index][2] -= Q_flrand(2, 16);
		}
		else
		{
			VectorCopy(NPCInfo->enemyLaggedPos[index - 1], NPCInfo->enemyLaggedPos[index]);
		}
	}
}

/*
-------------------------
NPC_BSSniper_Attack
-------------------------
*/

static void Sniper_StartHide()
{
	const int duck_time = Q_irand(2000, 5000);

	TIMER_Set(NPC, "duck", duck_time);
	if (NPC->client->NPC_class == CLASS_SABOTEUR)
	{
		Saboteur_Cloak(NPC);
	}
	TIMER_Set(NPC, "watch", 500);
	TIMER_Set(NPC, "attackDelay", duck_time + Q_irand(500, 2000));
}

static void NPC_BSSniper_Attack()
{
	//Don't do anything if we're hurt
	if (NPC->painDebounceTime > level.time)
	{
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if (NPC_CheckEnemyExt() == qfalse) //!NPC->enemy )//
	{
		NPC_BSSniper_Patrol(); //FIXME: or patrol?
		return;
	}

	//  now if this timer is done and the enemy is no longer in our line of sight, try to get a new enemy later
	if (TIMER_Done(NPC, "sje_check_enemy"))
	{
		TIMER_Set(NPC, "sje_check_enemy", Q_irand(2500, 5000));

		if (NPC->enemy && !NPC_ClearLOS(NPC->enemy) && NPC->health > 0)
		{
			// if enemy cant be seen, try getting one later
			if (NPC->client)
			{
				NPC->enemy = nullptr;
				NPC_BSSniper_Patrol();
				return;
			}
			if (NPC->client)
			{
				// guardians have a different way to find enemies. He tries to find the quest player and his allies
				for (int sje_it = 0; sje_it < level.maxclients; sje_it++)
				{
					gentity_t* allied_player = &g_entities[sje_it];

					if (allied_player && allied_player->client &&
						NPC_ClearLOS(allied_player))
					{
						// the quest player or one of his allies. If one of them is in line of sight, choose him as enemy
						NPC->enemy = allied_player;
					}
				}
			}
		}
	}

	if (TIMER_Done(NPC, "flee") && NPC_CheckForDanger(NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER)))
	{
		//going to run
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}

	if (!NPC->enemy)
	{
		//WTF?  somehow we lost our enemy?
		NPC_BSSniper_Patrol(); //FIXME: or patrol?
		return;
	}

	enemyLOS = enemyCS = qfalse;
	doMove = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	enemyDist = DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);
	if (enemyDist < 16384) //128 squared
	{
		//too close, so switch to primary fire
		if (NPC->client->ps.weapon == WP_DISRUPTOR
			|| NPC->client->ps.weapon == WP_TUSKEN_RIFLE)
		{
			//sniping... should be assumed
			if (NPCInfo->scriptFlags & SCF_altFire)
			{
				//use primary fire
				trace_t trace;
				gi.trace(&trace, NPC->enemy->currentOrigin, NPC->enemy->mins, NPC->enemy->maxs, NPC->currentOrigin,
					NPC->enemy->s.number, NPC->enemy->clipmask, static_cast<EG2_Collision>(0), 0);
				if (!trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == NPC->s.number))
				{
					//he can get right to me
					NPCInfo->scriptFlags &= ~SCF_altFire;
					//reset fire-timing variables
					NPC_ChangeWeapon(NPC->client->ps.weapon);
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
			//FIXME: switch back if he gets far away again?
		}
	}
	else if (enemyDist > 65536) //256 squared
	{
		if (NPC->client->ps.weapon == WP_DISRUPTOR
			|| NPC->client->ps.weapon == WP_TUSKEN_RIFLE)
		{
			//sniping... should be assumed
			if (!(NPCInfo->scriptFlags & SCF_altFire))
			{
				//use primary fire
				NPCInfo->scriptFlags |= SCF_altFire;
				//reset fire-timing variables
				NPC_ChangeWeapon(NPC->client->ps.weapon);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	Sniper_UpdateEnemyPos();
	//can we see our target?
	if (NPC_ClearLOS(NPC->enemy))
		//|| (NPCInfo->stats.aim >= 5 && gi.inPVS( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin )) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation);
		enemyLOS = qtrue;
		const float max_shoot_dist = NPC_MaxDistSquaredForWeapon();
		if (enemyDist < max_shoot_dist)
		{
			vec3_t fwd, right, up, muzzle, end;
			trace_t tr;
			AngleVectors(NPC->client->ps.viewangles, fwd, right, up);
			calcmuzzlePoint(NPC, fwd, muzzle, 0);
			VectorMA(muzzle, 8192, fwd, end);
			gi.trace(&tr, muzzle, nullptr, nullptr, end, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 0);

			const int hit = tr.entityNum;
			//can we shoot our target?
			if (Sniper_EvaluateShot(hit))
			{
				enemyCS = qtrue;
			}
		}
	}

	if (enemyLOS)
	{
		//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy = qtrue;
	}

	if (!TIMER_Done(NPC, "taunting"))
	{
		doMove = qfalse;
		shoot = qfalse;
	}
	else if (enemyCS)
	{
		shoot = qtrue;
	}
	else if (level.time - NPCInfo->enemyLastSeenTime > 3000)
	{
		//Hmm, have to get around this bastard... FIXME: this NPCInfo->enemyLastSeenTime builds up when ducked seems to make them want to run when they uncrouch
		Sniper_ResolveBlockedShot();
	}
	else if (NPC->client->ps.weapon == WP_TUSKEN_RIFLE && !Q_irand(0, 100))
	{
		//start a taunt
		NPC_Tusken_Taunt();
		TIMER_Set(NPC, "duck", -1);
		doMove = qfalse;
	}

	//Check for movement to take care of
	Sniper_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	Sniper_CheckFireState();

	if (doMove)
	{
		//doMove toward goal
		if (NPCInfo->goalEntity) //&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist > 10000 ) )//100 squared
		{
			doMove = Sniper_Move();
		}
		else
		{
			doMove = qfalse;
		}
	}

	if (!doMove)
	{
		if (!TIMER_Done(NPC, "duck"))
		{
			if (TIMER_Done(NPC, "watch"))
			{
				//not while watching
				ucmd.upmove = -127;
				if (NPC->client->NPC_class == CLASS_SABOTEUR)
				{
					Saboteur_Cloak(NPC);
				}
			}
		}
		//FIXME: what about leaning?
		//FIXME: also, when stop ducking, start looking, if enemy can see me, chance of ducking back down again
	}
	else
	{
		//stop ducking!
		TIMER_Set(NPC, "duck", -1);
		if (NPC->client->NPC_class == CLASS_SABOTEUR)
		{
			Saboteur_Decloak(NPC);
		}
	}

	if (TIMER_Done(NPC, "duck")
		&& TIMER_Done(NPC, "watch")
		&& TIMER_Get(NPC, "attackDelay") - level.time > 1000
		&& NPC->attackDebounceTime < level.time)
	{
		if (enemyLOS && NPCInfo->scriptFlags & SCF_altFire)
		{
			if (NPC->fly_sound_debounce_time < level.time)
			{
				NPC->fly_sound_debounce_time = level.time + 2000;
			}
		}
	}

	if (!faceEnemy)
	{
		//we want to face in the dir we're running
		if (doMove)
		{
			//don't run away and shoot
			NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
			NPCInfo->desiredPitch = 0;
			shoot = qfalse;
		}
		NPC_UpdateAngles(qtrue, qtrue);
	}
	else // if ( faceEnemy )
	{
		//face the enemy
		Sniper_FaceEnemy();
	}

	if (NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		shoot = qfalse;
	}

	//FIXME: don't shoot right away!
	if (shoot)
	{
		//try to shoot if it's time
		if (TIMER_Done(NPC, "attackDelay"))
		{
			WeaponThink();
			if (ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK))
			{
				G_SoundOnEnt(NPC, CHAN_WEAPON, "sound/null.wav");
			}

			//took a shot, now hide
			if (!(NPC->spawnflags & SPF_NO_HIDE) && !Q_irand(0, 1))
			{
				//FIXME: do this if in combat point and combat point has duck-type cover... also handle lean-type cover
				Sniper_StartHide();
			}
			else
			{
				TIMER_Set(NPC, "attackDelay", NPCInfo->shotTime - level.time);
			}
		}
	}
}

void NPC_BSSniper_Default()
{
	if (!NPC->enemy)
	{
		//don't have an enemy, look for one
		NPC_BSSniper_Patrol();
	}
	else //if ( NPC->enemy )
	{
		//have an enemy
		NPC_BSSniper_Attack();

		npc_check_speak(NPC);
	}
}