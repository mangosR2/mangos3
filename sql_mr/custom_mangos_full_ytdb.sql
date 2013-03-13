-- query's (for auto updater) that will be run with a full clean YTDB install

-- rename database script tables, based on commit mt31, tables are already created by mangos.sql
-- this will give an error when YTDB releases a new full clean database, should be removed then
INSERT INTO `dbscripts_on_creature_movement` SELECT * FROM `creature_movement_scripts`;
DROP TABLE `creature_movement_scripts`;
INSERT INTO `dbscripts_on_event` SELECT * FROM `event_scripts`;
DROP TABLE `event_scripts`;
INSERT INTO `dbscripts_on_go_use` SELECT * FROM `gameobject_scripts`;
DROP TABLE `gameobject_scripts`;
INSERT INTO `dbscripts_on_go_template_use` SELECT * FROM `gameobject_template_scripts`;
DROP TABLE `gameobject_template_scripts`;
INSERT INTO `dbscripts_on_gossip` SELECT * FROM `gossip_scripts`;
DROP TABLE `gossip_scripts`;
INSERT INTO `dbscripts_on_quest_end` SELECT * FROM `quest_end_scripts`;
DROP TABLE `quest_end_scripts`;
INSERT INTO `dbscripts_on_quest_start` SELECT * FROM `quest_start_scripts`;
DROP TABLE `quest_start_scripts`;
INSERT INTO `dbscripts_on_spell` SELECT * FROM `spell_scripts`;
DROP TABLE `spell_scripts`;
