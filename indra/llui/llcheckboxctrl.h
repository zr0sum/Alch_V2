/** 
 * @file llcheckboxctrl.h
 * @brief LLCheckBoxCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLCHECKBOXCTRL_H
#define LL_LLCHECKBOXCTRL_H

#include "lluictrl.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "v4color.h"

//
// Constants
//

const BOOL	RADIO_STYLE = TRUE;
const BOOL	CHECK_STYLE = FALSE;

//
// Classes
//
class LLFontGL;
class LLViewBorder;

class LLCheckBoxCtrl
: public LLUICtrl
, public ll::ui::SearchableControl
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>			initial_value;	// override LLUICtrl initial_value

		Optional<LLTextBox::Params> label_text;
		Optional<LLButton::Params> check_button;

		Ignored					radio_style;

		Params();
	};

	virtual ~LLCheckBoxCtrl() = default;

protected:
	LLCheckBoxCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
	// LLView interface

	void		setEnabled( BOOL b ) final override;

	void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) final override;

	// LLUICtrl interface
	void		setValue(const LLSD& value ) override;
	LLSD		getValue() const final override;
			BOOL		get() { return (BOOL)getValue().asBoolean(); }
			void		set(BOOL value) { setValue(value); }

	void		setTentative(BOOL b) final override;
	BOOL		getTentative() const final override;

	BOOL		setLabelArg( const std::string& key, const LLStringExplicit& text ) final override;

	void		clear() final override;
	void		onCommit() final override;

	// LLCheckBoxCtrl interface
	virtual BOOL		toggle()				{ return mButton->toggleState(); }		// returns new state

	void				setBtnFocus() { mButton->setFocus(TRUE); }

	void				setEnabledColor( const LLColor4 &color ) { mTextEnabledColor = color; }
	void				setDisabledColor( const LLColor4 &color ) { mTextDisabledColor = color; }

	void				setLabel( const LLStringExplicit& label );
	const std::string&	getLabel() const;

	void				setFont( const LLFontGL* font ) { mFont = font; }
	const LLFontGL*		getFont() { return mFont; }

	void		setControlName(const std::string& control_name, LLView* context) final override;

	BOOL		isDirty()	const final override;		// Returns TRUE if the user has modified this control.
	void		resetDirty() final override;			// Clear dirty state

protected:
	std::string _getSearchText() const final override
	{
		return getLabel() + getToolTip();
	}

	void onSetHighlight() const  final override // When highlight, really do highlight the label
	{
		if( mLabel )
			mLabel->ll::ui::SearchableControl::setHighlighted( ll::ui::SearchableControl::getHighlighted() );
	}

protected:
	// note: value is stored in toggle state of button
	LLButton*		mButton;
	LLTextBox*		mLabel;
	const LLFontGL* mFont;

	LLUIColor		mTextEnabledColor;
	LLUIColor		mTextDisabledColor;
};

// Build time optimization, generate once in .cpp file
#ifndef LLCHECKBOXCTRL_CPP
extern template class LLCheckBoxCtrl* LLView::getChild<class LLCheckBoxCtrl>(
	const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_LLCHECKBOXCTRL_H