/*
 * PokeFinderPlus: new file. CelebiSwitchRule tests (see CelebiSwitchRuleTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSwitchRuleTest.hpp"
#include <Core/Gen2/CelebiCalibration.hpp>
#include <Core/Gen2/CelebiSwitchRule.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <cmath>

using namespace Gen2;

namespace
{
    const json &G()
    {
        return Gen2Test::golden("gen2_switch.json");
    }

    std::uint16_t dvOfHex(const std::string &s)
    {
        return static_cast<std::uint16_t>(std::stoi(s, nullptr, 16));
    }

    /// switch-rule grids rebuilt from the filtered golden leaves; other leaves hold a DV that no attempt has
    struct Grids
    {
        std::map<std::array<int, 3>, Grid> g;
        const Grid *operator()(int plan, int F, int X) const
        {
            auto it = g.find({ plan, F, X });
            return it == g.end() ? nullptr : &it->second;
        }
    };

    Grids grids(const json &gs)
    {
        std::set<std::uint16_t> cands;
        for (const auto &c : gs["cands"])
        {
            cands.insert(dvOfHex(c.get<std::string>()));
        }
        std::uint16_t filler = 0;
        while (cands.count(filler))
        {
            filler++;
        }
        int n = gs["n_leaves"].get<int>();
        int D = (static_cast<int>(std::lround(std::cbrt(n))) - 1) / 2;
        Grids out;
        for (int i = 0; i < 2; i++)
        {
            for (int F = 0; F < 60; F++)
            {
                for (int X = -2; X <= 2; X++)
                {
                    Grid g;
                    g.X = X;
                    g.D = D;
                    g.dv.assign(n, filler);
                    out.g[{ i, F, X }] = g;
                }
            }
        }
        for (const auto &l : gs["leaves"])
        {
            Grid &g = out.g[{ l[0].get<int>(), l[1].get<int>(), l[2].get<int>() }];
            int w = g.width();
            g.dv[((l[3].get<int>() + D) * w + l[4].get<int>() + D) * w + l[5].get<int>() + D] = dvOfHex(l[6].get<std::string>());
        }
        return out;
    }
}

void CelebiSwitchRuleTest::errModels()
{
    ErrModel em = errModel(.5, 3, .05);
    for (const auto &[k, v] : G()["em"].items())
    {
        QVERIFY(std::abs(em.at(std::stoi(k)) - v.get<double>()) < 1e-15);
    }
    QCOMPARE(em.size(), G()["em"].size());
    ErrModel s = shiftEm(em, 1);
    QCOMPARE(s.at(-3), em.at(-2));
    QCOMPARE(s.count(3), std::size_t(0));
}

void CelebiSwitchRuleTest::harnessSteps()
{
    std::map<std::string, Grids> gridsOf;
    for (const auto &[k, gs] : G()["grids"].items())
    {
        gridsOf[k] = grids(gs);
    }
    ErrModel em = errModel(.5, 3, .05);
    int n = 0, bad = 0;
    for (const auto &b : G()["blocks"])
    {
        if (b["steps"].empty())
        {
            continue;
        }
        const Grids &gr = gridsOf[b["grids"].get<std::string>()];
        JointModel lik = jointModel([&](int i, int F, int X) { return gr(i, F, X); }, em);
        auto s1 = b["S1"].get<std::vector<int>>(), s2 = b["S2"].get<std::vector<int>>();
        std::set<int> S1(s1.begin(), s1.end()), S2(s2.begin(), s2.end());
        std::vector<HuntObs> all;
        for (const auto &x : b["hunt"])
        {
            std::array<int, 6> st;
            for (int i = 0; i < 6; i++)
            {
                st[i] = x["stats"][i].get<int>();
            }
            all.push_back({ x["plan"].get<int>() - 1, dvCands(st), x["timer_moved"].get<int>() });
        }
        for (const auto &s : b["steps"])
        {
            int k = s["n"].get<int>();
            std::vector<HuntObs> obs(all.begin(), all.begin() + k);
            SwitchAdvice a = switchRule(lik, obs, S1, S2);
            JointPosterior jp = jointPosterior(lik, obs);
            bool ok = a.recalibrate == s["recalibrate"].get<bool>() && a.advice == s["advice"].get<std::string>() && a.move == s["move"].get<int>()
                && jp.misfit == s["misfit"].get<std::vector<bool>>() && std::abs(a.p1 - s["p1"].get<double>()) < 1e-9
                && std::abs(a.p2 - s["p2"].get<double>()) < 1e-9;
            for (int r = 0; ok && r < 5; r++)
            {
                ok = std::abs(a.pr[r] - s["pr"][r].get<double>()) < 1e-9;
            }
            // the CLI line, to 2 decimals
            char line[128];
            std::snprintf(line, sizeof line, "P(F in plan 1 set) %.2f   P(F in plan 2 set) %.2f   -> %s", a.p1, a.p2, a.advice.c_str());
            ok = ok && (a.recalibrate || s["cli_all"].get<std::string>().find(line) != std::string::npos);
            if (!ok)
            {
                bad++;
                qWarning("%s step %d: %s / %s, move %d", b["tag"].get<std::string>().c_str(), k, a.advice.c_str(),
                         s["advice"].get<std::string>().c_str(), a.move);
            }
            n++;
        }
    }
    qInfo("switch rule: %d/%d harness advice steps identical to the prototype", n - bad, n);
    QCOMPARE(n, 68);
    QCOMPARE(bad, 0);
}

void CelebiSwitchRuleTest::retail()
{
    // realcheck.py: Victor's retail calibration attempts against the joint model on the calibration grids (Kenji 1, F fixed)
    const json &cal = Gen2Test::golden("gen2_cal.json");
    ErrModel em = errModel(.5, 3, .05);
    for (const auto &rc : G()["realcheck"])
    {
        const json *cs = nullptr;
        for (const auto &c : cal["cases"])
        {
            if (c["name"] == rc["name"] && c["D"] == 3)
            {
                cs = &c;
            }
        }
        QVERIFY(cs);
        // leaves by (k, X): DV -> leaves (from the filtered calibration index; it holds every candidate of these attempts)
        std::map<std::tuple<int, int, std::uint16_t>, std::vector<std::array<int, 3>>> idx;
        int hour = rc["hour"].get<int>();
        for (const auto &e : (*cs)["grid"])
        {
            if (e[0].get<int>() != 1 || e[2].get<int>() != hour)
            {
                continue;
            }
            for (const auto &x : e[4])
            {
                idx[{ e[1].get<int>(), x[0].get<int>(), dvOfHex(e[3].get<std::string>()) }].push_back({ x[1].get<int>(), x[2].get<int>(), x[3].get<int>() });
            }
        }
        std::array<double, 5> post;
        for (int j = 0; j < 5; j++)
        {
            post[j] = rPrior(RS[j]);
        }
        int Xh = rc["Xh"].get<int>();
        auto ks = rc["ks"].get<std::vector<int>>();
        std::vector<bool> fit;
        for (const auto &a : (*cs)["attempts"])
        {
            std::array<int, 6> st;
            for (int i = 0; i < 6; i++)
            {
                st[i] = a["stats"][i].get<int>();
            }
            const auto &cands = dvCands(st);
            std::array<double, 5> L {};
            for (int j = 0; j < 5; j++)
            {
                for (const auto &[dl, w] : infoModel())
                {
                    ErrModel es = shiftEm(em, dl);
                    int X = Xh + RS[j] + dl;
                    double best = 0;
                    for (int k : ks)
                    {
                        double p = 0;
                        for (auto c : cands)
                        {
                            auto it = idx.find({ k, X, c });
                            if (it == idx.end())
                            {
                                continue;
                            }
                            for (const auto &e : it->second)
                            {
                                auto g = [&](int v) { return es.count(v) ? es.at(v) : 0.0; };
                                p += g(e[0]) * g(e[1]) * g(e[2]);
                            }
                        }
                        best = std::max(best, p);
                    }
                    L[j] += w * best;
                }
            }
            double sum = L[0] + L[1] + L[2] + L[3] + L[4], z = 0;
            fit.push_back(sum > 0);
            for (int j = 0; j < 5; j++)
            {
                post[j] *= L[j] + 1e-4 / 65536;
                z += post[j];
            }
            for (double &p : post)
            {
                p /= z;
            }
        }
        QCOMPARE(fit, rc["fit"].get<std::vector<bool>>());
        for (int j = 0; j < 5; j++)
        {
            QVERIFY(std::abs(post[j] - rc["pr"][j].get<double>()) < 1e-9);
        }
        QVERIFY(post[2] >= 0.99);
        qInfo("retail %s: %zu/%zu attempts fit, P(r = 0) %.3f", rc["name"].get<std::string>().c_str(),
              static_cast<std::size_t>(std::count(fit.begin(), fit.end(), true)), fit.size(), post[2]);
    }
}
