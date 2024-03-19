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

// q_shared.c -- stateless support routines that are included in each code dll

#include "../game/common_headers.h"

/*
============
COM_SkipPath
============
*/
char* COM_SkipPath(char* pathname)
{
	char* last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

/*
============
COM_GetExtension
============
*/
const char* COM_GetExtension(const char* name)
{
	const char* dot = strrchr(name, '.'), * slash;
	if (dot && (!((slash = strrchr(name, '/'))) || slash < dot))
		return dot + 1;
	return "";
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(const char* in, char* out, int destsize)
{
	const char* dot = strrchr(in, '.'), * slash;
	if (dot && (!((slash = strrchr(in, '/'))) || slash < dot))
		destsize = destsize < dot - in + 1 ? destsize : dot - in + 1;

	if (in == out && destsize > 1)
		out[destsize - 1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char* in, const char* ext)
{
	const int inlen = strlen(in);
	const int extlen = strlen(ext);

	if (extlen <= inlen)
	{
		in += inlen - extlen;

		if (!Q_stricmp(in, ext))
			return qtrue;
	}

	return qfalse;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char* path, const int maxSize, const char* extension)
{
	const char* dot = strrchr(path, '.'), * slash;
	if (dot && (!((slash = strrchr(path, '/'))) || slash < dot))
		return;
	Q_strcat(path, maxSize, extension);
}

/*
============================================================================

PARSING

============================================================================
*/

static char com_token[MAX_TOKEN_CHARS];
//JLFCALLOUT MPNOTUSED
int parseDataCount = -1;
constexpr int MAX_PARSE_DATA = 5;
parseData_t parseData[MAX_PARSE_DATA];

void COM_ParseInit()
{
	memset(parseData, 0, sizeof parseData);
	parseDataCount = -1;
}

void COM_BeginParseSession()
{
	parseDataCount++;
#ifdef _DEBUG
	if (parseDataCount >= MAX_PARSE_DATA)
	{
		Com_Error(ERR_FATAL, "COM_BeginParseSession: cannot nest more than %d parsing sessions.\n", MAX_PARSE_DATA);
	}
#endif

	parseData[parseDataCount].com_lines = 1;
	parseData[parseDataCount].com_tokenline = 0;
}

void COM_EndParseSession()
{
	parseDataCount--;
#ifdef _DEBUG
	assert(parseDataCount >= -1 && "COM_EndParseSession: called without a starting COM_BeginParseSession.\n");
#endif
}

int COM_GetCurrentParseLine(void)
{
	if (parseDataCount < 0)
		Com_Error(ERR_FATAL, "COM_GetCurrentParseLine: parseDataCount < 0 (be sure to call COM_BeginParseSession!)");

	if (parseData[parseDataCount].com_tokenline)
		return parseData[parseDataCount].com_tokenline;

	return parseData[parseDataCount].com_lines;
}

char* COM_Parse(const char** data_p)
{
	return COM_ParseExt(data_p, qtrue);
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
const char* SkipWhitespace(const char* data, qboolean* hasNewLines)
{
	int c;

	if (parseDataCount < 0)
		Com_Error(ERR_FATAL, "SkipWhitespace: parseDataCount < 0");

	while ((c = *(const unsigned char* /*eurofix*/)data) <= ' ')
	{
		if (!c)
		{
			return nullptr;
		}
		if (c == '\n')
		{
			parseData[parseDataCount].com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress(char* data_p)
{
	char* out;
	qboolean newline = qfalse, whitespace = qfalse;

	char* in = out = data_p;
	if (in)
	{
		int c;
		while ((c = *in) != 0)
		{
			// skip double slash comments
			if (c == '/' && in[1] == '/')
			{
				while (*in && *in != '\n')
				{
					in++;
				}
				// skip /* */ comments
			}
			else if (c == '/' && in[1] == '*')
			{
				while (*in && (*in != '*' || in[1] != '/'))
					in++;
				if (*in)
					in += 2;
				// record when we hit a newline
			}
			else if (c == '\n' || c == '\r')
			{
				newline = qtrue;
				in++;
				// record when we hit whitespace
			}
			else if (c == ' ' || c == '\t')
			{
				whitespace = qtrue;
				in++;
				// an actual token
			}
			else
			{
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline)
				{
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				}
				if (whitespace)
				{
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"')
				{
					*out++ = c;
					in++;
					while (true)
					{
						c = *in;
						if (c && c != '"')
						{
							*out++ = c;
							in++;
						}
						else
						{
							break;
						}
					}
					if (c == '"')
					{
						*out++ = c;
						in++;
					}
				}
				else
				{
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

char* COM_ParseExt(const char** data_p, const qboolean allow_line_breaks)
{
	int c;
	qboolean hasNewLines = qfalse;

	const char* data = *data_p;
	int len = 0;
	com_token[0] = 0;
	if (parseDataCount >= 0)
		parseData[parseDataCount].com_tokenline = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = nullptr;
		return com_token;
	}

	if (parseDataCount < 0)
		Com_Error(ERR_FATAL, "COM_ParseExt: parseDataCount < 0 (be sure to call COM_BeginParseSession!)");

	while (true)
	{
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			*data_p = nullptr;
			return com_token;
		}
		if (hasNewLines && !allow_line_breaks)
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
			{
				if (*data == '\n')
				{
					parseData[parseDataCount].com_lines++;
				}
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// token starts on this line
	parseData[parseDataCount].com_tokenline = parseData[parseDataCount].com_lines;

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = const_cast<char*>(data);
				return com_token;
			}
			if (c == '\n')
			{
				parseData[parseDataCount].com_lines++;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32);

	com_token[len] = 0;

	*data_p = const_cast<char*>(data);
	return com_token;
}

/*
===============
COM_ParseString
===============
*/
qboolean COM_ParseString(const char** data, const char** s)
{
	*s = COM_ParseExt(data, qfalse);
	if (s[0] == nullptr)
	{
		Com_Printf("unexpected EOF in COM_ParseString\n");
		return qtrue;
	}
	return qfalse;
}

/*
===============
COM_ParseInt
===============
*/
qboolean COM_ParseInt(const char** data, int* i)
{
	const char* token = COM_ParseExt(data, qfalse);
	if (token[0] == 0)
	{
		Com_Printf("unexpected EOF in COM_ParseInt\n");
		return qtrue;
	}

	*i = atoi(token);
	return qfalse;
}

/*
===============
COM_ParseFloat
===============
*/
qboolean COM_ParseFloat(const char** data, float* f)
{
	const char* token = COM_ParseExt(data, qfalse);
	if (token[0] == 0)
	{
		Com_Printf("unexpected EOF in COM_ParseFloat\n");
		return qtrue;
	}

	*f = atof(token);
	return qfalse;
}

/*
===============
COM_ParseVec4
===============
*/
qboolean COM_ParseVec4(const char** buffer, vec4_t* c)
{
	float f;

	for (int i = 0; i < 4; i++)
	{
		if (COM_ParseFloat(buffer, &f))
		{
			return qtrue;
		}
		(*c)[i] = f;
	}
	return qfalse;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken(const char** buf_p, const char* match)
{
	const char* token = COM_Parse(buf_p);
	if (strcmp(token, match) != 0)
	{
		Com_Error(ERR_DROP, "MatchToken: %s != %s", token, match);
	}
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection(const char** program)
{
	int depth = 0;

	if (com_token[0] == '{')
	{
		//for tr_shader which just ate the brace
		depth = 1;
	}
	do
	{
		const char* token = COM_ParseExt(program, qtrue);
		if (token[1] == 0)
		{
			if (token[0] == '{')
			{
				depth++;
			}
			else if (token[0] == '}')
			{
				depth--;
			}
		}
	} while (depth && *program);
}

/*
=================
SkipBracedSection

The next token should be an open brace or set depth to 1 if already parsed it.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
qboolean SkipBracedSection(const char** program, int depth) {
	char* token;

	do {
		token = COM_ParseExt(program, qtrue);
		if (token[1] == 0) {
			if (token[0] == '{') {
				depth++;
			}
			else if (token[0] == '}') {
				depth--;
			}
		}
	} while (depth && *program);

	return (qboolean)(depth == 0);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine(const char** data)
{
	int c;

	if (parseDataCount < 0)
		Com_Error(ERR_FATAL, "SkipRestOfLine: parseDataCount < 0");

	const char* p = *data;

	if (!*p)
		return;

	while ((c = *p++) != 0)
	{
		if (c == '\n')
		{
			parseData[parseDataCount].com_lines++;
			break;
		}
	}

	*data = p;
}

void Parse1DMatrix(const char** buf_p, const int x, float* m)
{
	COM_MatchToken(buf_p, "(");

	for (int i = 0; i < x; i++)
	{
		const char* token = COM_Parse(buf_p);
		m[i] = atof(token);
	}

	COM_MatchToken(buf_p, ")");
}

void Parse2DMatrix(const char** buf_p, const int y, const int x, float* m)
{
	COM_MatchToken(buf_p, "(");

	for (int i = 0; i < y; i++)
	{
		Parse1DMatrix(buf_p, x, m + i * x);
	}

	COM_MatchToken(buf_p, ")");
}

void Parse3DMatrix(const char** buf_p, const int z, const int y, const int x, float* m)
{
	COM_MatchToken(buf_p, "(");

	for (int i = 0; i < z; i++)
	{
		Parse2DMatrix(buf_p, y, x, m + i * x * y);
	}

	COM_MatchToken(buf_p, ")");
}

/*
 ===================
 Com_HexStrToInt
 ===================
 */
int Com_HexStrToInt(const char* str)
{
	if (!str || !str[0])
		return -1;

	// check for hex code
	if (str[0] == '0' && str[1] == 'x')
	{
		size_t n = 0;

		for (size_t i = 2; i < strlen(str); i++)
		{
			n *= 16;

			char digit = tolower(str[i]);

			if (digit >= '0' && digit <= '9')
				digit -= '0';
			else if (digit >= 'a' && digit <= 'f')
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}

		return n;
	}

	return -1;
}

int Q_parseSaberColor(const char* p, float* color)
{
	char c = *p++;
	if (c >= 'a' && c < 'u' || c >= 'A' && c < 'U')
	{
		if (!color)
			return 1;
		const int deg = ((c | 32) - 'a') * 360 / 24;
		const float angle = DEG2RAD(deg % 120);
		const float v = (cos(angle) / cos(M_PI / 3 - angle) + 1) / 3;
		if (deg <= 120)
		{
			color[0] = v;
			color[1] = 1 - v;
			color[2] = 0;
		}
		else if (deg <= 240)
		{
			color[0] = 0;
			color[1] = v;
			color[2] = 1 - v;
		}
		else
		{
			color[0] = 1 - v;
			color[1] = 0;
			color[2] = v;
		}
		return 1;
	}
	if (c == 'u' || c == 'U' || c == 'v' || c == 'V'
		|| c == 'w' || c == 'W' || c == 'x' || c == 'X'
		|| c == 'y' || c == 'Y' || c == 'z' || c == 'Z')
	{
		int val = 0;
		for (int i = 0; i < 6; i++)
		{
			int readHex;
			c = p[i];
			if (c >= '0' && c <= '9')
			{
				readHex = c - '0';
			}
			else if (c >= 'a' && c <= 'f')
			{
				readHex = 0xa + c - 'a';
			}
			else if (c >= 'A' && c <= 'F')
			{
				readHex = 0xa + c - 'A';
			}
			else
			{
				if (color)
				{
					color[0] = color[1] = color[2] = 1.0f;
				}
				return 1;
			}
			if (!color)
				continue;
			if (i & 1)
			{
				val |= readHex;
				color[i >> 1] = val * (1 / 255.0f);
			}
			else
			{
				val = readHex << 4;
			}
		}
		return 7;
	}
	if (color)
	{
		color[0] = color[1] = color[2] = 1.0f;
	}
	return 0;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int QDECL Com_sprintf(char* dest, int size, const char* fmt, ...)
{
	va_list		argptr;

	va_start(argptr, fmt);
	const int len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	if (len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1);

	return len;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define	MAX_VA_STRING	32000
#define MAX_VA_BUFFERS 4

char* QDECL va(const char* format, ...)
{
	va_list		argptr;
	static char	string[MAX_VA_BUFFERS][MAX_VA_STRING];	// in case va is called by nested functions
	static int	index = 0;
	char* buf;

	va_start(argptr, format);
	buf = (char*)&string[index++ & 3];
	Q_vsnprintf(buf, sizeof(*string), format, argptr);
	va_end(argptr);

	return buf;
}

/*
============
Com_TruncateLongString

Assumes buffer is atleast TRUNCATE_LENGTH big
============
*/
void Com_TruncateLongString(char* buffer, const char* s)
{
	const int length = strlen(s);

	if (length <= TRUNCATE_LENGTH)
		Q_strncpyz(buffer, s, TRUNCATE_LENGTH);
	else
	{
		Q_strncpyz(buffer, s, TRUNCATE_LENGTH / 2 - 3);
		Q_strcat(buffer, TRUNCATE_LENGTH, " ... ");
		Q_strcat(buffer, TRUNCATE_LENGTH, s + length - TRUNCATE_LENGTH / 2 + 3);
	}
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
const char* Info_ValueForKey(const char* s, const char* key)
{
	// work without stomping on each other
	static int valueindex = 0;

	if (!s || !key)
	{
		return "";
	}

	if (strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring");
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (true)
	{
		static char value[2][MAX_INFO_VALUE];
		char pkey[MAX_INFO_KEY]{};
		char* o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair(const char** head, char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE])
{
	const char* s = *head;

	if (*s == '\\')
	{
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	char* o = key;
	while (*s != '\\')
	{
		if (!*s)
		{
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while (*s != '\\' && *s)
	{
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey(char* s, const char* key)
{
	if (strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring");
	}

	if (strchr(key, '\\'))
	{
		return;
	}

	while (true)
	{
		char value[MAX_INFO_VALUE]{};
		char pkey[MAX_INFO_KEY]{};
		char* start = s;
		if (*s == '\\')
			s++;
		char* o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		//OJKNOTE: static analysis pointed out pkey may not be null-terminated
		if (strcmp(key, pkey) == 0)
		{
			memmove(start, s, strlen(s) + 1); // remove this part
			return;
		}

		if (!*s)
			return;
	}
}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate(const char* s)
{
	if (strchr(s, '\"'))
	{
		return qfalse;
	}
	if (strchr(s, ';'))
	{
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey(char* s, const char* key, const char* value)
{
	char newi[MAX_INFO_STRING];
	auto blacklist = "\\;\"";

	if (strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring");
	}

	for (; *blacklist; ++blacklist)
	{
		if (strchr(key, *blacklist) || strchr(value, *blacklist))
		{
			Com_Printf(S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey(s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf(newi, sizeof newi, "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf("Info string length exceeded: %s\n", s);
		return;
	}

	strcat(newi, s);
	strcpy(s, newi);
}

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset(const char c, const char* set)
{
	for (size_t i = 0; i < strlen(set); i++)
	{
		if (set[i] == c)
			return qtrue;
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char* Com_SkipCharset(char* s, const char* sep)
{
	char* p = s;

	while (p)
	{
		if (Com_CharIsOneOfCharset(*p, sep))
			p++;
		else
			break;
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char* Com_SkipTokens(char* s, const int numTokens, const char* sep)
{
	int sepCount = 0;
	char* p = s;

	while (sepCount < numTokens)
	{
		if (Com_CharIsOneOfCharset(*p++, sep))
		{
			sepCount++;
			while (Com_CharIsOneOfCharset(*p, sep))
				p++;
		}
		else if (*p == '\0')
			break;
	}

	if (sepCount == numTokens)
		return p;
	return s;
}

/*
========================================================================

String ID Tables

========================================================================
*/

/*
-------------------------
GetIDForString
-------------------------
*/

int GetIDForString(const stringID_table_t* table, const char* string)
{
	int index = 0;

	while (VALIDSTRING(table[index].name))
	{
		if (!Q_stricmp(table[index].name, string))
			return table[index].id;

		index++;
	}

	return -1;
}

/*
-------------------------
GetStringForID
-------------------------
*/

const char* GetStringForID(const stringID_table_t* table, const int id)
{
	int index = 0;

	while (VALIDSTRING(table[index].name))
	{
		if (table[index].id == id)
			return table[index].name;

		index++;
	}

	return nullptr;
}

int Q_clampi(const int min, const int value, const int max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

qboolean Q_InBitflags(const uint32_t* bits, const int index, const uint32_t bitsPerByte)
{
	return bits[index / bitsPerByte] & 1 << index % bitsPerByte ? qtrue : qfalse;
}

void Q_AddToBitflags(uint32_t* bits, const int index, const uint32_t bitsPerByte)
{
	bits[index / bitsPerByte] |= 1 << index % bitsPerByte;
}

void Q_RemoveFromBitflags(uint32_t* bits, const int index, const uint32_t bitsPerByte)
{
	bits[index / bitsPerByte] &= ~(1 << index % bitsPerByte);
}

void* Q_LinearSearch(const void* key, const void* ptr, const size_t count,
	const size_t size, const cmpFunc_t cmp)
{
	for (size_t i = 0; i < count; i++)
	{
		if (cmp(key, ptr) == 0) return const_cast<void*>(ptr);
		ptr = static_cast<const char*>(ptr) + size;
	}
	return nullptr;
}