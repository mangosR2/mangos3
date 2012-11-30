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
#include "World.h"
#include "ObjectMgr.h"
#include "Log.h"

void WorldSession::SendVoidStorageTransferResult(VoidTransferError result)
{
    WorldPacket data(SMSG_VOID_TRANSFER_RESULT, 4);
    data << uint32(result);
    SendPacket(&data);
}

void WorldSession::HandleVoidStorageUnlock(WorldPacket& recvData)
{
    DEBUG_LOG("WORLD: Received CMSG_VOID_STORAGE_UNLOCK");
    Player* player = GetPlayer();

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<4, 5, 3, 0, 2, 1, 7, 6>(npcGuid);
    recvData.ReadGuidBytes<7, 1, 2, 3, 5, 0, 6, 4>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleVoidStorageUnlock - %s not found or player can't interact with it.", npcGuid.GetString().c_str());
        return;
    }

    if (player->IsVoidStorageUnlocked())
    {
        DEBUG_LOG("WORLD: HandleVoidStorageUnlock - %s tried to unlock void storage a 2nd time.", player->GetGuidStr().c_str());
        return;
    }

    player->ModifyMoney(-int64(VOID_STORAGE_UNLOCK));
    player->UnlockVoidStorage();
}

void WorldSession::HandleVoidStorageQuery(WorldPacket& recvData)
{
    DEBUG_LOG("WORLD: Received CMSG_VOID_STORAGE_QUERY");
    Player* player = GetPlayer();

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<4, 0, 5, 7, 6, 3, 1, 2>(npcGuid);
    recvData.ReadGuidBytes<5, 6, 3, 7, 1, 0, 4, 2>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleVoidStorageQuery - %s not found or player can't interact with it.", npcGuid.GetString().c_str());
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        DEBUG_LOG("WORLD: HandleVoidStorageQuery - %s queried void storage without unlocking it.", player->GetGuidStr().c_str());
        return;
    }

    uint8 count = 0;
    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
        if (player->GetVoidStorageItem(i))
            ++count;

    WorldPacket data(SMSG_VOID_STORAGE_CONTENTS, 2 * count + (14 + 4 + 4 + 4 + 4) * count);

    data.WriteBits(count, 8);

    ByteBuffer itemData((14 + 4 + 4 + 4 + 4) * count);

    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
    {
        VoidStorageItem* item = player->GetVoidStorageItem(i);
        if (!item)
            continue;

        ObjectGuid itemId = ObjectGuid(item->ItemId);
        ObjectGuid creatorGuid = item->CreatorGuid;

        data.WriteGuidMask<3>(creatorGuid);
        data.WriteGuidMask<5>(itemId);
        data.WriteGuidMask<6, 1>(creatorGuid);
        data.WriteGuidMask<1, 3, 6>(itemId);
        data.WriteGuidMask<5, 2>(creatorGuid);
        data.WriteGuidMask<2>(itemId);
        data.WriteGuidMask<4>(creatorGuid);
        data.WriteGuidMask<0, 4, 7>(itemId);
        data.WriteGuidMask<0, 7>(creatorGuid);

        itemData.WriteGuidBytes<3>(creatorGuid);

        itemData << uint32(item->ItemSuffixFactor);

        itemData.WriteGuidBytes<4>(creatorGuid);

        itemData << uint32(i);

        itemData.WriteGuidBytes<0, 6>(itemId);
        itemData.WriteGuidBytes<0, 1>(creatorGuid);

        itemData << uint32(item->ItemRandomPropertyId);

        itemData.WriteGuidBytes<4, 5, 2>(itemId);
        itemData.WriteGuidBytes<2, 6>(creatorGuid);
        itemData.WriteGuidBytes<1, 3>(itemId);
        itemData.WriteGuidBytes<5, 7>(creatorGuid);

        itemData << uint32(item->ItemEntry);

        itemData.WriteGuidBytes<7>(itemId);
    }

    if (!itemData.empty())
    {
        data.FlushBits();
        data.append(itemData);
    }

    SendPacket(&data);
}

void WorldSession::HandleVoidStorageTransfer(WorldPacket& recvData)
{
    DEBUG_LOG("WORLD: Received CMSG_VOID_STORAGE_TRANSFER");
    Player* player = GetPlayer();

    // Read everything

    ObjectGuid npcGuid;
    recvData.ReadGuidMask<1>(npcGuid);

    uint32 countDeposit = recvData.ReadBits(26);

    if (countDeposit > VOID_STORAGE_MAX_DEPOSIT)
    {
        sLog.outError("WORLD: HandleVoidStorageTransfer - %s wants to deposit more than %u items (%u).", player->GetGuidStr().c_str(), VOID_STORAGE_MAX_DEPOSIT, countDeposit);
        recvData.rfinish();
        return;
    }

    std::vector<ObjectGuid> itemGuids(countDeposit);
    for (uint32 i = 0; i < countDeposit; ++i)
        recvData.ReadGuidMask<4, 6, 7, 0, 1, 5, 3, 2>(itemGuids[i]);

    recvData.ReadGuidMask<2, 0, 3, 5, 6, 4>(npcGuid);

    uint32 countWithdraw = recvData.ReadBits(26);

    if (countWithdraw > VOID_STORAGE_MAX_WITHDRAW)
    {
        sLog.outError("WORLD: HandleVoidStorageTransfer - %s wants to withdraw more than %u items (%u).", player->GetGuidStr().c_str(), VOID_STORAGE_MAX_WITHDRAW, countWithdraw);
        recvData.rfinish();
        return;
    }

    std::vector<ObjectGuid> itemIds(countWithdraw);
    for (uint32 i = 0; i < countWithdraw; ++i)
        recvData.ReadGuidMask<4, 7, 1, 0, 2, 3, 5, 6>(itemIds[i]);

    recvData.ReadGuidMask<7>(npcGuid);

    for (uint32 i = 0; i < countDeposit; ++i)
        recvData.ReadGuidBytes<6, 1, 0, 2, 4, 5, 3, 7>(itemGuids[i]);

    recvData.ReadGuidBytes<5, 6>(npcGuid);

    for (uint32 i = 0; i < countWithdraw; ++i)
        recvData.ReadGuidBytes<3, 0, 1, 6, 2, 7, 5, 4>(itemIds[i]);

    recvData.ReadGuidBytes<1, 4, 7, 3, 2, 0>(npcGuid);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleVoidStorageTransfer - %s not found or player can't interact with it.", npcGuid.GetString().c_str());
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        sLog.outError("WORLD: HandleVoidStorageTransfer - %s queried void storage without unlocking it.", player->GetGuidStr().c_str());
        return;
    }

    if (itemGuids.size() > player->GetNumOfVoidStorageFreeSlots())
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_FULL);
        return;
    }

    uint32 freeBagSlots = 0;
    if (itemIds.size() != 0)
    {
        // make this a Player function
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            if (Item* bag = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                freeBagSlots += ((Bag*)bag)->GetFreeSlots();
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
            if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ++freeBagSlots;
    }

    if (itemIds.size() > freeBagSlots)
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
        return;
    }

    if (player->GetMoney() < uint64(itemGuids.size() * VOID_STORAGE_STORE_ITEM))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NOT_ENOUGH_MONEY);
        return;
    }

    for (std::vector<ObjectGuid>::iterator itr = itemGuids.begin(); itr != itemGuids.end(); ++itr)
    {
        Item* item = player->GetItemByGuid(*itr);
        if (!item)
        {
            sLog.outError("WORLD: HandleVoidStorageTransfer - %s wants to deposit an invalid item %s.", player->GetGuidStr().c_str(), itr->GetString().c_str());
            SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INTERNAL_ERROR_1);
            return;
        }

        if (!item->FitsToVoidStorage())
        {
            sLog.outError("WORLD: HandleVoidStorageTransfer - %s wants to deposit an item %s that cannot be put in void storage.", player->GetGuidStr().c_str(), itr->GetString().c_str());
            SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INTERNAL_ERROR_1);
            return;
        }
    }

    std::pair<VoidStorageItem, uint8> depositItems[VOID_STORAGE_MAX_DEPOSIT];
    uint8 depositCount = 0;
    for (std::vector<ObjectGuid>::iterator itr = itemGuids.begin(); itr != itemGuids.end(); ++itr)
    {
        Item* item = player->GetItemByGuid(*itr);

        VoidStorageItem itemVS(sObjectMgr.GenerateVoidStorageItemId(), item->GetEntry(), item->GetOwnerGuid(), item->GetItemRandomPropertyId(), item->GetItemSuffixFactor());

        uint8 slot = player->AddVoidStorageItem(itemVS);

        depositItems[depositCount++] = std::make_pair(itemVS, slot);

        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
    }

    int64 cost = depositCount * VOID_STORAGE_STORE_ITEM;

    player->ModifyMoney(-cost);

    VoidStorageItem withdrawItems[VOID_STORAGE_MAX_WITHDRAW];
    uint8 withdrawCount = 0;
    for (std::vector<ObjectGuid>::iterator itr = itemIds.begin(); itr != itemIds.end(); ++itr)
    {
        uint8 slot;
        VoidStorageItem* itemVS = player->GetVoidStorageItem(*itr, slot);
        if (!itemVS)
        {
            sLog.outError("WORLD: HandleVoidStorageTransfer - %s tried to withdraw an invalid item (id: " UI64FMTD ")", player->GetGuidStr().c_str(), itr->GetRawValue());
            continue;
        }

        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemVS->ItemEntry, 1);
        if (msg != EQUIP_ERR_OK)
        {
            SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
            DEBUG_LOG("WORLD: HandleVoidStorageTransfer - %s couldn't withdraw item id " UI64FMTD " because inventory was full.", player->GetGuidStr().c_str(), itr->GetRawValue());
            return;
        }

        Item* item = player->StoreNewItem(dest, itemVS->ItemEntry, true, itemVS->ItemRandomPropertyId);
        item->SetUInt64Value(ITEM_FIELD_CREATOR, uint64(itemVS->CreatorGuid));
        item->SetBinding(true);
        player->SendNewItem(item, 1, false, false, false);

        withdrawItems[withdrawCount++] = *itemVS;

        player->DeleteVoidStorageItem(slot);
    }

    WorldPacket data(SMSG_VOID_STORAGE_TRANSFER_CHANGES, ((5 + 5 + (7 + 7) * depositCount +
        7 * withdrawCount) / 8) + 7 * withdrawCount + (7 + 7 + 4 * 4) * depositCount);

    data.WriteBits(depositCount, 5);
    data.WriteBits(withdrawCount, 5);

    for (uint8 i = 0; i < depositCount; ++i)
    {
        ObjectGuid itemId = ObjectGuid(depositItems[i].first.ItemId);
        ObjectGuid creatorGuid = depositItems[i].first.CreatorGuid;
        data.WriteGuidMask<7>(creatorGuid);
        data.WriteGuidMask<7, 4>(itemId);
        data.WriteGuidMask<6, 5>(creatorGuid);
        data.WriteGuidMask<3, 5>(itemId);
        data.WriteGuidMask<4, 2, 0, 3, 1>(creatorGuid);
        data.WriteGuidMask<2, 0, 1, 6>(itemId);
    }

    for (uint8 i = 0; i < withdrawCount; ++i)
    {
        ObjectGuid itemId = ObjectGuid(withdrawItems[i].ItemId);
        data.WriteGuidMask<1, 7, 3, 5, 6, 2, 4, 0>(itemId);
    }

    for (uint8 i = 0; i < withdrawCount; ++i)
    {
        ObjectGuid itemId = ObjectGuid(withdrawItems[i].ItemId);
        data.WriteGuidBytes<3, 1, 0, 2, 7, 5, 6, 4>(itemId);
    }

    for (uint8 i = 0; i < depositCount; ++i)
    {
        ObjectGuid itemId = ObjectGuid(depositItems[i].first.ItemId);
        ObjectGuid creatorGuid = depositItems[i].first.CreatorGuid;

        data << uint32(depositItems[i].first.ItemSuffixFactor);

        data.WriteGuidBytes<6, 4>(itemId);
        data.WriteGuidBytes<4>(creatorGuid);
        data.WriteGuidBytes<2>(itemId);
        data.WriteGuidBytes<1, 3>(creatorGuid);
        data.WriteGuidBytes<3>(itemId);
        data.WriteGuidBytes<0>(creatorGuid);
        data.WriteGuidBytes<0>(itemId);
        data.WriteGuidBytes<6>(creatorGuid);
        data.WriteGuidBytes<5>(itemId);
        data.WriteGuidBytes<5, 7>(creatorGuid);

        data << uint32(depositItems[i].first.ItemEntry);

        data.WriteGuidBytes<1>(itemId);

        data << uint32(depositItems[i].second); // slot

        data.WriteGuidBytes<2>(creatorGuid);
        data.WriteGuidBytes<7>(itemId);

        data << uint32(depositItems[i].first.ItemRandomPropertyId);
    }

    SendPacket(&data);

    SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NO_ERROR);
}

void WorldSession::HandleVoidSwapItem(WorldPacket& recvData)
{
    DEBUG_LOG("WORLD: Received CMSG_VOID_SWAP_ITEM");

    Player* player = GetPlayer();
    uint32 newSlot;
    ObjectGuid npcGuid;
    ObjectGuid itemId;

    recvData >> newSlot;

    recvData.ReadGuidMask<2, 4, 0>(npcGuid);
    recvData.ReadGuidMask<2, 6, 5>(itemId);
    recvData.ReadGuidMask<1, 7>(npcGuid);
    recvData.ReadGuidMask<3, 7, 0>(itemId);
    recvData.ReadGuidMask<6, 5, 3>(npcGuid);
    recvData.ReadGuidMask<1, 4>(itemId);

    recvData.ReadGuidBytes<1>(npcGuid);
    recvData.ReadGuidBytes<3, 2, 4>(itemId);
    recvData.ReadGuidBytes<3, 0>(npcGuid);
    recvData.ReadGuidBytes<6, 1>(itemId);
    recvData.ReadGuidBytes<5>(npcGuid);
    recvData.ReadGuidBytes<5>(itemId);
    recvData.ReadGuidBytes<6>(npcGuid);
    recvData.ReadGuidBytes<0>(itemId);
    recvData.ReadGuidBytes<2, 7, 4>(npcGuid);
    recvData.ReadGuidBytes<7>(itemId);

    Creature* unit = player->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleVoidSwapItem - %s not found or player can't interact with it.", npcGuid.GetString().c_str());
        return;
    }

    if (!player->IsVoidStorageUnlocked())
    {
        sLog.outError("WORLD: HandleVoidSwapItem - %s queried void storage without unlocking it.", player->GetGuidStr().c_str());
        return;
    }

    uint8 oldSlot;
    if (!player->GetVoidStorageItem(itemId, oldSlot))
    {
        sLog.outError("WORLD: HandleVoidSwapItem - %s requested swapping an invalid item (slot: %u, itemid: " UI64FMTD ").", player->GetGuidStr().c_str(), newSlot, itemId.GetRawValue());
        return;
    }

    bool usedSrcSlot = player->GetVoidStorageItem(oldSlot) != NULL; // should be always true
    bool usedDestSlot = player->GetVoidStorageItem(newSlot) != NULL;
    ObjectGuid itemIdDest;
    if (usedDestSlot)
        itemIdDest = ObjectGuid(player->GetVoidStorageItem(newSlot)->ItemId);

    if (!player->SwapVoidStorageItem(oldSlot, newSlot))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INTERNAL_ERROR_1);
        return;
    }

    WorldPacket data(SMSG_VOID_ITEM_SWAP_RESPONSE, 1 + (usedSrcSlot + usedDestSlot) * (1 + 7 + 4));

    data.WriteBit(!usedDestSlot);
    data.WriteBit(!usedSrcSlot);

    if (usedSrcSlot)
        data.WriteGuidMask<5, 2, 1, 4, 0, 6, 7, 3>(itemId);

    data.WriteBit(!usedDestSlot);

    if (usedDestSlot)
        data.WriteGuidMask<7, 3, 4, 0, 5, 1, 2, 6>(itemIdDest);

    data.WriteBit(!usedSrcSlot);

    if (usedDestSlot)
        data.WriteGuidBytes<4, 6, 5, 2, 3, 1, 7, 0>(itemIdDest);

    if (usedSrcSlot)
        data.WriteGuidBytes<6, 3, 5, 0, 1, 2, 4, 7>(itemId);

    if (usedDestSlot)
        data << uint32(oldSlot);

    if (usedSrcSlot)
        data << uint32(newSlot);

    SendPacket(&data);
}
