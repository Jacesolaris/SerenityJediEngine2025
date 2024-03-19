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

//b_goal.cpp
#include "b_local.h"
#include "Q3_Interface.h"
#include "g_navigator.h"

extern qboolean FlyingCreature(const gentity_t* ent);
/*
SetGoal
*/

void SetGoal(gentity_t* goal, float rating)
{
	NPCInfo->goalEntity = goal;
	NPCInfo->goalTime = level.time;
	
	if (goal)
	{
		//		Debug_NPCPrintf( NPC, debugNPCAI, DEBUG_LEVEL_INFO, "NPC_SetGoal: %s @ %s (%f)\n", goal->classname, vtos( goal->currentOrigin), rating );
	}
	else
	{
		//		Debug_NPCPrintf( NPC, debugNPCAI, DEBUG_LEVEL_INFO, "NPC_SetGoal: NONE\n" );
	}
}

/*
NPC_SetGoal
*/

void NPC_SetGoal(gentity_t* goal, const float rating)
{
	if (goal == NPCInfo->goalEntity)
	{
		return;
	}

	if (!goal)
	{
		return;
	}

	if (goal->client)
	{
		return;
	}

	if (NPCInfo->goalEntity)
	{
		NPCInfo->lastGoalEntity = NPCInfo->goalEntity;
	}

	SetGoal(goal, rating);
}

/*
NPC_ClearGoal
*/

void NPC_ClearGoal()
{
	if (!NPCInfo->lastGoalEntity)
	{
		SetGoal(nullptr, 0.0);
		return;
	}

	gentity_t* goal = NPCInfo->lastGoalEntity;
	NPCInfo->lastGoalEntity = nullptr;
	if (goal->inuse && !(goal->s.eFlags & EF_NODRAW))
	{
		SetGoal(goal, 0); //, NPCInfo->lastGoalEntityNeed
		return;
	}

	SetGoal(nullptr, 0.0);
}

/*
-------------------------
G_BoundsOverlap
-------------------------
*/

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	//NOTE: flush up against counts as overlapping
	if (mins1[0] > maxs2[0])
		return qfalse;

	if (mins1[1] > maxs2[1])
		return qfalse;

	if (mins1[2] > maxs2[2])
		return qfalse;

	if (maxs1[0] < mins2[0])
		return qfalse;

	if (maxs1[1] < mins2[1])
		return qfalse;

	if (maxs1[2] < mins2[2])
		return qfalse;

	return qtrue;
}

void NPC_ReachedGoal()
{
	NPC_ClearGoal();
	NPCInfo->goalTime = level.time;

	//MCG - Begin
	NPCInfo->aiFlags &= ~NPCAI_MOVING;
	ucmd.forwardmove = 0;
	//Return that the goal was reached
	Q3_TaskIDComplete(NPC, TID_MOVE_NAV);
	//MCG - End
}

/*
ReachedGoal

id removed checks against waypoints and is now checking surfaces
*/
static qboolean ReachedGoal(gentity_t* goal)
{
	if (NPCInfo->aiFlags & NPCAI_TOUCHED_GOAL)
	{
		NPCInfo->aiFlags &= ~NPCAI_TOUCHED_GOAL;
		return qtrue;
	}
	return static_cast<qboolean>(STEER::Reached(NPC, goal, NPCInfo->goalRadius, FlyingCreature(NPC) != qfalse));
}

/*
static gentity_t *UpdateGoal( void )

Id removed a lot of shit here... doesn't seem to handle waypoints independantly of goalentity

In fact, doesn't seem to be any waypoint info on entities at all any more?

MCG - Since goal is ALWAYS goalEntity, took out a lot of sending goal entity pointers around for no reason
*/

gentity_t* UpdateGoal()
{
	//FIXME: CREED should look at this
	//		this func doesn't seem to be working correctly for the sand creature

	if (!NPCInfo->goalEntity)
	{
		return nullptr;
	}

	if (!NPCInfo->goalEntity->inuse)
	{
		//Somehow freed it, but didn't clear it
		NPC_ClearGoal();
		return nullptr;
	}

	gentity_t* goal = NPCInfo->goalEntity;

	if (ReachedGoal(goal))
	{
		NPC_ReachedGoal();
		goal = nullptr; //so they don't keep trying to move to it
	} //else if fail, need to tell script so?

	return goal;
}