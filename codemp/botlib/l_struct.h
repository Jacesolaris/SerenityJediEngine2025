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

/*****************************************************************************
 * name:		l_struct.h
 *
 * desc:		structure reading/writing
 *
 * $Archive: /source/code/botlib/l_struct.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#pragma once

#define MAX_STRINGFIELD				80
 //field types
constexpr auto FT_CHAR = 1;			// char;
constexpr auto FT_INT = 2;			// int;
constexpr auto FT_FLOAT = 3;			// float;
constexpr auto FT_STRING = 4;			// char [MAX_STRINGFIELD];
constexpr auto FT_STRUCT = 6;			// struct (sub structure);
//type only mask
constexpr auto FT_TYPE = 0x00FF;// only type, clear subtype;
//sub types
constexpr auto FT_ARRAY = 0x0100;	// array of type;
constexpr auto FT_BOUNDED = 0x0200;	// bounded value;
constexpr auto FT_UNSIGNED = 0x0400;

//structure field definition
using fielddef_t = struct fielddef_s
{
	char* name;										//name of the field
	int offset;										//offset in the structure
	int type;										//type of the field
	//type specific fields
	int maxarray;									//maximum array size
	float floatmin, floatmax;					//float min and max
	struct structdef_s* substruct;			//sub structure
};

//structure definition
using structdef_t = struct structdef_s
{
	int size;
	fielddef_t* fields;
};

//read a structure from a script
int ReadStructure(source_t* source, structdef_t* def, char* structure);
//write a structure to a file
int WriteStructure(FILE* fp, structdef_t* def, char* structure);
//writes indents
int WriteIndent(FILE* fp, int indent);
//writes a float without traling zeros
int WriteFloat(FILE* fp, float value);
