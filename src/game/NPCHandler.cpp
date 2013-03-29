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
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "UpdateMask.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "Pet.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"

enum StableResultCode
{
    STABLE_ERR_NONE         = 0x00,                         // does nothing, just resets stable states
    STABLE_ERR_MONEY        = 0x01,                         // "you don't have enough money"
    STABLE_INVALID_SLOT     = 0x03,
    STABLE_ERR_STABLE       = 0x06,                         // currently used in most fail cases
    STABLE_SUCCESS_STABLE   = 0x08,                         // stable success
    STABLE_SUCCESS_UNSTABLE = 0x09,                         // unstable/swap success
    STABLE_SUCCESS_BUY_SLOT = 0x0A,                         // buy slot success
    STABLE_ERR_EXOTIC       = 0x0B,                         // "you are unable to control exotic creatures"
    STABLE_ERR_INTERNAL     = 0x0C,
};

void WorldSession::HandleTabardVendorActivateOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTabardVendorActivateOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendTabardVendorActivate(guid);
}

void WorldSession::SendTabardVendorActivate(ObjectGuid guid)
{
    WorldPacket data( MSG_TABARDVENDOR_ACTIVATE, 8 );
    data << ObjectGuid(guid);
    SendPacket(&data);
}

void WorldSession::HandleBankerActivateOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;

    DEBUG_LOG("WORLD: Received CMSG_BANKER_ACTIVATE");

    recv_data >> guid;

    if (!CheckBanker(guid))
        return;

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendShowBank(guid);
}

void WorldSession::SendShowBank(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_BANK, 8);
    data << ObjectGuid(guid);
    SendPacket(&data);
}

void WorldSession::SendShowMailBox(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_MAILBOX, 8);
    data << ObjectGuid(guid);
    SendPacket(&data);
}

void WorldSession::HandleTrainerListOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;

    recv_data >> guid;

    SendTrainerList(guid);
}

void WorldSession::SendTrainerList(ObjectGuid guid)
{
    std::string str = GetMangosString(LANG_NPC_TAINER_HELLO);
    SendTrainerList(guid, str);
}


static void SendTrainerSpellHelper(WorldPacket& data, TrainerSpell const* tSpell, TrainerSpellState state, float fDiscountMod, bool can_learn_primary_prof, uint32 reqLevel)
{
    bool primary_prof_first_rank = false;
    for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (sSpellMgr.IsPrimaryProfessionFirstRankSpell(tSpell->learnedSpell[i]))
        {
            primary_prof_first_rank = true;
            break;
        }
    }

    data << uint32(tSpell->spell);                        // learned spell (or cast-spell in profession case)
    data << uint8(state == TRAINER_SPELL_GREEN_DISABLED ? TRAINER_SPELL_GREEN : state);
    data << uint32(floor(tSpell->spellCost * fDiscountMod));
    data << uint8(reqLevel);
    data << uint32(tSpell->reqSkill);
    data << uint32(tSpell->reqSkillValue);

    bool fill = true;
    for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (tSpell->learnedSpell[i] != 0)
        {
            SpellChainNode const* chain_node = sSpellMgr.GetSpellChainNode(tSpell->learnedSpell[i]);
            data << uint32(!tSpell->IsCastable() && chain_node ? (chain_node->prev ? chain_node->prev : chain_node->req) : 0);
            data << uint32(!tSpell->IsCastable() && chain_node && chain_node->prev ? chain_node->req : 0);
            fill = false;
            break;
        }
    }

    if (fill)
        data << uint32(0) << uint32(0);

    data << uint32(primary_prof_first_rank && can_learn_primary_prof ? 1 : 0);
    // primary prof. learn confirmation dialog
    data << uint32(primary_prof_first_rank ? 1 : 0);    // must be equal prev. field to have learn button in enabled state
}

void WorldSession::SendTrainerList(ObjectGuid guid, const std::string& strTitle)
{
    DEBUG_LOG( "WORLD: SendTrainerList" );

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: SendTrainerList - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // trainer list loaded at check;
    if (!unit->IsTrainerOf(_player,true))
        return;

    CreatureInfo const *ci = unit->GetCreatureInfo();
    if (!ci)
        return;

    TrainerSpellData const* cSpells = unit->GetTrainerSpells();
    TrainerSpellData const* tSpells = unit->GetTrainerTemplateSpells();

    if (!cSpells && !tSpells)
    {
        DEBUG_LOG("WORLD: SendTrainerList - Training spells not found for %s", guid.GetString().c_str());
        return;
    }

    uint32 maxcount = (cSpells ? cSpells->spellList.size() : 0) + (tSpells ? tSpells->spellList.size() : 0);
    uint32 trainer_type = cSpells && cSpells->trainerType ? cSpells->trainerType : (tSpells ? tSpells->trainerType : 0);

    WorldPacket data( SMSG_TRAINER_LIST, 8+4+4+maxcount*38 + strTitle.size()+1);
    data << ObjectGuid(guid);
    data << uint32(trainer_type);
    data << uint32(ci->trainerId);

    size_t count_pos = data.wpos();
    data << uint32(maxcount);

    // reputation discount
    float fDiscountMod = _player->GetReputationPriceDiscount(unit);
    bool can_learn_primary_prof = GetPlayer()->GetFreePrimaryProfessionPoints() > 0;

    uint32 count = 0;

    if (cSpells)
    {
        for(TrainerSpellMap::const_iterator itr = cSpells->spellList.begin(); itr != cSpells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            uint32 reqLevel = 0;
            bool valid = true;
            for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
                if (!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell[i], &reqLevel))
                {
                    valid = false;
                    break;
                }
            if (!valid)
                continue;

            reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);

            TrainerSpellState state = _player->GetTrainerSpellState(tSpell, reqLevel);

            SendTrainerSpellHelper(data, tSpell, state, fDiscountMod, can_learn_primary_prof, reqLevel);

            ++count;
        }
    }

    if (tSpells)
    {
        for(TrainerSpellMap::const_iterator itr = tSpells->spellList.begin(); itr != tSpells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            uint32 reqLevel = 0;
            bool valid = true;
            for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
                if (!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell[i], &reqLevel))
                {
                    valid = false;
                    break;
                }
            if (!valid)
                continue;

            reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);

            TrainerSpellState state = _player->GetTrainerSpellState(tSpell, reqLevel);

            SendTrainerSpellHelper(data, tSpell, state, fDiscountMod, can_learn_primary_prof, reqLevel);

            ++count;
        }
    }

    data << strTitle;

    data.put<uint32>(count_pos,count);
    SendPacket(&data);
}

void WorldSession::HandleTrainerBuySpellOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    uint32 spellId = 0, trainerId = 0;

    recv_data >> guid >> trainerId >> spellId;
    DEBUG_LOG("WORLD: Received opcode CMSG_TRAINER_BUY_SPELL Trainer: %s, learn spell id is: %u", guid.GetString().c_str(), spellId);

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTrainerBuySpellOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    WorldPacket sendData(SMSG_TRAINER_SERVICE, 16);

    uint32 trainState = 2;

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if (!unit->IsTrainerOf(_player, true))
        trainState = 1;

    // check present spell in trainer spell list
    TrainerSpellData const* cSpells = unit->GetTrainerSpells();
    TrainerSpellData const* tSpells = unit->GetTrainerTemplateSpells();

    if (!cSpells && !tSpells)
        trainState = 1;

    // Try find spell in npc_trainer
    TrainerSpell const* trainer_spell = cSpells ? cSpells->Find(spellId) : NULL;

    // Not found, try find in npc_trainer_template
    if (!trainer_spell && tSpells)
        trainer_spell = tSpells->Find(spellId);

    // Not found anywhere, cheating?
    if (!trainer_spell)
        trainState = 1;

    // can't be learn, cheat? Or double learn with lags...
    uint32 reqLevel = 0;
    for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (!_player->IsSpellFitByClassAndRace(trainer_spell->learnedSpell[i], &reqLevel))
        {
            trainState = 1;
            break;
        }

    reqLevel = trainer_spell->isProvidedReqLevel ? trainer_spell->reqLevel : std::max(reqLevel, trainer_spell->reqLevel);
    if (_player->GetTrainerSpellState(trainer_spell, reqLevel) != TRAINER_SPELL_GREEN)
        trainState = 1;

    // apply reputation discount
    uint32 nSpellCost = uint32(floor(trainer_spell->spellCost * _player->GetReputationPriceDiscount(unit)));

    // check money requirement
    if (_player->GetMoney() < nSpellCost && trainState > 1)
        trainState = 0;

    if (trainState != 2)
    {
        sendData << ObjectGuid(guid);
        sendData << uint32(spellId);
        sendData << uint32(trainState);
        SendPacket(&sendData);
    }
    else
    {
        _player->ModifyMoney(-int64(nSpellCost));

        // visual effect on trainer
        WorldPacket data;
        unit->BuildSendPlayVisualPacket(&data, 0xB3, false);
        SendPacket(&data);

        // visual effect on player
        _player->BuildSendPlayVisualPacket(&data, 0x016A, true);
        SendPacket(&data);

        // learn explicitly or cast explicitly
        if (trainer_spell->IsCastable())
            _player->CastSpell(_player, trainer_spell->spell, true);
        else
            _player->learnSpell(spellId, false);

        sendData << ObjectGuid(guid);
        sendData << uint32(spellId);                                // should be same as in packet from client
        sendData << uint32(trainState);
        SendPacket(&sendData);
    }
}

void WorldSession::HandleGossipHelloOpcode(WorldPacket & recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_GOSSIP_HELLO");

    ObjectGuid guid;
    recv_data >> guid;

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: HandleGossipHelloOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    pCreature->StopMoving();

    if (pCreature->isSpiritGuide())
        pCreature->SendAreaSpiritHealerQueryOpcode(_player);

    if (!sScriptMgr.OnGossipHello(_player, pCreature))
    {
        _player->PrepareGossipMenu(pCreature, pCreature->GetCreatureInfo()->GossipMenuId);
        _player->SendPreparedGossip(pCreature);
    }
}

void WorldSession::HandleGossipSelectOptionOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 gossipListId;
    uint32 menuId;
    ObjectGuid guid;
    std::string code;

    recv_data >> guid >> menuId >> gossipListId;

    if (_player->PlayerTalkClass->GossipOptionCoded(gossipListId))
    {
        recv_data >> code;
        DEBUG_LOG("Gossip code: %s", code.c_str());
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    uint32 sender = _player->PlayerTalkClass->GossipOptionSender(gossipListId);
    uint32 action = _player->PlayerTalkClass->GossipOptionAction(gossipListId);

    if (guid.IsAnyTypeCreature())
    {
        Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);

        if (!pCreature)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!sScriptMgr.OnGossipSelect(_player, pCreature, sender, action, code.empty() ? NULL : code.c_str()))
            _player->OnGossipSelect(pCreature, gossipListId, menuId);
    }
    else if (guid.IsGameObject())
    {
        GameObject *pGo = GetPlayer()->GetGameObjectIfCanInteractWith(guid);

        if (!pGo)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!sScriptMgr.OnGossipSelect(_player, pGo, sender, action, code.empty() ? NULL : code.c_str()))
            _player->OnGossipSelect(pGo, gossipListId, menuId);
    }
}

void WorldSession::HandleSpiritHealerActivateOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    ObjectGuid guid;

    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleSpiritHealerActivateOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendSpiritResurrect();
}

void WorldSession::SendSpiritResurrect()
{
    _player->ResurrectPlayer(50, true);

    _player->DurabilityLossAll(0.25f,true);

    // get corpse nearest graveyard
    WorldSafeLocsEntry const *corpseGrave = NULL;
    Corpse *corpse = _player->GetCorpse();
    if(corpse)
        corpseGrave = sObjectMgr.GetClosestGraveYard(
            corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if(corpseGrave)
    {
        WorldSafeLocsEntry const *ghostGrave = sObjectMgr.GetClosestGraveYard(
            _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetMapId(), _player->GetTeam());

        if(corpseGrave != ghostGrave)
            _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation(), TELE_TO_NODELAY);
        // or update at original position
        else
        {
            _player->GetCamera()->UpdateVisibilityForOwner();
            _player->UpdateObjectVisibility();
        }
    }
    // or update at original position
    else
    {
        _player->GetCamera()->UpdateVisibilityForOwner();
        _player->UpdateObjectVisibility();
    }
}

void WorldSession::HandleReturnToGraveyardOpcode(WorldPacket& recv_data)
{
    Corpse *corpse = _player->GetCorpse();
    if (!corpse)
        return;

    WorldSafeLocsEntry const* corpseGrave;

    if (BattleGround* bg = _player->GetBattleGround())
        corpseGrave = bg->GetClosestGraveYard(_player);
    else
        corpseGrave = sObjectMgr.GetClosestGraveYard(corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());

    if (!corpseGrave)
        return;

    _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
}

void WorldSession::HandleBinderActivateOpcode(WorldPacket& recv_data)
{
    ObjectGuid npcGuid;
    recv_data >> npcGuid;

    if(!GetPlayer()->IsInWorld() || !GetPlayer()->isAlive())
        return;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGuid,UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleBinderActivateOpcode - %s not found or you can't interact with him.", npcGuid.GetString().c_str());
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendBindPoint(unit);
}

void WorldSession::SendBindPoint(Creature *npc)
{
    // prevent set homebind to instances in any case
    if(GetPlayer()->GetMap()->Instanceable())
        return;

    // send spell for bind 3286 bind magic
    npc->CastSpell(_player, 3286, true);                    // Bind

    WorldPacket data(SMSG_TRAINER_SERVICE, 16);
    data << npc->GetObjectGuid();
    data << uint32(3286);                                   // Bind
    data << uint32(2);
    SendPacket( &data );

    _player->PlayerTalkClass->CloseGossip();
}

void WorldSession::HandleListStabledPetsOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendStablePet(npcGUID);
}

void WorldSession::SendStablePet( ObjectGuid guid )
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS Send.");

    WorldPacket data(MSG_LIST_STABLED_PETS, 200);           // guess size
    data << guid;

    Pet *pet = _player->GetPet();

    size_t wpos = data.wpos();
    data << uint8(0);                                       // place holder for slot show number

    data << uint8(MAX_PET_STABLES);

    uint8 num = 0;                                          // counter for place holder

    //                                                     0      1   2      3      4     5
    QueryResult* result = CharacterDatabase.PQuery("SELECT owner, id, entry, level, name, actual_slot FROM character_pet WHERE owner = '%u' AND actual_slot >= '%u' AND actual_slot <= '%u' ORDER BY actual_slot",
        _player->GetGUIDLow(), PET_SAVE_AS_CURRENT, PET_SAVE_LAST_STABLE_SLOT);

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            data << uint32(fields[1].GetUInt32());          // petnumber
            data << uint32(fields[2].GetUInt32());          // creature entry
            data << uint32(fields[3].GetUInt32());          // level
            data << fields[4].GetString();                  // name
            data << uint8(fields[5].GetUInt32() < PET_SAVE_FIRST_STABLE_SLOT ? 1 : 3);  // 1 = current, 2/3 = in stable (any from 4,5,... create problems with proper show)

            ++num;
        }
        while (result->NextRow());

        delete result;
    }

    data.put<uint8>(wpos, num);                             // set real data to placeholder
    SendPacket(&data);

    SendStableResult(STABLE_ERR_NONE);
}

void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

bool WorldSession::CheckStableMaster(ObjectGuid guid)
{
    // spell case or GM
    if (guid == GetPlayer()->GetObjectGuid())
    {
        // command case will return only if player have real access to command
        if (!GetPlayer()->HasAuraType(SPELL_AURA_OPEN_STABLE) && !ChatHandler(GetPlayer()).FindCommand("stable"))
        {
            DEBUG_LOG("%s attempt open stable in cheating way.", guid.GetString().c_str());
            return false;
        }
    }
    // stable master case
    else
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_STABLEMASTER))
        {
            DEBUG_LOG("Stablemaster %s not found or you can't interact with him.", guid.GetString().c_str());
            return false;
        }
    }

    return true;
}

void WorldSession::HandleStableRevivePet( WorldPacket &/* recv_data */)
{
    DEBUG_LOG("HandleStableRevivePet: Not implemented");
}

void WorldSession::HandleRepairItemOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_REPAIR_ITEM");

    ObjectGuid npcGuid;
    ObjectGuid itemGuid;
    uint8 guildBank;                                        // new in 2.3.2, bool that means from guild bank money

    recv_data >> npcGuid >> itemGuid >> guildBank;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleRepairItemOpcode - %s not found or you can't interact with him.", npcGuid.GetString().c_str());
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    uint32 TotalCost = 0;
    if (itemGuid)
    {
        DEBUG_LOG("ITEM: %s repair of %s", npcGuid.GetString().c_str(), itemGuid.GetString().c_str());

        Item* item = _player->GetItemByGuid(itemGuid);

        if(item)
            TotalCost= _player->DurabilityRepair(item->GetPos(), true, discountMod, (guildBank > 0));
    }
    else
    {
        DEBUG_LOG("ITEM: %s repair all items", npcGuid.GetString().c_str());

        TotalCost = _player->DurabilityRepairAll(true, discountMod, (guildBank > 0));
    }
    if (guildBank)
    {
        uint32 GuildId = _player->GetGuildId();
        if (!GuildId)
            return;
        Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
        if (!pGuild)
            return;
        pGuild->LogBankEvent(GUILD_BANK_LOG_REPAIR_MONEY, 0, _player->GetGUIDLow(), TotalCost);
        pGuild->SendMoneyInfo(this, _player->GetGUIDLow());
    }
}

void WorldSession::HandleSetPetSlotOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_SET_PET_SLOT");

    uint32 petNumber;
    uint8 newSlot;
    ObjectGuid guid;

    recv_data >> petNumber >> newSlot;
    recv_data.ReadGuidMask<3, 2, 0, 7, 5, 6, 1, 4>(guid);
    recv_data.ReadGuidBytes<5, 3, 1, 7, 4, 0, 6, 2>(guid);

    DEBUG_LOG("PetNumber: %u slot: %u guid: %s", petNumber, newSlot, guid.GetString().c_str());

    if (!_player->isAlive())
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (!CheckStableMaster(guid))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    GetPlayer()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ACTION);

    // find swapped pet slot in stable
    QueryResult *result = CharacterDatabase.PQuery("SELECT actual_slot, entry, id FROM character_pet WHERE owner = '%u' AND id = '%u'",
        _player->GetGUIDLow(), petNumber);
    if (!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field* fields = result->Fetch();

    uint32 slot = PetSaveMode(fields[0].GetUInt32());
    uint32 creatureId = fields[1].GetUInt32();
    uint32 srcId = fields[2].GetUInt32();
    delete result;

    uint32 destId = 0;
    if (result = CharacterDatabase.PQuery("SELECT id FROM character_pet WHERE owner = '%u' AND actual_slot = '%u'",
        _player->GetGUIDLow(), newSlot))
    {
        destId = (*result)[0].GetUInt32();
        delete result;
    }

    if (newSlot < PET_SAVE_FIRST_STABLE_SLOT)
    {
        // check existance of slot
        uint32 slotToSpell[] = { 883, 83242, 83243, 83244, 83245 };
        if (!_player->HasSpell(slotToSpell[newSlot]))
        {
            SendStableResult(STABLE_INVALID_SLOT);
            return;
        }

         CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creatureId);
        if (!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
        {
            // if problem in exotic pet
            if (creatureInfo && creatureInfo->isTameable(true))
                SendStableResult(STABLE_ERR_EXOTIC);
            else
                SendStableResult(STABLE_ERR_STABLE);
            return;
        }
    }

    // unsummon pet if it is it that we are swapping
    if (Pet* pet = _player->GetPet())
        if (pet->m_actualSlot == PetSaveMode(slot) || newSlot == PetSaveMode(slot))
            pet->Unsummon(pet->isAlive() ? PetSaveMode(slot) : PET_SAVE_AS_DELETED, _player);

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("UPDATE `character_pet` SET `slot` = '%u', `actual_slot` = '%u' WHERE `id` = '%u'",
        newSlot, newSlot, srcId);
    if (destId)
        CharacterDatabase.PExecute("UPDATE `character_pet` SET `slot` = '%u', `actual_slot` = '%u' WHERE `id` = '%u'",
            slot, slot, destId);
    CharacterDatabase.CommitTransaction();

    SendStableResult(newSlot < PET_SAVE_FIRST_STABLE_SLOT ? STABLE_SUCCESS_STABLE : STABLE_SUCCESS_UNSTABLE);
}
