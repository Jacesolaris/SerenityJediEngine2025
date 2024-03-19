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

// cl.input.c  -- builds an intended movement command to send to the server

#include "../server/exe_headers.h"

#include "client.h"
#include "client_ui.h"

#ifndef _WIN32
#include <cmath>
#endif

unsigned frame_msec;
int old_com_frameTime;
float cl_mPitchOverride = 0.0f;
float cl_mYawOverride = 0.0f;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

kbutton_t in_left, in_right, in_forward, in_back;
kbutton_t in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t in_strafe, in_speed;
kbutton_t in_up, in_down;

kbutton_t in_buttons[32];

qboolean in_mlooking;

extern cvar_t* in_joystick;

extern cvar_t* j_pitch;
extern cvar_t* j_yaw;
extern cvar_t* j_forward;
extern cvar_t* j_side;
extern cvar_t* j_up;
extern cvar_t* j_pitch_axis;
extern cvar_t* j_yaw_axis;
extern cvar_t* j_forward_axis;
extern cvar_t* j_side_axis;
extern cvar_t* j_up_axis;
extern cvar_t* j_sensitivity;

static void IN_UseGivenForce()
{
	const char* c = Cmd_Argv(1);
	int forceNum;
	int genCmdNum = 0;

	if (c)
	{
		forceNum = atoi(c);
	}
	else
	{
		return;
	}

	switch (forceNum)
	{
	case FP_DRAIN:
		genCmdNum = GENCMD_FORCE_DRAIN;
		break;
	case FP_PUSH:
		genCmdNum = GENCMD_FORCE_THROW;
		break;
	case FP_SPEED:
		genCmdNum = GENCMD_FORCE_SPEED;
		break;
	case FP_PULL:
		genCmdNum = GENCMD_FORCE_PULL;
		break;
	case FP_TELEPATHY:
		genCmdNum = GENCMD_FORCE_DISTRACT;
		break;
	case FP_GRIP:
		genCmdNum = GENCMD_FORCE_GRIP;
		break;
	case FP_LIGHTNING:
		genCmdNum = GENCMD_FORCE_LIGHTNING;
		break;
	case FP_RAGE:
		genCmdNum = GENCMD_FORCE_RAGE;
		break;
	case FP_PROTECT:
		genCmdNum = GENCMD_FORCE_PROTECT;
		break;
	case FP_ABSORB:
		genCmdNum = GENCMD_FORCE_ABSORB;
		break;
	case FP_SEE:
		genCmdNum = GENCMD_FORCE_SEEING;
		break;
	case FP_HEAL:
		genCmdNum = GENCMD_FORCE_HEAL;
		break;
	case FP_STASIS:
		genCmdNum = GENCMD_FORCE_STASIS;
		break;
	case FP_DESTRUCTION:
		genCmdNum = GENCMD_FORCE_DESTRUCTION;
		break;
	case FP_GRASP:
		genCmdNum = GENCMD_FORCE_GRASP;
		break;
	case FP_REPULSE:
		genCmdNum = GENCMD_FORCE_REPULSE;
		break;
	case FP_LIGHTNING_STRIKE:
		genCmdNum = GENCMD_FORCE_LIGHTNING_STRIKE;
		break;
	case FP_BLAST:
		genCmdNum = GENCMD_FORCE_BLAST;
		break;
	case FP_FEAR:
		genCmdNum = GENCMD_FORCE_FEAR;
		break;
	case FP_DEADLYSIGHT:
		genCmdNum = GENCMD_FORCE_DEADLYSIGHT;
		break;
	case FP_INSANITY:
		genCmdNum = GENCMD_FORCE_INSANITY;
		break;
	case FP_BLINDING:
		genCmdNum = GENCMD_FORCE_BLINDING;
		break;
	default:
		assert(0);
		break;
	}

	if (genCmdNum)
	{
		cl.gcmdSendValue = qtrue;
		cl.gcmdValue = genCmdNum;
	}
}

static void IN_MLookDown()
{
	in_mlooking = qtrue;
}

void IN_CenterView();

static void IN_MLookUp()
{
	in_mlooking = qfalse;
	if (!cl_freelook->integer)
	{
		IN_CenterView();
	}
}

static void IN_KeyDown(kbutton_t* b)
{
	int k;

	const char* c = Cmd_Argv(1);
	if (c[0])
	{
		k = atoi(c);
	}
	else
	{
		k = -1; // typed manually at the console for continuous down
	}

	if (k == b->down[0] || k == b->down[1])
	{
		return; // repeating key
	}

	if (!b->down[0])
	{
		b->down[0] = k;
	}
	else if (!b->down[1])
	{
		b->down[1] = k;
	}
	else
	{
		Com_Printf("Three keys down for a button!\n");
		return;
	}

	if (b->active)
	{
		return; // still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

static void IN_KeyUp(kbutton_t* b)
{
	int k;

	const char* c = Cmd_Argv(1);
	if (c[0])
	{
		k = atoi(c);
	}
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if (b->down[0] == k)
	{
		b->down[0] = 0;
	}
	else if (b->down[1] == k)
	{
		b->down[1] = 0;
	}
	else
	{
		return; // key up without coresponding down (menu pass through)
	}
	if (b->down[0] || b->down[1])
	{
		return; // some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	const unsigned uptime = atoi(c);
	if (uptime)
	{
		b->msec += uptime - b->downtime;
	}
	else
	{
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState(kbutton_t* key)
{
	float val;

	int msec = key->msec;
	key->msec = 0;

	if (key->active)
	{
		// still down
		if (!key->downtime)
		{
			msec = com_frameTime;
		}
		else
		{
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf("%i ", msec);
	}
#endif

	val = static_cast<float>(msec) / frame_msec;
	if (val < 0)
	{
		val = 0;
	}
	if (val > 1)
	{
		val = 1;
	}

	return val;
}

static void IN_UpDown() { IN_KeyDown(&in_up); }
static void IN_UpUp() { IN_KeyUp(&in_up); }
static void IN_DownDown() { IN_KeyDown(&in_down); }
static void IN_DownUp() { IN_KeyUp(&in_down); }
static void IN_LeftDown() { IN_KeyDown(&in_left); }
static void IN_LeftUp() { IN_KeyUp(&in_left); }
static void IN_RightDown() { IN_KeyDown(&in_right); }
static void IN_RightUp() { IN_KeyUp(&in_right); }
static void IN_ForwardDown() { IN_KeyDown(&in_forward); }
static void IN_ForwardUp() { IN_KeyUp(&in_forward); }
static void IN_BackDown() { IN_KeyDown(&in_back); }
static void IN_BackUp() { IN_KeyUp(&in_back); }
static void IN_LookupDown() { IN_KeyDown(&in_lookup); }
static void IN_LookupUp() { IN_KeyUp(&in_lookup); }
static void IN_LookdownDown() { IN_KeyDown(&in_lookdown); }
static void IN_LookdownUp() { IN_KeyUp(&in_lookdown); }
static void IN_MoveleftDown() { IN_KeyDown(&in_moveleft); }
static void IN_MoveleftUp() { IN_KeyUp(&in_moveleft); }
static void IN_MoverightDown() { IN_KeyDown(&in_moveright); }
static void IN_MoverightUp() { IN_KeyUp(&in_moveright); }

static void IN_SpeedDown() { IN_KeyDown(&in_speed); }
static void IN_SpeedUp() { IN_KeyUp(&in_speed); }
static void IN_StrafeDown() { IN_KeyDown(&in_strafe); }
static void IN_StrafeUp() { IN_KeyUp(&in_strafe); }

static void IN_Button0Down() { IN_KeyDown(&in_buttons[0]); }
static void IN_Button0Up() { IN_KeyUp(&in_buttons[0]); }
static void IN_Button1Down() { IN_KeyDown(&in_buttons[1]); }
static void IN_Button1Up() { IN_KeyUp(&in_buttons[1]); }
static void IN_Button2Down() { IN_KeyDown(&in_buttons[2]); }
static void IN_Button2Up() { IN_KeyUp(&in_buttons[2]); }
static void IN_Button3Down() { IN_KeyDown(&in_buttons[3]); }
static void IN_Button3Up() { IN_KeyUp(&in_buttons[3]); }
static void IN_Button4Down() { IN_KeyDown(&in_buttons[4]); }
static void IN_Button4Up() { IN_KeyUp(&in_buttons[4]); }
static void IN_Button5Down() { IN_KeyDown(&in_buttons[5]); }
static void IN_Button5Up() { IN_KeyUp(&in_buttons[5]); }
void IN_Button6Down() { IN_KeyDown(&in_buttons[6]); }
void IN_Button6Up() { IN_KeyUp(&in_buttons[6]); }
static void IN_Button7Down() { IN_KeyDown(&in_buttons[7]); }
static void IN_Button7Up() { IN_KeyUp(&in_buttons[7]); }
static void IN_Button8Down() { IN_KeyDown(&in_buttons[8]); }
static void IN_Button8Up() { IN_KeyUp(&in_buttons[8]); }
static void IN_Button9Down() { IN_KeyDown(&in_buttons[9]); }
static void IN_Button9Up() { IN_KeyUp(&in_buttons[9]); }
void IN_Button10Down() { IN_KeyDown(&in_buttons[10]); }
void IN_Button10Up() { IN_KeyUp(&in_buttons[10]); }
void IN_Button11Down() { IN_KeyDown(&in_buttons[11]); }
void IN_Button11Up() { IN_KeyUp(&in_buttons[11]); }
static void IN_Button12Down() { IN_KeyDown(&in_buttons[12]); }
static void IN_Button12Up() { IN_KeyUp(&in_buttons[12]); }
static void IN_Button13Down() { IN_KeyDown(&in_buttons[13]); }
static void IN_Button13Up() { IN_KeyUp(&in_buttons[13]); }
static void IN_Button14Down() { IN_KeyDown(&in_buttons[14]); }
static void IN_Button14Up() { IN_KeyUp(&in_buttons[14]); }
static void IN_Button15Down() { IN_KeyDown(&in_buttons[15]); }
static void IN_Button15Up() { IN_KeyUp(&in_buttons[15]); }

static void IN_Button16Down() { IN_KeyDown(&in_buttons[16]); }
static void IN_Button16Up() { IN_KeyUp(&in_buttons[16]); }
static void IN_Button17Down() { IN_KeyDown(&in_buttons[17]); }
static void IN_Button17Up() { IN_KeyUp(&in_buttons[17]); }
static void IN_Button18Down() { IN_KeyDown(&in_buttons[18]); }
static void IN_Button18Up() { IN_KeyUp(&in_buttons[18]); }
static void IN_Button19Down() { IN_KeyDown(&in_buttons[19]); }
static void IN_Button19Up() { IN_KeyUp(&in_buttons[19]); }
static void IN_Button20Down() { IN_KeyDown(&in_buttons[20]); }
static void IN_Button20Up() { IN_KeyUp(&in_buttons[20]); }

void IN_CenterView()
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.frame.ps.delta_angles[PITCH]);
}

//==========================================================================

cvar_t* cl_upspeed;
cvar_t* cl_forwardspeed;
cvar_t* cl_sidespeed;

cvar_t* cl_yawspeed;
cvar_t* cl_pitchspeed;

cvar_t* cl_run;

cvar_t* cl_anglespeedkey;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles()
{
	float speed;

	if (in_speed.active)
	{
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	}
	else
	{
		speed = 0.001 * cls.frametime;
	}

	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			cl.viewangles[YAW] -= cl_mYawOverride * 5.0f * speed * cl_yawspeed->value * CL_KeyState(&in_right);
			cl.viewangles[YAW] += cl_mYawOverride * 5.0f * speed * cl_yawspeed->value * CL_KeyState(&in_left);
		}
		else
		{
			cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState(&in_right);
			cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState(&in_left);
		}
	}

	if (cl_mPitchOverride)
	{
		cl.viewangles[PITCH] -= cl_mPitchOverride * 5.0f * speed * cl_pitchspeed->value * CL_KeyState(&in_lookup);
		cl.viewangles[PITCH] += cl_mPitchOverride * 5.0f * speed * cl_pitchspeed->value * CL_KeyState(&in_lookdown);
	}
	else
	{
		cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState(&in_lookup);
		cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState(&in_lookdown);
	}
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
static void CL_KeyMove(usercmd_t* cmd)
{
	int movespeed;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if (in_speed.active ^ cl_run->integer)
	{
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	}
	else
	{
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	int forward = 0;
	int side = 0;
	int up = 0;
	if (in_strafe.active)
	{
		side += movespeed * CL_KeyState(&in_right);
		side -= movespeed * CL_KeyState(&in_left);
	}

	side += movespeed * CL_KeyState(&in_moveright);
	side -= movespeed * CL_KeyState(&in_moveleft);

	up += movespeed * CL_KeyState(&in_up);
	up -= movespeed * CL_KeyState(&in_down);

	forward += movespeed * CL_KeyState(&in_forward);
	forward -= movespeed * CL_KeyState(&in_back);

	cmd->forwardmove = ClampChar(forward);
	cmd->rightmove = ClampChar(side);
	cmd->upmove = ClampChar(up);
}

void _UI_MouseEvent(int dx, int dy);

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent(const int dx, const int dy, int time)
{
	if (Key_GetCatcher() & KEYCATCH_UI)
	{
		_UI_MouseEvent(dx, dy);
	}
	else
	{
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent(const int axis, const int value, int time)
{
	if (axis < 0 || axis >= MAX_JOYSTICK_AXIS)
	{
		Com_Error(ERR_DROP, "CL_JoystickEvent: bad axis %i", axis);
	}

	if (Key_GetCatcher() & (KEYCATCH_UI))
	{
		return;
	}
	cl.joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/

static void CL_JoystickMove(usercmd_t* cmd)
{
	float	anglespeed;

	if (!in_joystick->integer)
	{
		return;
	}

	float yaw = j_yaw->value * cl.joystickAxis[j_yaw_axis->integer];
	float right = j_side->value * cl.joystickAxis[j_side_axis->integer];
	float forward = j_forward->value * cl.joystickAxis[j_forward_axis->integer];
	float pitch = j_pitch->value * cl.joystickAxis[j_pitch_axis->integer];
	float up = j_up->value * cl.joystickAxis[j_up_axis->integer];

	if (!(in_speed.active ^ cl_run->integer))
	{
		cmd->buttons |= BUTTON_WALKING;
	}
	else if (pitch <= 180 && pitch >= -180 && pitch != 0)
	{
		cmd->buttons |= BUTTON_WALKING;
	}

	if (in_speed.active)
	{
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	}
	else
	{
		anglespeed = 0.001 * cls.frametime;
	}

	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			cl.viewangles[YAW] += 5.0f * cl_mYawOverride * yaw;
		}
		else
		{
			cl.viewangles[YAW] += anglespeed * (yaw * j_sensitivity->value);
		}
		cmd->rightmove = ClampCharMove(cmd->rightmove + (int)right);
	}
	else
	{
		cl.viewangles[YAW] += anglespeed * right;
		cmd->rightmove = ClampCharMove(cmd->rightmove + (int)yaw);
	}

	if (in_mlooking || cl_freelook->integer)
	{
		if (cl_mPitchOverride)
		{
			cl.viewangles[PITCH] += 5.0f * cl_mPitchOverride * forward;
		}
		else
		{
			cl.viewangles[PITCH] -= anglespeed * ((forward / 11.38) * j_sensitivity->value);
		}
		cmd->forwardmove = ClampCharMove(cmd->forwardmove - (int)pitch);
	}
	else
	{
		cl.viewangles[PITCH] += anglespeed * pitch;
		cmd->forwardmove = ClampCharMove(cmd->forwardmove + (int)forward);
	}

	cmd->upmove = ClampCharMove(cmd->upmove + (int)up);

#if 0
	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			cl.viewangles[YAW] += 5.0f * cl_mYawOverride * cl.joystickAxis[AXIS_SIDE];
		}
		else
		{
			cl.viewangles[YAW] += anglespeed * (cl_yawspeed->value / 100.0f) * cl.joystickAxis[AXIS_SIDE];
		}
	}
	else
	{
		cmd->rightmove = ClampChar(cmd->rightmove + cl.joystickAxis[AXIS_SIDE]);
	}

	if (in_mlooking)
	{
		if (cl_mPitchOverride)
		{
			cl.viewangles[PITCH] += 5.0f * cl_mPitchOverride * cl.joystickAxis[AXIS_FORWARD];
		}
		else
		{
			cl.viewangles[PITCH] += anglespeed * (cl_pitchspeed->value / 100.0f) * cl.joystickAxis[AXIS_FORWARD];
		}
	}
	else
	{
		cmd->forwardmove = ClampChar(cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD]);
	}

	cmd->upmove = ClampChar(cmd->upmove + cl.joystickAxis[AXIS_UP]);
#endif
}

/*
=================
CL_MouseMove
=================
*/
static void CL_MouseMove(usercmd_t* cmd)
{
	float mx, my;
	const float speed = static_cast<float>(frame_msec);
	const float pitch = m_pitch->value;

	// allow mouse smoothing
	if (m_filter->integer)
	{
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5;
	}
	else
	{
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}

	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	const float rate = SQRTFAST(mx * mx + my * my) / speed;
	float accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	// scale by FOV
	accelSensitivity *= cl.cgameSensitivity;

	if (rate && cl_showMouseRate->integer)
	{
		Com_Printf("%f : %f\n", rate, accelSensitivity);
	}

	mx *= accelSensitivity;
	my *= accelSensitivity;

	if (!mx && !my)
	{
		return;
	}

	// add mouse X/Y movement to cmd
	if (in_strafe.active)
	{
		cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
	}
	else
	{
		if (cl_mYawOverride)
		{
			cl.viewangles[YAW] -= cl_mYawOverride * mx;
		}
		else
		{
			cl.viewangles[YAW] -= m_yaw->value * mx;
		}
	}

	if ((in_mlooking || cl_freelook->integer) && !in_strafe.active)
	{
		// VVFIXME - This is supposed to be a CVAR
		constexpr float cl_pitch_sensitivity = 1.0f;
		if (cl_mPitchOverride)
		{
			if (pitch > 0)
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * my * cl_pitch_sensitivity;
			}
			else
			{
				cl.viewangles[PITCH] -= cl_mPitchOverride * my * cl_pitch_sensitivity;
			}
		}
		else
		{
			cl.viewangles[PITCH] += pitch * my * cl_pitch_sensitivity;
		}
	}
	else
	{
		cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward->value * my);
	}
}

/*
==============
CL_CmdButtons
==============
*/
static void CL_CmdButtons(usercmd_t* cmd)
{
	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for (int i = 0; i < 32; i++)
	{
		if (in_buttons[i].active || in_buttons[i].wasPressed)
		{
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if (Key_GetCatcher())
	{
		//cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	/*
	if ( kg.anykeydown && Key_GetCatcher( ) == 0 ) {
		cmd->buttons |= BUTTON_ANY;
	}
	*/
}

/*
==============
CL_FinishMove
==============
*/
static void CL_FinishMove(usercmd_t* cmd)
{
	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;

	if (cl.gcmdSendValue)
	{
		cmd->generic_cmd = cl.gcmdValue;
		cl.gcmdSendValue = qfalse;
	}
	else
	{
		cmd->generic_cmd = 0;
	}

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for (int i = 0; i < 3; i++)
	{
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
	}
}

/*
=================
CL_CreateCmd
=================
*/
vec3_t cl_overriddenAngles = { 0, 0, 0 };
qboolean cl_overrideAngles = qfalse;

static usercmd_t CL_CreateCmd()
{
	usercmd_t cmd;
	vec3_t oldAngles;

	VectorCopy(cl.viewangles, oldAngles);

	// keyboard angle adjustment
	CL_AdjustAngles();

	memset(&cmd, 0, sizeof cmd);

	CL_CmdButtons(&cmd);

	// get basic movement from keyboard
	CL_KeyMove(&cmd);

	// get basic movement from mouse
	CL_MouseMove(&cmd);

	// get basic movement from joystick
	CL_JoystickMove(&cmd);

	// check to make sure the angles haven't wrapped
	if (cl.viewangles[PITCH] - oldAngles[PITCH] > 90)
	{
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	}
	else if (oldAngles[PITCH] - cl.viewangles[PITCH] > 90)
	{
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	if (cl_overrideAngles)
	{
		VectorCopy(cl_overriddenAngles, cl.viewangles);
		cl_overrideAngles = qfalse;
	}
	// store out the final values
	CL_FinishMove(&cmd);

	// draw debug graphs of turning for mouse testing
	if (cl_debugMove->integer)
	{
		if (cl_debugMove->integer == 1)
		{
			SCR_DebugGraph(abs(cl.viewangles[YAW] - oldAngles[YAW]), 0);
		}
		if (cl_debugMove->integer == 2)
		{
			SCR_DebugGraph(abs(cl.viewangles[PITCH] - oldAngles[PITCH]), 0);
		}
	}

	return cmd;
}

/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
static void CL_CreateNewCommands()
{
	// no need to create usercmds until we have a gamestate
	//	if ( cls.state < CA_PRIMED )
	//		return;

	frame_msec = com_frameTime - old_com_frameTime;

	// if running over 1000fps, act as if each frame is 1ms
	// prevents divisions by zero
	if (frame_msec < 1)
		frame_msec = 1;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if (frame_msec > 200)
		frame_msec = 200;

	old_com_frameTime = com_frameTime;

	// generate a command for this frame
	cl.cmdNumber++;
	const int cmdNum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdNum] = CL_CreateCmd();
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
static qboolean CL_ReadyToSendPacket()
{
	// don't send anything if playing back a demo
	//	if ( cls.state == CA_CINEMATIC )
	if (cls.state == CA_CINEMATIC || CL_IsRunningInGameCinematic())
	{
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if (cls.state != CA_ACTIVE && cls.state != CA_PRIMED
		&& cls.realtime - clc.lastPacketSentTime < 1000)
	{
		return qfalse;
	}

	// send every frame for loopbacks
	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket()
{
	msg_t buf;
	byte data[MAX_MSGLEN];
	int i;
	usercmd_t nullcmd;

	// don't send anything if playing back a demo
	//	if ( cls.state == CA_CINEMATIC )
	if (cls.state == CA_CINEMATIC || CL_IsRunningInGameCinematic())
	{
		return;
	}

	MSG_Init(&buf, data, sizeof data);

	// write any unacknowledged clientCommands
	for (i = clc.reliableAcknowledge + 1; i <= clc.reliableSequence; i++)
	{
		MSG_WriteByte(&buf, clc_clientCommand);
		MSG_WriteLong(&buf, i);
		MSG_WriteString(&buf, clc.reliableCommands[i & MAX_RELIABLE_COMMANDS - 1]);
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if (cl_packetdup->integer < 0)
	{
		Cvar_Set("cl_packetdup", "0");
	}
	else if (cl_packetdup->integer > 5)
	{
		Cvar_Set("cl_packetdup", "5");
	}
	const int oldPacketNum = clc.netchan.outgoingSequence - 1 - cl_packetdup->integer & PACKET_MASK;
	int count = cl.cmdNumber - cl.packetCmdNumber[oldPacketNum];
	if (count > MAX_PACKET_USERCMDS)
	{
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if (count >= 1)
	{
		// begin a client move command
		MSG_WriteByte(&buf, clc_move);

		// write the last reliable message we received
		MSG_WriteLong(&buf, clc.serverCommandSequence);

		// write the current serverId so the server
		// can tell if this is from the current gameState
		MSG_WriteLong(&buf, cl.serverId);

		// write the current time
		MSG_WriteLong(&buf, cls.realtime);

		// let the server know what the last messagenum we
		// got was, so the next message can be delta compressed
		// FIXME: this could just be a bit flag, with the message implicit
		// from the unreliable ack of the netchan
		if (cl_nodelta->integer || !cl.frame.valid)
		{
			MSG_WriteLong(&buf, -1); // no compression
		}
		else
		{
			MSG_WriteLong(&buf, cl.frame.messageNum);
		}

		// write the cmdNumber so the server can determine which ones it
		// has already received
		MSG_WriteLong(&buf, cl.cmdNumber);

		// write the command count
		MSG_WriteByte(&buf, count);

		// write all the commands, including the predicted command
		memset(&nullcmd, 0, sizeof nullcmd);
		const usercmd_t* oldcmd = &nullcmd;
		for (i = 0; i < count; i++)
		{
			const int j = cl.cmdNumber - count + i + 1 & CMD_MASK;
			usercmd_t* cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmd(&buf, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	const int packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.packetTime[packetNum] = cls.realtime;
	cl.packetCmdNumber[packetNum] = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;
	Netchan_Transmit(&clc.netchan, buf.cursize, buf.data);
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd()
{
	// don't send any message if not connected
	if (cls.state < CA_CONNECTED)
	{
		return;
	}

	// don't send commands if paused
	if (com_sv_running->integer && sv_paused->integer && cl_paused->integer)
	{
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if (!CL_ReadyToSendPacket())
	{
		return;
	}

	CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput()
{
	Cmd_AddCommand("centerview", IN_CenterView);

	Cmd_AddCommand("+moveup", IN_UpDown);
	Cmd_AddCommand("-moveup", IN_UpUp);
	Cmd_AddCommand("+movedown", IN_DownDown);
	Cmd_AddCommand("-movedown", IN_DownUp);
	Cmd_AddCommand("+left", IN_LeftDown);
	Cmd_AddCommand("-left", IN_LeftUp);
	Cmd_AddCommand("+right", IN_RightDown);
	Cmd_AddCommand("-right", IN_RightUp);
	Cmd_AddCommand("+forward", IN_ForwardDown);
	Cmd_AddCommand("-forward", IN_ForwardUp);
	Cmd_AddCommand("+back", IN_BackDown);
	Cmd_AddCommand("-back", IN_BackUp);
	Cmd_AddCommand("+lookup", IN_LookupDown);
	Cmd_AddCommand("-lookup", IN_LookupUp);
	Cmd_AddCommand("+lookdown", IN_LookdownDown);
	Cmd_AddCommand("-lookdown", IN_LookdownUp);
	Cmd_AddCommand("+strafe", IN_StrafeDown);
	Cmd_AddCommand("-strafe", IN_StrafeUp);
	Cmd_AddCommand("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand("+moveright", IN_MoverightDown);
	Cmd_AddCommand("-moveright", IN_MoverightUp);
	Cmd_AddCommand("+speed", IN_SpeedDown);
	Cmd_AddCommand("-speed", IN_SpeedUp);
	Cmd_AddCommand("useGivenForce", IN_UseGivenForce);
	//buttons
	Cmd_AddCommand("+attack", IN_Button0Down); //attack
	Cmd_AddCommand("-attack", IN_Button0Up);
	Cmd_AddCommand("+force_lightning", IN_Button1Down); //force lightning
	Cmd_AddCommand("-force_lightning", IN_Button1Up);
	Cmd_AddCommand("+useforce", IN_Button2Down); //use current force power
	Cmd_AddCommand("-useforce", IN_Button2Up);
#ifdef JK2_MODE
	Cmd_AddCommand("+block", IN_Button3Down);//manual blocking
	Cmd_AddCommand("-block", IN_Button3Up);
#else
	Cmd_AddCommand("+force_drain", IN_Button3Down); //force drain
	Cmd_AddCommand("-force_drain", IN_Button3Up);
#endif
	Cmd_AddCommand("+walk", IN_Button4Down); //walking
	Cmd_AddCommand("-walk", IN_Button4Up);
	Cmd_AddCommand("+use", IN_Button5Down); //use object
	Cmd_AddCommand("-use", IN_Button5Up);
	Cmd_AddCommand("+force_grip", IN_Button6Down); //force jump
	Cmd_AddCommand("-force_grip", IN_Button6Up);
	Cmd_AddCommand("+altattack", IN_Button7Down); //altattack
	Cmd_AddCommand("-altattack", IN_Button7Up);
#ifndef JK2_MODE
	Cmd_AddCommand("+forcefocus", IN_Button8Down); //special saber attacks
	Cmd_AddCommand("-forcefocus", IN_Button8Up);
	Cmd_AddCommand("+block", IN_Button8Down); //manual blocking
	Cmd_AddCommand("-block", IN_Button8Up);
#endif
	Cmd_AddCommand("+force_grasp", IN_Button9Down);
	Cmd_AddCommand("-force_grasp", IN_Button9Up);

	Cmd_AddCommand("+force_repulse", IN_Button10Down);
	Cmd_AddCommand("-force_repulse", IN_Button10Up);

	Cmd_AddCommand("+force_strike", IN_Button11Down);
	Cmd_AddCommand("-force_strike", IN_Button11Up);

	Cmd_AddCommand("+force_fear", IN_Button12Down);
	Cmd_AddCommand("-force_fear", IN_Button12Up);

	Cmd_AddCommand("+force_deadlysight", IN_Button13Down);
	Cmd_AddCommand("-force_deadlysight", IN_Button13Up);

	Cmd_AddCommand("+force_blast", IN_Button14Down);
	Cmd_AddCommand("-force_blast", IN_Button14Up);

	Cmd_AddCommand("+force_insanity", IN_Button15Down);
	Cmd_AddCommand("-force_insanity", IN_Button15Up);

	Cmd_AddCommand("+force_blinding", IN_Button16Down);
	Cmd_AddCommand("-force_blinding", IN_Button16Up);

	Cmd_AddCommand("+button0", IN_Button0Down);
	Cmd_AddCommand("-button0", IN_Button0Up);
	Cmd_AddCommand("+button1", IN_Button1Down);
	Cmd_AddCommand("-button1", IN_Button1Up);
	Cmd_AddCommand("+button2", IN_Button2Down);
	Cmd_AddCommand("-button2", IN_Button2Up);
	Cmd_AddCommand("+button3", IN_Button3Down);
	Cmd_AddCommand("-button3", IN_Button3Up);
	Cmd_AddCommand("+button4", IN_Button4Down);
	Cmd_AddCommand("-button4", IN_Button4Up);
	Cmd_AddCommand("+button5", IN_Button5Down);
	Cmd_AddCommand("-button5", IN_Button5Up);
	Cmd_AddCommand("+button6", IN_Button6Down);
	Cmd_AddCommand("-button6", IN_Button6Up);
	Cmd_AddCommand("+button7", IN_Button7Down);
	Cmd_AddCommand("-button7", IN_Button7Up);
	Cmd_AddCommand("+button8", IN_Button8Down);
	Cmd_AddCommand("-button8", IN_Button8Up);
	Cmd_AddCommand("+button9", IN_Button9Down);
	Cmd_AddCommand("-button9", IN_Button9Up);
	Cmd_AddCommand("+button10", IN_Button10Down);
	Cmd_AddCommand("-button10", IN_Button10Up);
	Cmd_AddCommand("+button11", IN_Button11Down);
	Cmd_AddCommand("-button11", IN_Button11Up);
	Cmd_AddCommand("+button12", IN_Button12Down);
	Cmd_AddCommand("-button12", IN_Button12Up);
	Cmd_AddCommand("+button13", IN_Button13Down);
	Cmd_AddCommand("-button13", IN_Button13Up);
	Cmd_AddCommand("+button14", IN_Button14Down);
	Cmd_AddCommand("-button14", IN_Button14Up);
	Cmd_AddCommand("+button15", IN_Button15Down);
	Cmd_AddCommand("-button15", IN_Button15Up);
	Cmd_AddCommand("+button16", IN_Button16Down); //free
	Cmd_AddCommand("-button16", IN_Button16Up);
	Cmd_AddCommand("+button17", IN_Button17Down); //free
	Cmd_AddCommand("-button17", IN_Button17Up);
	Cmd_AddCommand("+button18", IN_Button18Down); //free
	Cmd_AddCommand("-button18", IN_Button18Up);
	Cmd_AddCommand("+button19", IN_Button19Down); //free
	Cmd_AddCommand("-button19", IN_Button19Up);
	Cmd_AddCommand("+button20", IN_Button20Down); //free
	Cmd_AddCommand("-button20", IN_Button20Up);

	// can add up to button31 this just brings the number of available binds up to par with MP

	//end buttons
	Cmd_AddCommand("+mlook", IN_MLookDown);
	Cmd_AddCommand("-mlook", IN_MLookUp);

	cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get("cl_debugMove", "0", 0);
}