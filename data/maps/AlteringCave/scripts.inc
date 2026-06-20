AlteringCave_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, AlteringCave_OnTransition
	.byte 0

AlteringCave_OnTransition:
	setflag FLAG_LANDMARK_ALTERING_CAVE
	random NUM_ALTERING_CAVE_TABLES
	copyvar VAR_ALTERING_CAVE_WILD_SET, VAR_RESULT
	end

