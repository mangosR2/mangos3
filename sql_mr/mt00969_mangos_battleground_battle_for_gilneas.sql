-- set non-attackable and not-selectable flag to cosmetic spawns
UPDATE `creature_template` SET `unit_flags` = `unit_flags` | 0x02000000 | 2 WHERE `entry` IN
(41911, 41913, 41914, 41915, 42053);

DELETE FROM `battleground_events` WHERE `map` = 761;
INSERT INTO `battleground_events` (`map`, `event1`, `event2`, `description`) VALUES
(761, 0, 0, 'Lighthouse - neutral'),
(761, 0, 1, 'Lighthouse - alliance contested'),
(761, 0, 2, 'Lighthouse - horde contested'),
(761, 0, 3, 'Lighthouse - alliance owned'),
(761, 0, 4, 'Lighthouse - horde owned'),
(761, 1, 0, 'Waterworks - neutral'),
(761, 1, 1, 'Waterworks - alliance contested'),
(761, 1, 2, 'Waterworks - horde contested'),
(761, 1, 3, 'Waterworks - alliance owned'),
(761, 1, 4, 'Waterworks - horde owned'),
(761, 2, 0, 'Mine - neutral'),
(761, 2, 1, 'Mine - alliance contested'),
(761, 2, 2, 'Mine - horde contested'),
(761, 2, 3, 'Mine - alliance owned'),
(761, 2, 4, 'Mine - horde owned'),
(761, 254, 0, 'doors');

SET @CGUID := 708000;
SET @GGUID := 708000;

DELETE FROM `creature_battleground` WHERE `guid` IN (SELECT guid FROM creature WHERE `map` = 761);

-- mine horde creatures
INSERT INTO `creature_battleground` SELECT `guid`, 2, 4 FROM `creature` WHERE position_x > 1150 AND map = 761 AND id IN (41911, 41914, 42053);
-- ww horde creatures
INSERT INTO `creature_battleground` SELECT `guid`, 1, 4 FROM `creature` WHERE position_x < 1067 AND position_y < 1097 AND map = 761 AND id IN (41911, 41914, 42053);
-- lh horde creatures
INSERT INTO `creature_battleground` SELECT `guid`, 0, 4 FROM `creature` WHERE position_x > 1000 AND position_y > 1200 AND map = 761 AND id IN (41911, 41914, 42053);

-- mine ally creatures
INSERT INTO `creature_battleground` SELECT `guid`, 2, 3 FROM `creature` WHERE position_x > 1150 AND map = 761 AND id IN (41913, 41915);
-- ww ally creatures
INSERT INTO `creature_battleground` SELECT `guid`, 1, 3 FROM `creature` WHERE position_x < 1067 AND position_y < 1097 AND map = 761 AND id IN (41913, 41915);
-- lh ally creatures
INSERT INTO `creature_battleground` SELECT `guid`, 0, 3 FROM `creature` WHERE position_x > 1000 AND position_y > 1200 AND map = 761 AND id IN (41913, 41915);

DELETE FROM `creature_battleground` WHERE `guid` BETWEEN @GGUID AND @GGUID + 100;
INSERT INTO `creature_battleground` VALUES
-- ally spirit healers
(@CGUID, 2, 3),
(@CGUID+1, 1, 3),
-- (@CGUID+2, 254, 0), -- alliance base, must not have data
(@CGUID+3, 0, 3),
-- horde spirit healers
-- (@CGUID+4, 254, 0), -- horde base, must not have data
(@CGUID+5, 2, 4),
(@CGUID+6, 0, 4),
(@CGUID+7, 1, 4),

(@CGUID+8, 2, 4),
(@CGUID+9, 0, 4),
(@CGUID+10, 1, 4),

(@CGUID+11, 2, 3),
(@CGUID+12, 0, 3),
(@CGUID+13, 1, 3);


DELETE FROM `creature` WHERE `map` = 761 AND `id` NOT IN (14848, 41911, 41913, 41914, 41915, 42053);
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID AND @CGUID+13;
INSERT INTO `creature` (guid, id, map, spawnMask, phaseMask, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, spawndist, currentwaypoint, curhealth, curmana, DeathState, MovementType) VALUES
-- ally spirit healers
(@CGUID+0, 13116, 761, 1, 65535, 0, 13116, 1252.15, 831.632, 27.7908, 1.50268, 25, 0, 0, 121750, 3500, 0, 0),
(@CGUID+1, 13116, 761, 1, 65535, 0, 13116, 886.229, 936.559, 24.4162, 0.438406, 25, 0, 0, 121750, 3500, 0, 0),
(@CGUID+2, 13116, 761, 1, 65535, 0, 13116, 908, 1338, 27.5597, 6.12083, 25, 0, 0, 121750, 3500, 0, 0),
(@CGUID+3, 13116, 761, 1, 65535, 0, 13116, 1035.86, 1340.82, 11.5059, 4.78172, 25, 0, 0, 121750, 3500, 0, 0),
-- horde spirit healers
(@CGUID+4, 13117, 761, 1, 65535, 0, 13117, 1401, 977, 7.43812, 3.15988, 25, 0, 0, 644960, 3500, 0, 0),
(@CGUID+5, 13117, 761, 1, 65535, 0, 13117, 1252.15, 831.632, 27.7908, 1.50268, 25, 0, 0, 644960, 3500, 0, 0),
(@CGUID+6, 13117, 761, 1, 65535, 0, 13117, 1035.86, 1340.82, 11.5059, 4.78172, 25, 0, 0, 644960, 3500, 0, 0),
(@CGUID+7, 13117, 761, 1, 65535, 0, 13117, 886.229, 936.559, 24.4162, 0.438406, 25, 0, 0, 644960, 3500, 0, 0),
-- honorable defender mine horde
(@CGUID+8, 20212, 761, 1, 1, 15435, 0, 1250.78, 958.878, 6.39886, 0, 7200, 0, 0, 4120, 0, 0, 0),
-- honorable defender lh horde
(@CGUID+9, 20212, 761, 1, 1, 15435, 0, 1058.785, 1278.43, 3.629, 0, 7200, 0, 0, 4120, 0, 0, 0),
-- honorable defender ww horde
(@CGUID+10, 20212, 761, 1, 1, 15435, 0, 981.079, 948.93, 13.1725, 0, 7200, 0, 0, 4120, 0, 0, 0),
-- honorable defender mine ally
(@CGUID+11, 20213, 761, 1, 1, 15435, 0, 1250.78, 958.878, 6.39886, 0, 7200, 0, 0, 4120, 0, 0, 0),
-- honorable defender lh ally
(@CGUID+12, 20213, 761, 1, 1, 15435, 0, 1058.785, 1278.43, 3.629, 0, 7200, 0, 0, 4120, 0, 0, 0),
-- honorable defender ww ally
(@CGUID+13, 20213, 761, 1, 1, 15435, 0, 981.079, 948.93, 13.1725, 0, 7200, 0, 0, 4120, 0, 0, 0);

DELETE FROM `gameobject_battleground` WHERE `guid` IN (SELECT guid FROM gameobject WHERE `map` = 761);
DELETE FROM `gameobject_battleground` WHERE `guid` BETWEEN @GGUID AND @GGUID + 100;
INSERT INTO `gameobject_battleground` VALUES
(@GGUID, 254, 0),
(@GGUID+1, 254, 0),
(@GGUID+2, 2, 0),
(@GGUID+3, 0, 0),
(@GGUID+4, 1, 0),

(@GGUID+5, 1, 4),
(@GGUID+6, 0, 4),
(@GGUID+7, 2, 4),

(@GGUID+8, 1, 3),
(@GGUID+9, 0, 3),
(@GGUID+10, 2, 3),

(@GGUID+11, 0, 0),
(@GGUID+12, 2, 0),
(@GGUID+13, 1, 0),

(@GGUID+14, 0, 3),
(@GGUID+15, 1, 3),
(@GGUID+16, 2, 3),

(@GGUID+17, 1, 4),
(@GGUID+18, 0, 4),
(@GGUID+19, 2, 4),

(@GGUID+20, 0, 1),
(@GGUID+21, 1, 1),
(@GGUID+22, 2, 1),

(@GGUID+23, 2, 2),
(@GGUID+24, 0, 2),
(@GGUID+25, 1, 2);

98322

-- remove buff spawns
DELETE FROM `gameobject` WHERE `map` = 761 AND `id` IN (180146, 180147, 180380, 180381, 180382, 180383);
-- remove incorrect banner spawns
DELETE FROM `gameobject` WHERE `map` = 761 AND `id` IN (180102, 180101, 180100, 208779, 208782, 208785, 208673, 208748, 208763, 208733);
DELETE FROM `gameobject` WHERE `map` = 761 AND `id` IN (208730, 208718, 208721, 208727, 208742, 208754, 208760, 208766, 208769);
-- remove door spawns
DELETE FROM `gameobject` WHERE `map` = 761 AND `id` IN (207177, 207178);

DELETE FROM `gameobject` WHERE `guid` BETWEEN @GGUID AND @GGUID+100;
INSERT INTO `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
-- doors
(@GGUID+0, 207177, 761, 1, 1, 918.391, 1336.64, 27.4252, 0.0, 0.0, 0.0, 0.989016, 0.147811, 86400, 255, 1),
(@GGUID+1, 207178, 761, 1, 1, 1396.15, 977.014, 7.43169, 6.27043, 0.0, 0.0, 0.006378, -0.99998, 86400, 255, 1),
-- mine neutral aura
(@GGUID+2, 180102, 761, 1, 1, 1250.99, 958.311, 5.66513, 0, 0, 0, 0.981627, 0.190812, 7200, 255, 1),
-- lh neutral aura
(@GGUID+3, 180102, 761, 1, 1, 1057.91, 1278.31, 3.22758, 0, 0, 0, -0.608761, 0.793354, 7200, 255, 1),
-- ww neutral aura
(@GGUID+4, 180102, 761, 1, 1, 980.038, 948.703, 12.7478, 0, 0, 0, -0.190808, 0.981627, 7200, 255, 1),
-- ww horde aura
(@GGUID+5, 180101, 761, 1, 1, 980.049, 948.811, 12.7478, 0, 0, 0, -0.199367, 0.979925, 7200, 255, 1),
-- lh horde aura
(@GGUID+6, 180101, 761, 1, 1, 1057.91, 1278.32, 3.25759, 0, 0, 0, -0.608761, 0.793354, 7200, 255, 1),
-- mine horde aura
(@GGUID+7, 180101, 761, 1, 1, 1251.04, 958.29, 5.71601, 0, 0, 0, 0.979924, 0.19937, 7200, 255, 1),
-- ww ally aura
(@GGUID+8, 180100, 761, 1, 1, 980.049, 948.811, 12.7478, 0, 0, 0, -0.199367, 0.979925, 7200, 255, 1),
-- lh ally aura
(@GGUID+9, 180100, 761, 1, 1, 1057.91, 1278.32, 3.25759, 0, 0, 0, -0.608761, 0.793354, 7200, 255, 1),
-- mine ally aura
(@GGUID+10, 180100, 761, 1, 1, 1251.04, 958.29, 5.71601, 0, 0, 0, 0.979924, 0.19937, 7200, 255, 1),
-- lh banner neutral
(@GGUID+11, 208779, 761, 1, 1, 1057.7800, 1278.26001, 3.1924, 1.86482, 0, 0, 0.803, 0.596, 7200, 255, 1),
-- mine banner neutral
(@GGUID+12, 208782, 761, 1, 1, 1251.02, 958.359, 5.67407, 0, 0, 0, 0.983254, 0.182238, 7200, 255, 1),
-- ww banner neutral
(@GGUID+13, 208785, 761, 1, 1, 980.08, 948.707, 12.7478, 0, 0, 0, 0.979924, 0.19937, 7200, 255, 1),
-- lh ally banner
(@GGUID+14, 208673, 761, 1, 1, 1057.92, 1278.31, 3.32431, 0, 0, 0, 0.798635, 0.601815, 7200, 255, 1),
-- ww ally banner
(@GGUID+15, 208673, 761, 1, 1, 980.08, 948.707, 12.7478, 0, 0, 0, 0.979924, 0.19937, 7200, 255, 1),
-- mine ally banner
(@GGUID+16, 208673, 761, 1, 1, 1251.02, 958.359, 5.67407, 0, 0, 0, 0.983254, 0.182238, 7200, 255, 1),
-- ww horde banner
(@GGUID+17, 208748, 761, 1, 1, 980.049, 948.811, 12.7478, 0, 0, 0, -0.199367, 0.979925, 7200, 255, 1),
-- lh horde banner
(@GGUID+18, 208748, 761, 1, 1, 1057.7800, 1278.26001, 3.1924, 1.86482, 0, 0, 0.803, 0.596, 7200, 255, 1),
-- mine horde banner
(@GGUID+19, 208748, 761, 1, 1, 1251.02, 958.359, 5.67407, 0, 0, 0, 0.983254, 0.182238, 7200, 255, 1),
-- lh ally contested banner
(@GGUID+20, 208763, 761, 1, 1, 1057.91, 1278.31, 3.22758, 0, 0, 0, -0.608761, 0.793354, 7200, 255, 1),
-- ww ally contested banner
(@GGUID+21, 208763, 761, 1, 1, 980.08, 948.707, 12.7478, 0, 0, 0, 0.979924, 0.19937, 7200, 255, 1),
-- mine ally contested banner
(@GGUID+22, 208763, 761, 1, 1, 1251.02, 958.359, 5.67407, 0, 0, 0, 0.983254, 0.182238, 7200, 255, 1),
-- mine cont horde
(@GGUID+23, 208733, 761, 1, 1, 1250.99, 958.311, 5.66513, 0, 0, 0, 0.981627, 0.190812, 7200, 255, 1),
-- lh cont horde
(@GGUID+24, 208733, 761, 1, 1, 1057.7800, 1278.26001, 3.1924, 1.86482, 0, 0, 0.803, 0.596, 7200, 255, 1),
-- ww cont horde
(@GGUID+25, 208733, 761, 1, 1, 980.049, 948.811, 12.7478, 0, 0, 0, -0.199367, 0.979925, 7200, 255, 1);

DELETE FROM `mangos_string` WHERE `entry` IN (756, 757, 758, 830, 831, 832, 833, 834, 835);
INSERT INTO `mangos_string` (`entry`, `content_default`, `content_loc8`) VALUES
(756, 'The Battle for Gilneas begins in 1 minute.', 'Битва за Гилнеас начнется через 1 минуту.'),
(757, 'The Battle for Gilneas begins in 30 seconds. Prepare yourselves!', 'Битва за Гилнеас начнется через 30 секунд.'),
(758, 'The Battle for Gilneas has begun!', 'Битва за Гилнеас началась!'),
(832, 'lighthouse', 'маяк'),
(830, 'waterworks', 'водокачку'),
(831, 'mine', 'рудник'),
(833, '$n claims the %s! If left unchallenged, the %s will control it in 1 minute!', '$n атакует %s! Если не помешать, %s захватит эту точку через 1 минуту!'),
(834, '$n has assaulted the %s.', '$n нападает на %s.'),
(835, 'The %s has taken the %s.', '%s захватывает %s.');

-- correct reputation
-- check buff spawns

DELETE FROM `playercreateinfo_spell` WHERE `spell` = 98322;
INSERT INTO `playercreateinfo_spell` (`race`, `class`, `Spell`, `Note`) VALUES
(1, 1, 98322, 'Capturing'),
(2, 1, 98322, 'Capturing'),
(3, 1, 98322, 'Capturing'),
(4, 1, 98322, 'Capturing'),
(5, 1, 98322, 'Capturing'),
(6, 1, 98322, 'Capturing'),
(7, 1, 98322, 'Capturing'),
(8, 1, 98322, 'Capturing'),
(9, 1, 98322, 'Capturing'),
(10, 1, 98322, 'Capturing'),
(11, 1, 98322, 'Capturing'),
(12, 1, 98322, 'Capturing'),

(1, 2, 98322, 'Capturing'),
(3, 2, 98322, 'Capturing'),
(6, 2, 98322, 'Capturing'),
(10, 2, 98322, 'Capturing'),
(11, 2, 98322, 'Capturing'),

(1, 3, 98322, 'Capturing'),
(2, 3, 98322, 'Capturing'),
(3, 3, 98322, 'Capturing'),
(4, 3, 98322, 'Capturing'),
(5, 3, 98322, 'Capturing'),
(6, 3, 98322, 'Capturing'),
(8, 3, 98322, 'Capturing'),
(9, 3, 98322, 'Capturing'),
(10, 3, 98322, 'Capturing'),
(11, 3, 98322, 'Capturing'),
(22, 3, 98322, 'Capturing'),

(1, 4, 98322, 'Capturing'),
(2, 4, 98322, 'Capturing'),
(3, 4, 98322, 'Capturing'),
(4, 4, 98322, 'Capturing'),
(5, 4, 98322, 'Capturing'),
(7, 4, 98322, 'Capturing'),
(8, 4, 98322, 'Capturing'),
(9, 4, 98322, 'Capturing'),
(10, 4, 98322, 'Capturing'),
(22, 4, 98322, 'Capturing'),

(1, 5, 98322, 'Capturing'),
(3, 5, 98322, 'Capturing'),
(4, 5, 98322, 'Capturing'),
(5, 5, 98322, 'Capturing'),
(6, 5, 98322, 'Capturing'),
(7, 5, 98322, 'Capturing'),
(8, 5, 98322, 'Capturing'),
(9, 5, 98322, 'Capturing'),
(10, 5, 98322, 'Capturing'),
(22, 5, 98322, 'Capturing'),

(1, 6, 98322, 'Capturing'),
(2, 6, 98322, 'Capturing'),
(3, 6, 98322, 'Capturing'),
(4, 6, 98322, 'Capturing'),
(5, 6, 98322, 'Capturing'),
(6, 6, 98322, 'Capturing'),
(7, 6, 98322, 'Capturing'),
(8, 6, 98322, 'Capturing'),
(9, 6, 98322, 'Capturing'),
(10, 6, 98322, 'Capturing'),
(11, 6, 98322, 'Capturing'),
(22, 6, 98322, 'Capturing'),

(2, 7, 98322, 'Capturing'),
(3, 7, 98322, 'Capturing'),
(6, 7, 98322, 'Capturing'),
(8, 7, 98322, 'Capturing'),
(9, 7, 98322, 'Capturing'),
(11, 7, 98322, 'Capturing'),

(1, 8, 98322, 'Capturing'),
(2, 8, 98322, 'Capturing'),
(3, 8, 98322, 'Capturing'),
(4, 8, 98322, 'Capturing'),
(5, 8, 98322, 'Capturing'),
(7, 8, 98322, 'Capturing'),
(8, 8, 98322, 'Capturing'),
(9, 8, 98322, 'Capturing'),
(10, 8, 98322, 'Capturing'),
(11, 8, 98322, 'Capturing'),
(22, 8, 98322, 'Capturing'),

(1, 9, 98322, 'Capturing'),
(2, 9, 98322, 'Capturing'),
(3, 9, 98322, 'Capturing'),
(5, 9, 98322, 'Capturing'),
(7, 9, 98322, 'Capturing'),
(8, 9, 98322, 'Capturing'),
(9, 9, 98322, 'Capturing'),
(10, 9, 98322, 'Capturing'),
(22, 9, 98322, 'Capturing'),

(4, 11, 98322, 'Capturing'),
(6, 11, 98322, 'Capturing'),
(8, 11, 98322, 'Capturing'),
(22, 11, 98322, 'Capturing');

-- Double Rainbow
DELETE FROM `achievement_criteria_requirement` WHERE `criteria_id` = 14989;
INSERT INTO `achievement_criteria_requirement` (`criteria_id`, `type`, `value1`, `value2`) VALUE
(14989, 6, 5683, 0);
