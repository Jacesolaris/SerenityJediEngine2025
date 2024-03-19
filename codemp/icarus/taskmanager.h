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

#pragma once

// Task Manager header file

#include <map>
#include <string>

#include "sequencer.h"
class CSequencer;

#define MAX_TASK_NAME	64

#define TASKFLAG_NORMAL	0x00000000

constexpr int RUNAWAY_LIMIT = 256;

enum
{
	TASK_RETURN_COMPLETE,
	TASK_RETURN_FAILED,
};

enum
{
	TASK_OK,
	TASK_FAILED,
	TASK_START,
	TASK_END,
};

// CTask

class CTask
{
public:
	CTask();
	~CTask();

	static CTask* Create(int guid, CBlock* block);

	void Free(void) const;

	unsigned int GetTimeStamp(void) const { return m_timeStamp; }
	CBlock* GetBlock(void) const { return m_block; }
	int GetGUID(void) const { return m_id; }
	int GetID(void) const { return m_block->GetBlockID(); }

	void SetTimeStamp(const unsigned int timeStamp) { m_timeStamp = timeStamp; }
	void SetBlock(CBlock* block) { m_block = block; }
	void SetGUID(const int id) { m_id = id; }

protected:
	int m_id;
	unsigned int m_timeStamp;
	CBlock* m_block;
};

// CTaskGroup

class CTaskGroup
{
public:
	using taskCallback_m = std::map<int, bool>;

	CTaskGroup(void);
	~CTaskGroup(void);

	void Init(void);

	int Add(const CTask* task);

	void SetGUID(int guid);
	void SetParent(CTaskGroup* group) { m_parent = group; }

	bool Complete(void) const { return m_numCompleted == static_cast<int>(m_completedTasks.size()); }

	bool MarkTaskComplete(int id);

	CTaskGroup* GetParent(void) const { return m_parent; }
	int GetGUID(void) const { return m_GUID; }

	//protected:

	taskCallback_m m_completedTasks;

	CTaskGroup* m_parent;

	int m_numCompleted;
	int m_GUID;
};

// CTaskManager

class CTaskManager
{
	using taskID_m = std::map<int, CTask*>;
	using taskGroupName_m = std::map<std::string, CTaskGroup*>;
	using taskGroupID_m = std::map<int, CTaskGroup*>;
	using taskGroup_v = std::vector<CTaskGroup*>;
	using tasks_l = std::list<CTask*>;

public:
	CTaskManager();
	~CTaskManager();

	static CTaskManager* Create(void);

	CBlock* GetCurrentTask(void);

	int Init(CSequencer* owner);
	int Free(void);

	static int Flush(void);

	int SetCommand(CBlock* command, int type);
	int Completed(int id);

	int Update(void);
	qboolean IsRunning(void) const;

	CTaskGroup* AddTaskGroup(const char* name);
	CTaskGroup* GetTaskGroup(const char* name);
	CTaskGroup* GetTaskGroup(int id);

	int MarkTask(int id, int operation);
	CBlock* RecallTask(void);

	static void Save(void);
	static void Load(void);

protected:
	int Go(void); //Heartbeat function called once per game frame
	int CallbackCommand(CTask* task, int returnCode);

	static inline bool Check(int target_id, const CBlock* block, int member_num);

	int GetVector(int entID, CBlock* block, int& memberNum, vector_t& value);
	int GetFloat(int entID, const CBlock* block, int& memberNum, float& value) const;
	int Get(int entID, CBlock* block, int& memberNum, char** value);

	int PushTask(CTask* task, int flag);
	CTask* PopTask(int flag);

	// Task functions
	int Rotate(CTask* task);
	int Remove(CTask* task);
	int Camera(CTask* task);
	int Print(CTask* task);
	int Sound(CTask* task);
	int Move(CTask* task);
	int Kill(CTask* task);
	int Set(CTask* task);
	int Use(CTask* task);
	int DeclareVariable(CTask* task);
	int FreeVariable(CTask* task);
	int Signal(CTask* task);
	int Play(CTask* task);

	int Wait(CTask* task, bool& completed);
	int WaitSignal(CTask* task, bool& completed);

	int SaveCommand(CBlock* block) const;

	// Variables

	CSequencer* m_owner;
	int m_ownerID;

	CTaskGroup* m_curGroup;

	taskGroup_v m_taskGroups;
	tasks_l m_tasks;

	int m_GUID;
	int m_count;

	taskGroupName_m m_taskGroupNameMap;
	taskGroupID_m m_taskGroupIDMap;

	bool m_resident;

	//CTask	*m_waitTask;		//Global pointer to the current task that is waiting for callback completion
};
