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

#ifndef __OBJECTIVES_H__
#define __OBJECTIVES_H__

// mission Objectives

// DO NOT CHANGE MAX_MISSION_OBJ. IT AFFECTS THE SAVEGAME STRUCTURE

using objectiveNumber_t = enum //# Objective_e
{
	//=================================================
	//
	//=================================================

	LIGHTSIDE_OBJ = 0,
	HOTH2_OBJ1,
	HOTH2_OBJ2,
	HOTH2_OBJ3,
	HOTH3_OBJ1,
	HOTH3_OBJ2,
	HOTH3_OBJ3,
	T2_DPREDICAMENT_OBJ1,
	T2_DPREDICAMENT_OBJ2,
	T2_DPREDICAMENT_OBJ3,
	T2_DPREDICAMENT_OBJ4,
	T2_RANCOR_OBJ1,
	T2_RANCOR_OBJ2,
	T2_RANCOR_OBJ3,
	T2_RANCOR_OBJ4,
	T2_RANCOR_OBJ5,
	T2_RANCOR_OBJ5_2,
	T2_RANCOR_OBJ6,
	T2_WEDGE_OBJ1,
	T2_WEDGE_OBJ2,
	T2_WEDGE_OBJ3,
	T2_WEDGE_OBJ4,
	T2_WEDGE_OBJ5,
	T2_WEDGE_OBJ6,
	T2_WEDGE_OBJ7,
	T2_WEDGE_OBJ8,
	T2_WEDGE_OBJ9,
	T2_WEDGE_OBJ10,
	T2_WEDGE_OBJ11,
	T2_WEDGE_OBJ12,
	T3_RIFT_OBJ1,
	T3_RIFT_OBJ2,
	T3_RIFT_OBJ3,
	T1_DANGER_OBJ1,
	T1_DANGER_OBJ2,
	T1_DANGER_OBJ3,
	T1_DANGER_OBJ4,
	T1_DANGER_OBJ5,
	T3_BOUNTY_OBJ1,
	T3_BOUNTY_OBJ2,
	T3_BOUNTY_OBJ3,
	T3_BOUNTY_OBJ4,
	T3_BOUNTY_OBJ5,
	T3_BOUNTY_OBJ6,
	T3_BOUNTY_OBJ7,
	T3_BOUNTY_OBJ8,
	T3_BOUNTY_OBJ9,
	T2_ROGUE_OBJ1,
	T2_ROGUE_OBJ2,
	T2_TRIP_OBJ1,
	T2_TRIP_OBJ2,
	T3_BYSS_OBJ1,
	T3_BYSS_OBJ2,
	T3_BYSS_OBJ3,
	T3_HEVIL_OBJ1,
	T3_HEVIL_OBJ2,
	T3_HEVIL_OBJ3,
	T3_STAMP_OBJ1,
	T3_STAMP_OBJ2,
	T3_STAMP_OBJ3,
	T3_STAMP_OBJ4,
	TASPIR1_OBJ1,
	TASPIR1_OBJ2,
	TASPIR1_OBJ3,
	TASPIR1_OBJ4,
	TASPIR2_OBJ1,
	TASPIR2_OBJ2,
	VJUN1_OBJ1,
	VJUN1_OBJ2,
	VJUN2_OBJ1,
	VJUN3_OBJ1,
	YAVIN1_OBJ1,
	YAVIN1_OBJ2,
	YAVIN2_OBJ1,
	T1_FATAL_OBJ1,
	T1_FATAL_OBJ2,
	T1_FATAL_OBJ3,
	T1_FATAL_OBJ4,
	T1_FATAL_OBJ5,
	T1_FATAL_OBJ6,
	KOR1_OBJ1,
	KOR1_OBJ2,
	KOR2_OBJ1,
	KOR2_OBJ2,
	KOR2_OBJ3,
	KOR2_OBJ4,
	T1_RAIL_OBJ1,
	T1_RAIL_OBJ2,
	T1_RAIL_OBJ3,
	T1_SOUR_OBJ1,
	T1_SOUR_OBJ2,
	T1_SOUR_OBJ3,
	T1_SOUR_OBJ4,
	T1_SURPRISE_OBJ1,
	T1_SURPRISE_OBJ2,
	T1_SURPRISE_OBJ3,
	T1_SURPRISE_OBJ4,
	KEJIM_POST_OBJ1,
	//# KEJIM POST
	KEJIM_POST_OBJ2,
	//# KEJIM POST
	KEJIM_BASE_OBJ1,
	//# KEJIM BASE
	KEJIM_BASE_OBJ2,
	//# KEJIM BASE
	KEJIM_BASE_OBJ3,
	//# KEJIM BASE
	ARTUS_MINE_OBJ1,
	//# ARTUS MINE
	ARTUS_MINE_OBJ2,
	//# ARTUS MINE
	ARTUS_MINE_OBJ3,
	//# ARTUS MINE
	ARTUS_DETENTION_OBJ1,
	//# ARTUS DETENTION
	ARTUS_DETENTION_OBJ2,
	//# ARTUS DETENTION
	ARTUS_TOPSIDE_OBJ1,
	//# ARTUS TOPSIDE
	ARTUS_TOPSIDE_OBJ2,
	//# ARTUS TOPSIDE
	YAVIN_TEMPLE_OBJ1,
	//# YAVIN TEMPLE
	YAVIN_TRIAL_OBJ1,
	//# YAVIN TRIAL
	YAVIN_TRIAL_OBJ2,
	//# YAVIN TRIAL
	NS_STREETS_OBJ1,
	//# NS STREETS
	NS_STREETS_OBJ2,
	//# NS STREETS
	NS_STREETS_OBJ3,
	//# NS STREETS
	NS_HIDEOUT_OBJ1,
	//# NS HIDEOUT
	NS_HIDEOUT_OBJ2,
	//# NS HIDEOUT
	NS_STARPAD_OBJ1,
	//# NS STARPAD
	NS_STARPAD_OBJ2,
	//# NS STARPAD
	NS_STARPAD_OBJ3,
	//# NS STARPAD
	NS_STARPAD_OBJ4,
	//# NS STARPAD
	NS_STARPAD_OBJ5,
	//# NS STARPAD
	BESPIN_UNDERCITY_OBJ1,
	//# BESPIN UNDERCITY
	BESPIN_UNDERCITY_OBJ2,
	//# BESPIN UNDERCITY
	BESPIN_STREETS_OBJ1,
	//# BESPIN STREETS
	BESPIN_STREETS_OBJ2,
	//# BESPIN STREETS
	BESPIN_PLATFORM_OBJ1,
	//# BESPIN PLATFORM
	BESPIN_PLATFORM_OBJ2,
	//# BESPIN PLATFORM
	CAIRN_BAY_OBJ1,
	//# CAIRN BAY
	CAIRN_BAY_OBJ2,
	//# CAIRN BAY
	CAIRN_ASSEMBLY_OBJ1,
	//# CAIRN ASSEMBLY
	CAIRN_REACTOR_OBJ1,
	//# CAIRN REACTOR
	CAIRN_REACTOR_OBJ2,
	//# CAIRN REACTOR
	CAIRN_DOCK1_OBJ1,
	//# CAIRN DOCK1
	CAIRN_DOCK1_OBJ2,
	//# CAIRN DOCK1
	DOOM_COMM_OBJ1,
	//# DOOM COMM
	DOOM_COMM_OBJ2,
	//# DOOM COMM
	DOOM_COMM_OBJ3,
	//# DOOM COMM
	DOOM_DETENTION_OBJ1,
	//# DOOM DETENTION
	DOOM_DETENTION_OBJ2,
	//# DOOM DETENTION
	DOOM_SHIELDS_OBJ1,
	//# DOOM SHIELDS
	DOOM_SHIELDS_OBJ2,
	//# DOOM SHIELDS
	YAVIN_SWAMP_OBJ1,
	//# YAVIN SWAMP
	YAVIN_SWAMP_OBJ2,
	//# YAVIN SWAMP
	YAVIN_CANYON_OBJ1,
	//# YAVIN CANYON
	YAVIN_CANYON_OBJ2,
	//# YAVIN CANYON
	YAVIN_COURTYARD_OBJ1,
	//# YAVIN COURTYARD
	YAVIN_COURTYARD_OBJ2,
	//# YAVIN COURTYARD
	YAVIN_FINAL_OBJ1,
	//# YAVIN FINAL
	KEJIM_POST_OBJ3,
	//# KEJIM POST - GRAPHICS IN IT.
	KEJIM_POST_OBJ4,
	//# KEJIM POST - GRAPHICS IN IT.
	KEJIM_POST_OBJ5,
	//# KEJIM POST - GRAPHICS IN IT.
	ARTUS_DETENTION_OBJ3,
	//# ARTUS DETENTION
	DOOM_COMM_OBJ4,
	//# DOOM COMM - GRAPHICS IN IT.
	DOOM_SHIELDS_OBJ3,
	//# DOOM SHIELDS
	DEMO_OBJ1,
	//# DEMO
	DEMO_OBJ2,
	//# DEMO
	DEMO_OBJ3,
	//# DEMO
	DEMO_OBJ4,
	//# DEMO
	SECBASE_OBJ1,
	//# Darkforces Mod
	SECBASE_OBJ2,
	//# Darkforces Mod
	TALAY_OBJ1,
	//# Darkforces Mod
	TALAY_OBJ2,
	//# Darkforces Mod
	TALAY_OBJ3,
	//# Darkforces Mod
	SEWERS_OBJ1,
	//# Darkforces Mod
	TESTBASE_OBJ1,
	//# Darkforces Mod
	TESTBASE_OBJ2,
	//# Darkforces Mod
	GROMAS_OBJ1,
	//# Darkforces Mod
	GROMAS_OBJ2,
	//# Darkforces Mod
	DTENTION_OBJ1,
	//# Darkforces Mod
	TOURNAMENT_OBJ1,
	//# TOURNAMENT Mod
	TOURNAMENT_OBJ2,
	//# TOURNAMENT Mod
	TOURNAMENT_OBJ3,
	//# TOURNAMENT Mod
	PRIVATEERHOTH2_OBJ1,
	//# PRIVATEER
	PRIVATEERHOTH2_OBJ2,
	//# PRIVATEER
	PRIVATEERHOTH2_OBJ3,
	//# PRIVATEER Mod
	PRIVATEERHOTH3_OBJ1,
	//# PRIVATEER Mod
	PRIVATEERHOTH3_OBJ2,
	//# PRIVATEER Mod
	PRIVATEERHOTH3_OBJ3,
	//# PRIVATEER Mod
	PRIVATEERT2_DPREDICAMENT_OBJ1,
	//# PRIVATEER Mod
	PRIVATEERT2_DPREDICAMENT_OBJ2,
	//# PRIVATEER Mod
	PRIVATEERT2_DPREDICAMENT_OBJ3,
	//# PRIVATEER Mod
	PRIVATEERT2_DPREDICAMENT_OBJ4,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ1,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ2,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ3,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ4,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ5,
	//# PRIVATEER Mod
	PRIVATEERT2_RANCOR_OBJ6,
	//# PRIVATEER Mod
	HOTH5_OBJ1,
	//# redemtion Mod
	HOTH5_OBJ2,
	//# redemtion Mod
	HOTH5_OBJ3,
	//# redemtion Mod
	MOTH2_OBJ1,
	//# DASH Mod
	MOTH2_OBJ2,
	//# DASH Mod
	T5_WEDGE_OBJ1,
	//# DASH Mod
	T5_WEDGE_OBJ2,
	//# DASH Mod
	T5_WEDGE_OBJ3,
	//# DASH Mod
	T5_WEDGE_OBJ4,
	//# DASH Mod
	T5_WEDGE_OBJ5,
	//# DASH Mod
	T0_ROGUE_OBJ1,
	//# tatooinecrash Mod
	T0_ROGUE_OBJ2,
	//# tatooinecrash Mod
	VOTS_OBJ1,
	//# vengance Mod
	VOTS_OBJ2,
	//# vengance Mod
	VOTS_OBJ3,
	//# vengance Mod
	VOTS_OBJ4,
	//# vengance Mod
	VOTS_OBJ5,
	//# vengance Mod
	VOTS_OBJ6,
	//# vengance Mod
	VOTS_OBJ7,
	//# vengance Mod
	VOTS_OBJ8,
	//# vengance Mod
	VOTS_OBJ9,
	//# vengance Mod
	VOTS_OBJ10,
	//# vengance Mod
	VOTS_OBJ11,
	//# vengance Mod
	VOTS_OBJ12,
	//# vengance Mod
	VOTS_OBJ13,
	//# vengance Mod
	VOTS_OBJ14,
	//# vengance Mod
	VOTS_OBJ15,
	//# vengance Mod
	VOTS_OBJ16,
	//# vengance Mod
	VOTS_OBJ17,
	//# vengance Mod
	VOTS_OBJ18,
	//# vengance Mod
	VOTS_OBJ19,
	//# vengance Mod
	VOTS_OBJ20,
	//# vengance Mod
	VOTS_OBJ21,
	//# vengance Mod
	DF2_01NAR_OBJ,
	//# Darkforces2 Mod
	DF2_02NAR_OBJ1,
	//# Darkforces2 Mod
	DF2_02NAR_OBJ2,
	//# Darkforces2 Mod
	DF2_02NAR_OBJ3,
	//# Darkforces2 Mod
	DF2_03KATARN_OBJ,
	//# Darkforces2 Mod
	DF2_07YUN_OBJ,
	DF2_11GORC_OBJ1,
	DF2_11GORC_OBJ2,
	P2_WEDGE_OBJ1,
	P2_WEDGE_OBJ2,
	P2_WEDGE_OBJ3,

	L1_OBJ1,
	//# escapeyavinIV Mod
	L1_OBJ2,
	L1_OBJ3,
	L1_OBJ4,
	L1_OBJ5,
	L1_OBJ6,
	L1_OBJ7,

	L1_LAVA,
	//# escapeyavinIV Mod
	L1_DESERT_OUTPOST,
	//# escapeyavinIV Mod

	L2_OBJ1,
	//# escapeyavinIV Mod
	L2_OBJ2,
	L2_OBJ3,
	L2_OBJ4,
	L2_OBJ5,
	L2_OBJ6,
	L2_OBJ7,
	L2_OBJ8,
	L2_OBJ9,

	L2_SABER_SHIPMENT,
	//# escapeyavinIV Mod
	L2_TRANSPORT_ENTRY,
	//# escapeyavinIV Mod

	L3_OBJ1,
	//# escapeyavinIV Mod
	L3_OBJ2,
	L3_OBJ3,
	L3_OBJ4,
	L3_OBJ5,
	L3_OBJ6,
	L3_OBJ7,
	L3_OBJ8,

	L3_OLD_REACTOR,
	//# escapeyavinIV Mod

	L4_OBJ1,
	//# escapeyavinIV Mod
	L4_OBJ2,
	L4_OBJ3,
	L4_OBJ4,
	L4_OBJ5,

	L4_WEAPONS,
	//# escapeyavinIV Mod
	L4_CRYSTAL,
	//# escapeyavinIV Mod

	L5_OBJ1,
	//# escapeyavinIV Mod
	L5_OBJ2,
	L5_OBJ3,
	L5_OBJ4,
	L5_OBJ5,
	L5_OBJ6,

	L5_FUELING,
	//# escapeyavinIV Mod
	L5_SITH,
	//# escapeyavinIV Mod

	L6_HOME,
	//# escapeyavinIV Mod
	L6_OUTSKIRTS,
	//# escapeyavinIV Mod
	L6_RESCUE_GOOD,
	//# escapeyavinIV Mod
	L6_OUTSKIRTS_SITH,
	//# escapeyavinIV Mod
	L6_SITH,
	//# escapeyavinIV Mod
	L6_QUEEN_SITH,
	//# escapeyavinIV Mod
	L6_OUTSKIRTS_DARK,
	//# escapeyavinIV Mod
	L6_DARK,
	//# escapeyavinIV Mod
	L6_QUEEN_DARK,
	//# escapeyavinIV Mod

	CLOUDY_OBJ1,
	//#
	CLOUDY_OBJ2,
	//#
	CLOUDY_OBJ3,
	//#
	CLOUDY_OBJ4,
	//#
	CLOUDY_OBJ5,
	//#

	//MOVIEDUELS
	MDT1_PO_OBJ1,
	MD2_ROC_JEDI_OBJ1,

	MD_TOT_OBJ1,
	MD_TOT_OBJ2,
	MD_TOT_OBJ3,
	MD_TOT_OBJ4,
	MD_TOT_OBJ5,

	MD_SN_OBJ1,
	MD_SN_OBJ2,
	MD_SN_OBJ3,

	MD_OOT_OBJ1,
	MD_OOT_OBJ2,
	MD_OOT_OBJ3,

	MD_DOTF_OBJ1,

	MD_GD_OBJ1,
	MD_GD_OBJ2,

	MD_EGG_OBJ1,
	MD_EGG_OBJ2,
	MD_EGG_OBJ3,

	MD_ROC_OBJ1,
	MD_ROC_OBJ2,
	MD_ROC_OBJ3,
	MD_ROC_OBJ4,
	MD_ROC_OBJ5,
	MD_ROC_OBJ6,

	MD_LUKE_OBJ1,
	MD_LUKE_OBJ2,

	MD_RT_OBJ1,
	MD_RT_OBJ2,
	MD_RT_OBJ3,

	MD_JT_STAR_OBJ1,
	MD_JT_STAR_OBJ2,
	MD_JT_STAR_OBJ3,
	MD_JT_STAR_OBJ4,

	MD_MUSJUMP_OBJ1,

	MD_MOF_OBJ1,
	MD_MOF_OBJ2,
	MD_MOF_OBJ3,
	MD_MOF_OBJ4,

	MD_MUS_OBJ1,
	MD_MUS_OBJ2,

	MD_JT_OBJ1,
	MD_JT_OBJ2,
	MD_JT_OBJ3,

	MD_JT2_OBJ1,
	MD_JT2_OBJ2,
	MD_JT2_OBJ3,
	MD_JT2_OBJ4,

	MD_GB_OBJ1,
	MD_GB_OBJ2,
	MD_GB_OBJ3,
	MD_GB_OBJ4,
	MD_GB_OBJ5,
	MD_GB_OBJ6,
	MD_GB_OBJ7,

	MD_GA_OBJ1,
	MD_GA_OBJ2,
	MD_GA_OBJ3,
	MD_GA_OBJ4,
	MD_GA_OBJ5,
	MD_GA_OBJ6,
	MD_GA_OBJ7,
	MD_GA_OBJ8,
	MD_GA_OBJ9,
	MD_GA_OBJ10,
	MD_GA_OBJ11,
	MD_GA_OBJ12,
	MD_GA_OBJ13,

	MD_TC_OBJ1,
	MD_TC_OBJ2,

	MD_AM_OBJ1,
	MD_AM_OBJ2,

	MD_KA_OBJ1,
	MD_KA_OBJ2,

	FINAL_SCRIPT,

	//# #eol
	MAX_OBJECTIVES,
};

using missionFailed_t = enum //# MissionFailed_e
{
	MISSIONFAILED_JAN = 0,
	//#
	MISSIONFAILED_LUKE,
	//#
	MISSIONFAILED_LANDO,
	//#
	MISSIONFAILED_R5D2,
	//#
	MISSIONFAILED_WARDEN,
	//#
	MISSIONFAILED_PRISONERS,
	//#
	MISSIONFAILED_EMPLACEDGUNS,
	//#
	MISSIONFAILED_LADYLUCK,
	//#
	MISSIONFAILED_KYLECAPTURE,
	//#
	MISSIONFAILED_TOOMANYALLIESDIED,
	//#
	MISSIONFAILED_CHEWIE,
	//#
	MISSIONFAILED_KYLE,
	//#
	MISSIONFAILED_ROSH,
	//#
	MISSIONFAILED_WEDGE,
	//#
	MISSIONFAILED_TURNED,
	//# Turned on your friends.

	//# #eol
	MAX_MISSIONFAILED,
};

using statusText_t = enum //# StatusText_e
{
	//=================================================
	//
	//=================================================
	STAT_INSUBORDINATION = 0,
	//# Starfleet will not tolerate such insubordination
	STAT_YOUCAUSEDDEATHOFTEAMMATE,
	//# You caused the death of a teammate.
	STAT_DIDNTPROTECTTECH,
	//# You failed to protect Chell, your technician.
	STAT_DIDNTPROTECT7OF9,
	//# You failed to protect 7 of 9
	STAT_NOTSTEALTHYENOUGH,
	//# You weren't quite stealthy enough
	STAT_STEALTHTACTICSNECESSARY,
	//# Starfleet will not tolerate such insubordination
	STAT_WATCHYOURSTEP,
	//# Watch your step
	STAT_JUDGEMENTMUCHDESIRED,
	//# Your judgement leaves much to be desired

	//# #eol
	MAX_STATUSTEXT,
};

extern qboolean missionInfo_Updated;

constexpr auto SET_TACTICAL_OFF = 0;
constexpr auto SET_TACTICAL_ON = 1;

constexpr auto SET_OBJ_HIDE = 0;
constexpr auto SET_OBJ_SHOW = 1;
constexpr auto SET_OBJ_PENDING = 2;
constexpr auto SET_OBJ_SUCCEEDED = 3;
constexpr auto SET_OBJ_FAILED = 4;

constexpr auto OBJECTIVE_HIDE = 0;
constexpr auto OBJECTIVE_SHOW = 1;

constexpr auto OBJECTIVE_STAT_PENDING = 0;
constexpr auto OBJECTIVE_STAT_SUCCEEDED = 1;
constexpr auto OBJECTIVE_STAT_FAILED = 2;

extern int statusTextIndex;

void OBJ_SaveObjectiveData();
void OBJ_LoadObjectiveData();
extern void OBJ_SetPendingObjectives(const gentity_t* ent);

#ifndef G_OBJECTIVES_CPP

extern stringID_table_t objectiveTable[];
extern stringID_table_t statusTextTable[];
extern stringID_table_t missionFailedTable[];

#else

stringID_table_t objectiveTable[] =
{
	//=================================================
	//
	//=================================================
	ENUM2STRING(LIGHTSIDE_OBJ),
	ENUM2STRING(HOTH2_OBJ1),
	ENUM2STRING(HOTH2_OBJ2),
	ENUM2STRING(HOTH2_OBJ3),
	ENUM2STRING(HOTH3_OBJ1),
	ENUM2STRING(HOTH3_OBJ2),
	ENUM2STRING(HOTH3_OBJ3),
	ENUM2STRING(T2_DPREDICAMENT_OBJ1),
	ENUM2STRING(T2_DPREDICAMENT_OBJ2),
	ENUM2STRING(T2_DPREDICAMENT_OBJ3),
	ENUM2STRING(T2_DPREDICAMENT_OBJ4),
	ENUM2STRING(T2_RANCOR_OBJ1),
	ENUM2STRING(T2_RANCOR_OBJ2),
	ENUM2STRING(T2_RANCOR_OBJ3),
	ENUM2STRING(T2_RANCOR_OBJ4),
	ENUM2STRING(T2_RANCOR_OBJ5),
	ENUM2STRING(T2_RANCOR_OBJ5_2),
	ENUM2STRING(T2_RANCOR_OBJ6),
	ENUM2STRING(T2_WEDGE_OBJ1),
	ENUM2STRING(T2_WEDGE_OBJ2),
	ENUM2STRING(T2_WEDGE_OBJ3),
	ENUM2STRING(T2_WEDGE_OBJ4),
	ENUM2STRING(T2_WEDGE_OBJ5),
	ENUM2STRING(T2_WEDGE_OBJ6),
	ENUM2STRING(T2_WEDGE_OBJ7),
	ENUM2STRING(T2_WEDGE_OBJ8),
	ENUM2STRING(T2_WEDGE_OBJ9),
	ENUM2STRING(T2_WEDGE_OBJ10),
	ENUM2STRING(T2_WEDGE_OBJ11),
	ENUM2STRING(T2_WEDGE_OBJ12),
	ENUM2STRING(T3_RIFT_OBJ1),
	ENUM2STRING(T3_RIFT_OBJ2),
	ENUM2STRING(T3_RIFT_OBJ3),
	ENUM2STRING(T1_DANGER_OBJ1),
	ENUM2STRING(T1_DANGER_OBJ2),
	ENUM2STRING(T1_DANGER_OBJ3),
	ENUM2STRING(T1_DANGER_OBJ4),
	ENUM2STRING(T1_DANGER_OBJ5),
	ENUM2STRING(T3_BOUNTY_OBJ1),
	ENUM2STRING(T3_BOUNTY_OBJ2),
	ENUM2STRING(T3_BOUNTY_OBJ3),
	ENUM2STRING(T3_BOUNTY_OBJ4),
	ENUM2STRING(T3_BOUNTY_OBJ5),
	ENUM2STRING(T3_BOUNTY_OBJ6),
	ENUM2STRING(T3_BOUNTY_OBJ7),
	ENUM2STRING(T3_BOUNTY_OBJ8),
	ENUM2STRING(T3_BOUNTY_OBJ9),
	ENUM2STRING(T2_ROGUE_OBJ1),
	ENUM2STRING(T2_ROGUE_OBJ2),
	ENUM2STRING(T2_TRIP_OBJ1),
	ENUM2STRING(T2_TRIP_OBJ2),
	ENUM2STRING(T3_BYSS_OBJ1),
	ENUM2STRING(T3_BYSS_OBJ2),
	ENUM2STRING(T3_BYSS_OBJ3),
	ENUM2STRING(T3_HEVIL_OBJ1),
	ENUM2STRING(T3_HEVIL_OBJ2),
	ENUM2STRING(T3_HEVIL_OBJ3),
	ENUM2STRING(T3_STAMP_OBJ1),
	ENUM2STRING(T3_STAMP_OBJ2),
	ENUM2STRING(T3_STAMP_OBJ3),
	ENUM2STRING(T3_STAMP_OBJ4),
	ENUM2STRING(TASPIR1_OBJ1),
	ENUM2STRING(TASPIR1_OBJ2),
	ENUM2STRING(TASPIR1_OBJ3),
	ENUM2STRING(TASPIR1_OBJ4),
	ENUM2STRING(TASPIR2_OBJ1),
	ENUM2STRING(TASPIR2_OBJ2),
	ENUM2STRING(VJUN1_OBJ1),
	ENUM2STRING(VJUN1_OBJ2),
	ENUM2STRING(VJUN2_OBJ1),
	ENUM2STRING(VJUN3_OBJ1),
	ENUM2STRING(YAVIN1_OBJ1),
	ENUM2STRING(YAVIN1_OBJ2),
	ENUM2STRING(YAVIN2_OBJ1),
	ENUM2STRING(T1_FATAL_OBJ1),
	ENUM2STRING(T1_FATAL_OBJ2),
	ENUM2STRING(T1_FATAL_OBJ3),
	ENUM2STRING(T1_FATAL_OBJ4),
	ENUM2STRING(T1_FATAL_OBJ5),
	ENUM2STRING(T1_FATAL_OBJ6),
	ENUM2STRING(KOR1_OBJ1),
	ENUM2STRING(KOR1_OBJ2),
	ENUM2STRING(KOR2_OBJ1),
	ENUM2STRING(KOR2_OBJ2),
	ENUM2STRING(KOR2_OBJ3),
	ENUM2STRING(KOR2_OBJ4),
	ENUM2STRING(T1_RAIL_OBJ1),
	ENUM2STRING(T1_RAIL_OBJ2),
	ENUM2STRING(T1_RAIL_OBJ3),
	ENUM2STRING(T1_SOUR_OBJ1),
	ENUM2STRING(T1_SOUR_OBJ2),
	ENUM2STRING(T1_SOUR_OBJ3),
	ENUM2STRING(T1_SOUR_OBJ4),
	ENUM2STRING(T1_SURPRISE_OBJ1),
	ENUM2STRING(T1_SURPRISE_OBJ2),
	ENUM2STRING(T1_SURPRISE_OBJ3),
	ENUM2STRING(T1_SURPRISE_OBJ4),
	ENUM2STRING(KEJIM_POST_OBJ1),		//# KEJIM POST
	ENUM2STRING(KEJIM_POST_OBJ2),			//# KEJIM POST
	ENUM2STRING(KEJIM_BASE_OBJ1),			//# KEJIM BASE
	ENUM2STRING(KEJIM_BASE_OBJ2),			//# KEJIM BASE
	ENUM2STRING(KEJIM_BASE_OBJ3),			//# KEJIM BASE
	ENUM2STRING(ARTUS_MINE_OBJ1),			//# ARTUS MINE
	ENUM2STRING(ARTUS_MINE_OBJ2),			//# ARTUS MINE
	ENUM2STRING(ARTUS_MINE_OBJ3),			//# ARTUS MINE
	ENUM2STRING(ARTUS_DETENTION_OBJ1),		//# ARTUS DETENTION
	ENUM2STRING(ARTUS_DETENTION_OBJ2),		//# ARTUS DETENTION
	ENUM2STRING(ARTUS_TOPSIDE_OBJ1),			//# ARTUS TOPSIDE
	ENUM2STRING(ARTUS_TOPSIDE_OBJ2),			//# ARTUS TOPSIDE
	ENUM2STRING(YAVIN_TEMPLE_OBJ1),			//# YAVIN TEMPLE
	ENUM2STRING(YAVIN_TRIAL_OBJ1),			//# YAVIN TRIAL
	ENUM2STRING(YAVIN_TRIAL_OBJ2),			//# YAVIN TRIAL
	ENUM2STRING(NS_STREETS_OBJ1),			//# NS STREETS
	ENUM2STRING(NS_STREETS_OBJ2),			//# NS STREETS
	ENUM2STRING(NS_STREETS_OBJ3),			//# NS STREETS
	ENUM2STRING(NS_HIDEOUT_OBJ1),			//# NS HIDEOUT
	ENUM2STRING(NS_HIDEOUT_OBJ2),			//# NS HIDEOUT
	ENUM2STRING(NS_STARPAD_OBJ1),			//# NS STARPAD
	ENUM2STRING(NS_STARPAD_OBJ2),			//# NS STARPAD
	ENUM2STRING(NS_STARPAD_OBJ3),			//# NS STARPAD
	ENUM2STRING(NS_STARPAD_OBJ4),			//# NS STARPAD
	ENUM2STRING(NS_STARPAD_OBJ5),			//# NS STARPAD
	ENUM2STRING(BESPIN_UNDERCITY_OBJ1),		//# BESPIN UNDERCITY
	ENUM2STRING(BESPIN_UNDERCITY_OBJ2),		//# BESPIN UNDERCITY
	ENUM2STRING(BESPIN_STREETS_OBJ1),		//# BESPIN STREETS
	ENUM2STRING(BESPIN_STREETS_OBJ2),		//# BESPIN STREETS
	ENUM2STRING(BESPIN_PLATFORM_OBJ1),		//# BESPIN PLATFORM
	ENUM2STRING(BESPIN_PLATFORM_OBJ2),		//# BESPIN PLATFORM
	ENUM2STRING(CAIRN_BAY_OBJ1),				//# CAIRN BAY
	ENUM2STRING(CAIRN_BAY_OBJ2),				//# CAIRN BAY
	ENUM2STRING(CAIRN_ASSEMBLY_OBJ1),		//# CAIRN ASSEMBLY
	ENUM2STRING(CAIRN_REACTOR_OBJ1),			//# CAIRN REACTOR
	ENUM2STRING(CAIRN_REACTOR_OBJ2),			//# CAIRN REACTOR
	ENUM2STRING(CAIRN_DOCK1_OBJ1),			//# CAIRN DOCK1
	ENUM2STRING(CAIRN_DOCK1_OBJ2),			//# CAIRN DOCK1
	ENUM2STRING(DOOM_COMM_OBJ1),				//# DOOM COMM
	ENUM2STRING(DOOM_COMM_OBJ2),				//# DOOM COMM
	ENUM2STRING(DOOM_COMM_OBJ3),				//# DOOM COMM
	ENUM2STRING(DOOM_DETENTION_OBJ1),		//# DOOM DETENTION
	ENUM2STRING(DOOM_DETENTION_OBJ2),		//# DOOM DETENTION
	ENUM2STRING(DOOM_SHIELDS_OBJ1),			//# DOOM SHIELDS
	ENUM2STRING(DOOM_SHIELDS_OBJ2),			//# DOOM SHIELDS
	ENUM2STRING(YAVIN_SWAMP_OBJ1),			//# YAVIN SWAMP
	ENUM2STRING(YAVIN_SWAMP_OBJ2),			//# YAVIN SWAMP
	ENUM2STRING(YAVIN_CANYON_OBJ1),			//# YAVIN CANYON
	ENUM2STRING(YAVIN_CANYON_OBJ2),			//# YAVIN CANYON
	ENUM2STRING(YAVIN_COURTYARD_OBJ1),		//# YAVIN COURTYARD
	ENUM2STRING(YAVIN_COURTYARD_OBJ2),		//# YAVIN COURTYARD
	ENUM2STRING(YAVIN_FINAL_OBJ1),			//# YAVIN FINAL
	ENUM2STRING(KEJIM_POST_OBJ3),			//# KEJIM POST - GRAPHICS IN IT.
	ENUM2STRING(KEJIM_POST_OBJ4),			//# KEJIM POST - GRAPHICS IN IT.
	ENUM2STRING(KEJIM_POST_OBJ5),			//# KEJIM POST - GRAPHICS IN IT.
	ENUM2STRING(ARTUS_DETENTION_OBJ3),		//# ARTUS DETENTION
	ENUM2STRING(DOOM_COMM_OBJ4),			//# DOOM COMM - GRAPHICS IN IT. IT MUST BE LAST IN THE LIST
	ENUM2STRING(DOOM_SHIELDS_OBJ3),			//# DOOM SHIELDS
	ENUM2STRING(DEMO_OBJ1),					//# DEMO
	ENUM2STRING(DEMO_OBJ2),					//# DEMO
	ENUM2STRING(DEMO_OBJ3),					//# DEMO
	ENUM2STRING(DEMO_OBJ4),					//# DEMO
	ENUM2STRING(SECBASE_OBJ1),				//# Darkforces Mod
	ENUM2STRING(SECBASE_OBJ2),				//# Darkforces Mod
	ENUM2STRING(TALAY_OBJ1),				//# Darkforces Mod
	ENUM2STRING(TALAY_OBJ2),				//# Darkforces Mod
	ENUM2STRING(TALAY_OBJ3),				//# Darkforces Mod
	ENUM2STRING(SEWERS_OBJ1),				//# Darkforces Mod
	ENUM2STRING(TESTBASE_OBJ1),				//# Darkforces Mod
	ENUM2STRING(TESTBASE_OBJ2),				//# Darkforces Mod
	ENUM2STRING(GROMAS_OBJ1),				//# Darkforces Mod
	ENUM2STRING(GROMAS_OBJ1),				//# Darkforces Mod
	ENUM2STRING(DTENTION_OBJ1),				//# Darkforces Mod
	ENUM2STRING(TOURNAMENT_OBJ1),			//# TOURNAMENT Mod
	ENUM2STRING(TOURNAMENT_OBJ2),			//# TOURNAMENT Mod
	ENUM2STRING(TOURNAMENT_OBJ3),			//# TOURNAMENT Mod
	ENUM2STRING(PRIVATEERHOTH2_OBJ1),		//# PRIVATEER
	ENUM2STRING(PRIVATEERHOTH2_OBJ2),		//# PRIVATEER
	ENUM2STRING(PRIVATEERHOTH2_OBJ3),		//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERHOTH3_OBJ1),		//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERHOTH3_OBJ2),		//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERHOTH3_OBJ3),		//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_DPREDICAMENT_OBJ1),//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_DPREDICAMENT_OBJ2),//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_DPREDICAMENT_OBJ3),//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_DPREDICAMENT_OBJ4),//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ1),	//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ2),	//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ3),	//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ4),	//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ5),	//# PRIVATEER Mod
	ENUM2STRING(PRIVATEERT2_RANCOR_OBJ6),	//# PRIVATEER Mod
	ENUM2STRING(HOTH5_OBJ1),	//# redemtion Mod
	ENUM2STRING(HOTH5_OBJ2),	//# redemtion Mod
	ENUM2STRING(HOTH5_OBJ3),	//# redemtion Mod
	ENUM2STRING(MOTH2_OBJ1),	//# DASH Mod
	ENUM2STRING(MOTH2_OBJ2),	//# DASH Mod
	ENUM2STRING(T5_WEDGE_OBJ1),	//# DASH Mod
	ENUM2STRING(T5_WEDGE_OBJ2),	//# DASH Mod
	ENUM2STRING(T5_WEDGE_OBJ3),	//# DASH Mod
	ENUM2STRING(T5_WEDGE_OBJ4),	//# DASH Mod
	ENUM2STRING(T5_WEDGE_OBJ5),	//# DASH Mod
	ENUM2STRING(T0_ROGUE_OBJ1),	//# tatooinecrash Mod
	ENUM2STRING(T0_ROGUE_OBJ2),	//# tatooinecrash Mod
	ENUM2STRING(VOTS_OBJ1),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ2),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ3),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ4),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ5),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ6),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ7),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ8),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ9),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ10),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ11),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ12),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ13),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ14),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ15),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ16),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ17),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ18),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ19),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ20),	    //# vengance Mod
	ENUM2STRING(VOTS_OBJ21),	    //# vengance Mod
	ENUM2STRING(DF2_01NAR_OBJ),		//# Darkforces2 Mod
	ENUM2STRING(DF2_02NAR_OBJ1),
	ENUM2STRING(DF2_02NAR_OBJ2),
	ENUM2STRING(DF2_02NAR_OBJ3),
	ENUM2STRING(DF2_03KATARN_OBJ),
	ENUM2STRING(DF2_07YUN_OBJ),
	ENUM2STRING(DF2_11GORC_OBJ1),
	ENUM2STRING(DF2_11GORC_OBJ2),
	ENUM2STRING(P2_WEDGE_OBJ1),
	ENUM2STRING(P2_WEDGE_OBJ2),
	ENUM2STRING(P2_WEDGE_OBJ3),

	ENUM2STRING(L1_OBJ1),
	ENUM2STRING(L1_OBJ2),
	ENUM2STRING(L1_OBJ3),
	ENUM2STRING(L1_OBJ4),
	ENUM2STRING(L1_OBJ5),
	ENUM2STRING(L1_OBJ6),
	ENUM2STRING(L1_OBJ7),

	ENUM2STRING(L1_LAVA),
	ENUM2STRING(L1_DESERT_OUTPOST),

	ENUM2STRING(L2_OBJ1),
	ENUM2STRING(L2_OBJ2),
	ENUM2STRING(L2_OBJ3),
	ENUM2STRING(L2_OBJ4),
	ENUM2STRING(L2_OBJ5),
	ENUM2STRING(L2_OBJ6),
	ENUM2STRING(L2_OBJ7),
	ENUM2STRING(L2_OBJ8),
	ENUM2STRING(L2_OBJ9),

	ENUM2STRING(L2_SABER_SHIPMENT),
	ENUM2STRING(L2_TRANSPORT_ENTRY),

	ENUM2STRING(L3_OBJ1),
	ENUM2STRING(L3_OBJ2),
	ENUM2STRING(L3_OBJ3),
	ENUM2STRING(L3_OBJ4),
	ENUM2STRING(L3_OBJ5),
	ENUM2STRING(L3_OBJ6),
	ENUM2STRING(L3_OBJ7),
	ENUM2STRING(L3_OBJ8),

	ENUM2STRING(L3_OLD_REACTOR),

	ENUM2STRING(L4_OBJ1),
	ENUM2STRING(L4_OBJ2),
	ENUM2STRING(L4_OBJ3),
	ENUM2STRING(L4_OBJ4),
	ENUM2STRING(L4_OBJ5),

	ENUM2STRING(L4_WEAPONS),
	ENUM2STRING(L4_CRYSTAL),

	ENUM2STRING(L5_OBJ1),
	ENUM2STRING(L5_OBJ2),
	ENUM2STRING(L5_OBJ3),
	ENUM2STRING(L5_OBJ4),
	ENUM2STRING(L5_OBJ5),
	ENUM2STRING(L5_OBJ6),

	ENUM2STRING(L5_FUELING),
	ENUM2STRING(L5_SITH),

	ENUM2STRING(L6_HOME),
	ENUM2STRING(L6_OUTSKIRTS),
	ENUM2STRING(L6_RESCUE_GOOD),
	ENUM2STRING(L6_OUTSKIRTS_SITH),
	ENUM2STRING(L6_SITH),
	ENUM2STRING(L6_QUEEN_SITH),
	ENUM2STRING(L6_OUTSKIRTS_DARK),
	ENUM2STRING(L6_DARK),

	ENUM2STRING(CLOUDY_OBJ1),
	ENUM2STRING(CLOUDY_OBJ2),
	ENUM2STRING(CLOUDY_OBJ3),
	ENUM2STRING(CLOUDY_OBJ4),
	ENUM2STRING(CLOUDY_OBJ5),

	//MOVIEDUELS

ENUM2STRING(MDT1_PO_OBJ1),
ENUM2STRING(MD2_ROC_JEDI_OBJ1),

ENUM2STRING(MD_TOT_OBJ1),
ENUM2STRING(MD_TOT_OBJ2),
ENUM2STRING(MD_TOT_OBJ3),
ENUM2STRING(MD_TOT_OBJ4),
ENUM2STRING(MD_TOT_OBJ5),

ENUM2STRING(MD_SN_OBJ1),
ENUM2STRING(MD_SN_OBJ2),
ENUM2STRING(MD_SN_OBJ3),

ENUM2STRING(MD_OOT_OBJ1),
ENUM2STRING(MD_OOT_OBJ2),
ENUM2STRING(MD_OOT_OBJ3),

ENUM2STRING(MD_DOTF_OBJ1),

ENUM2STRING(MD_GD_OBJ1),
ENUM2STRING(MD_GD_OBJ2),

ENUM2STRING(MD_EGG_OBJ1),
ENUM2STRING(MD_EGG_OBJ2),
ENUM2STRING(MD_EGG_OBJ3),

ENUM2STRING(MD_ROC_OBJ1),
ENUM2STRING(MD_ROC_OBJ2),
ENUM2STRING(MD_ROC_OBJ3),
ENUM2STRING(MD_ROC_OBJ4),
ENUM2STRING(MD_ROC_OBJ5),
ENUM2STRING(MD_ROC_OBJ6),

ENUM2STRING(MD_LUKE_OBJ1),
ENUM2STRING(MD_LUKE_OBJ2),

ENUM2STRING(MD_RT_OBJ1),
ENUM2STRING(MD_RT_OBJ2),
ENUM2STRING(MD_RT_OBJ3),

ENUM2STRING(MD_JT_STAR_OBJ1),
ENUM2STRING(MD_JT_STAR_OBJ2),
ENUM2STRING(MD_JT_STAR_OBJ3),
ENUM2STRING(MD_JT_STAR_OBJ4),

ENUM2STRING(MD_MUSJUMP_OBJ1),

ENUM2STRING(MD_MOF_OBJ1),
ENUM2STRING(MD_MOF_OBJ2),
ENUM2STRING(MD_MOF_OBJ3),
ENUM2STRING(MD_MOF_OBJ4),

ENUM2STRING(MD_MUS_OBJ1),
ENUM2STRING(MD_MUS_OBJ2),

ENUM2STRING(MD_JT_OBJ1),
ENUM2STRING(MD_JT_OBJ2),
ENUM2STRING(MD_JT_OBJ3),

ENUM2STRING(MD_JT2_OBJ1),
ENUM2STRING(MD_JT2_OBJ2),
ENUM2STRING(MD_JT2_OBJ3),
ENUM2STRING(MD_JT2_OBJ4),

ENUM2STRING(MD_GB_OBJ1),
ENUM2STRING(MD_GB_OBJ2),
ENUM2STRING(MD_GB_OBJ3),
ENUM2STRING(MD_GB_OBJ4),
ENUM2STRING(MD_GB_OBJ5),
ENUM2STRING(MD_GB_OBJ6),
ENUM2STRING(MD_GB_OBJ7),

ENUM2STRING(MD_GA_OBJ1),
ENUM2STRING(MD_GA_OBJ2),
ENUM2STRING(MD_GA_OBJ3),
ENUM2STRING(MD_GA_OBJ4),
ENUM2STRING(MD_GA_OBJ5),
ENUM2STRING(MD_GA_OBJ6),
ENUM2STRING(MD_GA_OBJ7),
ENUM2STRING(MD_GA_OBJ8),
ENUM2STRING(MD_GA_OBJ9),
ENUM2STRING(MD_GA_OBJ10),
ENUM2STRING(MD_GA_OBJ11),
ENUM2STRING(MD_GA_OBJ12),
ENUM2STRING(MD_GA_OBJ13),

ENUM2STRING(MD_TC_OBJ1),
ENUM2STRING(MD_TC_OBJ2),

ENUM2STRING(MD_AM_OBJ1),
ENUM2STRING(MD_AM_OBJ2),

ENUM2STRING(MD_KA_OBJ1),
ENUM2STRING(MD_KA_OBJ2),

ENUM2STRING(FINAL_SCRIPT),

//stringID_table_t Must end with a null entry
{ "", 0 }
};

stringID_table_t missionFailedTable[] =
{
	ENUM2STRING(MISSIONFAILED_JAN),			//# JAN DIED
	ENUM2STRING(MISSIONFAILED_LUKE),		//# LUKE DIED
	ENUM2STRING(MISSIONFAILED_LANDO),		//# LANDO DIED
	ENUM2STRING(MISSIONFAILED_R5D2),		//# R5D2 DIED
	ENUM2STRING(MISSIONFAILED_WARDEN),		//# THE WARDEN DIED
	ENUM2STRING(MISSIONFAILED_PRISONERS),	//#	TOO MANY PRISONERS DIED
	ENUM2STRING(MISSIONFAILED_EMPLACEDGUNS),//#	ALL EMPLACED GUNS GONE
	ENUM2STRING(MISSIONFAILED_LADYLUCK),	//#	LADY LUCK DISTROYED
	ENUM2STRING(MISSIONFAILED_KYLECAPTURE),	//# KYLE HAS BEEN CAPTURED
	ENUM2STRING(MISSIONFAILED_TOOMANYALLIESDIED),	//# TOO MANY ALLIES DIED
	ENUM2STRING(MISSIONFAILED_CHEWIE),
	ENUM2STRING(MISSIONFAILED_KYLE),
	ENUM2STRING(MISSIONFAILED_ROSH),
	ENUM2STRING(MISSIONFAILED_WEDGE),
	ENUM2STRING(MISSIONFAILED_TURNED),		//# Turned on your friends.

	//stringID_table_t Must end with a null entry
	{ "", 0 }
};

stringID_table_t statusTextTable[] =
{
	//=================================================
	//
	//=================================================
	ENUM2STRING(STAT_INSUBORDINATION),				//# Starfleet will not tolerate such insubordination
	ENUM2STRING(STAT_YOUCAUSEDDEATHOFTEAMMATE),		//# You caused the death of a teammate.
	ENUM2STRING(STAT_DIDNTPROTECTTECH),				//# You failed to protect Chell, your technician.
	ENUM2STRING(STAT_DIDNTPROTECT7OF9),				//# You failed to protect 7 of 9
	ENUM2STRING(STAT_NOTSTEALTHYENOUGH),			//# You weren't quite stealthy enough
	ENUM2STRING(STAT_STEALTHTACTICSNECESSARY),		//# Starfleet will not tolerate such insubordination
	ENUM2STRING(STAT_WATCHYOURSTEP),				//# Watch your step
	ENUM2STRING(STAT_JUDGEMENTMUCHDESIRED),			//# Your judgement leaves much to be desired
	//stringID_table_t Must end with a null entry
	{ "", 0 }
};

#endif// #ifndef G_OBJECTIVES_CPP

#endif// #ifndef __OBJECTIVES_H__
