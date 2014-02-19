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

#ifndef BATTLEFIELD_H
#define BATTLEFIELD_H

#include "Common.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "OutdoorPvP/OutdoorPvP.h"

class BattleField;
class Group;

enum BFState
{
    BF_STATE_COOLDOWN       = 0,
    BF_STATE_IN_PROGRESS    = 1
};

enum
{
    BF_INACTIVE_REMOVE_DELAY    = 5 * MINUTE,
    BF_UNACCEPTED_REMOVE_DELAY  = 10,
    BF_TIME_TO_ACCEPT           = 20,
};

enum BFObjectState
{
    BF_OBJECTSTATE_NONE                 = 0,
    BF_OBJECTSTATE_NEUTRAL_INTACT       = 1,
    BF_OBJECTSTATE_NEUTRAL_DAMAGED      = 2,
    BF_OBJECTSTATE_NEUTRAL_DESTROYED    = 3,
    BF_OBJECTSTATE_HORDE_INTACT         = 4,
    BF_OBJECTSTATE_HORDE_DAMAGED        = 5,
    BF_OBJECTSTATE_HORDE_DESTROYED      = 6,
    BF_OBJECTSTATE_ALLIANCE_INTACT      = 7,
    BF_OBJECTSTATE_ALLIANCE_DAMAGED     = 8,
    BF_OBJECTSTATE_ALLIANCE_DESTROYED   = 9,
};

const uint32 BFFactions[3] = { 1, 2, 35 };

enum
{
    SPELL_ALLIANCE_CONTROLS_FACTORY_PHASE_SHIFT = 56617, // PHASE 32
    SPELL_HORDE_CONTROLS_FACTORY_PHASE_SHIFT    = 56618, // PHASE 16

    SPELL_ALLIANCE_CONTROL_PHASE_SHIFT          = 60027, // PHASE 65
    SPELL_HORDE_CONTROL_PHASE_SHIFT             = 60028, // PHASE 129
};

class BFObject
{
    public:
        BFObject(uint32 _id, BattleField* _opvp) : id(_id), opvp(_opvp) { }

        uint32 id;
        BattleField* opvp;
        TeamIndex owner;
        uint32 worldState;
        BFObjectState state;
        ObjectGuid guid;

        virtual void InitFor(TeamIndex teamIdx, bool reset = true);
        bool IsIntact() const;
        bool IsDamaged() const;
        bool IsDestroyed() const;

        void SetIntact();
        void SetDamaged();
        void SetDestroyed();

        void UpdateStateForOwner();

        virtual void SendUpdateWorldState();
};

class BFPlayerScore
{
    public:
        BFPlayerScore() : removeTime(0), removeDelay(0) { }
        virtual ~BFPlayerScore() { }

        time_t removeTime;
        uint32 removeDelay;
};

typedef std::map<ObjectGuid, BFPlayerScore*> BFPlayerScoreMap;

class MANGOS_DLL_SPEC BattleField : public OutdoorPvP
{
    public:
        BattleField(uint32 id);

        virtual void HandlePlayerEnterZone(Player* pPlayer, bool isMainZone) override;
        virtual void HandlePlayerLeaveZone(Player* pPlayer, bool isMainZone) override;

        void OnPlayerInviteResponse(Player* plr, bool accept);
        virtual bool OnPlayerPortResponse(Player* plr, bool accept);
        virtual bool OnPlayerQueueExitRequest(Player* plr);
        virtual bool OnGroupDeleted(Group* group);
        virtual void OnPlayerGroupDisband(Player* plr);
        void HandlePlayerAFK(Player* plr);
        void PlayerLoggedIn(Player* plr);

        virtual void StartBattle(TeamIndex defender);
        virtual void EndBattle(TeamIndex winner, bool byTimer);
        virtual void Update(uint32 diff) override;

        Map* GetMap();
        TeamIndex GetDefender() const { return m_defender; }
        TeamIndex GetAttacker() const { return m_defender == TEAM_INDEX_ALLIANCE ? TEAM_INDEX_HORDE : TEAM_INDEX_ALLIANCE; }
        BFState GetState() const { return m_state; }
        uint32 GetTimer() const { return m_timer; }
        void SetTimer(uint32 value) { m_timer = value; }

        virtual void GraveYardChanged(uint8 id, TeamIndex newOwner) { };

        BFPlayerScoreMap& GetPlayerScoreMap() { return m_playerScores; }

        virtual bool CriteriaMeets(uint32 criteriaId, Player* plr) { return false; }

        uint32 GetBattleDuration() const { return m_battleDuration; }
        uint32 GetStartInviteDelay() const { return m_startInviteDelay; }
        uint32 GetCooldownDuration() const { return m_cooldownDuration; }
        uint32 GetMaxPlayersPerTeam() const { return m_maxPlayersPerTeam; }
        uint32 GetStopTeleportingTime() const { return m_stopTeleportingTime; }

        void SendWarningToAll(int32 entry);

        void QuestCreditTeam(uint32 credit, Team team, WorldObject* source = NULL, float radius = -1.0f);

        uint32 GetZoneId() { return m_zoneId; }

        bool IsMember(ObjectGuid guid) override;

        void SpawnCreature(Creature* pCreature, uint32 respawnTime = 0, bool despawn = false);
        Creature* SpawnCreature(ObjectGuid guid, uint32 respawnTime = 0, bool despawn = false);

        bool AddPlayerToRaid(Player* player);
        bool RemovePlayerFromRaid(ObjectGuid guid);

        void InvitePlayerToQueue(Player* player);

        uint32 GetBattlefieldId() const { return m_battleFieldId; }
        ObjectGuid GetBattlefieldGuid() const { return ObjectGuid(HIGHGUID_BATTLEGROUND, uint32((2 << 16) | m_battleFieldId)); }
        ObjectGuid GetQueueGuid() const { return ObjectGuid(uint64((HIGHGUID_BATTLEGROUND << 24) | m_battleFieldId)); }

    protected:
        void KickPlayer(Player* plr);
        virtual bool GetKickPosition(Player* plr, float& x, float& y, float& z) { return false; }
        virtual void InitPlayerScore(Player* plr);
        void InitPlayerScores();
        virtual bool UpdatePlayerScores();

        Group* GetFreeRaid(TeamIndex teamIdx);
        Group* GetGroupFor(ObjectGuid guid);
        uint32 GetPlayerCountByTeam(TeamIndex teamIdx);
        bool IsTeamFull(TeamIndex teamIdx);

        virtual void RewardPlayersAtEnd(TeamIndex winner) { };

        void SetupPlayerPositions();
        virtual void SetupPlayerPosition(Player* player) { };

        virtual void SendUpdateWorldStatesToAll();
        virtual void SendUpdateWorldStatesTo(Player* player) { }

        // variables
        uint32 m_battleFieldId;
        BFPlayerScoreMap m_playerScores;
        uint32 m_mapId;
        std::set<Group*> m_Raids[PVP_TEAM_COUNT];
        std::set<ObjectGuid> m_QueuedPlayers[PVP_TEAM_COUNT];           // players that are in queue
        std::map<ObjectGuid, time_t> m_InvitedPlayers[PVP_TEAM_COUNT];  // player to whom teleport invitation is send and who are waited to accept

        uint32 m_zoneId;
        uint32 m_queueUpdateTimer;
        uint32 m_scoresUpdateTimer;

        TeamIndex m_defender;
        BFState m_state;
        uint32 m_timer;

        uint32 m_battleDuration;
        uint32 m_startInviteDelay;
        uint32 m_cooldownDuration;
        uint32 m_maxPlayersPerTeam;
        uint32 m_stopTeleportingTime;

        time_t m_startTime;

        bool bInvited;
        bool bAboutSend;
};

#endif