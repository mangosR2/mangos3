-- Spell DBC

-- Paladin - Consegration
DELETE FROM `spell_dbc` WHERE `Id` = 26573;
INSERT IGNORE INTO `spell_dbc` (`Id`, `Effect1`, `EffectApplyAuraName1`, `EffectImplicitTargetA1`, `EffectImplicitTargetB1`) VALUES
(26573, 27, 4, 18, 16);

-- Druid - Mushroom
DELETE FROM `spell_dbc` WHERE `Id` = 82365;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectBasePoints1`) VALUES
(82365, 25);

-- Druid - Innervate
DELETE FROM `spell_dbc` WHERE `Id` = 54833;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectApplyAuraName1`) VALUES
(54833, 24);

-- Enrage (Rank 1)
DELETE FROM `spell_dbc` WHERE `Id` = 12880;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectBasePoints1`) VALUES
(12880, 2);

-- Enrage (Rank 2)
DELETE FROM `spell_dbc` WHERE `Id` = 14201;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectBasePoints1`) VALUES
(14201, 4);

-- Enrage (Rank 3)
DELETE FROM `spell_dbc` WHERE `Id` = 14202;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectBasePoints1`) VALUES
(14202, 6);

-- Rogue - Gouge
-- DELETE FROM `spell_dbc` WHERE `Id` = 1776;
-- INSERT IGNORE INTO `spell_dbc` (`Id`, `Attributes`) VALUES
-- (1776, X); ToDo: Fixme: spell->Attributes &= ~SPELL_ATTR_UNK11;

-- Smoke Bomb
DELETE FROM `spell_dbc` WHERE `Id` = 88611;
INSERT IGNORE INTO `spell_dbc` (`Id`, `EffectImplicitTargetA1`, `EffectImplicitTargetB1`) VALUES
(88611, 8, 0);
