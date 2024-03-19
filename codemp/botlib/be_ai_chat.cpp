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
 * name:		be_ai_chat.c
 *
 * desc:		bot chat AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_chat.c $
 * $Author: Ttimo $
 * $Revision: 12 $
 * $Modtime: 4/13/01 4:45p $
 * $Date: 4/13/01 4:45p $
 *
 *****************************************************************************/

#include "qcommon/q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ea.h"
#include "be_ai_chat.h"

 //escape character
#define ESCAPE_CHAR				0x01	//'_'
//
// "hi ", people, " ", 0, " entered the game"
//becomes:
// "hi _rpeople_ _v0_ entered the game"
//

//match piece types
constexpr auto MT_VARIABLE = 1;		//variable match piece;
constexpr auto MT_STRING = 2;		//string match piece;
//reply chat key flags
constexpr auto RCKFL_AND = 1;		//key must be present;
constexpr auto RCKFL_NOT = 2;		//key must be absent;
constexpr auto RCKFL_NAME = 4;		//name of bot must be present;
constexpr auto RCKFL_STRING = 8;		//key is a string;
constexpr auto RCKFL_VARIABLES = 16;		//key is a match template;
constexpr auto RCKFL_BOTNAMES = 32;		//key is a series of botnames;
constexpr auto RCKFL_GENDERFEMALE = 64;		//bot must be female;
constexpr auto RCKFL_GENDERMALE = 128;		//bot must be male;
constexpr auto RCKFL_GENDERLESS = 256;		//bot must be genderless;
//time to ignore a chat message after using it
constexpr auto CHATMESSAGE_RECENTTIME = 20;

//the actuall chat messages
using bot_chatmessage_t = struct bot_chatmessage_s
{
	char* chatmessage;					//chat message string
	float time;							//last time used
	bot_chatmessage_s* next;		//next chat message in a list
};
//bot chat type with chat lines
using bot_chattype_t = struct bot_chattype_s
{
	char name[MAX_CHATTYPE_NAME];
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_chattype_s* next;
};
//bot chat lines
using bot_chat_t = struct bot_chat_s
{
	bot_chattype_t* types;
};

//random string
using bot_randomstring_t = struct bot_randomstring_s
{
	char* string;
	bot_randomstring_s* next;
};
//list with random strings
using bot_randomlist_t = struct bot_randomlist_s
{
	char* string;
	int numstrings;
	bot_randomstring_t* firstrandomstring;
	bot_randomlist_s* next;
};

//synonym
using bot_synonym_t = struct bot_synonym_s
{
	char* string;
	float weight;
	bot_synonym_s* next;
};
//list with synonyms
using bot_synonymlist_t = struct bot_synonymlist_s
{
	unsigned long int context;
	float totalweight;
	bot_synonym_t* firstsynonym;
	bot_synonymlist_s* next;
};

//fixed match string
using bot_matchstring_t = struct bot_matchstring_s
{
	char* string;
	bot_matchstring_s* next;
};

//piece of a match template
using bot_matchpiece_t = struct bot_matchpiece_s
{
	int type;
	bot_matchstring_t* firststring;
	int variable;
	bot_matchpiece_s* next;
};
//match template
using bot_matchtemplate_t = struct bot_matchtemplate_s
{
	unsigned long int context;
	int type;
	int subtype;
	bot_matchpiece_t* first;
	bot_matchtemplate_s* next;
};

//reply chat key
using bot_replychatkey_t = struct bot_replychatkey_s
{
	int flags;
	char* string;
	bot_matchpiece_t* match;
	bot_replychatkey_s* next;
};
//reply chat
using bot_replychat_t = struct bot_replychat_s
{
	bot_replychatkey_t* keys;
	float priority;
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_replychat_s* next;
};

//string list
using bot_stringlist_t = struct bot_stringlist_s
{
	char* string;
	bot_stringlist_s* next;
};

//chat state of a bot
using bot_chatstate_t = struct bot_chatstate_s
{
	int gender;											//0=it, 1=female, 2=male
	int client;											//client number
	char name[32];										//name of the bot
	char chatmessage[MAX_MESSAGE_SIZE];
	int handle;
	//the console messages visible to the bot
	bot_consolemessage_t* firstmessage;			//first message is the first typed message
	bot_consolemessage_t* lastmessage;			//last message is the last typed message, bottom of console
	//number of console messages stored in the state
	int numconsolemessages;
	//the bot chat lines
	bot_chat_t* chat;
};

using bot_ichatdata_t = struct bot_ichatdata_s {
	bot_chat_t* chat;
	char		filename[MAX_QPATH];
	char		chatname[MAX_QPATH];
};

bot_ichatdata_t* ichatdata[MAX_CLIENTS];

bot_chatstate_t* botchatstates[MAX_CLIENTS + 1];
//console message heap
bot_consolemessage_t* consolemessageheap = nullptr;
bot_consolemessage_t* freeconsolemessages = nullptr;
//list with match strings
bot_matchtemplate_t* matchtemplates = nullptr;
//list with synonyms
bot_synonymlist_t* synonyms = nullptr;
//list with random strings
bot_randomlist_t* randomstrings = nullptr;
//reply chats
bot_replychat_t* replychats = nullptr;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
bot_chatstate_t* BotChatStateFromHandle(const int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
		return nullptr;
	} //end if
	if (!botchatstates[handle])
	{
		botimport.Print(PRT_FATAL, "invalid chat state %d\n", handle);
		return nullptr;
	} //end if
	return botchatstates[handle];
} //end of the function BotChatStateFromHandle
//===========================================================================
// initialize the heap with unused console messages
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void InitConsoleMessageHeap(void)
{
	if (consolemessageheap) FreeMemory(consolemessageheap);
	//
	const int max_messages = static_cast<int>(LibVarValue("max_messages", "1024"));
	consolemessageheap = static_cast<bot_consolemessage_t*>(GetClearedHunkMemory(max_messages *
		sizeof(bot_consolemessage_t)));
	consolemessageheap[0].prev = nullptr;
	consolemessageheap[0].next = &consolemessageheap[1];
	for (int i = 1; i < max_messages - 1; i++)
	{
		consolemessageheap[i].prev = &consolemessageheap[i - 1];
		consolemessageheap[i].next = &consolemessageheap[i + 1];
	} //end for
	consolemessageheap[max_messages - 1].prev = &consolemessageheap[max_messages - 2];
	consolemessageheap[max_messages - 1].next = nullptr;
	//pointer to the free console messages
	freeconsolemessages = consolemessageheap;
} //end of the function InitConsoleMessageHeap
//===========================================================================
// allocate one console message from the heap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_consolemessage_t* AllocConsoleMessage(void)
{
	bot_consolemessage_t* message = freeconsolemessages;
	if (freeconsolemessages) freeconsolemessages = freeconsolemessages->next;
	if (freeconsolemessages) freeconsolemessages->prev = nullptr;
	return message;
} //end of the function AllocConsoleMessage
//===========================================================================
// deallocate one console message from the heap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void FreeConsoleMessage(bot_consolemessage_t* message)
{
	if (freeconsolemessages) freeconsolemessages->prev = message;
	message->prev = nullptr;
	message->next = freeconsolemessages;
	freeconsolemessages = message;
} //end of the function FreeConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotRemoveConsoleMessage(const int chatstate, const int handle)
{
	bot_consolemessage_t* nextm;

	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;

	for (bot_consolemessage_t* m = cs->firstmessage; m; m = nextm)
	{
		nextm = m->next;
		if (m->handle == handle)
		{
			if (m->next) m->next->prev = m->prev;
			else cs->lastmessage = m->prev;
			if (m->prev) m->prev->next = m->next;
			else cs->firstmessage = m->next;

			FreeConsoleMessage(m);
			cs->numconsolemessages--;
			break;
		} //end if
	} //end for
} //end of the function BotRemoveConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotQueueConsoleMessage(const int chatstate, const int type, char* message)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;

	bot_consolemessage_t* m = AllocConsoleMessage();
	if (!m)
	{
		botimport.Print(PRT_ERROR, "empty console message heap\n");
		return;
	} //end if
	cs->handle++;
	if (cs->handle <= 0 || cs->handle > 8192) cs->handle = 1;
	m->handle = cs->handle;
	m->time = AAS_Time();
	m->type = type;
	strncpy(m->message, message, MAX_MESSAGE_SIZE);
	m->next = nullptr;
	if (cs->lastmessage)
	{
		cs->lastmessage->next = m;
		m->prev = cs->lastmessage;
		cs->lastmessage = m;
	} //end if
	else
	{
		cs->lastmessage = m;
		cs->firstmessage = m;
		m->prev = nullptr;
	} //end if
	cs->numconsolemessages++;
} //end of the function BotQueueConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNextConsoleMessage(const int chatstate, bot_consolemessage_t* cm)
{
	bot_consolemessage_t* firstmsg;

	const bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return 0;
	if ((firstmsg = cs->firstmessage))
	{
		cm->handle = firstmsg->handle;
		cm->time = firstmsg->time;
		cm->type = firstmsg->type;
		Q_strncpyz(cm->message, firstmsg->message,
			sizeof cm->message);

		/* We omit setting the two pointers in cm because pointer
		 * size in the VM differs between the size in the engine on
		 * 64 bit machines, which would lead to a buffer overflow if
		 * this functions is called from the VM. The pointers are
		 * of no interest to functions calling
		 * BotNextConsoleMessage anyways.
		 */

		return cm->handle;
	} //end if
	return 0;
} //end of the function BotConsoleMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNumConsoleMessages(const int chatstate)
{
	const bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return 0;
	return cs->numconsolemessages;
} //end of the function BotNumConsoleMessages
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int IsWhiteSpace(const char c)
{
	if (c >= 'a' && c <= 'z'
		|| c >= 'A' && c <= 'Z'
		|| c >= '0' && c <= '9'
		|| c == '(' || c == ')'
		|| c == '?' || c == ':'
		|| c == '\'' || c == '/'
		|| c == ',' || c == '.'
		|| c == '[' || c == ']'
		|| c == '-' || c == '_'
		|| c == '+' || c == '=') return qfalse;
	return qtrue;
} //end of the function IsWhiteSpace
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static void BotRemoveTildes(char* message)
{
	//remove all tildes from the chat message
	for (int i = 0; message[i]; i++)
	{
		if (message[i] == '~')
		{
			memmove(&message[i], &message[i + 1], strlen(&message[i + 1]) + 1);
		} //end if
	} //end for
} //end of the function BotRemoveTildes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void UnifyWhiteSpaces(char* string)
{
	char* oldptr;

	for (char* ptr = oldptr = string; *ptr; oldptr = ptr)
	{
		while (*ptr && IsWhiteSpace(*ptr)) ptr++;
		if (ptr > oldptr)
		{
			//if not at the start and not at the end of the string
			//write only one space
			if (oldptr > string && *ptr) *oldptr++ = ' ';
			//remove all other white spaces
			if (ptr > oldptr) memmove(oldptr, ptr, strlen(ptr) + 1);
		} //end if
		while (*ptr && !IsWhiteSpace(*ptr)) ptr++;
	} //end while
} //end of the function UnifyWhiteSpaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int StringContains(char* str1, char* str2, const int casesensitive)
{
	int j;

	if (str1 == nullptr || str2 == nullptr) return -1;

	const int len = strlen(str1) - strlen(str2);
	int index = 0;
	for (int i = 0; i <= len; i++, str1++, index++)
	{
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j]) break;
			} //end if
			else
			{
				if (toupper(str1[j]) != toupper(str2[j])) break;
			} //end else
		} //end for
		if (!str2[j]) return index;
	} //end for
	return -1;
} //end of the function StringContains
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static char* StringContainsWord(char* str1, char* str2, const int casesensitive)
{
	int j;

	const int len = strlen(str1) - strlen(str2);
	for (int i = 0; i <= len; i++, str1++)
	{
		//if not at the start of the string
		if (i)
		{
			//skip to the start of the next word
			while (*str1 && *str1 != ' ' && *str1 != '.' && *str1 != ',' && *str1 != '!') str1++;
			if (!*str1) break;
			str1++;
		} //end for
		//compare the word
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j]) break;
			} //end if
			else
			{
				if (toupper(str1[j]) != toupper(str2[j])) break;
			} //end else
		} //end for
		//if there was a word match
		if (!str2[j])
		{
			//if the first string has an end of word
			if (!str1[j] || str1[j] == ' ' || str1[j] == '.' || str1[j] == ',' || str1[j] == '!') return str1;
		} //end if
	} //end for
	return nullptr;
} //end of the function StringContainsWord
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void StringReplaceWords(char* string, char* synonym, char* replacement)
{
	//find the synonym in the string
	char* str = StringContainsWord(string, synonym, qfalse);
	//if the synonym occured in the string
	while (str)
	{
		//if the synonym isn't part of the replacement which is already in the string
		//usefull for abreviations
		char* str2 = StringContainsWord(string, replacement, qfalse);
		while (str2)
		{
			if (str2 <= str && str < str2 + strlen(replacement)) break;
			str2 = StringContainsWord(str2 + 1, replacement, qfalse);
		} //end while
		if (!str2)
		{
			memmove(str + strlen(replacement), str + strlen(synonym), strlen(str + strlen(synonym)) + 1);
			//append the synonum replacement
			Com_Memcpy(str, replacement, strlen(replacement));
		} //end if
		//find the next synonym in the string
		str = StringContainsWord(str + strlen(replacement), synonym, qfalse);
	} //end if
} //end of the function StringReplaceWords
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpSynonymList(bot_synonymlist_t* synlist)
{
	FILE* fp = Log_FilePointer();
	if (!fp) return;
	for (const bot_synonymlist_t* syn = synlist; syn; syn = syn->next)
	{
		fprintf(fp, "%ld : [", syn->context);
		for (const bot_synonym_t* synonym = syn->firstsynonym; synonym; synonym = synonym->next)
		{
			fprintf(fp, "(\"%s\", %1.2f)", synonym->string, synonym->weight);
			if (synonym->next) fprintf(fp, ", ");
		} //end for
		fprintf(fp, "]\n");
	} //end for
} //end of the function BotDumpSynonymList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_synonymlist_t* BotLoadSynonyms(char* filename)
{
	unsigned long int contextstack[32];
	char* ptr = nullptr;
	token_t token;

	int size = 0;
	bot_synonymlist_t* synlist = nullptr; //make compiler happy
	bot_synonymlist_t* syn = nullptr; //make compiler happy
	bot_synonym_t* synonym = nullptr; //make compiler happy
	//the synonyms are parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		//
		if (pass && size) ptr = static_cast<char*>(GetClearedHunkMemory(size));
		//
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
		source_t* source = LoadSourceFile(filename);
		if (!source)
		{
			botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
			return nullptr;
		} //end if
		//
		unsigned long int context = 0;
		int contextlevel = 0;
		synlist = nullptr; //list synonyms
		bot_synonymlist_t* lastsyn = nullptr; //last synonym in the list
		//
		while (PC_ReadToken(source, &token))
		{
			if (token.type == TT_NUMBER)
			{
				context |= token.intvalue;
				contextstack[contextlevel] = token.intvalue;
				contextlevel++;
				if (contextlevel >= 32)
				{
					SourceError(source, "more than 32 context levels");
					FreeSource(source);
					return nullptr;
				} //end if
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return nullptr;
				} //end if
			} //end if
			else if (token.type == TT_PUNCTUATION)
			{
				if (strcmp(token.string, "}") == 0)
				{
					contextlevel--;
					if (contextlevel < 0)
					{
						SourceError(source, "too many }");
						FreeSource(source);
						return nullptr;
					} //end if
					context &= ~contextstack[contextlevel];
				} //end if
				else if (strcmp(token.string, "[") == 0)
				{
					size += sizeof(bot_synonymlist_t);
					if (pass)
					{
						syn = reinterpret_cast<bot_synonymlist_t*>(ptr);
						ptr += sizeof(bot_synonymlist_t);
						syn->context = context;
						syn->firstsynonym = nullptr;
						syn->next = nullptr;
						if (lastsyn) lastsyn->next = syn;
						else synlist = syn;
						lastsyn = syn;
					} //end if
					int numsynonyms = 0;
					bot_synonym_t* lastsynonym = nullptr;
					while (true)
					{
						if (!PC_ExpectTokenString(source, "(") ||
							!PC_ExpectTokenType(source, TT_STRING, 0, &token))
						{
							FreeSource(source);
							return nullptr;
						} //end if
						StripDoubleQuotes(token.string);
						if (strlen(token.string) <= 0)
						{
							SourceError(source, "empty string");
							FreeSource(source);
							return nullptr;
						} //end if
						size_t len = strlen(token.string) + 1;
						len = PAD(len, sizeof(long));
						size += sizeof(bot_synonym_t) + len;
						if (pass)
						{
							synonym = reinterpret_cast<bot_synonym_t*>(ptr);
							ptr += sizeof(bot_synonym_t);
							synonym->string = ptr;
							ptr += len;
							strcpy(synonym->string, token.string);
							//
							if (lastsynonym) lastsynonym->next = synonym;
							else syn->firstsynonym = synonym;
							lastsynonym = synonym;
						} //end if
						numsynonyms++;
						if (!PC_ExpectTokenString(source, ",") ||
							!PC_ExpectTokenType(source, TT_NUMBER, 0, &token) ||
							!PC_ExpectTokenString(source, ")"))
						{
							FreeSource(source);
							return nullptr;
						} //end if
						if (pass)
						{
							synonym->weight = token.floatvalue;
							syn->totalweight += synonym->weight;
						} //end if
						if (PC_CheckTokenString(source, "]")) break;
						if (!PC_ExpectTokenString(source, ","))
						{
							FreeSource(source);
							return nullptr;
						} //end if
					} //end while
					if (numsynonyms < 2)
					{
						SourceError(source, "synonym must have at least two entries");
						FreeSource(source);
						return nullptr;
					} //end if
				} //end else
				else
				{
					SourceError(source, "unexpected %s", token.string);
					FreeSource(source);
					return nullptr;
				} //end if
			} //end else if
		} //end while
		//
		FreeSource(source);
		//
		if (contextlevel > 0)
		{
			SourceError(source, "missing }");
			return nullptr;
		} //end if
	} //end for
	botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
	//
	//BotDumpSynonymList(synlist);
	//
	return synlist;
} //end of the function BotLoadSynonyms
//===========================================================================
// replace all the synonyms in the string
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceSynonyms(char* string, const unsigned long int context)
{
	for (const bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context)) continue;
		for (const bot_synonym_t* synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
		{
			StringReplaceWords(string, synonym->string, syn->firstsynonym->string);
		} //end for
	} //end for
} //end of the function BotReplaceSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotReplaceWeightedSynonyms(char* string, const unsigned long int context)
{
	bot_synonym_t* replacement;

	for (const bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context)) continue;
		//choose a weighted random replacement synonym
		const float weight = Q_flrand(0.0f, 1.0f) * syn->totalweight;
		if (!weight) continue;
		float curweight = 0;
		for (replacement = syn->firstsynonym; replacement; replacement = replacement->next)
		{
			curweight += replacement->weight;
			if (weight < curweight) break;
		} //end for
		if (!replacement) continue;
		//replace all synonyms with the replacement
		for (const bot_synonym_t* synonym = syn->firstsynonym; synonym; synonym = synonym->next)
		{
			if (synonym == replacement) continue;
			StringReplaceWords(string, synonym->string, replacement->string);
		} //end for
	} //end for
} //end of the function BotReplaceWeightedSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotReplaceReplySynonyms(char* string, const unsigned long int context)
{
	bot_synonym_t* synonym;

	for (char* str1 = string; *str1; )
	{
		//go to the start of the next word
		while (*str1 && *str1 <= ' ') str1++;
		if (!*str1) break;
		//
		for (const bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
		{
			if (!(syn->context & context)) continue;
			for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
			{
				//if the synonym is not at the front of the string continue
				const char* str2 = StringContainsWord(str1, synonym->string, qfalse);
				if (!str2 || str2 != str1) continue;
				//
				char* replacement = syn->firstsynonym->string;
				//if the replacement IS in front of the string continue
				str2 = StringContainsWord(str1, replacement, qfalse);
				if (str2 && str2 == str1) continue;
				//
				memmove(str1 + strlen(replacement), str1 + strlen(synonym->string),
					strlen(str1 + strlen(synonym->string)) + 1);
				//append the synonum replacement
				Com_Memcpy(str1, replacement, strlen(replacement));
				//
				break;
			} //end for
			//if a synonym has been replaced
			if (synonym) break;
		} //end for
		//skip over this word
		while (*str1 && *str1 > ' ') str1++;
		if (!*str1) break;
	} //end while
} //end of the function BotReplaceReplySynonyms
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static int BotLoadChatMessage(source_t* source, char* chatmessagestring)
{
	token_t token;

	char* ptr = chatmessagestring;
	*ptr = 0;
	//
	while (true)
	{
		if (!PC_ExpectAnyToken(source, &token)) return qfalse;
		//fixed string
		if (token.type == TT_STRING)
		{
			StripDoubleQuotes(token.string);
			if (strlen(ptr) + strlen(token.string) + 1 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long");
				return qfalse;
			} //end if
			strcat(ptr, token.string);
		} //end else if
		//variable string
		else if (token.type == TT_NUMBER && token.subtype & TT_INTEGER)
		{
			if (strlen(ptr) + 7 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long");
				return qfalse;
			} //end if
			sprintf(&ptr[strlen(ptr)], "%cv%ld%c", ESCAPE_CHAR, token.intvalue, ESCAPE_CHAR);
		} //end if
		//random string
		else if (token.type == TT_NAME)
		{
			if (strlen(ptr) + 7 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long");
				return qfalse;
			} //end if
			sprintf(&ptr[strlen(ptr)], "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR);
		} //end else if
		else
		{
			SourceError(source, "unknown message component %s", token.string);
			return qfalse;
		} //end else
		if (PC_CheckTokenString(source, ";")) break;
		if (!PC_ExpectTokenString(source, ",")) return qfalse;
	} //end while
	//
	return qtrue;
} //end of the function BotLoadChatMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpRandomStringList(bot_randomlist_t* randomlist)
{
	FILE* fp = Log_FilePointer();
	if (!fp) return;
	for (const bot_randomlist_t* random = randomlist; random; random = random->next)
	{
		fprintf(fp, "%s = {", random->string);
		for (const bot_randomstring_t* rs = random->firstrandomstring; rs; rs = rs->next)
		{
			fprintf(fp, "\"%s\"", rs->string);
			if (rs->next) fprintf(fp, ", ");
			else fprintf(fp, "}\n");
		} //end for
	} //end for
} //end of the function BotDumpRandomStringList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_randomlist_t* BotLoadRandomStrings(char* filename)
{
	int size;
	char* ptr = nullptr;
	source_t* source;
	token_t token;
	bot_randomlist_t* randomlist, * lastrandom, * random;
	bot_randomstring_t* randomstring;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif //DEBUG

	size = 0;
	randomlist = nullptr;
	random = nullptr;
	//the synonyms are parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		//
		if (pass && size) ptr = static_cast<char*>(GetClearedHunkMemory(size));
		//
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
		source = LoadSourceFile(filename);
		if (!source)
		{
			botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
			return nullptr;
		} //end if
		//
		randomlist = nullptr; //list
		lastrandom = nullptr; //last
		//
		while (PC_ReadToken(source, &token))
		{
			if (token.type != TT_NAME)
			{
				SourceError(source, "unknown random %s", token.string);
				FreeSource(source);
				return nullptr;
			} //end if
			size_t len = strlen(token.string) + 1;
			len = PAD(len, sizeof(long));
			size += sizeof(bot_randomlist_t) + len;
			if (pass)
			{
				random = reinterpret_cast<bot_randomlist_t*>(ptr);
				ptr += sizeof(bot_randomlist_t);
				random->string = ptr;
				ptr += len;
				strcpy(random->string, token.string);
				random->firstrandomstring = nullptr;
				random->numstrings = 0;
				//
				if (lastrandom) lastrandom->next = random;
				else randomlist = random;
				lastrandom = random;
			} //end if
			if (!PC_ExpectTokenString(source, "=") ||
				!PC_ExpectTokenString(source, "{"))
			{
				FreeSource(source);
				return nullptr;
			} //end if
			while (!PC_CheckTokenString(source, "}"))
			{
				char chatmessagestring[MAX_MESSAGE_SIZE];
				if (!BotLoadChatMessage(source, chatmessagestring))
				{
					FreeSource(source);
					return nullptr;
				} //end if
				size_t len1 = strlen(chatmessagestring) + 1;
				len1 = PAD(len1, sizeof(long));
				size += sizeof(bot_randomstring_t) + len1;
				if (pass)
				{
					randomstring = reinterpret_cast<bot_randomstring_t*>(ptr);
					ptr += sizeof(bot_randomstring_t);
					randomstring->string = ptr;
					ptr += len1;
					strcpy(randomstring->string, chatmessagestring);
					//
					random->numstrings++;
					randomstring->next = random->firstrandomstring;
					random->firstrandomstring = randomstring;
				} //end if
			} //end while
		} //end while
		//free the source after one pass
		FreeSource(source);
	} //end for
	botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
	//
#ifdef DEBUG
	botimport.Print(PRT_MESSAGE, "random strings %d msec\n", Sys_MilliSeconds() - starttime);
	//BotDumpRandomStringList(randomlist);
#endif //DEBUG
	//
	return randomlist;
} //end of the function BotLoadRandomStrings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static char* RandomString(const char* name)
{
	bot_randomstring_t* rs;

	for (const bot_randomlist_t* random = randomstrings; random; random = random->next)
	{
		if (strcmp(random->string, name) == 0)
		{
			int i = Q_flrand(0.0f, 1.0f) * random->numstrings;
			for (rs = random->firstrandomstring; rs; rs = rs->next)
			{
				if (--i < 0) break;
			} //end for
			if (rs)
			{
				return rs->string;
			} //end if
		} //end for
	} //end for
	return nullptr;
} //end of the function RandomString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotDumpMatchTemplates(bot_matchtemplate_t* matches)
{
	FILE* fp = Log_FilePointer();
	if (!fp) return;
	for (const bot_matchtemplate_t* mt = matches; mt; mt = mt->next)
	{
		fprintf(fp, "{ ");
		for (const bot_matchpiece_t* mp = mt->first; mp; mp = mp->next)
		{
			if (mp->type == MT_STRING)
			{
				for (const bot_matchstring_t* ms = mp->firststring; ms; ms = ms->next)
				{
					fprintf(fp, "\"%s\"", ms->string);
					if (ms->next) fprintf(fp, "|");
				} //end for
			} //end if
			else if (mp->type == MT_VARIABLE)
			{
				fprintf(fp, "%d", mp->variable);
			} //end else if
			if (mp->next) fprintf(fp, ", ");
		} //end for
		fprintf(fp, " = (%d, %d);}\n", mt->type, mt->subtype);
	} //end for
} //end of the function BotDumpMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeMatchPieces(bot_matchpiece_t* matchpieces)
{
	bot_matchpiece_t* nextmp;
	bot_matchstring_t* nextms;

	for (bot_matchpiece_t* mp = matchpieces; mp; mp = nextmp)
	{
		nextmp = mp->next;
		if (mp->type == MT_STRING)
		{
			for (bot_matchstring_t* ms = mp->firststring; ms; ms = nextms)
			{
				nextms = ms->next;
				FreeMemory(ms);
			} //end for
		} //end if
		FreeMemory(mp);
	} //end for
} //end of the function BotFreeMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static bot_matchpiece_t* BotLoadMatchPieces(source_t* source, char* endtoken)
{
	token_t token;
	bot_matchpiece_t* matchpiece;

	bot_matchpiece_t* firstpiece = nullptr;
	bot_matchpiece_t* lastpiece = nullptr;
	//
	int lastwasvariable = qfalse;
	//
	while (PC_ReadToken(source, &token))
	{
		if (token.type == TT_NUMBER && token.subtype & TT_INTEGER)
		{
			if (token.intvalue >= MAX_MATCHVARIABLES)
			{
				SourceError(source, "can't have more than %d match variables", MAX_MATCHVARIABLES);
				FreeSource(source);
				BotFreeMatchPieces(firstpiece);
				return nullptr;
			} //end if
			if (lastwasvariable)
			{
				SourceError(source, "not allowed to have adjacent variables");
				FreeSource(source);
				BotFreeMatchPieces(firstpiece);
				return nullptr;
			} //end if
			lastwasvariable = qtrue;
			//
			matchpiece = static_cast<bot_matchpiece_t*>(GetClearedHunkMemory(sizeof(bot_matchpiece_t)));
			matchpiece->type = MT_VARIABLE;
			matchpiece->variable = token.intvalue;
			matchpiece->next = nullptr;
			if (lastpiece) lastpiece->next = matchpiece;
			else firstpiece = matchpiece;
			lastpiece = matchpiece;
		} //end if
		else if (token.type == TT_STRING)
		{
			//
			matchpiece = static_cast<bot_matchpiece_t*>(GetClearedHunkMemory(sizeof(bot_matchpiece_t)));
			matchpiece->firststring = nullptr;
			matchpiece->type = MT_STRING;
			matchpiece->variable = 0;
			matchpiece->next = nullptr;
			if (lastpiece) lastpiece->next = matchpiece;
			else firstpiece = matchpiece;
			lastpiece = matchpiece;
			//
			bot_matchstring_t* lastmatchstring = nullptr;
			int emptystring = qfalse;
			//
			do
			{
				if (matchpiece->firststring)
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						FreeSource(source);
						BotFreeMatchPieces(firstpiece);
						return nullptr;
					} //end if
				} //end if
				StripDoubleQuotes(token.string);
				const auto matchstring = static_cast<bot_matchstring_t*>(GetClearedHunkMemory(
					sizeof(bot_matchstring_t) + strlen(token.string) + 1));
				matchstring->string = reinterpret_cast<char*>(matchstring) + sizeof(bot_matchstring_t);
				strcpy(matchstring->string, token.string);
				if (!strlen(token.string)) emptystring = qtrue;
				matchstring->next = nullptr;
				if (lastmatchstring) lastmatchstring->next = matchstring;
				else matchpiece->firststring = matchstring;
				lastmatchstring = matchstring;
			} while (PC_CheckTokenString(source, "|"));
			//if there was no empty string found
			if (!emptystring) lastwasvariable = qfalse;
		} //end if
		else
		{
			SourceError(source, "invalid token %s", token.string);
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return nullptr;
		} //end else
		if (PC_CheckTokenString(source, endtoken)) break;
		if (!PC_ExpectTokenString(source, ","))
		{
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return nullptr;
		} //end if
	} //end while
	return firstpiece;
} //end of the function BotLoadMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static void BotFreeMatchTemplates(bot_matchtemplate_t* mt)
{
	bot_matchtemplate_t* nextmt;

	for (; mt; mt = nextmt)
	{
		nextmt = mt->next;
		BotFreeMatchPieces(mt->first);
		FreeMemory(mt);
	} //end for
} //end of the function BotFreeMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_matchtemplate_t* BotLoadMatchTemplates(char* matchfile)
{
	token_t token;

	PC_SetBaseFolder(BOTFILESBASEFOLDER);
	source_t* source = LoadSourceFile(matchfile);
	if (!source)
	{
		botimport.Print(PRT_ERROR, "counldn't load %s\n", matchfile);
		return nullptr;
	} //end if
	//
	bot_matchtemplate_t* matches = nullptr; //list with matches
	bot_matchtemplate_t* lastmatch = nullptr; //last match in the list

	while (PC_ReadToken(source, &token))
	{
		if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
		{
			SourceError(source, "expected integer, found %s", token.string);
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return nullptr;
		} //end if
		//the context
		const unsigned long int context = token.intvalue;
		//
		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return nullptr;
		} //end if
		//
		while (PC_ReadToken(source, &token))
		{
			if (strcmp(token.string, "}") == 0) break;
			//
			PC_UnreadLastToken(source);
			//
			const auto matchtemplate = static_cast<bot_matchtemplate_t*>(GetClearedHunkMemory(sizeof(bot_matchtemplate_t)));
			matchtemplate->context = context;
			matchtemplate->next = nullptr;
			//add the match template to the list
			if (lastmatch) lastmatch->next = matchtemplate;
			else matches = matchtemplate;
			lastmatch = matchtemplate;
			//load the match template
			matchtemplate->first = BotLoadMatchPieces(source, "=");
			if (!matchtemplate->first)
			{
				BotFreeMatchTemplates(matches);
				return nullptr;
			} //end if
			//read the match type
			if (!PC_ExpectTokenString(source, "(") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return nullptr;
			} //end if
			matchtemplate->type = token.intvalue;
			//read the match subtype
			if (!PC_ExpectTokenString(source, ",") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return nullptr;
			} //end if
			matchtemplate->subtype = token.intvalue;
			//read trailing punctuations
			if (!PC_ExpectTokenString(source, ")") ||
				!PC_ExpectTokenString(source, ";"))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return nullptr;
			} //end if
		} //end while
	} //end while
	//free the source
	FreeSource(source);
	botimport.Print(PRT_MESSAGE, "loaded %s\n", matchfile);
	//
	//BotDumpMatchTemplates(matches);
	//
	return matches;
} //end of the function BotLoadMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int StringsMatch(bot_matchpiece_t* pieces, bot_match_t* match)
{
	bot_matchpiece_t* mp;
	bot_matchstring_t* ms;

	//no last variable
	int lastvariable = -1;
	//pointer to the string to compare the match string with
	char* strptr = match->string;
	//Log_Write("match: %s", strptr);
	//compare the string with the current match string
	for (mp = pieces; mp; mp = mp->next)
	{
		//if it is a piece of string
		if (mp->type == MT_STRING)
		{
			char* newstrptr = nullptr;
			for (ms = mp->firststring; ms; ms = ms->next)
			{
				if (!strlen(ms->string))
				{
					newstrptr = strptr;
					break;
				} //end if
				//Log_Write("MT_STRING: %s", mp->string);
				const int index = StringContains(strptr, ms->string, qfalse);
				if (index >= 0)
				{
					newstrptr = strptr + index;
					if (lastvariable >= 0)
					{
						match->variables[lastvariable].length =
							newstrptr - match->string - match->variables[lastvariable].offset;
						//newstrptr - match->variables[lastvariable].ptr;
						lastvariable = -1;
						break;
					} //end if
					if (index == 0)
					{
						break;
					}
					//end else
					newstrptr = nullptr;
				} //end if
			} //end for
			if (!newstrptr) return qfalse;
			strptr = newstrptr + strlen(ms->string);
		} //end if
		//if it is a variable piece of string
		else if (mp->type == MT_VARIABLE)
		{
			//Log_Write("MT_VARIABLE");
			match->variables[mp->variable].offset = strptr - match->string;
			lastvariable = mp->variable;
		} //end else if
	} //end for
	//if a match was found
	if (!mp && (lastvariable >= 0 || !strlen(strptr)))
	{
		//if the last piece was a variable string
		if (lastvariable >= 0)
		{
			assert(match->variables[lastvariable].offset >= 0); // bk001204
			match->variables[lastvariable].length =
				strlen(&match->string[static_cast<int>(match->variables[lastvariable].offset)]);
		} //end if
		return qtrue;
	} //end if
	return qfalse;
} //end of the function StringsMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotFindMatch(char* str, bot_match_t* match, const unsigned long int context)
{
	strncpy(match->string, str, MAX_MESSAGE_SIZE);
	//remove any trailing enters
	while (strlen(match->string) &&
		match->string[strlen(match->string) - 1] == '\n')
	{
		match->string[strlen(match->string) - 1] = '\0';
	} //end while
	//compare the string with all the match strings
	for (const bot_matchtemplate_t* ms = matchtemplates; ms; ms = ms->next)
	{
		if (!(ms->context & context)) continue;
		//reset the match variable offsets
		for (int i = 0; i < MAX_MATCHVARIABLES; i++) match->variables[i].offset = -1;
		//
		if (StringsMatch(ms->first, match))
		{
			match->type = ms->type;
			match->subtype = ms->subtype;
			return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function BotFindMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMatchVariable(bot_match_t* match, const int variable, char* buf, int size)
{
	if (variable < 0 || variable >= MAX_MATCHVARIABLES)
	{
		botimport.Print(PRT_FATAL, "BotMatchVariable: variable out of range\n");
		strcpy(buf, "");
		return;
	} //end if

	if (match->variables[variable].offset >= 0)
	{
		if (match->variables[variable].length < size)
			size = match->variables[variable].length + 1;
		assert(match->variables[variable].offset >= 0); // bk001204
		strncpy(buf, &match->string[static_cast<int>(match->variables[variable].offset)], size - 1);
		buf[size - 1] = '\0';
	} //end if
	else
	{
		strcpy(buf, "");
	} //end else
} //end of the function BotMatchVariable
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_stringlist_t* BotFindStringInList(bot_stringlist_t* list, char* string)
{
	for (bot_stringlist_t* s = list; s; s = s->next)
	{
		if (strcmp(s->string, string) == 0) return s;
	} //end for
	return nullptr;
} //end of the function BotFindStringInList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_stringlist_t* BotCheckChatMessageIntegrety(char* message, bot_stringlist_t* stringlist)
{
	int i;
	char temp[MAX_MESSAGE_SIZE];

	char* msgptr = message;
	//
	while (*msgptr)
	{
		if (*msgptr == ESCAPE_CHAR)
		{
			msgptr++;
			switch (*msgptr)
			{
			case 'v': //variable
			{
				//step over the 'v'
				msgptr++;
				while (*msgptr && *msgptr != ESCAPE_CHAR) msgptr++;
				//step over the trailing escape char
				if (*msgptr) msgptr++;
				break;
			} //end case
			case 'r': //random
			{
				//step over the 'r'
				msgptr++;
				for (i = 0; *msgptr && *msgptr != ESCAPE_CHAR; i++)
				{
					temp[i] = *msgptr++;
				} //end while
				temp[i] = '\0';
				//step over the trailing escape char
				if (*msgptr) msgptr++;
				//find the random keyword
				if (!RandomString(temp))
				{
					if (!BotFindStringInList(stringlist, temp))
					{
						Log_Write("%s = {\"%s\"} //MISSING RANDOM\r\n", temp, temp);
						const auto s = static_cast<bot_stringlist_s*>(GetClearedMemory(sizeof(bot_stringlist_t) + strlen(temp) + 1));
						s->string = reinterpret_cast<char*>(s) + sizeof(bot_stringlist_t);
						strcpy(s->string, temp);
						s->next = stringlist;
						stringlist = s;
					} //end if
				} //end if
				break;
			} //end case
			default:
			{
				botimport.Print(PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message);
				break;
			} //end default
			} //end switch
		} //end if
		else
		{
			msgptr++;
		} //end else
	} //end while
	return stringlist;
} //end of the function BotCheckChatMessageIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotCheckInitialChatIntegrety(bot_chat_t* chat)
{
	bot_stringlist_t* nexts;

	bot_stringlist_t* stringlist = nullptr;
	for (const bot_chattype_t* t = chat->types; t; t = t->next)
	{
		for (const bot_chatmessage_t* cm = t->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		} //end for
	} //end for
	for (bot_stringlist_t* s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		FreeMemory(s);
	} //end for
} //end of the function BotCheckInitialChatIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotCheckReplyChatIntegrety(bot_replychat_t* replychat)
{
	bot_stringlist_t* nexts;

	bot_stringlist_t* stringlist = nullptr;
	for (const bot_replychat_t* rp = replychat; rp; rp = rp->next)
	{
		for (const bot_chatmessage_t* cm = rp->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		} //end for
	} //end for
	for (bot_stringlist_t* s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		FreeMemory(s);
	} //end for
} //end of the function BotCheckReplyChatIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpReplyChat(bot_replychat_t* replychat)
{
	FILE* fp = Log_FilePointer();
	if (!fp) return;
	fprintf(fp, "BotDumpReplyChat:\n");
	for (const bot_replychat_t* rp = replychat; rp; rp = rp->next)
	{
		fprintf(fp, "[");
		for (const bot_replychatkey_t* key = rp->keys; key; key = key->next)
		{
			if (key->flags & RCKFL_AND) fprintf(fp, "&");
			else if (key->flags & RCKFL_NOT) fprintf(fp, "!");
			//
			if (key->flags & RCKFL_NAME) fprintf(fp, "name");
			else if (key->flags & RCKFL_GENDERFEMALE) fprintf(fp, "female");
			else if (key->flags & RCKFL_GENDERMALE) fprintf(fp, "male");
			else if (key->flags & RCKFL_GENDERLESS) fprintf(fp, "it");
			else if (key->flags & RCKFL_VARIABLES)
			{
				fprintf(fp, "(");
				for (const bot_matchpiece_t* mp = key->match; mp; mp = mp->next)
				{
					if (mp->type == MT_STRING) fprintf(fp, "\"%s\"", mp->firststring->string);
					else fprintf(fp, "%d", mp->variable);
					if (mp->next) fprintf(fp, ", ");
				} //end for
				fprintf(fp, ")");
			} //end if
			else if (key->flags & RCKFL_STRING)
			{
				fprintf(fp, "\"%s\"", key->string);
			} //end if
			if (key->next) fprintf(fp, ", ");
			else fprintf(fp, "] = %1.0f\n", rp->priority);
		} //end for
		fprintf(fp, "{\n");
		for (const bot_chatmessage_t* cm = rp->firstchatmessage; cm; cm = cm->next)
		{
			fprintf(fp, "\t\"%s\";\n", cm->chatmessage);
		} //end for
		fprintf(fp, "}\n");
	} //end for
} //end of the function BotDumpReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeReplyChat(bot_replychat_t* replychat)
{
	bot_replychat_t* nextrp;
	bot_replychatkey_t* nextkey;
	bot_chatmessage_t* nextcm;

	for (bot_replychat_t* rp = replychat; rp; rp = nextrp)
	{
		nextrp = rp->next;
		for (bot_replychatkey_t* key = rp->keys; key; key = nextkey)
		{
			nextkey = key->next;
			if (key->match) BotFreeMatchPieces(key->match);
			if (key->string) FreeMemory(key->string);
			FreeMemory(key);
		} //end for
		for (bot_chatmessage_t* cm = rp->firstchatmessage; cm; cm = nextcm)
		{
			nextcm = cm->next;
			FreeMemory(cm);
		} //end for
		FreeMemory(rp);
	} //end for
} //end of the function BotFreeReplyChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotCheckValidReplyChatKeySet(source_t* source, bot_replychatkey_t* keys)
{
	int hasstringkey;
	bot_matchpiece_t* m;
	bot_matchstring_t* ms;
	bot_replychatkey_t* key2;

	//
	int allprefixed = qtrue;
	int hasvariableskey = hasstringkey = qfalse;
	for (const bot_replychatkey_t* key = keys; key; key = key->next)
	{
		if (!(key->flags & (RCKFL_AND | RCKFL_NOT)))
		{
			allprefixed = qfalse;
			if (key->flags & RCKFL_VARIABLES)
			{
				for (m = key->match; m; m = m->next)
				{
					if (m->type == MT_VARIABLE) hasvariableskey = qtrue;
				} //end for
			} //end if
			else if (key->flags & RCKFL_STRING)
			{
				hasstringkey = qtrue;
			} //end else if
		} //end if
		else if (key->flags & RCKFL_AND && key->flags & RCKFL_STRING)
		{
			for (key2 = keys; key2; key2 = key2->next)
			{
				if (key2 == key) continue;
				if (key2->flags & RCKFL_NOT) continue;
				if (key2->flags & RCKFL_VARIABLES)
				{
					for (m = key2->match; m; m = m->next)
					{
						if (m->type == MT_STRING)
						{
							for (ms = m->firststring; ms; ms = ms->next)
							{
								if (StringContains(ms->string, key->string, qfalse) != -1)
								{
									break;
								} //end if
							} //end for
							if (ms) break;
						} //end if
						else if (m->type == MT_VARIABLE)
						{
							break;
						} //end if
					} //end for
					if (!m)
					{
						SourceWarning(source, "one of the match templates does not "
							"leave space for the key %s with the & prefix", key->string);
					} //end if
				} //end if
			} //end for
		} //end else
		if (key->flags & RCKFL_NOT && key->flags & RCKFL_STRING)
		{
			for (key2 = keys; key2; key2 = key2->next)
			{
				if (key2 == key) continue;
				if (key2->flags & RCKFL_NOT) continue;
				if (key2->flags & RCKFL_STRING)
				{
					if (StringContains(key2->string, key->string, qfalse) != -1)
					{
						SourceWarning(source, "the key %s with prefix ! is inside the key %s", key->string, key2->string);
					} //end if
				} //end if
				else if (key2->flags & RCKFL_VARIABLES)
				{
					for (m = key2->match; m; m = m->next)
					{
						if (m->type == MT_STRING)
						{
							for (ms = m->firststring; ms; ms = ms->next)
							{
								if (StringContains(ms->string, key->string, qfalse) != -1)
								{
									SourceWarning(source, "the key %s with prefix ! is inside "
										"the match template string %s", key->string, ms->string);
								} //end if
							} //end for
						} //end if
					} //end for
				} //end else if
			} //end for
		} //end if
	} //end for
	if (allprefixed) SourceWarning(source, "all keys have a & or ! prefix");
	if (hasvariableskey && hasstringkey)
	{
		SourceWarning(source, "variables from the match template(s) could be "
			"invalid when outputting one of the chat messages");
	} //end if
} //end of the function BotCheckValidReplyChatKeySet
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bot_replychat_t* BotLoadReplyChat(char* filename)
{
	token_t token;

	PC_SetBaseFolder(BOTFILESBASEFOLDER);
	source_t* source = LoadSourceFile(filename);
	if (!source)
	{
		botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
		return nullptr;
	} //end if
	//
	bot_replychat_t* replychatlist = nullptr;
	//
	while (PC_ReadToken(source, &token))
	{
		if (strcmp(token.string, "[") != 0)
		{
			SourceError(source, "expected [, found %s", token.string);
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return nullptr;
		} //end if
		//
		const auto replychat = static_cast<bot_replychat_s*>(GetClearedHunkMemory(sizeof(bot_replychat_t)));
		replychat->keys = nullptr;
		replychat->next = replychatlist;
		replychatlist = replychat;
		//read the keys, there must be at least one key
		do
		{
			//allocate a key
			const auto key = static_cast<bot_replychatkey_t*>(GetClearedHunkMemory(sizeof(bot_replychatkey_t)));
			key->flags = 0;
			key->string = nullptr;
			key->match = nullptr;
			key->next = replychat->keys;
			replychat->keys = key;
			//check for MUST BE PRESENT and MUST BE ABSENT keys
			if (PC_CheckTokenString(source, "&")) key->flags |= RCKFL_AND;
			else if (PC_CheckTokenString(source, "!")) key->flags |= RCKFL_NOT;
			//special keys
			if (PC_CheckTokenString(source, "name")) key->flags |= RCKFL_NAME;
			else if (PC_CheckTokenString(source, "female")) key->flags |= RCKFL_GENDERFEMALE;
			else if (PC_CheckTokenString(source, "male")) key->flags |= RCKFL_GENDERMALE;
			else if (PC_CheckTokenString(source, "it")) key->flags |= RCKFL_GENDERLESS;
			else if (PC_CheckTokenString(source, "(")) //match key
			{
				key->flags |= RCKFL_VARIABLES;
				key->match = BotLoadMatchPieces(source, ")");
				if (!key->match)
				{
					BotFreeReplyChat(replychatlist);
					return nullptr;
				} //end if
			} //end else if
			else if (PC_CheckTokenString(source, "<")) //bot names
			{
				char namebuffer[MAX_MESSAGE_SIZE];
				key->flags |= RCKFL_BOTNAMES;
				strcpy(namebuffer, "");
				do
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						BotFreeReplyChat(replychatlist);
						FreeSource(source);
						return nullptr;
					} //end if
					StripDoubleQuotes(token.string);
					if (strlen(namebuffer)) strcat(namebuffer, "\\");
					strcat(namebuffer, token.string);
				} while (PC_CheckTokenString(source, ","));
				if (!PC_ExpectTokenString(source, ">"))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return nullptr;
				} //end if
				key->string = static_cast<char*>(GetClearedHunkMemory(strlen(namebuffer) + 1));
				strcpy(key->string, namebuffer);
			} //end else if
			else //normal string key
			{
				key->flags |= RCKFL_STRING;
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return nullptr;
				} //end if
				StripDoubleQuotes(token.string);
				key->string = static_cast<char*>(GetClearedHunkMemory(strlen(token.string) + 1));
				strcpy(key->string, token.string);
			} //end else
			//
			PC_CheckTokenString(source, ",");
		} while (!PC_CheckTokenString(source, "]"));
		//
		BotCheckValidReplyChatKeySet(source, replychat->keys);
		//read the = sign and the priority
		if (!PC_ExpectTokenString(source, "=") ||
			!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return nullptr;
		} //end if
		replychat->priority = token.floatvalue;
		//read the leading {
		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return nullptr;
		} //end if
		replychat->numchatmessages = 0;
		//while the trailing } is not found
		while (!PC_CheckTokenString(source, "}"))
		{
			char chatmessagestring[MAX_MESSAGE_SIZE];
			if (!BotLoadChatMessage(source, chatmessagestring))
			{
				BotFreeReplyChat(replychatlist);
				FreeSource(source);
				return nullptr;
			} //end if
			const auto chatmessage = static_cast<bot_chatmessage_t*>(GetClearedHunkMemory(
				sizeof(bot_chatmessage_t) + strlen(chatmessagestring) + 1));
			chatmessage->chatmessage = reinterpret_cast<char*>(chatmessage) + sizeof(bot_chatmessage_t);
			strcpy(chatmessage->chatmessage, chatmessagestring);
			chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
			chatmessage->next = replychat->firstchatmessage;
			//add the chat message to the reply chat
			replychat->firstchatmessage = chatmessage;
			replychat->numchatmessages++;
		} //end while
	} //end while
	FreeSource(source);
	botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
	//
	//BotDumpReplyChat(replychatlist);
	if (botDeveloper)
	{
		BotCheckReplyChatIntegrety(replychatlist);
	} //end if
	//
	if (!replychatlist) botimport.Print(PRT_MESSAGE, "no rchats\n");
	//
	return replychatlist;
} //end of the function BotLoadReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpInitialChat(bot_chat_t* chat)
{
	Log_Write("{");
	for (bot_chattype_t* t = chat->types; t; t = t->next)
	{
		Log_Write(" type \"%s\"", t->name);
		Log_Write(" {");
		Log_Write("  numchatmessages = %d", t->numchatmessages);
		for (const bot_chatmessage_t* m = t->firstchatmessage; m; m = m->next)
		{
			Log_Write("  \"%s\"", m->chatmessage);
		} //end for
		Log_Write(" }");
	} //end for
	Log_Write("}");
} //end of the function BotDumpInitialChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_chat_t* BotLoadInitialChat(char* chatfile, char* chatname)
{
	int foundchat, indent, size;
	char* ptr = nullptr;
	source_t* source;
	token_t token;
	bot_chat_t* chat = nullptr;
	bot_chattype_t* chattype = nullptr;
	bot_chatmessage_t* chatmessage = nullptr;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif //DEBUG
	//
	size = 0;
	foundchat = qfalse;
	//a bot chat is parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		//allocate memory
		if (pass && size) ptr = static_cast<char*>(GetClearedMemory(size));
		//load the source file
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
		source = LoadSourceFile(chatfile);
		if (!source)
		{
			botimport.Print(PRT_ERROR, "couldn't load %s\n", chatfile);
			return nullptr;
		} //end if
		//chat structure
		if (pass)
		{
			chat = reinterpret_cast<bot_chat_t*>(ptr);
			ptr += sizeof(bot_chat_t);
		} //end if
		size = sizeof(bot_chat_t);
		//
		while (PC_ReadToken(source, &token))
		{
			if (strcmp(token.string, "chat") == 0)
			{
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					FreeSource(source);
					return nullptr;
				} //end if
				StripDoubleQuotes(token.string);
				//after the chat name we expect an opening brace
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return nullptr;
				} //end if
				//if the chat name is found
				if (!Q_stricmp(token.string, chatname))
				{
					foundchat = qtrue;
					//read the chat types
					while (true)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return nullptr;
						} //end if
						if (strcmp(token.string, "}") == 0) break;
						if (strcmp(token.string, "type") != 0)
						{
							SourceError(source, "expected type found %s", token.string);
							FreeSource(source);
							return nullptr;
						} //end if
						//expect the chat type name
						if (!PC_ExpectTokenType(source, TT_STRING, 0, &token) ||
							!PC_ExpectTokenString(source, "{"))
						{
							FreeSource(source);
							return nullptr;
						} //end if
						StripDoubleQuotes(token.string);
						if (pass)
						{
							chattype = reinterpret_cast<bot_chattype_t*>(ptr);
							strncpy(chattype->name, token.string, MAX_CHATTYPE_NAME);
							chattype->firstchatmessage = nullptr;
							//add the chat type to the chat
							chattype->next = chat->types;
							chat->types = chattype;
							//
							ptr += sizeof(bot_chattype_t);
						} //end if
						size += sizeof(bot_chattype_t);
						//read the chat messages
						while (!PC_CheckTokenString(source, "}"))
						{
							char chatmessagestring[MAX_MESSAGE_SIZE];
							if (!BotLoadChatMessage(source, chatmessagestring))
							{
								FreeSource(source);
								return nullptr;
							} //end if
							size_t len = strlen(chatmessagestring) + 1;
							len = PAD(len, sizeof(long));
							if (pass)
							{
								chatmessage = reinterpret_cast<bot_chatmessage_t*>(ptr);
								chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
								//put the chat message in the list
								chatmessage->next = chattype->firstchatmessage;
								chattype->firstchatmessage = chatmessage;
								//store the chat message
								ptr += sizeof(bot_chatmessage_t);
								chatmessage->chatmessage = ptr;
								strcpy(chatmessage->chatmessage, chatmessagestring);
								ptr += len;
								//the number of chat messages increased
								chattype->numchatmessages++;
							} //end if
							size += sizeof(bot_chatmessage_t) + len;
						} //end if
					} //end while
				} //end if
				else //skip the bot chat
				{
					indent = 1;
					while (indent)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return nullptr;
						} //end if
						if (strcmp(token.string, "{") == 0) indent++;
						else if (strcmp(token.string, "}") == 0) indent--;
					} //end while
				} //end else
			} //end if
			else
			{
				SourceError(source, "unknown definition %s", token.string);
				FreeSource(source);
				return nullptr;
			} //end else
		} //end while
		//free the source
		FreeSource(source);
		//if the requested character is not found
		if (!foundchat)
		{
			botimport.Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile);
			return nullptr;
		} //end if
	} //end for
	//
	botimport.Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);
	//
	//BotDumpInitialChat(chat);
	if (botDeveloper)
	{
		BotCheckInitialChatIntegrety(chat);
	} //end if
#ifdef DEBUG
	botimport.Print(PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_MilliSeconds() - starttime);
#endif //DEBUG
	//character was read successfully
	return chat;
} //end of the function BotLoadInitialChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotFreeChatFile(const int chatstate)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;
	if (cs->chat) FreeMemory(cs->chat);
	cs->chat = nullptr;
} //end of the function BotFreeChatFile
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadChatFile(const int chatstate, char* chatfile, char* chatname)
{
	int avail = 0;

	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return BLERR_CANNOTLOADICHAT;
	BotFreeChatFile(chatstate);

	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		avail = -1;
		for (int n = 0; n < MAX_CLIENTS; n++) {
			if (!ichatdata[n]) {
				if (avail == -1) {
					avail = n;
				}
				continue;
			}
			if (strcmp(chatfile, ichatdata[n]->filename) != 0) {
				continue;
			}
			if (strcmp(chatname, ichatdata[n]->chatname) != 0) {
				continue;
			}
			cs->chat = ichatdata[n]->chat;
			//		botimport.Print( PRT_MESSAGE, "retained %s from %s\n", chatname, chatfile );
			return BLERR_NOERROR;
		}

		if (avail == -1) {
			botimport.Print(PRT_FATAL, "ichatdata table full; couldn't load chat %s from %s\n", chatname, chatfile);
			return BLERR_CANNOTLOADICHAT;
		}
	}

	cs->chat = BotLoadInitialChat(chatfile, chatname);
	if (!cs->chat)
	{
		botimport.Print(PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile);
		return BLERR_CANNOTLOADICHAT;
	} //end if
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		ichatdata[avail] = static_cast<bot_ichatdata_t*>(GetClearedMemory(sizeof(bot_ichatdata_t)));
		ichatdata[avail]->chat = cs->chat;
		Q_strncpyz(ichatdata[avail]->chatname, chatname, sizeof ichatdata[avail]->chatname);
		Q_strncpyz(ichatdata[avail]->filename, chatfile, sizeof ichatdata[avail]->filename);
	} //end if

	return BLERR_NOERROR;
} //end of the function BotLoadChatFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotExpandChatMessage(char* outmessage, char* message, const unsigned long mcontext,
	bot_match_t* match, const unsigned long vcontext, const int reply)
{
	int i;
	char* ptr;
	char temp[MAX_MESSAGE_SIZE];

	int expansion = qfalse;
	char* msgptr = message;
	char* outputbuf = outmessage;
	int len = 0;
	//
	while (*msgptr)
	{
		if (*msgptr == ESCAPE_CHAR)
		{
			msgptr++;
			switch (*msgptr)
			{
			case 'v': //variable
			{
				msgptr++;
				int num = 0;
				while (*msgptr && *msgptr != ESCAPE_CHAR)
				{
					num = num * 10 + *msgptr++ - '0';
				} //end while
				//step over the trailing escape char
				if (*msgptr) msgptr++;
				if (num > MAX_MATCHVARIABLES)
				{
					botimport.Print(PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num);
					return qfalse;
				} //end if
				if (match->variables[num].offset >= 0)
				{
					assert(match->variables[num].offset >= 0); // bk001204
					ptr = &match->string[static_cast<int>(match->variables[num].offset)];
					for (i = 0; i < match->variables[num].length; i++)
					{
						temp[i] = ptr[i];
					} //end for
					temp[i] = 0;
					//if it's a reply message
					if (reply)
					{
						//replace the reply synonyms in the variables
						BotReplaceReplySynonyms(temp, vcontext);
					} //end if
					else
					{
						//replace synonyms in the variable context
						BotReplaceSynonyms(temp, vcontext);
					} //end else
					//
					if (len + strlen(temp) >= MAX_MESSAGE_SIZE)
					{
						botimport.Print(PRT_ERROR, "BotConstructChat: message %s too long\n", message);
						return qfalse;
					} //end if
					strcpy(&outputbuf[len], temp);
					len += strlen(temp);
				} //end if
				break;
			} //end case
			case 'r': //random
			{
				msgptr++;
				for (i = 0; *msgptr && *msgptr != ESCAPE_CHAR; i++)
				{
					temp[i] = *msgptr++;
				} //end while
				temp[i] = '\0';
				//step over the trailing escape char
				if (*msgptr) msgptr++;
				//find the random keyword
				ptr = RandomString(temp);
				if (!ptr)
				{
					botimport.Print(PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp);
					return qfalse;
				} //end if
				if (len + strlen(ptr) >= MAX_MESSAGE_SIZE)
				{
					botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
					return qfalse;
				} //end if
				strcpy(&outputbuf[len], ptr);
				len += strlen(ptr);
				expansion = qtrue;
				break;
			} //end case
			default:
			{
				botimport.Print(PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message);
				break;
			} //end default
			} //end switch
		} //end if
		else
		{
			outputbuf[len++] = *msgptr++;
			if (len >= MAX_MESSAGE_SIZE)
			{
				botimport.Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
				break;
			} //end if
		} //end else
	} //end while
	outputbuf[len] = '\0';
	//replace synonyms weighted in the message context
	BotReplaceWeightedSynonyms(outputbuf, mcontext);
	//return true if a random was expanded
	return expansion;
} //end of the function BotExpandChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotConstructChatMessage(bot_chatstate_t* chatstate, char* message, const unsigned long mcontext,
	bot_match_t* match, const unsigned long vcontext, const int reply)
{
	int i;
	char srcmessage[MAX_MESSAGE_SIZE];

	strcpy(srcmessage, message);
	for (i = 0; i < 10; i++)
	{
		if (!BotExpandChatMessage(chatstate->chatmessage, srcmessage, mcontext, match, vcontext, reply))
		{
			break;
		} //end if
		strcpy(srcmessage, chatstate->chatmessage);
	} //end for
	if (i >= 10)
	{
		botimport.Print(PRT_WARNING, "too many expansions in chat message\n");
		botimport.Print(PRT_WARNING, "%s\n", chatstate->chatmessage);
	} //end if
} //end of the function BotConstructChatMessage
//===========================================================================
// randomly chooses one of the chat message of the given type
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char* BotChooseInitialChatMessage(bot_chatstate_t* cs, char* type)
{
	bot_chatmessage_t* m;

	const bot_chat_t* chat = cs->chat;
	for (const bot_chattype_t* t = chat->types; t; t = t->next)
	{
		if (!Q_stricmp(t->name, type))
		{
			int numchatmessages = 0;
			for (m = t->firstchatmessage; m; m = m->next)
			{
				if (m->time > AAS_Time()) continue;
				numchatmessages++;
			} //end if
			//if all chat messages have been used recently
			if (numchatmessages <= 0)
			{
				float besttime = 0;
				const bot_chatmessage_t* bestchatmessage = nullptr;
				for (m = t->firstchatmessage; m; m = m->next)
				{
					if (!besttime || m->time < besttime)
					{
						bestchatmessage = m;
						besttime = m->time;
					} //end if
				} //end for
				if (bestchatmessage) return bestchatmessage->chatmessage;
			} //end if
			else //choose a chat message randomly
			{
				int n = Q_flrand(0.0f, 1.0f) * numchatmessages;
				for (m = t->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time()) continue;
					if (--n < 0)
					{
						m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
						return m->chatmessage;
					} //end if
				} //end for
			} //end else
			return nullptr;
		} //end if
	} //end for
	return nullptr;
} //end of the function BotChooseInitialChatMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNumInitialChats(const int chatstate, char* type)
{
	const bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return 0;

	for (const bot_chattype_t* t = cs->chat->types; t; t = t->next)
	{
		if (!Q_stricmp(t->name, type))
		{
			if (LibVarGetValue("bot_testichat")) {
				botimport.Print(PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages);
				botimport.Print(PRT_MESSAGE, "-------------------\n");
			}
			return t->numchatmessages;
		} //end if
	} //end for
	return 0;
} //end of the function BotNumInitialChats
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInitialChat(const int chatstate, char* type, const int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	bot_match_t match;

	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;
	//if no chat file is loaded
	if (!cs->chat) return;
	//choose a chat message randomly of the given type
	char* message = BotChooseInitialChatMessage(cs, type);
	//if there's no message of the given type
	if (!message)
	{
#ifdef DEBUG
		botimport.Print(PRT_MESSAGE, "no chat messages of type %s\n", type);
#endif //DEBUG
		return;
	} //end if
	//
	Com_Memset(&match, 0, sizeof match);
	int index = 0;
	if (var0) {
		strcat(match.string, var0);
		match.variables[0].offset = index;
		match.variables[0].length = strlen(var0);
		index += strlen(var0);
	}
	if (var1) {
		strcat(match.string, var1);
		match.variables[1].offset = index;
		match.variables[1].length = strlen(var1);
		index += strlen(var1);
	}
	if (var2) {
		strcat(match.string, var2);
		match.variables[2].offset = index;
		match.variables[2].length = strlen(var2);
		index += strlen(var2);
	}
	if (var3) {
		strcat(match.string, var3);
		match.variables[3].offset = index;
		match.variables[3].length = strlen(var3);
		index += strlen(var3);
	}
	if (var4) {
		strcat(match.string, var4);
		match.variables[4].offset = index;
		match.variables[4].length = strlen(var4);
		index += strlen(var4);
	}
	if (var5) {
		strcat(match.string, var5);
		match.variables[5].offset = index;
		match.variables[5].length = strlen(var5);
		index += strlen(var5);
	}
	if (var6) {
		strcat(match.string, var6);
		match.variables[6].offset = index;
		match.variables[6].length = strlen(var6);
		index += strlen(var6);
	}
	if (var7) {
		strcat(match.string, var7);
		match.variables[7].offset = index;
		match.variables[7].length = strlen(var7);
		index += strlen(var7);
	}
	//
	BotConstructChatMessage(cs, message, mcontext, &match, 0, qfalse);
} //end of the function BotInitialChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPrintReplyChatKeys(bot_replychat_t* replychat)
{
	botimport.Print(PRT_MESSAGE, "[");
	for (const bot_replychatkey_t* key = replychat->keys; key; key = key->next)
	{
		if (key->flags & RCKFL_AND) botimport.Print(PRT_MESSAGE, "&");
		else if (key->flags & RCKFL_NOT) botimport.Print(PRT_MESSAGE, "!");
		//
		if (key->flags & RCKFL_NAME) botimport.Print(PRT_MESSAGE, "name");
		else if (key->flags & RCKFL_GENDERFEMALE) botimport.Print(PRT_MESSAGE, "female");
		else if (key->flags & RCKFL_GENDERMALE) botimport.Print(PRT_MESSAGE, "male");
		else if (key->flags & RCKFL_GENDERLESS) botimport.Print(PRT_MESSAGE, "it");
		else if (key->flags & RCKFL_VARIABLES)
		{
			botimport.Print(PRT_MESSAGE, "(");
			for (const bot_matchpiece_t* mp = key->match; mp; mp = mp->next)
			{
				if (mp->type == MT_STRING) botimport.Print(PRT_MESSAGE, "\"%s\"", mp->firststring->string);
				else botimport.Print(PRT_MESSAGE, "%d", mp->variable);
				if (mp->next) botimport.Print(PRT_MESSAGE, ", ");
			} //end for
			botimport.Print(PRT_MESSAGE, ")");
		} //end if
		else if (key->flags & RCKFL_STRING)
		{
			botimport.Print(PRT_MESSAGE, "\"%s\"", key->string);
		} //end if
		if (key->next) botimport.Print(PRT_MESSAGE, ", ");
		else botimport.Print(PRT_MESSAGE, "] = %1.0f\n", replychat->priority);
	} //end for
	botimport.Print(PRT_MESSAGE, "{\n");
} //end of the function BotPrintReplyChatKeys
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotReplyChat(const int chatstate, char* message, const int mcontext, const int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	bot_chatmessage_t* m;
	bot_match_t match, bestmatch;

	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return qfalse;
	Com_Memset(&match, 0, sizeof(bot_match_t));
	strcpy(match.string, message);
	int bestpriority = -1;
	bot_chatmessage_t* bestchatmessage = nullptr;
	const bot_replychat_t* bestrchat = nullptr;
	//go through all the reply chats
	for (const bot_replychat_t* rchat = replychats; rchat; rchat = rchat->next)
	{
		int found = qfalse;
		for (const bot_replychatkey_t* key = rchat->keys; key; key = key->next)
		{
			int res = qfalse;
			//get the match result
			if (key->flags & RCKFL_NAME) res = StringContains(message, cs->name, qfalse) != -1;
			else if (key->flags & RCKFL_BOTNAMES) res = StringContains(key->string, cs->name, qfalse) != -1;
			else if (key->flags & RCKFL_GENDERFEMALE) res = cs->gender == CHAT_GENDERFEMALE;
			else if (key->flags & RCKFL_GENDERMALE) res = cs->gender == CHAT_GENDERMALE;
			else if (key->flags & RCKFL_GENDERLESS) res = cs->gender == CHAT_GENDERLESS;
			else if (key->flags & RCKFL_VARIABLES) res = StringsMatch(key->match, &match);
			else if (key->flags & RCKFL_STRING) res = StringContainsWord(message, key->string, qfalse) != nullptr;
			//if the key must be present
			if (key->flags & RCKFL_AND)
			{
				if (!res)
				{
					found = qfalse;
					break;
				} //end if
			} //end else if
			//if the key must be absent
			else if (key->flags & RCKFL_NOT)
			{
				if (res)
				{
					found = qfalse;
					break;
				} //end if
			} //end if
			else if (res)
			{
				found = qtrue;
			} //end else
		} //end for
		//
		if (found)
		{
			if (rchat->priority > bestpriority)
			{
				int numchatmessages = 0;
				for (m = rchat->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time()) continue;
					numchatmessages++;
				} //end if
				int num = Q_flrand(0.0f, 1.0f) * numchatmessages;
				for (m = rchat->firstchatmessage; m; m = m->next)
				{
					if (--num < 0) break;
					if (m->time > AAS_Time()) continue;
				} //end for
				//if the reply chat has a message
				if (m)
				{
					Com_Memcpy(&bestmatch, &match, sizeof(bot_match_t));
					bestchatmessage = m;
					bestrchat = rchat;
					bestpriority = rchat->priority;
				} //end if
			} //end if
		} //end if
	} //end for
	if (bestchatmessage)
	{
		int index = strlen(bestmatch.string);
		if (var0) {
			strcat(bestmatch.string, var0);
			bestmatch.variables[0].offset = index;
			bestmatch.variables[0].length = strlen(var0);
			index += strlen(var0);
		}
		if (var1) {
			strcat(bestmatch.string, var1);
			bestmatch.variables[1].offset = index;
			bestmatch.variables[1].length = strlen(var1);
			index += strlen(var1);
		}
		if (var2) {
			strcat(bestmatch.string, var2);
			bestmatch.variables[2].offset = index;
			bestmatch.variables[2].length = strlen(var2);
			index += strlen(var2);
		}
		if (var3) {
			strcat(bestmatch.string, var3);
			bestmatch.variables[3].offset = index;
			bestmatch.variables[3].length = strlen(var3);
			index += strlen(var3);
		}
		if (var4) {
			strcat(bestmatch.string, var4);
			bestmatch.variables[4].offset = index;
			bestmatch.variables[4].length = strlen(var4);
			index += strlen(var4);
		}
		if (var5) {
			strcat(bestmatch.string, var5);
			bestmatch.variables[5].offset = index;
			bestmatch.variables[5].length = strlen(var5);
			index += strlen(var5);
		}
		if (var6) {
			strcat(bestmatch.string, var6);
			bestmatch.variables[6].offset = index;
			bestmatch.variables[6].length = strlen(var6);
			index += strlen(var6);
		}
		if (var7) {
			strcat(bestmatch.string, var7);
			bestmatch.variables[7].offset = index;
			bestmatch.variables[7].length = strlen(var7);
			index += strlen(var7);
		}
		if (LibVarGetValue("bot_testrchat"))
		{
			for (m = bestrchat->firstchatmessage; m; m = m->next)
			{
				BotConstructChatMessage(cs, m->chatmessage, mcontext, &bestmatch, vcontext, qtrue);
				BotRemoveTildes(cs->chatmessage);
				botimport.Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
			} //end if
		} //end if
		else
		{
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage(cs, bestchatmessage->chatmessage, mcontext, &bestmatch, vcontext, qtrue);
		} //end else
		return qtrue;
	} //end if
	return qfalse;
} //end of the function BotReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChatLength(const int chatstate)
{
	const bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return 0;
	return strlen(cs->chatmessage);
} //end of the function BotChatLength
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotEnterChat(const int chatstate, const int clientto, const int sendto)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;

	if (strlen(cs->chatmessage))
	{
		BotRemoveTildes(cs->chatmessage);
		if (LibVarGetValue("bot_testichat")) {
			botimport.Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
		}
		else {
			switch (sendto) {
			case CHAT_TEAM:
				EA_Command(cs->client, va("say_team %s", cs->chatmessage));
				break;
			case CHAT_TELL:
				EA_Command(cs->client, va("tell %d %s", clientto, cs->chatmessage));
				break;
			default: //CHAT_ALL
				EA_Command(cs->client, va("say %s", cs->chatmessage));
				break;
			}
		}
		//clear the chat message from the state
		strcpy(cs->chatmessage, "");
	} //end if
} //end of the function BotEnterChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotGetChatMessage(const int chatstate, char* buf, const int size)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;

	BotRemoveTildes(cs->chatmessage);
	strncpy(buf, cs->chatmessage, size - 1);
	buf[size - 1] = '\0';
	//clear the chat message from the state
	strcpy(cs->chatmessage, "");
} //end of the function BotGetChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotSetChatGender(const int chatstate, const int gender)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;
	switch (gender)
	{
	case CHAT_GENDERFEMALE: cs->gender = CHAT_GENDERFEMALE; break;
	case CHAT_GENDERMALE: cs->gender = CHAT_GENDERMALE; break;
	default: cs->gender = CHAT_GENDERLESS; break;
	} //end switch
} //end of the function BotSetChatGender
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSetChatName(const int chatstate, char* name, const int client)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs) return;
	cs->client = client;
	Com_Memset(cs->name, 0, sizeof cs->name);
	strncpy(cs->name, name, sizeof cs->name);
	cs->name[sizeof cs->name - 1] = '\0';
} //end of the function BotSetChatName
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetChatAI(void)
{
	for (const bot_replychat_t* rchat = replychats; rchat; rchat = rchat->next)
	{
		for (bot_chatmessage_t* m = rchat->firstchatmessage; m; m = m->next)
		{
			m->time = 0;
		} //end for
	} //end for
} //end of the function BotResetChatAI
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotAllocChatState(void)
{
	for (int i = 1; i <= MAX_CLIENTS; i++)
	{
		if (!botchatstates[i])
		{
			botchatstates[i] = static_cast<bot_chatstate_s*>(GetClearedMemory(sizeof(bot_chatstate_t)));
			return i;
		} //end if
	} //end for
	return 0;
} //end of the function BotAllocChatState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeChatState(const int handle)
{
	bot_consolemessage_t m;

	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		botimport.Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
		return;
	} //end if
	if (!botchatstates[handle])
	{
		botimport.Print(PRT_FATAL, "invalid chat state %d\n", handle);
		return;
	} //end if
	if (LibVarGetValue("bot_reloadcharacters"))
	{
		BotFreeChatFile(handle);
	} //end if
	//free all the console messages left in the chat state
	for (int h = BotNextConsoleMessage(handle, &m); h; h = BotNextConsoleMessage(handle, &m))
	{
		//remove the console message
		BotRemoveConsoleMessage(handle, h);
	} //end for
	FreeMemory(botchatstates[handle]);
	botchatstates[handle] = nullptr;
} //end of the function BotFreeChatState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupChatAI(void)
{
	char* file;

#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif //DEBUG

	file = LibVarString("synfile", "syn.c");
	synonyms = BotLoadSynonyms(file);
	file = LibVarString("rndfile", "rnd.c");
	randomstrings = BotLoadRandomStrings(file);
	file = LibVarString("matchfile", "match.c");
	matchtemplates = BotLoadMatchTemplates(file);
	//
	if (!LibVarValue("nochat", "0"))
	{
		file = LibVarString("rchatfile", "rchat.c");
		replychats = BotLoadReplyChat(file);
	} //end if

	InitConsoleMessageHeap();

#ifdef DEBUG
	botimport.Print(PRT_MESSAGE, "setup chat AI %d msec\n", Sys_MilliSeconds() - starttime);
#endif //DEBUG
	return BLERR_NOERROR;
} //end of the function BotSetupChatAI
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotShutdownChatAI(void)
{
	int i;

	//free all remaining chat states
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		} //end if
	} //end for
	//free all cached chats
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (ichatdata[i])
		{
			FreeMemory(ichatdata[i]->chat);
			FreeMemory(ichatdata[i]);
			ichatdata[i] = nullptr;
		} //end if
	} //end for
	if (consolemessageheap) FreeMemory(consolemessageheap);
	consolemessageheap = nullptr;
	if (matchtemplates) BotFreeMatchTemplates(matchtemplates);
	matchtemplates = nullptr;
	if (randomstrings) FreeMemory(randomstrings);
	randomstrings = nullptr;
	if (synonyms) FreeMemory(synonyms);
	synonyms = nullptr;
	if (replychats) BotFreeReplyChat(replychats);
	replychats = nullptr;
} //end of the function BotShutdownChatAI