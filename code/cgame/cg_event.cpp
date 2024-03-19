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

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

extern qboolean CG_TryPlayCustomSound(vec3_t origin, int entityNum, soundChannel_t channel, const char* sound_name,
	int custom_sound_set);
extern void fx_kothos_beam(vec3_t start, vec3_t end);
extern void CG_GibPlayerHeadshot(vec3_t player_origin);
extern void CG_GibPlayer(vec3_t player_origin);
extern float ShortestLineSegBewteen2LineSegs(vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1,
	vec3_t close_pnt2);
extern void CG_StrikeBolt(const centity_t* cent, vec3_t origin);

//==========================================================================

static qboolean CG_IsFemale(const char* infostring)
{
	const char* sex = Info_ValueForKey(infostring, "s");
	if (sex[0] == 'f' || sex[0] == 'F')
		return qtrue;
	return qfalse;
}

const char* CG_PlaceString(int rank)
{
	static char str[64];
	char* s, * t;

	if (rank & RANK_TIED_FLAG)
	{
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	}
	else
	{
		t = "";
	}

	if (rank == 1)
	{
		s = "\03341st\0337"; // draw in blue
	}
	else if (rank == 2)
	{
		s = "\03312nd\0337"; // draw in red
	}
	else if (rank == 3)
	{
		s = "\03333rd\0337"; // draw in yellow
	}
	else if (rank == 11)
	{
		s = "11th";
	}
	else if (rank == 12)
	{
		s = "12th";
	}
	else if (rank == 13)
	{
		s = "13th";
	}
	else if (rank % 10 == 1)
	{
		s = va("%ist", rank);
	}
	else if (rank % 10 == 2)
	{
		s = va("%ind", rank);
	}
	else if (rank % 10 == 3)
	{
		s = va("%ird", rank);
	}
	else
	{
		s = va("%ith", rank);
	}

	Com_sprintf(str, sizeof str, "%s%s", t, s);
	return str;
}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
void CG_ItemPickup(const int itemNum, const qboolean bHadItem)
{
	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;

	// see if it should be the grabbed weapon
	if (bg_itemlist[itemNum].giType == IT_WEAPON)
	{
		const int n_cur_wpn = cg.predictedPlayerState.weapon;
		const int n_new_wpn = bg_itemlist[itemNum].giTag;

		if (n_cur_wpn == WP_SABER || bHadItem)
		{
			//never switch away from the saber!
			return;
		}

		// kef -- check cg_autoswitch...
		//
		// 0 == no switching
		// 1 == automatically switch to best SAFE weapon
		// 2 == automatically switch to best weapon, safe or otherwise
		//
		// NOTE: automatically switching to any weapon you pick up is stupid and annoying and we won't do it.
		//

		if (n_new_wpn == WP_SABER)
		{
			//always switch to saber
			SetWeaponSelectTime();
			cg.weaponSelect = n_new_wpn;
		}
		else if (0 == cg_autoswitch.integer)
		{
			// don't switch
		}
		else if (1 == cg_autoswitch.integer)
		{
			// safe switching
			if (n_new_wpn > n_cur_wpn &&
				n_new_wpn != WP_DET_PACK &&
				n_new_wpn != WP_TRIP_MINE &&
				n_new_wpn != WP_THERMAL &&
				n_new_wpn != WP_ROCKET_LAUNCHER &&
				n_new_wpn != WP_CONCUSSION)
			{
				// switch to new wpn
				SetWeaponSelectTime();
				cg.weaponSelect = n_new_wpn;
			}
		}
		else if (2 == cg_autoswitch.integer)
		{
			// best
			if (n_new_wpn > n_cur_wpn)
			{
				// switch to new wpn
				//				cg.weaponSelectTime = cg.time;
				SetWeaponSelectTime();
				cg.weaponSelect = n_new_wpn;
			}
		}
	}
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent(centity_t* cent, const int health)
{
	char* snd;

	// don't do more than two pain sounds a second
	if (cg.time - cent->pe.painTime < 500)
	{
		return;
	}

	if (health < 25)
	{
		snd = "*pain25.wav";
	}
	else if (health < 50)
	{
		snd = "*pain50.wav";
	}
	else if (health < 75)
	{
		snd = "*pain75.wav";
	}
	else
	{
		snd = "*pain100.wav";
	}
	CG_TryPlayCustomSound(nullptr, cent->currentState.number, CHAN_VOICE, snd, CS_BASIC);

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

/*
===============
UseItem
===============
*/
extern void CG_ToggleBinoculars();
extern void CG_ToggleLAGoggles();

void UseItem(const int itemNum)
{
	const centity_t* cent = &cg_entities[cg.snap->ps.clientNum];

	switch (itemNum)
	{
	case INV_ELECTROBINOCULARS:
		CG_ToggleBinoculars();
		break;
	case INV_LIGHTAMP_GOGGLES:
		CG_ToggleLAGoggles();
		break;
	case INV_GOODIE_KEY:
		if (cent->gent->client->ps.inventory[INV_GOODIE_KEY])
		{
			cent->gent->client->ps.inventory[INV_GOODIE_KEY]--;
		}
		break;
	case INV_SECURITY_KEY:
		if (cent->gent->client->ps.inventory[INV_SECURITY_KEY])
		{
			cent->gent->client->ps.inventory[INV_SECURITY_KEY]--;
		}
		break;
	default:;
	}
}

/*
===============
CG_UseForce
===============
*/
static void CG_UseForce()
{
	//FIXME: sound or graphic change or something?
	//actual force power action is on game/pm side
}

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem(const centity_t* cent)
{
	const entityState_t* es = &cent->currentState;

	int itemNum = cg.inventorySelect;
	if (itemNum < 0 || itemNum > INV_MAX)
	{
		itemNum = 0;
	}

	UseItem(itemNum);
}

/*
==============
CG_UnsafeEventType

Returns qtrue for event types that access cent->gent directly (and don't require it
to be the player / entity 0).
==============
*/
static qboolean CG_UnsafeEventType(const int event_type)
{
	switch (event_type)
	{
	case EV_CHANGE_WEAPON:
	case EV_DISRUPTOR_SNIPER_SHOT:
	case EV_DISRUPTOR_SNIPER_MISS:
	case EV_CONC_ALT_MISS:
	case EV_DISINTEGRATION:
	case EV_GRENADE_BOUNCE:
	case EV_MISSILE_HIT:
	case EV_MISSILE_MISS:
	case EV_PAIN:
	case EV_PLAY_EFFECT:
	case EV_TARGET_BEAM_DRAW:
		return qtrue;
	default:
		return qfalse;
	}
}

/*
==============
CG_EntityEvent

An entity has an event value
==============
*/
#define	DEBUGNAME(x) if(cg_debugEvents.integer){CG_Printf(x"\n");}

void CG_EntityEvent(centity_t* cent, vec3_t position)
{
	const char* s;
	vec3_t dir;

	entityState_t* es = &cent->currentState;
	const int event = es->event & ~EV_EVENT_BITS;

	if (cg_debugEvents.integer)
	{
		CG_Printf("ent:%3i  event:%3i ", es->number, event);
	}

	if (!event)
	{
		DEBUGNAME("ZEROEVENT");
		return;
	}

	if (!cent->gent)
	{
		return;
	}

	//When skipping a cutscene the timescale is drastically increased, causing entities to be freed
	//and possibly reused between the snapshot currentState and the actual state of the gent when accessed.
	//We try to avoid this issue by ignoring events on entities that have been freed since the snapshot.
	if (cent->gent->freetime > cg.snap->serverTime && CG_UnsafeEventType(event))
	{
		return;
	}

	const int clientNum = cent->gent->s.number;

	switch (event)
	{
		//
		// movement generated events
		//
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if (cg_footsteps.integer)
		{
			if (cent->gent->s.number == 0 && !cg.renderingThirdPerson) //!cg_thirdPerson.integer )
			{
				//Everyone else has keyframed footsteps in animsounds.cfg
				cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_NORMAL][rand() & 3]);
			}
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if (cg_footsteps.integer)
		{
			if (cent->gent && cent->gent->s.number == 0 && !cg.renderingThirdPerson) //!cg_thirdPerson.integer )
			{
				//Everyone else has keyframed footsteps in animsounds.cfg
				cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_METAL][rand() & 3]);
			}
		}
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if (cg_footsteps.integer)
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if (cg_footsteps.integer)
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_WADE][rand() & 3]);
		}
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if (cg_footsteps.integer)
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SWIM][rand() & 3]);
		}
		break;

	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.landSound);
		if (clientNum == cg.predictedPlayerState.clientNum)
		{
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound -
		if (g_entities[es->number].health <= 0)
		{
			//dead
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.landSound);
		}
		else if (g_entities[es->number].s.weapon == WP_SABER
			|| g_entities[es->number].client && g_entities[es->number].client->ps.forcePowersKnown & 1 << FP_LEVITATION)
		{
			//jedi or someone who has force jump (so probably took no damage)
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_BODY, "*land1.wav", CS_BASIC);
		}
		else
		{
			//still alive
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_BODY, "*pain100.wav", CS_BASIC);
		}
		if (clientNum == cg.predictedPlayerState.clientNum)
		{
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_BODY, "*land1.wav", CS_BASIC);
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.landSound);
		cent->pe.painTime = cg.time; // don't play a pain sound right after this
		if (clientNum == cg.predictedPlayerState.clientNum)
		{
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		//FIXME: maybe kick up some dust?
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16: // smooth out step up transitions
		DEBUGNAME("EV_STEP");
		{
			float old_step;

			if (clientNum != cg.predictedPlayerState.clientNum)
			{
				break;
			}
			// if we are interpolating, we don't need to smooth steps
			if (cg_timescale.value >= 1.0f)
			{
				break;
			}
			// check for stepping up before a previous step is completed
			const int delta = cg.time - cg.stepTime;
			if (delta < STEP_TIME)
			{
				old_step = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
			}
			else
			{
				old_step = 0;
			}

			// add this amount
			const int step = 4 * (event - EV_STEP_4 + 1);
			cg.stepChange = old_step + step;
			if (cg.stepChange > MAX_STEP_CHANGE)
			{
				cg.stepChange = MAX_STEP_CHANGE;
			}
			cg.stepTime = cg.time;
			break;
		}

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		if (!cg_jumpSounds.integer)
		{
			break;
		}
		if (cg.time - cent->pe.painTime < 500) //don't play immediately after pain/fall sound?
		{
			break;
		}
		if (cg_jumpSounds.integer == 1)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE //play all jump sounds
		}
		else if (cg_jumpSounds.integer == 2 && cg.snap->ps.clientNum != es->number)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE //only play other players' jump sounds
		}
		else if (cg_jumpSounds.integer > 2 && cg.snap->ps.clientNum == es->number)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE//only play my jump sounds
		}
		break;

	case EV_ROLL:
		DEBUGNAME("EV_ROLL");
		if (!cg_rollSounds.integer)
		{
			break;
		}
		if (cg_rollSounds.integer == 1)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE //play all jump sounds
		}
		else if (cg_rollSounds.integer == 2 && cg.snap->ps.clientNum != es->number)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE //only play other players' jump sounds
		}
		else if (cg_rollSounds.integer > 2 && cg.snap->ps.clientNum == es->number)
		{
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*jump1.wav", CS_BASIC);
			//CHAN_VOICE//only play my jump sounds
		}
		cgi_S_StartSound(nullptr, es->number, CHAN_BODY, cgs.media.rollSound); //CHAN_AUTO
		break;

	case EV_LAVA_TOUCH:
		DEBUGNAME("EV_LAVA_TOUCH");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.lavaInSound);
		break;

	case EV_LAVA_LEAVE:
		DEBUGNAME("EV_LAVA_LEAVE");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.lavaOutSound);
		break;

	case EV_LAVA_UNDER:
		DEBUGNAME("EV_LAVA_UNDER");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.lavaUnSound);
		break;

	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		if (cent->gent->client->NPC_class == CLASS_ATST)
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.atstWaterInSound[Q_irand(0, 1)]);
		}
		else
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.watrInSound);
		}
		break;

	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		if (cent->gent->client->NPC_class == CLASS_ATST)
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.atstWaterOutSound[Q_irand(0, 1)]);
		}
		else
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		}
		break;

	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.watrUnSound);
		break;

	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_AUTO, "*gasp.wav", CS_BASIC);
		break;

	case EV_WATER_GURP1:
	case EV_WATER_GURP2:
		DEBUGNAME("EV_WATER_GURPx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_AUTO, va("*gurp%d.wav", event - EV_WATER_GURP1 + 1), CS_BASIC);
		break;

	case EV_WATER_DROWN:
		DEBUGNAME("EV_WATER_DROWN");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_AUTO, "*drown.wav", CS_BASIC);
		break;

	case EV_CALLOUT_BEHIND:
		DEBUGNAME("EV_CALLOUT_BEHIND");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*confuse%i.wav", event - EV_CONFUSE1 + 1),
			CS_CALLOUT);
		break;

	case EV_CALLOUT_NEAR:
		DEBUGNAME("EV_CALLOUT_NEAR");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*sight%i.wav", event - EV_SIGHT1 + 1), CS_CALLOUT);
		break;

	case EV_CALLOUT_FAR:
		DEBUGNAME("EV_CALLOUT_FAR");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*escaping%i.wav", event - EV_ESCAPING1 + 1),
			CS_CALLOUT);
		break;

	case EV_CALLOUT_VC_REQ_ASSIST:
		DEBUGNAME("EV_CALLOUT_VC_REQ_ASSIST");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*req_assist.mp3", CS_CALLOUT);
		break;

	case EV_CALLOUT_VC_REQ_MEDIC:
		DEBUGNAME("EV_CALLOUT_VC_REQ_MEDIC");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*req_medic.mp3", CS_CALLOUT);
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			qboolean bHadItem = qfalse;

			int index = es->eventParm; // player predicted

			if (static_cast<char>(index) < 0)
			{
				index = -static_cast<char>(index);
				bHadItem = qtrue;
			}

			if (index >= bg_numItems)
			{
				break;
			}
			const gitem_t* item = &bg_itemlist[index];
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgi_S_RegisterSound(item->pickup_sound));

			// show icon and name on status bar
			if (es->number == cg.snap->ps.clientNum)
			{
				CG_ItemPickup(index, bHadItem);
			}
		}
		break;

		//
		// weapon events
		//
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
		//cgi_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if (es->number == cg.snap->ps.clientNum)
		{
			CG_OutOfAmmoChange();
		}
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		if (es->weapon == WP_SABER)
		{
			if (cent->gent && cent->gent->client)
			{
				//if ( cent->gent->client->ps.saberInFlight )
				{
					//if it's not in flight or lying around, turn it off!
					cent->currentState.saberActive = qfalse;
				}
			}
		}

		// FIXME: if it happens that you don't want the saber to play the switch sounds, feel free to modify this bit.
		if (weaponData[cg.weaponSelect].selectSnd[0])
		{
			// custom select sound
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO,
				cgi_S_RegisterSound(weaponData[cg.weaponSelect].selectSnd));
		}
		else
		{
			// generic sound
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.media.selectSound);
		}
		break;

	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		CG_FireWeapon(cent, qfalse);
		break;

	case EV_ALTFIRE:
		DEBUGNAME("EV_ALTFIRE");
		CG_FireWeapon(cent, qtrue);
		break;

	case EV_DISRUPTOR_MAIN_SHOT:
		DEBUGNAME("EV_DISRUPTOR_MAIN_SHOT");
		FX_DisruptorMainShot(cent->currentState.origin2, cent->lerpOrigin);
		break;

	case EV_DISRUPTOR_SNIPER_SHOT:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_SHOT");
		FX_DisruptorAltShot(cent->currentState.origin2, cent->lerpOrigin, cent->gent->alt_fire);
		break;

	case EV_DISRUPTOR_SNIPER_MISS:
		DEBUGNAME("EV_DISRUPTOR_SNIPER_MISS");
		FX_DisruptorAltMiss(cent->lerpOrigin, cent->gent->pos1);
		break;

	case EV_DEMP2_ALT_IMPACT:
		FX_DEMP2_AltDetonate(cent->lerpOrigin, es->eventParm);
		break;

	case EV_CONC_ALT_SHOT:
		DEBUGNAME("EV_CONC_ALT_SHOT");
		FX_ConcAltShot(cent->currentState.origin2, cent->lerpOrigin);
		break;

	case EV_CONC_ALT_MISS:
		DEBUGNAME("EV_CONC_ALT_MISS");
		FX_ConcAltMiss(cent->lerpOrigin, cent->gent->pos1);
		break;

	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if (es->number == cg.snap->ps.clientNum)
		{
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		break;

	case EV_KOTHOS_BEAM:
		DEBUGNAME("EV_KOTHOS_BEAM");
		if (Q_irand(0, 1))
		{
			fx_kothos_beam(cg_entities[cent->currentState.otherentity_num].gent->client->renderInfo.handRPoint,
				cg_entities[cent->currentState.otherentity_num2].lerpOrigin);
		}
		else
		{
			fx_kothos_beam(cg_entities[cent->currentState.otherentity_num].gent->client->renderInfo.handLPoint,
				cg_entities[cent->currentState.otherentity_num2].lerpOrigin);
		}
		break;
		//=================================================================

		//
		// other events
		//
	case EV_REPLICATOR:
		DEBUGNAME("EV_REPLICATOR");
		//		FX_Replicator( cent, position );
		break;

	case EV_LIGHTNING_STRIKE:
		DEBUGNAME("EV_LIGHTNING_STRIKE");
		FX_LightningStrike(cg_entities[cent->currentState.otherentity_num].gent->client->renderInfo.handLPoint,
			cg_entities[cent->currentState.otherentity_num2].lerpOrigin);
		break;

	case EV_LIGHTNING_BOLT:
		DEBUGNAME("EV_LIGHTNING_BOLT");
		CG_StrikeBolt(cent, cg_entities[cent->currentState.otherentity_num].gent->client->renderInfo.handLPoint);
		break;

	case EV_BATTERIES_CHARGED:
		cg.batteryChargeTime = cg.time + 3000;
		cgi_S_StartSound(vec3_origin, es->number, CHAN_AUTO, cgs.media.batteryChargeSound);
		break;

	case EV_DISINTEGRATION:
	{
		DEBUGNAME("EV_DISINTEGRATION");
		qboolean make_not_solid;
		const int disint_pw = es->eventParm;
		int disint_effect;
		int disint_length;
		qhandle_t disint_sound1;
		qhandle_t disint_sound2;

		switch (disint_pw)
		{
		case PW_DISRUPTION: // sniper rifle
			disint_effect = EF_DISINTEGRATION; //ef_
			disint_sound1 = cgs.media.disintegrateSound; //with scream
			disint_sound2 = cgs.media.disintegrate2Sound; //no scream
			disint_length = 2000;
			make_not_solid = qtrue;
			break;
		default:
			return;
		}

		if (cent->gent->owner)
		{
			cent->gent->owner->fx_time = cg.time;
			if (cent->gent->owner->client)
			{
				if (disint_sound1 && disint_sound2)
				{
					//play an extra sound
					// listed all the non-humanoids, because there's a lot more humanoids
					const class_t npc_class = cent->gent->owner->client->NPC_class;

					if (npc_class != CLASS_ATST && npc_class != CLASS_GONK &&
						npc_class != CLASS_INTERROGATOR && npc_class != CLASS_MARK1 &&
						npc_class != CLASS_MARK2 && npc_class != CLASS_MOUSE &&
						npc_class != CLASS_PROBE && npc_class != CLASS_PROTOCOL &&
						npc_class != CLASS_R2D2 && npc_class != CLASS_R5D2 &&
						npc_class != CLASS_SEEKER && npc_class != CLASS_SENTRY &&
						npc_class != CLASS_SBD && npc_class != CLASS_BATTLEDROID &&
						npc_class != CLASS_DROIDEKA && npc_class != CLASS_OBJECT &&
						npc_class != CLASS_ASSASSIN_DROID && npc_class != CLASS_SABER_DROID)
					{
						//Only the humanoids scream
						cgi_S_StartSound(nullptr, cent->gent->owner->s.number, CHAN_VOICE, disint_sound1);
					}
					else
					{
						cgi_S_StartSound(nullptr, cent->gent->s.number, CHAN_AUTO, disint_sound2);
					}
				}
				cent->gent->owner->s.powerups |= 1 << disint_pw;
				cent->gent->owner->client->ps.powerups[disint_pw] = cg.time + disint_length;

				// Things that are being disintegrated should probably not be solid...
				if (make_not_solid && cent->gent->owner->client->playerTeam != TEAM_NEUTRAL)
				{
					cent->gent->contents = CONTENTS_NONE;
				}
			}
			else
			{
				cent->gent->owner->s.eFlags = disint_effect;
				cent->gent->owner->delay = cg.time + disint_length;
			}
		}
	}
	break;

	// This does not necessarily have to be from a grenade...
	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		CG_BounceEffect(es->weapon, position, cent->gent->pos1);
		break;

		//
		// missile impacts
		//

	case EV_MISSILE_STICK:
		DEBUGNAME("EV_MISSILE_STICK");
		CG_MissileStick(cent, es->weapon);
		break;

	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		if (CG_VehicleWeaponImpact(cent))
		{
		}
		else
		{
			cg_missile_hit_player(cent, es->weapon, position, cent->gent->pos1, cent->gent->alt_fire);
		}
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		if (CG_VehicleWeaponImpact(cent))
		{
		}
		else
		{
			cg_missile_hit_wall(cent, es->weapon, position, cent->gent->pos1, cent->gent->alt_fire);
		}
		break;

	case EV_BMODEL_SOUND:
		DEBUGNAME("EV_BMODEL_SOUND");
		cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, es->eventParm);
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if (cgs.sound_precache[es->eventParm])
		{
			cgi_S_StartSound(nullptr, es->number, CHAN_AUTO, cgs.sound_precache[es->eventParm]);
		}
		else
		{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			CG_TryPlayCustomSound(nullptr, es->number, CHAN_AUTO, s, CS_BASIC);
		}
		break;

	case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if (cgs.sound_precache[es->eventParm])
		{
			cgi_S_StartSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, cgs.sound_precache[es->eventParm]);
		}
		else
		{
			s = CG_ConfigString(CS_SOUNDS + es->eventParm);
			CG_TryPlayCustomSound(nullptr, cg.snap->ps.clientNum, CHAN_AUTO, s, CS_BASIC);
		}
		break;

	case EV_DRUGGED:
		DEBUGNAME("EV_DRUGGED");
		if (cent->gent && cent->gent->owner && cent->gent->owner->s.number == 0)
		{
			// Only allow setting up the wonky vision on the player..do it for 10 seconds...must be synchronized with calcs done in cg_view.  Just search for cg.wonkyTime to find 'em.
			cg.wonkyTime = cg.time + 10000;
		}
		break;

	case EV_STUNNED:
		DEBUGNAME("EV_STUNNED");
		if (cent->gent && cent->gent->owner && cent->gent->owner->s.number == 0)
		{
			// Only allow setting up the wonky vision on the player..do it for 5 seconds...must be synchronized with calcs done in cg_view.  Just search for cg.wonkyTime to find 'em.
			cg.stunnedTime = cg.time + 5000;
		}
		break;

	case EV_SHIELD_HIT:
		DEBUGNAME("EV_SHIELD_HIT");
		ByteToDir(es->eventParm, dir);
		CG_PlayerShieldHit(es->otherentity_num, dir, es->time2);
		break;

	case EV_TESTLINE:
		DEBUGNAME("EV_TESTLINE");
		CG_TestLine(es->origin, es->origin2, es->time2, es->weapon, 1);
		break;

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		CG_PainEvent(cent, es->eventParm);
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*death%i.wav", event - EV_DEATH1 + 1), CS_BASIC);
		break;

		// Called by the FxRunner entity...usually for Environmental FX Events
	case EV_PLAY_EFFECT:
		DEBUGNAME("EV_PLAY_EFFECT");
		{
			vec3_t axis[3];
			const bool portal_ent = !!es->isPortalEnt;
			//the fx runner spawning this effect is within a skyportal, so only render this effect within that portal.

			s = CG_ConfigString(CS_EFFECTS + es->eventParm);

			if (es->boltInfo != 0)
			{
				const bool is_relative = !!es->weapon;
				theFxScheduler.PlayEffect(s, cent->lerpOrigin, axis, es->boltInfo, -1, portal_ent, es->loopSound,
					is_relative); //loopSound 0 = not looping, 1 for infinite, else duration
			}
			else
			{
				VectorCopy(cent->gent->pos3, axis[0]);
				VectorCopy(cent->gent->pos4, axis[1]);
				CrossProduct(axis[0], axis[1], axis[2]);

				// the entNum the effect may be attached to
				if (es->otherentity_num)
				{
					theFxScheduler.PlayEffect(s, cent->lerpOrigin, axis, -1, es->otherentity_num, portal_ent);
				}
				else
				{
					theFxScheduler.PlayEffect(s, cent->lerpOrigin, axis, -1, -1, portal_ent);
				}
			}
		}
		// Ghoul2 Insert End
		break;

		// play an effect bolted onto a muzzle
	case EV_PLAY_MUZZLE_EFFECT:
		DEBUGNAME("EV_PLAY_MUZZLE_EFFECT");
		s = CG_ConfigString(CS_EFFECTS + es->eventParm);

		theFxScheduler.PlayEffect(s, es->otherentity_num);
		break;

	case EV_STOP_EFFECT:
		DEBUGNAME("EV_STOP_EFFECT");
		{
			bool portal_ent = false;

			if (es->isPortalEnt)
			{
				//the fx runner spawning this effect is within a skyportal, so only render this effect within that portal.
				portal_ent = true;
			}

			s = CG_ConfigString(CS_EFFECTS + es->eventParm);
			if (es->boltInfo != 0)
			{
				theFxScheduler.StopEffect(s, es->boltInfo, portal_ent);
			}
		}
		break;

	case EV_TARGET_BEAM_DRAW:
		DEBUGNAME("EV_TARGET_BEAM_DRAW");
		if (cent->gent)
		{
			s = CG_ConfigString(CS_EFFECTS + es->eventParm);

			if (s && s[0])
			{
				const char* s2;
				if (cent->gent->delay)
				{
					s2 = CG_ConfigString(CS_EFFECTS + cent->gent->delay);
				}
				else
				{
					s2 = nullptr;
				}

				CG_DrawTargetBeam(cent->lerpOrigin, cent->gent->s.origin2, cent->gent->pos1, s, s2);
			}
		}
		break;

	case EV_ANGER1: //Say when acquire an enemy when didn't have one before
	case EV_ANGER2:
	case EV_ANGER3:
		DEBUGNAME("EV_ANGERx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*anger%i.wav", event - EV_ANGER1 + 1), CS_COMBAT);
		break;

	case EV_VICTORY1: //Say when killed an enemy
	case EV_VICTORY2:
	case EV_VICTORY3:
		DEBUGNAME("EV_VICTORYx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*victory%i.wav", event - EV_VICTORY1 + 1),
			CS_COMBAT);
		break;

	case EV_CONFUSE1: //Say when confused
	case EV_CONFUSE2:
	case EV_CONFUSE3:
		DEBUGNAME("EV_CONFUSEDx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*confuse%i.wav", event - EV_CONFUSE1 + 1),
			CS_COMBAT);
		break;

	case EV_PUSHED1: //Say when pushed
	case EV_PUSHED2:
	case EV_PUSHED3:
		DEBUGNAME("EV_PUSHEDx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*pushed%i.wav", event - EV_PUSHED1 + 1), CS_COMBAT);
		break;

	case EV_CHOKE1: //Say when choking
	case EV_CHOKE2:
	case EV_CHOKE3:
		DEBUGNAME("EV_CHOKEx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*choke%i.wav", event - EV_CHOKE1 + 1), CS_COMBAT);
		break;

	case EV_FFWARN: //Warn ally to stop shooting you
		DEBUGNAME("EV_FFWARN");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*ffwarn.wav", CS_COMBAT);
		break;

	case EV_FFTURN: //Turn on ally after being shot by them
		DEBUGNAME("EV_FFTURN");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*ffturn.wav", CS_COMBAT);
		break;

		//extra sounds for ST
	case EV_CHASE1:
	case EV_CHASE2:
	case EV_CHASE3:
		DEBUGNAME("EV_CHASEx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*chase%i.wav", event - EV_CHASE1 + 1), CS_EXTRA);
		break;
	case EV_COVER1:
	case EV_COVER2:
	case EV_COVER3:
	case EV_COVER4:
	case EV_COVER5:
		DEBUGNAME("EV_COVERx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*cover%i.wav", event - EV_COVER1 + 1), CS_EXTRA);
		break;
	case EV_DETECTED1:
	case EV_DETECTED2:
	case EV_DETECTED3:
	case EV_DETECTED4:
	case EV_DETECTED5:
		DEBUGNAME("EV_DETECTEDx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*detected%i.wav", event - EV_DETECTED1 + 1),
			CS_EXTRA);
		break;
	case EV_GIVEUP1:
	case EV_GIVEUP2:
	case EV_GIVEUP3:
	case EV_GIVEUP4:
		DEBUGNAME("EV_GIVEUPx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*giveup%i.wav", event - EV_GIVEUP1 + 1), CS_EXTRA);
		break;
	case EV_LOOK1:
	case EV_LOOK2:
		DEBUGNAME("EV_LOOKx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*look%i.wav", event - EV_LOOK1 + 1), CS_EXTRA);
		break;
	case EV_LOST1:
		DEBUGNAME("EV_LOST1");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*lost1.wav", CS_EXTRA);
		break;
	case EV_OUTFLANK1:
	case EV_OUTFLANK2:
		DEBUGNAME("EV_OUTFLANKx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*outflank%i.wav", event - EV_OUTFLANK1 + 1),
			CS_EXTRA);
		break;
	case EV_ESCAPING1:
	case EV_ESCAPING2:
	case EV_ESCAPING3:
		DEBUGNAME("EV_ESCAPINGx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*escaping%i.wav", event - EV_ESCAPING1 + 1),
			CS_EXTRA);
		break;
	case EV_SIGHT1:
	case EV_SIGHT2:
	case EV_SIGHT3:
		DEBUGNAME("EV_SIGHTx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*sight%i.wav", event - EV_SIGHT1 + 1), CS_EXTRA);
		break;
	case EV_SOUND1:
	case EV_SOUND2:
	case EV_SOUND3:
		DEBUGNAME("EV_SOUNDx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*sound%i.wav", event - EV_SOUND1 + 1), CS_EXTRA);
		break;
	case EV_SUSPICIOUS1:
	case EV_SUSPICIOUS2:
	case EV_SUSPICIOUS3:
	case EV_SUSPICIOUS4:
	case EV_SUSPICIOUS5:
		DEBUGNAME("EV_SUSPICIOUSx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*suspicious%i.wav", event - EV_SUSPICIOUS1 + 1),
			CS_EXTRA);
		break;
		//extra sounds for Jedi
	case EV_COMBAT1:
	case EV_COMBAT2:
	case EV_COMBAT3:
		DEBUGNAME("EV_COMBATx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*combat%i.wav", event - EV_COMBAT1 + 1), CS_JEDI);
		break;
	case EV_JDETECTED1:
	case EV_JDETECTED2:
	case EV_JDETECTED3:
		DEBUGNAME("EV_JDETECTEDx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*jdetected%i.wav", event - EV_JDETECTED1 + 1),
			CS_JEDI);
		break;
	case EV_TAUNT1:
	case EV_TAUNT2:
	case EV_TAUNT3:
		DEBUGNAME("EV_TAUNTx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*taunt%i.wav", event - EV_TAUNT1 + 1), CS_JEDI);
		break;
	case EV_JCHASE1:
	case EV_JCHASE2:
	case EV_JCHASE3:
		DEBUGNAME("EV_JCHASEx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*jchase%i.wav", event - EV_JCHASE1 + 1), CS_JEDI);
		break;
	case EV_JLOST1:
	case EV_JLOST2:
	case EV_JLOST3:
		DEBUGNAME("EV_JLOSTx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*jlost%i.wav", event - EV_JLOST1 + 1), CS_JEDI);
		break;
	case EV_DEFLECT1:
	case EV_DEFLECT2:
	case EV_DEFLECT3:
		DEBUGNAME("EV_DEFLECTx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*deflect%i.wav", event - EV_DEFLECT1 + 1), CS_JEDI);
		break;
	case EV_GLOAT1:
	case EV_GLOAT2:
	case EV_GLOAT3:
		DEBUGNAME("EV_GLOATx");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, va("*gloat%i.wav", event - EV_GLOAT1 + 1), CS_JEDI);
		break;
	case EV_PUSHFAIL:
		DEBUGNAME("EV_PUSHFAIL");
		CG_TryPlayCustomSound(nullptr, es->number, CHAN_VOICE, "*pushfail.wav", CS_JEDI);
		break;

	case EV_USE_FORCE:
		DEBUGNAME("EV_USE_FORCEITEM");
		CG_UseForce();
		break;

	case EV_USE_ITEM:
		DEBUGNAME("EV_USE_ITEM");
		CG_UseItem(cent);
		break;

	case EV_USE_INV_BINOCULARS:
		DEBUGNAME("EV_USE_INV_BINOCULARS");
		UseItem(INV_ELECTROBINOCULARS);
		break;

	case EV_USE_INV_BACTA:
		DEBUGNAME("EV_USE_INV_BACTA");
		UseItem(INV_BACTA_CANISTER);
		break;

	case EV_USE_INV_SEEKER:
		DEBUGNAME("EV_USE_INV_SEEKER");
		UseItem(INV_SEEKER);
		break;

	case EV_USE_INV_LIGHTAMP_GOGGLES:
		DEBUGNAME("EV_USE_INV_LIGHTAMP_GOGGLES");
		UseItem(INV_LIGHTAMP_GOGGLES);
		break;

	case EV_USE_INV_SENTRY:
		DEBUGNAME("EV_USE_INV_SENTRY");
		UseItem(INV_SENTRY);
		break;

	case EV_USE_INV_CLOAK:
		DEBUGNAME("EV_USE_INV_CLOAK");
		UseItem(INV_CLOAK);
		break;

	case EV_USE_INV_BARRIER:
		DEBUGNAME("EV_USE_INV_BARRIER");
		UseItem(INV_BARRIER);
		break;

	case EV_GIB_PLAYER_HEADSHOT:
		DEBUGNAME("EV_GIB_PLAYER_HEADSHOT");
		cent->pe.noHead = qtrue;
		CG_GibPlayerHeadshot(cent->lerpOrigin);
		break;

	case EV_BODY_NOHEAD:
		DEBUGNAME("EV_BODY_NOHEAD");
		cent->pe.noHead = qtrue;
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		CG_GibPlayer(cent->lerpOrigin);
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		CG_TestLine(position, es->origin2, es->time, static_cast<unsigned>(es->time2), es->weapon);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		CG_Error("Unknown event: %i", event);
		break;
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents(centity_t* cent)
{
	// check for event-only entities
	if (cent->currentState.eType > ET_EVENTS)
	{
		if (cent->previousEvent)
		{
			return; // already fired
		}
		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	}
	else
	{
		// check for events riding with another entity
		if (cent->currentState.event == cent->previousEvent)
		{
			return;
		}
		cent->previousEvent = cent->currentState.event;
		if ((cent->currentState.event & ~EV_EVENT_BITS) == 0)
		{
			return;
		}
	}

	// calculate the position at exactly the frame time
	EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
	CG_SetEntitySoundPosition(cent);

	CG_EntityEvent(cent, cent->lerpOrigin);
}