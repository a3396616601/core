/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "Player.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "World.h"

#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::Initialize(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING);
    Movement::MoveSplineInit init(unit);
    init.MoveTo(i_x, i_y, i_z, m_generatePath);
    if (speed > 0.0f)
        init.SetVelocity(speed);

    init.Launch();
}

template<class T>
void PointMovementGenerator<T>::Interrupt(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING);
}

template<class T>
void PointMovementGenerator<T>::Reset(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING);
}

template<class T>
bool PointMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if (!_arrived && unit.IsWithinDist3d(i_x, i_y, i_z, 0.05f))
    {
        MovementInform(unit);
        return true;
    }

    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        if (!unit.IsStopped())
        {
            unit.DisableSpline();
            unit.StopMoving();
        }
        return true;
    }
    else if (unit.IsStopped() && !_arrived)
    {
        Initialize(unit);
        return true;
    }

    return !unit.movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>::Finalize(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING);
}

template<>
void PointMovementGenerator<Player>::MovementInform(Player&)
{
    _arrived = true;
}

template <>
void PointMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    _arrived = true;

    if (unit.AI())
        unit.AI()->MovementInform(POINT_MOTION_TYPE, id);

    if (unit.GetFormation() && unit.GetFormation()->getLeader() && unit.GetFormation()->getLeader()->GetGUID() != unit.GetGUID())
    {
        unit.GetFormation()->ReachedWaypoint();
        unit.SetOrientation(unit.GetFormation()->getLeader()->GetOrientation());
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
template bool PointMovementGenerator<Player>::Update(Player &, const uint32 &diff);
template bool PointMovementGenerator<Creature>::Update(Creature&, const uint32 &diff);

void AssistanceMovementGenerator::Finalize(Unit &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING);

    ((Creature*)&unit)->SetNoCallAssistance(false);
    ((Creature*)&unit)->CallAssistance();
    if (unit.isAlive())
        unit.GetMotionMaster()->MoveSeekAssistanceDistract(sWorld.getConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

bool EffectMovementGenerator::Update(Unit &unit, const uint32 &)
{
    return !unit.movespline->Finalized();
}

void EffectMovementGenerator::Finalize(Unit &unit)
{
    if (unit.GetTypeId() != TYPEID_UNIT)
        return;

    if (((Creature&)unit).AI() && unit.movespline->Finalized())
        ((Creature&)unit).AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);

    // Need restore previous movement since we have no proper states system
    if (unit.isAlive() && !unit.hasUnitState(UNIT_STAT_CONFUSED|UNIT_STAT_FLEEING))
    {
        if (Unit * victim = unit.getVictim())
            unit.GetMotionMaster()->MoveChase(victim);
        else
            unit.GetMotionMaster()->Initialize();
    }
}