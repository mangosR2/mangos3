-- 4.3.4 15595 spell_proc_event

-- Shadow Orb
DELETE FROM `spell_proc_event` WHERE `entry` = 77487;

-- Evangelism
DELETE FROM `spell_proc_event` WHERE `entry` = 81659;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskC0`) VALUES
(81659, 6, 1048704, 1088);

-- Inner Focus
DELETE FROM `spell_proc_event` WHERE `entry` = 89485;
INSERT INTO `spell_proc_event` (`entry`, `procFlags`, `procEx`, `CustomChance`) VALUES
(89485, 16384, 524288, 100);

-- Borrowed Time
DELETE FROM `spell_proc_event` WHERE `entry` IN (59887, 59888);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUES
(59887, 127),
(59888, 127);

-- Sin and Punishment
DELETE FROM `spell_proc_event` WHERE `entry` = 87099;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyMaskC0`, `procEx`) VALUES
(87099, 64, 2);

-- Masochism
DELETE FROM `spell_proc_event` WHERE `entry` = 88994;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUES
(88994, 127);

-- Strength of Soul
DELETE FROM `spell_proc_event` WHERE `entry` = 89488;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`) VALUES
(89488, 6, 2048+4096+1024); 

-- Divine Aegis
DELETE FROM `spell_proc_event` WHERE `entry` = 47509;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUES
(47509, 127);

-- Focused Will
DELETE FROM `spell_proc_event` WHERE `entry` = 45234;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUES
(45234, 127);

-- Atonement
DELETE FROM `spell_proc_event` WHERE `entry` = 14523;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `procFlags`) VALUES
(14523, 6, 1048704, 65536);

-- Surge of Light
DELETE FROM `spell_proc_event` WHERE `entry` = 88687;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `CustomChance`) VALUES
(88687, 6, 7296, 4, 0); -- remove 100%

-- Surge of Light (proc)
DELETE FROM `spell_proc_event` WHERE `entry` = 88688;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `procFlags`, `CustomChance`) VALUES
(88688, 6, 2048, 16384, 100);

-- Serendipity
DELETE FROM `spell_proc_event` WHERE `entry` = 63730;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`) VALUES
(63730, 6, 2048, 4);

-- Chakra
DELETE FROM `spell_proc_event` WHERE `entry` = 14751;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `SpellFamilyMaskA1`, `SpellFamilyMaskB1`, `SpellFamilyMaskA2`, `SpellFamilyMaskC2`, `procEx`) VALUES
(14751, 6, 7168, 4, 512, 32, 128, 65536, 524288);

-- Body and Soul
DELETE FROM `spell_proc_event` WHERE `entry` = 64127;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB1`, `procFlags`, `CustomChance`) VALUES
(64127, 6, 1, 1, 16384, 100);

-- Echo of Light
DELETE FROM `spell_proc_event` WHERE `entry` = 77485;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `CustomChance`) VALUES
(77485, 0, 0, 0);

-- Vengeance
DELETE FROM `spell_proc_event` WHERE `entry` = 84839;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUE
(84839, 127);

-- Vengeance
DELETE FROM `spell_proc_event` WHERE `entry` = 93098;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUE
(93098, 127);

-- Vigilance
DELETE FROM `spell_proc_event` WHERE `entry` = 50720;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUE
(50720, 127);

-- Sanctuary
DELETE FROM `spell_proc_event` WHERE `entry` = 20911;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `procEx`) VALUE
(20911, 127, 80);

-- Protector of the Innocent
DELETE FROM `spell_proc_event` WHERE `entry` = 20138;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUE
(20138, 127);

-- Speed of Light
DELETE FROM `spell_proc_event` WHERE `entry` = 85495;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskC0`) VALUE
(85495, 10, 256);

-- Daybreak
DELETE FROM `spell_proc_event` WHERE `entry` = 88819;
INSERT INTO `spell_proc_event` (`entry`, `procEx`) VALUES
(88819, 524288);

-- Daybreak
DELETE FROM `spell_proc_event` WHERE `entry` = 88820;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskC0`) VALUE
(88820, 10, 3221225472, 1024);

-- Tower of Radiance
DELETE FROM `spell_proc_event` WHERE `entry` = 84800;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskC0`, `procFlags`, `CustomChance`) VALUE
(84800, 10, 1073741824, 1024, 16384, 100);

-- Illuminated Healing
DELETE FROM `spell_proc_event` WHERE `entry` = 76669;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `SpellFamilyMaskC0`) VALUE
(76669, 10, 2147483648+1073741824, 65536, 1024+16384+512);

-- Judgements of the Wise
DELETE FROM `spell_proc_event` WHERE `entry` = 31878;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`) VALUE
(31878, 10, 8388608);

-- Divine Purpose
DELETE FROM `spell_proc_event` WHERE `entry` IN (85117, 86172);
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `SpellFamilyMaskC0`, `procFlags`, `CustomChance`) VALUE
(85117, 10, 8388608, 2+131072+2097152+128, 32768+8192, 65536+16+16384, 100),
(86172, 10, 8388608, 2+131072+2097152+128, 32768+8192, 65536+16+16384, 100);

-- Divine Purpose proc
DELETE FROM `spell_proc_event` WHERE `entry` = 90174;
INSERT INTO `spell_proc_event` (`entry`, `procFlags`, `procEx`, `CustomChance`) VALUE
(90174, 65536+16+16384, 524288, 100);

-- Grand Crusader
DELETE FROM `spell_proc_event` WHERE `entry` = 75806;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskB0`) VALUE
(75806, 10, 262144+32768);

-- Eye for an Eye
DELETE FROM `spell_proc_event` WHERE `entry` = 25988;
DELETE FROM `spell_proc_event` WHERE `entry` = 9799;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `CustomChance`) VALUE
(9799, 126, 0);

-- Pursuit of Justice
DELETE FROM `spell_proc_event` WHERE `entry` = 26022;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `procFlags`, `procEx`, `Cooldown`) VALUE
(26022, 127, 131072+8192, 65536, 8);

-- Seals of Command
DELETE FROM `spell_proc_event` WHERE `entry` = 85126;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskB0`, `SpellFamilyMaskC0`, `procEx`) VALUE
(85126, 10, 536872960, 65536, 0);

-- Seal of Righteousness proc
DELETE FROM `spell_proc_event` WHERE `entry` = 20154;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `procFlags`) VALUE
(20154, 127, 4);

-- Seal of Justice proc
DELETE FROM `spell_proc_event` WHERE `entry` = 20164;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `ppmRate`, `procFlags`) VALUE
(20164, 127, 0, 4); -- no ppm rate in cata?

-- Seal of Truth
DELETE FROM `spell_proc_event` WHERE `entry` = 31801;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`) VALUE
(31801, 1);

-- Seal of Insight
DELETE FROM `spell_proc_event` WHERE `entry` = 20165;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `ppmRate`) VALUE
(20165, 127, 20);

-- Sacred Shield
DELETE FROM `spell_proc_event` WHERE `entry` = 85285;
INSERT INTO `spell_proc_event` (`entry`, `Cooldown`) VALUE
(85285, 60);

-- Selfless Healer
DELETE FROM `spell_proc_event` WHERE `entry` = 85803;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskC1`) VALUE
(85803, 10, 16384);

-- Judgements of the Bold
DELETE FROM `spell_proc_event` WHERE `entry` = 89901;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`) VALUE
(89901, 10, 8388608);

-- Hand of Light
DELETE FROM `spell_proc_event` WHERE `entry` = 76672;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskB0`, `SpellFamilyMaskC0`, `procFlags`) VALUE
(76672, 10, 131072+32768, 8192, 16+4096);


### WARLOCK ###

-- Aftermath
DELETE FROM `spell_proc_event` WHERE `entry` = 85113;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `procFlags`) VALUE
(85113, 5, 32, 8388608, 4096+65536);

-- Soul Fire!
DELETE FROM `spell_proc_event` WHERE `entry` = 61189;
INSERT INTO `spell_proc_event` (`entry`, `procEx`) VALUES
(61189, 524288);

-- Improved Soul Fire
DELETE FROM `spell_proc_event` WHERE `entry` IN (18119, 18120);
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskB0`, `procFlags`, `CustomChance`) VALUE
(18119, 5, 128, 65536, 100),
(18120, 5, 128, 65536, 100);

-- Burning Embers
DELETE FROM `spell_proc_event` WHERE `entry` = 91986;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`) VALUE
(91986, 5, 4096, 128);

-- Soul Leech
DELETE FROM `spell_proc_event` WHERE `entry` = 30293;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`, `SpellFamilyMaskB0`, `procEx`) VALUE
(30293, 5, 128, 128+131072, 3);

-- Nether Protection
DELETE FROM `spell_proc_event` WHERE `entry` IN (30299, 30301);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `procFlags`, `procEx`, `CustomChance`) VALUE
(30299, 127, 664232, 1024, 100),
(30301, 127, 664232, 1024, 100);

-- Empowered Imp (passive)
DELETE FROM `spell_proc_event` WHERE `entry` = 54278;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`) VALUE
(54278, 5, 4096);

-- Empowered Imp (proc)
DELETE FROM `spell_proc_event` WHERE `entry` = 47283;
INSERT INTO `spell_proc_event` (`entry`, `procFlags`, `procEx`, `CustomChance`) VALUE
(47283, 65536, 524288, 100);

-- Bane of Havoc
DELETE FROM `spell_proc_event` WHERE `entry` IN (85466, 85468);
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `procFlags`) VALUE
(85466, 127, 332116),
(85468, 127, 332116);

-- Jinx
DELETE FROM `spell_proc_event` WHERE `entry` = 18179;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA1`) VALUE
(18179, 5, 32768);

### Mage ###

-- Ring of Frost
DELETE FROM `creature_template_addon` WHERE `entry` = 44199;
INSERT INTO `creature_template_addon` (`entry`, `b2_0_sheath`, `auras`) VALUE
(44199, 1, '91264');

-- Siphon Life
DELETE FROM `spell_proc_event` WHERE `entry` = 63108;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SpellFamilyMaskA0`) VALUE
(63108, 5, 2);

