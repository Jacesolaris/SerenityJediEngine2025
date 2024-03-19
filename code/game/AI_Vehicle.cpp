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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK III
//  (c) 2002 Activision
//
// April 3, 2003 - This file has been commandeered for use by AI vehicle pilots.
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#include "anims.h"
#include "g_navigator.h"
#include "g_vehicles.h"
#include "g_functions.h"
#if !defined(RATL_VECTOR_VS_INC)
#include "../Ratl/vector_vs.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
constexpr auto MAX_VEHICLES_REGISTERED = 100;

constexpr auto ATTACK_FWD = 0.95f;
constexpr auto ATTACK_SIDE = 0.20f;
constexpr auto AIM_SIDE = 0.60f;
constexpr auto FUTURE_PRED_DIST = 20.0f;
constexpr auto FUTURE_SIDE_DIST = 60.0f;
constexpr auto ATTACK_FLANK_SLOWING = 1000.0f;
constexpr auto RAM_DIST = 150.0f;
constexpr auto MIN_STAY_VIEWABLE_TIME = 20000;

////////////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////////////
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern void G_SoundAtSpot(vec3_t org, int sound_index, qboolean broadcast);
extern void CG_DrawEdge(vec3_t start, vec3_t end, int type);

trace_t mPilotViewTrace;
int mPilotViewTraceCount;
int mActivePilotCount;
ratl::vector_vs<gentity_t*, MAX_VEHICLES_REGISTERED> mRegistered;

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void Pilot_Reset()
{
	mPilotViewTraceCount = 0;
	mActivePilotCount = 0;
	mRegistered.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
int Pilot_ActivePilotCount()
{
	return mActivePilotCount;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void Pilot_Update()
{
	mActivePilotCount = 0;
	mRegistered.clear();
	for (int i = 0; i < ENTITYNUM_WORLD; i++)
	{
		if (g_entities[i].inuse &&
			g_entities[i].client &&
			g_entities[i].NPC &&
			g_entities[i].NPC->greetEnt &&
			g_entities[i].NPC->greetEnt->owner == &g_entities[i]
			)
		{
			mActivePilotCount++;
		}
		if (g_entities[i].inuse &&
			g_entities[i].client &&
			g_entities[i].m_pVehicle &&
			!g_entities[i].owner &&
			g_entities[i].health > 0 &&
			g_entities[i].m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER &&
			!mRegistered.full())
		{
			mRegistered.push_back(&g_entities[i]);
		}
	}

	if (player &&
		player->inuse &&
		TIMER_Done(player, "FlybySoundArchitectureDebounce"))
	{
		TIMER_Set(player, "FlybySoundArchitectureDebounce", 300);

		const Vehicle_t* p_veh = G_IsRidingVehicle(player);

		if (p_veh &&
			(p_veh->m_pVehicleInfo->soundFlyBy || p_veh->m_pVehicleInfo->soundFlyBy2) &&
			//fabsf(p_veh->m_pParentEntity->currentAngles[2])<15.0f &&
			VectorLength(p_veh->m_pParentEntity->client->ps.velocity) > 500.0f)
		{
			vec3_t projectedPosition;
			vec3_t projectedDirection;
			vec3_t projectedRight;
			vec3_t anglesNoRoll;

			VectorCopy(p_veh->m_pParentEntity->currentAngles, anglesNoRoll);
			anglesNoRoll[2] = 0;
			AngleVectors(anglesNoRoll, projectedDirection, projectedRight, nullptr);

			VectorMA(player->currentOrigin, 1.2f, p_veh->m_pParentEntity->client->ps.velocity, projectedPosition);
			VectorMA(projectedPosition, Q_flrand(-200.0f, 200.0f), projectedRight, projectedPosition);

			gi.trace(&mPilotViewTrace,
				player->currentOrigin,
				nullptr,
				nullptr,
				projectedPosition,
				player->s.number,
				MASK_SHOT, static_cast<EG2_Collision>(0), 0);

			if (mPilotViewTrace.allsolid == qfalse &&
				mPilotViewTrace.startsolid == qfalse &&
				mPilotViewTrace.fraction < 0.99f &&
				mPilotViewTrace.plane.normal[2] < 0.5f &&
				DotProduct(projectedDirection, mPilotViewTrace.plane.normal) < -0.5f)
			{
				TIMER_Set(player, "FlybySoundArchitectureDebounce", Q_irand(1000, 2000));

				int soundFlyBy = p_veh->m_pVehicleInfo->soundFlyBy;
				if (p_veh->m_pVehicleInfo->soundFlyBy2 && (!soundFlyBy || !Q_irand(0, 1)))
				{
					soundFlyBy = p_veh->m_pVehicleInfo->soundFlyBy2;
				}
				G_SoundAtSpot(mPilotViewTrace.endpos, soundFlyBy, qtrue);
			}
			else
			{
				//	CG_DrawEdge(player->currentOrigin, mPilotViewTrace.endpos, EDGE_IMPACT_SAFE);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool Pilot_AnyVehiclesRegistered()
{
	return !mRegistered.empty();
}

////////////////////////////////////////////////////////////////////////////////////////
// Vehicle Registration
//
// Any vehicles that can be ridden by NPCs should be registered here
//
////////////////////////////////////////////////////////////////////////////////////////
void Vehicle_Register(gentity_t* ent)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Vehicle Remove From The List Of Valid
////////////////////////////////////////////////////////////////////////////////////////
void Vehicle_Remove(gentity_t* ent)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// Vehicle_Find
//
// Will look through all registered vehicles and choose the closest one that the given
// entity can get to.
//
////////////////////////////////////////////////////////////////////////////////////////
static gentity_t* Vehicle_Find(gentity_t* ent)
{
	gentity_t* closest = nullptr;
	float closestDist = 0;

	for (int i = 0; i < mRegistered.size(); i++)
	{
		if (!mRegistered[i]->owner)
		{
			const float curDist = Distance(mRegistered[i]->currentOrigin, ent->currentOrigin);
			if (curDist < 1000 && (!closest || curDist < closestDist))
			{
				if (NAV::InSameRegion(ent, mRegistered[i]))
				{
					closest = mRegistered[i];
					closestDist = curDist;
				}
			}
		}
	}

	return closest;
}

void Pilot_Update_Enemy();
void Pilot_Steer_Vehicle();
void Pilot_Goto_Vehicle();

////////////////////////////////////////////////////////////////////////////////////////
// Pilot_MasterUpdate() - Master think function for Pilot NPCs
//
// Will return true if the character is either driving a vehicle or on his way to get
// onto one.
////////////////////////////////////////////////////////////////////////////////////////
bool Pilot_MasterUpdate()
{
	if (!NPC->enemy)
	{
		// If Still On A Vehicle, Jump Off
		//---------------------------------
		if (NPCInfo->greetEnt)
		{
			ucmd.upmove = 127;

			if (NPCInfo->greetEnt && NPCInfo->greetEnt->m_pVehicle && (level.time < NPCInfo->confusionTime || level.time
				< NPCInfo->insanityTime))
			{
				Vehicle_t* p_veh = NPCInfo->greetEnt->m_pVehicle;
				if (!(p_veh->m_ulFlags & VEH_OUTOFCONTROL))
				{
					gentity_t* parent = p_veh->m_pParentEntity;
					const float CurSpeed = VectorLength(parent->client->ps.velocity);
					p_veh->m_pVehicleInfo->StartDeathDelay(p_veh, 10000);
					p_veh->m_ulFlags |= VEH_OUTOFCONTROL;
					VectorScale(parent->client->ps.velocity, 1.25f, parent->pos3);
					if (CurSpeed < p_veh->m_pVehicleInfo->speedMax)
					{
						VectorNormalize(parent->pos3);
						if (fabsf(parent->pos3[2]) < 0.25f)
						{
							VectorScale(parent->pos3, p_veh->m_pVehicleInfo->speedMax * 1.25f, parent->pos3);
						}
						else
						{
							VectorScale(parent->client->ps.velocity, 1.25f, parent->pos3);
						}
					}
				}
			}

			if (NPCInfo->greetEnt->owner == NPC)
			{
				return true;
			}
			NPCInfo->greetEnt = nullptr;
		}

		// Otherwise Nothing To See Here
		//-------------------------------
		return false;
	}

	// If We Already Have A Target Vehicle, Make Sure It Is Still Valid
	//------------------------------------------------------------------
	if (NPCInfo->greetEnt)
	{
		if (!NPCInfo->greetEnt->inuse ||
			!NPCInfo->greetEnt->m_pVehicle ||
			!NPCInfo->greetEnt->m_pVehicle->m_pVehicleInfo)
		{
			NPCInfo->greetEnt = Vehicle_Find(NPC);
		}
		else
		{
			if (NPCInfo->greetEnt->owner && NPCInfo->greetEnt->owner != NPC)
			{
				NPCInfo->greetEnt = Vehicle_Find(NPC);
			}
		}
	}

	// If We Have An Enemy, Try To Find A Vehicle Nearby
	//---------------------------------------------------
	else
	{
		NPCInfo->greetEnt = Vehicle_Find(NPC);
	}

	// If No Vehicle Available, Continue As Usual
	//--------------------------------------------
	if (!NPCInfo->greetEnt)
	{
		return false;
	}

	if (NPCInfo->greetEnt->owner == NPC)
	{
		Pilot_Steer_Vehicle();
	}
	else
	{
		Pilot_Goto_Vehicle();
	}

	Pilot_Update_Enemy();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void Pilot_Update_Enemy()
{
	if (!TIMER_Exists(NPC, "PilotRemoveTime"))
	{
		TIMER_Set(NPC, "PilotRemoveTime", MIN_STAY_VIEWABLE_TIME);
	}

	if (TIMER_Done(NPC, "NextPilotCheckEnemyTime"))
	{
		TIMER_Set(NPC, "NextPilotCheckEnemyTime", Q_irand(1000, 2000));
		if (NPC->enemy && Distance(NPC->currentOrigin, NPC->enemy->currentOrigin) > 1000.0f)
		{
			mPilotViewTraceCount++;
			gi.trace(&mPilotViewTrace,
				NPC->currentOrigin,
				nullptr,
				nullptr,
				NPC->enemy->currentOrigin,
				NPC->s.number,
				MASK_SHOT,
				static_cast<EG2_Collision>(0), 0);

			if (mPilotViewTrace.allsolid == qfalse &&
				mPilotViewTrace.startsolid == qfalse &&
				(mPilotViewTrace.entityNum == NPC->enemy->s.number || mPilotViewTrace.entityNum == NPC->enemy->s.
					m_iVehicleNum))
			{
				TIMER_Set(NPC, "PilotRemoveTime", MIN_STAY_VIEWABLE_TIME);
			}
		}
		else
		{
			TIMER_Set(NPC, "PilotRemoveTime", MIN_STAY_VIEWABLE_TIME);
		}
	}

	if (TIMER_Done(NPC, "PilotRemoveTime"))
	{
		if (NPCInfo->greetEnt->owner == NPC)
		{
			NPCInfo->greetEnt->e_ThinkFunc = thinkF_G_FreeEntity;
			NPCInfo->greetEnt->nextthink = level.time;
		}
		NPC->e_ThinkFunc = thinkF_G_FreeEntity;
		NPC->nextthink = level.time;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void Pilot_Goto_Vehicle()
{
	STEER::Activate(NPC);
	{
		if (STEER::Reached(NPC, NPCInfo->greetEnt, 80.0f))
		{
			NPC_Use(NPCInfo->greetEnt, NPC, NPC);
		}
		else if (NAV::OnNeighboringPoints(NPC, NPCInfo->greetEnt))
		{
			STEER::Persue(NPC, NPCInfo->greetEnt, 50.0f, 0.0f, 30.0f, 0.0f, true);
		}
		else
		{
			if (!NAV::GoTo(NPC, NPCInfo->greetEnt))
			{
				STEER::Stop(NPC);
			}
		}
	}
	STEER::AvoidCollisions(NPC);
	STEER::DeActivate(NPC, &ucmd);
	NPC_UpdateAngles(qtrue, qtrue);
}

extern bool VEH_StartStrafeRam(Vehicle_t* p_veh, bool Right);

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void Pilot_Steer_Vehicle()
{
	if (!NPC->enemy || !NPC->enemy->client)
	{
		return;
	}

	// SETUP
	//=======
	// Setup Actor Data
	//------------------
	const CVec3 ActorPos(NPC->currentOrigin);
	CVec3 ActorAngles(NPC->currentAngles);
	ActorAngles[2] = 0;
	Vehicle_t* ActorVeh = NPCInfo->greetEnt->m_pVehicle;
	const bool ActorInTurbo = ActorVeh->m_iTurboTime > level.time;
	const float ActorSpeed = ActorVeh
		? VectorLength(ActorVeh->m_pParentEntity->client->ps.velocity)
		: NPC->client->ps.speed;

	// If my vehicle is spinning out of control, just hold on, we're going to die!!!!!
	//---------------------------------------------------------------------------------
	if (ActorVeh && ActorVeh->m_ulFlags & VEH_OUTOFCONTROL)
	{
		if (NPC->client->ps.weapon != WP_NONE)
		{
			NPC_ChangeWeapon(WP_NONE);
		}
		ucmd.buttons &= ~BUTTON_ATTACK;
		ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		return;
	}

	CVec3 ActorDirection{};
	AngleVectors(ActorAngles.v, ActorDirection.v, nullptr, nullptr);

	CVec3 ActorFuturePos(ActorPos);
	ActorFuturePos.ScaleAdd(ActorDirection, FUTURE_PRED_DIST);

	bool ActorDoTurbo = false;
	bool ActorAccelerate;
	bool ActorAimAtTarget = true;

	// Setup Enemy Data
	//------------------
	const CVec3 EnemyPos(NPC->enemy->currentOrigin);
	CVec3 EnemyAngles(NPC->enemy->currentAngles);
	EnemyAngles[2] = 0;
	const Vehicle_t* EnemyVeh = NPC->enemy->s.m_iVehicleNum
		? g_entities[NPC->enemy->s.m_iVehicleNum].m_pVehicle
		: nullptr;
	const bool EnemyInTurbo = EnemyVeh && EnemyVeh->m_iTurboTime > level.time;
	const float EnemySpeed = EnemyVeh ? EnemyVeh->m_pParentEntity->client->ps.speed : NPC->enemy->resultspeed;
	const bool EnemySlideBreak = EnemyVeh && (EnemyVeh->m_ulFlags & VEH_SLIDEBREAKING || EnemyVeh->m_ulFlags &
		VEH_STRAFERAM);
	const bool EnemyDead = NPC->enemy->health <= 0;

	const bool ActorFlank = NPCInfo->lastAvoidSteerSideDebouncer > level.time && EnemyVeh && EnemySpeed > 10.0f;

	CVec3 EnemyDirection{};
	CVec3 EnemyRight{};
	AngleVectors(EnemyAngles.v, EnemyDirection.v, EnemyRight.v, nullptr);

	CVec3 EnemyFuturePos(EnemyPos);
	EnemyFuturePos.ScaleAdd(EnemyDirection, FUTURE_PRED_DIST);

	const ESide EnemySide = ActorPos.LRTest(EnemyPos, EnemyFuturePos);
	CVec3 EnemyFlankPos(EnemyFuturePos);
	EnemyFlankPos.ScaleAdd(EnemyRight, EnemySide == Side_Right ? FUTURE_SIDE_DIST : -FUTURE_SIDE_DIST);

	// Debug Draw Enemy Data
	//-----------------------

	// Setup Move And Aim Directions
	//-------------------------------
	CVec3 MoveDirection(ActorFlank ? EnemyFlankPos : EnemyFuturePos);
	MoveDirection -= ActorPos;
	const float MoveDistance = MoveDirection.SafeNorm();
	const float MoveAccuracy = MoveDirection.Dot(ActorDirection);

	CVec3 AimDirection(EnemyPos);
	AimDirection -= ActorPos;
	const float AimDistance = AimDirection.SafeNorm();
	const float AimAccuracy = AimDirection.Dot(ActorDirection);

	if (!ActorFlank && TIMER_Done(NPC, "FlankAttackCheck"))
	{
		TIMER_Set(NPC, "FlankAttackCheck", Q_irand(1000, 3000));
		if (MoveDistance < 4000 && Q_irand(0, 1) == 0)
		{
			NPCInfo->lastAvoidSteerSideDebouncer = level.time + Q_irand(8000, 14000);
		}
	}

	// Fly By Sounds
	//---------------
	if (TIMER_Done(NPC, "FlybySoundDebouncer") &&
		(ActorVeh->m_pVehicleInfo->soundFlyBy || ActorVeh->m_pVehicleInfo->soundFlyBy2) &&
		EnemyVeh &&
		MoveDistance < 800 &&
		ActorSpeed > 500.0f)
	{
		if (EnemySpeed < 100.0f || ActorDirection.Dot(EnemyDirection) * (MoveDistance / 800.0f) < -0.5f)
		{
			TIMER_Set(NPC, "FlybySoundDebouncer", 2000);
			int soundFlyBy = ActorVeh->m_pVehicleInfo->soundFlyBy;
			if (ActorVeh->m_pVehicleInfo->soundFlyBy2 && (!soundFlyBy || !Q_irand(0, 1)))
			{
				soundFlyBy = ActorVeh->m_pVehicleInfo->soundFlyBy2;
			}
			G_Sound(ActorVeh->m_pParentEntity, soundFlyBy);
		}
	}

	// FLY PAST BEHAVIOR
	//===================
	if (EnemySlideBreak || !TIMER_Done(NPC, "MinHoldDirectionTime"))
	{
		if (TIMER_Done(NPC, "MinHoldDirectionTime"))
		{
			TIMER_Set(NPC, "MinHoldDirectionTime", 500); // Hold For At Least 500 ms
		}
		ActorAccelerate = true; // Go
		ActorAimAtTarget = false; // Don't Alter Our Aim Direction
		ucmd.buttons &= ~BUTTON_VEH_SPEED; // Let Normal Vehicle Controls Go
	}

	// FLANKING BEHAVIOR
	//===================
	else if (ActorFlank)
	{
		ActorAccelerate = true;
		ActorDoTurbo = MoveDistance > 2500 || EnemyInTurbo;
		ucmd.buttons |= BUTTON_VEH_SPEED;
		// Tells PMove to use the ps.speed we calculate here, not the one from g_vehicles.c

		// For Flanking, We Calculate The Speed By Hand, Rather Than Using Pure Accelerate / No Accelerate Functionality
		//---------------------------------------------------------------------------------------------------------------
		NPC->client->ps.speed = ActorVeh->m_pVehicleInfo->speedMax * (ActorInTurbo ? 1.35f : 1.15f);

		// If In Slowing Distance, Scale Down The Speed As We Approach Our Move Target
		//-----------------------------------------------------------------------------
		if (MoveDistance < ATTACK_FLANK_SLOWING)
		{
			NPC->client->ps.speed *= MoveDistance / ATTACK_FLANK_SLOWING;
			NPC->client->ps.speed += EnemySpeed;

			// Match Enemy Speed
			//-------------------
			if (NPC->client->ps.speed < 5.0f && EnemySpeed < 5.0f)
			{
				NPC->client->ps.speed = EnemySpeed;
			}

			// Extra Slow Down When Out In Front
			//-----------------------------------
			if (MoveAccuracy < 0.0f)
			{
				NPC->client->ps.speed *= MoveAccuracy + 1.0f;
			}

			MoveDirection *= MoveDistance / ATTACK_FLANK_SLOWING;
			EnemyDirection *= 1.0f - MoveDistance / ATTACK_FLANK_SLOWING;
			MoveDirection += EnemyDirection;

			if (TIMER_Done(NPC, "RamCheck"))
			{
				TIMER_Set(NPC, "RamCheck", Q_irand(1000, 3000));
				if (MoveDistance < RAM_DIST && Q_irand(0, 2) == 0)
				{
					VEH_StartStrafeRam(ActorVeh, EnemySide == Side_Left);
				}
			}
		}
	}
	// NORMAL CHASE BEHAVIOR
	//=======================
	else
	{
		if (!EnemyVeh && AimAccuracy > 0.99f && MoveDistance < 500 && !EnemyDead)
		{
			ActorAccelerate = true;
			ActorDoTurbo = false;
		}
		else
		{
			ActorAccelerate = MoveDistance > 500 && EnemySpeed > 20.0f || MoveDistance > 1000;
			ActorDoTurbo = MoveDistance > 3000 && EnemySpeed > 20.0f;
		}
		ucmd.buttons &= ~BUTTON_VEH_SPEED;
	}

	// APPLY RESULTS
	//=======================
	// Decide Turbo
	//--------------
	if (ActorDoTurbo || ActorInTurbo)
	{
		ucmd.buttons |= BUTTON_ALT_ATTACK;
	}
	else
	{
		ucmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	// Decide Acceleration
	//---------------------
	ucmd.forwardmove = ActorAccelerate ? 127 : 0;

	// Decide To Shoot
	//-----------------
	ucmd.buttons &= ~BUTTON_ATTACK;
	ucmd.rightmove = 0;
	if (AimDistance < 2000 && !EnemyDead)
	{
		// If Doing A Ram Attack
		//-----------------------
		if (AimAccuracy > ATTACK_FWD)
		{
			if (NPC->client->ps.weapon != WP_NONE)
			{
				NPC_ChangeWeapon(WP_NONE);
			}
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if (AimAccuracy < AIM_SIDE && AimAccuracy > -AIM_SIDE)
		{
			if (NPC->client->ps.weapon != WP_BLASTER)
			{
				NPC_ChangeWeapon(WP_BLASTER);
			}

			if (AimAccuracy < ATTACK_SIDE && AimAccuracy > -ATTACK_SIDE)
			{
				ucmd.buttons |= BUTTON_ATTACK;
				WeaponThink();
			}
			ucmd.rightmove = EnemySide == Side_Left ? 127 : -127;
		}
		else
		{
			if (NPC->client->ps.weapon != WP_NONE)
			{
				NPC_ChangeWeapon(WP_NONE);
			}
		}
	}
	else
	{
		if (NPC->client->ps.weapon != WP_NONE)
		{
			NPC_ChangeWeapon(WP_NONE);
		}
	}

	// Aim At Target
	//---------------
	if (ActorAimAtTarget)
	{
		constexpr float ActorYawOffset = 0.0f;
		MoveDirection.VecToAng();
		NPCInfo->desiredPitch = AngleNormalize360(MoveDirection[PITCH]);
		NPCInfo->desiredYaw = AngleNormalize360(MoveDirection[YAW] + ActorYawOffset);
	}
	NPC_UpdateAngles(qtrue, qtrue);
}