/*
 * Copyright (C) 2011-2012 /dev/rsa for MangosR2 <http://github.com/MangosR2>
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "WorldSession.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "GuildMgr.h"

void WorldSession::HandleGuildFinderAddRecruitOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_ADD_RECRUIT");

    if (sGuildFinderMgr.GetAllMembershipRequestsForPlayer(_player->GetGUIDLow()).size() == 10)
        return;

    uint32 classRoles = 0;
    uint32 availability = 0;
    uint32 guildInterests = 0;

    std::string comment;
    std::string playerName;

    recvPacket >> classRoles >> guildInterests >> availability;

    ObjectGuid guid;
    recvPacket.ReadGuidMask<3, 0, 6, 1>(guid);

    uint16 commentLength = recvPacket.ReadBits(11);

    recvPacket.ReadGuidMask<5, 4, 7>(guid);

    uint8 nameLength = recvPacket.ReadBits(7);

    recvPacket.ReadGuidMask<2>(guid);

    recvPacket.ReadGuidBytes<2, 4, 5>(guid);
    comment = recvPacket.ReadString(commentLength);
    playerName = recvPacket.ReadString(nameLength);
    recvPacket.ReadGuidBytes<7, 2, 0, 6, 1, 3>(guid);

    if (!guid.IsGuild())
        return;

    if (!(classRoles & GUILDFINDER_ALL_ROLES) || classRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(availability & ALL_WEEK) || availability > ALL_WEEK)
        return;
    if (!(guildInterests & ALL_INTERESTS) || guildInterests > ALL_INTERESTS)
        return;

    MembershipRequest request = MembershipRequest(_player->GetGUIDLow(), guid.GetCounter(), availability, classRoles, guildInterests, comment, time(NULL));
    sGuildFinderMgr.AddMembershipRequest(guid.GetCounter(), request);
}

void WorldSession::HandleGuildFinderBrowseOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_BROWSE");
    uint32 classRoles;
    uint32 availability;
    uint32 guildInterests;
    uint32 playerLevel; // Raw player level (1-85), do they use MAX_FINDER_LEVEL when on level 85 ?

    recvPacket >> classRoles >> availability >> guildInterests >> playerLevel;

    bool failed = false;
    if (!(classRoles & GUILDFINDER_ALL_ROLES) || classRoles > GUILDFINDER_ALL_ROLES)
        failed = true;
    else if (!(availability & ALL_WEEK) || availability > ALL_WEEK)
        failed = true;
    else if (!(guildInterests & ALL_INTERESTS) || guildInterests > ALL_INTERESTS)
        failed = true;
    else if (playerLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) || playerLevel < 1)
        failed = true;

    if (failed)
    {
        WorldPacket packet(SMSG_LF_GUILD_BROWSE_UPDATED, 0);
        packet.WriteBits(0, 19);
        SendPacket(&packet);
        return;
    }

    LFGuildPlayer settings(_player->GetGUIDLow(), classRoles, availability, guildInterests, ANY_FINDER_LEVEL);
    LFGuildStore guildList = sGuildFinderMgr.GetGuildsMatchingSetting(settings, _player->GetTeam());
    uint32 guildCount = guildList.size();

    if (guildCount == 0)
    {
        WorldPacket packet(SMSG_LF_GUILD_BROWSE_UPDATED, 0);
        packet.WriteBits(0, 19);
        SendPacket(&packet);
        return;
    }

    ByteBuffer bufferData(65 * guildCount);
    WorldPacket data(SMSG_LF_GUILD_BROWSE_UPDATED, 3 + guildCount * 65); // Estimated size
    data.WriteBits(guildCount, 19);

    for (LFGuildStore::const_iterator itr = guildList.begin(); itr != guildList.end(); ++itr)
    {
        LFGuildSettings guildSettings = itr->second;
        Guild* guild = sGuildMgr.GetGuildById(itr->first);

        ObjectGuid guildGUID = guild->GetObjectGuid();

        data.WriteGuidMask<7, 5>(guildGUID);
        data.WriteBits(guild->GetName().size(), 8);
        data.WriteGuidMask<0>(guildGUID);
        data.WriteBits(guildSettings.GetComment().size(), 11);
        data.WriteGuidMask<4, 1, 2, 6, 3>(guildGUID);

        bufferData << uint32(guild->GetEmblemColor());
        bufferData << uint32(guild->GetBorderStyle()); // Guessed
        bufferData << uint32(guild->GetEmblemStyle());

        bufferData.WriteStringData(guildSettings.GetComment());

        bufferData << uint8(0); // Unk

        bufferData.WriteGuidBytes<5>(guildGUID);

        bufferData << uint32(guildSettings.GetInterests());

        bufferData.WriteGuidBytes<6, 4>(guildGUID);

        bufferData << uint32(guild->GetLevel());

        bufferData.WriteStringData(guild->GetName());

        bufferData << uint32(0); // guild->GetAchievementMgr().GetAchievementPoints()

        bufferData.WriteGuidBytes<7>(guildGUID);

        bufferData << uint8(sGuildFinderMgr.HasRequest(_player->GetGUIDLow(), guild->GetId())); // Request pending

        bufferData.WriteGuidBytes<2, 0>(guildGUID);

        bufferData << uint32(guildSettings.GetAvailability());

        bufferData.WriteGuidBytes<1>(guildGUID);

        bufferData << uint32(guild->GetBackgroundColor());
        bufferData << uint32(0); // Unk Int 2 (+ 128) // Always 0 or 1
        bufferData << uint32(guild->GetBorderColor());
        bufferData << uint32(guildSettings.GetClassRoles());

        bufferData.WriteGuidBytes<3>(guildGUID);
        bufferData << int32(guild->GetMemberSize());
    }

    data.FlushBits();
    if (!bufferData.empty())
        data.append(bufferData);

    SendPacket(&data);
}

void WorldSession::HandleGuildFinderDeclineRecruitOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_DECLINE_RECRUIT");

    ObjectGuid playerGuid;

    recvPacket.ReadGuidMask<1, 4, 5, 2, 6, 7, 0, 3>(playerGuid);
    recvPacket.ReadGuidBytes<5, 7, 2, 3, 4, 1, 0, 6>(playerGuid);

    if (!playerGuid.IsPlayer())
        return;

    sGuildFinderMgr.RemoveMembershipRequest(playerGuid.GetCounter(), _player->GetGuildId());
}

void WorldSession::HandleGuildFinderGetApplicationsOpcode(WorldPacket& /*recvPacket*/)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_GET_APPLICATIONS"); // Empty opcode

    std::list<MembershipRequest> applicatedGuilds = sGuildFinderMgr.GetAllMembershipRequestsForPlayer(_player->GetGUIDLow());
    uint32 applicationsCount = applicatedGuilds.size();
    WorldPacket data(SMSG_LF_GUILD_MEMBERSHIP_LIST_UPDATED, 7 + 54 * applicationsCount);
    data.WriteBits(applicationsCount, 20);

    if (applicationsCount > 0)
    {
        ByteBuffer bufferData(54 * applicationsCount);
        for (std::list<MembershipRequest>::const_iterator itr = applicatedGuilds.begin(); itr != applicatedGuilds.end(); ++itr)
        {
            Guild* guild = sGuildMgr.GetGuildById(itr->GetGuildId());
            LFGuildSettings guildSettings = sGuildFinderMgr.GetGuildSettings(itr->GetGuildId());
            MembershipRequest request = *itr;

            ObjectGuid guildGuid = guild->GetObjectGuid();

            data.WriteGuidMask<1, 0, 5>(guildGuid);
            data.WriteBits(request.GetComment().size(), 11);
            data.WriteGuidMask<3, 7, 4, 6, 2>(guildGuid);
            data.WriteBits(guild->GetName().size(), 8);

            bufferData.WriteGuidBytes<2>(guildGuid);
            bufferData.WriteStringData(request.GetComment());
            bufferData.WriteGuidBytes<5>(guildGuid);
            bufferData.WriteStringData(guild->GetName());

            bufferData << uint32(guildSettings.GetAvailability());
            bufferData << uint32(request.GetExpiryTime() - time(NULL)); // Time left to application expiry (seconds)

            bufferData.WriteGuidBytes<0, 6, 3, 7>(guildGuid);

            bufferData << uint32(guildSettings.GetClassRoles());

            bufferData.WriteGuidBytes<4, 1>(guildGuid);

            bufferData << uint32(time(NULL) - request.GetSubmitTime()); // Time since application (seconds)
            bufferData << uint32(guildSettings.GetInterests());
        }

        data.FlushBits();
        if (!bufferData.empty())
            data.append(bufferData);
    }
    data << uint32(10 - sGuildFinderMgr.CountRequestsFromPlayer(_player->GetGUIDLow())); // Applications count left

    SendPacket(&data);
}

// Lists all recruits for a guild - Misses times
void WorldSession::HandleGuildFinderGetRecruitsOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_GET_RECRUITS");

    uint32 unkTime;
    recvPacket >> unkTime;

    if (!_player->GetGuildId())
        return;

    std::vector<MembershipRequest> recruitsList = sGuildFinderMgr.GetAllMembershipRequestsForGuild(_player->GetGuildId());

    ByteBuffer dataBuffer(53 * recruitsList.size());

    uint32 recruitCount = 0;
    WorldPacket data(SMSG_LF_GUILD_RECRUIT_LIST_UPDATED, 7 + 26 * recruitsList.size() + 53 * recruitsList.size());
    for (std::vector<MembershipRequest>::const_iterator itr = recruitsList.begin(); itr != recruitsList.end(); ++itr)
    {
        CharacterNameData const* nameData = sObjectMgr.GetCharacterNameData(itr->GetPlayerGuid());
        if (!nameData)
            continue;

        ++recruitCount;
    }

    data.WriteBits(recruitCount, 20);

    for (std::vector<MembershipRequest>::const_iterator itr = recruitsList.begin(); itr != recruitsList.end(); ++itr)
    {
        MembershipRequest request = *itr;
        ObjectGuid playerGuid = ObjectGuid(HIGHGUID_PLAYER, request.GetPlayerGuid());
        CharacterNameData const* nameData = sObjectMgr.GetCharacterNameData(playerGuid.GetCounter());
        if (!nameData)
            continue;

        data.WriteBits(request.GetComment().size(), 11);
        data.WriteGuidMask<2, 4, 3, 7, 0>(playerGuid);
        data.WriteBits(nameData->m_name.size(), 7);
        data.WriteGuidMask<5, 1, 6>(playerGuid);

        dataBuffer.WriteGuidBytes<4>(playerGuid);

        dataBuffer << int32(time(NULL) <= request.GetExpiryTime());

        dataBuffer.WriteGuidBytes<3, 0, 1>(playerGuid);

        dataBuffer << int32(nameData->m_level);

        dataBuffer.WriteGuidBytes<6, 7, 2>(playerGuid);

        dataBuffer << int32(time(NULL) - request.GetSubmitTime());  // Time in seconds since application submitted.
        dataBuffer << int32(request.GetAvailability());
        dataBuffer << int32(request.GetClassRoles());
        dataBuffer << int32(request.GetInterests());
        dataBuffer << int32(request.GetExpiryTime() - time(NULL));  // TIme in seconds until application expires.

        dataBuffer.WriteStringData(nameData->m_name);
        dataBuffer.WriteStringData(request.GetComment());

        dataBuffer << int32(nameData->m_class);

        dataBuffer.WriteGuidBytes<5>(playerGuid);
    }

    data.FlushBits();
    if (!dataBuffer.empty())
        data.append(dataBuffer);
    data << uint32(time(NULL)); // Unk time

    SendPacket(&data);
}

void WorldSession::HandleGuildFinderPostRequestOpcode(WorldPacket& /*recvPacket*/)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_POST_REQUEST"); // Empty opcode

    if (!_player->GetGuildId()) // Player must be in guild
        return;

    bool isGuildMaster = true;
    if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId()))
        if (guild->GetLeaderGuid() != _player->GetObjectGuid())
            isGuildMaster = false;

    LFGuildSettings settings = sGuildFinderMgr.GetGuildSettings(_player->GetGuildId());

    WorldPacket data(SMSG_LF_GUILD_POST_UPDATED, 35);
    data.WriteBit(isGuildMaster); // Guessed

    if (isGuildMaster)
    {
        data.WriteBit(settings.IsListed());
        data.WriteBits(settings.GetComment().size(), 11);
        data << uint32(settings.GetLevel());
        data.WriteStringData(settings.GetComment());
        data << uint32(0); // Unk Int32
        data << uint32(settings.GetAvailability());
        data << uint32(settings.GetClassRoles());
        data << uint32(settings.GetInterests());
    }

    SendPacket(&data);
}

void WorldSession::HandleGuildFinderRemoveRecruitOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_REMOVE_RECRUIT");

    ObjectGuid guildGuid;

    recvPacket.ReadGuidMask<0, 4, 3, 5, 7, 6, 2, 1>(guildGuid);
    recvPacket.ReadGuidBytes<4, 0, 3, 6, 5, 1, 2, 7>(guildGuid);

    if (!guildGuid.IsGuild())
        return;

    sGuildFinderMgr.RemoveMembershipRequest(_player->GetGUIDLow(), guildGuid.GetCounter());
}

// Sent any time a guild master sets an option in the interface and when listing / unlisting his guild
void WorldSession::HandleGuildFinderSetGuildPostOpcode(WorldPacket& recvPacket)
{
    sLog.outDebug("WORLD: Received CMSG_LF_GUILD_SET_GUILD_POST");

    uint32 classRoles;
    uint32 availability;
    uint32 guildInterests;
    uint32 level;
    std::string comment;

    recvPacket >> level >> availability >> guildInterests >> classRoles;
    // Level sent is zero if untouched, force to any (from interface). Idk why
    if (!level)
        level = ANY_FINDER_LEVEL;

    uint32 length = recvPacket.ReadBits(11);
    bool listed = recvPacket.ReadBit();
    comment = recvPacket.ReadString(length);

    if (classRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (availability > ALL_WEEK)
        return;
    if (guildInterests > ALL_INTERESTS)
        return;
    if (level != ANY_FINDER_LEVEL && level != MAX_FINDER_LEVEL)
        return;

    if (!_player->GetGuildId()) // Player must be in guild
        return;

    if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId())) // Player must be guild master
        if (guild->GetLeaderGuid() != _player->GetObjectGuid())
            return;

    LFGuildSettings settings(listed, _player->GetTeam(), _player->GetGuildId(), classRoles, availability, guildInterests, level, comment);
    sGuildFinderMgr.SetGuildSettings(_player->GetGuildId(), settings);
}
