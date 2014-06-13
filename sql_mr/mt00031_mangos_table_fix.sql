-- Table fix from Kirix
-- Don't include DROP TABLES for new formats here! If tables exists, script must throw error!

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
