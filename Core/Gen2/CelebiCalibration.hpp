/*
 * GS Ball Celebi (Crystal VC): calibration (timer offset X, Kenji class, clock carries from the player's attempts) and the
 * reset band (which reset moment keeps the hunt in the planned clock class).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's celebi_tool.py v0.6 (stat / dv_cands / dv_stats, carry_rep, cal_jobs, cal_index,
 * cal_rank, cal_verdict, ml_seconds, p_carry, band_of, carry_evidence, band_posterior, band_advice). Checked against the
 * prototype (Test/Gen2).
 */

#ifndef CELEBICALIBRATION_HPP
#define CELEBICALIBRATION_HPP

#include <Core/Gen2/CelebiSearch.hpp>
#include <Core/Gen2/CrystalSave.hpp>
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Gen2
{
    /// One attempt as the player notes it: Celebi's six L30 stats and the Pokégear time read just before the reset.
    struct Attempt
    {
        std::array<int, 6> stats {}; // HP, Atk, Def, SpA, SpD, Spe
        std::string ingame = "20:30";
        int plan = 1; // hunt attempts: plan 1 or 2
        int timerMoved = 0; // frames the timer was moved since calibration
        char reset = 'A'; // reset moment rule used (calibration, v0.6)
    };

    // ------------------------------------------------------------------ stats <-> DVs
    int stat(int base, int dv, int level = 30, bool hp = false);
    /// DV candidates (as 16-bit Atk/Def Spe/Spc) of Celebi's six L30 stats.
    const std::set<std::uint16_t> &dvCands(const std::array<int, 6> &stats);
    std::array<int, 6> dvStats(std::uint16_t dv);
    /// dv_cands(dv_stats(dv)), cached.
    const std::set<std::uint16_t> &dvCandsOf(std::uint16_t dv);

    // ------------------------------------------------------------------ calibration grids
    constexpr int CARRY[4][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 } }; // seconds, minutes, hours carry
    struct CarryRep
    {
        Rtc rtc;
        std::array<int, 3> offset; // Oh, Om, Os
    };
    /// Chip clock + offset giving in-game ih:im:isec with exactly the carry pattern CARRY[k].
    CarryRep carryRep(int ih, int im, int k, int isec = 30);

    struct CalJob
    {
        std::string key; // g_<kenji>_<k>_<hh>_<X>
        int kenji, k, hour, X, D;
        Rtc rtc;
        Bytes sram; // template with the representative clock offset and the Kenji class
    };
    /// One grid per (Kenji class, carry count, in-game hour of the attempts), split per X.
    std::vector<CalJob> calJobs(const CrystalSave &tpl, const Minimal &mini, const std::vector<Attempt> &attempts,
                                const std::vector<int> &kenjis = { 1, 2 }, int xlo = -40, int xhi = 40, int D = 2);

    // ------------------------------------------------------------------ ranking and verdict
    /// A leaf of a calibration grid: timer offset X and press errors on 6th / YES / final.
    using Leaf = std::array<int, 4>;
    struct CalKey
    {
        int kenji, k, hour;
        std::uint16_t dv;
        auto operator<=>(const CalKey &) const = default;
    };
    /// (Kenji, k, hour, DV) -> leaves, over the calibration grids (cal_index).
    using CalIndex = std::map<CalKey, std::vector<Leaf>>;
    void calIndexAdd(CalIndex &inv, int kenji, int k, int hour, const Grid &g);
    /// Computes the calibration grids of `jobs` (plan d6 / dyes / final) on `threads` threads and indexes them.
    CalIndex calibrationIndex(const RomPtr &rom, const Plan &p, int d6, int dyes, int final, const std::vector<CalJob> &jobs, int threads = 0,
                              const std::function<void(int, int)> &progress = {});

    struct Fit
    {
        int explained, kenji;
        std::vector<int> carries;
        int X, ms;
        /// per attempt: the error triples at X, or "X+d" (nearest explaining X) / "none"
        std::vector<std::vector<std::array<int, 3>>> errors;
        std::vector<std::string> errorNote;
    };
    constexpr int KSETS[7][2] = { { 0, -1 }, { 1, -1 }, { 2, -1 }, { 3, -1 }, { 0, 1 }, { 1, 2 }, { 2, 3 } };
    std::vector<Fit> calRank(const std::vector<Attempt> &attempts, const CalIndex &inv, const std::vector<int> &kenjis = { 1, 2 }, int D = 2);

    constexpr int MIN_ATT = 6, MAX_ATT = 10; // at least 6; +2 at a time; at 10 shift the timer and start again
    constexpr int SHIFT = 5; // frames (+84 ms): breaks the calibration aliases
    struct Verdict
    {
        bool accepted;
        Fit best;
        std::optional<Fit> runnerUp;
        std::vector<std::vector<int>> carriesBest, carriesNext;
    };
    /// Accepted when the best explains >= N-2 and no other Kenji class or X (beyond +-2) explains more than N-3.
    Verdict calVerdict(const std::vector<Fit> &rank, int N);

    enum class CalState
    {
        Accepted,
        NeedMore, // fewer than 6 attempts
        AddTwo, // not conclusive: add 2 attempts
        ShiftTimer // not conclusive at 10: move the timer +5 frames and start again with 6 attempts
    };
    CalState calState(const Verdict &v, int N);

    // ------------------------------------------------------------------ reset band (v0.6, option a)
    constexpr double ML_DELAY[2] = { 12.03, 12.42 }; // s from the first emulated frame to the map load's clock read
    constexpr double REACTION[2] = { 0.0, 3.0 };
    constexpr double BAND_EPS = 0.03; // attempts outside the model (rule not followed, misread)
    double resetWait(char rule); // A: 20 s after the Pokégear minute changes; B: right after it
    std::pair<double, double> mlSeconds(char rule, double timerMs);
    double pCarry(double Os, double lo, double hi);
    std::vector<int> bandOf(char rule, double timerMs, double eps = 0.05);
    /// Per attempt: the carry counts (0-3) that explain it at the accepted Kenji class with X_i within +-2 of X.
    std::vector<std::set<int>> carryEvidence(const std::vector<Attempt> &att, const CalIndex &inv, int kenji, int X, int D);

    struct BandObs
    {
        std::set<int> ks; // carry counts explaining the attempt
        int minute; // in-game minute
        char rule; // 'A' or 'B'
    };
    /// Marginal posterior over the offset seconds (60).
    std::vector<double> bandPosterior(const std::vector<BandObs> &obs, double timerMs, double eps = BAND_EPS);
    struct BandAdvice
    {
        char rule;
        std::optional<int> kPlan;
        double pBand, value; // unrounded
        bool more, confirm, mixed;
        std::vector<int> bandA, bandB;
    };
    BandAdvice bandAdvice(const std::vector<BandObs> &obs, double timerMs, double margin = 0.01, double vmin = 0.99);
}

#endif // CELEBICALIBRATION_HPP
