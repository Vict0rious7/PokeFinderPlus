/*
 * GS Ball Celebi (Crystal VC): the switch rule. After each hunt attempt: stay on plan 1, switch to plan 2, re-save, move the
 * timer, or recalibrate, from a joint posterior over the timer residual r (-2..2 frames) and the hidden play-time frame
 * counter F (0-59). The info-box press is in the model (an info-box miss of d frames = the whole timeline shifted by d with
 * the later presses at e - d).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 *
 * Port of the research prototype's celebi_tool.py v0.6 (err_model, _shift_em, joint_model, joint_posterior, joint_advise,
 * MISFIT, RESAVE_HINT). Checked against the prototype on the 68 advice steps of the v0.6 harness (Test/Gen2).
 */

#ifndef CELEBISWITCHRULE_HPP
#define CELEBISWITCHRULE_HPP

#include <Core/Gen2/CelebiSearch.hpp>
#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Gen2
{
    /// Press-error distribution for 6th / YES / final: error (-3..3) -> probability.
    using ErrModel = std::map<int, double>;
    /// exact p0; +-1 80% of the rest, +-2 20%; accuracy 3: a share w3 of the non-exact presses is +-3.
    ErrModel errModel(double p0, int D = 2, double w3 = 0.05);
    /// The error law seen at an info-box shift of d: {k: em[k + d]}.
    ErrModel shiftEm(const ErrModel &em, int d);

    constexpr int RS[5] = { -2, -1, 0, 1, 2 }; // timer residual left by calibration
    const std::vector<std::pair<int, double>> &infoModel(); // info-box law {0: .5, 1: .2, -1: .2, 2: .05, -2: .05}, prototype order
    double rPrior(int r); // {0: .8, +-1: .08, +-2: .02}
    constexpr double THETA_R = 0.9; // "move your timer" when P(r*) > this
    constexpr int MISFIT = 2; // attempts of the last 3 that fit no (r, F) -> recalibrate
    constexpr double TAU = 0.1; // leave a plan when P(F in its set) < tau

    /// P(DV | plan, r, F) as 5 x 60 (r index, F).
    using Lik = std::array<double, 300>;
    /// LIK[plan][DV]
    using JointModel = std::vector<std::map<std::uint16_t, Lik>>;
    /// Grid lookup: the switch-rule grid of (plan, F, X), or null when missing.
    using GridOf = std::function<const Grid *(int plan, int F, int X)>;
    JointModel jointModel(const GridOf &grid, const ErrModel &em, int nplans = 2);

    struct HuntObs
    {
        int plan; // 0 or 1
        std::set<std::uint16_t> cands; // DV candidates of the attempt's stats
        int timerMoved; // frames the timer was moved before that attempt
    };
    struct JointPosterior
    {
        std::array<double, 300> post; // (r index, F)
        std::vector<bool> misfit; // per attempt: its DVs fit no (r, F)
    };
    JointPosterior jointPosterior(const JointModel &lik, const std::vector<HuntObs> &obs, double eps = 1e-4);

    struct SwitchAdvice
    {
        bool recalibrate = false; // MISFIT of the last 3 attempts fit nothing
        double p1 = 0, p2 = 0; // P(F in plan 1 / plan 2 set)
        std::string advice; // "stay on plan 1" / "stay on plan 2" / "switch to plan 2" / "re-save"
        int move = 0; // frames to move the timer (0 = don't)
        std::array<double, 5> pr {}; // P(timer residual r), r = -2..2
    };
    SwitchAdvice jointAdvise(const JointPosterior &jp, const std::set<int> &S1, const std::set<int> &S2, int on, int c, double tau = TAU);
    /// The whole rule for a list of hunt attempts (the prototype's switchrule --attempts).
    SwitchAdvice switchRule(const JointModel &lik, const std::vector<HuntObs> &obs, const std::set<int> &S1, const std::set<int> &S2,
                            double tau = TAU);

    extern const char *RESAVE_HINT;
}

#endif // CELEBISWITCHRULE_HPP
