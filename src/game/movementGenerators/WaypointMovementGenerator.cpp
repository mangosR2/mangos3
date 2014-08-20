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

#include <ctime>

#include "WaypointMovementGenerator.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "WaypointManager.h"
#include "WorldPacket.h"
#include "ScriptMgr.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

#include <cassert>

//-----------------------------------------------//
void WaypointMovementGenerator<Creature>::LoadPath(Creature& creature)
{
    DETAIL_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "LoadPath: loading waypoint path for %s", creature.GetGuidStr().c_str());

    m_path = sWaypointMgr.GetPath(creature.GetGUIDLow());

    // We may LoadPath() for several occasions:

    // 1: When creature.MovementType=2
    //    1a) Path is selected by creature.guid == creature_movement.id
    //    1b) Path for 1a) does not exist and then use path from creature.GetEntry() == creature_movement_template.entry

    // 2: When creature_template.MovementType=2
    //    2a) Creature is summoned and has creature_template.MovementType=2
    //        Creators need to be sure that creature_movement_template is always valid for summons.
    //        Mob that can be summoned anywhere should not have creature_movement_template for example.

    // No movement found for guid
    if (!m_path)
    {
        m_path = sWaypointMgr.GetPathTemplate(creature.GetEntry());

        // No movement found for entry
        if (!m_path)
        {
            sLog.outErrorDb("WaypointMovementGenerator::LoadPath: %s doesn't have waypoint path",
                creature.GetGuidStr().c_str());
            return;
        }
    }

    // Initialize the m_currentNode to point to the first node
    if (m_path->empty())
        return;

    m_currentNode = m_path->begin()->first;
    m_lastReachedWaypoint = 0;
}

void WaypointMovementGenerator<Creature>::Initialize(Creature& creature)
{
    creature.addUnitState(UNIT_STAT_ROAMING);
    creature.clearUnitState(UNIT_STAT_WAYPOINT_PAUSED);

    LoadPath(creature);

    StartMoveNow(creature);
}

void WaypointMovementGenerator<Creature>::Finalize(Creature& creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    creature.SetWalk(!creature.hasUnitState(UNIT_STAT_RUNNING_STATE), false);
}

void WaypointMovementGenerator<Creature>::Interrupt(Creature& creature)
{
    creature.InterruptMoving();
    creature.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    creature.SetWalk(!creature.hasUnitState(UNIT_STAT_RUNNING_STATE), false);
}

void WaypointMovementGenerator<Creature>::Reset(Creature& creature)
{
    creature.addUnitState(UNIT_STAT_ROAMING);
    StartMove(creature);
}

void WaypointMovementGenerator<Creature>::OnArrived(Creature& creature)
{
    if (!m_path || m_path->empty())
        return;

    m_lastReachedWaypoint = m_currentNode;

    if (m_isArrivalDone)
        return;

    m_isArrivalDone = true;

    WaypointPath::const_iterator currPoint = m_path->find(m_currentNode);
    MANGOS_ASSERT(currPoint != m_path->end());
    WaypointNode const& node = currPoint->second;

    if (node.script_id)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature movement start script %u at point %u for %s.", node.script_id, m_currentNode, creature.GetGuidStr().c_str());
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        creature.GetMap()->ScriptsStart(sCreatureMovementScripts, node.script_id, &creature, &creature);
    }

    // We have reached the destination and can process behavior
    if (WaypointBehavior* behavior = node.behavior)
    {
        if (behavior->emote != 0)
            creature.HandleEmote(behavior->emote);

        if (behavior->spell != 0)
            creature.CastSpell(&creature, behavior->spell, false);

        if (behavior->model1 != 0)
            creature.SetDisplayId(behavior->model1);

        if (behavior->textid[0])
        {
            int32 textId = behavior->textid[0];
            // Not only one text is set
            if (behavior->textid[1])
            {
                // Select one from max 5 texts (0 and 1 already checked)
                int i = 2;
                for (; i < MAX_WAYPOINT_TEXT; ++i)
                {
                    if (!behavior->textid[i])
                        break;
                }

                textId = behavior->textid[urand(0, i - 1)];
            }

            if (MangosStringLocale const* textData = sObjectMgr.GetMangosStringLocale(textId))
                creature.MonsterText(textData, NULL);
            else
                sLog.outErrorDb("%s reached waypoint %u, attempted to do text %i, but required text-data could not be found", creature.GetGuidStr().c_str(), m_currentNode, textId);
        }
    }

    // Inform script
    MovementInform(creature);
    Stop(node.delay);
}

void WaypointMovementGenerator<Creature>::StartMoveNow(Creature& creature)
{
    m_nextMoveTime.Reset(0);
    StartMove(creature);
}

void WaypointMovementGenerator<Creature>::StartMove(Creature& creature)
{
    if (!m_path || m_path->empty())
        return;

    if (Stopped(creature))
        return;

    if (!creature.isAlive() || creature.hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    WaypointPath::const_iterator currPoint = m_path->find(m_currentNode);
    MANGOS_ASSERT(currPoint != m_path->end());

    if (WaypointBehavior* behavior = currPoint->second.behavior)
    {
        if (behavior->model2 != 0)
            creature.SetDisplayId(behavior->model2);

        creature.SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
    }

    if (m_isArrivalDone)
    {
        ++currPoint;
        if (currPoint == m_path->end())
            currPoint = m_path->begin();

        m_currentNode = currPoint->first;
    }

    m_isArrivalDone = false;

    creature.addUnitState(UNIT_STAT_ROAMING_MOVE);

    WaypointNode const& nextNode = currPoint->second;
    Movement::MoveSplineInit<Unit*> init(creature);
    init.MoveTo(nextNode.x, nextNode.y, nextNode.z, true);

    if (nextNode.orientation != 100 && nextNode.delay != 0)
        init.SetFacing(nextNode.orientation);
    creature.SetWalk(!creature.hasUnitState(UNIT_STAT_RUNNING_STATE) && !creature.IsLevitating(), false);
    init.Launch();
}

bool WaypointMovementGenerator<Creature>::Update(Creature& creature, const uint32& diff)
{
    // Waypoint movement can be switched on/off
    // This is quite handy for escort quests and other stuff
    if (creature.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    // prevent a crash at empty waypoint path.
    if (!m_path || m_path->empty())
    {
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    if (Stopped(creature))
    {
        if (CanMove(diff, creature))
            StartMove(creature);
    }
    else
    {
        if (creature.IsStopped())
            Stop(STOP_TIME_FOR_PLAYER);
        else if (creature.movespline->Finalized())
        {
            OnArrived(creature);
            StartMove(creature);
        }
    }
    return true;
}

void WaypointMovementGenerator<Creature>::MovementInform(Creature& creature)
{
    if (creature.AI())
        creature.AI()->MovementInform(WAYPOINT_MOTION_TYPE, m_currentNode);
}

bool WaypointMovementGenerator<Creature>::GetResetPosition(Creature&, float& x, float& y, float& z) const
{
    // prevent a crash at empty waypoint path.
    if (!m_path || m_path->empty())
        return false;

    WaypointPath::const_iterator lastPoint = m_path->find(m_lastReachedWaypoint);
    // Special case: Before the first waypoint is reached, m_lastReachedWaypoint is set to 0 (which may not be contained in m_path)
    if (!m_lastReachedWaypoint && lastPoint == m_path->end())
        return false;

    MANGOS_ASSERT(lastPoint != m_path->end());

    x = lastPoint->second.x;
    y = lastPoint->second.y;
    z = lastPoint->second.z;
    return true;
}

bool WaypointMovementGenerator<Creature>::Stopped(Creature& u)
{
    return !m_nextMoveTime.Passed() || u.hasUnitState(UNIT_STAT_WAYPOINT_PAUSED);
}

bool WaypointMovementGenerator<Creature>::CanMove(int32 diff, Creature& u)
{
    m_nextMoveTime.Update(diff);
    if (m_nextMoveTime.Passed() && u.hasUnitState(UNIT_STAT_WAYPOINT_PAUSED))
        m_nextMoveTime.Reset(1);

    return m_nextMoveTime.Passed() && !u.hasUnitState(UNIT_STAT_WAYPOINT_PAUSED);
}

void WaypointMovementGenerator<Creature>::AddToWaypointPauseTime(int32 waitTimeDiff)
{
    if (!m_nextMoveTime.Passed())
    {
        // Prevent <= 0, the code in Update requires to catch the change from moving to not moving
        int32 newWaitTime = m_nextMoveTime.GetExpiry() + waitTimeDiff;
        m_nextMoveTime.Reset(newWaitTime > 0 ? newWaitTime : 1);
    }
}

//----------------------------------------------------//

uint32 FlightPathMovementGenerator::GetPathAtMapEnd() const
{
    if (m_currentNode >= m_path->size())
        return m_path->size();

    uint32 curMapId = (*m_path)[m_currentNode].mapid;

    for (uint32 i = m_currentNode; i < m_path->size(); ++i)
    {
        if ((*m_path)[i].mapid != curMapId)
            return i;
    }

    return m_path->size();
}

void FlightPathMovementGenerator::_Initialize(Player& player)
{
    _Reset(player);
}

void FlightPathMovementGenerator::_Finalize(Player& player)
{
    if (player.m_taxi.empty())
    {
        player.getHostileRefManager().setOnlineOfflineState(true);
        if (player.pvpInfo.inHostileArea)
            player.CastSpell(&player, SPELL_ID_HONORLESS_TARGET, true);

        // update z position to ground and orientation for landing point
        // this prevent cheating with landing  point at lags
        // when client side flight end early in comparison server side
        player.StopMoving(true);
    }

    player.RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_TAXI_BENCHMARK);
}

void FlightPathMovementGenerator::_Interrupt(Player& player)
{
}

#define PLAYER_FLIGHT_SPEED        32.0f

void FlightPathMovementGenerator::_Reset(Player& player)
{
    Movement::MoveSplineInit<Unit*> init(player);
    uint32 end = GetPathAtMapEnd();
    for (uint32 i = GetCurrentNode(); i != end; ++i)
    {
        G3D::Vector3 vertice((*m_path)[i].x, (*m_path)[i].y, (*m_path)[i].z);
        init.Path().push_back(vertice);
    }
    init.SetFirstPointId(GetCurrentNode());
    init.SetFly();
    init.SetVelocity(PLAYER_FLIGHT_SPEED);
    init.Launch();
}

bool FlightPathMovementGenerator::Update(Player& player, const uint32& diff)
{
    uint32 pointId = (uint32)player.movespline->currentPathIdx();
    if (pointId > m_currentNode)
    {
        bool departureEvent = true;
        do
        {
            DoEventIfAny(player, (*m_path)[m_currentNode], departureEvent);
            if (pointId == m_currentNode)
                break;

            m_currentNode += uint32(departureEvent);
            departureEvent = !departureEvent;
        }
        while (true);
    }

    return !(player.movespline->Finalized() || m_currentNode >= (m_path->size() - 1));
}

void FlightPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
    if (m_path->empty())
        return;

    uint32 map0 = (*m_path)[0].mapid;

    for (size_t i = 1; i < m_path->size(); ++i)
    {
        if ((*m_path)[i].mapid != map0)
        {
            m_currentNode = i;
            return;
        }
    }
}

void FlightPathMovementGenerator::DoEventIfAny(Player& player, TaxiPathNodeEntry const& node, bool departure)
{
    if (uint32 eventid = departure ? node.departureEventID : node.arrivalEventID)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Taxi %s event %u of node %u of path %u for player %s", departure ? "departure" : "arrival", eventid, node.index, node.path, player.GetName());
        StartEvents_Event(player.GetMap(), eventid, &player, &player, departure);
    }
}

bool FlightPathMovementGenerator::GetResetPosition(Player&, float& x, float& y, float& z) const
{
    TaxiPathNodeEntry const& node = (*m_path)[m_currentNode];
    x = node.x;
    y = node.y;
    z = node.z;
    return true;
}

//----------------------------------------------------//
uint32 TransportPathMovementGenerator::GetPathAtMapEnd() const
{
    if (m_currentNode >= m_path->size())
        return m_path->size();

    uint32 curMapId = (*m_path)[m_currentNode].mapid;

    for (uint32 i = m_currentNode; i < m_path->size(); ++i)
    {
        if ((*m_path)[i].mapid != curMapId)
            return i;
    }

    return m_path->size();
}

void TransportPathMovementGenerator::Initialize(GameObject& go)
{
}

void TransportPathMovementGenerator::Finalize(GameObject& go)
{
    //go.StopMoving();
}

void TransportPathMovementGenerator::Interrupt(GameObject& go)
{
}

void TransportPathMovementGenerator::Reset(GameObject& go)
{
    Movement::MoveSplineInit<GameObject*> init(go);
    uint32 end = GetPathAtMapEnd();
    for (uint32 i = GetCurrentNode(); i != end; ++i)
    {
        G3D::Vector3 vertice((*m_path)[i].x, (*m_path)[i].y, (*m_path)[i].z);
        init.Path().push_back(vertice);
    }
    init.SetFirstPointId(GetCurrentNode());
    init.SetFly();
    init.SetVelocity(PLAYER_FLIGHT_SPEED);
    init.Launch();
}

bool TransportPathMovementGenerator::Update(GameObject& go, uint32 const& diff)
{
    uint32 pointId = (uint32)go.movespline->currentPathIdx();
    if (pointId > m_currentNode)
    {
        bool departureEvent = true;
        do
        {
            DoEventIfAny(go, (*m_path)[m_currentNode], departureEvent);
            if (pointId == m_currentNode)
                break;
            m_currentNode += (uint32)departureEvent;
            departureEvent = !departureEvent;
        }
        while(true);
    }

    return !(go.movespline->Finalized() || m_currentNode >= (m_path->size() - 1));
}

void TransportPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
    if (m_path->empty())
        return;

    uint32 map0 = (*m_path)[0].mapid;

    for (size_t i = 1; i < m_path->size(); ++i)
    {
        if ((*m_path)[i].mapid != map0)
        {
            m_currentNode = i;
            return;
        }
    }
}

void TransportPathMovementGenerator::DoEventIfAny(GameObject& go, TaxiPathNodeEntry const& node, bool departure)
{
    if (uint32 eventid = departure ? node.departureEventID : node.arrivalEventID)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Transport %s event %u of node %u of path %u for GO %s", departure ? "departure" : "arrival", eventid, node.index, node.path, go.GetObjectGuid().GetString().c_str());
//        StartEvents_Event(player.GetMap(), eventid, &player, &go, departure);
    }
}

bool TransportPathMovementGenerator::GetResetPosition(GameObject& go, float& x, float& y, float& z) const
{
    TaxiPathNodeEntry const& node = (*m_path)[m_currentNode];
    x = node.x; y = node.y; z = node.z;
    return true;
}
