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

//g_objectives.cpp
//reads in ext_data\objectives.dat to objectives[]

#include "g_local.h"
#include "g_items.h"

#define	G_OBJECTIVES_CPP

#include "objectives.h"
#include "qcommon/ojk_saved_game_helper.h"

qboolean missionInfo_Updated;

/*
============
OBJ_SetPendingObjectives
============
*/
void OBJ_SetPendingObjectives(const gentity_t* ent)
{
	for (int i = 0; i < MAX_OBJECTIVES; ++i)
	{
		if (ent->client->sess.mission_objectives[i].status == OBJECTIVE_STAT_PENDING &&
			ent->client->sess.mission_objectives[i].display)
		{
			ent->client->sess.mission_objectives[i].status = OBJECTIVE_STAT_FAILED;
		}
	}
}

/*
============
OBJ_SaveMissionObjectives
============
*/
static void OBJ_SaveMissionObjectives(const gclient_t* client)
{
	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	saved_game.write_chunk(
		INT_ID('O', 'B', 'J', 'T'),
		client->sess.mission_objectives);
}

/*
============
OBJ_SaveObjectiveData
============
*/
void OBJ_SaveObjectiveData()
{
	const gclient_t* client = &level.clients[0];

	OBJ_SaveMissionObjectives(client);
}

/*
============
OBJ_LoadMissionObjectives
============
*/
static void OBJ_LoadMissionObjectives(gclient_t* client)
{
	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	saved_game.read_chunk(
		INT_ID('O', 'B', 'J', 'T'),
		client->sess.mission_objectives);
}

/*
============
OBJ_LoadObjectiveData
============
*/
void OBJ_LoadObjectiveData()
{
	gclient_t* client = &level.clients[0];

	OBJ_LoadMissionObjectives(client);
}