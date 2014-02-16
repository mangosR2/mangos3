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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef OUTDOOR_PVP_H
#define OUTDOOR_PVP_H

#include "Common.h"
#include "../ObjectGuid.h"
#include "../SharedDefines.h"
#include "OutdoorPvPMgr.h"

class WorldPacket;
class WorldObject;
class Player;
class GameObject;
class Unit;
class Creature;
class Map;

enum CapturePointArtKits
{
    CAPTURE_ARTKIT_ALLIANCE = 2,
    CAPTURE_ARTKIT_HORDE    = 1,
    CAPTURE_ARTKIT_NEUTRAL  = 21
};

enum CapturePointAnimations
{
    CAPTURE_ANIM_ALLIANCE   = 1,
    CAPTURE_ANIM_HORDE      = 0,
    CAPTURE_ANIM_NEUTRAL    = 2
};

typedef std::map<ObjectGuid /*playerGuid*/, bool /*isMainZone*/> GuidZoneMap;

class MANGOS_DLL_SPEC OutdoorPvP
{
    friend class OutdoorPvPMgr;

    public:
        OutdoorPvP(uint32 _id) : m_id(_id), m_isBattleField(false) {}
        virtual ~OutdoorPvP() {}

        // called when the zone is initialized
        virtual void FillInitialWorldStates(uint32 zoneId) {}

        // Process Capture event
        virtual bool HandleEvent(uint32 /*eventId*/, GameObject* /*go*/) { return false; }

        // handle capture objective complete
        virtual void HandleObjectiveComplete(uint32 /*eventId*/, std::list<Player*> /*players*/, Team /*team*/) {}

        // Called when a creature or gameobject is created
        virtual void HandleCreatureCreate(Creature* /*creature*/) {}
        virtual void HandleGameObjectCreate(GameObject* /*go*/) {}

        // Called on creature death
        virtual void HandleCreatureDeath(Creature* /*creature*/) {}

        // Called on creature respawn
        virtual void HandleCreatureRespawn(Creature* /*creature*/) {}

        // called when a player uses a gameobject related to outdoor pvp events
        virtual bool HandleGameObjectUse(Player* /*player*/, GameObject* /*go*/) { return false; }

        // called when a player triggers an areatrigger
        virtual bool HandleAreaTrigger(Player* /*player*/, uint32 /*triggerId*/) { return false; }

        // called when a player drops a flag
        virtual bool HandleDropFlag(Player* /*player*/, uint32 /*spellId*/) { return false; }

        // update - called by the OutdoorPvPMgr
        virtual void Update(uint32 /*diff*/) {}

        // Handle player kill
        void HandlePlayerKill(Player* killer, Player* victim);

        // Wrapper for initial (per-script) WorldState filling, once per zone
        void FillInitialWorldState(uint32 zoneId, uint32 stateId, uint32 value);

        // Damage GO - for WG mostly
        virtual void EventPlayerDamageGO(Player* /*player*/, GameObject* /*target_obj*/, uint32 /*eventId*/, uint32 /*bySpellId*/) {}
        
        // send world state update to all players present
        void SendUpdateWorldStateForMap(uint32 field, uint32 value, Map* map);
        void SendUpdateWorldState(uint32 field, uint32 value);

        // set banner visual
        void SetBannerVisual(const WorldObject* objRef, ObjectGuid goGuid, uint32 artKit, uint32 animId);
        void SetBannerVisual(GameObject* go, uint32 artKit, uint32 animId);

        // applies buff to a team inside the specific zone
        void BuffTeam(Team team, uint32 spellId, bool remove = false, bool onlyMembers = true, uint32 area = 0);

        uint32 GetId() const { return m_id; }
        bool IsBattleField() const { return m_isBattleField; }

        virtual bool IsMember(ObjectGuid guid) { return true; }

        virtual bool InitOutdoorPvPArea() { return true; }

    protected:

        // Player related stuff
        virtual void HandlePlayerEnterZone(Player* /*player*/, bool /*isMainZone*/);
        virtual void HandlePlayerLeaveZone(Player* /*player*/, bool /*isMainZone*/);

        // remove world states
        virtual void SendRemoveWorldStates(Player* /*player*/) {}

        // handle npc/player kill
        virtual void HandlePlayerKillInsideArea(Player* /*killer*/) {}

        // get banner artkit based on controlling team
        uint32 GetBannerArtKit(Team team, uint32 artKitAlliance = CAPTURE_ARTKIT_ALLIANCE, uint32 artKitHorde = CAPTURE_ARTKIT_HORDE, uint32 artKitNeutral = CAPTURE_ARTKIT_NEUTRAL);

        // Handle gameobject spawn / despawn
        void RespawnGO(const WorldObject* objRef, ObjectGuid goGuid, bool respawn);

        // store the players inside the area
        GuidZoneMap m_zonePlayers;

        // outdoor pvp type id
        uint32 m_id;
        bool m_isBattleField;
};

#endif
