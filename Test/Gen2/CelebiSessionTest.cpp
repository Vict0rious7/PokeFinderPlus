/*
 * PokeFinderPlus: new file. CelebiSession tests (see CelebiSessionTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiSessionTest.hpp"
#include <Core/Gen2/CelebiSession.hpp>
#include <QDir>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>
#include <cmath>

using namespace Gen2;

namespace
{
    const json &G()
    {
        return Gen2Test::golden("gen2_session.json");
    }

    /// session JSON equal to the prototype's dict (p_band compared as a number)
    bool sameSession(const json &a, const json &b)
    {
        json x = a, y = b;
        if (x.contains("p_band") && y.contains("p_band"))
        {
            if (std::abs(x["p_band"].get<double>() - y["p_band"].get<double>()) > 1e-12)
            {
                return false;
            }
            x.erase("p_band");
            y.erase("p_band");
        }
        return x == y;
    }
}

void CelebiSessionTest::resavePaths()
{
    int n = 0;
    for (const auto &seq : G()["resave"])
    {
        Session s = Session::fromJson(seq[0]["before"]);
        for (const auto &st : seq)
        {
            QVERIFY(sameSession(s.toJson(), st["before"]));
            auto lines = resaveStep(s);
            QCOMPARE(lines, st["lines"].get<std::vector<std::string>>());
            QVERIFY2(sameSession(s.toJson(), st["after"]), s.toJson().dump().c_str());
            if (!s.kenjiClass)
            {
                s.kenjiClass = 2; // the recalibration found class >= 2 (as the golden sequence)
            }
            n++;
        }
    }
    qInfo("resave_step: %d steps (class 1 -> >= 2, class 0, class >= 2 -> recalibrate, hint from the 2nd) identical", n);
}

void CelebiSessionTest::calibrateUpdate()
{
    // the calibrate --session update on the b5 calibration (10 attempts): rebuild the verdict and band from the cal golden
    const json &cal = Gen2Test::golden("gen2_cal.json");
    const json *cs = nullptr;
    for (const auto &c : cal["cases"])
    {
        if (c["name"] == "b5_10")
        {
            cs = &c;
        }
    }
    QVERIFY(cs);
    std::vector<Attempt> att;
    for (const auto &x : (*cs)["attempts"])
    {
        Attempt t;
        for (int i = 0; i < 6; i++)
        {
            t.stats[i] = x["stats"][i].get<int>();
        }
        t.ingame = x["ingame"].get<std::string>();
        t.reset = x.value("reset", std::string("A"))[0];
        att.push_back(t);
    }
    CalIndex inv;
    for (const auto &e : (*cs)["grid"])
    {
        CalKey k { e[0].get<int>(), e[1].get<int>(), e[2].get<int>(), static_cast<std::uint16_t>(std::stoi(e[3].get<std::string>(), nullptr, 16)) };
        for (const auto &x : e[4])
        {
            inv[k].push_back({ x[0].get<int>(), x[1].get<int>(), x[2].get<int>(), x[3].get<int>() });
        }
    }
    Verdict v = calVerdict(calRank(att, inv, { 1, 2 }, 3), static_cast<int>(att.size()));
    auto ev = carryEvidence(att, inv, v.best.kenji, v.best.X, 3);
    std::vector<BandObs> obs;
    for (std::size_t i = 0; i < att.size(); i++)
    {
        obs.push_back({ ev[i], hm(att[i].ingame) % 60, att[i].reset });
    }
    Session s = Session::fromJson(G()["calibrate"]["before"]);
    sessionAfterCalibration(s, v, bandAdvice(obs, 1700), static_cast<int>(att.size()), 1700, 3, "en");
    QVERIFY2(sameSession(s.toJson(), G()["calibrate"]["after"]), s.toJson().dump().c_str());
}

void CelebiSessionTest::fileRoundTrip()
{
    Session s = Session::fromJson(G()["resave"][0].back()["after"]);
    QString path = QDir::temp().filePath("pokefinderplus_gen2_session_test.json");
    sessionSave(path.toStdString(), s);
    QCOMPARE(sessionLoad(path.toStdString()).toJson().dump(), s.toJson().dump()); // strings: MSVC can't print json in QCOMPARE
    QFile::remove(path);
    QCOMPARE(sessionLoad("").toJson()["needs_calibration"].get<bool>(), true);
}

void CelebiSessionTest::resaveGate()
{
    auto romFr = Gen2Test::langRom("fr");
    auto romEn = Gen2Test::langRom("en");
    auto victor = Gen2Test::namedSave("victor");
    auto enTest = Gen2Test::namedSave("en_test");
    if (!romFr || !romEn || !victor || !enTest)
    {
        Gen2Test::countSkip("CelebiSessionTest::resaveGate: needs the FR and EN ROMs and both saves");
        QSKIP("needs the FR and EN ROMs and both saves");
    }
    const json &plans = Gen2Test::golden("gen2_plan.json");
    Plan pfr = Gen2Test::planFromJson(plans["plans"]["fr"]);
    Plan pen = Gen2Test::planFromJson(plans["en_test"]["plan"]);
    const auto &cases = G()["gate"];
    int n = static_cast<int>(cases.size());
    std::vector<std::string> fail(n);
    std::vector<std::array<int, 2>> same(n);
    parallelFor(n, 0, [&](int i) {
        const auto &c = cases[i];
        CrystalSave orig(c["save"] == "victor" ? *victor : *enTest);
        if (!c["K"].is_null())
        {
            orig.wr8(Sym::wKenjiBreakTimer, c["K"].get<std::uint8_t>());
        }
        Bytes origB = orig.bytes();
        Bytes res = coreResave(romFr, origB, pfr, Gen2Test::rtcFromJson(c["moment"]));
        if (Gen2Test::fnv64(res.data(), res.size()) != c["resaved"].get<std::string>())
        {
            fail[i] = "re-saved file differs";
            return;
        }
        CrystalSave rs(res);
        if (rs.rd8(Sym::wKenjiBreakTimer) != c["K_after"].get<int>())
        {
            fail[i] = "K after";
            return;
        }
        Minimal mini = readMinimal(rs);
        Bytes rb[2];
        for (int k = 1; k <= 2; k++)
        {
            Minimal m = mini;
            m.kenji = k;
            rb[k - 1] = applyMinimal(CrystalSave(origB), m).bytes();
        }
        bool en = c["lang"] == "en";
        const Plan &P = en ? pen : pfr;
        const RomPtr &rom = en ? romEn : romFr;
        auto p1 = c["plan1"].get<std::vector<int>>();
        auto core = std::make_unique<VcCore>();
        same[i] = { 0, 0 };
        for (const auto &r : c["rows"])
        {
            auto e = r["e"].get<std::vector<int>>();
            Bytes j = shifted(build(planEvents(P, p1[0] + e[0], p1[1] + e[1], p1[2] + e[2])), r["X"].get<int>());
            Rtc rtc = Gen2Test::rtcFromJson(r["rtc"]);
            std::string got[3];
            const Bytes *sb[3] = { &res, &rb[0], &rb[1] };
            for (int k = 0; k < 3; k++)
            {
                boot(*core, rom, *sb[k], rtc);
                RunResult rr = run(*core, j);
                got[k] = rr.ret > 0 ? hex(*rr.enemy) : "";
            }
            const char *names[3] = { "truth", "k1", "k2" };
            for (int k = 0; k < 3; k++)
            {
                std::string want = r[names[k]].is_null() ? "" : r[names[k]].get<std::string>();
                if (got[k] != want)
                {
                    fail[i] = std::string("timeline differs from the prototype (") + names[k] + ")";
                    return;
                }
            }
            same[i][0] += got[1] == got[0];
            same[i][1] += got[2] == got[0];
        }
    });
    for (int i = 0; i < n; i++)
    {
        const auto &c = cases[i];
        int kAfter = c["K_after"].get<int>(), cls = kenjiClass(kAfter);
        int kb = c["K_before"].get<int>();
        // the class the tool assumes: saved 0 / 1 -> >= 2 (no attempts); saved >= 2 -> the class recalibration finds
        int assumed = kenjiClass(kb) <= 1 ? 2 : cls;
        QVERIFY2(fail[i].empty(), fail[i].c_str());
        QCOMPARE(same[i][assumed - 1], 20);
        QCOMPARE(assumed, cls == 1 ? 1 : 2);
        QVERIFY(same[i][2 - assumed] <= 6); // the other class is distinguishable
        qInfo("re-save gate: %s K %d -> %d, moment %02d:%02d: rebuild class 1 %d/20, class >=2 %d/20 (tool assumes %s)",
              c["save"].get<std::string>().c_str(), kb, kAfter, c["moment"][2].get<int>(), c["moment"][1].get<int>(), same[i][0], same[i][1],
              assumed == 1 ? "1" : ">=2");
    }
}
