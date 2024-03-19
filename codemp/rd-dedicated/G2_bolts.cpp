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

#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "tr_local.h"

//=====================================================================================================================
// Bolt List handling routines - so entities can attach themselves to any part of the model in question

// Given a bone number, see if that bone is already in our bone list
int G2_Find_Bolt_boneNum(const boltInfo_v& bltlist, const int boneNum)
{
	// look through entire list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (bltlist[i].boneNumber == -1)
		{
			continue;
		}

		if (bltlist[i].boneNumber == boneNum)
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

// Given a bone number, see if that surface is already in our surfacelist list
int G2_Find_Bolt_surfaceNum(const boltInfo_v& bltlist, const int surfaceNum, const int flags)
{
	// look through entire list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this bone entry has no info in it, bounce over it
		if (bltlist[i].surfaceNumber == -1)
		{
			continue;
		}

		if (bltlist[i].surfaceNumber == surfaceNum && (bltlist[i].surfaceType & flags) == flags)
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

//=========================================================================================
//// Public Bolt Routines
int G2_Add_Bolt_Surf_Num(const CGhoul2Info* ghlInfo, boltInfo_v& bltlist, const surfaceInfo_v& slist, const int surfNum)
{
	assert(ghlInfo && ghlInfo->mValid);
	boltInfo_t temp_bolt;

	// first up, make sure have a surface first
	if (surfNum >= static_cast<int>(slist.size()))
	{
		return -1;
	}

	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// already there??
		if (bltlist[i].surfaceNumber == surfNum)
		{
			// increment the usage count
			bltlist[i].boltUsed++;
			return i;
		}
	}

	// we have a surface
	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this surface entry has info in it, bounce over it
		if (bltlist[i].boneNumber == -1 && bltlist[i].surfaceNumber == -1)
		{
			// if we found an entry that had a -1 for the bone / surface number, then we hit a surface / bone slot that was empty
			bltlist[i].surfaceNumber = surfNum;
			bltlist[i].surfaceType = G2SURFACEFLAG_GENERATED;
			bltlist[i].boltUsed = 1;
			return i;
		}
	}

	// ok, we didn't find an existing surface of that name, or an empty slot. Lets add an entry
	temp_bolt.surfaceNumber = surfNum;
	temp_bolt.surfaceType = G2SURFACEFLAG_GENERATED;
	temp_bolt.boneNumber = -1;
	temp_bolt.boltUsed = 1;
	bltlist.push_back(temp_bolt);
	return bltlist.size() - 1;
}

int G2_Add_Bolt(const CGhoul2Info* ghlInfo, boltInfo_v& bltlist, surfaceInfo_v& slist, const char* boneName)
{
	assert(ghlInfo && ghlInfo->mValid);
	const auto mod_m = const_cast<model_t*>(ghlInfo->currentModel);
	const model_t* mod_a = const_cast<model_t*>(ghlInfo->animModel);
	int x;
	boltInfo_t temp_bolt;
	int flags;

	// first up, we'll search for that which this bolt names in all the surfaces
	const int surfNum = G2_IsSurfaceLegal(mod_m, boneName, &flags);

	// did we find it as a surface?
	if (surfNum != -1)
	{
		// look through entire list - see if it's already there first
		for (size_t i = 0; i < bltlist.size(); i++)
		{
			// already there??
			if (bltlist[i].surfaceNumber == surfNum)
			{
				// increment the usage count
				bltlist[i].boltUsed++;
				return i;
			}
		}

		// look through entire list - see if we can re-use one
		for (size_t i = 0; i < bltlist.size(); i++)
		{
			// if this surface entry has info in it, bounce over it
			if (bltlist[i].boneNumber == -1 && bltlist[i].surfaceNumber == -1)
			{
				// if we found an entry that had a -1 for the bone / surface number, then we hit a surface / bone slot that was empty
				bltlist[i].surfaceNumber = surfNum;
				bltlist[i].boltUsed = 1;
				bltlist[i].surfaceType = 0;
				return i;
			}
		}

		// ok, we didn't find an existing surface of that name, or an empty slot. Lets add an entry
		temp_bolt.surfaceNumber = surfNum;
		temp_bolt.boneNumber = -1;
		temp_bolt.boltUsed = 1;
		temp_bolt.surfaceType = 0;
		bltlist.push_back(temp_bolt);
		return bltlist.size() - 1;
	}

	// no, check to see if it's a bone then

	const mdxaSkelOffsets_t* offsets = reinterpret_cast<mdxaSkelOffsets_t*>(reinterpret_cast<byte*>(mod_a->mdxa) +
		sizeof(mdxaHeader_t));

	// walk the entire list of bones in the gla file for this model and see if any match the name of the bone we want to find
	for (x = 0; x < mod_a->mdxa->numBones; x++)
	{
		const mdxaSkel_t* skel = reinterpret_cast<mdxaSkel_t*>(reinterpret_cast<byte*>(mod_a->mdxa) + sizeof(
			mdxaHeader_t) + offsets->offsets[x]);
		// if name is the same, we found it
		if (!Q_stricmp(skel->name, boneName))
		{
			break;
		}
	}

	// check to see we did actually make a match with a bone in the model
	if (x == mod_a->mdxa->numBones)
	{
		// didn't find it? Error
		//assert(0&&x == mod_a->mdxa->numBones);
#ifdef _DEBUG
		//		Com_Printf("WARNING: %s not found on skeleton\n", boneName);
#endif
		return -1;
	}

	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// already there??
		if (bltlist[i].boneNumber == x)
		{
			// increment the usage count
			bltlist[i].boltUsed++;
			return i;
		}
	}

	// look through entire list - see if we can re-use it
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this bone entry has info in it, bounce over it
		if (bltlist[i].boneNumber == -1 && bltlist[i].surfaceNumber == -1)
		{
			// if we found an entry that had a -1 for the bone number, then we hit a bone slot that was empty
			bltlist[i].boneNumber = x;
			bltlist[i].boltUsed = 1;
			bltlist[i].surfaceType = 0;
			return i;
		}
	}

	// ok, we didn't find an existing bone of that name, or an empty slot. Lets add an entry
	temp_bolt.boneNumber = x;
	temp_bolt.surfaceNumber = -1;
	temp_bolt.boltUsed = 1;
	temp_bolt.surfaceType = 0;
	bltlist.push_back(temp_bolt);
	return bltlist.size() - 1;
}

// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bolt(boltInfo_v& bltlist, const int index)
{
	// did we find it?
	if (index != -1)
	{
		bltlist[index].boltUsed--;
		if (!bltlist[index].boltUsed)
		{
			// set this bone to not used
			bltlist[index].boneNumber = -1;
			bltlist[index].surfaceNumber = -1;

			unsigned int newSize = bltlist.size();
			// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
			for (int i = bltlist.size() - 1; i > -1; i--)
			{
				if (bltlist[i].surfaceNumber == -1 && bltlist[i].boneNumber == -1)
				{
					newSize = i;
				}
				// once we hit one that isn't a -1, we are done.
				else
				{
					break;
				}
			}
			// do we need to resize?
			if (newSize != bltlist.size())
			{
				// yes, so lets do it
				bltlist.resize(newSize);
			}
		}
		return qtrue;
	}

	assert(0);

	// no
	return qfalse;
}

// set the bolt list to all unused so the bone transformation routine ignores it.
void G2_Init_Bolt_List(boltInfo_v& bltlist)
{
	bltlist.clear();
}

// remove any bolts that reference original surfaces, generated surfaces, or bones that aren't active anymore
void G2_RemoveRedundantBolts(boltInfo_v& bltlist, surfaceInfo_v& slist, const int* activeSurfaces, const int* active_bones)
{
	// walk the bolt list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// are we using this bolt?
		if (bltlist[i].surfaceNumber != -1 || bltlist[i].boneNumber != -1)
		{
			// is this referenceing a surface?
			if (bltlist[i].surfaceNumber != -1)
			{
				// is this bolt looking at a generated surface?
				if (bltlist[i].surfaceType)
				{
					// yes, so look for it in the surface list
					if (!G2_FindOverrideSurface(bltlist[i].surfaceNumber, slist))
					{
						// no - we want to remove this bolt, regardless of how many people are using it
						bltlist[i].boltUsed = 1;
						G2_Remove_Bolt(bltlist, i);
					}
				}
				// no, it's an original, so look for it in the active surfaces list
				{
					if (!activeSurfaces[bltlist[i].surfaceNumber])
					{
						// no - we want to remove this bolt, regardless of how many people are using it
						bltlist[i].boltUsed = 1;
						G2_Remove_Bolt(bltlist, i);
					}
				}
			}
			// no, must be looking at a bone then
			else
			{
				// is that bone active then?
				if (!active_bones[bltlist[i].boneNumber])
				{
					// no - we want to remove this bolt, regardless of how many people are using it
					bltlist[i].boltUsed = 1;
					G2_Remove_Bolt(bltlist, i);
				}
			}
		}
	}
}