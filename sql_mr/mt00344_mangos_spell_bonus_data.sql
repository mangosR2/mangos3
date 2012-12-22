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
(85222, 0.144, 0, 0, 0, 'Paladin - Light of Dawn'),
(85256, 0.171, 0, 0, 0, 'Paladin - Templar''s Verdict'),
-- (85673, 0.209, 0, 0.198, 0, 'Paladin - Word of Glory'),
(85673, 0.627, 0, 0.594, 0, 'Paladin - Word of Glory'),
(86452, 0.144/4, 0, 0, 0, 'Paladin - Holy Radiance HoT'),
(88686, 0.047, 0, 0, 0, 'Priest - Holy Word: Sanctuary');

-- Warlock
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(980, 0, 0.096, 0, 0, 'Warlock - Bane of Agony'),
(1120, 0, 0.378, 0, 0, 'Warlock - Drain Soul'),
(5676, 0.423, 0, 0, 0, 'Warlock - Searing Pain'),
(17877, 1.056, 0, 0, 0, 'Warlock - Shadowburn'),
(17962, 0, 0, 0, 0, 'Warlock - Conflagrate'),
(29722, 0.676, 0, 0, 0, 'Warlock - Incinerate'),
(77799, 0.302, 0, 0, 0, 'Warlock - Fel Flame'),
(91711, 0, 0, 0, 0, 'Warlock - Nether Ward');

-- Druid
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(78777, 0.6032, 0, 0, 0, 'Druid - Wild Mushroom: Detonate');

-- Warrior
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(772, 0, 0, 0, 0, 'Warrior - Rend'),
(1680, 0, 0, 0.236, 0, 'Warrior - Whirlwind'),
(6272, 0, 0, 0.31, 0, 'Warrior - Revenge'),
(6343, 0, 0, 0.228, 0, 'Warrior - Thunder Clap'),
(20253, 0, 0, 0.12, 0, 'Warrior - Intercept'),
(44949, 0, 0, 0.236, 0, 'Warrior - Whirlwind (offhand)'),
(57755, 0, 0, 0.75, 0, 'Warrior - Heroic Throw'),
(64382, 0, 0, 0.5, 0, 'Warrior - Shattering Throw');
