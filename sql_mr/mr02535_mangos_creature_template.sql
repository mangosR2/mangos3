-- Creature template

-- Goregek
UPDATE `creature_template` SET `MinLevel` = 76, `MaxLevel` = 80, `MinLevelHealth` = 4200, `MaxLevelHealth` = 4200, `ExtraFlags` = `ExtraFlags` | 2048 WHERE `entry` = 28214;

DELETE FROM `creature_spell` WHERE `guid` IN (28214);
INSERT INTO `creature_spell` (`guid`, `spell`, `index`) VALUES
(28214, 52743, 0),
(28214, 52744, 1),
(28214, 54188, 2),
(28214, 54178, 3),
(28214, 52748, 4);
-- (28214, 51959, 5);

-- Charged War Golem from Venom
DELETE FROM `creature_spell` WHERE `guid` IN (29005);
INSERT INTO `creature_spell` (`guid`, `spell`, `index`) VALUES
(29005, 61380, 0),
(29005, 47911, 1);
