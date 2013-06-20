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

#ifndef _GUILDFINDERMGR_H
#define _GUILDFINDERMGR_H

#include "Policies/Singleton.h"
#include "Common.h"
#include "World.h"
#include "GuildMgr.h"

enum GuildFinderOptionsInterest
{
    INTEREST_QUESTING        = 0x01,
    INTEREST_DUNGEONS        = 0x02,
    INTEREST_RAIDS           = 0x04,
    INTEREST_PVP             = 0x08,
    INTEREST_ROLE_PLAYING    = 0x10,
    ALL_INTERESTS            = INTEREST_QUESTING | INTEREST_DUNGEONS | INTEREST_RAIDS | INTEREST_PVP | INTEREST_ROLE_PLAYING
};

enum GuildFinderOptionsAvailability
{
    AVAILABILITY_WEEKDAYS    = 0x1,
    AVAILABILITY_WEEKENDS    = 0x2,
    ALL_WEEK                 = AVAILABILITY_WEEKDAYS | AVAILABILITY_WEEKENDS
};

enum GuildFinderOptionsRoles
{
    GUILDFINDER_ROLE_TANK    = 0x1,
    GUILDFINDER_ROLE_HEALER  = 0x2,
    GUILDFINDER_ROLE_DPS     = 0x4,
    GUILDFINDER_ALL_ROLES    = GUILDFINDER_ROLE_TANK | GUILDFINDER_ROLE_HEALER | GUILDFINDER_ROLE_DPS
};

enum GuildFinderOptionsLevel
{
    ANY_FINDER_LEVEL         = 0x1,
    MAX_FINDER_LEVEL         = 0x2,
    ALL_GUILDFINDER_LEVELS   = ANY_FINDER_LEVEL | MAX_FINDER_LEVEL
};

/// Holds all required informations about a membership request
struct MembershipRequest
{
    public:
        MembershipRequest(MembershipRequest const& settings) : m_comment(settings.GetComment())
        {
            m_availability = settings.GetAvailability();
            m_classRoles = settings.GetClassRoles();
            m_interests = settings.GetInterests();
            m_guildId = settings.GetGuildId();
            m_playerGuid = settings.GetPlayerGuid();
            m_time = settings.GetSubmitTime();
        }

        MembershipRequest(uint32 playerGuid, uint32 guildId, uint32 availability, uint32 classRoles, uint32 interests, std::string& comment, time_t submitTime) :
            m_playerGuid(playerGuid), m_guildId(guildId), m_availability(availability), m_classRoles(classRoles),
            m_interests(interests), m_time(submitTime), m_comment(comment) {}

        MembershipRequest() : m_playerGuid(0), m_guildId(0), m_availability(0), m_classRoles(0),
            m_interests(0), m_time(time(NULL)) {}

        uint32 GetGuildId() const      { return m_guildId; }
        uint32 GetPlayerGuid() const   { return m_playerGuid; }
        uint8 GetAvailability() const  { return m_availability; }
        uint8 GetClassRoles() const    { return m_classRoles; }
        uint8 GetInterests() const     { return m_interests; }
        time_t GetSubmitTime() const   { return m_time; }
        time_t GetExpiryTime() const   { return time_t(m_time + 30 * 24 * 3600); } // Adding 30 days
        std::string const& GetComment() const { return m_comment; }
    private:
        std::string m_comment;

        uint32 m_guildId;
        uint32 m_playerGuid;

        uint8 m_availability;
        uint8 m_classRoles;
        uint8 m_interests;

        time_t m_time;
};

/// Holds all informations about a player's finder settings. _NOT_ stored in database.
struct LFGuildPlayer
{
    public:
        LFGuildPlayer()
        {
            m_guid = 0;
            m_roles = 0;
            m_availability = 0;
            m_interests = 0;
            m_level = 0;
        }

        LFGuildPlayer(uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level)
        {
            m_guid = guid;
            m_roles = role;
            m_availability = availability;
            m_interests = interests;
            m_level = level;
        }

        LFGuildPlayer(uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level, std::string& comment) : m_comment(comment)
        {
            m_guid = guid;
            m_roles = role;
            m_availability = availability;
            m_interests = interests;
            m_level = level;
        }

        LFGuildPlayer(LFGuildPlayer const& settings) : m_comment(settings.GetComment())
        {
            m_guid = settings.GetGuid();
            m_roles = settings.GetClassRoles();
            m_availability = settings.GetAvailability();
            m_interests = settings.GetInterests();
            m_level = settings.GetLevel();
        }

        uint32 GetGuid() const         { return m_guid; }
        uint8 GetClassRoles() const    { return m_roles; }
        uint8 GetAvailability() const  { return m_availability; }
        uint8 GetInterests() const     { return m_interests; }
        uint8 GetLevel() const         { return m_level; }
        std::string const& GetComment() const { return m_comment; }

    private:
        std::string m_comment;
        uint32 m_guid;
        uint8 m_roles;
        uint8 m_availability;
        uint8 m_interests;
        uint8 m_level;
};

/// Holds settings for a guild in the finder system. Saved to database.
struct LFGuildSettings : public LFGuildPlayer
{
    public:
        LFGuildSettings() : LFGuildPlayer(), m_listed(false), m_team(ALLIANCE) {}

        LFGuildSettings(bool listed, Team team) : LFGuildPlayer(), m_listed(listed), m_team(team) {}

        LFGuildSettings(bool listed, Team team, uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level) :
            LFGuildPlayer(guid, role, availability, interests, level), m_listed(listed), m_team(team) {}

        LFGuildSettings(bool listed, Team team, uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level, std::string& comment) :
            LFGuildPlayer(guid, role, availability, interests, level, comment), m_listed(listed), m_team(team) {}

        LFGuildSettings(LFGuildSettings const& settings) :
            LFGuildPlayer(settings), m_listed(settings.IsListed()), m_team(settings.GetTeam()) {}


        bool IsListed() const      { return m_listed; }
        void SetListed(bool state) { m_listed = state; }

        Team GetTeam() const       { return m_team; }
    private:
        bool m_listed;
        Team m_team;
};

typedef std::map<uint32 /* guildGuid */, LFGuildSettings> LFGuildStore;
typedef std::map<uint32 /* guildGuid */, std::vector<MembershipRequest> > MembershipRequestStore;

class GuildFinderMgr : public MaNGOS::Singleton<GuildFinderMgr, MaNGOS::ClassLevelLockable<GuildFinderMgr, MANGOSR2_MUTEX_MODEL_2> >
{
    private:
        LFGuildStore m_guildSettings;
        MembershipRequestStore m_membershipRequests;

        void LoadGuildSettings();
        void LoadMembershipRequests();

    public:
        GuildFinderMgr();
        ~GuildFinderMgr();

        void LoadFromDB();

        /**
         * @brief Stores guild settings and begins an asynchronous database insert
         * @param guildGuid The guild's database guid.
         * @param LFGuildSettings The guild's settings storage.
         */
        void SetGuildSettings(uint32 guildGuid, LFGuildSettings const& settings);

        /**
         * @brief Returns settings for a guild.
         * @param guildGuid The guild's database guid.
         */
        LFGuildSettings GetGuildSettings(uint32 guildGuid) { return m_guildSettings[guildGuid]; }

        /**
         * @brief Files a membership request to a guild
         * @param guildGuid The guild's database guid.
         * @param MembershipRequest An object storing all data related to the request.
         */
        void AddMembershipRequest(uint32 guildGuid, MembershipRequest const& request);

        /**
         * @brief Removes all membership request from a player.
         * @param playerId The player's database guid whose application shall be deleted.
         */
        void RemoveAllMembershipRequestsFromPlayer(uint32 playerId);

        /**
         * @brief Removes a membership request to a guild.
         * @param playerId The player's database guid whose application shall be deleted.
         * @param guildId  The guild's database guid
         */
        void RemoveMembershipRequest(uint32 playerId, uint32 guildId);

        /// wipes everything related to a guild. Used when that guild is disbanded
        void DeleteGuild(uint32 guildId);

        /**
         * @brief Returns a set of membership requests for a guild
         * @param guildGuid The guild's database guid.
         */
        std::vector<MembershipRequest> GetAllMembershipRequestsForGuild(uint32 guildGuid) { return m_membershipRequests[guildGuid]; }

        /**
         * @brief Returns a list of membership requests for a player.
         * @param playerGuid The player's database guid.
         */
        std::list<MembershipRequest> GetAllMembershipRequestsForPlayer(uint32 playerGuid);

        /**
         * @brief Returns a store of guilds matching the settings provided, using bitmask operators.
         * @param settings The player's finder settings
         * @param teamId   The player's faction (TEAM_ALLIANCE or TEAM_HORDE)
         */
        LFGuildStore GetGuildsMatchingSetting(LFGuildPlayer& settings, Team faction);

        /// Provided a player DB guid and a guild DB guid, determines if a pending request is filed with these keys.
        bool HasRequest(uint32 playerId, uint32 guildId);

        /// Counts the amount of pending membership requests, given the player's db guid.
        uint8 CountRequestsFromPlayer(uint32 playerId);

        void SendApplicantListUpdate(Guild& guild);
        void SendMembershipRequestListUpdate(Player& player);
};

#define sGuildFinderMgr MaNGOS::Singleton<GuildFinderMgr>::Instance()

#endif
