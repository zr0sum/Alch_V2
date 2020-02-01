/** 
 * @file common.cpp
 * @author Phoenix
 * @date 2005-10-12
 * @brief Common templates for test framework
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

/** 
 * 
 * THOROUGH_DESCRIPTION of common.cpp
 *
 */

#include <algorithm>
#include <iomanip>
#include <iterator>

#include "linden_common.h"

#include "../llmemorystream.h"
#include "../llsd.h"
#include "../llsdserialize.h"

#include "../test/lltut.h"


#if LL_WINDOWS
// disable overflow warnings
#pragma warning(disable: 4307)
#endif

namespace tut
{
	struct sd_data
	{
	};
	typedef test_group<sd_data> sd_test;
	typedef sd_test::object sd_object;
	tut::sd_test sd("LLSD");

	template<> template<>
	void sd_object::test<1>()
	{
		std::ostringstream resp;
		resp << "{'connect':true,  'position':[r128,r128,r128], 'look_at':[r0,r1,r0], 'agent_access':'M', 'region_x':i8192, 'region_y':i8192}";
		std::string str = resp.str();
		LLMemoryStream mstr((U8*)str.c_str(), str.size());
		LLSD response;
		S32 count = LLSDSerialize::fromNotation(response, mstr, str.size());
		ensure("stream parsed", response.isDefined());
		ensure_equals("stream parse count", count, 13);
		ensure_equals("sd type", response.type(), LLSD::TypeMap);
		ensure_equals("map element count", response.size(), 6);
		ensure_equals("value connect", response["connect"].asBoolean(), true);
		ensure_equals("value region_x", response["region_x"].asInteger(),8192);
		ensure_equals("value region_y", response["region_y"].asInteger(),8192);
	}

	template<> template<>
	void sd_object::test<2>()
	{
		const std::string decoded("random");
		//const std::string encoded("cmFuZG9t\n");
		const std::string streamed("b(6)\"random\"");
		typedef std::vector<U8> buf_t;
		buf_t buf;
		std::copy(
			decoded.begin(),
			decoded.end(),
			std::back_insert_iterator<buf_t>(buf));
		LLSD sd;
		sd = buf;
		std::stringstream str;
		S32 count = LLSDSerialize::toNotation(sd, str);
		ensure_equals("output count", count, 1);
		std::string actual(str.str());
		ensure_equals("formatted binary encoding", actual, streamed);
		sd.clear();
		LLSDSerialize::fromNotation(sd, str, str.str().size());
		std::vector<U8> after;
		after = sd.asBinary();
		ensure_equals("binary decoded size", after.size(), decoded.size());
		ensure("binary decoding", (0 == memcmp(
									   &after[0],
									   decoded.c_str(),
									   decoded.size())));
	}

	template<> template<>
	void sd_object::test<3>()
	{
		for(S32 i = 0; i < 100; ++i)
		{
			// gen up a starting point
			typedef std::vector<U8> buf_t;
			buf_t source;
			srand(i);		/* Flawfinder: ignore */
			S32 size = rand() % 1000 + 10;
			std::generate_n(
				std::back_insert_iterator<buf_t>(source),
				size,
				rand);
			LLSD sd(source);
			std::stringstream str;
			S32 count = LLSDSerialize::toNotation(sd, str);
			sd.clear();
			ensure_equals("format count", count, 1);
			LLSD sd2;
			count = LLSDSerialize::fromNotation(sd2, str, str.str().size());
			ensure_equals("parse count", count, 1);
			buf_t dest = sd2.asBinary();
			str.str("");
			str << "binary encoding size " << i;
			ensure_equals(str.str().c_str(), dest.size(), source.size());
			str.str("");
			str << "binary encoding " << i;
			ensure(str.str().c_str(), (source == dest));
		}
	}

	template<> template<>
	void sd_object::test<4>()
	{
		std::ostringstream ostr;
		ostr << "{'task_id':u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}\n"
			 << "{\n\tname\tObject|\n}\n";
		std::string expected = ostr.str();
		std::stringstream serialized;
		serialized << "'" << LLSDNotationFormatter::escapeString(expected)
			   << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(
			sd,
			serialized,
			serialized.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("String streaming", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<5>()
	{
		for(S32 i = 0; i < 100; ++i)
		{
			// gen up a starting point
			typedef std::vector<U8> buf_t;
			buf_t source;
			srand(666 + i);		/* Flawfinder: ignore */
			S32 size = rand() % 1000 + 10;
			std::generate_n(
				std::back_insert_iterator<buf_t>(source),
				size,
				rand);
			std::stringstream str;
			str << "b(" << size << ")\"";
			str.write((const char*)&source[0], size);
			str << "\"";
			LLSD sd;
			S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
			ensure_equals("binary parse", count, 1);
			buf_t actual = sd.asBinary();
			ensure_equals("binary size", actual.size(), (size_t)size);
			ensure("binary data", (0 == memcmp(&source[0], &actual[0], size)));
		}
	}

	template<> template<>
	void sd_object::test<6>()
	{
		std::string expected("'{\"task_id\":u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}'\t\n\t\t");
		std::stringstream str;
		str << "s(" << expected.size() << ")'";
		str.write(expected.c_str(), expected.size());
		str << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		std::string actual = sd.asString();
		ensure_equals("string sizes", actual.size(), expected.size());
		ensure_equals("string content", actual, expected);
	}

	template<> template<>
	void sd_object::test<7>()
	{
		std::string msg("come on in");
		std::stringstream stream;
		stream << "{'connect':1, 'message':'" << msg << "',"
			   << " 'position':[r45.65,r100.1,r25.5],"
			   << " 'look_at':[r0,r1,r0],"
			   << " 'agent_access':'PG'}";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(
			sd,
			stream,
			stream.str().size());
		ensure_equals("parse count", count, 12);
		ensure_equals("bool value", sd["connect"].asBoolean(), true);
		ensure_equals("message value", sd["message"].asString(), msg);
		ensure_equals("pos x", sd["position"][0].asReal(), 45.65);
		ensure_equals("pos y", sd["position"][1].asReal(), 100.1);
		ensure_equals("pos z", sd["position"][2].asReal(), 25.5);
		ensure_equals("look x", sd["look_at"][0].asReal(), 0.0);
		ensure_equals("look y", sd["look_at"][1].asReal(), 1.0);
		ensure_equals("look z", sd["look_at"][2].asReal(), 0.0);
	}

	template<> template<>
	void sd_object::test<8>()
	{
		std::stringstream resp;
		resp << "{'label':'short string test', 'singlechar':'a', 'empty':'', 'endoftest':'end' }";
		LLSD response;
		S32 count = LLSDSerialize::fromNotation(
			response,
			resp,
			resp.str().size());
		ensure_equals("parse count", count, 5);
		ensure_equals("sd type", response.type(), LLSD::TypeMap);
		ensure_equals("map element count", response.size(), 4);
		ensure_equals("singlechar", response["singlechar"].asString(), "a");
		ensure_equals("empty", response["empty"].asString(), "");
	}

	template<> template<>
	void sd_object::test<9>()
	{
		std::ostringstream resp;
		resp << "{'label':'short binary test', 'singlebinary':b(1)\"A\", 'singlerawstring':s(1)\"A\", 'endoftest':'end' }";
		std::string str = resp.str();
		LLSD sd;
		LLMemoryStream mstr((U8*)str.c_str(), str.size());
		S32 count = LLSDSerialize::fromNotation(sd, mstr, str.size());
		ensure_equals("parse count", count, 5);
		ensure("sd created", sd.isDefined());
		ensure_equals("sd type", sd.type(), LLSD::TypeMap);
		ensure_equals("map element count", sd.size(), 4);
		ensure_equals(
			"label",
			sd["label"].asString(),
			"short binary test");
		std::vector<U8> bin =  sd["singlebinary"].asBinary();
		std::vector<U8> expected;
		expected.resize(1);
		expected[0] = 'A';
		ensure("single binary", (0 == memcmp(&bin[0], &expected[0], 1)));
		ensure_equals(
			"single string",
			sd["singlerawstring"].asString(),
			std::string("A"));
		ensure_equals("end", sd["endoftest"].asString(), "end");
	}

	template<> template<>
	void sd_object::test<10>()
	{

		std::string message("parcel '' is naughty.");
		std::stringstream str;
		str << "{'message':'" << LLSDNotationFormatter::escapeString(message)
			<< "'}";
		std::string expected_str("{'message':'parcel \\'\\' is naughty.'}");
		std::string actual_str = str.str();
		ensure_equals("stream contents", actual_str, expected_str);
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, actual_str.size());
		ensure_equals("parse count", count, 2);
		ensure("valid parse", sd.isDefined());
		std::string actual = sd["message"].asString();
		ensure_equals("message contents", actual, message);
	}

	template<> template<>
	void sd_object::test<11>()
	{
		std::string expected("\"\"\"\"''''''\"");
		std::stringstream str;
		str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("string value", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<12>()
	{
		std::string expected("mytest\\");
		std::stringstream str;
		str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("string value", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<13>()
	{
		for(S32 i = 0; i < 1000; ++i)
		{
			// gen up a starting point
			std::string expected;
			srand(1337 + i);		/* Flawfinder: ignore */
			S32 size = rand() % 30 + 5;
			std::generate_n(
				std::back_insert_iterator<std::string>(expected),
				size,
				rand);
			std::stringstream str;
			str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
			LLSD sd;
			S32 count = LLSDSerialize::fromNotation(sd, str, expected.size());
			ensure_equals("parse count", count, 1);
			std::string actual = sd.asString();
/*
			if(actual != expected)
			{
				LL_WARNS() << "iteration " << i << LL_ENDL;
				std::ostringstream e_str;
				std::string::iterator iter = expected.begin();
				std::string::iterator end = expected.end();
				for(; iter != end; ++iter)
				{
					e_str << (S32)((U8)(*iter)) << " ";
				}
				e_str << std::endl;
				llsd_serialize_string(e_str, expected);
				LL_WARNS() << "expected size: " << expected.size() << LL_ENDL;
				LL_WARNS() << "expected:      " << e_str.str() << LL_ENDL;

				std::ostringstream a_str;
				iter = actual.begin();
				end = actual.end();
				for(; iter != end; ++iter)
				{
					a_str << (S32)((U8)(*iter)) << " ";
				}
				a_str << std::endl;
				llsd_serialize_string(a_str, actual);
				LL_WARNS() << "actual size:   " << actual.size() << LL_ENDL;
				LL_WARNS() << "actual:      " << a_str.str() << LL_ENDL;
			}
*/
			ensure_equals("string value", actual, expected);
		}
	}

	template<> template<>
	void sd_object::test<14>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		std::string param = "[{'version':i1},{'data':{'binary_bucket':b(0)\"\"},'from_id':u3c115e51-04f4-523c-9fa6-98aff1034730,'from_name':'Phoenix Linden','id':u004e45e5-5576-277a-fba7-859d6a4cb5c8,'message':'hey','offline':i0,'timestamp':i0,'to_id':u3c5f1bb4-5182-7546-6401-1d329b4ff2f8,'type':i0},{'agent_id':u3c115e51-04f4-523c-9fa6-98aff1034730,'god_level':i0,'limited_to_estate':i1}]";
		std::istringstream istr;
		istr.str(param);
		LLSD param_sd;
		LLSDSerialize::fromNotation(param_sd, istr, param.size());
		ensure_equals("parsed type", param_sd.type(), LLSD::TypeArray);
		LLSD version_sd = param_sd[0];
		ensure_equals("version type", version_sd.type(), LLSD::TypeMap);
		ensure("has version", version_sd.has("version"));
		ensure_equals("version number", version_sd["version"].asInteger(), 1);
		LLSD src_sd = param_sd[1];
		ensure_equals("src type", src_sd.type(), LLSD::TypeMap);
		LLSD dst_sd = param_sd[2];
		ensure_equals("dst type", dst_sd.type(), LLSD::TypeMap);
	}

	template<> template<>
	void sd_object::test<15>()
	{
		std::string val = "[{'failures':!,'successfuls':[u3c115e51-04f4-523c-9fa6-98aff1034730]}]";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parsed type", sd.type(), LLSD::TypeArray);
		ensure_equals("parsed size", sd.size(), 1);
		LLSD failures = sd[0]["failures"];
		ensure("no failures.", failures.isUndefined());
		LLSD success = sd[0]["successfuls"];
		ensure_equals("success type", success.type(), LLSD::TypeArray);
		ensure_equals("success size", success.size(), 1);
		ensure_equals("success instance type", success[0].type(), LLSD::TypeUUID);
	}

	template<> template<>
	void sd_object::test<16>()
	{
		std::string val = "[f,t,0,1,{'foo':t,'bar':f}]";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parsed type", sd.type(), LLSD::TypeArray);
		ensure_equals("parsed size", sd.size(), 5);
		ensure_equals("element 0 false", sd[0].asBoolean(), false);
		ensure_equals("element 1 true", sd[1].asBoolean(), true);
		ensure_equals("element 2 false", sd[2].asBoolean(), false);
		ensure_equals("element 3 true", sd[3].asBoolean(), true);
		LLSD map = sd[4];
		ensure_equals("element 4 type", map.type(), LLSD::TypeMap);
		ensure_equals("map foo type", map["foo"].type(), LLSD::TypeBoolean);
		ensure_equals("map foo value", map["foo"].asBoolean(), true);
		ensure_equals("map bar type", map["bar"].type(), LLSD::TypeBoolean);
		ensure_equals("map bar value", map["bar"].asBoolean(), false);
	}

/*
	template<> template<>
	void sd_object::test<16>()
	{
	}
*/
}

#if 0
'{\'task_id\':u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t00082000\n\t\tcreator_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t00000000-0000-0000-0000-000000000000\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t10284\n\ttotal_crc\t35\n\ttype\t1\n\ttask_valid\t2\n\ttravel_access\t21\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t0\t0\t0\n\toldpos\t0\t0\t0\n\trotation\t4.371139183945160766597837e-08\t1\t4.371139183945160766597837e-08\t0\n\tvelocity\t0\t0\t0\n\tangvel\t0\t0\t0\n\tscale\t0.2816932\t0.2816932\t0.2816932\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t80\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t6\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t1132625972249870\n\tbirthtime\t1132625953120694\n\treztime\t1132625953120694\n\tparceltime\t1132625953120694\n\ttax_rate\t1.01615\n\tnamevalue\tAttachmentOrientation VEC3 RW DS -3.141593, 0.000000, -3.141593\n\tnamevalue\tAttachmentOffset VEC3 RW DS 0.000000, 0.000000, 0.000000\n\tnamevalue\tAttachPt U32 RW S 5\n\tnamevalue\tAttachItemID STRING RW SV 1f9975c0-2951-1b93-dd83-46e2b932fcc8\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\torig_asset_id\t52019cdd-b464-ba19-e66d-3da751fef9da\n\torig_item_id\t1f9975c0-2951-1b93-dd83-46e2b932fcc8\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n'
#endif

namespace tut
{
	struct mem_data
	{
	};
	typedef test_group<mem_data> mem_test;
	typedef mem_test::object mem_object;
	tut::mem_test mem_stream("LLMemoryStream");

	template<> template<>
	void mem_object::test<1>()
	{
		const char HELLO_WORLD[] = "hello world";
		LLMemoryStream mem((U8*)&HELLO_WORLD[0], strlen(HELLO_WORLD));		/* Flawfinder: ignore */
		std::string hello;
		std::string world;
		mem >> hello >> world;
		ensure_equals("first word", hello, std::string("hello"));
		ensure_equals("second word", world, std::string("world"));
	}
}

namespace tut
{
	// llstrtou64 
	// seems to be deprecated - could not find it being used 
	// anywhere in the tarball - skipping unit tests for now
}