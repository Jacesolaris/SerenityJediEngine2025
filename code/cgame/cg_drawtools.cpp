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

#include "cg_headers.h"

#include "cg_media.h"

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(const float x, const float y, const float w, const float h, const float size)
{
	//size *= cgs.screenXScale;
	cgi_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
	cgi_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
}

void CG_DrawTopBottom(const float x, const float y, const float w, const float h, const float size)
{
	//size *= cgs.screenYScale;
	cgi_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
	cgi_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

/*
================
CG_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect(const float x, const float y, const float width, const float height, const float size,
	const float* color)
{
	cgi_R_SetColor(color);

	CG_DrawTopBottom(x, y, width, height, size);
	CG_DrawSides(x, y, width, height, size);

	cgi_R_SetColor(nullptr);
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect(const float x, const float y, const float width, const float height, const float* color)
{
	cgi_R_SetColor(color);
	cgi_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader);
	cgi_R_SetColor(nullptr);
}

/*
================
CG_Scissor

Coordinates are 640*480 virtual values
=================
*/
void CG_Scissor(const float x, const float y, const float width, const float height)
{
	cgi_R_Scissor(x, y, width, height);
}

/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
=================
*/
void CG_DrawPic(const float x, const float y, const float width, const float height, const qhandle_t hShader)
{
	cgi_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/*
================
CG_DrawPic2

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
Can also specify the exact texture coordinates
=================
*/
void CG_DrawPic2(const float x, const float y, const float width, const float height, const float s1, const float t1,
	const float s2, const float t2, const qhandle_t hShader)
{
	cgi_R_DrawStretchPic(x, y, width, height, s1, t1, s2, t2, hShader);
}

/*
================
CG_DrawRotatePic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
rotates around the upper right corner of the passed in point
=================
*/
void CG_DrawRotatePic(const float x, const float y, const float width, const float height, const float angle,
	const qhandle_t hShader,
	const float aspectCorrection)
{
	cgi_R_DrawRotatePic(x, y, width, height, 0, 0, 1, 1, angle, hShader, aspectCorrection);
}

/*
================
CG_DrawRotatePic2

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
Actually rotates around the center point of the passed in coordinates
=================
*/
void CG_DrawRotatePic2(const float x, const float y, const float width, const float height, const float angle,
	const qhandle_t hShader,
	const float aspectCorrection)
{
	cgi_R_DrawRotatePic2(x, y, width, height, 0, 0, 1, 1, angle, hShader, aspectCorrection);
}

/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
static void CG_DrawChar(const int x, const int y, const int width, const int height, int ch)
{
	ch &= 255;

	if (ch == ' ')
	{
		return;
	}

	const float ax = x;
	const float ay = y;
	const float aw = width;
	const float ah = height;

	const int row = ch >> 4;
	const int col = ch & 15;
	/*
		frow = row*0.0625;
		fcol = col*0.0625;
		size = 0.0625;

		cgi_R_DrawStretchPic( ax, ay, aw, ah,
						   fcol, frow,
						   fcol + size, frow + size,
						   cgs.media.charsetShader );
	*/

	const float frow = row * 0.0625;
	const float fcol = col * 0.0625;
	constexpr float size = 0.03125;
	constexpr float size2 = 0.0625;

	cgi_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size2,
		cgs.media.charsetShader);
}

/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt(const int x, const int y, const char* string, const float* setColor,
	const qboolean forceColor, const qboolean shadow, const int charWidth, const int charHeight)
{
	vec4_t color{};
	const char* s;
	int xx;

	// draw the drop shadow
	if (shadow)
	{
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		cgi_R_SetColor(color);
		s = string;
		xx = x;
		while (*s)
		{
			if (Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			CG_DrawChar(xx + 2, y + 2, charWidth, charHeight, *s);
			xx += charWidth;
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cgi_R_SetColor(setColor);
	while (*s)
	{
		if (Q_IsColorString(s))
		{
			if (!forceColor)
			{
				memcpy(color, g_color_table[ColorIndex(*(s + 1))], sizeof color);
				color[3] = setColor[3];
				cgi_R_SetColor(color);
			}
			s += 2;
			continue;
		}
		CG_DrawChar(xx, y, charWidth, charHeight, *s);
		xx += charWidth;
		s++;
	}
	cgi_R_SetColor(nullptr);
}

void CG_DrawSmallStringColor(const int x, const int y, const char* s, vec4_t color)
{
	CG_DrawStringExt(x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT);
}

void CG_DrawBigStringColor(const int x, const int y, const char* s, vec4_t color)
{
	CG_DrawStringExt(x, y, s, color, qtrue, qtrue, MINUTE_WIDTH, MINUTE_HEIGHT);
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen(const char* str)
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
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox(const int x, const int y, const int w, const int h, const qhandle_t hShader)
{
	const float s1 = x / 64.0;
	const float t1 = y / 64.0;
	const float s2 = (x + w) / 64.0;
	const float t2 = (y + h) / 64.0;
	cgi_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
}

/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear()
{
	const int w = cgs.glconfig.vidWidth;
	const int h = cgs.glconfig.vidHeight;

	if (cg.refdef.x == 0 && cg.refdef.y == 0 &&
		cg.refdef.width == w && cg.refdef.height == h)
	{
		return; // full screen rendering
	}

	const int top = cg.refdef.y;
	const int bottom = top + cg.refdef.height - 1;
	const int left = cg.refdef.x;
	const int right = left + cg.refdef.width - 1;

	// clear above view screen
	CG_TileClearBox(0, 0, w, top, cgs.media.backTileShader);

	// clear below view screen
	CG_TileClearBox(0, bottom, w, h - bottom, cgs.media.backTileShader);

	// clear left of view screen
	CG_TileClearBox(0, top, left, bottom - top + 1, cgs.media.backTileShader);

	// clear right of view screen
	CG_TileClearBox(right, top, w - right, bottom - top + 1, cgs.media.backTileShader);
}

/*
================
CG_FadeColor
================
*/
float* CG_FadeColor(const int startMsec, const int totalMsec)
{
	static vec4_t color;

	if (startMsec == 0)
	{
		return nullptr;
	}

	const int t = cg.time - startMsec;

	if (t >= totalMsec)
	{
		return nullptr;
	}

	// fade out
	if (totalMsec - t < FADE_TIME)
	{
		color[3] = (totalMsec - t) * 1.0 / FADE_TIME;
	}
	else
	{
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}

/*
==============
CG_DrawNumField

Take x,y positions as if 640 x 480 and scales them to the proper resolution

==============
*/
void CG_DrawNumField(int x, const int y, int width, int value, const int charWidth, const int charHeight,
	const int style, const qboolean zeroFill)
{
	char num[16];
	int frame;
	int xWidth;

	if (width < 1)
	{
		return;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	default:;
	}

	Com_sprintf(num, sizeof num, "%i", value);
	int l = strlen(num);
	if (l > width)
		l = width;

	// FIXME: Might need to do something different for the chunky font??
	switch (style)
	{
	case NUM_FONT_SMALL:
		xWidth = charWidth;
		break;
	case NUM_FONT_CHUNKY:
		xWidth = charWidth / 1.2f + 2;
		break;
	default:
	case NUM_FONT_BIG:
		xWidth = charWidth / 2 + 7; //(charWidth/6);
		break;
	}

	if (zeroFill)
	{
		for (int i = 0; i < width - l; i++)
		{
			switch (style)
			{
			case NUM_FONT_SMALL:
				CG_DrawPic(x, y, charWidth, charHeight, cgs.media.smallnumberShaders[0]);
				break;
			case NUM_FONT_CHUNKY:
				CG_DrawPic(x, y, charWidth, charHeight, cgs.media.chunkyNumberShaders[0]);
				break;
			default:
			case NUM_FONT_BIG:
				CG_DrawPic(x, y, charWidth, charHeight, cgs.media.numberShaders[0]);
				break;
			}
			x += 2 + xWidth;
		}
	}
	else
	{
		x += 2 + xWidth * (width - l);
	}

	char* ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		switch (style)
		{
		case NUM_FONT_SMALL:
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.smallnumberShaders[frame]);
			x++; // For a one line gap
			break;
		case NUM_FONT_CHUNKY:
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.chunkyNumberShaders[frame]);
			break;
		default:
		case NUM_FONT_BIG:
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.numberShaders[frame]);
			break;
		}

		x += xWidth;
		ptr++;
		l--;
	}
}

/*
=================
CG_DrawProportionalString
=================
*/
void CG_DrawProportionalString(const int x, const int y, const char* str, int style, vec4_t color)
{
	//assert(!style);//call this directly if you need style (OR it into the font handle)
	cgi_R_Font_DrawString(x, y, str, color, cgs.media.qhFontMedium, -1, 1.0f);
}