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
#include "AchievementMgr.h"
#include "DBCStores.h"
#include "Player.h"
#include "WorldPacket.h"
#include "DBCEnums.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "SpellMgr.h"
#include "ArenaTeam.h"
#include "ProgressBar.h"
#include "Mail.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Language.h"
#include "MapManager.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundAB.h"
#include "BattleGround/BattleGroundAV.h"
#include "BattleGround/BattleGroundEY.h"
#include "BattleGround/BattleGroundIC.h"
#include "BattleGround/BattleGroundSA.h"
#include "BattleGround/BattleGroundWS.h"
#include "Map.h"
#include "InstanceData.h"
#include "DBCStructure.h"
#include "Chat.h"
#include "Guild.h"
#include "GuildMgr.h"

#include "Policies/Singleton.h"


INSTANTIATE_SINGLETON_1(AchievementGlobalMgr);

namespace MaNGOS
{
    class AchievementChatBuilder
    {
        public:
            AchievementChatBuilder(Player const& pl, ChatMsg msgtype, int32 textId, uint32 ach_id)
                : i_player(pl), i_msgtype(msgtype), i_textId(textId), i_achievementId(ach_id) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);

                ChatHandler::BuildChatPacket(data, i_msgtype, text, LANG_UNIVERSAL, i_player.GetChatTag(),  i_player.GetObjectGuid(), NULL, i_player.GetObjectGuid(), NULL, NULL,
                    i_achievementId);
            }

        private:
            Player const& i_player;
            ChatMsg i_msgtype;
            int32 i_textId;
            uint32 i_achievementId;
    };
}                                                           // namespace MaNGOS

// Helper function to avoid having to specialize template for a 800 line long function
template <typename T> static bool IsGuild() { return false; }
template <> bool IsGuild<Guild>() { return true; }

bool AchievementCriteriaRequirement::IsValid(AchievementCriteriaEntry const* criteria)
{
    switch (criteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:      // only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            break;
        default:
            sLog.outErrorDb("Table `achievement_criteria_requirement` have not supported data for criteria %u (Not supported as of its criteria type: %u), ignore.", criteria->ID, criteria->requiredType);
            return false;
    }

    switch (requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!creature.id || !ObjectMgr::GetCreatureTemplate(creature.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent creature id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!classRace.class_id && !classRace.race_id)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_CLASS_RACE (%u) must have not 0 in one from value fields, ignore.",
                                criteria->ID, criteria->requiredType, requirementType);
                return false;
            }
            if (classRace.class_id && ((1 << (classRace.class_id - 1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent class in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id - 1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent race in value2 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, classRace.race_id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_LESS_HEALTH (%u) have wrong percent value in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, health.percent);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (player_dead.own_team_flag > 1)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD (%u) have wrong boolean value1 (%u).",
                                criteria->ID, criteria->requiredType, requirementType, player_dead.own_team_flag);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
        {
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(aura.spell_id);
            if (!spellEntry)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.spell_id);
                return false;
            }
            if (aura.effect_idx >= MAX_EFFECT_INDEX)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell effect index in value2 (%u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.effect_idx);
                return false;
            }
            SpellEffectEntry const* spellEffect = spellEntry->GetSpellEffect(SpellEffectIndex(aura.effect_idx));
            if (!spellEffect || !spellEffect->EffectApplyAuraName)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have non-aura spell effect (ID: %u Effect: %u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.spell_id, aura.effect_idx);
                return false;
            }
            return true;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
            if (!GetAreaEntryByAreaID(area.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA (%u) have wrong area id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (level.minlevel > STRONG_MAX_LEVEL)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL (%u) have wrong minlevel in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, level.minlevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER (%u) have wrong gender in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            if (difficulty.difficulty >= MAX_DIFFICULTY)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY (%u) have wrong difficulty in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, difficulty.difficulty);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            if (map_players.maxcount <= 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT (%u) have wrong max players count in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, map_players.maxcount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM (%u) have unknown team in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            if (drunk.state >= MAX_DRUNKEN)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK (%u) have unknown drunken state in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY (%u) have unknown holiday in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
            if (equipped_item.item_quality >= MAX_ITEM_QUALITY)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL (%u) have unknown quality state in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, equipped_item.item_quality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
        {
            CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id);
            if (!titleInfo)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE (%u) have unknown title_id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, known_title.title_id);
                return false;
            }
            return true;
        }
        default:
            sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) have data for not supported data type (%u), ignore.", criteria->ID, criteria->requiredType, requirementType);
            return false;
    }
}

bool AchievementCriteriaRequirement::Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 /*= 0*/) const
{
    switch (requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!target || target->GetTypeId() != TYPEID_UNIT)
                return false;
            return target->GetEntry() == creature.id;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            if (classRace.class_id && classRace.class_id != ((Player*)target)->getClass())
                return false;
            if (classRace.race_id && classRace.race_id != ((Player*)target)->getRace())
                return false;
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            return target->GetHealth() * 100 <= health.percent * target->GetMaxHealth();
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (!target || target->GetTypeId() != TYPEID_PLAYER || target->isAlive() || ((Player*)target)->GetDeathTimer() == 0)
                return false;
            // flag set == must be same team, not set == different team
            return (((Player*)target)->GetTeam() == source->GetTeam()) == (player_dead.own_team_flag != 0);
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
            return source->HasAura(aura.spell_id, SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
        {
            uint32 zone_id, area_id;
            source->GetZoneAndAreaId(zone_id, area_id);
            return area.id == zone_id || area.id == area_id;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
            return target && target->HasAura(aura.spell_id, SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
            return miscvalue1 >= value.minvalue;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (!target)
                return false;
            return target->getLevel() >= level.minlevel;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (!target)
                return false;
            return target->getGender() == gender.gender;
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
            return false;                                   // always fail
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            return source->GetMap()->GetSpawnMode() == difficulty.difficulty;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            return source->GetMap()->GetPlayersCountExceptGMs() <= map_players.maxcount;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            return ((Player*)target)->GetTeam() == Team(team.team);
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            return (uint32)Player::GetDrunkenstateByValue(source->GetDrunkValue()) >= drunk.state;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            return sGameEventMgr.IsActiveHoliday(HolidayIds(holiday.id));
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        {
            BattleGround* bg = source->GetBattleGround();
            if (!bg)
                return false;
            return bg->IsTeamScoreInRange(source->GetTeam() == ALLIANCE ? HORDE : ALLIANCE, bg_loss_team_score.min_score, bg_loss_team_score.max_score);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        {
            if (!source->IsInWorld())
                return false;
            InstanceData* data = source->GetInstanceData();
            if (!data)
            {
                sLog.outErrorDb("Achievement system call ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT (%u) for achievement criteria %u for map %u but map not have instance script",
                                ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT, criteria_id, source->GetMapId());
                return false;
            }
            return data->CheckAchievementCriteriaMeet(criteria_id, source, target, miscvalue1);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
        {
            Item* item = source->GetItemByPos(INVENTORY_SLOT_BAG_0, miscvalue1);
            if (!item)
                return false;
            ItemPrototype const* proto = item->GetProto();
            return proto->ItemLevel >= equipped_item.item_level && proto->Quality >= equipped_item.item_quality;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
        {
            time_t birthday_start = time_t(sWorld.getConfig(CONFIG_UINT32_BIRTHDAY_TIME));

            tm birthday_tm = *localtime(&birthday_start);

            // exactly N birthday
            birthday_tm.tm_year += birthday_login.nth_birthday;

            time_t birthday = mktime(&birthday_tm);

            time_t now = sWorld.GetGameTime();
            return now <= birthday + DAY && now >= birthday;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
        {
            if (CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id))
                return source && source->HasTitle(titleInfo->bit_index);

            return false;
        }
    }
    return false;
}

bool AchievementCriteriaRequirementSet::Meets(Player const* source, Unit const* target, uint32 miscvalue /*= 0*/) const
{
    for (Storage::const_iterator itr = storage.begin(); itr != storage.end(); ++itr)
        if (!itr->Meets(criteria_id, source, target, miscvalue))
            return false;

    return true;
}

template <class T>
AchievementMgr<T>::AchievementMgr(T* owner)
{
    m_owner = owner;
    m_achievementPoints = 0;
}

template <class T>
AchievementMgr<T>::~AchievementMgr()
{
}

template<class T>
void AchievementMgr<T>::SendPacket(WorldPacket* data) const
{
}

template<>
void AchievementMgr<Guild>::SendPacket(WorldPacket* data) const
{
    GetOwner()->BroadcastPacket(data);
}

template<>
void AchievementMgr<Player>::SendPacket(WorldPacket* data) const
{
    GetOwner()->GetSession()->SendPacket(data);
}

template<class T>
void AchievementMgr<T>::SendCriteriaProgressRemove(uint32 criteriaId)
{
    WorldPacket data(SMSG_CRITERIA_DELETED, 4);
    data << uint32(criteriaId);
    SendPacket(&data);
}

template<>
void AchievementMgr<Guild>::SendCriteriaProgressRemove(uint32 criteriaId)
{
    ObjectGuid guid = GetOwner()->GetObjectGuid();

    WorldPacket data(SMSG_GUILD_CRITERIA_DELETED, 4 + 8);
    data.WriteGuidMask<6, 5, 7, 0, 1, 3, 2, 4>(guid);

    data.WriteGuidBytes<2, 3, 4, 1, 7>(guid);
    data << uint32(criteriaId);
    data.WriteGuidBytes<5, 0, 6>(guid);

    SendPacket(&data);
}

template <class T>
void AchievementMgr<T>::Reset()
{
}

template <>
void AchievementMgr<Player>::Reset()
{
    for(CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4);
        data << uint32(iter->first);
        SendPacket(&data);
    }

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        SendCriteriaProgressRemove(iter->first);

    m_completedAchievements.clear();
    m_criteriaProgress.clear();
    DeleteFromDB(m_owner->GetObjectGuid());

    m_achievementPoints = 0;

    // re-fill data
    CheckAllAchievementCriteria(GetOwner());
}

template <>
void AchievementMgr<Guild>::Reset()
{
    ObjectGuid guid = GetOwner()->GetObjectGuid();

    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DELETED, 4);
        data.WriteGuidMask<4, 1, 2, 3, 0, 7, 5, 6>(guid);
        data << uint32(iter->first);
        data.WriteGuidBytes<5, 1, 3, 6, 0, 7>(guid);
        data << uint32(secsToTimeBitFields(iter->second.date));
        data.WriteGuidBytes<4, 2>(guid);
        SendPacket(&data);
    }

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        SendCriteriaProgressRemove(iter->first);

    m_achievementPoints = 0;

    m_completedAchievements.clear();
    m_criteriaProgress.clear();
    DeleteFromDB(m_owner->GetObjectGuid());

    // re-fill data
    //CheckAllAchievementCriteria(GetOwner());
}

template <class T>
void AchievementMgr<T>::ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, Player* referencePlayer)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::ResetAchievementCriteria(%u, %u, %u)", type, miscvalue1, miscvalue2);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && referencePlayer->GetSession()->GetSecurity() > SEC_MODERATOR)
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type);
    for (AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const* achievementCriteria = (*i);

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria, achievement))
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY: // reset days of daily quests complete
                if(miscvalue1 == ACHIEVEMENT_CRITERIA_CONDITION_DAILY)
                    SetCriteriaProgress(achievementCriteria, achievement, 0, referencePlayer, PROGRESS_SET);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:     // have total statistic also not expected to be reset
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:    // have total statistic also not expected to be reset
                if (achievementCriteria->healing_done.flag == miscvalue1 &&
                        achievementCriteria->healing_done.mapid == miscvalue2)
                    SetCriteriaProgress(achievementCriteria, achievement, 0, referencePlayer, PROGRESS_SET);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA: // have total statistic also not expected to be reset
                // reset only the criteria having the miscvalue1 condition
                if (achievementCriteria->win_rated_arena.flag == miscvalue1)
                    SetCriteriaProgress(achievementCriteria, achievement, 0, referencePlayer, PROGRESS_SET);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            {
                switch(achievementCriteria->referredAchievement) // All achievements that should reset its progress.
                {
                    case 73:
                    case 204:
                    case 203:
                    case 213:
                    case 216:
                    case 229:
                    case 231:
                    case 582:
                    case 583:
                    case 584:
                    case 587:
                    case 872:
                    case 1153:
                    case 1251:
                    case 1765:
                    case 1766:
                    case 2189:
                    case 2190:
                    case 2193:
                    case 3845:
                    case 3848:
                    case 3849:
                    case 3853:
                    case 5210:      // Two-Timer
                    case 5220:      // I'm in the Black Lodge
                    case 5226:      // Cloud Nine
                    case 5227:      // Cloud Nine
                    case 5228:      // Wild Hammering
                        SetCriteriaProgress(achievementCriteria, achievement, 0, referencePlayer, PROGRESS_SET);
                    default: continue; // Do not reset progress for other achievements.
                }
                break;
            }
            default:                                        // reset all cases
                break;
        }
    }
}

template<>
void AchievementMgr<Guild>::ResetAchievementCriteria(AchievementCriteriaTypes /*type*/, uint32 /*miscvalue1*/, uint32 /*miscvalue2*/, Player* /*referencePlayer*/)
{
    // Not needed
}

template <class T>
void AchievementMgr<T>::DeleteFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    CharacterDatabase.BeginTransaction();
    if (IsGuild<T>())
    {
        CharacterDatabase.PExecute("DELETE FROM guild_achievement WHERE guildId = %u", lowguid);
        CharacterDatabase.PExecute("DELETE FROM guild_achievement_progress WHERE guildId = %u", lowguid);
    }
    else
    {
        CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = %u", lowguid);
        CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE guid = %u", lowguid);
    }
    CharacterDatabase.CommitTransaction();
}

template <class T>
void AchievementMgr<T>::SaveToDB()
{
}

template <>
void AchievementMgr<Player>::SaveToDB()
{
    static SqlStatementID delComplAchievements ;
    static SqlStatementID insComplAchievements ;
    static SqlStatementID delProgress ;
    static SqlStatementID insProgress ;

    if(!m_completedAchievements.empty())
    {
        //delete existing achievements in the loop
        for (CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            /// mark as saved in db
            iter->second.changed = false;

            SqlStatement stmt = CharacterDatabase.CreateStatement(delComplAchievements, "DELETE FROM character_achievement WHERE guid = ? AND achievement = ?");
            stmt.PExecute(GetOwner()->GetGUIDLow(), iter->first);

            stmt = CharacterDatabase.CreateStatement(insComplAchievements, "INSERT INTO character_achievement (guid, achievement, date) VALUES (?, ?, ?)");
            stmt.PExecute(GetOwner()->GetGUIDLow(), iter->first, uint64(iter->second.date));
        }
    }

    if (!m_criteriaProgress.empty())
    {
        static SqlStatementID delProgress;
        static SqlStatementID insProgress;

        SqlStatement delStmt = CharacterDatabase.CreateStatement(delProgress, "DELETE FROM character_achievement_progress WHERE guid = ? AND criteria = ?");
        SqlStatement insStmt = CharacterDatabase.CreateStatement(insProgress, "INSERT INTO character_achievement_progress (guid, criteria, counter, date) VALUES (?, ?, ?, ?)");

        // insert achievements
        for (CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            /// mark as updated in db
            iter->second.changed = false;

            // new/changed record data
            SqlStatement stmt = CharacterDatabase.CreateStatement(delProgress, "DELETE FROM character_achievement_progress WHERE guid = ? AND criteria = ?");
            stmt.PExecute(GetOwner()->GetGUIDLow(), iter->first);

            bool needSave = iter->second.counter != 0;
            if (!needSave)
            {
                AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(iter->first);
                needSave = criteria && criteria->timeLimit > 0;
            }

            if (needSave)
            {
                stmt = CharacterDatabase.CreateStatement(insProgress, "INSERT INTO character_achievement_progress (guid, criteria, counter, date) VALUES (?, ?, ?, ?)");
                stmt.PExecute(GetOwner()->GetGUIDLow(), iter->first, iter->second.counter, uint64(iter->second.date));
            }
        }
    }
}

template<>
void AchievementMgr<Guild>::SaveToDB()
{
    static SqlStatementID delGuildComplAchievements ;
    static SqlStatementID insGuildComplAchievements ;
    static SqlStatementID delGuildProgress ;
    static SqlStatementID insGuildProgress ;

    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        if (!itr->second.changed)
            continue;

        SqlStatement stmt = CharacterDatabase.CreateStatement(delGuildComplAchievements, "DELETE FROM guild_achievement WHERE guildId = ? AND achievement = ?");
        stmt.PExecute(GetOwner()->GetId(), itr->first);

        stmt = CharacterDatabase.CreateStatement(insGuildComplAchievements, "INSERT INTO guild_achievement (guildId, achievement, date, guids) VALUES (?, ?, ?, ?)");
        stmt.addUInt32(GetOwner()->GetId());
        stmt.addUInt16(itr->first);
        stmt.addUInt32(itr->second.date);
        std::ostringstream guidstr;
        for (std::set<ObjectGuid>::const_iterator gItr = itr->second.guids.begin(); gItr != itr->second.guids.end(); ++gItr)
            guidstr << gItr->GetCounter() << ' ';

        stmt.addString(guidstr.str());
        stmt.Execute();
    }

    for (CriteriaProgressMap::const_iterator itr = m_criteriaProgress.begin(); itr != m_criteriaProgress.end(); ++itr)
    {
        if (!itr->second.changed)
            continue;

        SqlStatement stmt = CharacterDatabase.CreateStatement(delGuildProgress, "DELETE FROM guild_achievement_progress WHERE guildId = ? AND criteria = ?");
        stmt.PExecute(GetOwner()->GetId(), itr->first);

        stmt = CharacterDatabase.CreateStatement(insGuildProgress, "INSERT INTO guild_achievement_progress (guildId, criteria, counter, date, completedGuid) VALUES (?, ?, ?, ?, ?)");
        stmt.addUInt32(GetOwner()->GetId());
        stmt.addUInt16(itr->first);
        stmt.addUInt32(itr->second.counter);
        stmt.addUInt32(itr->second.date);
        stmt.addUInt32(itr->second.CompletedGUID.GetCounter());
        stmt.Execute();
    }
}

template <class T>
void AchievementMgr<T>::LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult)
{
}

template <>
void AchievementMgr<Player>::LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult)
{
    // Note: this code called before any character data loading so don't must triggering any events req. inventory/etc
    // all like cases must be happens in CheckAllAchievementCriteria called after character data load

    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();

            uint32 achievement_id = fields[0].GetUInt32();

            // don't must happen: cleanup at server startup in sAchievementMgr.LoadCompletedAchievements()
            AchievementEntry const * achievement = sAchievementStore.LookupEntry(achievement_id);
            if (!achievement)
                continue;

            if (achievement->flags & ACHIEVEMENT_FLAG_GUILD)
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievement_id];
            ca.date = time_t(fields[1].GetUInt64());
            ca.changed = false;

            m_achievementPoints += achievement->points;
        }
        while(achievementResult->NextRow());

        delete achievementResult;
    }

    if (criteriaResult)
    {
        do
        {
            Field* fields = criteriaResult->Fetch();

            uint32 id      = fields[0].GetUInt32();
            uint32 counter = fields[1].GetUInt32();
            time_t date    = time_t(fields[2].GetUInt64());

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // we will remove nonexistent criteria for all characters
                sLog.outError("AchievementMgr::LoadFromDB Nonexistent achievement criteria %u data removed from table `character_achievement_progress`.", id);
                CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE criteria = %u", id);
                continue;
            }

            CriteriaProgress& progress = m_criteriaProgress[id];
            progress.counter = counter;
            progress.date    = date;
            progress.changed = false;
            progress.timedCriteriaFailed = false;

            AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);

            if (!achievement)
            {
                // we will remove nonexistent referred achievement for all characters
                sLog.outError("AchievementMgr::LoadFromDB Nonexistent achievement criteria %u (referred achievement %u) data removed from table `character_achievement_progress`.",id, criteria->referredAchievement);
                CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE criteria = %u",id);
                continue;
            }

            // Checked in LoadAchievementCriteriaList

            // A failed achievement will be removed on next tick - TODO: Possible that timer 2 is reseted
            if (criteria->timeLimit)
            {
                // Add not-completed achievements to time map
                if (!IsCompletedCriteria(criteria, achievement))
                {
                    time_t failTime = time_t(progress.date + criteria->timeLimit);
                    m_criteriaFailTimes[criteria->ID] = failTime;
                    // A failed Achievement - will be removed by DoFailedTimedAchievementCriterias on next tick for player
                    if (failTime <= time(NULL))
                        progress.timedCriteriaFailed = true;
                }
            }

            // check integrity with max allowed counter value
            if (uint32 maxcounter = GetCriteriaProgressMaxCounter(criteria, achievement))
            {
                if (progress.counter > maxcounter)
                {
                    progress.counter = maxcounter;
                    progress.changed = true;
                }
            }
        }
        while (criteriaResult->NextRow());

        delete criteriaResult;
    }
}

template<>
void AchievementMgr<Guild>::LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].GetUInt16();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            if (!(achievement->flags & ACHIEVEMENT_FLAG_GUILD))
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievementid];
            ca.date = time_t(fields[1].GetUInt32());
            Tokens tokens(fields[2].GetCppString(), ' ');
            for (uint32 i = 0; i < tokens.size(); ++i)
                ca.guids.insert(ObjectGuid(HIGHGUID_PLAYER, 0, uint32(atol(tokens[i]))));

            ca.changed = false;

            m_achievementPoints += achievement->points;

        } while (achievementResult->NextRow());

        delete achievementResult;
    }

    if (criteriaResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id      = fields[0].GetUInt16();
            uint32 counter = fields[1].GetUInt32();
            time_t date    = time_t(fields[2].GetUInt32());
            uint32 guidLow = fields[3].GetUInt32();

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // we will remove not existed criteria for all guilds
                sLog.outErrorDb("Non-existing achievement criteria %u data removed from table `guild_achievement_progress`.", id);

                CharacterDatabase.PExecute("DELETE FROM guild_achievement_progress WHERE criteria = %u", id);
                continue;
            }

            CriteriaProgress& progress = m_criteriaProgress[id];
            progress.counter = counter;
            progress.date    = date;
            progress.CompletedGUID = ObjectGuid(HIGHGUID_PLAYER, 0, guidLow);
            progress.changed = false;
            progress.timedCriteriaFailed = false;

            AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);

            // A failed achievement will be removed on next tick - TODO: Possible that timer 2 is reseted
            if (criteria->timeLimit)
            {
                // Add not-completed achievements to time map
                if (!IsCompletedCriteria(criteria, achievement))
                {
                    time_t failTime = time_t(progress.date + criteria->timeLimit);
                    m_criteriaFailTimes[criteria->ID] = failTime;
                    // A failed Achievement - will be removed by DoFailedTimedAchievementCriterias on next tick for player
                    if (failTime <= time(NULL))
                        progress.timedCriteriaFailed = true;
                }
            }

            // check integrity with max allowed counter value
            if (uint32 maxcounter = GetCriteriaProgressMaxCounter(criteria, achievement))
            {
                if (progress.counter > maxcounter)
                {
                    progress.counter = maxcounter;
                    progress.changed = true;
                }
            }

        } while (criteriaResult->NextRow());

        delete criteriaResult;
    }
}

template<class T>
void AchievementMgr<T>::SendAchievementEarned(AchievementEntry const* achievement)
{
    if (GetOwner()->GetSession()->PlayerLoading())
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::SendAchievementEarned(%u)", achievement->ID);

    if (Guild* guild = sGuildMgr.GetGuildById(GetOwner()->GetGuildId()))
    {
        MaNGOS::AchievementChatBuilder say_builder(*GetOwner(), CHAT_MSG_GUILD_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED,achievement->ID);
        MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> say_do(say_builder);
        guild->BroadcastWorker(say_do,GetOwner());
    }

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetOwner()->GetName())+1+8+4+4);
        data << GetOwner()->GetName();
        data << GetOwner()->GetObjectGuid();
        data << uint32(achievement->ID);
        data << uint32(0);                                  // 1=link supplied string as player name, 0=display plain string
        sWorld.SendGlobalMessage(&data);
    }
    // if player is in world he can tell his friends about new achievement
    else if (GetOwner()->IsInWorld())
    {
        MaNGOS::AchievementChatBuilder say_builder(*GetOwner(), CHAT_MSG_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED,achievement->ID);
        MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> say_do(say_builder);
        MaNGOS::CameraDistWorker<MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> > say_worker(GetOwner(),sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY),say_do);

        Cell::VisitWorldObjects(GetOwner(), say_worker, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, GetOwner()->GetPackGUID().size() + 4 + 4);
    data << GetOwner()->GetPackGUID();
    data << uint32(achievement->ID);
    data.AppendPackedTime(time(NULL));
    data << uint32(0);
    GetOwner()->SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY), true);

    //megai2: moved from compl to real send messages due to FLOODING 
}

template<>
void AchievementMgr<Guild>::SendAchievementEarned(AchievementEntry const* achievement)
{
    ObjectGuid guid = GetOwner()->GetObjectGuid();

    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_EARNED, 8+4+8);
    data.WriteGuidMask<3, 1, 0, 7, 4, 6, 2, 5>(guid);

    data.WriteGuidBytes<2>(guid);

    data << uint32(secsToTimeBitFields(time(NULL)));

    data.WriteGuidBytes<0, 4, 1, 3>(guid);
    data << uint32(achievement->ID);
    data.WriteGuidBytes<7, 5, 6>(guid);

    SendPacket(&data);
}

template <class T>
void AchievementMgr<T>::SendCriteriaUpdate(uint32 id, CriteriaProgress const* /*progress*/)
{
}

template <>
void AchievementMgr<Player>::SendCriteriaUpdate(uint32 id, CriteriaProgress const* progress)
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8+4+8);
    data << uint32(id);

    time_t now = time(NULL);
    // the counter is packed like a packed Guid
    data.appendPackGUID(progress->counter);

    data << GetOwner()->GetPackGUID();
    data << uint32(progress->timedCriteriaFailed ? 1 : 0);
    data.AppendPackedTime(now);
    data << uint32(now - progress->date);                   // timer 1
    data << uint32(now - progress->date);                   // timer 2
    SendPacket(&data);
}

template <>
void AchievementMgr<Guild>::SendCriteriaUpdate(uint32 id, CriteriaProgress const* progress)
{
    // will send response to criteria progress request
    WorldPacket data(SMSG_GUILD_CRITERIA_DATA, 3 + 1 + 1 + 8 + 8 + 4 + 4 + 4 + 4 + 4);

    ObjectGuid criteriaProgress = ObjectGuid(uint64(progress->counter));
    ObjectGuid criteriaGuid = progress->CompletedGUID;

    data.WriteBits(1, 21);

    data.WriteGuidMask<4, 1>(criteriaProgress);
    data.WriteGuidMask<2>(criteriaGuid);
    data.WriteGuidMask<3>(criteriaProgress);
    data.WriteGuidMask<1>(criteriaGuid);
    data.WriteGuidMask<5, 0>(criteriaProgress);
    data.WriteGuidMask<3>(criteriaGuid);
    data.WriteGuidMask<2>(criteriaProgress);
    data.WriteGuidMask<7, 5, 0>(criteriaGuid);
    data.WriteGuidMask<6>(criteriaProgress);
    data.WriteGuidMask<6>(criteriaGuid);
    data.WriteGuidMask<7>(criteriaProgress);
    data.WriteGuidMask<4>(criteriaGuid);

    data.WriteGuidBytes<5>(criteriaGuid);
    data << uint32(progress->date);      // unknown date
    data.WriteGuidBytes<3, 7>(criteriaProgress);
    data << uint32(progress->date);      // unknown date
    data.WriteGuidBytes<6>(criteriaProgress);
    data.WriteGuidBytes<4, 1>(criteriaGuid);
    data.WriteGuidBytes<4>(criteriaProgress);
    data.WriteGuidBytes<3>(criteriaGuid);
    data.WriteGuidBytes<0>(criteriaProgress);
    data.WriteGuidBytes<2>(criteriaGuid);
    data.WriteGuidBytes<1>(criteriaProgress);
    data.WriteGuidBytes<6>(criteriaGuid);
    data << uint32(progress->date);      // last update time (not packed!)
    data << uint32(id);
    data.WriteGuidBytes<5>(criteriaProgress);
    data << uint32(0);
    data.WriteGuidBytes<7>(criteriaGuid);
    data.WriteGuidBytes<2>(criteriaProgress);
    data.WriteGuidBytes<0>(criteriaGuid);

    SendPacket(&data);
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
template<class T>
void AchievementMgr<T>::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (uint32 i = 0; i < ACHIEVEMENT_CRITERIA_TYPE_TOTAL; ++i)
        referencePlayer->UpdateAchievementCriteria(AchievementCriteriaTypes(i), 0, 0, NULL, 0);
}

template<>
void AchievementMgr<Guild>::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (uint32 i = 0; i < ACHIEVEMENT_CRITERIA_TYPE_TOTAL; ++i)
        UpdateAchievementCriteria(AchievementCriteriaTypes(i), 0, 0, NULL, 0, referencePlayer);
}

static const uint32 achievIdByArenaSlot[MAX_ARENA_SLOT] = { 1057, 1107, 1108 };
static const uint32 achievIdForDungeon[][4] =
{
    // ach_cr_id,is_dungeon,is_raid,is_heroic_dungeon
    { 321,       true,      true,   true  },                // Total raid and dungeon deaths
    //323                                                   // Total deaths to Lich King 10-player raid bosses
    //324                                                   // Total deaths to Lich King 25-player raid bosses
    { 916,       false,     true,   false },                // Total deaths in 25-player raids
    { 917,       false,     true,   false },                // Total deaths in 10-player raids
    { 918,       true,      false,  false },                // Total deaths in 5-player dungeons
    { 2219,      false,     false,  true  },                // Total deaths in 5-player heroic dungeons
    { 0,         false,     false,  false }
};

static const uint32 achievIdByClass[MAX_CLASSES] = { 0, 459, 465 , 462, 458, 464, 461, 467, 460, 463, 0, 466 };
static const uint32 achievIdByRace[MAX_RACES]    = { 0, 1408, 1410, 1407, 1409, 1413, 1411, 1404, 1412, 0, 1405, 1406 };

/**
 * this function will be called whenever the user might have done a timed-criteria relevant action, or by scripting side?
 */
template <class T>
void AchievementMgr<T>::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
}

template <>
void AchievementMgr<Player>::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::StartTimedAchievementCriteria(%u, %u)", type, timedRequirementId);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && GetOwner()->GetSession()->GetSecurity() > SEC_MODERATOR)
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type);
    for (AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const* achievementCriteria = (*i);

        // only apply to specific timedRequirementId related criteria
        if (achievementCriteria->timedCriteriaMiscId != timedRequirementId)
            continue;

        if (!achievementCriteria->IsExplicitlyStartedTimedCriteria())
            continue;

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        if ((achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_HORDE    && GetOwner()->GetTeam() != HORDE) ||
            (achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_ALLIANCE && GetOwner()->GetTeam() != ALLIANCE))
            continue;

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria, achievement))
            continue;

        // Only the Quest-Complete Timed Achievements need the groupcheck, so this check is only needed here
        if (achievementCriteria->requiredType == ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST && GetOwner()->GetGroup())
            continue;

        // do not start already failed timers
        if (startTime && time_t(startTime + achievementCriteria->timeLimit) < time(NULL))
            continue;

        CriteriaProgress* progress = NULL;

        CriteriaProgressMap::iterator iter = m_criteriaProgress.find(achievementCriteria->ID);
        if (iter == m_criteriaProgress.end())
            progress = &m_criteriaProgress[achievementCriteria->ID];
        else
            progress = &iter->second;

        progress->changed = true;
        progress->counter = 0;

        // Start with given startTime or now
        progress->date = startTime ? startTime : time(NULL);
        progress->timedCriteriaFailed = false;

        // Add to timer map
        m_criteriaFailTimes[achievementCriteria->ID] = time_t(progress->date + achievementCriteria->timeLimit);

        SendCriteriaUpdate(achievementCriteria->ID, progress);
    }
}

/**
 * this function will be called whenever there could be a timed achievement criteria failed because of ellapsed time
 */
template<class T>
void AchievementMgr<T>::DoFailedTimedAchievementCriterias()
{
    if (m_criteriaFailTimes.empty())
        return;

    time_t now = time(NULL);
    for (AchievementCriteriaFailTimeMap::iterator iter = m_criteriaFailTimes.begin(); iter != m_criteriaFailTimes.end();)
    {
        if (iter->second > now)
        {
            ++iter;
            continue;
        }

        // Possible failed achievement criteria found
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(iter->first);
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        // Send Fail for failed criterias
        if (!IsCompletedCriteria(criteria, achievement))
        {
            DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::DoFailedTimedAchievementCriterias for criteria %u", criteria->ID);

            CriteriaProgressMap::iterator pro_iter = m_criteriaProgress.find(criteria->ID);
            MANGOS_ASSERT(pro_iter != m_criteriaProgress.end());

            CriteriaProgress* progress = &pro_iter->second;

            // Set to failed, and send to client
            progress->timedCriteriaFailed = true;
            SendCriteriaUpdate(criteria->ID, progress);

            // Remove failed progress
            m_criteriaProgress.erase(pro_iter);
        }

        m_criteriaFailTimes.erase(iter++);
    }
}

/**
 * this function will be called whenever the user might have done a criteria relevant action
 */
template <class T>
void AchievementMgr<T>::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, Unit *unit, uint32 time, Player* referencePlayer /*= NULL*/)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::UpdateAchievementCriteria(%u, %u, %u, %u)", type, miscvalue1, miscvalue2, time);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && referencePlayer->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    // Lua_GetGuildLevelEnabled() is checked in achievement UI to display guild tab
    if (IsGuild<T>() && !sWorld.getConfig(CONFIG_BOOL_GUILD_LEVELING_ENABLED))
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type, IsGuild<T>());
    for (AchievementCriteriaEntryList::const_iterator itr = achievementCriteriaList.begin(); itr != achievementCriteriaList.end(); ++itr)
    {
        AchievementCriteriaEntry const* achievementCriteria = *itr;

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // *achievement Checked in LoadAchievementCriteriaList

        if ((achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_HORDE    && referencePlayer->GetTeam() != HORDE) ||
            (achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
            continue;

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria, achievement))
            continue;

        if (!AdditionalRequirementsSatisfied(achievementCriteria, miscvalue1, miscvalue2, unit, referencePlayer))
            continue;

        // init values, real set in switch
        uint32 change = 0;
        ProgressType progressType = PROGRESS_HIGHEST;       // when need it will replaced by PROGRESS_ACCUMULATE

        switch (type)
        {
                // std. case: increment at 1
            case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
            case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
            case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:    /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
            case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
            case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
                // std case: increment at miscvalue1
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:/* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
                // std case: high value at miscvalue1
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD: /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_HIGHEST;
                break;

                // specialized cases

            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->win_bg.bgMapID != referencePlayer->GetMapId())
                    continue;

                BattleGround* bg = referencePlayer->GetBattleGround();
                if (!bg)
                    continue;

                if (achievementCriteria->win_bg.additionalRequirement1_type || achievementCriteria->win_bg.additionalRequirement2_type)
                {
                    // some hardcoded requirements
                    switch(achievementCriteria->referredAchievement)
                    {
                        case 1164:             // AV, own both mines (horde)
                        case 225:              // AV, own both mines (alliance)
                        {

                            TeamIndex team = GetTeamIndex(referencePlayer->GetTeam());
                            if(!((BattleGroundAV*)bg)->IsMineOwnedBy(BG_AV_NORTH_MINE,team) || !((BattleGroundAV*)bg)->IsMineOwnedBy(BG_AV_SOUTH_MINE,team))
                                continue;
                            break;
                        }
                        case 220:               // AV, win while your towers and captain are ok, enemy towers destroyed (alliance)
                        case 873:               // AV, win while your towers and captain are ok, enemy towers destroyed (horde)
                        {
                            if (bg->GetTypeID(true) != BATTLEGROUND_AV)
                                continue;

                            TeamIndex team = GetTeamIndex(referencePlayer->GetTeam());
                            if(!((BattleGroundAV*)bg)->hasAllTowers(team))
                                continue;
                            break;
                        }
                        case 3851:              // IoC, win while controlling each 5 nodes (alliance)
                        case 4177:              // IoC, win while controlling each 5 nodes (horde)
                        {
                            if (bg->GetTypeID(true) != BATTLEGROUND_IC)
                                continue;

                            TeamIndex team = GetTeamIndex(referencePlayer->GetTeam());
                            if(!((BattleGroundIC*)bg)->hasAllNodes(team))
                                continue;
                            break;
                        }
                        default:
                        {
                            // those requirements couldn't be found in the dbc
                            AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                            if (!data || !data->Meets(referencePlayer,unit))
                                continue;
                            break;
                        }
                    }
                }
                // some hardcoded requirements
                else
                {

                    switch (achievementCriteria->referredAchievement)
                    {
                        case 161:                           // AB, Overcome a 500 resource disadvantage
                        {
                            if (bg->GetTypeID(true) != BATTLEGROUND_AB)
                                continue;
                            if (!((BattleGroundAB*)bg)->IsTeamScores500Disadvantage(referencePlayer->GetTeam()))
                                continue;
                            break;
                        }
                        case 156:                           // AB, win while controlling all 5 flags (all nodes)
                        case 784:                           // EY, win while holding 4 bases (all nodes)
                        {
                            if (!bg->IsAllNodesControlledByTeam(referencePlayer->GetTeam()))
                                continue;
                            break;
                        }
                        case 1762:                          // SA, win without losing any siege vehicles
                        case 2192:                          // SA, win without losing any siege vehicles
                        {
                            if (bg->GetTypeID(true) != BATTLEGROUND_SA)
                                continue;

                            if (((BattleGroundSA*)bg)->isDemolisherDestroyed[GetTeamIndex(referencePlayer->GetTeam())])
                                continue;
                            break;
                        }
                        case 3846:                          // IoC, win while controlling each 5 nodes (alliance)
                        case 4176:                          // IoC, win while controlling each 5 nodes (horde)
                        {
                            if (bg->GetTypeID(true) != BATTLEGROUND_IC)
                                continue;

                            TeamIndex team = GetTeamIndex(referencePlayer->GetTeam());
                            if(!((BattleGroundIC*)bg)->hasAllResNodes(team))
                                continue;
                            break;
                        }
                    }
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->kill_creature.creatureID != miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (!data || !data->Meets(referencePlayer,unit))
                    continue;

                // For custom cases
                uint32 changeValue = miscvalue2;

                // some hardcoded criterias
                switch (achievementCriteria->referredAchievement)
                {
                    case 2189:                  // Artillery Expert (SotA)
                    case 1763:                  // Artillery Veteran (SotA)
                    {
                        //if not at bg
                        BattleGround* bg = referencePlayer->GetBattleGround();
                        if (!bg)
                            continue;
                        // if not on vehicle
                        if (!referencePlayer->hasUnitState(UNIT_STAT_ON_VEHICLE))
                            continue;
                        break;
                    }
                    case 1871:                  // Experienced Drake Rider (The Oculus)
                    {
                        VehicleKitPtr vehicleKit = referencePlayer->GetVehicle();
                        if(!vehicleKit)
                            continue;
                        uint32 DragonEntry = vehicleKit->GetBase()->GetEntry();

                        if (!(achievementCriteria->ID == 7177 && DragonEntry == 27756) &&   // Ruby Dragon
                            !(achievementCriteria->ID == 7178 && DragonEntry == 27692) &&   // Emerald Dragon
                            !(achievementCriteria->ID == 7179 && DragonEntry == 27755))     // Amber Dragon
                            continue;
                        break;
                    }
                    case 4539:                  // Once Bitten, Twice Shy(10) (ICC Lana'thel)
                    {
                        if (changeValue)
                            continue;

                        if (referencePlayer->GetMap()->GetDifficulty() != RAID_DIFFICULTY_10MAN_HEROIC &&
                            referencePlayer->GetMap()->GetDifficulty() != RAID_DIFFICULTY_10MAN_NORMAL)
                            continue;

                        if (!(achievementCriteria->ID == 12780 && !referencePlayer->HasAura(70871)) &&
                            !(achievementCriteria->ID == 13011 && referencePlayer->HasAura(70871)))
                            continue;

                        changeValue = 1;
                        break;
                    }
                    case 4618:                  // Once Bitten, Twice Shy(25) (ICC Lana'thel)
                    {
                        if (changeValue)
                            continue;

                        if (referencePlayer->GetMap()->GetDifficulty() != RAID_DIFFICULTY_25MAN_HEROIC &&
                            referencePlayer->GetMap()->GetDifficulty() != RAID_DIFFICULTY_25MAN_NORMAL)
                            continue;

                        if (!(achievementCriteria->ID == 13012 && !referencePlayer->HasAura(70871)) &&
                            !(achievementCriteria->ID == 13013 && referencePlayer->HasAura(70871)))
                            continue;

                        changeValue = 1;
                        break;
                    }
                }

                change = changeValue;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            {
                bool ok = true;

                // skip wrong class achievements
                for (uint8 i = 1; i < MAX_CLASSES; ++i)
                {
                    if (achievIdByClass[i] == achievement->ID && i != referencePlayer->getClass())
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                // skip wrong race achievements
                for (uint8 i = 1; i < MAX_RACES; ++i)
                {
                    if (achievIdByRace[i] == achievement->ID && i != referencePlayer->getRace())
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (data && !data->Meets(referencePlayer, referencePlayer))
                    continue;

                change = referencePlayer->getLevel();
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            {
                // update at loading or specific skill update
                if (miscvalue1 && miscvalue1 != achievementCriteria->reach_skill_level.skillID)
                    continue;
                change = referencePlayer->GetBaseSkillValue(achievementCriteria->reach_skill_level.skillID);
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            {
                // update at loading or specific skill update
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_skill_level.skillID)
                    continue;
                change = referencePlayer->GetPureMaxSkillValue(achievementCriteria->learn_skill_level.skillID);
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            {
                if (m_completedAchievements.find(achievementCriteria->complete_achievement.linkedAchievement) == m_completedAchievements.end())
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            {
                uint32 counter =0;
                for (QuestStatusMap::const_iterator itr = referencePlayer->GetQuestStatusMap().begin(); itr!=referencePlayer->GetQuestStatusMap().end(); ++itr)
                    if (itr->second.m_rewarded)
                        ++counter;
                change = counter;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            {
                // skip at login
                if(!miscvalue1)
                    continue;

                // Add one day of daily quests
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            {
                // speedup for non-login case
                if (miscvalue1 && miscvalue1 != achievementCriteria->complete_quests_in_zone.zoneID)
                    continue;

                uint32 counter =0;
                for (QuestStatusMap::const_iterator itr = referencePlayer->GetQuestStatusMap().begin(); itr!=referencePlayer->GetQuestStatusMap().end(); ++itr)
                {
                    Quest const* quest = sObjectMgr.GetQuestTemplate(itr->first);
                    if (itr->second.m_rewarded && quest->GetZoneOrSort() >= 0 && uint32(quest->GetZoneOrSort()) == achievementCriteria->complete_quests_in_zone.zoneID)
                        ++counter;
                }
                change = counter;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            {
                // skip at login
                if (!miscvalue1)
                    continue;

                // Daily Chores (Children's Week event)
                if (achievementCriteria->referredAchievement == 1789)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data)
                        continue;
                    if(!data->Meets(referencePlayer,unit))
                        continue;
                }

                change = 1;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY_EARNED:
            {
                if (!miscvalue1 || !miscvalue2 || miscvalue1 != achievementCriteria->currencyEarned.currencyId)
                    return;

                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(referencePlayer->GetMapId() != achievementCriteria->complete_battleground.mapID)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(referencePlayer->GetMapId() != achievementCriteria->death_at_map.mapID)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                // skip wrong arena achievements, if not achievIdByArenaSlot then normal total death counter
                bool notfit = false;
                for (int j = 0; j < MAX_ARENA_SLOT; ++j)
                {
                    if (achievIdByArenaSlot[j] == achievement->ID)
                    {
                        BattleGround* bg = referencePlayer->GetBattleGround();
                        if (!bg || !bg->isArena() || ArenaTeam::GetSlotByType(bg->GetArenaType()) != j)
                            notfit = true;

                        break;
                    }
                }
                if (notfit)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                Map const* map = referencePlayer->IsInWorld() ? referencePlayer->GetMap() : sMapMgr.FindMap(referencePlayer->GetMapId(), referencePlayer->GetInstanceId());
                if (!map || !map->IsDungeon())
                    continue;

                // search case
                bool found = false;
                for (int j = 0; achievIdForDungeon[j][0]; ++j)
                {
                    if (achievIdForDungeon[j][0] == achievement->ID)
                    {
                        if (map->IsRaid())
                        {
                            // if raid accepted (ignore difficulty)
                            if (!achievIdForDungeon[j][2])
                                break;                      // for
                        }
                        else if(referencePlayer->GetDungeonDifficulty()==DUNGEON_DIFFICULTY_NORMAL)
                        {
                            // dungeon in normal mode accepted
                            if (!achievIdForDungeon[j][1])
                                break;                      // for
                        }
                        else
                        {
                            // dungeon in heroic mode accepted
                            if (!achievIdForDungeon[j][3])
                                break;                      // for
                        }

                        found = true;
                        break;                              // for
                    }
                }
                if (!found)
                    continue;

                // FIXME: work only for instances where max==min for players
                if (map->GetMaxPlayers() != achievementCriteria->death_in_dungeon.manLimit)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;

            }
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->killed_by_creature.creatureEntry)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                // if team check required: must kill by opposition faction
                if(achievement->ID==318 && miscvalue2==uint32(referencePlayer->GetTeam()))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data || !data->Meets(referencePlayer,unit))
                    continue;

                // miscvalue1 is the ingame fallheight*100 as stored in dbc
                change = miscvalue1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (miscvalue2 != achievementCriteria->death_from.type)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                // if miscvalues != 0, it contains the questID.
                if (miscvalue1)
                {
                    if (miscvalue1 != achievementCriteria->complete_quest.questID)
                        continue;
                }
                else
                {
                    // login case.
                    if(!referencePlayer->GetQuestRewardStatus(achievementCriteria->complete_quest.questID))
                        continue;
                }

                // exist many achievements with this criteria, use at this moment hardcoded check to skip simple case
                switch (achievement->ID)
                {
                    case 31:
                        // case 1275: // these timed achievements have to be "started" on Quest Accept
                        // case 1276:
                        // case 1277:
                    case 1282:
                    case 1789:
                    {
                        // those requirements couldn't be found in the dbc
                        AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                        if(!data || !data->Meets(referencePlayer,unit))
                            continue;
                        break;
                    }
                    default:
                        break;
                }

                // As the groupFlag had wrong meaning, only the Quest-Complete Timed Achievements need the groupcheck, so this check is only needed here
                if (achievementCriteria->timeLimit > 0 && referencePlayer->GetGroup())
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->be_spell_target.spellID)
                    continue;

                // for special conditions
                switch (achievementCriteria->referredAchievement)
                {
                    case 1757:          // Defense of the Ancients(alliance)
                    case 2200:          // Defense of the Ancients(horde)
                    {
                        Player* plr = referencePlayer;
                        BattleGround* bg = plr->GetBattleGround();
                        if (!bg || bg->GetTypeID(true) != BATTLEGROUND_SA)
                            continue;

                        // If hasnt all walls.
                        if(!((BattleGroundSA*)bg)->winSAwithAllWalls(referencePlayer->GetTeam()))
                            continue;
                        break;
                    }
                    /*case 7666:      // Within Our Grasp
                    {
                        BattleFieldWG* opvp = (BattleFieldWG*)sOutdoorPvPMgr.GetScript(ZONE_ID_WINTERGRASP);
                        if (!opvp || !opvp->CriteriaMeets(achievementCriteria->ID, referencePlayer))
                            continue;
                        break;
                    }*/
                }

                // for special conditions
                switch(achievementCriteria->referredAchievement)
                {
                    case 1761:          // The Dapper Sapper (SotA)
                    case 2193:          // Explosives Expert (SotA)
                    {
                        // If not in SotA
                        BattleGround * bg = referencePlayer->GetBattleGround();
                        if(!bg || bg->GetTypeID(true) != BATTLEGROUND_SA)
                            continue;
                        break;
                    }
                    case 3850:          // Mowed Down (IoC) (both)
                    {
                        //if not at bg
                        BattleGround* bg = referencePlayer->GetBattleGround();
                        if (!bg)
                            continue;
                        if (bg->GetTypeID(true) != BATTLEGROUND_IC)
                            continue;
                        if(!((referencePlayer->GetVehicle()) && (referencePlayer->GetVehicle()->GetBase()->GetEntry() == 34944)))
                            continue;
                        break;
                    }
                    case 1310:          // Storm the Beach (SotA)
                    {
                        // If not in SotA
                        BattleGround * bg = referencePlayer->GetBattleGround();
                        if(!bg || bg->GetTypeID(true) != BATTLEGROUND_SA)
                            continue;
                        break;
                    }
                    default:
                    {
                        // those requirements couldn't be found in the dbc
                        AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);

                        if (!data)
                            continue;
                        if (!data->Meets(referencePlayer,unit))
                            continue;
                        break;
                    }
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->cast_spell.spellID)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (!data)
                    continue;

                if(!data->Meets(referencePlayer,unit))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_spell.spellID)
                    continue;

                if(!referencePlayer->HasSpell(achievementCriteria->learn_spell.spellID))
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            {
                // miscvalue1=loot_type (note: 0 = LOOT_CORPSE and then it ignored)
                // miscvalue2=count of item loot
                if (!miscvalue1 || !miscvalue2)
                    continue;
                if (miscvalue1 != achievementCriteria->loot_type.lootType)
                    continue;

                // zone specific
                if (achievementCriteria->loot_type.lootTypeCount == 1)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(referencePlayer,unit))
                        continue;
                }

                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            {
                // speedup for non-login case
                if(miscvalue1 && achievementCriteria->own_item.itemID != miscvalue1)
                    continue;

                // check item count
                if(!miscvalue2)
                {
                    continue;

                    change = referencePlayer->GetItemCount(achievementCriteria->own_item.itemID, true);
                    progressType = PROGRESS_HIGHEST;
                }
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            {
                //miscvalue1 = mapID
                //miscvalue2 = ArenaType
                if(!miscvalue1)
                    continue;
                if(achievementCriteria->win_arena.mapID != miscvalue1)
                    continue;

                if (achievementCriteria->win_arena.mapID != referencePlayer->GetMapId())
                    continue;

                // Matches statistic for ArenaType
                switch(achievementCriteria->referredAchievement)
                {
                    case 363:
                        if(miscvalue2!=ARENA_TYPE_5v5)
                            continue;
                        break;
                    case 365:
                        if(miscvalue2!=ARENA_TYPE_3v3)
                            continue;
                        break;
                    case 367:
                        if(miscvalue2!=ARENA_TYPE_2v2)
                            continue;
                        break;
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                // miscvalue1 contains the personal rating
                if (!miscvalue1)                            // no update at login
                    continue;

                // additional requirements
                if (achievementCriteria->win_rated_arena.flag == ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(referencePlayer,unit,miscvalue1))
                    {
                        // reset the progress as we have a win without the requirement.
                        SetCriteriaProgress(achievementCriteria, achievement, 0, referencePlayer, PROGRESS_SET);
                        continue;
                    }
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->use_item.itemID != miscvalue1)
                    continue;
                // possible additional requirements
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (data && !data->Meets(referencePlayer, unit, miscvalue1))
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                // You _have_ to loot that item, just owning it when logging in does _not_ count!
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->own_item.itemID)
                    continue;
                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            {
                WorldMapOverlayEntry const* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(achievementCriteria->explore_area.areaReference);
                if (!worldOverlayEntry)
                    break;

                bool matchFound = false;
                for (int j = 0; j < MAX_WORLD_MAP_OVERLAY_AREA_IDX; ++j)
                {
                    uint32 area_id = worldOverlayEntry->areatableID[j];
                    if (!area_id)                           // array have 0 only in empty tail
                        break;

                    int32 exploreFlag = GetAreaFlagByAreaID(area_id);
                    if (exploreFlag < 0)
                        continue;

                    uint32 playerIndexOffset = uint32(exploreFlag) / 32;
                    uint32 mask = 1 << (uint32(exploreFlag) % 32);

                    if(referencePlayer->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + playerIndexOffset) & mask)
                    {
                        matchFound = true;
                        break;
                    }
                }

                if (!matchFound)
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                change = referencePlayer->GetBankBagSlotCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            {
                // skip faction check only at loading
                if (miscvalue1 && miscvalue1 != achievementCriteria->gain_reputation.factionID)
                    continue;

                int32 reputation = referencePlayer->GetReputationMgr().GetReputation(achievementCriteria->gain_reputation.factionID);
                if (reputation <= 0)
                    continue;

                change = reputation;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            {
                change = referencePlayer->GetReputationMgr().GetExaltedFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            {
                // skip for login case
                if (!miscvalue1)
                    continue;
                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            {
                // miscvalue1 = equip_slot+1 (for avoid use 0)
                if (!miscvalue1)
                    continue;
                uint32 item_slot = miscvalue1 - 1;
                if (item_slot != achievementCriteria->equip_epic_item.itemSlot)
                    continue;
                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data || !data->Meets(referencePlayer,unit,item_slot))
                    continue;
                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            {
                // miscvalue1 = itemid
                // miscvalue2 = diced value
                if (!miscvalue1)
                    continue;
                if (miscvalue2 != achievementCriteria->roll_greed_on_loot.rollValue)
                    continue;
                ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(miscvalue1);

                uint32 requiredItemLevel = 0;
                if (achievementCriteria->ID == 2412 || achievementCriteria->ID == 2358)
                    requiredItemLevel = 185;

                if (!pProto || pProto->ItemLevel < requiredItemLevel)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            {
                // miscvalue1 = emote
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->do_emote.emoteID)
                    continue;
                if (achievementCriteria->do_emote.count)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(referencePlayer,unit))
                        continue;
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            {
                if (!miscvalue1)
                    continue;

                if (achievementCriteria->healing_done.flag == ACHIEVEMENT_CRITERIA_CONDITION_MAP)
                {
                    if(referencePlayer->GetMapId() != achievementCriteria->healing_done.mapid)
                        continue;

                    // map specific case (BG in fact) expected player targeted damage/heal
                    if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                        continue;
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                // miscvalue1 = item_id
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->equip_item.itemID)
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                // miscvalue1 = go entry
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->use_gameobject.goEntry)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                BattleGround* bg = referencePlayer->GetBattleGround();
                switch(achievementCriteria->referredAchievement)
                {
                    case 207:                       // Save The Day
                    {
                        if (!bg || !unit)
                            continue;

                        if (bg->GetTypeID(true) != BATTLEGROUND_WS)
                            continue;

                        switch(referencePlayer->GetTeam())
                        {
                            case ALLIANCE:
                                if (!(((BattleGroundWS*)bg)->GetFlagState(HORDE) == BG_WS_FLAG_STATE_ON_BASE) || !unit->HasAura(23335) || referencePlayer->GetAreaId() != 4572)
                                    continue;
                                break;
                            case HORDE:
                                if (!(((BattleGroundWS*)bg)->GetFlagState(ALLIANCE) == BG_WS_FLAG_STATE_ON_BASE) || !unit->HasAura(23333) || referencePlayer->GetAreaId() != 4571)
                                    continue;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    case 1259:                      // Not So Fast (WS)
                        if (!bg || !unit)
                            continue;
                        if (bg->GetTypeID(true) != BATTLEGROUND_WS)
                            continue;
                        if(!unit->HasAura(23451))
                            continue;
                        break;
                    case 1764:                      // Drop It (SotA)
                    case 2190:                      // Drop It Now (SotA)
                        if (!bg || !unit)
                            continue;
                        if (bg->GetTypeID(true) != BATTLEGROUND_SA)
                            continue;
                        if(!unit->HasAura(52418))
                            continue;
                        break;
                    case 1109:                      // 5v5 Honorable Kills
                        if(!bg)
                            continue;
                        if(!bg->isArena())
                            continue;
                        if(bg->GetArenaType() != ARENA_TYPE_5v5)
                            continue;
                        break;
                    case 1110:                      // 3v3 Honorable Kills
                        if(!bg)
                            continue;
                        if(!bg->isArena())
                            continue;
                        if(bg->GetArenaType() != ARENA_TYPE_3v3)
                            continue;
                        break;
                    case 1111:                      // 2v2 Honorable Kills
                        if(!bg)
                            continue;
                        if(!bg->isArena())
                            continue;
                        if(bg->GetArenaType() != ARENA_TYPE_2v2)
                            continue;
                        break;
                    /*case 1723:                      // Vehicular Gnomeslaughter
                    {
                        if (!referencePlayer->GetVehicle() || referencePlayer->GetVehicle()->GetBase()->GetTypeId() != TYPEID_UNIT)
                            continue;

                        BattleFieldWG* opvp = (BattleFieldWG*)sOutdoorPvPMgr.GetScript(ZONE_ID_WINTERGRASP);
                        if (!opvp || opvp->GetState() != BF_STATE_IN_PROGRESS)
                            continue;

                        if (referencePlayer->GetZoneId() != 4197)
                            continue;

                        uint32 vehicleEntry = referencePlayer->GetVehicle()->GetBase()->GetEntry();
                        switch(achievementCriteria->ID)
                        {
                            case 7704:      // Vehicle
                                if (vehicleEntry != 27881 && vehicleEntry != 28094 && vehicleEntry != 28312 &&
                                    vehicleEntry != 32627 && vehicleEntry != 28319 && vehicleEntry != 32629)
                                    continue;
                                break;
                            case 7705:      // Cannon
                                if (vehicleEntry != 28366)
                                    continue;
                                break;
                            case 7706:      // Shredder
                            case 7707:      // Fighter
                            case 7708:      // Bomber
                                continue;
                            default:
                                continue;
                        }
                        break;
                    }*/
                    /*case 1751:                      // Didn't Stand a Chance
                    {
                        if (!referencePlayer->GetVehicle() || referencePlayer->GetVehicle()->GetBase()->GetTypeId() != TYPEID_UNIT)
                            continue;

                        if (referencePlayer->GetVehicle()->GetBase()->GetEntry() != 28366)
                            continue;

                        BattleFieldWG* opvp = (BattleFieldWG*)sOutdoorPvPMgr.GetScript(ZONE_ID_WINTERGRASP);
                        if (!opvp || opvp->GetState() != BF_STATE_IN_PROGRESS)
                            continue;

                        if (referencePlayer->GetZoneId() != 4197)
                            continue;

                        if (!unit || !unit->IsMounted())
                            continue;

                        break;
                    }*/
                    case 5220:                      // I'm in the Black Lodge
                    {
                        if (!bg || !unit)
                            continue;

                        if (bg->GetTypeID(true) != BATTLEGROUND_TP)
                            continue;

                        switch(referencePlayer->GetTeam())
                        {
                            case ALLIANCE:
                                if (!(((BattleGroundWS*)bg)->GetFlagState(HORDE) == BG_WS_FLAG_STATE_ON_BASE) || !unit->HasAura(23335) || referencePlayer->GetAreaId() != 4572)
                                    continue;
                                break;
                            case HORDE:
                                if (!(((BattleGroundWS*)bg)->GetFlagState(ALLIANCE) == BG_WS_FLAG_STATE_ON_BASE) || !unit->HasAura(23333) || referencePlayer->GetAreaId() != 4571)
                                    continue;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                    {
                        // those requirements couldn't be found in the dbc
                        AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                        if(!data)
                            continue;
                        if(!data->Meets(referencePlayer,unit))
                            continue;
                        break;
                    }
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->fish_in_gameobject.goEntry)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            {
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_skillline_spell.skillLine)
                    continue;

                uint32 spellCount = 0;
                for (PlayerSpellMap::const_iterator spellIter = referencePlayer->GetSpellMap().begin();
                    spellIter != referencePlayer->GetSpellMap().end();
                    ++spellIter)
                {
                    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellIter->first);
                    for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                    {
                        if (skillIter->second->skillId == achievementCriteria->learn_skillline_spell.skillLine)
                            ++spellCount;
                    }
                }
                change = spellCount;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                if (achievementCriteria->win_duel.duelCount)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if (!data)
                        continue;

                    if (!data->Meets(referencePlayer,unit))
                        continue;
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
            {
                // miscvalue1 = CreatureType
                // miscvalue2 = Given XP
                if(!miscvalue1)
                    continue;

                switch(achievementCriteria->referredAchievement)
                {
                    case 107:               // Creature kills
                    {
                        // Creature types is not stored in dbc
                        if( (achievementCriteria->ID == 4948 && miscvalue1 == CREATURE_TYPE_BEAST) ||
                            (achievementCriteria->ID == 4949 && miscvalue1 == CREATURE_TYPE_DEMON) ||
                            (achievementCriteria->ID == 4950 && miscvalue1 == CREATURE_TYPE_DRAGONKIN) ||
                            (achievementCriteria->ID == 4951 && miscvalue1 == CREATURE_TYPE_ELEMENTAL) ||
                            (achievementCriteria->ID == 4952 && miscvalue1 == CREATURE_TYPE_GIANT) ||
                            (achievementCriteria->ID == 4953 && miscvalue1 == CREATURE_TYPE_HUMANOID) ||
                            (achievementCriteria->ID == 4954 && miscvalue1 == CREATURE_TYPE_MECHANICAL) ||
                            (achievementCriteria->ID == 4955 && miscvalue1 == CREATURE_TYPE_UNDEAD) ||
                            (achievementCriteria->ID == 4956 && miscvalue1 == CREATURE_TYPE_NOT_SPECIFIED) ||
                            (achievementCriteria->ID == 4957 && miscvalue1 == CREATURE_TYPE_TOTEM) )
                            break;
                        continue;
                    }
                    case 108:               // Critters kills
                    {
                        if(miscvalue1 != CREATURE_TYPE_CRITTER)
                            continue;
                        break;
                    }
                    case 1197:              // Total kills
                    {
                        break;
                    }
                    case 1198:              // Total kills that grant experience or honor
                    {
                        if(miscvalue2 < 1)
                            continue;
                        break;
                    }
                    default:
                    {
                        continue;
                    }
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                change = referencePlayer->GetReputationMgr().GetReveredFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                change = referencePlayer->GetReputationMgr().GetHonoredFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                change = referencePlayer->GetReputationMgr().GetVisibleFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                ItemPrototype const* proto = ObjectMgr::GetItemPrototype(miscvalue1);
                if (!proto || proto->Quality < ITEM_QUALITY_EPIC)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            {
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_skill_line.skillLine)
                    continue;

                uint32 spellCount = 0;
                for (PlayerSpellMap::const_iterator spellIter = referencePlayer->GetSpellMap().begin();
                    spellIter != referencePlayer->GetSpellMap().end();
                    ++spellIter)
                {
                    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellIter->first);
                    for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                        if (skillIter->second->skillId == achievementCriteria->learn_skill_line.skillLine)
                            ++spellCount;
                }
                change = spellCount;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                change = referencePlayer->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS);
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                if (!miscvalue1 || miscvalue1 != achievementCriteria->hk_class.classID)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                if (!miscvalue1 || miscvalue1 != achievementCriteria->hk_race.raceID)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                change = referencePlayer->GetMoney();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            {
                if(!miscvalue1)
                    continue;
                BattleGround* bg=referencePlayer->GetBattleGround();

                // those requirements couldn't be found in the dbc
                switch(achievementCriteria->referredAchievement)
                {
                    case 252:                   // With a Little Helper from My Friends (Event: Feast of Winter Veil)
                    {
                        Player* plr = referencePlayer;
                        if (!((plr->GetAura(26274,EFFECT_INDEX_0)) && (achievementCriteria->ID == 3829)) ||
                            !((plr->GetAura(26157,EFFECT_INDEX_0)) && (achievementCriteria->ID == 3826)) ||
                            !((plr->GetAura(26272,EFFECT_INDEX_0)) && (achievementCriteria->ID == 3827)) ||
                            !((plr->GetAura(26273,EFFECT_INDEX_0)) && (achievementCriteria->ID == 3828)) )
                            continue;
                        break;
                    }
                    case 381:                   // World Honorable Kills
                    {
                        // if at battleground
                        if(bg)
                            continue;
                        if( (achievementCriteria->ID == 5492 && referencePlayer->GetMapId() == 0) ||    // Eastern Kingdoms
                            (achievementCriteria->ID == 5493 && referencePlayer->GetMapId() == 1) ||    // Kalimdor
                            (achievementCriteria->ID == 5494 && referencePlayer->GetMapId() == 530) ||  // Outland
                            (achievementCriteria->ID == 5495 && referencePlayer->GetMapId() == 571) )   // Northrend
                            break;
                        continue;
                    }
                    case 382:                   // BattleGround Honorable Kills
                    {
                        if(!bg)
                            continue;
                        if( (achievementCriteria->ID == 5499 && referencePlayer->GetMapId() == 529) ||  // AB
                            (achievementCriteria->ID == 5500 && referencePlayer->GetMapId() == 30) ||   // AV
                            (achievementCriteria->ID == 5501 && referencePlayer->GetMapId() == 489) ||  // WS
                            (achievementCriteria->ID == 5502 && referencePlayer->GetMapId() == 566) ||  // EY
                            (achievementCriteria->ID == 5503 && referencePlayer->GetMapId() == 607) ||  // SA
                            (achievementCriteria->ID == 13260 && referencePlayer->GetMapId() == 628) )  // IC
                            break;
                        continue;
                    }
                    case 1112:                  // Eye of the Storm Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_EY))
                            continue;
                        break;
                    case 1113:                  // Alterac Valley Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_AV))
                            continue;
                        break;
                    case 1114:                  // Arathi Basin Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_AB))
                            continue;
                        break;
                    case 1115:                  // Warsong Gulch Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_WS))
                            continue;
                        break;
                    case 1261:                  // G.N.E.R.D. Rage
                        if(!referencePlayer->HasAura(48890))
                            continue;
                        break;
                    case 1486:                  // Strand of the Ancients Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_SA))
                            continue;
                        break;
                    case 3850:                  // Mowed Down (IoC)
                    {
                        //if not at bg
                        BattleGround* bg = referencePlayer->GetBattleGround();
                        if (!bg)
                            continue;
                        if (bg->GetTypeID(true) != BATTLEGROUND_IC)
                            continue;
                        if(!((referencePlayer->GetVehicle()) && (referencePlayer->GetVehicle()->GetBase()->GetEntry() == 34944)))
                            continue;
                        break;
                    }
                    case 3855:                  // Glaive Grave (IoC)
                    {
                        // if not at bg
                        BattleGround* bg = referencePlayer->GetBattleGround();
                        if (!bg)
                            continue;
                        if (bg->GetTypeID(true) != BATTLEGROUND_IC)
                           continue;
                        // without deaths
                        if(bg->GetPlayerScore(referencePlayer,SCORE_DEATHS)!=0)
                            continue;
                        // if not on vehicle
                        if(!referencePlayer->GetVehicle())
                            continue;
                        // if vehicle is Glaive Thrower
                        if(referencePlayer->GetVehicle()->GetBase()->GetEntry() != 34802)
                            continue;
                        break;
                    }
                    case 4779:                  // Isle of Conquests Honorable Kills
                        if(!bg || (bg->GetTypeID(true) != BATTLEGROUND_IC))
                            continue;
                        break;
                }
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            {
                // miscvalue1 = kills count
                // miscvalue2 = creature entry
                if (!miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);

                if(achievementCriteria->healing_done.flag != 0)
                {
                    if(referencePlayer->GetMapId() != achievementCriteria->healing_done.mapid)
                    {
                        continue;
                    }
                    else
                    {
                        if(data && !data->Meets(referencePlayer,unit))
                            continue;
                    }
                }
                else
                {
                    if (!data || !data->Meets(referencePlayer,unit))
                        continue;
                }

                BattleGround* bg = referencePlayer->GetBattleGround();

                // some hardcoded requirements
                switch(achievementCriteria->referredAchievement)
                {
                    case 231:                   // Wrecking Ball
                        if (!bg || !unit || unit->GetTypeId() != TYPEID_PLAYER || bg->GetPlayerScore(referencePlayer, SCORE_DEATHS) != 0)
                            continue;
                        break;
                    case 233:                   // Bloodthirsty Berserker
                        if (!bg || !referencePlayer->HasAura(23505))
                            continue;
                        break;
                    case 1488:                  // World Killing Blows
                    {
                        if(bg)
                            continue;
                        if (achievementCriteria->ID == 5512 && referencePlayer->GetMapId() == 0 ||      // Eastern Kingdoms
                            achievementCriteria->ID == 5530 && referencePlayer->GetMapId() == 1 ||      // Kalimdor
                            achievementCriteria->ID == 5531 && referencePlayer->GetMapId() == 530 ||    // Burning Crusade Areas
                            achievementCriteria->ID == 5532 && referencePlayer->GetMapId() == 571)      // Northrend
                            break;
                        continue;
                    }
                    case 1490:                  // Arena Killing Blows
                    {
                        if(!bg)
                            continue;
                        if (achievementCriteria->ID == 5533 && referencePlayer->GetMapId() == 559 ||    // Nagrand Arena
                            achievementCriteria->ID == 5534 && referencePlayer->GetMapId() == 562 ||    // Blade's Edge Arena
                            achievementCriteria->ID == 5535 && referencePlayer->GetMapId() == 572 ||    // Ruins of Lordaeron
                            achievementCriteria->ID == 9165 && referencePlayer->GetMapId() == 617 ||    // Dalaran Sewers
                            achievementCriteria->ID == 9166 && referencePlayer->GetMapId() == 618)      // Ring of Valor
                            break;
                        continue;
                    }
                    case 1491:                  // Battleground Killing Blows
                    {
                        if(!bg)
                            continue;
                        if (achievementCriteria->ID == 5436 && referencePlayer->GetMapId() == 30 ||     // AV
                            achievementCriteria->ID == 5537 && referencePlayer->GetMapId() == 529 ||    // AB
                            achievementCriteria->ID == 5538 && referencePlayer->GetMapId() == 489 ||    // WS
                            achievementCriteria->ID == 5539 && referencePlayer->GetMapId() == 566 ||    // EY
                            achievementCriteria->ID == 5540 && referencePlayer->GetMapId() == 607 ||    // SA
                            achievementCriteria->ID == 13254 && referencePlayer->GetMapId() == 628)     // IC
                            break;
                        continue;
                    }
                    case 1492:                  // 2v2 Arena Killing Blows
                    case 1493:                  // 3v3 Arena Killing Blows
                    case 1494:                  // 5v5 Arena Killing Blows
                    {
                        if(!bg)
                            continue;
                        if(!bg->isArena())
                            continue;
                        if( (achievementCriteria->ID == 5441 && bg->GetArenaType() == ARENA_TYPE_2v2) ||
                            (achievementCriteria->ID == 5442 && bg->GetArenaType() == ARENA_TYPE_3v3) ||
                            (achievementCriteria->ID == 5443 && bg->GetArenaType() == ARENA_TYPE_5v5) )
                            break;
                        continue;
                    }
                    case 3856:                  // Demolition Derby (alliance)
                    case 4256:                  // Demolition Derby (horde)
                    {
                        uint32 AchCrID = achievementCriteria->ID;
                        if( ((AchCrID == 11497 || AchCrID == 12178) && miscvalue2 == 34802) ||  // Glaive Thrower
                            ((AchCrID == 11498 || AchCrID == 12179) && miscvalue2 == 34775) ||  // Demolisher
                            ((AchCrID == 11500 || AchCrID == 12181) && miscvalue2 == 34793) ||  // Catapult
                            ((AchCrID == 11501 || AchCrID == 12182) && miscvalue2 == 34776) )   // Siege Engine
                            break;
                        continue;
                    }
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            {
                if (referencePlayer->GetAreaId() != achievementCriteria->honorable_kill_at_area.areaID)
                    continue;

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            {
                if (!miscvalue1 || !miscvalue2 || miscvalue2 != achievementCriteria->objective_capture.captureID)
                    continue;

                BattleGround* bg = referencePlayer->GetBattleGround();
                if (!bg)
                    continue;

                // some hardcoded requirements
                switch(achievementCriteria->objective_capture.captureID)
                {
                    case WS_OBJECTIVE_CAPTURE_FLAG:      // WS, capture a flag
                    {
                        if (bg->GetTypeID(true) != BATTLEGROUND_WS)
                            continue;
                                                         // WS, capture 3 flags without dying
                        if (achievementCriteria->referredAchievement == 204)
                        {
                            if (bg->GetPlayerScore(referencePlayer, SCORE_DEATHS))
                                continue;
                        }
                        break;
                    }
                    case WS_OBJECTIVE_RETURN_FLAG:       // WS, return a flag
                    {
                        if (bg->GetTypeID(true) != BATTLEGROUND_WS)
                            continue;
                        break;
                    }
                    case AV_OBJECTIVE_ASSAULT_TOWER:     // AV, assault a tower
                    case AV_OBJECTIVE_ASSAULT_GRAVEYARD: // AV, assault a graveyard
                    case AV_OBJECTIVE_DEFEND_TOWER:      // AV, defend a tower
                    case AV_OBJECTIVE_DEFEND_GRAVEYARD:  // AV, defend a graveyard
                    {
                        if (bg->GetTypeID(true) != BATTLEGROUND_AV)
                            continue;
                        break;
                    }
                    case AB_OBJECTIVE_ASSAULT_BASE:      // AB, assault a base
                    case AB_OBJECTIVE_DEFEND_BASE:       // AB, defend a base
                    {
                        if (bg->GetTypeID(true) != BATTLEGROUND_AB)
                            continue;
                        break;
                    }
                    case EY_OBJECTIVE_CAPTURE_FLAG:      // EY, capture a flag
                    {
                        if (bg->GetTypeID(true) != BATTLEGROUND_EY)
                            continue;

                        switch(achievementCriteria->referredAchievement)
                        {
                            case 211:                    // EY, capture flag while controling all 4 bases
                            {
                                if (!bg->IsAllNodesControlledByTeam(referencePlayer->GetTeam()))
                                    continue;
                                break;
                            }
                            case 216:                    // EY, capture 3 flags without dying
                            {
                                if (bg->GetPlayerScore(referencePlayer, SCORE_DEATHS))
                                    continue;
                                break;
                            }
                        }
                        break;
                    }
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
            {
                if (!miscvalue1 || achievementCriteria->highest_team_rating.teamtype != miscvalue1)
                    continue;

                change = miscvalue2;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            {
                if (!miscvalue1 || achievementCriteria->highest_personal_rating.teamtype != miscvalue1)
                    continue;

                if (achievementCriteria->highest_personal_rating.teamrating != 0 && achievementCriteria->highest_personal_rating.teamrating > miscvalue2)
                    continue;

                change = miscvalue2;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            {
                // This criteria is only called directly after login - with expected miscvalue1 == 1
                if (!miscvalue1)
                    continue;

                // They have no proper requirements in dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (!data || !data->Meets(referencePlayer, NULL))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            {
                if (!miscvalue1)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            {
                change = referencePlayer->GetGuildLevel();
                progressType = PROGRESS_SET;
                break;
            }
            // std case: not exist in DBC, not triggered in code as result
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING:
                break;
                // FIXME: not triggered in code as result, need to implement
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                break;                                   // Not implemented yet :(
        }

        SetCriteriaProgress(achievementCriteria, achievement, change, referencePlayer, progressType);
    }
}

template<class T>
bool AchievementMgr<T>::AdditionalRequirementsSatisfied(AchievementCriteriaEntry const *criteria, uint64 miscValue1, uint64 /*miscValue2*/, Unit const* unit, Player* referencePlayer) const
{
    for (uint8 i = 0; i < MAX_ADDITIONAL_CRITERIA_CONDITIONS; ++i)
    {
        uint32 reqType = criteria->additionalConditionType[i];
        uint32 reqValue;

        // There is missing additionalConditionValue[2] field in DBC.
        // So we need to set values for those criterias manually.
        // I use values from 4.0.6 DBC.
        if (i < 2)
            reqValue = criteria->additionalConditionValue[i];
        else
        {
            switch (criteria->ID)
            {
                case 3929: reqValue = 8403; break;
                case 3931: reqValue = 9099; break;
                case 4112: reqValue = 4395; break;
                case 6237: reqValue = 6; break;
                case 6239: reqValue = 7; break;
                case 6240: reqValue = 11; break;
                case 6241: reqValue = 4; break;
                case 6242: reqValue = 3; break;
                case 6243: reqValue = 8; break;
                case 6244: reqValue = 2; break;
                case 6245: reqValue = 9; break;
                case 6246: reqValue = 5; break;
                case 6261: reqValue = 4395; break;
                case 6302: reqValue = 6; break;
                case 6312: reqValue = 9; break;
                case 6313: reqValue = 6; break;
                case 6314: reqValue = 5; break;
                case 6315: reqValue = 7; break;
                case 6316: reqValue = 11; break;
                case 6317: reqValue = 4; break;
                case 6319: reqValue = 8; break;
                case 6320: reqValue = 2; break;
                case 6321: reqValue = 3; break;
                case 2379:
                case 7573:
                case 10223:
                case 10240:
                case 10241:
                    reqValue = 0;
                    break;
                case 4227:
                case 12859:
                    reqValue = 68478;
                    break;
                case 6238:
                case 6318:
                case 7574:
                case 10229:
                case 10238:
                case 10239:
                case 14638:
                    reqValue = 1;
                    break;
                case 14808:
                    reqValue = 85;
                    break;
                case 14887:
                case 14888:
                    reqValue = 23505;
                    break;
                case 13905:
                case 14537:
                case 14538:
                case 14539:
                case 14540:
                case 14541:
                case 14542:
                case 14543:
                case 14544:
                case 14545:
                case 14546:
                case 14547:
                case 14548:
                case 14549:
                case 14550:
                case 14551:
                case 14552:
                case 14553:
                case 14554:
                case 14555:
                case 14556:
                case 14557:
                case 14558:
                case 14559:
                case 14560:
                case 14561:
                case 14562:
                case 14563:
                case 14564:
                case 14565:
                case 14566:
                case 14567:
                case 14568:
                case 14569:
                case 14570:
                case 14571:
                case 14572:
                case 14573:
                case 14574:
                case 14575:
                case 14602:
                case 14603:
                case 14604:
                case 14605:
                case 14606:
                case 14607:
                case 14608:
                case 14609:
                case 14610:
                case 14611:
                case 14612:
                case 14613:
                case 14614:
                case 14615:
                case 14616:
                case 14617:
                case 14618:
                case 14619:
                case 14620:
                case 14621:
                case 14622:
                case 14623:
                case 14624:
                case 14625:
                case 14626:
                case 14627:
                case 14628:
                case 14629:
                case 14630:
                case 14631:
                case 14632:
                case 14633:
                case 14634:
                case 14635:
                case 14636:
                case 14637:
                case 14639:
                case 14640:
                case 14641:
                case 14642:
                case 14643:
                case 14644:
                case 14645:
                case 14646:
                case 14647:
                case 14648:
                case 14649:
                case 14650:
                case 14651:
                case 14652:
                case 14653:
                case 15485:
                case 15486:
                case 15487:
                case 15488:
                case 15489:
                case 15490:
                case 15491:
                case 15492:
                case 15493:
                case 15494:
                case 15495:
                case 15496:
                case 15497:
                case 15498:
                case 15499:
                case 15500:
                case 15501:
                case 15502:
                case 15503:
                case 15504:
                case 15505:
                case 15506:
                case 15507:
                case 15508:
                case 15509:
                case 15510:
                case 15511:
                case 15512:
                case 15513:
                case 15514:
                case 15515:
                case 15516:
                case 15517:
                case 15518:
                case 15519:
                case 15520:
                case 15521:
                case 15522:
                case 15523:
                    reqValue = 9000;
                    break;
                case 9124:
                case 9143:
                case 9144:
                case 9145:
                case 9146:
                case 9147:
                case 9148:
                case 9149:
                case 9150:
                case 9151:
                case 17845:
                case 17846:
                    reqValue = 8128; 
                    break;
            }
        }

        switch (AchievementCriteriaMoreReqType(reqType))
        {
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_DRUNK_VALUE: // 1
            {
                if (referencePlayer->GetDrunkValue() < reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_LEVEL: // 2
            {
                ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(miscValue1);
                if (!pItem)
                    return false;

                if (pItem->ItemLevel < reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_CREATURE_ENTRY: // 4
                if (!unit || !unit->IsInWorld())
                    return false;

                // Hack for Bros. Before Ho Ho Ho's
                if (criteria->referredAchievement == 1685 || criteria->referredAchievement == 1686)
                    break;

                if (unit->GetEntry() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_MUST_BE_PLAYER: // 5
                if (!unit || !unit->IsInWorld() || unit->GetTypeId() != TYPEID_PLAYER)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_MUST_BE_DEAD: // 6
                if (!unit || !unit->IsInWorld() || unit->isAlive())
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_MUST_BE_ENEMY: // 7
                if (!unit || !unit->IsInWorld() || !referencePlayer->IsHostileTo(unit))
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_HAS_AURA: // 8
                // Hack for Fa-la-la-la-Ogri'la, there are wrong auras in dbc
                if (criteria->referredAchievement == 1282)
                {
                    if (referencePlayer->HasAura(62061))
                        break;
                }

                if (!referencePlayer->HasAura(reqValue))
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_HAS_AURA: // 10
                if (!unit || !unit->IsInWorld() || !unit->HasAura(reqValue))
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_MUST_BE_MOUNTED: // 11
                if (!unit || !unit->IsMounted())
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_QUALITY_MIN: // 14
            {
                // miscValue1 is itemid
                ItemPrototype const * const item = sObjectMgr.GetItemPrototype(uint32(miscValue1));
                if (!item || item->Quality < reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_QUALITY_EQUALS: // 15
            {
                // miscValue1 is itemid
                ItemPrototype const * const item = sObjectMgr.GetItemPrototype(uint32(miscValue1));
                if (!item || item->Quality < reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_AREA_OR_ZONE: // 17
            {
                uint32 zoneId, areaId;
                referencePlayer->GetZoneAndAreaId(zoneId, areaId);
                if (zoneId != reqValue && areaId != reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_AREA_OR_ZONE: // 18
            {
                if (!unit)
                    return false;

                uint32 zoneId, areaId;
                unit->GetZoneAndAreaId(zoneId, areaId);
                if (zoneId != reqValue && areaId != reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_MAP_DIFFICULTY: // 20
            {
                if (Map* pMap = referencePlayer->GetMap())
                {
                    if (pMap->IsNonRaidDungeon() || pMap->IsRaid())
                    {
                        if (pMap->GetDifficulty() < Difficulty(reqValue))
                            return false;
                    }
                    else
                        return false;
                }
                else
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_RACE: // 25
                if (referencePlayer->getRace() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_CLASS: // 26
                if (referencePlayer->getClass() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_RACE: // 27
                if (!unit || !unit->IsInWorld() || unit->GetTypeId() != TYPEID_PLAYER || unit->getRace() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_CLASS: // 28
                if (!unit || !unit->IsInWorld() || unit->GetTypeId() != TYPEID_PLAYER || unit->getClass() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_MAX_GROUP_MEMBERS: // 29
                if (referencePlayer->GetGroup() && referencePlayer->GetGroup()->GetMembersCount() >= reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_CREATURE_TYPE: // 30
            {
                if (!unit || unit->GetTypeId() != TYPEID_UNIT)
                    return false;

                if (((Creature const*)unit)->GetCreatureType() != reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_MAP: // 32
                if (referencePlayer->GetMapId() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_CLASS: // 33
            {
                ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(miscValue1);
                if (!pItem)
                    return false;

                if (pItem->Class != reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_SUBCLASS: // 34
            {
                ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(miscValue1);
                if (!pItem)
                    return false;

                if (pItem->SubClass != reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_MIN_PERSONAL_RATING: // 37
            {
                if (miscValue1 < reqValue)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TITLE_BIT_INDEX: // 38
                // miscValue1 is title's bit index
                if (miscValue1 != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_SOURCE_LEVEL: // 39
                if (referencePlayer->getLevel() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_LEVEL: // 40
                if (!unit || !unit->IsInWorld() || unit->getLevel() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_ZONE: // 41
                if (!unit || !unit->IsInWorld() || unit->GetZoneId() != reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_TARGET_HEALTH_PERCENT_BELOW: // 46
                if (!unit || !unit->IsInWorld() || unit->GetHealthPercent() >= reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_MIN_ACHIEVEMENT_POINTS: // 56
                if (referencePlayer->GetAchievementMgr().GetAchievementPoints() < reqValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_REQUIRES_GUILD_GROUP: // 61
            {
                Group* pGroup = referencePlayer->GetGroup();
                if (!pGroup)
                    return false;

                Map* map = referencePlayer->GetMap();
                if (!map)
                    return false;

                if (!map->HasGuildGroup(referencePlayer->GetGuildGuid(), referencePlayer))
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_GUILD_REPUTATION: // 62
            {
                if (uint32(referencePlayer->GetReputationMgr().GetReputation(GUILD_REP_FACTION)) < reqValue) // 1168 = Guild faction
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PROJECT_RARITY: // 65
            {
                //if (!miscValue1)
                //    return false;

                //bool ok = false;
                //for (std::set<ResearchProjectEntry const*>::const_iterator itr = sResearchProjectSet.begin(); itr != sResearchProjectSet.end(); ++itr)
                //{
                //    if ((*itr)->ID == miscValue1)
                //    {
                //        ok = ((*itr)->rare == reqValue);
                //        break;
                //    }
                //}
                //if (!ok)
                    return false;
                break;
            }
            case ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PROJECT_RACE: // 66
            {
                //if (!miscValue1)
                //    return false;

                //bool ok = false;
                //for (std::set<ResearchProjectEntry const*>::const_iterator itr = sResearchProjectSet.begin(); itr != sResearchProjectSet.end(); ++itr)
                //{
                //    if ((*itr)->ID == miscValue1)
                //    {
                //        ok = ((*itr)->branchId == reqValue);
                //        break;
                //    }
                //}
                //if (!ok)
                    return false;
                break;
            }
            default:
                break;
        }
    }

    return true;
}

template <class T>
uint32 AchievementMgr<T>::GetCriteriaProgressCounter(AchievementCriteriaEntry const* entry) const
{
    CriteriaProgressMap::const_iterator iter = m_criteriaProgress.find(entry->ID);
    return iter != m_criteriaProgress.end() ? iter->second.counter : 0;
}

template <class T>
uint32 AchievementMgr<T>::GetCriteriaProgressMaxCounter(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement)
{
    uint32 resultValue = 0;
    switch (achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            resultValue = achievementCriteria->win_bg.winCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            resultValue = achievementCriteria->kill_creature.creatureCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            resultValue = achievementCriteria->reach_level.level;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            resultValue = achievementCriteria->reach_skill_level.skillLevel;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            resultValue = achievementCriteria->complete_quest_count.totalQuestCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            resultValue = achievementCriteria->complete_daily_quest_daily.numberOfDays;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            resultValue = achievementCriteria->complete_quests_in_zone.questCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CURRENCY_EARNED:
            resultValue = achievementCriteria->currencyEarned.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            resultValue = achievementCriteria->healing_done.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            resultValue = achievementCriteria->complete_daily_quest.questCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            resultValue = achievementCriteria->fall_without_dying.fallHeight;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            resultValue = achievementCriteria->be_spell_target.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            resultValue = achievementCriteria->cast_spell.castCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            resultValue = achievementCriteria->own_item.itemCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            resultValue = achievementCriteria->win_arena.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            resultValue = achievementCriteria->win_rated_arena.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            resultValue = achievementCriteria->learn_skill_level.skillLevel * 75;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            resultValue = achievementCriteria->use_item.itemCount;
//            resultValue = achievementCriteria->use_item.itemCount ? achievementCriteria->use_item.itemCount : 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            resultValue = achievementCriteria->loot_item.itemCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            resultValue = achievementCriteria->buy_bank_slot.numberOfSlots;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            resultValue = achievementCriteria->gain_reputation.reputationAmount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            resultValue = achievementCriteria->gain_exalted_reputation.numberOfExaltedFactions;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            resultValue = achievementCriteria->visit_barber.numberOfVisits;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            resultValue = achievementCriteria->equip_epic_item.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            resultValue = achievementCriteria->roll_greed_on_loot.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            resultValue = achievementCriteria->hk_class.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            resultValue = achievementCriteria->hk_race.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            resultValue = achievementCriteria->do_emote.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            resultValue = achievementCriteria->equip_item.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            resultValue = achievementCriteria->quest_reward_money.goldInCopper;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            resultValue = achievementCriteria->loot_money.goldInCopper;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            resultValue = achievementCriteria->use_gameobject.useCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            resultValue = achievementCriteria->special_pvp_kill.killCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            resultValue = achievementCriteria->fish_in_gameobject.lootCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            resultValue = achievementCriteria->learn_skillline_spell.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            resultValue = achievementCriteria->win_duel.duelCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            resultValue = achievementCriteria->loot_type.lootTypeCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            resultValue = achievementCriteria->learn_skill_line.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
            resultValue = achievementCriteria->honorable_kill.killCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            resultValue = achievementCriteria->honorable_kill_at_area.killCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            resultValue = achievementCriteria->objective_capture.captureCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            resultValue = achievementCriteria->highest_personal_rating.teamrating;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            resultValue = achievementCriteria->use_lfg.dungeonsComplete;
            break;

            // handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            resultValue = 0;
            break;
    }

    if (achievement && (achievement->flags & ACHIEVEMENT_FLAG_COUNTER))
        resultValue = std::numeric_limits<uint32>::max();

    return resultValue;
}

template <class T>
bool AchievementMgr<T>::IsCompletedCriteria(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement) const
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr.IsRealmCompleted(achievement))
            return false;
    }

    CriteriaProgressMap::const_iterator itr = m_criteriaProgress.find(achievementCriteria->ID);
    if (itr == m_criteriaProgress.end())
        return false;

    CriteriaProgress const* progress = &itr->second;

    uint32 maxcounter = GetCriteriaProgressMaxCounter(achievementCriteria, achievement);

    return progress->counter >= maxcounter || ((achievement->flags & ACHIEVEMENT_FLAG_REQ_COUNT) && progress->counter);
}

template <class T>
void AchievementMgr<T>::CompletedCriteriaFor(AchievementEntry const* achievement, Player* referencePlayer)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (m_completedAchievements.find(achievement->ID) != m_completedAchievements.end())
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement, referencePlayer);
}

template<class T>
bool AchievementMgr<T>::IsCompletedAchievement(AchievementEntry const* entry)
{
    // counter can never complete
    if (entry->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    // for achievement with referenced achievement criterias get from referenced and counter from self
    uint32 achievementForTestId = entry->refAchievement ? entry->refAchievement : entry->ID;
    uint32 achievementForTestCount = entry->count;

    AchievementCriteriaEntryList const* cList = sAchievementMgr.GetAchievementCriteriaByAchievement(achievementForTestId);
    if (!cList)
        return false;
    uint32 count = 0;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT countained in the achievement, but in each individual criteria
    if (entry->flags & ACHIEVEMENT_FLAG_SUMM)
    {
        for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
        {
            AchievementCriteriaEntry const* criteria = *itr;

            CriteriaProgressMap::const_iterator itrProgress = m_criteriaProgress.find(criteria->ID);
            if (itrProgress == m_criteriaProgress.end())
                continue;

            CriteriaProgress const* progress = &itrProgress->second;
            count += progress->counter;

            // for counters, field4 contains the main count requirement
            if (count >= criteria->raw.count)
                return true;
        }
        return false;
    }

    // Default case - need complete all or
    bool completed_all = true;
    for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
    {
        AchievementCriteriaEntry const* criteria = *itr;

        bool completed = IsCompletedCriteria(criteria, entry);

        // found an uncompleted criteria, but DONT return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
        if (completed)
            ++count;
        else
            completed_all = false;

        // completed as have req. count of completed criterias
        if (achievementForTestCount > 0 && achievementForTestCount <= count)
            return true;
    }

    // all criterias completed requirement
    if (completed_all && achievementForTestCount == 0)
        return true;

    return false;
}

template<class T>
void AchievementMgr<T>::SetCriteriaProgress(AchievementCriteriaEntry const* criteria, AchievementEntry const* achievement, uint32 changeValue, Player* referencePlayer, ProgressType ptype)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::SetCriteriaProgress(%u, %u) for %s", criteria->ID, changeValue, GetOwner()->GetObjectGuid().GetString().c_str());

    uint32 max_value = GetCriteriaProgressMaxCounter(criteria, achievement);

    // change value must be in allowed value range for SET/HIGHEST directly
    if (changeValue > max_value)
        changeValue = max_value;

    CriteriaProgress* progress = NULL;
    uint32 old_value = 0;
    uint32 newValue = 0;

    CriteriaProgressMap::iterator iter = m_criteriaProgress.find(criteria->ID);
    if (iter == m_criteriaProgress.end())
    {
        // not create record for 0 counter
        if (changeValue == 0)
            return;

        // not start manually started timed achievements
        if (criteria->IsExplicitlyStartedTimedCriteria())
            return;

        progress = &m_criteriaProgress[criteria->ID];

        progress->date = time(NULL);
        progress->timedCriteriaFailed = false;

        // timed criterias are added to fail-timer map, and send the starting with counter=0
        if (criteria->timeLimit)
        {
            m_criteriaFailTimes[criteria->ID] = time_t(progress->date + criteria->timeLimit);
            progress->counter = 0;
            SendCriteriaUpdate(criteria->ID, progress);
        }

        newValue = changeValue;
    }
    else
    {
        progress = &iter->second;

        old_value = progress->counter;
        switch (ptype)
        {
            case PROGRESS_SET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
            {
                // avoid overflow
                newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // not update (not mark as changed) if counter will have same value
        if (progress->counter == newValue)
            return;
    }

    progress->counter = newValue;
    progress->changed = true;

    // update client side value
    SendCriteriaUpdate(criteria->ID, progress);

    // nothing do for counter case
    if ((achievement->flags & ACHIEVEMENT_FLAG_COUNTER) && !(achievement->flags & ACHIEVEMENT_FLAG_SUMM)) //second check for "Tastes Like Chicken" and alike
        return;

    // update dependent achievements state at criteria complete
    if (old_value < progress->counter)
    {
        if (IsCompletedCriteria(criteria, achievement))
        {
            if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_CRITERIA_MEMBERS && !progress->CompletedGUID)
                progress->CompletedGUID = referencePlayer->GetObjectGuid();

            CompletedCriteriaFor(achievement, referencePlayer);
        }

        // check again the completeness for SUMM and REQ COUNT achievements,
        // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
        if (achievement->flags & ACHIEVEMENT_FLAG_SUMM)
        {
            if (IsCompletedAchievement(achievement))
                CompletedAchievement(achievement, referencePlayer);
        }

        if (AchievementEntryList const* achRefList = sAchievementMgr.GetAchievementByReferencedId(achievement->ID))
        {
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (IsCompletedAchievement(*itr))
                    CompletedAchievement(*itr, referencePlayer);
        }
    }
    // update dependent achievements state at criteria incomplete
    else if (old_value > progress->counter)
    {
        if (progress->counter < max_value)
            SendCriteriaProgressRemove(criteria->ID);

        if (HasAchievement(achievement->ID))
            if (!IsCompletedAchievement(achievement))
                IncompletedAchievement(achievement);

        if (AchievementEntryList const* achRefList = sAchievementMgr.GetAchievementByReferencedId(achievement->ID))
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (HasAchievement((*itr)->ID))
                    if (!IsCompletedAchievement(*itr))
                        IncompletedAchievement(*itr);
    }
}

template <>
void AchievementMgr<Player>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    DETAIL_LOG("AchievementMgr<Player>::CompletedAchievement(%u)", achievement->ID);
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID) != m_completedAchievements.end())
        return;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = sGuildMgr.GetGuildById(referencePlayer->GetGuildId()))
            guild->LogNewsEvent(GUILD_NEWS_PLAYER_ACHIEVEMENT, time(NULL), referencePlayer->GetObjectGuid(), achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    if (!GetOwner()->GetSession()->PlayerLoading())
        SendAchievementEarned(achievement);
    CompletedAchievementData& ca =  m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if (!(achievement->flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr.SetRealmCompleted(achievement);

    m_achievementPoints += achievement->points;

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, NULL, 0, referencePlayer);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr.GetAchievementReward(achievement, GetOwner()->getGender());

    // no rewards
    if (!reward)
        return;

    // titles
    if (uint32 titleId = reward->titleId[GetOwner()->GetTeam() == HORDE ? 1 : 0])
    {
        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetOwner()->SetTitle(titleEntry);
    }

    // mail
    if (reward->sender)
    {
        Item* item = reward->itemId ? Item::CreateItem(reward->itemId, 1, GetOwner()) : NULL;

        int loc_idx = GetOwner()->GetSession()->GetSessionDbLocaleIndex();

        // subject and text
        std::string subject = reward->subject;
        std::string text = reward->text;
        if (loc_idx >= 0)
        {
            if(AchievementRewardLocale const* loc = sAchievementMgr.GetAchievementRewardLocale(achievement, GetOwner()->getGender()))
            {
                if (loc->subject.size() > size_t(loc_idx) && !loc->subject[loc_idx].empty())
                    subject = loc->subject[loc_idx];
                if (loc->text.size() > size_t(loc_idx) && !loc->text[loc_idx].empty())
                    text = loc->text[loc_idx];
            }
        }

        MailDraft draft(subject, text);

        if (item)
        {
            // save new item before send
            item->SaveToDB();                               // save for prevent lost at next mail load, if send fail then item will deleted

            // item
            draft.AddItem(item);
        }

        draft.SendMailTo(GetOwner(), MailSender(MAIL_CREATURE, reward->sender));
    }
}

template <class T>
void AchievementMgr<T>::IncompletedAchievement(AchievementEntry const* achievement)
{
}

template <>
void AchievementMgr<Player>::IncompletedAchievement(AchievementEntry const* achievement)
{
    DETAIL_LOG("AchievementMgr<Player>::IncompletedAchievement(%u)", achievement->ID);
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    CompletedAchievementMap::iterator itr = m_completedAchievements.find(achievement->ID);
    if (itr == m_completedAchievements.end())
        return;

    WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4);
    data << uint32(achievement->ID);
    SendPacket(&data);

    if (!itr->second.changed)                               // complete state saved
        CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = %u AND achievement = %u",
            GetOwner()->GetGUIDLow(), achievement->ID);

    m_completedAchievements.erase(achievement->ID);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr.GetAchievementReward(achievement, GetOwner()->getGender());

    // no rewards
    if (!reward)
        return;

    // titles
    if (uint32 titleId = reward->titleId[GetOwner()->GetTeam() == HORDE ? 0 : 1])
    {
        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetOwner()->SetTitle(titleEntry, true);
    }

    // items impossible remove in clear way...
}

template <>
void AchievementMgr<Guild>::IncompletedAchievement(AchievementEntry const* achievement)
{
}

template <>
void AchievementMgr<Guild>::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
   DETAIL_LOG("AchievementMgr<Guild>::CompletedAchievement(%u)", achievement->ID);

    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID) != m_completedAchievements.end())
        return;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = sGuildMgr.GetGuildById(referencePlayer->GetGuildId()))
            guild->LogNewsEvent(GUILD_NEWS_GUILD_ACHIEVEMENT, time(NULL), 0, achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    SendAchievementEarned(achievement);
    CompletedAchievementData& ca = m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_GUILD_MEMBERS)
    {
        if (referencePlayer->GetGuildId() == GetOwner()->GetId())
            ca.guids.insert(referencePlayer->GetObjectGuid());

        if (Group* group = referencePlayer->GetGroup())
            for (GroupReference* ref = group->GetFirstMember(); ref != NULL; ref = ref->next())
                if (Player* groupMember = ref->getSource())
                    if (groupMember->GetGuildId() == GetOwner()->GetId())
                        ca.guids.insert(groupMember->GetObjectGuid());
    }

    sAchievementMgr.SetRealmCompleted(achievement);

    m_achievementPoints += achievement->points;

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, NULL, 0, referencePlayer);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->points, 0, 0, 0, referencePlayer);
}

struct VisibleAchievementPred
{
    bool operator()(CompletedAchievementMap::value_type const& val)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(val.first);
        return achievement && !(achievement->flags & ACHIEVEMENT_FLAG_COUNTER);
    }
};

template <class T>
void AchievementMgr<T>::SendAllAchievementData(Player* /*player*/)
{
    // since we don't know the exact size of the packed GUIDs this is just an approximation
    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA, m_completedAchievements.size() * (4 + 4) + m_criteriaProgress.size() * (4 + 9 + 9 + 4 + 4 + 4 + 4));

    ObjectGuid guid = GetOwner()->GetObjectGuid();

    data.WriteBits(m_criteriaProgress.size(), 21);

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
    {
        ObjectGuid counter = ObjectGuid(uint64(iter->second.counter));

        data.WriteGuidMask<4>(guid);
        data.WriteGuidMask<3>(counter);
        data.WriteGuidMask<5>(guid);
        data.WriteGuidMask<0, 6>(counter);
        data.WriteGuidMask<3, 0>(guid);

        data.WriteGuidMask<4>(counter);
        data.WriteGuidMask<2>(guid);
        data.WriteGuidMask<7>(counter);
        data.WriteGuidMask<7>(guid);
        uint8 flags = 0;                                        // Seems always 0
        data.WriteBits(flags, 2);
        data.WriteGuidMask<6>(guid);

        data.WriteGuidMask<2, 1, 5>(counter);
        data.WriteGuidMask<1>(guid);
    }

    data.WriteBits(m_completedAchievements.size(), 23);

    time_t now = time(NULL);
    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
    {
        ObjectGuid counter = ObjectGuid(uint64(iter->second.counter));

        data.WriteGuidBytes<3>(guid);
        data.WriteGuidBytes<5, 6>(counter);
        data.WriteGuidBytes<4, 6>(guid);
        data.WriteGuidBytes<2>(counter);

        data << uint32(now - iter->second.date);               // Timer 2

        data.WriteGuidBytes<2>(guid);

        data << uint32(iter->first);

        data.WriteGuidBytes<5>(guid);
        data.WriteGuidBytes<0, 3, 1, 4>(counter);
        data.WriteGuidBytes<0, 7>(guid);
        data.WriteGuidBytes<7>(counter);

        data << uint32(now - iter->second.date);               // Timer 1
        data.AppendPackedTime(now);
        //data << uint32(secsToTimeBitFields(now));

        data.WriteGuidBytes<1>(guid);
    }

    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        data << uint32(iter->first);
        // data << uint32(secsToTimeBitFields(iter->second.date));
        data.AppendPackedTime(iter->second.date);
    }

    SendPacket(&data);
}

template <>
void AchievementMgr<Guild>::SendAllAchievementData(Player* player)
{
    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DATA, m_completedAchievements.size() * (4 + 4) + 4);
    data.WriteBits(m_completedAchievements.size(), 23);
    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        data << uint32(secsToTimeBitFields(itr->second.date));
        data << uint32(itr->first);
    }

    player->GetSession()->SendPacket(&data);
}

template <class T>
void AchievementMgr<T>::SendRespondInspectAchievements(Player* player, uint32 achievementId /*= 0*/)
{
    // since we don't know the exact size of the packed GUIDs this is just an approximation
    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS, GetOwner()->GetPackGUID().size() + m_completedAchievements.size() * (4 + 4) + m_criteriaProgress.size() * (4 + 9 + 9 + 4 + 4 + 4 + 4));

    ObjectGuid targetGuid = GetOwner()->GetObjectGuid();
    ObjectGuid guid = GetOwner()->GetObjectGuid();

    data.WriteGuidMask<7, 4, 1>(targetGuid);

    data.WriteBits(m_completedAchievements.size(), 23);

    data.WriteGuidMask<0, 3>(targetGuid);

    data.WriteBits(m_criteriaProgress.size(), 21);

    data.WriteGuidMask<2>(targetGuid);

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
    {
        ObjectGuid counter = ObjectGuid(uint64(iter->second.counter));

        data.WriteGuidMask<5, 3>(guid);
        data.WriteGuidMask<1, 4, 2>(counter);
        data.WriteGuidMask<6>(guid);
        data.WriteGuidMask<0>(counter);
        data.WriteGuidMask<4, 1, 2>(guid);
        data.WriteGuidMask<3, 7>(counter);

        uint8 flags = 0;                                    // Seems always 0
        data.WriteBits(flags, 2);

        data.WriteGuidMask<0>(guid);
        data.WriteGuidMask<5, 6>(counter);
        data.WriteGuidMask<7>(guid);
    }

    data.WriteGuidMask<6, 5>(targetGuid);

    time_t now = time(NULL);

    for (CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter != m_criteriaProgress.end(); ++iter)
    {
        ObjectGuid counter = ObjectGuid(uint64(iter->second.counter));

        data.WriteGuidBytes<3>(counter);
        data.WriteGuidBytes<4>(guid);

//        data->AppendPackedTime(iter->second.date);
        data << uint32(now - iter->second.date);            // Timer 1

        data.WriteGuidBytes<1>(counter);

        //data << uint32(secsToTimeBitFields(now));
        data.AppendPackedTime(now);

        data.WriteGuidBytes<3, 7>(guid);
        data.WriteGuidBytes<5>(counter);
        data.WriteGuidBytes<0>(guid);
        data.WriteGuidBytes<4, 2, 6, 7>(counter);
        data.WriteGuidBytes<6>(guid);

        data << uint32(iter->first);
        data << uint32(now - iter->second.date);            // Timer 2

        data.WriteGuidBytes<1, 5>(guid);
        data.WriteGuidBytes<0>(counter);
        data.WriteGuidBytes<2>(guid);
    }

    data.WriteGuidBytes<1, 6, 3, 0, 2>(targetGuid);

    for (CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter != m_completedAchievements.end(); ++iter)
    {
        data << uint32(iter->first);
        //data << uint32(secsToTimeBitFields(iter->second.date));
        data.AppendPackedTime(iter->second.date);
    }

    data.WriteGuidBytes<7, 4, 5>(targetGuid);

    player->GetSession()->SendPacket(&data);
}

template<>
void AchievementMgr<Guild>::SendRespondInspectAchievements(Player* player, uint32 achievementId /*= 0*/)
{
    //will send response to criteria progress request
    AchievementCriteriaEntryList const* criteria = sAchievementMgr.GetAchievementCriteriaByAchievement(achievementId);
    if (!criteria)
    {
        // send empty packet
        WorldPacket data(SMSG_GUILD_CRITERIA_DATA, 3);
        data.WriteBits(0, 21);
        player->GetSession()->SendPacket(&data);
        return;
    }

    uint32 numCriteria = 0;
    ByteBuffer criteriaBits(criteria->size() * (8 + 8) / 8);
    ByteBuffer criteriaData(criteria->size() * (8 + 8 + 4 + 4 + 4));
    for (AchievementCriteriaEntryList::const_iterator itr = criteria->begin(); itr != criteria->end(); ++itr)
    {
        uint32 criteriaId = (*itr)->ID;
        CriteriaProgressMap::const_iterator progress = m_criteriaProgress.find(criteriaId);
        if (progress == m_criteriaProgress.end())
            continue;

        ++numCriteria;
    }

    criteriaBits.WriteBits(numCriteria, 21);

    for (AchievementCriteriaEntryList::const_iterator itr = criteria->begin(); itr != criteria->end(); ++itr)
    {
        uint32 criteriaId = (*itr)->ID;
        CriteriaProgressMap::const_iterator progress = m_criteriaProgress.find(criteriaId);
        if (progress == m_criteriaProgress.end())
            continue;

        ObjectGuid criteriaProgress = ObjectGuid(uint64(progress->second.counter));
        ObjectGuid criteriaGuid = progress->second.CompletedGUID;

        criteriaBits.WriteGuidMask<4, 1>(criteriaProgress);
        criteriaBits.WriteGuidMask<2>(criteriaGuid);
        criteriaBits.WriteGuidMask<3>(criteriaProgress);
        criteriaBits.WriteGuidMask<1>(criteriaGuid);
        criteriaBits.WriteGuidMask<5, 0>(criteriaProgress);
        criteriaBits.WriteGuidMask<3>(criteriaGuid);
        criteriaBits.WriteGuidMask<2>(criteriaProgress);
        criteriaBits.WriteGuidMask<7, 5, 0>(criteriaGuid);
        criteriaBits.WriteGuidMask<6>(criteriaProgress);
        criteriaBits.WriteGuidMask<6>(criteriaGuid);
        criteriaBits.WriteGuidMask<7>(criteriaProgress);
        criteriaBits.WriteGuidMask<4>(criteriaGuid);

        criteriaData.WriteGuidBytes<5>(criteriaGuid);
        criteriaData << uint32(progress->second.date);      // unknown date
        criteriaData.WriteGuidBytes<3, 7>(criteriaProgress);
        criteriaData << uint32(progress->second.date);      // unknown date
        criteriaData.WriteGuidBytes<6>(criteriaProgress);
        criteriaData.WriteGuidBytes<4, 1>(criteriaGuid);
        criteriaData.WriteGuidBytes<4>(criteriaProgress);
        criteriaData.WriteGuidBytes<3>(criteriaGuid);
        criteriaData.WriteGuidBytes<0>(criteriaProgress);
        criteriaData.WriteGuidBytes<2>(criteriaGuid);
        criteriaData.WriteGuidBytes<1>(criteriaProgress);
        criteriaData.WriteGuidBytes<6>(criteriaGuid);
        criteriaData << uint32(progress->second.date);      // last update time (not packed!)
        criteriaData << uint32(criteriaId);
        criteriaData.WriteGuidBytes<5>(criteriaProgress);
        criteriaData << uint32(0);
        criteriaData.WriteGuidBytes<7>(criteriaGuid);
        criteriaData.WriteGuidBytes<2>(criteriaProgress);
        criteriaData.WriteGuidBytes<0>(criteriaGuid);
    }

    criteriaBits.FlushBits();
    WorldPacket data(SMSG_GUILD_CRITERIA_DATA, 3 + criteriaBits.size() + criteriaData.size());
    data.append(criteriaBits);
    if (numCriteria)
        data.append(criteriaData);

    player->GetSession()->SendPacket(&data);
 }

//==========================================================
AchievementCriteriaEntryList const& AchievementGlobalMgr::GetAchievementCriteriaByType(AchievementCriteriaTypes type, bool guild)
{
    return guild ? m_GuildAchievementCriteriasByType[type] : m_AchievementCriteriasByType[type];
}

AchievementCriteriaEntryList const* AchievementGlobalMgr::GetAchievementCriteriaByAchievement(uint32 id)
{
    AchievementCriteriaListByAchievement::const_iterator itr = m_AchievementCriteriaListByAchievement.find(id);
    return itr != m_AchievementCriteriaListByAchievement.end() ? &itr->second : NULL;
}

AchievementEntryList const* AchievementGlobalMgr::GetAchievementByReferencedId(uint32 id) const
{
    AchievementListByReferencedId::const_iterator itr = m_AchievementListByReferencedId.find(id);
    return itr != m_AchievementListByReferencedId.end() ? &itr->second : NULL;
}

AchievementReward const* AchievementGlobalMgr::GetAchievementReward(AchievementEntry const* achievement, uint8 gender) const
{
    AchievementRewardsMapBounds bounds = m_achievementRewards.equal_range(achievement->ID);
    for (AchievementRewardsMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        if (iter->second.gender == GENDER_NONE || uint8(iter->second.gender) == gender)
            return &iter->second;

    return NULL;
}

AchievementRewardLocale const* AchievementGlobalMgr::GetAchievementRewardLocale(AchievementEntry const* achievement, uint8 gender) const
{
    AchievementRewardLocalesMapBounds bounds = m_achievementRewardLocales.equal_range(achievement->ID);
    for (AchievementRewardLocalesMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        if (iter->second.gender == GENDER_NONE || uint8(iter->second.gender) == gender)
            return &iter->second;

    return NULL;
}

AchievementCriteriaRequirementSet const* AchievementGlobalMgr::GetCriteriaRequirementSet(AchievementCriteriaEntry const* achievementCriteria)
{
    AchievementCriteriaRequirementMap::const_iterator iter = m_criteriaRequirementMap.find(achievementCriteria->ID);
    return iter != m_criteriaRequirementMap.end() ? &iter->second : NULL;
}

bool AchievementGlobalMgr::IsRealmCompleted(AchievementEntry const* achievement) const
{
    AllCompletedAchievements::const_iterator itr = m_allCompletedAchievements.find(achievement->ID);
    return itr != m_allCompletedAchievements.end() && time_t(itr->second + 2) < time(NULL);
}

void AchievementGlobalMgr::SetRealmCompleted(AchievementEntry const* achievement)
{
    if (m_allCompletedAchievements.find(achievement->ID) == m_allCompletedAchievements.end())
        m_allCompletedAchievements[achievement->ID] = time(NULL);
}

template class AchievementMgr<Guild>;
template class AchievementMgr<Player>;

void AchievementGlobalMgr::LoadAchievementCriteriaList()
{
    if (sAchievementCriteriaStore.GetNumRows() == 0)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement criteria and 0 guild achievement criterias.");
        return;
    }

    uint32 criterias = 0;
    uint32 guildCriterias = 0;

    BarGoLink bar(sAchievementCriteriaStore.GetNumRows());
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        bar.step();

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        MANGOS_ASSERT(criteria->requiredType < ACHIEVEMENT_CRITERIA_TYPE_TOTAL && "Not updated ACHIEVEMENT_CRITERIA_TYPE_TOTAL?");

        // check if referredAchievement exists!
        AchievementEntry const* achiev = sAchievementStore.LookupEntry(criteria->referredAchievement);
        if (!achiev)
        {
            sLog.outDetail("Removed achievement-criteria %u, because referred achievement does not exist", entryId);
            sAchievementCriteriaStore.EraseEntry(entryId);
            continue;
        }

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
        if (achievement && (achievement->flags & ACHIEVEMENT_FLAG_GUILD))
            ++guildCriterias, m_GuildAchievementCriteriasByType[criteria->requiredType].push_back(criteria);
        else
            ++criterias, m_AchievementCriteriasByType[criteria->requiredType].push_back(criteria);


        m_AchievementCriteriaListByAchievement[criteria->referredAchievement].push_back(criteria);
    }

    sLog.outString();
    sLog.outString(">> Loaded %u achievement criteria and %u guild achievement criterias.", criterias, guildCriterias);
}

void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    if (sAchievementStore.GetNumRows() == 0)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement references.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(sAchievementStore.GetNumRows());
    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        bar.step();

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->refAchievement)
            continue;

        // Check refAchievement exists
        AchievementEntry const* refAchiev = sAchievementStore.LookupEntry(achievement->refAchievement);
        if (!refAchiev)
        {
            sLog.outDetail("Removed achieviement %u, because referred achievement does not exist", entryId);
            sAchievementStore.EraseEntry(entryId);
            continue;
        }

        m_AchievementListByReferencedId[achievement->refAchievement].push_back(achievement);
        ++count;
    }

    sLog.outString();
    sLog.outString(">> Loaded %u achievement references.", count);
}

void AchievementGlobalMgr::LoadAchievementCriteriaRequirements()
{
    m_criteriaRequirementMap.clear();                       // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT criteria_id, type, value1, value2 FROM achievement_criteria_requirement");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 additional achievement criteria data. DB table `achievement_criteria_requirement` is empty.");
        return;
    }

    uint32 count = 0;
    uint32 disabled_count = 0;
    BarGoLink bar(result->GetRowCount());
    do
    {
        bar.step();
        Field* fields = result->Fetch();
        uint32 criteria_id = fields[0].GetUInt32();

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(criteria_id);

        if (!criteria)
        {
            sLog.outErrorDb("Table `achievement_criteria_requirement`.`criteria_id` %u does not exist, ignoring.", criteria_id);
            continue;
        }

        AchievementCriteriaRequirement data(fields[1].GetUInt32(), fields[2].GetUInt32(), fields[3].GetUInt32());

        if (!data.IsValid(criteria))
        {
            continue;
        }

        // this will allocate empty data set storage
        AchievementCriteriaRequirementSet& dataSet = m_criteriaRequirementMap[criteria_id];
        dataSet.SetCriteriaId(criteria_id);

        // counting disable criteria requirements
        if (data.requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED)
            ++disabled_count;

        // add real data only for not NONE requirements
        if (data.requirementType != ACHIEVEMENT_CRITERIA_REQUIRE_NONE)
            dataSet.Add(data);

        // counting requirements
        ++count;
    }
    while (result->NextRow());

    delete result;

    // post loading checks
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        switch (criteria->requiredType)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                if (!criteria->win_bg.additionalRequirement1_type && !criteria->win_bg.additionalRequirement2_type)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
                // Checked in LoadAchievementCriteriaList

                // exist many achievements with this criteria, use at this moment hardcoded check to skil simple case
                switch (achievement->ID)
                {
                    case 31:
                        // case 1275: // these timed achievements are "started" on Quest Accept, and simple ended on quest-complete
                        // case 1276:
                        // case 1277:
                    case 1282:
                    case 1789:
                        break;
                    default:
                        continue;
                }
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:      // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA: // need skip generic cases
                if (criteria->win_rated_arena.flag != ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM: // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:        // need skip generic cases
                if (criteria->do_emote.count == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:// any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:        // skip statistics
                if (criteria->win_duel.duelCount == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:     // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:       // need skip generic cases
                if (criteria->loot_type.lootTypeCount != 1)
                    continue;
                break;
            default:                                        // type not use DB data, ignore
                continue;
        }

        if (!GetCriteriaRequirementSet(criteria))
            sLog.outErrorDb("Table `achievement_criteria_requirement` is missing expected data for `criteria_id` %u (type: %u) for achievement %u.", criteria->ID, criteria->requiredType, criteria->referredAchievement);
    }

    sLog.outString();
    sLog.outString(">> Loaded %u additional achievement criteria data (%u disabled).", count, disabled_count);
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    QueryResult* result = CharacterDatabase.Query("SELECT `achievement`, `date` FROM `character_achievement` GROUP BY `achievement`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 realm completed achievements . DB table `character_achievement` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());
    do
    {
        bar.step();
        Field* fields = result->Fetch();

        uint32 achievement_id = fields[0].GetUInt32();
        if (!sAchievementStore.LookupEntry(achievement_id))
        {
            // we will remove nonexistent achievement for all characters
            sLog.outError("Nonexistent achievement %u data removed from table `character_achievement`.", achievement_id);
            CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE achievement = %u", achievement_id);
            continue;
        }

        m_allCompletedAchievements[achievement_id] = time_t(fields[1].GetUInt64());
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded " SIZEFMTD " realm completed achievements.", m_allCompletedAchievements.size());
}

void AchievementGlobalMgr::LoadRewards()
{
    m_achievementRewards.clear();                           // need for reload case

    //                                                0      1       2        3        4     5       6        7
    QueryResult* result = WorldDatabase.Query("SELECT entry, gender, title_A, title_H, item, sender, subject, text FROM achievement_reward");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        if (!sAchievementStore.LookupEntry(entry))
        {
            sLog.outErrorDb("Table `achievement_reward` has wrong achievement (Entry: %u), ignore", entry);
            continue;
        }

        AchievementReward reward;
        reward.gender     = Gender(fields[1].GetUInt8());
        reward.titleId[0] = fields[2].GetUInt32();
        reward.titleId[1] = fields[3].GetUInt32();
        reward.itemId     = fields[4].GetUInt32();
        reward.sender     = fields[5].GetUInt32();
        reward.subject    = fields[6].GetCppString();
        reward.text       = fields[7].GetCppString();

        if (reward.gender >= MAX_GENDER)
            sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has wrong gender %u.", entry, reward.gender);

        // GENDER_NONE must be single (so or already in and none must be attempt added new data or just adding and none in)
        // other duplicate cases prevented by DB primary key
        bool dup = false;
        AchievementRewardsMapBounds bounds = m_achievementRewards.equal_range(entry);
        for (AchievementRewardsMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        {
            if (iter->second.gender == GENDER_NONE || reward.gender == GENDER_NONE)
            {
                dup = true;
                sLog.outErrorDb("Table `achievement_reward` must have single GENDER_NONE (%u) case (Entry: %u), ignore duplicate case", GENDER_NONE, entry);
                break;
            }
        }
        if (dup)
            continue;

        if ((reward.titleId[0] == 0) != (reward.titleId[1] == 0))
            sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has title (A: %u H: %u) only for one from teams.", entry, reward.titleId[0], reward.titleId[1]);

        // must be title or mail at least
        if (!reward.titleId[0] && !reward.titleId[1] && !reward.sender)
        {
            sLog.outErrorDb("Table `achievement_reward` (Entry: %u) not have title or item reward data, ignore.", entry);
            continue;
        }

        if (reward.titleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[0]);
            if (!titleEntry)
            {
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[0]);
                reward.titleId[0] = 0;
            }
        }

        if (reward.titleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[1]);
            if (!titleEntry)
            {
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[1]);
                reward.titleId[1] = 0;
            }
        }

        // check mail data before item for report including wrong item case
        if (reward.sender)
        {
            if (!ObjectMgr::GetCreatureTemplate(reward.sender))
            {
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has invalid creature entry %u as sender, mail reward skipped.", entry, reward.sender);
                reward.sender = 0;
            }
        }
        else
        {
            if (reward.itemId)
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) not have sender data but have item reward, item will not rewarded", entry);

            if (!reward.subject.empty())
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) not have sender data but have mail subject.", entry);

            if (!reward.text.empty())
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) not have sender data but have mail text.", entry);
        }

        if (reward.itemId)
        {
            if (!ObjectMgr::GetItemPrototype(reward.itemId))
            {
                sLog.outErrorDb("Table `achievement_reward` (Entry: %u) has invalid item id %u, reward mail will be without item.", entry, reward.itemId);
                reward.itemId = 0;
            }
        }

        m_achievementRewards.insert(AchievementRewardsMap::value_type(entry, reward));
        ++count;

    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u achievement rewards", count);
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    m_achievementRewardLocales.clear();                     // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT entry,gender,subject_loc1,text_loc1,subject_loc2,text_loc2,subject_loc3,text_loc3,subject_loc4,text_loc4,subject_loc5,text_loc5,subject_loc6,text_loc6,subject_loc7,text_loc7,subject_loc8,text_loc8,subject_loc9,text_loc9,subject_loc10,text_loc10,subject_loc11,text_loc11 FROM locales_achievement_reward");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 achievement reward locale strings. DB table `locales_achievement_reward` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 entry = fields[0].GetUInt32();

        if (m_achievementRewards.find(entry) == m_achievementRewards.end())
        {
            sLog.outErrorDb("Table `locales_achievement_reward` (Entry: %u) has locale strings for nonexistent achievement reward .", entry);
            continue;
        }

        AchievementRewardLocale data;

        data.gender = Gender(fields[1].GetUInt8());

        if (data.gender >= MAX_GENDER)
            sLog.outErrorDb("Table `locales_achievement_reward` (Entry: %u) has wrong gender %u.", entry, data.gender);

        // GENDER_NONE must be single (so or already in and none must be attempt added new data or just adding and none in)
        // other duplicate cases prevented by DB primary key
        bool dup = false;
        AchievementRewardLocalesMapBounds bounds = m_achievementRewardLocales.equal_range(entry);
        for (AchievementRewardLocalesMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        {
            if (iter->second.gender == GENDER_NONE || data.gender == GENDER_NONE)
            {
                dup = true;
                sLog.outErrorDb("Table `locales_achievement_reward` must have single GENDER_NONE (%u) case (Entry: %u), ignore duplicate case", GENDER_NONE, entry);
                break;
            }
        }
        if (dup)
            continue;

        for (int i = 1; i < MAX_LOCALE; ++i)
        {
            std::string str = fields[2 + 2 * (i - 1)].GetCppString();
            if (!str.empty())
            {
                int idx = sObjectMgr.GetOrNewIndexForLocale(LocaleConstant(i));
                if (idx >= 0)
                {
                    if (data.subject.size() <= size_t(idx))
                        data.subject.resize(idx + 1);

                    data.subject[idx] = str;
                }
            }
            str = fields[2 + 2 * (i - 1) + 1].GetCppString();
            if (!str.empty())
            {
                int idx = sObjectMgr.GetOrNewIndexForLocale(LocaleConstant(i));
                if (idx >= 0)
                {
                    if (data.text.size() <= size_t(idx))
                        data.text.resize(idx + 1);

                    data.text[idx] = str;
                }
            }
        }

        m_achievementRewardLocales.insert(AchievementRewardLocalesMap::value_type(entry, data));

    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded " SIZEFMTD " achievement reward locale strings", m_achievementRewardLocales.size());
}
