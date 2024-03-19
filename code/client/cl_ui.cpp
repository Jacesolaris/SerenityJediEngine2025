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

#include "../server/exe_headers.h"

#include "client.h"
#include "client_ui.h"
#include "qcommon/stringed_ingame.h"

#include "vmachine.h"

intptr_t CL_UISystemCalls(const intptr_t* args);

//prototypes
#ifdef JK2_MODE
extern qboolean SG_GetSaveImage(const char* psPathlessBaseName, void* pvAddress);
#endif
extern int SG_GetSaveGameComment(const char* psPathlessBaseName, char* sComment, char* sMapName);
extern qboolean SG_GameAllowedToSaveHere(qboolean inCamera);
extern void SG_StoreSaveGameComment(const char* s_comment);
extern byte* SCR_GetScreenshot(qboolean* qValid); // uncommented

/*
====================
Helper functions for User Interface
====================
*/

/*
====================
GetClientState
====================
*/
static connstate_t GetClientState()
{
	return cls.state;
}

/*
====================
CL_GetGlConfig
====================
*/
static void UI_GetGlconfig(glconfig_t* config)
{
	*config = cls.glconfig;
}

/*
====================
GetClipboardData
====================
*/
static void GetClipboardData(char* buf, const int buflen)
{
	char* cbd, * c;

	c = cbd = Sys_GetClipboardData();
	if (!cbd)
	{
		*buf = 0;
		return;
	}

	for (int i = 0, end = buflen - 1; *c && i < end; i++)
	{
		const uint32_t utf32 = ConvertUTF8ToUTF32(c, &c);
		buf[i] = ConvertUTF32ToExpectedCharset(utf32);
	}

	Z_Free(cbd);
}

/*
====================
Key_KeynumToStringBuf
====================
*/
// only ever called by binding-display code, therefore returns non-technical "friendly" names
//	in any language that don't necessarily match those in the config file...
//
void Key_KeynumToStringBuf(const int keynum, char* buf, const int buflen)
{
	const char* psKeyName = Key_KeynumToString(keynum/*, qtrue */);

	// see if there's a more friendly (or localised) name...
	//
	const char* psKeyNameFriendly = SE_GetString(va("KEYNAMES_KEYNAME_%s", psKeyName));

	Q_strncpyz(buf, psKeyNameFriendly && psKeyNameFriendly[0] ? psKeyNameFriendly : psKeyName, buflen);
}

/*
====================
Key_GetBindingBuf
====================
*/
void Key_GetBindingBuf(const int keynum, char* buf, const int buflen)
{
	const char* value = Key_GetBinding(keynum);
	if (value)
	{
		Q_strncpyz(buf, value, buflen);
	}
	else
	{
		*buf = 0;
	}
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt(const float f)
{
	byteAlias_t fi{};
	fi.f = f;
	return fi.i;
}

static void UI_Cvar_Create(const char* var_name, const char* var_value, const int flags)
{
	Cvar_Register(nullptr, var_name, var_value, flags);
}

static int GetConfigString(const int index, char* buf, const int size)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	const int offset = cl.gameState.stringOffsets[index];
	if (!offset)
		return qfalse;

	Q_strncpyz(buf, cl.gameState.stringData + offset, size);

	return qtrue;
}

/*
====================
CL_ShutdownUI
====================
*/
void UI_Shutdown();

void CL_ShutdownUI()
{
	UI_Shutdown();
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_UI);
	cls.uiStarted = qfalse;
}

void CL_DrawDatapad(const int HUDType)
{
	switch (HUDType)
	{
	case DP_HUD:
		VM_Call(CG_DRAW_DATAPAD_HUD);
		break;
	case DP_OBJECTIVES:
		VM_Call(CG_DRAW_DATAPAD_OBJECTIVES);
		break;
	case DP_WEAPONS:
		VM_Call(CG_DRAW_DATAPAD_WEAPONS);
		break;
	case DP_INVENTORY:
		VM_Call(CG_DRAW_DATAPAD_INVENTORY);
		break;
	case DP_FORCEPOWERS:
		VM_Call(CG_DRAW_DATAPAD_FORCEPOWERS);
		break;
	default:
		break;
	}
}

void UI_Init(int apiVersion, const uiimport_t* uiimport, qboolean inGameLoad);

/*
====================
CL_InitUI
====================
*/
void CL_InitUI()
{
#ifdef JK2_MODE
	JK2SP_Register("keynames", 0	/*SP_REGISTER_REQUIRED*/);		// reference is KEYNAMES
#endif

	uiimport_t uii = {};

	uii.Printf = Com_Printf;
	uii.Error = Com_Error;

	uii.Cvar_Set = Cvar_Set;
	uii.Cvar_VariableValue = Cvar_VariableValue;
	uii.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;
	uii.Cvar_SetValue = Cvar_SetValue;
	uii.Cvar_Reset = Cvar_Reset;
	uii.Cvar_Create = UI_Cvar_Create;
	uii.Cvar_InfoStringBuffer = Cvar_InfoStringBuffer;

	uii.Draw_DataPad = CL_DrawDatapad;

	uii.Argc = Cmd_Argc;
	uii.Argv = Cmd_ArgvBuffer;
	uii.Cmd_TokenizeString = Cmd_TokenizeString;

	uii.Cmd_ExecuteText = Cbuf_ExecuteText;

	uii.FS_FOpenFile = FS_FOpenFileByMode;
	uii.FS_Read = FS_Read;
	uii.FS_Write = FS_Write;
	uii.FS_FCloseFile = FS_FCloseFile;
	uii.FS_GetFileList = FS_GetFileList;
	uii.FS_ReadFile = FS_ReadFile;
	uii.FS_FreeFile = FS_FreeFile;

	uii.R_RegisterModel = re.RegisterModel;
	uii.R_RegisterSkin = re.RegisterSkin;
	uii.R_RegisterShader = re.RegisterShader;
	uii.R_RegisterShaderNoMip = re.RegisterShaderNoMip;
	uii.R_RegisterFont = re.RegisterFont;
	uii.R_Font_StrLenPixels = re.Font_StrLenPixels;
	uii.R_Font_HeightPixels = re.Font_HeightPixels;
	uii.R_Font_DrawString = re.Font_DrawString;
	uii.R_Font_StrLenChars = re.Font_StrLenChars;
	uii.Language_IsAsian = re.Language_IsAsian;
	uii.Language_UsesSpaces = re.Language_UsesSpaces;
	uii.AnyLanguage_ReadCharFromString = re.AnyLanguage_ReadCharFromString;

#ifdef JK2_MODE
	uii.SG_GetSaveImage = SG_GetSaveImage;
#endif
	uii.SG_GetSaveGameComment = SG_GetSaveGameComment;
	uii.SG_StoreSaveGameComment = SG_StoreSaveGameComment;
	uii.SG_GameAllowedToSaveHere = SG_GameAllowedToSaveHere;

	//uii.SCR_GetScreenshot		= SCR_GetScreenshot;

#ifdef JK2_MODE
	uii.DrawStretchRaw = re.DrawStretchRaw;
#endif
	uii.R_ClearScene = re.ClearScene;
	uii.R_AddRefEntityToScene = re.AddRefEntityToScene;
	uii.R_AddPolyToScene = re.AddPolyToScene;
	uii.R_AddLightToScene = re.AddLightToScene;
	uii.R_RenderScene = re.RenderScene;

	uii.R_ModelBounds = re.ModelBounds;

	uii.R_SetColor = re.SetColor;
	uii.R_DrawStretchPic = re.DrawStretchPic;
	uii.UpdateScreen = SCR_UpdateScreen;

#ifdef JK2_MODE
	uii.PrecacheScreenshot = SCR_PrecacheScreenshot;
#endif

	uii.R_LerpTag = re.LerpTag;

	uii.S_StartLocalLoopingSound = S_StartLocalLoopingSound;
	uii.S_StartLocalSound = S_StartLocalSound;
	uii.S_RegisterSound = S_RegisterSound;

	uii.Key_KeynumToStringBuf = Key_KeynumToStringBuf;
	uii.Key_GetBindingBuf = Key_GetBindingBuf;
	uii.Key_SetBinding = Key_SetBinding;
	uii.Key_IsDown = Key_IsDown;
	uii.Key_GetOverstrikeMode = Key_GetOverstrikeMode;
	uii.Key_SetOverstrikeMode = Key_SetOverstrikeMode;
	uii.Key_ClearStates = Key_ClearStates;
	uii.Key_GetCatcher = Key_GetCatcher;
	uii.Key_SetCatcher = Key_SetCatcher;
#ifdef JK2_MODE
	uii.SP_Register = JK2SP_Register;
	uii.SP_GetStringText = JK2SP_GetStringText;
	uii.SP_GetStringTextString = JK2SP_GetStringTextString;
#endif

	uii.GetClipboardData = GetClipboardData;

	uii.GetClientState = GetClientState;

	uii.GetGlconfig = UI_GetGlconfig;

	uii.GetConfigString = (void (*)(int, char*, int))GetConfigString;

	uii.Milliseconds = Sys_Milliseconds2;

	UI_Init(UI_API_VERSION, &uii, static_cast<qboolean>(cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE));
}

qboolean UI_GameCommand()
{
	if (!cls.uiStarted)
	{
		return qfalse;
	}
	return UI_ConsoleCommand();
}

void CL_GenericMenu_f()
{
	const char* arg = Cmd_Argv(1);

	if (cls.uiStarted)
	{
		if (cl_com_outcast->integer == 0)//academy version
		{
			UI_SetActiveMenu("ingame", arg);
		}
		else if (cl_com_outcast->integer == 1)//outcast version
		{
			UI_SetActiveMenu("ingame_jko", arg);
		}
		else if (cl_com_outcast->integer == 2)//mod version
		{
			UI_SetActiveMenu("ingame_cr", arg);
		}
		else if (cl_com_outcast->integer == 3)//yavIV version
		{
			UI_SetActiveMenu("ingame_yav", arg);
		}
		else if (cl_com_outcast->integer == 4)//darkforces version
		{
			UI_SetActiveMenu("ingame_df", arg);
		}
		else if (cl_com_outcast->integer == 5)//kotor version
		{
			UI_SetActiveMenu("ingame_kt", arg);
		}
		else if (cl_com_outcast->integer == 6)//survival version
		{
			UI_SetActiveMenu("ingame_suv", arg);
		}
		else if (cl_com_outcast->integer == 7)//nina version
		{
			UI_SetActiveMenu("ingame_nina", arg);
		}
		else if (cl_com_outcast->integer == 8)//veng version
		{
			UI_SetActiveMenu("ingame_veng", arg);
		}
		else
		{
			UI_SetActiveMenu("ingame", arg);
		}
	}
}

void CL_EndScreenDissolve_f()
{
	re.InitDissolve(qtrue); // dissolve from cinematic to underlying ingame
}

void CL_DataPad_f()
{
	if (cls.uiStarted && cls.cgameStarted && cls.state == CA_ACTIVE)
	{
		UI_SetActiveMenu("datapad", nullptr);
	}
}

/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig(glconfig_t* config)
{
	*config = cls.glconfig;
}

/*
int PC_ReadTokenHandle(int handle, pc_token_t *pc_token);
int PC_SourceFileAndLine(int handle, char *filename, int *line);
*/
/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
intptr_t CL_UISystemCalls(const intptr_t* args)
{
	switch (args[0])
	{
	case UI_ERROR:
		Com_Error(ERR_DROP, "%s", VMA(1));

	case UI_CVAR_REGISTER:
		Cvar_Register(static_cast<vmCvar_t*>(VMA(1)), static_cast<const char*>(VMA(2)),
			static_cast<const char*>(VMA(3)), args[4]);
		return 0;

	case UI_CVAR_SET:
		Cvar_Set(static_cast<const char*>(VMA(1)), static_cast<const char*>(VMA(2)));
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_SetValue(static_cast<const char*>(VMA(1)), VMF(2));
		return 0;

	case UI_CVAR_UPDATE:
		Cvar_Update(static_cast<vmCvar_t*>(VMA(1)));
		return 0;

	case UI_R_REGISTERMODEL:
		return re.RegisterModel(static_cast<const char*>(VMA(1)));

	case UI_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip(static_cast<const char*>(VMA(1)));

	case UI_GETGLCONFIG:
		CL_GetGlconfig(static_cast<glconfig_t*>(VMA(1)));
		return 0;

	case UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], static_cast<const char*>(VMA(2)));
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue(static_cast<const char*>(VMA(1))));

	case UI_FS_GETFILELIST:
		return FS_GetFileList(static_cast<const char*>(VMA(1)), static_cast<const char*>(VMA(2)),
			static_cast<char*>(VMA(3)), args[4]);

	case UI_KEY_SETCATCHER:
		Key_SetCatcher(args[1]);
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_R_SETCOLOR:
		re.SetColor(static_cast<const float*>(VMA(1)));
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer(static_cast<const char*>(VMA(1)), static_cast<char*>(VMA(2)), args[3]);
		return 0;

	case UI_R_MODELBOUNDS:
		re.ModelBounds(args[1], static_cast<float*>(VMA(2)), static_cast<float*>(VMA(3)));
		return 0;

	case UI_R_CLEARSCENE:
		re.ClearScene();
		return 0;

		//	case UI_KEY_GETOVERSTRIKEMODE:
		//		return Key_GetOverstrikeMode();
		//		return 0;

		//	case UI_PC_READ_TOKEN:
		//		return PC_ReadTokenHandle( args[1], VMA(2) );

		//	case UI_PC_SOURCE_FILE_AND_LINE:
		//		return PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_MILLISECONDS:
		return Sys_Milliseconds();

	case UI_S_REGISTERSOUND:
		return S_RegisterSound(static_cast<const char*>(VMA(1)));

	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2]);
		return 0;

		//	case UI_R_REGISTERFONT:
		//		re.RegisterFont( VMA(1), args[2], VMA(3));
		//		return 0;

	case UI_CIN_PLAYCINEMATIC:
		Com_DPrintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic(static_cast<const char*>(VMA(1)), args[2], args[3], args[4], args[5], args[6],
			static_cast<const char*>(VMA(7)));

	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding(args[1], static_cast<const char*>(VMA(2)));
		return 0;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;

	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], static_cast<char*>(VMA(2)), args[3]);
		return 0;

	default:
		Com_Error(ERR_DROP, "Bad UI system trap: %i", args[0]);
	}
}