ALTER TABLE guild ADD `level` INT(10) unsigned DEFAULT '1' AFTER `BankMoney`;
ALTER TABLE guild ADD `experience` BIGINT(20) unsigned DEFAULT '0' AFTER `level`;
ALTER TABLE guild ADD `todayExperience` BIGINT(20) unsigned DEFAULT '0' AFTER `experience`;

CREATE TABLE `guild_news_eventlog` (
  `guildid` int(10) unsigned NOT NULL,
  `LogGuid` int(10) unsigned NOT NULL,
  `EventType` int(10) unsigned NOT NULL,
  `PlayerGuid` bigint(20) unsigned NOT NULL,
  `Data` int(10) unsigned NOT NULL,
  `Flags` int(10) unsigned NOT NULL,
  `Date` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guildid`,`LogGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
