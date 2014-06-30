
DELETE FROM `gameobject_template` WHERE `entry` IN (402189, 402190, 402191, 402364, 402365, 402366);
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `ScriptName`) VALUES
('402189','0','10122','TWINPEAKS_DWARVEN_GATE_03','','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',''),
('402190','0','10123','TWINPEAKS_DWARVEN_GATE_01','','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',''),
('402191','0','10124','TWINPEAKS_DWARVEN_GATE_02','','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',''),
('402364','0','10442','TWINPEAKS_ORC_GATE_01',    '','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',''),
('402365','0','10443','TWINPEAKS_ORC_GATE_02',    '','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',''),
('402366','0','10444','TWINPEAKS_ORC_GATE_03',    '','','','114','32','1','0','0','0','0','0','0','0','-1','0','0','0','0','-1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','');

UPDATE `creature_template` SET `UnitFlags` = `UnitFlags` | 0x02000000 | 2 WHERE `entry` IN (51128, 51126, 42615, 51130, 51127);

DELETE FROM `battleground_events` WHERE `map` = 726;
INSERT INTO `battleground_events` (`map`, `event1`, `event2`, `description`) VALUES
(726, 0, 0, 'Alliance Flag'),
(726, 1, 0, 'Horde Flag'),
(726, 2, 0, 'Spirit Guides'),
(726, 254, 0, 'doors');

SET @CGUID := 550000;
SET @GGUID := 550000;

DELETE FROM `creature_battleground` WHERE `guid` IN (SELECT guid FROM creature WHERE `map` = 726);

DELETE FROM `creature_battleground` WHERE `guid` BETWEEN @GGUID AND @GGUID + 3;
INSERT INTO `creature_battleground` VALUES
-- ally spirit healers
(@CGUID, 2, 0),
(@CGUID+1, 2, 0),
(@CGUID+2, 2, 0),
(@CGUID+3, 2, 0);


DELETE FROM `creature` WHERE `map` = 726 AND `id` NOT IN (51128, 51126, 42615, 51130, 51127);
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID AND @CGUID+3;
INSERT INTO `creature` (guid, id, map, spawnMask, phaseMask, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, spawndist, currentwaypoint, curhealth, curmana, DeathState, MovementType) VALUES
-- spirit healers
(@CGUID+0, 13116, 726, 1, 1, 0, 0, 2178.08, 332.764, 34.062, 3.26377, 7200, 0, 0, 121750, 3500, 0, 0),
(@CGUID+1, 13117, 726, 1, 1, 0, 0, 1550.53, 211.552, 14.1743, 0.0523599, 7200, 0, 0, 644960, 3500, 0, 0),
(@CGUID+2, 13116, 726, 1, 1, 0, 0, 1879.16, 441.913, -3.91684, 4.18879, 7200, 0, 0, 121750, 3500, 0, 0),
(@CGUID+3, 13117, 726, 1, 65535, 0, 13117, 1817.4, 157.272, 1.8066, 1.6642, 25, 0, 0, 644960, 3500, 0, 0);

DELETE FROM `gameobject_battleground` WHERE `guid` IN (SELECT guid FROM gameobject WHERE `map` = 726);
DELETE FROM `gameobject_battleground` WHERE `guid` BETWEEN @GGUID AND @GGUID + 100;
INSERT INTO `gameobject_battleground` VALUES
-- doors
(@GGUID+0, 254, 0),
(@GGUID+1, 254, 0),
(@GGUID+2, 254, 0),
(@GGUID+3, 254, 0),
(@GGUID+4, 254, 0),
(@GGUID+5, 254, 0),
-- flags
(@GGUID+6, 0, 0),
(@GGUID+7, 1, 0);

-- remove buff spawns
DELETE FROM `gameobject` WHERE `map` = 726 AND `id` IN (179871, 179904, 179905);
-- remove door spawns
DELETE FROM `gameobject` WHERE `map` = 726 AND `id` IN (402191, 402190, 402189, 402364, 402365, 402366);

DELETE FROM `gameobject` WHERE `guid` BETWEEN @GGUID AND @GGUID+100;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
-- doors
(@GGUID+0, 402191, 726, 1, 1, 2115.399, 150.175, 43.526, 2.62, 0.0, 0.0, 0, 0, 86400, 255, 1),
(@GGUID+1, 402190, 726, 1, 1, 2156.803, 220.331, 43.482, 5.76, 0.0, 0.0, 0, 0, 86400, 255, 1),
(@GGUID+2, 402189, 726, 1, 1, 2126.760, 224.051, 43.647, 2.63, 0.0, 0.0, 0, 0, 86400, 255, 1),
(@GGUID+3, 402364, 726, 1, 1, 1556.595, 314.502, 1.223, 3.04, 0.0, 0.0, 0, 0, 86400, 255, 1),
(@GGUID+4, 402365, 726, 1, 1, 1587.415, 319.935, 1.522, 6.20, 0.0, 0.0, 0, 0, 86400, 255, 1),
(@GGUID+5, 402366, 726, 1, 1, 1558.315, 372.709, 1.484, 6.12, 0.0, 0.0, 0, 0, 86400, 255, 1),
-- alliance flag
(@GGUID+6, 179830, 726, 1, 1, 2118.210, 191.621, 44.052, 5.741259, 0.0, 0.0, 0.9996573, 0.02617699, 86400, 255, 1),
-- horde flag
(@GGUID+7, 179831, 726, 1, 1, 1578.380, 344.037, 2.419, 3.055978, 0, 0, 0.008726535, 0.9999619, 86400, 255, 1),
-- buffs
(@GGUID+8, 179871, 726, 1, 1, 1545.402, 304.028, 0.5923, -1.64061, 0, 0, 0.7313537, -0.6819983, 180, 100, 1),
(@GGUID+9, 179871, 726, 1, 1, 2171.279, 222.334, 43.8001, 2.663309, 0, 0, 0.7313537, 0.6819984, 180, 100, 1),
(@GGUID+10, 179904, 726, 1, 1, 1753.957, 242.092, -14.1170, 1.105848, 0, 0, 0.1305263, -0.9914448, 180, 100, 1),
(@GGUID+11, 179904, 726, 1, 1, 1952.121, 383.857, -10.2870, 4.192612, 0, 0, 0.333807, -0.9426414, 180, 100, 1),
(@GGUID+12, 179905, 726, 1, 1, 1934.369, 226.064, -17.0441, 2.499154, 0, 0, 0.5591929, 0.8290376, 180, 100, 1),
(@GGUID+13, 179905, 726, 1, 1, 1725.240, 446.431, -7.8327, 5.709677, 0, 0, 0.9396926, -0.3420201, 180, 100, 1);

DELETE FROM `mangos_string` WHERE `entry` BETWEEN 629 AND 616;
INSERT INTO `mangos_string` (`entry`, `content_default`, `content_loc8`) VALUES
(616, 'The battle for Twin Peaks begins in 1 minute.', 'Битва за Два Пика начнется через 1 минуту.'),
(617, 'The battle for Twin Peaks begins in 30 seconds. Prepare yourselves!', 'Битва за Два Пика начнется через 30 секунд. Приготовьтесь!'),
(618, 'Let the battle for Twin Peaks begin!', 'Битва за Два Пика началась!'),
(619, '$n captured the Horde flag!', '$n $gзахватил:захватила; флаг Орды!'),
(620, '$n captured the Alliance flag!', '$n $gзахватил:захватила; флаг Альянса!'),
(621, 'The Horde flag was dropped by $n!', '$n $gпотерял:потеряла; флаг Орды!'),
(622, 'The Alliance Flag was dropped by $n!', '$n $gпотерял:потеряла; флаг Альянса!'),
(623, 'The Alliance Flag was returned to its base by $n!', '$n $gвернул:вернула; флаг Альянса на свою базу!'),
(624, 'The Horde flag was returned to its base by $n!', '$n $gвернул:вернула; флаг Орды на свою базу!'),
(625, 'The Horde flag was picked up by $n!', '$n $gподобрал:подобрала; флаг Орды!'),
(626, 'The Alliance Flag was picked up by $n!', '$n $gподобрал:подобрала; флаг Альянса!'),
(627, 'The flags are now placed at their bases.', 'Флаги расположены по своим базам.'),
(628, 'The Alliance flag is now placed at its base.', 'Флаг Альянса теперь расположен на своей базе.'),
(629, 'The Horde flag is now placed at its base.', 'Флаг Орды теперь расположен на своей базе.');

INSERT IGNORE INTO `game_graveyard_zone` VALUES
(1750, 5031, 67),
(1749, 5031, 469),
(1726, 5031, 469),
(1727, 5031, 67),
(1729, 5031, 469),
(1728, 5031, 67);

-- Wild Hammering
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` = 14893;
INSERT INTO `achievement_criteria_requirement` (`criteria_id`, `type`, `value1`, `value2`) VALUE
(14893, 2, 0, 3),
(14893, 6, 5031, 0);

-- I'm in the Black Lodge
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` = 14979;
INSERT INTO `achievement_criteria_requirement` (`criteria_id`, `type`, `value1`, `value2`) VALUE
(14979, 6, 5681, 0),
(14979, 7, 23333, 0);

-- Twin Peaks Mountaineer
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` IN (14887, 14888);
INSERT INTO `achievement_criteria_requirement` (`criteria_id`, `type`, `value1`, `value2`) VALUE
(14887, 5, 23451, 0),
-- (14887, 5, 23505, 0),   -- Two criteria_ids with different value???
(14888, 11, 0, 0);

-- Fire, Walk With Me
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` = 14831;
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` = 14832;
INSERT INTO `achievement_criteria_requirement` (`criteria_id`, `type`, `value1`, `value2`) VALUE
(14831, 6, 5031, 0),
(14832, 6, 5031, 0);
