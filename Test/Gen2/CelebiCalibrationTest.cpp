/*
 * PokeFinderPlus: new file. CelebiCalibration tests (see CelebiCalibrationTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiCalibrationTest.hpp"
#include <Core/Gen2/CelebiCalibration.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <cmath>

using namespace Gen2;

namespace
{
    const json &G()
    {
        return Gen2Test::golden("gen2_cal.json");
    }

    std::vector<Attempt> attempts(const json &a)
    {
        std::vector<Attempt> v;
        for (const auto &x : a)
        {
            Attempt t;
            for (int i = 0; i < 6; i++)
            {
                t.stats[i] = x["stats"][i].get<int>();
            }
            t.ingame = x["ingame"].get<std::string>();
            t.reset = x.value("reset", std::string("A"))[0];
            v.push_back(t);
        }
        return v;
    }

    CalIndex index(const json &grid)
    {
        CalIndex inv;
        for (const auto &e : grid)
        {
            CalKey k { e[0].get<int>(), e[1].get<int>(), e[2].get<int>(), static_cast<std::uint16_t>(std::stoi(e[3].get<std::string>(), nullptr, 16)) };
            for (const auto &x : e[4])
            {
                inv[k].push_back({ x[0].get<int>(), x[1].get<int>(), x[2].get<int>(), x[3].get<int>() });
            }
        }
        return inv;
    }

    int ties = 0;

    /// a Fit against the prototype's dict; the nearest-X note may differ in sign on an exact tie (prototype set order)
    bool sameFit(const Fit &f, const json &o)
    {
        if (f.explained != o["explained"].get<int>() || f.kenji != o["kenji"].get<int>() || f.X != o["X"].get<int>()
            || f.ms != o["ms"].get<int>() || f.carries != o["carries"].get<std::vector<int>>() || f.errors.size() != o["errors"].size())
        {
            return false;
        }
        for (std::size_t i = 0; i < f.errors.size(); i++)
        {
            const auto &e = o["errors"][i];
            if (!e.empty() && e[0].is_string())
            {
                std::string want = e[0].get<std::string>();
                if (f.errorNote[i] == want)
                {
                    continue;
                }
                bool tie = want.size() > 2 && f.errorNote[i].size() > 2 && want.substr(2) == f.errorNote[i].substr(2) && want[1] != f.errorNote[i][1];
                if (!tie)
                {
                    return false;
                }
                ties++;
                continue;
            }
            if (!f.errorNote[i].empty() || f.errors[i].size() != e.size())
            {
                return false;
            }
            for (std::size_t j = 0; j < e.size(); j++)
            {
                if (f.errors[i][j] != (std::array<int, 3> { e[j][0].get<int>(), e[j][1].get<int>(), e[j][2].get<int>() }))
                {
                    return false;
                }
            }
        }
        return true;
    }

    std::vector<BandObs> bandObs(const json &obs)
    {
        std::vector<BandObs> v;
        for (const auto &o : obs)
        {
            auto ks = o[0].get<std::vector<int>>();
            v.push_back({ std::set<int>(ks.begin(), ks.end()), o[1].get<int>(), o[2].get<std::string>()[0] });
        }
        return v;
    }

    /// band_advice against the prototype's dict (p_band / value are rounded there to 3 / 4 decimals)
    bool sameAdvice(const BandAdvice &a, const std::vector<double> &post, const json &w, const json &wpost)
    {
        bool ok = std::string(1, a.rule) == w["rule"].get<std::string>() && a.more == w["more"].get<bool>() && a.confirm == w["confirm"].get<bool>()
            && a.mixed == w["mixed"].get<bool>() && a.bandA == w["bandA"].get<std::vector<int>>() && a.bandB == w["bandB"].get<std::vector<int>>()
            && std::abs(a.pBand - w["p_band"].get<double>()) <= 0.0005 + 1e-9 && std::abs(a.value - w["value"].get<double>()) <= 0.00005 + 1e-9;
        ok = ok && (w["k_plan"].is_null() ? !a.kPlan : (a.kPlan && *a.kPlan == w["k_plan"].get<int>()));
        for (int o = 0; ok && o < 60; o++)
        {
            ok = std::abs(post[o] - wpost[o].get<double>()) <= 1e-9;
        }
        return ok;
    }
}

void CelebiCalibrationTest::dvCandidates()
{
    // every DV is among the candidates of its own stats; SpA != SpD gives none
    for (int dv = 0; dv < 0x10000; dv += 7)
    {
        QVERIFY(dvCandsOf(static_cast<std::uint16_t>(dv)).count(static_cast<std::uint16_t>(dv)));
    }
    QVERIFY(dvCands({ 100, 68, 73, 68, 69, 71 }).empty());
    QCOMPARE(stat(100, 15, 30, true), 109);
    QCOMPARE(stat(100, 0), 65);
}

void CelebiCalibrationTest::calibrations()
{
    int n = 0;
    for (const auto &c : G()["cases"])
    {
        auto att = attempts(c["attempts"]);
        CalIndex inv = index(c["grid"]);
        int D = c["D"].get<int>();
        auto rank = calRank(att, inv, { 1, 2 }, D);
        QCOMPARE(rank.size(), c["rank"].size());
        for (std::size_t i = 0; i < rank.size(); i++)
        {
            QVERIFY2(sameFit(rank[i], c["rank"][i]), (c["name"].get<std::string>() + " rank " + std::to_string(i)).c_str());
        }
        Verdict v = calVerdict(rank, static_cast<int>(att.size()));
        const auto &w = c["verdict"];
        QCOMPARE(v.accepted, w["accepted"].get<bool>());
        QVERIFY(sameFit(v.best, w["best"]));
        QCOMPARE(v.runnerUp.has_value(), !w["runner_up"].is_null());
        if (v.runnerUp)
        {
            QVERIFY(sameFit(*v.runnerUp, w["runner_up"]));
        }
        QCOMPARE(v.carriesBest, w["carries_best"].get<std::vector<std::vector<int>>>());
        QCOMPARE(v.carriesNext, w["carries_next"].get<std::vector<std::vector<int>>>());
        auto ev = carryEvidence(att, inv, v.best.kenji, v.best.X, D);
        std::vector<std::vector<int>> evv;
        for (const auto &s : ev)
        {
            evv.emplace_back(s.begin(), s.end());
        }
        QCOMPARE(evv, c["evidence"].get<std::vector<std::vector<int>>>());
        for (const auto &[tms, b] : c["bands"].items())
        {
            std::vector<BandObs> obs;
            for (std::size_t i = 0; i < att.size(); i++)
            {
                obs.push_back({ ev[i], hm(att[i].ingame) % 60, att[i].reset });
            }
            double t = std::stod(tms);
            QVERIFY(sameAdvice(bandAdvice(obs, t), bandPosterior(obs, t), b, b["post"]));
        }
        n++;
    }
    qInfo("calibration: %d cases identical to the prototype (rank, verdict, carry evidence, band at 0 / 1,700 / 3,000 ms; %d "
          "nearest-X notes on an exact tie)",
          n, ties);
}

void CelebiCalibrationTest::gateNumbers()
{
    // the gate numbers, stated once more on the C++ outputs
    auto run = [](const std::string &name, int D) {
        for (const auto &c : G()["cases"])
        {
            if (c["name"] == name && c["D"] == D)
            {
                auto att = attempts(c["attempts"]);
                CalIndex inv = index(c["grid"]);
                Verdict v = calVerdict(calRank(att, inv, { 1, 2 }, D), static_cast<int>(att.size()));
                auto ev = carryEvidence(att, inv, v.best.kenji, v.best.X, D);
                std::vector<BandObs> obs;
                for (std::size_t i = 0; i < att.size(); i++)
                {
                    obs.push_back({ ev[i], hm(att[i].ingame) % 60, att[i].reset });
                }
                return std::pair(v, bandAdvice(obs, 1700));
            }
        }
        throw std::runtime_error("case missing");
    };
    auto [c1, b1] = run("cal1", 3);
    QVERIFY(c1.accepted);
    QCOMPARE(c1.best.X, -21);
    QCOMPARE(c1.best.kenji, 1);
    QCOMPARE(b1.rule, 'A');
    QCOMPARE(*b1.kPlan, 0);
    QVERIFY(b1.more); // "add 2 attempts"
    auto [c2, b2] = run("cal2", 3);
    QVERIFY(c2.accepted);
    QCOMPARE(c2.best.X, -1);
    QCOMPARE(c2.best.kenji, 1);
    QCOMPARE(b2.rule, 'A');
    QCOMPARE(std::round(b2.pBand * 100) / 100, 0.02);
    auto [c6, b6] = run("b5_6", 3);
    QVERIFY(!c6.accepted);
    QCOMPARE(calState(c6, 6), CalState::AddTwo);
    auto [c10, b10] = run("b5_10", 3);
    QVERIFY(c10.accepted);
    QCOMPARE(c10.best.X, 14);
    QCOMPARE(c10.best.kenji, 2);
    QCOMPARE(*b10.kPlan, 2);
    QCOMPARE(b10.rule, 'A');
}

void CelebiCalibrationTest::bandRandom()
{
    int n = 0, bad = 0;
    for (const auto &b : G()["band"])
    {
        auto obs = bandObs(b["obs"]);
        double t = b["tms"].get<double>();
        bool ok = sameAdvice(bandAdvice(obs, t), bandPosterior(obs, t), b["advice"], b["post"]);
        bad += !ok;
        n++;
    }
    qInfo("band_posterior / band_advice: %d/%d random observation sets identical to the prototype", n - bad, n);
    QCOMPARE(bad, 0);
}
