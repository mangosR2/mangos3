-- query for begin table masquerading (ALL changest must be rollbacked in  custom_mangos_masquerade_end.sql !)
--

RENAME TABLE `vehicle_accessory` TO `r2_vehicle_accessory`;

CREATE VIEW `vehicle_accessory` (`vehicle_entry`, `seat`, `accessory_entry`, `comment`)
    AS SELECT `vehicle_entry`, `seat`, `accessory_entry`, `comment`
    FROM `r2_vehicle_accessory`;

CREATE VIEW `creature_movement_scripts` AS SELECT * FROM `dbscripts_on_creature_movement`;
CREATE VIEW `event_scripts` AS SELECT * FROM  `dbscripts_on_event`;
CREATE VIEW `gameobject_scripts` AS SELECT * FROM  `dbscripts_on_go_use`;
CREATE VIEW `gameobject_template_scripts` AS SELECT * FROM `dbscripts_on_go_template_use`;
CREATE VIEW `gossip_scripts` AS SELECT * FROM `dbscripts_on_gossip`;
CREATE VIEW `quest_end_scripts` AS SELECT * FROM `dbscripts_on_quest_end`;
CREATE VIEW `quest_start_scripts` AS SELECT * FROM  `dbscripts_on_quest_start`;
CREATE VIEW `spell_scripts` AS SELECT * FROM `dbscripts_on_spell`;
