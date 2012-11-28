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

#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Guild.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Database/DatabaseEnv.h"
#include "Policies/Singleton.h"
#include "ProgressBar.h"
#include "World.h"

INSTANTIATE_SINGLETON_1(GuildMgr);

GuildMgr::GuildMgr()
{
}

GuildMgr::~GuildMgr()
{
    for (GuildMap::iterator itr = m_GuildMap.begin(); itr != m_GuildMap.end(); ++itr)
        delete itr->second;
}

void GuildMgr::AddGuild(Guild* guild)
{
    m_GuildMap[guild->GetId()] = guild;
}

void GuildMgr::RemoveGuild(uint32 guildId)
{
    m_GuildMap.erase(guildId);
}

void GuildMgr::RemoveGuild(ObjectGuid guildGuid)
{
    if (!guildGuid.IsGuild())
        return;

    RemoveGuild(guildGuid.GetCounter());
}

Guild* GuildMgr::GetGuildById(uint32 guildId) const
{
    GuildMap::const_iterator itr = m_GuildMap.find(guildId);
    if (itr != m_GuildMap.end())
        return itr->second;

    return NULL;
}

Guild* GuildMgr::GetGuildByGuid(ObjectGuid guildGuid) const
{
    if (!guildGuid.IsGuild())
        return NULL;

    return GetGuildById(guildGuid.GetCounter());
}

Guild* GuildMgr::GetGuildByName(std::string const& name) const
{
    for (GuildMap::const_iterator itr = m_GuildMap.begin(); itr != m_GuildMap.end(); ++itr)
        if (itr->second->GetName() == name)
            return itr->second;

    return NULL;
}

Guild* GuildMgr::GetGuildByLeader(ObjectGuid const& guid) const
{
    for (GuildMap::const_iterator itr = m_GuildMap.begin(); itr != m_GuildMap.end(); ++itr)
        if (itr->second->GetLeaderGuid() == guid)
            return itr->second;

    return NULL;
}

std::string GuildMgr::GetGuildNameById(uint32 guildId) const
{
    GuildMap::const_iterator itr = m_GuildMap.find(guildId);
    if (itr != m_GuildMap.end())
        return itr->second->GetName();

    return "";
}

std::string GuildMgr::GetGuildNameByGuid(ObjectGuid guildGuid) const
{
    if (!guildGuid.IsGuild())
        return "";

    return GetGuildNameById(guildGuid.GetCounter());
}

void GuildMgr::LoadGuilds()
{
    uint32 count = 0;

    //                                                    0             1          2          3           4           5           6
    QueryResult *result = CharacterDatabase.PQuery("SELECT guild.guildid,guild.name,leaderguid,EmblemStyle,EmblemColor,BorderStyle,BorderColor,"
    //   7               8    9    10         11        12    13         14              15
        "BackgroundColor,info,motd,createdate,BankMoney,level,experience,todayExperience,(SELECT COUNT(guild_bank_tab.guildid) FROM guild_bank_tab WHERE guild_bank_tab.guildid = guild.guildid) "
        "FROM guild ORDER BY guildid ASC");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded %u guild definitions", count);
        return;
    }

    // load guild ranks
    //                                                                0       1   2     3      4
    QueryResult* guildRanksResult   = CharacterDatabase.Query("SELECT guildid,rid,rname,rights,BankMoneyPerDay FROM guild_rank ORDER BY guildid ASC, rid ASC");

    // load guild members
    //                                                                0       1                 2    3     4       5                  6
    QueryResult* guildMembersResult = CharacterDatabase.Query("SELECT guildid,guild_member.guid,rank,pnote,offnote,BankResetTimeMoney,BankRemMoney,"
                                      //   7                 8                9                 10               11                12
                                      "BankResetTimeTab0,BankRemSlotsTab0,BankResetTimeTab1,BankRemSlotsTab1,BankResetTimeTab2,BankRemSlotsTab2,"
                                      //   13                14               15                16               17                18
                                      "BankResetTimeTab3,BankRemSlotsTab3,BankResetTimeTab4,BankRemSlotsTab4,BankResetTimeTab5,BankRemSlotsTab5,"
                                      //   19               20                21                22               23                      24
                                      "characters.name, characters.level, characters.class, characters.zone, characters.logout_time, characters.account "
                                      "FROM guild_member LEFT JOIN characters ON characters.guid = guild_member.guid ORDER BY guildid ASC");

    // load guild bank tab rights
    //                                                                      0       1     2   3       4
    QueryResult* guildBankTabRightsResult = CharacterDatabase.Query("SELECT guildid,TabId,rid,gbright,SlotPerDay FROM guild_bank_right ORDER BY guildid ASC, TabId ASC");

    BarGoLink bar(result->GetRowCount());

    do
    {
        // Field *fields = result->Fetch();

        bar.step();
        ++count;

        Guild* newGuild = new Guild;
        if (!newGuild->LoadGuildFromDB(result) ||
            !newGuild->LoadRanksFromDB(guildRanksResult) ||
            !newGuild->LoadMembersFromDB(guildMembersResult) ||
            !newGuild->LoadBankRightsFromDB(guildBankTabRightsResult) ||
            !newGuild->CheckGuildStructure())
        {
            newGuild->Disband();
            delete newGuild;
            continue;
        }

        newGuild->LoadGuildNewsEventLogFromDB();
        QueryResult* achievementResult = CharacterDatabase.PQuery("SELECT achievement, date, guids FROM guild_achievement WHERE guildId = %u", newGuild->GetId());
        QueryResult* criteriaResult = CharacterDatabase.PQuery("SELECT criteria, counter, date, completedGuid FROM guild_achievement_progress WHERE guildId = %u", newGuild->GetId());
        newGuild->GetAchievementMgr().LoadFromDB(achievementResult, criteriaResult);

        AddGuild(newGuild);
    }
    while (result->NextRow());

    delete result;
    delete guildRanksResult;
    delete guildMembersResult;
    delete guildBankTabRightsResult;

    // delete unused LogGuid records in guild_eventlog and guild_bank_eventlog table
    // you can comment these lines if you don't plan to change CONFIG_UINT32_GUILD_EVENT_LOG_COUNT and CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT
    CharacterDatabase.PExecute("DELETE FROM guild_news_eventlog WHERE LogGuid > '%u'", GUILD_NEWS_MAX_LOGS);
    CharacterDatabase.PExecute("DELETE FROM guild_eventlog WHERE LogGuid > '%u'", sWorld.getConfig(CONFIG_UINT32_GUILD_EVENT_LOG_COUNT));
    CharacterDatabase.PExecute("DELETE FROM guild_bank_eventlog WHERE LogGuid > '%u'", sWorld.getConfig(CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT));

    sLog.outString();
    sLog.outString(">> Loaded %u guild definitions", count);
}

uint32 GuildMgr::GetXPForGuildLevel(uint8 level) const
{
    if (level < GuildXPperLevel.size())
        return GuildXPperLevel[level];
    return 0;
}

void GuildMgr::ResetExperienceCaps()
{
    CharacterDatabase.BeginTransaction();
    CharacterDatabase.Execute("UPDATE guild SET todayExperience = 0");

    for (GuildMap::iterator itr = m_GuildMap.begin(); itr != m_GuildMap.end(); ++itr)
        itr->second->ResetDailyExperience();

    CharacterDatabase.CommitTransaction();
}

void GuildMgr::ResetReputationCaps()
{
    /// @TODO: Implement
}

void GuildMgr::SaveGuilds()
{
    for (GuildMap::iterator itr = m_GuildMap.begin(); itr != m_GuildMap.end(); ++itr)
        if (Guild* guild = itr->second)
            guild->SaveToDB();
}

void GuildMgr::LoadGuildXpForLevel()
{
    GuildXPperLevel.resize(sWorld.getConfig(CONFIG_UINT32_GUILD_MAX_LEVEL));
    for (uint8 level = 0; level < sWorld.getConfig(CONFIG_UINT32_GUILD_MAX_LEVEL); ++level)
        GuildXPperLevel[level] = 0;

    //                                                 0    1
    QueryResult* result  = WorldDatabase.Query("SELECT lvl, xp_for_next_level FROM guild_xp_for_level");

    if (!result)
    {
        sLog.outError(">> Loaded 0 xp for guild level definitions. DB table `guild_xp_for_level` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 level        = fields[0].GetUInt8();
        uint32 requiredXP   = fields[1].GetUInt64();

        if (level >= sWorld.getConfig(CONFIG_UINT32_GUILD_MAX_LEVEL))
        {
            sLog.outError("Unused (> Guild.MaxLevel in mangosd.conf) level %u in `guild_xp_for_level` table, ignoring.", uint32(level));
            continue;
        }

        GuildXPperLevel[level] = requiredXP;
        ++count;
    } while (result->NextRow());
    delete result;

    // fill level gaps
    for (uint8 level = 1; level < sWorld.getConfig(CONFIG_UINT32_GUILD_MAX_LEVEL); ++level)
    {
        if (!GuildXPperLevel[level])
        {
            sLog.outError("Level %i does not have XP for guild level data. Using data of level [%i] + 1660000.", level+1, level);
            GuildXPperLevel[level] = GuildXPperLevel[level - 1] + 1660000;
        }
    }

    sLog.outString(">> Loaded %u xp for guild level definitions.", count);
}

void GuildMgr::LoadGuildRewards()
{
    //                                                 0      1         2         3      4
    QueryResult* result  = WorldDatabase.Query("SELECT entry, standing, racemask, price, achievement FROM guild_rewards");

    if (!result)
    {
        sLog.outError(">> Loaded 0 guild reward definitions. DB table `guild_rewards` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        GuildReward reward;
        Field* fields = result->Fetch();
        uint32 Entry         = fields[0].GetUInt32();
        reward.Standing      = fields[1].GetUInt8();
        reward.Racemask      = fields[2].GetInt32();
        reward.Price         = fields[3].GetUInt64();
        reward.AchievementId = fields[4].GetUInt32();

        if (!sObjectMgr.GetItemPrototype(Entry))
        {
            sLog.outError("Guild rewards constains not existing item entry %u", Entry);
            continue;
        }

        if (reward.AchievementId != 0 && (!sAchievementStore.LookupEntry(reward.AchievementId)))
        {
            sLog.outError("Guild rewards constains not existing achievement entry %u", reward.AchievementId);
            continue;
        }

        if (reward.Standing >= MAX_REPUTATION_RANK)
        {
            sLog.outError("Guild rewards contains wrong reputation standing %u, max is %u", uint32(reward.Standing), MAX_REPUTATION_RANK - 1);
            continue;
        }

        m_GuildRewards[Entry] = reward;
        ++count;
    } while (result->NextRow());
    delete result;

    sLog.outString(">> Loaded %u guild reward definitions", count);
}
