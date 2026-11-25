# Gen 2: shiny GS Ball Celebi (Pokémon Crystal, 3DS Virtual Console)

The engine behind PokeFinderPlus's Gen 2 tool: it plans the presses that give a shiny Celebi at the Ilex Forest shrine
in the 3DS Virtual Console release of Pokémon Crystal (FR, DE, IT, ES, EN). It also infers the hidden values from the
player's attempts and says when to switch plans, move the timer, re-save or recalibrate. The window is `Form/Gen2/Celebi2`.

## What is here

| File | What it does |
|---|---|
| `VcCore` | A Game Boy Color core whose timing (instructions, DIV, timer, PPU, HDMA) is a transcription of the VC emulator's run loop. Each core holds its whole state in `VcCore::State`, so cores run in parallel threads and a snapshot is a plain copy (~82 KB). The ROM is shared, read-only. |
| `CrystalSave` | Crystal save fields (both copies, both checksums) and the values that change Celebi (`readMinimal` / `applyMinimal`). |
| `CelebiPlan` | Joypad timelines keyed by VBlank count, plan discovery (6 prompts, YES, final A from power-on) and the PokeTimerPlus phases. |
| `CelebiSearch` | Leaf grids (press errors on 6th / YES / final, timer offset X), the F-scored shiny search, plan 1 / plan 2, hidden play-time coverage, clock windows, and the threaded runners. |
| `CelebiCalibration` | Calibration from attempts (timer offset X, Kenji class, clock carries; accept rule) and the reset band (rule A / B). |
| `CelebiSwitchRule` | Hunt advice: joint posterior over timer residual × play-time frame counter F (info-box press in the model). |
| `CelebiSession` | Session state (the prototype's JSON) and the re-save step. |
| `CelebiTool` | The flow the Gen 2 window runs, as the prototype's CLI chains it: the shipped save template, game-file loading, calibration with cached grids (resumable, cancellable), the plans for the calibrated class, the switch-rule grids and the hunt advice, session JSON helpers, grid files. |
| `VcInputs` | The player's files: a .gbc ROM alone (recommended), recognised by its SHA-1 (`KNOWN_ROMS`: FR, DE, IT, ES, EN Rev 0, EN Rev A), or a decrypted CIA (the ROM from its romfs `rom/<code>` + the emulator check). An encrypted CIA is refused. No patch file is read: the VC patch never changes the encounter (test 4; Victor 2026-11-25). |

## Where the facts come from

- The core and every rule were found and checked in the crystal-vc-celebi research (Nov 2026). It covers the VC
  `.code` run loop, plan discovery, the hidden save values, calibration, and the switch rule, with harness runs in
  Nintendo's emulator and Victor's retail attempts. The research prototype is `vccore.c` + `celebi_tool.py` v0.6;
  this C++ is a port of it.
- The port is checked against the prototype's own outputs, not against the research notes: `Test/Gen2` compares enemy
  bytes, full state hashes, plans, grids, calibration verdicts, posteriors and advice with goldens made from the prototype
  (`Test/Gen2/tools/make_goldens.py`).
- The same `.code` and `config.ini` ship in the FR, EN, IT, DE and ES titles (hashes in `VcInputs.hpp`), so one core
  serves the five languages. JP is out (different save layout).

## No game data

No ROM, ROM bytes, `.code`, patch file or save is in this folder (only the ROMs' SHA-1), in `Test/Gen2`, or in CI. The player's files are read
at run time, locally. Golden data holds outputs only: DVs, hashes, frame numbers, verdicts, posteriors.

## The save template

`Core/Resources/Gen2/crystal_template.sav` is Victor's shrine save (FR) with every name and the trainer ID blanked
(`make_template.py` says what is changed). The tool writes the player's visible values and the hypotheses (Kenji class,
clock offset) into it; F and the play-time seconds stay the template's. Gate (2026-11-25, prototype core): the template
gives the same Celebi as the original save on 500/500 random timelines and minimal-value sets across the five languages,
and the tool's plan discovery on it equals the prototype's plans for all five (`CelebiToolTest::plansFromTemplate`).

## Tests

`./build-mac.sh --test` runs the tests that need no game files (save format, calibration, band, switch rule, session,
ROM hash table). The tests that need game files read these environment variables and are skipped (and counted) when they
are missing:

| Variable | Contents |
|---|---|
| `POKEFINDERPLUS_GBC_ROMS` | folders (`:`, `;` on Windows) with the VC ROMs the goldens were made with (`<lang>_vc.gbc`: dump + the title's patch) and the cartridge dumps (FR, DE, IT, ES, EN Rev 0, EN Rev A) for the check-mark and ROM-alone tests; files are recognised by hash, names don't matter |
| `POKEFINDERPLUS_GEN2_SAVES` | folder with the saves the goldens were made with (Victor's save, the EN test save) |
| `POKEFINDERPLUS_GEN2_CIAS` | folders with decrypted and encrypted CIAs, separated by `:` (`;` on Windows) |

With all three set, the Gen 2 tests take ~8 minutes on an M2. `POKEFINDERPLUS_GEN2_SLOW=1` adds
`CelebiToolTest::calibrationEndToEnd` (~17 min; also set `QTEST_FUNCTION_TIMEOUT=3600000`, Qt Test stops a function after 5 min by default): Victor's retail calibration 1 through the tool's own calibration path.
`POKEFINDERPLUS_TEST_ONLY=<class>[,<class>]` runs only those test classes.
