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

/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2023 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#define _STR(x) #x
#define STR(x) _STR(x)

// Current version of the multi player game

#define VERSION_MAJOR_RELEASE		24  // Build year
#define VERSION_MINOR_RELEASE		04  // Build month
#define VERSION_INTERNAL_BUILD		01  // Build number

#define VERSION_STRING				"Day-11,Month-04,Year-24,BuildNum-01" // build date
#define VERSION_STRING_DOTTED		"Day-11,Month-04,Year-24,BuildNum-01" // build date

#if defined(_DEBUG)
#define	JK_VERSION		"(debug)SerenityJediEngine2025-MP: " VERSION_STRING_DOTTED
#define JK_VERSION_OLD	"(debug)SJE-mp: " VERSION_STRING_DOTTED
#else
#define	JK_VERSION		"SerenityJediEngine2025-MP: " VERSION_STRING_DOTTED
#define JK_VERSION_OLD	"SJE-mp: " VERSION_STRING_DOTTED
#endif
#endif // GAME_VERSION_H
