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

#ifndef __SAY_H__
#define __SAY_H__

using saying_t = enum //# saying_e
{
	//Acknowledge command
	SAY_ACKCOMM1,
	SAY_ACKCOMM2,
	SAY_ACKCOMM3,
	SAY_ACKCOMM4,
	//Refuse command
	SAY_REFCOMM1,
	SAY_REFCOMM2,
	SAY_REFCOMM3,
	SAY_REFCOMM4,
	//Bad command
	SAY_BADCOMM1,
	SAY_BADCOMM2,
	SAY_BADCOMM3,
	SAY_BADCOMM4,
	//Unfinished hail
	SAY_BADHAIL1,
	SAY_BADHAIL2,
	SAY_BADHAIL3,
	SAY_BADHAIL4,
	//# #eol
	NUM_SAYINGS
};

#endif //#ifndef __SAY_H__