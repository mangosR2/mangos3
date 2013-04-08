
-- Pack Hobgoblin
DELETE FROM `playercreateinfo_spell` WHERE `Spell` = 69046;
INSERT IGNORE INTO playercreateinfo_spell
SELECT 9, class, 69046, 'Pack Hobgoblin' FROM playercreateinfo_spell WHERE race = 9;