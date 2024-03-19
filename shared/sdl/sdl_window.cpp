/*
===========================================================================
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

#include <SDL.h>
#include <SDL_syswm.h>
#include "qcommon/qcommon.h"
#include "rd-common/tr_types.h"
#include "sys/sys_local.h"
#include "sdl_icon.h"

enum rserr_t
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
};

static SDL_Window* screen = nullptr;
static SDL_GLContext opengl_context;
static float displayAspect;

cvar_t* r_sdlDriver;
cvar_t* r_allowSoftwareGL;

// Window cvars
cvar_t* r_fullscreen = nullptr;
cvar_t* r_noborder;
cvar_t* r_centerWindow;
cvar_t* r_customwidth;
cvar_t* r_customheight;
cvar_t* r_swapInterval;
cvar_t* r_stereo;
cvar_t* r_mode;
cvar_t* r_displayRefresh;

// Window surface cvars
cvar_t* r_stencilbits;
cvar_t* r_depthbits;
cvar_t* r_colorbits;
cvar_t* r_ignorehwgamma;
cvar_t* r_ext_multisample;

/*
** R_GetModeInfo
*/
using vidmode_t = struct vidmode_s
{
	const char* description;
	int width, height;
};

constexpr vidmode_t r_vidModes[] =
{
	{"Mode  0: 320x240", 320, 240},
	{"Mode  1: 400x300", 400, 300},
	{"Mode  2: 512x384", 512, 384},
	{"Mode  3: 640x480", 640, 480},
	{"Mode  4: 800x600", 800, 600},
	{"Mode  5: 856x480", 856, 480},
	{"Mode  6: 960x720", 960, 720},
	{"Mode  7: 1024x768", 1024, 768},
	{"Mode  8: 1152x864", 1152, 864},
	{"Mode  9: 1280x720", 1280, 720}, //720p
	{"Mode 10: 1280x1024", 1280, 1024},
	{"Mode 11: 1366x768", 1366, 768},
	{"Mode 12: 1440x900", 1440, 900},
	{"Mode 13: 1600x900", 1600, 900},
	{"Mode 14: 1600x1200", 1600, 1200},
	{"Mode 15: 1680x1050", 1680, 1050},
	{"Mode 16: 1920x1080", 1920, 1080}, //1080p
	{"Mode 17: 2048x1536", 2048, 1536},
	{"Mode 18: 2400x600", 2400, 600},
	{"Mode 19: 2560x1440", 2560, 1440},
	{"Mode 20: 3840x2160", 3840, 2160} //4K Video mode
};
static constexpr int s_numVidModes = ARRAY_LEN(r_vidModes);

constexpr auto R_MODE_FALLBACK = 4; // 640x480;

qboolean R_GetModeInfo(int* width, int* height, const int mode)
{
	if (mode < -1)
	{
		return qfalse;
	}
	if (mode >= s_numVidModes)
	{
		return qfalse;
	}

	if (mode == -1)
	{
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		return qtrue;
	}

	const vidmode_t* vm = &r_vidModes[mode];

	*width = vm->width;
	*height = vm->height;

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f(void)
{
	Com_Printf("\n");
	Com_Printf("Mode -2: Use desktop resolution\n");
	Com_Printf("Mode -1: Use r_customWidth and r_customHeight variables\n");
	for (int i = 0; i < s_numVidModes; i++)
	{
		Com_Printf("%s\n", r_vidModes[i].description);
	}
	Com_Printf("\n");
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize()
{
	SDL_MinimizeWindow(screen);
}

void WIN_Present(const window_t* window)
{
	if (window->api == GRAPHICS_API_OPENGL)
	{
		SDL_GL_SwapWindow(screen);

		if (r_swapInterval->modified)
		{
			r_swapInterval->modified = qfalse;
			if (SDL_GL_SetSwapInterval(r_swapInterval->integer) == -1)
			{
				Com_DPrintf("SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
			}
		}
	}

	if (r_fullscreen->modified)
	{
		// Find out the current state
		const bool fullscreen = (SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN) != 0;

		if (r_fullscreen->integer && Cvar_VariableIntegerValue("in_nograb"))
		{
			Com_Printf("Fullscreen not allowed with in_nograb 1\n");
			Cvar_Set("r_fullscreen", "0");
			r_fullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		const bool needToToggle = !!r_fullscreen->integer != fullscreen;

		if (needToToggle)
		{
			const bool sdlToggled = SDL_SetWindowFullscreen(screen, r_fullscreen->integer) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if (!sdlToggled)
				Cbuf_AddText("vid_restart\n");

			IN_Restart();
		}

		r_fullscreen->modified = qfalse;
	}
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes(const void* a, const void* b)
{
	constexpr float ASPECT_EPSILON = 0.001f;
	const SDL_Rect* modeA = (SDL_Rect*)a;
	const SDL_Rect* modeB = (SDL_Rect*)b;
	const float aspectA = static_cast<float>(modeA->w) / static_cast<float>(modeA->h);
	const float aspectB = static_cast<float>(modeB->w) / static_cast<float>(modeB->h);
	const int areaA = modeA->w * modeA->h;
	const int areaB = modeB->w * modeB->h;
	const float aspectDiffA = fabs(aspectA - displayAspect);
	const float aspectDiffB = fabs(aspectB - displayAspect);
	const float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if (aspectDiffsDiff > ASPECT_EPSILON)
		return 1;
	if (aspectDiffsDiff < -ASPECT_EPSILON)
		return -1;
	return areaA - areaB;
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static bool GLimp_DetectAvailableModes()
{
	int i, j;
	char buf[MAX_STRING_CHARS] = { 0 };
	int numModes = 0;

	const int display = SDL_GetWindowDisplayIndex(screen);
	if (display < 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Couldn't get window display index, no resolutions detected: %s\n",
			SDL_GetError());
		return false;
	}

	SDL_DisplayMode windowMode;

	if (SDL_GetWindowDisplayMode(screen, &windowMode) < 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Couldn't get window display mode, no resolutions detected (%s).\n",
			SDL_GetError());
		return false;
	}

	const int numDisplayModes = SDL_GetNumDisplayModes(display);
	if (numDisplayModes < 0)
		Com_Error(ERR_FATAL, "SDL_GetNumDisplayModes() FAILED (%s)", SDL_GetError());

	const auto modes = static_cast<SDL_Rect*>(SDL_calloc(static_cast<size_t>(numDisplayModes), sizeof(SDL_Rect)));
	if (!modes)
		Com_Error(ERR_FATAL, "Out of memory");

	for (i = 0; i < numDisplayModes; i++)
	{
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(display, i, &mode) < 0)
			continue;

		if (!mode.w || !mode.h)
		{
			Com_Printf("Display supports any resolution\n");
			SDL_free(modes);
			return true;
		}

		if (windowMode.format != mode.format)
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for (j = 0; j < numModes; j++)
		{
			if (mode.w == modes[j].w && mode.h == modes[j].h)
				break;
		}

		if (j != numModes)
			continue;

		modes[numModes].w = mode.w;
		modes[numModes].h = mode.h;
		numModes++;
	}

	if (numModes > 1)
		qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);

	for (i = 0; i < numModes; i++)
	{
		const char* newModeString = va("%ux%u ", modes[i].w, modes[i].h);

		if (strlen(newModeString) < static_cast<int>(sizeof buf) - strlen(buf))
			Q_strcat(buf, sizeof buf, newModeString);
		else
			Com_Printf("Skipping mode %ux%u, buffer too small\n", modes[i].w, modes[i].h);
	}

	if (*buf)
	{
		buf[strlen(buf) - 1] = 0;
		Com_Printf("Available modes: '%s'\n", buf);
		Cvar_Set("r_availableModes", buf);
	}

	SDL_free(modes);
	return true;
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode(glconfig_t* glConfig, const windowDesc_t* windowDesc, const char* windowTitle,
	const int mode,
	const qboolean fullscreen, const qboolean noborder)
{
	int depthBits;
	Uint32 flags = SDL_WINDOW_SHOWN;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	if (windowDesc->api == GRAPHICS_API_OPENGL)
	{
		flags |= SDL_WINDOW_OPENGL;
	}

	Com_Printf("Initializing display\n");

	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
		(void*)CLIENT_WINDOW_ICON.pixel_data,
		CLIENT_WINDOW_ICON.width,
		CLIENT_WINDOW_ICON.height,
		CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
		CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
		0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
	);

	// If a window exists, note its display index
	if (screen != nullptr)
	{
		display = SDL_GetWindowDisplayIndex(screen);
		if (display < 0)
		{
			Com_DPrintf("SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError());
		}
	}

	if (display >= 0 && SDL_GetDesktopDisplayMode(display, &desktopMode) == 0)
	{
		displayAspect = static_cast<float>(desktopMode.w) / static_cast<float>(desktopMode.h);

		Com_Printf("Display aspect: %.3f\n", displayAspect);
	}
	else
	{
		Com_Memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

		Com_Printf("Cannot determine display aspect, assuming 1.333\n");
	}

	Com_Printf("...setting mode %d:", mode);

	if (mode == -2)
	{
		// use desktop video resolution
		if (desktopMode.h > 0)
		{
			glConfig->vidWidth = desktopMode.w;
			glConfig->vidHeight = desktopMode.h;
		}
		else
		{
			glConfig->vidWidth = 640;
			glConfig->vidHeight = 480;
			Com_Printf("Cannot determine display resolution, assuming 640x480\n");
		}

		//glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if (!R_GetModeInfo(&glConfig->vidWidth, &glConfig->vidHeight, /*&glConfig.windowAspect,*/ mode))
	{
		Com_Printf(" invalid mode\n");
		SDL_FreeSurface(icon);
		return RSERR_INVALID_MODE;
	}
	Com_Printf(" %d %d\n", glConfig->vidWidth, glConfig->vidHeight);

	// Center window
	if (r_centerWindow->integer && !fullscreen)
	{
		x = desktopMode.w / 2 - glConfig->vidWidth / 2;
		y = desktopMode.h / 2 - glConfig->vidHeight / 2;
	}

	// Destroy existing state if it exists
	if (opengl_context != nullptr)
	{
		SDL_GL_DeleteContext(opengl_context);
		opengl_context = nullptr;
	}

	if (screen != nullptr)
	{
		SDL_GetWindowPosition(screen, &x, &y);
		Com_DPrintf("Existing window at %dx%d before being destroyed\n", x, y);
		SDL_DestroyWindow(screen);
		screen = nullptr;
	}

	if (fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig->isFullscreen = qtrue;
	}
	else
	{
		if (noborder)
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig->isFullscreen = qfalse;
	}

	int colorBits = r_colorbits->integer;
	if (!colorBits || colorBits >= 32)
		colorBits = 24;

	if (!r_depthbits->integer)
		depthBits = 24;
	else
		depthBits = r_depthbits->integer;

	int stencilBits = r_stencilbits->integer;
	const int samples = r_ext_multisample->integer;

	if (windowDesc->api == GRAPHICS_API_OPENGL)
	{
		int per_channel_color_bits;
		for (int i = 0; i < 16; i++)
		{
			// 0 - default
			// 1 - minus colorBits
			// 2 - minus depthBits
			// 3 - minus stencil
			if (i % 4 == 0 && i)
			{
				// one pass, reduce
				switch (i / 4)
				{
				case 2:
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1:
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3:
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
				default:;
				}
			}

			int testColorBits = colorBits;
			int testDepthBits = depthBits;
			int testStencilBits = stencilBits;

			if (i % 4 == 3)
			{
				// reduce colorBits
				if (testColorBits == 24)
					testColorBits = 16;
			}

			if (i % 4 == 2)
			{
				// reduce depthBits
				if (testDepthBits == 24)
					testDepthBits = 16;
				else if (testDepthBits == 16)
					testDepthBits = 8;
			}

			if (i % 4 == 1)
			{
				// reduce stencilBits
				if (testStencilBits == 24)
					testStencilBits = 16;
				else if (testStencilBits == 16)
					testStencilBits = 8;
				else
					testStencilBits = 0;
			}

			if (testColorBits == 24)
				per_channel_color_bits = 8;
			else
				per_channel_color_bits = 4;

			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, per_channel_color_bits);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, per_channel_color_bits);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, per_channel_color_bits);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);

			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

			if (windowDesc->gl.majorVersion)
			{
				const int compactVersion = windowDesc->gl.majorVersion * 100 + windowDesc->gl.minorVersion * 10;

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, windowDesc->gl.majorVersion);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, windowDesc->gl.minorVersion);

				if (windowDesc->gl.profile == GLPROFILE_ES || compactVersion >= 320)
				{
					int profile;
					switch (windowDesc->gl.profile)
					{
					default:
					case GLPROFILE_COMPATIBILITY:
						profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
						break;

					case GLPROFILE_CORE:
						profile = SDL_GL_CONTEXT_PROFILE_CORE;
						break;

					case GLPROFILE_ES:
						profile = SDL_GL_CONTEXT_PROFILE_ES;
						break;
					}

					SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
				}
			}

			if (windowDesc->gl.contextFlags & GLCONTEXT_DEBUG)
			{
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
			}

			if (r_stereo->integer)
			{
				glConfig->stereoEnabled = qtrue;
				SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
			}
			else
			{
				glConfig->stereoEnabled = qfalse;
				SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
			}

			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, !r_allowSoftwareGL->integer);

			if ((screen = SDL_CreateWindow(windowTitle, x, y,
				glConfig->vidWidth, glConfig->vidHeight, flags)) == nullptr)
			{
				Com_DPrintf("SDL_CreateWindow failed: %s\n", SDL_GetError());
				continue;
			}

#ifndef MACOS_X
			SDL_SetWindowIcon(screen, icon);
#endif

			if (fullscreen)
			{
				SDL_DisplayMode sdl_display_mode{};

				switch (testColorBits)
				{
				case 16: sdl_display_mode.format = SDL_PIXELFORMAT_RGB565;
					break;
				case 24: sdl_display_mode.format = SDL_PIXELFORMAT_RGB24;
					break;
				default: Com_DPrintf("testColorBits is %d, can't fullscreen\n", testColorBits);
					continue;
				}

				sdl_display_mode.w = glConfig->vidWidth;
				sdl_display_mode.h = glConfig->vidHeight;
				sdl_display_mode.refresh_rate = glConfig->displayFrequency = r_displayRefresh->integer;
				sdl_display_mode.driverdata = nullptr;

				if (SDL_SetWindowDisplayMode(screen, &sdl_display_mode) < 0)
				{
					Com_DPrintf("SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
					continue;
				}
			}

			if ((opengl_context = SDL_GL_CreateContext(screen)) == nullptr)
			{
				Com_Printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
				continue;
			}

			if (SDL_GL_SetSwapInterval(r_swapInterval->integer) == -1)
			{
				Com_DPrintf("SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
			}

			glConfig->colorBits = testColorBits;
			glConfig->depthBits = testDepthBits;
			glConfig->stencilBits = testStencilBits;

			Com_Printf("Using %d color bits, %d depth, %d stencil display.\n",
				glConfig->colorBits, glConfig->depthBits, glConfig->stencilBits);
			break;
		}

		if (opengl_context == nullptr)
		{
			SDL_FreeSurface(icon);
			return RSERR_UNKNOWN;
		}
	}
	else
	{
		// Just create a regular window
		if ((screen = SDL_CreateWindow(windowTitle, x, y,
			glConfig->vidWidth, glConfig->vidHeight, flags)) == nullptr)
		{
			Com_DPrintf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		}
		else
		{
#ifndef MACOS_X
			SDL_SetWindowIcon(screen, icon);
#endif
			if (fullscreen)
			{
				if (SDL_SetWindowDisplayMode(screen, nullptr) < 0)
				{
					Com_DPrintf("SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
				}
			}
		}
	}

	SDL_FreeSurface(icon);

	if (!GLimp_DetectAvailableModes())
	{
		return RSERR_UNKNOWN;
	}

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(glconfig_t* glConfig, const windowDesc_t* windowDesc, const int mode,
	qboolean fullscreen, const qboolean noborder)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if (SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			Com_Printf("SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		const char* driverName = SDL_GetCurrentVideoDriver();

		if (!driverName)
		{
			Com_Error(ERR_FATAL, "No video driver initialized");
		}

		Com_Printf("SDL using driver \"%s\"\n", driverName);
		Cvar_Set("r_sdlDriver", driverName);
	}

	if (SDL_GetNumVideoDisplays() <= 0)
	{
		Com_Error(ERR_FATAL, "SDL_GetNumVideoDisplays() FAILED (%s)", SDL_GetError());
	}

	if (fullscreen && Cvar_VariableIntegerValue("in_nograb"))
	{
		Com_Printf("Fullscreen not allowed with in_nograb 1\n");
		Cvar_Set("r_fullscreen", "0");
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	const rserr_t err = GLimp_SetMode(glConfig, windowDesc, CLIENT_WINDOW_TITLE, mode, fullscreen, noborder);

	switch (err)
	{
	case RSERR_INVALID_FULLSCREEN:
		Com_Printf("...WARNING: fullscreen unavailable in this mode\n");
		return qfalse;
	case RSERR_INVALID_MODE:
		Com_Printf("...WARNING: could not set the given mode (%d)\n", mode);
		return qfalse;
	case RSERR_UNKNOWN:
		Com_Printf("...ERROR: no display modes could be found.\n");
		return qfalse;
	default:
		break;
	}

	return qtrue;
}

window_t WIN_Init(const windowDesc_t* window_desc, glconfig_t* glConfig)
{
	Cmd_AddCommand("modelist", R_ModeList_f);
	Cmd_AddCommand("minimize", GLimp_Minimize);

	r_sdlDriver = Cvar_Get("r_sdlDriver", "", CVAR_ROM);
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);

	// Window cvars
	r_fullscreen = Cvar_Get("r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_noborder = Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_centerWindow = Cvar_Get("r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth = Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight = Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_swapInterval = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE_ND);
	r_stereo = Cvar_Get("r_stereo", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_mode = Cvar_Get("r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH);
	Cvar_CheckRange(r_displayRefresh, 0, 240, qtrue);

	// Window render surface cvars
	r_stencilbits = Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_depthbits = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_colorbits = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE_ND | CVAR_LATCH);
	r_ext_multisample = Cvar_Get("r_ext_multisample", "8", CVAR_ARCHIVE_ND | CVAR_LATCH);
	Cvar_Get("r_availableModes", "", CVAR_ROM);

	// Create the window and set up the context
	if (!GLimp_StartDriverAndSetMode(glConfig, window_desc, r_mode->integer,
		static_cast<qboolean>(r_fullscreen->integer),
		static_cast<qboolean>(r_noborder->integer)))
	{
		if (r_mode->integer != R_MODE_FALLBACK)
		{
			Com_Printf("Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK);

			if (!GLimp_StartDriverAndSetMode(glConfig, window_desc, R_MODE_FALLBACK, qfalse, qfalse))
			{
				// Nothing worked, give up
				Com_Error(ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem");
			}
		}
	}

	glConfig->deviceSupportsGamma =
		static_cast<qboolean>(!r_ignorehwgamma->integer && SDL_SetWindowBrightness(screen, 1.0f) >= 0);

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init(screen);

	// window_t is only really useful for Windows if the renderer wants to create a D3D context.
	window_t window = {};

	window.api = window_desc->api;

#if defined(_WIN32)
	SDL_SysWMinfo info{};
	SDL_VERSION(&info.version);

	if (SDL_GetWindowWMInfo(screen, &info))
	{
		switch (info.subsystem)
		{
		case SDL_SYSWM_WINDOWS:
			window.handle = info.info.win.window;
			break;

		default:
			break;
		}
	}
#endif

	return window;
}

/*
===============
GLimp_Shutdown
===============
*/
void WIN_Shutdown()
{
	Cmd_RemoveCommand("modelist");
	Cmd_RemoveCommand("minimize");

	IN_Shutdown();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	screen = nullptr;
}

void GLimp_EnableLogging(qboolean enable)
{
}

void GLimp_LogComment(char* comment)
{
}

void WIN_SetGamma(const glconfig_t* glConfig, byte red[256], byte green[256], byte blue[256])
{
	Uint16 table[3][256]{};
	int i, j;

	if (!glConfig->deviceSupportsGamma || r_ignorehwgamma->integer > 0)
		return;

	for (i = 0; i < 256; i++)
	{
		table[0][i] = static_cast<Uint16>(red[i]) << 8 | red[i];
		table[1][i] = static_cast<Uint16>(green[i]) << 8 | green[i];
		table[2][i] = static_cast<Uint16>(blue[i]) << 8 | blue[i];
	}

#if defined(_WIN32)
	// Win2K and newer put this odd restriction on gamma ramps...
	{
		OSVERSIONINFO vinfo{};

		vinfo.dwOSVersionInfoSize = sizeof vinfo;
		// ReSharper disable once CppDeprecatedEntity
		GetVersionExA(&vinfo);
		if (vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			Com_DPrintf("performing gamma clamp.\n");
			for (j = 0; j < 3; j++)
			{
				for (i = 0; i < 128; i++)
				{
					table[j][i] = Q_min(table[j][i], (128 + i) << 8);
				}

				table[j][127] = Q_min(table[j][127], 254 << 8);
			}
		}
	}
#endif

	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i - 1])
				table[j][i] = table[j][i - 1];
		}
	}

	if (SDL_SetWindowGammaRamp(screen, table[0], table[1], table[2]) < 0)
	{
		Com_DPrintf("SDL_SetWindowGammaRamp() failed: %s\n", SDL_GetError());
	}
}

void* WIN_GL_GetProcAddress(const char* proc)
{
	return SDL_GL_GetProcAddress(proc);
}

qboolean WIN_GL_ExtensionSupported(const char* extension)
{
	return SDL_GL_ExtensionSupported(extension) == SDL_TRUE ? qtrue : qfalse;
}