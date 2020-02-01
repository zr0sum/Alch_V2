/*
* @file llpanelavcomplexitypulldown.cpp
* @brief Avatar weight flyout
*
* Copyright (c) 2017-2019, Cinder Roxley <cinder@sdf.org>
*
* Permission is hereby granted, free of charge, to any person or organization
* obtaining a copy of the software and accompanying documentation covered by
* this license (the "Software") to use, reproduce, display, distribute,
* execute, and transmit the Software, and to prepare derivative works of the
* Software, and to permit third-parties to whom the Software is furnished to
* do so, all subject to the following:
*
* The copyright notices in the Software and this entire statement, including
* the above license grant, this restriction and the following disclaimer,
* must be included in all copies of the Software, in whole or in part, and
* all derivative works of the Software, unless such copies or derivative
* works are solely in the form of machine-executable object code generated by
* a source language processor.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*/

#include "llviewerprecompiledheaders.h"
#include "llpanelavcomplexitypulldown.h"

#include "lltextbox.h"
#include "lltrans.h"

LLPanelAvatarComplexityPulldown::LLPanelAvatarComplexityPulldown()
{
	buildFromFile("panel_avcomplexity_pulldown.xml");
}

BOOL LLPanelAvatarComplexityPulldown::postBuild()
{
	mComplexity = getChild<LLTextBox>("complexity");
	mVisibility = getChild<LLTextBox>("visibility");

    setAvComplexity(0, 0, 0);

	return LLPanel::postBuild();
}

void LLPanelAvatarComplexityPulldown::setAvComplexity(S32 complexity, F32 muted_pct, U32 agents)
{
    if (mComplexity)
    {
        if (complexity > 0) 
            mComplexity->setTextArg("COMPLEXITY", fmt::to_string(complexity));
        else if (complexity == 0)
            mComplexity->setTextArg("COMPLEXITY", LLTrans::getString("Unknown"));
        else 
            mComplexity->setTextArg("COMPLEXITY", LLTrans::getString("invalid"));
    }
	if (mVisibility)
	{
		mVisibility->setTextArg("VISIBILITY", agents ? llformat("%.2f", 100.f - muted_pct) : "100");
		mVisibility->setTextArg("AGENTS", agents ? llformat("(%d)", agents) : "");
	}
}