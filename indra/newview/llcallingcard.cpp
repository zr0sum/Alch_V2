/** 
 * @file llcallingcard.cpp
 * @brief Implementation of the LLPreviewCallingCard class
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

#include "llviewerprecompiledheaders.h"

#include "llcallingcard.h"

#include <algorithm>

#include "llsingleton.h"
#include "llstl.h"
#include "lltimer.h"
#include "lluuid.h"
#include "message.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llinventoryobserver.h"
#include "llinventorymodel.h"
#include "llnotifications.h"
#include "llslurl.h"
#include "llimview.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llavataractions.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

class LLTrackingData
{
public:
	LLTrackingData(const LLUUID& avatar_id, const std::string& name);
	bool haveTrackingInfo();
	void setTrackedCoarseLocation(const LLVector3d& global_pos);
	void agentFound(const LLUUID& prey,
					const LLVector3d& estimated_global_pos);
	
public:
	LLUUID mAvatarID;
	std::string mName;
	LLVector3d mGlobalPositionEstimate;
	bool mHaveInfo;
	bool mHaveCoarseInfo;
	LLTimer mCoarseLocationTimer;
	LLTimer mUpdateTimer;
	LLTimer mAgentGone;
};

const F32 COARSE_FREQUENCY = 2.2f;
const F32 FIND_FREQUENCY = 29.7f;	// This results in a database query, so cut these back
const F32 OFFLINE_SECONDS = FIND_FREQUENCY + 8.0f;

static void on_avatar_name_cache_notify(const LLUUID& agent_id,
										const LLAvatarName& av_name,
										bool online,
										LLSD payload);

///----------------------------------------------------------------------------
/// Class LLAvatarTracker
///----------------------------------------------------------------------------

LLAvatarTracker::LLAvatarTracker() :
	mTrackingData(nullptr),
	mTrackedAgentValid(false),
	mModifyMask(0x0),
	mIsNotifyObservers(FALSE)
{
}

LLAvatarTracker::~LLAvatarTracker()
{
	deleteTrackingData();
	std::for_each(mObservers.begin(), mObservers.end(), DeletePointer());
	mObservers.clear();
	std::for_each(mBuddyInfo.begin(), mBuddyInfo.end(), DeletePairedPointer());
	mBuddyInfo.clear();
}

void LLAvatarTracker::track(const LLUUID& avatar_id, const std::string& name)
{
	deleteTrackingData();
	mTrackedAgentValid = false;
	mTrackingData = new LLTrackingData(avatar_id, name);
	findAgent();

	// We track here because findAgent() is called on a timer (for now).
	if(avatar_id.notNull())
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_TrackAgent);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TargetData);
		msg->addUUIDFast(_PREHASH_PreyID, avatar_id);
		gAgent.sendReliableMessage();
	}
}

void LLAvatarTracker::untrack(const LLUUID& avatar_id)
{
	if (mTrackingData && mTrackingData->mAvatarID == avatar_id)
	{
		deleteTrackingData();
		mTrackedAgentValid = false;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_TrackAgent);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TargetData);
		msg->addUUIDFast(_PREHASH_PreyID, LLUUID::null);
		gAgent.sendReliableMessage();
	}
}

void LLAvatarTracker::setTrackedCoarseLocation(const LLVector3d& global_pos)
{
	if(mTrackingData)
	{
		mTrackingData->setTrackedCoarseLocation(global_pos);
	}
}

bool LLAvatarTracker::haveTrackingInfo() const
{
	if(mTrackingData)
	{
		return mTrackingData->haveTrackingInfo();
	}
	return false;
}

LLVector3d LLAvatarTracker::getGlobalPos() const
{
	if(!mTrackedAgentValid || !mTrackingData) return LLVector3d();
	LLVector3d global_pos;
	
	LLViewerObject* object = gObjectList.findObject(mTrackingData->mAvatarID);
	if(object && !object->isDead())
	{
		global_pos = object->getPositionGlobal();
		// HACK - for making the tracker point above the avatar's head
		// rather than its groin
		LLVOAvatar* av = (LLVOAvatar*)object;
		global_pos.mdV[VZ] += 0.7f * (av->mBodySize.mV[VZ] + av->mAvatarOffset.mV[VZ]);

		mTrackingData->mGlobalPositionEstimate = global_pos;
	}
	else
	{
		global_pos = mTrackingData->mGlobalPositionEstimate;
	}

	return global_pos;
}

void LLAvatarTracker::getDegreesAndDist(F32& rot,
										F64& horiz_dist,
										F64& vert_dist) const
{
	if(!mTrackingData) return;

	LLVector3d global_pos;

	LLViewerObject* object = gObjectList.findObject(mTrackingData->mAvatarID);
	if(object && !object->isDead())
	{
		global_pos = object->getPositionGlobal();
		mTrackingData->mGlobalPositionEstimate = global_pos;
	}
	else
	{
		global_pos = mTrackingData->mGlobalPositionEstimate;
	}
	LLVector3d to_vec = global_pos - gAgent.getPositionGlobal();
	horiz_dist = sqrt(to_vec.mdV[VX] * to_vec.mdV[VX] + to_vec.mdV[VY] * to_vec.mdV[VY]);
	vert_dist = to_vec.mdV[VZ];
	rot = F32(RAD_TO_DEG * atan2(to_vec.mdV[VY], to_vec.mdV[VX]));
}

const std::string& LLAvatarTracker::getName() const
{
	return mTrackingData ? mTrackingData->mName : LLStringUtil::null;
}

const LLUUID& LLAvatarTracker::getAvatarID() const
{
	return mTrackingData ? mTrackingData->mAvatarID : LLUUID::null;
}

S32 LLAvatarTracker::addBuddyList(const buddy_map_t& buds)
{
	U32 new_buddy_count = 0;
	LLUUID agent_id;
	for (const auto& bud : buds)
    {
		agent_id = bud.first;
		auto existing_buddy = mBuddyInfo.find(agent_id);
		if(existing_buddy == mBuddyInfo.end())
		{
			++new_buddy_count;
			auto buddy = bud.second;
			/* auto it = */mBuddyInfo.emplace(agent_id, buddy);

			// pre-request name for notifications?
			LLAvatarName av_name;
			LLAvatarNameCache::get(agent_id, &av_name);

			addChangedMask(LLFriendObserver::ADD, agent_id);
			LL_DEBUGS() << "Added buddy " << agent_id
					<< ", " << (buddy->isOnline() ? "Online" : "Offline")
					<< ", TO: " << buddy->getRightsGrantedTo()
					<< ", FROM: " << buddy->getRightsGrantedFrom()
					<< LL_ENDL;
		}
		else
		{
			LLRelationship* e_r = (*existing_buddy).second;
			LLRelationship* n_r = bud.second;
			LL_WARNS() << "!! Add buddy for existing buddy: " << agent_id
					<< " [" << (e_r->isOnline() ? "Online" : "Offline") << "->" << (n_r->isOnline() ? "Online" : "Offline")
					<< ", " <<  e_r->getRightsGrantedTo() << "->" << n_r->getRightsGrantedTo()
					<< ", " <<  e_r->getRightsGrantedTo() << "->" << n_r->getRightsGrantedTo()
					<< "]" << LL_ENDL;
		}
	}
	// do not notify observers here - list can be large so let it be done on idle.
	
	return new_buddy_count;
}


void LLAvatarTracker::copyBuddyList(buddy_map_t& buddies) const
{
	if(!mBuddyInfo.empty())
		buddies.insert(mBuddyInfo.begin(), mBuddyInfo.end());
}

void LLAvatarTracker::terminateBuddy(const LLUUID& id)
{
	LL_DEBUGS() << "LLAvatarTracker::terminateBuddy()" << LL_ENDL;
	LLRelationship* buddy = get_ptr_in_map(mBuddyInfo, id);
	if(!buddy) return;
	mBuddyInfo.erase(id);
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("TerminateFriendship");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("ExBlock");
	msg->addUUID("OtherID", id);
	gAgent.sendReliableMessage();
	 
	addChangedMask(LLFriendObserver::REMOVE, id);
	delete buddy;
}

// get all buddy info
const LLRelationship* LLAvatarTracker::getBuddyInfo(const LLUUID& id) const
{
	if(id.isNull()) return nullptr;
	return get_ptr_in_map(mBuddyInfo, id);
}

bool LLAvatarTracker::isBuddy(const LLUUID& id) const
{
	return (mBuddyInfo.find(id) != mBuddyInfo.end());
}

// online status
void LLAvatarTracker::setBuddyOnline(const LLUUID& id, bool is_online)
{
	LLRelationship* info = get_ptr_in_map(mBuddyInfo, id);
	if(info)
	{
		info->online(is_online);
		addChangedMask(LLFriendObserver::ONLINE, id);
		LL_DEBUGS() << "Set buddy " << id << (is_online ? " Online" : " Offline") << LL_ENDL;
	}
	else
	{
		LL_WARNS() << "!! No buddy info found for " << id 
				<< ", setting to " << (is_online ? "Online" : "Offline") << LL_ENDL;
	}
}

bool LLAvatarTracker::isBuddyOnline(const LLUUID& id) const
{
	LLRelationship* info = get_ptr_in_map(mBuddyInfo, id);
	if(info)
	{
		return info->isOnline();
	}
	return false;
}

// empowered status
void LLAvatarTracker::setBuddyEmpowered(const LLUUID& id, bool is_empowered)
{
	LLRelationship* info = get_ptr_in_map(mBuddyInfo, id);
	if(info)
	{
		info->grantRights(LLRelationship::GRANT_MODIFY_OBJECTS, 0);
		mModifyMask |= LLFriendObserver::POWERS;
	}
}

bool LLAvatarTracker::isBuddyEmpowered(const LLUUID& id) const
{
	LLRelationship* info = get_ptr_in_map(mBuddyInfo, id);
	if(info)
	{
		return info->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS);
	}
	return false;
}

void LLAvatarTracker::empower(const LLUUID& id, bool grant)
{
	// wrapper for ease of use in some situations.
	/*
	buddy_map_t list;
	list.insert(id);
	empowerList(list, grant);
	*/
}

void LLAvatarTracker::empowerList(const buddy_map_t& list, bool grant)
{
	LL_WARNS() << "LLAvatarTracker::empowerList() not implemented." << LL_ENDL;
/*
	LLMessageSystem* msg = gMessageSystem;
	const char* message_name;
	const char* block_name;
	const char* field_name;
	if(grant)
	{
		message_name = _PREHASH_GrantModification;
		block_name = _PREHASH_EmpoweredBlock;
		field_name = _PREHASH_EmpoweredID;
	}
	else
	{
		message_name = _PREHASH_RevokeModification;
		block_name = _PREHASH_RevokedBlock;
		field_name = _PREHASH_RevokedID;
	}

	std::string name;
	gAgent.buildFullnameAndTitle(name);

	bool start_new_message = true;
	buddy_list_t::const_iterator it = list.begin();
	buddy_list_t::const_iterator end = list.end();
	for(; it != end; ++it)
	{
		if(NULL == get_ptr_in_map(mBuddyInfo, (*it))) continue;
		setBuddyEmpowered((*it), grant);
		if(start_new_message)
		{
			start_new_message = false;
			msg->newMessageFast(message_name);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->addStringFast(_PREHASH_GranterName, name);
		}
		msg->nextBlockFast(block_name);
		msg->addUUIDFast(field_name, (*it));
		if(msg->isSendFullFast(block_name))
		{
			start_new_message = true;
			gAgent.sendReliableMessage();
		}
	}
	if(!start_new_message)
	{
		gAgent.sendReliableMessage();
	}
*/
}

void LLAvatarTracker::deleteTrackingData()
{
	//make sure mTrackingData never points to freed memory
	LLTrackingData* tmp = mTrackingData;
	mTrackingData = nullptr;
	delete tmp;
}

void LLAvatarTracker::findAgent() const
{
	if (!mTrackingData) return;
	if (mTrackingData->mAvatarID.isNull()) return;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_FindAgent); // Request
	msg->nextBlockFast(_PREHASH_AgentBlock);
	msg->addUUIDFast(_PREHASH_Hunter, gAgentID);
	msg->addUUIDFast(_PREHASH_Prey, mTrackingData->mAvatarID);
	msg->addIPAddrFast(_PREHASH_SpaceIP, 0); // will get filled in by simulator // <alchemy/>
	msg->nextBlockFast(_PREHASH_LocationBlock);
	const F64 NO_LOCATION = 0.0;
	msg->addF64Fast(_PREHASH_GlobalX, NO_LOCATION);
	msg->addF64Fast(_PREHASH_GlobalY, NO_LOCATION);
	gAgent.sendReliableMessage();
}

void LLAvatarTracker::addObserver(LLFriendObserver* observer)
{
	if(observer)
	{
		mObservers.push_back(observer);
	}
}

void LLAvatarTracker::removeObserver(LLFriendObserver* observer)
{
	mObservers.erase(
		std::remove(mObservers.begin(), mObservers.end(), observer),
		mObservers.end());
}

void LLAvatarTracker::idleNotifyObservers()
{
	if (mModifyMask == LLFriendObserver::NONE && mChangedBuddyIDs.empty())
	{
		return;
	}
	notifyObservers();
}

void LLAvatarTracker::notifyObservers()
{
	if (mIsNotifyObservers)
	{
		// Don't allow multiple calls.
		// new masks and ids will be processed later from idle.
		return;
	}
	mIsNotifyObservers = TRUE;

	auto observers(mObservers);
	for (auto friend_observer : observers)
	{
		friend_observer->changed(mModifyMask);
	}

	for (auto buddy_id : mChangedBuddyIDs)
	{
		notifyParticularFriendObservers(buddy_id);
	}

	mModifyMask = LLFriendObserver::NONE;
	mChangedBuddyIDs.clear();
	mIsNotifyObservers = FALSE;
}

void LLAvatarTracker::addParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer)
{
	if (buddy_id.notNull() && observer)
		mParticularFriendObserverMap[buddy_id].insert(observer);
}

void LLAvatarTracker::removeParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer)
{
	if (buddy_id.isNull() || !observer)
		return;

    observer_map_t::iterator obs_it = mParticularFriendObserverMap.find(buddy_id);
    if(obs_it == mParticularFriendObserverMap.end())
        return;

    obs_it->second.erase(observer);

    // purge empty sets from the map
    if (obs_it->second.empty())
    	mParticularFriendObserverMap.erase(obs_it);
}

void LLAvatarTracker::notifyParticularFriendObservers(const LLUUID& buddy_id)
{
    observer_map_t::iterator obs_it = mParticularFriendObserverMap.find(buddy_id);
    if(obs_it == mParticularFriendObserverMap.end())
        return;

    // Notify observers interested in buddy_id.
    observer_set_t& obs = obs_it->second;
    for (auto ob : obs)
    {
        ob->changed(mModifyMask);
    }
}

// store flag for change
// and id of object change applies to
void LLAvatarTracker::addChangedMask(U32 mask, const LLUUID& referent)
{
	mModifyMask |= mask; 
	if (referent.notNull())
	{
		mChangedBuddyIDs.insert(referent);
	}
}

void LLAvatarTracker::applyFunctor(LLRelationshipFunctor& f)
{
	for(auto& pair : mBuddyInfo)
	{
		f(pair.first, pair.second);
	}
}

void LLAvatarTracker::registerCallbacks(LLMessageSystem* msg)
{
	msg->setHandlerFuncFast(_PREHASH_FindAgent, processAgentFound);
	msg->setHandlerFuncFast(_PREHASH_OnlineNotification,
						processOnlineNotification);
	msg->setHandlerFuncFast(_PREHASH_OfflineNotification,
						processOfflineNotification);
	//msg->setHandlerFuncFast(_PREHASH_GrantedProxies,
	//					processGrantedProxies);
	msg->setHandlerFuncFast(_PREHASH_TerminateFriendship, processTerminateFriendship);
	msg->setHandlerFuncFast(_PREHASH_ChangeUserRights, processChangeUserRights);
}

// static
void LLAvatarTracker::processAgentFound(LLMessageSystem* msg, void**)
{
	LLUUID id;
	
	msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_Hunter, id);
	msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_Prey, id);
	// *FIX: should make sure prey id matches.
	LLVector3d estimated_global_pos;
	msg->getF64Fast(_PREHASH_LocationBlock, _PREHASH_GlobalX,
				 estimated_global_pos.mdV[VX]);
	msg->getF64Fast(_PREHASH_LocationBlock, _PREHASH_GlobalY,
				 estimated_global_pos.mdV[VY]);
	LLAvatarTracker::instance().agentFound(id, estimated_global_pos);
}

void LLAvatarTracker::agentFound(const LLUUID& prey,
								 const LLVector3d& estimated_global_pos)
{
	if(!mTrackingData) return;
	//if we get a valid reply from the server, that means the agent
	//is our friend and mappable, so enable interest list based updates
	setTrackedAgentValid(true);
	mTrackingData->agentFound(prey, estimated_global_pos);
}

// 	static
void LLAvatarTracker::processOnlineNotification(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLAvatarTracker::processOnlineNotification()" << LL_ENDL;
	LLAvatarTracker::instance().processNotify(msg, true);
}

// 	static
void LLAvatarTracker::processOfflineNotification(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLAvatarTracker::processOfflineNotification()" << LL_ENDL;
	LLAvatarTracker::instance().processNotify(msg, false);
}

void LLAvatarTracker::processChange(LLMessageSystem* msg)
{
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_Rights);
	LLUUID agent_id, agent_related;
	S32 new_rights;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	for(auto i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_Rights, _PREHASH_AgentRelated, agent_related, i);
		msg->getS32Fast(_PREHASH_Rights,_PREHASH_RelatedRights, new_rights, i);
		if(agent_id == gAgent.getID())
		{
			auto buddy_it = mBuddyInfo.find(agent_related);
			if(buddy_it != mBuddyInfo.end())
			{
				
				buddy_it->second->setRightsTo(new_rights);
			}
		}
		else
		{
			auto buddy_it = mBuddyInfo.find(agent_id);
			if(mBuddyInfo.find(agent_id) != mBuddyInfo.end())
			{
				S32 change = buddy_it->second->getRightsGrantedFrom() ^ new_rights;
				if (change)
				{
					LLSD args = LLSD().with("NAME", LLSLURL("agent", agent_id, "displayname").getSLURLString());
					LLSD payload = LLSD().with("from_id", agent_id);
					if (change & LLRelationship::GRANT_MODIFY_OBJECTS)
					{
						LLNotifications::instance().add(LLRelationship::GRANT_MODIFY_OBJECTS & new_rights
							? "GrantedModifyRights" : "RevokedModifyRights", args, payload);
					}
					if (change & LLRelationship::GRANT_MAP_LOCATION)
					{
						LLNotifications::instance().add(LLRelationship::GRANT_MAP_LOCATION & new_rights
							? "GrantedMapRights" : "RevokedMapRights", args, payload);
					}
				}
				buddy_it->second->setRightsFrom(new_rights);
			}
		}
	}

	addChangedMask(LLFriendObserver::POWERS, agent_id);
	notifyObservers();
}

void LLAvatarTracker::processChangeUserRights(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLAvatarTracker::processChangeUserRights()" << LL_ENDL;
	LLAvatarTracker::instance().processChange(msg);
}

void LLAvatarTracker::processNotify(LLMessageSystem* msg, bool online)
{
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_AgentBlock);
	BOOL chat_notify = gSavedSettings.getBOOL("ChatOnlineNotification");

	LL_DEBUGS() << "Received " << count << " online notifications **** " << LL_ENDL;
	if(count > 0)
	{
		LLUUID agent_id;
		const LLRelationship* info = nullptr;
		LLUUID tracking_id;
		if(mTrackingData)
		{
			tracking_id = mTrackingData->mAvatarID;
		}
		LLSD payload;
		for(S32 i = 0; i < count; ++i)
		{
			msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_AgentID, agent_id, i);
			payload["FROM_ID"] = agent_id;
			info = getBuddyInfo(agent_id);
			if(info)
			{
				setBuddyOnline(agent_id,online);
			}
			else
			{
				LL_WARNS() << "Received online notification for unknown buddy: " 
					<< agent_id << " is " << (online ? "ONLINE" : "OFFLINE") << LL_ENDL;
			}

			if(tracking_id == agent_id)
			{
				// we were tracking someone who went offline
				deleteTrackingData();
			}
			// *TODO: get actual inventory id
			gInventory.addChangedMask(LLInventoryObserver::CALLING_CARD, LLUUID::null);

			if (chat_notify)
			{
				// Look up the name of this agent for the notification
				LLAvatarNameCache::get(agent_id, boost::bind(&on_avatar_name_cache_notify, _1, _2, online, payload));
			}
			addChangedMask(LLFriendObserver::ONLINE, agent_id);
		}

		notifyObservers();
		gInventory.notifyObservers();
	}
}

static void on_avatar_name_cache_notify(const LLUUID& agent_id,
										const LLAvatarName& av_name,
										bool online,
										LLSD payload)
{
	// Popup a notify box with online status of this agent
	// Use display name only because this user is your friend
	LLSD args;
	args["NAME"] = av_name.getDisplayName();
	args["STATUS"] = online ? LLTrans::getString("OnlineStatus") : LLTrans::getString("OfflineStatus");

	LLNotificationPtr notification;
	if (online)
	{
		notification =
			LLNotifications::instance().add("FriendOnlineOffline",
									 args,
									 payload.with("respond_on_mousedown", TRUE),
									 boost::bind(&LLAvatarActions::startIM, agent_id));
	}
	else
	{
		notification =
			LLNotifications::instance().add("FriendOnlineOffline", args, payload);
	}

	// If there's an open IM session with this agent, send a notification there too.
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, agent_id);
	std::string notify_msg = notification->getMessage();
	LLIMModel::instance().proccessOnlineOfflineNotification(session_id, notify_msg);
}

void LLAvatarTracker::formFriendship(const LLUUID& id)
{
	if(id.notNull())
	{
		if(mBuddyInfo.find(id) == mBuddyInfo.end())
		{
			//The default for relationship establishment is to have both parties
			//visible online to each other.
			auto buddy_info = new LLRelationship(LLRelationship::GRANT_ONLINE_STATUS,LLRelationship::GRANT_ONLINE_STATUS, false);
			mBuddyInfo.emplace(id, buddy_info);
			addChangedMask(LLFriendObserver::ADD, id);
			notifyObservers();
		}
	}
}

void LLAvatarTracker::processTerminateFriendship(LLMessageSystem* msg, void**)
{
	LLUUID id;
	msg->getUUID("ExBlock", "OtherID", id);
	if(id.notNull())
	{
		LLAvatarTracker& at = LLAvatarTracker::instance();
		auto it = at.mBuddyInfo.find(id);
		if (it == at.mBuddyInfo.end()) return;
		auto buddy = it->second;
		at.mBuddyInfo.erase(it);
		at.addChangedMask(LLFriendObserver::REMOVE, id);
		delete buddy;
		at.notifyObservers();
	}
}

///----------------------------------------------------------------------------
/// Tracking Data
///----------------------------------------------------------------------------

LLTrackingData::LLTrackingData(const LLUUID& avatar_id, const std::string& name)
:	mAvatarID(avatar_id),
	mHaveInfo(false),
	mHaveCoarseInfo(false)
{
	mCoarseLocationTimer.setTimerExpirySec(COARSE_FREQUENCY);
	mUpdateTimer.setTimerExpirySec(FIND_FREQUENCY);
	mAgentGone.setTimerExpirySec(OFFLINE_SECONDS);
	if(!name.empty())
	{
		mName = name;
	}
}

void LLTrackingData::agentFound(const LLUUID& prey,
								const LLVector3d& estimated_global_pos)
{
	if(prey != mAvatarID)
	{
		LL_WARNS() << "LLTrackingData::agentFound() - found " << prey
				<< " but looking for " << mAvatarID << LL_ENDL;
	}
	mHaveInfo = true;
	mAgentGone.setTimerExpirySec(OFFLINE_SECONDS);
	mGlobalPositionEstimate = estimated_global_pos;
}

bool LLTrackingData::haveTrackingInfo()
{
	LLViewerObject* object = gObjectList.findObject(mAvatarID);
	if(object && !object->isDead())
	{
		mCoarseLocationTimer.checkExpirationAndReset(COARSE_FREQUENCY);
		mUpdateTimer.setTimerExpirySec(FIND_FREQUENCY);
		mAgentGone.setTimerExpirySec(OFFLINE_SECONDS);
		mHaveInfo = true;
		return true;
	}
	if(mHaveCoarseInfo &&
	   !mCoarseLocationTimer.checkExpirationAndReset(COARSE_FREQUENCY))
	{
		// if we reach here, then we have a 'recent' coarse update
		mUpdateTimer.setTimerExpirySec(FIND_FREQUENCY);
		mAgentGone.setTimerExpirySec(OFFLINE_SECONDS);
		return true;
	}
	if(mUpdateTimer.checkExpirationAndReset(FIND_FREQUENCY))
	{
		LLAvatarTracker::instance().findAgent();
		mHaveCoarseInfo = false;
	}
	if(mAgentGone.checkExpirationAndReset(OFFLINE_SECONDS))
	{
		mHaveInfo = false;
		mHaveCoarseInfo = false;
	}
	return mHaveInfo;
}

void LLTrackingData::setTrackedCoarseLocation(const LLVector3d& global_pos)
{
	mCoarseLocationTimer.setTimerExpirySec(COARSE_FREQUENCY);
	mGlobalPositionEstimate = global_pos;
	mHaveInfo = true;
	mHaveCoarseInfo = true;
}

///----------------------------------------------------------------------------
// various buddy functors
///----------------------------------------------------------------------------

bool LLCollectProxyBuddies::operator()(const LLUUID& buddy_id, LLRelationship* buddy)
{
	if(buddy->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS))
	{
		mProxy.insert(buddy_id);
	}
	return true;
}

bool LLCollectMappableBuddies::operator()(const LLUUID& buddy_id, LLRelationship* buddy)
{
	if(buddy->isOnline() && buddy->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION))
	{
		LLAvatarName av_name;
		LLAvatarNameCache::get(buddy_id, &av_name);
		mMappable.emplace(buddy_id, av_name.getDisplayName());
	}
	return true;
}

bool LLCollectOnlineBuddies::operator()(const LLUUID& buddy_id, LLRelationship* buddy)
{
	LLAvatarName av_name;
	LLAvatarNameCache::get(buddy_id, &av_name);
	mFullName = av_name.getUserName();
	if(buddy->isOnline())
	{
		mOnline.emplace(buddy_id, mFullName);
	}
	return true;
}

bool LLCollectAllBuddies::operator()(const LLUUID& buddy_id, LLRelationship* buddy)
{
	LLAvatarName av_name;
	LLAvatarNameCache::get(buddy_id, &av_name);
	mFullName = av_name.getCompleteName();
	if(buddy->isOnline())
	{
		mOnline.emplace(buddy_id, mFullName);
	}
	else
	{
		mOffline.emplace(buddy_id, mFullName);
	}
	return true;
}