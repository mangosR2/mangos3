/*
 * Copyright (C) 2011-2012 /dev/rsa for MangosR2 <http://github.com/MangosR2>
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
#include "GuildFinderMgr.h"
#include "GuildMgr.h"
#include "Guild.h"

void WorldSession::HandleGuildFinderAddRecruitOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_ADD_RECRUIT");

    if (sGuildFinderMgr.GetAllMembershipRequestsForPlayer(GetPlayer()->GetObjectGuid()).size() == 10)
        return;

    uint32 classRoles = 0;
    uint32 guildInterests = 0;
    uint32 availability = 0;

    recvPacket >> classRoles >> guildInterests >> availability;

    ObjectGuid guid;
    guid[3] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    uint16 commentLength = recvPacket.ReadBits(11);
    guid[5] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    uint8 nameLength = recvPacket.ReadBits(7);
    guid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[5]);
    std::string comment = recvPacket.ReadString(commentLength);
    std::string playerName = recvPacket.ReadString(nameLength);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[3]);

    uint32 guildLowGuid = ObjectGuid(uint64(guid));

    if (!guid.IsGuild())
        return;

    if (!(classRoles & GUILDFINDER_ALL_ROLES) || classRoles > GUILDFINDER_ALL_ROLES)
        return;

    if (!(availability & ALL_WEEK) || availability > ALL_WEEK)
        return;

    if (!(guildInterests & ALL_INTERESTS) || guildInterests > ALL_INTERESTS)
        return;

    MembershipRequest request = MembershipRequest(GetPlayer()->GetObjectGuid(), guildLowGuid, availability, classRoles, guildInterests, comment, time(NULL));
    sGuildFinderMgr.AddMembershipRequest(guildLowGuid, request);
}

void WorldSession::HandleGuildFinderBrowseOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_BROWSE");

    uint32 classRoles = 0;
    uint32 availability = 0;
    uint32 guildInterests = 0;
    uint32 playerLevel = 0; // Raw player level (1-85), do they use MAX_FINDER_LEVEL when on level 85 ?

    recvPacket >> classRoles >> availability >> guildInterests >> playerLevel;

    if (!(classRoles & GUILDFINDER_ALL_ROLES) || classRoles > GUILDFINDER_ALL_ROLES)
        return;

    if (!(availability & ALL_WEEK) || availability > ALL_WEEK)
        return;

    if (!(guildInterests & ALL_INTERESTS) || guildInterests > ALL_INTERESTS)
        return;

    if (playerLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) || playerLevel < 1)
        return;

    Player* player = GetPlayer();

    LFGuildPlayer settings(player->GetObjectGuid(), classRoles, availability, guildInterests, ANY_FINDER_LEVEL);
    LFGuildStore guildList = sGuildFinderMgr.GetGuildsMatchingSetting(settings, GetTeamIndex(player->GetTeam()));
    uint32 guildCount = guildList.size();

    if (guildCount == 0)
    {
        WorldPacket packet(SMSG_LF_GUILD_BROWSE_UPDATED, 0);
        player->SendDirectMessage(&packet);
        return;
    }

    ByteBuffer bufferData(65 * guildCount);
    WorldPacket data(SMSG_LF_GUILD_BROWSE_UPDATED, 3 + guildCount * 65); // Estimated size
    data.WriteBits(guildCount, 19);

    for (LFGuildStore::const_iterator itr = guildList.begin(); itr != guildList.end(); ++itr)
    {
        LFGuildSettings guildSettings = itr->second;
        Guild* guild = sGuildMgr.GetGuildById(itr->first);

        ObjectGuid guildGuid = ObjectGuid(guild->GetObjectGuid());

        data.WriteBit(guildGuid[7]);
        data.WriteBit(guildGuid[5]);
        data.WriteBits(guild->GetName().size(), 8);
        data.WriteBit(guildGuid[0]);
        data.WriteBits(guildSettings.GetComment().size(), 11);
        data.WriteBit(guildGuid[4]);
        data.WriteBit(guildGuid[1]);
        data.WriteBit(guildGuid[2]);
        data.WriteBit(guildGuid[6]);
        data.WriteBit(guildGuid[3]);

        bufferData << int32(guild->GetEmblemColor());
        bufferData << int32(guild->GetBorderStyle()); // Guessed
        bufferData << int32(guild->GetEmblemStyle());

        bufferData.WriteString(guildSettings.GetComment());

        bufferData << uint8(0); // Cached ? Idk

        bufferData.WriteByteSeq(guildGuid[5]);

        bufferData << uint32(guildSettings.GetInterests()); // Guild Interests

        bufferData.WriteByteSeq(guildGuid[6]);
        bufferData.WriteByteSeq(guildGuid[4]);

        bufferData << guild->GetLevel();

        bufferData.WriteString(guild->GetName());

        bufferData << int32(0); // guild->GetAchievementMgr().GetAchievementPoints()

        bufferData.WriteByteSeq(guildGuid[7]);

        bufferData << uint8(sGuildFinderMgr.HasRequest(player->GetObjectGuid(), guild->GetObjectGuid())); // Request pending

        bufferData.WriteByteSeq(guildGuid[2]);
        bufferData.WriteByteSeq(guildGuid[0]);

        bufferData << uint32(guildSettings.GetAvailability());

        bufferData.WriteByteSeq(guildGuid[1]);

        bufferData << int32(guild->GetBackgroundColor());
        bufferData << uint32(0); // Unk Int 2 (+ 128) // Always 0 or 1
        bufferData << int32(guild->GetBorderColor());
        bufferData << uint32(guildSettings.GetClassRoles());

        bufferData.WriteByteSeq(guildGuid[3]);
        bufferData << int32(guild->GetMemberSize());
    }

    data.FlushBits();
    data.append(bufferData);

    player->SendDirectMessage(&data);
}

void WorldSession::HandleGuildFinderDeclineRecruitOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_DECLINE_RECRUIT");

    ObjectGuid playerGuid;

    playerGuid[1] = recvPacket.ReadBit();
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[2] = recvPacket.ReadBit();
    playerGuid[6] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    playerGuid[3] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(playerGuid[5]);
    recvPacket.ReadByteSeq(playerGuid[7]);
    recvPacket.ReadByteSeq(playerGuid[2]);
    recvPacket.ReadByteSeq(playerGuid[3]);
    recvPacket.ReadByteSeq(playerGuid[4]);
    recvPacket.ReadByteSeq(playerGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[0]);
    recvPacket.ReadByteSeq(playerGuid[6]);

    if (!playerGuid.IsPlayer())
        return;

    sGuildFinderMgr.RemoveMembershipRequest(ObjectGuid(playerGuid), GetPlayer()->GetGuildId());
}

void WorldSession::HandleGuildFinderGetApplicationsOpcode(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_GET_APPLICATIONS"); // Empty opcode

    std::list<MembershipRequest> applicatedGuilds = sGuildFinderMgr.GetAllMembershipRequestsForPlayer(GetPlayer()->GetObjectGuid());
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

            ObjectGuid guildGuid = ObjectGuid(guild->GetObjectGuid());

            data.WriteBit(guildGuid[1]);
            data.WriteBit(guildGuid[0]);
            data.WriteBit(guildGuid[5]);
            data.WriteBits(request.GetComment().size(), 11);
            data.WriteBit(guildGuid[3]);
            data.WriteBit(guildGuid[7]);
            data.WriteBit(guildGuid[4]);
            data.WriteBit(guildGuid[6]);
            data.WriteBit(guildGuid[2]);
            data.WriteBits(guild->GetName().size(), 8);

            bufferData.WriteByteSeq(guildGuid[2]);
            bufferData.WriteString(request.GetComment());
            bufferData.WriteByteSeq(guildGuid[5]);
            bufferData.WriteString(guild->GetName());

            bufferData << uint32(guildSettings.GetAvailability());
            bufferData << uint32(request.GetExpiryTime() - time(NULL)); // Time left to application expiry (seconds)

            bufferData.WriteByteSeq(guildGuid[0]);
            bufferData.WriteByteSeq(guildGuid[6]);
            bufferData.WriteByteSeq(guildGuid[3]);
            bufferData.WriteByteSeq(guildGuid[7]);

            bufferData << uint32(guildSettings.GetClassRoles());

            bufferData.WriteByteSeq(guildGuid[4]);
            bufferData.WriteByteSeq(guildGuid[1]);
        
            bufferData << uint32(time(NULL) - request.GetSubmitTime()); // Time since application (seconds)
            bufferData << uint32(guildSettings.GetInterests());
        }

        data.FlushBits();
        data.append(bufferData);
    }
    data << uint32(10 - sGuildFinderMgr.CountRequestsFromPlayer(GetPlayer()->GetObjectGuid())); // Applications count left

    GetPlayer()->SendDirectMessage(&data);
}

// Lists all recruits for a guild - Misses times
void WorldSession::HandleGuildFinderGetRecruitsOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_GET_RECRUITS");

    uint32 unkTime = 0;
    recvPacket >> unkTime;

    Player* player = GetPlayer();
    if (!player->GetGuildId())
        return;

    std::vector<MembershipRequest> recruitsList = sGuildFinderMgr.GetAllMembershipRequestsForGuild(player->GetGuildId());
    uint32 recruitCount = recruitsList.size();

    ByteBuffer dataBuffer(53 * recruitCount);
    WorldPacket data(SMSG_LF_GUILD_RECRUIT_LIST_UPDATED, 7 + 26 * recruitCount + 53 * recruitCount);
    data.WriteBits(recruitCount, 20);

    for (std::vector<MembershipRequest>::const_iterator itr = recruitsList.begin(); itr != recruitsList.end(); ++itr)
    {
        MembershipRequest request = *itr;
        ObjectGuid playerGuid(ObjectGuid(HIGHGUID_PLAYER, 0, request.GetPlayerGuid()));

        data.WriteBits(request.GetComment().size(), 11);
        data.WriteBit(playerGuid[2]);
        data.WriteBit(playerGuid[4]);
        data.WriteBit(playerGuid[3]);
        data.WriteBit(playerGuid[7]);
        data.WriteBit(playerGuid[0]);
        data.WriteBits(request.GetName().size(), 7);
        data.WriteBit(playerGuid[5]);
        data.WriteBit(playerGuid[1]);
        data.WriteBit(playerGuid[6]);

        dataBuffer.WriteByteSeq(playerGuid[4]);
        
        dataBuffer << int32(time(NULL) <= request.GetExpiryTime());
        
        dataBuffer.WriteByteSeq(playerGuid[3]);
        dataBuffer.WriteByteSeq(playerGuid[0]);
        dataBuffer.WriteByteSeq(playerGuid[1]);

        dataBuffer << int32(request.GetLevel());

        dataBuffer.WriteByteSeq(playerGuid[6]);
        dataBuffer.WriteByteSeq(playerGuid[7]);
        dataBuffer.WriteByteSeq(playerGuid[2]);

        dataBuffer << int32(time(NULL) - request.GetSubmitTime()); // Time in seconds since application submitted.
        dataBuffer << int32(request.GetAvailability());
        dataBuffer << int32(request.GetClassRoles());
        dataBuffer << int32(request.GetInterests());
        dataBuffer << int32(request.GetExpiryTime() - time(NULL)); // TIme in seconds until application expires.

        dataBuffer.WriteString(request.GetName());
        dataBuffer.WriteString(request.GetComment());

        dataBuffer << int32(request.GetClass());

        dataBuffer.WriteByteSeq(playerGuid[5]);
    }

    data.FlushBits();
    data.append(dataBuffer);
    data << uint32(time(NULL)); // Unk time

    player->SendDirectMessage(&data);
}

void WorldSession::HandleGuildFinderPostRequestOpcode(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_POST_REQUEST"); // Empty opcode

    Player* player = GetPlayer();

    if (!player->GetGuildId()) // Player must be in guild
        return;

    bool isGuildMaster = true;
    if (Guild* guild = sGuildMgr.GetGuildById(player->GetGuildId()))
        if (guild->GetLeaderGuid() != player->GetObjectGuid())
            isGuildMaster = false;

    LFGuildSettings settings = sGuildFinderMgr.GetGuildSettings(player->GetGuildId());

    /// Client does not seem to receive correct data as only the comment is correctly displayed.
    WorldPacket data(SMSG_LF_GUILD_POST_UPDATED, 35);
    data.WriteBit(isGuildMaster); // Guessed

    if (isGuildMaster)
    {
        data.WriteBits(settings.GetComment().size(), 11);
        data.WriteBit(settings.IsListed());
        data << uint32(settings.GetLevel());
        data.WriteBits(settings.GetComment().size(), 11); //data.WriteString(settings.GetComment());
        data << uint32(0); // Unk Int32
        data << uint32(settings.GetAvailability());
        data << uint32(settings.GetClassRoles());
        data << uint32(settings.GetInterests());
    }
    else
        data.FlushBits();

    player->SendDirectMessage(&data);
}

void WorldSession::HandleGuildFinderRemoveRecruitOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_REMOVE_RECRUIT");

    ObjectGuid guildGuid;

    guildGuid[0] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    guildGuid[2] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[4]);
    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[6]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(guildGuid[2]);
    recvPacket.ReadByteSeq(guildGuid[7]);

    if (!guildGuid.IsGuild())
        return;

    sGuildFinderMgr.RemoveMembershipRequest(GetPlayer()->GetGUIDLow(), ObjectGuid(guildGuid));
}

// Sent any time a guild master sets an option in the interface and when listing / unlisting his guild
void WorldSession::HandleGuildFinderSetGuildPostOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_LF_GUILD_SET_GUILD_POST");

    uint32 classRoles = 0;
    uint32 availability = 0;
    uint32 guildInterests = 0;
    uint32 level = 0;

    recvPacket >> level >> availability >> guildInterests >> classRoles;

    // Level sent is zero if untouched, force to any (from interface). Idk why
    if (!level)
        level = ANY_FINDER_LEVEL;

    uint16 length = recvPacket.ReadBits(11);
    bool listed = recvPacket.ReadBit();
    std::string comment = recvPacket.ReadString(length);

    if (!(classRoles & GUILDFINDER_ALL_ROLES) || classRoles > GUILDFINDER_ALL_ROLES)
        return;

    if (!(availability & ALL_WEEK) || availability > ALL_WEEK)
        return;

    if (!(guildInterests & ALL_INTERESTS) || guildInterests > ALL_INTERESTS)
        return;

    if (!(level & ALL_GUILDFINDER_LEVELS) || level > ALL_GUILDFINDER_LEVELS)
        return;

    Player* player = GetPlayer();
    if (!player->GetGuildId()) // Player must be in guild
        return;

    if (Guild* guild = sGuildMgr.GetGuildById(player->GetGuildId())) // Player must be guild master
        if (guild->GetLeaderGuid() != player->GetObjectGuid())
            return;

    LFGuildSettings settings(listed, GetTeamIndex(player->GetTeam()), player->GetGuildId(), classRoles, availability, guildInterests, level, comment);
    sGuildFinderMgr.SetGuildSettings(player->GetGuildId(), settings);
}