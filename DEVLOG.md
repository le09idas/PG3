# PG3 Dev Log

Running journal of progress. Newest entry on top. At the end of each session add:
what changed, what's next, and any blockers. This is the canonical "where are we" —
read it first.

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