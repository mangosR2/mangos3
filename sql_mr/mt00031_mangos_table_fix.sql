-- Table fix from Kirix
-- Don't include DROP TABLES for new formats here! If tables exists, script must throw error!

-- DROP TABLE IF EXISTS `db_script_string`;
CREATE TABLE IF NOT EXISTS `db_script_string` (
  `entry` int(11) unsigned NOT NULL default '0',
  `content_default` text NOT NULL,
  `content_loc1` text,
  `content_loc2` text,
  `content_loc3` text,
  `content_loc4` text,
  `content_loc5` text,
  `content_loc6` text,
  `content_loc7` text,
  `content_loc8` text,
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- DROP TABLE IF EXISTS `dbscripts_on_creature_movement`;
CREATE TABLE IF NOT EXISTS `dbscripts_on_creature_movement` (
  `id` mediumint(8) unsigned NOT NULL default '0',
  `delay` int(10) unsigned NOT NULL default '0',
  `command` mediumint(8) unsigned NOT NULL default '0',
  `datalong` mediumint(8) unsigned NOT NULL default '0',
  `datalong2` int(10) unsigned NOT NULL default '0',
  `buddy_entry` int(10) unsigned NOT NULL default '0',
  `search_radius` int(10) unsigned NOT NULL default '0',
  `data_flags` tinyint(3) unsigned NOT NULL default '0',
  `dataint` int(11) NOT NULL default '0',
  `dataint2` int(11) NOT NULL default '0',
  `dataint3` int(11) NOT NULL default '0',
  `dataint4` int(11) NOT NULL default '0',
  `x` float NOT NULL default '0',
  `y` float NOT NULL default '0',
  `z` float NOT NULL default '0',
  `o` float NOT NULL default '0',
  `comments` varchar(255) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
INSERT INTO `dbscripts_on_creature_movement` SELECT * FROM `creature_movement_scripts`;
DROP TABLE `creature_movement_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_event;
CREATE TABLE IF NOT EXISTS `dbscripts_on_event` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_event` SELECT * FROM `event_scripts`;
DROP TABLE `event_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_go_use;
CREATE TABLE IF NOT EXISTS `dbscripts_on_go_use` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_go_use` SELECT * FROM `gameobject_scripts`;
DROP TABLE `gameobject_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_go_template_use;
CREATE TABLE IF NOT EXISTS `dbscripts_on_go_template_use` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_go_template_use` SELECT * FROM `gameobject_template_scripts`;
DROP TABLE `gameobject_template_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_gossip;
CREATE TABLE IF NOT EXISTS `dbscripts_on_gossip` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_gossip` SELECT * FROM `gossip_scripts`;
DROP TABLE `gossip_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_quest_end;
CREATE TABLE IF NOT EXISTS `dbscripts_on_quest_end` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_quest_end` SELECT * FROM `quest_end_scripts`;
DROP TABLE `quest_end_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_quest_start;
CREATE TABLE IF NOT EXISTS `dbscripts_on_quest_start` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_quest_start` SELECT * FROM `quest_start_scripts`;
DROP TABLE `quest_start_scripts`;

-- DROP TABLE IF EXISTS dbscripts_on_spell;
CREATE TABLE IF NOT EXISTS `dbscripts_on_spell` LIKE `dbscripts_on_creature_movement`;
INSERT INTO `dbscripts_on_spell` SELECT * FROM `spell_scripts`;
DROP TABLE `spell_scripts`;

DELETE FROM `command` WHERE name LIKE 'reload all_scripts';
INSERT INTO `command` VALUES
('reload all_scripts',3,'Syntax: .reload all_scripts\r\n\r\nReload `dbscripts_on_*` tables.');

UPDATE `creature_template_addon` SET `auras` = REPLACE(`auras`, '46598', '');
UPDATE `creature_template_addon` SET `auras` = REPLACE(`auras`, '  ', ' ');
UPDATE `creature_template_addon` SET `auras` = TRIM(`auras`);
UPDATE `creature_template_addon` SET `auras` = NULL WHERE `auras` = '';

UPDATE `creature_addon` SET `auras` = REPLACE(`auras`, '46598', '');
UPDATE `creature_addon` SET `auras` = REPLACE(`auras`, '  ', ' ');
UPDATE `creature_addon` SET `auras` = TRIM(`auras`);
UPDATE `creature_addon` SET `auras` = NULL WHERE `auras` = '';
