/*
 * PokeFinderPlus: see CelebiToolTest.hpp.
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiToolTest.hpp"
#include <Core/Gen2/CelebiTool.hpp>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>

using namespace Gen2;

namespace
{
    /// Victor's save as the player sees it: Fast, Mono, frame 7, no roamers, Lass there, no swarm
    Visible victorVisible()
    {
        Visible v;
        v.textSpeed = TextSpeed::Fast;
        v.stereo = false;
        v.frame = 7;
        v.raikou = v.entei = v.swarm = false;
        v.lass = true;
        return v;
    }
}

void CelebiToolTest::templateValues()
{
    CrystalSave t(templateSave());
    QCOMPARE(templateSave().size(), std::size_t(CrystalSave::SIZE));
    Minimal m = readMinimal(t);
    QCOMPARE(m.frame, 7);
    QCOMPARE(m.F, 26);
    QCOMPARE(m.playSec, 42);
    QCOMPARE(m.start, (std::array<int, 4> { 2, 2, 18, 3 }));
    QCOMPARE(m.kenji, 1);
    // both checksums are right: fixing changes nothing
    CrystalSave f(templateSave());
    QVERIFY(f.fix().bytes() == templateSave());
    // no name left: the player name field reads "PLAYER"
    auto name = t.rd(0xd47d, 7);
    QCOMPARE(name, (std::vector<std::uint8_t> { 0x8f, 0x8b, 0x80, 0x98, 0x84, 0x91, 0x50 }));
}

void CelebiToolTest::toolSaveEqualsMakesave()
{
    CrystalSave t(templateSave());
    Minimal m = readMinimal(t);
    m.kenji = 2;
    m.start = { m.start[0], 22, 30, 40 };
    m.stereo = true;
    m.textSpeed = TextSpeed::Slow;
    m.raikou = true;
    Visible v = victorVisible();
    v.stereo = true;
    v.textSpeed = TextSpeed::Slow;
    v.raikou = true;
    QVERIFY(toolSave(v, 2, std::array<int, 3> { 22, 30, 40 }) == applyMinimal(t, m).bytes());
    QVERIFY(toolSave(victorVisible(), 1) == CrystalSave(templateSave()).fix().bytes());
}

void CelebiToolTest::roundTrips()
{
    Visible v = victorVisible();
    v.textSpeed = TextSpeed::Instant;
    v.swarm = true;
    QVERIFY(visibleFromJson(toJson(v)) == v);
    Attempt a;
    a.stats = { 100, 68, 73, 68, 68, 71 };
    a.ingame = "23:24";
    a.reset = 'B';
    Attempt b = attemptFromJson(toJson(a));
    QVERIFY(b.stats == a.stats && b.ingame == a.ingame && b.reset == a.reset);
    HuntAttempt h { 2, { 101, 70, 70, 70, 70, 70 }, -1 };
    HuntAttempt h2 = huntAttemptFromJson(toJson(h));
    QVERIFY(h2.plan == 2 && h2.stats == h.stats && h2.timerMoved == -1);
    Hit hit { -3, 0, 1933, { 30, 12, 18, 117, 0 }, { 0xfb, 0x00, 0xd7, 0xdb, 0xf6, 0xf8, 0x2a, 0xaa }, { 1, 2, 3 }, { 4, 5 } };
    QVERIFY(hitFromJson(toJson(hit)) == hit);
    HuntSetup s = huntSetup(v, 2, 1, 20, 30);
    HuntSetup s2 = huntSetupFromJson(v, toJson(s));
    QVERIFY(s2.sram == s.sram && s2.rtc == s.rtc && s2.k == 1 && s2.kenji == 2);

    GridCache c;
    Grid g;
    g.X = -21;
    g.D = 3;
    for (int i = 0; i < 343; i++)
    {
        g.dv.push_back(static_cast<std::uint16_t>(i * 977));
    }
    c["g_1_0_23_-21"] = g;
    g.X = 2;
    c["f_0_07_x+2"] = g;
    QTemporaryDir dir;
    std::string path = QDir(dir.path()).filePath("t.grids").toStdString();
    saveGrids(path, c);
    GridCache back = loadGrids(path);
    QCOMPARE(back.size(), c.size());
    for (const auto &[k, x] : c)
    {
        QVERIFY(back.count(k) && back[k].X == x.X && back[k].D == x.D && back[k].dv == x.dv);
    }
    QVERIFY(loadGrids(path + ".missing").empty());
    QCOMPARE(framesToMs(-21), -351);
    QCOMPARE(framesToMs(5), 84);
    QVERIFY(shinyStats({ 105, 61, 64, 64, 64, 64 }) == dvShiny(*dvCands({ 105, 61, 64, 64, 64, 64 }).begin()));
    QVERIFY(impossibleStats({ 1, 1, 1, 1, 1, 1 }));
}

void CelebiToolTest::plansFromTemplate()
{
    // the tool discovers each language's plan on the template with the player's visible values: same plan as the
    // prototype on Victor's own save
    const json &g = Gen2Test::golden("gen2_plan.json");
    int n = 0;
    for (const auto &[lang, P] : g["plans"].items())
    {
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            continue;
        }
        Plan p = discover(rom, toolSave(victorVisible(), 2), chipRtc(hm(PLAN_INGAME), 0));
        p.lang = lang;
        json got = Gen2Test::planToJson(p);
        QVERIFY2(got["events"] == P["events"] && got["presses"] == P["presses"] && got["final_min"] == P["final_min"], lang.c_str());
        n++;
    }
    if (n == 0)
    {
        Gen2Test::countSkip("CelebiToolTest::plansFromTemplate: no Crystal ROM");
        QSKIP("needs the Crystal ROMs (POKEFINDERPLUS_GBC_ROMS)");
    }
    qInfo("plans from the template: %d/5 languages identical to the prototype's", n);
}

void CelebiToolTest::calibrationEndToEnd()
{
    auto rom = Gen2Test::langRom("fr");
    if (!rom || qEnvironmentVariable("POKEFINDERPLUS_GEN2_SLOW") != "1")
    {
        Gen2Test::countSkip("CelebiToolTest::calibrationEndToEnd: opt-in (POKEFINDERPLUS_GEN2_SLOW=1 and the FR ROM; ~17 min)");
        QSKIP("opt-in: POKEFINDERPLUS_GEN2_SLOW=1 with the FR ROM");
    }
    const json *c1 = nullptr;
    for (const auto &c : Gen2Test::golden("gen2_cal.json")["cases"])
    {
        if (c["name"] == "cal1" && c["D"] == 3)
        {
            c1 = &c;
        }
    }
    QVERIFY(c1);
    std::vector<Attempt> att;
    for (const auto &a : (*c1)["attempts"])
    {
        att.push_back(attemptFromJson(a));
    }
    Plan p = Gen2Test::planFromJson(Gen2Test::golden("gen2_plan.json")["plans"]["fr"]);
    GridCache cache;
    CalOutcome r = calibrate(rom, p, victorVisible(), att, 3, 1700, cache);
    QVERIFY(r.complete);
    QCOMPARE(int(cache.size()), 648);
    QCOMPARE(r.state, CalState::Accepted);
    QCOMPARE(r.verdict.best.X, -21);
    QCOMPARE(r.verdict.best.kenji, 1);
    const json &rank = (*c1)["rank"];
    QCOMPARE(int(r.rank.size()), int(rank.size()));
    for (std::size_t i = 0; i < r.rank.size(); i++)
    {
        QCOMPARE(r.rank[i].explained, rank[i]["explained"].get<int>());
        QCOMPARE(r.rank[i].kenji, rank[i]["kenji"].get<int>());
        QCOMPARE(r.rank[i].X, rank[i]["X"].get<int>());
        QVERIFY(r.rank[i].carries == rank[i]["carries"].get<std::vector<int>>());
    }
    QVERIFY(r.band);
    QCOMPARE(r.band->rule, 'A');
    // a second run reuses every grid
    QCOMPARE(calibrationTodo(p, victorVisible(), att, 3, cache), 0);
    qInfo("calibration end to end (cal1): accepted, X %d, Kenji %d, rank identical (%zu fits)", r.verdict.best.X, r.verdict.best.kenji,
          r.rank.size());
}
