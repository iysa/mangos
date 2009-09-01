update `creature_template` set `ScriptName`='mob_chaotic_rift' where `entry`=26918;
update `creature_template` set `ScriptName`='mob_crystal_spike' where `entry`=27099;
update `creature_template` set `ScriptName`='mob_crystalline_tangler' where `entry`=32665;
update `gameobject_template` set `ScriptName`='containment_sphere' where `entry` in (188527, 188528, 188526);
update `instance_template` set `script`='instance_nexus' where `map`=576;