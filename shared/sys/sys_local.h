/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#pragma once

#include "qcommon/qcommon.h"

void IN_Init(void* windowData);
void IN_Frame(void);
void IN_Shutdown(void);
void IN_Restart(void);

void Sys_PlatformInit();
void Sys_PlatformExit();
char* Sys_ConsoleInput();
void Sys_QueEvent(int ev_time, sysEventType_t ev_type, int value, int value2, int ptrLength, void* ptr);
void Sys_SigHandler(int signal);

struct UnpackDLLResult
{
	bool succeeded;
	char* tempDLLPath;
};

UnpackDLLResult Sys_UnpackDLL(const char* name);
bool Sys_DLLNeedsUnpacking();
