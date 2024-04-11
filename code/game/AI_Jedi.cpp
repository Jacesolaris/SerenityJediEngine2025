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
#include "anims.h"
#include "wp_saber.h"
#include "../qcommon/tri_coll_test.h"
#include "g_navigator.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

//Externs
extern qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy);
extern void CG_DrawAlert(vec3_t origin, float rating);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern void G_StartMatrixEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void G_StartStasisEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void ForceJump(gentity_t* self, const usercmd_t* ucmd);
extern void NPC_ClearLookTarget(const gentity_t* self);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clear_time);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean G_ExpandPointToBBox(vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask);
extern void PM_AddFatigue(playerState_t* ps, int fatigue);
extern gitem_t* FindItemForAmmo(ammo_t ammo);
extern void ForceLightning(gentity_t* self);
extern void ForceHeal(gentity_t* self);
extern void ForceRage(gentity_t* self);
extern void ForceProtect(gentity_t* self);
extern void ForceAbsorb(gentity_t* self);
extern qboolean ForceDrain2(gentity_t* self);
extern void ForceLightningStrike(gentity_t* self);
extern int WP_MissileBlockForBlock(int saber_block);
extern qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t force_power, int override_amt);
extern qboolean WP_ForcePowerAvailable(const gentity_t* self, forcePowers_t force_power, int override_amt);
extern void WP_ForcePowerStop(gentity_t* self, forcePowers_t force_power);
extern void WP_KnockdownTurret(gentity_t* pas);
extern void WP_DeactivateSaber(const gentity_t* self, qboolean clear_length = qfalse);
extern int PM_AnimLength(int index, animNumber_t anim);
extern qboolean PM_SaberInStart(int move);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInParry(int move);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_SaberInDeflect(int move);
extern qboolean PM_SpinningSaberAnim(int anim);
extern qboolean PM_FlippingAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_InRoll(const playerState_t* ps);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InSpecialJump(int anim);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern qboolean PM_InOnGroundAnim(playerState_t* ps);
extern qboolean PM_DodgeAnim(int anim);
extern qboolean PM_DodgeHoldAnim(int anim);
extern qboolean PM_InAirKickingAnim(int anim);
extern qboolean PM_KickingAnim(int anim);
extern qboolean PM_StabDownAnim(int anim);
extern qboolean PM_SuperBreakLoseAnim(int anim);
extern qboolean PM_SaberInKata(saber_moveName_t saber_move);
extern qboolean PM_InRollIgnoreTimer(const playerState_t* ps);
extern qboolean PM_PainAnim(int anim);
extern qboolean G_CanKickEntity(const gentity_t* self, const gentity_t* target);
extern saber_moveName_t G_PickAutoKick(const gentity_t* self, const gentity_t* enemy, qboolean store_move);
extern saber_moveName_t g_pick_auto_multi_kick(gentity_t* self, qboolean allow_singles, qboolean store_move);
extern qboolean NAV_DirSafe(const gentity_t* self, vec3_t dir, float dist);
extern qboolean NAV_MoveDirSafe(const gentity_t* self, const usercmd_t* cmd, float distScale = 1.0f);
extern float NPC_EnemyRangeFromBolt(int boltIndex);
extern qboolean WP_SabersCheckLock2(gentity_t* attacker, gentity_t* defender, saberslock_mode_t lockMode);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);
extern qboolean G_EntIsBreakable(int entityNum, const gentity_t* breaker);
extern qboolean PM_LockedAnim(int anim);
extern qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask);
void NPC_CheckEvasion();
static void Jedi_Patrol();
extern cvar_t* d_slowmodeath;
extern cvar_t* d_slowmoaction;
extern cvar_t* g_saberNewControlScheme;
extern int parry_debounce[];
extern void force_stasis(gentity_t* self);
extern void ForceDestruction(gentity_t* self);
extern void ForceTelepathy(gentity_t* self);
extern void ForceFear(gentity_t* self);
qboolean Jedi_WaitingAmbush(const gentity_t* self);
qboolean Rosh_BeingHealed(const gentity_t* self);
static qboolean enemy_in_striking_range = qfalse;
static int jediSpeechDebounceTime[TEAM_NUM_TEAMS];
extern void Boba_ChangeWeapon(int wp);
static qboolean Jedi_SaberBlock();
static qboolean Jedi_AttackDecide(int enemy_dist);
extern void add_npc_block_point_bonus(const gentity_t* self);
extern qboolean NPC_IsAlive(const gentity_t* self, const gentity_t* npc);
extern void WP_DeactivateLightSaber(const gentity_t* self);
extern qboolean IsSurrendering(const gentity_t* self);
extern qboolean IsRespecting(const gentity_t* self);
extern qboolean IsCowering(const gentity_t* self);
extern qboolean IsAnimRequiresResponce(const gentity_t* self);
extern void ForceGrip(const gentity_t* self);
extern qboolean WP_AbsorbKick(gentity_t* hit_ent, const gentity_t* pusher, const vec3_t push_dir);
extern qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps);
extern cvar_t* g_DebugSaberCombat;
extern qboolean wp_saber_block_check_random(gentity_t* self, vec3_t hitloc);
extern void npc_check_speak(gentity_t* speaker_npc);

int in_field_of_vision(vec3_t viewangles, const float fov, vec3_t angles)
{
	for (int i = 0; i < 2; i++)
	{
		const float angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		float diff = angles[i] - angle;

		if (angles[i] > angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
		if (diff > 0)
		{
			if (diff > fov * 0.5)
			{
				return 0;
			}
		}
		else
		{
			if (diff < -fov * 0.5)
			{
				return 0;
			}
		}
	}
	return 1;
}

static qboolean npc_is_jedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_SITHLORD:
	case CLASS_DESANN:
	case CLASS_VADER:
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LUKE:
	case CLASS_MORGANKATARN:
	case CLASS_REBORN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_YODA:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

static qboolean npc_should_not_throw_saber(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_DESANN:
	case CLASS_TAVION:
	case CLASS_VADER:
	case CLASS_SITHLORD:
	case CLASS_KYLE:
	case CLASS_LUKE:
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

static qboolean npc_is_saber_master(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_DESANN:
	case CLASS_TAVION:
	case CLASS_VADER:
	case CLASS_LUKE:
	case CLASS_KYLE:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

static qboolean npc_force_master(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_SITHLORD:
	case CLASS_REBORN:
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean npc_is_light_jedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_LUKE:
	case CLASS_MONMOTHA:
	case CLASS_MORGANKATARN:
	case CLASS_YODA:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean npc_is_dark_jedi(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_ALORA:
	case CLASS_DESANN:
	case CLASS_REBORN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_VADER:
	case CLASS_SITHLORD:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

void npc_cultist_destroyer_precache()
{
	G_SoundIndex("sound/movers/objects/green_beam_lp2.wav");
	G_EffectIndex("force/destruction_exp");
}

void npc_shadow_trooper_precache()
{
	register_item(FindItemForAmmo(AMMO_FORCE));
	G_SoundIndex("sound/chars/shadowtrooper/cloak.wav");
	G_SoundIndex("sound/chars/shadowtrooper/decloak.wav");
}

void npc_rosh_dark_precache()
{
	G_EffectIndex("force/kothos_recharge.efx");
	G_EffectIndex("force/kothos_beam.efx");
}

void npc_sbd_precache()
{
	G_SoundIndex("sound/chars/SBD/SBDWalk2.mp3");
	G_SoundIndex("sound/chars/SBD/SBDRun.mp3");
	G_SoundIndex("sound/chars/SBD/SBDStep.mp3");
	G_SoundIndex("sound/chars/SBD/SBDStep1.mp3");
	G_SoundIndex("sound/chars/SBD/SBDStep2.mp3");
	G_SoundIndex("sound/chars/SBD/SBDStep3.wav");
}

void jedi_clear_timers(const gentity_t* ent)
{
	TIMER_Set(ent, "roamTime", 0);
	TIMER_Set(ent, "chatter", 0);
	TIMER_Set(ent, "strafeLeft", 0);
	TIMER_Set(ent, "strafeRight", 0);
	TIMER_Set(ent, "noStrafe", 0);
	TIMER_Set(ent, "walking", 0);
	TIMER_Set(ent, "taunting", 0);
	TIMER_Set(ent, "parryTime", 0);
	TIMER_Set(ent, "parryReCalcTime", 0);
	TIMER_Set(ent, "forceJumpChasing", 0);
	TIMER_Set(ent, "jumpChaseDebounce", 0);
	TIMER_Set(ent, "moveforward", 0);
	TIMER_Set(ent, "moveback", 0);
	TIMER_Set(ent, "movenone", 0);
	TIMER_Set(ent, "moveright", 0);
	TIMER_Set(ent, "moveleft", 0);
	TIMER_Set(ent, "movecenter", 0);
	TIMER_Set(ent, "saberLevelDebounce", 0);
	TIMER_Set(ent, "noRetreat", 0);
	TIMER_Set(ent, "holdLightning", 0);
	TIMER_Set(ent, "gripping", 0);
	TIMER_Set(ent, "draining", 0);
	TIMER_Set(ent, "noturn", 0);
	TIMER_Set(ent, "specialEvasion", 0);
	TIMER_Set(ent, "slapattackDelay", 0);
	TIMER_Set(ent, "smackTime", 0);
	TIMER_Set(ent, "blocking", 0);
	TIMER_Set(ent, "TalkTime", 0);
	TIMER_Set(ent, "regenerate", 0);
	TIMER_Set(ent, "DeactivateTime", 0);
	TIMER_Set(ent, "grasping", 0);
}

qboolean Jedi_CultistDestroyer(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_REBORN
		&& self->s.weapon == WP_MELEE
		&& Q_stricmp("cultist_destroyer", self->NPC_type) == 0)
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean Jedi_CultistForceUser(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_REBORN
		&& Q_stricmp("cultist_destroyer", self->NPC_type) == 0
		|| Q_stricmp("cultist_grip", self->NPC_type) == 0
		|| Q_stricmp("cultist_lightning", self->NPC_type) == 0
		|| Q_stricmp("cultist_drain", self->NPC_type) == 0)
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean Jedi_Trainer(const gentity_t* self)
{
	if (!self || !self->client)
	{
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_KYLE
		&& self->s.weapon == WP_SABER
		&& Q_stricmp("JediSaberTrainer", self->NPC_type) == 0)
	{
		return qtrue;
	}
	return qfalse;
}

void Jedi_PlayBlockedPushSound(const gentity_t* self)
{
	if (!self->s.number)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, EV_PUSHFAIL, 3000);
		}
	}
	else if (self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, EV_PUSHFAIL, 3000);
		}
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void Jedi_PlayDeflectSound(const gentity_t* self)
{
	if (!self->s.number)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 3000);
		}
	}
	else if (self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time)
	{
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 3000);
		}
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void NPC_Jedi_PlayConfusionSound(const gentity_t* self)
{
	if (self->health > 0)
	{
		if (self->client
			&& (self->client->NPC_class == CLASS_ALORA
				|| self->client->NPC_class == CLASS_TAVION
				|| self->client->NPC_class == CLASS_DESANN
				|| self->client->NPC_class == CLASS_SITHLORD
				|| self->client->NPC_class == CLASS_VADER))
		{
			G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		}
		else if (Q_irand(0, 1))
		{
			if (!npc_is_dark_jedi(self))
			{
				G_AddVoiceEvent(self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000);
			}
			else
			{
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 2000);
			}
		}
		else
		{
			if (!npc_is_dark_jedi(self))
			{
				G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
			}
			else
			{
				G_AddVoiceEvent(self, Q_irand(EV_GLOAT1, EV_GLOAT3), 2000);
			}
		}
	}
}

qboolean Jedi_StopKnockdown(gentity_t* self, const vec3_t push_dir)
{
	if (self->s.number < MAX_CLIENTS || !self->NPC)
	{
		//only NPCs
		return qfalse;
	}

	if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
	{
		//only force-users
		return qfalse;
	}

	if (self->client->moveType == MT_FLYSWIM)
	{
		//can't knock me down when I'm flying
		return qtrue;
	}

	if (self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
	{
		//bosses always get out of a knockdown
	}
	else if (Q_irand(0, RANK_CAPTAIN + 5) > self->NPC->rank)
	{
		//lower their rank, the more likely they are fall down
		return qfalse;
	}

	vec3_t p_dir, fwd, right;
	const vec3_t ang = { 0, self->currentAngles[YAW], 0 };
	const int strafe_time = Q_irand(1000, 2000);

	AngleVectors(ang, fwd, right, nullptr);
	VectorNormalize2(push_dir, p_dir);
	const float f_dot = DotProduct(p_dir, fwd);
	const float r_dot = DotProduct(p_dir, right);

	//flip or roll with it
	usercmd_t temp_Cmd{};
	if (f_dot >= 0.4f)
	{
		temp_Cmd.forwardmove = 127;
		TIMER_Set(self, "moveforward", strafe_time);
	}
	else if (f_dot <= -0.4f)
	{
		temp_Cmd.forwardmove = -127;
		TIMER_Set(self, "moveback", strafe_time);
	}
	else if (r_dot > 0)
	{
		temp_Cmd.rightmove = 127;
		TIMER_Set(self, "strafeRight", strafe_time);
		TIMER_Set(self, "strafeLeft", -1);
	}
	else
	{
		temp_Cmd.rightmove = -127;
		TIMER_Set(self, "strafeLeft", strafe_time);
		TIMER_Set(self, "strafeRight", -1);
	}
	G_AddEvent(self, EV_JUMP, 0);
	if (!Q_irand(0, 1))
	{
		//flip
		self->client->ps.forceJumpCharge = 280; //FIXME: calc this intelligently?
		ForceJump(self, &temp_Cmd);
	}
	else
	{
		//roll
		TIMER_Set(self, "duck", strafe_time);
	}
	self->painDebounceTime = 0; //so we do something

	return qtrue;
}

extern void Boba_FireDecide();
extern void RT_FireDecide();
extern void Boba_FlyStart(gentity_t* self);

//===============================================================================================
//TAVION BOSS
//===============================================================================================
void NPC_TavionScepter_Precache()
{
	G_EffectIndex("scepter/beam_warmup.efx");
	G_EffectIndex("scepter/beam.efx");
	G_EffectIndex("scepter/slam_warmup.efx");
	G_EffectIndex("scepter/slam.efx");
	G_EffectIndex("scepter/impact.efx");
	G_SoundIndex("sound/weapons/scepter/loop.wav");
	G_SoundIndex("sound/weapons/scepter/slam_warmup.wav");
	G_SoundIndex("sound/weapons/scepter/beam_warmup.wav");
}

void NPC_TavionSithSword_Precache()
{
	G_EffectIndex("scepter/recharge.efx");
	G_EffectIndex("scepter/invincibility.efx");
	G_EffectIndex("scepter/sword.efx");
	G_SoundIndex("sound/weapons/scepter/recharge.wav");
}

static void Tavion_ScepterDamage()
{
	if (!NPC->ghoul2.size()
		|| NPC->weaponModel[1] <= 0)
	{
		return;
	}

	if (NPC->genericBolt1 != -1)
	{
		const int curTime = cg.time ? cg.time : level.time;
		qboolean hit = qfalse;
		int last_hit = ENTITYNUM_NONE;
		for (int time = curTime - 25; time <= curTime + 25 && !hit; time += 25)
		{
			mdxaBone_t boltMatrix;
			vec3_t tip, dir, base;
			const vec3_t angles = { 0, NPC->currentAngles[YAW], 0 };
			trace_t trace;

			gi.G2API_GetBoltMatrix(NPC->ghoul2, NPC->weaponModel[1],
				NPC->genericBolt1,
				&boltMatrix, angles, NPC->currentOrigin, time,nullptr, NPC->s.modelScale);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, base);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_X, dir);
			VectorMA(base, 512, dir, tip);
			if (d_saberCombat->integer > 1 || g_DebugSaberCombat->integer)
			{
				G_DebugLine(base, tip, 1000, 0x000000ff);
			}
			gi.trace(&trace, base, vec3_origin, vec3_origin, tip, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10);
			if (trace.fraction < 1.0f)
			{
				//hit something
				gentity_t* traceEnt = &g_entities[trace.entityNum];

				//UGH
				G_PlayEffect(G_EffectIndex("scepter/impact.efx"), trace.endpos, trace.plane.normal);

				if (traceEnt->takedamage
					&& trace.entityNum != last_hit
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->
						NPC_class))
				{
					//smack
					const int dmg = Q_irand(10, 20) * (g_spskill->integer + 1); //NOTE: was 6-12
					G_Damage(traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_SABER);
					if (traceEnt->client)
					{
						if (!Q_irand(0, 2))
						{
							G_AddVoiceEvent(NPC, Q_irand(EV_CONFUSE1, EV_CONFUSE2), 10000);
						}
						else
						{
							G_AddVoiceEvent(NPC, EV_JDETECTED3, 10000);
						}
						g_throw(traceEnt, dir, Q_flrand(50, 80));
						if (traceEnt->health > 0 && !Q_irand(0, 2)) //FIXME: base on skill!
						{
							//do pain on enemy
							G_Knockdown(traceEnt, NPC, dir, 300, qtrue);
						}
					}
					hit = qtrue;
					last_hit = trace.entityNum;
				}
			}
		}
	}
}

static void Tavion_ScepterSlam()
{
	if (!NPC->ghoul2.size()
		|| NPC->weaponModel[1] <= 0)
	{
		return;
	}

	const int boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[1]], "*weapon");
	if (boltIndex != -1)
	{
		mdxaBone_t boltMatrix;
		vec3_t handle, bottom;
		const vec3_t angles = { 0, NPC->currentAngles[YAW], 0 };
		trace_t trace;
		gentity_t* radius_ents[128];
		constexpr float radius = 300.0f;
		constexpr float half_rad = radius / 2;
		int i;
		vec3_t mins{}, maxs{}, ent_dir;

		gi.G2API_GetBoltMatrix(NPC->ghoul2, NPC->weaponModel[1],
			boltIndex,
			&boltMatrix, angles, NPC->currentOrigin, cg.time ? cg.time : level.time,
			nullptr, NPC->s.modelScale);
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, handle);
		VectorCopy(handle, bottom);
		bottom[2] -= 128.0f;

		gi.trace(&trace, handle, vec3_origin, vec3_origin, bottom, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10);
		G_PlayEffect(G_EffectIndex("scepter/slam.efx"), trace.endpos, trace.plane.normal);

		//Setup the bbox to search in
		for (i = 0; i < 3; i++)
		{
			mins[i] = trace.endpos[i] - radius;
			maxs[i] = trace.endpos[i] + radius;
		}

		//Get the number of entities in a given space
		const int num_ents = gi.EntitiesInBox(mins, maxs, radius_ents, 128);

		for (i = 0; i < num_ents; i++)
		{
			if (!radius_ents[i]->inuse)
			{
				continue;
			}

			if (radius_ents[i]->flags & FL_NO_KNOCKBACK)
			{
				//don't throw them back
				continue;
			}

			if (radius_ents[i] == NPC)
			{
				//Skip myself
				continue;
			}

			if (radius_ents[i]->client == nullptr)
			{
				//must be a client
				if (G_EntIsBreakable(radius_ents[i]->s.number, NPC))
				{
					//damage breakables within range, but not as much
					G_Damage(radius_ents[i], NPC, NPC, vec3_origin, radius_ents[i]->currentOrigin, 100, 0,
						MOD_EXPLOSIVE_SPLASH);
				}
				continue;
			}

			if (radius_ents[i]->client->ps.eFlags & EF_HELD_BY_RANCOR
				|| radius_ents[i]->client->ps.eFlags & EF_HELD_BY_WAMPA)
			{
				//can't be one being held
				continue;
			}

			VectorSubtract(radius_ents[i]->currentOrigin, trace.endpos, ent_dir);
			const float dist = VectorNormalize(ent_dir);
			if (dist <= radius)
			{
				if (dist < half_rad)
				{
					//close enough to do damage, too
					G_Damage(radius_ents[i], NPC, NPC, vec3_origin, radius_ents[i]->currentOrigin, Q_irand(20, 30),
						DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE_SPLASH);
				}
				if (radius_ents[i]->client
					&& radius_ents[i]->client->NPC_class != CLASS_RANCOR
					&& radius_ents[i]->client->NPC_class != CLASS_ATST)
				{
					float throw_str;
					if (g_spskill->integer > 1)
					{
						throw_str = 10.0f + (radius - dist) / 2.0f;
						if (throw_str > 150.0f)
						{
							throw_str = 150.0f;
						}
					}
					else
					{
						throw_str = 10.0f + (radius - dist) / 4.0f;
						if (throw_str > 85.0f)
						{
							throw_str = 85.0f;
						}
					}
					ent_dir[2] += 0.1f;
					VectorNormalize(ent_dir);
					g_throw(radius_ents[i], ent_dir, throw_str);
					if (radius_ents[i]->health > 0)
					{
						if (dist < half_rad
							|| radius_ents[i]->client->ps.groundEntityNum != ENTITYNUM_NONE)
						{
							//within range of my fist or within ground-shaking range and not in the air
							G_Knockdown(radius_ents[i], NPC, vec3_origin, 500, qtrue);
						}
					}
				}
			}
		}
	}
}

static void Tavion_StartScepterBeam()
{
	G_PlayEffect(G_EffectIndex("scepter/beam_warmup.efx"), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number,NPC->currentOrigin, 0, qtrue);
	G_SoundOnEnt(NPC, CHAN_ITEM, "sound/weapons/scepter/beam_warmup.wav");
	NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
	NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_SCEPTER_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPC->client->ps.torsoAnimTimer += 200;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear(NPC->client->ps.velocity);
	VectorClear(NPC->client->ps.moveDir);
}

static void Tavion_StartScepterSlam()
{
	G_PlayEffect(G_EffectIndex("scepter/slam_warmup.efx"), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number,
		NPC->currentOrigin, 0, qtrue);
	G_SoundOnEnt(NPC, CHAN_ITEM, "sound/weapons/scepter/slam_warmup.wav");
	NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
	NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_TAVION_SCEPTERGROUND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear(NPC->client->ps.velocity);
	VectorClear(NPC->client->ps.moveDir);
	NPC->count = 0;
}

static void Tavion_SithSwordRecharge()
{
	if (NPC->client->ps.torsoAnim != BOTH_TAVION_SWORDPOWER
		&& NPC->count
		&& TIMER_Done(NPC, "rechargeDebounce")
		&& NPC->weaponModel[0] != -1)
	{
		NPC->s.loopSound = G_SoundIndex("sound/weapons/scepter/recharge.wav");
		const int boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[0]], "*weapon");
		NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_TAVION_SWORDPOWER, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_PlayEffect(G_EffectIndex("scepter/recharge.efx"), NPC->weaponModel[0], boltIndex, NPC->s.number,
			NPC->currentOrigin, NPC->client->ps.torsoAnimTimer, qtrue);
		NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
		NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
		NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		VectorClear(NPC->client->ps.velocity);
		VectorClear(NPC->client->ps.moveDir);
		NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->ps.torsoAnimTimer + 10000;
		G_PlayEffect(G_EffectIndex("scepter/invincibility.efx"), NPC->playerModel, 0, NPC->s.number, NPC->currentOrigin,
			NPC->client->ps.torsoAnimTimer + 10000, qfalse);
		TIMER_Set(NPC, "rechargeDebounce", NPC->client->ps.torsoAnimTimer + 10000 + Q_irand(10000, 20000));
		NPC->count--;
		//now you have a chance of killing her
		NPC->flags &= ~FL_UNDYING;
	}
}

//======================================================================================
//END TAVION BOSS
//======================================================================================

void player_Decloak(gentity_t* self)
{
	if (self && self->client)
	{
		self->flags &= ~FL_NOTARGET;
		if (self->client->ps.powerups[PW_CLOAKED])
		{
			//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav");
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

static qboolean Self_IsGunner(const gentity_t* self)
{
	switch (self->client->ps.weapon)
	{
	case WP_BLASTER_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_STUN_BATON:
	case WP_BRYAR_PISTOL:
	case WP_TUSKEN_RIFLE:
	case WP_SBD_PISTOL:
	case WP_WRIST_BLASTER:
		return qtrue;
	default:;
	}
	return qfalse;
}

void player_Cloak(gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.weapon == WP_SABER && !self->client->ps.SaberActive() && !self->client->ps.saberInFlight
			|| self->client->ps.weapon == WP_NONE
			|| self->client->ps.weapon == WP_MELEE
			|| Self_IsGunner(self))
		{
			self->flags |= FL_NOTARGET;
		}
		else if (self->client->ps.weapon == WP_SABER && (self->client->ps.SaberActive() || self->client->ps.
			saberInFlight))
		{
			self->flags &= ~FL_NOTARGET;
		}
		if (!self->client->ps.powerups[PW_CLOAKED])
		{
			//cloak
			self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav");
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.cloakFuel -= 15;
		}
	}
}

void Jedi_Cloak(gentity_t* self)
{
	if (self && self->client)
	{
		if (!self->client->ps.powerups[PW_CLOAKED])
		{
			//cloak
			self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav");

			if (self->s.clientNum >= MAX_CLIENTS && !self->client->ps.SaberActive())
			{
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

void Jedi_Decloak(gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.powerups[PW_CLOAKED])
		{
			//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: temp sound
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav");

			if (self->s.number < MAX_CLIENTS)
			{
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

static void Jedi_CheckCloak()
{
	if (NPC
		&& NPC->client
		&& NPC->client->NPC_class == CLASS_SHADOWTROOPER
		&& Q_stricmpn("shadowtrooper", NPC->NPC_type, 13) == 0)
	{
		if (NPC->client->ps.SaberActive() ||
			NPC->health <= 0 ||
			NPC->client->ps.saberInFlight ||
			NPC->client->ps.eFlags & EF_FORCE_GRIPPED ||
			NPC->client->ps.eFlags & EF_FORCE_GRABBED ||
			NPC->client->ps.eFlags & EF_FORCE_DRAINED ||
			NPC->painDebounceTime > level.time)
		{
			//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak(NPC);
		}
		else if (NPC->health > 0
			&& !NPC->client->ps.saberInFlight
			&& !(NPC->client->ps.eFlags & EF_FORCE_GRIPPED)
			&& !(NPC->client->ps.eFlags & EF_FORCE_DRAINED)
			&& !(NPC->client->ps.eFlags & EF_FORCE_GRABBED)
			&& NPC->painDebounceTime < level.time)
		{
			//still alive, have saber in hand, not taking pain and not being gripped
			if (!PM_SaberInAttack(NPC->client->ps.saber_move))
			{
				Jedi_Cloak(NPC);
			}
		}
	}
}

void Self_CheckCloak(gentity_t* self)
{
	if (self && self->client)
	{
		if (self->health <= 0 ||
			self->client->ps.saberInFlight ||
			self->client->ps.eFlags & EF_FORCE_GRIPPED ||
			self->client->ps.eFlags & EF_FORCE_DRAINED ||
			self->client->ps.eFlags & EF_FORCE_GRABBED ||
			self->painDebounceTime > level.time)
		{
			//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			player_Decloak(self);
		}
		else if (self->health > 0
			&& !self->client->ps.saberInFlight
			&& !(self->client->ps.eFlags & EF_FORCE_GRIPPED)
			&& !(self->client->ps.eFlags & EF_FORCE_DRAINED)
			&& !(self->client->ps.eFlags & EF_FORCE_GRABBED)
			&& self->painDebounceTime < level.time)
		{
			//still alive, have saber in hand, not taking pain and not being gripped
			player_Cloak(self);
		}
	}
}

/*
==========================================================================================
AGGRESSION
==========================================================================================
*/
static void Jedi_Aggression(const gentity_t* self, const int change)
{
	int upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;

	//FIXME: base this on initial NPC stats
	if (self->client->playerTeam == TEAM_PLAYER)
	{
		//good guys are less aggressive
		upper_threshold = 10;
		lower_threshold = 5;
	}
	else
	{
		//bad guys are more aggressive
		if (npc_is_dark_jedi(self))
		{
			upper_threshold = 20;
			lower_threshold = 10;
		}
		else
		{
			upper_threshold = 15;
			lower_threshold = 5;
		}
	}

	if (self->NPC->stats.aggression > upper_threshold)
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if (self->NPC->stats.aggression < lower_threshold)
	{
		self->NPC->stats.aggression = lower_threshold;
	}
}

static void Jedi_AggressionErosion(const int amt)
{
	if (TIMER_Done(NPC, "roamTime"))
	{
		//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set(NPC, "roamTime", Q_irand(2000, 5000));
		Jedi_Aggression(NPC, amt);
	}

	if (TIMER_Done(NPC, "DeactivateTime"))
	{
		//turn off the saber
		if (!NPC_IsAlive(NPC, NPC->enemy))
		{
			//turn off the saber
			WP_DeactivateLightSaber(NPC);
			NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			G_AddVoiceEvent(NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), 3000);
		}
	}
	else if (NPCInfo->stats.aggression < 4 || NPCInfo->stats.aggression < 6 && NPC->client->NPC_class == CLASS_DESANN)
	{
		//turn off the saber
		WP_DeactivateSaber(NPC);
	}
	else if (NPCInfo->stats.aggression < 4 || NPCInfo->stats.aggression < 6 && NPC->client->NPC_class == CLASS_VADER)
	{
		//turn off the saber
		WP_DeactivateSaber(NPC);
	}
}

void NPC_Jedi_RateNewEnemy(const gentity_t* self, const gentity_t* enemy)
{
	float health_aggression;
	float weapon_aggression;

	switch (enemy->s.weapon)
	{
	case WP_SABER:
		health_aggression = static_cast<float>(self->health) / 200.0f * 6.0f;
		weapon_aggression = 7; //go after him
		break;
	case WP_BLASTER:
		if (DistanceSquared(self->currentOrigin, enemy->currentOrigin) < 65536) //256 squared
		{
			health_aggression = static_cast<float>(self->health) / 200.0f * 8.0f;
			weapon_aggression = 8; //go after him
		}
		else
		{
			health_aggression = 8.0f - static_cast<float>(self->health) / 200.0f * 8.0f;
			weapon_aggression = 2; //hang back for a second
		}
		break;
	default:
		health_aggression = static_cast<float>(self->health) / 200.0f * 8.0f;
		weapon_aggression = 6; //approach
		break;
	}
	//Average these with current aggression
	const int new_aggression = ceil(
		(health_aggression + weapon_aggression + static_cast<float>(self->NPC->stats.aggression)) / 3.0f);
	Jedi_Aggression(self, new_aggression - self->NPC->stats.aggression);

	//don't taunt right away
	TIMER_Set(self, "chatter", Q_irand(4000, 7000));
}

static void Jedi_Rage()
{
	Jedi_Aggression(NPC, 10 - NPCInfo->stats.aggression + Q_irand(-2, 2));
	TIMER_Set(NPC, "roamTime", 0);
	TIMER_Set(NPC, "chatter", 0);
	TIMER_Set(NPC, "walking", 0);
	TIMER_Set(NPC, "taunting", 0);
	TIMER_Set(NPC, "jumpChaseDebounce", 0);
	TIMER_Set(NPC, "movenone", 0);
	TIMER_Set(NPC, "movecenter", 0);
	TIMER_Set(NPC, "noturn", 0);
	ForceRage(NPC);
}

void Jedi_RageStop(const gentity_t* self)
{
	if (self->NPC)
	{
		//calm down and back off
		TIMER_Set(self, "roamTime", 0);
		Jedi_Aggression(self, Q_irand(-5, 0));
	}
}

/*
==========================================================================================
SPEAKING
==========================================================================================
*/

static qboolean Jedi_BattleTaunt(const gentity_t* NPC)
{
	if (TIMER_Done(NPC, "chatter")
		&& !Q_irand(0, 3)
		&& NPCInfo->blockedSpeechDebounceTime < level.time
		&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time)
	{
		int event = -1;

		if (NPC->enemy
			&& NPC->enemy->client
			&& (NPC->enemy->client->NPC_class == CLASS_RANCOR
				|| NPC->enemy->client->NPC_class == CLASS_WAMPA
				|| NPC->enemy->client->NPC_class == CLASS_SAND_CREATURE))
		{
			//never taunt these mindless creatures
		}
		else
		{
			if (NPC->client->playerTeam == TEAM_PLAYER
				&& NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI)
			{
				//a jedi fighting a jedi - training
				if (NPC->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER)
				{
					//only trainer taunts
					event = EV_TAUNT1;
				}
			}
			else
			{
				//reborn or a jedi fighting an enemy
				event = Q_irand(EV_TAUNT1, EV_TAUNT3);
			}
			if (event != -1)
			{
				if (!npc_is_dark_jedi(NPC))
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPC, event, 3000);
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 6000;
				if (NPCInfo->aiFlags & NPCAI_ROSH)
				{
					TIMER_Set(NPC, "chatter", Q_irand(8000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "chatter", Q_irand(5000, 10000));
				}

				if (NPC->enemy && NPC->enemy->NPC
					&& NPC->enemy->s.weapon == WP_SABER
					&& NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI)
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 2000);
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
==========================================================================================
MOVEMENT
==========================================================================================
*/
static qboolean Jedi_ClearPathToSpot(vec3_t dest, const int impactEntNum)
{
	trace_t trace;
	vec3_t mins, end, dir;
	float drop;

	//Offset the step height
	VectorSet(mins, NPC->mins[0], NPC->mins[1], NPC->mins[2] + STEPSIZE);

	gi.trace(&trace, NPC->currentOrigin, mins, NPC->maxs, dest, NPC->s.number, NPC->clipmask,
		static_cast<EG2_Collision>(0), 0);

	//Do a simple check
	if (trace.allsolid || trace.startsolid)
	{
		//inside solid
		return qfalse;
	}

	if (trace.fraction < 1.0f)
	{
		//hit something
		if (impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum)
		{
			//hit what we're going after
			return qtrue;
		}
		return qfalse;
	}

	//otherwise, clear path in a straight line.
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract(dest, NPC->currentOrigin, dir);
	const float dist = VectorNormalize(dir);
	if (dest[2] > NPC->currentOrigin[2])
	{
		//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{
		//going down or level, check for moderate drops
		drop = 64;
	}
	for (float i = NPC->maxs[0] * 2; i < dist; i += NPC->maxs[0] * 2)
	{
		vec3_t start;
		//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA(NPC->currentOrigin, i, dir, start);
		VectorCopy(start, end);
		end[2] -= drop;
		gi.trace(&trace, start, mins, NPC->maxs, end, NPC->s.number, NPC->clipmask, static_cast<EG2_Collision>(0),
			0); //NPC->mins?
		if (trace.fraction < 1.0f || trace.allsolid || trace.startsolid)
		{
			//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

qboolean NPC_MoveDirClear(const int forwardmove, const int rightmove, const qboolean reset)
{
	vec3_t forward, right, testPos, angles{}, mins;
	trace_t trace;
	float bottom_max = -STEPSIZE * 4 - 1;

	if (!forwardmove && !rightmove)
	{
		//not even moving
		return qtrue;
	}

	if (ucmd.upmove > 0 || NPC->client->ps.forceJumpCharge)
	{
		//Going to jump
		return qtrue;
	}

	if (NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in the air
		return qtrue;
	}

	//FIXME: to really do this right, we'd have to actually do a pmove to predict where we're
	//going to be... maybe this should be a flag and pmove handles it and sets a flag so AI knows
	//NEXT frame?  Or just incorporate current velocity, runspeed and possibly friction?
	VectorCopy(NPC->mins, mins);
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = NPC->client->ps.viewangles[YAW]; //Add ucmd.angles[YAW]?
	AngleVectors(angles, forward, right, nullptr);
	const float fwd_dist = static_cast<float>(forwardmove) / 2.0f;
	const float rt_dist = static_cast<float>(rightmove) / 2.0f;
	VectorMA(NPC->currentOrigin, fwd_dist, forward, testPos);
	VectorMA(testPos, rt_dist, right, testPos);
	gi.trace(&trace, NPC->currentOrigin, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask | CONTENTS_BOTCLIP,
		static_cast<EG2_Collision>(0), 0);
	if (trace.allsolid || trace.startsolid)
	{
		if (reset)
		{
			trace.fraction = 1.0f;
		}
		VectorCopy(testPos, trace.endpos);
	}
	if (trace.fraction < 0.6)
	{
		//Going to bump into something very close, don't move, just turn
		if (NPC->enemy && trace.entityNum == NPC->enemy->s.number || NPCInfo->goalEntity && trace.entityNum ==
			NPCInfo->goalEntity->s.number)
		{
			//okay to bump into enemy or goal
			return qtrue;
		}
		if (reset)
		{
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			VectorClear(NPC->client->ps.moveDir);
		}
		return qfalse;
	}

	if (NPCInfo->goalEntity)
	{
		qboolean enemy_in_air = qfalse;

		if (NPC->NPC->goalEntity->client)
		{
			if (NPC->NPC->goalEntity->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				enemy_in_air = qtrue;
			}
		}

		if (NPCInfo->goalEntity->currentOrigin[2] < NPC->currentOrigin[2])
		{
			//goal is below me, okay to step off at least that far plus stepheight
			if (!enemy_in_air)
			{
				bottom_max += NPCInfo->goalEntity->currentOrigin[2] - NPC->currentOrigin[2];
			}
		}
	}
	VectorCopy(trace.endpos, testPos);
	testPos[2] += bottom_max;

	gi.trace(&trace, trace.endpos, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask,
		static_cast<EG2_Collision>(0), 0);

	if (trace.allsolid || trace.startsolid)
	{
		//Not going off a cliff
		return qtrue;
	}

	if (trace.fraction < 1.0)
	{
		//Not going off a cliff
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if (reset)
	{
		//actually want to screw with the ucmd
		ucmd.forwardmove *= -1.0; //= 0;
		ucmd.rightmove *= -1.0; //= 0;
		VectorScale(NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir);
	}
	return qfalse;
}

/*
-------------------------
Jedi_HoldPosition
-------------------------
*/

static void Jedi_HoldPosition()
{
	NPCInfo->goalEntity = nullptr;
}

/*
-------------------------
Jedi_Move
-------------------------
*/

static qboolean Jedi_Move(gentity_t* goal, const qboolean retreat)
{
	const qboolean moved = NPC_MoveToGoal(qtrue);
	navInfo_t info;

	NPCInfo->combatMove = qtrue;
	NPCInfo->goalEntity = goal;

	if (retreat)
	{
		ucmd.forwardmove *= -1;
		ucmd.rightmove *= -1;
		VectorScale(NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir);
	}

	//Get the move info
	NAV_GetLastMove(info);

	//If we hit our target, then stop and fire!
	if (info.flags & NIF_COLLISION && info.blocker == NPC->enemy)
	{
		Jedi_HoldPosition();
	}

	//If our move failed, then reset
	if (moved == qfalse)
	{
		Jedi_HoldPosition();
	}
	return moved;
}

static qboolean Jedi_Hunt()
{
	if (NPCInfo->stats.aggression > 1)
	{
		//approach enemy
		NPCInfo->combatMove = qtrue;
		if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
		{
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
		if (NPCInfo->goalEntity == nullptr)
		{
			//hunt
			NPCInfo->goalEntity = NPC->enemy;
		}
		else
		{
			//hunt
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = 40.0f;
		}

		if (NPC_MoveToGoal(qfalse))
		{
			NPC_UpdateAngles(qtrue, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}

static void Jedi_StartBackOff()
{
	TIMER_Set(NPC, "roamTime", -level.time);
	TIMER_Set(NPC, "strafeLeft", -level.time);
	TIMER_Set(NPC, "strafeRight", -level.time);
	TIMER_Set(NPC, "walking", -level.time);
	TIMER_Set(NPC, "moveforward", -level.time);
	TIMER_Set(NPC, "movenone", -level.time);
	TIMER_Set(NPC, "moveright", -level.time);
	TIMER_Set(NPC, "moveleft", -level.time);
	TIMER_Set(NPC, "movecenter", -level.time);
	TIMER_Set(NPC, "moveback", 1000);
	ucmd.forwardmove = -127;
	ucmd.rightmove = 0;
	ucmd.upmove = 0;
	if (d_JediAI->integer || g_DebugSaberCombat->integer)
	{
		Com_Printf("%s backing off from spin attack!\n", NPC->NPC_type);
	}
	TIMER_Set(NPC, "specialEvasion", 1000);
	TIMER_Set(NPC, "noRetreat", -level.time);
	if (PM_PainAnim(NPC->client->ps.legsAnim))
	{
		NPC->client->ps.legsAnimTimer = 0;
	}
	VectorClear(NPC->client->ps.moveDir);
}

static qboolean Jedi_Retreat()
{
	if (!TIMER_Done(NPC, "noRetreat"))
	{
		//don't actually move
		return qfalse;
	}
	return Jedi_Move(NPC->enemy, qtrue);
}

static qboolean Jedi_Advance()
{
	if (NPCInfo->aiFlags & NPCAI_HEAL_ROSH)
	{
		return qfalse;
	}
	if (!NPC->client->ps.saberInFlight)
	{
		NPC->client->ps.SaberActivate();
	}
	return Jedi_Move(NPC->enemy, qfalse);
}

static void Jedi_AdjustSaberAnimLevel(const gentity_t* self, const int new_level)
{
	if (!self || !self->client)
	{
		return;
	}

	if (self->client->playerTeam == TEAM_ENEMY)
	{
		if (!Q_stricmp("cultist_saber_all", self->NPC_type)
			|| !Q_stricmp("cultist_saber_all_throw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_cyber_tcw", self->NPC_type))
		{
			//use any, regardless of rank, etc.
		}
		else if (!Q_stricmp("cultist_saber", self->NPC_type)
			|| !Q_stricmp("cultist_saber_throw", self->NPC_type))
		{
			//fast only
			self->client->ps.saberAnimLevel = SS_FAST;
		}
		else if (!Q_stricmp("cultist_saber_med", self->NPC_type)
			|| !Q_stricmp("cultist_saber_med_throw", self->NPC_type))
		{
			//med only
			self->client->ps.saberAnimLevel = SS_MEDIUM;
		}
		else if (!Q_stricmp("cultist_saber_strong", self->NPC_type)
			|| !Q_stricmp("cultist_saber_strong_throw", self->NPC_type))
		{
			//strong only
			self->client->ps.saberAnimLevel = SS_STRONG;
		}
		else if (!Q_stricmp("md_grievous", self->NPC_type)
			|| !Q_stricmp("md_grievous4", self->NPC_type)
			|| !Q_stricmp("md_grievous_robed", self->NPC_type)
			|| !Q_stricmp("md_clone_assassin", self->NPC_type)
			|| !Q_stricmp("md_jango", self->NPC_type)
			|| !Q_stricmp("md_jango_geo", self->NPC_type)
			|| !Q_stricmp("md_ani_ep2_dual", self->NPC_type)
			|| !Q_stricmp("md_serra", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_rebels", self->NPC_type)
			|| !Q_stricmp("md_ahsoka", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_s7", self->NPC_type)
			|| !Q_stricmp("md_ventress", self->NPC_type)
			|| !Q_stricmp("md_ven_ns", self->NPC_type)
			|| !Q_stricmp("md_ven_bh", self->NPC_type)
			|| !Q_stricmp("md_ven_dg", self->NPC_type)
			|| !Q_stricmp("md_asharad", self->NPC_type)
			|| !Q_stricmp("md_asharad_tus", self->NPC_type)
			|| !Q_stricmp("boba_fett", self->NPC_type)
			|| !Q_stricmp("boba_fett_esb", self->NPC_type)
			|| !Q_stricmp("md_pguard4", self->NPC_type)
			|| !Q_stricmp("md_grie_egg", self->NPC_type)
			|| !Q_stricmp("md_grie3_egg", self->NPC_type)
			|| !Q_stricmp("md_grie4_egg", self->NPC_type)
			|| !Q_stricmp("md_fet_ga", self->NPC_type)
			|| !Q_stricmp("md_fet2_ga", self->NPC_type)
			|| !Q_stricmp("md_fet3_ga", self->NPC_type)
			|| !Q_stricmp("md_ven2_ga", self->NPC_type)
			|| !Q_stricmp("md_ket_jt", self->NPC_type)
			|| !Q_stricmp("md_fet_ka", self->NPC_type)
			|| !Q_stricmp("md_fet2_ka", self->NPC_type)
			|| !Q_stricmp("md_clo2_rt", self->NPC_type)
			|| !Q_stricmp("md_clo5_rt", self->NPC_type)
			|| !Q_stricmp("reborn_dual", self->NPC_type)
			|| !Q_stricmp("reborn_dual2", self->NPC_type)
			|| !Q_stricmp("alora_dual", self->NPC_type)
			|| !Q_stricmp("JediTrainer", self->NPC_type)
			|| !Q_stricmp("jedi_zf2", self->NPC_type)
			|| !Q_stricmp("RebornMasterDual", self->NPC_type)
			|| !Q_stricmp("Tavion_scepter", self->NPC_type)
			|| !Q_stricmp("md_jed6_jt", self->NPC_type)
			|| !Q_stricmp("md_jed11_jt", self->NPC_type)
			|| !Q_stricmp("md_jed13_jt", self->NPC_type)
			|| !Q_stricmp("md_jediknight2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran3_jt", self->NPC_type)
			|| !Q_stricmp("md_serra_jt", self->NPC_type))
		{
			//dual only
			self->client->ps.saberAnimLevel = SS_DUAL;
		}
		else if (!Q_stricmp("md_magnaguard", self->NPC_type)
			|| !Q_stricmp("md_inquisitor", self->NPC_type)
			|| !Q_stricmp("md_5thbrother", self->NPC_type)
			|| !Q_stricmp("md_7thsister", self->NPC_type)
			|| !Q_stricmp("md_8thbrother", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels2", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels3", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels4", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels5", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels6", self->NPC_type)
			|| !Q_stricmp("md_jbrute", self->NPC_type)
			|| !Q_stricmp("md_templeguard", self->NPC_type)
			|| !Q_stricmp("md_sidious_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw_staff", self->NPC_type)
			|| !Q_stricmp("md_savage", self->NPC_type)
			|| !Q_stricmp("md_galen", self->NPC_type)
			|| !Q_stricmp("md_galen_jt", self->NPC_type)
			|| !Q_stricmp("md_galencjr", self->NPC_type)
			|| !Q_stricmp("md_starkiller", self->NPC_type)
			|| !Q_stricmp("md_sithstalker", self->NPC_type)
			|| !Q_stricmp("md_stk_lord", self->NPC_type)
			|| !Q_stricmp("md_stk_tat", self->NPC_type)
			|| !Q_stricmp("darthdesolous", self->NPC_type)
			|| !Q_stricmp("purge_trooper", self->NPC_type)
			|| !Q_stricmp("cal_kestis_staff", self->NPC_type)
			|| !Q_stricmp("md_2ndsister", self->NPC_type)
			|| !Q_stricmp("md_shadowguard", self->NPC_type)
			|| !Q_stricmp("md_ven_dual", self->NPC_type)
			|| !Q_stricmp("md_pguard5", self->NPC_type)
			|| !Q_stricmp("md_mau_dof", self->NPC_type)
			|| !Q_stricmp("md_mau2_dof", self->NPC_type)
			|| !Q_stricmp("md_mau3_dof", self->NPC_type)
			|| !Q_stricmp("md_mag_egg", self->NPC_type)
			|| !Q_stricmp("md_mag_ga", self->NPC_type)
			|| !Q_stricmp("md_ven_ga", self->NPC_type)
			|| !Q_stricmp("md_mau_luke", self->NPC_type)
			|| !Q_stricmp("darthphobos", self->NPC_type)
			|| !Q_stricmp("md_tus5_tc", self->NPC_type)
			|| !Q_stricmp("md_sta_tfu", self->NPC_type)
			|| !Q_stricmp("md_gua1_tfu", self->NPC_type)
			|| !Q_stricmp("md_sta_cs", self->NPC_type)
			|| !Q_stricmp("md_gua_am", self->NPC_type)
			|| !Q_stricmp("md_gua2_am", self->NPC_type)
			|| !Q_stricmp("md_mag_am", self->NPC_type)
			|| !Q_stricmp("md_mag2_am", self->NPC_type)
			|| !Q_stricmp("JediF", self->NPC_type)
			|| !Q_stricmp("JediMaster", self->NPC_type)
			|| !Q_stricmp("jedi_kdm1", self->NPC_type)
			|| !Q_stricmp("jedi_tf1", self->NPC_type)
			|| !Q_stricmp("reborn_staff", self->NPC_type)
			|| !Q_stricmp("reborn_staff2", self->NPC_type)
			|| !Q_stricmp("RebornMasterStaff", self->NPC_type)
			|| !Q_stricmp("md_jed7_jt", self->NPC_type)
			|| !Q_stricmp("md_jed12_jt", self->NPC_type)
			|| !Q_stricmp("md_jed14_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster3_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster5_jt", self->NPC_type)
			|| !Q_stricmp("md_guard_jt", self->NPC_type)
			|| !Q_stricmp("md_guardboss_jt", self->NPC_type)
			|| !Q_stricmp("md_jedibrute_jt", self->NPC_type)
			|| !Q_stricmp("md_maul", self->NPC_type)
			|| !Q_stricmp("md_maul_robed", self->NPC_type)
			|| !Q_stricmp("md_maul_hooded", self->NPC_type)
			|| !Q_stricmp("md_maul_wots", self->NPC_type))
		{
			//staff only
			self->client->ps.saberAnimLevel = SS_STAFF;
		}
		else
		{
			//use any, regardless of rank, etc.
		}
	}
	else
	{
		if (!Q_stricmp("cultist_saber_all", self->NPC_type)
			|| !Q_stricmp("cultist_saber_all_throw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_cyber_tcw", self->NPC_type))
		{
			//use any, regardless of rank, etc.
		}
		else if (!Q_stricmp("cultist_saber", self->NPC_type)
			|| !Q_stricmp("cultist_saber_throw", self->NPC_type))
		{
			//fast only
			self->client->ps.saberAnimLevel = SS_FAST;
		}
		else if (!Q_stricmp("cultist_saber_med", self->NPC_type)
			|| !Q_stricmp("cultist_saber_med_throw", self->NPC_type))
		{
			//med only
			self->client->ps.saberAnimLevel = SS_MEDIUM;
		}
		else if (!Q_stricmp("cultist_saber_strong", self->NPC_type)
			|| !Q_stricmp("cultist_saber_strong_throw", self->NPC_type))
		{
			//strong only
			self->client->ps.saberAnimLevel = SS_STRONG;
		}
		else if (!Q_stricmp("md_grievous", self->NPC_type)
			|| !Q_stricmp("md_grievous4", self->NPC_type)
			|| !Q_stricmp("md_grievous_robed", self->NPC_type)
			|| !Q_stricmp("md_clone_assassin", self->NPC_type)
			|| !Q_stricmp("md_jango", self->NPC_type)
			|| !Q_stricmp("md_jango_geo", self->NPC_type)
			|| !Q_stricmp("md_ani_ep2_dual", self->NPC_type)
			|| !Q_stricmp("md_serra", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_rebels", self->NPC_type)
			|| !Q_stricmp("md_ahsoka", self->NPC_type)
			|| !Q_stricmp("md_ahsoka_s7", self->NPC_type)
			|| !Q_stricmp("md_ventress", self->NPC_type)
			|| !Q_stricmp("md_ven_ns", self->NPC_type)
			|| !Q_stricmp("md_ven_bh", self->NPC_type)
			|| !Q_stricmp("md_ven_dg", self->NPC_type)
			|| !Q_stricmp("md_asharad", self->NPC_type)
			|| !Q_stricmp("md_asharad_tus", self->NPC_type)
			|| !Q_stricmp("boba_fett", self->NPC_type)
			|| !Q_stricmp("boba_fett_esb", self->NPC_type)
			|| !Q_stricmp("md_pguard4", self->NPC_type)
			|| !Q_stricmp("md_grie_egg", self->NPC_type)
			|| !Q_stricmp("md_grie3_egg", self->NPC_type)
			|| !Q_stricmp("md_grie4_egg", self->NPC_type)
			|| !Q_stricmp("md_fet_ga", self->NPC_type)
			|| !Q_stricmp("md_fet2_ga", self->NPC_type)
			|| !Q_stricmp("md_fet3_ga", self->NPC_type)
			|| !Q_stricmp("md_ven2_ga", self->NPC_type)
			|| !Q_stricmp("md_ket_jt", self->NPC_type)
			|| !Q_stricmp("md_fet_ka", self->NPC_type)
			|| !Q_stricmp("md_fet2_ka", self->NPC_type)
			|| !Q_stricmp("md_clo2_rt", self->NPC_type)
			|| !Q_stricmp("md_clo5_rt", self->NPC_type)
			|| !Q_stricmp("reborn_dual", self->NPC_type)
			|| !Q_stricmp("reborn_dual2", self->NPC_type)
			|| !Q_stricmp("alora_dual", self->NPC_type)
			|| !Q_stricmp("JediTrainer", self->NPC_type)
			|| !Q_stricmp("jedi_zf2", self->NPC_type)
			|| !Q_stricmp("RebornMasterDual", self->NPC_type)
			|| !Q_stricmp("Tavion_scepter", self->NPC_type)
			|| !Q_stricmp("md_jed6_jt", self->NPC_type)
			|| !Q_stricmp("md_jed11_jt", self->NPC_type)
			|| !Q_stricmp("md_jed13_jt", self->NPC_type)
			|| !Q_stricmp("md_jediknight2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran2_jt", self->NPC_type)
			|| !Q_stricmp("md_jediveteran3_jt", self->NPC_type)
			|| !Q_stricmp("md_serra_jt", self->NPC_type))
		{
			//dual only
			self->client->ps.saberAnimLevel = SS_DUAL;
		}
		else if (!Q_stricmp("md_magnaguard", self->NPC_type)
			|| !Q_stricmp("md_inquisitor", self->NPC_type)
			|| !Q_stricmp("md_5thbrother", self->NPC_type)
			|| !Q_stricmp("md_7thsister", self->NPC_type)
			|| !Q_stricmp("md_8thbrother", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels2", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels3", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels4", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels5", self->NPC_type)
			|| !Q_stricmp("md_maul_rebels6", self->NPC_type)
			|| !Q_stricmp("md_jbrute", self->NPC_type)
			|| !Q_stricmp("md_templeguard", self->NPC_type)
			|| !Q_stricmp("md_sidious_tcw", self->NPC_type)
			|| !Q_stricmp("md_maul_tcw_staff", self->NPC_type)
			|| !Q_stricmp("md_savage", self->NPC_type)
			|| !Q_stricmp("md_galen", self->NPC_type)
			|| !Q_stricmp("md_galen_jt", self->NPC_type)
			|| !Q_stricmp("md_galencjr", self->NPC_type)
			|| !Q_stricmp("md_starkiller", self->NPC_type)
			|| !Q_stricmp("md_sithstalker", self->NPC_type)
			|| !Q_stricmp("md_stk_lord", self->NPC_type)
			|| !Q_stricmp("md_stk_tat", self->NPC_type)
			|| !Q_stricmp("darthdesolous", self->NPC_type)
			|| !Q_stricmp("purge_trooper", self->NPC_type)
			|| !Q_stricmp("cal_kestis_staff", self->NPC_type)
			|| !Q_stricmp("md_2ndsister", self->NPC_type)
			|| !Q_stricmp("md_shadowguard", self->NPC_type)
			|| !Q_stricmp("md_ven_dual", self->NPC_type)
			|| !Q_stricmp("md_pguard5", self->NPC_type)
			|| !Q_stricmp("md_mau_dof", self->NPC_type)
			|| !Q_stricmp("md_mau2_dof", self->NPC_type)
			|| !Q_stricmp("md_mau3_dof", self->NPC_type)
			|| !Q_stricmp("md_mag_egg", self->NPC_type)
			|| !Q_stricmp("md_mag_ga", self->NPC_type)
			|| !Q_stricmp("md_ven_ga", self->NPC_type)
			|| !Q_stricmp("md_mau_luke", self->NPC_type)
			|| !Q_stricmp("darthphobos", self->NPC_type)
			|| !Q_stricmp("md_tus5_tc", self->NPC_type)
			|| !Q_stricmp("md_sta_tfu", self->NPC_type)
			|| !Q_stricmp("md_gua1_tfu", self->NPC_type)
			|| !Q_stricmp("md_sta_cs", self->NPC_type)
			|| !Q_stricmp("md_gua_am", self->NPC_type)
			|| !Q_stricmp("md_gua2_am", self->NPC_type)
			|| !Q_stricmp("md_mag_am", self->NPC_type)
			|| !Q_stricmp("md_mag2_am", self->NPC_type)
			|| !Q_stricmp("JediF", self->NPC_type)
			|| !Q_stricmp("JediMaster", self->NPC_type)
			|| !Q_stricmp("jedi_kdm1", self->NPC_type)
			|| !Q_stricmp("jedi_tf1", self->NPC_type)
			|| !Q_stricmp("reborn_staff", self->NPC_type)
			|| !Q_stricmp("reborn_staff2", self->NPC_type)
			|| !Q_stricmp("RebornMasterStaff", self->NPC_type)
			|| !Q_stricmp("md_jed7_jt", self->NPC_type)
			|| !Q_stricmp("md_jed12_jt", self->NPC_type)
			|| !Q_stricmp("md_jed14_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster3_jt", self->NPC_type)
			|| !Q_stricmp("md_jedimaster5_jt", self->NPC_type)
			|| !Q_stricmp("md_guard_jt", self->NPC_type)
			|| !Q_stricmp("md_guardboss_jt", self->NPC_type)
			|| !Q_stricmp("md_jedibrute_jt", self->NPC_type)
			|| !Q_stricmp("md_maul", self->NPC_type)
			|| !Q_stricmp("md_maul_robed", self->NPC_type)
			|| !Q_stricmp("md_maul_hooded", self->NPC_type)
			|| !Q_stricmp("md_maul_wots", self->NPC_type))
		{
			//staff only
			self->client->ps.saberAnimLevel = SS_STAFF;
		}
		else
		{
			//use any, regardless of rank, etc.
		}
	}
	if (new_level < SS_FAST)
	{
		Jedi_AdjustSaberAnimLevel(NPC, Q_irand(SS_FAST, SS_STAFF));
	}
	else if (new_level > SS_STAFF)
	{
		Jedi_AdjustSaberAnimLevel(NPC, Q_irand(SS_FAST, SS_STAFF));
	}
	//use the different attacks, how often they switch and under what circumstances
	if (!(self->client->ps.saberStylesKnown & 1 << new_level))
	{
		//don't know that style, sorry
		return;
	}
	//go ahead and set it
	self->client->ps.saberAnimLevel = new_level;

	if (d_JediAI->integer || g_DebugSaberCombat->integer)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_FAST:
			gi.Printf(S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type);
			break;
		case SS_MEDIUM:
			gi.Printf(S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type);
			break;
		case SS_STRONG:
			gi.Printf(S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type);
			break;
		case SS_DESANN:
			gi.Printf(S_COLOR_RED"%s Saber Attack Set: desann\n", self->NPC_type);
			break;
		case SS_TAVION:
			gi.Printf(S_COLOR_MAGENTA"%s Saber Attack Set: tavion\n", self->NPC_type);
			break;
		case SS_DUAL:
			gi.Printf(S_COLOR_CYAN"%s Saber Attack Set: dual\n", self->NPC_type);
			break;
		case SS_STAFF:
			gi.Printf(S_COLOR_ORANGE"%s Saber Attack Set: staff\n", self->NPC_type);
			break;
		default:;
		}
	}
}

static void Jedi_CheckDecreasesaber_anim_level()
{
	if (NPC->client->ps.saberAnimLevel == SS_DUAL
		|| NPC->client->ps.saberAnimLevel == SS_STAFF)
	{
		return;
	}
	if (!NPC->client->ps.weaponTime && !(ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS)))
	{
		//not attacking
		if (TIMER_Done(NPC, "saberLevelDebounce") && !Q_irand(0, 10))
		{
			Jedi_AdjustSaberAnimLevel(NPC, Q_irand(SS_FAST, SS_TAVION)); //random
			TIMER_Set(NPC, "saberLevelDebounce", Q_irand(3000, 10000));
		}
	}
	else
	{
		TIMER_Set(NPC, "saberLevelDebounce", Q_irand(1000, 5000));
	}
}

extern qboolean PM_SaberInMassiveBounce(int anim);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_SaberInBashedAnim(int anim);

static qboolean Jedi_DecideKick()
{
	if (PM_InKnockDown(&NPC->client->ps))
	{
		return qfalse;
	}
	if (PM_InRoll(&NPC->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInAttack(NPC->client->ps.saber_move) || PM_SaberInStart(NPC->client->ps.saber_move)
		|| PM_SaberInBounce(NPC->client->ps.saber_move) || PM_SaberInReturn(NPC->client->ps.saber_move))
	{
		return qfalse;
	}
	if (PM_InGetUp(&NPC->client->ps))
	{
		return qfalse;
	}
	if (!NPC->enemy || NPC->enemy->s.number < MAX_CLIENTS && NPC->enemy->health <= 0)
	{
		//have no enemy or enemy is a dead player
		return qfalse;
	}
	if (Q_irand(0, RANK_CAPTAIN + 5) > NPCInfo->rank)
	{
		//low chance, based on rank
		return qfalse;
	}
	if (Q_irand(0, 10) > NPCInfo->stats.aggression)
	{
		//the madder the better
		return qfalse;
	}
	if (!TIMER_Done(NPC, "kickDebounce"))
	{
		//just did one
		return qfalse;
	}
	if (PM_SaberInMassiveBounce(NPC->client->ps.torsoAnim) || PM_SaberInBashedAnim(NPC->client->ps.torsoAnim))
	{
		return qfalse;
	}
	if (NPC->client->ps.weapon == WP_SABER)
	{
		if (NPC->client->ps.saber[0].saberFlags & SFL_NO_KICKS)
		{
			return qfalse;
		}
		if (NPC->client->ps.dualSabers
			&& NPC->client->ps.saber[1].saberFlags & SFL_NO_KICKS)
		{
			return qfalse;
		}
	}
	//go for it!
	return qtrue;
}

static void Kyle_GrabEnemy()
{
	WP_SabersCheckLock2(NPC, NPC->enemy, static_cast<saberslock_mode_t>(Q_irand(LOCK_KYLE_GRAB1, LOCK_KYLE_GRAB2)));
	//LOCK_KYLE_GRAB3
	TIMER_Set(NPC, "grabEnemyDebounce", NPC->client->ps.torsoAnimTimer + Q_irand(4000, 20000));
}

static void NPC_GrabPlayer()
{
	WP_SabersCheckLock2(NPC, NPC->enemy, static_cast<saberslock_mode_t>(Q_irand(LOCK_KYLE_GRAB1, LOCK_KYLE_GRAB2)));
	//LOCK_KYLE_GRAB3
	TIMER_Set(NPC, "NPC_GrabPlayerDebounce", NPC->client->ps.torsoAnimTimer + Q_irand(20000, 30000));
}

static void Kyle_TryGrab()
{
	NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPC->client->ps.torsoAnimTimer += 200;
	NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.saber_move = NPC->client->ps.saberMoveNext = LS_READY;
	VectorClear(NPC->client->ps.velocity);
	VectorClear(NPC->client->ps.moveDir);
	ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	//WTF?
	NPC->client->ps.SaberDeactivate();
}

static qboolean Kyle_CanDoGrab()
{
	if (NPCInfo->aiFlags & NPCAI_BOSS_SERENITYJEDIENGINE)
	{
		if (NPC->client->ps.forcePower > BLOCKPOINTS_FULL
			&& NPC->client->ps.blockPoints > BLOCKPOINTS_FULL
			&& NPC->client->ps.saberFatigueChainCount < MISHAPLEVEL_LIGHT)
		{
			//Boss Kyle
			if (NPC->enemy && NPC->enemy->client)
			{
				//have a valid enemy
				if (TIMER_Done(NPC, "NPC_GrabPlayerDebounce"))
				{
					//okay to grab again
					if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//me and enemy are on ground
						if (!PM_InOnGroundAnim(&NPC->enemy->client->ps))
						{
							if ((NPC->client->ps.weaponTime <= 200 || NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
								&& !NPC->client->ps.saberInFlight)
							{
								if (fabs(NPC->enemy->currentOrigin[2] - NPC->currentOrigin[2]) <= 8.0f)
								{
									//close to same level of ground
									if (DistanceSquared(NPC->enemy->currentOrigin, NPC->currentOrigin) <= 10000.0f)
									{
										return qtrue;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if (NPC->client->NPC_class == CLASS_KYLE && NPC->spawnflags & 1)
		{
			//Boss Kyle
			if (NPC->enemy && NPC->enemy->client)
			{
				//have a valid enemy
				if (TIMER_Done(NPC, "grabEnemyDebounce"))
				{
					//okay to grab again
					if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//me and enemy are on ground
						if (!PM_InOnGroundAnim(&NPC->enemy->client->ps))
						{
							if ((NPC->client->ps.weaponTime <= 200 || NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
								&& !NPC->client->ps.saberInFlight)
							{
								if (fabs(NPC->enemy->currentOrigin[2] - NPC->currentOrigin[2]) <= 8.0f)
								{
									//close to same level of ground
									if (DistanceSquared(NPC->enemy->currentOrigin, NPC->currentOrigin) <= 10000.0f)
									{
										return qtrue;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return qfalse;
}

extern qboolean PM_SaberInTransition(int move);
extern qboolean PM_CrouchAnim(int anim);
qboolean Jedi_SaberBusy(const gentity_t* self);
static qboolean shoot;
static float enemyDist;
constexpr auto MELEE_DIST_SQUARED = 6400;
extern cvar_t* com_outcast;

static void jedi_combat_distance(const int enemy_dist)
{
	enemyDist = DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);

	if (Jedi_CultistDestroyer(NPC))
	{
		//destroyer
		Jedi_Advance();
		//always run, regardless of what navigation tells us to do!
		NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		ucmd.buttons &= ~BUTTON_WALKING;
		return;
	}
	if (enemy_dist < 128
		&& NPC->enemy
		&& NPC->enemy->client
		&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS))
	{
		//whoa, back off!!!
		if (Q_irand(-3, NPCInfo->rank) > RANK_CREWMAN)
		{
			Jedi_StartBackOff();
			return;
		}
	}
	if (enemy_dist < 128
		&& NPC->enemy
		&& NPC->enemy->client
		&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f)
		&& PM_SaberInKata(static_cast<saber_moveName_t>(NPC->enemy->client->ps.saber_move)))
	{
		//whoa, back off!!!
		if (Q_irand(0, 1))
		{
			Jedi_StartBackOff();
			G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
			return;
		}
	}

	if (enemy_dist < 128
		&& NPC->enemy
		&& NPC->enemy->client
		&& (NPC->client->ps.weapon == WP_SABER && (NPC->client->ps.saberInFlight || NPC->client->ps.saberEntityState ==
			SES_RETURNING))
		&& NPC->enemy->s.weapon == WP_SABER
		&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f))
	{
		//whoa, back off!!!
		if (Q_irand(0, 1))
		{
			Jedi_StartBackOff();
			G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
			return;
		}
	}

	//saber users must walk in combat
	if (!in_camera
		&& NPC->enemy
		&& enemy_dist < 100
		&& NPC_IsAlive(NPC, NPC->enemy)
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->client->ps.weapon == WP_SABER
		&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.9f))
	{
		if (enemy_dist < 60)
		{
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			ucmd.buttons |= BUTTON_WALKING;
		}
		else if (enemy_dist < 70 && NPC->client->ps.speed == NPCInfo->stats.walkSpeed)
		{
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			ucmd.buttons |= BUTTON_WALKING;
		}
		else
		{
			// do what navigation tells us to do!
		}
	}

	if (npc_is_jedi(NPC) && TIMER_Done(NPC, "regenerate") && enemy_dist > 256
		&& NPC->client->ps.weaponTime <= 0
		&& !Jedi_SaberBusy(NPC))
	{
		if (NPC->client->ps.blockPoints < BLOCKPOINTS_THIRTY)
		{
			add_npc_block_point_bonus(NPC);

			if (NPC->client->NPC_class == CLASS_TAVION
				|| NPC->client->NPC_class == CLASS_ALORA
				|| NPC->client->NPC_class == CLASS_DESANN
				|| NPC->client->NPC_class == CLASS_KYLE
				|| NPC->client->NPC_class == CLASS_LUKE)
			{
				NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_MEDITATE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}

			TIMER_Set(NPC, "regenerate", Q_irand(10000, 15000));
			NPC->client->ps.powerups[PW_MEDITATE] = level.time + NPC->client->ps.torsoAnimTimer + 3000;
			NPC->client->ps.eFlags |= EF_MEDITATING;
		}
	}

	if (npc_is_jedi(NPC) && TIMER_Done(NPC, "heal") && enemy_dist > 256
		&& !Jedi_SaberBusy(NPC)
		&& NPC->client->ps.forcePowerLevel[FP_HEAL] > 0
		&& NPC->health > 0
		&& NPC->client->ps.weaponTime <= 0
		&& NPC->client->ps.forcePower >= 25
		&& NPC->client->ps.forcePowerDebounce[FP_HEAL] <= level.time
		&& WP_ForcePowerUsable(NPC, FP_HEAL, 0)
		&& NPC->health <= NPC->client->ps.stats[STAT_MAX_HEALTH] / 3
		&& Q_irand(0, 2) == 2)
	{
		// Try to heal...
		ForceHeal(NPC);
		TIMER_Set(NPC, "heal", Q_irand(10000, 15000));
	}

	if (!in_camera
		&& NPC->enemy
		&& enemy_dist < 200
		&& NPC_IsAlive(NPC, NPC->enemy)
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->client->ps.weapon == WP_SABER)
	{
		if (NPC->saberPowerTime < level.time)
		{
			//Don't just use strong attacks constantly, switch around a bit
			if (Q_irand(1, 10) <= 5)
			{
				NPC->saberPower = qtrue;
			}
			else
			{
				NPC->saberPower = qfalse;
			}

			NPC->saberPowerTime = level.time + Q_irand(3000, 15000);
		}
		if (NPC->client->ps.saberAnimLevel != SS_STAFF
			&& NPC->client->ps.saberAnimLevel != SS_DUAL)
		{
			if (NPC->enemy->client->ps.blockPoints > BLOCKPOINTS_MISSILE &&
				NPC->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_2)
			{
				if (NPC->client->ps.saberAnimLevel != SS_STRONG &&
					NPC->client->ps.saberAnimLevel != SS_DESANN && NPC->saberPower)
				{ //if we are up against someone with a lot of blockpoints and we have a strong attack available, then h4q them
					Jedi_AdjustSaberAnimLevel(NPC, Q_irand(SS_DESANN, SS_STRONG)); //use a faster attack
				}
			}
			else if (NPC->enemy->client->ps.blockPoints > BLOCKPOINTS_FOURTY &&
				NPC->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_1)
			{
				if (NPC->client->ps.saberAnimLevel != SS_MEDIUM)
				{ //they're down on blockpoints a little, use level 2 if we can
					Jedi_AdjustSaberAnimLevel(NPC, SS_MEDIUM); //use a faster attack
				}
			}
			else
			{
				if (NPC->client->ps.saberAnimLevel != SS_FAST
					&& NPC->client->ps.saberAnimLevel != SS_TAVION)
				{ //they've gone below 40 blockpoints, go at them with quick attacks
					Jedi_AdjustSaberAnimLevel(NPC, Q_irand(SS_DESANN, SS_FAST)); //use a faster attack
				}
			}
		}
	}

	// SLAP
	shoot = qfalse;

	if (TIMER_Done(NPC, "closeattack"))
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
		{
			WeaponThink();
			TIMER_Set(NPC, "closeattack", Q_irand(2000, 4000));
			Jedi_Aggression(NPC, 5);
		}
	}

	if (npc_is_light_jedi(NPC)
		&& NPC->client->playerTeam == TEAM_PLAYER
		&& !(ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_USE | BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPC->client->ps))
	{
		if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B3 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
		{
			shoot = qfalse;
			if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPC->enemy, NPC, NPC, smack_dir, NPC->currentOrigin,
						(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

					WP_AbsorbKick(NPC->enemy, NPC, smack_dir);
					//done with the damage
					NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}
	else
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
			//within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				//animate me
				int swing_anim;
				if (NPC->health > BLOCKPOINTS_THIRTY)
				{
					swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				}
				else
				{
					swing_anim = BOTH_A7_HILT;
				}
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
			//within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B3);
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (npc_is_light_jedi(NPC)
		&& NPC->client->playerTeam == TEAM_ENEMY
		&& !(ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_USE | BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPC->client->ps))
	{
		if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B3 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
		{
			shoot = qfalse;
			if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPC->enemy, NPC, NPC, smack_dir, NPC->currentOrigin,
						(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

					WP_AbsorbKick(NPC->enemy, NPC, smack_dir);
					//done with the damage
					NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}
	else
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
			//within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				//animate me
				int swing_anim;
				if (NPC->health > BLOCKPOINTS_THIRTY)
				{
					swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				}
				else
				{
					swing_anim = BOTH_A7_HILT;
				}
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
			//within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B3);
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (npc_is_dark_jedi(NPC) || Jedi_CultistForceUser(NPC)
		&& NPC->client->playerTeam == TEAM_ENEMY
		&& !(ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_USE | BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPC->client->ps))
	{
		if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B3 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
		{
			shoot = qfalse;
			if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPC->enemy, NPC, NPC, smack_dir, NPC->currentOrigin,
						(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

					WP_AbsorbKick(NPC->enemy, NPC, smack_dir);
					//done with the damage
					NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}
	else
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
			//within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				//animate me
				int swing_anim;
				if (NPC->health > BLOCKPOINTS_THIRTY)
				{
					swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				}
				else
				{
					swing_anim = BOTH_A7_HILT;
				}
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
			//within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B3);
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
	}

	if (npc_is_dark_jedi(NPC) || Jedi_CultistForceUser(NPC)
		&& NPC->client->playerTeam == TEAM_PLAYER
		&& !(ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_USE | BUTTON_BLOCK))
		&& !PM_InKnockDown(&NPC->client->ps))
	{
		if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
			NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2 ||
			NPC->client->ps.torsoAnim == BOTH_A7_KICK_B3 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
		{
			shoot = qfalse;
			if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
			{
				//time to smack
				//recheck enemyDist and InFront
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				{
					vec3_t smack_dir;
					VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smack_dir);
					smack_dir[2] += 20;
					VectorNormalize(smack_dir);
					//hurt them
					G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
					G_Damage(NPC->enemy, NPC, NPC, smack_dir, NPC->currentOrigin,
						(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

					WP_AbsorbKick(NPC->enemy, NPC, smack_dir);
					//done with the damage
					NPCInfo->blockedDebounceTime = 1;
				}
			}
		}
	}
	else
	{
		if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
			//within 80 and in front
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				//animate me
				int swing_anim;
				if (NPC->health > BLOCKPOINTS_THIRTY)
				{
					swing_anim = Q_irand(BOTH_A7_SLAP_L, BOTH_A7_SLAP_R);
				}
				else
				{
					swing_anim = BOTH_A7_HILT;
				}
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
		else if (enemyDist < MELEE_DIST_SQUARED
			&& !NPC->client->ps.weaponTime //not firing
			&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
			&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
			//within 80 and generally behind
		{
			//enemy within 80, if very close, use melee attack to slap away
			if (TIMER_Done(NPC, "attackDelay"))
			{
				const int swing_anim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B3);
				G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 2000);
				NPC_SetAnim(NPC, SETANIM_BOTH, swing_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (NPC->client->ps.blockPoints > BLOCKPOINTS_THIRTY)
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
				}
				else
				{
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
				}
				//delay the hurt until the proper point in the anim
				TIMER_Set(NPC, "smackTime", 300);
				NPCInfo->blockedDebounceTime = 0;
			}
		}
	}
	if (NPC->client->ps.forcePowersActive & 1 << FP_GRIP &&
		NPC->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
	{
		//when gripping, don't move
		return;
	}
	if (!TIMER_Done(NPC, "gripping"))
	{
		//stopped gripping, clear timers just in case
		TIMER_Set(NPC, "gripping", -level.time);
		TIMER_Set(NPC, "attackDelay", Q_irand(0, 1000));
	}

	if (!TIMER_Done(NPC, "grasping"))
	{
		//stopped gripping, clear timers just in case
		TIMER_Set(NPC, "grasping", -level.time);
		TIMER_Set(NPC, "attackDelay", Q_irand(0, 1000));
	}

	if (NPC->client->ps.forcePowersActive & 1 << FP_DRAIN &&
		NPC->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
	{
		//when draining, don't move
		return;
	}
	if (!TIMER_Done(NPC, "draining"))
	{
		//stopped draining, clear timers just in case
		TIMER_Set(NPC, "draining", -level.time);
		TIMER_Set(NPC, "attackDelay", Q_irand(0, 1000));
	}

	if (NPC->client->NPC_class == CLASS_BOBAFETT)
	{
		if (!TIMER_Done(NPC, "flameTime"))
		{
			if (enemy_dist > 50)
			{
				Jedi_Advance();
			}
			else if (enemy_dist <= 0)
			{
				Jedi_Retreat();
			}
		}
		else if (enemy_dist < 200)
		{
			Jedi_Retreat();
		}
		else if (enemy_dist > 1024)
		{
			Jedi_Advance();
		}
	}
	else if (NPC->client->NPC_class == CLASS_MANDO)
	{
		if (!TIMER_Done(NPC, "flameTime"))
		{
			if (enemy_dist > 50)
			{
				Jedi_Advance();
			}
			else if (enemy_dist <= 0)
			{
				Jedi_Retreat();
			}
		}
		else if (enemy_dist < 200)
		{
			Jedi_Retreat();
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if (enemy_dist > 1024)
		{
			Jedi_Advance();
		}
		else
		{
			Jedi_Patrol();
		}
	}
	else if (NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW_MD2)
	{
		//don't move at all when alora is doing her spin throw attack.
	}
	else if (NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW)
	{
		//don't move at all when alora is doing her spin throw attack.
	}
	else if (NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{
		//see if we grabbed enemy
		if (NPC->client->ps.torsoAnimTimer <= 200)
		{
			if (Kyle_CanDoGrab()
				&& NPC_EnemyRangeFromBolt(NPC->handRBolt) <= 72.0f)
			{
				//grab him!
				if (NPCInfo->aiFlags & NPCAI_BOSS_SERENITYJEDIENGINE)
				{
					NPC_GrabPlayer();
				}
				else
				{
					Kyle_GrabEnemy();
				}
				return;
			}
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
			return;
		}
		//else just sit here?
		return;
	}
	else if (NPC->client->ps.saberInFlight &&
		!PM_SaberInBrokenParry(NPC->client->ps.saber_move) && NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
	{
		//maintain distance
		if (enemy_dist < NPC->client->ps.saberEntityDist)
		{
			Jedi_Retreat();
		}
		else if (enemy_dist > NPC->client->ps.saberEntityDist && enemy_dist > 100)
		{
			Jedi_Advance();
		}
		if (NPC->client->ps.weapon == WP_SABER //using saber
			&& NPC->client->ps.saberEntityState == SES_LEAVING //not returning yet
			&& NPC->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(NPC->client->ps.forcePowersActive & 1 << FP_SPEED)
			&& !(NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
		{
			//hold it out there
			ucmd.buttons &= ~BUTTON_ATTACK;
			ucmd.buttons |= BUTTON_ALT_ATTACK;
		}
	}
	else if (!TIMER_Done(NPC, "taunting"))
	{
		if (enemy_dist <= 64)
		{
			//he's getting too close
			ucmd.buttons &= ~BUTTON_ALT_ATTACK;
			if (!NPC->client->ps.saberInFlight)
			{
				NPC->client->ps.SaberActivate();
			}
			TIMER_Set(NPC, "taunting", -level.time);
		}
		else if (NPC->client->ps.torsoAnim == BOTH_GESTURE1 && NPC->client->ps.torsoAnimTimer < 2000)
		{
			//we're almost done with our special taunt
			if (!NPC->client->ps.saberInFlight)
			{
				NPC->client->ps.SaberActivate();
			}
		}
	}
	else if (NPC->client->ps.saberEventFlags & SEF_LOCK_WON)
	{
		//we won a saber lock, press the advantage
		if (enemy_dist > 0)
		{
			//get closer so we can hit!
			Jedi_Advance();
		}
		if (enemy_dist > 128)
		{
			//lost 'em
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if (NPC->enemy->painDebounceTime + 2000 < level.time)
		{
			//the window of opportunity is gone
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set(NPC, "strafeLeft", -1);
		TIMER_Set(NPC, "strafeRight", -1);
	}
	else if (NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time
		&& NPC->client->ps.saberLockTime < level.time)
	{
		//enemy is in a saberLock and we are not
		if (enemy_dist < 64)
		{
			Jedi_Retreat();
		}
	}
	else if (NPC->enemy->s.weapon == WP_TURRET
		&& !Q_stricmp("PAS", NPC->enemy->classname)
		&& NPC->enemy->s.apos.trType == TR_STATIONARY)
	{
		int testlevel;
		if (enemy_dist > forcePushPullRadius[FORCE_LEVEL_1] - 16)
		{
			Jedi_Advance();
		}
		if (NPC->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1)
		{
			//
			testlevel = FORCE_LEVEL_1;
		}
		else
		{
			testlevel = NPC->client->ps.forcePowerLevel[FP_PUSH];
		}
		if (enemy_dist < forcePushPullRadius[testlevel] - 16)
		{
			//close enough to push
			if (in_front(NPC->enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles,
				0.6f))
			{
				//knock it down
				WP_KnockdownTurret(NPC->enemy);
				//do the forcethrow call just for effect
				ForceThrow(NPC, qfalse);
			}
		}
	}
	else if (enemy_dist <= 64
		&& (NPCInfo->scriptFlags & SCF_DONT_FIRE || !Q_stricmp("Yoda", NPC->NPC_type) && !Q_irand(0, 10)))
	{
		//can't use saber and they're in striking range
		if (!Q_irand(0, 5) && in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.2f))
		{
			if ((NPCInfo->scriptFlags & SCF_DONT_FIRE || NPC->max_health - NPC->health > NPC->max_health * 0.25f)
				//lost over 1/4 of our health or not firing
				&& WP_ForcePowerUsable(NPC, FP_DRAIN, 20) //know how to drain and have enough power
				&& !Q_irand(0, 2))
			{
				//drain
				TIMER_Set(NPC, "draining", 3000);
				TIMER_Set(NPC, "attackDelay", 3000);
				Jedi_Advance();
				return;
			}
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (g_pick_auto_multi_kick(NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPC, NPC->enemy) && G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE)
				{
					//kicked!
					TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
					return;
				}
			}
			ForceThrow(NPC, qfalse);
		}
		Jedi_Retreat();
	}
	else if (enemy_dist <= 64
		&& NPC->max_health - NPC->health > NPC->max_health * 0.25f //lost over 1/4 of our health
		&& NPC->client->ps.forcePowersKnown & 1 << FP_DRAIN //know how to drain
		&& WP_ForcePowerAvailable(NPC, FP_DRAIN, 20) //have enough power
		&& !Q_irand(0, 10)
		&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.2f))
	{
		TIMER_Set(NPC, "draining", 3000);
		TIMER_Set(NPC, "attackDelay", 3000);
		Jedi_Advance();
		return;
	}
	else if (enemy_dist <= -16)
	{
		//we're too damn close!
		if (!Q_irand(0, 30)
			&& Kyle_CanDoGrab())
		{
			Kyle_TryGrab();
			return;
		}
		if (NPC->client->ps.stats[STAT_WEAPONS] & 1 << WP_SCEPTER
			&& !Q_irand(0, 20))
		{
			Tavion_StartScepterSlam();
			return;
		}
		if (Jedi_DecideKick())
		{
			//let's try a kick
			if (g_pick_auto_multi_kick(NPC, qfalse, qtrue) != LS_NONE
				|| G_CanKickEntity(NPC, NPC->enemy) && G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE)
			{
				//kicked!
				TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
				return;
			}
		}
		Jedi_Retreat();
	}
	else if (enemy_dist <= 0)
	{
		//we're within striking range
		//if we are attacking, see if we should stop
		if (NPCInfo->stats.aggression < 4)
		{
			//back off and defend
			if (!Q_irand(0, 30)
				&& Kyle_CanDoGrab())
			{
				Kyle_TryGrab();
				return;
			}
			if (NPC->client->ps.stats[STAT_WEAPONS] & 1 << WP_SCEPTER
				&& !Q_irand(0, 20))
			{
				Tavion_StartScepterSlam();
				return;
			}
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (g_pick_auto_multi_kick(NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPC, NPC->enemy) && G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE)
				{
					//kicked!
					TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
					return;
				}
			}
			Jedi_Retreat();
		}
	}
	else if (enemy_dist > 256)
	{
		//we're way out of range
		qboolean used_force = qfalse;
		if (NPCInfo->stats.aggression < Q_irand(0, 20)
			&& NPC->health < NPC->max_health * 0.9f
			&& !Q_irand(0, 2))
		{
			if (NPC->enemy
				&& NPC->enemy->s.number < MAX_CLIENTS
				&& (NPC->client->NPC_class != CLASS_KYLE && NPC->client->NPC_class != CLASS_YODA)
				&& (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
					|| NPC->client->NPC_class == CLASS_SHADOWTROOPER)
				&& Q_irand(0, 3 - g_spskill->integer))
			{
				//hmm, bosses should do this less against the player
			}
			else if (NPC->client->ps.saber[0].type == SABER_SITH_SWORD
				&& NPC->weaponModel[0] != -1)
			{
				Tavion_SithSwordRecharge();
				used_force = qtrue;
			}
			else if ((NPC->client->ps.forcePowersKnown & 1 << FP_HEAL) != 0
				&& (NPC->client->ps.forcePowersActive & 1 << FP_HEAL) == 0
				&& Q_irand(0, 1))
			{
				ForceHeal(NPC);
				used_force = qtrue;

				if (npc_is_jedi(NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPC->client->ps.forcePowersKnown & 1 << FP_TELEPATHY) != 0
				&& (NPC->client->ps.forcePowersActive & 1 << FP_TELEPATHY) == 0
				&& !Q_irand(0, 2))
			{
				//  npcs can now use Mind Trick
				ForceTelepathy(NPC);
				used_force = qtrue;
			}
			else if ((NPC->client->ps.forcePowersKnown & 1 << FP_PROTECT) != 0
				&& (NPC->client->ps.forcePowersActive & 1 << FP_PROTECT) == 0
				&& Q_irand(0, 1))
			{
				ForceProtect(NPC);
				used_force = qtrue;

				if (npc_is_jedi(NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPC, Q_irand(EV_COVER1, EV_COVER5), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPC->client->ps.forcePowersKnown & 1 << FP_ABSORB) != 0
				&& (NPC->client->ps.forcePowersActive & 1 << FP_ABSORB) == 0
				&& Q_irand(0, 1))
			{
				ForceAbsorb(NPC);
				used_force = qtrue;

				if (npc_is_jedi(NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPC, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 5000 + Q_irand(0, 15000));
				}
			}
			else if ((NPC->client->ps.forcePowersKnown & 1 << FP_RAGE) != 0
				&& (NPC->client->ps.forcePowersActive & 1 << FP_RAGE) == 0
				&& Q_irand(0, 1))
			{
				Jedi_Rage();
				used_force = qtrue;

				if (npc_is_jedi(NPC))
				{
					// Do deflect taunt...
					G_AddVoiceEvent(NPC, Q_irand(EV_ANGER1, EV_ANGER3), 5000 + Q_irand(0, 15000));
				}
			}
		}
		if (enemy_dist > 384)
		{
			//FIXME: check for enemy facing away and/or moving away
			if (!Q_irand(0, 10) && NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPC->client
				->playerTeam] < level.time)
			{
				if (NPC_ClearLOS(NPC->enemy))
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time +
					3000;
			}
		}
		//Unless we're totally hiding, go after him
		if (NPCInfo->stats.aggression > 0)
		{
			//approach enemy
			if (!used_force)
			{
				if (NPC->enemy
					&& NPC->enemy->client
					&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
						|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
						|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS))
				{
					//stay put!  They're trying to slice us up!
				}
				else
				{
					Jedi_Advance();
				}
			}
		}
	}
	else if (enemy_dist > 50)
	{
		//we're out of striking range and we are allowed to attack
		//first, check some tactical force power decisions
		if (NPC->enemy && (NPC->enemy->client->ps.eFlags & EF_FORCE_GRIPPED || NPC->enemy->client->ps.eFlags & EF_FORCE_GRABBED))
		{
			//They're being gripped, rush them!
			if (NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//they're on the ground, so advance
				if (TIMER_Done(NPC, "parryTime") || NPCInfo->rank > RANK_LT)
				{
					//not parrying
					if (enemy_dist > 200 || !(NPCInfo->scriptFlags & SCF_DONT_FIRE))
					{
						//far away or allowed to use saber
						Jedi_Advance();
					}
				}
			}
			if (enemy_dist > 128 && !(NPC->client->ps.saber[0].saberFlags & SFL_NOT_THROWABLE)
				&& NPC->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1
				&& !Q_irand(0, 5)
				&& !(NPC->client->ps.forcePowersActive & 1 << FP_SPEED)
				&& !(NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
			{
				//throw saber
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |= BUTTON_ALT_ATTACK;
			}
		}
		else if (NPC->enemy && NPC->enemy->client && //valid enemy
			NPC->enemy->client->ps.saberInFlight && NPC->enemy->client->ps.saber[0].Active() && //enemy throwing saber
			!NPC->client->ps.weaponTime && //I'm not busy
			WP_ForcePowerAvailable(NPC, FP_GRIP, 0) && //I can use the power
			!Q_irand(0, 10) && //don't do it all the time, averages to 1 check a second
			Q_irand(0, 6) < g_spskill->integer && //more likely on harder diff
			Q_irand(RANK_CIVILIAN, RANK_CAPTAIN) < NPCInfo->rank //more likely against harder enemies
			&& InFOV(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 30))
		{
			//They're throwing their saber, grip them!
			//taunt
			if (TIMER_Done(NPC, "chatter") && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->
				blockedSpeechDebounceTime < level.time)
			{
				if (!npc_is_dark_jedi(NPC))
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time +
					3000;
				if (NPCInfo->aiFlags & NPCAI_ROSH)
				{
					TIMER_Set(NPC, "chatter", 6000);
				}
				else
				{
					TIMER_Set(NPC, "chatter", 3000);
				}
			}

			//grip
			TIMER_Set(NPC, "gripping", 3000);
			TIMER_Set(NPC, "attackDelay", 3000);
		}
		else if (NPC->enemy && NPC->enemy->client && //valid enemy
			NPC->enemy->client->ps.saberInFlight && NPC->enemy->client->ps.saber[0].Active() && //enemy throwing saber
			!NPC->client->ps.weaponTime && //I'm not busy
			WP_ForcePowerAvailable(NPC, FP_GRASP, 0) && //I can use the power
			!Q_irand(0, 10) && //don't do it all the time, averages to 1 check a second
			Q_irand(0, 6) < g_spskill->integer && //more likely on harder diff
			Q_irand(RANK_CIVILIAN, RANK_CAPTAIN) < NPCInfo->rank //more likely against harder enemies
			&& InFOV(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 30))
		{
			//They're throwing their saber, grip them!
			//taunt
			if (TIMER_Done(NPC, "chatter") && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->
				blockedSpeechDebounceTime < level.time)
			{
				if (!npc_is_dark_jedi(NPC))
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time +
					3000;
				if (NPCInfo->aiFlags & NPCAI_ROSH)
				{
					TIMER_Set(NPC, "chatter", 6000);
				}
				else
				{
					TIMER_Set(NPC, "chatter", 3000);
				}
			}

			//grip
			TIMER_Set(NPC, "grasping", 3000);
			TIMER_Set(NPC, "attackDelay", 3000);
		}
		else
		{
			if (NPC->enemy && NPC->enemy->client && (NPC->enemy->client->ps.forcePowersActive & 1 << FP_GRIP || NPC->
				enemy->client->ps.forcePowersActive & 1 << FP_GRASP))
			{
				//They're choking someone, probably an ally, run at them and do some sort of attack
				if (NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//they're on the ground, so advance
					if (TIMER_Done(NPC, "parryTime") || NPCInfo->rank > RANK_LT)
					{
						//not parrying
						if (enemy_dist > 200 || !(NPCInfo->scriptFlags & SCF_DONT_FIRE))
						{
							//far away or allowed to use saber
							Jedi_Advance();
						}
					}
				}
			}
			if ((NPC->client->NPC_class == CLASS_KYLE || NPC->client->NPC_class == CLASS_YODA)
				&& NPC->spawnflags & 1
				&& (NPC->enemy && NPC->enemy->client && !NPC->enemy->client->ps.saberInFlight)
				&& TIMER_Done(NPC, "kyleTakesSaber")
				&& !Q_irand(0, 20))
			{
				ForceThrow(NPC, qtrue);
			}
			else if (NPC->client->ps.stats[STAT_WEAPONS] & 1 << WP_SCEPTER
				&& !Q_irand(0, 20))
			{
				Tavion_StartScepterBeam();
				return;
			}
			else
			{
				int chance_scale = 0;
				if ((NPC->client->NPC_class == CLASS_KYLE
					|| NPC->client->NPC_class == CLASS_YODA) && NPC->spawnflags & 1)
				{
					chance_scale = 4;
				}
				else if (NPC->enemy
					&& NPC->enemy->s.number < MAX_CLIENTS
					&& (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
						|| NPC->client->NPC_class == CLASS_SHADOWTROOPER))
				{
					//hmm, bosses do this less against player
					chance_scale = 8 - g_spskill->integer * 2;
				}
				else if (NPC->client->NPC_class == CLASS_DESANN)
				{
					chance_scale = 1;
				}
				else if (NPC->client->NPC_class == CLASS_VADER)
				{
					chance_scale = 1;
				}
				else if (NPCInfo->rank == RANK_ENSIGN)
				{
					chance_scale = 2;
				}
				else if (NPCInfo->rank >= RANK_LT_JG)
				{
					chance_scale = 5;
				}
				if (chance_scale
					&& (enemy_dist > Q_irand(100, 200) || NPCInfo->scriptFlags & SCF_DONT_FIRE || !Q_stricmp(
						"Yoda", NPC->NPC_type) && !Q_irand(0, 3))
					&& enemy_dist < 500
					&& (Q_irand(0, chance_scale * 10) < 5 || NPC->enemy->client && NPC->enemy->client->ps.weapon !=
						WP_SABER && !Q_irand(0, chance_scale)))
				{
					//else randomly try some kind of attack every now and then
					// Cultist fencers don't have any of these fancy powers
					if ((NPCInfo->rank == RANK_ENSIGN //old reborn crap
						|| NPCInfo->rank > RANK_LT_JG) //old reborn crap
						&& (!Q_irand(0, 1) || NPC->s.weapon != WP_SABER))
					{
						if (WP_ForcePowerUsable(NPC, FP_PULL, 0) && !Q_irand(0, 2))
						{
							ForceThrow(NPC, qtrue);
							//maybe strafe too?
							TIMER_Set(NPC, "duck", enemy_dist * 3);
							if (Q_irand(0, 1))
							{
								ucmd.buttons |= BUTTON_ATTACK;
							}
						} /////////  lightning ////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_LIGHTNING, 0)
							&& (NPCInfo->scriptFlags & SCF_DONT_FIRE &&
								(Q_stricmp("md_snoke_cin", NPC->NPC_type)
									|| Q_stricmp("md_snoke", NPC->NPC_type)
									|| Q_stricmp("md_palpatine", NPC->NPC_type)
									|| Q_stricmp("md_mother_talzin", NPC->NPC_type)
									|| Q_stricmp("md_sidious_ep2", NPC->NPC_type)
									|| Q_stricmp("md_sidious", NPC->NPC_type)
									|| Q_stricmp("md_sidious_ep3_red", NPC->NPC_type)
									|| Q_stricmp("md_pal_mof", NPC->NPC_type)
									|| Q_stricmp("md_emperor", NPC->NPC_type)
									|| Q_stricmp("md_emperor_fas", NPC->NPC_type)
									|| Q_stricmp("md_emperor_ros", NPC->NPC_type)
									|| Q_stricmp("md_emperor_ros_blind", NPC->NPC_type)
									|| Q_stricmp("cultist_lightning", NPC->NPC_type))
								|| Q_irand(0, 1)))
						{
							ForceLightning(NPC);
							if (NPC->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
							{
								NPC->client->ps.weaponTime = Q_irand(1000, 3000 + g_spskill->integer * 500);
								TIMER_Set(NPC, "holdLightning", NPC->client->ps.weaponTime);
							}
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						} ///////////// drain /////////////////////////
						else if (NPC->health < NPC->max_health * 0.75f
							&& Q_irand(FORCE_LEVEL_0, NPC->client->ps.forcePowerLevel[FP_DRAIN]) > FORCE_LEVEL_1
							&& WP_ForcePowerUsable(NPC, FP_DRAIN, 0)
							&& (NPCInfo->scriptFlags & SCF_DONT_FIRE && Q_stricmp("cultist_drain", NPC->NPC_type) ||
								Q_irand(0, 1)))
						{
							ForceDrain2(NPC);
							NPC->client->ps.weaponTime = Q_irand(1000, 3000 + g_spskill->integer * 500);
							TIMER_Set(NPC, "draining", NPC->client->ps.weaponTime);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}
						////////////////////  New ForcePowers  ///////////////////////////

						////////////////////  FP_FEAR  //////////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_FEAR, 0) && Q_irand(0, 2)
							&& NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.stasisTime < level.time)
						{
							ForceFear(NPC);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}

						/////////////////////  FP_LIGHTNING_STRIKE ////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_LIGHTNING_STRIKE, 0)
							&& Q_irand(0, 1) && NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.stasisTime <
							level.time)
						{
							ForceLightningStrike(NPC);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}

						//////////////////////  FP_DESTRUCTION  ////////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_DESTRUCTION, 0)
							&& Q_irand(0, 1) && NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.stasisTime <
							level.time)
						{
							ForceDestruction(NPC);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}

						//////////////////////  FP_STASIS  ////////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_STASIS, 0) && Q_irand(0, 1) && NPC->enemy && NPC->enemy->
							client && NPC->enemy->client->ps.stasisTime < level.time)
						{
							force_stasis(NPC);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}

						//////////////////////  FP_STASIS  ////////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_STASIS, 0) && Q_irand(0, 1) && NPC->enemy && NPC->enemy->
							client && NPC->enemy->client->ps.stasisJediTime < level.time)
						{
							force_stasis(NPC);
							TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
						}

						//////////////////////  FP_GRASP  ////////////////////////////////////
						else if (WP_ForcePowerUsable(NPC, FP_GRASP, 0)
							&& NPC->enemy && InFOV(NPC->enemy->currentOrigin, NPC->currentOrigin,
								NPC->client->ps.viewangles, 20, 30))
						{
							//taunt
							if (TIMER_Done(NPC, "chatter") && jediSpeechDebounceTime[NPC->client->playerTeam] < level.
								time && NPCInfo->blockedSpeechDebounceTime < level.time)
							{
								G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
								jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime =
									level.time + 3000;
								if (NPCInfo->aiFlags & NPCAI_ROSH)
								{
									TIMER_Set(NPC, "chatter", 6000);
								}
								else
								{
									TIMER_Set(NPC, "chatter", 3000);
								}
							}

							//grip
							TIMER_Set(NPC, "grasping", 3000);
							TIMER_Set(NPC, "attackDelay", 3000);
						} /////////  FP_GRASP ////////////////////////////
						////////////////////  New ForcePowers end ///////////////////////////

						//////////////// FP_GRIP /////////////////////////

						else if (WP_ForcePowerUsable(NPC, FP_GRIP, 0)
							&& NPC->enemy && InFOV(NPC->enemy->currentOrigin, NPC->currentOrigin,
								NPC->client->ps.viewangles, 20, 30))
						{
							//taunt
							if (TIMER_Done(NPC, "chatter") && jediSpeechDebounceTime[NPC->client->playerTeam] < level.
								time && NPCInfo->blockedSpeechDebounceTime < level.time)
							{
								G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
								jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime =
									level.time + 3000;
								if (NPCInfo->aiFlags & NPCAI_ROSH)
								{
									TIMER_Set(NPC, "chatter", 6000);
								}
								else
								{
									TIMER_Set(NPC, "chatter", 3000);
								}
							}

							//grip
							TIMER_Set(NPC, "gripping", 3000);
							TIMER_Set(NPC, "attackDelay", 3000);
						}
						else
						{
							if (!npc_should_not_throw_saber(NPC) &&
								NPC->client->ps.forcePower > BLOCKPOINTS_KNOCKAWAY &&
								enemy_dist > 128 && WP_ForcePowerUsable(NPC, FP_SABERTHROW, 0)
								&& !(NPC->client->ps.forcePowersActive & 1 << FP_SPEED)
								&& !(NPC->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
								&& !(NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
							{
								//throw saber
								ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
						}
					}
					else
					{
						if (!npc_should_not_throw_saber(NPC) &&
							NPC->client->ps.forcePower > BLOCKPOINTS_KNOCKAWAY &&
							enemy_dist > 128 && (NPCInfo->rank >= RANK_LT_JG || WP_ForcePowerUsable(
								NPC, FP_SABERTHROW, 0))
							&& !(NPC->client->ps.forcePowersActive & 1 << FP_SPEED)
							&& !(NPC->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
							&& !(NPC->client->ps.saberEventFlags & SEF_INWATER)) //saber not in water
						{
							//throw saber
							ucmd.buttons |= BUTTON_ALT_ATTACK;
						}
					}
				}
				//see if we should advance now
				else if (NPCInfo->stats.aggression > 5)
				{
					//approach enemy
					if (TIMER_Done(NPC, "parryTime") || NPCInfo->rank > RANK_LT)
					{
						//not parrying
						if (!NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
						{
							//they're on the ground, so advance
							if (enemy_dist > 200 || !(NPCInfo->scriptFlags & SCF_DONT_FIRE))
							{
								//far away or allowed to use saber
								Jedi_Advance();
							}
						}
					}
				}
				else
				{
					//maintain this distance?
					//walk?
				}
			}
		}
	}
	else
	{
		//we're not close enough to attack, but not far enough away to be safe
		if (!Q_irand(0, 30)
			&& Kyle_CanDoGrab())
		{
			Kyle_TryGrab();
			return;
		}
		if (NPCInfo->stats.aggression < 4)
		{
			//back off and defend
			if (Jedi_DecideKick())
			{
				//let's try a kick
				if (g_pick_auto_multi_kick(NPC, qfalse, qtrue) != LS_NONE
					|| G_CanKickEntity(NPC, NPC->enemy) && G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE)
				{
					//kicked!
					TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
					return;
				}
			}
			Jedi_Retreat();
		}
		else if (NPCInfo->stats.aggression > 5)
		{
			//try to get closer
			if (enemy_dist > 0 && !(NPCInfo->scriptFlags & SCF_DONT_FIRE))
			{
				//we're allowed to use our lightsaber, get closer
				if (TIMER_Done(NPC, "parryTime") || NPCInfo->rank > RANK_LT)
				{
					//not parrying
					if (!NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//they're on the ground, so advance
						Jedi_Advance();
					}
				}
			}
		}
		else
		{
			//agression is 4 or 5... somewhere in the middle
			//what do we do here?  Nothing?
			//Move forward and back?
		}
	}
	//if really really mad, rage!
	if (enemy_dist > 1024 && NPCInfo->stats.aggression > Q_irand(5, 15)
		&& NPC->health < NPC->max_health * 0.75f
		&& !Q_irand(0, 2))
	{
		if ((NPC->client->ps.forcePowersKnown & 1 << FP_RAGE) != 0
			&& (NPC->client->ps.forcePowersActive & 1 << FP_RAGE) == 0)
		{
			Jedi_Rage();
		}
	}
}

static qboolean jedi_strafe(const int strafe_time_min, const int strafe_time_max, const int next_strafe_time_min,
	const int next_strafe_time_max, const qboolean walking)
{
	if (Jedi_CultistDestroyer(NPC))
	{
		//never strafe
		return qfalse;
	}
	if (NPC->client->ps.saberEventFlags & SEF_LOCK_WON && NPC->enemy && NPC->enemy->painDebounceTime > level.time)
	{
		//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if (TIMER_Done(NPC, "strafeLeft") && TIMER_Done(NPC, "strafeRight"))
	{
		qboolean strafed = qfalse;
		const int strafe_time = Q_irand(strafe_time_min, strafe_time_max);

		if (Q_irand(0, 1))
		{
			if (NPC_MoveDirClear(ucmd.forwardmove, -127, qfalse))
			{
				TIMER_Set(NPC, "strafeLeft", strafe_time);
				strafed = qtrue;
			}
			else if (NPC_MoveDirClear(ucmd.forwardmove, 127, qfalse))
			{
				TIMER_Set(NPC, "strafeRight", strafe_time);
				strafed = qtrue;
			}
		}
		else
		{
			if (NPC_MoveDirClear(ucmd.forwardmove, 127, qfalse))
			{
				TIMER_Set(NPC, "strafeRight", strafe_time);
				strafed = qtrue;
			}
			else if (NPC_MoveDirClear(ucmd.forwardmove, -127, qfalse))
			{
				TIMER_Set(NPC, "strafeLeft", strafe_time);
				strafed = qtrue;
			}
		}

		if (strafed)
		{
			TIMER_Set(NPC, "noStrafe", strafe_time + Q_irand(next_strafe_time_min, next_strafe_time_max));
			if (walking)
			{
				//should be a slow strafe
				TIMER_Set(NPC, "walking", strafe_time);
			}
			return qtrue;
		}
	}
	return qfalse;
}

extern qboolean PM_EvasionAnim(int anim);
extern qboolean PM_EvasionHoldAnim(int anim);

qboolean jedi_disruptor_dodge_evasion(gentity_t* self, gentity_t* shooter, trace_t* tr, int hit_loc)
{
	int dodge_anim = -1;

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge in mid-air
		return qfalse;
	}

	if (self->client->ps.pm_time && self->client->ps.pm_flags & PMF_TIME_KNOCKBACK)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (self->client->ps.forcePower < FATIGUE_DODGE)
	{
		//must have enough force power
		return qfalse;
	}

	if (self->client->NPC_class != CLASS_BOBAFETT && self->client->NPC_class != CLASS_MANDO)
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (self->client->ps.weapon == WP_TURRET
		|| self->client->ps.weapon == WP_EMPLACED_GUN
		|| self->client->ps.weapon == WP_ATST_MAIN
		|| self->client->ps.weapon == WP_ATST_SIDE)
	{
		return qfalse;
	}

	if (hit_loc == HL_NONE)
	{
		if (tr)
		{
			for (auto& z : tr->G2CollisionMap)
			{
				if (z.mEntityNum == -1)
				{
					//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
					continue;
				}

				CCollisionRecord& coll = z;
				G_GetHitLocFromSurfName(&g_entities[coll.mEntityNum],
					gi.G2API_GetSurfaceName(&g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex],
						coll.mSurfaceIndex), &hit_loc, coll.mCollisionPosition,
					nullptr, nullptr, MOD_UNKNOWN);
				//only want the first
				break;
			}
		}
	}

	switch (hit_loc)
	{
	case HL_NONE:
		return qfalse;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
		dodge_anim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		break;
	case HL_WAIST:
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_BACK_RT:
		dodge_anim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodge_anim = BOTH_DODGE_BL;
		break;
	case HL_BACK_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodge_anim = BOTH_DODGE_BR;
		break;
	case HL_BACK:
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_CHEST:
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_BL, BOTH_DODGE_BR);
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodge_anim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodge_anim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodge_anim = BOTH_CROUCHDODGE;
		break;
	default:;
	}

	if (dodge_anim != -1)
	{
		int extra_hold_time = 0;

		if (PM_EvasionAnim(self->client->ps.torsoAnim) && !PM_EvasionHoldAnim(self->client->ps.torsoAnim))
		{
			//already in a dodge
			//use the hold pose, don't start it all over again
			dodge_anim = BOTH_DODGE_HOLD_FL + (dodge_anim - BOTH_DODGE_FL);
			extra_hold_time = 200;
		}

		//set the dodge anim we chose
		NPC_SetAnim(self, SETANIM_BOTH, dodge_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD); //type
		G_Sound(self, G_SoundIndex("sound/weapons/melee/swing4.wav"));

		if (extra_hold_time && self->client->ps.torsoAnimTimer < extra_hold_time)
		{
			self->client->ps.torsoAnimTimer += extra_hold_time;
		}
		self->client->ps.legsAnimTimer = self->client->ps.torsoAnimTimer;

		WP_ForcePowerStop(self, FP_GRIP);
		WP_ForcePowerStop(self, FP_GRASP);

		if (!self->enemy && G_ValidEnemy(self, shooter))
		{
			G_SetEnemy(self, shooter);
			if (self->s.number)
			{
				Jedi_Aggression(self, 10);
			}
		}

		return qtrue;
	}
	return qfalse;
}

qboolean jedi_dodge_evasion(gentity_t* self, gentity_t* shooter, trace_t* tr, int hit_loc)
{
	int dodge_anim = -1;

	if (in_camera)
	{
		return qfalse;
	}

	if (self->NPC
		&& (self->client->NPC_class == CLASS_SABER_DROID ||
			self->client->NPC_class == CLASS_ASSASSIN_DROID ||
			self->client->NPC_class == CLASS_GONK ||
			self->client->NPC_class == CLASS_MOUSE ||
			self->client->NPC_class == CLASS_PROBE ||
			self->client->NPC_class == CLASS_PROTOCOL ||
			self->client->NPC_class == CLASS_R2D2 ||
			self->client->NPC_class == CLASS_R5D2 ||
			self->client->NPC_class == CLASS_SEEKER ||
			self->client->NPC_class == CLASS_DROIDEKA ||
			self->client->NPC_class == CLASS_INTERROGATOR))
	{
		//non-humans don't get Dodge.
		return qfalse;
	}

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (self->client->NPC_class == CLASS_STORMTROOPER
		|| self->client->NPC_class == CLASS_STORMCOMMANDO
		|| self->client->NPC_class == CLASS_SWAMPTROOPER
		|| self->client->NPC_class == CLASS_IMPWORKER
		|| self->client->NPC_class == CLASS_IMPERIAL
		|| self->client->NPC_class == CLASS_BATTLEDROID
		|| self->client->NPC_class == CLASS_SBD
		|| self->client->NPC_class == CLASS_OBJECT)
	{
		return qfalse; //dont do
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//can't dodge in mid-air
		return qfalse;
	}

	if (self->client->ps.weapon == WP_SABER)
	{
		return qfalse; //dont do THIS IS FOR GUNNERS ONLY
	}

	if (self->client->ps.pm_time && self->client->ps.pm_flags & PMF_TIME_KNOCKBACK)
	{
		//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (self->client->ps.forcePower < FATIGUE_DODGE)
	{
		//must have enough force power
		return qfalse;
	}

	if (self->client->NPC_class != CLASS_BOBAFETT && self->client->NPC_class != CLASS_MANDO)
	{
		PM_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (hit_loc == HL_NONE)
	{
		if (tr)
		{
			for (auto& z : tr->G2CollisionMap)
			{
				if (z.mEntityNum == -1)
				{
					//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
					continue; //break;//
				}

				CCollisionRecord& coll = z;
				G_GetHitLocFromSurfName(&g_entities[coll.mEntityNum],
					gi.G2API_GetSurfaceName(&g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex],
						coll.mSurfaceIndex), &hit_loc, coll.mCollisionPosition,
					nullptr, nullptr, MOD_UNKNOWN);
				//only want the first
				break;
			}
		}
	}

	switch (hit_loc)
	{
	case HL_NONE:
		return qfalse;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
	case HL_WAIST:
	{
		if (!self->s.number)
		{
			//don't force the player to jump
			return qfalse;
		}
		if (!self->enemy && G_ValidEnemy(self, shooter))
		{
			G_SetEnemy(self, shooter);
		}
		if (self->NPC
			&& (self->NPC->scriptFlags & SCF_NO_ACROBATICS || PM_InKnockDown(&self->client->ps)))
		{
			return qfalse;
		}
		if (self->client
			&& (self->client->ps.forceRageRecoveryTime > level.time || self->client->ps.forcePowersActive & 1 <<
				FP_RAGE))
		{
			//no fancy dodges when raging or recovering
			return qfalse;
		}
		if (self->client->NPC_class == CLASS_BOBAFETT && !Q_irand(0, 1))
		{
			return qfalse; // half the time he dodges
		}

		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280; //FIXME: calc this intelligently?
		}
		else
		{
			Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		}
		WP_ForcePowerStop(self, FP_GRIP);
		WP_ForcePowerStop(self, FP_GRASP);
		return qtrue;
	}

	case HL_BACK_RT:
		dodge_anim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodge_anim = BOTH_DODGE_BL;
		break;
	case HL_BACK_LT:
		dodge_anim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodge_anim = BOTH_DODGE_BR;
		break;
	case HL_BACK:
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280;
		}
		else
		{
			dodge_anim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_R);
		}
		break;
	case HL_CHEST:
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			self->client->ps.forceJumpCharge = 280;
		}
		else
		{
			dodge_anim = BOTH_DODGE_B;
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodge_anim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodge_anim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodge_anim = BOTH_CROUCHDODGE;
		break;
	default:;
	}

	if (dodge_anim != -1)
	{
		int extra_hold_time = 0; //Q_irand( 5, 40 ) * 50;
		if (self->s.number < MAX_CLIENTS)
		{
			//player
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				//in speed
				if (PM_EvasionAnim(self->client->ps.torsoAnim) && !PM_EvasionHoldAnim(self->client->ps.torsoAnim))
				{
					//already in a dodge
					//use the hold pose, don't start it all over again
					dodge_anim = BOTH_DODGE_HOLD_FL + (dodge_anim - BOTH_DODGE_FL);
					extra_hold_time = 200;
				}
			}
		}

		//set the dodge anim we chose
		NPC_SetAnim(self, SETANIM_BOTH, dodge_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD); //type
		G_Sound(self, G_SoundIndex("sound/weapons/melee/swing4.wav"));

		if (extra_hold_time && self->client->ps.torsoAnimTimer < extra_hold_time)
		{
			self->client->ps.torsoAnimTimer += extra_hold_time;
		}
		self->client->ps.legsAnimTimer = self->client->ps.torsoAnimTimer;

		WP_ForcePowerStop(self, FP_GRIP);
		WP_ForcePowerStop(self, FP_GRASP);
		if (!self->enemy && G_ValidEnemy(self, shooter))
		{
			G_SetEnemy(self, shooter);
			if (self->s.number)
			{
				Jedi_Aggression(self, 10);
			}
		}

		return qtrue;
	}
	return qfalse;
}

static evasionType_t jedi_check_flip_evasions(gentity_t* self, const float rightdot)
{
	if (self->NPC && self->NPC->scriptFlags & SCF_NO_ACROBATICS)
	{
		return EVASION_NONE;
	}
	if (self->client)
	{
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
		{
			//boba can't flip
			return EVASION_NONE;
		}
		if (self->client->ps.forceRageRecoveryTime > level.time || self->client->ps.forcePowersActive & 1 <<
			FP_RAGE)
		{
			//no fancy dodges when raging
			return EVASION_NONE;
		}
	}

	if (self->client && (self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim ==
		BOTH_WALL_RUN_RIGHT))
	{
		//already running on a wall
		vec3_t right;
		const vec3_t fwd_angles = { 0, self->client->ps.viewangles[YAW], 0 };
		int anim = -1;

		AngleVectors(fwd_angles, nullptr, right, nullptr);

		const float anim_length = PM_AnimLength(self->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(self->client->ps.legsAnim));
		if (self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0)
		{
			//I'm running on a wall to my left and the attack is on the left
			if (anim_length - self->client->ps.legsAnimTimer > 400
				&& self->client->ps.legsAnimTimer > 400)
			{
				//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if (self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0)
		{
			//I'm running on a wall to my right and the attack is on the right
			if (anim_length - self->client->ps.legsAnimTimer > 400
				&& self->client->ps.legsAnimTimer > 400)
			{
				//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if (anim != -1)
		{
			if (anim == BOTH_WALL_RUN_LEFT_FLIP)
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA(self->client->ps.velocity, 150, right, self->client->ps.velocity);
			}
			else if (anim == BOTH_WALL_RUN_RIGHT_FLIP)
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA(self->client->ps.velocity, -150, right, self->client->ps.velocity);
			}
			int parts = SETANIM_LEGS;
			if (!self->client->ps.weaponTime)
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
			G_AddEvent(self, EV_JUMP, 0);
			return EVASION_OTHER;
		}
	}//desann doesn't do these kind of frilly acrobatics
	else if (self->client->NPC_class != CLASS_DESANN && self->client->NPC_class != CLASS_VADER && Q_irand(0, 1)
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT)
		&& !PM_InRoll(&self->client->ps)
		&& !PM_InKnockDown(&self->client->ps)
		&& !pm_saber_in_special_attack(self->client->ps.torsoAnim))
	{
		vec3_t fwd, right, traceto;
		const vec3_t fwd_angles = { 0, self->client->ps.viewangles[YAW], 0 };
		const vec3_t maxs = { self->maxs[0], self->maxs[1], 24 };
		const vec3_t mins = { self->mins[0], self->mins[1], self->mins[2] + STEPSIZE };
		trace_t trace;

		AngleVectors(fwd_angles, fwd, right, nullptr);

		int parts = SETANIM_BOTH, anim;
		float speed, check_dist;
		qboolean allow_cart_wheels = qtrue;

		if (self->client->ps.weapon == WP_SABER)
		{
			if (self->client->ps.saber[0].saberFlags & SFL_NO_CARTWHEELS)
			{
				allow_cart_wheels = qfalse;
			}
			else if (self->client->ps.dualSabers
				&& self->client->ps.saber[1].saberFlags & SFL_NO_CARTWHEELS)
			{
				allow_cart_wheels = qfalse;
			}
		}
		if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1 || self->NPC->stats.move < 4)
		{
			//can't do no-handed cartwheels without a bit of force jumping ability or teh moves
			allow_cart_wheels = qfalse;
		}

		if (PM_SaberInAttack(self->client->ps.saber_move) || PM_SaberInStart(self->client->ps.saber_move))
		{
			parts = SETANIM_LEGS;
		}
		if (rightdot >= 0)
		{
			if (Q_irand(0, 1))
			{
				anim = BOTH_ARIAL_LEFT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_LEFT;
			}
			check_dist = -128;
			speed = -200;
		}
		else
		{
			if (Q_irand(0, 1))
			{
				anim = BOTH_ARIAL_RIGHT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_RIGHT;
			}
			check_dist = 128;
			speed = 200;
		}
		//trace in the dir that we want to go
		VectorMA(self->currentOrigin, check_dist, right, traceto);
		gi.trace(&trace, self->currentOrigin, mins, maxs, traceto, self->s.number,
			CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP, static_cast<EG2_Collision>(0), 0);

		if (trace.fraction >= 1.0f
			&& self->client->NPC_class == CLASS_ALORA
			|| (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			&& allow_cart_wheels)
		{
			NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.legsAnimTimer; //don't attack again until this anim is done
			vec3_t vec_out;
			vec3_t jump_rt;
			VectorCopy(self->client->ps.viewangles, vec_out);
			vec_out[PITCH] = vec_out[ROLL] = 0;
			//do the flip
			AngleVectors(vec_out, nullptr, jump_rt, nullptr);
			VectorScale(jump_rt, speed, self->client->ps.velocity);
			self->client->ps.forceJumpCharge = 0; //so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.forceJumpZStart = self->currentOrigin[2];
			//so we don't take damage if we land at same height
			self->client->ps.pm_flags |= PMF_JUMPING;
			if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
				|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER)
			{
				G_AddEvent(self, EV_JUMP, 0);
			}
			else
			{
				if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
				{
					//short burst
					G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
				}
				else
				{
					//holding it
					G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
				}
			}
			//ucmd.upmove = 0;
			return EVASION_CARTWHEEL;
		}
		if (!(trace.contents & CONTENTS_BOTCLIP))
		{
			//hit a wall, not a do-not-enter brush
			vec3_t ideal_normal;
			VectorSubtract(self->currentOrigin, traceto, ideal_normal);
			VectorNormalize(ideal_normal);

			const gentity_t* traceEnt = &g_entities[trace.entityNum];

			if (trace.entityNum < ENTITYNUM_WORLD && traceEnt && traceEnt->s.solid != SOLID_BMODEL
				|| DotProduct(trace.plane.normal, ideal_normal) > 0.7f)
			{
				//it's a ent of some sort or it's a wall roughly facing us
				float best_check_dist = 0;
				//hmm, see if we're moving forward
				if (DotProduct(self->client->ps.velocity, fwd) < 200)
				{
					//not running forward very fast
					//check to see if it's okay to move the other way
					if (trace.fraction * check_dist <= 32)
					{
						//wall on that side is close enough to wall-flip off of or wall-run on
						best_check_dist = check_dist;
						check_dist *= -1.0f;
						VectorMA(self->currentOrigin, check_dist, right, traceto);
						//trace in the dir that we want to go
						gi.trace(&trace, self->currentOrigin, mins, maxs, traceto, self->s.number,
							CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP,
							static_cast<EG2_Collision>(0), 0);
						if (trace.fraction >= 1.0f)
						{
							//it's clear, let's do it
							qboolean allow_wall_flips = qtrue;
							if (self->client->ps.weapon == WP_SABER)
							{
								if (self->client->ps.saber[0].saberFlags & SFL_NO_WALL_FLIPS)
								{
									allow_wall_flips = qfalse;
								}
								else if (self->client->ps.dualSabers
									&& self->client->ps.saber[1].saberFlags & SFL_NO_WALL_FLIPS)
								{
									allow_wall_flips = qfalse;
								}
							}
							if (allow_wall_flips)
							{
								if (rightdot > 0)
								{
									anim = BOTH_WALL_FLIP_LEFT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA(self->client->ps.velocity, 150, right, self->client->ps.velocity);
								}
								else
								{
									anim = BOTH_WALL_FLIP_RIGHT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA(self->client->ps.velocity, -150, right, self->client->ps.velocity);
								}
								self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
								//animate me
								int setAnimParts = SETANIM_LEGS;
								if (!self->client->ps.weaponTime)
								{
									setAnimParts = SETANIM_BOTH;
								}
								NPC_SetAnim(self, setAnimParts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								self->client->ps.forceJumpZStart = self->currentOrigin[2];
								//so we don't take damage if we land at same height
								self->client->ps.pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
								if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
									|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
									|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER)
								{
									G_AddEvent(self, EV_JUMP, 0);
								}
								else
								{
									if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
									{
										//short burst
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
									}
									else
									{
										//holding it
										G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
									}
								}
								return EVASION_OTHER;
							}
						}
						else
						{
							//boxed in on both sides
							if (DotProduct(self->client->ps.velocity, fwd) < 0)
							{
								//moving backwards
								return EVASION_NONE;
							}
							if (trace.fraction * check_dist <= 32 && trace.fraction * check_dist < best_check_dist)
							{
								best_check_dist = check_dist;
							}
						}
					}
					else
					{
						//too far from that wall to flip or run off it, check other side
						check_dist *= -1.0f;
						VectorMA(self->currentOrigin, check_dist, right, traceto);
						//trace in the dir that we want to go
						gi.trace(&trace, self->currentOrigin, mins, maxs, traceto, self->s.number,
							CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP,
							static_cast<EG2_Collision>(0), 0);
						if (trace.fraction * check_dist <= 32)
						{
							//wall on this side is close enough
							best_check_dist = check_dist;
						}
						else
						{
							//neither side has a wall within 32
							return EVASION_NONE;
						}
					}
				}
				//Try wall run?
				if (best_check_dist)
				{
					//one of the walls was close enough to wall-run on
					qboolean allow_wall_runs = qtrue;
					if (self->client->ps.weapon == WP_SABER)
					{
						if (self->client->ps.saber[0].saberFlags & SFL_NO_WALL_RUNS)
						{
							allow_wall_runs = qfalse;
						}
						else if (self->client->ps.dualSabers
							&& self->client->ps.saber[1].saberFlags & SFL_NO_WALL_RUNS)
						{
							allow_wall_runs = qfalse;
						}
					}
					if (allow_wall_runs)
					{
						if (best_check_dist > 0)
						{
							//it was to the right
							anim = BOTH_WALL_RUN_RIGHT;
						}
						else
						{
							//it was to the left
							anim = BOTH_WALL_RUN_LEFT;
						}
						self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						//animate me
						int setAnimParts = SETANIM_LEGS;
						if (!self->client->ps.weaponTime)
						{
							setAnimParts = SETANIM_BOTH;
						}
						NPC_SetAnim(self, setAnimParts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						self->client->ps.forceJumpZStart = self->currentOrigin[2];
						//so we don't take damage if we land at same height
						self->client->ps.pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
						if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO
							|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
							|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER)
						{
							G_AddEvent(self, EV_JUMP, 0);
						}
						else
						{
							if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
							{
								//short burst
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
							}
							else
							{
								//holding it
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
							}
						}
						return EVASION_OTHER;
					}
				}
				//else check for wall in front, do backflip off wall
			}
		}
	}
	return EVASION_NONE;
}

extern cvar_t* g_saberRealisticCombat;

int Jedi_ReCalcParryTime(const gentity_t* self, const evasionType_t evasionType)
{
	if (!self->client)
	{
		return 0;
	}
	if (!self->s.number)
	{
		//player blocks how he wants
	}
	else if (self->NPC)
	{
		int base_time;
		if (evasionType == EVASION_DODGE)
		{
			base_time = self->client->ps.torsoAnimTimer;
		}
		else if (evasionType == EVASION_CARTWHEEL)
		{
			base_time = self->client->ps.torsoAnimTimer;
		}
		else if (self->client->ps.saberInFlight)
		{
			base_time = Q_irand(1, 3) * 50;
		}
		else
		{
			switch (g_spskill->integer)
			{
			case 0:
				base_time = 500;
				break;
			case 1:
				base_time = 250;
				break;
			case 2:
			default:
				base_time = 100;
				break;
			}

			base_time = base_time += 400 * Q_irand(1, 2);

			if (evasionType == EVASION_DUCK || evasionType == EVASION_DUCK_PARRY)
			{
				base_time += 100;
			}
			else if (evasionType == EVASION_JUMP || evasionType == EVASION_JUMP_PARRY)
			{
				base_time += 50;
			}
			else if (evasionType == EVASION_OTHER)
			{
				base_time += 100;
			}
			else if (evasionType == EVASION_FJUMP)
			{
				base_time += 100;
			}
		}
		return base_time;
	}
	return 0;
}

qboolean Jedi_SaberBusy(const gentity_t* self)
{
	if (self->client->ps.torsoAnimTimer > 300
		&& (PM_SaberInAttack(self->client->ps.saber_move) && self->client->ps.saberAnimLevel == SS_STRONG
			|| PM_SpinningSaberAnim(self->client->ps.torsoAnim)
			|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
			|| PM_SaberInBrokenParry(self->client->ps.saber_move)
			|| PM_FlippingAnim(self->client->ps.torsoAnim)
			|| PM_RollingAnim(self->client->ps.torsoAnim)))
	{
		//my saber is not in a parrying position
		return qtrue;
	}
	return qfalse;
}

static qboolean Jedi_InNoAIAnim(const gentity_t* self)
{
	if (!self || !self->client)
	{
		//wtf???
		return qtrue;
	}

	if (NPCInfo->rank >= RANK_COMMANDER)
	{
		//boss-level guys can multitask, the rest need to chill out during special moves
		return qfalse;
	}

	if (PM_KickingAnim(NPC->client->ps.legsAnim)
		|| PM_StabDownAnim(NPC->client->ps.legsAnim)
		|| PM_InAirKickingAnim(NPC->client->ps.legsAnim)
		|| PM_InRollIgnoreTimer(&NPC->client->ps)
		|| PM_SaberInKata(static_cast<saber_moveName_t>(NPC->client->ps.saber_move))
		|| PM_SuperBreakWinAnim(NPC->client->ps.torsoAnim)
		|| PM_SuperBreakLoseAnim(NPC->client->ps.torsoAnim))
	{
		return qtrue;
	}

	switch (self->client->ps.legsAnim)
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FLIP_F:
	case BOTH_FLIP_B:
	case BOTH_FLIP_L:
	case BOTH_FLIP_R:
	case BOTH_DODGE_FL:
	case BOTH_DODGE_FR:
	case BOTH_DODGE_BL:
	case BOTH_DODGE_BR:
	case BOTH_DODGE_L:
	case BOTH_DODGE_R:
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_ROLL_STAB:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_SPINATTACKGRIEVOUS:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_SMACK_R:
	case BOTH_SMACK_L:
		return qtrue;
	default:;
	}
	return qfalse;
}

static void Jedi_CheckJumpEvasionSafety(usercmd_t* cmd, const evasionType_t evasionType)
{
	if (evasionType != EVASION_OTHER //not a FlipEvasion, which does it's own safety checks
		&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		//on terra firma right now
		if (NPC->client->ps.velocity[2] > 0
			|| NPC->client->ps.forceJumpCharge
			|| cmd->upmove > 0)
		{
			//going to jump
			if (!NAV_MoveDirSafe(NPC, cmd, NPC->client->ps.speed * 10.0f))
			{
				//we can't jump in the dir we're pushing in
				//cancel the evasion
				NPC->client->ps.velocity[2] = NPC->client->ps.forceJumpCharge = 0;
				cmd->upmove = 0;
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					Com_Printf(S_COLOR_RED"jump not safe, cancelling!");
				}
			}
			else if (NPC->client->ps.velocity[0] || NPC->client->ps.velocity[1])
			{
				//sliding
				vec3_t jump_dir;
				const float jump_dist = VectorNormalize2(NPC->client->ps.velocity, jump_dir);
				if (!NAV_DirSafe(NPC, jump_dir, jump_dist))
				{
					//this jump combined with our momentum would send us into a do not enter brush, so cancel it
					//cancel the evasion
					NPC->client->ps.velocity[2] = NPC->client->ps.forceJumpCharge = 0;
					cmd->upmove = 0;
					if (d_JediAI->integer || g_DebugSaberCombat->integer)
					{
						Com_Printf(S_COLOR_RED"jump not safe, cancelling!\n");
					}
				}
			}
			if (d_JediAI->integer || g_DebugSaberCombat->integer)
			{
				Com_Printf(S_COLOR_GREEN"jump checked, is safe\n");
			}
		}
	}
}

/*
-------------------------
Jedi_SaberBlock

Pick proper block anim

FIXME: Based on difficulty level/enemy saber combat skill, make this decision-making more/less effective

NOTE: always blocking projectiles in this func!

-------------------------
*/
extern qboolean G_FindClosestPointOnLineSegment(const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result);

evasionType_t jedi_saber_block_go(gentity_t* self, usercmd_t* cmd, vec3_t p_hitloc, vec3_t phit_dir,
	const gentity_t* incoming,
	const float dist = 0.0f)
{
	vec3_t hitloc;
	vec3_t hitdir;
	vec3_t diff;
	vec3_t fwdangles = { 0, 0, 0 };
	vec3_t right;
	int duck_chance = 0;
	int dodge_anim = -1;
	qboolean saber_busy = qfalse;
	evasionType_t evasion_type = EVASION_NONE;

	if (!self || !self->client)
	{
		return EVASION_NONE;
	}

	if (PM_LockedAnim(self->client->ps.torsoAnim)
		&& self->client->ps.torsoAnimTimer)
	{
		//Never interrupt these...
		return EVASION_NONE;
	}
	if (PM_InSpecialJump(self->client->ps.legsAnim)
		&& pm_saber_in_special_attack(self->client->ps.torsoAnim))
	{
		return EVASION_NONE;
	}

	if (Jedi_InNoAIAnim(self))
	{
		return EVASION_NONE;
	}

	if (!incoming)
	{
		VectorCopy(p_hitloc, hitloc);
		VectorCopy(phit_dir, hitdir);

		if (self->client->ps.saberInFlight)
		{
			//DOH!  do non-saber evasion!
			saber_busy = qtrue;
		}
		else
		{
			saber_busy = Jedi_SaberBusy(self);
		}
	}
	else
	{
		if (incoming->s.weapon == WP_SABER)
		{
			//flying lightsaber, face it!
		}
		VectorCopy(incoming->currentOrigin, hitloc);
		VectorNormalize2(incoming->s.pos.trDelta, hitdir);
	}

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	fwdangles[1] = self->client->ps.viewangles[1];

	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	qboolean do_dodge = qfalse;
	qboolean always_dodge_or_roll = qfalse;
	qboolean npc_can_dodge = qfalse;

	if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		saber_busy = qtrue;
		do_dodge = qtrue;
		always_dodge_or_roll = qtrue;
	}
	else
	{
		if (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER || self->client->NPC_class ==
			CLASS_SITHLORD && self->s.weapon != WP_SABER)
		{
			saber_busy = qtrue;
			always_dodge_or_roll = qtrue;
		}
		//see if we can dodge if need-be
		if (dist > 16 && (Q_irand(0, 2) || saber_busy)
			|| self->client->ps.saberInFlight
			|| !self->client->ps.SaberActive()
			|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
			|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER)
		{
			//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
			if (self->NPC && (self->client->ps.blockPoints < BLOCKPOINTS_THIRTY
				|| self->client->ps.forcePower < BLOCKPOINTS_HALF))
			{
				//acrobat or fencer or above
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && //on the ground
					!(self->client->ps.pm_flags & PMF_DUCKED) && cmd->upmove >= 0 && TIMER_Done(self, "duck")
					//not ducking
					&& !PM_InRoll(&self->client->ps) //not rolling
					&& !PM_InKnockDown(&self->client->ps) //not knocked down
					&& (self->client->ps.saberInFlight
						|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
						|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
						|| !PM_SaberInAttack(self->client->ps.saber_move) //not attacking
						&& !PM_SaberInStart(self->client->ps.saber_move) //not starting an attack
						&& !PM_SpinningSaberAnim(self->client->ps.torsoAnim) //not in a saber spin
						&& !pm_saber_in_special_attack(self->client->ps.torsoAnim))) //not in a special attack
				{
					//need to check all these because it overrides both torso and legs with the dodge
					do_dodge = qtrue;
				}
			}
		}
	}

	qboolean do_roll = qfalse;

	if ((self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO //boba fett
		|| self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER //boba fett
		|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER) //non-saber reborn (cultist)
		&& !Q_irand(0, 2))
	{
		do_roll = qtrue;
	}

	if (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER
		|| self->client->NPC_class == CLASS_SITHLORD && self->s.weapon != WP_SABER
		|| self->client->NPC_class == CLASS_BOBAFETT
		|| self->client->NPC_class == CLASS_MANDO
		|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive()
		|| self->s.weapon == WP_SABER && self->client->ps.saberInFlight)
	{
		npc_can_dodge = qtrue;
	}

	// Figure out what quadrant the block was in.
	if (d_JediAI->integer || g_DebugSaberCombat->integer)
	{
		gi.Printf("(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time,
			hitloc[2] - self->absmin[2], zdiff, rightdot);
	}

	if (zdiff >= -5) //was 0
	{
		if (incoming || !saber_busy || always_dodge_or_roll)
		{
			if (rightdot > 12
				|| rightdot > 3 && zdiff < 5
				|| !incoming && fabs(hitdir[2]) < 0.25f) //was normalized, 0.3
			{
				//coming from right
				if (do_dodge)
				{
					if (do_roll)
					{
						//roll!
						TIMER_Start(self, "duck", Q_irand(500, 1500));
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
						evasion_type = EVASION_DUCK;
					}
					else if (Q_irand(0, 1))
					{
						if (npc_can_dodge)
						{
							dodge_anim = BOTH_DODGE_FL;
							evasion_type = EVASION_DODGE;
						}
					}
					else
					{
						if (npc_can_dodge)
						{
							dodge_anim = BOTH_DODGE_BL;
							evasion_type = EVASION_DODGE;
						}
					}
				}
				else
				{
					if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
					{
						wp_saber_block_check_random(self, hitloc);

						evasion_type = EVASION_PARRY;
					}
					else
					{
						if (NPCInfo->stats.evasion >= 1)
						{
							dodge_anim = BOTH_ROLL_L;
							self->client->pers.cmd.rightmove = -127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
					}

					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						if (zdiff > 5)
						{
							TIMER_Start(self, "duck", Q_irand(500, 1500));
							evasion_type = EVASION_DUCK_PARRY;
							if (d_JediAI->integer || g_DebugSaberCombat->integer)
							{
								gi.Printf("duck ");
							}
						}
						else
						{
							duck_chance = 6;
						}
					}
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("UR block\n");
				}
			}
			else if (rightdot < -12
				|| rightdot < -3 && zdiff < 5
				|| !incoming && fabs(hitdir[2]) < 0.25f) //was normalized, -0.3
			{
				//coming from left
				if (do_dodge)
				{
					if (do_roll)
					{
						TIMER_Start(self, "duck", Q_irand(500, 1500));
						TIMER_Start(self, "strafeRight", Q_irand(500, 1500));
						TIMER_Set(self, "strafeLeft", 0);
						evasion_type = EVASION_DUCK;
					}
					else if (Q_irand(0, 1))
					{
						if (npc_can_dodge)
						{
							dodge_anim = BOTH_DODGE_FR;
							evasion_type = EVASION_DODGE;
						}
					}
					else
					{
						if (npc_can_dodge)
						{
							dodge_anim = BOTH_DODGE_BR;
							evasion_type = EVASION_DODGE;
						}
					}
				}
				else
				{
					if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
					{
						wp_saber_block_check_random(self, hitloc);

						evasion_type = EVASION_PARRY;
					}
					else
					{
						if (NPCInfo->stats.evasion >= 1)
						{
							dodge_anim = BOTH_ROLL_R;
							self->client->pers.cmd.rightmove = 127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
					}

					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						if (zdiff > 5)
						{
							TIMER_Start(self, "duck", Q_irand(500, 1500));
							evasion_type = EVASION_DUCK_PARRY;
							if (d_JediAI->integer || g_DebugSaberCombat->integer)
							{
								gi.Printf("duck ");
							}
						}
						else
						{
							duck_chance = 6;
						}
					}
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("UL block\n");
				}
			}
			else
			{
				if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
				{
					wp_saber_block_check_random(self, hitloc);

					evasion_type = EVASION_PARRY;
				}
				else
				{
					if (NPCInfo->stats.evasion >= 1)
					{
						dodge_anim = BOTH_ROLL_B;
						self->client->pers.cmd.forwardmove = -127;
						G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
						evasion_type = EVASION_DODGE;
					}
				}

				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					duck_chance = 4;
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("TOP block\n");
				}
			}
		}
		else
		{
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasion_type = EVASION_DUCK;
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("duck ");
				}
			}
		}
	}
	else if (zdiff > -22)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block, so we need to duck
			if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasion_type = EVASION_DUCK;
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("duck ");
				}
			}
			else
			{
				//in air!  Ducking does no good
			}
		}
		if (incoming || !saber_busy || always_dodge_or_roll)
		{
			if (rightdot > 8 || rightdot > 3 && zdiff < -11) //was normalized, 0.2
			{
				if (do_dodge)
				{
					if (do_roll)
					{
						//roll!
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
					}
					else
					{
						if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
							saberInFlight)
						{
							if (self->client->ps.blockPoints > BLOCKPOINTS_FATIGUE)
							{
								wp_saber_block_check_random(self, hitloc);
							}
							else
							{
								dodge_anim = BOTH_DASH_L;
								self->client->pers.cmd.rightmove = -127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/dash.wav");
								evasion_type = EVASION_DODGE;
							}
						}
						else
						{
							if (npc_can_dodge)
							{
								dodge_anim = BOTH_DODGE_L;
								evasion_type = EVASION_DODGE;
							}
						}
					}
				}
				else
				{
					if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
					{
						wp_saber_block_check_random(self, hitloc);

						evasion_type = EVASION_PARRY;
					}
					else
					{
						if (NPCInfo->stats.evasion >= 1)
						{
							if (rightdot > 0)
							{
								dodge_anim = BOTH_ROLL_L;
								self->client->pers.cmd.rightmove = -127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
							else
							{
								dodge_anim = BOTH_ROLL_R;
								self->client->pers.cmd.rightmove = 127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
						}
					}
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("mid-UR block\n");
				}
			}
			else if (rightdot < -8 || rightdot < -3 && zdiff < -11) //was normalized, -0.2
			{
				if (do_dodge)
				{
					if (do_roll)
					{
						//roll!
						TIMER_Start(self, "strafeLeft", Q_irand(500, 1500));
						TIMER_Set(self, "strafeRight", 0);
					}
					else
					{
						if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
							saberInFlight)
						{
							if (self->client->ps.blockPoints > BLOCKPOINTS_FATIGUE)
							{
								wp_saber_block_check_random(self, hitloc);
							}
							else
							{
								dodge_anim = BOTH_DASH_R;
								self->client->pers.cmd.rightmove = 127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/dash.wav");
								evasion_type = EVASION_DODGE;
							}
						}
						else
						{
							if (npc_can_dodge)
							{
								dodge_anim = BOTH_DODGE_R;
								evasion_type = EVASION_DODGE;
							}
						}
					}
				}
				else
				{
					if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
					{
						wp_saber_block_check_random(self, hitloc);

						evasion_type = EVASION_PARRY;
					}
					else
					{
						if (NPCInfo->stats.evasion >= 1)
						{
							if (rightdot > 0)
							{
								dodge_anim = BOTH_ROLL_L;
								self->client->pers.cmd.rightmove = -127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
							else
							{
								dodge_anim = BOTH_ROLL_R;
								self->client->pers.cmd.rightmove = 127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
						}
					}
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("mid-UL block\n");
				}
			}
			else
			{
				if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
				{
					wp_saber_block_check_random(self, hitloc);

					evasion_type = EVASION_PARRY;
				}
				else
				{
					if (NPCInfo->stats.evasion >= 1)
					{
						dodge_anim = BOTH_ROLL_B;
						self->client->pers.cmd.forwardmove = -127;
						G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
						evasion_type = EVASION_DODGE;
					}
				}
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("mid-TOP block\n");
				}
			}
		}
	}
	else
	{
		if (saber_busy || zdiff < -36 && (zdiff < -44 || !Q_irand(0, 2)))
		{
			//jump!
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				//already in air, duck to pull up legs
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				evasion_type = EVASION_DUCK;
				if (incoming && self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
					saberInFlight || !saber_busy)
				{
					wp_saber_block_check_random(self, hitloc);

					evasion_type = EVASION_DUCK_PARRY;
				}
				else
				{
					if (NPCInfo->stats.evasion >= 1)
					{
						if (rightdot > 0)
						{
							dodge_anim = BOTH_ROLL_L;
							self->client->pers.cmd.rightmove = -127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
						else
						{
							dodge_anim = BOTH_ROLL_R;
							self->client->pers.cmd.rightmove = 127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
					}
				}
			}
			else
			{
				//gotta block!
				if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
				{
					wp_saber_block_check_random(self, hitloc);

					evasion_type = EVASION_PARRY;
				}
				else
				{
					if (NPCInfo->stats.evasion >= 1)
					{
						if (rightdot > 0)
						{
							dodge_anim = BOTH_ROLL_L;
							self->client->pers.cmd.rightmove = -127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
						else
						{
							dodge_anim = BOTH_ROLL_R;
							self->client->pers.cmd.rightmove = 127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
					}
				}
				if ((evasion_type = jedi_check_flip_evasions(self, rightdot)) != EVASION_NONE)
				{
					if (d_slowmodeath->integer > 5 && self->enemy && !self->enemy->s.number)
					{
						G_StartStasisEffect(self);
					}
				}
				else if (incoming && self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
					saberInFlight || !saber_busy)
				{
					//since the jump may be cleared if not safe, set a lower block too
					if (rightdot >= 0)
					{
						wp_saber_block_check_random(self, hitloc);
						evasion_type = EVASION_PARRY;
						if (d_JediAI->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf("LR block\n");
						}
					}
					else
					{
						wp_saber_block_check_random(self, hitloc);
						evasion_type = EVASION_PARRY;
						if (d_JediAI->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf("LL block\n");
						}
					}
				}
				else
				{
					if (NPCInfo->stats.evasion >= 1)
					{
						if (rightdot > 0)
						{
							dodge_anim = BOTH_ROLL_L;
							self->client->pers.cmd.rightmove = -127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
						else
						{
							dodge_anim = BOTH_ROLL_R;
							self->client->pers.cmd.rightmove = 127;
							G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
							evasion_type = EVASION_DODGE;
						}
					}
				}
			}
		}
		else
		{
			if (incoming && self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
				saberInFlight || !saber_busy)
			{
				if (rightdot >= 0)
				{
					wp_saber_block_check_random(self, hitloc);
					evasion_type = EVASION_PARRY;
					if (d_JediAI->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf("LR block\n");
					}
				}
				else
				{
					wp_saber_block_check_random(self, hitloc);
					evasion_type = EVASION_PARRY;
					if (d_JediAI->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf("LL block\n");
					}
				}
				if (incoming && incoming->s.weapon == WP_SABER)
				{
					//thrown saber!
					if (self->s.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
					{
						wp_saber_block_check_random(self, hitloc);

						evasion_type = EVASION_PARRY;
					}
					else
					{
						if (NPCInfo->stats.evasion >= 1)
						{
							if (rightdot > 0)
							{
								dodge_anim = BOTH_ROLL_L;
								self->client->pers.cmd.rightmove = -127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
							else
							{
								dodge_anim = BOTH_ROLL_R;
								self->client->pers.cmd.rightmove = 127;
								G_SoundOnEnt(self, CHAN_BODY, "sound/player/roll1.wav");
								evasion_type = EVASION_DODGE;
							}
						}
					}
				}
			}
		}
	}
	if (evasion_type == EVASION_NONE)
	{
		return EVASION_NONE;
	}
	//=======================================================================================
	//see if it's okay to jump
	Jedi_CheckJumpEvasionSafety(cmd, evasion_type);
	//=======================================================================================
	//stop taunting
	TIMER_Set(self, "taunting", 0);
	//stop gripping
	TIMER_Set(self, "gripping", -level.time);
	TIMER_Set(self, "grasping", -level.time);
	WP_ForcePowerStop(self, FP_GRIP);
	WP_ForcePowerStop(self, FP_GRASP);
	//stop draining
	TIMER_Set(self, "draining", -level.time);
	WP_ForcePowerStop(self, FP_DRAIN);

	if (dodge_anim != -1)
	{
		//dodged
		evasion_type = EVASION_DODGE;
		NPC_SetAnim(self, SETANIM_BOTH, dodge_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoAnimTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
		if (d_slowmodeath->integer > 5 && self->enemy && !self->enemy->s.number)
		{
			G_StartStasisEffect(self);
		}
	}
	else
	{
		if (duck_chance)
		{
			if (!Q_irand(0, duck_chance))
			{
				TIMER_Start(self, "duck", Q_irand(500, 1500));
				if (evasion_type == EVASION_PARRY)
				{
					evasion_type = EVASION_DUCK_PARRY;
				}
				else
				{
					evasion_type = EVASION_DUCK;
				}
			}
		}

		if (incoming)
		{
			self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	const int parry_re_calc_time = Jedi_ReCalcParryTime(self, evasion_type);
	if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parry_re_calc_time)
	{
		self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parry_re_calc_time;
	}
	return evasion_type;
}

static evasionType_t Jedi_CheckEvadeSpecialAttacks()
{
	if (!NPC
		|| !NPC->client)
	{
		return EVASION_NONE;
	}

	if (!NPC->enemy
		|| NPC->enemy->health <= 0
		|| !NPC->enemy->client)
	{
		//don't keep blocking him once he's dead (or if not a client)
		return EVASION_NONE;
	}

	if (NPC->enemy->s.number >= MAX_CLIENTS)
	{
		//only do these against player
		return EVASION_NONE;
	}

	if (!TIMER_Done(NPC, "specialEvasion"))
	{
		//still evading from last time
		return EVASION_NONE;
	}

	if (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
		|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7
		|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS)
	{
		//back away from these
		if (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
			|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
			|| NPC->client->NPC_class == CLASS_ALORA
			|| Q_irand(0, NPCInfo->rank) > RANK_LT_JG)
		{
			//see if we should back off
			if (in_front(NPC->currentOrigin, NPC->enemy->currentOrigin, NPC->enemy->currentAngles))
			{
				//facing me
				float min_safe_dist_sq = NPC->maxs[0] * 1.5f + NPC->enemy->maxs[0] * 1.5f + NPC->enemy->client->ps.
					SaberLength() + 24.0f;
				min_safe_dist_sq *= min_safe_dist_sq;
				if (DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin) < min_safe_dist_sq)
				{
					//back off!
					Jedi_StartBackOff();
					return EVASION_OTHER;
				}
			}
		}
	}
	else
	{
		//check some other attacks?
		//check roll-stab
		if (NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB
			|| (NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F || NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F1 ||
				NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F2
				&& (NPC->enemy->client->pers.lastCommand.buttons & BUTTON_ATTACK || NPC->enemy->client->ps.pm_flags &
					PMF_ATTACK_HELD)))
		{
			//either already in a roll-stab or may go into one
			if (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| Q_irand(-3, NPCInfo->rank) > RANK_LT_JG)
			{
				//see if we should evade
				vec3_t yaw_only_angles = { 0, NPC->enemy->currentAngles[YAW], 0 };
				if (in_front(NPC->currentOrigin, NPC->enemy->currentOrigin, yaw_only_angles, 0.25f))
				{
					//facing me
					float min_safe_dist_sq = NPC->maxs[0] * 1.5f + NPC->enemy->maxs[0] * 1.5f + NPC->enemy->client->ps.
						SaberLength() + 24.0f;
					min_safe_dist_sq *= min_safe_dist_sq;
					const float dist_sq = DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);
					if (dist_sq < min_safe_dist_sq)
					{
						//evade!
						auto do_jump = static_cast<qboolean>(NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB ||
							dist_sq
							< 3000.0f); //not much time left, just jump!
						if (NPCInfo->scriptFlags & SCF_NO_ACROBATICS
							|| !do_jump)
						{
							//roll?
							vec3_t enemy_right, dir2_me;

							AngleVectors(yaw_only_angles, nullptr, enemy_right, nullptr);
							VectorSubtract(NPC->currentOrigin, NPC->enemy->currentOrigin, dir2_me);
							VectorNormalize(dir2_me);
							const float dot = DotProduct(enemy_right, dir2_me);

							ucmd.forwardmove = 0;
							TIMER_Start(NPC, "duck", Q_irand(500, 1500));
							ucmd.upmove = -127;
							//NOTE: this *assumes* I'm facing him!
							if (dot > 0)
							{
								//I'm to his right
								if (!NPC_MoveDirClear(0, -127, qfalse))
								{
									//fuck, jump instead
									do_jump = qtrue;
								}
								else
								{
									TIMER_Start(NPC, "strafeLeft", Q_irand(500, 1500));
									TIMER_Set(NPC, "strafeRight", 0);
									ucmd.rightmove = -127;
									if (d_JediAI->integer || g_DebugSaberCombat->integer)
									{
										Com_Printf("%s rolling left from roll-stab!\n", NPC->NPC_type);
									}
									if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
									{
										//fuck it, just force it
										NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ROLL_L,
											SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
										G_AddEvent(NPC, EV_ROLL, 0);
										NPC->client->ps.saber_move = LS_NONE;
									}
								}
							}
							else
							{
								//I'm to his left
								if (!NPC_MoveDirClear(0, 127, qfalse))
								{
									//fuck, jump instead
									do_jump = qtrue;
								}
								else
								{
									TIMER_Start(NPC, "strafeRight", Q_irand(500, 1500));
									TIMER_Set(NPC, "strafeLeft", 0);
									ucmd.rightmove = 127;
									if (d_JediAI->integer || g_DebugSaberCombat->integer)
									{
										Com_Printf("%s rolling right from roll-stab!\n", NPC->NPC_type);
									}
									if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
									{
										//fuck it, just force it
										NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ROLL_R,
											SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
										G_AddEvent(NPC, EV_ROLL, 0);
										NPC->client->ps.saber_move = LS_NONE;
									}
								}
							}
							if (!do_jump)
							{
								TIMER_Set(NPC, "specialEvasion", 3000);
								return EVASION_DUCK;
							}
						}
						//didn't roll, do jump
						if (NPC->s.weapon != WP_SABER
							|| NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER
							|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
							|| NPC->client->NPC_class == CLASS_ALORA
							|| Q_irand(-3, NPCInfo->rank) > RANK_CREWMAN)
						{
							//superjump
							NPC->client->ps.forceJumpCharge = 320;

							if (Q_irand(0, 2))
							{
								//make it a backflip
								ucmd.forwardmove = -127;
								TIMER_Set(NPC, "roamTime", -level.time);
								TIMER_Set(NPC, "strafeLeft", -level.time);
								TIMER_Set(NPC, "strafeRight", -level.time);
								TIMER_Set(NPC, "walking", -level.time);
								TIMER_Set(NPC, "moveforward", -level.time);
								TIMER_Set(NPC, "movenone", -level.time);
								TIMER_Set(NPC, "moveright", -level.time);
								TIMER_Set(NPC, "moveleft", -level.time);
								TIMER_Set(NPC, "movecenter", -level.time);
								TIMER_Set(NPC, "moveback", Q_irand(500, 1000));
								if (d_JediAI->integer || g_DebugSaberCombat->integer)
								{
									Com_Printf("%s backflipping from roll-stab!\n", NPC->NPC_type);
								}
							}
							else
							{
								if (d_JediAI->integer || g_DebugSaberCombat->integer)
								{
									Com_Printf("%s force-jumping over roll-stab!\n", NPC->NPC_type);
								}
							}
							TIMER_Set(NPC, "specialEvasion", 3000);
							return EVASION_FJUMP;
						}
						//normal jump
						Jedi_SaberBlock();
					}
				}
			}
		}
	}
	return EVASION_NONE;
}

extern int wp_debug_saber_colour(saber_colors_t saber_color);

static qboolean Jedi_SaberBlock()
{
	vec3_t hitloc, saber_tip_old, saber_tip, top, bottom, axis_point, saber_point, dir;
	vec3_t point_dir, base_dir, tip_dir;
	constexpr vec3_t saber_mins = { -4, -4, -4 };
	constexpr vec3_t saber_maxs = { 4, 4, 4 };
	float base_dir_perc;
	float dist, best_dist = Q3_INFINITE;
	int closest_saber_num = 0, closest_blade_num = 0;

	if (!TIMER_Done(NPC, "parryReCalcTime"))
	{
		//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if (NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] > level.time)
	{
		//can't move the saber to another position yet
		return qfalse;
	}

	if (NPC->enemy->health <= 0 || !NPC->enemy->client)
	{
		//don't keep blocking him once he's dead (or if not a client)
		return qfalse;
	}

	for (int saberNum = 0; saberNum < MAX_SABERS; saberNum++)
	{
		for (int blade_num = 0; blade_num < NPC->enemy->client->ps.saber[saberNum].numBlades; blade_num++)
		{
			if (NPC->enemy->client->ps.saber[saberNum].type != SABER_NONE
				&& NPC->enemy->client->ps.saber[saberNum].blade[blade_num].length > 0)
			{
				//valid saber and this blade is on
				VectorMA(NPC->enemy->client->ps.saber[saberNum].blade[blade_num].muzzlePointOld,
					NPC->enemy->client->ps.saber[saberNum].blade[blade_num].length,
					NPC->enemy->client->ps.saber[saberNum].blade[blade_num].muzzleDirOld, saber_tip_old);
				VectorMA(NPC->enemy->client->ps.saber[saberNum].blade[blade_num].muzzlePoint,
					NPC->enemy->client->ps.saber[saberNum].blade[blade_num].length,
					NPC->enemy->client->ps.saber[saberNum].blade[blade_num].muzzleDir, saber_tip);

				VectorCopy(NPC->currentOrigin, top);
				top[2] = NPC->absmax[2];
				VectorCopy(NPC->currentOrigin, bottom);
				bottom[2] = NPC->absmin[2];

				dist = ShortestLineSegBewteen2LineSegs(
					NPC->enemy->client->ps.saber[saberNum].blade[blade_num].muzzlePoint, saber_tip, bottom, top,
					saber_point, axis_point);
				if (dist < best_dist)
				{
					best_dist = dist;
					closest_saber_num = saberNum;
					closest_blade_num = blade_num;
				}
			}
		}
	}

	if (best_dist > NPC->maxs[0] * 5)
	{
		//FIXME: sometimes he reacts when you're too far away to actually hit him
		if (d_JediAI->integer || g_DebugSaberCombat->integer)
		{
			Com_Printf(S_COLOR_RED"enemy saber dist: %4.2f\n", best_dist);
		}
		TIMER_Set(NPC, "parryTime", -1);
		return qfalse;
	}

	dist = best_dist;

	if (d_JediAI->integer || g_DebugSaberCombat->integer)
	{
		Com_Printf(S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist);
	}

	//now use the closest blade for my evasion check
	VectorMA(NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePointOld,
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].length,
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzleDirOld, saber_tip_old);
	VectorMA(NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePoint,
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].length,
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzleDir, saber_tip);

	VectorCopy(NPC->currentOrigin, top);
	top[2] = NPC->absmax[2];
	VectorCopy(NPC->currentOrigin, bottom);
	bottom[2] = NPC->absmin[2];

	dist = ShortestLineSegBewteen2LineSegs(
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePoint, saber_tip, bottom, top,
		saber_point, axis_point);
	VectorSubtract(saber_point, NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePoint,
		point_dir);
	const float point_dist = VectorLength(point_dir);

	if (NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].length <= 0)
	{
		base_dir_perc = 0.5f;
	}
	else
	{
		base_dir_perc = point_dist / NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].length;
	}
	VectorSubtract(NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePoint,
		NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].muzzlePointOld, base_dir);
	VectorSubtract(saber_tip, saber_tip_old, tip_dir);
	VectorScale(base_dir, base_dir_perc, base_dir);
	VectorMA(base_dir, 1.0f - base_dir_perc, tip_dir, dir);
	VectorMA(saber_point, 200, dir, hitloc);

	//get the actual point of impact
	trace_t tr;
	gi.trace(&tr, saber_point, saber_mins, saber_maxs, hitloc, NPC->enemy->s.number, CONTENTS_BODY,
		static_cast<EG2_Collision>(0), 0); //, G2_RETURNONHIT, 10 );
	if (tr.allsolid || tr.startsolid || tr.fraction >= 1.0f)
	{
		vec3_t saber_hit_point;
		//estimate
		vec3_t dir2_me;
		VectorSubtract(axis_point, saber_point, dir2_me);
		dist = VectorNormalize(dir2_me);
		if (DotProduct(dir, dir2_me) < 0.2f)
		{
			//saber is not swinging in my direction
			TIMER_Set(NPC, "parryTime", -1);
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs(saber_point, hitloc, bottom, top, saber_hit_point, hitloc);
	}
	else
	{
		VectorCopy(tr.endpos, hitloc);
	}

	if (d_JediAI->integer || g_DebugSaberCombat->integer)
	{
		G_DebugLine(saber_point, hitloc, FRAMETIME,
			wp_debug_saber_colour(
				NPC->enemy->client->ps.saber[closest_saber_num].blade[closest_blade_num].color));
	}
	evasionType_t evasion_type;

	if ((evasion_type = jedi_saber_block_go(NPC, &ucmd, hitloc, dir, nullptr, dist)) != EVASION_NONE)
	{
		//did some sort of evasion
		if (evasion_type != EVASION_DODGE)
		{
			//(not dodge)
			if (!NPC->client->ps.saberInFlight)
			{
				//make sure saber is on
				NPC->client->ps.SaberActivate();
			}

			//debounce our parry recalc time
			const int parry_re_calc_time = Jedi_ReCalcParryTime(NPC, evasion_type);

			TIMER_Set(NPC, "parryReCalcTime", Q_irand(0, parry_re_calc_time));

			if (d_JediAI->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf("Keep parry choice until: %d\n", level.time + parry_re_calc_time);
			}

			//determine how long to hold this anim
			if (TIMER_Done(NPC, "parryTime"))
			{
				TIMER_Set(NPC, "parryTime", level.time + parry_re_calc_time);
			}
		}
		else
		{
			//dodged
			int dodge_time = NPC->client->ps.torsoAnimTimer;
			if (NPCInfo->rank > RANK_LT_COMM && NPC->client->NPC_class != CLASS_SITHLORD && NPC->client->NPC_class !=
				CLASS_DESANN && NPC->client->NPC_class != CLASS_VADER)
			{
				//higher-level guys can dodge faster
				dodge_time -= 200;
			}
			TIMER_Set(NPC, "parryReCalcTime", dodge_time);
			TIMER_Set(NPC, "parryTime", dodge_time);
		}
	}
	if (evasion_type != EVASION_DUCK_PARRY
		&& evasion_type != EVASION_JUMP_PARRY
		&& evasion_type != EVASION_JUMP
		&& evasion_type != EVASION_DUCK
		&& evasion_type != EVASION_FJUMP)
	{
		if (Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE)
		{
			//got a new evasion!
			//see if it's okay to jump
			Jedi_CheckJumpEvasionSafety(&ucmd, evasion_type);
		}
	}
	return qtrue;
}

extern qboolean NPC_CheckFallPositionOK(const gentity_t* NPC, vec3_t position);

static qboolean Jedi_EvasionRoll(gentity_t* ai_ent)
{
	if (ai_ent->client->NPC_class == CLASS_STORMTROOPER)
	{
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}
	if (!ai_ent->enemy->client)
	{
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}
	if (ai_ent->enemy->client
		&& ai_ent->enemy->s.weapon == WP_SABER
		&& ai_ent->enemy->client->ps.saberLockTime > level.time)
	{
		//don't try to block/evade an enemy who is in a saberLock
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}
	if (ai_ent->client->ps.saberEventFlags & SEF_LOCK_WON && ai_ent->enemy->painDebounceTime > level.time)
	{
		//pressing the advantage of winning a saber lock
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}

	if (ai_ent->npc_roll_time >= level.time)
	{
		// Already in a roll...
		ai_ent->npc_roll_start = qfalse;
		return qfalse;
	}

	// Init...
	ai_ent->npc_roll_direction = EVASION_ROLL_DIR_NONE;
	ai_ent->npc_roll_start = qfalse;

	qboolean can_roll_back = qfalse;
	qboolean can_roll_left = qfalse;
	qboolean can_roll_right = qfalse;

	trace_t tr;
	vec3_t fwd, right, up, start, end;
	AngleVectors(ai_ent->currentAngles, fwd, right, up);

	VectorSet(start, ai_ent->currentOrigin[0], ai_ent->currentOrigin[1], ai_ent->currentOrigin[2] + 24.0);
	VectorMA(start, -128, fwd, end);
	gi.trace(&tr, start, nullptr, nullptr, end, ai_ent->s.number, MASK_NPCSOLID, static_cast<EG2_Collision>(0), 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		can_roll_back = qtrue;
	}

	VectorMA(start, -128, right, end);
	gi.trace(&tr, start, nullptr, nullptr, end, ai_ent->s.number, MASK_NPCSOLID, static_cast<EG2_Collision>(0), 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		can_roll_left = qtrue;
	}

	VectorMA(start, 128, right, end);
	gi.trace(&tr, start, nullptr, nullptr, end, ai_ent->s.number, MASK_NPCSOLID, static_cast<EG2_Collision>(0), 0);

	if (tr.fraction == 1.0 && !NPC_CheckFallPositionOK(ai_ent, end))
	{
		// We can roll back...
		can_roll_right = qtrue;
	}

	if (can_roll_back && can_roll_left && can_roll_right)
	{
		const int choice = Q_irand(0, 2);

		switch (choice)
		{
		case 2:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (can_roll_back && can_roll_left)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (can_roll_back && can_roll_right)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
			break;
		}

		return qtrue;
	}
	if (can_roll_left && can_roll_right)
	{
		const int choice = Q_irand(0, 1);

		switch (choice)
		{
		case 1:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
			break;
		default:
			ai_ent->npc_roll_time = level.time + 5000;
			ai_ent->npc_roll_start = qtrue;
			ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
			break;
		}

		return qtrue;
	}
	if (can_roll_left)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_LEFT;
		return qtrue;
	}
	if (can_roll_right)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_RIGHT;
		return qtrue;
	}
	if (can_roll_back)
	{
		ai_ent->npc_roll_time = level.time + 5000;
		ai_ent->npc_roll_start = qtrue;
		ai_ent->npc_roll_direction = EVASION_ROLL_DIR_BACK;
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
Jedi_EvasionSaber

defend if other is using saber and attacking me!
-------------------------
*/
static void Jedi_EvasionSaber(vec3_t enemy_movedir, const float enemy_dist, vec3_t enemy_dir)
{
	vec3_t dir_enemy2_me;
	int evasion_chance = 30; //only step aside 30% if he's moving at me but not attacking
	qboolean enemy_attacking = qfalse;
	qboolean throwing_saber = qfalse;
	qboolean shooting_lightning = qfalse;

	if (!NPC->enemy->client)
	{
		return;
	}
	if (NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time)
	{
		//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	if (NPC->client->ps.saberEventFlags & SEF_LOCK_WON && NPC->enemy->painDebounceTime > level.time)
	{
		//pressing the advantage of winning a saber lock
		return;
	}

	if (NPC->enemy->client->ps.saberInFlight && !TIMER_Done(NPC, "taunting"))
	{
		//if he's throwing his saber, stop taunting
		TIMER_Set(NPC, "taunting", -level.time);
		if (!NPC->client->ps.saberInFlight)
		{
			NPC->client->ps.SaberActivate();
		}
	}

	if (TIMER_Done(NPC, "parryTime"))
	{
		if (NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
		{
			//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if (NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING)
	{
		if ((!NPC->client->ps.saberInFlight ||
			NPC->client->ps.dualSabers &&
			NPC->client->ps.saber[1].Active())
			&& Jedi_SaberBlock())
		{
			return;
		}
	}
	else if (Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE)
	{
		return;
	}

	VectorSubtract(NPC->currentOrigin, NPC->enemy->currentOrigin, dir_enemy2_me);
	VectorNormalize(dir_enemy2_me);

	if (NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING)
	{
		//enemy is attacking
		enemy_attacking = qtrue;
		evasion_chance = 90;
	}

	if (NPC->enemy->client->ps.forcePowersActive & 1 << FP_LIGHTNING || NPC->enemy->client->ps.forcePowersActive &
		1 << FP_LIGHTNING_STRIKE)
	{
		//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasion_chance = 50;
	}

	if (NPC->enemy->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
	{
		//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasion_chance = 80;
	}

	if (NPC->enemy->client->ps.saberInFlight
		&& NPC->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE
		&& NPC->enemy->client->ps.saberEntityState != SES_RETURNING)
	{
		//enemy is shooting lightning
		enemy_attacking = qtrue;
		throwing_saber = qtrue;
	}

	if (Q_irand(0, 100) < evasion_chance)
	{
		//check to see if he's coming at me
		float facing_amt;
		if (VectorCompare(enemy_movedir, vec3_origin) || shooting_lightning || throwing_saber)
		{
			//he's not moving (or he's using a ranged attack), see if he's facing me
			vec3_t enemy_fwd;
			AngleVectors(NPC->enemy->client->ps.viewangles, enemy_fwd, nullptr, nullptr);
			facing_amt = DotProduct(enemy_fwd, dir_enemy2_me);
		}
		else
		{
			//he's moving
			facing_amt = DotProduct(enemy_movedir, dir_enemy2_me);
		}

		if (Q_flrand(0.25, 1) < facing_amt)
		{
			//coming at/facing me!
			int which_defense = 0;

			if (NPC->client->ps.weaponTime
				|| NPC->client->ps.saberInFlight
				|| (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO)
				|| NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER)
			{
				//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if (Q_irand(0, 10) < NPCInfo->stats.aggression)
				{
					return;
				}
				which_defense = 100;
			}
			else
			{
				if (shooting_lightning)
				{
					//check for lightning attack
					//only valid defense is strafe and/or jump
					which_defense = 100;
					if (NPC->s.weapon == WP_SABER)
					{
						Jedi_SaberBlock();
					}
					else
					{
						//already chose one
					}
				}
				else if (throwing_saber)
				{
					vec3_t saber_dir2_me;
					vec3_t saber_move_dir;
					const gentity_t* saber = &g_entities[NPC->enemy->client->ps.saberEntityNum];
					VectorSubtract(NPC->currentOrigin, saber->currentOrigin, saber_dir2_me);
					const float saber_dist = VectorNormalize(saber_dir2_me);
					VectorCopy(saber->s.pos.trDelta, saber_move_dir);
					VectorNormalize(saber_move_dir);
					if (!Q_irand(0, 3))
					{
						Jedi_Aggression(NPC, 1);
					}
					if (DotProduct(saber_move_dir, saber_dir2_me) > 0.5)
					{
						//it's heading towards me
						if (saber_dist < 100)
						{
							//it's close
							which_defense = Q_irand(3, 6);
						}
						else if (saber_dist < 200)
						{
							//got some time, yet, try pushing
							which_defense = Q_irand(0, 8);
						}
					}
				}

				if (which_defense)
				{
					if (NPC->s.weapon == WP_SABER)
					{
						Jedi_SaberBlock();
					}
					else
					{
						//already chose one
					}
				}
				else if (enemy_dist > 80 || !enemy_attacking)
				{
					//he's pretty far, or not swinging, just strafe
					if (VectorCompare(enemy_movedir, vec3_origin))
					{
						//if he's not moving, not swinging and far enough away, no evasion necc.
						return;
					}
					if (Q_irand(0, 10) < NPCInfo->stats.aggression)
					{
						return;
					}
					which_defense = 100;
				}
				else
				{
					//he's getting close and swinging at me
					vec3_t fwd;
					//see if I'm facing him
					AngleVectors(NPC->client->ps.viewangles, fwd, nullptr, nullptr);
					if (DotProduct(enemy_dir, fwd) < 0.5)
					{
						//I'm not really facing him, best option is to strafe
						which_defense = Q_irand(5, 16);
					}
					else if (enemy_dist < 56)
					{
						//he's very close, maybe we should be more inclined to block or throw
						which_defense = Q_irand(Q_min(NPCInfo->stats.aggression, 24), 24);
					}
					else
					{
						which_defense = Q_irand(2, 16);
					}
				}
			}

			if (which_defense >= 4 && which_defense <= 12)
			{
				//would try to block
				if (NPC->client->ps.saberInFlight)
				{
					//can't, saber in not in hand, so fall back to strafe/jump
					which_defense = 100;
				}
			}

			switch (which_defense)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				//use jedi force push? or kick?
				if (Jedi_DecideKick() //let's try a kick
					&& (g_pick_auto_multi_kick(NPC, qfalse, qtrue) != LS_NONE
						|| G_CanKickEntity(NPC, NPC->enemy)
						&& G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE))
				{
					//kicked
					TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
				}
				else if ((NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && TIMER_Done(NPC, "parryTime"))
				{
					ForceThrow(NPC, qfalse);
				}
				else if (!Jedi_SaberBlock())
				{
					Jedi_EvasionRoll(NPC);
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				Jedi_SaberBlock();
				break;
			default:
				//Evade!
				if (!Q_irand(0, 5) || !jedi_strafe(300, 1000, 0, 1000, qfalse))
				{
					if (Jedi_DecideKick()
						&& G_CanKickEntity(NPC, NPC->enemy)
						&& G_PickAutoKick(NPC, NPC->enemy, qtrue) != LS_NONE)
					{
						//kicked!
						TIMER_Set(NPC, "kickDebounce", Q_irand(20000, 30000));
					}
					else if (shooting_lightning || throwing_saber || enemy_dist < 80)
					{
						if (shooting_lightning || !Q_irand(0, 2) && NPCInfo->stats.aggression < 4 && TIMER_Done(
							NPC, "parryTime"))
						{
							if ((NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && !shooting_lightning &&
								Q_irand(0, 2))
							{
								ForceThrow(NPC, qfalse);
							}
							else if ((NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG)
								&& !(NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
								&& NPC->client->ps.forcePower > 80
								&& NPC->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2
								&& NPC->client->ps.forceRageRecoveryTime < level.time
								&& !(NPC->client->ps.forcePowersActive & 1 << FP_RAGE)
								&& !PM_InKnockDown(&NPC->client->ps))
							{
								if (NPC->s.weapon != WP_SABER)
								{
									//i dont have a saber can only try to jump right now
									NPC->client->ps.forceJumpCharge = 480;
									TIMER_Set(NPC, "jumpChaseDebounce", Q_irand(10000, 15000));

									if (Q_irand(0, 2))
									{
										ucmd.forwardmove = 127;
										VectorClear(NPC->client->ps.moveDir);
									}
									else
									{
										ucmd.forwardmove = -127;
										VectorClear(NPC->client->ps.moveDir);
									}
								}
								else
								{
									Jedi_SaberBlock();
								}
							}
							else
							{
								if (!Jedi_SaberBlock())
								{
									Jedi_EvasionRoll(NPC);
								}
							}
						}
						else if (enemy_attacking)
						{
							Jedi_SaberBlock();
						}
					}
					else if (!Jedi_SaberBlock())
					{
						Jedi_EvasionRoll(NPC);
					}
				}
				else if (!Jedi_SaberBlock())
				{
					Jedi_EvasionRoll(NPC);
				}
				else
				{
					//strafed
					if (d_JediAI->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf("def strafe\n");
					}
					if (!(NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
						&& NPC->client->ps.forceRageRecoveryTime < level.time
						&& !(NPC->client->ps.forcePowersActive & 1 << FP_RAGE)
						&& (NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG)
						&& !PM_InKnockDown(&NPC->client->ps)
						&& !Q_irand(0, 5))
					{
						if (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO
							|| NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER
							|| NPC->client->NPC_class == CLASS_ROCKETTROOPER)
						{
							NPC->client->ps.forceJumpCharge = 280; //FIXME: calc this intelligently?
						}
						else
						{
							if (NPC->s.weapon == WP_SABER)
							{
								Jedi_SaberBlock();
							}
							else
							{
								NPC->client->ps.forceJumpCharge = 320;
							}
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set(NPC, "jumpChaseDebounce", Q_irand(2000, 5000));
					}
					else if (!Jedi_SaberBlock())
					{
						Jedi_EvasionRoll(NPC);
					}
				}
				break;
			}

			//turn off slow walking no matter what
			TIMER_Set(NPC, "walking", -level.time);
			TIMER_Set(NPC, "taunting", -level.time);
		}
	}
}

/*
==========================================================================================
INTERNAL AI ROUTINES
==========================================================================================
*/
gentity_t* jedi_find_enemy_in_cone(const gentity_t* self, gentity_t* fallback, const float minDot)
{
	vec3_t forward, mins{}, maxs{};
	gentity_t* enemy = fallback;
	gentity_t* entity_list[MAX_GENTITIES];
	int e;
	trace_t tr;

	if (!self->client)
	{
		return enemy;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);

	for (e = 0; e < 3; e++)
	{
		mins[e] = self->currentOrigin[e] - 1024;
		maxs[e] = self->currentOrigin[e] + 1024;
	}
	const int num_listed_entities = gi.EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

	for (e = 0; e < num_listed_entities; e++)
	{
		constexpr float best_dist = Q3_INFINITE;
		vec3_t dir;
		gentity_t* check = entity_list[e];
		if (check == self)
		{
			//me
			continue;
		}
		if (!check->inuse)
		{
			//freed
			continue;
		}
		if (!check->client)
		{
			//not a client - FIXME: what about turrets?
			continue;
		}
		if (check->client->playerTeam != self->client->enemyTeam)
		{
			//not an enemy - FIXME: what about turrets?
			continue;
		}
		if (check->health <= 0)
		{
			//dead
			continue;
		}

		if (!gi.inPVS(check->currentOrigin, self->currentOrigin))
		{
			//can't potentially see them
			continue;
		}

		VectorSubtract(check->currentOrigin, self->currentOrigin, dir);
		const float dist = VectorNormalize(dir);

		if (DotProduct(dir, forward) < minDot)
		{
			//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		gi.trace(&tr, self->currentOrigin, vec3_origin, vec3_origin, check->currentOrigin, self->s.number, MASK_SHOT,
			static_cast<EG2_Collision>(0), 0);
		if (tr.fraction < 1.0f && tr.entityNum != check->s.number)
		{
			//must have clear shot
			continue;
		}

		if (dist < best_dist)
		{
			enemy = check;
		}
	}
	return enemy;
}

void jedi_set_enemy_info(vec3_t enemy_dest, vec3_t enemy_dir, float* enemy_dist, vec3_t enemy_movedir,
	float* enemy_movespeed, const int prediction)
{
	if (!NPC || !NPC->enemy)
	{
		//no valid enemy
		return;
	}
	if (!NPC->enemy->client)
	{
		VectorClear(enemy_movedir);
		*enemy_movespeed = 0;
		VectorCopy(NPC->enemy->currentOrigin, enemy_dest);
		enemy_dest[2] += NPC->enemy->mins[2] + 24; //get it's origin to a height I can work with
		VectorSubtract(enemy_dest, NPC->currentOrigin, enemy_dir);
		*enemy_dist = VectorNormalize(enemy_dir);
	}
	else
	{
		//see where enemy is headed
		VectorCopy(NPC->enemy->client->ps.velocity, enemy_movedir);
		*enemy_movespeed = VectorNormalize(enemy_movedir);
		//figure out where he'll be, say, 3 frames from now
		VectorMA(NPC->enemy->currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest);
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract(enemy_dest, NPC->currentOrigin, enemy_dir); //NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize(enemy_dir) - (NPC->client->ps.SaberLengthMax() + NPC->maxs[0] * 1.5 + 16);
	}
	//init this to false
	enemy_in_striking_range = qfalse;
	if (*enemy_dist <= 0.0f)
	{
		enemy_in_striking_range = qtrue;
	}
	else
	{
		//if he's too far away, see if he's at least facing us or coming towards us
		if (*enemy_dist <= 32.0f)
		{
			//has to be facing us
			vec3_t e_angles = { 0, NPC->currentAngles[YAW], 0 };
			if (InFOV(NPC->currentOrigin, NPC->enemy->currentOrigin, e_angles, 30, 90))
			{
				//in striking range
				enemy_in_striking_range = qtrue;
			}
		}
		if (*enemy_dist >= 64.0f)
		{
			//we have to be approaching each other
			float v_dot;
			if (!VectorCompare(NPC->client->ps.velocity, vec3_origin))
			{
				//I am moving, see if I'm moving toward the enemy
				vec3_t e_dir;
				VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, e_dir);
				VectorNormalize(e_dir);
				v_dot = DotProduct(e_dir, NPC->client->ps.velocity);
			}
			else if (NPC->enemy->client && !VectorCompare(NPC->enemy->client->ps.velocity, vec3_origin))
			{
				//I'm not moving, but the enemy is, see if he's moving towards me
				vec3_t me_dir;
				VectorSubtract(NPC->currentOrigin, NPC->enemy->currentOrigin, me_dir);
				VectorNormalize(me_dir);
				v_dot = DotProduct(me_dir, NPC->enemy->client->ps.velocity);
			}
			else
			{
				//neither of us is moving, below check will fail, so just return
				return;
			}
			if (v_dot >= *enemy_dist)
			{
				//moving towards each other
				enemy_in_striking_range = qtrue;
			}
		}
	}
}

void NPC_EvasionSaber()
{
	if (ucmd.upmove <= 0 //not jumping
		&& (!ucmd.upmove || !ucmd.rightmove)) //either just ducking or just strafing (i.e.: not rolling
	{
		//see if we need to avoid their saber
		vec3_t enemy_dir, enemy_movedir, enemy_dest;
		float enemy_dist, enemy_movespeed;
		//set enemy
		jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
}

extern float WP_SpeedOfMissileForWeapon(int wp, qboolean alt_fire);

static void Jedi_FaceEnemy(const qboolean doPitch)
{
	vec3_t enemy_eyes, eyes, angles{};

	if (NPC == nullptr)
		return;

	if (NPC->enemy == nullptr)
		return;

	if (NPC->client->ps.forcePowersActive & 1 << FP_GRIP &&
		NPC->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
	{
		//don't update?
		NPCInfo->desiredPitch = NPC->client->ps.viewangles[PITCH];
		NPCInfo->desiredYaw = NPC->client->ps.viewangles[YAW];
		return;
	}
	if (NPC->client->ps.forcePowersActive & 1 << FP_GRASP &&
		NPC->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
	{
		//don't update?
		NPCInfo->desiredPitch = NPC->client->ps.viewangles[PITCH];
		NPCInfo->desiredYaw = NPC->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot(NPC, SPOT_HEAD, eyes);

	CalcEntitySpot(NPC->enemy, SPOT_HEAD, enemy_eyes);

	if ((NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO)
		&& TIMER_Done(NPC, "flameTime")
		&& NPC->s.weapon != WP_NONE
		&& NPC->s.weapon != WP_DISRUPTOR
		&& (NPC->s.weapon != WP_ROCKET_LAUNCHER || !(NPCInfo->scriptFlags & SCF_altFire))
		&& NPC->s.weapon != WP_THERMAL
		&& NPC->s.weapon != WP_TRIP_MINE
		&& NPC->s.weapon != WP_DET_PACK
		&& NPC->s.weapon != WP_STUN_BATON
		&& NPC->s.weapon != WP_MELEE)
	{
		//boba leads his enemy
		if (NPC->health < NPC->max_health * 0.5f)
		{
			//lead
			const float missile_speed = WP_SpeedOfMissileForWeapon(NPC->s.weapon,
				static_cast<qboolean>(NPCInfo->scriptFlags &
					SCF_altFire));
			if (missile_speed)
			{
				float e_dist = Distance(eyes, enemy_eyes);
				e_dist /= missile_speed; //How many seconds it will take to get to the enemy
				VectorMA(enemy_eyes, e_dist * Q_flrand(0.95f, 1.25f), NPC->enemy->client->ps.velocity, enemy_eyes);
			}
		}
	}

	//Find the desired angles
	if (!NPC->client->ps.saberInFlight
		&& (NPC->client->ps.legsAnim == BOTH_A2_STABBACK1
			|| NPC->client->ps.legsAnim == BOTH_A2_STABBACK1B
			|| NPC->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| NPC->client->ps.legsAnim == BOTH_ATTACK_BACK
			|| NPC->client->ps.legsAnim == BOTH_A7_KICK_B
			|| NPC->client->ps.legsAnim == BOTH_A7_KICK_B2
			|| NPC->client->ps.legsAnim == BOTH_A7_KICK_B3))
	{
		//point *away*
		GetAnglesForDirection(enemy_eyes, eyes, angles);
	}
	else if (NPC->client->ps.legsAnim == BOTH_A7_KICK_R)
	{
		//keep enemy to right
	}
	else if (NPC->client->ps.legsAnim == BOTH_A7_KICK_L)
	{
		//keep enemy to left
	}
	else if (NPC->client->ps.legsAnim == BOTH_A7_KICK_RL
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_BF
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_S)
	{
		//???
	}
	else
	{
		//point towards him
		GetAnglesForDirection(eyes, enemy_eyes, angles);
	}

	NPCInfo->desiredYaw = AngleNormalize360(angles[YAW]);

	if (doPitch)
	{
		NPCInfo->desiredPitch = AngleNormalize360(angles[PITCH]);
		if (NPC->client->ps.saberInFlight)
		{
			//tilt down a little
			NPCInfo->desiredPitch += 10;
		}
	}
}

static void Jedi_DebounceDirectionChanges()
{
	if (ucmd.forwardmove > 0)
	{
		if (!TIMER_Done(NPC, "moveback") || !TIMER_Done(NPC, "movenone"))
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if (ucmd.rightmove > 0)
			{
				ucmd.rightmove = 127;
			}
			else if (ucmd.rightmove < 0)
			{
				ucmd.rightmove = -127;
			}
			VectorClear(NPC->client->ps.moveDir);
			TIMER_Set(NPC, "moveback", -level.time);
			if (TIMER_Done(NPC, "movenone"))
			{
				TIMER_Set(NPC, "movenone", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPC, "moveforward"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPC, "lastmoveforward"))
			{
				const int hold_dir_time = Q_irand(500, 2000);
				TIMER_Set(NPC, "moveforward", hold_dir_time);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPC, "lastmoveforward", hold_dir_time + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
			//if being forced to move forward, do a full-speed moveforward
			ucmd.forwardmove = 127;
			VectorClear(NPC->client->ps.moveDir);
		}
	}
	else if (ucmd.forwardmove < 0)
	{
		if (!TIMER_Done(NPC, "moveforward") || !TIMER_Done(NPC, "movenone"))
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if (ucmd.rightmove > 0)
			{
				ucmd.rightmove = 127;
			}
			else if (ucmd.rightmove < 0)
			{
				ucmd.rightmove = -127;
			}
			VectorClear(NPC->client->ps.moveDir);
			TIMER_Set(NPC, "moveforward", -level.time);
			if (TIMER_Done(NPC, "movenone"))
			{
				TIMER_Set(NPC, "movenone", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPC, "moveback"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPC, "lastmoveback"))
			{
				const int hold_dir_time = Q_irand(500, 2000);
				TIMER_Set(NPC, "moveback", hold_dir_time);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPC, "lastmoveback", hold_dir_time + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveback
			ucmd.forwardmove = -127;
			VectorClear(NPC->client->ps.moveDir);
		}
	}
	else if (!TIMER_Done(NPC, "moveforward"))
	{
		//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		ucmd.forwardmove = 127;
		VectorClear(NPC->client->ps.moveDir);
	}
	else if (!TIMER_Done(NPC, "moveback"))
	{
		//NOTE: edge checking should stop me if this is bad...
		ucmd.forwardmove = -127;
		VectorClear(NPC->client->ps.moveDir);
	}
	//Time-debounce changes in right/left dir
	if (ucmd.rightmove > 0)
	{
		if (!TIMER_Done(NPC, "moveleft") || !TIMER_Done(NPC, "movecenter"))
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if (ucmd.forwardmove > 0)
			{
				ucmd.forwardmove = 127;
			}
			else if (ucmd.forwardmove < 0)
			{
				ucmd.forwardmove = -127;
			}
			VectorClear(NPC->client->ps.moveDir);
			TIMER_Set(NPC, "moveleft", -level.time);
			if (TIMER_Done(NPC, "movecenter"))
			{
				TIMER_Set(NPC, "movecenter", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPC, "moveright"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPC, "lastmoveright"))
			{
				const int hold_dir_time = Q_irand(250, 1500);
				TIMER_Set(NPC, "moveright", hold_dir_time);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPC, "lastmoveright", hold_dir_time + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveright
			ucmd.rightmove = 127;
			VectorClear(NPC->client->ps.moveDir);
		}
	}
	else if (ucmd.rightmove < 0)
	{
		if (!TIMER_Done(NPC, "moveright") || !TIMER_Done(NPC, "movecenter"))
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if (ucmd.forwardmove > 0)
			{
				ucmd.forwardmove = 127;
			}
			else if (ucmd.forwardmove < 0)
			{
				ucmd.forwardmove = -127;
			}
			VectorClear(NPC->client->ps.moveDir);
			TIMER_Set(NPC, "moveright", -level.time);
			if (TIMER_Done(NPC, "movecenter"))
			{
				TIMER_Set(NPC, "movecenter", Q_irand(1000, 2000));
			}
		}
		else if (TIMER_Done(NPC, "moveleft"))
		{
			//FIXME: should be if it's zero?
			if (TIMER_Done(NPC, "lastmoveleft"))
			{
				const int hold_dir_time = Q_irand(250, 1500);
				TIMER_Set(NPC, "moveleft", hold_dir_time);
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set(NPC, "lastmoveleft", hold_dir_time + Q_irand(1000, 2000));
			}
		}
		else
		{
			//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveleft
			ucmd.rightmove = -127;
			VectorClear(NPC->client->ps.moveDir);
		}
	}
	else if (!TIMER_Done(NPC, "moveright"))
	{
		//NOTE: edge checking should stop me if this is bad...
		ucmd.rightmove = 127;
		VectorClear(NPC->client->ps.moveDir);
	}
	else if (!TIMER_Done(NPC, "moveleft"))
	{
		//NOTE: edge checking should stop me if this is bad...
		ucmd.rightmove = -127;
		VectorClear(NPC->client->ps.moveDir);
	}
}

static void Jedi_TimersApply()
{
	//use careful anim/slower movement if not already moving
	if (!ucmd.forwardmove && !TIMER_Done(NPC, "walking"))
	{
		ucmd.buttons |= BUTTON_WALKING;
	}

	if (!TIMER_Done(NPC, "taunting"))
	{
		ucmd.buttons |= BUTTON_WALKING;
	}

	if (!ucmd.rightmove)
	{
		//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if (!TIMER_Done(NPC, "strafeLeft"))
		{
			if (NPCInfo->desiredYaw > NPC->client->ps.viewangles[YAW] + 60)
			{
				//we want to turn left, don't apply the strafing
			}
			else
			{
				//go ahead and strafe left
				ucmd.rightmove = -127;
				VectorClear(NPC->client->ps.moveDir);
			}
		}
		else if (!TIMER_Done(NPC, "strafeRight"))
		{
			if (NPCInfo->desiredYaw < NPC->client->ps.viewangles[YAW] - 60)
			{
				//we want to turn right, don't apply the strafing
			}
			else
			{
				//go ahead and strafe left
				ucmd.rightmove = 127;
				VectorClear(NPC->client->ps.moveDir);
			}
		}
	}

	Jedi_DebounceDirectionChanges();

	if (!TIMER_Done(NPC, "gripping"))
	{
		//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCEGRIP;
	}

	if (!TIMER_Done(NPC, "grasping"))
	{
		//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCEGRASP;
	}

	if (!TIMER_Done(NPC, "draining"))
	{
		//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if (!TIMER_Done(NPC, "holdLightning"))
	{
		//hold down the lightning key
		ucmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate(const int enemy_dist)
{
	if (Jedi_CultistDestroyer(NPC))
	{
		Jedi_Aggression(NPC, 5);
		return;
	}
	//===START MISSING CODE=================================================================
	if (TIMER_Done(NPC, "roamTime"))
	{
		TIMER_Set(NPC, "roamTime", Q_irand(2000, 5000));
		//okay, now mess with aggression
		if (NPC->enemy && NPC->enemy->client)
		{
			switch (NPC->enemy->client->ps.weapon)
			{
				//FIXME: add new weapons
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if (!NPC->enemy->client->ps.SaberActive())
				{
					//fool!  Standing around unarmed, charge!
					Jedi_Aggression(NPC, 2);
				}
				else
				{
					Jedi_Aggression(NPC, 1);
				}
				break;
			case WP_BLASTER:
			case WP_BRYAR_PISTOL:
			case WP_SBD_PISTOL:
			case WP_WRIST_BLASTER:
			case WP_BLASTER_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
			case WP_CONCUSSION:
			case WP_DROIDEKA:
				//if he has a blaster, move in when:
				//They're not shooting at me
				if (NPC->enemy->attackDebounceTime < level.time)
				{
					//does this apply to players?
					//Com_Printf( "(%d) raise agg - enemy not shooting ranged weap\n", level.time );
					Jedi_Aggression(NPC, 1);
				}
				//He's closer than a dist that gives us time to deflect
				if (enemy_dist < 256)
				{
					Jedi_Aggression(NPC, 1);
				}
				break;
			default:
				break;
			}
		}
	}

	if (TIMER_Done(NPC, "noStrafe") && TIMER_Done(NPC, "strafeLeft") && TIMER_Done(NPC, "strafeRight"))
	{
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if (!Q_irand(0, 4))
		{
			//start a strafe
			if (jedi_strafe(1000, 3000, 0, 4000, qtrue))
			{
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("off strafe\n");
				}
			}
		}
		else
		{
			//postpone any strafing for a while
			TIMER_Set(NPC, "noStrafe", Q_irand(1000, 3000));
		}
	}
	//===END MISSING CODE=================================================================
	if (NPC->client->ps.saberEventFlags)
	{
		//some kind of saber combat event is still pending
		int newFlags = NPC->client->ps.saberEventFlags;
		if (NPC->client->ps.saberEventFlags & SEF_PARRIED)
		{
			//parried
			TIMER_Set(NPC, "parryTime", -1);

			if (NPC->enemy && (!NPC->enemy->client || PM_SaberInKnockaway(NPC->enemy->client->ps.saber_move)))
			{
				//advance!
				Jedi_Aggression(NPC, 1); //get closer

				if (NPC->client->ps.saberAnimLevel == SS_STRONG
					|| NPC->client->ps.saberAnimLevel == SS_MEDIUM
					|| NPC->client->ps.saberAnimLevel == SS_DESANN
					|| NPC->client->ps.saberAnimLevel == SS_TAVION)
				{
					//my saber is not in a parrying position
					Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel - 1); //use a faster attack
				}
			}
			else
			{
				if (!Q_irand(0, 1))
				{
					Jedi_Aggression(NPC, -1);
				}
				if (!Q_irand(0, 1))
				{
					if (NPC->client->ps.saberAnimLevel == SS_STRONG
						|| NPC->client->ps.saberAnimLevel == SS_MEDIUM
						|| NPC->client->ps.saberAnimLevel == SS_DESANN
						|| NPC->client->ps.saberAnimLevel == SS_TAVION)
					{
						//my saber is not in a parrying position
						Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel - 1); //use a faster attack
					}
				}
			}
			if (d_JediAI->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf("(%d) PARRY: agg %d, no parry until %d\n", level.time, NPCInfo->stats.aggression,
					level.time + 100);
			}
			newFlags &= ~SEF_PARRIED;
		}
		if (!NPC->client->ps.weaponTime && NPC->client->ps.saberEventFlags & SEF_HITENEMY) //hit enemy
		{
			//we hit our enemy last time we swung, drop our aggression
			if (!Q_irand(0, 1)) //FIXME: dependent on rank/diff?
			{
				Jedi_Aggression(NPC, -1);
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("(%d) HIT: agg %d\n", level.time, NPCInfo->stats.aggression);
				}
				if (!Q_irand(0, 3)
					&& NPCInfo->blockedSpeechDebounceTime < level.time
					&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time
					&& NPC->painDebounceTime < level.time - 1000)
				{
					if (!npc_is_dark_jedi(NPC))
					{
						G_AddVoiceEvent(NPC, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
					}
					else
					{
						G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 10000);
					}

					jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time +
						3000;
				}
			}
			if (!Q_irand(0, 2))
			{
				Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel + 1);
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if (NPC->client->ps.saberEventFlags & SEF_BLOCKED)
		{
			//was blocked whilst attacking
			if (PM_SaberInBrokenParry(NPC->client->ps.saber_move) || NPC->client->ps.saberBlocked ==
				BLOCKED_PARRY_BROKEN)
			{
				if (NPC->client->ps.saberInFlight)
				{
					//lost our saber, too!!!
					Jedi_Aggression(NPC, -5); //really really really should back off!!!
				}
				else
				{
					Jedi_Aggression(NPC, -2); //really should back off!
				}
				Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel + 1); //use a stronger attack
				if (d_JediAI->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf("(%d) KNOCK-BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression);
				}
			}
			else
			{
				if (!Q_irand(0, 2))
				{
					Jedi_Aggression(NPC, -1);
					if (d_JediAI->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf("(%d) BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression);
					}
				}
				if (!Q_irand(0, 1))
				{
					Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel + 1);
				}
			}
			newFlags &= ~SEF_BLOCKED;
		}
		if (NPC->client->ps.saberEventFlags & SEF_DEFLECTED)
		{
			//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if (!Q_irand(0, 3))
			{
				Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel - 1);
			}
		}
		if (NPC->client->ps.saberEventFlags & SEF_HITWALL)
		{
			//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if (NPC->client->ps.saberEventFlags & SEF_HITOBJECT)
		{
			//hit some other damagable object
			if (!Q_irand(0, 3))
			{
				Jedi_AdjustSaberAnimLevel(NPC, NPC->client->ps.saberAnimLevel - 1);
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		NPC->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle(const int enemy_dist)
{
	if (!TIMER_Done(NPC, "parryTime"))
	{
		return;
	}
	if (NPC->client->ps.saberInFlight)
	{
		//don't do this idle stuff if throwing saber
		return;
	}
	if (NPC->client->ps.forcePowersActive & 1 << FP_RAGE
		|| NPC->client->ps.forceRageRecoveryTime > level.time)
	{
		//never taunt while raging or recovering from rage
		return;
	}
	if (NPC->client->ps.stats[STAT_WEAPONS] & 1 << WP_SCEPTER)
	{
		//never taunt when holding scepter
		return;
	}
	if (NPC->client->ps.saber[0].type == SABER_SITH_SWORD)
	{
		//never taunt when holding sith sword
		return;
	}
	//FIXME: make these distance numbers defines?
	if (enemy_dist >= 64)
	{
		//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		constexpr int chance = 20;
		//FIXME: possibly throw local objects at enemy?
		if (Q_irand(2, chance) < NPCInfo->stats.aggression)
		{
			if (TIMER_Done(NPC, "chatter"))
			{
				//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if (enemy_dist > 200
					&& (NPC->client->NPC_class != CLASS_BOBAFETT
						&& NPC->client->NPC_class != CLASS_MANDO)
					&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
					&& (NPC->client->NPC_class != CLASS_SITHLORD || NPC->s.weapon == WP_SABER)
					&& NPC->client->NPC_class != CLASS_ROCKETTROOPER
					&& NPC->client->ps.SaberActive()
					&& !Q_irand(0, 5))
				{
					//taunt even more, turn off the saber
					//FIXME: don't do this if health low?
					if (NPC->client->ps.saberAnimLevel != SS_STAFF
						&& NPC->client->ps.saberAnimLevel != SS_DUAL)
					{
						//those taunts leave saber on
						WP_DeactivateSaber(NPC);
					}
					//Don't attack for a bit
					NPCInfo->stats.aggression = 3;

					if (NPC->client->playerTeam != TEAM_PLAYER && !Q_irand(0, 1))
					{
						NPC->client->ps.taunting = level.time + 100;
						TIMER_Set(NPC, "chatter", Q_irand(5000, 10000));
						TIMER_Set(NPC, "taunting", 5500);
					}
					else
					{
						Jedi_BattleTaunt(NPC);
						TIMER_Set(NPC, "taunting", Q_irand(5000, 10000));
					}
				}
				else if (Jedi_BattleTaunt(NPC))
				{
					//FIXME: pick some anims
				}
			}
		}
	}
}

static qboolean Jedi_AttackDecide(const int enemy_dist)
{
	if (!TIMER_Done(NPC, "allyJediDelay"))
	{
		return qfalse;
	}

	if (Jedi_CultistDestroyer(NPC))
	{
		//destroyer
		if (enemy_dist <= 32)
		{
			//go boom!
			NPC->flags |= FL_GODMODE;
			NPC->takedamage = qfalse;

			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD | SETANIM_FLAG_OVERRIDE);
			NPC->client->ps.forcePowersActive |= 1 << FP_RAGE;
			NPC->painDebounceTime = NPC->useDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			return qtrue;
		}
		return qfalse;
	}

	if (NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time
		&& NPC->client->ps.saberLockTime < level.time)
	{
		//enemy is in a saberLock and we are not
		return qfalse;
	}

	if (NPC->client->ps.saberEventFlags & SEF_LOCK_WON)
	{
		//we won a saber lock, press the advantage with an attack!
		int chance;
		if (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER)
		{
			//desann and luke
			chance = 20;
		}
		else if (NPC->client->NPC_class == CLASS_TAVION || NPC->client->NPC_class == CLASS_ALORA)
		{
			//tavion
			chance = 10;
		}
		else if (NPC->client->NPC_class == CLASS_SHADOWTROOPER)
		{
			//shadowtrooper
			chance = 10;
		}
		else if (NPC->client->NPC_class == CLASS_REBORN)
		{
			//fencer
			chance = 5;
		}
		else
		{
			chance = NPCInfo->rank;
		}
		if (Q_irand(0, 30) < chance)
		{
			//based on skill with some randomness
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON; //clear this now that we are using the opportunity
			TIMER_Set(NPC, "noRetreat", Q_irand(500, 2000));
			NPC->client->ps.weaponTime = NPCInfo->shotTime = NPC->attackDebounceTime = 0;
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink();
			return qtrue;
		}
	}

	//try to hit them if we can
	if (!enemy_in_striking_range)
	{
		return qfalse;
	}

	if (!TIMER_Done(NPC, "parryTime"))
	{
		return qfalse;
	}

	if (NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		//not allowed to attack
		return qfalse;
	}

	if (!(ucmd.buttons & BUTTON_ATTACK)
		&& !(ucmd.buttons & BUTTON_ALT_ATTACK)
		&& !(ucmd.buttons & BUTTON_FORCE_FOCUS))
	{
		//not already attacking
		if (CanShoot(NPC->enemy, NPC))
		{
			if (NPC->s.weapon == WP_SABER)
			{
				//Try to attack
				Jedi_FaceEnemy(qtrue);

				if (!PM_SaberInAttack(NPC->client->ps.saber_move) && !Jedi_SaberBusy(NPC))
				{
					if (NPC->health <= 30
						|| NPC->client->ps.forcePower < BLOCKPOINTS_THIRTY
						|| NPC->client->ps.blockPoints < BLOCKPOINTS_THIRTY)
					{
						// Back away while attacking...
						const int rand = irand(0, 100);

						if (rand < 20)
						{
							NPC->client->pers.cmd.rightmove = 64;
						}
						else if (rand < 40)
						{
							NPC->client->pers.cmd.rightmove = -64;
						}

						WeaponThink();

						if (rand > 90)
						{
							// Do a lunge, etc occasionally...
							NPC->client->pers.cmd.upmove = -127;
							NPC->client->pers.cmd.buttons |= BUTTON_ATTACK;
						}
					}
					else
					{
						const int rand = irand(0, 100);

						Jedi_Advance();

						if (rand < 20)
						{
							NPC->client->pers.cmd.rightmove = 64;
						}
						else if (rand < 40)
						{
							NPC->client->pers.cmd.rightmove = -64;
						}

						WeaponThink();

						if (rand > 90
							&& NPC->client->ps.blockPoints < BLOCKPOINTS_TWENTYFIVE)
						{
							// Do a lunge, etc occasionally...
							NPC->client->pers.cmd.upmove = -127;
							NPC->client->pers.cmd.buttons |= BUTTON_ATTACK;
						}
					}
				}
				else
				{
					//Try to attack
					WeaponThink();
				}
			}
			else
			{
				//Try to attack
				if (NPC->health <= 30
					|| NPC->client->ps.forcePower < BLOCKPOINTS_THIRTY
					|| NPC->client->ps.blockPoints < BLOCKPOINTS_THIRTY)
				{
					// Back away while attacking...
					const int rand = irand(0, 100);

					if (!npc_is_jedi(NPC) && NPC->client->ps.weapon != WP_SABER)
					{
						// non Jedi handle their own attack/retreats...
						Jedi_Retreat();
					}

					if (npc_force_master(NPC) && NPC->client->ps.weapon != WP_SABER)
					{
						// non Jedi handle their own attack/retreats...
						Jedi_Retreat();
					}

					if (rand < 20)
					{
						NPC->client->pers.cmd.rightmove = 64;
					}
					else if (rand < 40)
					{
						NPC->client->pers.cmd.rightmove = -64;
					}

					WeaponThink();
				}
			}
		}
		else
		{
			//Try to attack
			WeaponThink();
		}
	}

	if (ucmd.buttons & BUTTON_ATTACK && !NPC_Jumping())
	{
		//attacking
		if (!ucmd.rightmove)
		{
			//not already strafing
			if (!Q_irand(0, 3))
			{
				//25% chance of doing this
				vec3_t right, dir2_enemy;

				AngleVectors(NPC->currentAngles, nullptr, right, nullptr);
				VectorSubtract(NPC->enemy->currentOrigin, NPC->currentAngles, dir2_enemy);
				if (DotProduct(right, dir2_enemy) > 0)
				{
					//he's to my right, strafe left
					if (NPC_MoveDirClear(ucmd.forwardmove, -127, qfalse))
					{
						ucmd.rightmove = -127;
						VectorClear(NPC->client->ps.moveDir);
					}
				}
				else
				{
					//he's to my left, strafe right
					if (NPC_MoveDirClear(ucmd.forwardmove, 127, qfalse))
					{
						ucmd.rightmove = 127;
						VectorClear(NPC->client->ps.moveDir);
					}
				}
			}
		}
		return qtrue;
	}

	return qfalse;
}

constexpr auto APEX_HEIGHT = 200.0f;
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
constexpr auto JUMP_SPEED = 200.0f;

static qboolean Jedi_Jump(vec3_t dest, const int goal_ent_num)
{
	if (true)
	{
		float shot_speed = 300, best_impact_dist = Q3_INFINITE; //fireSpeed,
		vec3_t shot_vel, fail_case;
		trace_t trace;
		trajectory_t tr{};
		int hit_count = 0;
		constexpr int max_hits = 7;
		vec3_t bottom;

		while (hit_count < max_hits)
		{
			vec3_t target_dir;
			VectorSubtract(dest, NPC->currentOrigin, target_dir);
			const float target_dist = VectorNormalize(target_dir);

			VectorScale(target_dir, shot_speed, shot_vel);
			float travel_time = target_dist / shot_speed;
			shot_vel[2] += travel_time * 0.5 * NPC->client->ps.gravity;

			if (!hit_count)
			{
				//save the first one as the worst case scenario
				VectorCopy(shot_vel, fail_case);
			}

			if (true) //tracePath )
			{
				vec3_t last_pos;
				constexpr int time_step = 500;
				//do a rough trace of the path
				qboolean blocked = qfalse;

				VectorCopy(NPC->currentOrigin, tr.trBase);
				VectorCopy(shot_vel, tr.trDelta);
				tr.trType = TR_GRAVITY;
				tr.trTime = level.time;
				travel_time *= 1000.0f;
				VectorCopy(NPC->currentOrigin, last_pos);

				//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
				for (int elapsed_time = time_step; elapsed_time < floor(travel_time) + time_step; elapsed_time +=
					time_step)
				{
					vec3_t test_pos;
					if (static_cast<float>(elapsed_time) > travel_time)
					{
						//cap it
						elapsed_time = floor(travel_time);
					}
					EvaluateTrajectory(&tr, level.time + elapsed_time, test_pos);
					if (test_pos[2] < last_pos[2])
					{
						//going down, ignore botclip
						gi.trace(&trace, last_pos, NPC->mins, NPC->maxs, test_pos, NPC->s.number, NPC->clipmask,
							G2_NOCOLLIDE, 0);
					}
					else
					{
						//going up, check for botclip
						gi.trace(&trace, last_pos, NPC->mins, NPC->maxs, test_pos, NPC->s.number,
							NPC->clipmask | CONTENTS_BOTCLIP, G2_NOCOLLIDE, 0);
					}

					if (trace.allsolid || trace.startsolid)
					{
						blocked = qtrue;
						break;
					}
					if (trace.fraction < 1.0f)
					{
						//hit something
						if (trace.entityNum == goal_ent_num)
						{
							//hit the enemy, that's perfect!
							//Hmm, don't want to land on him, though...
							break;
						}
						if (trace.contents & CONTENTS_BOTCLIP)
						{
							//hit a do-not-enter brush
							blocked = qtrue;
							break;
						}
						if (trace.plane.normal[2] > 0.7 && DistanceSquared(trace.endpos, dest) < 4096)
							//hit within 64 of desired location, should be okay
						{
							//close enough!
							break;
						}
						//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
						const float impact_dist = DistanceSquared(trace.endpos, dest);
						if (impact_dist < best_impact_dist)
						{
							best_impact_dist = impact_dist;
							VectorCopy(shot_vel, fail_case);
						}
						blocked = qtrue;
						break;
					}
					if (elapsed_time == floor(travel_time))
					{
						//reached end, all clear
						if (trace.fraction >= 1.0f)
						{
							//hmm, make sure we'll land on the ground...
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy(trace.endpos, bottom);
							bottom[2] -= 128;
							gi.trace(&trace, trace.endpos, NPC->mins, NPC->maxs, bottom, NPC->s.number, NPC->clipmask,
								G2_NOCOLLIDE, 0);
							if (trace.fraction >= 1.0f)
							{
								//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					//all clear, try next slice
					VectorCopy(test_pos, last_pos);
				}
				if (blocked)
				{
					//hit something, adjust speed (which will change arc)
					hit_count++;
					shot_speed = 300 + (hit_count - 2) * 100; //from 100 to 900 (skipping 300)
					if (hit_count >= 2)
					{
						//skip 300 since that was the first value we tested
						shot_speed += 100;
					}
				}
				else
				{
					//made it!
					break;
				}
			}
		}

		if (hit_count >= max_hits)
		{
			//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
			//NOTE: or try failcase?
			VectorCopy(fail_case, NPC->client->ps.velocity);
		}
		VectorCopy(shot_vel, NPC->client->ps.velocity);
	}
	return qtrue;
}

static qboolean Jedi_TryJump(const gentity_t* goal)
{
	//FIXME: never does a simple short, regular jump...
	//FIXME: I need to be on ground too!
	if (NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		return qfalse;
	}
	if (TIMER_Done(NPC, "jumpChaseDebounce"))
	{
		if (!goal->client || goal->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			if (!PM_InKnockDown(&NPC->client->ps) && !PM_InRoll(&NPC->client->ps))
			{
				//enemy is on terra firma
				vec3_t goal_diff;
				VectorSubtract(goal->currentOrigin, NPC->currentOrigin, goal_diff);
				const float goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				const float goal_xy_dist = VectorNormalize(goal_diff);
				if (goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/)
					//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if (NPC->health < 150 && (NPC->health < 30 && goal_z_diff < 0 || goal_z_diff < -128))
					{
						//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if (goal_z_diff < 32 && goal_xy_dist < 200)
					{
						//what is their ideal jump height?
						ucmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						if (goal_z_diff > 0 || goal_xy_dist > 128)
						{
							//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy(goal->currentOrigin, dest);
							if (goal == NPC->enemy)
							{
								int side_try = 0;
								while (side_try < 10)
								{
									//FIXME: make it so it doesn't try the same spot again?
									if (Q_irand(0, 1))
									{
										dest[0] += NPC->enemy->maxs[0] * 1.25;
									}
									else
									{
										dest[0] += NPC->enemy->mins[0] * 1.25;
									}
									if (Q_irand(0, 1))
									{
										dest[1] += NPC->enemy->maxs[1] * 1.25;
									}
									else
									{
										dest[1] += NPC->enemy->mins[1] * 1.25;
									}
									trace_t trace;
									vec3_t bottom;
									VectorCopy(dest, bottom);
									bottom[2] -= 128;
									gi.trace(&trace, dest, NPC->mins, NPC->maxs, bottom, goal->s.number, NPC->clipmask,
										G2_NOCOLLIDE, 0);
									if (trace.fraction < 1.0f)
									{
										//hit floor, okay to land here
										break;
									}
									side_try++;
								}
								if (side_try >= 10)
								{
									//screw it, just jump right at him?
									VectorCopy(goal->currentOrigin, dest);
								}
							}
							if (Jedi_Jump(dest, goal->s.number))
							{
								{
									//FIXME: make this a function call
									int jump_anim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									if (NPC->client->NPC_class == CLASS_BOBAFETT
										|| NPC->client->NPC_class == CLASS_MANDO
										|| NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG)
									{
										//can't do acrobatics
										jump_anim = BOTH_FORCEJUMP1;
									}
									else
									{
										jump_anim = BOTH_FLIP_F;
									}
									NPC_SetAnim(NPC, SETANIM_BOTH, jump_anim,
										SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								}

								NPC->client->ps.forceJumpZStart = NPC->currentOrigin[2];
								NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
								NPC->client->ps.forcePowersActive |= 1 << FP_LEVITATION;

								if (NPC->client->NPC_class == CLASS_BOBAFETT
									|| NPC->client->NPC_class == CLASS_MANDO)
								{
									G_SoundOnEnt(NPC, CHAN_ITEM, "sound/boba/jeton.wav");
									NPC->client->jetPackTime = level.time + Q_irand(1000, 3000);
								}
								else
								{
									if (NPC->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
									{
										//short burst
										G_SoundOnEnt(NPC, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
									}
									else
									{
										//holding it
										G_SoundOnEnt(NPC, CHAN_BODY, "sound/weapons/force/jump.mp3");
									}
								}

								TIMER_Set(NPC, "forceJumpChasing", Q_irand(2000, 3000));
								debounce = qtrue;
							}
						}
					}
					if (debounce)
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set(NPC, "jumpChaseDebounce", Q_irand(2000, 5000));
						ucmd.forwardmove = 127;
						VectorClear(NPC->client->ps.moveDir);
						TIMER_Set(NPC, "duck", -level.time);
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

static qboolean Jedi_Jumping(const gentity_t* goal)
{
	if (!TIMER_Done(NPC, "forceJumpChasing") && goal)
	{
		//force-jumping at the enemy
		if (!(NPC->client->ps.pm_flags & PMF_JUMPING)
			&& !(NPC->client->ps.pm_flags & PMF_TRIGGER_PUSHED))
		{
			//landed
			TIMER_Set(NPC, "forceJumpChasing", 0);
		}
		else
		{
			NPC_FaceEntity(goal, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}

extern void G_UcmdMoveForDir(const gentity_t* self, usercmd_t* cmd, vec3_t dir);
extern qboolean PM_KickingAnim(int anim);

static void Jedi_CheckEnemyMovement(const float enemy_dist)
{
	if (!NPC->enemy || !NPC->enemy->client)
	{
		return;
	}

	if (!(NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER))
	{
		if (PM_KickingAnim(NPC->enemy->client->ps.legsAnim)
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			//FIXME: I'm relatively close to him
			&& (NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_RL
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_BF
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_S
				|| NPC->enemy->enemy && NPC->enemy->enemy == NPC)
			)
		{
			//run into the kick!
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear(NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if (NPC->enemy->client->ps.torsoAnim == BOTH_A7_HILT
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SMACK_R
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SMACK_L
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//run into the hilt bash
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear(NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if ((NPC->enemy->client->ps.torsoAnim == BOTH_A6_FB
			|| NPC->enemy->client->ps.torsoAnim == BOTH_A6_LR)
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//run into the attack
			//FIXME : only if on R/L or F/B?
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear(NPC->client->ps.moveDir);
			Jedi_Advance();
		}
		else if (NPC->enemy->enemy && NPC->enemy->enemy == NPC)
		{
			//enemy is mad at *me*
			if (NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1
				|| NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN
				|| NPC->enemy->client->ps.legsAnim == BOTH_FLIP_ATTACK7)
			{
				//enemy is flipping over me
				if (Q_irand(0, NPCInfo->rank) < RANK_LT)
				{
					//be nice and stand still for him...
					ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
					VectorClear(NPC->client->ps.moveDir);
					NPC->client->ps.forceJumpCharge = 0;
					TIMER_Set(NPC, "strafeLeft", -1);
					TIMER_Set(NPC, "strafeRight", -1);
					TIMER_Set(NPC, "noStrafe", Q_irand(500, 1000));
					TIMER_Set(NPC, "movenone", Q_irand(500, 1000));
					TIMER_Set(NPC, "movecenter", Q_irand(500, 1000));
				}
			}
			else if (NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP)
			{
				//he's flipping off a wall
				if (NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					//still in air
					if (enemy_dist < 256)
					{
						//close
						if (Q_irand(0, NPCInfo->rank) < RANK_LT)
						{
							//be nice and stand still for him...
							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear(NPC->client->ps.moveDir);
							NPC->client->ps.forceJumpCharge = 0;
							TIMER_Set(NPC, "strafeLeft", -1);
							TIMER_Set(NPC, "strafeRight", -1);
							TIMER_Set(NPC, "noStrafe", Q_irand(500, 1000));
							TIMER_Set(NPC, "noturn", Q_irand(250, 500) * (3 - g_spskill->integer));

							vec3_t enemy_fwd, dest, dir;

							VectorCopy(NPC->enemy->client->ps.velocity, enemy_fwd);
							VectorNormalize(enemy_fwd);
							VectorMA(NPC->enemy->currentOrigin, -64, enemy_fwd, dest);
							VectorSubtract(dest, NPC->currentOrigin, dir);
							if (VectorNormalize(dir) > 32)
							{
								G_UcmdMoveForDir(NPC, &ucmd, dir);
							}
							else
							{
								TIMER_Set(NPC, "movenone", Q_irand(500, 1000));
								TIMER_Set(NPC, "movecenter", Q_irand(500, 1000));
							}
						}
					}
				}
			}
			else if (NPC->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 || NPC->enemy->client->ps.legsAnim ==
				BOTH_A2_STABBACK1B)
			{
				//he's stabbing backwards
				if (enemy_dist < 256 && enemy_dist > 64)
				{
					//close
					if (!in_front(NPC->currentOrigin, NPC->enemy->currentOrigin, NPC->enemy->currentAngles, 0.0f))
					{
						//behind him
						if (!Q_irand(0, NPCInfo->rank))
						{
							//be nice and stand still for him...
							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear(NPC->client->ps.moveDir);
							NPC->client->ps.forceJumpCharge = 0;
							TIMER_Set(NPC, "strafeLeft", -1);
							TIMER_Set(NPC, "strafeRight", -1);
							TIMER_Set(NPC, "noStrafe", Q_irand(500, 1000));

							vec3_t enemyFwd, dest, dir;

							AngleVectors(NPC->enemy->currentAngles, enemyFwd, nullptr, nullptr);
							VectorMA(NPC->enemy->currentOrigin, -32, enemyFwd, dest);
							VectorSubtract(dest, NPC->currentOrigin, dir);
							if (VectorNormalize(dir) > 64)
							{
								G_UcmdMoveForDir(NPC, &ucmd, dir);
							}
							else
							{
								TIMER_Set(NPC, "movenone", Q_irand(500, 1000));
								TIMER_Set(NPC, "movecenter", Q_irand(500, 1000));
							}
						}
					}
				}
			}
		}
	}
}

static void Jedi_CheckJumps()
{
	if (NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		NPC->client->ps.forceJumpCharge = 0;
		ucmd.upmove = 0;
		return;
	}
	vec3_t jump_vel = { 0, 0, 0 };

	if (NPC->client->ps.forceJumpCharge)
	{
		//gi.Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump(NPC, jump_vel, &ucmd);
	}
	else if (ucmd.upmove > 0)
	{
		//gi.Printf( "(%d) regular jump\n", level.time );
		VectorCopy(NPC->client->ps.velocity, jump_vel);
		jump_vel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if (!jump_vel[0] && !jump_vel[1]) //FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{
		//we assume a jump straight up is safe
		//gi.Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}
	//Now predict where this is going
	//in steps, keep evaluating the trajectory until the new z pos is <= than current z pos, trace down from there
	trace_t trace;
	trajectory_t tr{};
	vec3_t last_pos, bottom;

	VectorCopy(NPC->currentOrigin, tr.trBase);
	VectorCopy(jump_vel, tr.trDelta);
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy(NPC->currentOrigin, last_pos);

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for (int elapsed_time = 500; elapsed_time <= 4000; elapsed_time += 500)
	{
		vec3_t test_pos;
		EvaluateTrajectory(&tr, level.time + elapsed_time, test_pos);
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if (test_pos[2] < last_pos[2])
		{
			//going down, don't check for BOTCLIP
			gi.trace(&trace, last_pos, NPC->mins, NPC->maxs, test_pos, NPC->s.number, NPC->clipmask,
				static_cast<EG2_Collision>(0), 0); //FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{
			//going up, check for BOTCLIP
			gi.trace(&trace, last_pos, NPC->mins, NPC->maxs, test_pos, NPC->s.number, NPC->clipmask | CONTENTS_BOTCLIP,
				static_cast<EG2_Collision>(0), 0);
		}
		if (trace.allsolid || trace.startsolid)
		{
			//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
		}
		if (trace.fraction < 1.0f)
		{
			//hit something
			if (trace.contents & CONTENTS_BOTCLIP)
			{
				//hit a do-not-enter brush
				goto jump_unsafe;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy(test_pos, last_pos);
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy(trace.endpos, bottom);
	if (bottom[2] > NPC->currentOrigin[2])
	{
		//only care about dist down from current height or lower
		bottom[2] = NPC->currentOrigin[2];
	}
	else if (NPC->currentOrigin[2] - bottom[2] > 400)
	{
		//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
	}
	bottom[2] -= 128;
	gi.trace(&trace, trace.endpos, NPC->mins, NPC->maxs, bottom, NPC->s.number, NPC->clipmask,
		static_cast<EG2_Collision>(0), 0);
	if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f)
	{
		//hit ground!
		if (trace.entityNum < ENTITYNUM_WORLD)
		{
			//landed on an ent
			const gentity_t* groundEnt = &g_entities[trace.entityNum];
			if (groundEnt->svFlags & SVF_GLASS_BRUSH)
			{
				//don't land on breakable glass!
				goto jump_unsafe;
			}
		}
		//gi.Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	//gi.Printf( "(%d) unsafe jump cleared\n", level.time );
	NPC->client->ps.forceJumpCharge = 0;
	ucmd.upmove = 0;
}

extern void RT_JetPackEffect(int duration);

void RT_CheckJump()
{
	int jump_ent_num = ENTITYNUM_NONE;
	vec3_t jump_pos = { 0, 0, 0 };

	if (!NPCInfo->goalEntity)
	{
		if (NPC->enemy)
		{
			//FIXME: debounce this?
			if (TIMER_Done(NPC, "roamTime")
				&& Q_irand(0, 9))
			{
				//okay to try to find another spot to be
				int cpFlags = CP_CLEAR | CP_HAS_ROUTE; //must have a clear shot at enemy
				const float enemyDistSq = DistanceHorizontalSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);
				//FIXME: base these ranges on weapon
				if (enemyDistSq > 2048 * 2048)
				{
					//hmm, close in?
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if (enemyDistSq < 256 * 256)
				{
					//back off!
					cpFlags |= CP_RETREAT;
				}
				int sendFlags = cpFlags;
				int cp = NPC_FindCombatPointRetry(NPC->currentOrigin,
					NPC->currentOrigin,
					NPC->currentOrigin,
					&sendFlags,
					256,
					NPCInfo->lastFailedCombatPoint);
				if (cp == -1)
				{
					//try again, no route needed since we can rocket-jump to it!
					cpFlags &= ~CP_HAS_ROUTE;
					cp = NPC_FindCombatPointRetry(NPC->currentOrigin,
						NPC->currentOrigin,
						NPC->currentOrigin,
						&cpFlags,
						256,
						NPCInfo->lastFailedCombatPoint);
				}
				if (cp != -1)
				{
					NPC_SetMoveGoal(NPC, level.combatPoints[cp].origin, 8, qtrue, cp);
				}
				else
				{
					jump_ent_num = NPC->enemy->s.number;
					VectorCopy(NPC->enemy->currentOrigin, jump_pos);
					//return;
				}
				TIMER_Set(NPC, "roamTime", Q_irand(3000, 12000));
			}
			else
			{
				jump_ent_num = NPC->enemy->s.number;
				VectorCopy(NPC->enemy->currentOrigin, jump_pos);
				//return;
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		jump_ent_num = NPCInfo->goalEntity->s.number;
		VectorCopy(NPCInfo->goalEntity->currentOrigin, jump_pos);
	}
	vec3_t vec2_goal;
	VectorSubtract(jump_pos, NPC->currentOrigin, vec2_goal);
	if (fabs(vec2_goal[2]) < 32)
	{
		//not a big height diff, see how far it is
		vec2_goal[2] = 0;
		if (VectorLengthSquared(vec2_goal) < 256 * 256)
		{
			//too close!  Don't rocket-jump to it...
			return;
		}
	}
	//If we can't get straight at him
	if (!Jedi_ClearPathToSpot(jump_pos, jump_ent_num))
	{
		//hunt him down
		if ((NPC_ClearLOS(NPC->enemy) || NPCInfo->enemyLastSeenTime > level.time - 500)
			&& InFOV(jump_pos, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 60))
		{
			if (NPC_TryJump(jump_pos)) // Rocket Trooper
			{
				//just do the jetpack effect for a litte bit
				RT_JetPackEffect(Q_irand(800, 1500));
				return;
			}
		}

		if (Jedi_Hunt() && !(NPCInfo->aiFlags & NPCAI_BLOCKED))
		{
			//can macro-navigate to him
			return;
		}
		//FIXME: try to find a waypoint that can see enemy, jump from there
		if (STEER::HasBeenBlockedFor(NPC, 2000))
		{
			//try to jump to the blockedTargetPosition
			if (NPC_TryJump(NPCInfo->blockedTargetPosition)) // Rocket Trooper
			{
				//just do the jetpack effect for a litte bit
				RT_JetPackEffect(Q_irand(800, 1500));
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
static void Jedi_Combat()
{
	vec3_t enemy_dir, enemy_movedir, enemy_dest;
	float enemy_dist, enemy_movespeed;

	//See where enemy will be 300 ms from now
	jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);

	if (NPC_Jumping())
	{
		//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide(enemy_dist);
		return;
	}

	if (TIMER_Done(NPC, "allyJediDelay"))
	{
		if ((!(NPC->client->ps.forcePowersActive & 1 << FP_GRIP) || NPC->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2)
			&& (!(NPC->client->ps.forcePowersActive & 1 << FP_GRASP) || NPC->client->ps.forcePowerLevel[FP_GRASP] < FORCE_LEVEL_2))
		{
			//not gripping
			//If we can't get straight at him
			if (!Jedi_ClearPathToSpot(enemy_dest, NPC->enemy->s.number))
			{
				//hunt him down
				if ((NPC_ClearLOS(NPC->enemy) || NPCInfo->enemyLastSeenTime > level.time - 500) && NPC_FaceEnemy(qtrue))
				{
					if (NPC->client && NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO)
					{
						Boba_FireDecide();
					}
				}

				//Check for evasion
				if (TIMER_Done(NPC, "parryTime"))
				{
					//finished parrying
					if (NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
						NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
					{
						//wasn't blocked myself
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
					}
				}
				if (Jedi_Hunt() && !(NPCInfo->aiFlags & NPCAI_BLOCKED))
				{
					//can macro-navigate to him
					if (enemy_dist < 384 && !Q_irand(0, 10) && NPCInfo->blockedSpeechDebounceTime < level.time &&
						jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && !NPC_ClearLOS(NPC->enemy))
					{
						if (!npc_is_dark_jedi(NPC))
						{
							G_AddVoiceEvent(NPC, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 3000);
						}
						else
						{
							G_AddVoiceEvent(NPC, Q_irand(EV_JLOST1, EV_JLOST3), 3000);
						}
						jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.
							time + 3000;
					}
					if (NPC->client && (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class ==
						CLASS_MANDO))
					{
						Boba_FireDecide();
					}
					return;
				}
				if (NPC->s.weapon == WP_SABER && (com_outcast->integer == 1 || com_outcast->integer == 4))
				{
					if (NPCInfo->aiFlags & NPCAI_BLOCKED)
					{
						//try to jump to the blockedTargetPosition
						gentity_t* temp_goal = G_Spawn(); //ugh, this is NOT good...?
						G_SetOrigin(temp_goal, NPCInfo->blockedTargetPosition);
						gi.linkentity(temp_goal);
						if (Jedi_TryJump(temp_goal))
						{
							//going to jump to the dest
							G_FreeEntity(temp_goal);
							return;
						}
						G_FreeEntity(temp_goal);
					}
					else if (STEER::HasBeenBlockedFor(NPC, 3000))
					{
						//try to jump to the blockedDest
						if (NPCInfo->blockedTargetEntity)
						{
							NPC_TryJump(NPCInfo->blockedTargetEntity); // commented Out
						}
						else
						{
							NPC_TryJump(NPCInfo->blockedTargetPosition); // commented Out
						}
					}
				}
			}
		}
		//else, we can see him or we can't track him at all

		//every few seconds, decide if we should we advance or retreat?
		Jedi_CombatTimersUpdate(enemy_dist);
		//We call this even if lost enemy to keep him moving and to update the taunting behavior
		//maintain a distance from enemy appropriate for our aggression level
		jedi_combat_distance(enemy_dist);
	}

	if (NPC->client->NPC_class != CLASS_BOBAFETT && NPC->client->NPC_class != CLASS_MANDO)
	{
		//Update our seen enemy position
		if (!NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && NPC->client->ps.
			groundEntityNum != ENTITYNUM_NONE)
		{
			VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation);
		}
		NPCInfo->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	if (TIMER_Done(NPC, "noturn") && !NPC_Jumping())
	{
		Jedi_FaceEnemy(qtrue);
	}
	NPC_UpdateAngles(qtrue, qtrue);

	//Check for evasion
	if (TIMER_Done(NPC, "parryTime"))
	{
		//finished parrying
		if (NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
		{
			//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
	if (NPC->enemy->s.weapon == WP_SABER)
	{
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
	else
	{
		if (!in_camera)
		{
			NPC_CheckEvasion();
		}

		if (!npc_is_dark_jedi(NPC))
		{
			G_AddVoiceEvent(NPC, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 2000);
		}
		else
		{
			G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
		}
	}

	//apply strafing/walking timers, etc.
	Jedi_TimersApply();

	if (TIMER_Done(NPC, "allyJediDelay"))
	{
		if ((!NPC->client->ps.saberInFlight || NPC->client->ps.saberAnimLevel == SS_DUAL && NPC->client->ps.saber[1].
			Active())
			&& (!(NPC->client->ps.forcePowersActive & 1 << FP_GRIP) || NPC->client->ps.forcePowerLevel[FP_GRIP] <
				FORCE_LEVEL_2)
			&& (!(NPC->client->ps.forcePowersActive & 1 << FP_GRASP) || NPC->client->ps.forcePowerLevel[FP_GRASP] <
				FORCE_LEVEL_2))
		{
			//not throwing saber or using force grip
			//see if we can attack
			if (!Jedi_AttackDecide(enemy_dist))
			{
				//we're not attacking, decide what else to do
				Jedi_CombatIdle(enemy_dist);

				if (!npc_is_dark_jedi(NPC))
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
				}
				else
				{
					G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 10000);
				}
			}
			else
			{
				//we are attacking
				//stop taunting
				TIMER_Set(NPC, "taunting", -level.time);
			}
		}
		else
		{
		}

		if (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO)
		{
			Boba_FireDecide();
		}
		else if (NPC->client->NPC_class == CLASS_ROCKETTROOPER)
		{
			RT_FireDecide();
		}
	}

	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement(enemy_dist);
	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps();
	//Just make sure we don't strafe into walls or off cliffs

	if (VectorCompare(NPC->client->ps.moveDir, vec3_origin) //stomped the NAV system's moveDir
		&& !NPC_MoveDirClear(ucmd.forwardmove, ucmd.rightmove, qtrue)) //check ucmd-driven movement
	{
		//uh-oh, we are going to fall or hit something
		navInfo_t info;
		//Get the move info
		NAV_GetLastMove(info);
		if (!(info.flags & NIF_MACRO_NAV))
		{
			//micro-navigation told us to step off a ledge, try using macronav for now
			NPC_MoveToGoal(qfalse);
		}
		//reset the timers.
		TIMER_Set(NPC, "strafeLeft", 0);
		TIMER_Set(NPC, "strafeRight", 0);
	}
}

/*
==========================================================================================
EXTERNALLY CALLED BEHAVIOR STATES
==========================================================================================
*/

/*
-------------------------
NPC_Jedi_Pain
-------------------------
*/

void NPC_Jedi_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, const vec3_t point, const int damage,
	const int mod, int hit_loc)
{
	if (attacker->s.weapon == WP_SABER)
	{
		//back off
		TIMER_Set(self, "parryTime", -1);
		if (self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda", self->NPC_type))
		{
			//less for Desann
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_spskill->integer) * 50;
		}
		else if (self->client->NPC_class == CLASS_SITHLORD)
		{
			//less for Desann
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_spskill->integer) * 50;
		}
		else if (self->client->NPC_class == CLASS_VADER)
		{
			//less for Desann
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_spskill->integer) * 50;
		}
		else if (self->NPC->rank >= RANK_LT_JG)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_spskill->integer) * 100; //300
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3 - g_spskill->integer) * 200; //500
		}
		if (!Q_irand(0, 3))
		{
			//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel(self, Q_irand(SS_FAST, SS_TAVION));
		}
		if (!Q_irand(0, 1)) //damage > 20 || self->health < 40 ||
		{
			//Com_Printf( "(%d) drop agg - hit by saber\n", level.time );
			Jedi_Aggression(self, -1);
		}
		if (d_JediAI->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf("(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression,
				level.time + 500);
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if (d_JediAI->integer || g_DebugSaberCombat->integer)
		{
			vec3_t diff, fwdangles{}, right;

			VectorSubtract(point, self->client->renderInfo.eyePoint, diff);
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors(fwdangles, nullptr, right, nullptr);
			const float rightdot = DotProduct(right, diff);
			const float zdiff = point[2] - self->client->renderInfo.eyePoint[2];

			gi.Printf("(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time,
				point[2] - self->absmin[2], zdiff, rightdot);
		}
	}
	else
	{
		//attack
		Jedi_Aggression(self, 1);
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop(self, FP_GRIP);
	WP_ForcePowerStop(self, FP_GRASP);

	NPC_Pain(self, inflictor, attacker, point, damage, mod);

	if (!damage && self->health > 0)
	{
		//FIXME: better way to know I was pushed
		if (!npc_is_dark_jedi(self))
		{
			G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		}
		else
		{
			G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
		}
	}

	//drop me from the ceiling if I'm on it
	if (Jedi_WaitingAmbush(self))
	{
		self->client->noclip = false;
	}
	if (self->client->ps.legsAnim == BOTH_CEILING_CLING)
	{
		NPC_SetAnim(self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	if (self->client->ps.torsoAnim == BOTH_CEILING_CLING)
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	//check special defenses
	if (attacker
		&& attacker->client
		&& !OnSameTeam(self, attacker))
	{
		//hit by a client
		if (mod == MOD_FORCE_GRIP
			|| mod == MOD_FORCE_LIGHTNING
			|| mod == MOD_LIGHTNING_STRIKE
			|| mod == MOD_FORCE_DRAIN)
		{
			//see if we should turn on absorb
			if ((self->client->ps.forcePowersKnown & 1 << FP_ABSORB) != 0
				&& (self->client->ps.forcePowersActive & 1 << FP_ABSORB) == 0)
			{
				//know absorb and not already using it
				if (attacker->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand(0, g_spskill->integer + 1)) //enemy is player
				{
					if (Q_irand(0, self->NPC->rank) > RANK_ENSIGN)
					{
						if (!Q_irand(0, 5))
						{
							ForceAbsorb(self);
						}
					}
				}
			}
		}
		else if (damage > Q_irand(5, 20))
		{
			//respectable amount of normal damage
			if ((self->client->ps.forcePowersKnown & 1 << FP_PROTECT) != 0
				&& (self->client->ps.forcePowersActive & 1 << FP_PROTECT) == 0)
			{
				//know protect and not already using it
				if (attacker->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand(0, g_spskill->integer + 1)) //enemy is player
				{
					if (Q_irand(0, self->NPC->rank) > RANK_ENSIGN)
					{
						if (!Q_irand(0, 1))
						{
							if (attacker->s.number < MAX_CLIENTS
								&& (self->NPC->aiFlags & NPCAI_BOSS_CHARACTER
									|| self->client->NPC_class == CLASS_SHADOWTROOPER)
								&& Q_irand(0, 6 - g_spskill->integer))
							{
							}
							else
							{
								ForceProtect(self);
							}
						}
					}
				}
			}
		}
	}
}

static qboolean Jedi_CheckDanger()
{
	const int alert_event = NPC_CheckAlertEvents(qtrue, qtrue);
	if (level.alertEvents[alert_event].level >= AEL_DANGER)
	{
		//run away!
		if (!level.alertEvents[alert_event].owner
			|| !level.alertEvents[alert_event].owner->client
			|| level.alertEvents[alert_event].owner != NPC && level.alertEvents[alert_event].owner->client->playerTeam
			!=
			NPC->client->playerTeam)
		{
			//no owner
			return qfalse;
		}
		G_SetEnemy(NPC, level.alertEvents[alert_event].owner);
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set(NPC, "attackDelay", Q_irand(500, 2500));
		return qtrue;
	}
	return qfalse;
}

extern int g_crosshairEntNum;

static qboolean Jedi_CheckAmbushPlayer()
{
	if (!player || !player->client)
	{
		return qfalse;
	}

	if (!NPC_ValidEnemy(player))
	{
		return qfalse;
	}

	if (NPC->client->ps.powerups[PW_CLOAKED] || g_crosshairEntNum != NPC->s.number)
	{
		//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
		if (!gi.inPVS(player->currentOrigin, NPC->currentOrigin))
		{
			//must be in same room
			return qfalse;
		}
		if (!NPC->client->ps.powerups[PW_CLOAKED])
		{
			NPC_SetLookTarget(NPC, 0, 0);
		}
		float target_dist;
		const float z_diff = NPC->currentOrigin[2] - player->currentOrigin[2];
		if (z_diff <= 0 || z_diff > 512)
		{
			//never ambush if they're above me or way way below me
			return qfalse;
		}

		//If the target is this close, then wake up regardless
		if ((target_dist = DistanceHorizontalSquared(player->currentOrigin, NPC->currentOrigin)) > 4096)
		{
			//closer than 64 - always ambush
			if (target_dist > 147456)
			{
				//> 384, not close enough to ambush
				return qfalse;
			}
			//Check FOV first
			if (NPC->client->ps.powerups[PW_CLOAKED])
			{
				if (InFOV(player, NPC, 30, 90) == qfalse)
				{
					return qfalse;
				}
			}
			else
			{
				if (InFOV(player, NPC, 45, 90) == qfalse)
				{
					return qfalse;
				}
			}
		}

		if (!NPC_ClearLOS(player))
		{
			return qfalse;
		}
	}

	G_SetEnemy(NPC, player);
	NPCInfo->enemyLastSeenTime = level.time;
	TIMER_Set(NPC, "attackDelay", Q_irand(500, 2500));
	return qtrue;
}

void Jedi_Ambush(gentity_t* self)
{
	self->client->noclip = false;
	self->client->ps.pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;
	NPC_SetAnim(self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	self->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
	if (self->client->NPC_class != CLASS_BOBAFETT
		&& self->client->NPC_class != CLASS_MANDO
		&& self->client->NPC_class != CLASS_ROCKETTROOPER)
	{
		self->client->ps.SaberActivate();
	}
	Jedi_Decloak(self);
	G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
}

qboolean Jedi_WaitingAmbush(const gentity_t* self)
{
	if (self->spawnflags & JSF_AMBUSH && self->client->noclip)
	{
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
Jedi_Patrol
-------------------------
*/

static void Jedi_Patrol()
{
	NPC->client->ps.saberBlocked = BLOCKED_NONE;

	if (Jedi_WaitingAmbush(NPC))
	{
		//hiding on the ceiling
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//look for enemies
			if (Jedi_CheckAmbushPlayer() || Jedi_CheckDanger())
			{
				//found him!
				Jedi_Ambush(NPC);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}
	else if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
	{
		//look for enemies
		gentity_t* best_enemy = nullptr;
		float best_enemy_dist = Q3_INFINITE;
		for (int i = 0; i < ENTITYNUM_WORLD; i++)
		{
			gentity_t* enemy = &g_entities[i];
			if (enemy && enemy->client && NPC_ValidEnemy(enemy))
			{
				if (gi.inPVS(NPC->currentOrigin, enemy->currentOrigin))
				{
					//we could potentially see him
					const float enemy_dist = DistanceSquared(NPC->currentOrigin, enemy->currentOrigin);
					if (enemy->s.number == 0 || enemy_dist < best_enemy_dist)
					{
						if (enemy_dist < 220 * 220 || NPCInfo->investigateCount >= 3 && NPC->client->ps.
							SaberActive())
						{
							G_SetEnemy(NPC, enemy);
							NPCInfo->stats.aggression = 3;
							break;
						}
						if (enemy->client->ps.saberInFlight && enemy->client->ps.SaberActive())
						{
							vec3_t saber_dir2_me;
							vec3_t saber_move_dir;
							const gentity_t* saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract(NPC->currentOrigin, saber->currentOrigin, saber_dir2_me);
							const float saber_dist = VectorNormalize(saber_dir2_me);
							VectorCopy(saber->s.pos.trDelta, saber_move_dir);
							VectorNormalize(saber_move_dir);
							if (DotProduct(saber_move_dir, saber_dir2_me) > 0.5)
							{
								//it's heading towards me
								if (saber_dist < 200)
								{
									//incoming!
									G_SetEnemy(NPC, enemy);
									NPCInfo->stats.aggression = 3;
									break;
								}
							}
						}
						best_enemy_dist = enemy_dist;
						best_enemy = enemy;
					}
				}
			}
		}
		if (!NPC->enemy)
		{
			//still not mad
			if (!best_enemy)
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(-1);
				TIMER_Set(NPC, "DeactivateTime", Q_irand(3000, 6000));
				//FIXME: what about alerts?  But not if ignore alerts
			}
			else
			{
				//have one to consider
				if (NPC_ClearLOS(best_enemy))
				{
					//we have a clear (of architecture) LOS to him
					if (NPCInfo->aiFlags & NPCAI_NO_JEDI_DELAY)
					{
						//just get mad right away
						if (DistanceHorizontalSquared(NPC->currentOrigin, best_enemy->currentOrigin) < 1024 * 1024)
						{
							G_SetEnemy(NPC, best_enemy);
							NPCInfo->stats.aggression = 20;
						}
					}
					else if (best_enemy->s.number)
					{
						//just attack
						G_SetEnemy(NPC, best_enemy);
						NPCInfo->stats.aggression = 3;
					}
					else if (NPC->client->NPC_class != CLASS_BOBAFETT && NPC->client->NPC_class != CLASS_MANDO)
					{
						//the player, toy with him
						//get progressively more interested over time
						if (TIMER_Done(NPC, "watchTime"))
						{
							//we want to pick him up in stages
							if (TIMER_Get(NPC, "watchTime") == -1)
							{
								//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set(NPC, "watchTime", Q_irand(3000, 5000));
								goto finish;
							}
							//okay, we've ignored him, now start to notice him
							if (!NPCInfo->investigateCount)
							{
								if (!npc_is_dark_jedi(NPC))
								{
									G_AddVoiceEvent(NPC, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000);
								}
								else
								{
									G_AddVoiceEvent(NPC, Q_irand(EV_JDETECTED1, EV_JDETECTED3), 10000);
								}
							}
							NPCInfo->investigateCount++;
							TIMER_Set(NPC, "watchTime", Q_irand(4000, 10000));
						}
						//while we're waiting, do what we need to do
						if (best_enemy_dist < 440 * 440 || NPCInfo->investigateCount >= 2)
						{
							//stage three: keep facing him
							NPC_FaceEntity(best_enemy, qtrue);
							if (best_enemy_dist < 330 * 330)
							{
								//stage four: turn on the saber
								if (!NPC->client->ps.saberInFlight)
								{
									NPC->client->ps.SaberActivate();
								}
							}
						}
						else if (best_enemy_dist < 550 * 550 || NPCInfo->investigateCount == 1)
						{
							//stage two: stop and face him every now and then
							if (TIMER_Done(NPC, "watchTime"))
							{
								NPC_FaceEntity(best_enemy, qtrue);
							}
						}
						else
						{
							//stage one: look at him.
							NPC_SetLookTarget(NPC, best_enemy->s.number, 0);
						}
					}
				}
				else if (TIMER_Done(NPC, "watchTime"))
				{
					//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget(NPC);
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}

	NPC_UpdateAngles(qtrue, qtrue);

	if (NPC->enemy)
	{
		//just picked one up
		NPCInfo->enemyCheckDebounceTime = level.time + Q_irand(3000, 10000);
	}
}

static qboolean Jedi_CanPullBackSaber(const gentity_t* self)
{
	if (self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done(self, "parryTime"))
	{
		return qfalse;
	}

	if (self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
	{
		return qtrue;
	}

	if (self->painDebounceTime > level.time)
	{
		return qfalse;
	}

	return qtrue;
}

/*
-------------------------
NPC_BSJedi_FollowLeader
-------------------------
*/
extern qboolean NAV_CheckAhead(const gentity_t* self, vec3_t end, trace_t& trace, int clipmask);

void NPC_BSJedi_FollowLeader()
{
	NPC->client->ps.saberBlocked = BLOCKED_NONE;
	if (!NPC->enemy)
	{
		Jedi_AggressionErosion(-1);
		TIMER_Set(NPC, "DeactivateTime", Q_irand(3000, 6000));
	}

	//did we drop our saber?  If so, go after it!
	if (NPC->client->ps.saberInFlight)
	{
		//saber is not in hand
		if (NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0) //player is 0
		{
			//
			if (g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY)
			{
				//fell to the ground, try to pick it up...
				if (Jedi_CanPullBackSaber(NPC))
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->ps.saberEntityNum];
					ucmd.buttons |= BUTTON_ATTACK;
					ucmd.buttons &= ~BUTTON_ALT_ATTACK;
					if (NPC->enemy && NPC->enemy->health > 0)
					{
						//get our saber back NOW!
						if (!NPC_MoveToGoal(qtrue))
						{
							//can't nav to it, try jumping to it
							NPC_FaceEntity(NPCInfo->goalEntity, qtrue);
							if (!NPC_TryJump(NPCInfo->goalEntity))
							{
								Jedi_Move(NPCInfo->goalEntity, qfalse);
							}
						}
						NPC_UpdateAngles(qtrue, qtrue);
						return;
					}
				}
			}
		}
	}

	if (NPC->s.weapon == WP_SABER && (com_outcast->integer == 1 || com_outcast->integer == 4) && NPCInfo->goalEntity)
	{
		trace_t trace;

		if (Jedi_Jumping(NPCInfo->goalEntity))
		{
			//in mid-jump
			return;
		}

		if (!NAV_CheckAhead(NPC, NPCInfo->goalEntity->currentOrigin, trace,
			NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP))
		{
			//can't get straight to him
			if (NPC_ClearLOS(NPCInfo->goalEntity) && NPC_FaceEntity(NPCInfo->goalEntity, qtrue))
			{
				//no line of sight
				if (Jedi_TryJump(NPCInfo->goalEntity))
				{
					//started a jump
					return;
				}
			}
		}
		if (NPCInfo->aiFlags & NPCAI_BLOCKED)
		{
			//try to jump to the blockedTargetPosition
			gentity_t* temp_goal = G_Spawn();
			G_SetOrigin(temp_goal, NPCInfo->blockedTargetPosition);
			gi.linkentity(temp_goal);
			if (Jedi_TryJump(temp_goal))
			{
				//going to jump to the dest
				G_FreeEntity(temp_goal);
				return;
			}
			G_FreeEntity(temp_goal);
		}
		else if (STEER::HasBeenBlockedFor(NPC, 3000))
		{
			//try to jump to the blockedDest
			if (NPCInfo->blockedTargetEntity)
			{
				NPC_TryJump(NPCInfo->blockedTargetEntity); // commented Out
			}
			else
			{
				NPC_TryJump(NPCInfo->blockedTargetPosition); // commented Out
			}
		}
	}

	//try normal movement
	NPC_BSFollowLeader();

	if (!NPC->enemy &&
		NPC->health < NPC->max_health &&
		(NPC->client->ps.forcePowersKnown & 1 << FP_HEAL) != 0 &&
		(NPC->client->ps.forcePowersActive & 1 << FP_HEAL) == 0 &&
		TIMER_Done(NPC, "FollowHealDebouncer"))
	{
		if (Q_irand(0, 3) == 0)
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(12000, 18000));
			ForceHeal(NPC);

			if (npc_is_jedi(NPC))
			{
				// Do deflect taunt...
				G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
			}
		}
		else
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(1000, 2000));
		}
	}
}

static qboolean Jedi_CheckKataAttack()
{
	if (NPCInfo->rank == RANK_CAPTAIN)
	{
		//only top-level guys and bosses do this
		if (ucmd.buttons & BUTTON_ATTACK)
		{
			//attacking
			if (g_saberNewControlScheme->integer
				&& !(ucmd.buttons & BUTTON_FORCE_FOCUS)
				|| !g_saberNewControlScheme->integer
				&& !(ucmd.buttons & BUTTON_ALT_ATTACK))
			{
				//not already going to do a kata move somehow
				if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//on the ground
					if (ucmd.upmove <= 0 && NPC->client->ps.forceJumpCharge <= 0)
					{
						//not going to try to jump
						if (Q_irand(0, g_spskill->integer + 1) //50% chance on easy, 66% on medium, 75% on hard
							&& !Q_irand(0, 9)) //10% chance overall
						{
							//base on skill level
							ucmd.upmove = 0;
							VectorClear(NPC->client->ps.moveDir);
							if (g_saberNewControlScheme->integer)
							{
								ucmd.buttons |= BUTTON_FORCE_FOCUS;
							}
							else
							{
								ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}

/*
-------------------------
Jedi_Attack
-------------------------
*/

extern qboolean PM_SaberInAttackback(int move);

static void Jedi_Attack()
{
	//Don't do anything if we're in a pain anim
	if (NPC->painDebounceTime > level.time)
	{
		if (Q_irand(0, 1))
		{
			Jedi_FaceEnemy(qtrue);
		}
		NPC_UpdateAngles(qtrue, qtrue);

		if (NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
		{
			//see if we grabbed enemy
			if (NPC->client->ps.torsoAnimTimer <= 200)
			{
				if (Kyle_CanDoGrab()
					&& NPC_EnemyRangeFromBolt(NPC->handRBolt) < 88.0f)
				{
					//grab him!
					if (NPCInfo->aiFlags & NPCAI_BOSS_SERENITYJEDIENGINE)
					{
						NPC_GrabPlayer();
					}
					else
					{
						Kyle_GrabEnemy();
					}
					return;
				}
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
				return;
			}
		}
		return;
	}

	if (NPC->enemy->s.number < MAX_CLIENTS)
	{
		if (NPC->client->NPC_class != CLASS_RANCOR
			&& NPC->client->NPC_class != CLASS_WAMPA
			&& NPC->client->NPC_class != CLASS_SAND_CREATURE
			&& NPC->client->NPC_class != CLASS_VADER
			&& NPC->client->NPC_class != CLASS_DESANN
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& !PM_SaberInAttack(NPC->client->ps.saber_move) //not attacking
			&& !PM_SaberInStart(NPC->client->ps.saber_move) //not starting an attack
			&& !PM_SpinningSaberAnim(NPC->client->ps.torsoAnim) //not in a saber spin
			&& !pm_saber_in_special_attack(NPC->client->ps.torsoAnim)
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f)
			&& IsSurrendering(NPC->enemy))
		{
			//We're ignoring all enemies for now
			if (NPC->s.weapon == WP_SABER)
			{
				WP_DeactivateSaber(NPC);
				G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), Q_irand(5000, 10000));
				return;
			}
			G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), Q_irand(5000, 10000));
			return;
		} //IsCowering
		if (NPC->client->NPC_class != CLASS_RANCOR
			&& NPC->client->NPC_class != CLASS_WAMPA
			&& NPC->client->NPC_class != CLASS_SAND_CREATURE
			&& NPC->client->NPC_class != CLASS_VADER
			&& NPC->client->NPC_class != CLASS_DESANN
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& !PM_SaberInAttack(NPC->client->ps.saber_move) //not attacking
			&& !PM_SaberInStart(NPC->client->ps.saber_move) //not starting an attack
			&& !PM_SpinningSaberAnim(NPC->client->ps.torsoAnim) //not in a saber spin
			&& !pm_saber_in_special_attack(NPC->client->ps.torsoAnim)
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f)
			&& IsCowering(NPC->enemy))
		{
			//We're ignoring all enemies for now
			if (NPC->s.weapon == WP_SABER)
			{
				WP_DeactivateSaber(NPC);
				G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), Q_irand(5000, 10000));
				return;
			}
			G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), Q_irand(5000, 10000));
			return;
		} //IsRespecting
		if (NPC->client->NPC_class != CLASS_RANCOR
			&& NPC->client->NPC_class != CLASS_WAMPA
			&& NPC->client->NPC_class != CLASS_SAND_CREATURE
			&& NPC->client->NPC_class != CLASS_VADER
			&& NPC->client->NPC_class != CLASS_DESANN
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& !PM_SaberInAttack(NPC->client->ps.saber_move) //not attacking
			&& !PM_SaberInStart(NPC->client->ps.saber_move) //not starting an attack
			&& !PM_SpinningSaberAnim(NPC->client->ps.torsoAnim) //not in a saber spin
			&& !pm_saber_in_special_attack(NPC->client->ps.torsoAnim)
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f)
			&& NPC->enemy->client->ps.communicatingflags & 1 << RESPECTING)
		{
			//NPC shows respect if you bow to him
			if (NPC->s.weapon == WP_SABER)
			{
				WP_DeactivateSaber(NPC);
				NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_BOW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (TIMER_Done(NPC, "talkDebounce") && !Q_irand(0, 10))
				{
					if (NPCInfo->enemyCheckDebounceTime < 8)
					{
						int speech = -1;
						switch (NPCInfo->enemyCheckDebounceTime)
						{
						case 0:
						case 1:
						case 2:
							speech = EV_TAUNT1 + NPCInfo->enemyCheckDebounceTime;
							break;
						case 3:
						case 4:
						case 5:
							speech = EV_GLOAT1 + NPCInfo->enemyCheckDebounceTime - 3;
							break;
						case 6:
						case 7:
							speech = EV_COMBAT1 + NPCInfo->enemyCheckDebounceTime - 6;
							break;
						default:;
						}
						NPCInfo->enemyCheckDebounceTime++;
						if (speech != -1)
						{
							G_AddVoiceEvent(NPC, speech, Q_irand(3000, 5000));
							TIMER_Set(NPC, "talkDebounce", Q_irand(5000, 7000));
						}
					}
				}
			}
			else
			{
				NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (TIMER_Done(NPC, "talkDebounce") && !Q_irand(0, 10))
				{
					if (NPCInfo->enemyCheckDebounceTime < 8)
					{
						int speech = -1;
						switch (NPCInfo->enemyCheckDebounceTime)
						{
						case 0:
						case 1:
						case 2:
							speech = EV_TAUNT1 + NPCInfo->enemyCheckDebounceTime;
							break;
						case 3:
						case 4:
						case 5:
							speech = EV_GLOAT1 + NPCInfo->enemyCheckDebounceTime - 3;
							break;
						case 6:
						case 7:
							speech = EV_COMBAT1 + NPCInfo->enemyCheckDebounceTime - 6;
							break;
						default:;
						}
						NPCInfo->enemyCheckDebounceTime++;
						if (speech != -1)
						{
							G_AddVoiceEvent(NPC, speech, Q_irand(3000, 5000));
							TIMER_Set(NPC, "talkDebounce", Q_irand(5000, 7000));
						}
					}
				}
			}
		} //IsCommunicating
		else if (NPC->client->NPC_class != CLASS_RANCOR
			&& NPC->client->NPC_class != CLASS_WAMPA
			&& NPC->client->NPC_class != CLASS_SAND_CREATURE
			&& NPC->client->NPC_class != CLASS_VADER
			&& NPC->client->NPC_class != CLASS_DESANN
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& !PM_SaberInAttack(NPC->client->ps.saber_move) //not attacking
			&& !PM_SaberInStart(NPC->client->ps.saber_move) //not starting an attack
			&& !PM_SpinningSaberAnim(NPC->client->ps.torsoAnim) //not in a saber spin
			&& !pm_saber_in_special_attack(NPC->client->ps.torsoAnim)
			&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.7f)
			&& NPC->enemy->client->ps.communicatingflags & 1 << GESTURING)
		{
			//NPC will return your gesture
			if (NPC->s.weapon == WP_SABER)
			{
				switch (NPC->client->ps.saberAnimLevel)
				{
				case SS_FAST:
				case SS_TAVION:
				case SS_MEDIUM:
				case SS_STRONG:
				case SS_DESANN:
					NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_ENGAGETAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DUAL:
					NPC->client->ps.SaberActivate();
					NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_DUAL_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					NPC->client->ps.SaberActivate();
					NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_STAFF_TAUNT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				default:;
				}
				if (TIMER_Done(NPC, "talkDebounce") && !Q_irand(0, 10))
				{
					if (NPCInfo->enemyCheckDebounceTime < 8)
					{
						int speech = -1;
						switch (NPCInfo->enemyCheckDebounceTime)
						{
						case 0:
						case 1:
						case 2:
							speech = EV_TAUNT1 + NPCInfo->enemyCheckDebounceTime;
							break;
						case 3:
						case 4:
						case 5:
							speech = EV_GLOAT1 + NPCInfo->enemyCheckDebounceTime - 3;
							break;
						case 6:
						case 7:
							speech = EV_COMBAT1 + NPCInfo->enemyCheckDebounceTime - 6;
							break;
						default:;
						}
						NPCInfo->enemyCheckDebounceTime++;
						if (speech != -1)
						{
							G_AddVoiceEvent(NPC, speech, Q_irand(3000, 5000));
							TIMER_Set(NPC, "talkDebounce", Q_irand(5000, 7000));
						}
					}
				}
			}
			else
			{
				NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_HANDSIGNAL3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (TIMER_Done(NPC, "talkDebounce") && !Q_irand(0, 10))
				{
					if (NPCInfo->enemyCheckDebounceTime < 8)
					{
						int speech = -1;
						switch (NPCInfo->enemyCheckDebounceTime)
						{
						case 0:
						case 1:
						case 2:
							speech = EV_TAUNT1 + NPCInfo->enemyCheckDebounceTime;
							break;
						case 3:
						case 4:
						case 5:
							speech = EV_GLOAT1 + NPCInfo->enemyCheckDebounceTime - 3;
							break;
						case 6:
						case 7:
							speech = EV_COMBAT1 + NPCInfo->enemyCheckDebounceTime - 6;
							break;
						default:;
						}
						NPCInfo->enemyCheckDebounceTime++;
						if (speech != -1)
						{
							G_AddVoiceEvent(NPC, speech, Q_irand(3000, 5000));
							TIMER_Set(NPC, "talkDebounce", Q_irand(5000, 7000));
						}
					}
				}
			}
		}
	}

	if (NPC->client->ps.saberLockTime > level.time)
	{
		if (NPC->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
			&& NPC->client->ps.saberLockTime < level.time + 5000
			&& !Q_irand(0, 10))
		{
			if (!Jedi_Trainer(NPC) || g_saberLockCinematicCamera->integer < 1) // dont do it in training / cam mode
			{
				ForceThrow(NPC, qfalse);
			}
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;

			if (NPC->client->NPC_class == CLASS_DESANN ||
				NPC->client->NPC_class == CLASS_SITHLORD ||
				NPC->client->NPC_class == CLASS_VADER ||
				NPC->client->NPC_class == CLASS_YODA ||
				!Q_stricmp("Yoda", NPC->NPC_type))
			{
				if (g_spskill->integer)
				{
					chance = 4.0f; //he pushes *hard*
				}
				else
				{
					chance = 3.0f; //he pushes *hard*
				}
			}
			else if (NPC->client->NPC_class == CLASS_TAVION
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| NPC->client->NPC_class == CLASS_KYLE && NPC->spawnflags & 1)
			{
				chance = 2.0f + g_spskill->value; //from 2 to 4
			}
			else if (NPC->client->NPC_class == CLASS_REBORN)
			{
				chance = 1.5f + g_spskill->value;
			}
			else
			{
				//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				constexpr float max_chance = static_cast<float>(RANK_LT) / 2.0f + 3.0f; //5?
				if (!g_spskill->value)
				{
					chance = static_cast<float>(NPCInfo->rank) / 2.0f;
				}
				else
				{
					chance = static_cast<float>(NPCInfo->rank) / 2.0f + 1.0f;
				}
				if (chance > max_chance)
				{
					chance = max_chance;
				}
			}
			if (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER)
			{
				chance += Q_irand(0, 2);
			}
			else if (NPCInfo->aiFlags & NPCAI_SUBBOSS_CHARACTER)
			{
				chance += Q_irand(-1, 1);
			}
			if (Q_flrand(-4.0f, chance) >= 0.0f && !(NPC->client->ps.pm_flags & PMF_ATTACK_HELD))
			{
				ucmd.buttons |= BUTTON_ATTACK;
			}
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}
	//did we drop our saber?  If so, go after it!
	if (NPC->client->ps.saberInFlight)
	{
		//saber is not in hand
		if (NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0) //player is 0
		{
			//
			if (g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY)
			{
				//fell to the ground, try to pick it up
				if (Jedi_CanPullBackSaber(NPC))
				{
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->ps.saberEntityNum];
					ucmd.buttons |= BUTTON_ATTACK;
					ucmd.buttons &= ~BUTTON_ALT_ATTACK;
					if (NPC->enemy && NPC->enemy->health > 0)
					{
						//get our saber back NOW!
						Jedi_Move(NPCInfo->goalEntity, qfalse);
						NPC_UpdateAngles(qtrue, qtrue);
						if (NPC->enemy->s.weapon == WP_SABER)
						{
							//be sure to continue evasion
							vec3_t enemy_dir, enemy_movedir, enemy_dest;
							float enemy_dist, enemy_movespeed;
							jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed,
								300);
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
						return;
					}
				}
				else
				{
					if (NPC->enemy && NPC->enemy->health > 0)
					{
						if (NPC->enemy->s.weapon == WP_SABER)
						{
							//be sure to continue evasion even if can't pull back saber yet
							vec3_t enemy_dir, enemy_movedir, enemy_dest;
							float enemy_dist, enemy_movespeed;
							jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed,
								300);
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
					}
				}
			}
			if (NPC->enemy && NPC->enemy->health > 0)
			{
				if (NPC->enemy->s.weapon == WP_SABER)
				{
					//be sure to continue evasion even if can't pull back saber yet
					vec3_t enemy_dir, enemy_movedir, enemy_dest;
					float enemy_dist, enemy_movespeed;
					jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
			}
		}
	}

	//see if our enemy was killed by us, gloat and turn off saber after cool down.
	if (NPC->enemy)
	{
		if (NPC->enemy->health <= 0
			&& NPC->enemy->enemy == NPC
			&& (NPC->client->playerTeam != TEAM_PLAYER || (NPC->client->NPC_class == CLASS_KYLE || NPC->client->
				NPC_class == CLASS_YODA) && NPC->spawnflags & 1 && NPC->enemy == player))
			//good guys don't gloat (unless it's Kyle having just killed his student
		{
			//my enemy is dead and I killed him
			NPCInfo->enemyCheckDebounceTime = 0; //keep looking for others

			if (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO
				|| NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER)
			{
				if (NPCInfo->walkDebounceTime < level.time && NPCInfo->walkDebounceTime >= 0)
				{
					TIMER_Set(NPC, "gloatTime", 10000);
					NPCInfo->walkDebounceTime = -1;
				}
				if (!TIMER_Done(NPC, "gloatTime"))
				{
					if (DistanceHorizontalSquared(NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin) > 4096 &&
						NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) //64 squared
					{
						NPCInfo->goalEntity = NPC->enemy;
						Jedi_Move(NPC->enemy, qfalse);
						ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{
						TIMER_Set(NPC, "gloatTime", 0);
					}
				}
				else if (NPCInfo->walkDebounceTime == -1)
				{
					NPCInfo->walkDebounceTime = -2;
					G_AddVoiceEvent(NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), 3000);
					jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
					NPCInfo->desiredPitch = 0;
					NPCInfo->goalEntity = nullptr;
				}
				Jedi_FaceEnemy(qtrue);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
			if (!TIMER_Done(NPC, "parryTime"))
			{
				TIMER_Set(NPC, "parryTime", -1);
				NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			if (NPC->client->ps.SaberActive() || NPC->client->ps.saberInFlight)
			{
				Jedi_AggressionErosion(-3);
				TIMER_Set(NPC, "DeactivateTime", Q_irand(3000, 6000));
				if (!NPC->client->ps.SaberActive() && !NPC->client->ps.saberInFlight)
				{
					//turned off saber (in hand), gloat
					G_AddVoiceEvent(NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), 3000);
					jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
					NPCInfo->desiredPitch = 0;
					NPCInfo->goalEntity = nullptr;
				}
				TIMER_Set(NPC, "gloatTime", 10000);
			}
			if (NPC->client->ps.SaberActive() || NPC->client->ps.saberInFlight || !TIMER_Done(NPC, "gloatTime"))
			{
				//keep walking
				if (DistanceHorizontalSquared(NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin) > 4096 &&
					NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) //64 squared
				{
					NPCInfo->goalEntity = NPC->enemy;
					Jedi_Move(NPC->enemy, qfalse);
					ucmd.buttons |= BUTTON_WALKING;
				}
				else
				{
					//got there
					if (NPC->health < NPC->max_health)
					{
						if (NPC->client->ps.saber[0].type == SABER_SITH_SWORD
							&& NPC->weaponModel[0] != -1)
						{
							Tavion_SithSwordRecharge();
						}
						else if ((NPC->client->ps.forcePowersKnown & 1 << FP_HEAL) != 0
							&& (NPC->client->ps.forcePowersActive & 1 << FP_HEAL) == 0)
						{
							ForceHeal(NPC);

							if (npc_is_jedi(NPC))
							{
								// Do deflect taunt...
								G_AddVoiceEvent(NPC, Q_irand(EV_GLOAT1, EV_GLOAT3), 5000 + Q_irand(0, 15000));
							}
						}
					}
				}
				Jedi_FaceEnemy(qtrue);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//If we don't have an enemy, just idle
	if (NPC->enemy && NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp("PAS", NPC->enemy->classname))
	{
		if (NPC->enemy->count <= 0)
		{
			//it's out of ammo
			if (NPC->enemy->activator && NPC_ValidEnemy(NPC->enemy->activator))
			{
				gentity_t* turretOwner = NPC->enemy->activator;
				G_ClearEnemy(NPC);
				G_SetEnemy(NPC, turretOwner);
			}
			else
			{
				G_ClearEnemy(NPC);
			}
		}
	}
	else if (NPC->enemy && NPC->enemy->NPC
		&& (NPC->enemy->NPC->charmedTime > level.time || NPC->enemy->NPC->darkCharmedTime > level.time))
	{
		//my enemy was charmed
		if (OnSameTeam(NPC, NPC->enemy))
		{
			//has been charmed to be on my team
			G_ClearEnemy(NPC);
		}
	}
	if (NPC->client->playerTeam == TEAM_ENEMY
		&& NPC->client->enemyTeam == TEAM_PLAYER
		&& NPC->enemy
		&& NPC->enemy->client
		&& NPC->enemy->client->playerTeam != NPC->client->enemyTeam
		&& OnSameTeam(NPC, NPC->enemy)
		&& !(NPC->svFlags & SVF_LOCKEDENEMY))
	{
		//an evil jedi somehow got another evil NPC as an enemy, they were probably charmed and it's run out now
		if (!NPC_ValidEnemy(NPC->enemy))
		{
			G_ClearEnemy(NPC);
		}
	}

	NPC_CheckEnemy(qtrue, qtrue);

	if (!NPC->enemy)
	{
		NPC->client->ps.saberBlocked = BLOCKED_NONE;
		if (NPCInfo->tempBehavior == BS_HUNT_AND_KILL)
		{
			//lost him, go back to what we were doing before
			NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles(qtrue, qtrue);
			return;
		}
		Jedi_Patrol(); //was calling Idle... why?
		return;
	}

	//always face enemy if have one
	NPCInfo->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat();

	if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		|| NPC->client->ps.forcePowersActive & 1 << FP_HEAL && NPC->client->ps.forcePowerLevel[FP_HEAL] <
		FORCE_LEVEL_2)
	{
		//this is really stupid, but okay...
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		if (ucmd.upmove > 0)
		{
			ucmd.upmove = 0;
		}
		NPC->client->ps.forceJumpCharge = 0;
		VectorClear(NPC->client->ps.moveDir);
	}

	if (NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		VectorClear(NPC->client->ps.moveDir);
	}

	if (!TIMER_Done(NPC, "duck"))
	{
		ucmd.upmove = -127;
	}

	if (NPC->client->NPC_class != CLASS_BOBAFETT && NPC->client->NPC_class != CLASS_MANDO
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER)
	{
		if (PM_SaberInBrokenParry(NPC->client->ps.saber_move) || NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN ||
			NPC->client->ps.saberBlocked == BLOCKED_ATK_BOUNCE)
		{
			//just make sure they don't pull their saber to them if they're being blocked
			ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_KICK | BUTTON_BLOCK |
				BUTTON_USE_FORCE);
		}
	}

	if (NPCInfo->aiFlags & NPCAI_BOSS_CHARACTER || NPCInfo->aiFlags & NPCAI_SUBBOSS_CHARACTER || NPC->s.weapon ==
		WP_SABER)
	{
		if (PM_SaberInBrokenParry(NPC->client->ps.saber_move) || NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN ||
			NPC->client->ps.saberBlocked == BLOCKED_ATK_BOUNCE)
		{
			//just make sure they don't pull their saber to them if they're being blocked
			ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_KICK | BUTTON_BLOCK |
				BUTTON_USE_FORCE);
		}
	}

	if (NPCInfo->scriptFlags & SCF_DONT_FIRE //not allowed to attack
		|| NPC->client->ps.forcePowersActive & 1 << FP_HEAL && NPC->client->ps.forcePowerLevel[FP_HEAL] <
		FORCE_LEVEL_3
		|| NPC->client->ps.saberEventFlags & SEF_INWATER && !NPC->client->ps.saberInFlight) //saber in water
	{
		ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS);
	}

	if (NPCInfo->scriptFlags & SCF_NO_ACROBATICS)
	{
		ucmd.upmove = 0;
		NPC->client->ps.forceJumpCharge = 0;
	}

	if (NPC->client->NPC_class != CLASS_BOBAFETT && NPC->client->NPC_class != CLASS_MANDO
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& (NPC->client->NPC_class != CLASS_SITHLORD || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER)
	{
		Jedi_CheckDecreasesaber_anim_level();
	}

	if (ucmd.buttons & BUTTON_ATTACK && NPC->client->playerTeam == TEAM_ENEMY)
	{
		if (Q_irand(0, NPC->client->ps.saberAnimLevel) > 0
			&& Q_irand(0, NPC->max_health + 10) > NPC->health
			&& !Q_irand(0, 3))
		{
			//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 1000);
		}
	}

	if (Jedi_CheckKataAttack())
	{
		//doing a kata attack
	}
	else
	{
		//check other special combat behavior
		if (NPC->client->NPC_class != CLASS_BOBAFETT && NPC->client->NPC_class != CLASS_MANDO
			&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
			&& (NPC->client->NPC_class != CLASS_SITHLORD || NPC->s.weapon == WP_SABER)
			&& NPC->client->NPC_class != CLASS_ROCKETTROOPER)
		{
			if (NPC->client->NPC_class == CLASS_TAVION
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| g_spskill->integer && (NPC->client->NPC_class == CLASS_DESANN || NPC->client->NPC_class ==
					CLASS_SITHLORD || NPC->client->NPC_class == CLASS_VADER || NPCInfo->rank >= Q_irand(
						RANK_CREWMAN, RANK_CAPTAIN)))
			{
				//Tavion will kick in force speed if the player does...
				if (NPC->enemy
					&& !NPC->enemy->s.number
					&& NPC->enemy->client
					&& NPC->enemy->client->ps.forcePowersActive & 1 << FP_SPEED
					&& !(NPC->client->ps.forcePowersActive & 1 << FP_SPEED))
				{
					int chance = 0;
					switch (g_spskill->integer)
					{
					case 0:
						chance = 9;
						break;
					case 1:
						chance = 3;
						break;
					case 2:
						chance = 1;
						break;
					default:;
					}
					if (!Q_irand(0, chance))
					{
						ForceSpeed(NPC, 0);
					}
				}
			}
		}
		//Sometimes Alora flips towards you instead of runs
		if (NPC->client->NPC_class == CLASS_ALORA)
		{
			if (ucmd.buttons & BUTTON_ALT_ATTACK)
			{
				//chance of doing a special dual saber throw
				if (NPC->client->ps.saberAnimLevel == SS_DUAL
					&& !NPC->client->ps.saberInFlight)
				{
					if (Distance(NPC->enemy->currentOrigin, NPC->currentOrigin) >= 120)
					{
						NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ALORA_SPIN_THROW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD,
							0);
						NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
					}
				}
			}
			else if (NPC->enemy
				&& ucmd.forwardmove > 0
				&& fabs(static_cast<float>(ucmd.rightmove)) < 32
				&& !(ucmd.buttons & BUTTON_WALKING)
				&& !(ucmd.buttons & BUTTON_ATTACK)
				&& NPC->client->ps.saber_move == LS_READY
				&& NPC->client->ps.legsAnim == BOTH_RUN_DUAL)
			{
				//running at us, not attacking
				if (Distance(NPC->enemy->currentOrigin, NPC->currentOrigin) > 80)
				{
					if (NPC->client->ps.legsAnim == BOTH_FLIP_F
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1_MD2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2_MD2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3_MD2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3)
					{
						if (NPC->client->ps.legsAnimTimer <= 200 && Q_irand(0, 2))
						{
							//go ahead and start anotther
							NPC_SetAnim(NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3),
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
						}
					}
					else if (!Q_irand(0, 6))
					{
						NPC_SetAnim(NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3),
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
					}
				}
			}
		}
	}

	if (VectorCompare(NPC->client->ps.moveDir, vec3_origin)
		&& (ucmd.forwardmove || ucmd.rightmove))
	{
		//using ucmds to move this turn, not NAV
		if (ucmd.buttons & BUTTON_WALKING)
		{
			//FIXME: NAV system screws with speed directly, so now I have to re-set it myself!
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
		}
		else
		{
			NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		}
	}
}

qboolean Rosh_BeingHealed(const gentity_t* self)
{
	if (self
		&& self->NPC
		&& self->client
		&& self->NPC->aiFlags & NPCAI_ROSH
		&& self->flags & FL_UNDYING
		&& (self->health == 1 //need healing
			|| self->client->ps.powerups[PW_INVINCIBLE] > level.time)) //being healed
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Rosh_TwinPresent()
{
	const gentity_t* found_twin = G_Find(nullptr, FOFS(NPC_type), "DKothos");
	if (!found_twin
		|| found_twin->health < 0)
	{
		found_twin = G_Find(nullptr, FOFS(NPC_type), "VKothos");
	}
	if (!found_twin
		|| found_twin->health < 0)
	{
		//oh well, both twins are dead...
		return qfalse;
	}
	return qtrue;
}

qboolean Rosh_TwinNearBy(const gentity_t* self)
{
	const gentity_t* found_twin = G_Find(nullptr, FOFS(NPC_type), "DKothos");
	if (!found_twin
		|| found_twin->health < 0)
	{
		found_twin = G_Find(nullptr, FOFS(NPC_type), "VKothos");
	}
	if (!found_twin
		|| found_twin->health < 0)
	{
		//oh well, both twins are dead...
		return qfalse;
	}
	if (self->client
		&& found_twin->client)
	{
		if (Distance(self->currentOrigin, found_twin->currentOrigin) <= 512.0f
			&& G_ClearLineOfSight(self->client->renderInfo.eyePoint, found_twin->client->renderInfo.eyePoint,
				found_twin->s.number, MASK_OPAQUE))
		{
			//make them look charge me for a bit while I do this
			TIMER_Set(self, "chargeMeUp", Q_irand(2000, 4000));
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean Kothos_HealRosh()
{
	if (NPC->client
		&& NPC->client->leader
		&& NPC->client->leader->client)
	{
		if (DistanceSquared(NPC->client->leader->currentOrigin, NPC->currentOrigin) <= 256 * 256
			&& G_ClearLineOfSight(NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint,
				NPC->s.number, MASK_OPAQUE))
		{
			//NPC_FaceEntity( NPC->client->leader, qtrue );
			NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPC->client->ps.torsoAnimTimer = 1000;

			//FIXME: unique effect and sound
			//NPC->client->ps.eFlags |= EF_POWERING_ROSH;
			if (NPC->ghoul2.size())
			{
				mdxaBone_t boltMatrix;
				vec3_t fx_org, fx_dir;
				const vec3_t angles = { 0, NPC->currentAngles[YAW], 0 };

				gi.G2API_GetBoltMatrix(NPC->ghoul2, NPC->playerModel,
					Q_irand(0, 1) ? NPC->handLBolt : NPC->handRBolt,
					&boltMatrix, angles, NPC->currentOrigin, cg.time ? cg.time : level.time,
					nullptr, NPC->s.modelScale);
				gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, fx_org);
				VectorSubtract(NPC->client->leader->currentOrigin, fx_org, fx_dir);
				VectorNormalize(fx_dir);
				G_PlayEffect(G_EffectIndex("force/kothos_beam.efx"), fx_org, fx_dir);
			}
			//BEG HACK LINE
			gentity_t* tent = G_TempEntity(NPC->currentOrigin, EV_KOTHOS_BEAM);
			tent->svFlags |= SVF_BROADCAST;
			tent->s.otherentity_num = NPC->s.number;
			tent->s.otherentity_num2 = NPC->client->leader->s.number;
			//END HACK LINE

			NPC->client->leader->health += Q_irand(1 + g_spskill->integer * 2, 4 + g_spskill->integer * 3);
			//from 1-5 to 4-10
			if (NPC->client->leader->client)
			{
				if (NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& NPC->client->leader->health >= NPC->client->leader->max_health)
				{
					//let him get up now
					NPC_SetAnim(NPC->client->leader, SETANIM_BOTH, BOTH_FORCEHEAL_STOP,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					//FIXME: temp effect
					G_PlayEffect(G_EffectIndex("force/kothos_recharge.efx"), NPC->client->leader->playerModel, 0,
						NPC->client->leader->s.number, NPC->client->leader->currentOrigin,
						NPC->client->leader->client->ps.torsoAnimTimer, qfalse);
					//make him invincible while we recharge him
					NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->leader->client->
						ps.torsoAnimTimer;
					NPC->client->leader->NPC->ignorePain = qfalse;
					NPC->client->leader->health = NPC->client->leader->max_health;
				}
				else
				{
					G_PlayEffect(G_EffectIndex("force/kothos_recharge.efx"), NPC->client->leader->playerModel, 0,
						NPC->client->leader->s.number, NPC->client->leader->currentOrigin, 500, qfalse);
					NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
				}
			}
			//decrement
			NPC->count--;
			if (!NPC->count)
			{
				TIMER_Set(NPC, "healRoshDebounce", Q_irand(5000, 10000));
				NPC->count = 100;
			}
			//now protect me, too
			if (g_spskill->integer)
			{
				//not on easy
				G_PlayEffect(G_EffectIndex("force/kothos_recharge.efx"), NPC->playerModel, 0, NPC->s.number,
					NPC->currentOrigin, 500, qfalse);
				NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
			}
			return qtrue;
		}
	}
	return qfalse;
}

static void Kothos_PowerRosh()
{
	if (NPC->client
		&& NPC->client->leader)
	{
		if (Distance(NPC->client->leader->currentOrigin, NPC->currentOrigin) <= 512.0f
			&& G_ClearLineOfSight(NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint,
				NPC->s.number, MASK_OPAQUE))
		{
			NPC_FaceEntity(NPC->client->leader, qtrue);
			NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPC->client->ps.torsoAnimTimer = 500;
			G_PlayEffect(G_EffectIndex("force/kothos_beam.efx"), NPC->playerModel, NPC->handLBolt, NPC->s.number,
				NPC->currentOrigin, 500, qfalse);
			if (NPC->client->leader->client)
			{
				//hmm, give him some force?
				NPC->client->leader->client->ps.forcePower++;
			}
		}
	}
}

static qboolean Kothos_Retreat()
{
	STEER::Activate(NPC);
	STEER::Evade(NPC, NPC->enemy);
	STEER::AvoidCollisions(NPC, NPC->client->leader);
	STEER::DeActivate(NPC, &ucmd);
	if (NPCInfo->aiFlags & NPCAI_BLOCKED)
	{
		if (level.time - NPCInfo->blockedDebounceTime > 1000)
		{
			return qfalse;
		}
	}
	return qtrue;
}

constexpr auto TWINS_DANGER_DIST_EASY = 128.0f * 128.0f;
constexpr auto TWINS_DANGER_DIST_MEDIUM = 192.0f * 192.0f;
constexpr auto TWINS_DANGER_DIST_HARD = 256.0f * 256.0f;

static float Twins_DangerDist()
{
	switch (g_spskill->integer)
	{
	case 0:
		return TWINS_DANGER_DIST_EASY;
	case 1:
		return TWINS_DANGER_DIST_MEDIUM;
	case 2:
	default:
		return TWINS_DANGER_DIST_HARD;
	}
}

static qboolean Jedi_InSpecialMove()
{
	if (NPC->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_2
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_3
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED)
	{
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (Jedi_InNoAIAnim(NPC))
	{
		//in special anims, don't do force powers or attacks, just face the enemy
		if (NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}

	if (NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
	{
		if (!TIMER_Done(NPC, "draining"))
		{
			//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			ucmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (NPC->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER)
	{
		NPC->health += Q_irand(1, 2);
		if (NPC->health > NPC->max_health)
		{
			NPC->health = NPC->max_health;
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (NPC->client->ps.torsoAnim == BOTH_SCEPTER_START)
	{
		if (NPC->client->ps.torsoAnimTimer <= 100)
		{
			//go into the hold
			NPC->s.loopSound = G_SoundIndex("sound/weapons/scepter/loop.wav");
			G_PlayEffect(G_EffectIndex("scepter/beam.efx"), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number,NPC->currentOrigin, 10000, qtrue);
			NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_SCEPTER_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPC->client->ps.torsoAnimTimer += 200;
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear(NPC->client->ps.velocity);
			VectorClear(NPC->client->ps.moveDir);
		}
		if (NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD)
	{
		if (NPC->client->ps.torsoAnimTimer <= 100)
		{
			NPC->s.loopSound = 0;
			G_StopEffect(G_EffectIndex("scepter/beam.efx"), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number);
			NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear(NPC->client->ps.velocity);
			VectorClear(NPC->client->ps.moveDir);
		}
		else
		{
			Tavion_ScepterDamage();
		}
		if (NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPC->client->ps.torsoAnim == BOTH_SCEPTER_STOP)
	{
		if (NPC->enemy)
		{
			NPC_FaceEnemy(qtrue);
		}
		else
		{
			NPC_UpdateAngles(qtrue, qtrue);
		}
		return qtrue;
	}
	if (NPC->client->ps.torsoAnim == BOTH_TAVION_SCEPTERGROUND)
	{
		if (NPC->client->ps.torsoAnimTimer <= 1200
			&& !NPC->count)
		{
			Tavion_ScepterSlam();
			NPC->count = 1;
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return qtrue;
	}

	if (Jedi_CultistDestroyer(NPC))
	{
		if (!NPC->takedamage)
		{
			//ready to explode
			if (NPC->useDebounceTime <= level.time)
			{
				NPC->client->playerTeam = TEAM_FREE; //FIXME: will this destroy wampas, tusken & rancors?
				WP_Explode(NPC);
				return qtrue;
			}
			if (NPC->enemy)
			{
				NPC_FaceEnemy(qfalse);
			}
			return qtrue;
		}
	}

	if (NPC->client->NPC_class == CLASS_REBORN)
	{
		if (NPCInfo->aiFlags & NPCAI_HEAL_ROSH)
		{
			if (!NPC->client->leader)
			{
				//find Rosh
				NPC->client->leader = G_Find(nullptr, FOFS(NPC_type), "rosh_dark");
			}
			if (NPC->client->leader)
			{
				qboolean helping_rosh = qfalse;
				NPC->flags |= FL_LOCK_PLAYER_WEAPONS;
				NPC->client->leader->flags |= FL_UNDYING;
				if (NPC->client->leader->client)
				{
					NPC->client->leader->client->ps.forcePowersKnown |= FORCE_POWERS_ROSH_FROM_TWINS;
				}
				if (NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START)
				{
					if (TIMER_Done(NPC, "healRoshDebounce"))
					{
						if (Kothos_HealRosh())
						{
							helping_rosh = qtrue;
						}
						else
						{
							//can't get to him!
							NPC_BSJedi_FollowLeader();
							NPC_UpdateAngles(qtrue, qtrue);
							return qtrue;
						}
					}
				}

				if (helping_rosh)
				{
					WP_ForcePowerStop(NPC, FP_LIGHTNING_STRIKE);
					WP_ForcePowerStop(NPC, FP_LIGHTNING);
					WP_ForcePowerStop(NPC, FP_DRAIN);
					WP_ForcePowerStop(NPC, FP_GRIP);
					WP_ForcePowerStop(NPC, FP_GRASP);
					NPC_FaceEntity(NPC->client->leader, qtrue);
					return qtrue;
				}
				if (NPC->enemy && DistanceSquared(NPC->enemy->currentOrigin, NPC->currentOrigin) < Twins_DangerDist())
				{
					if (NPC->enemy && Kothos_Retreat())
					{
						NPC_FaceEnemy(qtrue);
						if (TIMER_Done(NPC, "attackDelay"))
						{
							if (NPC->painDebounceTime > level.time
								|| NPC->health < 100 && Q_irand(-20, (g_spskill->integer + 1) * 10) > 0
								|| !Q_irand(0, 80 - g_spskill->integer * 20))
							{
								NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
								switch (Q_irand(0, 7 + g_spskill->integer)) //on easy: no lightning
								{
								case 0:
								case 1:
								case 2:
								case 3:
									ForceThrow(NPC, qfalse, qfalse);
									NPC->client->ps.weaponTime = Q_irand(1000, 3000) + (2 - g_spskill->integer) * 1000;
									if (NPC->painDebounceTime <= level.time
										&& NPC->health >= 100)
									{
										TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
									}
									break;
								case 4:
								case 5:
									ForceDrain2(NPC);
									NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_spskill->integer) * 2000;
									TIMER_Set(NPC, "draining", NPC->client->ps.weaponTime);
									if (NPC->painDebounceTime <= level.time
										&& NPC->health >= 100)
									{
										TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
									}
									break;
								case 6:
								case 7:
									if (NPC->enemy && InFOV(NPC->enemy->currentOrigin, NPC->currentOrigin,
										NPC->client->ps.viewangles, 20, 30))
									{
										NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_spskill->integer) *
											2000;
										TIMER_Set(NPC, "gripping", 3000);
										if (NPC->painDebounceTime <= level.time
											&& NPC->health >= 100)
										{
											TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
										}
									}
									break;
								case 8:
								case 9:
								default:
									ForceLightning(NPC);
									if (NPC->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
									{
										NPC->client->ps.weaponTime = Q_irand(3000, 6000) + (2 - g_spskill->integer) *
											2000;
										TIMER_Set(NPC, "holdLightning", NPC->client->ps.weaponTime);
									}
									if (NPC->painDebounceTime <= level.time
										&& NPC->health >= 100)
									{
										TIMER_Set(NPC, "attackDelay", NPC->client->ps.weaponTime);
									}
									break;
								}
							}
						}
						else
						{
							NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
						}
						Jedi_TimersApply();
						return qtrue;
					}
					NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
				}
				else if (!G_ClearLOS(NPC, NPC->client->leader)
					|| DistanceSquared(NPC->currentOrigin, NPC->client->leader->currentOrigin) > 512 * 512)
				{
					//can't see Rosh or too far away, catch up with him
					if (!TIMER_Done(NPC, "attackDelay"))
					{
						NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					NPC_BSJedi_FollowLeader();
					NPC_UpdateAngles(qtrue, qtrue);
					return qtrue;
				}
				else
				{
					if (!TIMER_Done(NPC, "attackDelay"))
					{
						NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					STEER::Activate(NPC);
					STEER::Stop(NPC);
					STEER::DeActivate(NPC, &ucmd);
					NPC_FaceEnemy(qtrue);
					return qtrue;
				}
			}
			NPC_UpdateAngles(qtrue, qtrue);
		}
		else if (NPCInfo->aiFlags & NPCAI_ROSH)
		{
			if (NPC->flags & FL_UNDYING)
			{
				//Vil and/or Dasariah still around to heal me
				if (NPC->health == 1 //need healing
					|| NPC->client->ps.powerups[PW_INVINCIBLE] > level.time) //being healed
				{
					//FIXME: custom anims
					if (Rosh_TwinPresent())
					{
						if (!NPC->client->ps.weaponTime)
						{
							//not attacking
							if (NPC->client->ps.legsAnim != BOTH_FORCEHEAL_START
								&& NPC->client->ps.legsAnim != BOTH_FORCEHEAL_STOP)
							{
								//get down and wait for Vil or Dasariah to help us
								//FIXME: sound?
								NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
								NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_FORCEHEAL_START,
									SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								NPC->client->ps.torsoAnimTimer = NPC->client->ps.legsAnimTimer = -1;
								NPC->client->ps.SaberDeactivate();
								NPCInfo->ignorePain = qtrue;
							}
						}
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
						NPC->client->ps.saber_move = NPC->client->ps.saberMoveNext = LS_NONE;
						NPC->painDebounceTime = level.time + 500;
						NPC->client->ps.pm_time = 500;
						NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						VectorClear(NPC->client->ps.velocity);
						VectorClear(NPC->client->ps.moveDir);
						return qtrue;
					}
				}
			}
		}
	}

	if (PM_SuperBreakWinAnim(NPC->client->ps.torsoAnim))
	{
		NPC_FaceEnemy(qtrue);
		if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(NPC->client->ps.velocity);
		}
		VectorClear(NPC->client->ps.moveDir);
		ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
		return qtrue;
	}

	return qfalse;
}

void NPC_CheckEvasion()
{
	vec3_t enemy_dir, enemy_movedir, enemy_dest;
	float enemy_dist, enemy_movespeed;

	if (in_camera)
	{
		return;
	}

	if (!NPC->enemy || !NPC->enemy->inuse || NPC->enemy->NPC && NPC->enemy->health <= 0)
	{
		return;
	}

	switch (NPC->client->NPC_class)
	{
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_GALAK:
	case CLASS_GRAN:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	case CLASS_JAN:
	case CLASS_LANDO:
	case CLASS_MONMOTHA:
	case CLASS_PRISONER:
	case CLASS_PROTOCOL:
	case CLASS_REBEL:
	case CLASS_REELO:
	case CLASS_RODIAN:
	case CLASS_TRANDOSHAN:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	case CLASS_MANDO:
	case CLASS_SABOTEUR:
	case CLASS_STORMTROOPER:
	case CLASS_CLONETROOPER:
	case CLASS_STORMCOMMANDO:
	case CLASS_SWAMPTROOPER:
	case CLASS_NOGHRI:
	case CLASS_UGNAUGHT:
	case CLASS_WOOKIE:
		// OK... EVADE AWAY!!!
		break;
	default:
		// NOT OK...
		return;
	}

	//See where enemy will be 300 ms from now
	jedi_set_enemy_info(enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300);

	if (NPC->enemy->s.weapon == WP_SABER)
	{
		Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
	}
	else
	{
		//do we need to do any evasion for other kinds of enemies?
		if (NPC->enemy->client)
		{
			vec3_t shot_dir, ang;

			VectorSubtract(NPC->currentOrigin, NPC->enemy->currentOrigin, shot_dir);
			vectoangles(shot_dir, ang);

			if (NPC->enemy->client->ps.weaponstate == WEAPON_FIRING && in_field_of_vision(NPC->enemy->client->ps.viewangles, 90, ang))
			{
				// They are shooting at us. Evade!!!
				if (NPC->enemy->s.weapon == WP_SABER)
				{
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
				else
				{
					NPC_StartFlee(NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER, 1000, 3000);
				}
			}
			else if (in_field_of_vision(NPC->enemy->client->ps.viewangles, 60, ang))
			{
				// Randomly (when they are targetting us)... Evade!!!
				if (NPC->enemy->s.weapon == WP_SABER)
				{
					Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
				}
				else
				{
					NPC_StartFlee(NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER, 1000, 3000);
				}
			}
			else
			{
				gentity_t* entity_list[MAX_GENTITIES];

				vec3_t mins{};
				vec3_t maxs{};

				for (int e = 0; e < 3; e++)
				{
					mins[e] = NPC->currentOrigin[e] - 256;
					maxs[e] = NPC->currentOrigin[e] + 256;
				}

				const int num_ents = gi.EntitiesInBox(mins, maxs, entity_list, MAX_GENTITIES);

				for (int i = 0; i < num_ents; i++)
				{
					const gentity_t* missile = &g_entities[i];

					if (!missile) continue;
					if (!missile->inuse) continue;

					if (missile->s.eType == ET_MISSILE)
					{
						// Missile incoming!!! Evade!!!
						if (NPC->enemy->s.weapon == WP_SABER)
						{
							Jedi_EvasionSaber(enemy_movedir, enemy_dist, enemy_dir);
						}
						else
						{
							NPC_StartFlee(NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER, 1000, 3000);
						}
						return;
					}
				}
			}
		}
	}
}

extern void NPC_BSST_Patrol();
extern void NPC_BSSniper_Default();
extern void G_UcmdMoveForDir(const gentity_t* self, usercmd_t* cmd, vec3_t dir);

void NPC_BSJedi_Default()
{
	if (Jedi_InSpecialMove())
	{
		return;
	}

	if (NPC->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_3)
	{
		//ramp up his blocking
		NPC->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
	}

	Jedi_CheckCloak();

	if (!NPC->enemy)
	{
		//don't have an enemy, look for one
		if (!npc_is_jedi(NPC))
		{
			NPC_BSST_Patrol();
		}
		else
		{
			Jedi_Patrol();
		}
	}
	else
	{
		//have an enemy
		if (Jedi_WaitingAmbush(NPC))
		{
			//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush(NPC);
		}

		if (Jedi_CultistDestroyer(NPC) && !NPCInfo->charmedTime)
		{
			//destroyer
			//permanent effect
			NPCInfo->charmedTime = Q3_INFINITE;
			NPC->client->ps.forcePowersActive |= 1 << FP_RAGE;
			NPC->client->ps.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			NPC->s.loopSound = G_SoundIndex("sound/movers/objects/green_beam_lp2.wav");
		}

		if (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDO)
		{
			if (NPC->enemy->enemy != NPC && NPC->health == NPC->client->pers.maxHealth && DistanceSquared(
				NPC->currentOrigin, NPC->enemy->currentOrigin) > 800 * 800)
			{
				NPCInfo->scriptFlags |= SCF_altFire;
				Boba_ChangeWeapon(WP_DISRUPTOR);
				NPC_BSSniper_Default();
				return;
			}
		}

		if (NPC->enemy->s.weapon != WP_SABER)
		{
			// Normal non-jedi enemy
			if (!in_camera)
			{
				NPC_CheckEvasion();
			}

			Jedi_Attack();
		}
		else
		{
			Jedi_Attack();
		}

		npc_check_speak(NPC);

		//if we have multiple-jedi combat, probably need to keep checking (at certain debounce intervals) for a better (closer, more active) enemy and switch if needbe...
		if ((!ucmd.buttons && !NPC->client->ps.forcePowersActive || NPC->enemy && NPC->enemy->health <= 0) &&
			NPCInfo->enemyCheckDebounceTime < level.time)
		{
			//not doing anything (or walking toward a vanquished enemy - fixme: always taunt the player?), not using force powers and it's time to look again
			//FIXME: build a list of all local enemies (since we have to find best anyway) for other AI factors- like when to use group attacks, determine when to change tactics, when surrounded, when blocked by another in the enemy group, etc.  Should we build this group list or let the enemies maintain their own list and we just access it?
			gentity_t* sav_enemy = NPC->enemy; //FIXME: what about NPC->lastEnemy?
			NPC->enemy = nullptr;
			gentity_t* new_enemy = NPC_CheckEnemy(
				static_cast<qboolean>(NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time),
				qfalse, qfalse);
			NPC->enemy = sav_enemy;
			if (new_enemy && new_enemy != sav_enemy)
			{
				//picked up a new enemy!
				NPC->lastEnemy = NPC->enemy;
				G_SetEnemy(NPC, new_enemy);
			}
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand(1000, 3000);
		}
	}
	if (NPC->client->ps.saber[0].type == SABER_SITH_SWORD
		&& NPC->weaponModel[0] != -1)
	{
		if (NPC->health < 100
			&& !Q_irand(0, 20))
		{
			Tavion_SithSwordRecharge();
		}
	}
}