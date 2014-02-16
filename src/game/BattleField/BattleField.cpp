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

#include "BattleField.h"
#include "Language.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Object.h"
#include "GameObject.h"
#include "Player.h"
#include "MapManager.h"
#include "SpellAuras.h"
#include "Unit.h"

BattleField::BattleField(uint32 id) : OutdoorPvP(id), m_battleFieldId(0)
{
    m_isBattleField = true;

    m_defender = TeamIndex(urand(0, 1));
    m_state = BF_STATE_COOLDOWN;
    m_timer = 15 * MINUTE * IN_MILLISECONDS;
    m_queueUpdateTimer = 30000;
    m_scoresUpdateTimer = 5000;

    bAboutSend = false;
}

void BattleField::KickPlayer(Player* plr)
{
    float x, y, z;
    if (!GetKickPosition(plr, x, y, z))
        return;

    plr->TeleportTo(plr->GetMapId(), x, y, z, plr->GetOrientation());
}

void BattleField::InitPlayerScore(Player* plr)
{
    m_playerScores[plr->GetObjectGuid()] = new BFPlayerScore();
}

void BattleField::InitPlayerScores()
{
    for (BFPlayerScoreMap::iterator itr = m_playerScores.begin(); itr != m_playerScores.end(); ++itr)
        delete itr->second;

    m_playerScores.clear();

    for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
    {
        if (!itr->first)
            continue;

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        InitPlayerScore(plr);
    }
}

bool BattleField::UpdatePlayerScores()
{
    bool bRemoved = false;
    for (BFPlayerScoreMap::iterator itr = m_playerScores.begin(); itr != m_playerScores.end();)
    {
        if (itr->second->removeTime && itr->second->removeTime + itr->second->removeDelay < time(NULL))
        {
            delete itr->second;
            if (Player* plr = GetMap()->GetPlayer(itr->first))
                KickPlayer(plr);

            if (IsMember(itr->first))
            {
                RemovePlayerFromRaid(itr->first);
                bRemoved = true;
            }

            m_playerScores.erase(itr++);

        }
        else
            ++itr;
    }

    return bRemoved;
}

Map* BattleField::GetMap()
{
    return sMapMgr.CreateMap(m_mapId, NULL);
}

Group* BattleField::GetFreeRaid(TeamIndex teamIdx)
{
    for (std::set<Group*>::iterator itr = m_Raids[teamIdx].begin(); itr != m_Raids[teamIdx].end(); ++itr)
        if (!(*itr)->IsFull())
            return *itr;

    return NULL;
}

bool BattleField::IsMember(ObjectGuid guid)
{
    return GetGroupFor(guid) != NULL;
}

Group* BattleField::GetGroupFor(ObjectGuid guid)
{
    for (uint8 i = 0; i < PVP_TEAM_COUNT; ++i)
        for (std::set<Group*>::iterator itr = m_Raids[i].begin(); itr != m_Raids[i].end(); ++itr)
            if ((*itr)->IsMember(guid))
                return *itr;

    return NULL;
}

bool BattleField::AddPlayerToRaid(Player* player)
{
    if (!player->IsInWorld())
        return false;

    DEBUG_LOG("Battlefield: Adding player %s to raid", player->GetGuidStr().c_str());

    if (Group* group = player->GetGroup())
    {
        DEBUG_LOG("Battlefield: Player %s already has group %s, uninviting", player->GetGuidStr().c_str(), group->GetObjectGuid().GetString().c_str());
        group->RemoveMember(player->GetObjectGuid(), 0);
    }

    TeamIndex teamIdx = GetTeamIndex(player->GetTeam());
    Group* group = GetFreeRaid(teamIdx);
    if (!group)
    {
        DEBUG_LOG("Battlefield: No free raid for %s!", player->GetGuidStr().c_str());
        if (IsTeamFull(teamIdx))
        {
            DEBUG_LOG("Battlefield: BattleField is full! Can't add player %s!", player->GetGuidStr().c_str());
            return false;
        }

        DEBUG_LOG("Battlefield: Trying to create new group for %s!", player->GetGuidStr().c_str());
        group = new Group(GROUPTYPE_NORMAL);
        group->SetBattlefieldGroup(this);
        if (group->Create(player->GetObjectGuid(), player->GetName()))
            DEBUG_LOG("Battlefield: Successfully created new group %s", group->GetObjectGuid().GetString().c_str());
        else
            DEBUG_LOG("Failed to create group!");

        m_Raids[teamIdx].insert(group);
    }
    else if (group->IsMember(player->GetObjectGuid()))
    {
        DEBUG_LOG("Battlefield: Raid already has players %s, making some shit", player->GetGuidStr().c_str());
        uint8 subgroup = group->GetMemberGroup(player->GetObjectGuid());
        player->SetBattleGroundRaid(group->GetObjectGuid(), subgroup);
    }
    else
    {
        if (IsTeamFull(teamIdx))
        {
            DEBUG_LOG("Battlefield: Group %s found, but battlefield is full! Can't add player %s!", group->GetObjectGuid().GetString().c_str(), player->GetGuidStr().c_str());
            return false;
        }

        return group->AddMember(player->GetObjectGuid(), player->GetName());
    }

    return true;
}

bool BattleField::RemovePlayerFromRaid(ObjectGuid guid)
{
    if (Group* group = GetGroupFor(guid))
    {
        if (group->RemoveMember(guid, 0) == 0)
            delete group;

        return true;
    }

    return false;
}

uint32 BattleField::GetPlayerCountByTeam(TeamIndex teamIdx)
{
    uint32 count = 0;
    for (std::set<Group*>::iterator itr = m_Raids[teamIdx].begin(); itr != m_Raids[teamIdx].end(); ++itr)
        count += (*itr)->GetMembersCount();

    return count;
}

bool BattleField::IsTeamFull(TeamIndex teamIdx)
{
    return m_QueuedPlayers[teamIdx].size() + m_InvitedPlayers[teamIdx].size() + GetPlayerCountByTeam(teamIdx) >= m_maxPlayersPerTeam;
}

void BattleField::InvitePlayerToQueue(Player* player)
{
    if (IsMember(player->GetObjectGuid()))
        return;

    TeamIndex teamIdx = GetTeamIndex(player->GetTeam());
    // he is in queue or waiting to accept teleport button
    if (m_QueuedPlayers[teamIdx].find(player->GetObjectGuid()) != m_QueuedPlayers[teamIdx].end())
        return;
    if (m_InvitedPlayers[teamIdx].find(player->GetObjectGuid()) != m_InvitedPlayers[teamIdx].end())
        return;

    if (!IsTeamFull(teamIdx))
        player->GetSession()->SendBfInvitePlayerToQueue(GetBattlefieldGuid());
    //else
    //    player->GetSession()->SendBfQueueInviteResponse(GetBattlefieldGuid(), GetQueueGuid(), m_zoneId, true, true);
}

void BattleField::OnPlayerInviteResponse(Player* plr, bool accept)
{
    if (!accept || IsMember(plr->GetObjectGuid()))
        return;

    TeamIndex teamIdx = GetTeamIndex(plr->GetTeam());
    // not needed
    std::map<ObjectGuid, time_t>::iterator itr = m_InvitedPlayers[teamIdx].find(plr->GetObjectGuid());
    if (itr != m_InvitedPlayers[teamIdx].end())
        m_InvitedPlayers[teamIdx].erase(itr);

    if (IsTeamFull(teamIdx))
    {
        plr->GetSession()->SendBfQueueInviteResponse(GetBattlefieldGuid(), GetQueueGuid(), m_zoneId, true, true);
        return;
    }

    if (m_QueuedPlayers[teamIdx].find(plr->GetObjectGuid()) == m_QueuedPlayers[teamIdx].end())
    {
        if (m_state == BF_STATE_IN_PROGRESS)
        {
            m_InvitedPlayers[teamIdx][plr->GetObjectGuid()] = time(NULL) + BF_TIME_TO_ACCEPT;
            plr->GetSession()->SendBfInvitePlayerToWar(GetBattlefieldGuid(), m_zoneId, BF_TIME_TO_ACCEPT);
        }
        else
        {
            m_QueuedPlayers[teamIdx].insert(plr->GetObjectGuid());
            plr->GetSession()->SendBfQueueInviteResponse(GetBattlefieldGuid(), GetQueueGuid(), m_zoneId, true, false);
        }
    }
}

bool BattleField::OnPlayerPortResponse(Player* plr, bool accept)
{
    if (IsMember(plr->GetObjectGuid()))
        return false;

    if (accept)
    {
        TeamIndex teamIdx = GetTeamIndex(plr->GetTeam());
        std::map<ObjectGuid, time_t>::iterator itr = m_InvitedPlayers[teamIdx].find(plr->GetObjectGuid());
        // he was not invited by core
        if (itr == m_InvitedPlayers[teamIdx].end())
            return false;

        m_InvitedPlayers[teamIdx].erase(itr);

        if (AddPlayerToRaid(plr))
        {
            DEBUG_LOG("Battlefield: AddPlayerToRaid for %s returned: TRUE", plr->GetGuidStr().c_str());
            plr->GetSession()->SendBfEntered(GetBattlefieldGuid());
        }
        else
            DEBUG_LOG("Battlefield: AddPlayerToRaid for %s returned: FALSE", plr->GetGuidStr().c_str());

        if (plr->GetZoneId() == m_zoneId)
        {
            if (m_playerScores.find(plr->GetObjectGuid()) == m_playerScores.end())  // must never happen
                InitPlayerScore(plr);

            SendUpdateWorldStatesTo(plr);
        }
        //else
            // controlled in script

        return true;
    }
    else if (m_state == BF_STATE_IN_PROGRESS && plr->GetZoneId() == m_zoneId)
    {
        plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
        if (m_playerScores.find(plr->GetObjectGuid()) != m_playerScores.end())
        {
            m_playerScores[plr->GetObjectGuid()]->removeTime = time(NULL);
            m_playerScores[plr->GetObjectGuid()]->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
        }

        return true;
    }

    return false;
}

bool BattleField::OnPlayerQueueExitRequest(Player* plr)
{
    if (IsMember(plr->GetObjectGuid()))
    {
        plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
        RemovePlayerFromRaid(plr->GetObjectGuid());
        SendRemoveWorldStates(plr);
        if (m_playerScores.find(plr->GetObjectGuid()) != m_playerScores.end())
        {
            m_playerScores[plr->GetObjectGuid()]->removeTime = time(NULL);
            m_playerScores[plr->GetObjectGuid()]->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
        }
        return true;
    }

    std::set<ObjectGuid>::iterator itr = m_QueuedPlayers[GetTeamIndex(plr->GetTeam())].find(plr->GetObjectGuid());
    if (itr != m_QueuedPlayers[GetTeamIndex(plr->GetTeam())].end())
    {
        m_QueuedPlayers[GetTeamIndex(plr->GetTeam())].erase(itr);

        plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
        if (m_playerScores.find(plr->GetObjectGuid()) != m_playerScores.end())
        {
            m_playerScores[plr->GetObjectGuid()]->removeTime = time(NULL);
            m_playerScores[plr->GetObjectGuid()]->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
        }
        return true;
    }

    return false;
}

bool BattleField::OnGroupDeleted(Group* group)
{
    for (uint32 i = 0; i < 2; ++i)
        for (std::set<Group*>::iterator itr = m_Raids[i].begin(); itr != m_Raids[i].end(); ++itr)
            if (*itr == group)
            {
                m_Raids[i].erase(itr);
                return true;
            }

    return false;
}

void BattleField::OnPlayerGroupDisband(Player* plr)
{
    SendRemoveWorldStates(plr);
    plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);

    if (m_playerScores.find(plr->GetObjectGuid()) != m_playerScores.end())
    {
        m_playerScores[plr->GetObjectGuid()]->removeTime = time(NULL);
        m_playerScores[plr->GetObjectGuid()]->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
    }
}

void BattleField::HandlePlayerAFK(Player* plr)
{
    if (m_state != BF_STATE_IN_PROGRESS || !IsMember(plr->GetObjectGuid()))
        return;

    RemovePlayerFromRaid(plr->GetObjectGuid());
    plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
    KickPlayer(plr);
}

bool BFObject::IsIntact() const
{
    return state == BF_OBJECTSTATE_NEUTRAL_INTACT || state == BF_OBJECTSTATE_HORDE_INTACT || state == BF_OBJECTSTATE_ALLIANCE_INTACT;
}

bool BFObject::IsDamaged() const
{
    return state == BF_OBJECTSTATE_NEUTRAL_DAMAGED || state == BF_OBJECTSTATE_HORDE_DAMAGED || state == BF_OBJECTSTATE_ALLIANCE_DAMAGED;
}

bool BFObject::IsDestroyed() const
{
    return state == BF_OBJECTSTATE_NEUTRAL_DESTROYED || state == BF_OBJECTSTATE_HORDE_DESTROYED || state == BF_OBJECTSTATE_ALLIANCE_DESTROYED;
}

void BFObject::SendUpdateWorldState()
{
    opvp->SendUpdateWorldStateForMap(worldState, state, ((BattleField*)opvp)->GetMap());
}

void BattleField::StartBattle(TeamIndex defender)
{
    m_startTime = time(NULL);
    m_state = BF_STATE_IN_PROGRESS;
    m_timer = m_battleDuration;
    m_defender = defender;

    InitPlayerScores();

    DEBUG_LOG("Disbanding groups");
    for (uint32 i = 0; i < PVP_TEAM_COUNT; ++i)
    {
        while (!m_Raids[i].empty())
        {
            Group* group = *m_Raids[i].begin();
            group->Disband();
            delete group;
        }
    }

    for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
    {
        if (!itr->first)
            continue;

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        uint8 idx = 0;
        std::set<ObjectGuid>::iterator itr2 = m_QueuedPlayers[idx].find(itr->first);
        if (itr2 == m_QueuedPlayers[idx].end())
        {
            ++idx;
            itr2 = m_QueuedPlayers[idx].find(itr->first);
        }

        if (itr2 != m_QueuedPlayers[idx].end())
        {
            m_InvitedPlayers[idx][itr->first] = time(NULL) + BF_TIME_TO_ACCEPT;
            plr->GetSession()->SendBfInvitePlayerToWar(GetBattlefieldGuid(), m_zoneId, BF_TIME_TO_ACCEPT);
            m_QueuedPlayers[idx].erase(itr2);
        }
        else
        {
            plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
            SendRemoveWorldStates(plr);
            if (m_playerScores.find(itr->first) != m_playerScores.end())
            {
                m_playerScores[itr->first]->removeTime = time(NULL);
                m_playerScores[itr->first]->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
            }
        }
    }

    // non-zone queued players
    for (uint32 i = 0; i < PVP_TEAM_COUNT; ++i)
    {
        for (std::set<ObjectGuid>::iterator itr = m_QueuedPlayers[i].begin(); itr != m_QueuedPlayers[i].end();)
        {
            if (Player* plr = sObjectMgr.GetPlayer(*itr, true))
            {
                m_InvitedPlayers[i][plr->GetObjectGuid()] = time(NULL) + BF_TIME_TO_ACCEPT;
                plr->GetSession()->SendBfInvitePlayerToWar(GetBattlefieldGuid(), m_zoneId, BF_TIME_TO_ACCEPT);
            }

            m_QueuedPlayers[i].erase(itr++);
        }
    }
}

void BattleField::EndBattle(TeamIndex winner, bool byTimer)
{
    m_defender = winner;
    m_state = BF_STATE_COOLDOWN;
    m_timer = m_cooldownDuration;
    bInvited = false;
    bAboutSend = false;

    RewardPlayersAtEnd(winner);
}

void BattleField::Update(uint32 diff)
{
    if (m_timer < diff)
    {
        if (m_state == BF_STATE_COOLDOWN)
            StartBattle(m_defender);
        else if (m_state == BF_STATE_IN_PROGRESS)
            EndBattle(m_defender, true);

        return;
    }
    else
        m_timer -= diff;

    if (m_state == BF_STATE_IN_PROGRESS)
        if (m_scoresUpdateTimer < diff)
        {
            m_scoresUpdateTimer = 5000;
            UpdatePlayerScores();
        }
        else
            m_scoresUpdateTimer -= diff;

    if (m_state == BF_STATE_COOLDOWN && m_timer <= m_startInviteDelay && !bInvited)
    {
        bInvited = true;
        for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
        {
            if (!itr->first)
                continue;

            Player* plr = sObjectMgr.GetPlayer(itr->first);
            if (!plr)
                continue;

            InvitePlayerToQueue(plr);
        }
    }

    if (m_state == BF_STATE_IN_PROGRESS)
    {
        for (uint32 i = 0; i < PVP_TEAM_COUNT; ++i)
        {
            for (std::map<ObjectGuid, time_t>::iterator itr = m_InvitedPlayers[i].begin(); itr != m_InvitedPlayers[i].end();)
            {
                if (itr->second < time(NULL))
                {
                    if (Player* plr = GetMap()->GetPlayer(itr->first))
                        plr->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
                    m_InvitedPlayers[i].erase(itr++);
                }
                else
                    ++itr;
            }
        }
    }

    if (m_queueUpdateTimer < diff)
    {
        m_queueUpdateTimer = 30000;

        for (uint32 i = 0; i < PVP_TEAM_COUNT; ++i)
        {
            for (std::set<ObjectGuid>::iterator itr = m_QueuedPlayers[i].begin(); itr != m_QueuedPlayers[i].end();)
            {
                if (!sObjectMgr.GetPlayer(*itr, true))
                    m_QueuedPlayers[i].erase(itr++);
                else
                    ++itr;
            }
        }
    }
    else
        m_queueUpdateTimer -= diff;
}

void BattleField::HandlePlayerEnterZone(Player* pPlayer, bool isMainZone)
{
    OutdoorPvP::HandlePlayerEnterZone(pPlayer, isMainZone);

    if (!IsMember(pPlayer->GetObjectGuid()))
    {
        if (m_state == BF_STATE_IN_PROGRESS)
        {
            m_InvitedPlayers[GetTeamIndex(pPlayer->GetTeam())][pPlayer->GetObjectGuid()] = time(NULL) + BF_TIME_TO_ACCEPT;
            pPlayer->GetSession()->SendBfInvitePlayerToWar(GetBattlefieldGuid(), m_zoneId, BF_TIME_TO_ACCEPT);
        }
        else if (m_state == BF_STATE_COOLDOWN && m_timer < m_startInviteDelay)
            InvitePlayerToQueue(pPlayer);
    }

    BFPlayerScoreMap::iterator itr = m_playerScores.find(pPlayer->GetObjectGuid());
    if (itr == m_playerScores.end())
        InitPlayerScore(pPlayer);
    else
    {
        if (m_state == BF_STATE_IN_PROGRESS && itr->second->removeTime && m_startTime > itr->second->removeTime)
            SetupPlayerPosition(pPlayer);

        itr->second->removeTime = 0;
    }
}

void BattleField::HandlePlayerLeaveZone(Player* pPlayer, bool isMainZone)
{
    OutdoorPvP::HandlePlayerLeaveZone(pPlayer, isMainZone);

    BFPlayerScoreMap::iterator itr = m_playerScores.find(pPlayer->GetObjectGuid());
    if (itr != m_playerScores.end())
    {
        if (IsMember(itr->first))
        {
            pPlayer->GetSession()->SendBfLeaveMessage(GetBattlefieldGuid(), BATTLEFIELD_LEAVE_REASON_EXITED);
            itr->second->removeTime = time(NULL);
            itr->second->removeDelay = BF_UNACCEPTED_REMOVE_DELAY;
        }
        else
            m_playerScores.erase(itr);
    }
}

void BattleField::SpawnCreature(Creature* pCreature, uint32 respawnTime, bool despawn)
{
    if (despawn)
    {
        pCreature->SetRespawnDelay(respawnTime ? respawnTime : 7 * DAY);
        pCreature->ForcedDespawn();
    }
    else
    {
        pCreature->SetRespawnDelay(respawnTime ? respawnTime : 5 * MINUTE);
        pCreature->Respawn();
    }
}

Creature* BattleField::SpawnCreature(ObjectGuid guid, uint32 respawnTime, bool despawn)
{
    if (Creature* pCreature = GetMap()->GetCreature(guid))
    {
        SpawnCreature(pCreature, respawnTime, despawn);

        return pCreature;
    }

    return NULL;
}

void BattleField::SendWarningToAll(int32 entry)
{
    for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
    {
        if (!itr->first)
            continue;

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        if (plr->GetZoneId() != m_zoneId)
            continue;

        int32 loc_idx = plr->GetSession()->GetSessionDbLocaleIndex();

        char const* text = sObjectMgr.GetMangosString(entry, loc_idx);

        WorldPacket data(SMSG_MESSAGECHAT, 200);
        data << uint8(CHAT_MSG_RAID_BOSS_EMOTE);
        data << uint32(LANG_UNIVERSAL);
        data << ObjectGuid();
        data << uint32(0);
        data << uint32(1);
        data << uint8(0);
        data << ObjectGuid();
        data << uint32(strlen(text) + 1);
        data << text;
        data << uint8(0);
        plr->GetSession()->SendPacket(&data);
    }
}

void BattleField::SetupPlayerPositions()
{
    for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
    {
        if (!itr->first || !IsMember(itr->first))
            continue;

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        SetupPlayerPosition(plr);
    }
}

void BattleField::PlayerLoggedIn(Player* plr)
{
    if (Group* group = GetGroupFor(plr->GetObjectGuid()))
    {
        uint8 subgroup = group->GetMemberGroup(plr->GetObjectGuid());
        plr->SetBattleGroundRaid(group->GetObjectGuid(), subgroup);
    }
}

void BattleField::QuestCreditTeam(uint32 credit, Team team, WorldObject* source, float radius)
{
    for (GuidZoneMap::iterator itr = m_zonePlayers.begin(); itr != m_zonePlayers.end(); ++itr)
    {
        if (!itr->first || !IsMember(itr->first))
            continue;

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
            continue;

        if (plr->GetTeam() != team || source && radius > 0.0f && source->GetDistance2d(plr->GetPositionX(), plr->GetPositionY()) > radius)
            continue;

        plr->KilledMonsterCredit(credit);
    }
}

void BattleField::SendUpdateWorldStatesToAll()
{
    Map::PlayerList const& pList = GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator itr = pList.begin(); itr != pList.end(); ++itr)
    {
        Player* player = itr->getSource();
        if (!player || !player->IsInWorld())
            continue;

        // timer sent before function
        SendUpdateWorldStatesTo(player);
    }
}

void BFObject::SetIntact()
{
    if (owner == TEAM_INDEX_ALLIANCE)
        state = BF_OBJECTSTATE_ALLIANCE_INTACT;
    else if (owner == TEAM_INDEX_HORDE)
        state = BF_OBJECTSTATE_HORDE_INTACT;
    else
        state = BF_OBJECTSTATE_NEUTRAL_INTACT;
}

void BFObject::SetDamaged()
{
    if (owner == TEAM_INDEX_ALLIANCE)
        state = BF_OBJECTSTATE_ALLIANCE_DAMAGED;
    else if (owner == TEAM_INDEX_HORDE)
        state = BF_OBJECTSTATE_HORDE_DAMAGED;
    else
        state = BF_OBJECTSTATE_NEUTRAL_DAMAGED;
}

void BFObject::SetDestroyed()
{
    if (owner == TEAM_INDEX_ALLIANCE)
        state = BF_OBJECTSTATE_ALLIANCE_DESTROYED;
    else if (owner == TEAM_INDEX_HORDE)
        state = BF_OBJECTSTATE_HORDE_DESTROYED;
    else
        state = BF_OBJECTSTATE_NEUTRAL_DESTROYED;
}

void BFObject::UpdateStateForOwner()
{
    if (IsIntact())
        SetIntact();
    else if (IsDamaged())
        SetDamaged();
    else if (IsDestroyed())
        SetDestroyed();
}

void BFObject::InitFor(TeamIndex teamIdx, bool reset)
{
    owner = teamIdx;
    if (reset)
        SetIntact();
    else
        UpdateStateForOwner();

    if (GameObject* obj = opvp->GetMap()->GetGameObject(guid))
    {
        if (reset)
            obj->Rebuild(NULL);

        obj->SetUInt32Value(GAMEOBJECT_FACTION, BFFactions[owner]);
    }
}
