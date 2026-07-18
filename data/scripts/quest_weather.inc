@ PG3 quest: Aurora Ticket via Rayquaza calming weather anomalies.
@ See DEVLOG.md for the full design. Shared across the Weather Institute
@ call gates (field_control_avatar.c), the three anomaly sites
@ (Route111/Route119/MossdeepCity), and Steven's Space Center handoff.

QuestWeather_EventScript_InstituteCall::
	lockall
	pokenavcall QuestWeather_Text_InstituteCall
	waitmessage
	setflag FLAG_QUEST_WEATHER_STARTED
	releaseall
	end

QuestWeather_EventScript_StevenCall::
	lockall
	pokenavcall QuestWeather_Text_StevenCall
	waitmessage
	clearflag FLAG_ENABLE_STEVEN_TICKET_CALL
	releaseall
	end

@ Called by each site's puzzle right after that site's own DONE flag is set.
@ Checks whether all three are now done and arms Steven's call if so.
QuestWeather_EventScript_SiteResolved::
	lockall
	setweather WEATHER_SUNNY
	doweather
	playmoncry SPECIES_RAYQUAZA, CRY_MODE_NORMAL
	waitmoncry
	msgbox QuestWeather_Text_SiteCalmed, MSGBOX_DEFAULT
	goto_if_unset FLAG_QUEST_WEATHER_ROUTE111_DONE, QuestWeather_EventScript_SiteResolvedReturn
	goto_if_unset FLAG_QUEST_WEATHER_ROUTE119_DONE, QuestWeather_EventScript_SiteResolvedReturn
	goto_if_unset FLAG_QUEST_WEATHER_MOSSDEEP_DONE, QuestWeather_EventScript_SiteResolvedReturn
	setflag FLAG_QUEST_WEATHER_ALL_DONE
	setvar VAR_QUEST_STEVEN_CALL_STEP_COUNTER, 0
	setflag FLAG_ENABLE_STEVEN_TICKET_CALL
QuestWeather_EventScript_SiteResolvedReturn::
	releaseall
	end

QuestWeather_EventScript_GiveTicket::
	msgbox QuestWeather_Text_StevenGivesTicket, MSGBOX_DEFAULT
	giveitem ITEM_AURORA_TICKET
	setflag FLAG_ENABLE_SHIP_BIRTH_ISLAND
	release
	end

@ Shared by all 3 anomaly sites' marker-touch scripts. Each site uses its own
@ VAR_TEMP_8 as a 3-step order counter (per-map scope, auto-resets on load) —
@ touching markers out of order silently resets progress, no penalty.
QuestWeather_EventScript_NoRayquaza::
	msgbox QuestWeather_Text_NothingHappens, MSGBOX_DEFAULT
	release
	end

QuestWeather_EventScript_MarkerWrongOrder::
	setvar VAR_TEMP_8, 0
	msgbox QuestWeather_Text_MarkerReset, MSGBOX_DEFAULT
	release
	end

QuestWeather_Text_NothingHappens:
	.string "Nothing seems to happen…$"

QuestWeather_Text_MarkerReset:
	.string "Whatever was building up fades away…$"

QuestWeather_Text_MarkerResponds:
	.string "RAYQUAZA's presence stirs something\n"
	.string "in the air around the marker…$"

QuestWeather_Text_InstituteCall:
	.string "This is the Weather Institute!\p"
	.string "We're detecting bizarre weather at\n"
	.string "three different spots.\p"
	.string "The desert, the route right by our\n"
	.string "building, and over Mossdeep!\p"
	.string "Could your Pokémon possibly have\n"
	.string "something to do with this…?$"

QuestWeather_Text_SiteCalmed:
	.string "RAYQUAZA lets out a mighty roar!\n"
	.string "The strange weather clears at once!$"

QuestWeather_Text_StevenCall:
	.string "Steven here.\p"
	.string "The Weather Institute told me what\n"
	.string "you've been up to.\p"
	.string "Could you swing by the SPACE CENTER?\n"
	.string "I've got some news for you.$"

QuestWeather_Text_StevenGivesTicket:
	.string "STEVEN: An astronaut friend of mine\n"
	.string "just got back from orbit.\p"
	.string "She said she saw a meteor fall near\n"
	.string "an island to the south.\p"
	.string "Funny enough, I happen to have this -\n"
	.string "the AURORA TICKET.\p"
	.string "Something tells me you'll want to go\n"
	.string "take a look for yourself.$"
