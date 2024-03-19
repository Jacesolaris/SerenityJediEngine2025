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

#include "qcommon/q_shared.h"
#include "g_local.h"

#include "bg_vehicles.h"

extern gentity_t* NPC_Spawn_Do(gentity_t* ent);
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
extern void G_DamageFromKiller(gentity_t* pEnt, const gentity_t* p_veh_ent, gentity_t* attacker, vec3_t org,
	int damage,
	int dflags, int mod);

extern void BG_SetAnim(playerState_t* ps, const animation_t* animations, int setAnimParts, int anim, int setAnimFlags);
extern void BG_SetLegsAnimTimer(playerState_t* ps, int time);
extern void BG_SetTorsoAnimTimer(playerState_t* ps, int time);
void G_VehUpdateShields(const gentity_t* targ);
#ifdef _GAME
extern void VEH_TurretThink(Vehicle_t* p_veh, gentity_t* parent, int turretNum);
#endif

extern qboolean BG_UnrestrainedPitchRoll(const playerState_t* ps, Vehicle_t* p_veh);

void Vehicle_SetAnim(gentity_t* ent, const int setAnimParts, const int anim, const int setAnimFlags, int i_blend)
{
	assert(ent->client);
	BG_SetAnim(&ent->client->ps, bgAllAnims[ent->localAnimIndex].anims, setAnimParts, anim, setAnimFlags);
	ent->s.legsAnim = ent->client->ps.legsAnim;
}

void G_VehicleTrace(trace_t* results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs, const vec3_t end,
	const int pass_entity_num, const int contentmask)
{
	trap->Trace(results, start, tMins, tMaxs, end, pass_entity_num, contentmask, qfalse, 0, 0);
}

Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt)
{
	const gentity_t* ent = pEnt;

	if (ent && ent->client && ent->client->NPC_class != CLASS_VEHICLE && ent->s.m_iVehicleNum != 0)
	{
		return g_entities[ent->s.m_iVehicleNum].m_pVehicle;
	}
	return NULL;
}

float G_CanJumpToEnemyVeh(Vehicle_t* p_veh, const usercmd_t* pUcmd)
{
	return 0.0f;
}

// Spawn this vehicle into the world.
void G_VehicleSpawn(gentity_t* self)
{
	VectorCopy(self->r.currentOrigin, self->s.origin);

	trap->LinkEntity((sharedEntity_t*)self);

	if (!self->count)
	{
		self->count = 1;
	}

	//save this because self gets removed in next func
	const float yaw = self->s.angles[YAW];

	gentity_t* vehEnt = NPC_Spawn_Do(self);

	if (!vehEnt)
	{
		return; //return NULL;
	}

	vehEnt->s.angles[YAW] = yaw;
	if (vehEnt->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL)
	{
		vehEnt->NPC->behaviorState = BS_CINEMATIC;
	}

	if (vehEnt->spawnflags & 1)
	{
		//die without pilot
		if (!vehEnt->damage)
		{
			//default 10 sec
			vehEnt->damage = 10000;
		}
		if (!vehEnt->speed)
		{
			//default 512 units
			vehEnt->speed = 512.0f;
		}
		vehEnt->m_pVehicle->m_iPilotTime = level.time + vehEnt->damage;
	}
	//return vehEnt;
}

// Attachs an entity to the vehicle it's riding (it's owner).
void G_AttachToVehicle(gentity_t* pEnt, usercmd_t** ucmd)
{
	mdxaBone_t boltMatrix;

	if (!pEnt || !ucmd)
		return;

	gentity_t* ent = pEnt;

	gentity_t* vehEnt = &g_entities[ent->r.ownerNum];
	ent->waypoint = vehEnt->waypoint; // take the veh's waypoint as your own

	if (!vehEnt->m_pVehicle)
		return;

	const int crotchBolt = trap->G2API_AddBolt(vehEnt->ghoul2, 0, "*driver");

	// Get the driver tag.
	trap->G2API_GetBoltMatrix(vehEnt->ghoul2, 0, crotchBolt, &boltMatrix,
		vehEnt->m_pVehicle->m_vOrientation, vehEnt->r.currentOrigin,
		level.time, NULL, vehEnt->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, ent->client->ps.origin);
	G_SetOrigin(ent, ent->client->ps.origin);
	trap->LinkEntity((sharedEntity_t*)ent);
}

// Animate the vehicle and it's riders.
void Animate(Vehicle_t* p_veh)
{
	// Validate a pilot rider.
	if (p_veh->m_pPilot)
	{
		if (p_veh->m_pVehicleInfo->AnimateRiders)
		{
			p_veh->m_pVehicleInfo->AnimateRiders(p_veh);
		}
	}

	p_veh->m_pVehicleInfo->AnimateVehicle(p_veh);
}

// Determine whether this entity is able to board this vehicle or not.
qboolean ValidateBoard(Vehicle_t* p_veh, bgEntity_t* pEnt)
{
	// Determine where the entity is entering the vehicle from (left, right, or back).
	vec3_t vVehToEnt;
	vec3_t vVehDir;
	const gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
	const gentity_t* ent = (gentity_t*)pEnt;
	vec3_t vVehAngles;

	if (p_veh->m_iDieTime > 0)
	{
		return qfalse;
	}

	if (p_veh->m_pPilot != NULL)
	{
		//already have a driver!
		if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
		{
			//I know, I know, this should by in the fighters's validateboard()
			//can never steal a fighter from it's pilot
			if (p_veh->m_iNumPassengers < p_veh->m_pVehicleInfo->maxPassengers)
			{
				return qtrue;
			}
			return qfalse;
		}
		if (p_veh->m_pVehicleInfo->type == VH_WALKER)
		{
			//I know, I know, this should by in the walker's validateboard()
			if (!ent->client || ent->client->ps.groundEntityNum != parent->s.number)
			{
				//can only steal an occupied AT-ST if you're on top (by the hatch)
				return qfalse;
			}
		}
		else if (p_veh->m_pVehicleInfo->type == VH_SPEEDER)
		{
			//you can only steal the bike from the driver if you landed on the driver or bike
			return p_veh->m_iBoarding == VEH_MOUNT_THROW_LEFT || p_veh->m_iBoarding == VEH_MOUNT_THROW_RIGHT;
		}
	}
	// Yes, you shouldn't have put this here (you 'should' have made an 'overriden' ValidateBoard func), but in this
	// instance it's more than adequate (which is why I do it too :-). Making a whole other function for this is silly.
	else if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
	{
		// If you're a fighter, you allow everyone to enter you from all directions.
		return qtrue;
	}

	// Clear out all orientation axis except for the yaw.
	VectorSet(vVehAngles, 0, parent->r.currentAngles[YAW], 0);

	// Vector from Entity to Vehicle.
	VectorSubtract(ent->r.currentOrigin, parent->r.currentOrigin, vVehToEnt);
	vVehToEnt[2] = 0;
	VectorNormalize(vVehToEnt);

	// Get the right vector.
	AngleVectors(vVehAngles, NULL, vVehDir, NULL);
	VectorNormalize(vVehDir);

	// Find the angle between the vehicle right vector and the vehicle to entity vector.
	const float fDot = DotProduct(vVehToEnt, vVehDir);

	// If the entity is within a certain angle to the left of the vehicle...
	if (fDot >= 0.5f)
	{
		// Right board.
		p_veh->m_iBoarding = -2;
	}
	else if (fDot <= -0.5f)
	{
		// Left board.
		p_veh->m_iBoarding = -1;
	}
	// Maybe they're trying to board from the back...
	else
	{
		// The forward vector of the vehicle.
		//	AngleVectors( vVehAngles, vVehDir, NULL, NULL );
		//	VectorNormalize( vVehDir );

		// Find the angle between the vehicle forward and the vehicle to entity vector.
		//	fDot = DotProduct( vVehToEnt, vVehDir );

		// If the entity is within a certain angle behind the vehicle...
		//if ( fDot <= -0.85f )
		{
			// Jump board.
			p_veh->m_iBoarding = -3;
		}
	}

	// If for some reason we couldn't board, leave...
	if (p_veh->m_iBoarding > -1)
		return qfalse;

	return qtrue;
}

#ifdef VEH_CONTROL_SCHEME_4
void FighterStorePilotViewAngles(Vehicle_t* p_veh, bgEntity_t* parent)
{
	playerState_t* rider_ps;
	bgEntity_t* rider = NULL;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}

	if (!rider)
	{
		rider = parent;
	}

	rider_ps = rider->playerState;
	VectorClear(p_veh->m_vPrevRiderViewAngles);
	p_veh->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(rider_ps->viewangles[YAW]);
}
#endif// VEH_CONTROL_SCHEME_4

// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
qboolean Board(Vehicle_t* p_veh, bgEntity_t* pEnt)
{
	vec3_t vPlayerDir;
	gentity_t* ent = (gentity_t*)pEnt;
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	// If it's not a valid entity, OR if the vehicle is blowing up (it's dead), OR it's not
	// empty, OR we're already being boarded, OR the person trying to get on us is already
	// in a vehicle (that was a fun bug :-), leave!
	if (!ent || parent->health <= 0 /*|| !( parent->client->ps.eFlags & EF_EMPTY_VEHICLE )*/ || p_veh->m_iBoarding > 0 ||
		ent->client->ps.m_iVehicleNum)
		return qfalse;

	// Bucking so we can't do anything (NOTE: Should probably be a better name since fighters don't buck...).
	if (p_veh->m_ulFlags & VEH_BUCKING)
		return qfalse;

	// Validate the entity's ability to board this vehicle.
	if (!p_veh->m_pVehicleInfo->ValidateBoard(p_veh, pEnt))
		return qfalse;

	// FIXME FIXME!!! Ask Mike monday where ent->client->ps.eFlags might be getting changed!!! It is always 0 (when it should
	// be 1024) so a person riding a vehicle is able to ride another vehicle!!!!!!!!

	// Tell everybody their status.
	// ALWAYS let the player be the pilot.
	if (ent->s.number < MAX_CLIENTS)
	{
		p_veh->m_pOldPilot = p_veh->m_pPilot;

		if (!p_veh->m_pPilot)
		{
			//become the pilot, if there isn't one now
			p_veh->m_pVehicleInfo->SetPilot(p_veh, (bgEntity_t*)ent);
		}
		// If we're not yet full...
		else if (p_veh->m_iNumPassengers < p_veh->m_pVehicleInfo->maxPassengers)
		{
			// Find an empty slot and put that passenger here.
			for (int i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
			{
				if (p_veh->m_ppPassengers[i] == NULL)
				{
					p_veh->m_ppPassengers[i] = (bgEntity_t*)ent;
#ifdef _GAME
					//Server just needs to tell client which passengernum he is
					if (ent->client)
					{
						ent->client->ps.generic1 = i + 1;
					}
#endif
					break;
				}
			}
			p_veh->m_iNumPassengers++;
		}
		// We're full, sorry...
		else
		{
			return qfalse;
		}
		ent->s.m_iVehicleNum = parent->s.number;
		if (ent->client)
		{
			ent->client->ps.m_iVehicleNum = ent->s.m_iVehicleNum;
		}
		if (p_veh->m_pPilot == (bgEntity_t*)ent)
		{
			parent->r.ownerNum = ent->s.number;
			parent->s.owner = parent->r.ownerNum; //for prediction
		}

#ifdef _GAME
		{
			gentity_t* gParent = parent;
			if (gParent->spawnflags & 2)
			{
				//was being suspended
				gParent->spawnflags &= ~2;
				//SUSPENDED - clear this spawnflag, no longer docked, okay to free-fall if not in space
				G_Sound(gParent, CHAN_AUTO, G_SoundIndex("sound/vehicles/common/release.wav"));
				if (gParent->fly_sound_debounce_time)
				{
					//we should drop like a rock for a few seconds
					p_veh->m_iDropTime = level.time + gParent->fly_sound_debounce_time;
				}
			}
		}
#endif

		//FIXME: rider needs to look in vehicle's direction when he gets in
		// Clear these since they're used to turn the vehicle now.
		/*SetClientViewAngle( ent, p_veh->m_vOrientation );
		memset( &parent->client->usercmd, 0, sizeof( usercmd_t ) );
		memset( &p_veh->m_ucmd, 0, sizeof( usercmd_t ) );
		VectorClear( parent->client->ps.viewangles );
		VectorClear( parent->client->ps.delta_angles );*/

		// Set the looping sound only when there is a pilot (when the vehicle is "on").
		if (p_veh->m_pVehicleInfo->soundLoop)
		{
			parent->client->ps.loopSound = parent->s.loopSound = p_veh->m_pVehicleInfo->soundLoop;
		}
	}
	else
	{
		// If there's no pilot, try to drive this vehicle.
		if (p_veh->m_pPilot == NULL)
		{
			p_veh->m_pVehicleInfo->SetPilot(p_veh, (bgEntity_t*)ent);
			// TODO: Set pilot should do all this stuff....
			parent->r.ownerNum = ent->s.number;
			parent->s.owner = parent->r.ownerNum; //for prediction
			// Set the looping sound only when there is a pilot (when the vehicle is "on").
			if (p_veh->m_pVehicleInfo->soundLoop)
			{
				parent->client->ps.loopSound = parent->s.loopSound = p_veh->m_pVehicleInfo->soundLoop;
			}

			parent->client->ps.speed = 0;
			memset(&p_veh->m_ucmd, 0, sizeof(usercmd_t));
		}
		// If we're not yet full...
		else if (p_veh->m_iNumPassengers < p_veh->m_pVehicleInfo->maxPassengers)
		{
			// Find an empty slot and put that passenger here.
			for (int i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
			{
				if (p_veh->m_ppPassengers[i] == NULL)
				{
					p_veh->m_ppPassengers[i] = (bgEntity_t*)ent;
#ifdef _GAME
					//Server just needs to tell client which passengernum he is
					if (ent->client)
					{
						ent->client->ps.generic1 = i + 1;
					}
#endif
					break;
				}
			}
			p_veh->m_iNumPassengers++;
		}
		// We're full, sorry...
		else
		{
			return qfalse;
		}
	}

	// Make sure the entity knows it's in a vehicle.
	ent->client->ps.m_iVehicleNum = parent->s.number;
	ent->r.ownerNum = parent->s.number;
	ent->s.owner = ent->r.ownerNum; //for prediction
	if (p_veh->m_pPilot == (bgEntity_t*)ent)
	{
		parent->client->ps.m_iVehicleNum = ent->s.number + 1;
		//always gonna be under MAX_CLIENTS so no worries about 1 byte overflow
	}

	//memset( &ent->client->usercmd, 0, sizeof( usercmd_t ) );

	//FIXME: no saber or weapons if numHands = 2, should switch to speeder weapon, no attack anim on player
	if (p_veh->m_pVehicleInfo->numHands == 2)
	{
		//switch to vehicle weapon
	}

	if (p_veh->m_pVehicleInfo->hideRider)
	{
		//hide the rider
		p_veh->m_pVehicleInfo->Ghost(p_veh, (bgEntity_t*)ent);
	}

	// Play the start sounds
	if (p_veh->m_pVehicleInfo->soundOn)
	{
		G_Sound(parent, CHAN_AUTO, p_veh->m_pVehicleInfo->soundOn);
	}

#ifdef VEH_CONTROL_SCHEME_4
	if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
	{//clear their angles
		FighterStorePilotViewAngles(p_veh, (bgEntity_t*)parent);
	}
#endif //VEH_CONTROL_SCHEME_4

	VectorCopy(p_veh->m_vOrientation, vPlayerDir);
	vPlayerDir[ROLL] = 0;
	SetClientViewAngle(ent, vPlayerDir);

	return qtrue;
}

qboolean VEH_TryEject(const Vehicle_t* p_veh,
	const gentity_t* parent,
	gentity_t* ent,
	const int ejectDir,
	vec3_t vExitPos)
{
	vec3_t vEntMins, vEntMaxs, vVehLeaveDir, vVehAngles;
	trace_t m_ExitTrace;

	// Make sure that the entity is not 'stuck' inside the vehicle (since their bboxes will now intersect).
	// This makes the entity leave the vehicle from the right side.
	VectorSet(vVehAngles, 0, parent->r.currentAngles[YAW], 0);
	switch (ejectDir)
	{
		// Left.
	case VEH_EJECT_LEFT:
		AngleVectors(vVehAngles, NULL, vVehLeaveDir, NULL);
		vVehLeaveDir[0] = -vVehLeaveDir[0];
		vVehLeaveDir[1] = -vVehLeaveDir[1];
		vVehLeaveDir[2] = -vVehLeaveDir[2];
		break;
		// Right.
	case VEH_EJECT_RIGHT:
		AngleVectors(vVehAngles, NULL, vVehLeaveDir, NULL);
		break;
		// Front.
	case VEH_EJECT_FRONT:
		AngleVectors(vVehAngles, vVehLeaveDir, NULL, NULL);
		break;
		// Rear.
	case VEH_EJECT_REAR:
		AngleVectors(vVehAngles, vVehLeaveDir, NULL, NULL);
		vVehLeaveDir[0] = -vVehLeaveDir[0];
		vVehLeaveDir[1] = -vVehLeaveDir[1];
		vVehLeaveDir[2] = -vVehLeaveDir[2];
		break;
		// Top.
	case VEH_EJECT_TOP:
		AngleVectors(vVehAngles, NULL, NULL, vVehLeaveDir);
		break;
		// Bottom?.
	case VEH_EJECT_BOTTOM:
		break;
	default:;
	}
	VectorNormalize(vVehLeaveDir);
	//NOTE: not sure why following line was needed - MCG
	//p_veh->m_EjectDir = VEH_EJECT_LEFT;

	// Since (as of this time) the collidable geometry of the entity is just an axis
	// aligned box, we need to get the diagonal length of it in case we come out on that side.
	// Diagonal Length == squareroot( squared( Sidex / 2 ) + squared( Sidey / 2 ) );

	// TODO: DO diagonal for entity.

	float fBias = 1.0f;
	if (p_veh->m_pVehicleInfo->type == VH_WALKER)
	{
		//hacktastic!
		fBias += 0.2f;
	}
	VectorCopy(ent->r.currentOrigin, vExitPos);
	const float fVehDiag = sqrtf(parent->r.maxs[0] * parent->r.maxs[0] + parent->r.maxs[1] * parent->r.maxs[1]);
	VectorCopy(ent->r.maxs, vEntMaxs);
	if (ent->s.number < MAX_CLIENTS)
	{
		//for some reason, in MP, player client mins and maxs are never stored permanently, just set to these hardcoded numbers in PMove
		vEntMaxs[0] = 15;
		vEntMaxs[1] = 15;
	}
	const float fEntDiag = sqrtf(vEntMaxs[0] * vEntMaxs[0] + vEntMaxs[1] * vEntMaxs[1]);
	vVehLeaveDir[0] *= (fVehDiag + fEntDiag) * fBias; // x
	vVehLeaveDir[1] *= (fVehDiag + fEntDiag) * fBias; // y
	vVehLeaveDir[2] *= (fVehDiag + fEntDiag) * fBias;
	VectorAdd(vExitPos, vVehLeaveDir, vExitPos);

	//we actually could end up *not* getting off if the trace fails...
	// Check to see if this new position is a valid place for our entity to go.
	VectorSet(vEntMins, -15.0f, -15.0f, DEFAULT_MINS_2);
	VectorSet(vEntMaxs, 15.0f, 15.0f, DEFAULT_MAXS_2);
	const int oldOwner = ent->r.ownerNum;
	ent->r.ownerNum = ENTITYNUM_NONE;
	G_VehicleTrace(&m_ExitTrace, ent->r.currentOrigin, vEntMins, vEntMaxs, vExitPos, ent->s.number, ent->clipmask);
	ent->r.ownerNum = oldOwner;

	if (m_ExitTrace.allsolid //in solid
		|| m_ExitTrace.startsolid)
	{
		return qfalse;
	}
	// If the trace hit something, we can't go there!
	if (m_ExitTrace.fraction < 1.0f)
	{
		//not totally clear
		if (parent->clipmask & ent->r.contents) //vehicle could actually get stuck on body
		{
			//the trace hit the vehicle, don't let them get out, just in case
			return qfalse;
		}
		//otherwise, use the trace.endpos
		VectorCopy(m_ExitTrace.endpos, vExitPos);
	}
	return qtrue;
}

static void G_EjectDroidUnit(Vehicle_t* p_veh, const qboolean kill)
{
	p_veh->m_pDroidUnit->s.m_iVehicleNum = ENTITYNUM_NONE;
	p_veh->m_pDroidUnit->s.owner = ENTITYNUM_NONE;
	//	p_veh->m_pDroidUnit->s.otherentity_num2 = ENTITYNUM_NONE;
#ifdef _GAME
	{
		gentity_t* droidEnt = (gentity_t*)p_veh->m_pDroidUnit;
		droidEnt->flags &= ~FL_UNDYING;
		droidEnt->r.ownerNum = ENTITYNUM_NONE;
		if (droidEnt->client)
		{
			droidEnt->client->ps.m_iVehicleNum = ENTITYNUM_NONE;
		}
		if (kill)
		{
			//Kill them, too
			//FIXME: proper origin, MOD and attacker (for credit/death message)?  Get from vehicle?
			G_MuteSound(droidEnt->s.number, CHAN_VOICE);
			G_Damage(droidEnt, NULL, NULL, NULL, droidEnt->s.origin, 10000, 0, MOD_SUICIDE);
			//FIXME: proper MOD?  Get from vehicle?
		}
	}
#endif
	p_veh->m_pDroidUnit = NULL;
}

// Eject the pilot from the vehicle.
static qboolean Eject(Vehicle_t* p_veh, bgEntity_t* pEnt, const qboolean forceEject)
{
	gentity_t* parent;
	vec3_t vExitPos;
	gentity_t* ent = (gentity_t*)pEnt;

	qboolean taintedRider = qfalse;
	qboolean deadRider = qfalse;

	if (pEnt == p_veh->m_pDroidUnit)
	{
		G_EjectDroidUnit(p_veh, qfalse);
		return qtrue;
	}

	if (ent)
	{
		if (!ent->inuse || !ent->client || ent->client->pers.connected != CON_CONNECTED)
		{
			taintedRider = qtrue;
			parent = (gentity_t*)p_veh->m_pParentEntity;
			goto getItOutOfMe;
		}
		if (ent->health < 1)
		{
			deadRider = qtrue;
		}
	}

	// Validate.
	if (!ent)
	{
		return qfalse;
	}
	if (!forceEject)
	{
		if (!(p_veh->m_iBoarding == 0 || p_veh->m_iBoarding == -999 || p_veh->m_iBoarding < -3 && p_veh->m_iBoarding >= -9))
		{
			deadRider = qtrue;
			p_veh->m_iBoarding = 0;
			p_veh->m_bWasBoarding = qfalse;
		}
	}

	parent = (gentity_t*)p_veh->m_pParentEntity;

	//Try ejecting in every direction
	if (p_veh->m_EjectDir < VEH_EJECT_LEFT)
	{
		p_veh->m_EjectDir = VEH_EJECT_LEFT;
	}
	else if (p_veh->m_EjectDir > VEH_EJECT_BOTTOM)
	{
		p_veh->m_EjectDir = VEH_EJECT_BOTTOM;
	}
	const int firstEjectDir = p_veh->m_EjectDir;
	while (!VEH_TryEject(p_veh, parent, ent, p_veh->m_EjectDir, vExitPos))
	{
		p_veh->m_EjectDir++;
		if (p_veh->m_EjectDir > VEH_EJECT_BOTTOM)
		{
			p_veh->m_EjectDir = VEH_EJECT_LEFT;
		}
		if (p_veh->m_EjectDir == firstEjectDir)
		{
			//they all failed
			if (!deadRider)
			{
				//if he's dead.. just shove him in solid, who cares.
				return qfalse;
			}
			if (forceEject)
			{
				//we want to always get out, just eject him here
				VectorCopy(ent->r.currentOrigin, vExitPos);
				break;
			}
			//can't eject
			return qfalse;
		}
	}

	// Move them to the exit position.
	G_SetOrigin(ent, vExitPos);
	VectorCopy(ent->r.currentOrigin, ent->client->ps.origin);
	trap->LinkEntity((sharedEntity_t*)ent);

	// If it's the player, stop overrides.
	if (ent->s.number < MAX_CLIENTS)
	{
	}

getItOutOfMe:

	// If he's the pilot...
	if ((gentity_t*)p_veh->m_pPilot == ent)
	{
		int j = 0;

		p_veh->m_pPilot = NULL;
		parent->r.ownerNum = ENTITYNUM_NONE;
		parent->s.owner = parent->r.ownerNum; //for prediction

		//keep these current angles
		//SetClientViewAngle( parent, p_veh->m_vOrientation );
		memset(&parent->client->pers.cmd, 0, sizeof(usercmd_t));
		memset(&p_veh->m_ucmd, 0, sizeof(usercmd_t));

		while (j < p_veh->m_iNumPassengers)
		{
			if (p_veh->m_ppPassengers[j])
			{
				int k = 1;
				p_veh->m_pVehicleInfo->SetPilot(p_veh, p_veh->m_ppPassengers[j]);
				parent->r.ownerNum = p_veh->m_ppPassengers[j]->s.number;
				parent->s.owner = parent->r.ownerNum; //for prediction
				parent->client->ps.m_iVehicleNum = p_veh->m_ppPassengers[j]->s.number + 1;

				//rearrange the passenger slots now..
#ifdef _GAME
				//Server just needs to tell client he's not a passenger anymore
				if (((gentity_t*)p_veh->m_ppPassengers[j])->client)
				{
					((gentity_t*)p_veh->m_ppPassengers[j])->client->ps.generic1 = 0;
				}
#endif
				p_veh->m_ppPassengers[j] = NULL;
				while (k < p_veh->m_iNumPassengers)
				{
					if (!p_veh->m_ppPassengers[k - 1])
					{
						//move down
						p_veh->m_ppPassengers[k - 1] = p_veh->m_ppPassengers[k];
						p_veh->m_ppPassengers[k] = NULL;
#ifdef _GAME
						//Server just needs to tell client which passenger he is
						if (p_veh->m_ppPassengers[k - 1] && ((gentity_t*)p_veh->m_ppPassengers[k - 1])->client)
						{
							((gentity_t*)p_veh->m_ppPassengers[k - 1])->client->ps.generic1 = k;
						}
#endif
					}
					k++;
				}
				p_veh->m_iNumPassengers--;

				break;
			}
			j++;
		}
	}
	else if (ent == (gentity_t*)p_veh->m_pOldPilot)
	{
		p_veh->m_pOldPilot = 0;
	}
	else
	{
		int i;
		// Look for this guy in the passenger list.
		for (i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
		{
			// If we found him...
			if ((gentity_t*)p_veh->m_ppPassengers[i] == ent)
			{
#ifdef _GAME
				//Server just needs to tell client he's not a passenger anymore
				if (((gentity_t*)p_veh->m_ppPassengers[i])->client)
				{
					((gentity_t*)p_veh->m_ppPassengers[i])->client->ps.generic1 = 0;
				}
#endif
				p_veh->m_ppPassengers[i] = NULL;
				p_veh->m_iNumPassengers--;
				break;
			}
		}

		// Didn't find him, can't eject because they aren't in the vehicle (hopefully)!
		if (i == p_veh->m_pVehicleInfo->maxPassengers)
		{
			return qfalse;
		}
	}

	//if (!taintedRider)
	{
		if (p_veh->m_pVehicleInfo->hideRider)
		{
			p_veh->m_pVehicleInfo->UnGhost(p_veh, (bgEntity_t*)ent);
		}
	}

	// If the vehicle now has no pilot...
	if (p_veh->m_pPilot == NULL)
	{
		parent->client->ps.loopSound = parent->s.loopSound = 0;
		// Completely empty vehicle...?
		if (p_veh->m_iNumPassengers == 0)
		{
			parent->client->ps.m_iVehicleNum = 0;
		}
	}

	if (taintedRider)
	{
		//you can go now
		p_veh->m_iBoarding = level.time + 1000;
		return qtrue;
	}

	// Client not in a vehicle.
	ent->client->ps.m_iVehicleNum = 0;
	ent->r.ownerNum = ENTITYNUM_NONE;
	ent->s.owner = ent->r.ownerNum; //for prediction

	ent->client->ps.viewangles[PITCH] = 0.0f;
	ent->client->ps.viewangles[ROLL] = 0.0f;
	ent->client->ps.viewangles[YAW] = p_veh->m_vOrientation[YAW];
	SetClientViewAngle(ent, ent->client->ps.viewangles);

	if (ent->client->solidHack)
	{
		ent->client->solidHack = 0;
		ent->r.contents = CONTENTS_BODY;
	}
	ent->s.m_iVehicleNum = 0;

	//if was using vehicle weapon, remove it and switch to normal weapon when hop out...
	if (ent->client->ps.weapon == WP_NONE)
	{
		//FIXME: check against this vehicle's gun from the g_vehicleInfo table
	}
	else
	{
		//FIXME: if they have their saber out:
	}
	BG_SetLegsAnimTimer(&ent->client->ps, 0);
	BG_SetTorsoAnimTimer(&ent->client->ps, 0);

	// Set how long until this vehicle can be boarded again.
	p_veh->m_iBoarding = level.time + 1000;

	return qtrue;
}

// Eject all the inhabitants of this vehicle.
qboolean EjectAll(Vehicle_t* p_veh)
{
	// TODO: Setup a default escape for ever vehicle type.

	p_veh->m_EjectDir = VEH_EJECT_TOP;
	// Make sure no other boarding calls exist. We MUST exit.
	p_veh->m_iBoarding = 0;
	p_veh->m_bWasBoarding = qfalse;

	// Throw them off.
	if (p_veh->m_pPilot)
	{
#ifdef _GAME
		gentity_t* pilot = (gentity_t*)p_veh->m_pPilot;
#endif
		p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_pPilot, qtrue);
#ifdef _GAME
		if (p_veh->m_pVehicleInfo->killRiderOnDeath && pilot)
		{
			//Kill them, too
			//FIXME: proper origin, MOD and attacker (for credit/death message)?  Get from vehicle?
			G_MuteSound(pilot->s.number, CHAN_VOICE);
			G_Damage(pilot, NULL, NULL, NULL, pilot->s.origin, 10000, 0, MOD_SUICIDE);
		}
#endif
	}
	if (p_veh->m_pOldPilot)
	{
#ifdef _GAME
		gentity_t* pilot = (gentity_t*)p_veh->m_pOldPilot;
#endif
		p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_pOldPilot, qtrue);
#ifdef _GAME
		if (p_veh->m_pVehicleInfo->killRiderOnDeath && pilot)
		{
			//Kill them, too
			//FIXME: proper origin, MOD and attacker (for credit/death message)?  Get from vehicle?
			G_MuteSound(pilot->s.number, CHAN_VOICE);
			G_Damage(pilot, NULL, NULL, NULL, pilot->s.origin, 10000, 0, MOD_SUICIDE);
		}
#endif
	}
	if (p_veh->m_iNumPassengers)
	{
		for (int i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
		{
			if (p_veh->m_ppPassengers[i])
			{
#ifdef _GAME
				gentity_t* rider = (gentity_t*)p_veh->m_ppPassengers[i];
#endif
				p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_ppPassengers[i], qtrue);
#ifdef _GAME
				if (p_veh->m_pVehicleInfo->killRiderOnDeath && rider)
				{
					//Kill them, too
					//FIXME: proper origin, MOD and attacker (for credit/death message)?  Get from vehicle?
					G_MuteSound(rider->s.number, CHAN_VOICE);
					G_Damage(rider, NULL, NULL, NULL, rider->s.origin, 10000, 0, MOD_SUICIDE);
					//FIXME: proper MOD?  Get from vehicle?
				}
#endif
			}
		}
		p_veh->m_iNumPassengers = 0;
	}

	if (p_veh->m_pDroidUnit)
	{
		G_EjectDroidUnit(p_veh, p_veh->m_pVehicleInfo->killRiderOnDeath);
	}

	return qtrue;
}

// Start a delay until the vehicle explodes.
static void StartDeathDelay(Vehicle_t* p_veh, const int iDelayTimeOverride)
{
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	if (iDelayTimeOverride)
	{
		p_veh->m_iDieTime = level.time + iDelayTimeOverride;
	}
	else
	{
		p_veh->m_iDieTime = level.time + p_veh->m_pVehicleInfo->explosionDelay;
	}

	if (p_veh->m_pVehicleInfo->flammable)
	{
		parent->client->ps.loopSound = parent->s.loopSound = G_SoundIndex("sound/vehicles/common/fire_lp.wav");
	}
}

// Decide whether to explode the vehicle or not.
static void DeathUpdate(Vehicle_t* p_veh)
{
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	if (level.time >= p_veh->m_iDieTime)
	{
		// If the vehicle is not empty.
		if (p_veh->m_pVehicleInfo->Inhabited(p_veh))
		{
			p_veh->m_pVehicleInfo->EjectAll(p_veh);
			if (p_veh->m_pVehicleInfo->Inhabited(p_veh))
			{
				//if we've still got people in us, just kill the bastards
				if (p_veh->m_pPilot)
				{
					//FIXME: does this give proper credit to the enemy who shot you down?
					G_Damage((gentity_t*)p_veh->m_pPilot, (gentity_t*)p_veh->m_pParentEntity,
						(gentity_t*)p_veh->m_pParentEntity,
						NULL, p_veh->m_pParentEntity->playerState->origin, 999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
				}
				if (p_veh->m_iNumPassengers)
				{
					for (int i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
					{
						if (p_veh->m_ppPassengers[i])
						{
							//FIXME: does this give proper credit to the enemy who shot you down?
							G_Damage((gentity_t*)p_veh->m_ppPassengers[i], (gentity_t*)p_veh->m_pParentEntity,
								(gentity_t*)p_veh->m_pParentEntity,
								NULL, p_veh->m_pParentEntity->playerState->origin, 999, DAMAGE_NO_PROTECTION,
								MOD_SUICIDE);
						}
					}
				}
			}
		}

		if (!p_veh->m_pVehicleInfo->Inhabited(p_veh))
		{
			//explode now as long as we managed to kick everyone out
			vec3_t bottom;
			trace_t trace;

			if (p_veh->m_pVehicleInfo->iExplodeFX)
			{
				vec3_t fxAng;

				VectorSet(fxAng, -90.0f, 0.0f, 0.0f);
				G_PlayEffectID(p_veh->m_pVehicleInfo->iExplodeFX, parent->r.currentOrigin, fxAng);
				//trace down and place mark
				VectorCopy(parent->r.currentOrigin, bottom);
				bottom[2] -= 80;
				G_VehicleTrace(&trace, parent->r.currentOrigin, vec3_origin, vec3_origin, bottom, parent->s.number,
					CONTENTS_SOLID);
				if (trace.fraction < 1.0f)
				{
					VectorCopy(trace.endpos, bottom);
					bottom[2] += 2;
					VectorSet(fxAng, -90.0f, 0.0f, 0.0f);
					G_PlayEffectID(G_EffectIndex("ships/ship_explosion_mark"), trace.endpos, fxAng);
				}
			}

			parent->takedamage = qfalse; //so we don't recursively damage ourselves
			if (p_veh->m_pVehicleInfo->explosionRadius > 0 && p_veh->m_pVehicleInfo->explosionDamage > 0)
			{
				vec3_t lMaxs;
				vec3_t lMins;
				VectorCopy(parent->r.mins, lMins);
				lMins[2] = -4; //to keep it off the ground a *little*
				VectorCopy(parent->r.maxs, lMaxs);
				VectorCopy(parent->r.currentOrigin, bottom);
				bottom[2] += parent->r.mins[2] - 32;
				G_VehicleTrace(&trace, parent->r.currentOrigin, lMins, lMaxs, bottom, parent->s.number, CONTENTS_SOLID);
				g_radius_damage(trace.endpos, NULL, p_veh->m_pVehicleInfo->explosionDamage,
					p_veh->m_pVehicleInfo->explosionRadius, NULL, NULL, MOD_SUICIDE);
				//FIXME: extern damage and radius or base on fuel
			}

			parent->think = G_FreeEntity;
			parent->nextthink = level.time + FRAMETIME;
		}
	}
}

// Register all the assets used by this vehicle.
static void RegisterAssets(Vehicle_t* p_veh)
{
}

extern void ChangeWeapon(const gentity_t* ent, int new_weapon);

// Initialize the vehicle.
static qboolean Initialize(Vehicle_t* p_veh)
{
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
	int i;

	if (!parent || !parent->client)
		return qfalse;

	parent->client->ps.m_iVehicleNum = 0;
	parent->s.m_iVehicleNum = 0;
	{
		p_veh->m_iArmor = p_veh->m_pVehicleInfo->armor;
		parent->client->pers.maxHealth = parent->client->ps.stats[STAT_MAX_HEALTH] = parent->NPC->stats.health = parent
			->health = parent->client->ps.stats[STAT_HEALTH] = p_veh->m_iArmor;
		p_veh->m_iShields = p_veh->m_pVehicleInfo->shields;
		G_VehUpdateShields(parent);
		parent->client->ps.stats[STAT_ARMOR] = p_veh->m_iShields;
	}
	parent->mass = p_veh->m_pVehicleInfo->mass;
	//initialize the ammo to max
	for (i = 0; i < MAX_VEHICLE_WEAPONS; i++)
	{
		parent->client->ps.ammo[i] = p_veh->weaponStatus[i].ammo = p_veh->m_pVehicleInfo->weapon[i].ammoMax;
	}
	for (i = 0; i < MAX_VEHICLE_TURRETS; i++)
	{
		p_veh->turretStatus[i].nextMuzzle = p_veh->m_pVehicleInfo->turret[i].iMuzzle[i] - 1;
		parent->client->ps.ammo[MAX_VEHICLE_WEAPONS + i] = p_veh->turretStatus[i].ammo = p_veh->m_pVehicleInfo->turret[i].
			iAmmoMax;
		if (p_veh->m_pVehicleInfo->turret[i].bAI)
		{
			//they're going to be finding enemies, init this to NONE
			p_veh->turretStatus[i].enemyEntNum = ENTITYNUM_NONE;
		}
	}
	//begin stopped...?
	parent->client->ps.speed = 0;

	VectorClear(p_veh->m_vOrientation);
	p_veh->m_vOrientation[YAW] = parent->s.angles[YAW];

	if (p_veh->m_pVehicleInfo->gravity &&
		p_veh->m_pVehicleInfo->gravity != g_gravity.value)
	{
		//not normal gravity
		if (parent->NPC)
		{
			parent->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		}
		parent->client->ps.gravity = p_veh->m_pVehicleInfo->gravity;
	}

	if (p_veh->m_pVehicleInfo->maxPassengers > 0)
	{
		// Allocate an array of entity pointers.
		for (i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
		{
			p_veh->m_ppPassengers[i] = NULL;
		}
	}

	p_veh->m_iNumPassengers = 0;
	/*
	if ( p_veh->m_iVehicleTypeID == VH_FIGHTER )
	{
		p_veh->m_ulFlags = VEH_GEARSOPEN;
	}
	else
	*/
	//why?! -rww
	{
		p_veh->m_ulFlags = 0;
	}
	p_veh->m_fTimeModifier = 1.0f;
	p_veh->m_iBoarding = 0;
	p_veh->m_bWasBoarding = qfalse;
	p_veh->m_pOldPilot = NULL;
	VectorClear(p_veh->m_vBoardingVelocity);
	p_veh->m_pPilot = NULL;
	memset(&p_veh->m_ucmd, 0, sizeof(usercmd_t));
	p_veh->m_iDieTime = 0;
	p_veh->m_EjectDir = VEH_EJECT_LEFT;

	memset(p_veh->m_iExhaustTag, -1, sizeof(int) * MAX_VEHICLE_EXHAUSTS);
	memset(p_veh->m_iMuzzleTag, -1, sizeof(int) * MAX_VEHICLE_MUZZLES);
	// FIXME! Use external values read from the vehicle data file!
	p_veh->m_iDroidUnitTag = -1;

	//initialize to blaster, just since it's a basic weapon and there's no lightsaber crap...?
	parent->client->ps.weapon = WP_BLASTER;
	parent->client->ps.weaponstate = WEAPON_READY;
	parent->client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;

	//Initialize to landed (wings closed, gears down) animation
	{
		const int iFlags = SETANIM_FLAG_NORMAL;

		p_veh->m_ulFlags |= VEH_GEARSOPEN;
		BG_SetAnim(p_veh->m_pParentEntity->playerState, bgAllAnims[p_veh->m_pParentEntity->localAnimIndex].anims,
			SETANIM_BOTH, BOTH_VS_IDLE, iFlags);
	}

	return qtrue;
}

// Like a think or move command, this updates various vehicle properties.
void G_VehicleDamageBoxSizing(const Vehicle_t* p_veh); //declared below
static qboolean Update(Vehicle_t* p_veh, const usercmd_t* pUmcd)
{
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
	gentity_t* pilotEnt;
	//static float fMod = 1000.0f / 60.0f;
	vec3_t vVehAngles;
	int i;
	int prevSpeed;
	int nextSpeed;
	int curTime;
	int halfMaxSpeed;
	qboolean linkHeld = qfalse;

	playerState_t* parent_ps = p_veh->m_pParentEntity->playerState;

#ifdef _GAME
	curTime = level.time;
#elif _CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif

	//increment the ammo for all rechargeable weapons
	for (i = 0; i < MAX_VEHICLE_WEAPONS; i++)
	{
		if (p_veh->m_pVehicleInfo->weapon[i].ID > VEH_WEAPON_BASE //have a weapon in this slot
			&& p_veh->m_pVehicleInfo->weapon[i].ammoRechargeMS //its ammo is rechargable
			&& p_veh->weaponStatus[i].ammo < p_veh->m_pVehicleInfo->weapon[i].ammoMax //its ammo is below max
			&& pUmcd->serverTime - p_veh->weaponStatus[i].lastAmmoInc >= p_veh->m_pVehicleInfo->weapon[i].ammoRechargeMS)
			//enough time has passed
		{
			//add 1 to the ammo
			p_veh->weaponStatus[i].lastAmmoInc = pUmcd->serverTime;
			p_veh->weaponStatus[i].ammo++;
			//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
			if (parent && parent->client)
			{
				parent->client->ps.ammo[i] = p_veh->weaponStatus[i].ammo;
			}
		}
	}
	for (i = 0; i < MAX_VEHICLE_TURRETS; i++)
	{
		if (p_veh->m_pVehicleInfo->turret[i].iWeapon > VEH_WEAPON_BASE //have a weapon in this slot
			&& p_veh->m_pVehicleInfo->turret[i].iAmmoRechargeMS //its ammo is rechargable
			&& p_veh->turretStatus[i].ammo < p_veh->m_pVehicleInfo->turret[i].iAmmoMax //its ammo is below max
			&& pUmcd->serverTime - p_veh->turretStatus[i].lastAmmoInc >= p_veh->m_pVehicleInfo->turret[i].iAmmoRechargeMS)
			//enough time has passed
		{
			//add 1 to the ammo
			p_veh->turretStatus[i].lastAmmoInc = pUmcd->serverTime;
			p_veh->turretStatus[i].ammo++;
			//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
			if (parent && parent->client)
			{
				parent->client->ps.ammo[MAX_VEHICLE_WEAPONS + i] = p_veh->turretStatus[i].ammo;
			}
		}
	}

	//increment shields for rechargable shields
	if (p_veh->m_pVehicleInfo->shieldRechargeMS
		&& parent_ps->stats[STAT_ARMOR] > 0 //still have some shields left
		&& parent_ps->stats[STAT_ARMOR] < p_veh->m_pVehicleInfo->shields //its below max
		&& pUmcd->serverTime - p_veh->lastShieldInc >= p_veh->m_pVehicleInfo->shieldRechargeMS) //enough time has passed
	{
		parent_ps->stats[STAT_ARMOR]++;
		if (parent_ps->stats[STAT_ARMOR] > p_veh->m_pVehicleInfo->shields)
		{
			parent_ps->stats[STAT_ARMOR] = p_veh->m_pVehicleInfo->shields;
		}
		p_veh->m_iShields = parent_ps->stats[STAT_ARMOR];
		G_VehUpdateShields(parent);
	}

	if (parent && parent->r.ownerNum != parent->s.owner)
	{
		parent->s.owner = parent->r.ownerNum;
	}

	//keep the PS value in sync. set it up here in case we return below at some point.
	if (p_veh->m_iBoarding)
	{
		parent->client->ps.vehBoarding = qtrue;
	}
	else
	{
		parent->client->ps.vehBoarding = qfalse;
	}

	// See whether this vehicle should be dieing or dead.
	if (p_veh->m_iDieTime != 0)
	{
		//NOTE!!!: This HAS to be consistent with cgame!!!
		// Keep track of the old orientation.
		VectorCopy(p_veh->m_vOrientation, p_veh->m_vPrevOrientation);

		// Process the orient commands.
		p_veh->m_pVehicleInfo->ProcessOrientCommands(p_veh);
		// Need to copy orientation to our entity's viewangles so that it renders at the proper angle and currentAngles is correct.
		SetClientViewAngle(parent, p_veh->m_vOrientation);
		if (p_veh->m_pPilot)
		{
			SetClientViewAngle((gentity_t*)p_veh->m_pPilot, p_veh->m_vOrientation);
		}
		/*
		for ( i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++ )
		{
			if ( p_veh->m_ppPassengers[i] )
			{
				SetClientViewAngle( (gentity_t *)p_veh->m_ppPassengers[i], p_veh->m_vOrientation );
			}
		}
		*/

		// Process the move commands.
		p_veh->m_pVehicleInfo->ProcessMoveCommands(p_veh);

		// Setup the move direction.
		if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
		{
			AngleVectors(p_veh->m_vOrientation, parent->client->ps.moveDir, NULL, NULL);
		}
		else
		{
			VectorSet(vVehAngles, 0, p_veh->m_vOrientation[YAW], 0);
			AngleVectors(vVehAngles, parent->client->ps.moveDir, NULL, NULL);
		}
		p_veh->m_pVehicleInfo->DeathUpdate(p_veh);
		return qfalse;
	}
	// Vehicle dead!
	if (parent->health <= 0)
	{
		// Instant kill.
		if (p_veh->m_pVehicleInfo->type == VH_FIGHTER &&
			p_veh->m_iLastImpactDmg > 500)
		{
			//explode instantly in inferno-y death
			p_veh->m_pVehicleInfo->StartDeathDelay(p_veh, -1/* -1 causes instant death */);
		}
		else
		{
			p_veh->m_pVehicleInfo->StartDeathDelay(p_veh, 0);
		}
		p_veh->m_pVehicleInfo->DeathUpdate(p_veh);
		return qfalse;
	}

#ifdef _GAME
	if (parent->spawnflags & 1)
	{
		if (p_veh->m_pPilot || !p_veh->m_bHasHadPilot)
		{
			if (p_veh->m_pPilot && !p_veh->m_bHasHadPilot)
			{
				p_veh->m_bHasHadPilot = qtrue;
				p_veh->m_iPilotLastIndex = p_veh->m_pPilot->s.number;
			}
			p_veh->m_iPilotTime = level.time + parent->damage;
		}
		else if (p_veh->m_iPilotTime)
		{
			//die
			const gentity_t* oldPilot = &g_entities[p_veh->m_iPilotLastIndex];

			if (!oldPilot->inuse || !oldPilot->client ||
				oldPilot->client->pers.connected != CON_CONNECTED)
			{
				//no longer in the game?
				G_Damage(parent, parent, parent, NULL, parent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION,
					MOD_SUICIDE);
			}
			else
			{
				vec3_t v;
				VectorSubtract(parent->client->ps.origin, oldPilot->client->ps.origin, v);

				if (VectorLength(v) < parent->speed)
				{
					//they are still within the minimum distance to their vehicle
					p_veh->m_iPilotTime = level.time + parent->damage;
				}
				else if (p_veh->m_iPilotTime < level.time)
				{
					//dying time
					G_Damage(parent, parent, parent, NULL, parent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION,
						MOD_SUICIDE);
				}
			}
		}
	}
#endif

	if (p_veh->m_iBoarding != 0)
	{
		pilotEnt = (gentity_t*)p_veh->m_pPilot;
		if (pilotEnt)
		{
			if (!pilotEnt->inuse || !pilotEnt->client || pilotEnt->health <= 0 ||
				pilotEnt->client->pers.connected != CON_CONNECTED)
			{
				p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_pPilot, qtrue);
				return qfalse;
			}
		}
	}

	// If we're not done mounting, can't do anything.
	if (p_veh->m_iBoarding != 0)
	{
		if (!p_veh->m_bWasBoarding)
		{
			VectorCopy(parent_ps->velocity, p_veh->m_vBoardingVelocity);
			p_veh->m_bWasBoarding = qtrue;
		}

		// See if we're done boarding.
		if (p_veh->m_iBoarding > -1 && p_veh->m_iBoarding <= level.time)
		{
			p_veh->m_bWasBoarding = qfalse;
			p_veh->m_iBoarding = 0;
		}
		else
		{
			goto maintainSelfDuringBoarding;
		}
	}

	parent = (gentity_t*)p_veh->m_pParentEntity;

	// Validate vehicle.
	if (!parent || !parent->client || parent->health <= 0)
		return qfalse;

	// See if any of the riders are dead and if so kick em off.
	if (p_veh->m_pPilot)
	{
		pilotEnt = (gentity_t*)p_veh->m_pPilot;

		if (!pilotEnt->inuse || !pilotEnt->client || pilotEnt->health <= 0 ||
			pilotEnt->client->pers.connected != CON_CONNECTED)
		{
			p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_pPilot, qtrue);
		}
	}
	// If we're not empty...
	if (p_veh->m_iNumPassengers > 0)
	{
		// See if any of these suckers are dead.
		for (i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
		{
			const gentity_t* psngr = (gentity_t*)p_veh->m_ppPassengers[i];

			if (psngr &&
				(!psngr->inuse || !psngr->client || psngr->health <= 0 || psngr->client->pers.connected !=
					CON_CONNECTED))
			{
				p_veh->m_pVehicleInfo->Eject(p_veh, p_veh->m_ppPassengers[i], qtrue);
				p_veh->m_iNumPassengers--;
			}
		}
	}

	// Copy over the commands for local storage.
	memcpy(&parent->client->pers.cmd, &p_veh->m_ucmd, sizeof(usercmd_t));
	p_veh->m_ucmd.buttons &= ~BUTTON_TALK;

	//check for weapon linking/unlinking command
	for (i = 0; i < MAX_VEHICLE_WEAPONS; i++)
	{
		//HMM... can't get a seperate command for each weapon, so do them all...?
		if (p_veh->m_pVehicleInfo->weapon[i].linkable == 2)
		{
			//always linked
			//FIXME: just set this once, on Initialize...?
			if (!p_veh->weaponStatus[i].linked)
			{
				p_veh->weaponStatus[i].linked = qtrue;
			}
		}
		else if (p_veh->m_ucmd.buttons & BUTTON_USE_HOLDABLE)
		{
			//pilot pressed the "weapon link" toggle button
			if (!p_veh->linkWeaponToggleHeld) //so we don't hold it down and toggle it back and forth
			{
				//okay to toggle
				if (p_veh->m_pVehicleInfo->weapon[i].linkable == 1)
				{
					//link-toggleable
					p_veh->weaponStatus[i].linked = !p_veh->weaponStatus[i].linked;
				}
			}
			linkHeld = qtrue;
		}
	}
	if (linkHeld)
	{
		//so we don't hold it down and toggle it back and forth
		p_veh->linkWeaponToggleHeld = qtrue;
	}
	else
	{
		//so we don't hold it down and toggle it back and forth
		p_veh->linkWeaponToggleHeld = qfalse;
	}
	//now pass it over the network so cgame knows about it
	//NOTE: SP can just cheat and check directly
	parent_ps->vehWeaponsLinked = qfalse;
	for (i = 0; i < MAX_VEHICLE_WEAPONS; i++)
	{
		//HMM... can't get a seperate command for each weapon, so do them all...?
		if (p_veh->weaponStatus[i].linked)
		{
			parent_ps->vehWeaponsLinked = qtrue;
		}
	}

#ifdef _GAME
	for (i = 0; i < MAX_VEHICLE_TURRETS; i++)
	{
		//HMM... can't get a seperate command for each weapon, so do them all...?
		VEH_TurretThink(p_veh, parent, i);
	}
#endif

maintainSelfDuringBoarding:

	if (p_veh->m_pPilot && p_veh->m_pPilot->playerState && p_veh->m_iBoarding != 0)
	{
		VectorCopy(p_veh->m_vOrientation, p_veh->m_pPilot->playerState->viewangles);
		p_veh->m_ucmd.buttons = 0;
		p_veh->m_ucmd.forwardmove = 0;
		p_veh->m_ucmd.rightmove = 0;
		p_veh->m_ucmd.upmove = 0;
	}

	// Keep track of the old orientation.
	VectorCopy(p_veh->m_vOrientation, p_veh->m_vPrevOrientation);

	// Process the orient commands.
	p_veh->m_pVehicleInfo->ProcessOrientCommands(p_veh);
	// Need to copy orientation to our entity's viewangles so that it renders at the proper angle and currentAngles is correct.
	SetClientViewAngle(parent, p_veh->m_vOrientation);
	if (p_veh->m_pPilot)
	{
		if (!BG_UnrestrainedPitchRoll(p_veh->m_pPilot->playerState, p_veh))
		{
			vec3_t newVAngle;
			newVAngle[PITCH] = p_veh->m_pPilot->playerState->viewangles[PITCH];
			newVAngle[YAW] = p_veh->m_pPilot->playerState->viewangles[YAW];
			newVAngle[ROLL] = p_veh->m_vOrientation[ROLL];
			SetClientViewAngle((gentity_t*)p_veh->m_pPilot, newVAngle);
		}
	}
	/*
	for ( i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++ )
	{
		if ( p_veh->m_ppPassengers[i] )
		{
			SetClientViewAngle( (gentity_t *)p_veh->m_ppPassengers[i], p_veh->m_vOrientation );
		}
	}
	*/

	// Process the move commands.
	prevSpeed = parent_ps->speed;
	p_veh->m_pVehicleInfo->ProcessMoveCommands(p_veh);
	nextSpeed = parent_ps->speed;
	halfMaxSpeed = p_veh->m_pVehicleInfo->speedMax * 0.5f;

	// Shifting Sounds
	//=====================================================================
	if (p_veh->m_iTurboTime < curTime &&
		p_veh->m_iSoundDebounceTimer < curTime &&
		(nextSpeed > prevSpeed && nextSpeed > halfMaxSpeed && prevSpeed < halfMaxSpeed || nextSpeed > halfMaxSpeed && !
			Q_irand(0, 1000)))
	{
		int shiftSound = Q_irand(1, 4);
		switch (shiftSound)
		{
		case 1: shiftSound = p_veh->m_pVehicleInfo->soundShift1;
			break;
		case 2: shiftSound = p_veh->m_pVehicleInfo->soundShift2;
			break;
		case 3: shiftSound = p_veh->m_pVehicleInfo->soundShift3;
			break;
		case 4: shiftSound = p_veh->m_pVehicleInfo->soundShift4;
			break;
		default:;
		}
		if (shiftSound)
		{
			p_veh->m_iSoundDebounceTimer = curTime + Q_irand(1000, 4000);
			// TODO: MP Shift Sound Playback
		}
	}
	//=====================================================================

	// Setup the move direction.
	if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
	{
		AngleVectors(p_veh->m_vOrientation, parent->client->ps.moveDir, NULL, NULL);
	}
	else
	{
		VectorSet(vVehAngles, 0, p_veh->m_vOrientation[YAW], 0);
		AngleVectors(vVehAngles, parent->client->ps.moveDir, NULL, NULL);
	}

	if (p_veh->m_pVehicleInfo->surfDestruction)
	{
		if (p_veh->m_iRemovedSurfaces)
		{
			G_VehicleDamageBoxSizing(p_veh);

			// 3 seconds max on death.
			const float dmg = (float)parent->client->ps.stats[STAT_MAX_HEALTH] * p_veh->m_fTimeModifier / 180.0f;
			//FIXME: aside from bypassing shields, maybe set m_iShields to 0, too... ?
			G_DamageFromKiller(parent, parent, parent, parent->client->ps.origin, dmg,
				DAMAGE_NO_SELF_PROTECTION | DAMAGE_NO_HIT_LOC | DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR,
				MOD_SUICIDE);
		}

		//make sure playerstate value stays in sync
		parent->client->ps.vehSurfaces = p_veh->m_iRemovedSurfaces;
	}

	//keep the PS value in sync
	if (p_veh->m_iBoarding)
	{
		parent->client->ps.vehBoarding = qtrue;
	}
	else
	{
		parent->client->ps.vehBoarding = qfalse;
	}

	return qtrue;
}

// Update the properties of a Rider (that may reflect what happens to the vehicle).
static qboolean UpdateRider(Vehicle_t* p_veh, bgEntity_t* pRider, const usercmd_t* pUmcd)
{
	if (p_veh->m_iBoarding != 0 && p_veh->m_iDieTime == 0)
		return qtrue;

	const gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
	gentity_t* rider = (gentity_t*)pRider;
	//MG FIXME !! Single player needs update!
	if (rider && rider->client
		&& parent && parent->client)
	{
		//so they know who we're locking onto with our rockets, if anyone
		rider->client->ps.rocketLockIndex = parent->client->ps.rocketLockIndex;
		rider->client->ps.rocketLockTime = parent->client->ps.rocketLockTime;
		rider->client->ps.rocketTargetTime = parent->client->ps.rocketTargetTime;
	}
	// Regular exit.
	if (pUmcd->buttons & BUTTON_USE && p_veh->m_pVehicleInfo->type != VH_SPEEDER)
	{
		if (p_veh->m_pVehicleInfo->type == VH_WALKER)
		{
			//just get the fuck out
			p_veh->m_EjectDir = VEH_EJECT_REAR;
			if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
				return qfalse;
		}
		else if (!(p_veh->m_ulFlags & VEH_FLYING))
		{
			// If going too fast, roll off.
			if (parent->client->ps.speed <= 600 && pUmcd->rightmove != 0)
			{
				if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
				{
					animNumber_t Anim;
					const int iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, i_blend = 300;
					if (pUmcd->rightmove > 0)
					{
						Anim = BOTH_ROLL_R;
						p_veh->m_EjectDir = VEH_EJECT_RIGHT;
					}
					else
					{
						Anim = BOTH_ROLL_L;
						p_veh->m_EjectDir = VEH_EJECT_LEFT;
					}
					VectorScale(parent->client->ps.velocity, 0.25f, rider->client->ps.velocity);
					Vehicle_SetAnim(rider, SETANIM_BOTH, Anim, iFlags, i_blend);

					rider->client->ps.weaponTime = rider->client->ps.torsoTimer - 200;
					//just to make sure it's cleared when roll is done
					G_AddEvent(rider, EV_ROLL, 0);
					return qfalse;
				}
			}
			else
			{
				// FIXME: Check trace to see if we should start playing the animation.
				animNumber_t Anim;
				const int iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, i_blend = 500;
				if (pUmcd->rightmove > 0)
				{
					Anim = BOTH_VS_DISMOUNT_R;
					p_veh->m_EjectDir = VEH_EJECT_RIGHT;
				}
				else
				{
					Anim = BOTH_VS_DISMOUNT_L;
					p_veh->m_EjectDir = VEH_EJECT_LEFT;
				}

				if (p_veh->m_iBoarding <= 1)
				{
					// NOTE: I know I shouldn't reuse p_veh->m_iBoarding so many times for so many different
					// purposes, but it's not used anywhere else right here so why waste memory???
					const int iAnimLen = BG_AnimLength(rider->localAnimIndex, Anim);
					p_veh->m_iBoarding = level.time + iAnimLen;
					// Weird huh? Well I wanted to reuse flags and this should never be set in an
					// entity, so what the heck.
					rider->flags |= FL_VEH_BOARDING;

					// Make sure they can't fire when leaving.
					rider->client->ps.weaponTime = iAnimLen;
				}

				VectorScale(parent->client->ps.velocity, 0.25f, rider->client->ps.velocity);

				Vehicle_SetAnim(rider, SETANIM_BOTH, Anim, iFlags, i_blend);
			}
		}
		// Flying, so just fall off.
		else
		{
			p_veh->m_EjectDir = VEH_EJECT_LEFT;
			if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
				return qfalse;
		}
	}

	// Getting off animation complete (if we had one going)?
	if (p_veh->m_iBoarding < level.time && rider->flags & FL_VEH_BOARDING)
	{
		rider->flags &= ~FL_VEH_BOARDING;
		// Eject this guy now.
		if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
		{
			return qfalse;
		}
	}

	if (p_veh->m_pVehicleInfo->type != VH_FIGHTER
		&& p_veh->m_pVehicleInfo->type != VH_WALKER)
	{
		// Jump off.
		if (pUmcd->upmove > 0)
		{
			if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
			{
				// Allow them to force jump off.
				VectorScale(parent->client->ps.velocity, 0.5f, rider->client->ps.velocity);
				rider->client->ps.velocity[2] += JUMP_VELOCITY;
				rider->client->ps.fd.forceJumpZStart = rider->client->ps.origin[2];

				if (!trap->ICARUS_TaskIDPending((sharedEntity_t*)rider, TID_CHAN_VOICE))
				{
					G_AddEvent(rider, EV_JUMP, 0);
				}
				Vehicle_SetAnim(rider, SETANIM_BOTH, BOTH_JUMP1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 300);
				return qfalse;
			}
		}

		// Roll off.
		if (pUmcd->upmove < 0)
		{
			animNumber_t Anim = BOTH_ROLL_B;
			p_veh->m_EjectDir = VEH_EJECT_REAR;
			if (pUmcd->rightmove > 0)
			{
				Anim = BOTH_ROLL_R;
				p_veh->m_EjectDir = VEH_EJECT_RIGHT;
			}
			else if (pUmcd->rightmove < 0)
			{
				Anim = BOTH_ROLL_L;
				p_veh->m_EjectDir = VEH_EJECT_LEFT;
			}
			else if (pUmcd->forwardmove < 0)
			{
				Anim = BOTH_ROLL_B;
				p_veh->m_EjectDir = VEH_EJECT_REAR;
			}
			else if (pUmcd->forwardmove > 0)
			{
				Anim = BOTH_ROLL_F;
				p_veh->m_EjectDir = VEH_EJECT_FRONT;
			}

			if (p_veh->m_pVehicleInfo->Eject(p_veh, pRider, qfalse))
			{
				if (!(p_veh->m_ulFlags & VEH_FLYING))
				{
					VectorScale(parent->client->ps.velocity, 0.25f, rider->client->ps.velocity);
					Vehicle_SetAnim(rider, SETANIM_BOTH, Anim,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, 300);
					rider->client->ps.weaponTime = rider->client->ps.torsoTimer - 200;
					//just to make sure it's cleared when roll is done
					G_AddEvent(rider, EV_ROLL, 0);
				}
				return qfalse;
			}
		}
	}

	return qtrue;
}

//generic vehicle function we care about over there
extern void AttachRidersGeneric(const Vehicle_t* p_veh);

// Attachs all the riders of this vehicle to their appropriate tag (*driver, *pass1, *pass2, whatever...).
static void AttachRiders(const Vehicle_t* p_veh)
{
	int i = 0;

	AttachRidersGeneric(p_veh);

	if (p_veh->m_pPilot)
	{
		const gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
		gentity_t* pilot = (gentity_t*)p_veh->m_pPilot;
		pilot->waypoint = parent->waypoint; // take the veh's waypoint as your own

		//assuming we updated him relative to the bolt in AttachRidersGeneric
		G_SetOrigin(pilot, pilot->client->ps.origin);
		trap->LinkEntity((sharedEntity_t*)pilot);
	}

	if (p_veh->m_pOldPilot)
	{
		const gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
		gentity_t* oldpilot = (gentity_t*)p_veh->m_pOldPilot;
		oldpilot->waypoint = parent->waypoint; // take the veh's waypoint as your own

		//assuming we updated him relative to the bolt in AttachRidersGeneric
		G_SetOrigin(oldpilot, oldpilot->client->ps.origin);
		trap->LinkEntity((sharedEntity_t*)oldpilot);
	}

	//attach passengers
	while (i < p_veh->m_iNumPassengers)
	{
		if (p_veh->m_ppPassengers[i])
		{
			mdxaBone_t boltMatrix;
			vec3_t yawOnlyAngles;
			gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
			gentity_t* pilot = (gentity_t*)p_veh->m_ppPassengers[i];

			assert(parent->ghoul2);
			const int crotchBolt = trap->G2API_AddBolt(parent->ghoul2, 0, "*driver");
			assert(parent->client);
			assert(pilot->client);

			VectorSet(yawOnlyAngles, 0, parent->client->ps.viewangles[YAW], 0);

			// Get the driver tag.
			trap->G2API_GetBoltMatrix(parent->ghoul2, 0, crotchBolt, &boltMatrix,
				yawOnlyAngles, parent->client->ps.origin,
				level.time, NULL, parent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, pilot->client->ps.origin);

			G_SetOrigin(pilot, pilot->client->ps.origin);
			trap->LinkEntity((sharedEntity_t*)pilot);
		}
		i++;
	}

	//attach droid
	if (p_veh->m_pDroidUnit
		&& p_veh->m_iDroidUnitTag != -1)
	{
		mdxaBone_t boltMatrix;
		gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;
		gentity_t* droid = (gentity_t*)p_veh->m_pDroidUnit;

		assert(parent->ghoul2);
		assert(parent->client);
		//assert(droid->client);

		if (droid->client)
		{
			vec3_t fwd;
			vec3_t yaw_only_angles;
			VectorSet(yaw_only_angles, 0, parent->client->ps.viewangles[YAW], 0);

			// Get the droid tag.
			trap->G2API_GetBoltMatrix(parent->ghoul2, 0, p_veh->m_iDroidUnitTag, &boltMatrix,
				yaw_only_angles, parent->r.currentOrigin,
				level.time, NULL, parent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droid->client->ps.origin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fwd);
			vectoangles(fwd, droid->client->ps.viewangles);

			G_SetOrigin(droid, droid->client->ps.origin);
			G_SetAngles(droid, droid->client->ps.viewangles);
			SetClientViewAngle(droid, droid->client->ps.viewangles);
			trap->LinkEntity((sharedEntity_t*)droid);

			if (droid->NPC)
			{
				NPC_SetAnim(droid, SETANIM_BOTH, BOTH_STAND2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				droid->client->ps.legsTimer = 500;
				droid->client->ps.torsoTimer = 500;
			}
		}
	}
}

// Make someone invisible and un-collidable.
static void Ghost(Vehicle_t* p_veh, bgEntity_t* pEnt)
{
	if (!pEnt)
		return;

	gentity_t* ent = (gentity_t*)pEnt;

	// This was introduced to prevent one extra entity from being sent to the clients
	ent->r.svFlags |= SVF_NOCLIENT;

	ent->s.eFlags |= EF_NODRAW;
	if (ent->client)
	{
		ent->client->ps.eFlags |= EF_NODRAW;
	}
	ent->r.contents = 0;
}

// Make someone visible and collidable.
static void UnGhost(Vehicle_t* p_veh, bgEntity_t* pEnt)
{
	if (!pEnt)
		return;

	gentity_t* ent = (gentity_t*)pEnt;

	// make sure the client is sent again
	ent->r.svFlags &= ~SVF_NOCLIENT;

	ent->s.eFlags &= ~EF_NODRAW;
	if (ent->client)
	{
		ent->client->ps.eFlags &= ~EF_NODRAW;
	}
	ent->r.contents = CONTENTS_BODY;
}

//try to resize the bounding box around a torn apart ship
void G_VehicleDamageBoxSizing(const Vehicle_t* p_veh)
{
	vec3_t fwd, right, up;
	vec3_t nose; //maxs
	vec3_t back; //mins
	trace_t trace;
	const float fDist = 256.0f; //estimated distance to nose from origin
	const float bDist = 256.0f; //estimated distance to back from origin
	const float wDist = 32.0f; //width on each side from origin
	const float hDist = 32.0f; //height on each side from origin
	gentity_t* parent = (gentity_t*)p_veh->m_pParentEntity;

	if (!parent->ghoul2 || !parent->m_pVehicle || !parent->client)
	{
		//shouldn't have gotten in here then
		return;
	}

	//for now, let's only do anything if all wings are stripped off.
	//this is because I want to be able to tear my wings off and fling
	//myself down narrow hallways to my death. Because it's fun! -rww
	if (!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) ||
		!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D) ||
		!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) ||
		!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
	{
		return;
	}

	//get directions based on orientation
	AngleVectors(p_veh->m_vOrientation, fwd, right, up);

	//get the nose and back positions (relative to 0, they're gonna be mins/maxs)
	VectorMA(vec3_origin, fDist, fwd, nose);
	VectorMA(vec3_origin, -bDist, fwd, back);

	//move the nose and back to opposite right/left, they will end up as our relative mins and maxs
	VectorMA(nose, wDist, right, nose);
	VectorMA(nose, -wDist, right, back);

	//use the same concept for up/down now
	VectorMA(nose, hDist, up, nose);
	VectorMA(nose, -hDist, up, back);

	//and now, let's trace and see if our new mins/maxs are safe..
	trap->Trace(&trace, parent->client->ps.origin, back, nose, parent->client->ps.origin, parent->s.number,
		parent->clipmask, qfalse, 0, 0);
	if (!trace.allsolid && !trace.startsolid && trace.fraction == 1.0f)
	{
		//all clear!
		VectorCopy(nose, parent->r.maxs);
		VectorCopy(back, parent->r.mins);
	}
	else
	{
		//oh well, DIE!
		//FIXME: does this give proper credit to the enemy who shot you down?
		G_Damage(parent, parent, parent, NULL, parent->client->ps.origin, 9999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
	}
}

//get one of 4 possible impact locations based on the trace direction
static int G_FlyVehicleImpactDir(const gentity_t* veh, const trace_t* trace)
{
	trace_t localTrace;
	vec3_t testMins, testMaxs;
	vec3_t fwd, right;
	vec3_t fPos;
	const Vehicle_t* p_veh = veh->m_pVehicle;
	qboolean noseClear = qfalse;

	if (!trace || !p_veh || !veh->client)
	{
		return -1;
	}

	AngleVectors(veh->client->ps.viewangles, fwd, right, 0);
	VectorSet(testMins, -24.0f, -24.0f, -24.0f);
	VectorSet(testMaxs, 24.0f, 24.0f, 24.0f);

	//do a trace to determine if the nose is clear
	VectorMA(veh->client->ps.origin, 256.0f, fwd, fPos);
	trap->Trace(&localTrace, veh->client->ps.origin, testMins, testMaxs, fPos, veh->s.number, veh->clipmask, qfalse, 0,
		0);
	if (!localTrace.startsolid && !localTrace.allsolid && localTrace.fraction == 1.0f)
	{
		//otherwise I guess it's not clear..
		noseClear = qtrue;
	}

	if (noseClear)
	{
		vec3_t lWing;
		vec3_t rWing;
		//if nose is clear check for tearing the wings off
		//sadly, the trace endpos given always matches the vehicle origin, so we
		//can't get a real impact direction. First we'll trace forward and see if the wings are colliding
		//with anything, and if not, we'll fall back to checking the trace plane normal.
		VectorMA(veh->client->ps.origin, 128.0f, right, rWing);
		VectorMA(veh->client->ps.origin, -128.0f, right, lWing);

		//test the right wing - unless it's already removed
		if (!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) ||
			!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
		{
			VectorMA(rWing, 256.0f, fwd, fPos);
			trap->Trace(&localTrace, rWing, testMins, testMaxs, fPos, veh->s.number, veh->clipmask, qfalse, 0, 0);
			if (localTrace.startsolid || localTrace.allsolid || localTrace.fraction != 1.0f)
			{
				//impact
				return SHIPSURF_RIGHT;
			}
		}

		//test the left wing - unless it's already removed
		if (!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) ||
			!(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D))
		{
			VectorMA(lWing, 256.0f, fwd, fPos);
			trap->Trace(&localTrace, lWing, testMins, testMaxs, fPos, veh->s.number, veh->clipmask, qfalse, 0, 0);
			if (localTrace.startsolid || localTrace.allsolid || localTrace.fraction != 1.0f)
			{
				//impact
				return SHIPSURF_LEFT;
			}
		}
	}

	//try to use the trace plane normal
	const float impactAngle = vectoyaw(trace->plane.normal);
	const float relativeAngle = AngleSubtract(impactAngle, veh->client->ps.viewangles[YAW]);

	if (relativeAngle > 130 ||
		relativeAngle < -130)
	{
		//consider this front
		return SHIPSURF_FRONT;
	}
	if (relativeAngle > 0)
	{
		return SHIPSURF_RIGHT;
	}
	if (relativeAngle < 0)
	{
		return SHIPSURF_LEFT;
	}

	return SHIPSURF_BACK;
}

//try to break surfaces off the ship on impact
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100
extern void NPC_SetSurfaceOnOff(gentity_t* ent, const char* surfaceName, int surfaceFlags); //NPC_utils.c
int G_ShipSurfaceForSurfName(const char* surfaceName)
{
	if (!surfaceName)
	{
		return -1;
	}
	if (!Q_strncmp("nose", surfaceName, 4)
		|| !Q_strncmp("f_gear", surfaceName, 6)
		|| !Q_strncmp("glass", surfaceName, 5))
	{
		return SHIPSURF_FRONT;
	}
	if (!Q_strncmp("body", surfaceName, 4))
	{
		return SHIPSURF_BACK;
	}
	if (!Q_strncmp("r_wing1", surfaceName, 7)
		|| !Q_strncmp("r_wing2", surfaceName, 7)
		|| !Q_strncmp("r_gear", surfaceName, 6))
	{
		return SHIPSURF_RIGHT;
	}
	if (!Q_strncmp("l_wing1", surfaceName, 7)
		|| !Q_strncmp("l_wing2", surfaceName, 7)
		|| !Q_strncmp("l_gear", surfaceName, 6))
	{
		return SHIPSURF_LEFT;
	}
	return -1;
}

void G_SetVehDamageFlags(gentity_t* veh, const int shipSurf, const int damageLevel)
{
	int dmgFlag;
	switch (damageLevel)
	{
	case 3: //destroyed
		//add both flags so cgame side knows this surf is GONE
		//add heavy
		dmgFlag = SHIPSURF_DAMAGE_FRONT_HEAVY + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs |= 1 << dmgFlag;
		//add light
		dmgFlag = SHIPSURF_DAMAGE_FRONT_LIGHT + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs |= 1 << dmgFlag;
		//copy down
		veh->s.brokenLimbs = veh->client->ps.brokenLimbs;
		//check droid
		if (shipSurf == SHIPSURF_BACK)
		{
			//destroy the droid if we have one
			if (veh->m_pVehicle
				&& veh->m_pVehicle->m_pDroidUnit)
			{
				//we have one
				gentity_t* droidEnt = (gentity_t*)veh->m_pVehicle->m_pDroidUnit;
				if (droidEnt
					&& (droidEnt->flags & FL_UNDYING || droidEnt->health > 0))
				{
					//boom
					//make it vulnerable
					droidEnt->flags &= ~FL_UNDYING;
					//blow it up
					G_Damage(droidEnt, veh->enemy, veh->enemy, NULL, NULL, 99999, 0, MOD_UNKNOWN);
				}
			}
		}
		break;
	case 2: //heavy only
		dmgFlag = SHIPSURF_DAMAGE_FRONT_HEAVY + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs |= 1 << dmgFlag;
		//remove light
		dmgFlag = SHIPSURF_DAMAGE_FRONT_LIGHT + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs &= ~(1 << dmgFlag);
		//copy down
		veh->s.brokenLimbs = veh->client->ps.brokenLimbs;
		//check droid
		if (shipSurf == SHIPSURF_BACK)
		{
			//make the droid vulnerable if we have one
			if (veh->m_pVehicle
				&& veh->m_pVehicle->m_pDroidUnit)
			{
				//we have one
				gentity_t* droidEnt = (gentity_t*)veh->m_pVehicle->m_pDroidUnit;
				if (droidEnt
					&& droidEnt->flags & FL_UNDYING)
				{
					//make it vulnerab;e
					droidEnt->flags &= ~FL_UNDYING;
				}
			}
		}
		break;
	case 1: //light only
		//add light
		dmgFlag = SHIPSURF_DAMAGE_FRONT_LIGHT + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs |= 1 << dmgFlag;
		//remove heavy (shouldn't have to do this, but...
		dmgFlag = SHIPSURF_DAMAGE_FRONT_HEAVY + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs &= ~(1 << dmgFlag);
		//copy down
		veh->s.brokenLimbs = veh->client->ps.brokenLimbs;
		break;
	case 0: //no damage
	default:
		//remove heavy
		dmgFlag = SHIPSURF_DAMAGE_FRONT_HEAVY + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs &= ~(1 << dmgFlag);
		//remove light
		dmgFlag = SHIPSURF_DAMAGE_FRONT_LIGHT + (shipSurf - SHIPSURF_FRONT);
		veh->client->ps.brokenLimbs &= ~(1 << dmgFlag);
		//copy down
		veh->s.brokenLimbs = veh->client->ps.brokenLimbs;
		break;
	}
}

void G_VehicleSetDamageLocFlags(gentity_t* veh, const int impactDir, int deathPoint)
{
	if (!veh->client)
	{
		return;
	}
	int heavyDamagePoint, lightDamagePoint;
	switch (impactDir)
	{
	case SHIPSURF_FRONT:
		deathPoint = veh->m_pVehicle->m_pVehicleInfo->health_front;
		break;
	case SHIPSURF_BACK:
		deathPoint = veh->m_pVehicle->m_pVehicleInfo->health_back;
		break;
	case SHIPSURF_RIGHT:
		deathPoint = veh->m_pVehicle->m_pVehicleInfo->health_right;
		break;
	case SHIPSURF_LEFT:
		deathPoint = veh->m_pVehicle->m_pVehicleInfo->health_left;
		break;
	default:
		return;
	}
	if (veh->m_pVehicle
		&& veh->m_pVehicle->m_pVehicleInfo
		&& veh->m_pVehicle->m_pVehicleInfo->malfunctionArmorLevel
		&& veh->m_pVehicle->m_pVehicleInfo->armor)
	{
		float perc = (float)veh->m_pVehicle->m_pVehicleInfo->malfunctionArmorLevel / (float)veh->m_pVehicle->
			m_pVehicleInfo->armor;
		if (perc > 0.99f)
		{
			perc = 0.99f;
		}
		lightDamagePoint = ceil(deathPoint * perc * 0.25f);
		heavyDamagePoint = ceil(deathPoint * perc);
	}
	else
	{
		heavyDamagePoint = ceil(deathPoint * 0.66f);
		lightDamagePoint = ceil(deathPoint * 0.14f);
	}

	if (veh->locationDamage[impactDir] >= deathPoint)
	{
		//destroyed
		G_SetVehDamageFlags(veh, impactDir, 3);
	}
	else if (veh->locationDamage[impactDir] <= lightDamagePoint)
	{
		//light only
		G_SetVehDamageFlags(veh, impactDir, 1);
	}
	else if (veh->locationDamage[impactDir] <= heavyDamagePoint)
	{
		//heavy only
		G_SetVehDamageFlags(veh, impactDir, 2);
	}
}

qboolean G_FlyVehicleDestroySurface(gentity_t* veh, const int surface)
{
	char* surfName[4]; //up to 4 surfs at once
	int numSurfs = 0;
	int smashedBits = 0;

	if (surface == -1)
	{
		//not valid?
		return qfalse;
	}

	switch (surface)
	{
	case SHIPSURF_FRONT: //break the nose off
		surfName[0] = "nose";

		smashedBits = SHIPSURF_BROKEN_G;

		numSurfs = 1;
		break;
	case SHIPSURF_BACK: //break both the bottom wings off for a backward impact I guess
		surfName[0] = "r_wing2";
		surfName[1] = "l_wing2";

		//get rid of the landing gear
		surfName[2] = "r_gear";
		surfName[3] = "l_gear";

		smashedBits = SHIPSURF_BROKEN_A | SHIPSURF_BROKEN_B | SHIPSURF_BROKEN_D | SHIPSURF_BROKEN_F;

		numSurfs = 4;
		break;
	case SHIPSURF_RIGHT: //break both right wings off
		surfName[0] = "r_wing1";
		surfName[1] = "r_wing2";

		//get rid of the landing gear
		surfName[2] = "r_gear";

		smashedBits = SHIPSURF_BROKEN_B | SHIPSURF_BROKEN_E | SHIPSURF_BROKEN_F;

		numSurfs = 3;
		break;
	case SHIPSURF_LEFT: //break both left wings off
		surfName[0] = "l_wing1";
		surfName[1] = "l_wing2";

		//get rid of the landing gear
		surfName[2] = "l_gear";

		smashedBits = SHIPSURF_BROKEN_A | SHIPSURF_BROKEN_C | SHIPSURF_BROKEN_D;

		numSurfs = 3;
		break;
	default:
		break;
	}

	if (numSurfs < 1)
	{
		//didn't get any valid surfs..
		return qfalse;
	}

	while (numSurfs > 0)
	{
		//use my silly system of automatically managing surf status on both client and server
		numSurfs--;
		NPC_SetSurfaceOnOff(veh, surfName[numSurfs], TURN_OFF);
	}

	if (!veh->m_pVehicle->m_iRemovedSurfaces)
	{
		//first time something got blown off
		if (veh->m_pVehicle->m_pPilot)
		{
			//make the pilot scream to his death
			G_EntitySound((gentity_t*)veh->m_pVehicle->m_pPilot, CHAN_VOICE, G_SoundIndex("*falling1.wav"));
		}
	}
	//so we can check what's broken
	veh->m_pVehicle->m_iRemovedSurfaces |= smashedBits;

	//do some explosive damage, but don't damage this ship with it
	g_radius_damage(veh->client->ps.origin, veh, 100, 500, veh, NULL, MOD_VEH_EXPLOSION);

	//when spiraling to your death, do the electical shader
	veh->client->ps.electrifyTime = level.time + 10000;

	return qtrue;
}

void G_FlyVehicleSurfaceDestruction(gentity_t* veh, const trace_t* trace, const int magnitude, const qboolean force)
{
	int death_point = -1;
	qboolean already_rebroken = qfalse;

	if (!veh->ghoul2 || !veh->m_pVehicle)
	{
		//no g2 instance.. or no vehicle instance
		return;
	}

	int impact_dir = G_FlyVehicleImpactDir(veh, trace);

anotherImpact:
	if (impact_dir == -1)
	{
		//not valid?
		return;
	}

	veh->locationDamage[impact_dir] += magnitude * 7;

	switch (impact_dir)
	{
	case SHIPSURF_FRONT:
		death_point = veh->m_pVehicle->m_pVehicleInfo->health_front;
		break;
	case SHIPSURF_BACK:
		death_point = veh->m_pVehicle->m_pVehicleInfo->health_back;
		break;
	case SHIPSURF_RIGHT:
		death_point = veh->m_pVehicle->m_pVehicleInfo->health_right;
		break;
	case SHIPSURF_LEFT:
		death_point = veh->m_pVehicle->m_pVehicleInfo->health_left;
		break;
	default:
		break;
	}

	if (death_point != -1)
	{
		//got a valid health value
		if (force && veh->locationDamage[impact_dir] < death_point)
		{
			//force that surf to be destroyed
			veh->locationDamage[impact_dir] = death_point;
		}
		if (veh->locationDamage[impact_dir] >= death_point)
		{
			//do it
			if (G_FlyVehicleDestroySurface(veh, impact_dir))
			{
				//actually took off a surface
				G_VehicleSetDamageLocFlags(veh, impact_dir, death_point);
			}
		}
		else
		{
			G_VehicleSetDamageLocFlags(veh, impact_dir, death_point);
		}
	}

	if (!already_rebroken)
	{
		const int second_impact = G_FlyVehicleImpactDir(veh, trace);
		if (impact_dir != second_impact)
		{
			//can break off another piece in this same impact.. but only break off up to 2 at once
			already_rebroken = qtrue;
			impact_dir = second_impact;
			goto anotherImpact;
		}
	}
}

void G_VehUpdateShields(const gentity_t* targ)
{
	if (!targ || !targ->client
		|| !targ->m_pVehicle || !targ->m_pVehicle->m_pVehicleInfo)
	{
		return;
	}
	if (targ->m_pVehicle->m_pVehicleInfo->shields <= 0)
	{
		//doesn't have shields, so don't have to send it
		return;
	}
	targ->client->ps.activeForcePass = floor(
		(float)targ->m_pVehicle->m_iShields / (float)targ->m_pVehicle->m_pVehicleInfo->shields * 10.0f);
}

// Set the parent entity of this Vehicle NPC.
static void _SetParent(Vehicle_t* p_veh, bgEntity_t* pParentEntity) { p_veh->m_pParentEntity = pParentEntity; }

// Add a pilot to the vehicle.
static void SetPilot(Vehicle_t* p_veh, bgEntity_t* pPilot) { p_veh->m_pPilot = pPilot; }

// Add a passenger to the vehicle (false if we're full).
static qboolean AddPassenger(Vehicle_t* p_veh) { return qfalse; }

// Whether this vehicle is currently inhabited (by anyone) or not.
static qboolean Inhabited(const Vehicle_t* p_veh) { return p_veh->m_pPilot || p_veh->m_iNumPassengers ? qtrue : qfalse; }

// Setup the shared functions (one's that all vehicles would generally use).
void G_SetSharedVehicleFunctions(vehicleInfo_t* pVehInfo)
{
	//	pVehInfo->AnimateVehicle				=		AnimateVehicle;
	//	pVehInfo->AnimateRiders					=		AnimateRiders;
	pVehInfo->ValidateBoard = ValidateBoard;
	pVehInfo->SetParent = _SetParent;
	pVehInfo->SetPilot = SetPilot;
	pVehInfo->AddPassenger = AddPassenger;
	pVehInfo->Animate = Animate;
	pVehInfo->Board = Board;
	pVehInfo->Eject = Eject;
	pVehInfo->EjectAll = EjectAll;
	pVehInfo->StartDeathDelay = StartDeathDelay;
	pVehInfo->DeathUpdate = DeathUpdate;
	pVehInfo->RegisterAssets = RegisterAssets;
	pVehInfo->Initialize = Initialize;
	pVehInfo->Update = Update;
	pVehInfo->UpdateRider = UpdateRider;
	//	pVehInfo->ProcessMoveCommands			=		ProcessMoveCommands;
	//	pVehInfo->ProcessOrientCommands			=		ProcessOrientCommands;
	pVehInfo->AttachRiders = AttachRiders;
	pVehInfo->Ghost = Ghost;
	pVehInfo->UnGhost = UnGhost;
	pVehInfo->Inhabited = Inhabited;
}