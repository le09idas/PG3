@ PG3 quest: Eon Ticket via the Lati awakening/reunion sidequest.
@ See DEVLOG.md for the full design. Shared across Birch's lab,
@ the step-count gates (field_control_avatar.c), and Southern Island.

QuestLati_EventScript_Awaken::
	lockall
	specialvar VAR_RESULT, GetLatiPartyStatus
	bufferpartymonnick STR_VAR_1, VAR_0x8004
	msgbox QuestLati_Text_BallTrembling, MSGBOX_DEFAULT
	getplayerxy VAR_0x8005, VAR_0x8006
	call_if_eq VAR_ROAMER_POKEMON, 0, QuestLati_EventScript_SpawnLatias
	call_if_ne VAR_ROAMER_POKEMON, 0, QuestLati_EventScript_SpawnLatios
	playmoncry VAR_TEMP_4, CRY_MODE_NORMAL
	special DoLatiSpawnOutAnim
	delay 25
	waitmoncry
	special DestroyVirtualObjects
	special StashQuestLati
	msgbox QuestLati_Text_FlewOff, MSGBOX_DEFAULT
	setflag FLAG_QUEST_LATI_AWAKENED
	setvar VAR_QUEST_LATI_STEP_COUNTER, 0
	setflag FLAG_ENABLE_LATI_BIRCH_CALL
	releaseall
	end

QuestLati_EventScript_SpawnLatias::
	setvar VAR_TEMP_4, SPECIES_LATIAS
	createvobject OBJ_EVENT_GFX_LATIAS, LATI_VOBJ_ID, VAR_0x8005, VAR_0x8006
	return

QuestLati_EventScript_SpawnLatios::
	setvar VAR_TEMP_4, SPECIES_LATIOS
	createvobject OBJ_EVENT_GFX_LATIOS, LATI_VOBJ_ID, VAR_0x8005, VAR_0x8006
	return

QuestLati_EventScript_BirchCall::
	lockall
	pokenavcall QuestLati_Text_BirchPokenavCall
	waitmessage
	clearflag FLAG_ENABLE_LATI_BIRCH_CALL
	releaseall
	end

QuestLati_EventScript_GiveTicket::
	msgbox QuestLati_Text_BirchGivesTicket, MSGBOX_DEFAULT
	giveitem ITEM_EON_TICKET
	setflag FLAG_ENABLE_SHIP_SOUTHERN_ISLAND
	release
	end

QuestLati_EventScript_Reunion::
	lockall
	msgbox QuestLati_Text_ReunionApproach, MSGBOX_DEFAULT
	getplayerxy VAR_0x8005, VAR_0x8006
	call_if_eq VAR_ROAMER_POKEMON, 0, QuestLati_EventScript_SpawnLatias
	call_if_ne VAR_ROAMER_POKEMON, 0, QuestLati_EventScript_SpawnLatios
	special DoLatiSpawnInAnim
	delay 25
	playmoncry VAR_TEMP_4, CRY_MODE_NORMAL
	waitmoncry
	special DestroyVirtualObjects
	special WithdrawQuestLati
	msgbox QuestLati_Text_ReunionComplete, MSGBOX_DEFAULT
	setflag FLAG_QUEST_LATI_REUNITED
	releaseall
	end

QuestLati_Text_BallTrembling:
	.string "Your bag stirs...\n"
	.string "{STR_VAR_1}'s Poké Ball is shaking!$"

QuestLati_Text_FlewOff:
	.string "{STR_VAR_1} burst out and shot into\n"
	.string "the sky, gone in an instant!$"

QuestLati_Text_BirchPokenavCall:
	.string "Oh, hey! It's Birch.\p"
	.string "Listen, something's come up -\n"
	.string "can you swing by the lab?$"

QuestLati_Text_BirchGivesTicket:
	.string "BIRCH: A friend of mine passed this\n"
	.string "along for you - the Eon Ticket!\p"
	.string "She said there's an island south of\n"
	.string "here you'll want to see for yourself.\p"
	.string "She said she saw your legendary\n"
	.string "POKéMON flying towards there!\p"
	.string "I guess you must have awoken something\p"
	.string "in it? Better go check it out!$"

QuestLati_Text_ReunionApproach:
	.string "Something streaks down from the sky\n"
	.string "toward you!$"

QuestLati_Text_ReunionComplete:
	.string "Your Pokémon nuzzles you happily!$"
