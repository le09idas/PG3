# PG3 Roadmap

**Base:** vanilla pret/pokeemerald · **Scope:** Gen 3 only, full real-cart interop preserved.
**North star:** a game spanning three Gen 3 regions. **Near-term:** a complete, replayable Hoenn first.

## Phase 0 — Working build ✅
Toolchain, repo + remotes, `make modern`, boots in mGBA.

## Phase 1 — Low-risk wins (learn the edit → build → test loop)
Roughly this order, interleaving data work and QoL:
- **QoL:** physical/special split (local, interop-safe); summary-screen info
  (category icons, egg group, IVs/nature); comfort features (B-to-run, faster text,
  reusable TMs).
- **In-game availability / cut content:** repopulate Altering Cave; make the event
  tickets obtainable in-game (Old Sea Map → Mew, Aurora → Deoxys, Mystic →
  Ho-Oh/Lugia, Eon → Lati@s) and restore the islands/encounters.
- **Trade-evolution alternatives:** in-game methods (e.g. Machoke + a special item)
  so the dex is completable solo. Interop-safe — evolution method is local logic.
- **Encounter overhaul:** rewrite wild tables map-wide — fauna consistency
  (e.g. common Zigzagoon + rare Linoone), surprise high-level encounters, soft
  level-gating; shiny-rate changes / flag-linked shiny boosts. Keep player-obtainable
  Pokémon legal.

## Phase 2 — Match Call / rematch overhaul (the centerpiece)
Distinct, escalating rosters for trainers, rivals, gym leaders, and the Elite Four,
gated on **progression flags** (got Fly, beat E4, etc.) instead of vanilla randomness.
Prototype ONE gym leader end-to-end before scaling out. Heaviest C work; trainer
rosters are interop-exempt, so teams can be anything.

## Phase 3 — Plot hooks
Scripted scenarios that hand the player the tickets / unlock the cut content
(optionally gated behind battles). Mostly Poryscript. Depends on Phase 1 content.

## Phase 4 — Pokédex research depth (capstone)
Modern-style catch/battle-X tasks with rewards. Custom system.

## Optional / parked
- **Retranslation pass** — high effort, uncertain payoff; revisit only with a
  specific goal in mind.

## Permanent constraints (see CLAUDE.md)
386 species · vanilla data format · vanilla game identity. No cross-gen, no Fairy,
no expanded limits. Player Pokémon stay migration-legal; trainer teams unconstrained.