-- Implement DBC encounters

DROP TABLE IF EXISTS `instance_encounters`;
CREATE TABLE `instance_encounters` (
    `entry` int(10) unsigned NOT NULL COMMENT 'Unique entry from DungeonEncounter.dbc',
    `creditType` tinyint(3) unsigned NOT NULL DEFAULT '0',
    `creditEntry` int(10) unsigned NOT NULL DEFAULT '0',
    `lastEncounterDungeon` smallint(5) unsigned NOT NULL DEFAULT '0' COMMENT 'If not 0, LfgDungeon.dbc entry for the instance it is last encounter in',
    `comment` varchar(255) NOT NULL DEFAULT '',
     PRIMARY KEY (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- LFG dungeon reward structure from TC

DROP TABLE IF EXISTS `lfg_dungeon_rewards`;
CREATE TABLE `lfg_dungeon_rewards` (
    `dungeonId` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Dungeon entry from dbc',
    `maxLevel` tinyint(3) unsigned NOT NULL DEFAULT '0' COMMENT 'Max level at which this reward is rewarded',
    `firstQuestId` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Quest id with rewards for first dungeon this day',
    `firstMoneyVar` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Money multiplier for completing the dungeon first time in a day with less players than max',
    `firstXPVar` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Experience multiplier for completing the dungeon first time in a day with less players than max',
    `otherQuestId` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Quest id with rewards for Nth dungeon this day',
    `otherMoneyVar` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Money multiplier for completing the dungeon Nth time in a day with less players than max',
    `otherXPVar` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Experience multiplier for completing the dungeon Nth time in a day with less players than max',
    PRIMARY KEY (`dungeonId`, `maxLevel`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Pet scaling data table from /dev/rsa

DROP TABLE IF EXISTS `pet_scaling_data`;
CREATE TABLE `pet_scaling_data` (
  `creature_entry` mediumint(8) unsigned NOT NULL,
  `aura` mediumint(8) unsigned NOT NULL default '0',
  `healthbase` mediumint(8) NOT NULL default '0',
  `health` mediumint(8) NOT NULL default '0',
  `powerbase` mediumint(8) NOT NULL default '0',
  `power` mediumint(8) NOT NULL default '0',
  `str` mediumint(8) NOT NULL default '0',
  `agi` mediumint(8) NOT NULL default '0',
  `sta` mediumint(8) NOT NULL default '0',
  `inte` mediumint(8) NOT NULL default '0',
  `spi` mediumint(8) NOT NULL default '0',
  `armor` mediumint(8) NOT NULL default '0',
  `resistance1` mediumint(8) NOT NULL default '0',
  `resistance2` mediumint(8) NOT NULL default '0',
  `resistance3` mediumint(8) NOT NULL default '0',
  `resistance4` mediumint(8) NOT NULL default '0',
  `resistance5` mediumint(8) NOT NULL default '0',
  `resistance6` mediumint(8) NOT NULL default '0',
  `apbase` mediumint(8) NOT NULL default '0',
  `apbasescale` mediumint(8) NOT NULL default '0',
  `attackpower` mediumint(8) NOT NULL default '0',
  `damage` mediumint(8) NOT NULL default '0',
  `spelldamage` mediumint(8) NOT NULL default '0',
  `spellhit` mediumint(8) NOT NULL default '0',
  `hit` mediumint(8) NOT NULL default '0',
  `expertize` mediumint(8) NOT NULL default '0',
  `attackspeed` mediumint(8) NOT NULL default '0',
  `crit` mediumint(8) NOT NULL default '0',
  `regen` mediumint(8) NOT NULL default '0',
  PRIMARY KEY (`creature_entry`, `aura`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Stores pet scaling data (in percent from owner value).';

-- Spell DBC
DROP TABLE IF EXISTS `spell_dbc`;
CREATE TABLE IF NOT EXISTS `spell_dbc` (
  `Id` int(10) unsigned NOT NULL,
  `Category` int(10) unsigned NOT NULL DEFAULT '0',
  `Dispel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Mechanic` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Attributes` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx2` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx3` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx4` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx5` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx6` int(10) unsigned NOT NULL DEFAULT '0',
  `AttributesEx7` int(10) unsigned NOT NULL DEFAULT '0',
  `Stances` int(10) unsigned NOT NULL DEFAULT '0',
  `StancesNot` int(10) unsigned NOT NULL DEFAULT '0',
  `Targets` int(10) unsigned NOT NULL DEFAULT '0',
  `RequiresSpellFocus` int(10) unsigned NOT NULL DEFAULT '0',
  `CasterAuraState` int(10) unsigned NOT NULL DEFAULT '0',
  `TargetAuraState` int(10) NOT NULL DEFAULT '0',
  `CasterAuraStateNot` int(10) unsigned NOT NULL DEFAULT '0',
  `TargetAuraStateNot` int(10) NOT NULL DEFAULT '0',
  `CasterAuraSpell` int(10) unsigned NOT NULL DEFAULT '0',
  `TargetAuraSpell` int(10) unsigned NOT NULL DEFAULT '0',
  `ExcludeCasterAuraSpell` int(10) unsigned NOT NULL DEFAULT '0',
  `ExcludeTargetAuraSpell` int(10) unsigned NOT NULL DEFAULT '0',
  `CastingTimeIndex` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `RecoveryTime` int(10) unsigned NOT NULL DEFAULT '0',
  `CategoryRecoveryTime` int(10) unsigned NOT NULL DEFAULT '0',
  `InterruptFlags` int(10) unsigned NOT NULL DEFAULT '0',
  `AuraInterruptFlags` int(10) unsigned NOT NULL DEFAULT '0',
  `ProcFlags` int(10) unsigned NOT NULL DEFAULT '0',
  `ProcChance` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `ProcCharges` int(10) unsigned NOT NULL DEFAULT '0',
  `MaxLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `BaseLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `SpellLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `DurationIndex` smallint(5) unsigned NOT NULL DEFAULT '0',
  `PowerType` int(10) NOT NULL DEFAULT '0',
  `ManaCost` int(10) NOT NULL DEFAULT '0',
  `ManaCostPerLevel` int(10) NOT NULL DEFAULT '0',
  `ManaPerSecond` int(10) NOT NULL DEFAULT '0',
  `ManaPerSecondPerLevel` int(10) NOT NULL DEFAULT '0',
  `RangeIndex` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `Speed` int(10) NOT NULL DEFAULT '0',
  `StackAmount` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EquippedItemClass` int(11) NOT NULL DEFAULT '-1',
  `EquippedItemSubClassMask` int(11) NOT NULL DEFAULT '0',
  `EquippedItemInventoryTypeMask` int(11) NOT NULL DEFAULT '0',
  `Effect1` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Effect2` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Effect3` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectDieSides1` int(11) NOT NULL DEFAULT '0',
  `EffectDieSides2` int(11) NOT NULL DEFAULT '0',
  `EffectDieSides3` int(11) NOT NULL DEFAULT '0',
  `EffectRealPointsPerLevel1` float NOT NULL DEFAULT '0',
  `EffectRealPointsPerLevel2` float NOT NULL DEFAULT '0',
  `EffectRealPointsPerLevel3` float NOT NULL DEFAULT '0',
  `EffectBasePoints1` int(11) NOT NULL DEFAULT '0',
  `EffectBasePoints2` int(11) NOT NULL DEFAULT '0',
  `EffectBasePoints3` int(11) NOT NULL DEFAULT '0',
  `EffectMechanic1` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectMechanic2` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectMechanic3` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetA1` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetA2` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetA3` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetB1` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetB2` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectImplicitTargetB3` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectRadiusIndex1` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectRadiusIndex2` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectRadiusIndex3` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EffectApplyAuraName1` smallint(5) unsigned NOT NULL DEFAULT '0',
  `EffectApplyAuraName2` smallint(5) unsigned NOT NULL DEFAULT '0',
  `EffectApplyAuraName3` smallint(5) unsigned NOT NULL DEFAULT '0',
  `EffectAmplitude1` int(11) NOT NULL DEFAULT '0',
  `EffectAmplitude2` int(11) NOT NULL DEFAULT '0',
  `EffectAmplitude3` int(11) NOT NULL DEFAULT '0',
  `EffectMultipleValue1` float NOT NULL DEFAULT '0',
  `EffectMultipleValue2` float NOT NULL DEFAULT '0',
  `EffectMultipleValue3` float NOT NULL DEFAULT '0',
  `EffectItemType1` int(11) NOT NULL DEFAULT '0',
  `EffectItemType2` int(11) unsigned NOT NULL DEFAULT '0',
  `EffectMiscValue1` int(11) NOT NULL DEFAULT '0',
  `EffectMiscValue2` int(11) NOT NULL DEFAULT '0',
  `EffectMiscValue3` int(11) NOT NULL DEFAULT '0',
  `EffectMiscValueB1` int(11) NOT NULL DEFAULT '0',
  `EffectMiscValueB2` int(11) NOT NULL DEFAULT '0',
  `EffectMiscValueB3` int(11) NOT NULL DEFAULT '0',
  `EffectTriggerSpell1` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectTriggerSpell2` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectTriggerSpell3` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskA1` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskA2` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskA3` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskB1` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskB2` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskB3` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskC1` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskC2` int(10) unsigned NOT NULL DEFAULT '0',
  `EffectSpellClassMaskC3` int(10) unsigned NOT NULL DEFAULT '0',
  `SpellIconID` int(10) unsigned NOT NULL DEFAULT '0',
  `ManaCostPercentage` int(10) NOT NULL DEFAULT '0',
  `StartRecoveryCategory` int(10) NOT NULL DEFAULT '0',
  `StartRecoveryTime` int(10) NOT NULL DEFAULT '0',
  `MaxTargetLevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `SpellFamilyName` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `SpellFamilyFlags1` int(10) unsigned NOT NULL DEFAULT '0',
  `SpellFamilyFlags2` int(10) unsigned NOT NULL DEFAULT '0',
  `SpellFamilyFlags3` int(10) unsigned NOT NULL DEFAULT '0',
  `MaxAffectedTargets` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `DmgClass` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `PreventionType` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `DmgMultiplier1` float NOT NULL DEFAULT '0',
  `DmgMultiplier2` float NOT NULL DEFAULT '0',
  `DmgMultiplier3` float NOT NULL DEFAULT '0',
  `AreaGroupId` int(11) NOT NULL DEFAULT '0',
  `SchoolMask` int(10) unsigned NOT NULL DEFAULT '0',
  `RuneCostID` int(10) NOT NULL DEFAULT '0',
  `Comment` text NOT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Custom spell.dbc entries';


-- Spell disabled
-- Commit 70d09c64ce0d3263a7e4

DROP TABLE IF EXISTS `spell_disabled`;
CREATE TABLE `spell_disabled` (
    `entry` int(11) unsigned NOT NULL default 0 COMMENT 'spell entry',
    `ischeat_spell` tinyint(3) unsigned NOT NULL default 0 COMMENT 'mark spell as cheat',
    `active` tinyint(3) unsigned NOT NULL default 1 COMMENT 'enable check of this spell',
    PRIMARY KEY (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Disabled Spell System';

-- Vehicle accessory

ALTER TABLE `vehicle_accessory`
    ADD COLUMN `flags` int(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'various flags' AFTER `accessory_entry`,
    ADD COLUMN `offset_x` FLOAT NOT NULL DEFAULT '0' COMMENT 'custom passenger offset X' AFTER `flags`,
    ADD COLUMN `offset_y` FLOAT NOT NULL DEFAULT '0' COMMENT 'custom passenger offset Y' AFTER `offset_x`,
    ADD COLUMN `offset_z` FLOAT NOT NULL DEFAULT '0' COMMENT 'custom passenger offset Z' AFTER `offset_y`,
    ADD COLUMN `offset_o` FLOAT NOT NULL DEFAULT '0' COMMENT 'custom passenger offset O' AFTER `offset_z`;

-- Areatrigger table format change

ALTER TABLE `areatrigger_teleport`
    CHANGE `required_quest_done` `required_quest_done_A` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Alliance quest",
    CHANGE `required_quest_done_heroic` `required_quest_done_heroic_A` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Alliance heroic quest",
    ADD COLUMN `required_quest_done_H` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Horde quest" AFTER `required_quest_done_A`,
    ADD COLUMN `required_quest_done_heroic_H` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Horde heroic quest" AFTER `required_quest_done_heroic_A`,
    ADD COLUMN `minGS` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Min player gear score",
    ADD COLUMN `maxGS` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Max player gear score",
    ADD COLUMN `achiev_id_0` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Required achievement to enter in heroic difficulty",
    ADD COLUMN `achiev_id_1` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Required achievement to enter in extra difficulty",
    ADD COLUMN `combat_mode` int(11) UNSIGNED NOT NULL DEFAULT "0" COMMENT "Possibility for enter while zone in combat";

UPDATE `areatrigger_teleport` SET `required_quest_done_H`=`required_quest_done_A` WHERE `required_quest_done_A` > 0;
UPDATE `areatrigger_teleport` SET `required_quest_done_heroic_H`=`required_quest_done_heroic_A` WHERE `required_quest_done_heroic_A` > 0;

-- Creature on kill Reputation

ALTER TABLE `creature_onkill_reputation`
    ADD COLUMN `ChampioningAura` int(11) unsigned NOT NULL default 0 AFTER `TeamDependent`;

-- Pet levelstats

ALTER TABLE `pet_levelstats`
    ADD `mindmg` MEDIUMINT(11) NOT NULL DEFAULT 0 COMMENT 'Min base damage' AFTER `armor`,
    ADD `maxdmg` MEDIUMINT(11) NOT NULL DEFAULT 0 COMMENT 'Max base damage' AFTER `mindmg`,
    ADD `attackpower` MEDIUMINT(11) NOT NULL DEFAULT 0 COMMENT 'Attack power' AFTER `maxdmg`;

-- Pet spell auras

ALTER TABLE `spell_pet_auras`
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`spell`, `effectId`, `pet`, `aura`);

CREATE TABLE IF NOT EXISTS `creature_spell` (
    `guid`      int(11) unsigned NOT NULL COMMENT 'Unique entry from creature_template',
    `spell`     int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell id from DBC',
    `index`     tinyint(3) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell index',
    `active`    tinyint(3) unsigned NOT NULL DEFAULT '0'COMMENT 'Active state for this spell',
    `disabled`  tinyint(3) unsigned NOT NULL DEFAULT '0' COMMENT 'Boolean state for spell',
    `flags`     int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Spell custom flags',
     PRIMARY KEY (`guid`,`index`,`active`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Creature spells storage';

-- Player race/faction change tables

CREATE TABLE IF NOT EXISTS `player_factionchange_achievements` (
    `alliance_id` int(8) NOT NULL,
    `horde_id` int(8) NOT NULL,
    `CommentA` varchar(255) NOT NULL,
    `CommentH` varchar(255) NOT NULL,
    PRIMARY KEY (`alliance_id`,`horde_id`)
) DEFAULT CHARSET=UTF8;

CREATE TABLE IF NOT EXISTS `player_factionchange_items` (
    `race_A` int(8) NOT NULL DEFAULT '0',
    `alliance_id` int(8) NOT NULL,
    `commentA` varchar(255) DEFAULT NULL,
    `race_H` int(8) NOT NULL DEFAULT '0',
    `horde_id` int(8) NOT NULL,
    `commentH` varchar(255) DEFAULT NULL,
    PRIMARY KEY (`alliance_id`,`horde_id`)
) DEFAULT CHARSET=UTF8;

CREATE TABLE IF NOT EXISTS `player_factionchange_reputations` (
    `race_A` int(8) NOT NULL DEFAULT '0',
    `alliance_id` int(8) NOT NULL,
    `commentA` varchar(255) DEFAULT NULL,
    `race_H` int(8) NOT NULL DEFAULT '0',
    `horde_id` int(8) NOT NULL,
    `commentH` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`alliance_id`,`horde_id`)
) DEFAULT CHARSET=UTF8;

CREATE TABLE IF NOT EXISTS  `player_factionchange_spells` (
    `race_A` int(8) NOT NULL DEFAULT '0',
    `alliance_id` int(8) NOT NULL,
    `commentA` varchar(255) DEFAULT NULL,
    `race_H` int(8) NOT NULL DEFAULT '0',
    `horde_id` int(8) NOT NULL,
    `commentH` varchar(255) DEFAULT NULL,
    PRIMARY KEY (`race_A`,`alliance_id`,`race_H`,`horde_id`)
) DEFAULT CHARSET=UTF8;

CREATE TABLE `player_factionchange_titles` (
  `alliance_id` int(8) NOT NULL,
  `alliance_comment` varchar(255) NOT NULL,
  `horde_id` int(8) NOT NULL,
  `horde_comment` varchar(255) NOT NULL,
  PRIMARY KEY (`alliance_id`,`horde_id`)
) DEFAULT CHARSET=utf8;

CREATE TABLE `player_factionchange_quests` (
    `alliance_id` int(8) NOT NULL,
    `commentA` varchar(255) DEFAULT NULL,
    `horde_id` int(8) NOT NULL,
    `commentH` varchar(255) DEFAULT NULL,
    PRIMARY KEY (`alliance_id`, `horde_id`)
) DEFAULT CHARSET=UTF8;

-- Implement spell linked definitions storage
CREATE TABLE IF NOT EXISTS `spell_linked` (
    `entry`            int(10) unsigned NOT NULL COMMENT 'Spell entry',
    `linked_entry`     int(10) unsigned NOT NULL COMMENT 'Linked spell entry',
    `type`             int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Type of link',
    `effect_mask`      int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'mask of effect (NY)',
    `comment`          varchar(255) NOT NULL DEFAULT '',
     PRIMARY KEY (`entry`,`linked_entry`,`type`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='Linked spells storage';

CREATE TABLE IF NOT EXISTS `worldstate_template` (
    `state_id`         int(10) unsigned NOT NULL COMMENT 'WorldState ID',
    `type`             int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'WorldState type',
    `condition`        int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Default condition (dependent from type)',
    `flags`            int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Default flags of WorldState',
    `default`          int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Default value of WorldState',
    `linked_id`        int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'ID of linked WorldState',
    `ScriptName`       char(64) NOT NULL default '' COMMENT 'Script name for WorldState (FFU)',
    `comment`          varchar(255) NOT NULL DEFAULT '',
    PRIMARY KEY (`state_id`,`type`,`condition`,`linked_id`)
) DEFAULT CHARSET=utf8 PACK_KEYS=0 COMMENT='WorldState templates storage';

CREATE TABLE IF NOT EXISTS `hotfix_data` (
  `entry` int(10) unsigned NOT NULL COMMENT 'Item entry',
  `type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Hotfix type',
  `hotfix_time` datetime NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT 'Hotfix date/time',
  PRIMARY KEY (`entry`,`type`,`hotfix_time`)
) DEFAULT CHARSET=utf8;

-- Extend locales support since client 4.x
-- Add Locale 9 = itIT, Locale 10 = ptBR, Locale 11= ptPT
ALTER TABLE `locales_achievement_reward`
        ADD COLUMN `subject_loc9` VARCHAR(100) NOT NULL DEFAULT '' AFTER `subject_loc8`,
        ADD COLUMN `subject_loc10` VARCHAR(100) NOT NULL DEFAULT '' AFTER `subject_loc9`,
        ADD COLUMN `subject_loc11` VARCHAR(100) NOT NULL DEFAULT '' AFTER `subject_loc10`,
        ADD COLUMN `text_loc9` TEXT AFTER `text_loc8`,
        ADD COLUMN `text_loc10` TEXT AFTER `text_loc9`,
        ADD COLUMN `text_loc11` TEXT AFTER `text_loc10`;

ALTER TABLE `locales_creature`
        ADD COLUMN `name_loc9` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc8`,
        ADD COLUMN `name_loc10` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc9`,
        ADD COLUMN `name_loc11` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc10`,
        ADD COLUMN `subname_loc9` VARCHAR(100) DEFAULT NULL AFTER `subname_loc8`,
        ADD COLUMN `subname_loc10` VARCHAR(100) DEFAULT NULL AFTER `subname_loc9`,
        ADD COLUMN `subname_loc11` VARCHAR(100) DEFAULT NULL AFTER `subname_loc10`;

ALTER TABLE `locales_gameobject`
        ADD COLUMN `name_loc9` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc8`,
        ADD COLUMN `name_loc10` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc9`,
        ADD COLUMN `name_loc11` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc10`,
        ADD COLUMN `castbarcaption_loc9` VARCHAR(100) NOT NULL DEFAULT '' AFTER `castbarcaption_loc8`,
        ADD COLUMN `castbarcaption_loc10` VARCHAR(100) NOT NULL DEFAULT '' AFTER `castbarcaption_loc9`,
        ADD COLUMN `castbarcaption_loc11` VARCHAR(100) NOT NULL DEFAULT '' AFTER `castbarcaption_loc10`;

ALTER TABLE `locales_gossip_menu_option`
        ADD COLUMN `option_text_loc9` TEXT AFTER `option_text_loc8`,
        ADD COLUMN `option_text_loc10` TEXT AFTER `option_text_loc9`,
        ADD COLUMN `option_text_loc11` TEXT AFTER `option_text_loc10`,
        ADD COLUMN `box_text_loc9` TEXT AFTER `box_text_loc8`,
        ADD COLUMN `box_text_loc10` TEXT AFTER `box_text_loc9`,
        ADD COLUMN `box_text_loc11` TEXT AFTER `box_text_loc10`;

ALTER TABLE `locales_item`
        ADD COLUMN `name_loc9` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc8`,
        ADD COLUMN `name_loc10` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc9`,
        ADD COLUMN `name_loc11` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name_loc10`,
        ADD COLUMN `description_loc9` VARCHAR(255) DEFAULT NULL AFTER `description_loc8`,
        ADD COLUMN `description_loc10` VARCHAR(255) DEFAULT NULL AFTER `description_loc9`,
        ADD COLUMN `description_loc11` VARCHAR(255) DEFAULT NULL AFTER `description_loc10`;

ALTER TABLE `locales_npc_text`
        ADD COLUMN `Text0_0_loc9` LONGTEXT AFTER `Text0_0_loc8`,
        ADD COLUMN `Text0_0_loc10` LONGTEXT AFTER `Text0_0_loc9`,
        ADD COLUMN `Text0_0_loc11` LONGTEXT AFTER `Text0_0_loc10`,
        ADD COLUMN `Text0_1_loc9` LONGTEXT AFTER `Text0_1_loc8`,
        ADD COLUMN `Text0_1_loc10` LONGTEXT AFTER `Text0_1_loc9`,
        ADD COLUMN `Text0_1_loc11` LONGTEXT AFTER `Text0_1_loc10`,
        ADD COLUMN `Text1_0_loc9` LONGTEXT AFTER `Text1_0_loc8`,
        ADD COLUMN `Text1_0_loc10` LONGTEXT AFTER `Text1_0_loc9`,
        ADD COLUMN `Text1_0_loc11` LONGTEXT AFTER `Text1_0_loc10`,
        ADD COLUMN `Text1_1_loc9` LONGTEXT AFTER `Text1_1_loc8`,
        ADD COLUMN `Text1_1_loc10` LONGTEXT AFTER `Text1_1_loc9`,
        ADD COLUMN `Text1_1_loc11` LONGTEXT AFTER `Text1_1_loc10`,
        ADD COLUMN `Text2_0_loc9` LONGTEXT AFTER `Text2_0_loc8`,
        ADD COLUMN `Text2_0_loc10` LONGTEXT AFTER `Text2_0_loc9`,
        ADD COLUMN `Text2_0_loc11` LONGTEXT AFTER `Text2_0_loc10`,
        ADD COLUMN `Text2_1_loc9` LONGTEXT AFTER `Text2_1_loc8`,
        ADD COLUMN `Text2_1_loc10` LONGTEXT AFTER `Text2_1_loc9`,
        ADD COLUMN `Text2_1_loc11` LONGTEXT AFTER `Text2_1_loc10`,
        ADD COLUMN `Text3_0_loc9` LONGTEXT AFTER `Text3_0_loc8`,
        ADD COLUMN `Text3_0_loc10` LONGTEXT AFTER `Text3_0_loc9`,
        ADD COLUMN `Text3_0_loc11` LONGTEXT AFTER `Text3_0_loc10`,
        ADD COLUMN `Text3_1_loc9` LONGTEXT AFTER `Text3_1_loc8`,
        ADD COLUMN `Text3_1_loc10` LONGTEXT AFTER `Text3_1_loc9`,
        ADD COLUMN `Text3_1_loc11` LONGTEXT AFTER `Text3_1_loc10`,
        ADD COLUMN `Text4_0_loc9` LONGTEXT AFTER `Text4_0_loc8`,
        ADD COLUMN `Text4_0_loc10` LONGTEXT AFTER `Text4_0_loc9`,
        ADD COLUMN `Text4_0_loc11` LONGTEXT AFTER `Text4_0_loc10`,
        ADD COLUMN `Text4_1_loc9` LONGTEXT AFTER `Text4_1_loc8`,
        ADD COLUMN `Text4_1_loc10` LONGTEXT AFTER `Text4_1_loc9`,
        ADD COLUMN `Text4_1_loc11` LONGTEXT AFTER `Text4_1_loc10`,
        ADD COLUMN `Text5_0_loc9` LONGTEXT AFTER `Text5_0_loc8`,
        ADD COLUMN `Text5_0_loc10` LONGTEXT AFTER `Text5_0_loc9`,
        ADD COLUMN `Text5_0_loc11` LONGTEXT AFTER `Text5_0_loc10`,
        ADD COLUMN `Text5_1_loc9` LONGTEXT AFTER `Text5_1_loc8`,
        ADD COLUMN `Text5_1_loc10` LONGTEXT AFTER `Text5_1_loc9`,
        ADD COLUMN `Text5_1_loc11` LONGTEXT AFTER `Text5_1_loc10`,
        ADD COLUMN `Text6_0_loc9` LONGTEXT AFTER `Text6_0_loc8`,
        ADD COLUMN `Text6_0_loc10` LONGTEXT AFTER `Text6_0_loc9`,
        ADD COLUMN `Text6_0_loc11` LONGTEXT AFTER `Text6_0_loc10`,
        ADD COLUMN `Text6_1_loc9` LONGTEXT AFTER `Text6_1_loc8`,
        ADD COLUMN `Text6_1_loc10` LONGTEXT AFTER `Text6_1_loc9`,
        ADD COLUMN `Text6_1_loc11` LONGTEXT AFTER `Text6_1_loc10`,
        ADD COLUMN `Text7_0_loc9` LONGTEXT AFTER `Text7_0_loc8`,
        ADD COLUMN `Text7_0_loc10` LONGTEXT AFTER `Text7_0_loc9`,
        ADD COLUMN `Text7_0_loc11` LONGTEXT AFTER `Text7_0_loc10`,
        ADD COLUMN `Text7_1_loc9` LONGTEXT AFTER `Text7_1_loc8`,
        ADD COLUMN `Text7_1_loc10` LONGTEXT AFTER `Text7_1_loc9`,
        ADD COLUMN `Text7_1_loc11` LONGTEXT AFTER `Text7_1_loc10`;

ALTER TABLE `locales_page_text`
        ADD COLUMN `Text_loc9` LONGTEXT AFTER `Text_loc8`,
        ADD COLUMN `Text_loc10` LONGTEXT AFTER `Text_loc9`,
        ADD COLUMN `Text_loc11` LONGTEXT AFTER `Text_loc10`;

ALTER TABLE `locales_points_of_interest`
        ADD COLUMN `icon_name_loc9` TEXT NULL AFTER `icon_name_loc8`,
        ADD COLUMN `icon_name_loc10` TEXT NULL AFTER `icon_name_loc9`,
        ADD COLUMN `icon_name_loc11` TEXT NULL AFTER `icon_name_loc10`;

ALTER TABLE `locales_quest`
        ADD COLUMN `Title_loc9` TEXT AFTER `Title_loc8`,
        ADD COLUMN `Title_loc10` TEXT AFTER `Title_loc9`,
        ADD COLUMN `Title_loc11` TEXT AFTER `Title_loc10`,
        ADD COLUMN `Details_loc9` TEXT AFTER `Details_loc8`,
        ADD COLUMN `Details_loc10` TEXT AFTER `Details_loc9`,
        ADD COLUMN `Details_loc11` TEXT AFTER `Details_loc10`,
        ADD COLUMN `Objectives_loc9` TEXT AFTER `Objectives_loc8`,
        ADD COLUMN `Objectives_loc10` TEXT AFTER `Objectives_loc9`,
        ADD COLUMN `Objectives_loc11` TEXT AFTER `Objectives_loc10`,
        ADD COLUMN `OfferRewardText_loc9` TEXT AFTER `OfferRewardText_loc8`,
        ADD COLUMN `OfferRewardText_loc10` TEXT AFTER `OfferRewardText_loc9`,
        ADD COLUMN `OfferRewardText_loc11` TEXT AFTER `OfferRewardText_loc10`,
        ADD COLUMN `RequestItemsText_loc9` TEXT AFTER `RequestItemsText_loc8`,
        ADD COLUMN `RequestItemsText_loc10` TEXT AFTER `RequestItemsText_loc9`,
        ADD COLUMN `RequestItemsText_loc11` TEXT AFTER `RequestItemsText_loc10`,
        ADD COLUMN `EndText_loc9` TEXT AFTER `EndText_loc8`,
        ADD COLUMN `EndText_loc10` TEXT AFTER `EndText_loc9`,
        ADD COLUMN `EndText_loc11` TEXT AFTER `EndText_loc10`,
        ADD COLUMN `CompletedText_loc9` TEXT AFTER `CompletedText_loc8`,
        ADD COLUMN `CompletedText_loc10` TEXT AFTER `CompletedText_loc9`,
        ADD COLUMN `CompletedText_loc11` TEXT AFTER `CompletedText_loc10`,
        ADD COLUMN `ObjectiveText1_loc9` TEXT AFTER `ObjectiveText1_loc8`,
        ADD COLUMN `ObjectiveText1_loc10` TEXT AFTER `ObjectiveText1_loc9`,
        ADD COLUMN `ObjectiveText1_loc11` TEXT AFTER `ObjectiveText1_loc10`,
        ADD COLUMN `ObjectiveText2_loc9` TEXT AFTER `ObjectiveText2_loc8`,
        ADD COLUMN `ObjectiveText2_loc10` TEXT AFTER `ObjectiveText2_loc9`,
        ADD COLUMN `ObjectiveText2_loc11` TEXT AFTER `ObjectiveText2_loc10`,
        ADD COLUMN `ObjectiveText3_loc9` TEXT AFTER `ObjectiveText3_loc8`,
        ADD COLUMN `ObjectiveText3_loc10` TEXT AFTER `ObjectiveText3_loc9`,
        ADD COLUMN `ObjectiveText3_loc11` TEXT AFTER `ObjectiveText3_loc10`,
        ADD COLUMN `ObjectiveText4_loc9` TEXT AFTER `ObjectiveText4_loc8`,
        ADD COLUMN `ObjectiveText4_loc10` TEXT AFTER `ObjectiveText4_loc9`,
        ADD COLUMN `ObjectiveText4_loc11` TEXT AFTER `ObjectiveText4_loc10`,
        ADD COLUMN `PortraitGiverName_loc9` TEXT AFTER `PortraitGiverName_loc8`,
        ADD COLUMN `PortraitGiverName_loc10` TEXT AFTER `PortraitGiverName_loc9`,
        ADD COLUMN `PortraitGiverName_loc11` TEXT AFTER `PortraitGiverName_loc10`,
        ADD COLUMN `PortraitGiverText_loc9` TEXT AFTER `PortraitGiverText_loc8`,
        ADD COLUMN `PortraitGiverText_loc10` TEXT AFTER `PortraitGiverText_loc9`,
        ADD COLUMN `PortraitGiverText_loc11` TEXT AFTER `PortraitGiverText_loc10`,
        ADD COLUMN `PortraitTurnInName_loc9` TEXT AFTER `PortraitTurnInName_loc8`,
        ADD COLUMN `PortraitTurnInName_loc10` TEXT AFTER `PortraitTurnInName_loc9`,
        ADD COLUMN `PortraitTurnInName_loc11` TEXT AFTER `PortraitTurnInName_loc10`,
        ADD COLUMN `PortraitTurnInText_loc9` TEXT AFTER `PortraitTurnInText_loc8`,
        ADD COLUMN `PortraitTurnInText_loc10` TEXT AFTER `PortraitTurnInText_loc9`,
        ADD COLUMN `PortraitTurnInText_loc11` TEXT AFTER `PortraitTurnInText_loc10`;

ALTER TABLE `mangos_string`
        ADD COLUMN `content_loc9` text AFTER `content_loc8`,
        ADD COLUMN `content_loc10` text AFTER `content_loc9`,
        ADD COLUMN `content_loc11` text AFTER `content_loc10`;

ALTER TABLE `creature_ai_texts`
        ADD COLUMN `content_loc9` TEXT AFTER `content_loc8`,
        ADD COLUMN `content_loc10` TEXT AFTER `content_loc9`,
        ADD COLUMN `content_loc11` TEXT AFTER `content_loc10`;

ALTER TABLE `db_script_string`
        ADD COLUMN `content_loc9` TEXT AFTER `content_loc8`,
        ADD COLUMN `content_loc10` TEXT AFTER `content_loc9`,
        ADD COLUMN `content_loc11` TEXT AFTER `content_loc10`;

-- Better naming for quest_locales table
ALTER TABLE `locales_quest`
    CHANGE `PortraitGiverName_loc1` `QuestGiverPortraitName_loc1` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc2` `QuestGiverPortraitName_loc2` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc3` `QuestGiverPortraitName_loc3` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc4` `QuestGiverPortraitName_loc4` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc5` `QuestGiverPortraitName_loc5` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc6` `QuestGiverPortraitName_loc6` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc7` `QuestGiverPortraitName_loc7` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc8` `QuestGiverPortraitName_loc8` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc9` `QuestGiverPortraitName_loc9` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc10` `QuestGiverPortraitName_loc10` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverName_loc11` `QuestGiverPortraitName_loc11` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL;

ALTER TABLE `locales_quest`
    CHANGE `PortraitGiverText_loc1` `QuestGiverPortraitText_loc1` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc2` `QuestGiverPortraitText_loc2` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc3` `QuestGiverPortraitText_loc3` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc4` `QuestGiverPortraitText_loc4` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc5` `QuestGiverPortraitText_loc5` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc6` `QuestGiverPortraitText_loc6` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc7` `QuestGiverPortraitText_loc7` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc8` `QuestGiverPortraitText_loc8` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc9` `QuestGiverPortraitText_loc9` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc10` `QuestGiverPortraitText_loc10` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitGiverText_loc11` `QuestGiverPortraitText_loc11` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL;

ALTER TABLE `locales_quest`
    CHANGE `PortraitTurnInName_loc1` `QuestTurnInPortraitName_loc1` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc2` `QuestTurnInPortraitName_loc2` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc3` `QuestTurnInPortraitName_loc3` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc4` `QuestTurnInPortraitName_loc4` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc5` `QuestTurnInPortraitName_loc5` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc6` `QuestTurnInPortraitName_loc6` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc7` `QuestTurnInPortraitName_loc7` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc8` `QuestTurnInPortraitName_loc8` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc9` `QuestTurnInPortraitName_loc9` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc10` `QuestTurnInPortraitName_loc10` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInName_loc11` `QuestTurnInPortraitName_loc11` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL;

ALTER TABLE `locales_quest`
    CHANGE `PortraitTurnInText_loc1` `QuestTurnInPortraitText_loc1` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc2` `QuestTurnInPortraitText_loc2` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc3` `QuestTurnInPortraitText_loc3` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc4` `QuestTurnInPortraitText_loc4` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc5` `QuestTurnInPortraitText_loc5` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc6` `QuestTurnInPortraitText_loc6` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc7` `QuestTurnInPortraitText_loc7` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc8` `QuestTurnInPortraitText_loc8` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc9` `QuestTurnInPortraitText_loc9` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc10` `QuestTurnInPortraitText_loc10` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL,
    CHANGE `PortraitTurnInText_loc11` `QuestTurnInPortraitText_loc11` TEXT CHARSET utf8 COLLATE utf8_general_ci NULL;

