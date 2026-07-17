# PG3 Dev Log

Running journal of progress. Newest entry on top. At the end of each session add:
what changed, what's next, and any blockers. This is the canonical "where are we" —
read it first.

---

## 2026-07-17 — Eon Ticket via Lati awakening/reunion sidequest

The Eon Ticket (Southern Island → Latios/Latias) was unreachable in real
gameplay — vanilla only grants it via Record Mixing with a second cartridge
that already has it, a dead end (`ShouldDistributeEonTicket()` in
`src/field_specials.c` is hardcoded to always return FALSE). Built a proper
in-game unlock, tied to the *other* Latios/Latias — the one already
obtainable via the vanilla roaming mechanic (unlocked post-Hall of Fame,
untouched).

**Flow:** catch the roaming Lati → bring it to Birch, who hints at hidden
power → train it 5 levels above its catch level → while outdoors (not
indoors/dungeon) its Poké Ball trembles, it bursts out, shoots skyward, and
leaves the party → Birch calls the PokéNav a bit later, asking the player to
the lab → Birch hands over the Eon Ticket (a friend gave it to him) →
`setflag FLAG_ENABLE_SHIP_SOUTHERN_ISLAND` — this alone unlocks both the
Lilycove Harbor destination and the interior encounter, no other vanilla
plumbing touched. On Southern Island, stepping off the ferry reunites the
player with their exact original Lati (rejoins party, or PC if full) before
they head north into the forest for the untouched vanilla encounter with the
*opposite* species of the pair.

**New constants** — `FLAG_QUEST_LATI_STARTED/AWAKENED/REUNITED`,
`FLAG_ENABLE_LATI_BIRCH_CALL` (`include/constants/flags.h`, reused-unused
`SYSTEM_FLAGS + 0x85` range); `VAR_QUEST_LATI_STEP_COUNTER/BOX_ID/BOX_POS`
(`include/constants/vars.h`, reused-unused tail range); `LATI_QUEST_ABSENT/
PRESENT/READY`, `LATI_VOBJ_ID` (`include/constants/field_specials.h`).

**New C (`src/field_specials.c`)** — `GetLatiPartyStatus()` (party species +
met-level check, mirrors `checkpartymove`'s shape), `ShouldLatiAwaken()` /
`ShouldDoLatiBirchCall()` (mirror `ShouldDoWallyCall`, hooked into
`TryStartStepCountScript` in `src/field_control_avatar.c`), and two tiny
wrappers around the engine's "virtual object" system
(`SetVirtualObjectSpriteAnim`/`IsVirtualObjectAnimating`, not script opcodes
by default) for the spawn-in/out animation. `DestroyVirtualObjects()`
(`src/event_object_movement.c`) was `static UNUSED` — un-static'd and
exposed as a special, since it's exactly the cleanup this needed.

**New C (`src/pokemon.c`)** — `StashQuestLati()`/`WithdrawQuestLati()`, using
the same "pull a mon out of the active party, hold it elsewhere, hand it back
later" pattern `src/daycare.c` already uses, backed by the existing PC-box
primitives (`CopyMonToPC`, `BoxMonAtToMon`, `GiveMonToPlayer`) rather than any
new save-data structure.

**Scripts** — new shared file `data/scripts/quest_lati.inc` (included from
`data/event_scripts.s`, per the project's aggregator rule) holds the
awaken/call/give-ticket/reunion scripts; `LittlerootTown_ProfessorBirchsLab/
scripts.inc` gained two new branches ahead of the existing dialogue chain;
`SouthernIsland_Exterior/scripts.inc` gained a `MAP_SCRIPT_ON_RESUME` hook
for the reunion beat. Southern Island's interior encounter script (opposite
species) is completely untouched.

The "shoots up and vanishes" / "descends from the sky" visuals reuse the
existing `OBJ_EVENT_GFX_LATIOS`/`LATIAS` graphics via `createvobject` (a
temporary sprite at the player's exact position, no map authoring needed —
same primitive vanilla uses for Union Room/crowd sprites) plus the engine's
existing Union Room spawn-in/out slide animation. No new art.

**Bug found in first mGBA test (2026-07-17), fixed same session:** the awaken
cutscene hard-froze right after the first message box (screen locked, music
kept playing — the giveaway). Root cause: the animation-wait was a hand-rolled
script poll —
```
specialvar VAR_RESULT, IsLatiVirtualObjectAnimating
goto_if_eq VAR_RESULT, TRUE, <same label>
```
— but `RunScriptCommand` (`src/script.c:91-121`) executes bytecode commands in
a tight `while(1)` and only yields back to the main game loop when a command
returns `TRUE` (as `delay`/`waitmovement`/`msgbox` do). Neither `specialvar`
nor `goto_if_eq` yield, so this was a genuine same-frame infinite loop —
sprite animation (and everything else frame-based) never got a chance to
advance, while sound hardware kept playing via interrupts independent of the
stuck main loop, matching the symptom exactly. Fixed by replacing the poll
with a fixed `delay 25` (the slide animation is a known 20-frame duration,
`y2` stepping by 8 over `DISPLAY_HEIGHT`=160px) in both the awaken and
reunion scripts, and removed the now-unused `IsLatiVirtualObjectAnimating`
special entirely (`src/field_specials.c`, `include/field_specials.h`,
`data/specials.inc`). Lesson: any script-side "wait for a condition" needs a
real yielding wait command, never a bare `goto` self-loop.

Confirmed the roaming-Lati catch → Birch hint → training-detection →
awaken-cutscene chain works end-to-end on a real retail-cartridge save
(dumped from actual hardware — retail saves load fine against PG3 since save
data structures/ROM identity are untouched). The fly-up animation completed
correctly after the `delay`-based fix above. ✅

**Second bug, found continuing the same test session:** after Birch hands
over the Eon Ticket and the player sails to Southern Island, the screen faded
to black on the warp and never came back. Root cause: the reunion cutscene
was hooked on `MAP_SCRIPT_ON_RESUME`
(`data/maps/SouthernIsland_Exterior/scripts.inc`), which fires during the
map's load-in — *before* the fade-in transition finishes. The `lockall`/
`msgbox` sequence was running and correctly waiting for a button press, but
the player could never see the prompt to press it — an invisible deadlock
behind the black screen, not a true CPU hang. Fixed by switching to
`MAP_SCRIPT_ON_FRAME_TABLE` instead — the same hook Sootopolis's Kyogre/
Groudon awakening cutscene uses, confirmed to only fire once the map is
actually visible. Since frame-table entries compare vars (not flags), used
`VAR_TEMP_5 == 0` as an always-true per-visit gate (temp vars reset every map
load) wrapped in `SouthernIsland_Exterior_EventScript_TryReunion`, which does
the real one-time gating via `goto_if_set FLAG_QUEST_LATI_REUNITED,
Common_EventScript_NopReturn` — the identical goto-into-NopReturn pattern
`SouthernIsland_Interior_EventScript_TryRemoveLati` already uses elsewhere in
this same map pair. Lesson: `ON_TRANSITION`/`ON_RESUME` are for quick,
non-interactive setup; anything with `lockall`/`msgbox`/camera work that
needs to be seen belongs on `ON_FRAME_TABLE`.

The `ON_FRAME_TABLE` fix worked (Southern Island loaded, reunion cutscene
played, Lati rejoined the party) — but exposed a **third bug**, found when
the player then talked to the existing sailor NPC and finished that
conversation: player movement was permanently locked afterward. Root cause:
`MapHeaderCheckScriptTable` (`src/script.c:299-326`) and
`TryRunOnFrameMapScript` (`src/script.c:353-362`) always return `TRUE` on any
table match — regardless of what the matched script actually does — and
`ProcessPlayerFieldInput` short-circuits (returns early, before normal
movement handling) whenever that happens. `VAR_TEMP_5 == 0` never stopped
being true (nothing changed it), so *every single frame, forever*, this
entry kept matching and re-triggering `TryReunion` — which, after the first
real reunion, was a fast no-op via the `FLAG_QUEST_LATI_REUNITED` check, but
still "consumed" the frame every time, permanently starving normal input
processing. This only became visible once the sailor's own dialogue (a
separate script that legitimately holds the field) finished and handed
control back — the very next frame, the table match immediately re-hijacked
it. Sootopolis's real cutscene avoids this because its own script changes
`VAR_SOOTOPOLIS_CITY_STATE` so the table stops matching — I'd left the
compared var permanently unchanged. Fixed by having
`SouthernIsland_Exterior_EventScript_TryReunion` bump `VAR_TEMP_5` to `1`
as its very first action, so the table entry only ever matches once per map
visit; `FLAG_QUEST_LATI_REUNITED` remains the real "already reunited" guard
across future visits (temp vars reset on every map load, so the table
re-arms — but immediately disarms again — each time the player returns).

Third fix confirmed live in mGBA — full chain verified end-to-end: catch
roaming Lati → Birch hint → train 5 levels → awaken cutscene → Birch PokéNav
call → ticket handoff → Southern Island reunion → player movement unaffected
afterward → vanilla opposite-species encounter untouched. ✅

Dialogue pass: user rewrote `QuestLati_Text_BirchGivesTicket` with a proper
narrative reason for the ticket (Birch's friend spotted the awakened Lati
flying south). Caught and fixed two rendering bugs in the edit — a message
box with 3 lines (engine only shows 2) and "POKéMON" split across a `\p`
box-break with a hyphen (`\p` opens a new box, not a same-box line-wrap;
splitting a word across two boxes like that isn't a working pattern here).
Rebuilt clean; user is satisfied with the result. Remaining quest text
(`QuestLati_Text_BallTrembling/FlewOff/BirchPokenavCall`,
`QuestLati_Text_ReunionApproach/Complete`,
`LittlerootTown_ProfessorBirchsLab_Text_LatiHint`) is still first-draft and
open for further workshopping whenever wanted.

**Current phase:** Phase 1 in progress — Eon Ticket sidequest complete and verified end-to-end.

**Next up:**
- Old Sea Map, Aurora Ticket, Mystic Ticket still need their own in-game
  unlock hooks (same "Mystery Gift dead end" shape, different narrative hook
  each).
- Ability info on the BATTLE MOVES page remains parked until after Phase 2
  (Match Call/rematch overhaul), per user's call.

**Known issues / open decisions:**
- `ShouldLatiAwaken` re-scans the party every step once the quest is started
  (until it awakens) — cheap (6-slot loop), consistent with other per-step
  checks already in the engine (egg hatch, friendship), not a concern.
- The awaken/reunion "fly" animations use a fixed virtual object id
  (`LATI_VOBJ_ID = 1`); fine since only one is ever alive at a time, but
  would need a real id-allocation scheme if another feature ever needs
  virtual objects concurrently.

---

## 2026-07-16 — Reusable TMs

- TMs now behave like HMs — never removed from the bag after teaching a move.
- Single change point: `Task_LearnedMove` (`src/party_menu.c:4769`) had
  `if (item < ITEM_HM01) RemoveBagItem(item, 1);` — TM vs HM was already just an
  item-ID range check (TMs 289–338, HMs 339–346), so this was the only call
  site removing TMs from the bag. Deleted the check/call entirely (and the now-
  unused `item` local). No config flag added — this is a permanent PG3 design
  choice, not a toggle.
- Builds clean with `make modern`. Verified live in mGBA (2026-07-16): TM stays
  in the bag after teaching a move. ✅

**Current phase:** Phase 1 in progress — reusable TMs shipped and verified.

**Next up:**
- Ability info deferred until after the Match Call/rematch overhaul (Phase 2) —
  user's call, tabled for now.
- Pick the next Phase 1 item: event tickets, island restoration, trade-evolution
  alternatives, or encounter overhaul.

**Known issues / open decisions:** none open right now.

---

## 2026-07-16 — B-to-run allowed everywhere (indoors + out)

- Still gated behind `FLAG_SYS_B_DASH` (Running Shoes) and the B button — untouched.
- Removed the per-map `allowRunning` restriction that blocked running in ~290 of
  518 maps (mostly interiors). `IsRunningDisallowed()` (`src/bike.c:1056`) now
  only checks `IsRunningDisallowedByMetatile()` — metatile-level restrictions
  (long grass, hot springs, Pacifidlog logs, `MB_NO_RUNNING` tiles, Fortree
  bridge elevation) are untouched, so those terrain-specific rules still apply.
  Left the dead legacy `RS_IsRunningDisallowed()` (mapType-based, unused, R/S
  leftover) alone — not on any call path.
- Builds clean with `make modern`. Verified live in mGBA (2026-07-16): running
  works inside buildings; Pacifidlog Town log-crossing tiles still block
  running as expected (metatile-level restriction intact). ✅

**Current phase:** Phase 1 in progress — run-anywhere QoL shipped and verified.

**Next up:**
- Continue Phase 1 comfort features: reusable TMs. (Faster text dropped from
  the list — default speed is fine, no complaint.)
- Implement BATTLE MOVES page ability info (see decision below).

**Known issues / open decisions:**
- **Decided:** BATTLE MOVES page will show ability name + description (resolves
  the 2026-07-15 open question). **Parked:** highlighting which moves are
  actually modified/boosted by the ability (e.g. STAB-style highlighting for
  an ability like Overgrow) — too advanced for what we have to work with right
  now; revisit later if wanted.

---

## 2026-07-15 — Summary screen full QoL pass (INFO / SKILLS / DATA pages)

All changes are in `src/pokemon_summary_screen.c` and the two background binaries in `graphics/summary_screen/`.

### INFO page
- Removed: ability name and trainer memo section.
- Added: held item ("ITEM: [name]"), ribbon count, current EXP ("EXP PTS") and EXP to next level ("TO NEXT LV.").
- OT name and OT ID retained.
- Binary-patched `page_info.bin` to clear the old "ABILITY" and "TRAINER MEMO" label rows (rows 8 and 13, cols 11–29 set to spacer tile).

### SKILLS page
- Removed: separate "STATS" heading window (blocked by the panel header at row 2 — will be added as background art in Tilemap Studio later).
- Stats section (rows 4–9): two-column layout — HP/ATK/DEF on left (cols 11–20, `PSS_DATA_WINDOW_SKILLS_STATS_LEFT`), SATK/SDEF/SPD on right (cols 21–29, `PSS_DATA_WINDOW_SKILLS_STATS_RIGHT`). Nature modifier printed inline next to the affected stat as `(+)` or `(-)`.
- Nature section (rows 10–13): `"NATURE: [name]"` on line 1, `"+ATK / -SP.ATK"` (or `"No stat changes"`) on line 2.
- Ability section (rows 14–19): `"ABILITY: [name]"` on line 1, description on line 2.
- Binary-patched `page_skills.bin` to shift and blank the old stat-label tile rows.

### DATA page
- Replaced egg group footer with a met description section (`PSS_DATA_WINDOW_DETAILS_MET`, rows 3–6): compact single-line format — "Met: Lv.5, Route 101", "Obtained in a trade", "Fateful encounter at Lv.30", etc. IV/EV column headers ("IV" / "EV") printed on the second line of this window so they sit directly above the stat table.
- IV/EV stat table (`PSS_DATA_WINDOW_DETAILS_STATS`, rows 7–18): 6 stat rows at 15 px each, no separate header row. Same two-column IV / EV layout as before.
- Removed all egg group strings (`sEggGroupNames[]` etc.) from this file — egg group belongs in the Pokédex, to be re-added there later.

### Pending visual polish (not yet done)
- Tilemap Studio redesign of `page_info.bin` and `page_skills.bin` to match the new 3-section layouts — deferred until all 5 pages are settled.
- BATTLE MOVES page: add ability name + description (decided 2026-07-16, see
  that entry — implementation still pending). Move-modifier highlighting
  parked as too advanced for now.

**Current phase:** Phase 1 in progress — summary screen QoL pass complete (placeholder layouts confirmed in mGBA; background art redesign pending).

**Next up:**
- Tilemap Studio pass on INFO and SKILLS backgrounds (Windows machine).
- Decide whether to add ability info to the BATTLE MOVES page.
- Continue Phase 1: B-to-run, faster text, reusable TMs, or event tickets.

**Known issues / open decisions:**
- "STATS" section header on SKILLS page is absent — panel header at rows 0–2 blocks any BG0 window there; needs to be baked into the tilemap background.
- IV/EV column x-positions (94 / 136) chosen by calculation; verify pixel alignment after Tilemap Studio redesign.

---

## 2026-07-05 — Summary screen "POKéMON DATA" page (IVs / EVs / Egg Groups)

- Added a new 5th summary screen page `PSS_PAGE_DETAILS` ("POKéMON DATA") accessible by scrolling past the Contest Moves page.
- Page shows per-stat IV (0–31) and EV (0–255) for all 6 stats in a two-column table, plus the Pokémon's egg group(s) in a footer line ("EGG: Group1 / Group2").
- Implementation in `src/pokemon_summary_screen.c`:
  - New page enum entry, window constants, static title window at baseBlock=449 (all prior dynamic-window baseBlocks shifted +22 to accommodate).
  - New `sPageDetailsTemplate[]` with STATS window (18×14 tiles, tilemapTop=4) and EGG window (18×2 tiles, tilemapTop=18).
  - New `sEggGroupNames[]` table (16 entries, all 15 Gen-3 egg groups + Undiscovered).
  - `PrintDetailsPageText()` / `Task_PrintDetailsPage()` read IVs and EVs via `GetMonData()` and egg groups via `gSpeciesInfo[species].eggGroups[]`.
  - Reuses `gSummaryPage_Skills_Tilemap` as the background for the new page; content windows cover the content area.
  - STATUS sliding window restricted to `PSS_PAGE_SKILLS` only (was: any non-Moves page).
  - Egg Pokémon skip the page content (return early); navigation to the page is still technically possible but harmless.
- Not yet built/tested in mGBA — needs a build run on Ubuntu to confirm compile and in-game layout.

**Current phase:** Phase 1 in progress — Altering Cave + summary DATA page shipped (pending build).

**Next up:**
- Build and verify DATA page in mGBA (pixel alignment may need tweaking after seeing it).
- Continue Phase 1: B-to-run, faster text, reusable TMs; or event ticket availability.

**Known issues / open decisions:**
- IV/EV column pixel positions (x=94 for IV column, x=136 for EV column) chosen by calculation; may need minor adjustment after visual check in mGBA.
- Skipped physical/special split per user request.

---

## 2026-06-20 — Phase 0 complete; Phase 1 kickoff (Altering Cave scoped)
- Installed the devkitARM toolchain on Ubuntu.
- Cloned pret/pokeemerald; renamed its remote to `upstream`.
- Created the private PG3 repo; set it as `origin`; pushed `master`.
- Built with `make modern` — compiles clean.
- ROM boots in mGBA; new game starts. ✅
- Installed Claude Code in the repo on Ubuntu; confirmed it reads CLAUDE.md. ✅
- Installed Porymap on the Windows machine; repo synced there. ✅
- Picked Altering Cave (Phase 1, cut-content) as the first real task and scoped it:
  - Map: `data/maps/AlteringCave/` — warp-only from Route 103, no entry gate, sets
    `FLAG_LANDMARK_ALTERING_CAVE` on transition.
  - Encounters already fully built: 9 tables in `src/data/wild_encounters.json`
    (`gAlteringCave1`-`9`) — Zubat, Mareep, Pineco, Houndour, Teddiursa, Aipom,
    Shuckle, Stantler, Smeargle — each with a guaranteed held item
    (`src/pokemon.c:2114-2125`).
  - Active table is picked by `VAR_ALTERING_CAVE_WILD_SET`
    (`src/wild_encounter.c:318-326`), but the **only** thing that ever changes that
    var is the vanilla Mystery Gift script (`data/scripts/gift_altering_cave.inc`).
    With no Mystery Gift hardware/distribution available, the var never leaves 0, so
    8 of the 9 species are practically unreachable — that's the actual "cut content"
    bug, not missing data.

- Decided the Mystery Gift replacement: random-per-visit. `AlteringCave_OnTransition`
  (`data/maps/AlteringCave/scripts.inc`) now rolls `random NUM_ALTERING_CAVE_TABLES`
  and stores it via `copyvar VAR_ALTERING_CAVE_WILD_SET, VAR_RESULT` on every entry.
  Added `#include "constants/wild_encounter.h"` to `data/event_scripts.s` so the
  table-count constant resolves in the script context. Builds clean with
  `make modern`; confirmed live in mGBA (user set up a save near Route 103) —
  species rotates across repeated visits as intended. ✅
- Noted to revisit this mechanic (e.g. trainer-ID-based fixed-per-save, closer to
  vanilla) during the Phase 1 "Encounter overhaul" pass rather than now.

**Current phase:** Phase 0 done. Phase 1 in progress — Altering Cave fix shipped.

**Next up:**
- Pick the next Phase 1 item (event tickets / cut-content islands, trade-evolution
  alternatives, or the physical/special split QoL work).

**Known issues / open decisions:** none open right now.

---

### Entry template (copy for each session)
## YYYY-MM-DD — <short title>
- <what changed>

**Next up:** <tasks>
**Blockers:** <none / details>