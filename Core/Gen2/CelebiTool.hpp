/*
 * This file is part of PokéFinder
 * Copyright (C) 2017-2024 by Admiral_Fish, bumba, and EzPzStreamz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * PokeFinderPlus: new file. The Celebi tool's flow for the Gen 2 window, as the prototype's CLI chains it
 * (celebi_tool.py v0.6: makesave, plan, timer, calibrate, fsearch + pick, switchrule, resave): the shipped save template,
 * the player's game files, calibration with cached grids, the plans for the calibrated class, the switch-rule grids and
 * the hunt advice. Long steps take a progress callback and a cancel flag; finished grids are kept so a step resumes.
 */

#ifndef CELEBITOOL_HPP
#define CELEBITOOL_HPP

#include <Core/Gen2/CelebiCalibration.hpp>
#include <Core/Gen2/CelebiPlan.hpp>
#include <Core/Gen2/CelebiSearch.hpp>
#include <Core/Gen2/CelebiSwitchRule.hpp>
#include <Core/Gen2/CrystalSave.hpp>
#include <atomic>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Gen2
{
    using Cancel = std::atomic<bool>;
    /// Finished grids by job key (calibration g_<kenji>_<k>_<hh>_<X>, switch rule f_<plan>_<FF>[_x<X>]).
    using GridCache = std::map<std::string, Grid>;

    constexpr int CAL_FINAL = 1933; // the calibration plan is 0 / 0 / 1933 (Victor's retail calibrations, the v0.6 harness)
    constexpr const char *PLAN_INGAME = "20:30"; // plan discovery clock (the prototype's default)
    constexpr int FS_TARGET = 40, FS_UNION = 50; // fsearch stops when plan 1 covers 40 F values and plans 1 + 2 together 50
    constexpr int SWITCH_XS[5] = { 0, -1, 1, -2, 2 };

    // ------------------------------------------------------------------ inputs

    /// The values the player reads in game (SPEC 2.1).
    struct Visible
    {
        TextSpeed textSpeed = TextSpeed::Fast;
        bool stereo = false;
        int frame = 1; // 1-8
        bool raikou = false, entei = false; // still roaming
        bool lass = true;
        bool swarm = false;
        bool operator==(const Visible &) const = default;
    };
    nlohmann::json toJson(const Visible &v);
    Visible visibleFromJson(const nlohmann::json &j);

    /// The shipped template (a Crystal shrine save with every name and the trainer ID blanked), 32 KB.
    const Bytes &templateSave();

    /// The tool's save: the template with the player's visible values, a Kenji class (1 or 2) and optionally a clock offset
    /// (h, m, s; the template's day is kept). F and the play-time seconds stay the template's (F is covered by the plans).
    Bytes toolSave(const Visible &v, int kenji = 2, std::optional<std::array<int, 3>> offset = std::nullopt);

    struct GameFiles
    {
        RomPtr rom;
        std::string lang; // fr, de, es, it, en
        std::string kind; // "cia", "rom+patch", "dump"
        std::string label; // what was recognised
        std::string titleCheck; // CIA: the VC emulator check result
        bool sameEmulator = true;
    };
    /// A decrypted CIA of the VC title; empty error string on success.
    std::pair<std::optional<GameFiles>, std::string> loadCia(const Bytes &cia);
    /// A cartridge dump and the title's patch file (text).
    std::pair<std::optional<GameFiles>, std::string> loadRomPatch(const Bytes &rom, const std::string &patch);
    /// A cartridge dump alone (the VC patch never matters on the shrine path).
    std::pair<std::optional<GameFiles>, std::string> loadDump(const Bytes &rom);

    // ------------------------------------------------------------------ attempts

    nlohmann::json toJson(const Attempt &a);
    Attempt attemptFromJson(const nlohmann::json &j);
    /// "That's a shiny" check of an attempt's stats: true when every DV candidate is shiny.
    bool shinyStats(const std::array<int, 6> &stats);
    /// No DV gives these stats (a typo, or another Pokémon).
    bool impossibleStats(const std::array<int, 6> &stats);

    // ------------------------------------------------------------------ calibration

    struct CalOutcome
    {
        bool complete = false; // false: cancelled before every grid was done
        std::vector<Fit> rank;
        Verdict verdict {};
        CalState state = CalState::NeedMore;
        std::optional<BandAdvice> band;
        std::vector<std::set<int>> carries; // per attempt: carry counts explaining it (with the band check)
        int hour = 20, minute = 30; // in-game hour of most attempts, lower-median minute of that hour
    };
    /// Jobs still to compute for these attempts (the cache holds the rest).
    int calibrationTodo(const Plan &p, const Visible &v, const std::vector<Attempt> &attempts, int accuracy, const GridCache &cache);
    CalOutcome calibrate(const RomPtr &rom, const Plan &p, const Visible &v, const std::vector<Attempt> &attempts, int accuracy,
                         std::optional<int> timerMs, GridCache &cache, int threads = 0, const Progress &progress = {},
                         const Cancel *cancel = nullptr);

    // ------------------------------------------------------------------ plans for the calibrated class

    /// The save and clock the plans are searched at: the Kenji class found, the carry count to plan for, at the
    /// calibration attempts' in-game hour and minute (representative clock, SPEC 4.3).
    struct HuntSetup
    {
        int kenji = 2, k = 0, hour = 20, minute = 30;
        Bytes sram;
        Rtc rtc {};
    };
    HuntSetup huntSetup(const Visible &v, int kenji, int k, int hour, int minute);
    nlohmann::json toJson(const HuntSetup &h);
    HuntSetup huntSetupFromJson(const Visible &v, const nlohmann::json &j);

    nlohmann::json toJson(const Hit &h);
    Hit hitFromJson(const nlohmann::json &j);

    /// fsearch for the setup (resumes from `done`, the jobs already computed); stops at FS_TARGET / FS_UNION or on cancel.
    FsearchResult planSearch(const RomPtr &rom, const Plan &p, const HuntSetup &h, const std::map<std::string, std::vector<Hit>> &done,
                             int threads = 0, const Progress &progress = {}, const Cancel *cancel = nullptr);

    /// The switch-rule grids of plans 1 and 2 (60 F x timer offsets -2..+2, 600 jobs at accuracy 3); true when all are done.
    bool switchGridsCached(const RomPtr &rom, const Plan &p, const HuntSetup &h, const Hit &p1, const Hit &p2, int accuracy,
                           GridCache &cache, int threads = 0, const Progress &progress = {}, const Cancel *cancel = nullptr);
    int switchTodo(int accuracy, const GridCache &cache);

    struct HuntAttempt
    {
        int plan = 1; // 1 or 2
        std::array<int, 6> stats {};
        int timerMoved = 0; // frames the timer was moved since calibration
    };
    nlohmann::json toJson(const HuntAttempt &a);
    HuntAttempt huntAttemptFromJson(const nlohmann::json &j);

    /// The switch rule after these hunt attempts (the prototype's switchrule --attempts).
    SwitchAdvice huntAdvice(const GridCache &cache, const Hit &p1, const Hit &p2, int accuracy, const std::vector<HuntAttempt> &attempts);

    // ------------------------------------------------------------------ files

    /// Grids as a small binary file (key, X, D, DVs).
    void saveGrids(const std::string &path, const GridCache &cache);
    GridCache loadGrids(const std::string &path);

    /// ms for a timer offset in frames (SPEC 3.4: Δms = X × 1000 / 59.8312).
    int framesToMs(int frames);
}

#endif // CELEBITOOL_HPP
