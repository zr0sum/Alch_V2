/** 
 * @file llfontfreetype.h
 * @brief Font library wrapper
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

#ifndef LL_LLFONTFREETYPE_H
#define LL_LLFONTFREETYPE_H

#include <absl/container/flat_hash_map.h>
#include "llpointer.h"
#include "llstl.h"

#include "llimagegl.h"
#include "llfontbitmapcache.h"

// Hack.  FT_Face is just a typedef for a pointer to a struct,
// but there's no simple forward declarations file for FreeType, 
// and the main include file is 200K.  
// We'll forward declare the struct here.  JC
struct FT_FaceRec_;
typedef struct FT_FaceRec_* LLFT_Face;
struct FT_StreamRec_;
typedef struct FT_StreamRec_ LLFT_Stream;

class LLFontManager
{
public:
	static void initClass();
	static void cleanupClass();

private:
	LLFontManager();
	~LLFontManager();
public:
	U8 const *loadFont( std::string const &aFilename, long &a_Size );

private:
	struct LoadedFont
	{
		LoadedFont(std::string aName, std::unique_ptr<U8[]> aAddress, long aSize)
			: mName(std::move(aName))
			, mAddress(std::move(aAddress))
			, mSize(aSize)
		{ }

		std::string mName;
		std::unique_ptr<U8[]> mAddress;
		long mSize;
	};

	void unloadAllFonts();
	absl::flat_hash_map< std::string, std::unique_ptr<LoadedFont> > m_LoadedFonts;
};

struct LLFontGlyphInfo
{
	LLFontGlyphInfo(U32 index);

	U32 mGlyphIndex;

	// Metrics
	S32 mWidth;			// In pixels
	S32 mHeight;		// In pixels
	F32 mXAdvance;		// In pixels
	F32 mYAdvance;		// In pixels

	// Information for actually rendering
	S32 mXBitmapOffset; // Offset to the origin in the bitmap
	S32 mYBitmapOffset; // Offset to the origin in the bitmap
	S32 mXBearing;	// Distance from baseline to left in pixels
	S32 mYBearing;	// Distance from baseline to top in pixels
	S32 mBitmapNum; // Which bitmap in the bitmap cache contains this glyph
};

extern LLFontManager *gFontManagerp;

class LLFontFreetype final : public LLRefCount, public LLTrace::MemTrackable<LLFontFreetype>
{
public:
	LLFontFreetype();
	~LLFontFreetype();

	// is_fallback should be true for fallback fonts that aren't used
	// to render directly (Unicode backup, primarily)
	BOOL loadFace(const std::string& filename, F32 point_size, F32 vert_dpi, F32 horz_dpi, S32 components, BOOL is_fallback, S32 face_n = 0);

	S32 getNumFaces(const std::string& filename);

	typedef std::vector<LLPointer<LLFontFreetype> > font_vector_t;

	void setFallbackFonts(const font_vector_t &font);
	const font_vector_t &getFallbackFonts() const;

	// Global font metrics - in units of pixels
	F32 getLineHeight() const;
	F32 getAscenderHeight() const;
	F32 getDescenderHeight() const;


// For a lowercase "g":
//
//	------------------------------
//	                     ^     ^
//						 |     |
//				xxx x    |Ascender
//	           x   x     v     |
//	---------   xxxx-------------- Baseline
//	^		       x	       |
//  | Descender    x           |
//	v			xxxx           |LineHeight
//  -----------------------    |
//                             v
//	------------------------------

	enum
	{
		FIRST_CHAR = 32, 
		NUM_CHARS = 127 - 32, 
		LAST_CHAR_BASIC = 127,

		// Need full 8-bit ascii range for spanish
		NUM_CHARS_FULL = 255 - 32,
		LAST_CHAR_FULL = 255
	};

	F32 getXAdvance(llwchar wc) const;
	F32 getXAdvance(const LLFontGlyphInfo* glyph) const;
	F32 getXKerning(llwchar char_left, llwchar char_right) const; // Get the kerning between the two characters
	F32 getXKerning(const LLFontGlyphInfo* left_glyph_info, const LLFontGlyphInfo* right_glyph_info) const; // Get the kerning between the two characters

	LLFontGlyphInfo* getGlyphInfo(llwchar wch) const;

	void reset(F32 vert_dpi, F32 horz_dpi);

	void destroyGL();

	const std::string& getName() const;

	const LLFontBitmapCache* getFontBitmapCache() const;

	void setStyle(U8 style);
	U8 getStyle() const;

	static std::string getVersionString();

private:
	void resetBitmapCache();
	void setSubImageLuminanceAlpha(U32 x, U32 y, U32 bitmap_num, U32 width, U32 height, U8 *data, S32 stride = 0) const;
	BOOL hasGlyph(llwchar wch) const;		// Has a glyph for this character
	LLFontGlyphInfo* addGlyph(llwchar wch) const;		// Add a new character to the font if necessary
	LLFontGlyphInfo* addGlyphFromFont(const LLFontFreetype *fontp, llwchar wch, U32 glyph_index) const;	// Add a glyph from this font to the other (returns the glyph_index, 0 if not found)
	void renderGlyph(U32 glyph_index) const;
	void insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gi) const;

	bool getKerningCache(U32 left_glyph, U32 right_glyph, F32& kerning) const;
	void setKerningCache(U32 left_glyph, U32 right_glyph, F32 kerning) const;

	mutable absl::flat_hash_map<U64, F32> mKerningCache;

	std::string mName;

	U8 mStyle;

	F32 mPointSize;
	F32 mAscender;			
	F32 mDescender;
	F32 mLineHeight;

	LLFT_Face mFTFace;

	BOOL mIsFallback;
	font_vector_t mFallbackFonts; // A list of fallback fonts to look for glyphs in (for Unicode chars)

	using char_glyph_info_map_t = absl::flat_hash_map<llwchar, LLFontGlyphInfo*>;
	mutable char_glyph_info_map_t mCharGlyphInfoMap; // Information about glyph location in bitmap

	mutable LLFontBitmapCache* mFontBitmapCachep;

	mutable S32 mRenderGlyphCount;
	mutable S32 mAddGlyphCount;
};

#endif // LL_FONTFREETYPE_H