/*
 * GS Ball Celebi (Crystal VC): the hunt's session state (Kenji class, timer offset, reset rule, re-saves) and the re-save
 * step (class 1 -> re-plan for class >= 2; class >= 2 -> calibrate again), plus the core re-save used to check it.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's celebi_tool.py v0.6 (session_load / session_save, resave_step, the calibrate --session
 * update) and runs/t25/mk_resave.py (core re-save). The session file is the prototype's JSON, key for key.
 */

#ifndef CELEBISESSION_HPP
#define CELEBISESSION_HPP

#include <Core/Gen2/CelebiCalibration.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Gen2
{
    constexpr const char *SESSION_VERSION = "0.6";

    struct Session
    {
        int resaves = 0;
        std::vector<std::optional<int>> classHistory; // Kenji class before each re-save
        std::optional<int> kenjiClass; // 1 or 2 (>= 2); empty = unknown (calibrate)
        std::optional<int> X; // accepted timer offset (frames)
        std::optional<int> timerMs; // PokeTimerPlus calibration
        std::string resetRule = "A";
        std::optional<int> kPlan; // carry count to plan for
        bool needsCalibration = true;
        std::optional<int> accuracy;
        std::optional<std::string> lang;
        std::optional<double> pBand;
        nlohmann::json log = nlohmann::json::array();

        nlohmann::json toJson() const;
        static Session fromJson(const nlohmann::json &j);
    };

    /// Missing or empty path: a new session.
    Session sessionLoad(const std::string &path);
    void sessionSave(const std::string &path, const Session &s);

    /// Records a re-save; returns the advice lines (the prototype's resave_step).
    std::vector<std::string> resaveStep(Session &s);
    /// The calibrate --session update after an accepted calibration.
    void sessionAfterCalibration(Session &s, const Verdict &v, const std::optional<BandAdvice> &band, int N, std::optional<int> timerMs,
                                 int accuracy, const std::string &lang);

    /// Core re-save (mk_resave.py): boot the save at chip clock rtc, run the plan (0 / 0 / 1933 of the given plan) to VBlank
    /// vb (past the map load, daily reset done), copy the saved WRAM segments into the save (both copies, checksums).
    Bytes coreResave(const RomPtr &rom, const Bytes &sram, const Plan &p, const Rtc &rtc = { 30, 12, 18, 117, 0 }, int vb = 900);
}

#endif // CELEBISESSION_HPP
