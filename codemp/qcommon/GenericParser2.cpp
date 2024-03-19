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

#include "GenericParser2.h"

#include <cstring>
#include "qcommon/qcommon.h"

#define MAX_TOKEN_SIZE	1024
static char token[MAX_TOKEN_SIZE];

static char* GetToken(char** text, const bool allowLineBreaks, const bool readUntilEOL = false)
{
	char* pointer = *text;
	int length = 0;
	int c;

	token[0] = 0;
	if (!pointer)
	{
		return token;
	}

	while (true)
	{
		bool foundLineBreak = false;
		while (true)
		{
			c = *pointer;
			if (c > ' ')
			{
				break;
			}
			if (!c)
			{
				*text = nullptr;
				return token;
			}
			if (c == '\n')
			{
				foundLineBreak = true;
			}
			pointer++;
		}
		if (foundLineBreak && !allowLineBreaks)
		{
			*text = pointer;
			return token;
		}

		c = *pointer;

		// skip single line comment
		if (c == '/' && pointer[1] == '/')
		{
			pointer += 2;
			while (*pointer && *pointer != '\n')
			{
				pointer++;
			}
		}
		// skip multi line comments
		else if (c == '/' && pointer[1] == '*')
		{
			pointer += 2;
			while (*pointer && (*pointer != '*' || pointer[1] != '/'))
			{
				pointer++;
			}
			if (*pointer)
			{
				pointer += 2;
			}
		}
		else
		{
			// found the start of a token
			break;
		}
	}

	if (c == '\"')
	{
		// handle a string
		pointer++;
		while (true)
		{
			c = *pointer++;
			if (c == '\"')
			{
				//				token[length++] = c;
				break;
			}
			if (!c)
			{
				break;
			}
			if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
		}
	}
	else if (readUntilEOL)
	{
		// absorb all characters until EOL
		while (c != '\n' && c != '\r')
		{
			if (c == '/' && (*(pointer + 1) == '/' || *(pointer + 1) == '*'))
			{
				break;
			}

			if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
			pointer++;
			c = *pointer;
		}
		// remove trailing white space
		while (length && token[length - 1] < ' ')
		{
			length--;
		}
	}
	else
	{
		while (c > ' ')
		{
			if (length < MAX_TOKEN_SIZE)
			{
				token[length++] = c;
			}
			pointer++;
			c = *pointer;
		}
	}

	if (token[0] == '\"')
	{
		// remove start quote
		length--;
		memmove(token, token + 1, length);

		if (length && token[length - 1] == '\"')
		{
			// remove end quote
			length--;
		}
	}

	if (length >= MAX_TOKEN_SIZE)
	{
		length = 0;
	}
	token[length] = 0;
	*text = pointer;

	return token;
}

CTextPool::CTextPool(const int initSize) :
	mNext(nullptr),
	mSize(initSize),
	mUsed(0)
{
	//	mPool = (char *)Z_Malloc(mSize, TAG_GP2);
	mPool = static_cast<char*>(Z_Malloc(mSize, TAG_TEXTPOOL, qtrue));
}

CTextPool::~CTextPool(void)
{
	Z_Free(mPool);
}

char* CTextPool::AllocText(char* text, const bool addNULL, CTextPool** poolPtr)
{
	const int length = strlen(text) + (addNULL ? 1 : 0);

	if (mUsed + length + 1 > mSize)
	{
		// extra 1 to put a null on the end
		if (poolPtr)
		{
			(*poolPtr)->SetNext(new CTextPool(mSize));
			*poolPtr = (*poolPtr)->GetNext();

			return (*poolPtr)->AllocText(text, addNULL);
		}

		return nullptr;
	}

	strcpy(mPool + mUsed, text);
	mUsed += length;
	mPool[mUsed] = 0;

	return mPool + mUsed - length;
}

void CleanTextPool(CTextPool* pool)
{
	while (pool)
	{
		CTextPool* next = pool->GetNext();
		delete pool;
		pool = next;
	}
}

CGPObject::CGPObject(const char* initName) :
	mName(initName),
	mNext(nullptr),
	mInOrderNext(nullptr),
	mInOrderPrevious(nullptr)
{
}

bool CGPObject::WriteText(CTextPool** textPool, const char* text) const
{
	if (strchr(text, ' ') || !text[0])
	{
		(*textPool)->AllocText("\"", false, textPool);
		(*textPool)->AllocText(const_cast<char*>(text), false, textPool);
		(*textPool)->AllocText("\"", false, textPool);
	}
	else
	{
		(*textPool)->AllocText(const_cast<char*>(text), false, textPool);
	}

	return true;
}

CGPValue::CGPValue(const char* initName, const char* initValue) :
	CGPObject(initName),
	mList(nullptr)
{
	if (initValue)
	{
		AddValue(initValue);
	}
}

CGPValue::~CGPValue(void)
{
	while (mList)
	{
		CGPObject* next = mList->GetNext();
		delete mList;
		mList = next;
	}
}

CGPValue* CGPValue::Duplicate(CTextPool** textPool) const
{
	char* name;

	if (textPool)
	{
		name = (*textPool)->AllocText(const_cast<char*>(mName), true, textPool);
	}
	else
	{
		name = const_cast<char*>(mName);
	}

	const auto newValue = new CGPValue(name);
	const CGPObject* iterator = mList;
	while (iterator)
	{
		if (textPool)
		{
			name = (*textPool)->AllocText(const_cast<char*>(iterator->GetName()), true, textPool);
		}
		else
		{
			name = const_cast<char*>(iterator->GetName());
		}
		newValue->AddValue(name);
		iterator = iterator->GetNext();
	}

	return newValue;
}

bool CGPValue::IsList(void) const
{
	if (!mList || !mList->GetNext())
	{
		return false;
	}

	return true;
}

const char* CGPValue::GetTopValue(void) const
{
	if (mList)
	{
		return mList->GetName();
	}

	return nullptr;
}

void CGPValue::AddValue(const char* newValue, CTextPool** textPool)
{
	if (textPool)
	{
		newValue = (*textPool)->AllocText(const_cast<char*>(newValue), true, textPool);
	}

	if (mList == nullptr)
	{
		mList = new CGPObject(newValue);
		mList->SetInOrderNext(mList);
	}
	else
	{
		mList->GetInOrderNext()->SetNext(new CGPObject(newValue));
		mList->SetInOrderNext(mList->GetInOrderNext()->GetNext());
	}
}

bool CGPValue::Parse(char** dataPtr, CTextPool** textPool)
{
	while (true)
	{
		char* token = GetToken(dataPtr, true, true);

		if (!token[0])
		{
			// end of data - error!
			return false;
		}
		if (Q_stricmp(token, "]") == 0)
		{
			// ending brace for this list
			break;
		}

		const char* value = (*textPool)->AllocText(token, true, textPool);
		AddValue(value);
	}

	return true;
}

bool CGPValue::Write(CTextPool** textPool, const int depth) const
{
	int i;

	if (!mList)
	{
		return true;
	}

	for (i = 0; i < depth; i++)
	{
		(*textPool)->AllocText("\t", false, textPool);
	}

	WriteText(textPool, mName);

	if (!mList->GetNext())
	{
		(*textPool)->AllocText("\t\t", false, textPool);
		mList->WriteText(textPool, mList->GetName());
		(*textPool)->AllocText("\r\n", false, textPool);
	}
	else
	{
		(*textPool)->AllocText("\r\n", false, textPool);

		for (i = 0; i < depth; i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("[\r\n", false, textPool);

		const CGPObject* next = mList;
		while (next)
		{
			for (i = 0; i < depth + 1; i++)
			{
				(*textPool)->AllocText("\t", false, textPool);
			}
			mList->WriteText(textPool, next->GetName());
			(*textPool)->AllocText("\r\n", false, textPool);

			next = next->GetNext();
		}

		for (i = 0; i < depth; i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("]\r\n", false, textPool);
	}

	return true;
}

CGPGroup::CGPGroup(const char* initName, CGPGroup* initParent) :
	CGPObject(initName),
	mPairs(nullptr),
	mInOrderPairs(nullptr),
	mCurrentPair(nullptr),
	mSubGroups(nullptr),
	mInOrderSubGroups(nullptr),
	mCurrentSubGroup(nullptr),
	mParent(initParent),
	mWriteable(false)
{
}

CGPGroup::~CGPGroup(void)
{
	Clean();
}

int CGPGroup::GetNumSubGroups(void) const
{
	int count = 0;
	const CGPGroup* group = mSubGroups;
	do
	{
		count++;
		group = group->GetNext();
	} while (group);

	return count;
}

int CGPGroup::GetNumPairs(void) const
{
	int count = 0;
	const CGPValue* pair = mPairs;
	do
	{
		count++;
		pair = pair->GetNext();
	} while (pair);

	return count;
}

void CGPGroup::Clean(void)
{
	while (mPairs)
	{
		mCurrentPair = mPairs->GetNext();
		delete mPairs;
		mPairs = mCurrentPair;
	}

	while (mSubGroups)
	{
		mCurrentSubGroup = mSubGroups->GetNext();
		delete mSubGroups;
		mSubGroups = mCurrentSubGroup;
	}

	mPairs = mInOrderPairs = mCurrentPair = nullptr;
	mSubGroups = mInOrderSubGroups = mCurrentSubGroup = nullptr;
	mParent = nullptr;
	mWriteable = false;
}

CGPGroup* CGPGroup::Duplicate(CTextPool** textPool, CGPGroup* initParent) const
{
	char* name;

	if (textPool)
	{
		name = (*textPool)->AllocText(const_cast<char*>(mName), true, textPool);
	}
	else
	{
		name = const_cast<char*>(mName);
	}

	const auto newGroup = new CGPGroup(name);

	const CGPGroup* subSub = mSubGroups;
	while (subSub)
	{
		CGPGroup* newSub = subSub->Duplicate(textPool, newGroup);
		newGroup->AddGroup(newSub);

		subSub = subSub->GetNext();
	}

	const CGPValue* subPair = mPairs;
	while (subPair)
	{
		CGPValue* newPair = subPair->Duplicate(textPool);
		newGroup->AddPair(newPair);

		subPair = subPair->GetNext();
	}

	return newGroup;
}

void CGPGroup::SortObject(CGPObject* object, CGPObject** unsortedList, CGPObject** sortedList,
	CGPObject** lastObject)
{
	if (!*unsortedList)
	{
		*unsortedList = *sortedList = object;
	}
	else
	{
		(*lastObject)->SetNext(object);

		CGPObject* test = *sortedList;
		CGPObject* last = nullptr;
		while (test)
		{
			if (Q_stricmp(object->GetName(), test->GetName()) < 0)
			{
				break;
			}

			last = test;
			test = test->GetInOrderNext();
		}

		if (test)
		{
			test->SetInOrderPrevious(object);
			object->SetInOrderNext(test);
		}
		if (last)
		{
			last->SetInOrderNext(object);
			object->SetInOrderPrevious(last);
		}
		else
		{
			*sortedList = object;
		}
	}

	*lastObject = object;
}

CGPValue* CGPGroup::AddPair(const char* name, const char* value, CTextPool** textPool)
{
	if (textPool)
	{
		name = (*textPool)->AllocText(const_cast<char*>(name), true, textPool);
		if (value)
		{
			value = (*textPool)->AllocText(const_cast<char*>(value), true, textPool);
		}
	}

	const auto newPair = new CGPValue(name, value);

	AddPair(newPair);

	return newPair;
}

void CGPGroup::AddPair(CGPValue* NewPair)
{
	SortObject(NewPair, reinterpret_cast<CGPObject**>(&mPairs), reinterpret_cast<CGPObject**>(&mInOrderPairs),
		reinterpret_cast<CGPObject**>(&mCurrentPair));
}

CGPGroup* CGPGroup::AddGroup(const char* name, CTextPool** textPool)
{
	if (textPool)
	{
		name = (*textPool)->AllocText(const_cast<char*>(name), true, textPool);
	}

	const auto newGroup = new CGPGroup(name, this);

	AddGroup(newGroup);

	return newGroup;
}

void CGPGroup::AddGroup(CGPGroup* NewGroup)
{
	SortObject(NewGroup, reinterpret_cast<CGPObject**>(&mSubGroups), reinterpret_cast<CGPObject**>(&mInOrderSubGroups),
		reinterpret_cast<CGPObject**>(&mCurrentSubGroup));
}

CGPGroup* CGPGroup::FindSubGroup(const char* name) const
{
	CGPGroup* group = mSubGroups;
	while (group)
	{
		if (!Q_stricmp(name, group->GetName()))
		{
			return group;
		}
		group = group->GetNext();
	}
	return nullptr;
}

bool CGPGroup::Parse(char** dataPtr, CTextPool** textPool)
{
	while (true)
	{
		char lastToken[MAX_TOKEN_SIZE];
		const char* token = GetToken(dataPtr, true);

		if (!token[0])
		{
			// end of data - error!
			if (mParent)
			{
				return false;
			}
			break;
		}
		if (Q_stricmp(token, "}") == 0)
		{
			// ending brace for this group
			break;
		}

		strcpy(lastToken, token);

		// read ahead to see what we are doing
		token = GetToken(dataPtr, true, true);
		if (Q_stricmp(token, "{") == 0)
		{
			// new sub group
			CGPGroup* newSubGroup = AddGroup(lastToken, textPool);
			newSubGroup->SetWriteable(mWriteable);
			if (!newSubGroup->Parse(dataPtr, textPool))
			{
				return false;
			}
		}
		else if (Q_stricmp(token, "[") == 0)
		{
			// new pair list
			CGPValue* newPair = AddPair(lastToken, nullptr, textPool);
			if (!newPair->Parse(dataPtr, textPool))
			{
				return false;
			}
		}
		else
		{
			// new pair
			AddPair(lastToken, token, textPool);
		}
	}

	return true;
}

bool CGPGroup::Write(CTextPool** textPool, const int depth) const
{
	int i;
	const CGPValue* mPair = mPairs;
	const CGPGroup* mSubGroup = mSubGroups;

	if (depth >= 0)
	{
		for (i = 0; i < depth; i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		WriteText(textPool, mName);
		(*textPool)->AllocText("\r\n", false, textPool);

		for (i = 0; i < depth; i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("{\r\n", false, textPool);
	}

	while (mPair)
	{
		mPair->Write(textPool, depth + 1);
		mPair = mPair->GetNext();
	}

	while (mSubGroup)
	{
		mSubGroup->Write(textPool, depth + 1);
		mSubGroup = mSubGroup->GetNext();
	}

	if (depth >= 0)
	{
		for (i = 0; i < depth; i++)
		{
			(*textPool)->AllocText("\t", false, textPool);
		}
		(*textPool)->AllocText("}\r\n", false, textPool);
	}

	return true;
}

/************************************************************************************************
 * CGPGroup::FindPair
 *    This function will search for the pair with the specified key name.  Multiple keys may be
 *    searched if you specify "||" inbetween each key name in the string.  The first key to be
 *    found (from left to right) will be returned.
 *
 * Input
 *    key: the name of the key(s) to be searched for.
 *
 * Output / Return
 *    the group belonging to the first key found or 0 if no group was found.
 *
 ************************************************************************************************/
CGPValue* CGPGroup::FindPair(const char* key) const
{
	size_t length;
	const char* next;

	const char* pos = key;
	while (pos[0])
	{
		const char* separator = strstr(pos, "||");
		if (separator)
		{
			length = separator - pos;
			next = separator + 2;
		}
		else
		{
			length = strlen(pos);
			next = pos + length;
		}

		CGPValue* pair = mPairs;
		while (pair)
		{
			if (strlen(pair->GetName()) == length &&
				Q_stricmpn(pair->GetName(), pos, length) == 0)
			{
				return pair;
			}

			pair = pair->GetNext();
		}

		pos = next;
	}

	return nullptr;
}

const char* CGPGroup::FindPairValue(const char* key, const char* defaultVal) const
{
	const CGPValue* pair = FindPair(key);

	if (pair)
	{
		return pair->GetTopValue();
	}

	return defaultVal;
}

CGenericParser2::CGenericParser2(void) :
	mTextPool(nullptr),
	mWriteable(false)
{
}

CGenericParser2::~CGenericParser2(void)
{
	Clean();
}

bool CGenericParser2::Parse(char** dataPtr, const bool cleanFirst, const bool writeable)
{
	CTextPool* topPool;

	if (cleanFirst)
	{
		Clean();
	}

	if (!mTextPool)
	{
		mTextPool = new CTextPool;
	}

	SetWriteable(writeable);
	mTopLevel.SetWriteable(writeable);
	topPool = mTextPool;
	const bool ret = mTopLevel.Parse(dataPtr, &topPool);

	return ret;
}

void CGenericParser2::Clean(void)
{
	mTopLevel.Clean();

	CleanTextPool(mTextPool);
	mTextPool = nullptr;
}

bool CGenericParser2::Write(CTextPool* textPool) const
{
	return mTopLevel.Write(&textPool, -1);
}

// The following groups of routines are used for a C interface into GP2.
// C++ users should just use the objects as normally and not call these routines below
//
// CGenericParser2 (void *) routines
TGenericParser2 GP_Parse(char** dataPtr, const bool cleanFirst, const bool writeable)
{
	const auto parse = new CGenericParser2;
	if (parse->Parse(dataPtr, cleanFirst, writeable))
	{
		return parse;
	}

	delete parse;
	return nullptr;
}

void GP_Clean(const TGenericParser2 GP2)
{
	if (!GP2)
	{
		return;
	}

	static_cast<CGenericParser2*>(GP2)->Clean();
}

void GP_Delete(TGenericParser2* GP2)
{
	if (!GP2 || !*GP2)
	{
		return;
	}

	delete static_cast<CGenericParser2*>(*GP2);
	*GP2 = nullptr;
}

TGPGroup GP_GetBaseParseGroup(const TGenericParser2 GP2)
{
	if (!GP2)
	{
		return nullptr;
	}

	return static_cast<CGenericParser2*>(GP2)->GetBaseParseGroup();
}

// CGPGroup (void *) routines
const char* GPG_GetName(const TGPGroup GPG)
{
	if (!GPG)
	{
		return "";
	}

	return static_cast<CGPGroup*>(GPG)->GetName();
}

bool GPG_GetName(const TGPGroup GPG, char* Value)
{
	if (!GPG)
	{
		Value[0] = 0;
		return false;
	}

	strcpy(Value, static_cast<CGPGroup*>(GPG)->GetName());
	return true;
}

TGPGroup GPG_GetNext(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetNext();
}

TGPGroup GPG_GetInOrderNext(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetInOrderNext();
}

TGPGroup GPG_GetInOrderPrevious(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetInOrderPrevious();
}

TGPGroup GPG_GetPairs(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetPairs();
}

TGPGroup GPG_GetInOrderPairs(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetInOrderPairs();
}

TGPGroup GPG_GetSubGroups(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetSubGroups();
}

TGPGroup GPG_GetInOrderSubGroups(const TGPGroup GPG)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->GetInOrderSubGroups();
}

TGPGroup GPG_FindSubGroup(const TGPGroup GPG, const char* name)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->FindSubGroup(name);
}

TGPValue GPG_FindPair(const TGPGroup GPG, const char* key)
{
	if (!GPG)
	{
		return nullptr;
	}

	return static_cast<CGPGroup*>(GPG)->FindPair(key);
}

const char* GPG_FindPairValue(const TGPGroup GPG, const char* key, const char* defaultVal)
{
	if (!GPG)
	{
		return defaultVal;
	}

	return static_cast<CGPGroup*>(GPG)->FindPairValue(key, defaultVal);
}

bool GPG_FindPairValue(const TGPGroup GPG, const char* key, const char* defaultVal, char* Value)
{
	strcpy(Value, GPG_FindPairValue(GPG, key, defaultVal));

	return true;
}

// CGPValue (void *) routines
const char* GPV_GetName(const TGPValue GPV)
{
	if (!GPV)
	{
		return "";
	}

	return static_cast<CGPValue*>(GPV)->GetName();
}

bool GPV_GetName(const TGPValue GPV, char* Value)
{
	if (!GPV)
	{
		Value[0] = 0;
		return false;
	}

	strcpy(Value, static_cast<CGPValue*>(GPV)->GetName());
	return true;
}

TGPValue GPV_GetNext(const TGPValue GPV)
{
	if (!GPV)
	{
		return nullptr;
	}

	return static_cast<CGPValue*>(GPV)->GetNext();
}

TGPValue GPV_GetInOrderNext(const TGPValue GPV)
{
	if (!GPV)
	{
		return nullptr;
	}

	return static_cast<CGPValue*>(GPV)->GetInOrderNext();
}

TGPValue GPV_GetInOrderPrevious(const TGPValue GPV)
{
	if (!GPV)
	{
		return nullptr;
	}

	return static_cast<CGPValue*>(GPV)->GetInOrderPrevious();
}

bool GPV_IsList(const TGPValue GPV)
{
	if (!GPV)
	{
		return false;
	}

	return static_cast<CGPValue*>(GPV)->IsList();
}

const char* GPV_GetTopValue(const TGPValue GPV)
{
	if (!GPV)
	{
		return "";
	}

	return static_cast<CGPValue*>(GPV)->GetTopValue();
}

bool GPV_GetTopValue(const TGPValue GPV, char* Value)
{
	if (!GPV)
	{
		Value[0] = 0;
		return false;
	}

	strcpy(Value, static_cast<CGPValue*>(GPV)->GetTopValue());

	return true;
}

TGPValue GPV_GetList(const TGPValue GPV)
{
	if (!GPV)
	{
		return nullptr;
	}

	return static_cast<CGPValue*>(GPV)->GetList();
}