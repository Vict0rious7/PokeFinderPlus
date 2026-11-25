/*
 * PokeFinderPlus: new file. CelebiSearch tests (see CelebiSearchTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSearchTest.hpp"
#include <Core/Gen2/CelebiCalibration.hpp>
#include <Core/Gen2/CelebiSearch.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <mutex>
#include <set>

using namespace Gen2;

namespace
{
    const json &G()
    {
        return Gen2Test::golden("gen2_search.json");
    }

    Plan plan(const std::string &lang)
    {
        return Gen2Test::planFromJson(Gen2Test::golden("gen2_plan.json")["plans"][lang]);
    }

    Hit hitFromJson(const json &h)
    {
        Hit x;
        x.d6 = h["d6"].get<int>();
        x.dyes = h["dyes"].get<int>();
        x.final = h["final"].get<int>();
        x.rtc = Gen2Test::rtcFromJson(h["rtc"]);
        std::string e = h["enemy"].get<std::string>();
        for (int i = 0; i < 8; i++)
        {
            x.enemy[i] = static_cast<std::uint8_t>(std::stoi(e.substr(2 * i, 2), nullptr, 16));
        }
        x.fOk = h["F_ok"].get<std::vector<int>>();
        x.sOk = h["s_ok"].get<std::vector<int>>();
        return x;
    }

    std::vector<Hit> hits(const std::string &set)
    {
        std::vector<Hit> v;
        for (const auto &h : G()["picks"][set]["hits"])
        {
            v.push_back(hitFromJson(h));
        }
        return v;
    }

    /// plans 1 and 2 of a fsearch set (pick on the stored hits)
    std::pair<Hit, Hit> plans12(const std::string &set)
    {
        auto v = hits(set);
        auto [a, b] = pick(v);
        return { v[*a], v[*b] };
    }

    Rtc rtcOfHHMM(const std::string &hhmm)
    {
        return { 30, std::stoi(hhmm.substr(2, 2)), std::stoi(hhmm.substr(0, 2)), 117, 0 };
    }

    std::string leaves(const Grid &g)
    {
        std::string s;
        for (auto dv : g.dv)
        {
            s += dvHex(dv);
        }
        return s;
    }

    std::string str(const std::vector<int> &v)
    {
        std::string s;
        for (int x : v)
        {
            s += std::to_string(x) + ",";
        }
        return s;
    }
}

void CelebiSearchTest::plansOfRecord()
{
    // ROM-free: pick on the stored hits = the prototype's pick, and the SPEC table
    for (const auto &[set, p] : G()["picks"].items())
    {
        auto v = hits(set);
        auto [a, b] = pick(v);
        QVERIFY(a && b);
        QCOMPARE(static_cast<int>(*a), p["p1"].get<int>());
        QCOMPARE(static_cast<int>(*b), p["p2"].get<int>());
        std::set<int> u(v[*a].fOk.begin(), v[*a].fOk.end());
        u.insert(v[*b].fOk.begin(), v[*b].fOk.end());
        QCOMPARE(static_cast<int>(u.size()), p["union"].get<int>());
    }
    struct Rec
    {
        const char *set;
        int d6, dy, fin;
        const char *dv;
        int F;
        int d6b, dyb, finb;
        const char *dvb;
        int Fb, uni;
    };
    // SPEC v0.6 section 3.2 (Victor's save, Kenji class 1), section 2.4 (class >= 2), the EN test save (tool's view)
    const Rec rec[] = { { "fr", -3, 0, 1933, "2aaa", 42, 1, -1, 1933, "2aaa", 24, 54 },  { "de", 0, 2, 1489, "6aaa", 39, -2, 1, 1933, "2aaa", 33, 52 },
                        { "es", 1, 0, 1505, "6aaa", 42, 0, 0, 1505, "6aaa", 31, 52 },    { "en", -3, 0, 1933, "2aaa", 42, -1, 0, 2994, "eaaa", 21, 54 },
                        { "it", 2, 0, 2597, "aaaa", 44, -1, -2, 2641, "aaaa", 41, 55 },  { "fr_k2", -3, 0, 2925, "faaa", 43, 1, 0, 2925, "faaa", 24, 56 },
                        { "it_k2", -1, -1, 2869, "faaa", 41, -1, -1, 2889, "6aaa", 39, 42 }, { "en_test", 0, -2, 1518, "7aaa", 49, 1, -1, 1942, "aaaa", 14, 52 } };
    for (const auto &r : rec)
    {
        auto [p1, p2] = plans12(r.set);
        QCOMPARE(p1.d6, r.d6);
        QCOMPARE(p1.dyes, r.dy);
        QCOMPARE(p1.final, r.fin);
        QCOMPARE(dvHex(dvOf(p1.enemy)), std::string(r.dv));
        QCOMPARE(static_cast<int>(p1.fOk.size()), r.F);
        QCOMPARE(p2.d6, r.d6b);
        QCOMPARE(p2.dyes, r.dyb);
        QCOMPARE(p2.final, r.finb);
        QCOMPARE(dvHex(dvOf(p2.enemy)), std::string(r.dvb));
        QCOMPARE(static_cast<int>(p2.fOk.size()), r.Fb);
        std::set<int> u(p1.fOk.begin(), p1.fOk.end());
        u.insert(p2.fOk.begin(), p2.fOk.end());
        QCOMPARE(static_cast<int>(u.size()), r.uni);
    }
}

void CelebiSearchTest::jobKeys()
{
    // ROM-free: the job lists hold every stored job (fsearch dirs may stop early: subset), grid dirs exactly
    for (const char *set : { "fr", "de", "es", "en", "it", "fr_k2", "it_k2" })
    {
        const auto &p = G()["picks"][set];
        std::set<std::string> rt;
        for (const auto &k : p["keys"])
        {
            std::string s = k.get<std::string>();
            rt.insert(s.substr(s.size() - 4));
        }
        std::vector<Rtc> rtcs;
        for (const auto &h : rt)
        {
            rtcs.push_back(rtcOfHHMM(h));
        }
        std::set<std::string> mine;
        for (const auto &j : fsJobs(plan(std::string(set).substr(0, 2)), rtcs))
        {
            mine.insert(j.key);
        }
        QCOMPARE(mine.size(), 980u * rtcs.size());
        for (const auto &k : p["keys"])
        {
            QVERIFY2(mine.count(k.get<std::string>()), k.get<std::string>().c_str());
        }
    }
    auto keysOf = [](const std::vector<FgridJob> &v) {
        std::vector<std::string> k;
        for (const auto &j : v)
        {
            k.push_back(j.key);
        }
        std::sort(k.begin(), k.end());
        return k;
    };
    QCOMPARE(keysOf(fgridJobs(2, 3, 0)), G()["grid_keys_fg3_fr"].get<std::vector<std::string>>());
    std::vector<FgridJob> x;
    for (int X : { -1, 1, -2, 2 })
    {
        auto v = fgridJobs(2, 3, X);
        x.insert(x.end(), v.begin(), v.end());
    }
    QCOMPARE(keysOf(x), G()["grid_keys_fgx_fr"].get<std::vector<std::string>>());
    // calibration job keys need the template save only for the job contents; the keys don't depend on it
    Minimal mini;
    std::vector<Attempt> att(1);
    att[0].ingame = "20:30";
    std::vector<std::string> ck;
    for (const auto &j : calJobs(CrystalSave(), mini, att, { 1, 2 }, -40, 40, 3))
    {
        ck.push_back(j.key);
    }
    std::sort(ck.begin(), ck.end());
    QCOMPARE(ck, G()["grid_keys_cal3_fr"].get<std::vector<std::string>>());
}

void CelebiSearchTest::windowsFromSweep()
{
    // ROM-free: windows() on the prototype's sweep results
    for (const auto &s : G()["sweeps"])
    {
        std::vector<std::optional<Enemy>> res;
        for (const auto &r : s["res"])
        {
            if (r.is_null())
            {
                res.push_back(std::nullopt);
                continue;
            }
            Enemy e;
            std::string h = r.get<std::string>();
            for (int i = 0; i < 8; i++)
            {
                e[i] = static_cast<std::uint8_t>(std::stoi(h.substr(2 * i, 2), nullptr, 16));
            }
            res.push_back(e);
        }
        QCOMPARE(windows(res, hm(s["offset"].get<std::string>())), s["windows"].get<std::vector<std::string>>());
    }
}

void CelebiSearchTest::fsearchJobs()
{
    auto victor = Gen2Test::namedSave("victor");
    if (!victor)
    {
        Gen2Test::countSkip("CelebiSearchTest::fsearchJobs: no Victor's save");
        QSKIP("needs Victor's save (POKEFINDERPLUS_GEN2_SAVES)");
    }
    const auto &jobs = G()["fsjobs"];
    int n = static_cast<int>(jobs.size());
    std::vector<int> ok(n, -1);
    parallelFor(n, 0, [&](int i) {
        const auto &j = jobs[i];
        std::string lang = j["lang"].get<std::string>();
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            return;
        }
        Bytes sram = *victor;
        if (!j["kenji"].is_null())
        {
            // class >= 2 template: Victor's save with Kenji 4 (vic_k2.sav = apply_minimal(save, kenji=2))
            CrystalSave t(sram);
            Minimal m = readMinimal(t);
            m.kenji = j["kenji"].get<int>();
            sram = applyMinimal(t, m).bytes();
        }
        std::vector<Rtc> rtcs;
        for (const auto &h : j["rtcs"])
        {
            rtcs.push_back(rtcOfHHMM(h.get<std::string>()));
        }
        Plan p = plan(lang);
        for (const auto &fj : fsJobs(p, rtcs))
        {
            if (fj.key == j["key"].get<std::string>())
            {
                auto got = fsJob(rom, sram, p, fj);
                std::vector<Hit> want;
                for (const auto &h : j["hits"])
                {
                    want.push_back(hitFromJson(h));
                }
                ok[i] = got == want;
            }
        }
    });
    int done = 0, bad = 0;
    for (int v : ok)
    {
        done += v >= 0;
        bad += v == 0;
    }
    if (done < n)
    {
        Gen2Test::countSkip("CelebiSearchTest::fsearchJobs: " + std::to_string(n - done) + " job(s) without their ROM");
    }
    qInfo("fsearch: %d/%d sampled jobs identical to the prototype (incl. the jobs holding plans 1 + 2)", done - bad, done);
    QCOMPARE(bad, 0);
}

void CelebiSearchTest::gridJobs()
{
    auto victor = Gen2Test::namedSave("victor");
    auto rom = Gen2Test::langRom("fr");
    if (!victor || !rom)
    {
        Gen2Test::countSkip("CelebiSearchTest::gridJobs: no FR ROM or Victor's save");
        QSKIP("needs the FR ROM and Victor's save");
    }
    Plan p = plan("fr");
    auto [p1, p2] = plans12("fr");
    const Hit *pl[2] = { &p1, &p2 };
    std::vector<int> X4 = { -1, 1, -2, 2 };
    std::vector<FgridJob> fj = fgridJobs(2, 3, 0);
    for (int X : X4)
    {
        auto v = fgridJobs(2, 3, X);
        fj.insert(fj.end(), v.begin(), v.end());
    }
    CrystalSave tpl(*victor);
    std::vector<Attempt> att(1);
    att[0].ingame = "20:30";
    auto cj = calJobs(tpl, readMinimal(tpl), att, { 1, 2 }, -40, 40, 3);
    const auto &grids = G()["grids"];
    int n = static_cast<int>(grids.size());
    std::vector<int> ok(n, 0);
    parallelFor(n, 0, [&](int i) {
        const auto &g = grids[i];
        std::string key = g["key"].get<std::string>();
        Grid got;
        if (key.starts_with("f_"))
        {
            for (const auto &j : fj)
            {
                if (j.key == key)
                {
                    const Hit &h = *pl[j.plan];
                    got = fgridJob(rom, *victor, p, h.d6, h.dyes, h.final, h.rtc, j.F, j.D, j.X);
                }
            }
        }
        else
        {
            for (const auto &j : cj)
            {
                if (j.key == key)
                {
                    got = gridX(rom, j.sram, j.rtc, p, 0, 0, p1.final, j.X, j.D);
                }
            }
        }
        ok[i] = got.D == g["D"].get<int>() && leaves(got) == g["leaves"].get<std::string>();
    });
    int good = 0;
    for (int v : ok)
    {
        good += v;
    }
    qInfo("grids: %d/%d sampled jobs (fg3_fr, fgx_fr, cal3_fr; 343 leaves each) identical to the prototype", good, n);
    QCOMPARE(good, n);
}

void CelebiSearchTest::regrRows()
{
    auto victor = Gen2Test::namedSave("victor");
    if (!victor)
    {
        Gen2Test::countSkip("CelebiSearchTest::regrRows: no Victor's save");
        QSKIP("needs Victor's save (POKEFINDERPLUS_GEN2_SAVES)");
    }
    const auto &rows = G()["regr"];
    int n = static_cast<int>(rows.size());
    std::vector<int> ok(n, -1);
    parallelFor(n, 0, [&](int i) {
        const auto &r = rows[i];
        std::string lang = r["lang"].get<std::string>();
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            return;
        }
        auto [p1, p2] = plans12(lang);
        const Hit &h = r["plan"].get<int>() == 0 ? p1 : p2;
        Grid g = fgridJob(rom, *victor, plan(lang), h.d6, h.dyes, h.final, h.rtc, r["F"].get<int>(), 2, 0);
        ok[i] = dvHex(g.at(0, 0, 0)) == r["dv"].get<std::string>();
    });
    int done = 0, bad = 0;
    for (int v : ok)
    {
        done += v >= 0;
        bad += v == 0;
    }
    if (done < n)
    {
        Gen2Test::countSkip("CelebiSearchTest::regrRows: " + std::to_string(n - done) + " row(s) without their ROM");
    }
    qInfo("regr.py rows: %d/%d match fg3_<lang>", done - bad, done);
    QCOMPARE(bad, 0);
}

void CelebiSearchTest::sweeps()
{
    auto victor = Gen2Test::namedSave("victor");
    if (!victor)
    {
        Gen2Test::countSkip("CelebiSearchTest::sweeps: no Victor's save");
        QSKIP("needs Victor's save (POKEFINDERPLUS_GEN2_SAVES)");
    }
    for (const auto &s : G()["sweeps"])
    {
        std::string lang = s["lang"].get<std::string>();
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            Gen2Test::countSkip("CelebiSearchTest::sweeps: no " + lang + " ROM");
            continue;
        }
        auto res = sweep(rom, *victor, planEvents(plan(lang), s["d6"].get<int>(), s["dyes"].get<int>(), s["final"].get<int>()));
        int same = 0;
        for (int t = 0; t < 1440; t++)
        {
            same += (s["res"][t].is_null() ? !res[t].has_value() : (res[t] && hex(*res[t]) == s["res"][t].get<std::string>()));
        }
        QCOMPARE(same, 1440);
        QCOMPARE(windows(res, hm(s["offset"].get<std::string>())), s["windows"].get<std::vector<std::string>>());
    }
}
