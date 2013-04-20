ALTER TABLE `character_currencies`
    ADD COLUMN `customWeekCap` int(11) unsigned NOT NULL DEFAULT '0' AFTER `seasonCount`;