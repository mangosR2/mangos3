-- Spell DBC

-- Paladin - Consegration
DELETE FROM `spell_dbc` WHERE `Id` = 26573;
INSERT IGNORE INTO `spell_dbc` (`Id`, `Effect1`, `EffectApplyAuraName1`, `EffectImplicitTargetA1`, `EffectImplicitTargetB1`) VALUES
(26573, 27, 4, 18, 16);
