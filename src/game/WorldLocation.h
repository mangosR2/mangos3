/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2011-2012 MangosR2 <http://mangosr2.2x2forum.com/>
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

#ifndef _WORLDLOCATION_H
#define _WORLDLOCATION_H

#include "Common.h"

struct MANGOS_DLL_SPEC Position
{
    Position() 
        : x(0.0f), y(0.0f), z(0.0f), o(0.0f)
    {};

    Position(float _x, float _y, float _z, float _o)
        : x(_x), y(_y), z(_z), o(_o)
    {};

    virtual ~Position()
    {};

    union
    {
        float x;
        float coord_x;
    };
    union
    {
        float y;
        float coord_y;
    };
    union
    {
        float z;
        float coord_z;
    };
    union
    {
        float o;
        float orientation;
    };

    virtual bool HasMap() const { return false; };

    bool operator == (Position const& pos) const
    {
        return ((fabs(x - pos.x) < M_NULL_F)
            && (fabs(y - pos.y) < M_NULL_F)
            && (fabs(z - pos.z) < M_NULL_F));
    };
};

class WorldObject;

struct MANGOS_DLL_SPEC WorldLocation : public Position
{

    public:
    WorldLocation()
        : Position(), mapid(-1), instance(0), realmid(0)
    {}

    WorldLocation(uint32 _mapid, float _x, float _y, float _z, float _o = 0)
        : Position(_x, _y, _z, _o), mapid(_mapid), instance(0), realmid(0)
    {}

    WorldLocation(uint32 _mapid, uint32 _instance, uint32 _realmid)
        : Position(), mapid(_mapid), instance(_instance), realmid(_realmid)
    {}

    WorldLocation(float _x, float _y, float _z, float _o, uint32 _mapid, uint32 _instance, uint32 _realmid)
        : Position(_x, _y, _z, _o), mapid(_mapid), instance(_instance), realmid(_realmid)
    {}

    WorldLocation(WorldLocation const &loc)
        : Position(loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation), mapid(loc.mapid), instance(loc.instance), realmid(loc.realmid)
    {}

    WorldLocation(WorldObject const& object);

    virtual ~WorldLocation() 
    {};

    bool operator == (WorldLocation const& loc) const;

    virtual bool HasMap() const override
    {
        return mapid >= 0;
    }

    Position const& GetPosition() { return *this; };

    uint32 GetZoneId() const;
    uint32 GetAreaId() const;

    uint32 GetRealmId()    const { return realmid; };
    uint32 GetMapId()      const { return HasMap() ? abs(mapid) :  451; /*Programmers Isle. possible need assert here.*/ };
    uint32 GetInstanceId() const { return HasMap() ? instance :  0; };


    void SetMapId(uint32 value);
    void SetInstanceId(uint32 value)  { instance = value; };
    //void SetRealmId(uint32 value)   { realmid  = value; }; // Currently not need make realm switch - awaiting multirealm implement.

    bool IsValidMapCoord(WorldLocation const& loc);

    void SetOrientation(float value);

    WorldLocation& operator = (WorldLocation const& loc);

    private:
    int32     mapid;                      // mapid    = -1 for not fully initialized WorldLocation
    uint32    instance;                   // instance = 0  for not fully initialized WorldLocation ("current instance")
    uint32    realmid;                    // realmid  = 0  for "always current realm". 

};
#endif
