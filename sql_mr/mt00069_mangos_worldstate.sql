-- Twin Peaks world state

DELETE FROM `worldstate_template` WHERE `type` = 4 AND `condition` = 726;
INSERT INTO `worldstate_template` (`state_id`, `type`, `condition`, `flags`, `default`, `linked_id`, `ScriptName`, `comment`) VALUES
(1545, 4, 726, @FLAG_INITIAL_ACTIVE, 0, 0, '', ''),
(1546, 4, 726, @FLAG_INITIAL_ACTIVE, 0, 0, '', ''),
(1581, 4, 726, @FLAG_INITIAL_ACTIVE, 0, 0, '', ''),
(1582, 4, 726, @FLAG_INITIAL_ACTIVE, 0, 0, '', ''),
(1601, 4, 726, @FLAG_INITIAL_ACTIVE, 3, 0, '', ''),
(2338, 4, 726, @FLAG_INITIAL_ACTIVE, 1, 0, '', ''),
(2339, 4, 726, @FLAG_INITIAL_ACTIVE, 1, 0, '', ''),
(4247, 4, 726, @FLAG_INITIAL_ACTIVE, 1, 0, '', ''),
(4248, 4, 726, @FLAG_INITIAL_ACTIVE, 25, 0, '', '');