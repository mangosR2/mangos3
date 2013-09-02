-- Correct tables
ALTER TABLE `npc_vendor_template` ADD COLUMN `condition_id` mediumint(8) unsigned NOT NULL default '0' AFTER ExtendedCost;
ALTER TABLE `npc_vendor` ADD COLUMN `condition_id` mediumint(8) unsigned NOT NULL default '0' AFTER ExtendedCost;
