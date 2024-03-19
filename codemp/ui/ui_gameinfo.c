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

//
// gameinfo.c
//

#include "ui_local.h"

//
// arena and bot info
//

int				ui_numBots;
static char* ui_botInfos[MAX_BOTS];

static int		ui_numArenas;
static char* ui_arenaInfos[MAX_ARENAS];

/*
===============
UI_ParseInfos
===============
*/
static int UI_ParseInfos(const char* buf, int max, char* infos[])
{
	char	info[MAX_INFO_STRING];

	int count = 0;

	COM_BeginParseSession("UI_ParseInfos");
	while (1) {
		char* token = COM_Parse(&buf);
		if (!token[0]) {
			break;
		}
		if (strcmp(token, "{") != 0) {
			Com_Printf("Missing { in info file\n");
			break;
		}

		if (count == max) {
			Com_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while (1) {
			char key[MAX_TOKEN_CHARS];
			token = COM_ParseExt(&buf, qtrue);
			if (!token[0]) {
				Com_Printf("Unexpected end of info file\n");
				break;
			}
			if (strcmp(token, "}") == 0) {
				break;
			}
			Q_strncpyz(key, token, sizeof key);

			token = COM_ParseExt(&buf, qfalse);
			if (!token[0]) {
				strcpy(token, "<NULL>");
			}
			Info_SetValueForKey(info, key, token);
		}
		//NOTE: extra space for arena number
		infos[count] = (char*)UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
#ifndef FINAL_BUILD
			if (trap->Cvar_VariableValue("com_buildScript"))
			{
				char* botFile = Info_ValueForKey(info, "personality");
				if (botFile && botFile[0])
				{
					int fh = 0;
					trap->FS_Open(botFile, &fh, FS_READ);
					if (fh)
					{
						trap->FS_Close(fh);
					}
				}
			}
#endif
			count++;
		}
	}
	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile(char* filename) {
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	const int len = trap->FS_Open(filename, &f, FS_READ);
	if (!f) {
		trap->Print(S_COLOR_RED "file not found: %s\n", filename);
		return;
	}
	if (len >= MAX_ARENAS_TEXT) {
		trap->Print(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT);
		trap->FS_Close(f);
		return;
	}

	trap->FS_Read(buf, len, f);
	buf[len] = 0;
	trap->FS_Close(f);

	ui_numArenas += UI_ParseInfos(buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas]);
}

/*
===============
UI_LoadArenas
===============
*/

#define MAPSBUFSIZE (MAX_MAPS * 64)
void UI_LoadArenas(void)
{
	char		dirlist[MAPSBUFSIZE];
	int			dirlen = 0;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	// get all arenas from .arena files
	const int numdirs = trap->FS_GetFileList("scriptsmp", ".arena", dirlist, ARRAY_LEN(dirlist));
	char* dirptr = dirlist;
	for (int i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		char filename[MAX_QPATH];
		dirlen = strlen(dirptr);
		strcpy(filename, "scriptsmp/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
	if (UI_OutOfMemory())
	{
		trap->Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");
	}

	for (int n = 0; n < ui_numArenas; n++) {
		// determine type

		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "map"));
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "longname"));
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc(va("levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName));
		uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

		const char* type = Info_ValueForKey(ui_arenaInfos[n], "type");
		// if no type specified, it will be treated as "ffa"
		if (*type) {
			if (strstr(type, "ffa"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
			}
			if (strstr(type, "team"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_TEAM;
			}
			if (strstr(type, "missions"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_SINGLE_PLAYER;
			}
			if (strstr(type, "holocron"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_HOLOCRON;
			}
			if (strstr(type, "jedimaster"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
			}
			if (strstr(type, "duel"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_DUEL;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_POWERDUEL;
			}
			if (strstr(type, "powerduel"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_DUEL;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_POWERDUEL;
			}
			if (strstr(type, "siege"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_SIEGE;
			}
			if (strstr(type, "ctf"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTF;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTY;
			}
			if (strstr(type, "cty"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTY;
			}
			if (strstr(type, "MB"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
			}
		}
		else
		{
			uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
			uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
		}

		uiInfo.mapCount++;
		if (uiInfo.mapCount >= MAX_MAPS) {
			break;
		}
	}
}

void UI_LoadSPArenas(void)
{
	char		dirlist[MAPSBUFSIZE];
	int			dirlen = 0;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	// get all arenas from .arena files
	const int numdirs = trap->FS_GetFileList("scriptsmp", ".arena", dirlist, ARRAY_LEN(dirlist));
	char* dirptr = dirlist;
	for (int i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		char filename[MAX_QPATH];
		dirlen = strlen(dirptr);
		strcpy(filename, "scriptsmp/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
	if (UI_OutOfMemory())
	{
		trap->Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");
	}

	for (int n = 0; n < ui_numArenas; n++)
	{
		// determine type

		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "map"));
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "longname"));
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc(va("levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName));
		uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

		const char* type = Info_ValueForKey(ui_arenaInfos[n], "type");
		// if no type specified, it will be treated as "ffa"
		if (*type) {
			if (strstr(type, "ffa"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
			}
			if (strstr(type, "team"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_TEAM;
			}
			if (strstr(type, "missions"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_SINGLE_PLAYER;
			}
			if (strstr(type, "holocron"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_HOLOCRON;
			}
			if (strstr(type, "jedimaster"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
			}
			if (strstr(type, "duel"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_DUEL;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_POWERDUEL;
			}
			if (strstr(type, "powerduel"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_DUEL;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_POWERDUEL;
			}
			if (strstr(type, "siege"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_SIEGE;
			}
			if (strstr(type, "ctf"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTF;
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTY;
			}
			if (strstr(type, "cty"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_CTY;
			}
			if (strstr(type, "MB"))
			{
				uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
			}
		}
		else
		{
			uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_FFA;
			uiInfo.mapList[uiInfo.mapCount].typeBits |= 1 << GT_JEDIMASTER;
		}

		uiInfo.mapCount++;
		if (uiInfo.mapCount >= MAX_MAPS) {
			break;
		}
	}
}

/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile(char* filename) {
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	const int len = trap->FS_Open(filename, &f, FS_READ);
	if (!f) {
		trap->Print(S_COLOR_RED "file not found: %s\n", filename);
		return;
	}
	if (len >= MAX_BOTS_TEXT) {
		trap->Print(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT);
		trap->FS_Close(f);
		return;
	}

	trap->FS_Read(buf, len, f);
	buf[len] = 0;

	const char* stopMark = strstr(buf, "@STOPHERE");

	//This bot is in place as a mark for modview's bot viewer.
	//If we hit it just stop and trace back to the beginning of the bot define and cut the string off.
	//This is only done in the UI and not the game so that "test" bots can be added manually and still
	//not show up in the menu.
	if (stopMark)
	{
		int startPoint = stopMark - buf;

		while (buf[startPoint] != '{')
		{
			startPoint--;
		}

		buf[startPoint] = 0;
	}

	trap->FS_Close(f);

	COM_Compress(buf);

	ui_numBots += UI_ParseInfos(buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots]);
}

/*
===============
UI_LoadBots
===============
*/
void UI_LoadBots(void)
{
	vmCvar_t	botsFile;
	char		dirlist[1024];
	int			dirlen = 0;

	ui_numBots = 0;

	trap->Cvar_Register(&botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM);
	if (*botsFile.string)
	{
		UI_LoadBotsFromFile(botsFile.string);
	}
	else
	{
		UI_LoadBotsFromFile("botfiles/bots.txt");
	}

	// get all bots from .bot files
	const int numdirs = trap->FS_GetFileList("scriptsmp", ".bot", dirlist, 1024);
	char* dirptr = dirlist;
	for (int i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		char filename[128];
		dirlen = strlen(dirptr);
		strcpy(filename, "scriptsmp/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}
}

/*
===============
UI_GetBotInfoByNumber
===============
*/
static char* UI_GetBotInfoByNumber(int num) {
	if (num < 0 || num >= ui_numBots) {
		trap->Print(S_COLOR_RED "Invalid bot number: %i\n", num);
		return NULL;
	}
	return ui_botInfos[num];
}

/*
===============
UI_GetBotInfoByName
===============
*/
static char* UI_GetBotInfoByName(const char* name) {
	for (int n = 0; n < ui_numBots; n++) {
		const char* value = Info_ValueForKey(ui_botInfos[n], "name");
		if (!Q_stricmp(value, name)) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

int UI_GetNumBots() {
	return ui_numBots;
}

char* UI_GetBotNameByNumber(int num) {
	const char* info = UI_GetBotInfoByNumber(num);
	if (info) {
		return Info_ValueForKey(info, "name");
	}
	return "Kyle";
}