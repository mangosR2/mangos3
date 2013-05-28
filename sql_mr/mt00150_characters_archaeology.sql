-- Character Archaeology
DROP TABLE IF EXISTS `character_archaeology`;
CREATE TABLE `character_archaeology` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `sites` int(10) unsigned NOT NULL DEFAULT '0',
  `counts` int(10) unsigned NOT NULL DEFAULT '0',
  `projects` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `character_archaeology_finds`;
CREATE TABLE `character_archaeology_finds` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `id` int(10) unsigned NOT NULL DEFAULT '0',
  `count` int(10) unsigned NOT NULL DEFAULT '0',
  `date` bigint(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
