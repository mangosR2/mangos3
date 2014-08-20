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

#ifndef MANGOS_POINTMOVEMENTGENERATOR_H
#define MANGOS_POINTMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"
#include "Creature.h"

template<class T>
class MANGOS_DLL_SPEC PointMovementGenerator: public MovementGeneratorMedium<T, PointMovementGenerator<T> >
{
    public:
        PointMovementGenerator(uint32 id, float x, float y, float z, bool generatePath) :
            m_id(id), m_x(x), m_y(y), m_z(z), m_generatePath(generatePath) {}

        void Initialize(T&);
        void Finalize(T&);
        void Interrupt(T&);
        void Reset(T&);
        bool Update(T&, uint32 const&);

        void MovementInform(T&);

        MovementGeneratorType GetMovementGeneratorType() const override { return POINT_MOTION_TYPE; }
        char const* Name() const { return "<Point>"; }

        bool GetDestination(float& x, float& y, float& z) const { x = m_x; y = m_y; z = m_z; return true; }

    private:
        uint32 m_id;
        float m_x, m_y, m_z;
        bool m_generatePath;
};

class MANGOS_DLL_SPEC AssistanceMovementGenerator : public PointMovementGenerator<Creature>
{
    public:
        AssistanceMovementGenerator(float x, float y, float z) :
            PointMovementGenerator<Creature>(0, x, y, z, true) {}

        void Finalize(Unit&) override;

        MovementGeneratorType GetMovementGeneratorType() const override { return ASSISTANCE_MOTION_TYPE; }
};

class MANGOS_DLL_SPEC FlyOrLandMovementGenerator : public PointMovementGenerator<Creature>
{
    public:
        FlyOrLandMovementGenerator(uint32 id, float x, float y, float z, bool liftOff) :
            PointMovementGenerator<Creature>(id, x, y, z, false),
            m_liftOff(liftOff) {}

        void Initialize(Unit& unit) override;

    private:
        bool m_liftOff;
};

// -----------------------------------------------------------------------------------------

// Used as base class for effect MGens.
class EffectMovementGenerator : public MovementGenerator
{
    public:
        explicit EffectMovementGenerator(uint32 id) : m_id(id) {}

        void Initialize(Unit&) override {}
        void Finalize(Unit&) override;
        void Interrupt(Unit&) override {}
        void Reset(Unit&) override {}
        bool Update(Unit&, const uint32&) override;
        MovementGeneratorType GetMovementGeneratorType() const override { return EFFECT_MOTION_TYPE; }
        char const* Name() const { return "<Effect>"; }

    protected:
        uint32 m_id;
};

class EjectMovementGenerator : public EffectMovementGenerator
{
    public:
        explicit EjectMovementGenerator(uint32 id) : EffectMovementGenerator(id) {}

        void Initialize(Unit& unit) override;
        void Finalize(Unit& unit) override;
};

class JumpMovementGenerator : public EffectMovementGenerator
{
    public:
        explicit JumpMovementGenerator(float x, float y, float z, float horizontalSpeed, float max_height, uint32 id) :
            EffectMovementGenerator(id),
            m_x(x), m_y(y), m_z(z), m_horizontalSpeed(horizontalSpeed), m_maxHeight(max_height) {}

        void Initialize(Unit&) override;

    private:
        float m_x, m_y, m_z;
        float m_horizontalSpeed;
        float m_maxHeight;
};

class MoveToDestMovementGenerator : public EffectMovementGenerator
{
    public:
        explicit MoveToDestMovementGenerator(float x, float y, float z, float o, Unit* target, float horizontalSpeed, float maxHeight, uint32 id, bool straightLine = false) :
            EffectMovementGenerator(id),
            m_x(x), m_y(y), m_z(z), m_o(o), m_target(target), m_horizontalSpeed(horizontalSpeed), m_maxHeight(maxHeight), m_straightLine(straightLine) {}

        void Initialize(Unit&) override;

    private:
        float m_x, m_y, m_z, m_o;
        Unit* m_target;
        float m_horizontalSpeed;
        float m_maxHeight;
        bool  m_straightLine;
};

class MoveWithSpeedMovementGenerator : public EffectMovementGenerator
{
    public:
        explicit MoveWithSpeedMovementGenerator(float x, float y, float z, float speed, bool generatePath, bool forceDestination) :
            EffectMovementGenerator(0),
            m_x(x), m_y(y), m_z(z), m_speed(speed), m_generatePath(generatePath), m_forceDestination(forceDestination) {}

        void Initialize(Unit&) override;

    private:
        float m_x, m_y, m_z;
        float m_speed;
        bool  m_generatePath;
        bool  m_forceDestination;
};

#endif
