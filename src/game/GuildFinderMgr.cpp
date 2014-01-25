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

#include "ObjectMgr.h"
#include "GuildFinderMgr.h"
#include "GuildMgr.h"
#include "Guild.h"
#include "World.h"
#include "DBCStores.h"
#include "ProgressBar.h"

GuildFinderMgr::GuildFinderMgr(){}
GuildFinderMgr::~GuildFinderMgr(){}

void GuildFinderMgr::LoadGuildSettings()
{
    //                                                    0             1                  2                3               4           5            6             7
    QueryResult* result = CharacterDatabase.Query("SELECT gfgs.guildId, gfgs.availability, gfgs.classRoles, gfgs.interests, gfgs.level, gfgs.listed, gfgs.comment, c.race "
                                                  "FROM guild_finder_guild_settings gfgs "
                                                  "LEFT JOIN guild_member gm ON gm.guildid=gfgs.guildId "
                                                  "LEFT JOIN characters c ON c.guid = gm.guid LIMIT 1");

    if (!result)
    {
        sLog.outString(">> Loaded 0 guild finder guild-related settings. Table `guild_finder_guild_settings` is empty.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(result->GetRowCount());
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId = fields[0].GetUInt32();
        uint8 availability = fields[1].GetUInt8();
        uint8 classRoles = fields[2].GetUInt8();
        uint8 interests = fields[3].GetUInt8();
        uint8 level = fields[4].GetUInt8();
        bool listed = (fields[5].GetUInt8() != 0);
        std::string comment = fields[6].GetString();

        TeamIndex guildTeam = TEAM_INDEX_ALLIANCE;
        if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(fields[7].GetUInt8()))
            if (raceEntry->TeamID == 1)
                guildTeam = TEAM_INDEX_HORDE;

        LFGuildSettings settings(listed, guildTeam, guildId, classRoles, availability, interests, level, comment);
        m_guildSettings[guildId] = settings;

        bar.step();
        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u guild finder guild-related settings.", count);
}

void GuildFinderMgr::LoadMembershipRequests()
{
    //                                                    0        1           2             3          4          5        6
    QueryResult* result = CharacterDatabase.Query("SELECT guildId, playerGuid, availability, classRole, interests, comment, submitTime "
                                                  "FROM guild_finder_applicant");

    if (!result)
    {
        sLog.outString(">> Loaded 0 guild finder membership requests. Table `guild_finder_applicant` is empty.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(result->GetRowCount());
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId = fields[0].GetUInt32();
        uint32 playerId = fields[1].GetUInt32();
        uint8 availability = fields[2].GetUInt8();
        uint8 classRoles = fields[3].GetUInt8();
        uint8 interests = fields[4].GetUInt8();
        std::string comment = fields[5].GetString();
        uint32 submitTime = fields[6].GetUInt32();

        MembershipRequest request(playerId, guildId, availability, classRoles, interests, comment, time_t(submitTime));
        m_membershipRequests[guildId].push_back(request);

        bar.step();
        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u guild finder membership requests.", count);
}

void GuildFinderMgr::AddMembershipRequest(uint32 guildGuid, MembershipRequest const& request)
{
    m_membershipRequests[guildGuid].push_back(request);

    static SqlStatementID insertFinderApplicant;
    CharacterDatabase.BeginTransaction();
    SqlStatement stmt = CharacterDatabase.CreateStatement(insertFinderApplicant, "REPLACE INTO guild_finder_applicant (guildId, playerGuid, availability, classRole, interests, comment, submitTime) VALUES(?, ?, ?, ?, ?, ?, ?)");
    stmt.addUInt32(request.GetGuildId());
    stmt.addUInt32(request.GetPlayerGuid());
    stmt.addUInt8(request.GetAvailability());
    stmt.addUInt8(request.GetClassRoles());
    stmt.addUInt8(request.GetInterests());
    stmt.addString(request.GetComment());
    stmt.addUInt32(request.GetSubmitTime());
    stmt.Execute();
    CharacterDatabase.CommitTransaction();

    // Notify the applicant his submittion has been added
    if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, request.GetPlayerGuid())))
        SendMembershipRequestListUpdate(*player);

    // Notify the guild master and officers the list changed
    if (Guild* guild = sGuildMgr.GetGuildById(guildGuid))
        SendApplicantListUpdate(*guild);
}

void GuildFinderMgr::RemoveMembershipRequest(uint32 playerId, uint32 guildId)
{
    std::vector<MembershipRequest>::iterator itr = m_membershipRequests[guildId].begin();
    for(; itr != m_membershipRequests[guildId].end(); ++itr)
        if (itr->GetPlayerGuid() == playerId)
            break;

    if (itr == m_membershipRequests[guildId].end())
        return;

    static SqlStatementID deleteFinderApplicant;
    CharacterDatabase.BeginTransaction();
    SqlStatement stmt = CharacterDatabase.CreateStatement(deleteFinderApplicant, "DELETE FROM guild_finder_applicant WHERE guildId = ? AND playerGuid = ?");
    stmt.addUInt32((*itr).GetGuildId());
    stmt.addUInt32((*itr).GetPlayerGuid());
    stmt.Execute();
    CharacterDatabase.CommitTransaction();

    m_membershipRequests[guildId].erase(itr);

    // Notify the applicant his submittion has been removed
    if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, playerId)))
        SendMembershipRequestListUpdate(*player);

    // Notify the guild master and officers the list changed
    if (Guild* guild = sGuildMgr.GetGuildById(guildId))
        SendApplicantListUpdate(*guild);
}

std::list<MembershipRequest> GuildFinderMgr::GetAllMembershipRequestsForPlayer(uint32 playerGuid)
{
    std::list<MembershipRequest> resultSet;
    for (MembershipRequestStore::const_iterator itr = m_membershipRequests.begin(); itr != m_membershipRequests.end(); ++itr)
    {
        std::vector<MembershipRequest> const& guildReqs = itr->second;
        for (std::vector<MembershipRequest>::const_iterator itr2 = guildReqs.begin(); itr2 != guildReqs.end(); ++itr2)
        {
            if (itr2->GetPlayerGuid() == playerGuid)
            {
                resultSet.push_back(*itr2);
                break;
            }
        }
    }
    return resultSet;
}

uint8 GuildFinderMgr::CountRequestsFromPlayer(uint32 playerId)
{
    uint8 result = 0;
    for (MembershipRequestStore::const_iterator itr = m_membershipRequests.begin(); itr != m_membershipRequests.end(); ++itr)
    {
        for (std::vector<MembershipRequest>::const_iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
        {
            if (itr2->GetPlayerGuid() != playerId)
                continue;

            ++result;
            break;
        }
    }
    return result;
}

LFGuildStore GuildFinderMgr::GetGuildsMatchingSetting(LFGuildPlayer& settings, TeamIndex faction)
{
    LFGuildStore resultSet;
    for (LFGuildStore::const_iterator itr = m_guildSettings.begin(); itr != m_guildSettings.end(); ++itr)
    {
        LFGuildSettings const& guildSettings = itr->second;

        if (guildSettings.GetTeam() != faction)
            continue;

        if (!(guildSettings.GetAvailability() & settings.GetAvailability()))
            continue;

        if (!(guildSettings.GetClassRoles() & settings.GetClassRoles()))
            continue;

        if (!(guildSettings.GetInterests() & settings.GetInterests()))
            continue;

        if (!(guildSettings.GetLevel() & settings.GetLevel()))
            continue;

        resultSet.insert(std::make_pair(itr->first, guildSettings));
    }

    return resultSet;
}

bool GuildFinderMgr::HasRequest(uint32 playerId, uint32 guildId)
{
    for (std::vector<MembershipRequest>::const_iterator itr = m_membershipRequests[guildId].begin(); itr != m_membershipRequests[guildId].end(); ++itr)
        if (itr->GetPlayerGuid() == playerId)
            return true;

    return false;
}

void GuildFinderMgr::SetGuildSettings(uint32 guildGuid, LFGuildSettings const& settings)
{
    m_guildSettings[guildGuid] = settings;

    static SqlStatementID replaceGuildFinderSettings;
    CharacterDatabase.BeginTransaction();
    SqlStatement stmt = CharacterDatabase.CreateStatement(replaceGuildFinderSettings, "REPLACE INTO guild_finder_guild_settings (guildId, availability, classRoles, interests, level, listed, comment) VALUES(?, ?, ?, ?, ?, ?, ?)");
    stmt.addUInt32(settings.GetGuid());
    stmt.addUInt8(settings.GetAvailability());
    stmt.addUInt8(settings.GetClassRoles());
    stmt.addUInt8(settings.GetInterests());
    stmt.addUInt8(settings.GetLevel());
    stmt.addUInt8(settings.IsListed());
    stmt.addString(settings.GetComment());
    stmt.Execute();
    CharacterDatabase.CommitTransaction();
}

void GuildFinderMgr::DeleteGuild(uint32 guildId)
{
    std::vector<MembershipRequest>::iterator itr = m_membershipRequests[guildId].begin();
    while (itr != m_membershipRequests[guildId].end())
    {
        uint32 applicant = itr->GetPlayerGuid();
        CharacterDatabase.BeginTransaction();

        static SqlStatementID deleteFinderApplicant;
        static SqlStatementID deleteGuildFinderSettings;
        SqlStatement stmt = CharacterDatabase.CreateStatement(deleteFinderApplicant, "DELETE FROM guild_finder_applicant WHERE guildId = ? AND playerGuid = ?");
        stmt.addUInt32(itr->GetGuildId());
        stmt.addUInt32(itr->GetPlayerGuid());
        stmt.Execute();

        SqlStatement stmt2 = CharacterDatabase.CreateStatement(deleteGuildFinderSettings, "DELETE FROM guild_finder_guild_settings WHERE guildId = ?");
        stmt2.addUInt32(itr->GetGuildId());
        stmt2.Execute();

        CharacterDatabase.CommitTransaction();

        m_membershipRequests[guildId].erase(itr);

        // Notify the applicant his submition has been removed
        if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, applicant)))
            SendMembershipRequestListUpdate(*player);
    }

    m_membershipRequests.erase(guildId);
    m_guildSettings.erase(guildId);

    // Notify the guild master the list changed (even if he's not a GM any more, not sure if needed)
    if (Guild* guild = sGuildMgr.GetGuildById(guildId))
        SendApplicantListUpdate(*guild);
}

void GuildFinderMgr::SendApplicantListUpdate(Guild& guild)
{
    WorldPacket data(SMSG_LF_GUILD_APPLICANT_LIST_UPDATED, 0);
    if (Player* player = ObjectAccessor::FindPlayer(guild.GetLeaderGuid()))
        player->SendDirectMessage(&data);

    guild.BroadcastPacketToRank(&data, GR_OFFICER);
}

void GuildFinderMgr::SendMembershipRequestListUpdate(Player& player)
{
    WorldPacket data(SMSG_LF_GUILD_APPLICATIONS_LIST_CHANGED, 0);
    player.SendDirectMessage(&data);
}