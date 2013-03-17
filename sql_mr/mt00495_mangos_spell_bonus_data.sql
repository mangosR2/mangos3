-- 4.3.4 15595 spell_bonus_data
-- Paladin
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
(12809, 0, 0, 0.75, 0, 'Warrior - Concussion Blow'),
(20243, 0, 0, 0.171, 0, 'Warrior - Devastate'),
(20253, 0, 0, 0.12, 0, 'Warrior - Intercept'),
(34428, 0, 0, 0, 0, 'Warrior - Victory Rush'),
(44949, 0, 0, 0.236, 0, 'Warrior - Whirlwind (offhand)'),
(57755, 0, 0, 0.75, 0, 'Warrior - Heroic Throw'),
(64382, 0, 0, 0.5, 0, 'Warrior - Shattering Throw');

-- Rogue
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(1776, 0, 0, 0.21, 0, 'Rogue - Gouge'),
(5374, 0, 0, 0.243, 0, 'Rogue - Mutilate'),
(5940, 0, 0, 0.121, 0, 'Rogue - Shiv'),
(8676, 0, 0, 0.171, 0, 'Rogue - Ambush'),
(51723, 0, 0, 0.129, 0, 'Rogue - Fan of Knives'),
(84617, 0, 0, 0.114, 0, 'Rogue - Revealing Strike');

-- Death Knight
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(47541, 0, 0, 0, 0, 'Death Knight - Death Coil'),
(47632, 0, 0, 0.23, 0, 'Death Knight - Death Coil damage'),
(47633, 0, 0, 0.805, 0, 'Death Knight - Death Coil heal'),
(48721, 0, 0, 0.096, 0, 'Death Knight - Blood Boil'),
(49184, 0, 0, 0.44, 0, 'Death Knight - Howling Blast'),
(49998, 0, 0, 0.236, 0, 'Death Knight - Death Strike'),
(52212, 0, 0, 0.064, 0, 'Death Knight - Death and Decay Triggered'),
(55050, 0, 0, 0.236, 0, 'Death Knight - Heart Strike'),
(85948, 0, 0, 0.236, 0, 'Death Knight - Festering Strike');

-- Mague
REPLACE INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(543, 0.807, 0, 0, 0, 'Mage - Mage Ward'),
(30455, 0.378, 0, 0, 0, 'Mage - Ice Lance'),
(87023, 0, 0, 0, 0, 'Mage - Cauterize');
