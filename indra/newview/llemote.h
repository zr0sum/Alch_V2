/** 
 * @file llemote.h
 * @brief Definition of LLEmote class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLEMOTE_H
#define LL_LLEMOTE_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lltimer.h"

#define MIN_REQUIRED_PIXEL_AREA_EMOTE 2000.f

#define EMOTE_MORPH_FADEIN_TIME 0.3f
#define EMOTE_MORPH_IN_TIME 1.1f
#define EMOTE_MORPH_FADEOUT_TIME 1.4f

class LLVisualParam;

//-----------------------------------------------------------------------------
// class LLEmote
//-----------------------------------------------------------------------------
class LLEmote final :
	public LLMotion
{
public:
	// Constructor
	LLEmote(const LLUUID &id);

	// Destructor
	virtual ~LLEmote() = default;

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLEmote(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	BOOL getLoop() override { return FALSE; }

	// motions must report their total duration
	F32 getDuration() override { return EMOTE_MORPH_FADEIN_TIME + EMOTE_MORPH_IN_TIME + EMOTE_MORPH_FADEOUT_TIME; }

	// motions must report their "ease in" duration
	F32 getEaseInDuration() override { return EMOTE_MORPH_FADEIN_TIME; }

	// motions must report their "ease out" duration.
	F32 getEaseOutDuration() override { return EMOTE_MORPH_FADEOUT_TIME; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	F32 getMinPixelArea() override { return MIN_REQUIRED_PIXEL_AREA_EMOTE; }

	// motions must report their priority
	LLJoint::JointPriority getPriority() override { return LLJoint::MEDIUM_PRIORITY; }

	LLMotionBlendType getBlendType() override { return NORMAL_BLEND; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	LLMotionInitStatus onInitialize(LLCharacter *character) override;

	// called when a motion is activated
	// must return TRUE to indicate success, or else
	// it will be deactivated 
	BOOL onActivate() override;

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	BOOL onUpdate(F32 time, U8* joint_mask) override;

	// called when a motion is deactivated
	void onDeactivate() override;

	BOOL canDeprecate() override { return FALSE; }

protected:

	LLCharacter*		mCharacter;

	LLVisualParam*		mParam;
};



#endif // LL_LLEMOTE_H

