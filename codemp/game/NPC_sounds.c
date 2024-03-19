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

//NPC_sounds.cpp
#include "b_local.h"
#include "icarus/Q3_Interface.h"

extern void G_SpeechEvent(const gentity_t* self, int event);

void G_AddVoiceEvent(const gentity_t* self, const int event, const int speak_debounce_time)
{
	if (!self->NPC)
	{
		return;
	}

	if (!self->client || self->client->ps.pm_type >= PM_DEAD)
	{
		return;
	}

	if (self->NPC->blockedSpeechDebounceTime > level.time)
	{
		return;
	}

	if (trap->ICARUS_TaskIDPending((sharedEntity_t*)self, TID_CHAN_VOICE))
	{
		return;
	}

	if (self->NPC->scriptFlags & SCF_NO_COMBAT_TALK && (event >= EV_ANGER1 && event <= EV_VICTORY3 || event >= EV_CHASE1
		&& event <= EV_SUSPICIOUS5))
		//(event < EV_FF_1A || event > EV_FF_3C) && (event < EV_RESPOND1 || event > EV_MISSION3) )
	{
		return;
	}

	if (self->NPC->scriptFlags & SCF_NO_ALERT_TALK && (event >= EV_GIVEUP1 && event <= EV_SUSPICIOUS5))
	{
		return;
	}
	//FIXME: Also needs to check for teammates. Don't want
	//		everyone babbling at once

	//NOTE: was losing too many speech events, so we do it directly now, screw networking!
	G_SpeechEvent(self, event);

	//won't speak again for 5 seconds (unless otherwise specified)
	self->NPC->blockedSpeechDebounceTime = level.time + (speak_debounce_time == 0 ? 5000 : speak_debounce_time);
}

void NPC_PlayConfusionSound(gentity_t* self)
{
	if (self->health > 0)
	{
		if (self->enemy || //was mad
			!TIMER_Done(self, "enemyLastVisible") || //saw something suspicious
			self->client->renderInfo.lookTarget == 0 //was looking at player
			)
		{
			self->NPC->blockedSpeechDebounceTime = 0; //make sure we say this
			G_AddVoiceEvent(self, Q_irand(EV_CONFUSE2, EV_CONFUSE3), 2000);
		}
		else if (self->NPC && self->NPC->investigateDebounceTime + self->NPC->pauseTime > level.time)
			//was checking something out
		{
			self->NPC->blockedSpeechDebounceTime = 0; //make sure we say this
			G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		}
	}
	//reset him to be totally unaware again
	TIMER_Set(self, "enemyLastVisible", 0);
	self->NPC->tempBehavior = BS_DEFAULT;

	G_ClearEnemy(self); //FIXME: or just self->enemy = NULL;?

	self->NPC->investigateCount = 0;
}

void NPC_AngerSound(void)
{
	if (NPCS.NPCInfo->investigateSoundDebounceTime)
		return;

	NPCS.NPCInfo->investigateSoundDebounceTime = 1;

	G_AddVoiceEvent(NPCS.NPC, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
}