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

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundBG.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "WorldPacket.h"
#include "Util.h"
#include "MapManager.h"

BattleGroundBG::BattleGroundBG()
{
    m_BuffChange = true;
    m_BgObjects.resize(BG_BG_OBJECT_MAX);

    m_Points_Trigger[BG_BG_NODE_LIGHTHOUSE]    = TR_LIGHTHOUSE_BUFF;
    m_Points_Trigger[BG_BG_NODE_WATERWORKS] = TR_WATERWORKS_BUFF;
    m_Points_Trigger[BG_BG_NODE_MINES]    = TR_MINES_BUFF;

    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_BG_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_BG_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_BG_HAS_BEGUN;
}

BattleGroundBG::~BattleGroundBG()
{
}

void BattleGroundBG::Update(uint32 diff)
{
    BattleGround::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        m_PointAddingTimer -= diff;
        if (m_PointAddingTimer <= 0)
        {
            m_PointAddingTimer = BG_BG_FPOINTS_TICK_TIME;
            if (m_TeamPointsCount[TEAM_INDEX_ALLIANCE] > 0)
                AddPoints(ALLIANCE, BG_BG_TickPoints[m_TeamPointsCount[TEAM_INDEX_ALLIANCE] - 1]);
            if (m_TeamPointsCount[TEAM_INDEX_HORDE] > 0)
                AddPoints(HORDE, BG_BG_TickPoints[m_TeamPointsCount[TEAM_INDEX_HORDE] - 1]);
        }
    }
}

void BattleGroundBG::StartingEventCloseDoors()
{
}

void BattleGroundBG::StartingEventOpenDoors()
{
    // eye-doors are despawned, not opened
    SpawnEvent(BG_EVENT_DOOR, 0, false);

    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        // randomly spawn buff
        uint8 buff = urand(0, 2);
        SpawnBGObject(m_BgObjects[BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE + buff + i * 3], RESPAWN_IMMEDIATELY);
    }

    // Players that join battleground after start are not eligible to get achievement.
    StartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, BG_BG_EVENT_START_BATTLE);
}

void BattleGroundBG::AddPoints(Team team, uint32 points)
{
    TeamIndex team_index = GetTeamIndexByTeamId(team);
    m_TeamScores[team_index] += points;
    m_HonorScoreTics[team_index] += points;
    if (m_HonorScoreTics[team_index] >= m_HonorTics)
    {
        RewardHonorToTeam(GetBonusHonorFromKill(1), team);
        m_HonorScoreTics[team_index] -= m_HonorTics;
    }
    UpdateTeamScore(team);
}

void BattleGroundBG::CheckSomeoneJoinedPoint()
{
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        uint8 j = 0;
        while (j < m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].size())
        {
            Player* plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS][j]);
            if (!plr)
            {
                sLog.outError("BattleGroundBG:CheckSomeoneJoinedPoint: %s not found!", m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS][j].GetString().c_str());
                ++j;
                continue;
            }
            if (plr->CanUseCapturePoint() &&
                    plr->IsWithinDist3d(BG_BG_NodePositions[i][0], BG_BG_NodePositions[i][1], BG_BG_NodePositions[i][2], BG_BG_POINT_RADIUS))
            {
                // player joined point!
                // show progress bar
                UpdateWorldStateForPlayer(BG_PROGRESS_BAR_PERCENT_GREY, BG_PROGRESS_BAR_PERCENT_GREY, plr);
                UpdateWorldStateForPlayer(BG_PROGRESS_BAR_STATUS, m_PointBarStatus[i], plr);
                UpdateWorldStateForPlayer(BG_PROGRESS_BAR_SHOW, BG_PROGRESS_BAR_SHOW, plr);
                // add player to point
                m_PlayersNearPoint[i].push_back(m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS][j]);
                // remove player from "free space"
                m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].erase(m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].begin() + j);
            }
            else
                ++j;
        }
    }
}

void BattleGroundBG::CheckSomeoneLeftPoint()
{
    // reset current point counts
    for (uint8 i = 0; i < 2 * BG_BG_NODES_MAX; ++i)
        m_CurrentPointPlayersCount[i] = 0;
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        uint8 j = 0;
        while (j < m_PlayersNearPoint[i].size())
        {
            Player* plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[i][j]);
            if (!plr)
            {
                sLog.outError("BattleGroundBG:CheckSomeoneLeftPoint %s not found!", m_PlayersNearPoint[i][j].GetString().c_str());
                // move nonexistent player to "free space" - this will cause many error showing in log, but it is a very important bug
                m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].push_back(m_PlayersNearPoint[i][j]);
                m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                ++j;
                continue;
            }
            if (!plr->CanUseCapturePoint() ||
                    !plr->IsWithinDist3d(BG_BG_NodePositions[i][0], BG_BG_NodePositions[i][1], BG_BG_NodePositions[i][2], BG_BG_POINT_RADIUS))
                // move player out of point (add him to players that are out of points
            {
                m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].push_back(m_PlayersNearPoint[i][j]);
                m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                UpdateWorldStateForPlayer(BG_PROGRESS_BAR_SHOW, BG_BG_PROGRESS_BAR_DONT_SHOW, plr);
            }
            else
            {
                // player is neat flag, so update count:
                m_CurrentPointPlayersCount[2 * i + GetTeamIndexByTeamId(plr->GetTeam())]++;
                ++j;
            }
        }
    }
}

void BattleGroundBG::UpdatePointStatuses()
{
    for (uint8 point = 0; point < BG_BG_NODES_MAX; ++point)
    {
        if (m_PlayersNearPoint[point].empty())
            continue;
        // count new point bar status:
        m_PointBarStatus[point] += (m_CurrentPointPlayersCount[2 * point] - m_CurrentPointPlayersCount[2 * point + 1] < BG_BG_POINT_MAX_CAPTURERS_COUNT) ? m_CurrentPointPlayersCount[2 * point] - m_CurrentPointPlayersCount[2 * point + 1] : BG_BG_POINT_MAX_CAPTURERS_COUNT;

        if (m_PointBarStatus[point] > BG_BG_PROGRESS_BAR_ALI_CONTROLLED)
            // point is fully alliance's
            m_PointBarStatus[point] = BG_BG_PROGRESS_BAR_ALI_CONTROLLED;
        if (m_PointBarStatus[point] < BG_BG_PROGRESS_BAR_HORDE_CONTROLLED)
            // point is fully horde's
            m_PointBarStatus[point] = BG_BG_PROGRESS_BAR_HORDE_CONTROLLED;

        Team pointOwnerTeamId;
        // find which team should own this point
        if (m_PointBarStatus[point] <= BG_BG_PROGRESS_BAR_NEUTRAL_LOW)
            pointOwnerTeamId = HORDE;
        else if (m_PointBarStatus[point] >= BG_BG_PROGRESS_BAR_NEUTRAL_HIGH)
            pointOwnerTeamId = ALLIANCE;
        else
            pointOwnerTeamId = TEAM_NONE;

        for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
        {
            if (Player* plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[point][i]))
            {
                UpdateWorldStateForPlayer(BG_PROGRESS_BAR_STATUS, m_PointBarStatus[point], plr);
                // if point owner changed we must evoke event!
                if (pointOwnerTeamId != m_PointOwnedByTeam[point])
                {
                    // point was uncontrolled and player is from team which captured point
                    if (m_PointState[point] == BG_POINT_STATE_UNCONTROLLED && plr->GetTeam() == pointOwnerTeamId)
                        EventTeamCapturedPoint(plr, point);

                    // point was under control and player isn't from team which controlled it
                    if (m_PointState[point] == BG_POINT_UNDER_CONTROL && plr->GetTeam() != m_PointOwnedByTeam[point])
                        EventTeamLostPoint(plr, point);
                }
            }
        }
    }
}

void BattleGroundBG::UpdateTeamScore(Team team)
{
    uint32 score = GetTeamScore(team);

    if (score >= BG_BG_MAX_TEAM_SCORE)
    {
        score = BG_BG_MAX_TEAM_SCORE;
        EndBattleGround(team);
    }

    if (team == ALLIANCE)
        UpdateWorldState(BG_ALLIANCE_RESOURCES, score);
    else
        UpdateWorldState(BG_HORDE_RESOURCES, score);
}

void BattleGroundBG::EndBattleGround(Team winner)
{
    // win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    // complete map reward
    RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);

    BattleGround::EndBattleGround(winner);
}

void BattleGroundBG::UpdatePointsCount(Team team)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_ALLIANCE_BASE, m_TeamPointsCount[TEAM_INDEX_ALLIANCE]);
    else
        UpdateWorldState(BG_HORDE_BASE, m_TeamPointsCount[TEAM_INDEX_HORDE]);
}

void BattleGroundBG::UpdatePointsIcons(Team team, uint32 point)
{
    if (m_PointState[point] == BG_POINT_UNDER_CONTROL)
    {
        UpdateWorldState(BGPointsIconStruct[point].WorldStateControlIndex, WORLD_STATE_REMOVE);
        if (team == ALLIANCE)
            UpdateWorldState(BGPointsIconStruct[point].WorldStateAllianceControlledIndex, WORLD_STATE_ADD);
        else
            UpdateWorldState(BGPointsIconStruct[point].WorldStateHordeControlledIndex, WORLD_STATE_ADD);
    }
    else
    {
        if (team == ALLIANCE)
            UpdateWorldState(BGPointsIconStruct[point].WorldStateAllianceControlledIndex, WORLD_STATE_REMOVE);
        else
            UpdateWorldState(BGPointsIconStruct[point].WorldStateHordeControlledIndex, WORLD_STATE_REMOVE);

        UpdateWorldState(BGPointsIconStruct[point].WorldStateControlIndex, WORLD_STATE_ADD);
    }
}

void BattleGroundBG::AddPlayer(Player* plr)
{
    BattleGround::AddPlayer(plr);
    // create score and add it to map
    BattleGroundBGScore* sc = new BattleGroundBGScore;

    m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].push_back(plr->GetObjectGuid());

    m_PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattleGroundBG::RemovePlayer(Player* plr, ObjectGuid guid)
{
    // sometimes flag aura not removed :(
    for (uint8 j = BG_BG_NODES_MAX; j >= 0; --j)
    {
        for (size_t i = 0; i < m_PlayersNearPoint[j].size(); ++i)
            if (m_PlayersNearPoint[j][i] == guid)
                m_PlayersNearPoint[j].erase(m_PlayersNearPoint[j].begin() + i);
    }
}

void BattleGroundBG::HandleAreaTrigger(Player* source, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!source->isAlive())                                 // hack code, must be removed later
        return;

    switch (trigger)
    {
        case TR_LIGHTHOUSE_POINT:
            if (m_PointState[BG_BG_NODE_LIGHTHOUSE] == BG_POINT_UNDER_CONTROL && m_PointOwnedByTeam[BG_BG_NODE_LIGHTHOUSE] == source->GetTeam())
            break;
        case TR_WATERWORKS_POINT:
            if (m_PointState[BG_BG_NODE_WATERWORKS] == BG_POINT_UNDER_CONTROL && m_PointOwnedByTeam[BG_BG_NODE_WATERWORKS] == source->GetTeam())
            break;
        case TR_MINES_POINT:
            if (m_PointState[BG_BG_NODE_MINES] == BG_POINT_UNDER_CONTROL && m_PointOwnedByTeam[BG_BG_NODE_MINES] == source->GetTeam())
            break;

            break;
        default:
            sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", trigger);
            source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", trigger);
            break;
    }
}

bool BattleGroundBG::SetupBattleGround()
{
    // buffs
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(m_Points_Trigger[i]);
        if (!at)
        {
            sLog.outError("BattleGroundBG: Unknown trigger: %u", m_Points_Trigger[i]);
            continue;
        }
        if (!AddObject(BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE + i * 3, Buff_Entries[0], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
                || !AddObject(BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE + i * 3 + 1, Buff_Entries[1], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
                || !AddObject(BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE + i * 3 + 2, Buff_Entries[2], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
           )
            sLog.outError("BattleGroundBG: Cannot spawn buff");
    }

    return true;
}

void BattleGroundBG::Reset()
{
    // call parent's class reset
    BattleGround::Reset();

    m_TeamScores[TEAM_INDEX_ALLIANCE] = 0;
    m_TeamScores[TEAM_INDEX_HORDE] = 0;
    m_TeamPointsCount[TEAM_INDEX_ALLIANCE] = 0;
    m_TeamPointsCount[TEAM_INDEX_HORDE] = 0;
    m_HonorScoreTics[TEAM_INDEX_ALLIANCE] = 0;
    m_HonorScoreTics[TEAM_INDEX_HORDE] = 0;
    m_PointAddingTimer = 0;
    m_TowerCapCheckTimer = 0;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_BG_BGWeekendHonorTicks : BG_BG_NotBGWeekendHonorTicks;

    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        m_PointOwnedByTeam[i] = TEAM_NONE;
        m_PointState[i] = BG_POINT_STATE_UNCONTROLLED;
        m_PointBarStatus[i] = BG_BG_PROGRESS_BAR_STATE_MIDDLE;
        m_PlayersNearPoint[i].clear();
        m_PlayersNearPoint[i].reserve(15);                  // tip size
        m_ActiveEvents[i] = BG_BGE_NEUTRAL_TEAM;            // neutral team owns every node
    }

    m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].clear();
    m_PlayersNearPoint[BG_BG_PLAYERS_OUT_OF_POINTS].reserve(30);
}

void BattleGroundBG::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    BattleGround::HandleKillPlayer(player, killer);
    EventPlayerDroppedFlag(player);
}

void BattleGroundBG::EventTeamLostPoint(Player* source, uint32 point)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    // neutral node
    Team team = m_PointOwnedByTeam[point];

    if (!team)
        return;

    if (team == ALLIANCE)
        --m_TeamPointsCount[TEAM_INDEX_ALLIANCE];
    else
        --m_TeamPointsCount[TEAM_INDEX_HORDE];

    // it's important to set the OwnedBy before despawning spiritguides, else
    // player won't get teleported away
    m_PointOwnedByTeam[point] = TEAM_NONE;
    m_PointState[point] = BG_POINT_NO_OWNER;

    SpawnEvent(point, BG_BGE_NEUTRAL_TEAM, true);           // will despawn alliance/horde

    // buff isn't despawned

    if (team == ALLIANCE)
        SendMessageToAll(BGLoosingPointTypes[point].MessageIdAlliance, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
    else
        SendMessageToAll(BGLoosingPointTypes[point].MessageIdHorde, CHAT_MSG_BG_SYSTEM_HORDE, source);

    UpdatePointsIcons(team, point);
    UpdatePointsCount(team);
}

void BattleGroundBG::EventTeamCapturedPoint(Player* source, uint32 point)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    Team team = source->GetTeam();

    ++m_TeamPointsCount[GetTeamIndexByTeamId(team)];
    SpawnEvent(point, GetTeamIndexByTeamId(team), true);

    // buff isn't respawned

    m_PointOwnedByTeam[point] = team;
    m_PointState[point] = BG_POINT_UNDER_CONTROL;

    if (team == ALLIANCE)
        SendMessageToAll(BGCapturingPointTypes[point].MessageIdAlliance, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
    else
        SendMessageToAll(BGCapturingPointTypes[point].MessageIdHorde, CHAT_MSG_BG_SYSTEM_HORDE, source);

    UpdatePointsIcons(team, point);
    UpdatePointsCount(team);
}

void BattleGroundBG::UpdatePlayerScore(Player* source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(source->GetObjectGuid());
    if (itr == m_PlayerScores.end())                        // player not found
        return;

    switch (type)
    {
        default:
            BattleGround::UpdatePlayerScore(source, type, value);
            break;
    }

    if (achCriId)
        Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, 1, achCriId);
}

void BattleGroundBG::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, BG_HORDE_BASE,    m_TeamPointsCount[TEAM_INDEX_HORDE]);
    FillInitialWorldState(data, count, BG_ALLIANCE_BASE, m_TeamPointsCount[TEAM_INDEX_ALLIANCE]);
    FillInitialWorldState(data, count, 0xab6, 0x0);
    FillInitialWorldState(data, count, 0xab5, 0x0);
    FillInitialWorldState(data, count, 0xab4, 0x0);
    FillInitialWorldState(data, count, 0xab3, 0x0);
    FillInitialWorldState(data, count, 0xab2, 0x0);
    FillInitialWorldState(data, count, 0xab1, 0x0);
    FillInitialWorldState(data, count, 0xab0, 0x0);
    FillInitialWorldState(data, count, 0xaaf, 0x0);

    FillInitialWorldState(data, count, LIGHTHOUSE_HORDE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_LIGHTHOUSE] == HORDE && m_PointState[BG_BG_NODE_LIGHTHOUSE] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, LIGHTHOUSE_ALLIANCE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_LIGHTHOUSE] == ALLIANCE && m_PointState[BG_BG_NODE_LIGHTHOUSE] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, LIGHTHOUSE_UNCONTROL, m_PointState[BG_BG_NODE_LIGHTHOUSE] != BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, WATERWORKS_ALLIANCE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_WATERWORKS] == ALLIANCE && m_PointState[BG_BG_NODE_WATERWORKS] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, WATERWORKS_HORDE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_WATERWORKS] == HORDE && m_PointState[BG_BG_NODE_WATERWORKS] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, WATERWORKS_UNCONTROL, m_PointState[BG_BG_NODE_WATERWORKS] != BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, MINES_HORDE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_MINES] == HORDE && m_PointState[BG_BG_NODE_MINES] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, MINES_ALLIANCE_CONTROL, m_PointOwnedByTeam[BG_BG_NODE_MINES] == ALLIANCE && m_PointState[BG_BG_NODE_MINES] == BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, MINES_UNCONTROL, m_PointState[BG_BG_NODE_MINES] != BG_POINT_UNDER_CONTROL);
    FillInitialWorldState(data, count, 0xad2, 0x1);
    FillInitialWorldState(data, count, 0xad1, 0x1);
    FillInitialWorldState(data, count, 0xabe, GetTeamScore(HORDE));
    FillInitialWorldState(data, count, 0xabd, GetTeamScore(ALLIANCE));
    FillInitialWorldState(data, count, 0xa05, 0x8e);
    FillInitialWorldState(data, count, 0xaa0, 0x0);
    FillInitialWorldState(data, count, 0xa9f, 0x0);
    FillInitialWorldState(data, count, 0xa9e, 0x0);
    FillInitialWorldState(data, count, 0xc0d, 0x17b);
}

WorldSafeLocsEntry const* BattleGroundBG::GetClosestGraveYard(Player* player)
{
    uint32 g_id = 0;

    switch (player->GetTeam())
    {
        case ALLIANCE: g_id = BG_GRAVEYARD_MAIN_ALLIANCE; break;
        case HORDE:    g_id = BG_GRAVEYARD_MAIN_HORDE;    break;
        default:       return NULL;
    }

    float distance, nearestDistance;

    WorldSafeLocsEntry const* entry = NULL;
    WorldSafeLocsEntry const* nearestEntry = NULL;
    entry = sWorldSafeLocsStore.LookupEntry(g_id);
    nearestEntry = entry;

    if (!entry)
    {
        sLog.outError("BattleGroundBG: Not found the main team graveyard. Graveyard system isn't working!");
        return NULL;
    }

    float plr_x = player->GetPositionX();
    float plr_y = player->GetPositionY();
    float plr_z = player->GetPositionZ();


    distance = (entry->x - plr_x) * (entry->x - plr_x) + (entry->y - plr_y) * (entry->y - plr_y) + (entry->z - plr_z) * (entry->z - plr_z);
    nearestDistance = distance;

    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        if (m_PointOwnedByTeam[i] == player->GetTeam() && m_PointState[i] == BG_POINT_UNDER_CONTROL)
        {
            entry = sWorldSafeLocsStore.LookupEntry(BGCapturingPointTypes[i].GraveYardId);
            if (!entry)
                sLog.outError("BattleGroundBG: Not found graveyard: %u", BGCapturingPointTypes[i].GraveYardId);
            else
            {
                distance = (entry->x - plr_x) * (entry->x - plr_x) + (entry->y - plr_y) * (entry->y - plr_y) + (entry->z - plr_z) * (entry->z - plr_z);
                if (distance < nearestDistance)
                {
                    nearestDistance = distance;
                    nearestEntry = entry;
                }
            }
        }
    }

    return nearestEntry;
}

bool BattleGroundBG::IsAllNodesControlledByTeam(Team team) const
{
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
        if (m_PointState[i] != BG_POINT_UNDER_CONTROL || m_PointOwnedByTeam[i] != team)
            return false;

    return true;
}