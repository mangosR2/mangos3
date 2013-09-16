/*
 * Copyright (C) 2011-2013 /dev/rsa for MangosR2 <http://github.com/MangosR2>
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include "MapUpdater.h"
#include "ObjectUpdateTaskBase.h"
#include "Map.h"
#include "MapManager.h"
#include "World.h"
#include "Database/DatabaseEnv.h"

void MapUpdater::FreezeDetect()
{
    // FIXME - Need rewrite on base timed mutexes
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guardRW(m_rwmutex);
    for (ThreadsMap::const_iterator itr = m_threadsMap.begin(); itr != m_threadsMap.end(); ++itr)
    {
        ObjectUpdateRequest<Map>* rq = itr->second;
        if (!rq)
            continue;

        if (WorldTimer::getMSTime() - rq->getStartTime() > sWorld.getConfig(CONFIG_UINT32_VMSS_FREEZEDETECTTIME))
        {
            if (Map* map = rq->getObject())
            {
                if (sWorld.getConfig(CONFIG_BOOL_VMSS_CONTINENTS_SKIP) && map->IsContinent())
                    continue;

                bool b_needKill = false;
                if (map->IsBroken())
                {
                    if (WorldTimer::getMSTime() - rq->getStartTime() - sWorld.getConfig(CONFIG_UINT32_VMSS_FREEZEDETECTTIME) > sWorld.getConfig(CONFIG_UINT32_VMSS_FORCEUNLOADDELAY))
                        b_needKill = true;
                }
                else
                    b_needKill = true;

                if (b_needKill)
                {
                    DEBUG_LOG("VMSS::MapUpdater::FreezeDetect thread "I64FMT" possible freezed (is update map %u instance %u). Killing.",itr->first,map->GetId(), map->GetInstanceId());
                    kill_thread(itr->first, true);
                }
            }
        }
    }
}

void MapUpdater::MapBrokenEvent(Map* map)
{
    if (!m_brokendata.empty())
    {
        MapBrokenDataMap::iterator itr =  m_brokendata.find(map);
        if (itr != m_brokendata.end())
        {
            if ((WorldTimer::getMSTime() - itr->second.lastErrorTime) > sWorld.getConfig(CONFIG_UINT32_VMSS_TBREMTIME))
                itr->second.Reset();
            else
                itr->second.IncreaseCount();
            return;
        }
    }
    m_brokendata.insert(std::make_pair(map, MapBrokenData()));
}

MapBrokenData const* MapUpdater::GetMapBrokenData(Map* map)
{
    if (!m_brokendata.empty())
    {
        MapBrokenDataMap::const_iterator itr =  m_brokendata.find(map);
        if (itr != m_brokendata.end())
            return &itr->second;
    }
    return NULL;
}
