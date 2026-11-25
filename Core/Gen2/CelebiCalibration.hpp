/*
 * GS Ball Celebi (Crystal VC): calibration (timer offset X, Kenji class, clock carries from the player's attempts).
 * Step 4 of the port: stats <-> DVs and the calibration grid jobs; ranking and the reset band follow in step 5.
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

}

#endif // CELEBICALIBRATION_HPP
