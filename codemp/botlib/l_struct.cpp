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
 * name:		l_struct.c
 *
 * desc:		structure reading / writing
 *
 * $Archive: /MissionPack/CODE/botlib/l_struct.c $
 * $Author: Raduffy $
 * $Revision: 1 $
 * $Modtime: 12/20/99 8:43p $
 * $Date: 3/08/00 11:28a $
 *
 *****************************************************************************/

#ifdef BOTLIB
#include "qcommon/q_shared.h"
#include "botlib.h"				//for the include of be_interface.h
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "be_interface.h"
#endif //BOTLIB

#ifdef BSPC
 //include files for usage in the BSP Converter
#include "../bspc/qbsp.h"
#include "../bspc/l_log.h"
#include "../bspc/l_mem.h"
#include "l_precomp.h"
#include "l_struct.h"

#define qtrue	true
#define qfalse	false
#endif //BSPC

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
fielddef_t* FindField(fielddef_t* defs, const char* name)
{
	for (int i = 0; defs[i].name; i++)
	{
		if (strcmp(defs[i].name, name) == 0) return &defs[i];
	} //end for
	return nullptr;
} //end of the function FindField
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ReadNumber(source_t* source, const fielddef_t* fd, void* p)
{
	token_t token;
	int negative = qfalse;
	long int intmin = 0, intmax = 0;

	if (!PC_ExpectAnyToken(source, &token)) return static_cast<qboolean>(0);

	//check for minus sign
	if (token.type == TT_PUNCTUATION)
	{
		if (fd->type & FT_UNSIGNED)
		{
			SourceError(source, "expected unsigned value, found %s", token.string);
			return static_cast<qboolean>(0);
		} //end if
		//if not a minus sign
		if (strcmp(token.string, "-") != 0)
		{
			SourceError(source, "unexpected punctuation %s", token.string);
			return static_cast<qboolean>(0);
		} //end if
		negative = qtrue;
		//read the number
		if (!PC_ExpectAnyToken(source, &token)) return static_cast<qboolean>(0);
	} //end if
	//check if it is a number
	if (token.type != TT_NUMBER)
	{
		SourceError(source, "expected number, found %s", token.string);
		return static_cast<qboolean>(0);
	} //end if
	//check for a float value
	if (token.subtype & TT_FLOAT)
	{
		if ((fd->type & FT_TYPE) != FT_FLOAT)
		{
			SourceError(source, "unexpected float");
			return static_cast<qboolean>(0);
		} //end if
		double floatval = token.floatvalue;
		if (negative) floatval = -floatval;
		if (fd->type & FT_BOUNDED)
		{
			if (floatval < fd->floatmin || floatval > fd->floatmax)
			{
				SourceError(source, "float out of range [%f, %f]", fd->floatmin, fd->floatmax);
				return static_cast<qboolean>(0);
			} //end if
		} //end if
		*static_cast<float*>(p) = static_cast<float>(floatval);
		return static_cast<qboolean>(1);
	} //end if
	//
	long int intval = token.intvalue;
	if (negative) intval = -intval;
	//check bounds
	if ((fd->type & FT_TYPE) == FT_CHAR)
	{
		if (fd->type & FT_UNSIGNED) { intmin = 0; intmax = 255; }
		else { intmin = -128; intmax = 127; }
	} //end if
	if ((fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_UNSIGNED) { intmin = 0; intmax = 65535; }
		else { intmin = -32768; intmax = 32767; }
	} //end else if
	if ((fd->type & FT_TYPE) == FT_CHAR || (fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_BOUNDED)
		{
			intmin = Maximum(intmin, fd->floatmin);
			intmax = Minimum(intmax, fd->floatmax);
		} //end if
		if (intval < intmin || intval > intmax)
		{
			SourceError(source, "value %ld out of range [%ld, %ld]", intval, intmin, intmax);
			return static_cast<qboolean>(0);
		} //end if
	} //end if
	else if ((fd->type & FT_TYPE) == FT_FLOAT)
	{
		if (fd->type & FT_BOUNDED)
		{
			if (intval < fd->floatmin || intval > fd->floatmax)
			{
				SourceError(source, "value %ld out of range [%f, %f]", intval, fd->floatmin, fd->floatmax);
				return static_cast<qboolean>(0);
			} //end if
		} //end if
	} //end else if
	//store the value
	if ((fd->type & FT_TYPE) == FT_CHAR)
	{
		if (fd->type & FT_UNSIGNED) *static_cast<unsigned char*>(p) = static_cast<unsigned char>(intval);
		else *static_cast<char*>(p) = static_cast<char>(intval);
	} //end if
	else if ((fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_UNSIGNED) *static_cast<unsigned*>(p) = static_cast<unsigned>(intval);
		else *static_cast<int*>(p) = static_cast<int>(intval);
	} //end else
	else if ((fd->type & FT_TYPE) == FT_FLOAT)
	{
		*static_cast<float*>(p) = static_cast<float>(intval);
	} //end else
	return static_cast<qboolean>(1);
} //end of the function ReadNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ReadChar(source_t* source, const fielddef_t* fd, void* p)
{
	token_t token;

	if (!PC_ExpectAnyToken(source, &token)) return static_cast<qboolean>(0);

	//take literals into account
	if (token.type == TT_LITERAL)
	{
		StripSingleQuotes(token.string);
		*static_cast<char*>(p) = token.string[0];
	} //end if
	else
	{
		PC_UnreadLastToken(source);
		if (!ReadNumber(source, fd, p)) return static_cast<qboolean>(0);
	} //end if
	return static_cast<qboolean>(1);
} //end of the function ReadChar
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int ReadString(source_t* source, fielddef_t* fd, void* p)
{
	token_t token;

	if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) return 0;
	//remove the double quotes
	StripDoubleQuotes(token.string);
	//copy the string
	strncpy(static_cast<char*>(p), token.string, MAX_STRINGFIELD);
	//make sure the string is closed with a zero
	static_cast<char*>(p)[MAX_STRINGFIELD - 1] = '\0';
	//
	return 1;
} //end of the function ReadString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int ReadStructure(source_t* source, structdef_t* def, char* structure)
{
	token_t token;
	int num;

	if (!PC_ExpectTokenString(source, "{")) return 0;
	while (true)
	{
		if (!PC_ExpectAnyToken(source, &token)) return qfalse;
		//if end of structure
		if (strcmp(token.string, "}") == 0) break;
		//find the field with the name
		fielddef_t* fd = FindField(def->fields, token.string);
		if (!fd)
		{
			SourceError(source, "unknown structure field %s", token.string);
			return qfalse;
		} //end if
		if (fd->type & FT_ARRAY)
		{
			num = fd->maxarray;
			if (!PC_ExpectTokenString(source, "{")) return qfalse;
		} //end if
		else
		{
			num = 1;
		} //end else
		void* p = structure + fd->offset;
		while (num-- > 0)
		{
			if (fd->type & FT_ARRAY)
			{
				if (PC_CheckTokenString(source, "}")) break;
			} //end if
			switch (fd->type & FT_TYPE)
			{
			case FT_CHAR:
			{
				if (!ReadChar(source, fd, p)) return qfalse;
				p = static_cast<char*>(p) + sizeof(char);
				break;
			} //end case
			case FT_INT:
			{
				if (!ReadNumber(source, fd, p)) return qfalse;
				p = static_cast<char*>(p) + sizeof(int);
				break;
			} //end case
			case FT_FLOAT:
			{
				if (!ReadNumber(source, fd, p)) return qfalse;
				p = static_cast<char*>(p) + sizeof(float);
				break;
			} //end case
			case FT_STRING:
			{
				if (!ReadString(source, fd, p)) return qfalse;
				p = static_cast<char*>(p) + MAX_STRINGFIELD;
				break;
			} //end case
			case FT_STRUCT:
			{
				if (!fd->substruct)
				{
					SourceError(source, "BUG: no sub structure defined");
					return qfalse;
				} //end if
				ReadStructure(source, fd->substruct, static_cast<char*>(p));
				p = static_cast<char*>(p) + fd->substruct->size;
				break;
			} //end case
			default:;
			} //end switch
			if (fd->type & FT_ARRAY)
			{
				if (!PC_ExpectAnyToken(source, &token)) return qfalse;
				if (strcmp(token.string, "}") == 0) break;
				if (strcmp(token.string, ",") != 0)
				{
					SourceError(source, "expected a comma, found %s", token.string);
					return qfalse;
				} //end if
			} //end if
		} //end while
	} //end while
	return qtrue;
} //end of the function ReadStructure
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int WriteIndent(FILE* fp, int indent)
{
	while (indent-- > 0)
	{
		if (fprintf(fp, "\t") < 0) return qfalse;
	} //end while
	return qtrue;
} //end of the function WriteIndent
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int WriteFloat(FILE* fp, const float value)
{
	char buf[128];

	Com_sprintf(buf, sizeof buf, "%f", value);
	int l = strlen(buf);
	//strip any trailing zeros
	while (l-- > 1)
	{
		if (buf[l] != '0' && buf[l] != '.') break;
		if (buf[l] == '.')
		{
			buf[l] = 0;
			break;
		} //end if
		buf[l] = 0;
	} //end while
	//write the float to file
	if (fprintf(fp, "%s", buf) < 0) return 0;
	return 1;
} //end of the function WriteFloat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int WriteStructWithIndent(FILE* fp, structdef_t* def, char* structure, int indent)
{
	int num;

	if (!WriteIndent(fp, indent)) return qfalse;
	if (fprintf(fp, "{\r\n") < 0) return qfalse;

	indent++;
	for (int i = 0; def->fields[i].name; i++)
	{
		const fielddef_t* fd = &def->fields[i];
		if (!WriteIndent(fp, indent)) return qfalse;
		if (fprintf(fp, "%s\t", fd->name) < 0) return qfalse;
		void* p = structure + fd->offset;
		if (fd->type & FT_ARRAY)
		{
			num = fd->maxarray;
			if (fprintf(fp, "{") < 0) return qfalse;
		} //end if
		else
		{
			num = 1;
		} //end else
		while (num-- > 0)
		{
			switch (fd->type & FT_TYPE)
			{
			case FT_CHAR:
			{
				if (fprintf(fp, "%d", *static_cast<char*>(p)) < 0) return qfalse;
				p = static_cast<char*>(p) + sizeof(char);
				break;
			} //end case
			case FT_INT:
			{
				if (fprintf(fp, "%d", *static_cast<int*>(p)) < 0) return qfalse;
				p = static_cast<char*>(p) + sizeof(int);
				break;
			} //end case
			case FT_FLOAT:
			{
				if (!WriteFloat(fp, *static_cast<float*>(p))) return qfalse;
				p = static_cast<char*>(p) + sizeof(float);
				break;
			} //end case
			case FT_STRING:
			{
				if (fprintf(fp, "\"%s\"", static_cast<char*>(p)) < 0) return qfalse;
				p = static_cast<char*>(p) + MAX_STRINGFIELD;
				break;
			} //end case
			case FT_STRUCT:
			{
				if (!WriteStructWithIndent(fp, fd->substruct, structure, indent)) return qfalse;
				p = static_cast<char*>(p) + fd->substruct->size;
				break;
			} //end case
			default:;
			} //end switch
			if (fd->type & FT_ARRAY)
			{
				if (num > 0)
				{
					if (fprintf(fp, ",") < 0) return qfalse;
				} //end if
				else
				{
					if (fprintf(fp, "}") < 0) return qfalse;
				} //end else
			} //end if
		} //end while
		if (fprintf(fp, "\r\n") < 0) return qfalse;
	} //end for
	indent--;

	if (!WriteIndent(fp, indent)) return qfalse;
	if (fprintf(fp, "}\r\n") < 0) return qfalse;
	return qtrue;
} //end of the function WriteStructWithIndent
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int WriteStructure(FILE* fp, structdef_t* def, char* structure)
{
	return WriteStructWithIndent(fp, def, structure, 0);
} //end of the function WriteStructure