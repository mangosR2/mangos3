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

#ifndef __BATTLEGROUNDBG_H
#define __BATTLEGROUNDBG_H

#include "Language.h"

class BattleGround;

#define BG_BG_FLAG_RESPAWN_TIME         (10*IN_MILLISECONDS)//10 seconds
#define BG_BG_FPOINTS_TICK_TIME         (2*IN_MILLISECONDS) //2 seconds

enum BG_BG_WorldStates
{
    BG_ALLIANCE_RESOURCES           = 2749,
    BG_HORDE_RESOURCES              = 2750,
    BG_ALLIANCE_BASE                = 2752,
    BG_HORDE_BASE                   = 2753,
    LIGHTHOUSE_HORDE_CONTROL        = 2733,
    LIGHTHOUSE_ALLIANCE_CONTROL     = 2732,
    LIGHTHOUSE_UNCONTROL            = 2731,
    WATERWORKS_ALLIANCE_CONTROL     = 2730,
    WATERWORKS_HORDE_CONTROL        = 2729,
    WATERWORKS_UNCONTROL            = 2728,
    MINES_HORDE_CONTROL             = 2727,
    MINES_ALLIANCE_CONTROL          = 2726,
    MINES_UNCONTROL                 = 2725,
    BG_PROGRESS_BAR_PERCENT_GREY    = 2720,                 //100 = empty (only grey), 0 = blue|red (no grey)
    BG_PROGRESS_BAR_STATUS          = 2719,                 //50 init!, 48 ... hordak bere .. 33 .. 0 = full 100% hordacky , 100 = full alliance
    BG_PROGRESS_BAR_SHOW            = 2718,                 //1 init, 0 druhy send - bez messagu, 1 = controlled alliance
};

enum BG_BG_ProgressBarConsts
{
    BG_BG_POINT_MAX_CAPTURERS_COUNT     = 5,
    BG_BG_POINT_RADIUS                  = 70,
    BG_BG_PROGRESS_BAR_DONT_SHOW        = 0,
    BG_BG_BG_PROGRESS_BAR_SHOW             = 1,
    BG_BG_BG_PROGRESS_BAR_PERCENT_GREY     = 40,
    BG_BG_PROGRESS_BAR_STATE_MIDDLE     = 50,
    BG_BG_PROGRESS_BAR_HORDE_CONTROLLED = 0,
    BG_BG_PROGRESS_BAR_NEUTRAL_LOW      = 30,
    BG_BG_PROGRESS_BAR_NEUTRAL_HIGH     = 70,
    BG_BG_PROGRESS_BAR_ALI_CONTROLLED   = 100
};

enum BG_BG_Sounds
{
    // strange ids, but sure about them
};

enum BG_BG_Spells
{
};

enum BGBattleGroundPointsTrigger
{
    TR_LIGHTHOUSE_POINT        = 4516,
    TR_LIGHTHOUSE_BUFF         = 4518,
    TR_WATERWORKS_POINT        = 4570,
    TR_WATERWORKS_BUFF         = 4571,
    TR_MINES_POINT             = 4514,
    TR_MINES_BUFF              = 4569
};

enum BGBattleGroundGaveyards
{
    BG_GRAVEYARD_MAIN_ALLIANCE = 1103,
    BG_GRAVEYARD_MAIN_HORDE    = 1104,
    BG_GRAVEYARD_LIGHTHOUSE    = 1105,
    BG_GRAVEYARD_WATERWORKS    = 1107,
    BG_GRAVEYARD_MINES         = 1108
};

enum BG_BG_Nodes
{
    BG_BG_NODE_LIGHTHOUSE      = 0,
    BG_BG_NODE_WATERWORKS      = 1,
    BG_BG_NODE_MINES           = 2,

    // special internal node
    BG_BG_PLAYERS_OUT_OF_POINTS   = 3,                      // used for store out of node players data
};

#define BG_BG_NODES_MAX                 3
#define BG_BG_NODES_MAX_WITH_SPECIAL    4

// node-events work like this: event1:nodeid, event2:state (0alliance,1horde,2neutral)
#define BG_BGE_NEUTRAL_TEAM 2
// all other event2 are just nodeids, i won't define something here

// x, y, z
// used to check, when player is in range of a node
const float BG_BG_NodePositions[BG_BG_NODES_MAX][3] =
{
    {2024.600708f, 1742.819580f, 1195.157715f},             // BG_BG_NODE_LIGHTHOUSE
    {2301.010498f, 1386.931641f, 1197.183472f},             // BG_BG_NODE_WATERWORKS
    {2282.121582f, 1760.006958f, 1189.707153f}              // BG_BG_NODE_MINES
};

enum BGBattleGroundObjectTypes
{
    // buffs
    BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE           = 0,
    BG_BG_OBJECT_REGENBUFF_LIGHTHOUSE           = 1,
    BG_BG_OBJECT_BERSERKBUFF_LIGHTHOUSE         = 2,
    BG_BG_OBJECT_SPEEDBUFF_WATERWORKS           = 3,
    BG_BG_OBJECT_REGENBUFF_WATERWORKS           = 4,
    BG_BG_OBJECT_BERSERKBUFF_WATERWORKS         = 5,
    BG_BG_OBJECT_SPEEDBUFF_MINES                = 6,
    BG_BG_OBJECT_REGENBUFF_MINES                = 7,
    BG_BG_OBJECT_BERSERKBUFF_MINES              = 8,
    BG_BG_OBJECT_MAX                            = 9
};

#define BG_BG_NotBGWeekendHonorTicks    330
#define BG_BG_BGWeekendHonorTicks       200
#define BG_BG_EVENT_START_BATTLE        13180

enum BG_BG_Score
{
    BG_BG_WARNING_NEAR_VICTORY_SCORE    = 1800,
    BG_BG_MAX_TEAM_SCORE                = 2000
};

enum BGBattleGroundPointState
{
    BG_POINT_NO_OWNER           = 0,
    BG_POINT_STATE_UNCONTROLLED = 0,
    BG_POINT_UNDER_CONTROL      = 3
};

struct BattleGroundBGPointIconsStruct
{
    BattleGroundBGPointIconsStruct(uint32 _WorldStateControlIndex, uint32 _WorldStateAllianceControlledIndex, uint32 _WorldStateHordeControlledIndex)
        : WorldStateControlIndex(_WorldStateControlIndex), WorldStateAllianceControlledIndex(_WorldStateAllianceControlledIndex), WorldStateHordeControlledIndex(_WorldStateHordeControlledIndex) {}
    uint32 WorldStateControlIndex;
    uint32 WorldStateAllianceControlledIndex;
    uint32 WorldStateHordeControlledIndex;
};

struct BattleGroundBGLoosingPointStruct
{
    BattleGroundBGLoosingPointStruct(uint32 _MessageIdAlliance, uint32 _MessageIdHorde)
        : MessageIdAlliance(_MessageIdAlliance), MessageIdHorde(_MessageIdHorde)
    {}

    uint32 MessageIdAlliance;
    uint32 MessageIdHorde;
};

struct BattleGroundBGCapturingPointStruct
{
    BattleGroundBGCapturingPointStruct(uint32 _MessageIdAlliance, uint32 _MessageIdHorde, uint32 _GraveYardId)
        : MessageIdAlliance(_MessageIdAlliance), MessageIdHorde(_MessageIdHorde), GraveYardId(_GraveYardId)
    {}
    uint32 MessageIdAlliance;
    uint32 MessageIdHorde;
    uint32 GraveYardId;
};

const uint8  BG_BG_TickPoints[BG_BG_NODES_MAX] = {1, 2, 5};

// constant arrays:
const BattleGroundBGPointIconsStruct BGPointsIconStruct[BG_BG_NODES_MAX] =
{
    BattleGroundBGPointIconsStruct(LIGHTHOUSE_UNCONTROL, LIGHTHOUSE_ALLIANCE_CONTROL, LIGHTHOUSE_HORDE_CONTROL),
    BattleGroundBGPointIconsStruct(WATERWORKS_UNCONTROL, WATERWORKS_ALLIANCE_CONTROL, WATERWORKS_HORDE_CONTROL),
    BattleGroundBGPointIconsStruct(MINES_UNCONTROL, MINES_ALLIANCE_CONTROL, MINES_HORDE_CONTROL)
};
const BattleGroundBGLoosingPointStruct BGLoosingPointTypes[BG_BG_NODES_MAX] =
{
    BattleGroundBGLoosingPointStruct(LANG_BG_BG_HAS_LOST_A_LIGHTHOUSE, LANG_BG_BG_HAS_LOST_H_LIGHTHOUSE),
    BattleGroundBGLoosingPointStruct(LANG_BG_BG_HAS_LOST_A_WATERWORKS, LANG_BG_BG_HAS_LOST_H_WATERWORKS),
    BattleGroundBGLoosingPointStruct(LANG_BG_BG_HAS_LOST_A_MINES, LANG_BG_BG_HAS_LOST_H_MINES)
};
const BattleGroundBGCapturingPointStruct BGCapturingPointTypes[BG_BG_NODES_MAX] =
{
    
    BattleGroundBGCapturingPointStruct(LANG_BG_BG_HAS_TAKEN_A_LIGHTHOUSE, LANG_BG_BG_HAS_TAKEN_H_LIGHTHOUSE, BG_GRAVEYARD_LIGHTHOUSE),
    BattleGroundBGCapturingPointStruct(LANG_BG_BG_HAS_TAKEN_A_WATERWORKS, LANG_BG_BG_HAS_TAKEN_H_WATERWORKS, BG_GRAVEYARD_WATERWORKS),
    BattleGroundBGCapturingPointStruct(LANG_BG_BG_HAS_TAKEN_A_MINES, LANG_BG_BG_HAS_TAKEN_H_MINES, BG_GRAVEYARD_MINES)
};

class BattleGroundBGScore : public BattleGroundScore
{
    public:
        BattleGroundBGScore() {};
        virtual ~BattleGroundBGScore() {};
};

class BattleGroundBG : public BattleGround
{
        friend class BattleGroundMgr;

    public:
        BattleGroundBG();
        ~BattleGroundBG();
        void Update(uint32 diff) override;

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player* plr) override;
        virtual void StartingEventCloseDoors() override;
        virtual void StartingEventOpenDoors() override;

        void RemovePlayer(Player* plr, ObjectGuid guid) override;
        void HandleAreaTrigger(Player* source, uint32 trigger) override;
        void HandleKillPlayer(Player* player, Player* killer) override;
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player) override;
        virtual bool SetupBattleGround() override;
        virtual void Reset() override;
        void UpdateTeamScore(Team team);
        void EndBattleGround(Team winner) override;
        void UpdatePlayerScore(Player* source, uint32 type, uint32 value) override;
        void FillInitialWorldStates(WorldPacket& data, uint32& count);

        /* achievement req. */
        bool IsAllNodesControlledByTeam(Team team) const;

    private:
        void EventTeamCapturedPoint(Player* source, uint32 point);
        void EventTeamLostPoint(Player* source, uint32 point);
        void UpdatePointsCount(Team team);
        void UpdatePointsIcons(Team team, uint32 point);

        /* Point status updating procedures */
        void CheckSomeoneLeftPoint();
        void CheckSomeoneJoinedPoint();
        void UpdatePointStatuses();

        /* Scorekeeping */
        uint32 GetTeamScore(Team team) const { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoints(Team team, uint32 points);

        void RemovePoint(Team team, uint32 points = 1) { m_TeamScores[GetTeamIndexByTeamId(team)] -= points; }
        void SetTeamPoint(Team team, uint32 points = 0) { m_TeamScores[GetTeamIndexByTeamId(team)] = points; }

        uint32 m_HonorScoreTics[2];
        uint32 m_TeamPointsCount[PVP_TEAM_COUNT];

        uint32 m_Points_Trigger[BG_BG_NODES_MAX];

        int32 m_TowerCapCheckTimer;

        Team m_PointOwnedByTeam[BG_BG_NODES_MAX];
        uint8 m_PointState[BG_BG_NODES_MAX];
        int32 m_PointBarStatus[BG_BG_NODES_MAX];
        GuidVector m_PlayersNearPoint[BG_BG_NODES_MAX_WITH_SPECIAL];
        uint8 m_CurrentPointPlayersCount[2 * BG_BG_NODES_MAX];

        int32 m_PointAddingTimer;
        uint32 m_HonorTics;
};
#endif