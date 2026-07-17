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
    - Old Sea Map, Aurora, Mystic tickets still need their own unlock hooks.
  - [Map] restore the islands (Southern Island, Navel Rock, Birth Island,
    Faraway Island) and their encounters — existing map data, currently
    inaccessible without the event scripts above. Southern Island now reachable
    via the Eon Ticket quest above; Navel Rock/Birth Island/Faraway Island
    still pending their own tickets.
- **Trade-evolution alternatives:**
  - [Programmatic] in-game methods (e.g. Machoke + a special item) so the dex is
    completable solo. Interop-safe — evolution method is local logic.
- **Encounter overhaul:**
  - [Programmatic] rewrite wild tables map-wide — fauna consistency (e.g. common
    Zigzagoon + rare Linoone), surprise high-level encounters, soft level-gating;
    shiny-rate changes / flag-linked shiny boosts. Keep player-obtainable Pokémon
    legal.
- **Visual (pending, Windows/Tilemap Studio):**
  - [Visual] summary screen INFO/SKILLS background redesign to match the new
    3-section layouts (see DEVLOG 2026-07-15).

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