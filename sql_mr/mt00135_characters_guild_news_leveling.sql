ALTER TABLE guild ADD `level` INT(10) unsigned DEFAULT '1' AFTER `BankMoney`;
ALTER TABLE guild ADD `experience` BIGINT(20) unsigned DEFAULT '0' AFTER `level`;
ALTER TABLE guild ADD `todayExperience` BIGINT(20) unsigned DEFAULT '0' AFTER `experience`;
ALTER TABLE guild_member ADD COLUMN thisWeekReputation int(11) unsigned NOT NULL DEFAULT '0' AFTER offnote;

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

alter table guild_member add BankResetTimeTab6 int(11) unsigned NOT NULL DEFAULT '0' after BankRemSlotsTab5;
alter table guild_member add BankRemSlotsTab6 int(11) unsigned NOT NULL DEFAULT '0' after BankResetTimeTab6;
alter table guild_member add BankResetTimeTab7 int(11) unsigned NOT NULL DEFAULT '0' after BankRemSlotsTab6;
alter table guild_member add BankRemSlotsTab7 int(11) unsigned NOT NULL DEFAULT '0' after BankResetTimeTab7;

alter table guild_member add achievementPoints int(11) unsigned NOT NULL DEFAULT '0' after BankRemSlotsTab7;
alter table guild_member add firstProfessionId int(11) unsigned NOT NULL DEFAULT '0' after achievementPoints;
alter table guild_member add firstProfessionRank int(11) unsigned NOT NULL DEFAULT '0' after firstProfessionId;
alter table guild_member add firstProfessionValue int(11) unsigned NOT NULL DEFAULT '0' after firstProfessionRank;
alter table guild_member add secondProfessionId int(11) unsigned NOT NULL DEFAULT '0' after firstProfessionValue;
alter table guild_member add secondProfessionRank int(11) unsigned NOT NULL DEFAULT '0' after secondProfessionId;
alter table guild_member add secondProfessionValue int(11) unsigned NOT NULL DEFAULT '0' after secondProfessionRank;