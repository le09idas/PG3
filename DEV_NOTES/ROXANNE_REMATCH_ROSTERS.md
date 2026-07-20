# Roxanne Rematch Rosters (reference)

Source: `src/data/trainer_parties.h` (`sParty_Roxanne1`-`sParty_Roxanne5`),
`src/data/trainers.h` (`TRAINER_ROXANNE_1`-`_5`). Gating logic:
`GetRoxanneAllowedRematchTier()` in `src/field_specials.c`. Format is
single battle for tiers 2-5 (double battle removed 2026-07-19).

User's note (2026-07-19): "not loving the lineups... but that can be
edited later" — this file exists so the actual rosters are easy to review
without digging through `trainer_parties.h` each time.

## Tier 1 — `TRAINER_ROXANNE_1` (first badge fight)
| Species | Level | Item | Moves |
|---|---|---|---|
| Geodude | 12 | — | Tackle, Defense Curl, Rock Throw, Rock Tomb |
| Geodude | 12 | — | Tackle, Defense Curl, Rock Throw, Rock Tomb |
| Nosepass | 15 | Oran Berry | Block, Harden, Tackle, Rock Tomb |

## Tier 2 — `TRAINER_ROXANNE_2` (unlocked: Flannery beaten)
| Species | Level | Item | Moves |
|---|---|---|---|
| Golem | 32 | — | Protect, Rollout, Magnitude, Explosion |
| Kabuto | 35 | Sitrus Berry | Swords Dance, Ice Beam, Surf, Rock Slide |
| Onix | 35 | — | Iron Tail, Explosion, Roar, Rock Slide |
| Nosepass | 37 | Sitrus Berry | Double Team, Explosion, Protect, Rock Slide |

## Tier 3 — `TRAINER_ROXANNE_3` (unlocked: Victory Road Wally beaten)
| Species | Level | Item | Moves |
|---|---|---|---|
| Omanyte | 37 | — | Protect, Ice Beam, Rock Slide, Surf |
| Golem | 37 | — | Protect, Rollout, Magnitude, Explosion |
| Kabutops | 40 | Sitrus Berry | Swords Dance, Ice Beam, Surf, Rock Slide |
| Onix | 40 | — | Iron Tail, Explosion, Roar, Rock Slide |
| Nosepass | 42 | Sitrus Berry | Double Team, Explosion, Protect, Rock Slide |

## Tier 4 — `TRAINER_ROXANNE_4` (unlocked: Elite Four beaten)
| Species | Level | Item | Moves |
|---|---|---|---|
| Omastar | 42 | — | Protect, Ice Beam, Rock Slide, Surf |
| Golem | 42 | — | Protect, Rollout, Earthquake, Explosion |
| Kabutops | 45 | Sitrus Berry | Swords Dance, Ice Beam, Surf, Rock Slide |
| Onix | 45 | — | Iron Tail, Explosion, Roar, Rock Slide |
| Nosepass | 47 | Sitrus Berry | Double Team, Explosion, Protect, Rock Slide |

## Tier 5 — `TRAINER_ROXANNE_5` (unlocked: 7+ total Frontier Symbols)
| Species | Level | Item | Moves |
|---|---|---|---|
| Aerodactyl | 47 | — | Rock Slide, Hyper Beam, Supersonic, Protect |
| Golem | 47 | — | Focus Punch, Rollout, Earthquake, Explosion |
| Omastar | 47 | — | Protect, Ice Beam, Rock Slide, Surf |
| Kabutops | 50 | Sitrus Berry | Swords Dance, Ice Beam, Surf, Rock Slide |
| Steelix | 50 | — | Iron Tail, Explosion, Roar, Rock Slide |
| Nosepass | 52 | Sitrus Berry | Double Team, Explosion, Protect, Rock Slide |

## Tier 6 (prototype, not a real roster)
Reuses tier 5's exact roster/levels above, **+10 levels applied at battle
setup** via `gTrainerPartyLevelBonus` (`src/battle_main.c`), gated (as of
2026-07-19) behind all 7 Battle Frontier Gold Symbols — currently
**temporarily also gated behind `FLAG_SYS_GAME_CLEAR` for testing** (see
`src/field_specials.c`, marked `TODO PG3 TEMP`), which needs to be removed
once the mechanism is confirmed working. No new `TRAINER_*` data — this is
the proof-of-concept for "uncapped" rematch tiers via level-scaling instead
of new rosters, requested 2026-07-19 (see DEVLOG). Verified 2026-07-19:
Aerodactyl showed at level 57 in-battle = 47 (tier 5 base) + 10 (bonus),
confirming the mechanism works as designed.
