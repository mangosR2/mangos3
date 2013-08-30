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
#include "BattleGroundTP.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "BattleGroundMgr.h"
#include "WorldPacket.h"
#include "Language.h"
#include "MapManager.h"

BattleGroundTP::BattleGroundTP()
{
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_TP_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_TP_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_TP_HAS_BEGUN;
}

BattleGroundTP::~BattleGroundTP()
{
}

void BattleGroundTP::Update(uint32 diff)
{
    BattleGround::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (m_FlagState[TEAM_INDEX_ALLIANCE] == BG_TP_FLAG_STATE_WAIT_RESPAWN)
        {
            m_FlagsTimer[TEAM_INDEX_ALLIANCE] -= diff;

            if (m_FlagsTimer[TEAM_INDEX_ALLIANCE] < 0)
            {
                m_FlagsTimer[TEAM_INDEX_ALLIANCE] = 0;
                RespawnFlag(ALLIANCE, true);
            }
        }
        if (m_FlagState[TEAM_INDEX_ALLIANCE] == BG_TP_FLAG_STATE_ON_GROUND)
        {
            m_FlagsDropTimer[TEAM_INDEX_ALLIANCE] -= diff;

            if (m_FlagsDropTimer[TEAM_INDEX_ALLIANCE] < 0)
            {
                m_FlagsDropTimer[TEAM_INDEX_ALLIANCE] = 0;
                RespawnFlagAfterDrop(ALLIANCE);
            }
        }
        if (m_FlagState[TEAM_INDEX_HORDE] == BG_TP_FLAG_STATE_WAIT_RESPAWN)
        {
            m_FlagsTimer[TEAM_INDEX_HORDE] -= diff;

            if (m_FlagsTimer[TEAM_INDEX_HORDE] < 0)
            {
                m_FlagsTimer[TEAM_INDEX_HORDE] = 0;
                RespawnFlag(HORDE, true);
            }
        }
        if (m_FlagState[TEAM_INDEX_HORDE] == BG_TP_FLAG_STATE_ON_GROUND)
        {
            m_FlagsDropTimer[TEAM_INDEX_HORDE] -= diff;

            if (m_FlagsDropTimer[TEAM_INDEX_HORDE] < 0)
            {
                m_FlagsDropTimer[TEAM_INDEX_HORDE] = 0;
                RespawnFlagAfterDrop(HORDE);
            }
        }

        if (m_EndTimer <= diff)
        {
            uint32 allianceScore = GetTeamScore(ALLIANCE);
            uint32 hordeScore    = GetTeamScore(HORDE);

            if (allianceScore > hordeScore)
                EndBattleGround(ALLIANCE);
            else if (allianceScore < hordeScore)
                EndBattleGround(HORDE);
            else
            {
                // if 0 => tie
                EndBattleGround(m_LastCapturedFlagTeam);
            }
        }
        else
        {
            uint32 minutesLeftPrev = GetRemainingTimeInMinutes();
            m_EndTimer -= diff;
            uint32 minutesLeft = GetRemainingTimeInMinutes();

            if (minutesLeft != minutesLeftPrev)
                UpdateWorldState(BG_TP_TIME_REMAINING, minutesLeft);
        }
    }
}

void BattleGroundTP::StartingEventCloseDoors()
{
}

void BattleGroundTP::StartingEventOpenDoors()
{
    OpenDoorEvent(BG_EVENT_DOOR);

    // TODO implement timer to despawn doors after a short while

    SpawnEvent(TP_EVENT_SPIRITGUIDES_SPAWN, 0, true);
    SpawnEvent(TP_EVENT_FLAG_A, 0, true);
    SpawnEvent(TP_EVENT_FLAG_H, 0, true);

    // Players that join battleground after start are not eligible to get achievement.
    StartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, BG_TP_EVENT_START_BATTLE);
}

void BattleGroundTP::AddPlayer(Player* plr)
{
    BattleGround::AddPlayer(plr);
    // create score and add it to map, default values are set in constructor
    BattleGroundTPScore* sc = new BattleGroundTPScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattleGroundTP::RespawnFlag(Team team, bool captured)
{
    if (team == ALLIANCE)
    {
        DEBUG_LOG("Respawn Alliance flag");
        m_FlagState[TEAM_INDEX_ALLIANCE] = BG_TP_FLAG_STATE_ON_BASE;
        SpawnEvent(TP_EVENT_FLAG_A, 0, true);
    }
    else
    {
        DEBUG_LOG("Respawn Horde flag");
        m_FlagState[TEAM_INDEX_HORDE] = BG_TP_FLAG_STATE_ON_BASE;
        SpawnEvent(TP_EVENT_FLAG_H, 0, true);
    }

    if (captured)
    {
        // when map_update will be allowed for battlegrounds this code will be useless
        SpawnEvent(TP_EVENT_FLAG_A, 0, true);
        SpawnEvent(TP_EVENT_FLAG_H, 0, true);
        SendMessageToAll(LANG_BG_TP_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
}

void BattleGroundTP::RespawnFlagAfterDrop(Team team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    RespawnFlag(team, false);
    if (team == ALLIANCE)
        SendMessageToAll(LANG_BG_TP_ALLIANCE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    else
        SendMessageToAll(LANG_BG_TP_HORDE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);

    PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);

    GameObject* obj = GetBgMap()->GetGameObject(GetDroppedFlagGuid(team));
    if (obj)
        obj->Delete();
    else
        sLog.outError("Unknown dropped flag bg: %s", GetDroppedFlagGuid(team).GetString().c_str());

    ClearDroppedFlagGuid(team);
}

void BattleGroundTP::EventPlayerCapturedFlag(Player* source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    m_LastCapturedFlagTeam = source->GetTeam();

    Team winner = TEAM_NONE;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    if (source->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        ClearHordeFlagPicker();                             // must be before aura remove to prevent 2 events (drop+capture) at the same time
        // horde flag in base (but not respawned yet)
        m_FlagState[TEAM_INDEX_HORDE] = BG_TP_FLAG_STATE_WAIT_RESPAWN;
        // Drop Horde Flag from Player
        source->RemoveAurasDueToSpell(BG_TP_SPELL_2_FLAG);
        if (GetTeamScore(ALLIANCE) < BG_TP_MAX_TEAM_SCORE)
            AddPoint(ALLIANCE, 1);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_ALLIANCE);
        RewardReputationToTeam(890, m_ReputationCapture, ALLIANCE);
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        ClearAllianceFlagPicker();                          // must be before aura remove to prevent 2 events (drop+capture) at the same time
        // alliance flag in base (but not respawned yet)
        m_FlagState[TEAM_INDEX_ALLIANCE] = BG_TP_FLAG_STATE_WAIT_RESPAWN;
        // Drop Alliance Flag from Player
        source->RemoveAurasDueToSpell(BG_TP_SPELL_1_FLAG);
        if (GetTeamScore(HORDE) < BG_TP_MAX_TEAM_SCORE)
            AddPoint(HORDE, 1);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_HORDE);
        RewardReputationToTeam(889, m_ReputationCapture, HORDE);
    }
    // for flag capture is reward 2 honorable kills
    RewardHonorToTeam(GetBonusHonorFromKill(2), source->GetTeam());

    // despawn flags
    SpawnEvent(TP_EVENT_FLAG_A, 0, false);
    SpawnEvent(TP_EVENT_FLAG_H, 0, false);

    if (source->GetTeam() == ALLIANCE)
        SendMessageToAll(LANG_BG_TP_CAPTURED_HF, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
    else
        SendMessageToAll(LANG_BG_TP_CAPTURED_AF, CHAT_MSG_BG_SYSTEM_HORDE, source);

    UpdateFlagState(source->GetTeam(), 1);                  // flag state none
    UpdateTeamScore(source->GetTeam());
    // only flag capture should be updated
    UpdatePlayerScore(source, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures

    if (GetTeamScore(ALLIANCE) == BG_TP_MAX_TEAM_SCORE)
        winner = ALLIANCE;

    if (GetTeamScore(HORDE) == BG_TP_MAX_TEAM_SCORE)
        winner = HORDE;

    if (winner)
    {
        UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 0);
        UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 0);
        UpdateWorldState(BG_TP_FLAG_STATE_ALLIANCE, 1);
        UpdateWorldState(BG_TP_FLAG_STATE_HORDE, 1);

        EndBattleGround(winner);
    }
    else
    {
        m_FlagsTimer[GetOtherTeamIndex(GetTeamIndex(source->GetTeam()))] = BG_TP_FLAG_RESPAWN_TIME;
    }
}

void BattleGroundTP::EventPlayerDroppedFlag(Player* source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (source->GetTeam() == ALLIANCE)
        {
            if (!IsHordeFlagPickedup())
                return;
            if (GetHordeFlagCarrierGuid() == source->GetObjectGuid())
            {
                ClearHordeFlagPicker();
                source->RemoveAurasDueToSpell(BG_TP_SPELL_2_FLAG);
            }
        }
        else
        {
            if (!IsAllianceFlagPickedup())
                return;
            if (GetAllianceFlagCarrierGuid() == source->GetObjectGuid())
            {
                ClearAllianceFlagPicker();
                source->RemoveAurasDueToSpell(BG_TP_SPELL_1_FLAG);
            }
        }
        return;
    }

    bool set = false;

    if (source->GetTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        if (GetHordeFlagCarrierGuid() == source->GetObjectGuid())
        {
            ClearHordeFlagPicker();
            source->RemoveAurasDueToSpell(BG_TP_SPELL_2_FLAG);
            m_FlagState[TEAM_INDEX_HORDE] = BG_TP_FLAG_STATE_ON_GROUND;
            source->CastSpell(source, BG_TP_SPELL_2_FLAG_DROPPED, true);
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        if (GetAllianceFlagCarrierGuid() == source->GetObjectGuid())
        {
            ClearAllianceFlagPicker();
            source->RemoveAurasDueToSpell(BG_TP_SPELL_1_FLAG);
            m_FlagState[TEAM_INDEX_ALLIANCE] = BG_TP_FLAG_STATE_ON_GROUND;
            source->CastSpell(source, BG_TP_SPELL_1_FLAG_DROPPED, true);
            set = true;
        }
    }

    if (set)
    {
        source->CastSpell(source, SPELL_RECENTLY_DROPPED_FLAG, true);
        UpdateFlagState(source->GetTeam(), 1);

        if (source->GetTeam() == ALLIANCE)
        {
            SendMessageToAll(LANG_BG_TP_DROPPED_HF, CHAT_MSG_BG_SYSTEM_HORDE, source);
            UpdateWorldState(BG_TP_FLAG_UNK_HORDE, uint32(-1));
        }
        else
        {
            SendMessageToAll(LANG_BG_TP_DROPPED_AF, CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
            UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, uint32(-1));
        }

        m_FlagsDropTimer[GetOtherTeamIndex(GetTeamIndex(source->GetTeam()))] = BG_TP_FLAG_DROP_TIME;
    }
}

void BattleGroundTP::EventPlayerClickedOnFlag(Player* source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    int32 message_id = 0;
    ChatMsg type;

    uint8 event = (sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetGUIDLow())).event1;

    // alliance flag picked up from base
    if (source->GetTeam() == HORDE && GetFlagState(ALLIANCE) == BG_TP_FLAG_STATE_ON_BASE
            && event == TP_EVENT_FLAG_A)
    {
        message_id = LANG_BG_TP_PICKEDUP_AF;
        type = CHAT_MSG_BG_SYSTEM_HORDE;
        PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SpawnEvent(TP_EVENT_FLAG_A, 0, false);
        SetAllianceFlagCarrier(source->GetObjectGuid());
        m_FlagState[TEAM_INDEX_ALLIANCE] = BG_TP_FLAG_STATE_ON_PLAYER;
        // update world state to show correct flag carrier
        UpdateFlagState(HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 1);
        source->CastSpell(source, BG_TP_SPELL_1_FLAG, true);
    }

    // horde flag picked up from base
    if (source->GetTeam() == ALLIANCE && GetFlagState(HORDE) == BG_TP_FLAG_STATE_ON_BASE
            && event == TP_EVENT_FLAG_H)
    {
        message_id = LANG_BG_TP_PICKEDUP_HF;
        type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
        PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
        SpawnEvent(TP_EVENT_FLAG_H, 0, false);
        SetHordeFlagCarrier(source->GetObjectGuid());
        m_FlagState[TEAM_INDEX_HORDE] = BG_TP_FLAG_STATE_ON_PLAYER;
        // update world state to show correct flag carrier
        UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 1);
        source->CastSpell(source, BG_TP_SPELL_2_FLAG, true);
    }

    // Alliance flag on ground(not in base) (returned or picked up again from ground!)
    if (GetFlagState(ALLIANCE) == BG_TP_FLAG_STATE_ON_GROUND && source->IsWithinDistInMap(target_obj, 10))
    {
        if (source->GetTeam() == ALLIANCE)
        {
            message_id = LANG_BG_TP_RETURNED_AF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            UpdateFlagState(HORDE, BG_TP_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(ALLIANCE, false);
            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(source, SCORE_FLAG_RETURNS, 1);
        }
        else
        {
            message_id = LANG_BG_TP_PICKEDUP_AF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SpawnEvent(TP_EVENT_FLAG_A, 0, false);
            SetAllianceFlagCarrier(source->GetObjectGuid());
            source->CastSpell(source, BG_TP_SPELL_1_FLAG, true);
            m_FlagState[TEAM_INDEX_ALLIANCE] = BG_TP_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
            UpdateWorldState(BG_TP_FLAG_UNK_ALLIANCE, 1);
        }
        // called in HandleGameObjectUseOpcode:
        // target_obj->Delete();
    }

    // Horde flag on ground(not in base) (returned or picked up again)
    if (GetFlagState(HORDE) == BG_TP_FLAG_STATE_ON_GROUND && source->IsWithinDistInMap(target_obj, 10))
    {
        if (source->GetTeam() == HORDE)
        {
            message_id = LANG_BG_TP_RETURNED_HF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(HORDE, false);
            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(source, SCORE_FLAG_RETURNS, 1);
        }
        else
        {
            message_id = LANG_BG_TP_PICKEDUP_HF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
            SpawnEvent(TP_EVENT_FLAG_H, 0, false);
            SetHordeFlagCarrier(source->GetObjectGuid());
            source->CastSpell(source, BG_TP_SPELL_2_FLAG, true);
            m_FlagState[TEAM_INDEX_HORDE] = BG_TP_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
            UpdateWorldState(BG_TP_FLAG_UNK_HORDE, 1);
        }
        // called in HandleGameObjectUseOpcode:
        // target_obj->Delete();
    }

    if (!message_id)
        return;

    SendMessageToAll(message_id, type, source);
    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattleGroundTP::RemovePlayer(Player* plr, ObjectGuid guid)
{
    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && m_FlagKeepers[TEAM_INDEX_ALLIANCE] == guid)
    {
        if (!plr)
        {
            sLog.outError("BattleGroundTP: Removing offline player who has the FLAG!!");
            ClearAllianceFlagPicker();
            RespawnFlag(ALLIANCE, false);
        }
        else
            EventPlayerDroppedFlag(plr);
    }
    if (IsHordeFlagPickedup() && m_FlagKeepers[TEAM_INDEX_HORDE] == guid)
    {
        if (!plr)
        {
            sLog.outError("BattleGroundTP: Removing offline player who has the FLAG!!");
            ClearHordeFlagPicker();
            RespawnFlag(HORDE, false);
        }
        else
            EventPlayerDroppedFlag(plr);
    }
}

void BattleGroundTP::UpdateFlagState(Team team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_TP_FLAG_STATE_ALLIANCE, value);
    else
        UpdateWorldState(BG_TP_FLAG_STATE_HORDE, value);
}

void BattleGroundTP::UpdateTeamScore(Team team)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_TP_FLAG_CAPTURES_ALLIANCE, GetTeamScore(team));
    else
        UpdateWorldState(BG_TP_FLAG_CAPTURES_HORDE, GetTeamScore(team));
}

void BattleGroundTP::HandleAreaTrigger(Player* source, uint32 trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    // uint32 SpellId = 0;
    // uint64 buff_guid = 0;
    switch (trigger)
    {
        case 5904:                                          // unk1
        case 5905:                                          // unk2
        case 5906:                                          // unk3
        case 5907:                                          // unk4
        case 5908:                                          // unk5
        case 5909:                                          // unk6
        case 5910:                                          // unk7
        case 5911:                                          // unk8
        case 5914:                                          // unk9
        case 5916:                                          // unk10
        case 5917:                                          // unk11
        case 5918:                                          // unk12
        case 5920:                                          // unk13
        case 5921:                                          // unk14
        case 6803:                                          // unk15
        case 6804:                                          // unk16
        case 6805:                                          // unk17
        case 6806:                                          // unk18
        case 8154:                                          // unk19
        case 8155:                                          // unk20
        case 8304:                                          // unk21
        case 8305:                                          // unk22
            break; 
        default:
            sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", trigger);
            source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", trigger);
            break;
    }
}

bool BattleGroundTP::SetupBattleGround()
{
    return true;
}

void BattleGroundTP::Reset()
{
    // call parent's class reset
    BattleGround::Reset();

    // spiritguides and flags not spawned at beginning
    m_ActiveEvents[TP_EVENT_SPIRITGUIDES_SPAWN] = BG_EVENT_NONE;
    m_ActiveEvents[TP_EVENT_FLAG_A] = BG_EVENT_NONE;
    m_ActiveEvents[TP_EVENT_FLAG_H] = BG_EVENT_NONE;

    for (uint8 i = 0; i < PVP_TEAM_COUNT; ++i)
    {
        m_DroppedFlagGuid[i].Clear();
        m_FlagKeepers[i].Clear();
        m_FlagState[i]       = BG_TP_FLAG_STATE_ON_BASE;
        m_TeamScores[i]      = 0;
    }
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    m_ReputationCapture = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills = (isBGWeekend) ? 3 : 1;
    m_HonorEndKills = (isBGWeekend) ? 4 : 2;

    m_EndTimer = BG_TP_TIME_LIMIT;
    m_LastCapturedFlagTeam = TEAM_NONE;
}

void BattleGroundTP::EndBattleGround(Team winner)
{
    // win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(m_HonorWinKills), HORDE);
    // complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(m_HonorEndKills), HORDE);

    BattleGround::EndBattleGround(winner);
}

void BattleGroundTP::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);

    BattleGround::HandleKillPlayer(player, killer);
}

void BattleGroundTP::UpdatePlayerScore(Player* source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(source->GetObjectGuid());
    if (itr == m_PlayerScores.end())                        // player not found
        return;

    switch (type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattleGroundTPScore*)itr->second)->FlagCaptures += value;
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattleGroundTPScore*)itr->second)->FlagReturns += value;
            break;
        default:
            BattleGround::UpdatePlayerScore(source, type, value);
            break;
    }
}

WorldSafeLocsEntry const* BattleGroundTP::GetClosestGraveYard(Player* player)
{
    // if status in progress, it returns main graveyards with spiritguides
    // else it will return the graveyard in the flagroom - this is especially good
    // if a player dies in preparation phase - then the player can't cheat
    // and teleport to the graveyard outside the flagroom
    // and start running around, while the doors are still closed
    if (GetStatus() == STATUS_WAIT_JOIN)
    {
        if (player->GetTeam() == ALLIANCE)
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_FLAGROOM_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(TP_GRAVEYARD_FLAGROOM_HORDE);
    }

    TeamIndex teamIndex = GetTeamIndex(player->GetTeam());

    // Is there any occupied node for this team?
    std::vector<uint32> nodes;
    if (teamIndex == TEAM_INDEX_ALLIANCE)
    {
        nodes.push_back(TP_GRAVEYARD_MAIN_ALLIANCE);
        nodes.push_back(TP_GRAVEYARD_CENTER_ALLIANCE);
    }
    else
    {
        nodes.push_back(TP_GRAVEYARD_MAIN_HORDE);
        nodes.push_back(TP_GRAVEYARD_CENTER_HORDE);
    }

    WorldSafeLocsEntry const* good_entry = NULL;
    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float plr_x = player->GetPositionX();
        float plr_y = player->GetPositionY();

        float mindist = 999999.0f;
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const*entry = sWorldSafeLocsStore.LookupEntry(nodes[i]);
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
        good_entry = sWorldSafeLocsStore.LookupEntry(teamIndex == TEAM_INDEX_ALLIANCE ? TP_GRAVEYARD_FLAGROOM_ALLIANCE : TP_GRAVEYARD_FLAGROOM_HORDE);

    return good_entry;
}

void BattleGroundTP::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    // FIXME - need move this in DB!
/*
    FillInitialWorldState(data, count, BG_TP_FLAG_CAPTURES_ALLIANCE, GetTeamScore(ALLIANCE));
    FillInitialWorldState(data, count, BG_TP_FLAG_CAPTURES_HORDE, GetTeamScore(HORDE));

    if (m_FlagState[TEAM_INDEX_ALLIANCE] == BG_TP_FLAG_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_ALLIANCE, -1);
    else if (m_FlagState[TEAM_INDEX_ALLIANCE] == BG_TP_FLAG_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_ALLIANCE, 1);
    else
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_ALLIANCE, 0);

    if (m_FlagState[TEAM_INDEX_HORDE] == BG_TP_FLAG_STATE_ON_GROUND)
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_HORDE, -1);
    else if (m_FlagState[TEAM_INDEX_HORDE] == BG_TP_FLAG_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_HORDE, 1);
    else
        FillInitialWorldState(data, count, BG_TP_FLAG_UNK_HORDE, 0);

    FillInitialWorldState(data, count, BG_TP_FLAG_CAPTURES_MAX, BG_TP_MAX_TEAM_SCORE);

    if (m_FlagState[TEAM_INDEX_HORDE] == BG_TP_FLAG_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TP_FLAG_STATE_HORDE, 2);
    else
        FillInitialWorldState(data, count, BG_TP_FLAG_STATE_HORDE, 1);

    if (m_FlagState[TEAM_INDEX_ALLIANCE] == BG_TP_FLAG_STATE_ON_PLAYER)
        FillInitialWorldState(data, count, BG_TP_FLAG_STATE_ALLIANCE, 2);
    else
        FillInitialWorldState(data, count, BG_TP_FLAG_STATE_ALLIANCE, 1);

    FillInitialWorldState(data, count, BG_TP_TIME_ENABLED, WORLD_STATE_ADD);
    FillInitialWorldState(data, count, BG_TP_TIME_REMAINING, GetRemainingTimeInMinutes());
*/
}