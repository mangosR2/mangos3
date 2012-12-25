SET @ConditionEntry := 30000;
REPLACE INTO `conditions` (`condition_entry`, `type`, `value1`, `value2`) VALUES
(@ConditionEntry    , 18, 0, 0), -- INSTANCE_CONDITION_ID_NORMAL_MODE
(@ConditionEntry + 1, 18, 1, 0), -- INSTANCE_CONDITION_ID_HARD_MODE
(@ConditionEntry + 2, 18, 2, 0), -- INSTANCE_CONDITION_ID_HARD_MODE_2
(@ConditionEntry + 3, 18, 3, 0), -- INSTANCE_CONDITION_ID_HARD_MODE_3
(@ConditionEntry + 4, 18, 4, 0), -- INSTANCE_CONDITION_ID_HARD_MODE_4
(@ConditionEntry + 5, 18, 5, 0), -- INSTANCE_CONDITION_ID_TEAM_HORDE
(@ConditionEntry + 6, 18, 6, 0); -- INSTANCE_CONDITION_ID_TEAM_ALLIANCE

/*
For use conditions system for limiting loot by SD2 always use:
@ConditionEntry     for INSTANCE_CONDITION_ID_NORMAL_MODE
@ConditionEntry + 1 for INSTANCE_CONDITION_ID_HARD_MODE
@ConditionEntry + 2 for INSTANCE_CONDITION_ID_HARD_MODE_2
@ConditionEntry + 3 for INSTANCE_CONDITION_ID_HARD_MODE_3
@ConditionEntry + 4 for INSTANCE_CONDITION_ID_HARD_MODE_4
@ConditionEntry + 5 for INSTANCE_CONDITION_ID_TEAM_HORDE
@ConditionEntry + 6 for INSTANCE_CONDITION_ID_TEAM_ALLIANCE
*/
-- -------------------- --
-- XT-002 Deconstructor --
-- -------------------- --
-- 10 Player
UPDATE `creature_loot_template` SET `condition_id` = @ConditionEntry     WHERE `entry` = 33293 AND `groupid` IN (1, 2); -- normal mode
UPDATE `creature_loot_template` SET `condition_id` = @ConditionEntry + 1 WHERE `entry` = 33293 AND `groupid` IN (3); -- hard mode
-- 25 Player
UPDATE `creature_loot_template` SET `condition_id` = @ConditionEntry     WHERE `entry` = 33885 AND `groupid` IN (1, 2, 3); -- normal mode
-- Missed loot
REPLACE INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `groupid`, `mincountOrRef`, `maxcount`) VALUES
(33885, 45445, 0, 4, 1, 1),
(33885, 45443, 0, 4, 1, 1),
(33885, 45444, 0, 4, 1, 1),
(33885, 45446, 0, 4, 1, 1),
(33885, 45442, 0, 4, 1, 1);
UPDATE `creature_loot_template` SET `condition_id` = @ConditionEntry + 1 WHERE `entry` = 33885 AND `groupid` IN (4); -- hard mode
