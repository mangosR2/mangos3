-- Creature on kill Reputation
UPDATE creature_onkill_reputation SET ChampioningAura = 57818 WHERE (RewOnKillRepFaction1= 1037 OR RewOnKillRepFaction2 = 1052) AND ChampioningAura = 0;
