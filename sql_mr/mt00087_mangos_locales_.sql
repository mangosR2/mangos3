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
