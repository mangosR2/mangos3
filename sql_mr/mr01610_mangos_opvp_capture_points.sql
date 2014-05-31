-- Zangarmarsh fixes
-- red aura in c_t_a
DELETE FROM `creature_template_addon` WHERE entry = 77096;
INSERT INTO `creature_template_addon` (`entry`, `mount`, `bytes1`, `b2_0_sheath`, `b2_1_pvp_state`, `emote`, `moveflags`, `auras`) VALUES (77096, 0, 0, 1, 0, 0, 0, 32839);
