# TODO — Next Session

## Porymap (Aurora Ticket weather-quest placement)

All 9 objects already exist in each map's `map.json` with scripts/flags fully
wired up — this is a "drag to a confirmed-walkable tile and save" pass, not
new object creation. See DEVLOG.md 2026-07-18 for full context.

- [ ] `Route111`: `LOCALID_ROUTE111_WEATHER_MARKER_A/B/C` — keep in the
      desert (sandy) portion of the route, close together.
- [ ] `Route119`: `LOCALID_ROUTE119_WEATHER_MARKER_A/B/C` — keep near the
      Weather Institute entrance.
- [ ] `MossdeepCity`: `LOCALID_MOSSDEEP_WEATHER_MARKER_A/B/C` — southwest
      corner, clear of the Magma/Maxie scene and other NPCs.
- [ ] `MossdeepCity_SpaceCenter_2F`: `LOCALID_SPACE_CENTER_2F_QUEST_WEATHER_STEVEN`
      — clear of the Rich Boy/Gentleman/Scientist/original-Steven spots.
- [ ] Save in Porymap, then `make modern` to confirm nothing broke.

## Tilemap Studio (summary screen background redesign)

See DEVLOG.md 2026-07-15 for full context.

- [ ] `page_info.bin` / `page_skills.bin` — redesign to match the new
      3-section INFO/SKILLS layout.
- [ ] Bake a "STATS" header into the SKILLS page background art (currently
      blocked by the panel header at rows 0-2 — can't be a window, has to be
      part of the tilemap itself).
- [ ] After the redesign, re-verify the IV/EV column pixel positions
      (x=94/136, chosen by calculation, never visually confirmed against
      real art).

## Once synced back — roadmap candidates

- [ ] **Phase 2 (Match Call)** — resume where tabled: finalize gym leader
      tier-gating flags (proposed: badge 4/Flannery, badge 8 or Victory
      Road, E4 cleared, half Frontier Symbols — see DEVLOG for the fuller
      7-tier scheme discussion), confirm the DEV_NOTES roster trims
      (`DESIRED_MATCH_CALL_ROSTERS.md`) are final, then implement the
      flag-gating prototype on one gym leader before scaling out.
- [ ] Old Sea Map / Mystic Ticket — same shape as Eon/Aurora, still untouched.
- [ ] Trade-evolution alternatives — in-game item/method so trade-only
      Pokémon are completable solo.
- [ ] Encounter overhaul — wild table rewrite for fauna consistency,
      level-gating, shiny-rate tweaks.
