ALTER TABLE `creature_template_classlevelstats`
    ADD COLUMN `BaseHealthExp3` mediumint(8) unsigned NOT NULL DEFAULT '0' AFTER `BaseHealthExp2`,
    ADD COLUMN `BaseDamageExp3` float NOT NULL DEFAULT '0' AFTER `BaseDamageExp2`;