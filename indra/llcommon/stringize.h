/**
 * @file   stringize.h
 * @author Nat Goodspeed
 * @date   2008-12-17
 * @brief  stringize(item) template function and STRINGIZE(expression) macro
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#if ! defined(LL_STRINGIZE_H)
#define LL_STRINGIZE_H

#include <sstream>
#include <functional>
#include <llstring.h>

/**
 * stringize_f(functor)
 */
template <typename CHARTYPE, typename Functor>
std::basic_string<CHARTYPE> stringize_f(Functor const & f)
{
    std::basic_ostringstream<CHARTYPE> out;
    f(out);
    return out.str();
}

/**
 * STRINGIZE(item1 << item2 << item3 ...) effectively expands to the
 * following:
 * @code
 * std::ostringstream out;
 * out << item1 << item2 << item3 ... ;
 * return out.str();
 * @endcode
 */
#define STRINGIZE(EXPRESSION) (stringize_f<char>([&](std::ostream& out){ out << EXPRESSION; }))

/**
 * WSTRINGIZE() is the wstring equivalent of STRINGIZE()
 */
#define WSTRINGIZE(EXPRESSION) (stringize_f<wchar_t>([&](std::wostream& out){ out << EXPRESSION; }))

/**
 * destringize(str)
 * defined for symmetry with stringize
 * @NOTE - this has distinct behavior from boost::lexical_cast<T> regarding
 * leading/trailing whitespace and handling of bad_lexical_cast exceptions
 * @NOTE - no need for dewstringize(), since passing std::wstring will Do The
 * Right Thing
 */
template <typename T, typename CHARTYPE>
T destringize(std::basic_string<CHARTYPE> const & str)
{
    T val;
    std::basic_istringstream<CHARTYPE> in(str);
    in >> val;
    return val;
}

/**
 * destringize_f(str, functor)
 */
template <typename CHARTYPE, typename Functor>
void destringize_f(std::basic_string<CHARTYPE> const & str, Functor const & f)
{
    std::basic_istringstream<CHARTYPE> in(str);
    f(in);
}

/**
 * DESTRINGIZE(str, item1 >> item2 >> item3 ...) effectively expands to the
 * following:
 * @code
 * std::istringstream in(str);
 * in >> item1 >> item2 >> item3 ... ;
 * @endcode
 * @NOTE - once we get generic lambdas, we shouldn't need DEWSTRINGIZE() any
 * more since DESTRINGIZE() should do the right thing with a std::wstring. But
 * until then, the lambda we pass must accept the right std::basic_istream.
 */
#define DESTRINGIZE(STR, EXPRESSION) (destringize_f((STR), [&](std::istream& in){in >> EXPRESSION;}))
#define DEWSTRINGIZE(STR, EXPRESSION) (destringize_f((STR), [&](std::wistream& in){in >> EXPRESSION;}))

#endif /* ! defined(LL_STRINGIZE_H) */
