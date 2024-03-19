/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

#include "../server/exe_headers.h"

// tr_subs.cpp - common function replacements for modular renderer
#include "tr_local.h"

void QDECL Com_Printf(const char* msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof text, msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_Error(const int level, const char* error, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof text, error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}

/*
================
Com_DPrintf

DLL glue
================
*/
void Com_DPrintf(const char* format, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, format);
	Q_vsnprintf(text, sizeof text, format, argptr);
	va_end(argptr);

	ri.Printf(PRINT_DEVELOPER, "%s", text);
}
// ZONE
void* R_Malloc(const int iSize, const memtag_t eTag, const qboolean bZeroit)
{
	return ri.Z_Malloc(iSize, eTag, bZeroit, 4);
}

void R_Free(void* pvAddress)
{
	ri.Z_Free(pvAddress);
}

int R_MemSize(const memtag_t eTag)
{
	return ri.Z_MemSize(eTag);
}

void R_MorphMallocTag(void* pvBuffer, const memtag_t eDesiredTag)
{
	ri.Z_MorphMallocTag(pvBuffer, eDesiredTag);
}

void* R_Hunk_Alloc(const int iSize, const qboolean bZeroit)
{
	return ri.Z_Malloc(iSize, TAG_HUNKALLOC, bZeroit, 4);
}