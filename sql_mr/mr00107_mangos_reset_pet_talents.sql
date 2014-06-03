-- missing trainer_class for enable gossip menu
UPDATE `creature_template` SET `TrainerClass` = 3 WHERE `subname` = 'Pet Trainer';
