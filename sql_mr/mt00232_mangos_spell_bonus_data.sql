-- 4.3.4 15595 spell_bonus_data
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(879, 0, 0, 0, 0, 'Paladin - Exorcism'),
(8092, 1.104, 0, 0, 0, 'Priest - Mind Blast'),
(33110, 0, 0, 0, 0, 'Priest - Prayer of Mending'),
(47666, 0.458, 0, 0, 0, 'Priest - Penance Damage'),
(47750, 0.321, 0, 0, 0, 'Priest - Penance Healing'),
(53600, 0, 0, 0.099, 0, 'Paladin - Shield of the Righteous'),
(73510, 0.836, 0, 0, 0, 'Priest - Mind Spike'),
(73921, 0.095, 0, 0, 0, 'Shaman - Healing Rain'),
(81297, 0.017, 0, 0.0027, 0, 'Paladin - Consecration'), -- ap coef is wrong
(82327, 0.259, 0, 0, 0, 'Paladin - Holy Radiance'),
-- (85673, 0.209, 0, 0.198, 0, 'Paladin - Word of Glory'),
(85673, 0.627, 0, 0.594, 0, 'Paladin - Word of Glory'),
(86452, 0.144/4, 0, 0, 0, 'Paladin - Holy Radiance HoT'),
(88686, 0.047, 0, 0, 0, 'Priest - Holy Word: Sanctuary');
