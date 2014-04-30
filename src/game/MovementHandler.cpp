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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "Vehicle.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "MapManager.h"
#include "Transports.h"
#include "BattleGround/BattleGround.h"
#include "movementGenerators/WaypointMovementGenerator.h"
#include "MapPersistentStateMgr.h"
#include "ObjectMgr.h"

void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket& /*recv_data*/)
{
    DEBUG_LOG("WORLD: got MSG_MOVE_WORLDPORT_ACK.");
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if (!GetPlayer()->IsBeingTeleportedFar())
        return;

    if (_player->GetVehicleKit())
        _player->GetVehicleKit()->RemoveAllPassengers();

    // get start teleport coordinates (will used later in fail case)
    WorldLocation const& old_loc = GetPlayer()->GetPosition();

    // get the teleport destination
    WorldLocation& loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check (only cheating case possible)
    if (!MapManager::IsValidMapCoord(loc))
    {
        sLog.outError("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far to a not valid location "
            "(map:%u, x:%f, y:%f, z:%f) We port him to his homebind instead..",
            GetPlayer()->GetGuidStr().c_str(), loc.GetMapId(), loc.x, loc.y, loc.z);
        // stop teleportation else we would try this again and again in LogoutPlayer...
        GetPlayer()->SetSemaphoreTeleportFar(false);
        // and teleport the player to a valid place
        GetPlayer()->TeleportToHomebind();
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.GetMapId());

    Map* map = NULL;

    // prevent crash at attempt landing to not existed battleground instance
    if (mEntry->IsBattleGroundOrArena())
    {
        if (GetPlayer()->GetBattleGroundId())
            map = sMapMgr.FindMap(loc.GetMapId(), GetPlayer()->GetBattleGroundId());

        if (!map)
        {
            DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far to nonexisten battleground instance "
                " (map:%u, x:%f, y:%f, z:%f) Trying to port him to his previous place..",
                GetPlayer()->GetGuidStr().c_str(), loc.GetMapId(), loc.x, loc.y, loc.z);

            GetPlayer()->SetSemaphoreTeleportFar(false);

            // Teleport to previous place, if cannot be ported back TP to homebind place
            if (!GetPlayer()->TeleportTo(old_loc, TELE_TO_NODELAY))
            {
                DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s cannot be ported to his previous place, teleporting him to his homebind place...",
                    GetPlayer()->GetGuidStr().c_str());
                GetPlayer()->TeleportToHomebind();
            }
            return;
        }
    }

    InstanceTemplate const* mInstance = ObjectMgr::GetInstanceTemplate(loc.GetMapId());

    // reset instance validity, except if going to an instance inside an instance
    if (GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;

    GetPlayer()->SetSemaphoreTeleportFar(false);

    // relocate the player to the teleport destination
    if (!map)
        map = sMapMgr.CreateMap(loc.GetMapId(), GetPlayer());

    if (!map)
    {
        DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: cannot create requested map %u for teleport player %s!",loc.GetMapId(), GetPlayer()->GetGuidStr().c_str());
        GetPlayer()->SetSemaphoreTeleportFar(false);
        GetPlayer()->TeleportToHomebind();
        return;
    }

    GetPlayer()->SetMap(map);
    GetPlayer()->Relocate(loc);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if (!map->Add(GetPlayer()))
    {
        // if player wasn't added to map, reset his map pointer!
        GetPlayer()->ResetMap();

        DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s was teleported far but couldn't be added to map "
            " (map:%u, x:%f, y:%f, z:%f) Trying to port him to his previous place..",
            GetPlayer()->GetGuidStr().c_str(), loc.GetMapId(), loc.x, loc.y, loc.z);

        // Teleport to previous place, if cannot be ported back TP to homebind place
        if (!GetPlayer()->TeleportTo(old_loc, TELE_TO_NODELAY))
        {
            DETAIL_LOG("WorldSession::HandleMoveWorldportAckOpcode: %s cannot be ported to his previous place, teleporting him to his homebind place...",
                GetPlayer()->GetGuidStr().c_str());
            GetPlayer()->TeleportToHomebind();
        }
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if (_player->InBattleGround())
    {
        // cleanup setting if outdated
        if (!mEntry->IsBattleGroundOrArena())
        {
            // We're not in BG
            _player->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);
            // reset destination bg team
            _player->SetBGTeam(TEAM_NONE);
        }
        // join to bg case
        else if (BattleGround* bg = _player->GetBattleGround())
        {
            if (_player->IsInvitedForBattleGroundInstance(_player->GetBattleGroundId()))
                bg->AddPlayer(_player);
        }
    }

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // flight fast teleport case
    if (GetPlayer()->IsInUnitState(UNIT_ACTION_TAXI))
    {
        if (!_player->InBattleGround())
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->CurrentMovementGenerator());
            flight->Reset(*GetPlayer());
            return;
        }

        // battleground state prepare, stop flight
        GetPlayer()->GetUnitStateMgr().InitDefaults();
        GetPlayer()->m_taxi.ClearTaxiDestinations();
    }

    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if (MapDifficultyEntry const* mapDiff = GetMapDifficultyData(mEntry->MapID, diff))
        {
            if (mapDiff->resetTime)
            {
                if (time_t timeReset = sMapPersistentStateMgr.GetScheduler().GetResetTimeFor(mEntry->MapID,diff))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->MapID, diff, timeleft);
                }
            }
        }
    }

    // mount allow check
    if (!mEntry->IsMountAllowed())
    {
        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        _player->RemoveSpellsCausingAura(SPELL_AURA_FLY);
    }
    else
    {
        // recheck mount capabilities at far teleport
        Unit::AuraList const& mMountAuras = _player->GetAurasByType(SPELL_AURA_MOUNTED);
        for (Unit::AuraList::const_iterator itr = mMountAuras.begin(); itr != mMountAuras.end(); )
        {
            AuraPair aura = *itr;

            // mount is no longer suitable
            MountCapabilityEntry const* entry = _player->GetMountCapability(aura->GetSpellEffect()->EffectMiscValueB);
            if (!entry)
            {
                _player->RemoveAurasDueToSpell(aura->GetId());
                itr = mMountAuras.begin();
                continue;
            }

            // mount capability changed
            if (entry->Id != aura->GetModifier()->m_amount)
            {
                if (MountCapabilityEntry const* oldEntry = sMountCapabilityStore.LookupEntry(aura->GetModifier()->m_amount))
                    _player->RemoveAurasDueToSpell(oldEntry->SpeedModSpell);

                _player->CastSpell(_player, entry->SpeedModSpell, true);

                aura->ChangeAmount(entry->Id);
            }

            ++itr;
        }

        uint32 zone, area;
        _player->GetZoneAndAreaId(zone, area);
        // recheck fly auras
        Unit::AuraList const& mFlyAuras = _player->GetAurasByType(SPELL_AURA_FLY);
        for (Unit::AuraList::const_iterator itr = mFlyAuras.begin(); itr != mFlyAuras.end(); )
        {
            AuraPair aura = *itr;
            if (!_player->CanStartFlyInArea(_player->GetMapId(), zone, area))
            {
                _player->RemoveAurasDueToSpell(aura->GetId());
                itr = mFlyAuras.begin();
                continue;
            }

            ++itr;
        }
    }

    // honorless target
    if (GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), SPELL_ID_HONORLESS_TARGET, true);

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    // lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();

    // Set last WS update time to 0 - grant sending ALL WS updates from new map.
    GetPlayer()->SetLastWorldStateUpdateTime(time_t(0));
}

void WorldSession::HandleMoveTeleportAckOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_TELEPORT_ACK");

    ObjectGuid guid;
    uint32 counter, time;
    recv_data >> counter >> time;

    recv_data.ReadGuidMask<5, 0, 1, 6, 3, 7, 2, 4>(guid);
    recv_data.ReadGuidBytes<4, 2, 7, 6, 5, 1, 3, 0>(guid);

    DEBUG_LOG("Guid: %s", guid.GetString().c_str());
    DEBUG_LOG("Counter %u, time %u", counter, time/IN_MILLISECONDS);

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    if (!plMover || !plMover->IsBeingTeleportedNear())
        return;

    if (guid != plMover->GetObjectGuid())
        return;

    plMover->SetSemaphoreTeleportNear(false);

    uint32 old_zone = plMover->GetZoneId();

    WorldLocation const& dest = plMover->GetTeleportDest();

    plMover->SetPosition(dest, true);

    uint32 newzone, newarea;
    plMover->GetZoneAndAreaId(newzone, newarea);
    plMover->UpdateZone(newzone, newarea);

    // new zone
    if (old_zone != newzone)
    {
        // honorless target
        if (plMover->pvpInfo.inHostileArea)
            plMover->CastSpell(plMover, SPELL_ID_HONORLESS_TARGET, true);
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    // lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMovementOpcodes(WorldPacket & recv_data)
{
    Opcodes opcode = recv_data.GetOpcode();

    DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_MOVES,"WorldSession::HandleMovementOpcodes: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);
    //recv_data.hexlike();

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    /* extract packet */
    MovementInfo movementInfo;
    recv_data >> movementInfo;

    /*----------------*/

    if (!VerifyMovementInfo(movementInfo, movementInfo.GetGuid()))
        return;

    if (mover && _player->GetObjectGuid() != mover->GetObjectGuid())
    {
        if (opcode == CMSG_MOVE_SET_WALK_MODE)
            movementInfo.RemoveMovementFlag(MOVEFLAG_WALK_MODE);
    }

    if (!movementInfo.GetTransportGuid().IsEmpty())
    {
        //if (movementInfo.GetPos()->x != movementInfo.GetTransportPos()->x)
        //    if (GetPlayer()->GetTransport())
        //        GetPlayer()->GetTransport()->UpdateMovementInfo(&movementInfo);
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == CMSG_MOVE_FALL_LAND && plMover && !plMover->IsTaxiFlying())
        plMover->HandleFall(movementInfo);

    /* process anticheat check */
    GetPlayer()->GetAntiCheat()->DoAntiCheatCheck(CHECK_MOVEMENT,movementInfo, opcode);

    /* process position-change */

    if (plMover)
    {
        HandlePlayerRelocation(movementInfo);

        plMover->UpdateFallInformationIfNeed(movementInfo, opcode);

        // sometimes there is a desync and it should be done here
        if (plMover && opcode == MSG_MOVE_HEARTBEAT && plMover->isMoving())
            plMover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_SEATED);

        if (plMover->IsSitState() && (movementInfo.GetMovementFlags() & (MOVEFLAG_MASK_MOVING | MOVEFLAG_MASK_TURNING)))
            plMover->SetStandState(UNIT_STAND_STATE_STAND);
    }

    // some movement packet fixes, taking into account client/server connection lag.
    uint32 newTime = WorldTimer::getMSTime();

    switch (sWorld.getConfig(CONFIG_UINT32_FIX_MOVE_PACKETS_METHOD))
    {
        case 1:
        {
            if (!m_clientTimeDelay)
            {
                if (newTime > movementInfo.GetTime())
                    m_clientTimeDelay = newTime - movementInfo.GetTime();
            }

            if (m_clientTimeDelay)
                newTime = movementInfo.GetTime() + m_clientTimeDelay;
            break;
        }
        case 2:
        {
            newTime += sWorld.getConfig(CONFIG_UINT32_FIX_MOVE_PACKETS_ADD_TIME);
            break;
        }
    }

    WorldPacket data(opcode, recv_data.size());
    data << mover->GetPackGUID();               // write guid

    movementInfo.UpdateTime(newTime);
    movementInfo.Write(data, opcode);           // write data

    data << movementInfo;
    mover->SendMessageToSetExcept(&data, _player);

    UpdateMoverPosition(movementInfo);

    if (opcode ==  CMSG_MOVE_JUMP && plMover)
        plMover->ProcDamageAndSpell(plMover, PROC_FLAG_NONE, PROC_FLAG_ON_JUMP, PROC_EX_NONE, 0);
}

void WorldSession::HandleForceSpeedChangeAckOpcodes(WorldPacket &recv_data)
{
    uint32 opcode = recv_data.GetOpcode();

    /* extract packet */
    ObjectGuid guid;
    MovementInfo movementInfo;
    float  newspeed;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // counter or moveEvent
    recv_data >> movementInfo;
    recv_data >> newspeed;

    DEBUG_LOG("WORLD: Recvd %s (%u, 0x%X) opcode, unit %s new speed %f ", LookupOpcodeName(opcode), opcode, opcode,
        guid ? guid.GetString().c_str() : "<none>", newspeed);

    // now can skip not our packet
    if (_player->GetObjectGuid() != guid)
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    switch (opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:          move_type = MOVE_WALK;          force_move_type = MOVE_WALK;        break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:           move_type = MOVE_RUN;           force_move_type = MOVE_RUN;         break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:      move_type = MOVE_RUN_BACK;      force_move_type = MOVE_RUN_BACK;    break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:          move_type = MOVE_SWIM;          force_move_type = MOVE_SWIM;        break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:     move_type = MOVE_SWIM_BACK;     force_move_type = MOVE_SWIM_BACK;   break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK:           move_type = MOVE_TURN_RATE;     force_move_type = MOVE_TURN_RATE;   break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:        move_type = MOVE_FLIGHT;        force_move_type = MOVE_FLIGHT;      break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:   move_type = MOVE_FLIGHT_BACK;   force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK:          move_type = MOVE_PITCH_RATE;    force_move_type = MOVE_PITCH_RATE;  break;
        default:
            sLog.outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if (_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if (_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }

    if (!_player->IsOnTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if (_player->GetSpeed(move_type) > newspeed)         // must be greater - just correct
        {
            sLog.outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeedRate(move_type, _player->GetSpeedRate(move_type), true);
        }
        else                                                // must be lesser - cheating
        {
            BASIC_LOG("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(),_player->GetSession()->GetAccountId(),_player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_SET_ACTIVE_MOVER");

    ObjectGuid guid;

    recv_data.ReadGuidMask<7, 2, 1, 0, 4, 5, 6, 3>(guid);
    recv_data.ReadGuidBytes<3, 2, 4, 0, 5, 1, 6, 7>(guid);

    ObjectGuid moverGuid = GetPlayer()->GetMover() ? GetPlayer()->GetMover()->GetObjectGuid() : ObjectGuid();

    if (moverGuid != guid)
    {
        sLog.outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is %s and should be %s",
            moverGuid.GetString().c_str(), guid.GetString().c_str());
        return;
    }
}

void WorldSession::HandleMoveNotActiveMoverOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");
    // recv_data.hexlike();

    ObjectGuid old_mover_guid;
    MovementInfo movementInfo;

    recv_data >> old_mover_guid.ReadAsPacked();
    recv_data >> movementInfo;

    if (_player->GetMover()->GetObjectGuid() == movementInfo.GetGuid())
    {
        DEBUG_LOG("World: CMSG_MOVE_NOT_ACTIVE_MOVER %s received, but his now is active mover!", movementInfo.GetGuid().GetString().c_str());
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
    }

    _player->m_movementInfo = movementInfo;
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvdata*/)
{
    //DEBUG_LOG("WORLD: Recvd CMSG_MOUNTSPECIAL_ANIM");

    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << GetPlayer()->GetObjectGuid();

    GetPlayer()->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveKnockBackAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_KNOCK_BACK_ACK");

    Unit* mover = _player->GetMover();
    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    MovementInfo movementInfo;
    recv_data >> movementInfo;

    if (!VerifyMovementInfo(movementInfo, movementInfo.GetGuid()))
        return;

    mover->m_movementInfo = movementInfo;

    WorldPacket data(SMSG_MOVE_UPDATE_KNOCK_BACK, recv_data.size() + 15);
    data << movementInfo;

    mover->SendMessageToSetExcept(&data, _player);
}

void WorldSession::SendKnockBack(float angle, float horizontalSpeed, float verticalSpeed)
{
    ObjectGuid guid = GetPlayer()->GetObjectGuid();
    float vsin = sin(angle);
    float vcos = cos(angle);

    WorldPacket data(SMSG_MOVE_KNOCK_BACK, 9 + 4 + 4 + 4 + 4 + 4 + 1 + 8);
    data.WriteGuidMask<0, 3, 6, 7, 2, 5, 1, 4>(guid);
    data.WriteGuidBytes<1>(guid);
    data << float(vsin);                                // y direction
    data << uint32(0);                                  // Sequence
    data.WriteGuidBytes<6, 7>(guid);
    data << float(horizontalSpeed);                     // Horizontal speed
    data.WriteGuidBytes<4, 5, 3>(guid);
    data << float(-verticalSpeed);                      // Z Movement speed (vertical)
    data << float(vcos);                                // x direction
    data.WriteGuidBytes<2, 0>(guid);
    SendPacket(&data);
}

void WorldSession::HandleMoveHoverAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_HOVER_ACK");

    recv_data.rfinish();
    /*
    MovementInfo movementInfo;
    recv_data >> movementInfo;
    */
}

void WorldSession::HandleMoveWaterWalkAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_WATER_WALK_ACK");

    recv_data.rfinish();

    /*
    MovementInfo movementInfo;
    recv_data >> movementInfo;
    */
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    if (!_player->isAlive() || _player->isInCombat())
        return;

    ObjectGuid summonerGuid;
    bool agree;
    recv_data >> summonerGuid;
    recv_data >> agree;

    _player->SummonIfPossible(agree);
}

bool WorldSession::VerifyMovementInfo(MovementInfo const& movementInfo, ObjectGuid const& guid) const
{
    // ignore wrong guid (player attempt cheating own session for not own guid possible...)
    if (guid != _player->GetMover()->GetObjectGuid())
        return false;

    Position const* pos = movementInfo.GetPos();

    if (!MaNGOS::IsValidMapCoord(pos->x, pos->y, pos->z, pos->o))
        return false;

    if (movementInfo.GetTransportGuid())
    {
        Position const* tPos = movementInfo.GetTransportPos();

        // transports size limited
        // (also received at zeppelin/lift leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if (tPos->x > 50.0f || tPos->y > 50.0f || tPos->z > 100.0f)
            return false;

        if (!MaNGOS::IsValidMapCoord(pos->x + tPos->x, pos->y + tPos->y, pos->z + tPos->z, pos->o + tPos->o))
            return false;
    }

    return true;
}

void WorldSession::HandlePlayerRelocation(MovementInfo& movementInfo)
{
    Unit* mover = _player->GetMover();

    Player* plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;
    if (!plMover)
        return;

    if (Player *plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL)
    {
        if (movementInfo.GetTransportGuid())
        {
            /* process anticheat check */
            GetPlayer()->GetAntiCheat()->DoAntiCheatCheck(CHECK_TRANSPORT, movementInfo);

            // elevators also cause the client to send MOVEFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
            if (Transport* transport = sObjectMgr.GetTransportByGuid(movementInfo.GetTransportGuid()))
                transport->AddPassenger(plMover, movementInfo.GetTransportPosition());
            // FIXME - unblock next strings after finish ipmlement regular (not MO) transports.
            //if (Transport* transport = plMover->GetMap()->GetTransport(movementInfo.GetTransportGuid()))
            //    transport->AddPassenger(plMover, movementInfo.GetTransportPosition());
        }
        else if (plMover->IsOnTransport())               // if we were on a transport, leave
        {
            plMover->GetTransport()->RemovePassenger(plMover);
            return;
        }
    }

    if (movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING) != plMover->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        Position const* pos = movementInfo.GetPos();
        plMover->SetInWater(!plMover->IsInWater() || plMover->GetTerrain()->IsUnderWater(pos->x, pos->y, pos->z));
    }
}

void WorldSession::UpdateMoverPosition(MovementInfo& movementInfo)
{
    Unit* unitMover = _player->GetMover();

    // player
    if (Player* plMover = unitMover->GetTypeId() == TYPEID_PLAYER ? (Player*)unitMover : NULL)
    {
        plMover->m_movementInfo = movementInfo;

        plMover->SetPosition(Position(movementInfo.GetPosition(), plMover->GetPosition().GetPhaseMask()));
        if (plMover->IsOnTransport())
            plMover->SetTransportPosition(movementInfo.GetTransportPosition());

        float z = movementInfo.GetPos()->z;
        uint32 mapId = plMover->GetMapId();

        // clear unit emote state
        plMover->HandleEmote(EMOTE_ONESHOT_NONE);

        AreaTableEntry const* zone = GetAreaEntryByAreaID(plMover->GetAreaId());

        if((movementInfo.GetPos()->z < (zone ? zone->MaxDepth : -500.0f)) || (plMover->GetMapId() == 617 && movementInfo.GetPos()->z < 2.0f) || (plMover->GetMapId() == 572 && movementInfo.GetPos()->z < 20.0f)
        || (plMover->GetMapId() == 562 && movementInfo.GetPos()->z < -20.0f)) // Prevent falling under textures on some arenas
        {
            if (plMover->GetBattleGround() && plMover->GetBattleGround()->HandlePlayerUnderMap(_player))
            {
                // do nothing, the handle already did if returned true
            }
            else
            {
                // NOTE: this is actually called many times while falling
                // even after the player has been teleported away
                // TODO: discard movement packets after the player is rooted
                if (plMover->isAlive())
                {
                    plMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, plMover->GetMaxHealth());
                    // pl can be alive if GM/etc
                    if (!plMover->isAlive())
                    {
                        // change the death state to CORPSE to prevent the death timer from
                        // starting in the next player update
                        plMover->KillPlayer();
                        plMover->BuildPlayerRepop();
                    }
                }

                // cancel the death timer here if started
                plMover->RepopAtGraveyard();
            }
        }
    }
    // creature charmed
    else if (unitMover->IsInWorld())
    {
        unitMover->m_movementInfo = movementInfo;

        unitMover->SetPosition(Position(movementInfo.GetPosition(), unitMover->GetPosition().GetPhaseMask()));
        if (unitMover->IsOnTransport())
            unitMover->SetTransportPosition(movementInfo.GetTransportPosition());
    }
}
