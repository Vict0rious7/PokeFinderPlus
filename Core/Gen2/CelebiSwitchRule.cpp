/*
 * GS Ball Celebi: the switch rule (see CelebiSwitchRule.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSwitchRule.hpp"
#include <algorithm>
#include <cmath>

namespace Gen2
{
    const char *RESAVE_HINT = "Two re-saves without the shiny: if you tend to be late or early on one press (often YES, the short gap after "
                              "the 6th prompt), practise that gap in PokeTimerPlus. The tool cannot detect a steady per-press delay.";

    ErrModel errModel(double p0, int D, double w3)
    {
        double r = 1 - p0;
        if (D == 2)
        {
            return { { 0, p0 }, { 1, .4 * r }, { -1, .4 * r }, { 2, .1 * r }, { -2, .1 * r } };
        }
        double q = r * (1 - w3);
        return { { 0, p0 }, { 1, .4 * q }, { -1, .4 * q }, { 2, .1 * q }, { -2, .1 * q }, { 3, .5 * r * w3 }, { -3, .5 * r * w3 } };
    }

    ErrModel shiftEm(const ErrModel &em, int d)
    {
        ErrModel out;
        for (int k = -3; k < 4; k++)
        {
            auto it = em.find(k + d);
            if (it != em.end())
            {
                out[k] = it->second;
            }
        }
        return out;
    }

    const std::vector<std::pair<int, double>> &infoModel()
    {
        static const std::vector<std::pair<int, double>> m = { { 0, .5 }, { 1, .2 }, { -1, .2 }, { 2, .05 }, { -2, .05 } };
        return m;
    }

    double rPrior(int r)
    {
        return r == 0 ? .8 : (std::abs(r) == 1 ? .08 : .02);
    }

    namespace
    {
        double get(const ErrModel &em, int k)
        {
            auto it = em.find(k);
            return it == em.end() ? 0.0 : it->second;
        }
    }

    JointModel jointModel(const GridOf &grid, const ErrModel &em, int nplans)
    {
        JointModel lik(nplans);
        for (int i = 0; i < nplans; i++)
        {
            auto &d = lik[i];
            for (int ri = 0; ri < 5; ri++)
            {
                for (const auto &[dl, w] : infoModel())
                {
                    int X = RS[ri] + dl;
                    if (std::abs(X) > 2)
                    {
                        continue;
                    }
                    ErrModel es = shiftEm(em, dl);
                    for (int F = 0; F < 60; F++)
                    {
                        const Grid *g = grid(i, F, X);
                        if (!g)
                        {
                            continue;
                        }
                        int D = g->D;
                        for (int e1 = -D; e1 <= D; e1++)
                        {
                            for (int e2 = -D; e2 <= D; e2++)
                            {
                                for (int e3 = -D; e3 <= D; e3++)
                                {
                                    double p = get(es, e1) * get(es, e2) * get(es, e3);
                                    if (p != 0)
                                    {
                                        auto it = d.try_emplace(g->at(e1, e2, e3), Lik {}).first;
                                        it->second[ri * 60 + F] += w * p;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return lik;
    }

    JointPosterior jointPosterior(const JointModel &lik, const std::vector<HuntObs> &obs, double eps)
    {
        JointPosterior jp;
        for (int j = 0; j < 5; j++)
        {
            for (int F = 0; F < 60; F++)
            {
                jp.post[j * 60 + F] = rPrior(RS[j]) * (1.0 / 60);
            }
        }
        for (const auto &o : obs)
        {
            std::array<double, 300> L {};
            bool hit = false;
            for (auto cd : o.cands)
            {
                auto it = lik[o.plan].find(cd);
                if (it != lik[o.plan].end())
                {
                    hit = true;
                    for (int x = 0; x < 300; x++)
                    {
                        L[x] += it->second[x];
                    }
                }
            }
            jp.misfit.push_back(!hit);
            double z = 0;
            for (int j = 0; j < 5; j++)
            {
                int src = RS[j] - o.timerMoved; // the residual at that attempt
                int si = src + 2;
                for (int F = 0; F < 60; F++)
                {
                    double ls = (si >= 0 && si < 5) ? L[si * 60 + F] : 0.0;
                    double &p = jp.post[j * 60 + F];
                    p *= (1 - eps) * ls + eps / 65536;
                    z += p;
                }
            }
            for (double &p : jp.post)
            {
                p /= z;
            }
        }
        return jp;
    }

    SwitchAdvice jointAdvise(const JointPosterior &jp, const std::set<int> &S1, const std::set<int> &S2, int on, int c, double tau)
    {
        SwitchAdvice a;
        std::array<double, 60> pF {};
        for (int j = 0; j < 5; j++)
        {
            for (int F = 0; F < 60; F++)
            {
                a.pr[j] += jp.post[j * 60 + F];
                pF[F] += jp.post[j * 60 + F];
            }
        }
        int ri = static_cast<int>(std::max_element(a.pr.begin(), a.pr.end()) - a.pr.begin()); // first maximum, as numpy argmax
        for (int F : S1)
        {
            a.p1 += pF[F];
        }
        for (int F : S2)
        {
            a.p2 += pF[F];
        }
        a.move = (RS[ri] != c && a.pr[ri] > THETA_R) ? RS[ri] - c : 0;
        if (on == 0 && a.p1 < tau)
        {
            a.advice = a.p2 >= tau ? "switch to plan 2" : "re-save";
        }
        else if (on == 1 && a.p2 < tau)
        {
            a.advice = "re-save";
        }
        else
        {
            a.advice = "stay on plan " + std::to_string(on + 1);
        }
        return a;
    }

    SwitchAdvice switchRule(const JointModel &lik, const std::vector<HuntObs> &obs, const std::set<int> &S1, const std::set<int> &S2, double tau)
    {
        JointPosterior jp = jointPosterior(lik, obs);
        int on = obs.empty() ? 0 : obs.back().plan, c = obs.empty() ? 0 : obs.back().timerMoved;
        int mis = 0;
        for (std::size_t i = jp.misfit.size() >= 3 ? jp.misfit.size() - 3 : 0; i < jp.misfit.size(); i++)
        {
            mis += jp.misfit[i];
        }
        SwitchAdvice a = jointAdvise(jp, S1, S2, on, c, tau);
        a.recalibrate = mis >= MISFIT;
        return a;
    }
}
