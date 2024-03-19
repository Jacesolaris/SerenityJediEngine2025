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

// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"
#include "cl_uiapi.h"

extern console_t con;
qboolean scr_initialized; // ready to draw

cvar_t* cl_timegraph;
cvar_t* cl_debuggraph;
cvar_t* cl_graphheight;
cvar_t* cl_graphscale;
cvar_t* cl_graphshift;
extern cvar_t* cl_com_outcast;

extern cvar_t* cl_com_rend2;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic(const float x, const float y, const float width, const float height, const char* picname)
{
	assert(width != 0);

	const qhandle_t hShader = re->RegisterShader(picname);
	re->DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect(const float x, const float y, const float width, const float height, const float* color)
{
	re->SetColor(color);

	re->DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);

	re->SetColor(nullptr);
}

/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic(const float x, const float y, const float width, const float height, const qhandle_t hShader)
{
	re->DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar(const int x, const int y, float size, int ch)
{
	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -size)
	{
		return;
	}

	const float ax = x;
	const float ay = y;
	const float aw = size;
	const float ah = size;

	const int row = ch >> 4;
	const int col = ch & 15;

	const float frow = row * 0.0625;
	const float fcol = col * 0.0625;
	size = 0.03125;
	constexpr float size2 = 0.0625;

	re->DrawStretchPic(ax, ay, aw, ah,
		fcol, frow,
		fcol + size, frow + size2,
		cls.charSetShader);
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar(const int x, const int y, int ch)
{
	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	if (y < -SMALLCHAR_HEIGHT)
	{
		return;
	}

	const int row = ch >> 4;
	const int col = ch & 15;

	const float frow = row * 0.0625;
	const float fcol = col * 0.0625;

	constexpr float size = 0.03125;
	//	size = 0.0625;

	constexpr float size2 = 0.0625;

	re->DrawStretchPic(x * con.xadjust, y * con.yadjust,
		SMALLCHAR_WIDTH * con.xadjust, SMALLCHAR_HEIGHT * con.yadjust,
		fcol, frow,
		fcol + size, frow + size2,
		cls.charSetShader);
}

/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt(const int x, const int y, const float size, const char* string, const float* setColor,
	const qboolean forceColor, const qboolean noColorEscape)
{
	vec4_t color{};

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3] = setColor[3];
	re->SetColor(color);
	const char* s = string;
	int xx = x;
	while (*s)
	{
		if (!noColorEscape && Q_IsColorString(s))
		{
			s += 2;
			continue;
		}
		SCR_DrawChar(xx + 2, y + 2, size, *s);
		xx += size;
		s++;
	}

	// draw the colored text
	s = string;
	xx = x;
	re->SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof color);
				color[3] = setColor[3];
				re->SetColor(color);
			}
			if (!noColorEscape)
			{
				s += 2;
				continue;
			}
		}
		SCR_DrawChar(xx, y, size, *s);
		xx += size;
		s++;
	}
	re->SetColor(nullptr);
}

void SCR_DrawBigString(const int x, const int y, const char* s, const float alpha, const qboolean noColorEscape)
{
	float color[4]{};

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qfalse, noColorEscape);
}

void SCR_DrawBigStringColor(const int x, const int y, const char* s, vec4_t color, const qboolean noColorEscape)
{
	SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qtrue, noColorEscape);
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt(const int x, const int y, const char* string, const float* setColor,
	const qboolean forceColor, const qboolean noColorEscape)
{
	vec4_t color;

	// draw the colored text
	const char* s = string;
	int xx = x;
	re->SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				Com_Memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof color);
				color[3] = setColor[3];
				re->SetColor(color);
			}
			if (!noColorEscape)
			{
				s += 2;
				continue;
			}
		}
		SCR_DrawSmallChar(xx, y, *s);
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re->SetColor(nullptr);
}

/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen(const char* str)
{
	const char* s = str;
	int count = 0;

	while (*s)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
		}
		else
		{
			count++;
			s++;
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/
int SCR_GetBigStringWidth(const char* str)
{
	return SCR_Strlen(str) * BIGCHAR_WIDTH;
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording(void)
{
	char string[1024];

	if (!clc.demorecording)
	{
		return;
	}
	if (clc.spDemoRecording)
	{
		return;
	}

	if (!cl_drawRecording->integer)
	{
		return;
	}

	const int pos = FS_FTell(clc.demofile);
	Com_sprintf(string, sizeof string, "RECORDING %s: %ik", clc.demoName, pos / 1024);

	SCR_DrawStringExt(320 - strlen(string) * 4, 20, 8, string, g_color_table[7], qtrue, qfalse);
}

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

using graphsamp_t = struct graphsamp_s
{
	float value;
	int color;
};

static int current;
static graphsamp_t values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph(const float value, const int color)
{
	values[current & 1023].value = value;
	values[current & 1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph(void)
{
	//
	// draw the graph
	//
	constexpr int w = 640;
	constexpr int x = 0;
	constexpr int y = 480;
	re->SetColor(g_color_table[0]);
	re->DrawStretchPic(x, y - cl_graphheight->integer,
		w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
	re->SetColor(nullptr);

	for (int a = 0; a < w; a++)
	{
		const int i = current - 1 - a + 1024 & 1023;
		float v = values[i].value;
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if (v < 0)
			v += cl_graphheight->integer * (1 + static_cast<int>(-v / cl_graphheight->integer));
		const int h = static_cast<int>(v) % cl_graphheight->integer;
		re->DrawStretchPic(x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader);
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	cl_timegraph = Cvar_Get("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get("graphshift", "0", CVAR_CHEAT);

	scr_initialized = qtrue;
}

//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField(const stereoFrame_t stereoFrame)
{
	re->BeginFrame(stereoFrame);

	const auto uiFullscreen = static_cast<qboolean>(cls.uiStarted && UIVM_IsFullscreen());

	if (!cls.uiStarted)
	{
		Com_DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	//actually, yes you do, unless you want clients to cycle out their reliable
	//commands from sitting in the menu. -rww
	if (cls.uiStarted && !uiFullscreen || !(cls.framecount & 7) && cls.state == CA_ACTIVE)
	{
		switch (cls.state)
		{
		default:
			Com_Error(ERR_FATAL, "SCR_DrawScreenField: bad cls.state");
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			UIVM_SetActiveMenu(UIMENU_MAIN);
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			UIVM_Refresh(cls.realtime);
			UIVM_DrawConnectScreen(qfalse);
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering(stereoFrame);

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			UIVM_Refresh(cls.realtime);
			UIVM_DrawConnectScreen(qtrue);
			break;
		case CA_ACTIVE:
			CL_CGameRendering(stereoFrame);
			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if (Key_GetCatcher() & KEYCATCH_UI && cls.uiStarted)
	{
		UIVM_Refresh(cls.realtime);
	}

	// console draws next
	Con_DrawConsole();

	// debug graph can be drawn on top of anything
	if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer)
	{
		SCR_DrawDebugGraph();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen(void)
{
	static int recursive;

	if (!scr_initialized)
	{
		return; // not initialized yet
	}

	if (++recursive > 2)
	{
		Com_Error(ERR_FATAL, "SCR_UpdateScreen: recursively called");
	}
	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if (cls.uiStarted || com_dedicated->integer)
	{
		// if running in stereo, we need to draw the frame twice
		if (cls.glconfig.stereoEnabled)
		{
			SCR_DrawScreenField(STEREO_LEFT);
			SCR_DrawScreenField(STEREO_RIGHT);
		}
		else
		{
			SCR_DrawScreenField(STEREO_CENTER);
		}

		if (com_speeds->integer)
		{
			re->EndFrame(&time_frontend, &time_backend);
		}
		else
		{
			re->EndFrame(nullptr, nullptr);
		}
	}

	recursive = 0;
}