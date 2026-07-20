# PG3 Roadmap

**Base:** vanilla pret/pokeemerald · **Scope:** Gen 3 only, full real-cart interop preserved.
**North star:** a game spanning three Gen 3 regions — Hoenn, then Kanto (via
portable `pokefirered` assets, see Phase 5), then Johto (see Phase 6,
contingent on Pokémon Heart & Soul permissions — see DEVLOG 2026-07-19).
**Near-term:** a complete, replayable Hoenn first.

Each item below is tagged by change type: **[One-off]** single value/file edits,
**[Programmatic]** rematches/events/special encounters/new logic,
**[Visual]** graphical/UI work, **[Map]** new or newly-accessible areas.

## Phase 0 — Working build ✅
Toolchain, repo + remotes, `make modern`, boots in mGBA.

## Phase 1 — Low-risk wins (learn the edit → build → test loop)
Roughly this order, interleaving data work and QoL:
- **QoL:**
  - [Programmatic] summary-screen info (category icons, egg group, IVs/nature,
    ability) — code/logic side done (2026-07-15 pass); see Visual below for the
    matching background redesign.
  - [One-off] comfort features (B-to-run indoors and out — done ✅; reusable
    TMs — done ✅).
- **In-game availability / cut content:**
  - [Programmatic] repopulate Altering Cave — done (random-per-visit roll). ✅
  - [Programmatic] make the Hoenn-native event tickets obtainable in-game
    (Aurora → Deoxys, Eon → Lati@s). Old Sea Map (→ Mew) and Mystic Ticket
    (→ Ho-Oh/Lugia) intentionally reassigned below — see Phase 5/Phase 6.
    - Eon Ticket: done via the Lati awakening/reunion sidequest (2026-07-17) ✅
      — train the roaming Latios/Latias 5 levels above catch level, it "awakens"
      and flies off, Birch calls, hands over the ticket; reunite with it on
      Southern Island before the vanilla opposite-species encounter. Verified
      end-to-end in mGBA.
    - Aurora Ticket: done via the Rayquaza weather-anomaly sidequest
      (2026-07-18/19) ✅ — catch/defeat Rayquaza, get a Weather Institute call
      about 3 anomalies (Route 111, Route 119, Mossdeep City), calm each with
      a touch-3-markers-in-order puzzle (Rayquaza in party, no training
      needed), Weather Institute calls again once all 3 are done, Steven at
      the Space Center relays an astronaut's meteor sighting and hands over
      the ticket. Verified end-to-end in mGBA.
  - [Map] restore Southern Island and Birth Island — done, reachable via the
    Eon/Aurora Ticket quests above. Navel Rock (Ho-Oh/Lugia) and Faraway
    Island (Mew) reassigned to Phase 6/Phase 5 respectively, alongside their
    tickets — see below.
  - Jirachi: no existing infrastructure at all in vanilla (no flags, map
    presence, encounter script, or overworld graphic) — tabled as a separate,
    later from-scratch effort per user's call (2026-07-18).
- **Encounter overhaul:**
  - [Programmatic] rewrite wild tables map-wide — fauna consistency (e.g. common
    Zigzagoon + rare Linoone), surprise high-level encounters, soft level-gating;
    shiny-rate changes / flag-linked shiny boosts. Keep player-obtainable Pokémon
    legal.
- **Visual (pending, Windows/Porymap + Tilemap Studio):**
  - [Visual] summary screen INFO/SKILLS background redesign to match the new
    3-section layouts (see DEVLOG 2026-07-15). Tool: **Tilemap Studio**.
  - [Visual] Aurora Ticket weather-quest object placement — done ✅
    (2026-07-19, Porymap pass + follow-on weather-system bug fixes, see
    DEVLOG). All 9 markers + Steven repositioned and verified working
    end-to-end in mGBA.

## Phase 2 — Match Call / rematch overhaul (the centerpiece) — [Programmatic]
Distinct, escalating rosters for trainers, rivals, gym leaders, and the Elite Four,
gated on **progression flags** (got Fly, beat E4, etc.) instead of vanilla randomness.
Prototype ONE gym leader end-to-end before scaling out. Heaviest C work; trainer
rosters are interop-exempt, so teams can be anything.

**Roxanne prototype implemented (2026-07-19)** — `GetRoxanneAllowedRematchTier()`
(`src/field_specials.c`) deterministically gates her 5 existing vanilla
tiers on badge/E4/Symbol milestones, fully independent of the shared
vanilla RNG rematch system (so the other 7 leaders + E4 are untouched).
Live testing surfaced a real bug: vanilla's `trainerbattle_rematch_double`
still separately checks its own RNG-driven `IsTrainerReadyForRematch`
bookkeeping before starting a battle, so the tier gate alone wasn't enough —
fixed by force-syncing that bookkeeping via `UpdateRematchIfDefeated()`, see
DEVLOG. Builds clean; **fix not yet re-verified live in mGBA**. Once
verified: decide gating flags for the other 7 leaders (each will need the
same `UpdateRematchIfDefeated()` call) and scale out.

**Trainer-ID budget note (2026-07-19):** only 9 free `TRAINER_*` IDs exist
project-wide out of 864 max (see `include/constants/opponents.h`). The 64
non-gym-leader Match Call trainers currently burn 320 IDs on 5 rematch tiers
each (full roster reference: `MATCH_CALL_ROSTERS.md`); trimming them to 2
tiers (1 regular + 1 rematch) frees ~192 IDs. When this rework happens,
**reserve ~100 of those freed IDs for Phase 5 (Kanto import)** rather than
spending them all on Hoenn's own gym leader tier 6/7 — Hoenn's expansion only
needs ~16-24 IDs, leaving ample headroom for both. Decide the actual 2-tier
roster per trainer (keep vanilla tier 5, or something else) before implementing.

## Phase 3 — Plot hooks — [Programmatic]
Scripted scenarios that hand the player the tickets / unlock the cut content
(optionally gated behind battles). Mostly Poryscript. Depends on Phase 1 content.

## Phase 4 — Pokédex research depth (capstone) — [Programmatic]
Modern-style catch/battle-X tasks with rewards. Custom system.

## Phase 5 — Kanto import (future, post-Hoenn) — [Programmatic] + [Visual/Map]
Second of the "three Gen 3 regions." Feasible specifically because FireRed/
LeafGreen is a sibling Gen-3 decomp (`pokefirered`) with genuinely portable
official assets — Kanto's map layouts, tilesets, gym designs, trainer
sprites, and rosters already exist as Gen-3-native content, unlike any
later-gen region (no official Gen-3-style art exists for Johto or beyond;
those would need commissioned custom art from scratch, a separate effort).
Scope: import Kanto's 8 gym leaders + Elite Four + Champion as battle-able
figures (trainer data/sprites ported from `pokefirered`); explorable
scaled-down map content is a stretch goal contingent on how cleanly Kanto's
maps port over. Draws on the ~100 trainer IDs reserved from the Phase 2
rework (see above). Not started — needs its own dedicated planning session.

**Old Sea Map → Mew (Faraway Island) folded in here (2026-07-19):** Mew's
Kanto-era association makes it a natural companion to this phase rather than
a standalone Phase 1 item — same "in-game unlock hook" shape as Eon/Aurora
Ticket, just implemented alongside the Kanto work instead of before it.

## Phase 6 — Johto import (future, pending permissions) — [Programmatic] + [Visual/Map]
Third of the "three Gen 3 regions" — the candidate slot ROADMAP previously
left open. Contingent entirely on **Pokémon Heart & Soul** (see north-star
note and DEVLOG 2026-07-19) actually panning out: a completed Johto hack on
the same pokeemerald decomp base, with Gen-3-native gym leader/E4/champion
sprites and built-out towns already done. Blocked on explicit permission
from the HnS team — not committed scope until that's resolved.

**Mystic Ticket → Ho-Oh/Lugia (Navel Rock) folded in here (2026-07-19):**
both are Gen 2/Johto's mascot legendaries, making this a natural companion
to a Johto phase rather than a standalone Phase 1 item — same reasoning as
Old Sea Map/Mew above.

## Optional / parked
- **Retranslation pass** — high effort, uncertain payoff; revisit only with a
  specific goal in mind.

## Rejected (2026-07-19)
- **Physical/Special split** — deliberately keeping Gen 3's authentic
  type-based split (not the Gen 4+ per-move split). Changing it would lose
  part of what makes this a *Gen 3* game — the type-based split is real
  Gen 3 challenge/design, not a bug to fix.
- **Trade-evolution alternatives** — decided against in-game workarounds for
  trade-only evolutions (Machoke, Kadabra, Haunter, etc.). Real trading
  between this hack and other ROM hacks (including this one) is technically
  achievable, so the canonical trade requirement stays respected rather than
  bypassed.

## Permanent constraints (see CLAUDE.md)
386 species · vanilla data format · vanilla game identity. No cross-gen, no Fairy,
no expanded limits. Player Pokémon stay migration-legal; trainer teams unconstrained.