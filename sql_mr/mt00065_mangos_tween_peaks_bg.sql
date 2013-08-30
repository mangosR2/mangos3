-- Twin Peaks BG SQL

delete from `battleground_events` where `map` = 726;
INSERT INTO `battleground_events` VALUES ('726', '254', '0', 'Doors');
INSERT INTO `battleground_events` VALUES ('726', '0', '0', 'Alliance Flag');
INSERT INTO `battleground_events` VALUES ('726', '1', '0', 'Horde Flag');
INSERT INTO `battleground_events` VALUES ('726', '2', '0', 'Spirit Guides');
delete from `gameobject_battleground` where `guid` in (500014, 500015, 500016, 500017, 500012, 500005, 500018, 500019, 500020, 500021);
INSERT INTO `gameobject_battleground` VALUES ('500014', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500015', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500016', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500017', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500012', '0', '0');
INSERT INTO `gameobject_battleground` VALUES ('500005', '1', '0');
INSERT INTO `gameobject_battleground` VALUES ('500018', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500019', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500020', '254', '0');
INSERT INTO `gameobject_battleground` VALUES ('500021', '254', '0');
delete from `mangos_string` where `entry` in (1802, 1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814, 1815);
INSERT INTO `mangos_string` VALUES ('1802', 'The battle for Twin Peaks begins in 1 minute!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1803', 'The battle for Twin Peaks begins in 30 seconds. Prepare yourselves!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1804', 'Let the battle for Twin Peaks begin!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1805', '$n captured the Horde flag!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1806', '$n captured the Alliance flag!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1807', 'The Horde flag was dropped by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1808', 'The Alliance Flag was dropped by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1809', 'The Alliance Flag was returned to its base by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1810', 'The Horde flag was returned to its base by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1811', 'The Horde flag was picked up by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1812', 'The Alliance Flag was picked up by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1813', 'The flags are now placed at their bases!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1814', 'The Alliance flag is now placed at its base!', null, null, null, null, null, null, null, null);
INSERT INTO `mangos_string` VALUES ('1815', 'The Horde flag is now placed at its base!', null, null, null, null, null, null, null, null);
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500000','179786','726','1','1','2107.04','210.61','43.663','3.96889','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500001','179904','726','1','1','1951.18','383.795','-10.5257','4.06662','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500002','179904','726','1','1','1951.18','383.795','-10.5257','4.06662','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500003','179905','726','1','1','1932.83','226.792','-17.0598','2.44346','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500004','179904','726','1','1','1951.18','383.795','-10.5257','4.06662','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500005','179831','726','1','1','1578.34','344.045','2.41841','2.79252','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500006','179906','726','1','1','1754.16','242.125','-14.1316','1.15192','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500007','179907','726','1','1','1737.57','435.845','-8.08634','5.51524','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500008','179906','726','1','1','1754.16','242.125','-14.1316','1.15192','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500009','179904','726','1','1','1951.18','383.795','-10.5257','4.06662','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500010','179904','726','1','1','1951.18','383.795','-10.5257','4.06662','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500011','179905','726','1','1','1932.83','226.792','-17.0598','2.44346','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500012','179830','726','1','1','2117.64','191.682','44.052','6.02139','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500013','179871','726','1','1','2175.87','226.622','43.7629','2.60053','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500014','206655','726','1','1','2115.399','150.175','43.526','2.544690','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500015','206654','726','1','1','2156.803','220.331','43.482','2.544690','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500016','206653','726','1','1','2127.512','223.711','43.640','2.544690','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500017','206653','726','1','1','2096.102','166.920','54.230','2.544690','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500018','208205','726','1','1','1556.595','314.502','1.2230','6.179126','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500019','208206','726','1','1','1587.093','319.853','1.5233','6.179126','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500020','208206','726','1','1','1591.463','365.732','13.494','6.179126','0','0','0','1','7200','255','1');
insert into `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) values('500021','208207','726','1','1','1558.315','372.709','1.4840','6.179126','0','0','0','1','7200','255','1');
insert into `game_graveyard_zone` values
(1750, 5031, 67),
(1749, 5031, 469),
(1726, 5031, 469),
(1727, 5031, 67),
(1729, 5031, 469),
(1728, 5031, 67);

INSERT INTO `creature_battleground` VALUES ('373896', '2', '0');
INSERT INTO `creature_battleground` VALUES ('373897', '2', '0');
INSERT INTO `creature_battleground` VALUES ('373898', '2', '0');
