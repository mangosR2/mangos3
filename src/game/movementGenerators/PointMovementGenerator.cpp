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

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "World.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

//----- Point Movement Generator

template<class T>
void PointMovementGenerator<T>::Initialize(T& unit)
{
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
        return;

    unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    Movement::MoveSplineInit<Unit*> init(unit);
    init.MoveTo(m_x, m_y, m_z, m_generatePath);
    init.Launch();
}

template<class T>
void PointMovementGenerator<T>::Finalize(T& unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    if (unit.movespline->Finalized())
        MovementInform(unit);
}

template<class T>
void PointMovementGenerator<T>::Interrupt(T& unit)
{
    unit.InterruptMoving();
    unit.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::Reset(T& unit)
{
    unit.StopMoving();
    unit.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
}

template<class T>
bool PointMovementGenerator<T>::Update(T& unit, uint32 const&)
{
    if (!&unit)
        return false;

    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        unit.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    if (!unit.hasUnitState(UNIT_STAT_ROAMING_MOVE) && unit.movespline->Finalized())
        Initialize(unit);

    return !unit.movespline->Finalized();
}

template<>
void PointMovementGenerator<Player>::MovementInform(Player&)
{
}

template <>
void PointMovementGenerator<Creature>::MovementInform(Creature& unit)
{
    if (unit.AI())
        unit.AI()->MovementInform(POINT_MOTION_TYPE, m_id);

    if (unit.IsTemporarySummon())
    {
        TemporarySummon* pSummon = (TemporarySummon*)(&unit);
        if (pSummon->GetSummonerGuid().IsCreatureOrVehicle())
        {
            if (Creature* pSummoner = unit.GetMap()->GetCreature(pSummon->GetSummonerGuid()))
            {
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, POINT_MOTION_TYPE, m_id);
            }
        }
    }
}

template void PointMovementGenerator<Player>::Initialize(Player&);
template void PointMovementGenerator<Creature>::Initialize(Creature&);
template void PointMovementGenerator<Player>::Finalize(Player&);
template void PointMovementGenerator<Creature>::Finalize(Creature&);
template void PointMovementGenerator<Player>::Interrupt(Player&);
template void PointMovementGenerator<Creature>::Interrupt(Creature&);
template void PointMovementGenerator<Player>::Reset(Player&);
template void PointMovementGenerator<Creature>::Reset(Creature&);
template bool PointMovementGenerator<Player>::Update(Player&, const uint32&);
template bool PointMovementGenerator<Creature>::Update(Creature&, const uint32&);

void AssistanceMovementGenerator::Finalize(Unit& unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    ((Creature*)&unit)->SetNoCallAssistance(false);
    ((Creature*)&unit)->CallAssistance();
    if (unit.isAlive())
        unit.GetMotionMaster()->MoveSeekAssistanceDistract(sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

void FlyOrLandMovementGenerator::Initialize(Unit& unit)
{
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
        return;

    unit.StopMoving();

    float x, y, z;
    GetDestination(x, y, z);
    unit.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    Movement::MoveSplineInit<Unit*> init(unit);
    init.SetFly();
    init.SetAnimation(m_liftOff ? Movement::FlyToGround : Movement::ToGround);
    init.MoveTo(x, y, z, false);
    init.Launch();
}

//----- Effect Movement Generator

void EffectMovementGenerator::Finalize(Unit& unit)
{
    if (unit.GetTypeId() != TYPEID_UNIT)
        return;

    if (((Creature&)unit).AI() && unit.movespline->Finalized())
        ((Creature&)unit).AI()->MovementInform(EFFECT_MOTION_TYPE, m_id);
}

bool EffectMovementGenerator::Update(Unit& unit, uint32 const&)
{
    return !unit.movespline->Finalized();
}

void EjectMovementGenerator::Initialize(Unit& unit)
{
    if (unit.GetTypeId() == TYPEID_PLAYER)
        ((Player&)unit).SetMover(&unit);
}

void EjectMovementGenerator::Finalize(Unit& unit)
{
    if (unit.GetTypeId() == TYPEID_UNIT && ((Creature&)unit).AI() && unit.movespline->Finalized())
        ((Creature&)unit).AI()->MovementInform(EFFECT_MOTION_TYPE, m_id);

    if (unit.hasUnitState(UNIT_STAT_ON_VEHICLE))
    {
        unit.clearUnitState(UNIT_STAT_ON_VEHICLE);
        unit.m_movementInfo.ClearTransportData();

        if (unit.GetTypeId() == TYPEID_PLAYER)
            ((Player&)unit).SetMover(&unit);
    }

    unit.m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
    unit.StopMoving(true);
}

void JumpMovementGenerator::Initialize(Unit& unit)
{
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
        return;

    unit.StopMoving();

    Movement::MoveSplineInit<Unit*> init(unit);
    init.MoveTo(m_x, m_y, m_z);
    init.SetParabolic(m_maxHeight, 0.0f);
    init.SetVelocity(m_horizontalSpeed);
    init.Launch();
}

void MoveToDestMovementGenerator::Initialize(Unit& unit)
{
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
        return;

    Movement::MoveSplineInit<Unit*> init(unit);
    init.MoveTo(m_x, m_y, m_z, bool(m_target), bool(m_target), m_straightLine);
    if (m_maxHeight > M_NULL_F)
        init.SetParabolic(m_maxHeight, 0.0f);
    init.SetVelocity(m_horizontalSpeed);
    m_target ? init.SetFacing(m_target) : init.SetFacing(m_o);
    init.Launch();
}

void MoveWithSpeedMovementGenerator::Initialize(Unit& unit)
{
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
        return;

    Movement::MoveSplineInit<Unit*> init(unit);
    init.MoveTo(m_x, m_y, m_z, m_generatePath, m_forceDestination);
    init.SetVelocity(m_speed);
    init.Launch();
}
