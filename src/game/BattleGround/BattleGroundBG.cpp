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
#include "GameObject.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "Util.h"
#include "WorldPacket.h"
#include "MapManager.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "DBCStores.h"                                      // TODO REMOVE this when graveyard handling for pvp is updated

BattleGroundBG::BattleGroundBG()
{
    m_BgObjects.resize(BG_BG_OBJECT_MAX);

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
        int team_points[PVP_TEAM_COUNT] = { 0, 0 };

        for (int node = 0; node < BG_BG_NODES_MAX; ++node)
        {
            // 3 sec delay to spawn new banner instead previous despawned one
            if (m_BannerTimers[node].timer)
            {
                if (m_BannerTimers[node].timer > diff)
                    m_BannerTimers[node].timer -= diff;
                else
                {
                    m_BannerTimers[node].timer = 0;
                    _CreateBanner(node, m_BannerTimers[node].type, m_BannerTimers[node].teamIndex, false);
                }
            }

            // 1-minute to occupy a node from contested state
            if (m_NodeTimers[node])
            {
                if (m_NodeTimers[node] > diff)
                    m_NodeTimers[node] -= diff;
                else
                {
                    m_NodeTimers[node] = 0;
                    // Change from contested to occupied !
                    uint8 teamIndex = m_Nodes[node]-1;
                    m_prevNodes[node] = m_Nodes[node];
                    m_Nodes[node] += 2;
                    // create new occupied banner
                    _CreateBanner(node, BG_BG_NODE_TYPE_OCCUPIED, teamIndex, true);
                    _SendNodeUpdate(node);
                    _NodeOccupied(node,(teamIndex == 0) ? ALLIANCE : HORDE);
                    // Message to chatlog

                    if (teamIndex == 0)
                    {
                        SendMessage2ToAll(LANG_BG_BG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE,NULL, LANG_BG_ALLY, _GetNodeNameId(node));
                        PlaySoundToAll(BG_BG_SOUND_NODE_CAPTURED_ALLIANCE);
                    }
                    else
                    {
                        SendMessage2ToAll(LANG_BG_BG_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_HORDE, _GetNodeNameId(node));
                        PlaySoundToAll(BG_BG_SOUND_NODE_CAPTURED_HORDE);
                    }
                }
            }

            for (int team = 0; team < PVP_TEAM_COUNT; ++team)
                if (m_Nodes[node] == team + BG_BG_NODE_TYPE_OCCUPIED)
                    ++team_points[team];
        }

        // Accumulate points
        for (int team = 0; team < PVP_TEAM_COUNT; ++team)
        {
            int points = team_points[team];
            if (!points)
                continue;
            m_lastTick[team] += diff;
            if (m_lastTick[team] > BG_BG_TickIntervals[points])
            {
                m_lastTick[team] -= BG_BG_TickIntervals[points];
                m_TeamScores[team] += BG_BG_TickPoints[points];
                m_honorScoreTicks[team] += BG_BG_TickPoints[points];
                m_ReputationScoreTics[team] += BG_BG_TickPoints[points];
                m_ExperiencesTicks[team] += BG_BG_TickPoints[points];
                if (m_ReputationScoreTics[team] >= m_ReputationTics)
                {
                    (team == TEAM_INDEX_ALLIANCE) ? RewardReputationToTeam(509, 10, ALLIANCE) : RewardReputationToTeam(510, 10, HORDE);
                    m_ReputationScoreTics[team] -= m_ReputationTics;
                }
                if (m_honorScoreTicks[team] >= m_honorTicks)
                {
                    RewardHonorToTeam(GetBonusHonorFromKill(1), (team == TEAM_INDEX_ALLIANCE) ? ALLIANCE : HORDE);
                    m_honorScoreTicks[team] -= m_honorTicks;
                }
                if (!m_IsInformedNearVictory && m_TeamScores[team] > BG_BG_WARNING_NEAR_VICTORY_SCORE)
                {
                    if (team == TEAM_INDEX_ALLIANCE)
                        SendMessageToAll(LANG_BG_AB_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    else
                        SendMessageToAll(LANG_BG_AB_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    PlaySoundToAll(BG_BG_SOUND_NEAR_VICTORY);
                    m_IsInformedNearVictory = true;
                }
                if (m_ExperiencesTicks[team] >= BG_BG_ExperiencesTicks)
                {
                    RewardXpToTeam(0, 0.8f, (team == TEAM_INDEX_ALLIANCE) ? ALLIANCE : HORDE);
                    m_ExperiencesTicks[team] -= BG_BG_ExperiencesTicks;
                }

                if (m_TeamScores[team] > BG_BG_MAX_TEAM_SCORE)
                    m_TeamScores[team] = BG_BG_MAX_TEAM_SCORE;
                if (team == TEAM_INDEX_ALLIANCE)
                    UpdateWorldState(BG_BG_OP_RESOURCES_ALLY, m_TeamScores[team]);
                if (team == TEAM_INDEX_HORDE)
                    UpdateWorldState(BG_BG_OP_RESOURCES_HORDE, m_TeamScores[team]);

                // update achievement flags
                // we increased m_TeamScores[team] so we just need to check if it is 500 more than other teams resources
                // horde will be a bit disadvantaged, but we can assume that points aren't updated for both team in same Update() call
                uint8 otherTeam = (team + 1) % PVP_TEAM_COUNT;
                if (m_TeamScores[team] > m_TeamScores[otherTeam] + 500)
                    m_TeamScores500Disadvantage[otherTeam] = true;
            }
        }

        // Test win condition
        if (m_TeamScores[TEAM_INDEX_ALLIANCE] >= BG_BG_MAX_TEAM_SCORE)
            EndBattleGround(ALLIANCE);
        if (m_TeamScores[TEAM_INDEX_HORDE] >= BG_BG_MAX_TEAM_SCORE)
            EndBattleGround(HORDE);
    }
}

void BattleGroundBG::StartingEventCloseDoors()
{
    // despawn buffs
    for (int i = 0; i < BG_BG_NODES_MAX * 3; ++i)
        SpawnBGObject(m_BgObjects[BG_BG_OBJECT_SPEEDBUFF_1 + i], RESPAWN_ONE_DAY);
}

void BattleGroundBG::StartingEventOpenDoors()
{
    for (int i = 0; i < BG_BG_BUFFS_MAX; ++i)
    {
        //randomly select buff to spawn
        uint8 buff = urand(0, 2);
        SpawnBGObject(m_BgObjects[BG_BG_OBJECT_SPEEDBUFF_1 + buff + i * 3], RESPAWN_IMMEDIATELY);
    }
    OpenDoorEvent(BG_EVENT_DOOR);

    // Players that join battleground after start are not eligible to get achievement.
    StartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, BG_EVENT_START_BATTLE);
}

void BattleGroundBG::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in the constructor
    BattleGroundBGScore* sc = new BattleGroundBGScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattleGroundBG::RemovePlayer(Player * /*plr*/, ObjectGuid /*guid*/)
{

}

void BattleGroundBG::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    switch (Trigger)
    {
        case 6265:                                          // Waterworks heal
        case 6266:                                          // Mine speed
        case 6267:                                          // Waterworks speed
        case 6268:                                          // Mine berserk
        case 6269:                                          // Lighthouse berserk
        case 6447:                                          // Alliance start
        case 6448:                                          // Horde start
            //break;
        default:
            //ERROR_LOG("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            //Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

/*  type: 0-neutral, 1-contested, 3-occupied
    teamIndex: 0-ally, 1-horde                        */
void BattleGroundBG::_CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
{
    // Just put it into the queue
    if (delay)
    {
        m_BannerTimers[node].timer = 2000;
        m_BannerTimers[node].type = type;
        m_BannerTimers[node].teamIndex = teamIndex;
        return;
    }

    // cause the node-type is in the generic form
    // please see in the headerfile for the ids
    if (type != BG_BG_NODE_TYPE_NEUTRAL)
        type += teamIndex;

    SpawnEvent(node, type, true);                           // will automaticly despawn other events
}

int32 BattleGroundBG::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_BG_NODE_LIGHTHOUSE: return LANG_BG_BG_NODE_LIGHTHOUSE;
        case BG_BG_NODE_WATERWORKS: return LANG_BG_BG_NODE_WATERWORKS;
        case BG_BG_NODE_MINE:       return LANG_BG_BG_NODE_MINE;
        default:
            MANGOS_ASSERT(0);
    }
    return 0;
}

void BattleGroundBG::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    // Node icons
    for (uint8 node = 0; node < BG_BG_NODES_MAX; ++node)
        FillInitialWorldState(BG_BG_OP_NODEICONS[node], m_Nodes[node] == 0);

    // Node occupied states
    for (uint8 node = 0; node < BG_BG_NODES_MAX; ++node)
        for (uint8 i = BG_BG_NODE_STATUS_ALLY_CONTESTED; i <= BG_BG_NODE_STATUS_HORDE_OCCUPIED; ++i)
            FillInitialWorldState(BG_BG_OP_NODESTATES[node] + plusArray[i], m_Nodes[node] == i);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 node = 0; node < BG_BG_NODES_MAX; ++node)
        if (m_Nodes[node] == BG_BG_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[node] == BG_BG_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    FillInitialWorldState(BG_BG_OP_OCCUPIED_BASES_ALLY, ally);
    FillInitialWorldState(BG_BG_OP_OCCUPIED_BASES_HORDE, horde);

    // Team scores
    FillInitialWorldState(BG_BG_OP_RESOURCES_MAX,      BG_BG_MAX_TEAM_SCORE);
    FillInitialWorldState(BG_BG_OP_RESOURCES_WARNING,  BG_BG_WARNING_NEAR_VICTORY_SCORE);
    FillInitialWorldState(BG_BG_OP_RESOURCES_ALLY,     m_TeamScores[TEAM_INDEX_ALLIANCE]);
    FillInitialWorldState(BG_BG_OP_RESOURCES_HORDE,    m_TeamScores[TEAM_INDEX_HORDE]);

    // other unknown
    //FillInitialWorldState(0x745, 0x2);         // 37 1861 unk
}

void BattleGroundBG::_SendNodeUpdate(uint8 node)
{
    // Send node owner state update to refresh map icons on client
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    if (m_prevNodes[node])
        UpdateWorldState(BG_BG_OP_NODESTATES[node] + plusArray[m_prevNodes[node]], WORLD_STATE_REMOVE);
    else
        UpdateWorldState(BG_BG_OP_NODEICONS[node], 0);

    UpdateWorldState(BG_BG_OP_NODESTATES[node] + plusArray[m_Nodes[node]], WORLD_STATE_ADD);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
        if (m_Nodes[i] == BG_BG_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[i] == BG_BG_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    UpdateWorldState(BG_BG_OP_OCCUPIED_BASES_ALLY, ally);
    UpdateWorldState(BG_BG_OP_OCCUPIED_BASES_HORDE, horde);
}

void BattleGroundBG::_NodeOccupied(uint8 node,Team team)
{
}

/* Invoked if a player used a banner as a gameobject */
void BattleGroundBG::EventPlayerClickedOnFlag(Player *source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 event = (sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetGUIDLow())).event1;
    if (event >= BG_BG_NODES_MAX)                           // not a node
        return;
    BG_BG_Nodes node = BG_BG_Nodes(event);

    TeamIndex teamIndex = GetTeamIndex(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(m_Nodes[node] == 0 || teamIndex == m_Nodes[node] % 2))
        return;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    uint32 sound = 0;

    // TODO in the following code we should restructure a bit to avoid
    // duplication (or maybe write functions?)
    // If node is neutral, change to contested
    if (m_Nodes[node] == BG_BG_NODE_TYPE_NEUTRAL)
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + 1;
        // create new contested banner
        _CreateBanner(node, BG_BG_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        m_NodeTimers[node] = BG_BG_FLAG_CAPTURING_TIME;

        if (teamIndex == 0)
            SendMessage2ToAll(LANG_BG_BG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node), LANG_BG_ALLY);
        else
            SendMessage2ToAll(LANG_BG_BG_NODE_CLAIMED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node), LANG_BG_HORDE);

        sound = BG_BG_SOUND_NODE_CLAIMED;
    }
    // If node is contested
    else if ((m_Nodes[node] == BG_BG_NODE_STATUS_ALLY_CONTESTED) || (m_Nodes[node] == BG_BG_NODE_STATUS_HORDE_CONTESTED))
    {
        // If last state is NOT occupied, change node to enemy-contested
        if (m_prevNodes[node] < BG_BG_NODE_TYPE_OCCUPIED)
        {
            UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_BG_NODE_TYPE_CONTESTED;
            // create new contested banner
            _CreateBanner(node, BG_BG_NODE_TYPE_CONTESTED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = BG_BG_FLAG_CAPTURING_TIME;

            if (teamIndex == TEAM_INDEX_ALLIANCE)
                SendMessage2ToAll(LANG_BG_BG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_BG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        // If contested, change back to occupied
        else
        {
            UpdatePlayerScore(source, SCORE_BASES_DEFENDED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_BG_NODE_TYPE_OCCUPIED;
            // create new occupied banner
            _CreateBanner(node, BG_BG_NODE_TYPE_OCCUPIED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = 0;
            _NodeOccupied(node, (teamIndex == TEAM_INDEX_ALLIANCE) ? ALLIANCE:HORDE);

            if (teamIndex == TEAM_INDEX_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED,CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED,CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        sound = (teamIndex == TEAM_INDEX_ALLIANCE) ? BG_BG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_BG_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied, change to enemy-contested
    else
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + BG_BG_NODE_TYPE_CONTESTED;
        // create new contested banner
        _CreateBanner(node, BG_BG_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        m_NodeTimers[node] = BG_BG_FLAG_CAPTURING_TIME;

        if (teamIndex == TEAM_INDEX_ALLIANCE)
            SendMessage2ToAll(LANG_BG_BG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_BG_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));

        sound = (teamIndex == TEAM_INDEX_ALLIANCE) ? BG_BG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_BG_SOUND_NODE_ASSAULTED_HORDE;
    }

    // If node is occupied again, send "X has taken the Y" msg.
    if (m_Nodes[node] >= BG_BG_NODE_TYPE_OCCUPIED)
    {
        if (teamIndex == TEAM_INDEX_ALLIANCE)
            SendMessage2ToAll(LANG_BG_BG_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_ALLY, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_BG_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_HORDE, _GetNodeNameId(node));
    }
    PlaySoundToAll(sound);
}

bool BattleGroundBG::SetupBattleGround()
{
    // buffs
    for (int i = 0; i < BG_BG_BUFFS_MAX; ++i)
    {
        // ToDo: Uncomment when Quaternions are implemented
//        if (!AddObject(BG_BG_OBJECT_SPEEDBUFF_1 + 3 * i, Buff_Entries[0], BG_BG_BuffPositions[i][0], BG_BG_BuffPositions[i][1], BG_BG_BuffPositions[i][2], BG_BG_BuffPositions[i][3], QuaternionData(0, 0, sin(BG_BG_BuffPositions[i][3]/2), cos(BG_BG_BuffPositions[i][3]/2)), RESPAWN_ONE_DAY)
//            || !AddObject(BG_BG_OBJECT_SPEEDBUFF_1 + 3 * i + 1, Buff_Entries[1], BG_BG_BuffPositions[i][0], BG_BG_BuffPositions[i][1], BG_BG_BuffPositions[i][2], BG_BG_BuffPositions[i][3], QuaternionData(0, 0, sin(BG_BG_BuffPositions[i][3]/2), cos(BG_BG_BuffPositions[i][3]/2)), RESPAWN_ONE_DAY)
//            || !AddObject(BG_BG_OBJECT_SPEEDBUFF_1 + 3 * i + 2, Buff_Entries[2], BG_BG_BuffPositions[i][0], BG_BG_BuffPositions[i][1], BG_BG_BuffPositions[i][2], BG_BG_BuffPositions[i][3], QuaternionData(0, 0, sin(BG_BG_BuffPositions[i][3]/2), cos(BG_BG_BuffPositions[i][3]/2)), RESPAWN_ONE_DAY)
//            )
//            sLog.outErrorDb("BatteGroundAB: Failed to spawn buff object!");
    }

    return true;
}

void BattleGroundBG::Reset()
{
    // call parent's class reset
    BattleGround::Reset();

    for (uint8 i = 0; i < PVP_TEAM_COUNT; ++i)
    {
        m_TeamScores[i] = 0;
        m_lastTick[i] = 0;
        m_honorScoreTicks[i] = 0;
        m_ReputationScoreTics[i] = 0;
        m_ExperiencesTicks[i]    = 0;
        m_TeamScores500Disadvantage[i] = false;
    }

    m_IsInformedNearVictory = false;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID(true));
    m_honorTicks = isBGWeekend ? BG_WEEKEND_HONOR_INTERVAL : BG_NORMAL_HONOR_INTERVAL;
    m_ReputationTics = isBGWeekend ? BG_WEEKEND_REPUTATION_INTERVAL : BG_NORMAL_REPUTATION_INTERVAL;

    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
    {
        m_Nodes[i] = 0;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;
        m_BannerTimers[i].timer = 0;

        // all nodes owned by neutral team at beginning
        m_ActiveEvents[i] = BG_BG_NODE_TYPE_NEUTRAL;
    }

}

void BattleGroundBG::EndBattleGround(Team winner)
{
    // win reward
    if (winner == ALLIANCE)
    {
        RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
        RewardXpToTeam(0, 0.8f, winner);
    }
    if (winner == HORDE)
    {
        RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
        RewardXpToTeam(0, 0.8f, winner);
    }
    // complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);

    RewardXpToTeam(0, 0.8f, HORDE);
    RewardXpToTeam(0, 0.8f, ALLIANCE);

    BattleGround::EndBattleGround(winner);
}

WorldSafeLocsEntry const* BattleGroundBG::GetClosestGraveYard(Player* player)
{
    TeamIndex teamIndex = GetTeamIndex(player->GetTeam());

    // Is there any occupied node for this team?
    std::vector<uint8> nodes;
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
        if (m_Nodes[i] == teamIndex + 3)
            nodes.push_back(i);

    WorldSafeLocsEntry const* good_entry = NULL;
    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float plr_x = player->GetPositionX();
        float plr_y = player->GetPositionY();

        float mindist = 999999.0f;
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const*entry = sWorldSafeLocsStore.LookupEntry(BG_BG_GraveyardIds[nodes[i]]);
            if (!entry)
                continue;
            float dist = (entry->x - plr_x) * (entry->x - plr_x) + (entry->y - plr_y) * (entry->y - plr_y);
            if (mindist > dist)
            {
                mindist = dist;
                good_entry = entry;
            }
        }
        nodes.clear();
    }
    // If not, place ghost on starting location
    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry(BG_BG_GraveyardIds[teamIndex + 3]);

    return good_entry;
}

void BattleGroundBG::UpdatePlayerScore(Player *Source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetObjectGuid());
    if (itr == m_PlayerScores.end())                            // player not found...
        return;

    uint32 achCriId = 0;
    switch(type)
    {
        case SCORE_BASES_ASSAULTED:
            ((BattleGroundBGScore*)itr->second)->BasesAssaulted += value;
            achCriId = BG_OBJECTIVE_ASSAULT_BASE;
            break;
        case SCORE_BASES_DEFENDED:
            ((BattleGroundBGScore*)itr->second)->BasesDefended += value;
            achCriId = BG_OBJECTIVE_DEFEND_BASE;
            break;
        default:
            BattleGround::UpdatePlayerScore(Source,type,value);
            break;
    }

    if (achCriId)
        Source->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, 1, achCriId);
}

bool BattleGroundBG::IsAllNodesControlledByTeam(Team team) const
{
    for (uint8 i = 0; i < BG_BG_NODES_MAX; ++i)
        if ((team == ALLIANCE && m_Nodes[i] != BG_BG_NODE_STATUS_ALLY_OCCUPIED) ||
                (team == HORDE && m_Nodes[i] != BG_BG_NODE_STATUS_HORDE_OCCUPIED))
            return false;

    return true;
}

uint32 BattleGroundBG::GetPlayerScore(Player *Source, uint32 type)
{
    BattleGroundScoreMap::const_iterator itr = m_PlayerScores.find(Source->GetObjectGuid());

    if (itr == m_PlayerScores.end())                         // player not found...
        return 0;

    switch(type)
    {
        case SCORE_BASES_ASSAULTED:
            return ((BattleGroundBGScore*)itr->second)->BasesAssaulted;
        case SCORE_BASES_DEFENDED:
            return ((BattleGroundBGScore*)itr->second)->BasesDefended;
    }

    return BattleGround::GetPlayerScore(Source, type);
}

void BattleGroundBG::CheckBuggers()
{
    if (GetStatus() == STATUS_WAIT_JOIN)
    {
        for (BattleGroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
        {
            if (Player* plr = sObjectMgr.GetPlayer(itr->first))
            {
                if (plr->isGameMaster() || plr->IsBeingTeleported())
                    continue;

                // alliance buggers
                if (plr->GetPositionX() < 1164.0f)
                {
                    if (plr->GetPositionX() > 922.0f || plr->GetPositionY() < 1316.0f)
                        plr->TeleportTo(GetMapId(), 905.12f, 1338.88f, 27.468f, plr->GetOrientation());
                }
                // horde buggers
                else
                {
                    if (plr->GetPositionX() < 1393.0f || plr->GetPositionY() < 948.0f || plr->GetPositionY() > 1032.0f)
                        plr->TeleportTo(GetMapId(), 1404.306f, 976.73f, 7.44f, plr->GetOrientation());
                }
            }
        }
    }
}

