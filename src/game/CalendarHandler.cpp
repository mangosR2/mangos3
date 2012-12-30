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

#include "Common.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "MapPersistentStateMgr.h"
#include "Calendar.h"
#include "SocialMgr.h"
#include "World.h"
#include "Guild.h"
#include "GuildMgr.h"

void WorldSession::HandleCalendarGetCalendar(WorldPacket& /*recv_data*/)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_GET_CALENDAR [%u]", guid.GetCounter());

    time_t currTime = time(NULL);

    WorldPacket data(SMSG_CALENDAR_SEND_CALENDAR);

    CalendarInvitesList* invites = NULL;
    invites = sCalendarMgr.GetPlayerInvitesList(guid);

    data << uint32(invites->size());
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "Sending > %u invites", invites->size());

    for (CalendarInvitesList::const_iterator itr = invites->begin(); itr != invites->end(); ++itr)
    {
        CalendarEvent const* calendarEvent = (*itr)->GetCalendarEvent();
        MANGOS_ASSERT(calendarEvent);                           // TODO: be sure no way to have a null event

        data << uint64(calendarEvent->EventId);
        data << uint64((*itr)->InviteId);
        data << uint8((*itr)->Status);
        data << uint8((*itr)->Rank);
        
        data << uint8(calendarEvent->IsGuildEvent());
        data << calendarEvent->CreatorGuid.WriteAsPacked();
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "invite> EventId[%u], InviteId[%u], status[%u], rank[%u]",
            uint32((*itr)->GetCalendarEvent()->EventId), uint32((*itr)->InviteId), uint32((*itr)->Status), uint32((*itr)->Rank));
    }
    delete invites;

    CalendarEventsList* events = sCalendarMgr.GetPlayerEventsList(guid);
    data << uint32(events->size());
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "Sending > %u events", events->size());

    for (CalendarEventsList::const_iterator itr = events->begin(); itr != events->end(); ++itr)
    {
        CalendarEvent const* calendarEvent = *itr;

        data << uint64(calendarEvent->EventId);
        data << calendarEvent->Title;
        data << uint32(calendarEvent->Type);
        data.AppendPackedTime(calendarEvent->EventTime);
        data << uint32(calendarEvent->Flags);
        data << int32(calendarEvent->DungeonId);
        data << calendarEvent->CreatorGuid.WriteAsPacked();

        std::string timeStr = TimeToTimestampStr(calendarEvent->EventTime);
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "Events> EventId[%u], Title[%s], Time[%s], Type[%u],  Flag[%u], DungeonId[%d], CreatorGuid[%u]",
                         uint32(calendarEvent->EventId), calendarEvent->Title.c_str(), timeStr.c_str(), uint32(calendarEvent->Type),
                         uint32(calendarEvent->Flags), calendarEvent->DungeonId, calendarEvent->CreatorGuid.GetCounter());
    }
    delete events;

    data.AppendPackedTime(currTime);                      // server time
    data.AppendPackedTime(currTime);                      // zone time ??

    ByteBuffer dataBuffer;
    uint32 boundCounter = 0;
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        Player::BoundInstancesMap boundInstances = _player->GetBoundInstances(Difficulty(i));
        for (Player::BoundInstancesMap::const_iterator itr = boundInstances.begin(); itr != boundInstances.end(); ++itr)
        {
            if (itr->second.perm)
            {
                DungeonPersistentState const* state = itr->second.state;
                dataBuffer << uint32(state->GetMapId());
                dataBuffer << uint32(state->GetDifficulty());
                dataBuffer << uint32(state->GetResetTime() - currTime);
                dataBuffer << uint64(state->GetInstanceId());   // instance save id as unique instance copy id
                ++boundCounter;
            }
        }
    }

    data << uint32(boundCounter);
    data.append(dataBuffer);

    data << uint32(1135753200);                                 // Constant date, unk (28.12.2005 07:00)

    // Reuse variables
    boundCounter = 0;
    std::set<uint32> sentMaps;
    dataBuffer.clear();

    // TODO: check why that don't work returning some imposible reset time
    /*DungeonResetScheduler::ResetTimeByMapDifficultyMap const& resets = sMapPersistentStateMgr.GetScheduler().GetResetTimeMap();

    for (DungeonResetScheduler::ResetTimeByMapDifficultyMap::const_iterator itr = resets.begin(); itr != resets.end(); ++itr)
    {
        uint32 mapId = PAIR32_LOPART(itr->first);
        if (sentMaps.find(mapId) != sentMaps.end())
            continue;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsRaid())
            continue;

        sentMaps.insert(mapId);

        dataBuffer << int32(mapId);
        //dataBuffer << int32(itr->second - currTime);

        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR,"Map [%u] will reset in %s", mapId, secsToTimeString(itr->second - currTime).c_str());

        dataBuffer << int32(600); //test value
        dataBuffer << int32(0); // Never seen anything else in sniffs - still unknown
        ++boundCounter;
    }*/

    data << uint32(boundCounter);
    data.append(dataBuffer);

    // TODO: Fix this, how we do know how many and what holidays to send?
    uint32 holidayCount = 0;
    data << uint32(holidayCount);
    /*for (uint32 i = 0; i < holidayCount; ++i)
    {
        HolidaysEntry const* holiday = sHolidaysStore.LookupEntry(666);

        data << uint32(holiday->Id);                        // m_ID
        data << uint32(holiday->Region);                    // m_region, might be looping
        data << uint32(holiday->Looping);                   // m_looping, might be region
        data << uint32(holiday->Priority);                  // m_priority
        data << uint32(holiday->CalendarFilterType);        // m_calendarFilterType

        for (uint8 j = 0; j < MAX_HOLIDAY_DATES; ++j)
            data << uint32(holiday->Date[j]);               // 26 * m_date -- WritePackedTime ?

        for (uint8 j = 0; j < MAX_HOLIDAY_DURATIONS; ++j)
            data << uint32(holiday->Duration[j]);           // 10 * m_duration

        for (uint8 j = 0; j < MAX_HOLIDAY_FLAGS; ++j)
            data << uint32(holiday->CalendarFlags[j]);      // 10 * m_calendarFlags

        data << holiday->TextureFilename;                   // m_textureFilename (holiday name)
    }*/

    data.hexlike();
    SendPacket(&data);
}

void WorldSession::HandleCalendarGetEvent(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GET_EVENT [%u]", guid.GetCounter());

    uint64 eventId;
    recv_data >> eventId;

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
        sCalendarMgr.SendCalendarEvent(_player->GetObjectGuid(), calendarEvent, CALENDAR_SENDTYPE_GET);
    else
        sCalendarMgr.SendCalendarCommandResult(_player->GetObjectGuid(), CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarGuildFilter(WorldPacket& recv_data)
{
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_GUILD_FILTER [%u]", _player->GetObjectGuid().GetCounter());

    uint32 minLevel;
    uint32 maxLevel;
    uint32 minRank;

    recv_data >> minLevel >> maxLevel >> minRank;

    if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId()))
        guild->MassInviteToEvent(this, minLevel, maxLevel, minRank);

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_GUILD_FILTER: Min level [%u], Max level [%u], Min rank [%u]", minLevel, maxLevel, minRank);
}

void WorldSession::HandleCalendarEventSignup(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_SIGNUP [%u]", guid.GetCounter());

    uint64 eventId;
    bool tentative;

    recv_data >> eventId;
    recv_data >> tentative; // uint8 == bool size in all compilator???
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_SIGNUP [%u] EventId [%u] Tentative %u", guid.GetCounter(), uint32(eventId), tentative);

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
    {
        if (calendarEvent->IsGuildEvent() && calendarEvent->GuildId != _player->GetGuildId())
        {
            sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD);
            return;
        }

        CalendarInviteStatus status = tentative ? CALENDAR_STATUS_TENTATIVE : CALENDAR_STATUS_SIGNED_UP;
        sCalendarMgr.AddInvite(calendarEvent, guid, guid, CalendarInviteStatus(status), CALENDAR_RANK_PLAYER, "", time(NULL));
        sCalendarMgr.SendCalendarClearPendingAction(guid);
    }
    else
        sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarArenaTeam(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_ARENA_TEAM");
    recv_data.hexlike();
    recv_data.read_skip<uint32>();                          // unk
}

void WorldSession::HandleCalendarAddEvent(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_ADD_EVENT [%u]", guid.GetCounter());

    std::string title;
    std::string description;
    uint8 type;
    uint8 repeatable;
    uint32 maxInvites;
    int32 dungeonId;
    uint32 eventPackedTime;
    uint32 unkPackedTime;
    uint32 flags;

    recv_data >> title;
    recv_data >> description;
    recv_data >> type;
    recv_data >> repeatable;
    recv_data >> maxInvites;
    recv_data >> dungeonId;
    eventPackedTime = recv_data.ReadPackedTime();
    unkPackedTime   = recv_data.ReadPackedTime();
    recv_data >> flags;

    // 946684800 is 01/01/2000 00:00:00 - default response time
    CalendarEvent* cal =  sCalendarMgr.AddEvent(_player->GetObjectGuid(), title, description, type, repeatable, maxInvites, dungeonId, eventPackedTime, unkPackedTime, flags);

    if (cal)
    {
        if (cal->IsGuildAnnouncement())
        {
            sCalendarMgr.AddInvite(cal, guid, ObjectGuid(uint64(0)),  CALENDAR_STATUS_NOT_SIGNED_UP, CALENDAR_RANK_PLAYER, "", time(NULL));
        }
        else
        {
            uint32 inviteCount;
            recv_data >> inviteCount;

            for (uint32 i = 0; i < inviteCount; ++i)
            {
                ObjectGuid invitee;
                uint8 status = 0;
                uint8 rank = 0;
                recv_data >> invitee.ReadAsPacked();
                recv_data >> status;
                recv_data >> rank;

                sCalendarMgr.AddInvite(cal, guid, invitee, CalendarInviteStatus(status), CalendarModerationRank(rank), "", time(NULL));
            }
        }
        sCalendarMgr.SendCalendarEvent(_player->GetObjectGuid(), cal, CALENDAR_SENDTYPE_ADD);
    }
}

void WorldSession::HandleCalendarUpdateEvent(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_UPDATE_EVENT [%u]", guid.GetCounter());

    time_t oldEventTime;
    uint64 eventId;
    uint64 inviteId;
    std::string title;
    std::string description;
    uint8 type;
    uint8 repetitionType;
    uint32 maxInvites;
    int32 dungeonId;
    uint32 eventPackedTime;
    uint32 UnknownPackedTime;
    uint32 flags;

    recv_data >> eventId >> inviteId >> title >> description >> type >> repetitionType >> maxInvites >> dungeonId;
    eventPackedTime   = recv_data.ReadPackedTime();
    UnknownPackedTime = recv_data.ReadPackedTime();
    recv_data >> flags;

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_UPDATE_EVENT [%u] EventId [%u], InviteId [%u] Title %s, Description %s, type %u "
        "Repeatable %u, MaxInvites %u, Dungeon ID %d, Flags %u", guid.GetCounter(), uint32(eventId), uint32(inviteId), title.c_str(),
        description.c_str(), uint32(type), uint32(repetitionType), maxInvites, dungeonId, flags);

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
    {
        if (guid != calendarEvent->CreatorGuid)
        {
            CalendarInvite* updaterInvite = calendarEvent->GetInviteByGuid(guid);
            if (updaterInvite == NULL)
            {
                sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_NOT_INVITED);
                return ;
            }

            if (updaterInvite->Rank != CALENDAR_RANK_MODERATOR)
            {
                // remover have not enough right to change invite status
                sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_PERMISSIONS);
                return;
            }
        }

        oldEventTime = calendarEvent->EventTime;

        calendarEvent->Type = CalendarEventType(type);
        calendarEvent->Flags = flags;
        calendarEvent->EventTime = eventPackedTime;
        calendarEvent->UnknownTime = UnknownPackedTime;
        calendarEvent->DungeonId = dungeonId;
        calendarEvent->Title = title;
        calendarEvent->Description = description;

        sCalendarMgr.SendCalendarEventUpdateAlert(calendarEvent, oldEventTime);
    }
    else
        sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarRemoveEvent(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_REMOVE_EVENT [%u]", guid.GetCounter());

    uint64 eventId;
    ObjectGuid creatorGuid;
    uint32 Flags;

    recv_data >> eventId;
    recv_data >> creatorGuid.ReadAsPacked();
    recv_data >> Flags;
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_REMOVE_EVENT [%u], creator [%u]",
        guid.GetCounter(), creatorGuid.GetCounter());
    sCalendarMgr.RemoveEvent(eventId, guid);
}

void WorldSession::HandleCalendarCopyEvent(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_COPY_EVENT [%u]", guid.GetCounter());

    uint64 eventId;
    uint64 inviteId;
    uint32 packedTime;

    recv_data >> eventId >> inviteId;
    packedTime = recv_data.ReadPackedTime();

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_COPY_EVENT [%u], EventId [%u] inviteId [%u]",
        guid.GetCounter(), uint32(eventId), uint32(inviteId));

    sCalendarMgr.CopyEvent(eventId, packedTime, guid);
}

void WorldSession::HandleCalendarEventInvite(WorldPacket& recv_data)
{
    ObjectGuid playerGuid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_INVITE [%u]", playerGuid.GetCounter());

    uint64 eventId;

    // TODO it seem its not inviteID but event->CreatorGuid
    uint64 inviteId;
    std::string name;
    bool isPreInvite;
    bool isGuildEvent;

    ObjectGuid inviteeGuid;
    uint32 inviteeTeam = 0;
    uint32 inviteeGuildId = 0;

    recv_data >> eventId >> inviteId >> name >> isPreInvite >> isGuildEvent;

    if (Player* player = sObjectAccessor.FindPlayerByName(name.c_str()))
    {
        // Invitee is online
        inviteeGuid = player->GetObjectGuid();
        inviteeTeam = player->GetTeam();
        inviteeGuildId = player->GetGuildId();
    }
    else
    {
        // Invitee offline, get data from database
        CharacterDatabase.escape_string(name);
        QueryResult* result = CharacterDatabase.PQuery("SELECT guid,race FROM characters WHERE name ='%s'", name.c_str());
        if (result)
        {
            Field* fields = result->Fetch();
            inviteeGuid = ObjectGuid(HIGHGUID_PLAYER, fields[0].GetUInt32());
            inviteeTeam = Player::TeamForRace(fields[1].GetUInt8());
            inviteeGuildId = Player::GetGuildIdFromDB(inviteeGuid);
            delete result;
        }
    }

    if (inviteeGuid.IsEmpty())
    {
        sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_PLAYER_NOT_FOUND);
        return;
    }

    if (_player->GetTeam() != inviteeTeam && !sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CALENDAR))
    {
        sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NOT_ALLIED);
        return;
    }

    if (QueryResult* result = CharacterDatabase.PQuery("SELECT flags FROM character_social WHERE guid = " UI64FMTD " AND friend = " UI64FMTD, inviteeGuid, playerGuid))
    {
        Field* fields = result->Fetch();
        if (fields[0].GetUInt8() & SOCIAL_FLAG_IGNORED)
        {
            sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_IGNORING_YOU_S, name.c_str());
            return;
        }
    }

    if (!isPreInvite)
    {
        if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
        {
            if (calendarEvent->IsGuildEvent() && calendarEvent->GuildId == inviteeGuildId)
            {
                // we can't invite guild members to guild events
                sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
                return;
            }

            sCalendarMgr.AddInvite(calendarEvent, playerGuid, inviteeGuid, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "", time(NULL));
        }
        else
            sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_EVENT_INVALID);
    }
    else
    {
        if (isGuildEvent && inviteeGuildId == _player->GetGuildId())
        {
            sCalendarMgr.SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
            return;
        }

        // create a temp invite to send it back to client
        CalendarInvite calendarInvite;
        calendarInvite.SenderGuid = playerGuid;
        calendarInvite.InviteeGuid = inviteeGuid;
        calendarInvite.Status = CALENDAR_STATUS_INVITED;
        calendarInvite.Rank = CALENDAR_RANK_PLAYER;
        calendarInvite.LastUpdateTime = time(NULL);

        sCalendarMgr.SendCalendarEventInvite(&calendarInvite);
        DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "PREINVITE> sender[%u], Invitee[%u]", playerGuid.GetCounter(), inviteeGuid.GetCounter());
    }
}

void WorldSession::HandleCalendarEventRsvp(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_RSVP [%u]", guid.GetCounter());

    uint64 eventId;
    uint64 inviteId;
    uint32 status;

    recv_data >> eventId >> inviteId >> status;
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_RSVP [" UI64FMTD"] EventId ["
        UI64FMTD "], InviteId [" UI64FMTD "], status %u\n", guid, eventId,
        inviteId, status);

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
    {
        // i think we still should be able to remove self from locked events
        if (status != CALENDAR_STATUS_REMOVED && calendarEvent->Flags & CALENDAR_FLAG_INVITES_LOCKED)
        {
            sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_LOCKED);
            return;
        }

        if (CalendarInvite* invite = calendarEvent->GetInviteById(inviteId))
        {
            if (invite->InviteeGuid != guid)
            {
                CalendarInvite* updaterInvite = calendarEvent->GetInviteByGuid(guid);
                if (updaterInvite == NULL)
                {
                    sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_NOT_INVITED);
                    return ;
                }

                if (updaterInvite->Rank != CALENDAR_RANK_MODERATOR && updaterInvite->Rank != CALENDAR_RANK_OWNER)
                {
                    // remover have not enough right to change invite status
                    sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_PERMISSIONS);
                    return;
                }
            }
            invite->Status = CalendarInviteStatus(status);
            invite->LastUpdateTime = time(NULL);

            sCalendarMgr.SendCalendarEventStatus(invite);
            sCalendarMgr.SendCalendarClearPendingAction(guid);
        }
        else
            sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_REMOVE_INVITE [%u]", guid.GetCounter());

    ObjectGuid invitee;
    uint64 eventId;
    uint64 ownerInviteId; // isn't it sender's inviteId? TODO: need more test to see what we can do with that value
    uint64 inviteId;

    recv_data >> invitee.ReadAsPacked();
    recv_data >> inviteId >> ownerInviteId >> eventId;

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_REMOVE_INVITE [%u] EventId [%u], ownerInviteId [%u], Invitee ([%u] id: [%u])",
        guid.GetCounter(), uint32(eventId), uint32(ownerInviteId), invitee.GetCounter(), uint32(inviteId));

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
        sCalendarMgr.RemoveInvite(eventId, inviteId, guid);
    else
        sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventStatus(WorldPacket& recv_data)
{
    ObjectGuid updaterGuid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_STATUS [%u]", updaterGuid.GetCounter());
    recv_data.hexlike();

    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint32 status;

    recv_data >> invitee.ReadAsPacked();
    recv_data >> eventId >> inviteId >> ownerInviteId >> status;
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_STATUS [%u] EventId [%u] ownerInviteId [%u], Invitee ([%u] id: [%u], status %u",
        updaterGuid.GetCounter(), uint32(eventId), uint32(ownerInviteId), invitee.GetCounter(), uint32(inviteId), status);

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
    {
        if (CalendarInvite* invite = calendarEvent->GetInviteById(inviteId))
        {
            if (invite->InviteeGuid != updaterGuid)
            {
                CalendarInvite* updaterInvite = calendarEvent->GetInviteByGuid(updaterGuid);
                if (updaterInvite == NULL)
                {
                    sCalendarMgr.SendCalendarCommandResult(updaterGuid, CALENDAR_ERROR_NOT_INVITED);
                    return ;
                }

                if (updaterInvite->Rank != CALENDAR_RANK_MODERATOR && updaterInvite->Rank != CALENDAR_RANK_OWNER)
                {
                    // remover have not enough right to change invite status
                    sCalendarMgr.SendCalendarCommandResult(updaterGuid, CALENDAR_ERROR_PERMISSIONS);
                    return;
                }
            }
            invite->Status = (CalendarInviteStatus)status;
            invite->LastUpdateTime = time(NULL);                                        // not sure if we should set response time when moderator changes invite status

            //sCalendarMgr.UpdateInvite(invite);
            sCalendarMgr.SendCalendarEventStatus(invite);
            sCalendarMgr.SendCalendarClearPendingAction(invitee);
        }
        else
            sCalendarMgr.SendCalendarCommandResult(updaterGuid, CALENDAR_ERROR_NO_INVITE);
    }
    else
        sCalendarMgr.SendCalendarCommandResult(updaterGuid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_MODERATOR_STATUS [%u]", guid.GetCounter());

    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint32 rank;

    recv_data >> invitee.ReadAsPacked();
    recv_data >> eventId >>  inviteId >> ownerInviteId >> rank;
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_MODERATOR_STATUS [%u] EventId [%u] ownerInviteId [%u], Invitee ([%u] id: [%u], rank %u",
        guid.GetCounter(), uint32(eventId), uint32(ownerInviteId), invitee.GetCounter(), uint32(inviteId), rank);

    if (CalendarEvent* calendarEvent = sCalendarMgr.GetEventById(eventId))
    {
        if (CalendarInvite* invite = calendarEvent->GetInviteById(inviteId))
        {
            if (invite->InviteeGuid != guid)
            {
                CalendarInvite* updaterInvite = calendarEvent->GetInviteByGuid(guid);
                if (updaterInvite == NULL)
                {
                    sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_NOT_INVITED);
                    return ;
                }
                if (updaterInvite->Rank != CALENDAR_RANK_MODERATOR && updaterInvite->Rank != CALENDAR_RANK_OWNER)
                {
                    // remover have not enough right to change invite status
                    sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_PERMISSIONS);
                    return;
                }
            }

            if (CalendarModerationRank(rank) == CALENDAR_RANK_OWNER)
            {
                 // cannot set owner
                 sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_PERMISSIONS);
                 return;
            }

            invite->Rank = CalendarModerationRank(rank);
            sCalendarMgr.SendCalendarEventModeratorStatusAlert(invite);
        }
        else
            sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
    }
    else
        sCalendarMgr.SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarComplain(WorldPacket& recv_data)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_COMPLAIN [%u]", guid.GetCounter());

    ObjectGuid badGuyGuid;
    uint64 eventId;
    uint64 inviteId;

    recv_data >> badGuyGuid;
    recv_data >> eventId >>  inviteId;
    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_EVENT_MODERATOR_STATUS [%u] EventId [%u], Invitee ([%u] id: [%u])",
        guid.GetCounter(), uint32(eventId), badGuyGuid.GetCounter(), uint32(inviteId));

    // Remove the invite
    if (sCalendarMgr.RemoveInvite(eventId, inviteId, guid))
    {
        WorldPacket data(SMSG_COMPLAIN_RESULT, 1 + 1);
        data << uint8(0);
        data << uint8(0); // show complain saved. We can send 0x0C to show windows with ok button
        SendPacket(&data);
    }
}

void WorldSession::HandleCalendarGetNumPending(WorldPacket& /*recv_data*/)
{
    ObjectGuid guid = _player->GetObjectGuid();
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GET_NUM_PENDING [%u]", guid.GetCounter());

    uint32 pending = sCalendarMgr.GetPlayerNumPending(guid);

    DEBUG_FILTER_LOG(LOG_FILTER_CALENDAR, "CMSG_CALENDAR_GET_NUM_PENDING: [" UI64FMTD
        "] Pending: %u\n", guid, pending);

    WorldPacket data(SMSG_CALENDAR_SEND_NUM_PENDING, 4);
    data << uint32(pending);
    SendPacket(&data);
}
