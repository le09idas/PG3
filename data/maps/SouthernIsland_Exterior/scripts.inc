SouthernIsland_Exterior_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, SouthernIsland_Exterior_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, SouthernIsland_Exterior_OnFrame
	.byte 0

SouthernIsland_Exterior_OnTransition:
	setflag FLAG_LANDMARK_SOUTHERN_ISLAND
	end

@ PG3 quest: reunite with the awakened Lati the moment the player steps off
@ the ferry, before they head north into the interior for the vanilla
@ opposite-species encounter. ON_FRAME_TABLE (not ON_RESUME/ON_TRANSITION) is
@ used deliberately here, matching Sootopolis's legendary-awakening cutscene
@ (data/maps/SootopolisCity/scripts.inc) — it only fires once the map has
@ actually faded in and is visible, unlike ON_RESUME/ON_TRANSITION which run
@ during the load-in transition, before the player can see or respond to a
@ msgbox. VAR_TEMP_5 starts at 0 each map load (temp vars reset every load),
@ so the table entry matches on the first idle frame. Critically, the
@ triggered script immediately bumps VAR_TEMP_5 to 1 so the table stops
@ matching — a frame-table entry that keeps matching forever hijacks
@ ProcessPlayerFieldInput every single frame (TryRunOnFrameMapScript always
@ returns TRUE on a match, short-circuiting before normal movement input is
@ ever processed), which locks the player out of moving. FLAG_QUEST_LATI_
@ REUNITED is the real "already done" guard across future visits to this map.
SouthernIsland_Exterior_OnFrame:
	map_script_2 VAR_TEMP_5, 0, SouthernIsland_Exterior_EventScript_TryReunion
	.2byte 0

SouthernIsland_Exterior_EventScript_TryReunion::
	setvar VAR_TEMP_5, 1
	goto_if_set FLAG_QUEST_LATI_REUNITED, Common_EventScript_NopReturn
	goto QuestLati_EventScript_Reunion
	end

SouthernIsland_Exterior_EventScript_Sailor::
	lock
	faceplayer
	msgbox EventTicket_Text_SouthernIslandSailBack, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, SouthernIsland_Exterior_EventScript_AsYouLike
	msgbox EventTicket_Text_SailHome, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	delay 30
	hideobjectat LOCALID_SOUTHERN_ISLAND_SAILOR, MAP_SOUTHERN_ISLAND_EXTERIOR
	setvar VAR_0x8004, LOCALID_SOUTHERN_ISLAND_SS_TIDAL
	call Common_EventScript_FerryDepartIsland
	warp MAP_LILYCOVE_CITY_HARBOR, 8, 11
	waitstate
	release
	end

SouthernIsland_Exterior_EventScript_AsYouLike::
	msgbox EventTicket_Text_AsYouLike, MSGBOX_DEFAULT
	release
	end

Ferry_EventScript_DepartIslandSouth::
	applymovement LOCALID_PLAYER, Ferry_Movement_DepartIslandBoardSouth
	waitmovement 0
	return

Ferry_EventScript_DepartIslandWest::
	applymovement LOCALID_PLAYER, Ferry_Movement_DepartIslandBoardWest
	waitmovement 0
	return

Ferry_Movement_DepartIslandBoardSouth:
	walk_down
	step_end

Ferry_Movement_DepartIslandBoardWest:
	walk_left
	walk_in_place_faster_down
	step_end

SouthernIsland_Exterior_EventScript_Sign::
	msgbox SouthernIsland_Exterior_Text_Sign, MSGBOX_SIGN
	end

