/** 
 * @file llfloaternamedesc.h
 * @brief LLFloaterNameDesc class definition
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

#ifndef LL_LLFLOATERNAMEDESC_H
#define LL_LLFLOATERNAMEDESC_H

#include "llfloater.h"
#include "llresizehandle.h"

class LLLineEditor;
class LLButton;
class LLRadioGroup;

class LLFloaterNameDesc : public LLFloater
{
public:
	LLFloaterNameDesc(const LLSD& filename);
	virtual ~LLFloaterNameDesc();
	BOOL postBuild() override;
	
	void		onBtnOK();
	void		onBtnCancel();
	void		doCommit();
	
protected:
	void		onCommit() final override;

protected:

	std::string		mFilenameAndPath;
	std::string		mFilename;
};

class LLFloaterSoundPreview final : public LLFloaterNameDesc
{
public:
	LLFloaterSoundPreview(const LLSD& filename );
	BOOL postBuild() override;
};

class LLFloaterAnimPreview final : public LLFloaterNameDesc
{
public:
	LLFloaterAnimPreview(const LLSD& filename );
	BOOL postBuild() override;
};

class LLFloaterScriptPreview final : public LLFloaterNameDesc
{
public:
	LLFloaterScriptPreview(const LLSD& filename );
	BOOL postBuild() override;
};

#endif  // LL_LLFLOATERNAMEDESC_H