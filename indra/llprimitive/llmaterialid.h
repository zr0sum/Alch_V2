/** 
* @file   llmaterialid.h
* @brief  Header file for llmaterialid
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLMATERIALID_H
#define LL_LLMATERIALID_H

#define MATERIAL_ID_SIZE 16

#include "llsd.h"

class LLMaterialID
{
public:
	LLMaterialID();
	LLMaterialID(const LLSD& pMaterialID);
	LLMaterialID(const LLSD::Binary& pMaterialID);
	LLMaterialID(const void* pMemory);
	LLMaterialID(const LLMaterialID& pOtherMaterialID) = default;
	LLMaterialID(const LLUUID& lluid);
	~LLMaterialID() = default;

	bool          operator == (const LLMaterialID& pOtherMaterialID) const;
	bool          operator != (const LLMaterialID& pOtherMaterialID) const;

	bool          operator < (const LLMaterialID& pOtherMaterialID) const;
	bool          operator <= (const LLMaterialID& pOtherMaterialID) const;
	bool          operator > (const LLMaterialID& pOtherMaterialID) const;
	bool          operator >= (const LLMaterialID& pOtherMaterialID) const;

	LLMaterialID& operator = (const LLMaterialID& pOtherMaterialID) = default;

	template <typename H>
	friend H AbslHashValue(H h, const LLMaterialID& id) {
		return H::combine_contiguous(std::move(h), id.mID, MATERIAL_ID_SIZE);
	}

	bool          isNull() const;

	const U8*     get() const;
	void          set(const void* pMemory);
	void          clear();

	LLSD          asLLSD() const;
	std::string   asString() const;

	inline size_t hash() const
	{
		size_t seed = 0;
		for (U8 i = 0; i < 4; ++i)
		{
			seed ^= static_cast<size_t>(mID[i * 4]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= static_cast<size_t>(mID[i * 4 + 1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= static_cast<size_t>(mID[i * 4 + 2]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= static_cast<size_t>(mID[i * 4 + 3]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	friend std::ostream& operator<<(std::ostream& s, const LLMaterialID &material_id);

	static const LLMaterialID null;

private:
	void parseFromBinary(const LLSD::Binary& pMaterialID);
	int  compareToOtherMaterialID(const LLMaterialID& pOtherMaterialID) const;

	U8 mID[MATERIAL_ID_SIZE];
} ;

static_assert(sizeof(LLMaterialID) == MATERIAL_ID_SIZE, "LLMaterialID must be sizeof(mID)");
static_assert(std::is_trivially_copyable<LLMaterialID>{}, "LLMaterialID must be a trivially copyable type");

#endif // LL_LLMATERIALID_H
