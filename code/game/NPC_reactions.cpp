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

//NPC_reactions.cpp

#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"

extern qboolean G_CheckForStrongAttackMomentum(const gentity_t* self);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speak_debounce_time);
extern int PM_AnimLength(int index, animNumber_t anim);
extern void cgi_S_StartSound(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);
extern int PM_PickAnim(const gentity_t* self, int minAnim, int maxAnim);
extern qboolean NPC_CheckLookTarget(const gentity_t* self);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clear_time);
extern qboolean Jedi_WaitingAmbush(const gentity_t* self);
extern void Jedi_Ambush(gentity_t* self);
extern qboolean G_EntIsBreakable(int entityNum, const gentity_t* breaker);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SpinningSaberAnim(int anim);
extern qboolean PM_SpinningAnim(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_FlippingAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern qboolean PM_InCartwheel(int anim);
extern qboolean IsSurrendering(const gentity_t* self);
extern qboolean IsRespecting(const gentity_t* self);
extern qboolean IsAnimRequiresResponce(const gentity_t* self);
extern qboolean IsCowering(const gentity_t* self);
extern void WP_DeactivateSaber(const gentity_t* self, qboolean clear_length = qfalse);
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInAttack(int move);

extern cvar_t* g_spskill;
extern qboolean stop_icarus;
extern int killPlayerTimer;

float g_crosshairEntDist = Q3_INFINITE;
int g_crosshairSameEntTime = 0;
int g_crosshairEntNum = ENTITYNUM_NONE;
int g_crosshairEntTime = 0;
/*
-------------------------
NPC_CheckAttacker
-------------------------
*/
static void NPC_CheckAttacker(gentity_t* other, const int mod)
{
	if (!other)
		return;

	if (other == NPC)
		return;

	if (!other->inuse)
		return;

	//Don't take a target that doesn't want to be
	if (other->flags & FL_NOTARGET)
		return;

	if (NPC->svFlags & SVF_LOCKEDENEMY)
	{
		//IF LOCKED, CANNOT CHANGE ENEMY!!!!!
		return;
	}

	//If we haven't taken a target, just get mad
	if (NPC->enemy == nullptr) //was using "other", fixed to NPC
	{
		G_SetEnemy(NPC, other);
		return;
	}

	//we have an enemy, see if he's dead
	if (NPC->enemy->health <= 0)
	{
		G_ClearEnemy(NPC);
		G_SetEnemy(NPC, other);
		return;
	}

	//Don't take the same enemy again
	if (other == NPC->enemy)
		return;

	if (NPC->client->ps.weapon == WP_SABER)
	{
		//I'm a jedi
		if (mod == MOD_SABER)
		{
			//I was hit by a saber  FIXME: what if this was a thrown saber?
			//always switch to this enemy if I'm a jedi and hit by another saber
			G_ClearEnemy(NPC);
			G_SetEnemy(NPC, other);
			return;
		}
	}
	//Special case player interactions
	if (other == &g_entities[0])
	{
		//Account for the skill level to skew the results
		float luckThreshold;

		switch (g_spskill->integer)
		{
			//Easiest difficulty, mild chance of picking up the player
		case 0:
			luckThreshold = 0.9f;
			break;

			//Medium difficulty, half-half chance of picking up the player
		case 1:
			luckThreshold = 0.5f;
			break;

			//Hardest difficulty, always turn on attacking player
		case 2:
		default:
			luckThreshold = 0.0f;
			break;
		}

		//Randomly pick up the target
		if (Q_flrand(0.0f, 1.0f) > luckThreshold)
		{
			G_ClearEnemy(other);
			other->enemy = NPC;
		}
	}
}

void NPC_SetPainEvent(gentity_t* self)
{
	if (!self->NPC || !(self->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
	{
		if (!Q3_TaskIDPending(self, TID_CHAN_VOICE))
		{
			G_AddEvent(self, EV_PAIN, floor(static_cast<float>(self->health) / self->max_health * 100.0f));
		}
	}
}

/*
-------------------------
NPC_GetPainChance
-------------------------
*/

float NPC_GetPainChance(const gentity_t* self, const int damage)
{
	if (!self->enemy)
	{
		//surprised, always take pain
		return 1.0f;
	}

	if (damage > self->max_health / 2.0f)
	{
		return 1.0f;
	}

	float pain_chance = static_cast<float>(self->max_health - self->health) / (self->max_health * 2.0f) + static_cast<
		float>(damage) / (self->max_health / 2.0f);
	switch (g_spskill->integer)
	{
	case 0: //easy
		//return 0.75f;
		break;

	case 1: //med
		pain_chance *= 0.5f;
		//return 0.35f;
		break;

	case 2: //hard
	default:
		pain_chance *= 0.1f;
		//return 0.05f;
		break;
	}
	//Com_Printf( "%s: %4.2f\n", self->NPC_type, pain_chance );
	return pain_chance;
}

/*
-------------------------
NPC_ChoosePainAnimation
-------------------------
*/

constexpr auto MIN_PAIN_TIME = 200;

extern int G_PickPainAnim(const gentity_t* self, const vec3_t point, int hit_loc);

static void NPC_ChoosePainAnimation(gentity_t* self, const gentity_t* other, const vec3_t point, const int damage,
	const int mod, const int hit_loc,
	const int voice_event = -1)
{
	//If we've already taken pain, then don't take it again
	if (level.time < self->painDebounceTime && mod != MOD_ELECTROCUTE && mod != MOD_MELEE)
	{
		//FIXME: if hit while recovering from losing a saber lock, we should still play a pain anim?
		return;
	}

	int pain_anim = -1;
	float pain_chance;

	if (self->s.weapon == WP_THERMAL && self->client->fireDelay > 0)
	{
		//don't interrupt thermal throwing anim
		return;
	}
	if (self->client->ps.powerups[PW_GALAK_SHIELD])
	{
		return;
	}
	if (self->client->NPC_class == CLASS_GALAKMECH || self->client->NPC_class == CLASS_DROIDEKA)
	{
		if (hit_loc == HL_GENERIC1)
		{
			//hit the antenna!
			pain_chance = 1.0f;
			self->s.powerups |= 1 << PW_SHOCKED;
			self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand(500, 2500);
		}
		else if (self->client->ps.powerups[PW_GALAK_SHIELD])
		{
			//shield up
			return;
		}
		else if (self->health > 200 && damage < 100)
		{
			//have a *lot* of health
			pain_chance = 0.05f;
		}
		else
		{
			//the lower my health and greater the damage, the more likely I am to play a pain anim
			pain_chance = (200.0f - self->health) / 100.0f + damage / 50.0f;
		}
	}
	else if (self->client && self->client->playerTeam == TEAM_PLAYER && other && !other->s.number)
	{
		//ally shot by player always complains
		pain_chance = 1.1f;
	}
	else
	{
		if (other && (other->s.weapon == WP_SABER || mod == MOD_ELECTROCUTE || mod == MOD_CRUSH))
		{
			if (self->client->ps.weapon == WP_SABER
				&& other->s.number < MAX_CLIENTS)
			{
				//hmm, shouldn't *always* react to damage from player if I have a saber
				pain_chance = 1.05f - self->NPC->rank / static_cast<float>(RANK_CAPTAIN);
			}
			else
			{
				pain_chance = 1.0f; //always take pain from saber
			}
		}
		else if (mod == MOD_GAS)
		{
			pain_chance = 1.0f;
		}
		else if (mod == MOD_MELEE)
		{
			//higher in rank (skill) we are, less likely we are to be fazed by a punch
			pain_chance = 1.0f - (RANK_CAPTAIN - self->NPC->rank) / static_cast<float>(RANK_CAPTAIN);
		}
		else if (self->client->NPC_class == CLASS_PROTOCOL)
		{
			pain_chance = 1.0f;
		}
		else
		{
			pain_chance = NPC_GetPainChance(self, damage);
		}
		if (self->client->NPC_class == CLASS_DESANN ||
			self->client->NPC_class == CLASS_SITHLORD ||
			self->client->NPC_class == CLASS_VADER)
		{
			pain_chance *= 0.5f;
		}
	}

	//See if we're going to flinch
	if (Q_flrand(0.0f, 1.0f) < pain_chance)
	{
		//Pick and play our animation
		if (self->client->ps.eFlags & EF_FORCE_GRIPPED)
		{
			G_AddVoiceEvent(self, Q_irand(EV_CHOKE1, EV_CHOKE3), 0);
		}
		else if (self->client->ps.eFlags & EF_FORCE_GRABBED)
		{
			G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 0);
		}
		else if (mod == MOD_GAS)
		{
			//SIGH... because our choke sounds are inappropriately long, I have to debounce them in code!
			if (TIMER_Done(self, "gasChokeSound"))
			{
				TIMER_Set(self, "gasChokeSound", Q_irand(1000, 2000));
				G_AddVoiceEvent(self, Q_irand(EV_CHOKE1, EV_CHOKE3), 0);
			}
		}
		else if (self->client->ps.eFlags & EF_FORCE_DRAINED)
		{
			NPC_SetPainEvent(self);
		}
		else
		{
			//not being force-gripped or force-drained
			if (G_CheckForStrongAttackMomentum(self)
				|| PM_SpinningAnim(self->client->ps.legsAnim)
				|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
				|| PM_InKnockDown(&self->client->ps)
				|| PM_RollingAnim(self->client->ps.legsAnim)
				|| PM_FlippingAnim(self->client->ps.legsAnim) && !PM_InCartwheel(self->client->ps.legsAnim))
			{
				//strong attacks, rolls, knockdowns, flips and spins cannot be interrupted by pain
				return;
			}
			//play an anim
			if (self->client->NPC_class == CLASS_GALAKMECH)
			{
				//only has 1 for now
				//FIXME: never plays this, it seems...
				pain_anim = BOTH_PAIN1;
			}
			else if (mod == MOD_MELEE)
			{
				pain_anim = PM_PickAnim(self, BOTH_PAIN2, BOTH_PAIN3);
			}
			else if (self->s.weapon == WP_SABER)
			{
				//temp HACK: these are the only 2 pain anims that look good when holding a saber
				pain_anim = PM_PickAnim(self, BOTH_PAIN2, BOTH_PAIN3);
			}
			else if (mod != MOD_ELECTROCUTE)
			{
				pain_anim = G_PickPainAnim(self, point, hit_loc);
			}

			if (pain_anim == -1)
			{
				pain_anim = PM_PickAnim(self, BOTH_PAIN1, BOTH_PAIN18);
			}
			if (self->client->ps.saberStylesKnown & 1 << SS_FAST)
			{
				self->client->ps.saberAnimLevel = SS_FAST; //next attack must be a quick attack
			}
			if (self->client->ps.saber[0].type == SABER_STAFF)
			{
				self->client->ps.saberAnimLevel = SS_STAFF;
			}
			self->client->ps.saber_move = LS_READY; //don't finish whatever saber move you may have been in
			int parts = SETANIM_BOTH;
			if (PM_CrouchAnim(self->client->ps.legsAnim) || PM_InCartwheel(self->client->ps.legsAnim))
			{
				parts = SETANIM_LEGS;
			}
			self->NPC->aiFlags &= ~NPCAI_KNEEL;
			NPC_SetAnim(self, parts, pain_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			if (voice_event != -1)
			{
				G_AddVoiceEvent(self, voice_event, Q_irand(2000, 4000));
			}
			else
			{
				NPC_SetPainEvent(self);
			}
		}

		//Setup the timing for it
		if (mod == MOD_ELECTROCUTE)
		{
			self->painDebounceTime = level.time + 4000;
		}
		self->painDebounceTime = level.time + PM_AnimLength(self->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(pain_anim));
		self->client->fireDelay = 0;
	}
}

extern qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy);

gentity_t* G_CheckControlledTurretEnemy(const gentity_t* self, gentity_t* enemy, const qboolean validate)
{
	if (enemy->e_UseFunc == useF_emplaced_gun_use
		|| enemy->e_UseFunc == useF_eweb_use)
	{
		if (enemy->activator
			&& enemy->activator->client)
		{
			//return the controller of the eweb/emplaced gun
			if (validate == qfalse || !self->client || G_ValidEnemy(self, enemy))
			{
				return enemy->activator;
			}
		}
		return nullptr;
	}
	return enemy;
}

extern void Boba_Pain(gentity_t* self, int mod);
/*
===============
NPC_Pain
===============
*/
void NPC_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* other, const vec3_t point, const int damage,
	const int mod,
	const int hit_loc)
{
	team_t otherTeam = TEAM_FREE;
	int voiceEvent = -1;

	if (self->NPC == nullptr)
		return;

	if (other == nullptr)
		return;

	//or just remove ->pain in player_die?
	if (self->client->ps.pm_type == PM_DEAD)
		return;

	if (other == self)
		return;

	other = G_CheckControlledTurretEnemy(self, other, qfalse);
	if (!other)
	{
		return;
	}

	//MCG: Ignore damage from your own team for now
	if (other->client)
	{
		otherTeam = other->client->playerTeam;
	}

	if (self->client->playerTeam
		&& other->client
		&& otherTeam == self->client->playerTeam
		&& (!player->client->ps.viewEntity || other->s.number != player->client->ps.viewEntity))
	{
		//hit by a teammate
		if (other != self->enemy && self != other->enemy)
		{
			//we weren't already enemies
			if (self->enemy || other->enemy
				|| other->s.number && other->s.number != player->client->ps.viewEntity
				/*|| (!other->s.number&&Q_irand( 0, 3 ))*/)
			{
				//if one of us actually has an enemy already, it's okay, just an accident OR wasn't hit by player or someone controlled by player OR player hit ally and didn't get 25% chance of getting mad (FIXME:accumulate anger+base on diff?)
				//FIXME: player should have to do a certain amount of damage to ally or hit them several times to make them mad
				//Still run pain and flee scripts
				if (self->client && self->NPC)
				{
					//Run any pain instructions
					if (self->health <= self->max_health / 3 && G_ActivateBehavior(self, BSET_FLEE))
					{
					}
					else
					{
						G_ActivateBehavior(self, BSET_PAIN);
					}
				}
				if (damage != -1)
				{
					//-1 == don't play pain anim
					//Set our proper pain animation
					if (Q_irand(0, 1))
					{
						NPC_ChoosePainAnimation(self, other, point, damage, mod, hit_loc, EV_FFWARN);
					}
					else
					{
						NPC_ChoosePainAnimation(self, other, point, damage, mod, hit_loc);
					}
				}
				return;
			}
			if (self->NPC && !other->s.number) //should be assumed, but...
			{
				//dammit, stop that!
				if (self->NPC->charmedTime > level.time || self->NPC->darkCharmedTime > level.time)
				{
					//mindtricked
					return;
				}
				if (self->NPC->ffireCount < 3 + (2 - g_spskill->integer) * 2)
				{
					//not mad enough yet
					//Com_Printf( "chck: %d < %d\n", self->NPC->ffireCount, 3+((2-g_spskill->integer)*2) );
					if (damage != -1)
					{
						//-1 == don't play pain anim
						//Set our proper pain animation
						if (Q_irand(0, 1))
						{
							NPC_ChoosePainAnimation(self, other, point, damage, mod, hit_loc, EV_FFWARN);
						}
						else
						{
							NPC_ChoosePainAnimation(self, other, point, damage, mod, hit_loc);
						}
					}
					return;
				}
				if (G_ActivateBehavior(self, BSET_FFIRE))
				{
					//we have a specific script to run, so do that instead
					return;
				}
				//okay, we're going to turn on our ally, we need to set and lock our enemy and put ourselves in a bstate that lets us attack him (and clear any flags that would stop us)
				self->NPC->blockedSpeechDebounceTime = 0;
				voiceEvent = EV_FFTURN;
				self->NPC->behaviorState = self->NPC->tempBehavior = self->NPC->defaultBehavior = BS_DEFAULT;
				other->flags &= ~FL_NOTARGET;
				self->svFlags &= ~(SVF_IGNORE_ENEMIES | SVF_ICARUS_FREEZE | SVF_NO_COMBAT_SOUNDS);
				G_SetEnemy(self, other);
				self->svFlags |= SVF_LOCKEDENEMY;
				self->NPC->scriptFlags &= ~(SCF_DONT_FIRE | SCF_CROUCHED | SCF_WALKING | SCF_NO_COMBAT_TALK |
					SCF_FORCED_MARCH);
				self->NPC->scriptFlags |= SCF_CHASE_ENEMIES | SCF_NO_MIND_TRICK;
				//NOTE: we also stop ICARUS altogether
				stop_icarus = qtrue;
				if (!killPlayerTimer)
				{
					killPlayerTimer = level.time + 10000;
				}
			}
		}
	}

	SaveNPCGlobals();
	SetNPCGlobals(self);

	//Do extra bits
	if (NPCInfo->ignorePain == qfalse)
	{
		NPCInfo->confusionTime = 0; //clear any charm or confusion, regardless
		if (NPCInfo->insanityTime && NPCInfo->insanityTime > level.time)
		{
			NPCInfo->insanityTime = 0;
			NPC->client->ps.torsoAnimTimer = 0;
			NPC->client->ps.weaponTime -= level.time - NPCInfo->insanityTime;
			if (NPC->client->ps.weaponTime < 0)
			{
				NPC->client->ps.weaponTime = 0;
			}
		}
		if (NPC->ghoul2.size() && NPC->headBolt != -1)
		{
			G_StopEffect("force/confusion", NPC->playerModel, NPC->headBolt, NPC->s.number);
			G_StopEffect("force/drain_hand", NPC->playerModel, NPC->headBolt, NPC->s.number);
		}
		if (damage != -1)
		{
			//-1 == don't play pain anim
			//Set our proper pain animation
			NPC_ChoosePainAnimation(self, other, point, damage, mod, hit_loc, voiceEvent);
		}
		//Check to take a new enemy
		if (NPC->enemy != other && NPC != other)
		{
			//not already mad at them
			//if it's an eweb or emplaced gun, get mad at the owner, not the gun
			NPC_CheckAttacker(other, mod);
		}
	}

	//Attempt to run any pain instructions
	if (self->client && self->NPC)
	{
		//FIXME: This needs better heuristics perhaps
		if (self->health <= self->max_health / 3 && G_ActivateBehavior(self, BSET_FLEE))
		{
		}
		else //if( VALIDSTRING( self->behaviorSet[BSET_PAIN] ) )
		{
			G_ActivateBehavior(self, BSET_PAIN);
		}
	}

	//Attempt to fire any paintargets we might have
	if (self->paintarget && self->paintarget[0])
	{
		G_UseTargets2(self, other, self->paintarget);
	}

	if (self->client && (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO))
	{
		Boba_Pain(self, mod);
	}

	RestoreNPCGlobals();
}

/*
-------------------------
NPC_Touch
-------------------------
*/
extern qboolean INV_SecurityKeyGive(const gentity_t* target, const char* keyname);

void NPC_Touch(gentity_t* self, gentity_t* other, trace_t* trace)
{
	if (!self->NPC)
		return;

	SaveNPCGlobals();
	SetNPCGlobals(self);

	if (self->message && self->health <= 0)
	{
		//I am dead and carrying a key
		if (other && player && player->health > 0 && other == player)
		{
			//player touched me
			char* text;
			qboolean keyTaken;
			//give him my key
			if (Q_stricmp("goodie", self->message) == 0)
			{
				//a goodie key
				if ((keyTaken = INV_GoodieKeyGive(other)) == qtrue)
				{
					text = "cp @SP_INGAME_TOOK_IMPERIAL_GOODIE_KEY";
					G_AddEvent(other, EV_ITEM_PICKUP, FindItemForInventory(INV_GOODIE_KEY) - bg_itemlist);
				}
				else
				{
					text = "cp @SP_INGAME_CANT_CARRY_GOODIE_KEY";
				}
			}
			else
			{
				//a named security key
				if ((keyTaken = INV_SecurityKeyGive(player, self->message)) == qtrue)
				{
					text = "cp @SP_INGAME_TOOK_IMPERIAL_SECURITY_KEY";
					G_AddEvent(other, EV_ITEM_PICKUP, FindItemForInventory(INV_SECURITY_KEY) - bg_itemlist);
				}
				else
				{
					text = "cp @SP_INGAME_CANT_CARRY_SECURITY_KEY";
				}
			}
			if (keyTaken)
			{
				//remove my key
				gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "l_arm_key", 0x00000002);
				self->message = nullptr;
				self->client->ps.eFlags &= ~EF_FORCE_VISIBLE; //remove sight flag
				G_Sound(player, G_SoundIndex("sound/weapons/key_pkup.wav"));
				NPC_SetAnim(player, SETANIM_TORSO, BOTH_BUTTON2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			gi.SendServerCommand(0, text);
		}
	}

	if (other->client)
	{
		if (other->health > 0)
		{
			NPCInfo->touchedByPlayer = other;
		}

		if (other == NPCInfo->goalEntity)
		{
			NPCInfo->aiFlags |= NPCAI_TOUCHED_GOAL;
		}

		if (!(self->svFlags & SVF_LOCKEDENEMY) && !(self->svFlags & SVF_IGNORE_ENEMIES) && !(other->flags &
			FL_NOTARGET))
		{
			if (self->client->enemyTeam)
			{
				//See if we bumped into an enemy
				if (other->client->playerTeam == self->client->enemyTeam)
				{
					//bumped into an enemy
					if (NPCInfo->behaviorState != BS_HUNT_AND_KILL && !NPCInfo->tempBehavior)
					{
						//MCG - Begin: checking specific BS mode here, this is bad, a HACK
						//FIXME: not medics?
						if (NPC->enemy != other)
						{
							//not already mad at them
							G_SetEnemy(NPC, other);
						}
					}
				}
			}
		}
	}
	else
	{
		//FIXME: check for SVF_NONNPC_ENEMY flag here?
		if (other->health > 0)
		{
			if (NPC->enemy == other && other->svFlags & SVF_NONNPC_ENEMY)
			{
				NPCInfo->touchedByPlayer = other;
			}
		}

		if (other == NPCInfo->goalEntity)
		{
			NPCInfo->aiFlags |= NPCAI_TOUCHED_GOAL;
		}
	}

	if (NPC->client->NPC_class == CLASS_RANCOR)
	{
		//rancor
		if (NPCInfo->blockedEntity != other && TIMER_Done(NPC, "blockedEntityIgnore"))
		{//bumped into another breakable, so take that one instead?
			NPCInfo->blockedEntity = other; //???
		}
	}

	RestoreNPCGlobals();
}

/*
-------------------------
NPC_TempLookTarget
-------------------------
*/

void NPC_TempLookTarget(const gentity_t* self, const int lookEntNum, int minLookTime, int maxLookTime)
{
	if (!self->client)
	{
		return;
	}

	if (!minLookTime)
	{
		minLookTime = 1000;
	}

	if (!maxLookTime)
	{
		maxLookTime = 1000;
	}

	if (!NPC_CheckLookTarget(self))
	{
		//Not already looking at something else
		//Look at him for 1 to 3 seconds
		NPC_SetLookTarget(self, lookEntNum, level.time + Q_irand(minLookTime, maxLookTime));
	}
}

static void NPC_Respond(gentity_t* self, int userNum)
{
	int event = -1;

	if (!Q_irand(0, 1))
	{
		//set looktarget to them for a second or two
		NPC_TempLookTarget(self, userNum, 1000, 3000);
	}

	//some last-minute hacked in responses
	switch (self->client->NPC_class)
	{
	case CLASS_ALORA:
	case CLASS_TAVION:
	case CLASS_JAN:
		if (self->enemy)
		{
			if (Q_irand(0, 9) > 6)
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else if (Q_irand(0, 6) > 4)
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_DETECTED1, EV_DETECTED5);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = Q_irand(EV_COVER1, EV_COVER5);
			}
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_SUSPICIOUS4;
		}
		else if (!Q_irand(0, 3))
		{
			event = Q_irand(EV_SIGHT1, EV_SIGHT2);
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_LOST1;
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_GIVEUP3;
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_ESCAPING2;
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_ANGER1, EV_ANGER3);
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	case CLASS_REBORN:
	case CLASS_LANDO:
		if (self->enemy)
		{
			if (Q_irand(0, 9) > 6)
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else if (Q_irand(0, 6) > 4)
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_DETECTED1, EV_DETECTED5);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = Q_irand(EV_COVER1, EV_COVER5);
			}
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_SUSPICIOUS4;
		}
		else if (!Q_irand(0, 3))
		{
			event = Q_irand(EV_SIGHT1, EV_SIGHT2);
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_LOST1;
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_GIVEUP3;
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_ESCAPING2;
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_ANGER1, EV_ANGER3);
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	case CLASS_DESANN:
	case CLASS_SITHLORD:
	case CLASS_VADER:
	case CLASS_LUKE:
		if (self->enemy)
		{
			if (Q_irand(0, 9) > 6)
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else if (Q_irand(0, 6) > 4)
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_DETECTED1, EV_DETECTED5);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = Q_irand(EV_COVER1, EV_COVER5);
			}
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_SUSPICIOUS4;
		}
		else if (!Q_irand(0, 3))
		{
			event = Q_irand(EV_SIGHT1, EV_SIGHT2);
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_LOST1;
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_GIVEUP3;
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_ESCAPING2;
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_ANGER1, EV_ANGER3);
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	case CLASS_JEDI:
	case CLASS_KYLE:
	case CLASS_YODA:
		if (!self->enemy)
		{
			if (!(self->svFlags & SVF_IGNORE_ENEMIES)
				&& self->NPC->scriptFlags & SCF_LOOK_FOR_ENEMIES
				&& self->client->enemyTeam == TEAM_ENEMY)
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				if (self->enemy)
				{
					if (Q_irand(0, 9) > 6)
					{
						event = Q_irand(EV_CHASE1, EV_CHASE3);
					}
					else if (Q_irand(0, 6) > 4)
					{
						event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
					}
					else if (Q_irand(0, 1))
					{
						event = Q_irand(EV_DETECTED1, EV_DETECTED5);
					}
					else if (!Q_irand(0, 1))
					{
						event = Q_irand(EV_ANGER1, EV_ANGER3);
					}
					else
					{
						event = Q_irand(EV_COVER1, EV_COVER5);
					}
				}
				else if (!Q_irand(0, 2))
				{
					event = EV_SUSPICIOUS4;
				}
				else if (!Q_irand(0, 3))
				{
					event = Q_irand(EV_SIGHT1, EV_SIGHT2);
				}
				else if (!Q_irand(0, 1))
				{
					event = Q_irand(EV_SOUND1, EV_SOUND3);
				}
				else if (!Q_irand(0, 2))
				{
					event = EV_LOST1;
				}
				else if (!Q_irand(0, 1))
				{
					event = EV_GIVEUP3;
				}
				else if (Q_irand(0, 1))
				{
					event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
				}
				else if (Q_irand(0, 1))
				{
					event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
				}
				else if (!Q_irand(0, 1))
				{
					event = EV_ESCAPING2;
				}
				else if (!Q_irand(0, 1))
				{
					event = Q_irand(EV_ANGER1, EV_ANGER3);
				}
				else
				{
					event = Q_irand(EV_TAUNT1, EV_TAUNT3);
				}
			}
		}
		break;
	case CLASS_PRISONER:
		if (self->enemy)
		{
			if (Q_irand(0, 1))
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
		}
		else
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		break;
	case CLASS_BATTLEDROID:
	case CLASS_STORMTROOPER:
	case CLASS_IMPERIAL:
	case CLASS_REBEL:
	case CLASS_WOOKIE:
		if (self->enemy)
		{
			if (Q_irand(0, 9) > 6)
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else if (Q_irand(0, 6) > 4)
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_DETECTED1, EV_DETECTED5);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = Q_irand(EV_COVER1, EV_COVER5);
			}
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_SUSPICIOUS4;
		}
		else if (!Q_irand(0, 3))
		{
			event = Q_irand(EV_SIGHT1, EV_SIGHT2);
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_LOST1;
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_GIVEUP3;
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_ESCAPING2;
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_ANGER1, EV_ANGER3);
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	case CLASS_BESPIN_COP:
		if (!Q_stricmp("bespincop", self->NPC_type))
		{
			//variant 1
			if (self->enemy)
			{
				if (Q_irand(0, 9) > 6)
				{
					event = Q_irand(EV_CHASE1, EV_CHASE3);
				}
				else if (Q_irand(0, 6) > 4)
				{
					event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
				}
				else
				{
					event = Q_irand(EV_COVER1, EV_COVER5);
				}
			}
			else if (!Q_irand(0, 3))
			{
				event = Q_irand(EV_SIGHT2, EV_SIGHT3);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_SOUND1, EV_SOUND3);
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_LOST1;
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_ESCAPING2;
			}
			else
			{
				event = EV_GIVEUP4;
			}
		}
		else
		{
			//variant2
			if (self->enemy)
			{
				if (Q_irand(0, 9) > 6)
				{
					event = Q_irand(EV_CHASE1, EV_CHASE3);
				}
				else if (Q_irand(0, 6) > 4)
				{
					event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
				}
				else if (Q_irand(0, 1))
				{
					event = Q_irand(EV_DETECTED1, EV_DETECTED5);
				}
				else if (!Q_irand(0, 1))
				{
					event = Q_irand(EV_ANGER1, EV_ANGER3);
				}
				else
				{
					event = Q_irand(EV_COVER1, EV_COVER5);
				}
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_SUSPICIOUS4;
			}
			else if (!Q_irand(0, 3))
			{
				event = Q_irand(EV_SIGHT1, EV_SIGHT2);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_SOUND1, EV_SOUND3);
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_LOST1;
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_GIVEUP3;
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_ESCAPING2;
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = EV_CONFUSE1;
			}
		}
		break;
	case CLASS_SBD:
		if (!Q_stricmp("SBD", self->NPC_type))
		{
			//variant 1
			if (self->enemy)
			{
				if (Q_irand(0, 9) > 6)
				{
					event = Q_irand(EV_CHASE1, EV_CHASE3);
				}
				else if (Q_irand(0, 6) > 4)
				{
					event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
				}
				else if (Q_irand(0, 1))
				{
					event = Q_irand(EV_DETECTED1, EV_DETECTED5);
				}
				else if (!Q_irand(0, 1))
				{
					event = Q_irand(EV_ANGER1, EV_ANGER3);
				}
				else
				{
					event = Q_irand(EV_COVER1, EV_COVER5);
				}
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_SUSPICIOUS4;
			}
			else if (!Q_irand(0, 3))
			{
				event = Q_irand(EV_SIGHT1, EV_SIGHT2);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_SOUND1, EV_SOUND3);
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_LOST1;
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_GIVEUP3;
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_ESCAPING2;
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = EV_CONFUSE1;
			}
		}
		else
		{
			//variant2
			if (self->enemy)
			{
				if (Q_irand(0, 9) > 6)
				{
					event = Q_irand(EV_CHASE1, EV_CHASE3);
				}
				else if (Q_irand(0, 6) > 4)
				{
					event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
				}
				else if (Q_irand(0, 1))
				{
					event = Q_irand(EV_DETECTED1, EV_DETECTED5);
				}
				else if (!Q_irand(0, 1))
				{
					event = Q_irand(EV_ANGER1, EV_ANGER3);
				}
				else
				{
					event = Q_irand(EV_COVER1, EV_COVER5);
				}
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_SUSPICIOUS4;
			}
			else if (!Q_irand(0, 3))
			{
				event = Q_irand(EV_SIGHT1, EV_SIGHT2);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_SOUND1, EV_SOUND3);
			}
			else if (!Q_irand(0, 2))
			{
				event = EV_LOST1;
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_GIVEUP3;
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (!Q_irand(0, 1))
			{
				event = EV_ESCAPING2;
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = EV_CONFUSE1;
			}
		}
		break;
	case CLASS_R2D2: // droid
		G_Sound(self, G_SoundIndex(va("sound/chars/r2d2/misc/r2d2talk0%d.wav", Q_irand(1, 3))));
		break;
	case CLASS_R5D2: // droid
		G_Sound(self, G_SoundIndex(va("sound/chars/r5d2/misc/r5talk%d.wav", Q_irand(1, 4))));
		break;
	case CLASS_MOUSE: // droid
		G_Sound(self, G_SoundIndex(va("sound/chars/mouse/misc/mousego%d.wav", Q_irand(1, 3))));
		break;
	case CLASS_GONK: // droid
		G_Sound(self, G_SoundIndex(va("sound/chars/gonk/misc/gonktalk%d.wav", Q_irand(1, 2))));
		break;
	case CLASS_JAWA:
		G_SoundOnEnt(self, CHAN_VOICE, va("sound/chars/jawa/misc/chatter%d.wav", Q_irand(1, 6)));
		if (self->NPC)
		{
			self->NPC->blockedSpeechDebounceTime = level.time + 2000;
		}
		break;
	default:
		if (self->enemy)
		{
			if (Q_irand(0, 9) > 6)
			{
				event = Q_irand(EV_CHASE1, EV_CHASE3);
			}
			else if (Q_irand(0, 6) > 4)
			{
				event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
			}
			else if (Q_irand(0, 1))
			{
				event = Q_irand(EV_DETECTED1, EV_DETECTED5);
			}
			else if (!Q_irand(0, 1))
			{
				event = Q_irand(EV_ANGER1, EV_ANGER3);
			}
			else
			{
				event = Q_irand(EV_COVER1, EV_COVER5);
			}
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_SUSPICIOUS4;
		}
		else if (!Q_irand(0, 3))
		{
			event = Q_irand(EV_SIGHT1, EV_SIGHT2);
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_SOUND1, EV_SOUND3);
		}
		else if (!Q_irand(0, 2))
		{
			event = EV_LOST1;
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_GIVEUP3;
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_JDETECTED1, EV_JDETECTED2);
		}
		else if (Q_irand(0, 1))
		{
			event = Q_irand(EV_OUTFLANK1, EV_OUTFLANK2);
		}
		else if (!Q_irand(0, 1))
		{
			event = EV_ESCAPING2;
		}
		else if (!Q_irand(0, 1))
		{
			event = Q_irand(EV_ANGER1, EV_ANGER3);
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	}

	if (event != -1)
	{
		//hack here because we reuse some "combat" and "extra" sounds
		auto addFlag = static_cast<qboolean>((self->NPC->scriptFlags & SCF_NO_COMBAT_TALK) != 0);
		self->NPC->scriptFlags &= ~SCF_NO_COMBAT_TALK;

		G_AddVoiceEvent(self, event, 3000);

		if (addFlag)
		{
			self->NPC->scriptFlags |= SCF_NO_COMBAT_TALK;
		}
	}
}

/*
-------------------------
NPC_UseResponse
-------------------------
*/

void NPC_UseResponse(gentity_t* self, const gentity_t* user, const qboolean useWhenDone)
{
	if (!self->NPC || !self->client)
	{
		return;
	}

	if (user->s.number != 0)
	{
		//not used by the player
		if (useWhenDone)
		{
			G_ActivateBehavior(self, BSET_USE);
		}
		return;
	}

	if (user->client && self->client->playerTeam != user->client->playerTeam && self->client->playerTeam !=
		TEAM_NEUTRAL)
	{
		//only those on the same team react
		if (useWhenDone)
		{
			G_ActivateBehavior(self, BSET_USE);
		}
		return;
	}

	if (self->NPC->blockedSpeechDebounceTime > level.time)
	{
		//I'm not responding right now
		return;
	}

	if (gi.VoiceVolume[self->s.number])
	{
		//I'm talking already
		if (!useWhenDone)
		{
			//you're not trying to use me
			return;
		}
	}

	if (useWhenDone)
	{
		G_ActivateBehavior(self, BSET_USE);
	}
	else
	{
		NPC_Respond(self, user->s.number);
	}
}

/*
-------------------------
NPC_Use
-------------------------
*/
extern void Add_Batteries(gentity_t* ent, int* count);

void NPC_Use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	if (self->client->ps.pm_type == PM_DEAD)
	{
		//or just remove ->pain in player_die?
		return;
	}

	SaveNPCGlobals();
	SetNPCGlobals(self);

	if (self->client && self->NPC)
	{
		// If this is a vehicle, let the other guy board it. Added 12/14/02 by AReis.
		if (self->client->NPC_class == CLASS_VEHICLE)
		{
			Vehicle_t* p_veh = self->m_pVehicle;

			if (p_veh && p_veh->m_pVehicleInfo && other && other->client)
			{
				//safety
				//if I used myself, eject everyone on me
				if (other == self)
				{
					p_veh->m_pVehicleInfo->EjectAll(p_veh);
				}
				// If other is already riding this vehicle (self), eject him.
				else if (other->owner == self)
				{
					p_veh->m_pVehicleInfo->Eject(p_veh, other, qfalse);
				}
				// Otherwise board this vehicle.
				else
				{
					p_veh->m_pVehicleInfo->Board(p_veh, other);
				}
			}
		}
		else if (Jedi_WaitingAmbush(NPC))
		{
			Jedi_Ambush(NPC);
		}
		//Run any use instructions
		if (activator && activator->s.number == 0 && self->client->NPC_class == CLASS_GONK)
		{
			Add_Batteries(activator, &self->client->ps.batteryCharge);
		}

		if (self->behaviorSet[BSET_USE])
		{
			NPC_UseResponse(self, other, qtrue);
		}
		else if (!self->enemy && 
			!gi.VoiceVolume[self->s.number] && 
			!(self->NPC->scriptFlags & SCF_NO_RESPONSE)	&&
			activator->s.number == 0)
		{
			//I don't have an enemy and I'm not talking and I was used by the player
			NPC_UseResponse(self, other, qfalse);
		}

		if (self->NPC->behaviorState == BS_FOLLOW_LEADER)
		{
			// Store the backup info we need.
			self->NPC_backupinfo.behaviorState = self->NPC->behaviorState;
			self->NPC_backupinfo.tempBehavior = self->NPC->tempBehavior;
			self->NPC_backupinfo.enemy = self->enemy;
			self->NPC_backupinfo.leader = self->client->leader;

			// Set our override behavior.
			self->NPC->tempBehavior = BS_HUNT_AND_KILL;
			self->NPC->behaviorState = BS_HUNT_AND_KILL;
			self->enemy = self->NPC_backupinfo.enemy;
			self->client->leader = &g_entities[0];
			NPC_UseResponse(self, other, qtrue);
		}
		else
		{
			// Restore normal npc behavior
			self->NPC->behaviorState = BS_FOLLOW_LEADER;
			self->NPC->tempBehavior = BS_FOLLOW_LEADER;
			self->enemy = self->NPC_backupinfo.enemy;
			self->client->leader = self->NPC_backupinfo.leader;
			NPC_Respond(self, NPC->s.number);
			NPC_UseResponse(self, other, qtrue);
		}
	}

	RestoreNPCGlobals();
}