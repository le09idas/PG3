# PG3 Dev Log

Running journal of progress. Newest entry on top. At the end of each session add:
what changed, what's next, and any blockers. This is the canonical "where are we" —
read it first.

---

## 2026-07-19 — Var-budget audit + shared/bit-packed rematch-call scheme

**Budget question:** with 8 leaders eventually needing their own rematch
call system, the naive "2 vars per leader" estimate (16 vars) badly
strained the var budget -- checked `include/global.h`'s `SaveBlock1`
layout precisely: `vars[VARS_COUNT]` sits at a fixed offset, and the whole
struct (`sizeof: 0x3D88`) has exactly 120 bytes (60 more `u16` vars) of
headroom before hitting the 4-sector budget reserved for it
(`SECTOR_DATA_SIZE * 4`). Expanding `VARS_END` is technically free
space-wise and needs no new file, but shifts every field *after* `vars[]`
to a new byte offset -- silently corrupting any save written before the
change (every `test_game_saves/` checkpoint included), since nothing in
pokeemerald migrates old saves to a new `SaveBlock1` layout automatically.
That'd be a real, custom, high-risk piece of save-file surgery to build,
not just "resize and copy."

**Turned out to be unnecessary.** Neither var actually needs to be
per-leader: the step-counter's only job is pacing the call by ~50 steps,
which can be shared across all leaders (one pending call shares the wait
timer); the "last acknowledged tier" value only ranges 0-6 (3 bits), so
bit-packing 5 leaders per `u16` var fits all 8 gym leaders in 2 vars total.
Retrofit-tested on Roxanne now, before replicating to the other 7:
- `VAR_GYM_REMATCH_CALL_STEP_COUNTER` (shared, `vars.h` 0x40FC) replaces
  the old per-leader step counter.
- `VAR_GYM_REMATCH_LAST_ACK_TIERS_0`/`_1` (0x40FD/0x40FE) bit-pack up to
  10 leaders' "last acknowledged tier" values, 3 bits each.
- New `GYM_REMATCH_LEADER_*` enum + `GetLastAcknowledgedRematchTier()` /
  `SetLastAcknowledgedRematchTier()` helpers (`src/field_specials.c`) do
  the pack/unpack in C; scripts are unaffected -- they still just call a
  special (`SetRoxanneLastAcknowledgedRematchTier`, reads `VAR_0x8008`)
  the same as before, since bit-packing can't be done with `copyvar`.

Net result: the entire 8-leader rollout should cost **~3 vars total**
instead of 16, comfortably within the 15 that were free -- no `VARS_END`
expansion, no save-migration risk, needed for this scope. Confirmed with
user that this doesn't block a future Kanto/Johto import either, since the
binding constraint there is the separate `TRAINER_*` ID budget (~9 free,
~100 reserved), not vars.

**Not yet re-verified live.**

---

## 2026-07-19 — Call cadence: per-fight, not per-readiness-window

Live test on the post-Battle-Frontier save exposed the exact tradeoff
flagged earlier: since the save's milestone tier was already maxed before
ever touching a Roxanne rematch, the old "one call per readiness window"
design meant only a single call ever fired (tier 2/Flannery), then silence
through tiers 3-6 despite each becoming genuinely fightable in turn.

User's fix suggestion: tie the call to finishing each individual
benchmark rather than the overall readiness window. Implemented by
swapping the trigger condition in `ShouldDoRoxanneRematchCall()`
(`src/field_specials.c`) from a "already called, still ready" flag to
comparing `GetRoxanneNextRematchTier()` (the specific next fight) against
a re-added `VAR_ROXANNE_LAST_ACKNOWLEDGED_REMATCH_TIER` -- still gated by
`IsRoxanneRematchReady()` so it can never promise an unreachable fight, but
now re-arms itself the instant the "next fight" value changes (i.e., right
after each individual rematch is won), rather than only when the player's
overall milestone tier changes. Removed the now-unused
`FLAG_CALLED_ABOUT_ROXANNE_REMATCH`.

**Not yet re-verified live** -- next test should confirm beating `ROXANNE_2`
immediately re-arms the tier-3 call (~50 steps later) even when milestone
tier was already maxed from the start.

---

## 2026-07-19 — Per-tier PokeNav call flavor text

Replaced the single generic rematch-call message with 5 distinct ones, one
per tier (`RustboroCity_Gym_Text_RoxanneRematchCall_Tier2` through `_Tier6`,
`data/maps/RustboroCity_Gym/scripts.inc`), each referencing the specific
milestone that unlocked it (Flannery / Victory Road / Elite Four / Frontier
Symbols / all-Gold). New `GetRoxanneNextRematchTier()`
(`src/field_specials.c`, registered as a special) determines which one to
show -- deliberately walks the same undefeated-trainer sequence as
`IsRoxanneRematchReady` rather than using `GetRoxanneAllowedRematchTier()`
directly, since the call should describe the *next fightable battle*, not
the player's overall milestone tier (which can be ahead of it, since
rematches can't be skipped). `RustboroCity_Gym_EventScript_RoxanneRematchCall`
now branches on this value to pick the right `pokenavcall`.

Also confirmed with user: no prior instruction exists to keep any specific
tier as a double battle -- the original ask was to remove doubles across
the board, which was already done (all of `TRAINER_ROXANNE_2`-`_5` set to
`.doubleBattle = FALSE`).

Match-call cadence (one call per "readiness window," not one per individual
chained battle when catching up across multiple missed tiers at once) was
discussed and left as-is per user's call -- revisit when generalizing to
the other 7 gym leaders.

**Not yet re-verified live.**

---

## 2026-07-19 — Fixed: blink indicator / PokeNav call promising a rematch that wasn't actually ready

User report: after the off-by-one gate fix, Roxanne correctly blocked the
premature Aerodactyl fight, but the PokeNav match call list still showed
her name as "ready for rematch" (blinking pokeball). Investigated and
found the root cause -- both the blink indicator and the new proactive
call were relying on vanilla's `trainerRematches[]`/`UpdateRematchIfDefeated`
bookkeeping, which only tracks "is there an undefeated trainer next in the
sequence," with **no awareness of the tier gate at all**. So the moment
*any* milestone unlocked tier 2+, she'd get marked "ready" for whichever
`TRAINER_ROXANNE_N` came next in line, even if that N actually required a
higher tier than the player currently had -- promising a battle the gym's
own gate would then correctly refuse.

**Fix:** added `IsRoxanneRematchReady(tier)` (`src/field_specials.c`,
static) -- mirrors the gym script's own gate chain in C (walks
`TRAINER_ROXANNE_2..5`, checks `HasTrainerBeenFought` against each one's
required tier) as the single source of truth for "would talking to her
right now actually start a battle." Both the vanilla bookkeeping sync
inside `GetRoxanneAllowedRematchTier()` and `ShouldDoRoxanneRematchCall()`
now gate on this instead of on raw tier value. Replaced the call system's
"last acknowledged tier" var tracking with a simpler
`FLAG_CALLED_ABOUT_ROXANNE_REMATCH` that clears itself once
`IsRoxanneRematchReady` goes false again (i.e., once the pending rematch is
actually fought) -- removed the now-unused
`VAR_ROXANNE_LAST_ACKNOWLEDGED_REMATCH_TIER` reservation. Added
`#include "constants/opponents.h"` to `field_specials.c` for the
`TRAINER_ROXANNE_*` constants.

**Not yet re-verified live.**

---

## 2026-07-19 — Rematch call system + off-by-one gate bug caught by live testing

**PokéNav rematch-call system built:** `ShouldDoRoxanneRematchCall()`
(`src/field_specials.c`) checks `GetRoxanneAllowedRematchTier()` against a
new persistent "last acknowledged tier" var every qualifying outdoor step;
once it's gone up, counts 50 steps then fires
`RustboroCity_Gym_EventScript_RoxanneRematchCall`
(`data/maps/RustboroCity_Gym/scripts.inc`), mirroring her existing
vanilla registration-call script. New vars
`VAR_ROXANNE_REMATCH_CALL_STEP_COUNTER` /
`VAR_ROXANNE_LAST_ACKNOWLEDGED_REMATCH_TIER` (tail of `vars.h`). Wired
into `TryStartStepCountScript` (`src/field_control_avatar.c`).

**Blink-indicator bug found before testing, fixed proactively:** the
PokéNav list's "wants a rematch" pokéball is purely
`trainerRematches[REMATCH_ROXANNE] != 0`, and `UpdateRematchIfDefeated()`
was running unconditionally at the top of `GetRoxanneAllowedRematchTier()`
-- meaning it would've lit the blink the moment `TRAINER_ROXANNE_1` was
beaten and the player took one outdoor step, regardless of whether any
real milestone (even tier 2) had been met. Restructured the function to
compute the tier first and only sync the bookkeeping when `tier != 0`.

**Off-by-one gate bug found via live testing:** user reported being able
to rematch Roxanne a 4th time (reaching her Aerodactyl team,
`TRAINER_ROXANNE_5`) on a save that should only have unlocked 3 rematches
(E4 beaten = tier 4, no Frontier symbols yet = tier 5 not unlocked). Root
cause: the gate chain in `RustboroCity_Gym_EventScript_Roxanne` set
`VAR_0x8008` to `1,2,3,4` right before checking whether
`TRAINER_ROXANNE_2,3,4,5` were undefeated -- one less than the tier each
of those trainers actually represents (2,3,4,5). So checking `ROXANNE_5`
compared the player's tier against `4` instead of `5`, letting a tier-4
player through a stage early. Fixed by setting `VAR_0x8008` to `2,3,4,5`
before each respective check, and `6` as the final fallthrough value (for
the tier-6 level-bonus reuse). Verified against the tier=4 case by hand:
`ROXANNE_2`(needs 2)/`_3`(needs 3)/`_4`(needs 4) all correctly pass at
tier 4; `_5`(needs 5) now correctly fails until Frontier symbols are
earned.

**Not yet re-verified live** — next session should confirm the corrected
gate blocks Aerodactyl until tier 5, and that the PokéNav call/blink
system behaves as described above.

---

## 2026-07-19 — "Uncapped" rematch tiers prototype: level-scaling instead of new rosters

User pitched a full rematch redesign for all 8 gym leaders + Elite Four +
rivals (5 milestone flags: Flannery beat / Victory Road Wally / E4 beat /
Silver medals / Gold medals, with a per-trainer eligibility table), plus a
PokéNav call-when-eligible idea and Steven referring the player back to E4.
Central open question before committing to that design: whether rematch
tiers *have* to cost a dedicated `TRAINER_*` ID each (the scarce resource
identified earlier this session), or whether a tier could just reuse an
existing roster at a higher level.

**Answer: yes, and it's now proven working.** Added `gTrainerPartyLevelBonus`
(self-clearing `EWRAM_DATA u8`, `src/battle_main.c`) plus a
`GetTrainerMonLevel()` helper wired into all 4 `CreateNPCTrainerParty`
party-building cases (clamped to `MAX_LEVEL`). Exposed via `battle.h`.
`GetRoxanneAllowedRematchTier()` (`src/field_specials.c`) now has a tier 6:
gated on all 7 Battle Frontier Gold Symbols, it sets the bonus to +10 and
returns 6 — no new trainer data or script changes needed, since
`GetRematchTrainerIdFromTable`'s own "already beaten at max stage" fallback
already resolves back to `TRAINER_ROXANNE_5` once tiers 2-5 are cleared, and
6 satisfies the script's existing `>= 5` gate check as-is.

**Verification detour:** temporarily lowered the tier-6 gate to
`FLAG_SYS_GAME_CLEAR` (gold symbols aren't reachable on the test save) to
test live, plus temporary debug msgboxes showing the computed tier/bonus.
First read looked like a failure ("Aerodactyl still at 57") — turned out to
be a flawed test, not a bug: since the test save already had
`FLAG_SYS_GAME_CLEAR` set from the start, the temp gate meant *every*
rematch fought during the whole debugging session (tiers 2 through 6) had
already been getting the +10 bonus from the very first fight, so there was
never an unbonused baseline to compare against. Checked
`TRAINER_ROXANNE_5`'s real data (`src/data/trainer_parties.h`): Aerodactyl
is defined at level 47. `47 + 10 = 57` — matches exactly. Mechanism
confirmed working. All debug scaffolding removed, temp gate reverted to the
real Gold Symbols condition.

Added `DEV_NOTES/ROXANNE_REMATCH_ROSTERS.md` — full tier 1-6 roster
reference table, since the user doesn't love the current hand-built
lineups and wants to revisit them later without re-deriving from
`trainer_parties.h` each time.

**Current phase:** Phase 2 design discussion, not yet committed. The
5-milestone table, per-trainer eligibility, PokéNav call system, and
lineup-shuffle idea are all still just discussed, not built.

**Next up:**
- Decide real values for the tier-6 style bonus (±level amount) and how
  many "scaled" tiers stack on top of a hand-built base roster, now that
  the mechanism is proven.
- Design the generic PokéNav "your rematch is ready" call system (mirrors
  `ShouldDoWallyCall`/the Aurora quest's Institute calls).
- Build the 5-milestone flag set + per-trainer eligibility table (Roxanne's
  4 real milestones already map 1:1 to what's built; needs generalizing to
  the other 12 trainers).
- Clarify what "Wattson's quest" (mentioned as his milestone-1
  prerequisite) refers to — not yet answered.
- Revisit Roxanne's actual lineups (species/levels/movesets) — user
  flagged them as not-great, deferred for later.

---

## 2026-07-19 — Roxanne rematch fix: vanilla's RNG "ready" bookkeeping still gated the battle

User report: on a save with all 8 badges + Elite Four beaten, talking to
Roxanne never actually started a rematch battle, despite the new tier gate
(previous entry below) looking correct.

**Root cause, found via targeted debug msgboxes (not guesswork) added
temporarily to `RustboroCity_Gym_EventScript_Roxanne`:** the tier gate itself
was working perfectly (confirmed tier=4, gate=1, correctly entered the
rematch branch) — but `trainerbattle_rematch_double`'s underlying script
chain (`EventScript_TryDoDoubleRematchBattle` in
`data/scripts/trainer_battle.inc`) does its own independent check before
starting a battle: `specialvar VAR_RESULT, IsTrainerReadyForRematch` /
`goto_if_eq VAR_RESULT, FALSE, EventScript_NoDoubleRematchTrainerBattle`.
`IsTrainerReadyForRematch` reads `gSaveBlock1Ptr->trainerRematches[]` —
vanilla's own RNG bookkeeping (31%-per-battle roll via
`UpdateGymLeaderRematch`), completely separate from the new story-flag gate.
Since the new gate replaces the *decision to route toward a rematch* but
never touched that bookkeeping, it stayed zeroed for Roxanne and the battle
command silently no-op'd straight to the post-battle text every time,
regardless of tier.

**Fix:** `GetRoxanneAllowedRematchTier()` (`src/field_specials.c`) now opens
with `UpdateRematchIfDefeated(REMATCH_ROXANNE)` — an existing, non-static
vanilla helper already used the same way elsewhere (`src/match_call.c:1528`,
for the Match Call system) that recomputes and force-syncs
`trainerRematches[REMATCH_ROXANNE]` from `HasTrainerBeenFought()` state.
Added `#include "battle_setup.h"` and `#include "constants/rematches.h"` to
`field_specials.c`. This keeps the two systems in sync without touching
vanilla's shared RNG code path (still zero blast radius on the other 7
leaders + Elite Four). Debug msgboxes fully removed after confirming the
fix. Builds clean; **this specific fix has not yet been re-verified live in
mGBA** — next session should confirm the rematch battle actually starts now
on the same save that surfaced the bug.

**Lesson for scaling to the other 7 leaders:** any deterministic gate that
still ends in vanilla's `trainerbattle_rematch`/`_double` macro needs this
same `UpdateRematchIfDefeated(REMATCH_<leader>)` call — the macro's
"ready" check isn't optional/bypassable from script level, so every future
leader's gating function must include it too.

**Follow-up: rematch was still a double battle after the fix above.**
Turns out vanilla's design choice was two-layered — the script had also been
switched from `trainerbattle_rematch_double` to `trainerbattle_rematch`
(single) per the user's call ("not sure why they wanted this" — no strong
reason to keep vanilla's doubles-only rematch format), but the battle format
is actually decided by the trainer's own data, not the script macro:
`TRAINER_ROXANNE_2`-`_5` all had `.doubleBattle = TRUE` baked into their
entries in `src/data/trainers.h`. Flipped all 4 to `FALSE`. Removed the
now-orphaned `RustboroCity_Gym_Text_RoxanneRematchNeedTwoMons` text (was
only reachable via the double-battle macro's 4th argument). Builds clean;
**not yet re-verified live**.

---

## 2026-07-19 — Phase 2 prototype: deterministic Roxanne rematch gating

First real Phase 2 implementation, per ROADMAP's "prototype ONE gym leader
end-to-end before scaling out." Picked Roxanne, the established reference
(already has 5 baked-in vanilla tiers, `TRAINER_ROXANNE_1`-`_5`).

**What vanilla actually does (confirmed via code read, not assumption):**
`UpdateGymLeaderRematch()` (`src/gym_leader_rematch.c`) rolls `Random() % 100
<= 30` on a cadence (every ~20 trainer battles / ~60 wild battles,
`TryUpdateGymLeaderRematchFromTrainer`/`_Wild` in `src/battle_setup.c`),
gated only by `FLAG_SYS_GAME_CLEAR`. On success, it finds whichever
gym leader(s) are "furthest behind" (fewest tiers beaten) and randomly
flips ONE of their `trainerRematches[]` entries non-zero. Critically,
`trainerRematches[]`'s stored value is **only ever read as a boolean**
everywhere in the codebase (checked `!= 0`/truthy in every call site, in
`match_call.c`/`pokenav_match_call_*.c`/`battle_setup.c`) — it just gates
whether a rematch is offered *at all*. Which specific tier you actually
fight is separately and correctly resolved by `GetRematchTrainerIdFromTable`
purely from `HasTrainerBeenFought()` checks against the 5 real trainer IDs —
already fully deterministic/sequential on its own. So the *only* thing
standing between "flag-gated" and vanilla was the initial RNG eligibility
gate — but that gate doesn't stop the player from immediately chaining
through tiers 3/4/5 back-to-back once *any* rematch is offered, since
sequential resolution doesn't know about story milestones.

**Design:** rather than touch the shared `UpdateGymLeaderRematch`/
`ShouldTryRematchBattle`/`trainerRematches[]` system (which all 8 leaders +
regular trainers use), built a fully parallel, independent gate just for
Roxanne — zero shared-code blast radius, easy to rip out/adjust before
scaling to the other 7. New `GetRoxanneAllowedRematchTier()`
(`src/field_specials.c`) returns the highest tier index (2-5) currently
unlocked:
- Tier 2 (Roxanne/Brawly/Wattson's first rematch) — `FLAG_BADGE04_GET`
  (Flannery beaten).
- Tier 3 — `FLAG_BADGE08_GET` (Juan beaten) OR
  `FLAG_DEFEATED_WALLY_VICTORY_ROAD` (Victory Road cleared).
- Tier 4 — `FLAG_SYS_GAME_CLEAR` (Elite Four beaten once).
- Tier 5 — 7+ Battle Frontier Symbols total (summed across all 7 facilities
  via the already-public `GetPlayerSymbolCountForFacility`, each worth up to
  2 — Silver + Gold).

`data/maps/RustboroCity_Gym/scripts.inc`'s `RustboroCity_Gym_EventScript_
Roxanne` now calls this special instead of `ShouldTryRematchBattle`, walks a
`goto_if_not_defeated` chain against `TRAINER_ROXANNE_2`-`_5` to determine
which tier would be fought next (sequential, matching vanilla's own
resolution logic), and only proceeds to `trainerbattle_rematch_double` if
that next tier is `<=` the currently-unlocked tier — otherwise falls through
to the same vanilla "post-battle" message as before. This lets the player
freely re-fight any tier they've already unlocked, but blocks racing ahead
of the story milestones.

Builds clean with `make modern`. Not yet tested live — would need a save
with all 8 badges + Elite Four cleared to meaningfully exercise tiers 3-5;
tier 2 (Flannery beaten) is the easiest to verify from an earlier save.

**Current phase:** Phase 2 in progress — Roxanne prototype implemented, pending mGBA verification.

**Next up:**
- Verify in mGBA: rematch stays locked pre-Flannery, unlocks at each
  milestone in order, doesn't let the player skip ahead.
- Once verified, decide the gating scheme for the other 7 leaders (some
  share Roxanne's tier-2 trigger per the original wave-design idea — Brawly/
  Wattson beaten-Flannery gate — others need their own story-position-
  appropriate flags) and scale out.
- Tier 6/7 (new trainer data beyond vanilla's 5) still needs the
  `DEV_NOTES/DESIRED_MATCH_CALL_ROSTERS.md` trim decided and the freed
  trainer-ID budget actually reclaimed before those can be built.
- Elite Four has **zero** vanilla rematch escalation (`IsRematchForbidden`
  explicitly blocks `rematchTableId >= REMATCH_ELITE_FOUR_ENTRIES`) — giving
  them tiers at all (the user's "tier 7" idea) needs an entirely new system,
  not an extension of the gym leader pattern.

---

## 2026-07-19 — Roadmap reorganization: region-tagged tickets + 2 items rejected

**Old Sea Map (→ Mew) and Mystic Ticket (→ Ho-Oh/Lugia) reassigned** from
generic Phase 1 scope to the region phases their legendaries thematically
belong to — Old Sea Map/Mew folded into Phase 5 (Kanto import), Mystic
Ticket/Ho-Oh/Lugia folded into the new Phase 6 (Johto import, contingent on
the Heart & Soul permissions ask). Eon and Aurora Tickets stay in Phase 1 as
the Hoenn-native pair (both already shipped). North-star line updated to
name Johto/Phase 6 explicitly instead of leaving the third region as a bare
"TBD."

**Two items dropped from scope entirely, not just deferred:**
- **Physical/Special split** — user's call: keeping Gen 3's authentic
  type-based damage-category split is part of what makes this a *Gen 3*
  game. Changing it would remove real Gen 3 design/challenge, not fix a
  flaw.
- **Trade-evolution alternatives** — user's call: real trading between this
  hack and other ROM hacks (including future PG3 builds) is technically
  achievable, so there's no need to bypass canonical trade-evolution
  requirements (Machoke, Kadabra, Haunter, etc.) with in-game workarounds.

Both recorded in ROADMAP.md's new "Rejected" section with rationale, so
they don't get silently re-proposed later.

**Current phase:** Phase 1 nearly done — only Jirachi (needs its own design
pass) and the Tilemap Studio summary-screen work remain there.

**Next up:**
- Encounter overhaul is now the only untouched "big" Phase 1 item.
- Otherwise: Phase 2 (Match Call), Jirachi design, or Tilemap Studio,
  whichever the user picks up next.

---

## 2026-07-19 — Weather-not-reverting bug fix (Ubuntu)

First mGBA test of the puzzle sites (post-Porymap-placement): script/trigger
mechanics worked, but the anomaly weather (Route 111 rain, Route 119
sandstorm) never reverted after solving — stuck permanently instead of
going back to normal.

**Root cause:** `QuestWeather_EventScript_SiteResolved`
(`data/scripts/quest_weather.inc`) hardcoded `setweather WEATHER_SUNNY` as
"the resolved state" for all 3 sites. That's only actually correct for
Mossdeep. Route 111 and Route 119 don't have "sunny" as their true
baseline — Route 111's desert sandstorm is dynamically re-evaluated by
player Y-coordinate every map transition (`Route111_EventScript_
CheckSetSandstorm`), and Route 119's weather is driven by sparse coord-event
trigger tiles clustered only near the far north (y≈6-13) and south
(y≈130-137) ends of the route — nowhere near the puzzle site (y≈28-30), so
there's no natural "correction" happening nearby either. Freezing both at
sunny was a real bug, not a data typo.

**Fix:** `SiteResolved` no longer decides the weather — it only calls
`doweather` now (applies whatever's already pending). Each site's own tier-C
completion script sets the correct pending weather immediately before
jumping in:
- Route 111: `call Route111_EventScript_CheckSetSandstorm` — reuses the
  exact same position-based logic the map's own `OnTransition` already
  relies on, so it's provably correct for wherever the player is standing
  (confirmed the markers' coordinates fall inside its sandstorm Y/X range).
- Route 119: `resetweather` (`SetSavedWeatherFromCurrMapHeader`) — falls
  back to the map's own default (`WEATHER_SUNNY` per `map.json`). Given the
  puzzle site isn't covered by any of Route 119's real weather-cycle
  trigger tiles, there isn't a more "correct" special value to restore
  here — sunny is the honest answer for that exact spot.
- Mossdeep: `setweather WEATHER_SUNNY` (already its real baseline; unchanged
  in effect, just moved out of the shared script).

Builds clean with `make modern`. Not yet re-tested live — next mGBA pass
should confirm weather now visibly returns to normal per-route immediately
after solving each site.

**Resolved:** the "can't do the puzzle" report turned out to be about Birth
Island's existing vanilla Deoxys rock-triangle puzzle, not the new weather
markers — user was just flagging that it's notoriously fiddly (vanilla
Emerald content, teleporting rock + tight step budget), not asking for a
change. Decided to leave it exactly as vanilla built it. Separately
confirmed the weather markers' own trial-and-error puzzle design (no
in-game hint) is fine as-is — "they weren't too hard." Both open design
questions now closed, no further changes needed on either front.

**Next up:**
- Re-verify all 3 sites' weather correctly reverts after solving.

### Follow-up: the above fix wasn't actually sufficient

Re-tested per the "next up" above — still broken. Route 111 stayed on
sandstorm throughout (anomaly never visibly showed), Route 119's rain never
stopped (anomaly never visibly showed either). Mossdeep was the only one
that worked, both times.

**Real root cause:** Route 111 and Route 119 both have their own
*continuous*, per-tile weather systems that run independent of map
transitions — Route 111 has scripted coord-event tiles
(`Route111_EventScript_SandstormTrigger`/`SunTrigger`/`ViciousSandstormTrigger*`)
scattered through the desert, and Route 119 has declarative "weather" bg_events
(`COORD_EVENT_WEATHER_ROUTE119_CYCLE`/`_SUNNY`) handled generically by
`DoCoordEventWeather()` (`src/coord_event_weather.c`), fired from
`TryRunCoordEventScript` (`src/field_control_avatar.c:897-905`) every time the
player steps onto one of those tiles. These fire independent of and *after*
whatever `OnTransition` set, silently overwriting it the moment the player
takes a few steps toward the puzzle markers — explaining why the "before"
fix (which only changed what `OnTransition`/the tier-C script set once) never
actually became visible. Mossdeep worked because city maps don't have this
per-tile system at all.

**Real fix:** added `IsQuestWeatherCoordEventSuppressed()`
(`src/field_specials.c`) — returns true while `FLAG_QUEST_WEATHER_STARTED`
is set for the current map (Route 111 or Route 119), and hooked it into
`DoCoordEventWeather()` (`src/coord_event_weather.c`) as an early return.
This is the single, correct choke point — every per-tile weather trigger on
both routes funnels through this one function, so one check there
neutralizes all of them for the duration of the quest (both while the
anomaly is active and permanently after solving, matching the simpler
design below), without touching the dozens of individual trigger
definitions.

**Also redesigned the weather assignments** (user's call, simpler mental
model — a fixed anomaly/resolved pair per site, not vanilla's positional
variation):
- Route 111: anomaly `WEATHER_RAIN_THUNDERSTORM` (unchanged) → resolved
  `WEATHER_SANDSTORM` (now a flat `setweather` in the tier-C script, not the
  position-dependent `CheckSetSandstorm` call from the first fix attempt).
- Route 119: anomaly changed to `WEATHER_DROUGHT` ("overly sunny," was
  `WEATHER_SANDSTORM`) → resolved `WEATHER_RAIN` (flat constant, was
  `resetweather`/sunny fallback).
- Mossdeep: anomaly changed to `WEATHER_SANDSTORM` (was
  `WEATHER_RAIN_THUNDERSTORM`) → resolved `WEATHER_SUNNY` (unchanged, its
  real baseline).

Both `Route111_EventScript_CheckWeatherAnomaly` and
`Route119_EventScript_CheckWeatherAnomaly` (and Mossdeep's equivalent) now
explicitly set the resolved constant on every `OnTransition` too (not just
once, in the tier-C script) — so a fresh map re-entry after solving is just
as correct as the moment right after solving, no reliance on vanilla's
position-based logic lingering around.

Builds clean with `make modern`. Not yet re-tested live.

### Follow-up #2: Route 111 specifically still bugged (Route 119/Mossdeep untested at this point)

Re-tested: entering Route 111 after the Institute call showed sandstorm
again, not the intended thunderstorm anomaly — same symptom as before, on
this one route specifically.

**Real root cause (this time actually the last one):** `DoCoordEventWeather`
only handles *declarative* coord-events (map.json `"type": "weather"`
entries with no script attached — this is Route 119's mechanism). Route
111's `SandstormTrigger`/`SunTrigger` coord-events are the *other* kind —
regular scripted coord-events (`"script": "Route111_EventScript_
SandstormTrigger"`), which run via `RunScriptImmediately` directly
(`src/field_control_avatar.c:906-909`, the branch *before* the
`DoCoordEventWeather` call), bypassing my suppression check entirely. These
tiles are scattered throughout the desert (dozens of them, confirmed via
the map's coord_events list back in the original Route 111 scoping), so the
player crosses one within a few steps of entering and it silently
re-asserts sandstorm via its own direct `setweather`/`doweather` call — a
completely different code path than the one I patched.

**Fix:** guarded the two scripts directly — `Route111_EventScript_
SunTrigger`/`SandstormTrigger` (`data/maps/Route111/scripts.inc`) now check
`IsQuestWeatherCoordEventSuppressed` (already built, now also registered as
a script special in `data/specials.inc`) before touching weather; skips
straight to the music-fade/state-tracking side effects if suppressed, so
the desert/route BGM still correctly follows the player's physical position
even while the quest owns the visual weather. Checked Route 119 and
Mossdeep for the same "scripted coord-event bypass" pattern — clean, all
their `setweather` calls are already ones I control.

Builds clean with `make modern`. **Confirmed working live in mGBA** —
user-verified, this was the actual last piece.

**Aurora Ticket sidequest is complete.** Full chain verified end-to-end:
catch/defeat Rayquaza → Weather Institute call → all 3 anomaly sites show
correct anomaly weather and stay that way while active → puzzles solve and
each site's weather permanently reverts to its correct resolved state
(including on later re-entry, not just the instant of solving) → second
Institute call → Steven's Space Center handoff → Aurora Ticket →
Birth Island's untouched vanilla Deoxys content. ROADMAP.md updated to
close this out.

**Next up:**
- Old Sea Map, Mystic Ticket still tabled, no unlock hooks yet.
- Tilemap Studio pass on the summary screen (only remaining pending visual
  item — see "Pending Windows-side visual work" further down).
- Phase 2 (Match Call) — pick back up whenever: finalize gym leader
  tier-gating flags, confirm the `DEV_NOTES/DESIRED_MATCH_CALL_ROSTERS.md`
  trims are final, implement the flag-gating prototype on one gym leader.
- Jirachi: separate from-scratch effort, still needs its own design
  conversation before implementation.
- Johto/Heart & Soul thread: still needs the permissions ask before it's
  more than a research note.

---

## 2026-07-19 — Aurora Ticket marker placement (Porymap) + Johto/Heart & Soul research thread

**Porymap pass, Windows machine.** Finalized the 9 weather-anomaly marker
objects (Route111/Route119/MossdeepCity) and Steven's Space Center 2F spot
that were placed blind from `map.json` coordinates in the 2026-07-18 session.
All repositioned onto confirmed-walkable tiles in Porymap; also swapped the
markers' graphic from the placeholder `OBJ_EVENT_GFX_BREAKABLE_ROCK` to
`OBJ_EVENT_GFX_DEOXYS_TRIANGLE` (Birth Island's puzzle-stone sprite, already
in the project, unused anywhere else) — a better thematic fit for "mysterious
marker stone" than a smashable rock. Steven's `movement_type` changed to
`MOVEMENT_TYPE_FACE_UP` so he reads as looking out the window. Ran an
automated placement check before committing (bounds, per-tile collision bit,
metatile behavior, object-overlap) since a real mGBA visual pass wasn't
available this session — all 4 sites came back clean (Route 111's markers
specifically landed on `MB_DEEP_SAND`, confirming they're actually in the
sandy portion, not just nearby). Committed as `a450dfac5`. Still recommend a
real mGBA look when convenient — data-level checks confirm walkability, not
whether it *looks* right next to nearby scenery.

**Side-effect cleanup:** Porymap's project load/tab-open touched three
unrelated files with no real content changes — reverted before committing:
- `data/maps/map_groups.json` — had dropped the entire
  `connections_include_order` list (67 map names, controls the build's
  `connections.inc` include order per `tools/mapjson/mapjson.cpp:465`).
  Not fatal (code falls back to default order if empty) but an unwanted
  project-wide structural change.
- `src/data/region_map/region_map_sections.json` — had stripped
  `"name_clone": true` from 7 entries (e.g. `MAPSEC_ROUTE_4_POKECENTER`,
  `MAPSEC_KANTO_VICTORY_ROAD`). **Would have broken the build** — that flag
  is what makes the codegen template emit a `_Clone`-suffixed C symbol name
  when two map sections share a display name; without it, two sections
  sharing a name (e.g. "ROUTE 4") generate the same
  `static const u8 sMapName_ROUTE_4[]` symbol twice → duplicate-definition
  compile error.
- `src/data/wild_encounters.json` — harmless, just reformatted (one array
  value per line instead of one line per array) with identical values.

Lesson for future Porymap sessions: check `git status` for unexpected
diffs beyond the maps actually being edited before committing — Porymap's
own save routine doesn't fully round-trip some of this project's
hand-maintained JSON fields.

**New research thread: Pokémon Heart & Soul as a Johto candidate.** User
asked about importing gym leader sprites from other generations (Gen 2/Gen
4) — technically possible but real per-sprite art conversion work (GBA's
64×64 4bpp front-sprite / 16×16-per-frame overworld format doesn't accept
GBC or DS source art without a redraw pass), not a batch port. Then asked
about **Pokémon Heart & Soul** (pokecommunity.com thread, also
pokemonheartsoul.com, GitHub `PokemonHnS-Development/pokemonHnS`) — a
**completed, fully playtested Johto region hack built on the same
pokeemerald/Modern Emerald decomp base PG3 uses.** Confirmed via the repo's
file tree: standard pokeemerald layout, all 8 Johto gym leaders present
(`leader_falkner.png` through `leader_clair.png`), full E4 + Lance
(`elite_four_will/koga/bruno/karen.png`, `champion_lance.png`) using the
exact same naming convention PG3 already has for Hoenn, and real built-out
towns in `data/maps` (Azalea, Blackthorn, etc., each with Gym/Mart/
PokemonCenter/houses — not a skeleton). Being the same decomp base means
none of the conversion-work problem the Gen 2/4 sprite question ran into —
this would integrate the way Hoenn's own trainer data already does.

**This directly contradicts ROADMAP.md's north-star line** ("no other
region has portable Gen-3-native map/trainer assets" for the third region) —
HnS is a real counterexample. Not treating this as decided scope yet, just
flagging it as a live candidate; see ROADMAP.md north-star note.

**Licensing status, unresolved:** GitHub's license API returns 404 for the
HnS repo — no formal LICENSE file. Their README says "feel free to take
advantage of the open source," a genuine but informal invitation, and HnS
itself credits several other community contributors for some of its own
assets (so not everything in their repo is originally theirs to grant
permission for either). Recommended next step, not yet done: ask the HnS
team directly (they explicitly invite Discord contact) for an explicit
yes on reusing specific Johto assets in a separate hack, rather than
pulling anything based on the README alone.

**Current phase:** Phase 1 in progress — Aurora Ticket marker placement
done, pending final mGBA verification of the full quest chain (per
2026-07-18's next-up list, still open).

**Next up:**
- Push this session's commit, then on Ubuntu: `make modern`, verify the
  full Aurora Ticket chain end-to-end in mGBA (per 2026-07-18's list).
- Tilemap Studio pass on the summary screen INFO/SKILLS backgrounds
  (Windows, queued below in the Pending Windows-side visual work list) —
  taken up directly after this entry, see that section for the plan.
- If the Johto/HnS thread gets picked back up: reach out to the HnS team
  for explicit permission before importing anything; if granted, scope
  which assets (sprites only vs. maps too) and where this fits against the
  existing Phase 5 Kanto plan (parallel third-region effort, or does it
  reshape Phase 5 itself?).

**Known issues / open decisions:**
- Aurora Ticket marker/Steven placement now data-verified but not yet
  eyeballed in mGBA — carried over from 2026-07-18, still the one real risk
  item pending close-out.
- Johto-via-Heart&Soul is unresolved scope, not committed — needs the
  permissions ask before it's anything more than a research note.

---

## 2026-07-18 — Aurora Ticket via Rayquaza weather-anomaly sidequest

Same shape as the Eon Ticket fix: the Aurora Ticket (→ Birth Island →
Deoxys) was a Mystery-Gift-only dead end (`data/scripts/gift_aurora_ticket.inc`,
`MysteryGiftScript_AuroraTicket`, only reachable via `trywondercardscript`).
Birth Island's rock-triangle puzzle + Deoxys battle
(`data/maps/BirthIsland_Exterior/scripts.inc`) and the harbor plumbing were
already fully intact — only needed an in-game path to the ticket.

**Quest, per user's design:** catch/defeat Rayquaza (`FLAG_DEFEATED_RAYQUAZA`,
vanilla, covers both outcomes — matches how Sky Pillar already treats it as
one-shot) → walking around with Rayquaza in the party triggers a PokéNav call
from the Weather Institute (Route 119) reporting 3 simultaneous anomalies →
Route 111 (desert, made abnormal with rain/thunderstorm instead of its native
sandstorm), Route 119 (made abnormal with a sandstorm on a route that's
normally clear), Mossdeep City (storm over a normally sunny coastal city) →
each site has 3 marker objects that must be touched in the correct order
with Rayquaza in the party (wrong order silently resets, no penalty — modeled
on Mauville Gym's switch-press idiom + Trick House's scratch-var-reset
convention, using one `VAR_TEMP_8` per map as an auto-resetting progress
counter, not a persistent var) → solving all 3 arms a second Weather
Institute call → Steven at the Mossdeep Space Center relays a secondhand
report from an astronaut friend about a meteor near a southern island, hands
over the Aurora Ticket → `setflag FLAG_ENABLE_SHIP_BIRTH_ISLAND` unlocks the
harbor + Birth Island's untouched existing content, same as Southern Island
needed zero changes for Eon Ticket.

**New constants** — 8 flags in the same reserved `SYSTEM_FLAGS` range the
Lati quest started (`FLAG_QUEST_WEATHER_STARTED/ROUTE111_DONE/ROUTE119_DONE/
MOSSDEEP_DONE/ALL_DONE`, `FLAG_ENABLE_STEVEN_TICKET_CALL`, 3 marker-visibility
flags, Steven's 2nd-appearance visibility flag); 2 vars in the same reserved
tail range (`VAR_QUEST_WEATHER_CALL_STEP_COUNTER`/`VAR_QUEST_STEVEN_CALL_STEP_COUNTER`).
Puzzle progress deliberately uses `VAR_TEMP_8` (auto-resets per map load)
instead of a new persistent var, per the researched Trick House precedent.

**New C (`src/field_specials.c`)** — `IsRayquazaInParty()` (trivial species
check, no level/met requirement — "doesn't need training, just needs to be
carried," unlike the Lati quest's level-gated check), `ShouldDoWeatherInstituteCall()`
/ `ShouldDoStevenTicketCall()` (mirror `ShouldDoLatiBirchCall`, hooked into
`TryStartStepCountScript`). The first call's gate has no separate "enable"
flag to arm — unlike Birch's call, this one *is* the quest's entry point, so
it's gated directly on the standing story flag (`FLAG_DEFEATED_RAYQUAZA`)
plus its own "already started" guard.

**Scripts** — new shared file `data/scripts/quest_weather.inc` (institute
call, Steven call, shared "site resolved" payoff that checks whether all 3
are done and arms Steven's call, ticket handoff, shared marker-touch
reset/no-Rayquaza handlers reused by all 3 sites to avoid tripling
boilerplate). Each site (`Route111`, `Route119`, `MossdeepCity`) got: a
weather-override check appended to its existing `OnTransition` (layered
*after* each map's existing weather logic — e.g. Route 111 already has its
own desert sandstorm system, Route 119 has `special SetRoute119Weather`,
Mossdeep has the vanilla storm-trio effect — so ours takes precedence only
while the anomaly flag conditions hold, otherwise falls through to whatever
was already there), 3 new marker objects reusing `OBJ_EVENT_GFX_BREAKABLE_ROCK`
(no new art), and 3 short per-marker touch scripts.

Steven's ticket hand-off got its own new object at Space Center 2F rather
than reusing his existing one there — that one's already committed to the
Team Magma confrontation scene (`LOCALID_SPACE_CENTER_2F_STEVEN`,
`FLAG_HIDE_MOSSDEEP_CITY_SPACE_CENTER_2F_STEVEN`, a real vanilla flag whose
name I collided with on the first attempt and had to rename mine away from).
The new object's visibility additionally requires `VAR_STEVENS_HOUSE_STATE
>= 2` as a defensive guard, since the original Steven flag is never
re-hidden after the Magma battle — without that guard there's a narrow
window where both appearances could theoretically be visible at once.

**Known limitation, flagged rather than guessed past:** all new object
placements (9 markers + Steven) were positioned from `map.json` coordinate
data and existing coord_event locations, not Porymap — there's no way to
verify from data alone whether a given tile is actually walkable/unobstructed.
Route 111's markers were placed near known-walkable sandstorm coord_event
triggers for that reason (highest confidence); Route 119, Mossdeep, and
Steven's Space Center spot are reasoned best-guesses from open coordinate
gaps between existing NPCs. **This needs a visual pass in mGBA (or Porymap on
the Windows side) before considering placement final** — nudging a marker a
tile or two if it lands on a rock/wall/tree is a quick fix but needs eyes on
it.

Builds clean with `make modern`. Not yet verified live in mGBA.

Checked whether the user had Porymap available to do the placement pass
themselves right away — turns out not on hand currently. Documenting the
pending Windows-side visual work below so it's not lost; this is now the
canonical list of everything waiting on Porymap/Tilemap Studio across the
project, not just this session's item.

### Pending Windows-side visual work

**Porymap** (map/object placement — get it at
github.com/huderlem/porymap, the standard pret-decomp map editor; it's
listed as already installed on the Windows machine per the Phase 0 setup):
- Aurora Ticket weather-quest object placement (2026-07-18, this entry) — 9
  new marker objects + Steven's new spot were placed from `map.json`
  coordinates blind (no way to verify walkability from raw data). Open each
  map in Porymap's Events tab, find the objects by name, and drag onto
  confirmed-open ground:
  - `Route111`: `LOCALID_ROUTE111_WEATHER_MARKER_A/B/C` — keep in the desert
    (sandy) portion of the route, close together.
  - `Route119`: `LOCALID_ROUTE119_WEATHER_MARKER_A/B/C` — keep near the
    Weather Institute entrance.
  - `MossdeepCity`: `LOCALID_MOSSDEEP_WEATHER_MARKER_A/B/C` — southwest
    corner, away from the Magma/Maxie scene and other NPCs.
  - `MossdeepCity_SpaceCenter_2F`: `LOCALID_SPACE_CENTER_2F_QUEST_WEATHER_STEVEN`
    — avoid overlapping the Rich Boy/Gentleman/Scientist/original-Steven spots.
  - Save in Porymap (writes straight to `map.json`), then rebuild with
    `make modern` to confirm nothing broke.

**Tilemap Studio** (background tilemap/pixel art — get it at
github.com/Rangi42/tilemap-studio, used for GBA background tile/palette
editing):
- Summary screen INFO/SKILLS page background redesign (2026-07-15) — the
  code/logic side of the summary screen QoL pass is done and confirmed
  working in mGBA with placeholder layouts, but `page_info.bin` and
  `page_skills.bin` still need real background art to match the new
  3-section layouts. Specifically: the SKILLS page is missing a "STATS"
  section header (blocked by the panel header at rows 0-2 — needs to be
  baked into the tilemap background itself, not a window). After that
  redesign, also re-verify the IV/EV column pixel positions (x=94/136,
  chosen by calculation, never visually confirmed against real art).

**Current phase:** Phase 1 in progress — Aurora Ticket sidequest shipped, pending mGBA verification (including marker/NPC placement check).

**Next up:**
- Verify the full chain in mGBA: Rayquaza → institute call → all 3 sites →
  second call → Steven → ticket → Birth Island's existing puzzle/Deoxys
  encounter (should be completely unaffected).
- Do the Porymap placement pass above (Windows machine), sync back, rebuild.
- Workshop all the new dialogue (institute call, marker responses, Steven's
  call and ticket handoff) — first draft, same as every other quest so far.
- Jirachi: separate from-scratch effort, tabled per user's call — needs its
  own design conversation (trigger, location, mechanic) before implementation.
- Old Sea Map, Mystic Ticket still tabled, no unlock hooks yet.
- Tilemap Studio pass on the summary screen (see above) — waiting on
  Windows-side time, not blocked on anything else.

**Known issues / open decisions:**
- Marker/Steven placement needs visual verification (see above) — the one
  real risk item in this session's work.
- Route 111/119/Mossdeep's weather-anomaly override always re-applies on
  every map re-entry while active (matching the existing abnormal_weather.inc
  precedent's approach) rather than persisting a single weather state across
  visits — consistent with how vanilla's own weather systems on these maps
  already behave, not considered a bug.

### Phase 2 (Match Call) planning + Kanto import scoping

Started scoping Phase 2 (Match Call/rematch overhaul). Key research finding:
vanilla already has fully deterministic, escalating 5-tier rosters baked in
for gym leaders (e.g. `TRAINER_ROXANNE_1`-`_5`, level 12→32→37→42→47, real
species/moveset/item progression) — the only actual randomness in vanilla is
*when* a trainer becomes eligible for their next tier (`Random() % 100 <= 30`
in `src/gym_leader_rematch.c`/`src/battle_setup.c`), not which roster they
use. So "gate on progression flags instead of randomness" is a narrower,
more surgical change than initially scoped — replacing the RNG eligibility
roll with flag checks, using the existing `FLAG_DEFEATED_WALLY_VICTORY_ROAD`-
gated Wally rematch as a real precedent for "flag-gated, not random."

User designed a 7-tier wave-gating scheme (tier2=Flannery beaten unlocks
Roxanne/Brawly/Wattson, tier3=Juan beaten or Victory Road cleared, tier4=E4
beaten once, tier5=half Battle Frontier Symbols, tier6=all Symbols, tier7=
"last batch," more TBD) — tiers 1-5 map cleanly onto vanilla's existing 5
baked-in trainer tiers per gym leader (zero new trainer data needed); tiers
6-7 would need brand-new `TRAINER_*` IDs, which surfaced the real constraint:
**only 9 free trainer IDs exist out of 864 max** (`include/constants/opponents.h`).

Investigated reclaiming budget from the 64 non-gym-leader Match Call
trainers, who currently burn 320 IDs on 5 rematch tiers each (route
trainers/rivals — fisherman, triathletes, hikers, etc., full breakdown now
in `MATCH_CALL_ROSTERS.md`, auto-generated for the user to review and decide
keep/cut/scale per trainer). Trimming to 2 tiers each (1 regular + 1
rematch) frees exactly 192 IDs (precisely computed, not estimated).

This connected to a bigger conversation: user wants to eventually import
Kanto's gym leaders/Elite Four as battle-able figures (a "send-off" for Gen 3
spanning regions), tied to the project's existing north-star ("three Gen 3
regions" was already in ROADMAP.md before today). Checked real space
constraints to ground the discussion: ROM is a non-issue (~19.3 MB free of
32MB), but EWRAM (~12.1 KB free of 256KB, 95.3% used) and IWRAM (~2.3 KB free
of 32KB, 92.8% used) are both extremely tight — worth remembering for any
future feature needing significant new runtime RAM, though trainer/roster
data itself is ROM-resident and doesn't touch this budget. Kanto specifically
is feasible because FireRed/LeafGreen is a sibling Gen-3 pret decomp
(`pokefirered`) with genuinely portable official Gen-3-native maps, tilesets,
and trainer sprites — no other region (Johto included) has any Gen-3-style
assets to port, so those would need commissioned custom art from scratch.

Decision, now recorded in ROADMAP.md: reserve ~100 of the ~192 freed trainer
IDs for a future Phase 5 (Kanto import) rather than spending them all on
Hoenn's own gym leader tier 6/7 (which only needs ~16-24). North-star line
updated to name Kanto as the concrete second region, third region left
explicitly open pending art feasibility. Match Call implementation itself
remains tabled — no code changes this session, planning/documentation only.

**Next up (Phase 2, whenever resumed):**
- User reviews `MATCH_CALL_ROSTERS.md`, decides which of the 64 route
  trainers keep their full 5 tiers vs. get cut to 2 vs. something else
  (hybrid approach floated: retain more of the original game where it adds
  value, simplify where it doesn't).
- Finalize the exact tier-gating flags for gym leader tiers 2-5 (proposed:
  badge 4/Flannery, badge 8 or Victory Road, E4 cleared, half Frontier
  Symbols — user was mid-revision with a richer 7-tier scheme when this got
  tabled to discuss Kanto).
- Prototype the flag-gating change on one gym leader before scaling out, per
  the original Phase 2 framing.

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