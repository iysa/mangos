DELETE FROM `spell_bonus_data` where `entry` in (20424,20467,31803,53742,31804,53733,31893,32221,32220,31898,53719,53718,53725,53726);
INSERT INTO `spell_bonus_data` VALUES
(20424,0,0,0,"Paladin - Seal of Command Proc"),
(20467,0.7695,0,0.16,"Paladin - Judgement of Command"),
(31803,0,0.0176,0.03,"Paladin - Holy Vengeance"),
(53742,0,0.0176,0.03," Paladin - Blood Corruption"),
(31804,0,0,0,"Paladin - Judgement of Vengeance"),
(53733,0,0,0,"Paladin - Judgement of Corruption"),
(31893,0,0,0,"Paladin- Seal of Blood Proc Enemy"),
(32221,0,0,0,"Paladin - Seal of Blood Proc Self"),
(31898,0.25,0,0.16,"Paladin - Judgement of Blood Enemy"),
(32220,0.0833,0,0.0533,"Paladin - Judgement of Blood Self"),
(53719,0,0,0,"Paladin - Seal of the Martyr Proc Enemy"),
(53718,0,0,0,"Paladin - Seal of the Martyr Proc Self"),
(53726,0.25,0,0.16,"Paladin - Judgement of the Martyr Enemy"),
(53725,0.0833,0,0.0533,"Paladin - Judgement of the Martyr Self");
