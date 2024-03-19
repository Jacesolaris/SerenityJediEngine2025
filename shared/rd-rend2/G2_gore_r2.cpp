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

#include "G2_gore_r2.h"
#include "../rd-common/tr_common.h"

#ifdef _G2_GORE

R2GoreTextureCoordinates::R2GoreTextureCoordinates()
{
	Com_Memset(tex, 0, sizeof(tex));
}

R2GoreTextureCoordinates::~R2GoreTextureCoordinates()
{
	for (int i = 0; i < MAX_LODS; i++)
	{
		if (tex[i])
		{
			ri.Z_Free(tex[i]->verts);
			tex[i]->verts = NULL;
			ri.Z_Free(tex[i]->indexes);
			tex[i]->indexes = NULL;
			ri.Z_Free(tex[i]);
			tex[i] = NULL;
		}
	}
}

#endif