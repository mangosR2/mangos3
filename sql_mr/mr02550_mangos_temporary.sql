-- ------------------------------------- --
-- ------- I am a temporary file ------- --
-- After a month my data will be deleted --
-- ------------------------------------- --
-- ===================================== --

-- ---------- --
-- 2012-11-28 --
-- ---------- --
-- DK Gargoyle mr2481
UPDATE `creature_template` SET `ScriptName` = '' WHERE `entry` = '27829';

-- ---------- --
-- 2012-12-13 --
-- ---------- --
-- mr2523
ALTER TABLE `spell_dbc` CHANGE `ProcCharges` `ProcCharges` INT(10) UNSIGNED NOT NULL DEFAULT 0;

-- mr2550
UPDATE `creature_template` SET `IconName` = 'vehichleCursor' WHERE `IconName` = 'vehicleCursor';
