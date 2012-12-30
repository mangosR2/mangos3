/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Calendar.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectMgr.h"

INSTANTIATE_SINGLETON_1(CalendarMgr);

//////////////////////////////////////////////////////////////////////////
// CalendarEvent Class
//////////////////////////////////////////////////////////////////////////

CalendarEvent::~CalendarEvent()
{
    RemoveAllInvite();
}

// Add an invite to internal invite map return true if success
bool CalendarEvent::AddInvite(CalendarInvite* invite)
{
    if (!invite)
        return false;

    std::pair<CalendarInviteMap::iterator, bool> result;
    result = m_Invitee.insert(CalendarInviteMap::value_type(invite->InviteId, invite));
    return result.second;
}

CalendarInvite* CalendarEvent::GetInviteById(uint64 inviteId)
{
    CalendarInviteMap::iterator itr;
    itr = m_Invitee.find(inviteId);
    if (itr != m_Invitee.end())
        return itr->second;
    return NULL;
}

CalendarInvite* CalendarEvent::GetInviteByGuid( ObjectGuid const& guid )
{
    CalendarInviteMap::const_iterator inviteItr = m_Invitee.begin();
    while (inviteItr != m_Invitee.end())
    {
        if (inviteItr->second->InviteeGuid == guid)
            break;
        ++inviteItr;
    }

    if (inviteItr != m_Invitee.end())
    {
        return inviteItr->second;
    }
    return NULL;
}

CalendarInviteMap::iterator CalendarEvent::RemoveInviteByItr(CalendarInviteMap::iterator inviteItr)
{
    if (inviteItr != m_Invitee.end())
    {
        // TODO: check why only send alert if its not guild event
        if (!IsGuildEvent())
            sCalendarMgr.SendCalendarEventInviteRemoveAlert(inviteItr->second->InviteeGuid, this, CALENDAR_STATUS_REMOVED);

        sCalendarMgr.SendCalendarEventInviteRemove(inviteItr->second, Flags);
        delete inviteItr->second;
        return m_Invitee.erase(inviteItr);
    }
    return m_Invitee.end();
}

void CalendarEvent::RemoveInviteByGuid(ObjectGuid const& playerGuid)
{
    CalendarInviteMap::iterator itr = m_Invitee.begin();
    while (itr != m_Invitee.end())
    {
        if (itr->second->InviteeGuid == playerGuid)
        {
            itr = RemoveInviteByItr(itr);
        }
        else
            ++itr;
    }
}

bool CalendarEvent::RemoveInviteById(uint64 inviteId, ObjectGuid const& removerGuid)
{
    CalendarInviteMap::iterator inviteItr = m_Invitee.find(inviteId);
    if (inviteItr == m_Invitee.end())
    {
        // invite not found
        sCalendarMgr.SendCalendarCommandResult(removerGuid, CALENDAR_ERROR_NO_INVITE);
        return false;
    }

    // assign a pointer to CalendarInvite class to make read more easy
    CalendarInvite* invite = inviteItr->second;

    if (invite->InviteeGuid != removerGuid)
    {
        // check if remover is an invitee
        CalendarInvite* removerInvite = GetInviteByGuid(removerGuid);
        if (removerInvite == NULL)
        {
            // remover is not invitee cheat???
            sCalendarMgr.SendCalendarCommandResult(removerGuid, CALENDAR_ERROR_NOT_INVITED);
            return false;
        }

        if (removerInvite->Rank != CALENDAR_RANK_MODERATOR && removerInvite->Rank != CALENDAR_RANK_OWNER)
        {
            // remover have not enough right to remove invite
            sCalendarMgr.SendCalendarCommandResult(removerGuid, CALENDAR_ERROR_PERMISSIONS);
            return false;
        }
    }

    if (CreatorGuid == invite->InviteeGuid)
    {
        sCalendarMgr.SendCalendarCommandResult(removerGuid, CALENDAR_ERROR_DELETE_CREATOR_FAILED);
        return false;
    }

    // TODO: Send mail to invitee if needed

    RemoveInviteByItr(inviteItr);
    return true;
}

void CalendarEvent::RemoveAllInvite()
{
    CalendarInviteMap::iterator itr = m_Invitee.begin();
    while (itr != m_Invitee.end())
    {
        itr = RemoveInviteByItr(itr);
    }
}

//////////////////////////////////////////////////////////////////////////
// CalendarInvite Classes
//////////////////////////////////////////////////////////////////////////

CalendarInvite::CalendarInvite(CalendarEvent* event, uint64 inviteId, ObjectGuid senderGuid, ObjectGuid inviteeGuid, time_t statusTime, CalendarInviteStatus status, CalendarModerationRank rank, std::string text) :
m_calendarEvent(event), InviteId(inviteId), SenderGuid(senderGuid), InviteeGuid(inviteeGuid), LastUpdateTime(statusTime), Status(status), Rank(rank), Text(text)
{
    // only for pre invite case
    if (!event)
        InviteId = 0;
}

//////////////////////////////////////////////////////////////////////////
// CalendarMgr Classes
//////////////////////////////////////////////////////////////////////////

CalendarMgr::CalendarMgr() : m_MaxEventId(0),m_MaxInviteId(0)
{
    m_FreeEventIds.clear();
    m_FreeInviteIds.clear();
}

CalendarMgr::~CalendarMgr()
{
}

CalendarEventsList* CalendarMgr::GetPlayerEventsList(ObjectGuid const& guid)
{
    CalendarEventsList* events = new CalendarEventsList;

    uint32 guildId = 0;
    Player* player = sObjectMgr.GetPlayer(guid);
    if (player)
        guildId = player->GetGuildId();
    else
        guildId = Player::GetGuildIdFromDB(guid);

    for (CalendarEventStore::iterator itr = m_EventStore.begin(); itr != m_EventStore.end(); ++itr)
    {
        CalendarEvent* event = &itr->second;

        // add own event and same guild event or announcement
        if ((event->CreatorGuid == guid) || ((event->IsGuildAnnouncement() || event->IsGuildEvent()) && event->GuildId == guildId))
        {
            events->push_back(event);
            continue;
        }

        // add all event where player is invited
        if (event->GetInviteByGuid(guid))
            events->push_back(event);
    }
    return events;
}

CalendarInvitesList* CalendarMgr::GetPlayerInvitesList(ObjectGuid const& guid)
{
    CalendarInvitesList* invites = new CalendarInvitesList;

    uint32 guildId = 0;
    Player* player = sObjectMgr.GetPlayer(guid);
    if (player)
        guildId = player->GetGuildId();
    else
        guildId = Player::GetGuildIdFromDB(guid);

    for (CalendarEventStore::iterator itr = m_EventStore.begin(); itr != m_EventStore.end(); ++itr)
    {
        CalendarEvent* event = &itr->second;

        if (event->IsGuildAnnouncement())
            continue;

        CalendarInviteMap const* cInvMap = event->GetInviteMap();
        CalendarInviteMap::const_iterator ci_itr = cInvMap->begin();
        while (ci_itr != cInvMap->end())
        {
            if (ci_itr->second->InviteeGuid == guid)
            {
                invites->push_back(ci_itr->second);
                break;
            }
            ++ci_itr;
        }
    }
    return invites;
}

uint64 CalendarMgr::GetNewEventId()
{
    // actualy that method seem too complicated but its needed when DB part will be implemented
    if (m_FreeEventIds.empty())
        return ++m_MaxEventId;
    else
    {
        uint64 eventId = m_FreeEventIds.front();
        m_FreeEventIds.pop_front();
        return eventId;
    }
}

uint32 CalendarMgr::GetNewInviteId()
{
    // actualy that method seem too complicated but its needed when DB part will be implemented
    if (m_FreeInviteIds.empty())
        return ++m_MaxInviteId;
    else
    {
        uint64 inviteId = m_FreeInviteIds.front();
        m_FreeInviteIds.pop_front();
        return inviteId;
    }
}

CalendarEvent* CalendarMgr::AddEvent(ObjectGuid const& guid, std::string title, std::string description, uint32 type, uint32 repeatable,
    uint32 maxInvites, int32 dungeonId, time_t eventTime, time_t unkTime, uint32 flags)
{
    Player* player = sObjectMgr.GetPlayer(guid);
    if (!player)
        return NULL;

    if (title.empty())
    {
        SendCalendarCommandResult(guid, CALENDAR_ERROR_NEEDS_TITLE);
        return NULL;
    }

    if (eventTime < time(NULL))
    {
        SendCalendarCommandResult(guid, CALENDAR_ERROR_INVALID_DATE);
        return NULL;
    }

    uint64 nId = GetNewEventId();

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CalendarMgr::AddEvent> '%s', ID(%u), Desc > '%s', type=%u, repeat=%u, maxInvites=%u, dungeonId=%u,\netime=%s,\nutime=%s,\nflags=%u",
        title.c_str(), nId, description.c_str(), type, repeatable, maxInvites, dungeonId, secsToTimeString(eventTime - time(NULL)).c_str(), secsToTimeString(unkTime - time(NULL)).c_str(), flags);

    m_EventStore[nId].EventId = nId;
    m_EventStore[nId].CreatorGuid = guid;
    m_EventStore[nId].Title = title;
    m_EventStore[nId].Description = description;
    m_EventStore[nId].Type = (CalendarEventType) type;
    m_EventStore[nId].Repeatable = (CalendarRepeatType) repeatable;
    m_EventStore[nId].DungeonId = dungeonId;
    m_EventStore[nId].EventTime = eventTime;
    m_EventStore[nId].Flags = flags;

    if ((flags & CALENDAR_FLAG_GUILD_EVENT) || (flags && CALENDAR_FLAG_GUILD_ANNOUNCEMENT))
        m_EventStore[nId].GuildId = player->GetGuildId();

    return &m_EventStore[nId];
}

void CalendarMgr::RemoveEvent(uint64 eventId, ObjectGuid const& remover)
{
    CalendarEventStore::iterator citr = m_EventStore.find(eventId);
    if (citr == m_EventStore.end())
    {
        SendCalendarCommandResult(remover, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    SendCalendarEventRemovedAlert(&citr->second);

    m_EventStore.erase(citr);
}

// Add invit to an event and inform client
CalendarInvite* CalendarMgr::AddInvite(CalendarEvent* event, ObjectGuid const& senderGuid, ObjectGuid const& inviteeGuid, CalendarInviteStatus status, CalendarModerationRank rank, std::string text, time_t statusTime)
{
    if (!event)
        return NULL;

    CalendarInvite* calendarInvite = new CalendarInvite(event, GetNewInviteId(), senderGuid, inviteeGuid, statusTime, status, rank, text);

    if (!event->IsGuildAnnouncement())
        SendCalendarEventInvite(calendarInvite);

    if (!event->IsGuildEvent() || calendarInvite->InviteeGuid == event->CreatorGuid)
        SendCalendarEventInviteAlert(calendarInvite);

    if (event->IsGuildAnnouncement())
    {
        delete calendarInvite;
        return NULL;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "Add Invite> eventId[%u], senderGuid[%u], inviteGuid[%u], Status[%u], rank[%u], text[%s], time[%u]", (uint32)event->EventId, senderGuid.GetCounter(), inviteeGuid.GetCounter(), status, rank, text.c_str(), (uint32)statusTime);

    if (!event->AddInvite(calendarInvite))
    {
        sLog.outError("CalendarEvent::AddInvite > Fail adding invite!");
        delete calendarInvite;
        return NULL;
    }
    return calendarInvite;
}

bool CalendarMgr::RemoveInvite(uint32 eventId, uint32 invitId, ObjectGuid const& removerGuid)
{
    CalendarEventStore::iterator citr = m_EventStore.find(eventId);
    if (citr == m_EventStore.end())
    {
        SendCalendarCommandResult(removerGuid, CALENDAR_ERROR_EVENT_INVALID);
        return false;
    }

    CalendarEvent& event = citr->second;

    return event.RemoveInviteById(invitId, removerGuid);
}

uint32 CalendarMgr::GetPlayerNumPending(ObjectGuid const& guid)
{
    CalendarInvitesList* inviteList = GetPlayerInvitesList(guid);

    uint32 pendingNum = 0;
    for (CalendarInvitesList::const_iterator itr = inviteList->begin(); itr != inviteList->end(); ++itr)
    {
        CalendarEvent const* cal = (*itr)->GetCalendarEvent();

        if (cal && (cal->Flags & CALENDAR_FLAG_INVITES_LOCKED))
            continue;


        if ((*itr)->Status == CALENDAR_STATUS_INVITED || (*itr)->Status == CALENDAR_STATUS_TENTATIVE || (*itr)->Status == CALENDAR_STATUS_NOT_SIGNED_UP)
            ++pendingNum;
    }

    delete inviteList;
    return pendingNum;
}

void CalendarMgr::CopyEvent(uint64 eventId, time_t newTime, ObjectGuid const& guid)
{
    CalendarEvent* event = GetEventById(eventId);
    if (!event)
    {
        SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    CalendarEvent* newEvent = AddEvent(guid, event->Title, event->Description, event->Type, event->Repeatable,
        CALENDAR_MAX_INVITES, event->DungeonId, newTime, event->UnknownTime, event->Flags);

    if (!newEvent)
    {
        SendCalendarCommandResult(guid, CALENDAR_ERROR_INTERNAL);
        return;
    }

    if (newEvent->IsGuildAnnouncement())
        AddInvite(newEvent, guid, guid,  CALENDAR_STATUS_CONFIRMED, CALENDAR_RANK_OWNER, "", time(NULL));
    else
    {
        // copy all invitees, set new owner as the one who make the copy, set invitees status to invited
        CalendarInviteMap const* cInvMap = event->GetInviteMap();
        CalendarInviteMap::const_iterator ci_itr = cInvMap->begin();

        while (ci_itr != cInvMap->end())
        {
            if (ci_itr->second->InviteeGuid == guid)
            {
                AddInvite(newEvent, guid, ci_itr->second->InviteeGuid,  CALENDAR_STATUS_CONFIRMED, CALENDAR_RANK_OWNER, "", time(NULL));
            }
            else
            {
                CalendarModerationRank rank = CALENDAR_RANK_PLAYER;
                // copy moderator rank
                if (ci_itr->second->Rank == CALENDAR_RANK_MODERATOR)
                    rank = CALENDAR_RANK_MODERATOR;

                AddInvite(newEvent, guid, ci_itr->second->InviteeGuid,  CALENDAR_STATUS_INVITED, rank, "", time(NULL));
            }
            ++ci_itr;
        }
    }

    SendCalendarEvent(guid, newEvent, CALENDAR_SENDTYPE_COPY);
}

void CalendarMgr::RemovePlayerCalendar(ObjectGuid const& playerGuid)
{
    CalendarEventStore::iterator itr = m_EventStore.begin();

    while (itr != m_EventStore.end())
    {
        uint64 eventId = itr->first;
        if (itr->second.CreatorGuid == playerGuid)
        {
            // all invite will be automaticaly deleted
            m_EventStore.erase(eventId);
            // itr already incremented so go recheck event owner
            continue;
        }
        // event not owned by playerGuid but an invite can still be found
        CalendarEvent* event = &itr->second;
        event->RemoveInviteByGuid(playerGuid);
        ++itr;
    }
}

void CalendarMgr::RemoveGuildCalendar(ObjectGuid const& playerGuid, uint32 GuildId)
{
    CalendarEventStore::iterator itr = m_EventStore.begin();

    while (itr != m_EventStore.end())
    {
        CalendarEvent* event = &itr->second;
        uint64 eventId = itr->first;
        if (event->CreatorGuid == playerGuid && (event->IsGuildEvent()|| event->IsGuildAnnouncement()))
        {
            // all invite will be automaticaly deleted
            m_EventStore.erase(eventId);
            // itr already incremented so go recheck event owner
            continue;
        }
        // event not owned by playerGuid but an guild invite can still be found

        if (event->GuildId != GuildId || !(event->IsGuildEvent() || event->IsGuildAnnouncement()))
            continue;

        event->RemoveInviteByGuid(playerGuid);
        ++itr;
    }
}

void CalendarMgr::SendCalendarEventInviteAlert(CalendarInvite const* invite)
{
    DEBUG_LOG("WORLD: SMSG_CALENDAR_EVENT_INVITE_ALERT\n");

    CalendarEvent const* event = invite->GetCalendarEvent();

    if (!event)
        return;

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_ALERT);
    data << uint64(event->EventId);
    data << event->Title;
    data.AppendPackedTime(event->EventTime);
    data << uint32(event->Flags);
    data << uint32(event->Type);
    data << int32(event->DungeonId);
    data << uint64(invite->InviteId);
    data << uint8(invite->Status);
    data << uint8(invite->Rank);
    data << event->CreatorGuid.WriteAsPacked();
    data << invite->SenderGuid.WriteAsPacked();
    data.hexlike();

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SendCalendarInviteAlert> senderGuid[%u], inviteeGuid[%u], EventId[%lu], Status[%u], InviteId[%u]",
        invite->SenderGuid.GetCounter(), invite->InviteeGuid.GetCounter(), uint32(event->EventId), uint32(invite->Status), uint32(invite->InviteId));

    if (event->IsGuildEvent() || event->IsGuildAnnouncement())
    {
        if (Guild* guild = sGuildMgr.GetGuildById(event->GuildId))
            guild->BroadcastPacket(&data);
    }
    else if (Player* player = sObjectMgr.GetPlayer(invite->InviteeGuid))
        player->SendDirectMessage(&data);
}

void CalendarMgr::SendCalendarEventInvite(CalendarInvite const* invite)
{
    CalendarEvent const* event = invite->GetCalendarEvent();

    time_t statusTime = invite->LastUpdateTime;
    bool preInvite = true;
    uint64 eventId = 0;
    if (event != NULL)
    {
       preInvite = false;
       eventId = event->EventId;
    }

    Player* player = sObjectMgr.GetPlayer(invite->InviteeGuid);

    uint8 level = player ? player->getLevel() : Player::GetLevelFromDB(invite->InviteeGuid);
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_INVITE\n");
    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE, 8 + 8 + 8 + 1 + 1 + 1 + (preInvite ? 0 : 4) + 1);
    data << invite->InviteeGuid.WriteAsPacked();
    data << uint64(eventId);
    data << uint64(invite->InviteId);
    data << uint8(level);
    data << uint8(invite->Status);
    data << uint8(!preInvite);
    if (!preInvite)
        data.AppendPackedTime(statusTime);
    data << uint8(invite->SenderGuid != invite->InviteeGuid); // false only if the invite is sign-up

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SendCalendarInvit> %s senderGuid[%u], inviteeGuid[%u], EventId[%lu], Status[%u], InviteId[%u]",
        preInvite ? "is PreInvite," : "", invite->SenderGuid.GetCounter(), invite->InviteeGuid.GetCounter(), uint32(eventId), uint32(invite->Status), uint32(invite->InviteId));

    data.hexlike();
    if (preInvite)
    {
        if (Player* sender = sObjectMgr.GetPlayer(invite->SenderGuid))
            sender->SendDirectMessage(&data);
    }
    else
        SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendCalendarCommandResult(ObjectGuid const& guid, CalendarResponseResult err, char const* param /*= NULL*/)
{
    if (Player* player = sObjectMgr.GetPlayer(guid))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_COMMAND_RESULT (%u)\n", err);
        WorldPacket data(SMSG_CALENDAR_COMMAND_RESULT, 0);
        data << uint32(0);
        data << uint8(0);
        switch (err)
        {
        case CALENDAR_ERROR_OTHER_INVITES_EXCEEDED:
        case CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S:
        case CALENDAR_ERROR_IGNORING_YOU_S:
            data << param;
            break;
        default:
            data << uint8(0);
            break;
        }

        data << uint32(err);
        data.hexlike();
        player->SendDirectMessage(&data);
    }
}

void CalendarMgr::SendCalendarEventRemovedAlert(CalendarEvent const* event)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_REMOVED_ALERT\n");
    WorldPacket data(SMSG_CALENDAR_EVENT_REMOVED_ALERT, 1 + 8 + 1);
    data << uint8(1);       // show pending alert?
    data << uint64(event->EventId);
    data.AppendPackedTime(event->EventTime);
    data.hexlike();
    SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendCalendarEvent(ObjectGuid const& guid, CalendarEvent const* event, uint32 sendType)
{
    Player* player = sObjectMgr.GetPlayer(guid);
    if (!player || !event)
        return;

    std::string timeStr = TimeToTimestampStr(event->EventTime);
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SendCalendarEvent> sendType[%u], CreatorGuid[%u], EventId[%u], Type[%u], Flags[%u], Time[%s]",
        sendType, event->CreatorGuid.GetCounter(), uint32(event->EventId), uint32(event->Type), event->Flags, event->Title.c_str());

    WorldPacket data(SMSG_CALENDAR_SEND_EVENT);
    data << uint8(sendType);
    data << event->CreatorGuid.WriteAsPacked();
    data << uint64(event->EventId);
    data << event->Title;
    data << event->Description;
    data << uint8(event->Type);
    data << uint8(event->Repeatable);
    data << uint32(CALENDAR_MAX_INVITES);
    data << event->DungeonId;
    data << event->Flags;
    data.AppendPackedTime(event->EventTime);
    data.AppendPackedTime(event->UnknownTime);
    data << event->GuildId;

    CalendarInviteMap const* cInvMap = event->GetInviteMap();
    data << (uint32)cInvMap->size();
    for (CalendarInviteMap::const_iterator itr = cInvMap->begin(); itr != cInvMap->end(); ++itr)
    {
        CalendarInvite const* calendarInvite = itr->second;
        ObjectGuid inviteeGuid = calendarInvite->InviteeGuid;
        Player* invitee = sObjectMgr.GetPlayer(inviteeGuid);

        uint8 inviteeLevel = invitee ? invitee->getLevel() : Player::GetLevelFromDB(inviteeGuid);
        uint32 inviteeGuildId = invitee ? invitee->GetGuildId() : Player::GetGuildIdFromDB(inviteeGuid);

        data << inviteeGuid.WriteAsPacked();
        data << uint8(inviteeLevel);
        data << uint8(calendarInvite->Status);
        data << uint8(calendarInvite->Rank);
        data << uint8(event->IsGuildEvent() && event->GuildId == inviteeGuildId);
        data << uint64(itr->first);
        data.AppendPackedTime(calendarInvite->LastUpdateTime);
        data << calendarInvite->Text;

        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "Invite> InviteId[%u], InviteLvl[%u], Status[%u], Rank[%u],  GuildEvent[%s], Text[%s]",
            uint32(calendarInvite->InviteId), uint32(inviteeLevel), uint32(calendarInvite->Status), uint32(calendarInvite->Rank),
            (event->IsGuildEvent() && event->GuildId == inviteeGuildId) ? "true" : "false", calendarInvite->Text.c_str());
    }
    data.hexlike();
    player->SendDirectMessage(&data);
}

void CalendarMgr::SendCalendarEventInviteRemove(CalendarInvite const* invite, uint32 flags)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_INVITE_REMOVED\n");

    CalendarEvent const* event = invite->GetCalendarEvent();

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED, 8 + 4 + 4 + 1);
    data.appendPackGUID(invite->InviteeGuid);
    data << uint64(event->EventId);
    data << uint32(flags);
    data << uint8(1);       // show pending alert?
    data.hexlike();
    SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendCalendarEventInviteRemoveAlert(ObjectGuid const& guid, CalendarEvent const* event, CalendarInviteStatus status)
{
    if (Player* player = sObjectMgr.GetPlayer(guid))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT\n");
        WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT, 8 + 4 + 4 + 1);
        data << uint64(event->EventId);
        data.AppendPackedTime(event->EventTime);
        data << uint32(event->Flags);
        data << uint8(status);
        data.hexlike();
        player->SendDirectMessage(&data);
    }
}

void CalendarMgr::SendCalendarEventStatus(CalendarInvite const* invite)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_STATUS\n");
    WorldPacket data(SMSG_CALENDAR_EVENT_STATUS, 8 + 8 + 4 + 4 + 1 + 1 + 4);
    CalendarEvent const* event = invite->GetCalendarEvent();

    data << invite->InviteeGuid.WriteAsPacked();
    data << uint64(event->EventId);
    data.AppendPackedTime(event->EventTime);
    data << uint32(event->Flags);
    data << uint8(invite->Status);
    data << uint8(invite->Rank);
    data.AppendPackedTime(invite->LastUpdateTime);
    data.hexlike();
    SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendCalendarClearPendingAction(ObjectGuid const& guid)
{
    if (Player* player = sObjectMgr.GetPlayer(guid))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_CLEAR_PENDING_ACTION TO [%u]", guid.GetCounter());
        WorldPacket data(SMSG_CALENDAR_CLEAR_PENDING_ACTION, 0);
        player->SendDirectMessage(&data);
    }
}

void CalendarMgr::SendCalendarEventModeratorStatusAlert(CalendarInvite const* invite)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT\n");
    CalendarEvent const* event = invite->GetCalendarEvent();
    WorldPacket data(SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT, 8 + 8 + 1 + 1);
    data << invite->InviteeGuid.WriteAsPacked();
    data << uint64(event->EventId);
    data << uint8(invite->Rank);
    data << uint8(1); // Display pending action to client?
    data.hexlike();
    SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendCalendarEventUpdateAlert(CalendarEvent const* event, time_t oldEventTime)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "SMSG_CALENDAR_EVENT_UPDATED_ALERT\n");
    WorldPacket data(SMSG_CALENDAR_EVENT_UPDATED_ALERT, 1 + 8 + 4 + 4 + 4 + 1 + 4 +
        event->Title.size() + event->Description.size() + 1 + 4 + 4);
    data << uint8(1);       // show pending alert?
    data << uint64(event->EventId);
    data.AppendPackedTime(oldEventTime);
    data << uint32(event->Flags);
    data.AppendPackedTime(event->EventTime);
    data << uint8(event->Type);
    data << int32(event->DungeonId);
    data << event->Title;
    data << event->Description;
    data << uint8(event->Repeatable);
    data << uint32(CALENDAR_MAX_INVITES);
    data << uint32(0);      // title updated? (string type)
    data.hexlike();

    SendPacketToAllEventRelatives(data, event);
}

void CalendarMgr::SendPacketToAllEventRelatives(WorldPacket packet, CalendarEvent const* event)
{
    // Send packet to all guild members
    if (event->IsGuildEvent() || event->IsGuildAnnouncement())
        if (Guild* guild = sGuildMgr.GetGuildById(event->GuildId))
            guild->BroadcastPacket(&packet);

    // Send packet to all invitees if event is non-guild, in other case only to non-guild invitees (packet was broadcasted for them)
    CalendarInviteMap const* cInvMap = event->GetInviteMap();
    for (CalendarInviteMap::const_iterator itr = cInvMap->begin(); itr != cInvMap->end(); ++itr)
        if (Player* player = sObjectMgr.GetPlayer(itr->second->InviteeGuid))
            if (!event->IsGuildEvent() || (event->IsGuildEvent() && player->GetGuildId() != event->GuildId))
                player->SendDirectMessage(&packet);
}