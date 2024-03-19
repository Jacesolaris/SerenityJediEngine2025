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

// Script Command Sequencer
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "icarus.h"

#include "assert.h"
#include <algorithm>

// Sequencer

CSequencer::CSequencer(void)
{
	m_numCommands = 0;

	m_curStream = nullptr;
	m_curSequence = nullptr;

	m_elseValid = 0;
	m_elseOwner = nullptr;

	m_curGroup = nullptr;
}

CSequencer::~CSequencer(void)
{
	Free(); //Safe even if already freed
}

/*
========================
Create

Static creation function
========================
*/

CSequencer* CSequencer::Create(void)
{
	const auto sequencer = new CSequencer;

	return sequencer;
}

/*
========================
Init

Initializes the sequencer
========================
*/
int CSequencer::Init(const int ownerID, interface_export_t* ie, CTaskManager* task_manager, ICARUS_Instance* i_icarus)
{
	m_ownerID = ownerID;
	m_owner = i_icarus;
	m_taskManager = task_manager;
	m_ie = ie;

	return SEQ_OK;
}

/*
========================
Free

Releases all resources and re-inits the sequencer
========================
*/
int CSequencer::Free(void)
{
	//Flush the sequences
	for (auto sli = m_sequences.begin(); sli != m_sequences.end(); ++sli)
	{
		m_owner->DeleteSequence(*sli);
	}

	m_sequences.clear();
	m_taskSequences.clear();

	//Clean up any other info
	m_numCommands = 0;
	m_curSequence = nullptr;

	while (!m_streamsCreated.empty())
	{
		const bstream_t* streamToDel = m_streamsCreated.back();
		DeleteStream(streamToDel);
	}

	return SEQ_OK;
}

/*
-------------------------
Flush
-------------------------
*/

int CSequencer::Flush(CSequence* owner)
{
	if (owner == nullptr)
		return SEQ_FAILED;

	Recall();

	//Flush the sequences
	for (auto sli = m_sequences.begin(); sli != m_sequences.end();)
	{
		if (*sli == owner || owner->HasChild(*sli) || (*sli)->HasFlag(SQ_PENDING) || (*sli)->HasFlag(SQ_TASK))
		{
			++sli;
			continue;
		}

		//Delete it, and remove all references
		RemoveSequence(*sli);
		m_owner->DeleteSequence(*sli);

		//Delete from the sequence list and move on
		sli = m_sequences.erase(sli);
	}

	//Make sure this owner knows it's now the root sequence
	owner->SetParent(nullptr);
	owner->SetReturn(nullptr);

	return SEQ_OK;
}

/*
========================
AddStream

Creates a stream for parsing
========================
*/

bstream_t* CSequencer::AddStream(void)
{
	const auto stream = new bstream_t; //deleted in Route()
	stream->stream = new CBlockStream; //deleted in Route()
	stream->last = m_curStream;

	m_streamsCreated.push_back(stream);

	return stream;
}

/*
========================
DeleteStream

Deletes parsing stream
========================
*/
void CSequencer::DeleteStream(const bstream_t* bstream)
{
	const auto finder = std::find(m_streamsCreated.begin(), m_streamsCreated.end(), bstream);
	if (finder != m_streamsCreated.end())
	{
		m_streamsCreated.erase(finder);
	}

	bstream->stream->Free();

	delete bstream->stream;
	delete bstream;

	bstream = nullptr;
}

/*
-------------------------
AddTaskSequence
-------------------------
*/

void CSequencer::AddTaskSequence(CSequence* sequence, CTaskGroup* group)
{
	m_taskSequences[group] = sequence;
}

/*
-------------------------
GetTaskSequence
-------------------------
*/

CSequence* CSequencer::GetTaskSequence(CTaskGroup* group)
{
	const auto tsi = m_taskSequences.find(group);

	if (tsi == m_taskSequences.end())
		return nullptr;

	return (*tsi).second;
}

/*
========================
AddSequence

Creates and adds a sequence to the sequencer
========================
*/

CSequence* CSequencer::AddSequence(void)
{
	CSequence* sequence = m_owner->GetSequence();

	assert(sequence);
	if (sequence == nullptr)
		return nullptr;

	//Add it to the list
	m_sequences.insert(m_sequences.end(), sequence);

	//FIXME: Temp fix
	sequence->SetFlag(SQ_PENDING);

	return sequence;
}

CSequence* CSequencer::AddSequence(CSequence* parent, CSequence* returnSeq, const int flags)
{
	CSequence* sequence = m_owner->GetSequence();

	assert(sequence);
	if (sequence == nullptr)
		return nullptr;

	//Add it to the list
	m_sequences.insert(m_sequences.end(), sequence);

	sequence->SetFlags(flags);
	sequence->SetParent(parent);
	sequence->SetReturn(returnSeq);

	return sequence;
}

/*
========================
GetSequence

Retrieves a sequence by its ID
========================
*/

CSequence* CSequencer::GetSequence(const int id)
{
	/*	sequenceID_m::iterator mi;

		mi = m_sequenceMap.find( id );

		if ( mi == m_sequenceMap.end() )
			return NULL;

		return (*mi).second;*/

	sequence_l::iterator iterSeq;
	STL_ITERATE(iterSeq, m_sequences)
	{
		if ((*iterSeq)->GetID() == id)
			return *iterSeq;
	}

	return nullptr;
}

/*
-------------------------
Interrupt
-------------------------
*/

void CSequencer::Interrupt(void)
{
	CBlock* command = m_taskManager->GetCurrentTask();

	if (command == nullptr)
		return;

	//Save it
	PushCommand(command, PUSH_BACK);
}

/*
========================
Run

Runs a script
========================
*/
int CSequencer::Run(char* buffer, const long size)
{
	Recall();

	//Create a new stream
	bstream_t* blockStream = AddStream();

	//Open the stream as an IBI stream
	if (!blockStream->stream->Open(buffer, size))
	{
		m_ie->I_DPrintf(WL_ERROR, "invalid stream");
		return SEQ_FAILED;
	}

	CSequence* sequence = AddSequence(nullptr, m_curSequence, SQ_COMMON);

	// Interpret the command blocks and route them properly
	if (S_FAILED(Route(sequence, blockStream)))
	{
		//Error code is set inside of Route()
		return SEQ_FAILED;
	}

	return SEQ_OK;
}

/*
========================
ParseRun

Parses a user triggered run command
========================
*/

int CSequencer::ParseRun(CBlock* block)
{
	char* buffer = nullptr;
	char newname[MAX_STRING_SIZE];

	//Get the name and format it
	COM_StripExtension(static_cast<char*>(block->GetMemberData(0)), newname, sizeof newname);

	//Get the file from the game engine
	const int buffer_size = m_ie->I_LoadFile(newname, reinterpret_cast<void**>(&buffer));

	if (buffer_size <= 0)
	{
		m_ie->I_DPrintf(WL_ERROR, "'%s' : could not open file\n", static_cast<char*>(block->GetMemberData(0)));
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	//Create a new stream for this file
	bstream_t* new_stream = AddStream();

	//Begin streaming the file
	if (!new_stream->stream->Open(buffer, buffer_size))
	{
		m_ie->I_DPrintf(WL_ERROR, "invalid stream");
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	//Create a new sequence
	CSequence* new_sequence = AddSequence(m_curSequence, m_curSequence, SQ_RUN | SQ_PENDING);

	m_curSequence->AddChild(new_sequence);

	// Interpret the command blocks and route them properly
	if (S_FAILED(Route(new_sequence, new_stream)))
	{
		//Error code is set inside of Route()
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	m_curSequence = m_curSequence->GetReturn();

	assert(m_curSequence);

	block->Write(TK_FLOAT, static_cast<float>(new_sequence->GetID()));
	PushCommand(block, PUSH_FRONT);

	return SEQ_OK;
}

/*
========================
ParseIf

Parses an if statement
========================
*/

int CSequencer::ParseIf(CBlock* block, bstream_t* bstream)
{
	//Create the container sequence
	CSequence* sequence = AddSequence(m_curSequence, m_curSequence, SQ_CONDITIONAL);

	assert(sequence);
	if (sequence == nullptr)
	{
		m_ie->I_DPrintf(WL_ERROR, "ParseIf: failed to allocate container sequence");
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	m_curSequence->AddChild(sequence);

	//Add a unique conditional identifier to the block for reference later
	block->Write(TK_FLOAT, static_cast<float>(sequence->GetID()));

	//Push this onto the stack to mark the conditional entrance
	PushCommand(block, PUSH_FRONT);

	//Recursively obtain the conditional body
	Route(sequence, bstream);

	m_elseValid = 2;
	m_elseOwner = block;

	return SEQ_OK;
}

/*
========================
ParseElse

Parses an else statement
========================
*/

int CSequencer::ParseElse(const CBlock* block, bstream_t* bstream)
{
	//The else is not retained
	delete block;
	block = nullptr;

	//Create the container sequence
	CSequence* sequence = AddSequence(m_curSequence, m_curSequence, SQ_CONDITIONAL);

	assert(sequence);
	if (sequence == nullptr)
	{
		m_ie->I_DPrintf(WL_ERROR, "ParseIf: failed to allocate container sequence");
		return SEQ_FAILED;
	}

	m_curSequence->AddChild(sequence);

	//Add a unique conditional identifier to the block for reference later
	//TODO: Emit warning
	if (m_elseOwner == nullptr)
	{
		m_ie->I_DPrintf(WL_ERROR, "Invalid 'else' found!\n");
		return SEQ_FAILED;
	}

	m_elseOwner->Write(TK_FLOAT, static_cast<float>(sequence->GetID()));

	m_elseOwner->SetFlag(BF_ELSE);

	//Recursively obtain the conditional body
	Route(sequence, bstream);

	m_elseValid = 0;
	m_elseOwner = nullptr;

	return SEQ_OK;
}

/*
========================
ParseLoop

Parses a loop command
========================
*/

int CSequencer::ParseLoop(CBlock* block, bstream_t* bstream)
{
	int memberNum = 0;

	//Set the parent
	CSequence* sequence = AddSequence(m_curSequence, m_curSequence, SQ_LOOP | SQ_RETAIN);

	assert(sequence);
	if (sequence == nullptr)
	{
		m_ie->I_DPrintf(WL_ERROR, "ParseLoop : failed to allocate container sequence");
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	m_curSequence->AddChild(sequence);

	//Set the number of iterations of this sequence

	const CBlockMember* bm = block->GetMember(memberNum++);

	if (bm->GetID() == ID_RANDOM)
	{
		//Parse out the random number
		const float min = *static_cast<float*>(block->GetMemberData(memberNum++));
		const float max = *static_cast<float*>(block->GetMemberData(memberNum++));

		const int rIter = static_cast<int>(m_ie->I_Random(min, max));
		sequence->SetIterations(rIter);
	}
	else
	{
		sequence->SetIterations(static_cast<int>(*static_cast<float*>(bm->GetData())));
	}

	//Add a unique loop identifier to the block for reference later
	block->Write(TK_FLOAT, static_cast<float>(sequence->GetID()));

	//Push this onto the stack to mark the loop entrance
	PushCommand(block, PUSH_FRONT);

	//Recursively obtain the loop
	Route(sequence, bstream);

	return SEQ_OK;
}

/*
========================
AddAffect

Adds a sequence that is saved until the affect is called by the parent
========================
*/

int CSequencer::AddAffect(const bstream_t* bstream, const int retain, int* id)
{
	CSequence* sequence = AddSequence();
	bstream_t new_stream{};

	sequence->SetFlag(SQ_AFFECT | SQ_PENDING);

	if (retain)
		sequence->SetFlag(SQ_RETAIN);

	//This will be replaced once it's actually used, but this will restore the route state properly
	sequence->SetReturn(m_curSequence);

	//We need this as a temp holder
	new_stream.last = m_curStream;
	new_stream.stream = bstream->stream;

	if S_FAILED(Route(sequence, &new_stream))
	{
		return SEQ_FAILED;
	}

	*id = sequence->GetID();

	sequence->SetReturn(nullptr);

	return SEQ_OK;
}

/*
========================
ParseAffect

Parses an affect command
========================
*/

int CSequencer::ParseAffect(CBlock* block, bstream_t* bstream)
{
	CSequencer* stream_sequencer = nullptr;
	int ret;

	const auto entname = static_cast<char*>(block->GetMemberData(0));
	const sharedEntity_t* ent = m_ie->I_GetEntityByName(entname);

	if (!ent) // if there wasn't a valid entname in the affect, we need to check if it's a get command
	{
		//try to parse a 'get' command that is embeded in this 'affect'

		char* p1 = nullptr;
		//
		//	Get the first parameter (this should be the get)
		//
		const CBlockMember* bm = block->GetMember(0);
		const int id = bm->GetID();

		switch (id)
		{
			// these 3 cases probably aren't necessary
		case TK_STRING:
		case TK_IDENTIFIER:
		case TK_CHAR:
			p1 = static_cast<char*>(bm->GetData());
			break;

		case ID_GET:
		{
			//get( TYPE, NAME )
			const int type = static_cast<int>(*static_cast<float*>(block->GetMemberData(1)));
			const char* name = static_cast<char*>(block->GetMemberData(2));

			switch (type) // what type are they attempting to get
			{
			case TK_STRING:
				//only string is acceptable for affect, store result in p1
				if (m_ie->I_GetString(m_ownerID, type, name, &p1) == false)
				{
					delete block;
					block = nullptr;
					return false;
				}
				break;
			default:
				//FIXME: Make an enum id for the error...
				m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on affect _1");
				delete block;
				block = nullptr;
				return false;
			}

			break;
		}

		default:
			//FIXME: Make an enum id for the error...
			m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on affect _2");
			delete block;
			block = nullptr;
			return false;
		} //end id switch

		if (p1)
		{
			ent = m_ie->I_GetEntityByName(p1);
		}
		if (!ent)
		{
			// a valid entity name was not returned from the get command
			m_ie->I_DPrintf(WL_WARNING, "'%s' : invalid affect() target\n");
		}
	} // end if(!ent)

	if (ent)
	{
		stream_sequencer = gSequencers[ent->s.number]; //ent->sequencer;
	}

	if (stream_sequencer == nullptr)
	{
		m_ie->I_DPrintf(WL_WARNING, "'%s' : invalid affect() target\n", entname);

		//Fast-forward out of this affect block onto the next valid code
		CSequence* backSeq = m_curSequence;

		CSequence* trashSeq = m_owner->GetSequence();
		Route(trashSeq, bstream);
		Recall();
		DestroySequence(trashSeq);
		m_curSequence = backSeq;
		delete block;
		block = nullptr;
		return SEQ_OK;
	}

	if S_FAILED(stream_sequencer->AddAffect(bstream, (int)m_curSequence->HasFlag(SQ_RETAIN), &ret))
	{
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	//Hold onto the id for later use
	//FIXME: If the target sequence is freed, what then?		(!suspect!)

	block->Write(TK_FLOAT, static_cast<float>(ret));

	PushCommand(block, PUSH_FRONT);
	/*
	//Don't actually do these right now, we're just pre-processing (parsing) the affect
	if( ent )
	{	// ents need to update upon being affected
		ent->task_manager->Update();
	}
	*/

	return SEQ_OK;
}

/*
-------------------------
ParseTask
-------------------------
*/

int CSequencer::ParseTask(const CBlock* block, bstream_t* bstream)
{
	//Setup the container sequence
	CSequence* sequence = AddSequence(m_curSequence, m_curSequence, SQ_TASK | SQ_RETAIN);
	m_curSequence->AddChild(sequence);

	//Get the name of this task for reference later
	const auto taskName = static_cast<const char*>(block->GetMemberData(0));

	//Get a new task group from the task manager
	CTaskGroup* group = m_taskManager->AddTaskGroup(taskName);

	if (group == nullptr)
	{
		m_ie->I_DPrintf(WL_ERROR, "error : unable to allocate a new task group");
		delete block;
		block = nullptr;
		return SEQ_FAILED;
	}

	//The current group is set to this group, all subsequent commands (until a block end) will fall into this task group
	group->SetParent(m_curGroup);
	m_curGroup = group;

	//Keep an association between this task and the container sequence
	AddTaskSequence(sequence, group);

	//PushCommand( block, PUSH_FRONT );
	delete block;
	block = nullptr;

	//Recursively obtain the loop
	Route(sequence, bstream);

	return SEQ_OK;
}

/*
========================
Route

Properly handles and routes commands to the sequencer
========================
*/

//FIXME: Re-entering this code will produce unpredictable results if a script has already been routed and is running currently

//FIXME: A sequencer cannot properly affect itself

int CSequencer::Route(CSequence* sequence, bstream_t* bstream)
{
	CBlock* block;

	//Take the stream as the current stream
	m_curStream = bstream;
	CBlockStream* stream = bstream->stream;

	m_curSequence = sequence;

	//Obtain all blocks
	while (stream->BlockAvailable())
	{
		block = new CBlock; //deleted in Free()
		stream->ReadBlock(block);

		//TEMP: HACK!
		if (m_elseValid)
			m_elseValid--;

		switch (block->GetBlockID())
		{
			//Marks the end of a blocked section
		case ID_BLOCK_END:

			//Save this as a pre-process marker
			PushCommand(block, PUSH_FRONT);

			if (m_curSequence->HasFlag(SQ_RUN) || m_curSequence->HasFlag(SQ_AFFECT))
			{
				//Go back to the last stream
				m_curStream = bstream->last;
			}

			if (m_curSequence->HasFlag(SQ_TASK))
			{
				//Go back to the last stream
				m_curStream = bstream->last;
				m_curGroup = m_curGroup->GetParent();
			}

			m_curSequence = m_curSequence->GetReturn();

			return SEQ_OK;

			//Affect pre-processor
		case ID_AFFECT:

			if S_FAILED(ParseAffect(block, bstream))
				return SEQ_FAILED;

			break;

			//Run pre-processor
		case ID_RUN:

			if S_FAILED(ParseRun(block))
				return SEQ_FAILED;

			break;

			//Loop pre-processor
		case ID_LOOP:

			if S_FAILED(ParseLoop(block, bstream))
				return SEQ_FAILED;

			break;

			//Conditional pre-processor
		case ID_IF:

			if S_FAILED(ParseIf(block, bstream))
				return SEQ_FAILED;

			break;

		case ID_ELSE:

			//TODO: Emit warning
			if (m_elseValid == 0)
			{
				m_ie->I_DPrintf(WL_ERROR, "Invalid 'else' found!\n");
				return SEQ_FAILED;
			}

			if S_FAILED(ParseElse(block, bstream))
				return SEQ_FAILED;

			break;

		case ID_TASK:

			if S_FAILED(ParseTask(block, bstream))
				return SEQ_FAILED;

			break;

			//FIXME: For now this is to catch problems, but can ultimately be removed
		case ID_WAIT:
		case ID_PRINT:
		case ID_SOUND:
		case ID_MOVE:
		case ID_ROTATE:
		case ID_SET:
		case ID_USE:
		case ID_REMOVE:
		case ID_KILL:
		case ID_FLUSH:
		case ID_CAMERA:
		case ID_DO:
		case ID_DECLARE:
		case ID_FREE:
		case ID_SIGNAL:
		case ID_WAITSIGNAL:
		case ID_PLAY:

			//Commands go directly into the sequence without pre-process
			PushCommand(block, PUSH_FRONT);
			break;

			//Error
		default:

			m_ie->I_DPrintf(WL_ERROR, "'%d' : invalid block ID", block->GetBlockID());

			return SEQ_FAILED;
		}
	}

	//Check for a run sequence, it must be marked
	if (m_curSequence->HasFlag(SQ_RUN))
	{
		block = new CBlock;
		block->Create(ID_BLOCK_END);
		PushCommand(block, PUSH_FRONT); //mark the end of the run

		/*
		//Free the stream
		m_curStream = bstream->last;
		DeleteStream( bstream );
		*/

		return SEQ_OK;
	}

	//Check to start the communication
	if (bstream->last == nullptr && m_numCommands > 0)
	{
		//Everything is routed, so get it all rolling
		Prime(m_taskManager, PopCommand(POP_BACK));
	}

	m_curStream = bstream->last;

	//Free the stream
	DeleteStream(bstream);

	return SEQ_OK;
}

/*
========================
CheckRun

Checks for run command pre-processing
========================
*/

//Directly changes the parameter to avoid excess push/pop

void CSequencer::CheckRun(CBlock** command)
{
	CBlock* block = *command;

	if (block == nullptr)
		return;

	//Check for a run command
	if (block->GetBlockID() == ID_RUN)
	{
		const int id = static_cast<int>(*static_cast<float*>(block->GetMemberData(1)));

		m_ie->I_DPrintf(WL_DEBUG, "%4d run( \"%s\" ); [%d]", m_ownerID, static_cast<char*>(block->GetMemberData(0)),
			m_ie->I_GetTime());

		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;

			*command = nullptr;
		}

		m_curSequence = GetSequence(id);

		//TODO: Emit warning
		assert(m_curSequence);
		if (m_curSequence == nullptr)
		{
			m_ie->I_DPrintf(WL_ERROR, "Unable to find 'run' sequence!\n");
			*command = nullptr;
			return;
		}

		if (m_curSequence->GetNumCommands() > 0)
		{
			*command = PopCommand(POP_BACK);

			Prep(command); //Account for any other pre-processes
			return;
		}

		return;
	}

	//Check for the end of a run
	if (block->GetBlockID() == ID_BLOCK_END && m_curSequence->HasFlag(SQ_RUN))
	{
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		m_curSequence = ReturnSequence(m_curSequence);

		if (m_curSequence && m_curSequence->GetNumCommands() > 0)
		{
			*command = PopCommand(POP_BACK);

			Prep(command); //Account for any other pre-processes
		}

		//FIXME: Check this...
	}
}

/*
-------------------------
EvaluateConditional
-------------------------
*/

//FIXME: This function will be written better later once the functionality of the ideas here are tested

int CSequencer::EvaluateConditional(CBlock* block) const
{
	CBlockMember* bm;
	char tempString1[128], tempString2[128];
	vector_t vec{};
	int id, i, oper, memberNum = 0;
	char* p1 = nullptr, * p2 = nullptr;
	int t1, t2;

	//
	//	Get the first parameter
	//

	bm = block->GetMember(memberNum++);
	id = bm->GetID();

	t1 = id;

	switch (id)
	{
	case TK_FLOAT:
		Com_sprintf(tempString1, sizeof tempString1, "%.3f", *static_cast<float*>(bm->GetData()));
		p1 = static_cast<char*>(tempString1);
		break;

	case TK_VECTOR:

		tempString1[0] = '\0';

		for (i = 0; i < 3; i++)
		{
			bm = block->GetMember(memberNum++);
			vec[i] = *static_cast<float*>(bm->GetData());
		}

		Com_sprintf(tempString1, sizeof tempString1, "%.3f %.3f %.3f", vec[0], vec[1], vec[2]);
		p1 = static_cast<char*>(tempString1);

		break;

	case TK_STRING:
	case TK_IDENTIFIER:
	case TK_CHAR:

		p1 = static_cast<char*>(bm->GetData());
		break;

	case ID_GET:
	{
		int type;
		char* name;

		//get( TYPE, NAME )
		type = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum++)));
		name = static_cast<char*>(block->GetMemberData(memberNum++));

		//Get the type returned and hold onto it
		t1 = type;

		switch (type)
		{
		case TK_FLOAT:
		{
			float fVal;

			if (m_ie->I_GetFloat(m_ownerID, type, name, &fVal) == false)
				return false;

			Com_sprintf(tempString1, sizeof tempString1, "%.3f", fVal);
			p1 = static_cast<char*>(tempString1);
		}

		break;

		case TK_INT:
		{
			float fVal;

			if (m_ie->I_GetFloat(m_ownerID, type, name, &fVal) == false)
				return false;

			Com_sprintf(tempString1, sizeof tempString1, "%d", static_cast<int>(fVal));
			p1 = static_cast<char*>(tempString1);
		}
		break;

		case TK_STRING:

			if (m_ie->I_GetString(m_ownerID, type, name, &p1) == false)
				return false;

			break;

		case TK_VECTOR:
		{
			vector_t vVal;

			if (m_ie->I_GetVector(m_ownerID, type, name, vVal) == false)
				return false;

			Com_sprintf(tempString1, sizeof tempString1, "%.3f %.3f %.3f", vVal[0], vVal[1], vVal[2]);
			p1 = static_cast<char*>(tempString1);
		}

		break;
		default:;
		}

		break;
	}

	case ID_RANDOM:
	{
		float min, max;
		//FIXME: This will not account for nested Q_flrand(0.0f, 1.0f) statements

		min = *static_cast<float*>(block->GetMemberData(memberNum++));
		max = *static_cast<float*>(block->GetMemberData(memberNum++));

		//A float value is returned from the function
		t1 = TK_FLOAT;

		Com_sprintf(tempString1, sizeof tempString1, "%.3f", m_ie->I_Random(min, max));
		p1 = static_cast<char*>(tempString1);
	}

	break;

	case ID_TAG:
	{
		char* name;
		float type;

		name = static_cast<char*>(block->GetMemberData(memberNum++));
		type = *static_cast<float*>(block->GetMemberData(memberNum++));

		t1 = TK_VECTOR;

		//TODO: Emit warning
		if (m_ie->I_GetTag(m_ownerID, name, static_cast<int>(type), vec) == false)
		{
			m_ie->I_DPrintf(WL_ERROR, "Unable to find tag \"%s\"!\n", name);
			return false;
		}

		Com_sprintf(tempString1, sizeof tempString1, "%.3f %.3f %.3f", vec[0], vec[1], vec[2]);
		p1 = static_cast<char*>(tempString1);

		break;
	}

	default:
		//FIXME: Make an enum id for the error...
		m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on conditional");
		return false;
	}

	//
	//	Get the comparison operator
	//

	bm = block->GetMember(memberNum++);
	id = bm->GetID();

	switch (id)
	{
	case TK_EQUALS:
	case TK_GREATER_THAN:
	case TK_LESS_THAN:
	case TK_NOT:
		oper = id;
		break;

	default:
		m_ie->I_DPrintf(WL_ERROR, "Invalid operator type found on conditional!\n");
		return false; //FIXME: Emit warning
	}

	//
	//	Get the second parameter
	//

	bm = block->GetMember(memberNum++);
	id = bm->GetID();

	t2 = id;

	switch (id)
	{
	case TK_FLOAT:
		Com_sprintf(tempString2, sizeof tempString2, "%.3f", *static_cast<float*>(bm->GetData()));
		p2 = static_cast<char*>(tempString2);
		break;

	case TK_VECTOR:

		tempString2[0] = '\0';

		for (i = 0; i < 3; i++)
		{
			bm = block->GetMember(memberNum++);
			vec[i] = *static_cast<float*>(bm->GetData());
		}

		Com_sprintf(tempString2, sizeof tempString2, "%.3f %.3f %.3f", vec[0], vec[1], vec[2]);
		p2 = static_cast<char*>(tempString2);

		break;

	case TK_STRING:
	case TK_IDENTIFIER:
	case TK_CHAR:

		p2 = static_cast<char*>(bm->GetData());
		break;

	case ID_GET:
	{
		int type;
		char* name;

		//get( TYPE, NAME )
		type = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum++)));
		name = static_cast<char*>(block->GetMemberData(memberNum++));

		//Get the type returned and hold onto it
		t2 = type;

		switch (type)
		{
		case TK_FLOAT:
		{
			float fVal;

			if (m_ie->I_GetFloat(m_ownerID, type, name, &fVal) == false)
				return false;

			Com_sprintf(tempString2, sizeof tempString2, "%.3f", fVal);
			p2 = static_cast<char*>(tempString2);
		}

		break;

		case TK_INT:
		{
			float fVal;

			if (m_ie->I_GetFloat(m_ownerID, type, name, &fVal) == false)
				return false;

			Com_sprintf(tempString2, sizeof tempString2, "%d", static_cast<int>(fVal));
			p2 = static_cast<char*>(tempString2);
		}
		break;

		case TK_STRING:

			if (m_ie->I_GetString(m_ownerID, type, name, &p2) == false)
				return false;

			break;

		case TK_VECTOR:
		{
			vector_t vVal;

			if (m_ie->I_GetVector(m_ownerID, type, name, vVal) == false)
				return false;

			Com_sprintf(tempString2, sizeof tempString2, "%.3f %.3f %.3f", vVal[0], vVal[1], vVal[2]);
			p2 = static_cast<char*>(tempString2);
		}

		break;
		default:;
		}

		break;
	}

	case ID_RANDOM:

	{
		float min, max;
		//FIXME: This will not account for nested Q_flrand(0.0f, 1.0f) statements

		min = *static_cast<float*>(block->GetMemberData(memberNum++));
		max = *static_cast<float*>(block->GetMemberData(memberNum++));

		//A float value is returned from the function
		t2 = TK_FLOAT;

		Com_sprintf(tempString2, sizeof tempString2, "%.3f", m_ie->I_Random(min, max));
		p2 = static_cast<char*>(tempString2);
	}

	break;

	case ID_TAG:

	{
		char* name;
		float type;

		name = static_cast<char*>(block->GetMemberData(memberNum++));
		type = *static_cast<float*>(block->GetMemberData(memberNum++));

		t2 = TK_VECTOR;

		//TODO: Emit warning
		if (m_ie->I_GetTag(m_ownerID, name, static_cast<int>(type), vec) == false)
		{
			m_ie->I_DPrintf(WL_ERROR, "Unable to find tag \"%s\"!\n", name);
			return false;
		}

		Com_sprintf(tempString2, sizeof tempString2, "%.3f %.3f %.3f", vec[0], vec[1], vec[2]);
		p2 = static_cast<char*>(tempString2);

		break;
	}

	default:
		//FIXME: Make an enum id for the error...
		m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on conditional");
		return false;
	}

	return m_ie->I_Evaluate(t1, p1, t2, p2, oper);
}

/*
========================
CheckIf

Checks for if statement pre-processing
========================
*/

void CSequencer::CheckIf(CBlock** command)
{
	CBlock* block = *command;

	if (block == nullptr)
		return;

	if (block->GetBlockID() == ID_IF)
	{
		const int ret = EvaluateConditional(block);

		if (ret /*TRUE*/)
		{
			int successID;
			if (block->HasFlag(BF_ELSE))
			{
				successID = static_cast<int>(*static_cast<float*>(block->GetMemberData(block->GetNumMembers() - 2)));
			}
			else
			{
				successID = static_cast<int>(*static_cast<float*>(block->GetMemberData(block->GetNumMembers() - 1)));
			}

			CSequence* successSeq = GetSequence(successID);

			//TODO: Emit warning
			assert(successSeq);
			if (successSeq == nullptr)
			{
				m_ie->I_DPrintf(WL_ERROR, "Unable to find conditional success sequence!\n");
				*command = nullptr;
				return;
			}

			//Only save the conditional statement if the calling sequence is retained
			if (m_curSequence->HasFlag(SQ_RETAIN))
			{
				PushCommand(block, PUSH_FRONT);
			}
			else
			{
				delete block;
				block = nullptr;
				*command = nullptr;
			}

			m_curSequence = successSeq;

			//Recursively work out any other pre-processors
			*command = PopCommand(POP_BACK);
			Prep(command);

			return;
		}

		if (ret == false && block->HasFlag(BF_ELSE))
		{
			const int failureID = static_cast<int>(*static_cast<float*>(block->
				GetMemberData(block->GetNumMembers() - 1)));
			CSequence* failureSeq = GetSequence(failureID);

			//TODO: Emit warning
			assert(failureSeq);
			if (failureSeq == nullptr)
			{
				m_ie->I_DPrintf(WL_ERROR, "Unable to find conditional failure sequence!\n");
				*command = nullptr;
				return;
			}

			//Only save the conditional statement if the calling sequence is retained
			if (m_curSequence->HasFlag(SQ_RETAIN))
			{
				PushCommand(block, PUSH_FRONT);
			}
			else
			{
				delete block;
				block = nullptr;
				*command = nullptr;
			}

			m_curSequence = failureSeq;

			//Recursively work out any other pre-processors
			*command = PopCommand(POP_BACK);
			Prep(command);

			return;
		}

		//Only save the conditional statement if the calling sequence is retained
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		//Conditional failed, just move on to the next command
		*command = PopCommand(POP_BACK);
		Prep(command);

		return;
	}

	if (block->GetBlockID() == ID_BLOCK_END && m_curSequence->HasFlag(SQ_CONDITIONAL))
	{
		assert(m_curSequence->GetReturn());
		if (m_curSequence->GetReturn() == nullptr)
		{
			*command = nullptr;
			return;
		}

		//Check to retain it
		if (m_curSequence->GetParent()->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		//Back out of the conditional and resume the previous sequence
		m_curSequence = ReturnSequence(m_curSequence);

		//This can safely happen
		if (m_curSequence == nullptr)
		{
			*command = nullptr;
			return;
		}

		*command = PopCommand(POP_BACK);
		Prep(command);
	}
}

/*
========================
CheckLoop

Checks for loop command pre-processing
========================
*/

void CSequencer::CheckLoop(CBlock** command)
{
	CBlock* block = *command;

	if (block == nullptr)
		return;

	//Check for a loop
	if (block->GetBlockID() == ID_LOOP)
	{
		int memberNum = 0;
		int iterations;
		//Get the loop ID
		const CBlockMember* bm = block->GetMember(memberNum++);

		if (bm->GetID() == ID_RANDOM)
		{
			//Parse out the random number
			const float min = *static_cast<float*>(block->GetMemberData(memberNum++));
			const float max = *static_cast<float*>(block->GetMemberData(memberNum++));

			iterations = static_cast<int>(m_ie->I_Random(min, max));
		}
		else
		{
			iterations = static_cast<int>(*static_cast<float*>(bm->GetData()));
		}

		const int loopID = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum++)));

		CSequence* loop = GetSequence(loopID);

		//TODO: Emit warning
		assert(loop);
		if (loop == nullptr)
		{
			m_ie->I_DPrintf(WL_ERROR, "Unable to find 'loop' sequence!\n");
			*command = nullptr;
			return;
		}

		assert(loop->GetParent());
		if (loop->GetParent() == nullptr)
		{
			*command = nullptr;
			return;
		}

		//Restore the count if it has been lost
		loop->SetIterations(iterations);

		//Only save the loop command if the calling sequence is retained
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		m_curSequence = loop;

		//Recursively work out any other pre-processors
		*command = PopCommand(POP_BACK);
		Prep(command);

		return;
	}

	//Check for the end of the loop
	if (block->GetBlockID() == ID_BLOCK_END && m_curSequence->HasFlag(SQ_LOOP))
	{
		//We don't want to decrement -1
		if (m_curSequence->GetIterations() > 0)
			m_curSequence->SetIterations(m_curSequence->GetIterations() - 1); //Nice, eh?

		//Either there's another iteration, or it's infinite
		if (m_curSequence->GetIterations() != 0)
		{
			//Another iteration is going to happen, so this will need to be considered again
			PushCommand(block, PUSH_FRONT);

			*command = PopCommand(POP_BACK);
			Prep(command);

			return;
		}
		assert(m_curSequence->GetReturn());
		if (m_curSequence->GetReturn() == nullptr)
		{
			*command = nullptr;
			return;
		}

		//Check to retain it
		if (m_curSequence->GetParent()->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		//Back out of the loop and resume the previous sequence
		m_curSequence = ReturnSequence(m_curSequence);

		//This can safely happen
		if (m_curSequence == nullptr)
		{
			*command = nullptr;
			return;
		}

		*command = PopCommand(POP_BACK);
		Prep(command);
	}
}

/*
========================
CheckFlush

Checks for flush command pre-processing
========================
*/

void CSequencer::CheckFlush(CBlock** command)
{
	CBlock* block = *command;

	if (block == nullptr)
		return;

	if (block->GetBlockID() == ID_FLUSH)
	{
		//Flush the sequence
		Flush(m_curSequence);

		//Check to retain it
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		*command = PopCommand(POP_BACK);
		Prep(command);
	}
}

/*
========================
CheckAffect

Checks for affect command pre-processing
========================
*/

void CSequencer::CheckAffect(CBlock** command)
{
	CBlock* block = *command;
	const sharedEntity_t* ent = nullptr;

	if (block == nullptr)
	{
		return;
	}

	if (block->GetBlockID() == ID_AFFECT)
	{
		int memberNum = 0;
		CSequencer* sequencer = nullptr;
		const char* entname = static_cast<char*>(block->GetMemberData(memberNum++));
		ent = m_ie->I_GetEntityByName(entname);

		if (!ent) // if there wasn't a valid entname in the affect, we need to check if it's a get command
		{
			//try to parse a 'get' command that is embeded in this 'affect'

			char* p1 = nullptr;
			//
			//	Get the first parameter (this should be the get)
			//
			const CBlockMember* bm = block->GetMember(0);
			const int id = bm->GetID();

			switch (id)
			{
				// these 3 cases probably aren't necessary
			case TK_STRING:
			case TK_IDENTIFIER:
			case TK_CHAR:
				p1 = static_cast<char*>(bm->GetData());
				break;

			case ID_GET:
			{
				//get( TYPE, NAME )
				const int type = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum++)));
				const char* name = static_cast<char*>(block->GetMemberData(memberNum++));

				switch (type) // what type are they attempting to get
				{
				case TK_STRING:
					//only string is acceptable for affect, store result in p1
					if (m_ie->I_GetString(m_ownerID, type, name, &p1) == false)
					{
						return;
					}
					break;
				default:
					//FIXME: Make an enum id for the error...
					m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on affect _1");
					return;
				}

				break;
			}

			default:
				//FIXME: Make an enum id for the error...
				m_ie->I_DPrintf(WL_ERROR, "Invalid parameter type on affect _2");
				return;
			} //end id switch

			if (p1)
			{
				ent = m_ie->I_GetEntityByName(p1);
			}
			if (!ent)
			{
				// a valid entity name was not returned from the get command
				m_ie->I_DPrintf(WL_WARNING, "'%s' : invalid affect() target\n");
			}
		} // end if(!ent)

		if (ent)
		{
			sequencer = gSequencers[ent->s.number]; //ent->sequencer;
		}
		if (memberNum == 0)
		{
			//there was no get, increment manually before next step
			memberNum++;
		}
		const int type = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum)));
		const int id = static_cast<int>(*static_cast<float*>(block->GetMemberData(memberNum + 1)));

		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		//NOTENOTE: If this isn't found, continue on to the next command
		if (sequencer == nullptr)
		{
			*command = PopCommand(POP_BACK);
			Prep(command);
			return;
		}

		sequencer->Affect(id, type);

		*command = PopCommand(POP_BACK);
		Prep(command);
		if (ent)
		{
			// ents need to update upon being affected
			//ent->task_manager->Update();
			gTaskManagers[ent->s.number]->Update();
		}

		return;
	}

	if (block->GetBlockID() == ID_BLOCK_END && m_curSequence->HasFlag(SQ_AFFECT))
	{
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		m_curSequence = ReturnSequence(m_curSequence);

		if (m_curSequence == nullptr)
		{
			*command = nullptr;
			return;
		}

		*command = PopCommand(POP_BACK);
		Prep(command);
		if (ent)
		{
			// ents need to update upon being affected
			//ent->task_manager->Update();
			gTaskManagers[ent->s.number]->Update();
		}
	}
}

/*
-------------------------
CheckDo
-------------------------
*/

void CSequencer::CheckDo(CBlock** command)
{
	CBlock* block = *command;

	if (block == nullptr)
		return;

	if (block->GetBlockID() == ID_DO)
	{
		//Get the sequence
		const auto groupName = static_cast<const char*>(block->GetMemberData(0));
		CTaskGroup* group = m_taskManager->GetTaskGroup(groupName);
		CSequence* sequence = GetTaskSequence(group);

		//TODO: Emit warning
		assert(group);
		if (group == nullptr)
		{
			//TODO: Give name/number of entity trying to execute, too
			m_ie->I_DPrintf(WL_ERROR, "ICARUS Unable to find task group \"%s\"!\n", groupName);
			*command = nullptr;
			return;
		}

		//TODO: Emit warning
		assert(sequence);
		if (sequence == nullptr)
		{
			//TODO: Give name/number of entity trying to execute, too
			m_ie->I_DPrintf(WL_ERROR, "ICARUS Unable to find task 'group' sequence!\n", groupName);
			*command = nullptr;
			return;
		}

		//Only save the loop command if the calling sequence is retained
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		//Set this to our current sequence
		sequence->SetReturn(m_curSequence);
		m_curSequence = sequence;

		group->SetParent(m_curGroup);
		m_curGroup = group;

		//Mark all the following commands as being in the task
		m_taskManager->MarkTask(group->GetGUID(), TASK_START);

		//Recursively work out any other pre-processors
		*command = PopCommand(POP_BACK);
		Prep(command);

		return;
	}

	if (block->GetBlockID() == ID_BLOCK_END && m_curSequence->HasFlag(SQ_TASK))
	{
		if (m_curSequence->HasFlag(SQ_RETAIN))
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
			*command = nullptr;
		}

		m_taskManager->MarkTask(m_curGroup->GetGUID(), TASK_END);
		m_curGroup = m_curGroup->GetParent();

		CSequence* returnSeq = ReturnSequence(m_curSequence);
		m_curSequence->SetReturn(nullptr);
		m_curSequence = returnSeq;

		if (m_curSequence == nullptr)
		{
			*command = nullptr;
			return;
		}

		*command = PopCommand(POP_BACK);
		Prep(command);
	}
}

/*
========================
Prep

Handles internal sequencer maintenance
========================
*/

void CSequencer::Prep(CBlock** command)
{
	//Check all pre-processes
	CheckAffect(command);
	CheckFlush(command);
	CheckLoop(command);
	CheckRun(command);
	CheckIf(command);
	CheckDo(command);
}

/*
========================
Prime

Starts communication between the task manager and this sequencer
========================
*/

int CSequencer::Prime(CTaskManager* task_manager, CBlock* command)
{
	Prep(&command);

	if (command)
	{
		task_manager->SetCommand(command, PUSH_BACK);
	}

	return SEQ_OK;
}

/*
========================
Callback

Handles a completed task and returns a new task to be completed
========================
*/

int CSequencer::Callback(CTaskManager* task_manager, CBlock* block, const int returnCode)
{
	CBlock* command;

	if (returnCode == TASK_RETURN_COMPLETE)
	{
		//There are no more pending commands
		if (m_curSequence == nullptr)
		{
			delete block;
			block = nullptr;
			return SEQ_OK;
		}

		//Check to retain the command
		if (m_curSequence->HasFlag(SQ_RETAIN)) //This isn't true for affect sequences...?
		{
			PushCommand(block, PUSH_FRONT);
		}
		else
		{
			delete block;
			block = nullptr;
		}

		//Check for pending commands
		if (m_curSequence->GetNumCommands() <= 0)
		{
			if (m_curSequence->GetReturn() == nullptr)
				return SEQ_OK;

			m_curSequence = m_curSequence->GetReturn();
		}

		command = PopCommand(POP_BACK);
		Prep(&command);

		if (command)
			task_manager->SetCommand(command, PUSH_FRONT);

		return SEQ_OK;
	}

	//FIXME: This could be more descriptive
	m_ie->I_DPrintf(WL_ERROR, "command could not be called back\n");
	assert(0);

	return SEQ_FAILED;
}

/*
-------------------------
Recall
-------------------------
*/

int CSequencer::Recall(void)
{
	CBlock* block;

	if (!m_taskManager)
	{
		//uh.. ok.
		assert(0);
		return true;
	}

	while ((block = m_taskManager->RecallTask()) != nullptr)
	{
		if (m_curSequence)
		{
			PushCommand(block, PUSH_BACK);
		}
		else
		{
			delete block;
			block = nullptr;
		}
	}

	return true;
}

/*
-------------------------
Affect
-------------------------
*/

int CSequencer::Affect(const int id, const int type)
{
	CSequence* sequence = GetSequence(id);

	if (sequence == nullptr)
	{
		return SEQ_FAILED;
	}

	switch (type)
	{
	case TYPE_FLUSH:

		//Get rid of all old code
		Flush(sequence);

		sequence->RemoveFlag(SQ_PENDING, true);

		m_curSequence = sequence;

		Prime(m_taskManager, PopCommand(POP_BACK));

		break;

	case TYPE_INSERT:

		Recall();

		sequence->SetReturn(m_curSequence);

		sequence->RemoveFlag(SQ_PENDING, true);

		m_curSequence = sequence;

		Prime(m_taskManager, PopCommand(POP_BACK));

		break;

	default:
		m_ie->I_DPrintf(WL_ERROR, "unknown affect type found");
		break;
	}

	return SEQ_OK;
}

/*
========================
PushCommand

Pushes a commands onto the current sequence
========================
*/

int CSequencer::PushCommand(CBlock* command, const int flag)
{
	//Make sure everything is ok
	assert(m_curSequence);
	if (m_curSequence == nullptr)
		return SEQ_FAILED;

	m_curSequence->PushCommand(command, flag);
	m_numCommands++;

	//Invalid flag
	return SEQ_OK;
}

/*
========================
PopCommand

Pops a command off the current sequence
========================
*/

CBlock* CSequencer::PopCommand(const int flag)
{
	//Make sure everything is ok
	if (m_curSequence == nullptr)
		return nullptr;

	CBlock* block = m_curSequence->PopCommand(flag);

	if (block != nullptr)
		m_numCommands--;

	return block;
}

/*
-------------------------
RemoveSequence
-------------------------
*/

//NOTENOTE: This only removes references to the sequence, IT DOES NOT FREE THE ALLOCATED MEMORY!  You've be warned! =)

int CSequencer::RemoveSequence(CSequence* sequence) const
{
	const int numChildren = sequence->GetNumChildren();

	//Add all the children
	for (int i = 0; i < numChildren; i++)
	{
		CSequence* temp = sequence->GetChildByIndex(i);

		//TODO: Emit warning
		assert(temp);
		if (temp == nullptr)
		{
			m_ie->I_DPrintf(WL_WARNING, "Unable to find child sequence on RemoveSequence call!\n");
			continue;
		}

		//Remove the references to this sequence
		temp->SetParent(nullptr);
		temp->SetReturn(nullptr);
	}

	return SEQ_OK;
}

int CSequencer::DestroySequence(CSequence* sequence)
{
	if (!sequence)
		return SEQ_FAILED;

	//m_sequenceMap.erase( sequence->GetID() );
	m_sequences.remove(sequence);

	for (auto tsi = m_taskSequences.begin(); tsi != m_taskSequences.end();)
	{
		if ((*tsi).second == sequence)
		{
			m_taskSequences.erase(tsi++);
		}
		else
		{
			++tsi;
		}
	}

	CSequence* parent = sequence->GetParent();
	if (parent)
	{
		parent->RemoveChild(sequence);
		parent = nullptr;
	}

	int curChild = sequence->GetNumChildren();
	while (curChild)
	{
		// Stop if we're about to go negative (invalid index!).
		if (curChild > 0)
		{
			DestroySequence(sequence->GetChildByIndex(--curChild));
		}
		else
			break;
	}

	m_owner->DeleteSequence(sequence);

	return SEQ_OK;
}

/*
-------------------------
ReturnSequence
-------------------------
*/

inline CSequence* CSequencer::ReturnSequence(CSequence* sequence)
{
	while (sequence->GetReturn())
	{
		assert(sequence != sequence->GetReturn());
		if (sequence == sequence->GetReturn())
			return nullptr;

		sequence = sequence->GetReturn();

		if (sequence->GetNumCommands() > 0)
			return sequence;
	}
	return nullptr;
}

//Save / Load

/*
-------------------------
Save
-------------------------
*/

int CSequencer::Save(void)
{
#if 0 //asfsfasadf
	sequence_l::iterator		si;
	taskSequence_m::iterator	ti;
	int							numSequences = 0, id, numTasks;

	//Get the number of sequences to save out
	numSequences = m_sequences.size();

	//Save out the owner sequence
	m_ie->I_WriteSaveData('SQRE', &m_ownerID, sizeof(m_ownerID));

	//Write out the number of sequences we need to read
	m_ie->I_WriteSaveData('SQR#', &numSequences, sizeof(numSequences));

	//Second pass, save out all sequences, in order
	STL_ITERATE(si, m_sequences)
	{
		id = (*si)->GetID();
		m_ie->I_WriteSaveData('SQRI', &id, sizeof(id));
	}

	//Save out the task_manager
	m_taskManager->Save();

	//Save out the task sequences mapping the name to the GUIDs
	numTasks = m_taskSequences.size();
	m_ie->I_WriteSaveData('SQT#', &numTasks, sizeof(numTasks));

	STL_ITERATE(ti, m_taskSequences)
	{
		//Save the task group's ID
		id = ((*ti).first)->GetGUID();
		m_ie->I_WriteSaveData('STID', &id, sizeof(id));

		//Save the sequence's ID
		id = ((*ti).second)->GetID();
		m_ie->I_WriteSaveData('SSID', &id, sizeof(id));
	}

	int	curGroupID = (m_curGroup == NULL) ? -1 : m_curGroup->GetGUID();

	m_ie->I_WriteSaveData('SQCT', &curGroupID, sizeof(m_numCommands));

	//Output the number of commands
	m_ie->I_WriteSaveData('SQ#C', &m_numCommands, sizeof(m_numCommands));	//FIXME: This can be reconstructed

	//Output the ID of the current sequence
	id = (m_curSequence != NULL) ? m_curSequence->GetID() : -1;
	m_ie->I_WriteSaveData('SQCS', &id, sizeof(id));

	return true;
#endif
	return false;
}

/*
-------------------------
Load
-------------------------
*/

int CSequencer::Load(void)
{
#if 0 //asfsfasadf
	//Get the owner of this sequencer
	m_ie->I_ReadSaveData('SQRE', &m_ownerID, sizeof(m_ownerID));

	//Link the entity back to the sequencer
	m_ie->I_LinkEntity(m_ownerID, this, m_taskManager);

	CTaskGroup* taskGroup;
	CSequence* seq;
	int			numSequences, seqID, taskID, numTasks;

	//Get the number of sequences to read
	m_ie->I_ReadSaveData('SQR#', &numSequences, sizeof(numSequences));

	//Read in all the sequences
	for (int i = 0; i < numSequences; i++)
	{
		m_ie->I_ReadSaveData('SQRI', &seqID, sizeof(seqID));

		seq = m_owner->GetSequence(seqID);

		assert(seq);

		STL_INSERT(m_sequences, seq);
		m_sequenceMap[seqID] = seq;
	}

	//Setup the task manager
	m_taskManager->Init(this);

	//Load the task manager
	m_taskManager->Load();

	//Get the number of tasks in the map
	m_ie->I_ReadSaveData('SQT#', &numTasks, sizeof(numTasks));

	//Read in, and reassociate the tasks to the sequences
	for (i = 0; i < numTasks; i++)
	{
		//Read in the task's ID
		m_ie->I_ReadSaveData('STID', &taskID, sizeof(taskID));

		//Read in the sequence's ID
		m_ie->I_ReadSaveData('SSID', &seqID, sizeof(seqID));

		taskGroup = m_taskManager->GetTaskGroup(taskID);

		assert(taskGroup);

		seq = m_owner->GetSequence(seqID);

		assert(seq);

		//Associate the values
		m_taskSequences[taskGroup] = seq;
	}

	int	curGroupID;

	//Get the current task group
	m_ie->I_ReadSaveData('SQCT', &curGroupID, sizeof(curGroupID));

	m_curGroup = (curGroupID == -1) ? NULL : m_taskManager->GetTaskGroup(curGroupID);

	//Get the number of commands
	m_ie->I_ReadSaveData('SQ#C', &m_numCommands, sizeof(m_numCommands));

	//Get the current sequence
	m_ie->I_ReadSaveData('SQCS', &seqID, sizeof(seqID));

	m_curSequence = (seqID != -1) ? m_owner->GetSequence(seqID) : NULL;

	return true;
#endif
	return false;
}