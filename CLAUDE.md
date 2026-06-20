# PG3 — Pokémon Gen 3 (Project Context)

This file is read by Claude Code at the start of every session. Keep it current.

## What PG3 is
An enhancement hack of Pokémon Emerald built on the **vanilla pret/pokeemerald**
decompilation. The goal is a more complete, more replayable Emerald that stays
**100% within Gen 3** and **preserves full compatibility with real Gen 3 hardware**:
trading, Pokémon Box, FR/LG, and Pal Park migration to Gen 4.

## Hard rules — never break these (interop must survive)
1. **Do not change the Pokémon data structures, the species set, or the ROM's
   internal identity** (title / game code — Emerald is `BPEE`). These are what let
   the cart trade and Pal Park with retail games.
2. **Stay within the 386 National Dex species using vanilla data layouts.** No new
   species, no expanded species/box limits.
3. **No cross-gen content, no Fairy type, no later-gen moves/abilities.** PG3 is
   deliberately Gen-3-authentic.
4. **Player-obtainable Pokémon stay roughly legal** (vanilla learnsets/abilities) so
   they migrate cleanly. **Trainer rosters are exempt** — trainer Pokémon are never
   traded, so Match Call teams can be anything.
5. Purely local battle-mechanic changes (e.g. the physical/special split) are fine —
   they affect link *battles* with retail carts but never trading or Pal Park.

## Build & run
- Toolchain: devkitARM (devkitPro).
- Build: `make modern -j$(nproc)` → produces `pokeemerald.gba`.
- Test: `mgba-qt pokeemerald.gba`.
- The modern build is not byte-identical to retail, but interop is preserved because
  link/trade/Pal Park formats are source-defined, not compiler-defined.

## Repo & remotes
- `origin`   → this private PG3 repo.
- `upstream` → https://github.com/pret/pokeemerald (pull bugfixes/docs; never push).
- Default branch: `master`.

## Workflow (two machines)
- **Ubuntu** — all code, builds, and debugging (Claude Code lives here).
- **Windows** — Porymap and visual/asset work.
- Everything flows through git. **Pull before you work, push after.** Never edit the
  same files on both machines without syncing first.

## Conventions
- Small, descriptively-messaged commits — `git log --oneline` should read like a changelog.
- Update `DEVLOG.md` at the end of every working session (what changed, what's next, blockers).
- Events/dialogue: prefer Poryscript. Maps: Porymap.

## Where to look
- `ROADMAP.md` — the phased plan and feature list.
- `DEVLOG.md`  — current state, last actions, next tasks, known issues. Read this first.