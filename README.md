<p align="center"><img src="Form/Images/Icon/pokefinder_256x256.png" width="128" alt="PokeFinderPlus logo"></p>

# PokéFinderPlus

**Unofficial fork of [PokéFinder](https://github.com/Admiral-Fish/PokeFinder)** by Admiral-Fish and contributors, maintained by Vict0rious7.
It is not supported by the PokéFinder team: **report bugs in this fork to [Vict0rious7/PokeFinderPlus issues](https://github.com/Vict0rious7/PokeFinderPlus/issues)**, not to PokéFinder or its Discord.

Licensed under GPL-3.0, like PokéFinder (see `LICENSE`). Modified files say so in their header.

## What's different from PokéFinder

**Gen 3**
- **FRLG initial seeds in the Static and Wild searchers.** For FireRed/LeafGreen profiles, each result shows the best reachable initial seed, its seed time (ms), advances, total time and settings, with Ten Lines' Teachy TV mode. Pick the game (only the profile's game: ENG, SPA/FRE/ITA/GER, JPN, mGBA, Switch), console and settings (or Any) above the tabs. Results are sorted by shortest total time. This is a port of [Ten Lines](https://github.com/Lincoln-LM/ten-lines) by Lincoln-LM (GPL-3.0), with its seed tables: FRLG seeds farmed by blisy, po, HunarPG, 10Ben, Real96, ColdStoneSys, Papa Jefé, and トノ. Refresh the tables with `python3 Core/Resources/FRLG/pull_frlg_seeds.py`.
- **FRLG calibration in the Generator (as in Ten Lines' Calibration tab).** A **Seed ±** box next to the seed also generates from the neighbouring farmed seeds for the chosen game and settings (default 20), with Initial Seed and Seed Time columns. Right-click a searcher result → **Calibrate in Generator** fills the seed, advances window, settings and method, then generates.
- **"All methods" (Static) and "All wild methods" (Wild)** in the Generator and the Searcher, with a Method column.
- **IV Calculator: Find IVs also fills the filters**: the nature, the level (lowest level entered, Wild) and the species (when the current location or category has it), not only the IVs.

**Gen 4**
- **Seed to Time, "Today" checkbox (DPPt and HGSS).** Paste the seeds a searcher returned (one per line; whole copied rows work, the first value is used). Generate picks the seed you can still hit today without changing the console clock: earliest hour first, then the most tries left from the next minute, then the shortest delay. "Any / Even / Odd" restricts the delay parity. The table lists its remaining times; Calibrate works as usual. Uses the computer's clock. Unticked, Seed to Time is unchanged.

**Gen 6 (X / Y / Omega Ruby / Alpha Sapphire), new, not yet confirmed on hardware**
- **Static** generator, checked against 3DSRNGTool: the game's categories and Pokémon, Delay filled from the Pokémon, Synchronize, Time column (60 advances per second) and an optional Target with a Shift column for calibrating after a miss.
- **Seed Finder (IVs)**: initial seed from the IVs of two Pokémon (full search, port of poke6-seed-finder by ukikagi) or from one Pokémon after a known base seed (quick search, as in Lincoln-LM's Gen 6 GPU Tools), with XY / ORAS frame presets from retailrng's guides. An IV Calculator button fills the IVs.
- **Gen 6 profiles** (TID, SID, Shiny Charm; TSV / TRV shown) in Gen 6 Tools → Profile Manager.

**App**
- Its own logo (orange "+" in the magnifier), and the macOS app is **PokeFinderPlus.app** (Dock, Finder and menu bar). Settings keep the PokeFinder name, so your profiles carry over.
- Window title, About box and update check point to this fork. Version numbers are PokéFinder's base version + `-plus.N` (e.g. 4.4.2-plus.1).
- On Windows the app is **PokeFinderPlus.exe**.

## Download

Get the latest version on the [Releases page](https://github.com/Vict0rious7/PokeFinderPlus/releases/latest), under **Assets**:

- **Windows 10 / 11 (64-bit):** `PokeFinderPlus-windows.zip`. Unzip it, open the `PokeFinderPlus` folder, run `PokeFinderPlus.exe`. The app isn't signed, so Windows may show "Windows protected your PC": click **More info → Run anyway**. Nothing to install; the Visual C++ runtime is included.
- **macOS (Intel or Apple Silicon):** `PokeFinderPlus-macos.zip`. Unzip, move `PokeFinderPlus.app` to Applications, right-click → **Open** the first time (not notarised).
- **Linux:** `PokeFinderPlus-linux-AppImage.tar.gz` (self-contained) or `PokeFinderPlus-linux.tar.gz` (needs Qt 6).

The app checks for a new release once a day. Builds of every push are also on the [Actions page](https://github.com/Vict0rious7/PokeFinderPlus/actions) (needs a GitHub login).

## Build on a Mac

With [Homebrew](https://brew.sh) installed, from this folder:

```sh
./build-mac.sh           # installs Qt, CMake, Ninja and Python 3.14 if missing, builds, opens PokeFinderPlus.app
./build-mac.sh --test    # same, and runs the tests first
./build-mac.sh --clean   # start again from an empty build folder
```

Windows and Linux: see PokéFinder's Building section below (the app is called PokeFinder there).

To publish a release: bump `-plus.N` in `POKEFINDERPLUS_VERSION` (`CMakeLists.txt`), push to master, wait for a green CI, then push the tag `v` + that version (e.g. `v4.4.2-plus.1`). CI checks the tag matches, builds Windows, macOS and Linux, starts the packaged Windows app once to check the zip, and attaches everything to the release. Tags with `-beta` / `-rc` become pre-releases.

---

# PokéFinder

Join the PokéFinder Discord server to talk about development and contribute.

[![PokéFinder](https://discordapp.com/assets/07dca80a102d4149e9736d4b162cff6f.ico)](https://discord.gg/XmgQF9X)

This will be a RNG Tool for all main Pokémon games generations 3-8. It currently supports generations 3/4 and parts of generation 5/8.

# Download

[Latest Official Release](https://github.com/Admiral-Fish/PokeFinder/releases/latest)

[Latest Nightly Build](https://github.com/Admiral-Fish/PokeFinder/actions)

# Features
Gen 3
- Egg
- GameCube
- IDs
- Static
- Wild

Gen 4
- Egg
- Event
- IDs
- Static
- Wild

Gen 5
- Dream Radar
- Egg
- Event
- Hidden Grotto
- IDs
- Static
- Wild

Gen 8
- Egg
- Event
- IDs
- Raid
- Static
- Underground
- Wild

# Supported Platforms

Windows
- Windows 10
- Windows 11

macOS
- macOS Sequoia
- macOS Tahoe
- macOS Golden Gate

Linux
- Ubuntu 24.04
- Ubuntu 26.04

Qt
- 6.10 or newer

# Installing

Windows
- Install the [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)
- Download the win zip folder from the [releases page](https://github.com/Admiral-Fish/PokeFinder/releases/latest)
- Extract PokéFinder from the zip folder

MacOS
- Download the macos zip folder from the [releases page](https://github.com/Admiral-Fish/PokeFinder/releases/latest)
- Extract PokéFinder from the zip folder

Linux
- Install Qt 6
  - [Qt website](https://www.qt.io/download)
  - sudo apt install qt6-base-dev
- Download the linux zip folder from the [releases page](https://github.com/Admiral-Fish/PokeFinder/releases/latest)
- Extract PokéFinder from the zip folder

# Building

Windows
- Install the dependencies
  - [Qt 6](https://www.qt.io/download)
  - [Build tools for Visual Studio](https://visualstudio.microsoft.com/downloads/)
  - [Python 3.14](https://www.python.org/downloads/)
- Build
  - git submodule update
  - mkdir build
  - cd build
  - cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RELEASE ../
  - cmake --build .
- Bundle
  - mk PokeFinder-windows
  - move release\PokeFinder.exe PokeFinder-windows\PokeFinder.exe 
  - windeployqt --release --no-translations --no-angle --no-plugins --no-opengl-sw PokeFinder.exe
  - xcopy /I "QTPath"\plugins\platforms\qwindows.dll PokeFinder-windows\platforms\
  - xcopy /I "QTPath"\plugins\styles\qwindowsvistastyle.dll PokeFinder-windows\styles\

MacOS
- Install the dependencies
  - Qt 6 ([brew](https://formulae.brew.sh/formula/qt) or the [Qt website](https://www.qt.io/download))
  - [Python 3.14](https://www.python.org/downloads/)
- Build
  - git submodule update
  - mkdir build
  - cd build
  - PATH="PATH=$PATH:$HOME/Qt/6.10/macos/bin" cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE ../
    - Replace Qt path/version as necessary
  - cmake --build .
- Bundle
  - macdeployqt PokeFinder.app -dmg -verbose=2

Linux
- Install the dependencies
  - Qt 6
    - [Qt website](https://www.qt.io/download)
    - sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
  - [Python 3.14](https://www.python.org/downloads/)
  - sudo apt install build-essential libgl1-mesa-dev
- Build
  - git submodule update
  - mkdir build
  - cd build
  - cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10/gcc_64 ../
    - Replace Qt path/version as necessary
  - cmake --build .

# Credits (in no particular order)
- Bill Young, Mike Suleski, and Andrew Ringer for [RNG Reporter](https://github.com/Slashmolder/RNGReporter)
- chiizu for [PPRNG](https://github.com/chiizu/PPRNG)
- wwwwwwzx for [3DSRNG Tool](https://github.com/wwwwwwzx/3DSRNGTool)
- The PokemonRNG team for various contributions and research [zaksabeast](https://github.com/zaksabeast), [EzPzstreamz](https://github.com/SteveCookTU), [Shiny_Sylveon](https://github.com/ShinySylveon04), [Vlad](https://github.com/RichardPaulAstley), [Real96](https://github.com/Real96)
- Other great people for various help and research ([OmegaDonut](https://github.com/OmegaDonut), [Bond697](https://github.com/Bond697), [Kaphotics](https://github.com/kwsch), [SciresM](https://github.com/SciresM), Zari, amab, Marin, [Lean](https://github.com/Leanny), etc)
- Sans for initial GUI design
