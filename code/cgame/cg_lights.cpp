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

#include "cg_headers.h"

using clightstyle_t = struct clightstyle_s
{
	int length;
	color4ub_t value;
	color4ub_t map[MAX_QPATH];
};

static clightstyle_t cl_lightstyle[MAX_LIGHT_STYLES];
static int lastofs;

/*
================
FX_ClearLightStyles
================
*/
void CG_ClearLightStyles()
{
	memset(cl_lightstyle, 0, sizeof cl_lightstyle);
	lastofs = -1;

	for (int i = 0; i < MAX_LIGHT_STYLES * 3; i++)
	{
		CG_SetLightstyle(i);
	}
}

/*
================
FX_RunLightStyles
================
*/
void CG_RunLightStyles()
{
	int i;
	clightstyle_t* ls;

	const int ofs = cg.time / 50;
	//	if (ofs == lastofs)
	//		return;
	lastofs = ofs;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHT_STYLES; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->value[3] = 255;
		}
		else if (ls->length == 1)
		{
			ls->value[0] = ls->map[0][0];
			ls->value[1] = ls->map[0][1];
			ls->value[2] = ls->map[0][2];
			ls->value[3] = 255; //ls->map[0][3];
		}
		else
		{
			ls->value[0] = ls->map[ofs % ls->length][0];
			ls->value[1] = ls->map[ofs % ls->length][1];
			ls->value[2] = ls->map[ofs % ls->length][2];
			ls->value[3] = 255; //ls->map[ofs%ls->length][3];
		}
		const byteAlias_t* ba = reinterpret_cast<byteAlias_t*>(&ls->value);
		trap_R_SetLightStyle(i, ba->i);
	}
}

void CG_SetLightstyle(const int i)
{
	const char* s = CG_ConfigString(i + CS_LIGHT_STYLES);
	const int j = strlen(s);
	if (j >= MAX_QPATH)
	{
		Com_Error(ERR_DROP, "svc_lightstyle length=%i", j);
	}

	cl_lightstyle[(i / 3)].length = j;
	for (int k = 0; k < j; k++)
	{
		cl_lightstyle[(i / 3)].map[k][(i % 3)] = static_cast<float>(s[k] - 'a') / static_cast<float>('z' - 'a') * 255.0;
	}
}