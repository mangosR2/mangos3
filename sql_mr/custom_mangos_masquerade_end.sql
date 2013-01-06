-- query for finish table masquerading (must rollback all changes from custom_mangos_masquerade_begin.sql)
--
DROP VIEW IF EXISTS `vehicle_accessory`;
RENAME TABLE `r2_vehicle_accessory` TO `vehicle_accessory`;

DROP VIEW `creature_movement_scripts`;
DROP VIEW `event_scripts`;
DROP VIEW `gameobject_scripts`;
DROP VIEW `gameobject_template_scripts`;
DROP VIEW `gossip_scripts`;
DROP VIEW `quest_end_scripts`;
DROP VIEW `quest_start_scripts`;
DROP VIEW `spell_scripts`;
