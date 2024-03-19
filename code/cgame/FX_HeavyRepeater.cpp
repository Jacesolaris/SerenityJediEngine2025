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

// Heavy Repeater Weapon

#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_RepeaterProjectileThink
---------------------------
*/

void FX_RepeaterProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect(cgs.effects.repeaterProjectileEffect, cent->lerpOrigin, forward);
}

/*
------------------------
FX_RepeaterHitWall
------------------------
*/

void FX_RepeaterHitWall(vec3_t origin, vec3_t normal)
{
	theFxScheduler.PlayEffect(cgs.effects.repeaterWallImpactEffect, origin, normal);
}

/*
------------------------
FX_RepeaterHitPlayer
------------------------
*/

void FX_RepeaterHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.repeaterFleshImpactEffect, origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.blasterDroidImpactEffect, origin, normal);
	}
}

/*
------------------------------
FX_RepeaterAltProjectileThink
-----------------------------
*/

void FX_RepeaterAltProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect(cgs.effects.repeaterAltProjectileEffect, cent->lerpOrigin, forward);
}

/*
------------------------
FX_RepeaterAltHitWall
------------------------
*/

void FX_RepeaterAltHitWall(vec3_t origin, vec3_t normal)
{
	theFxScheduler.PlayEffect(cgs.effects.repeaterAltWallImpactEffect, origin, normal);
}

/*
------------------------
FX_RepeaterAltHitPlayer
------------------------
*/

void FX_RepeaterAltHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect(cgs.effects.repeaterAltWallImpactEffect, origin);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.blasterDroidImpactEffect, origin, normal);
	}
}

void FX_CloneProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (cent->currentState.eFlags & EF_USE_ANGLEDELTA)
	{
		AngleVectors(cent->currentState.angles, forward, nullptr, nullptr);
	}
	else
	{
		if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
		{
			if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
			{
				forward[2] = 1.0f;
			}
		}
	}

	// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
	int dif = cg.time - cent->gent->s.pos.trTime;

	if (dif < 75)
	{
		if (dif < 0)
		{
			dif = 0;
		}

		const float scale = dif / 75.0f * 0.95f + 0.05f;

		VectorScale(forward, scale, forward);
	}

	theFxScheduler.PlayEffect(cgs.effects.cloneShotEffect, cent->lerpOrigin, forward);
}