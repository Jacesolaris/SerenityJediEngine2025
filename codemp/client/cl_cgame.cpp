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

// cl_cgame.c  -- client system interaction with client game
#include "client.h"
#include "cl_cgameapi.h"
#include "botlib/botlib.h"
#include "FXExport.h"
#include "FxUtil.h"
#include "qcommon/RoffSystem.h"
#include "qcommon/stringed_ingame.h"
#include "ghoul2/G2_gore.h"

extern IHeapAllocator* G2VertSpaceClient;

#include "snd_ambient.h"
#include "qcommon/timing.h"

/*
Ghoul2 Insert End
*/

extern botlib_export_t* botlib_export;

extern qboolean loadCamera(const char* name);
extern void startCamera(int time);
extern qboolean getCameraInfo(int time, vec3_t* origin, vec3_t* angles);

/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd(const int cmdNumber, usercmd_t* ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if (cmdNumber > cl.cmdNumber)
	{
		Com_Error(ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if (cmdNumber <= cl.cmdNumber - CMD_BACKUP)
	{
		return qfalse;
	}

	*ucmd = cl.cmds[cmdNumber & CMD_MASK];

	return qtrue;
}

/*
====================
CL_GetParseEntityState
====================
*/
qboolean CL_GetParseEntityState(const int parseentity_number, entityState_t* state)
{
	// can't return anything that hasn't been parsed yet
	if (parseentity_number >= cl.parseEntitiesNum)
	{
		Com_Error(ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseentity_number, cl.parseEntitiesNum);
	}

	// can't return anything that has been overwritten in the circular buffer
	if (parseentity_number <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	*state = cl.parseEntities[parseentity_number & MAX_PARSE_ENTITIES - 1];
	return qtrue;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean CL_GetSnapshot(const int snapshotNumber, snapshot_t* snapshot)
{
	if (snapshotNumber > cl.snap.messageNum)
	{
		Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if (cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP)
	{
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	const clSnapshot_t* clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if (!clSnap->valid)
	{
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof snapshot->areamask);
	snapshot->ps = clSnap->ps;
	snapshot->vps = clSnap->vps; //get the vehicle ps
	int count = clSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT)
	{
		Com_DPrintf("CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;

	for (int i = 0; i < count; i++)
	{
		const int entNum = clSnap->parseEntitiesNum + i & MAX_PARSE_ENTITIES - 1;

		// copy everything but the ghoul2 pointer
		memcpy(&snapshot->entities[i], &cl.parseEntities[entNum], sizeof(entityState_t));
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

qboolean CL_GetDefaultState(const int index, entityState_t* state)
{
	if (index < 0 || index >= MAX_GENTITIES)
	{
		return qfalse;
	}

	if (!(cl.entityBaselines[index].eFlags & EF_PERMANENT))
	{
		return qfalse;
	}

	*state = cl.entityBaselines[index];

	return qtrue;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
extern float cl_mPitchOverride;
extern float cl_mYawOverride;
extern float cl_mSensitivityOverride;

void CL_SetUserCmdValue(const int userCmdValue, const float sensitivityScale, const float mPitchOverride,
	const float mYawOverride, const float mSensitivityOverride, const int fpSel, const int invenSel)
{
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
	cl_mPitchOverride = mPitchOverride;
	cl_mYawOverride = mYawOverride;
	cl_mSensitivityOverride = mSensitivityOverride;
	cl.cgameForceSelection = fpSel;
	cl.cgameInvenSelection = invenSel;
}

int gCLTotalclientNum = 0;
//keep track of the total number of clients
extern cvar_t* cl_autolodscale;
//if we want to do autolodscaling

void CL_DoAutoLODScale(void)
{
	float finalLODScaleFactor = 0;

	if (gCLTotalclientNum >= 8)
	{
		finalLODScaleFactor = gCLTotalclientNum / -8.0f;
	}

	Cvar_Set("r_autolodscalevalue", va("%f", finalLODScaleFactor));
}

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified(void)
{
	int i;
	char* dup;

	const int index = atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "CL_ConfigstringModified: bad index %i", index);
	}
	// get everything after "cs <num>"
	char* s = Cmd_ArgsFrom(2);

	const char* old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
	if (strcmp(old, s) == 0)
	{
		return; // unchanged
	}

	// build the new gameState_t
	gameState_t oldGs = cl.gameState;

	Com_Memset(&cl.gameState, 0, sizeof cl.gameState);

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if (!dup[0])
		{
			continue; // leave with the default empty string
		}

		const int len = strlen(dup);

		if (len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS)
		{
			Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
		Com_Memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
		cl.gameState.dataCount += len + 1;
	}

	if (cl_autolodscale && cl_autolodscale->integer)
	{
		if (index >= CS_PLAYERS &&
			index < CS_G2BONES)
		{
			//this means that a client was updated in some way. Go through and count the clients.
			int clientCount = 0;
			i = CS_PLAYERS;

			while (i < CS_G2BONES)
			{
				s = cl.gameState.stringData + cl.gameState.stringOffsets[i];

				if (s && s[0])
				{
					clientCount++;
				}

				i++;
			}

			gCLTotalclientNum = clientCount;

#ifdef _DEBUG
			Com_DPrintf("%i clients\n", gCLTotalclientNum);
#endif

			CL_DoAutoLODScale();
		}
	}

	if (index == CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}
}
#ifndef MAX_STRINGED_SV_STRING
#define MAX_STRINGED_SV_STRING 1024
#endif
// just copied it from CG_CheckSVStringEdRef(
void CL_CheckSVStringEdRef(char* buf, const char* str)
{
	//I don't really like doing this. But it utilizes the system that was already in place.
	int i = 0;
	int b = 0;

	if (!str || !str[0])
	{
		if (str)
		{
			strcpy(buf, str);
		}
		return;
	}

	strcpy(buf, str);

	const int strLen = strlen(str);

	if (strLen >= MAX_STRINGED_SV_STRING)
	{
		return;
	}

	while (i < strLen && str[i])
	{
		constexpr qboolean gotStrip = qfalse;

		if (str[i] == '@' && i + 1 < strLen)
		{
			if (str[i + 1] == '@' && i + 2 < strLen)
			{
				if (str[i + 2] == '@' && i + 3 < strLen)
				{
					//@@@ should mean to insert a StringEd reference here, so insert it into buf at the current place
					char stringRef[MAX_STRINGED_SV_STRING]{};
					int r = 0;

					while (i < strLen && str[i] == '@')
					{
						i++;
					}

					while (i < strLen && str[i] && str[i] != ' ' && str[i] != ':' && str[i] != '.' && str[i] != '\n')
					{
						stringRef[r] = str[i];
						r++;
						i++;
					}
					stringRef[r] = 0;

					buf[b] = 0;
					Q_strcat(buf, MAX_STRINGED_SV_STRING, SE_GetString("MP_SVGAME", stringRef));
					b = strlen(buf);
				}
			}
		}

		if (!gotStrip)
		{
			buf[b] = str[i];
			b++;
		}
		i++;
	}

	buf[b] = 0;
}

/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand(const int serverCommandNumber)
{
	static char bigConfigString[BIG_INFO_STRING];

	// if we have irretrievably lost a reliable command, drop the connection
	if (serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
	{
		int i = 0;

		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (clc.demoplaying)
			return qfalse;

		while (i < MAX_RELIABLE_COMMANDS)
		{
			//spew out the reliable command buffer
			if (clc.reliableCommands[i][0])
			{
				Com_Printf("%i: %s\n", i, clc.reliableCommands[i]);
			}
			i++;
		}
		Com_Error(ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out");
	}

	if (serverCommandNumber > clc.serverCommandSequence)
	{
		Com_Error(ERR_DROP, "CL_GetServerCommand: requested a command not received");
	}

	char* s = clc.serverCommands[serverCommandNumber & MAX_RELIABLE_COMMANDS - 1];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);

rescan:
	Cmd_TokenizeString(s);
	const char* cmd = Cmd_Argv(0);

	if (strcmp(cmd, "disconnect") == 0)
	{
		char strEd[MAX_STRINGED_SV_STRING];
		CL_CheckSVStringEdRef(strEd, Cmd_Argv(1));
		Com_Error(ERR_SERVERDISCONNECT, "%s: %s\n", SE_GetString("MP_SVGAME_SERVER_DISCONNECTED"), strEd);
	}

	if (strcmp(cmd, "bcs0") == 0)
	{
		Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return qfalse;
	}

	if (strcmp(cmd, "bcs1") == 0)
	{
		s = Cmd_Argv(2);
		if (strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		return qfalse;
	}

	if (strcmp(cmd, "bcs2") == 0)
	{
		s = Cmd_Argv(2);
		if (strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		strcat(bigConfigString, "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (strcmp(cmd, "cs") == 0)
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return qtrue;
	}

	if (strcmp(cmd, "map_restart") == 0)
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		Com_Memset(cl.cmds, 0, sizeof cl.cmds);
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make appropriate adjustments,
	// but we also clear the console and notify lines here
	if (strcmp(cmd, "clientLevelShot") == 0)
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if (!com_sv_running->integer)
		{
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame(void)
{
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CGAME);

	if (!cls.cgameStarted)
		return;

	cls.cgameStarted = qfalse;

	CL_UnbindCGame();
}

/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/

void CL_InitCGame(void)
{
	const int t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	const char* info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	const char* mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cl.mapname, sizeof cl.mapname, "maps/%s.bsp", mapname);

	// load the dll
	CL_BindCGame();

	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	CGVM_Init(clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum);

	const int clRate = Cvar_VariableIntegerValue("rate");
	if (clRate == 4000)
	{
		Com_Printf(
			S_COLOR_YELLOW
			"WARNING: Old default /rate value detected (4000). Suggest typing /rate 25000 into console for a smoother connection!\n");
	}

	// reset any CVAR_CHEAT cvars registered by cgame
	if (!clc.demoplaying && !cl_connectedToCheatServer)
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	const int t2 = Sys_Milliseconds();

	Com_Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re->EndRegistration();

	// make sure everything is paged in
	//	if (!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify();
}

/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand(void)
{
	if (!cls.cgameStarted)
		return qfalse;

	return CGVM_ConsoleCommand();
}

/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering(const stereoFrame_t stereo)
{
	//rww - RAGDOLL_BEGIN
	if (!com_sv_running->integer)
	{
		//set the server time to match the client time, if we don't have a server going.
		re->G2API_SetTime(cl.serverTime, 0);
	}
	re->G2API_SetTime(cl.serverTime, 1);
	//rww - RAGDOLL_END

	CGVM_DrawActiveFrame(cl.serverTime, stereo, clc.demoplaying);
}

/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

constexpr auto RESET_TIME = 500;

void CL_AdjustTimeDelta(void)
{
	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if (clc.demoplaying)
	{
		return;
	}

	const int newDelta = cl.snap.serverTime - cls.realtime;
	const int deltaDelta = abs(newDelta - cl.serverTimeDelta);

	if (deltaDelta > RESET_TIME)
	{
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime; // FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if (cl_showTimeDelta->integer)
		{
			Com_Printf("<RESET> ");
		}
	}
	else if (deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if (cl_showTimeDelta->integer)
		{
			Com_Printf("<FAST> ");
		}
		cl.serverTimeDelta = (cl.serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if (com_timescale->value == 0 || com_timescale->value == 1)
		{
			if (cl.extrapolatedSnapshot)
			{
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if (cl_showTimeDelta->integer)
	{
		Com_Printf("%i ", cl.serverTimeDelta);
	}
}

/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if (cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}

	re->RegisterMedia_LevelLoadEnd();

	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if (cl_activeAction->string[0])
	{
		Cbuf_AddText(cl_activeAction->string);
		Cvar_Set("activeAction", "");
	}
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if (cls.state != CA_ACTIVE)
	{
		if (cls.state != CA_PRIMED)
		{
			return;
		}
		if (clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if (!clc.firstDemoFrameSkipped)
			{
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if (cl.newSnapshots)
		{
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if (cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if (!cl.snap.valid)
	{
		Com_Error(ERR_DROP, "CL_SetCGameTime: !cl.snap.valid");
	}

	// allow pause in single player
	if (sv_paused->integer && CL_CheckPaused() && com_sv_running->integer)
	{
		// paused
		return;
	}

	if (cl.snap.serverTime < cl.oldFrameServerTime)
	{
		Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
	}
	cl.oldFrameServerTime = cl.snap.serverTime;

	// get our current view of time

	if (clc.demoplaying && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances
	}
	else
	{
		int tn = cl_timeNudge->integer;
#ifdef _DEBUG
		if (tn < -900)
		{
			tn = -900;
		}
		else if (tn > 900)
		{
			tn = 900;
		}
#else
		if (tn < -30) {
			tn = -30;
		}
		else if (tn > 30) {
			tn = 30;
		}
#endif

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if (cl.serverTime < cl.oldServerTime)
		{
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if (cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5)
		{
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if (cl.newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if (!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if (cl_timedemo->integer)
	{
		if (!clc.timeDemoStart)
		{
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while (cl.serverTime >= cl.snap.serverTime)
	{
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if (cls.state != CA_ACTIVE)
		{
			return; // end of demo
		}
	}
}