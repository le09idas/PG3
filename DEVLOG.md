# PG3 Dev Log

Running journal of progress. Newest entry on top. At the end of each session add:
what changed, what's next, and any blockers. This is the canonical "where are we" —
read it first.

---

## 2026-06-20 — Phase 0 complete
- Installed the devkitARM toolchain on Ubuntu.
- Cloned pret/pokeemerald; renamed its remote to `upstream`.
- Created the private PG3 repo; set it as `origin`; pushed `master`.
- Built with `make modern` — compiles clean.
- ROM boots in mGBA; new game starts. ✅

**Current phase:** Phase 0 done → starting Phase 1.

**Next up:**
- Install Claude Code in the repo on Ubuntu; confirm it reads CLAUDE.md.
- Clone PG3 on the Windows machine and install Porymap.
- Begin Phase 1 with one low-risk task to learn the edit → build → test loop.

**Known issues / open decisions:** none yet.

---

### Entry template (copy for each session)
## YYYY-MM-DD — <short title>
- <what changed>

**Next up:** <tasks>
**Blockers:** <none / details>