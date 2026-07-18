# PG3 Roadmap

**Base:** vanilla pret/pokeemerald · **Scope:** Gen 3 only, full real-cart interop preserved.
**North star:** a game spanning three Gen 3 regions. **Near-term:** a complete, replayable Hoenn first.

Each item below is tagged by change type: **[One-off]** single value/file edits,
**[Programmatic]** rematches/events/special encounters/new logic,
**[Visual]** graphical/UI work, **[Map]** new or newly-accessible areas.

## Phase 0 — Working build ✅
Toolchain, repo + remotes, `make modern`, boots in mGBA.

## Phase 1 — Low-risk wins (learn the edit → build → test loop)
Roughly this order, interleaving data work and QoL:
- **QoL:**
  - [Programmatic] physical/special split (local, interop-safe).
  - [Programmatic] summary-screen info (category icons, egg group, IVs/nature,
    ability) — code/logic side done (2026-07-15 pass); see Visual below for the
    matching background redesign.
  - [One-off] comfort features (B-to-run indoors and out — done ✅; reusable
    TMs — done ✅).
- **In-game availability / cut content:**
  - [Programmatic] repopulate Altering Cave — done (random-per-visit roll). ✅
  - [Programmatic] make the event tickets obtainable in-game (Old Sea Map → Mew,
    Aurora → Deoxys, Mystic → Ho-Oh/Lugia, Eon → Lati@s).
    - Eon Ticket: done via the Lati awakening/reunion sidequest (2026-07-17) ✅
      — train the roaming Latios/Latias 5 levels above catch level, it "awakens"
      and flies off, Birch calls, hands over the ticket; reunite with it on
      Southern Island before the vanilla opposite-species encounter. Verified
      end-to-end in mGBA.
    - Aurora Ticket: done via the Rayquaza weather-anomaly sidequest
      (2026-07-18) ✅ — catch/defeat Rayquaza, get a Weather Institute call
      about 3 anomalies (Route 111, Route 119, Mossdeep City), calm each with
      a touch-3-markers-in-order puzzle (Rayquaza in party, no training
      needed), Weather Institute calls again once all 3 are done, Steven at
      the Space Center relays an astronaut's meteor sighting and hands over
      the ticket. Builds clean; pending mGBA verification (and a visual
      passability check on the new marker/NPC placements — placed from map
      data, not Porymap).
    - Old Sea Map, Mystic tickets still tabled, no unlock hooks yet.
  - [Map] restore the islands (Southern Island, Navel Rock, Birth Island,
    Faraway Island) and their encounters — existing map data, currently
    inaccessible without the event scripts above. Southern Island and Birth
    Island now reachable via the Eon/Aurora Ticket quests above; Navel
    Rock/Faraway Island still pending their own tickets.
  - Jirachi: no existing infrastructure at all in vanilla (no flags, map
    presence, encounter script, or overworld graphic) — tabled as a separate,
    later from-scratch effort per user's call (2026-07-18).
- **Trade-evolution alternatives:**
  - [Programmatic] in-game methods (e.g. Machoke + a special item) so the dex is
    completable solo. Interop-safe — evolution method is local logic.
- **Encounter overhaul:**
  - [Programmatic] rewrite wild tables map-wide — fauna consistency (e.g. common
    Zigzagoon + rare Linoone), surprise high-level encounters, soft level-gating;
    shiny-rate changes / flag-linked shiny boosts. Keep player-obtainable Pokémon
    legal.
- **Visual (pending, Windows/Porymap + Tilemap Studio):**
  - [Visual] summary screen INFO/SKILLS background redesign to match the new
    3-section layouts (see DEVLOG 2026-07-15). Tool: **Tilemap Studio**.
  - [Visual] Aurora Ticket weather-quest object placement — verify/reposition
    9 new marker objects (Route111/Route119/MossdeepCity) and Steven's new
    Space Center 2F spot; all were placed from map.json coordinates blind,
    not visually confirmed walkable (see DEVLOG 2026-07-18). Tool: **Porymap**.

## Phase 2 — Match Call / rematch overhaul (the centerpiece) — [Programmatic]
Distinct, escalating rosters for trainers, rivals, gym leaders, and the Elite Four,
gated on **progression flags** (got Fly, beat E4, etc.) instead of vanilla randomness.
Prototype ONE gym leader end-to-end before scaling out. Heaviest C work; trainer
rosters are interop-exempt, so teams can be anything.

## Phase 3 — Plot hooks — [Programmatic]
Scripted scenarios that hand the player the tickets / unlock the cut content
(optionally gated behind battles). Mostly Poryscript. Depends on Phase 1 content.

## Phase 4 — Pokédex research depth (capstone) — [Programmatic]
Modern-style catch/battle-X tasks with rewards. Custom system.

## Optional / parked
- **Retranslation pass** — high effort, uncertain payoff; revisit only with a
  specific goal in mind.

## Permanent constraints (see CLAUDE.md)
386 species · vanilla data format · vanilla game identity. No cross-gen, no Fairy,
no expanded limits. Player Pokémon stay migration-legal; trainer teams unconstrained.