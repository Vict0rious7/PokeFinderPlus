/*
 * PokeFinderPlus: new file. CelebiPlan tests (see CelebiPlanTest.hpp).
 *
 * Copyright (C) 2026 Vict0rious7. GPL-3.0-or-later (see LICENSE).
 */

#include "CelebiPlanTest.hpp"
#include <Core/Gen2/CelebiPlan.hpp>
#include <QTest>
#include <Test/Gen2/Gen2TestFiles.hpp>

using namespace Gen2;

void CelebiPlanTest::timelineHelpers()
{
    // ROM-free: the prototype's build / plan_events / shifted / hm / chip_rtc semantics
    Bytes j = build({ { -2, 3, BTN_A }, { 2, 5, BTN_B }, { 3990, 4100, BTN_A } });
    QCOMPARE(j.size(), std::size_t(4000));
    QCOMPARE(j[0], std::uint8_t(1));
    QCOMPARE(j[2], std::uint8_t(3));
    QCOMPARE(j[4], std::uint8_t(2));
    QCOMPARE(j[5], std::uint8_t(0));
    QCOMPARE(j[3999], std::uint8_t(1));
    Bytes s = shifted(j, 2);
    QCOMPARE(s[0], std::uint8_t(0));
    QCOMPARE(s[2], std::uint8_t(1));
    QCOMPARE(s[4], std::uint8_t(3));
    Bytes t = shifted(j, -3);
    QCOMPARE(t[0], std::uint8_t(2));
    QCOMPARE(t[3999], std::uint8_t(1)); // the last value repeats
    QCOMPARE(hm("20:30"), 1230);
    QCOMPARE(hm("2:18:40"), 138);
    QCOMPARE(chipRtc(hm("20:30"), hm("2:18")), (Rtc { 30, 12, 18, 117, 0 }));
    QCOMPARE(chipRtc(hm("01:10"), hm("2:18")), (Rtc { 30, 52, 22, 117, 0 }));
    Plan p = Gen2Test::planFromJson(Gen2Test::golden("gen2_plan.json")["plans"]["fr"]);
    Events ev = planEvents(p, -3, 1, 1933);
    QCOMPARE(ev.size(), p.events.size() + 1);
    QCOMPARE(ev[9].start, p.events[9].start - 3);
    QCOMPARE(ev[10].end, p.events[10].end + 1);
    QCOMPARE(ev.back(), (Event { 1933, 1938, BTN_A }));
}

void CelebiPlanTest::discoverPlans()
{
    const json &g = Gen2Test::golden("gen2_plan.json");
    auto victor = Gen2Test::namedSave("victor");
    auto enTest = Gen2Test::namedSave("en_test");
    int n = 0, bad = 0, skipped = 0;
    auto check = [&](const std::string &lang, const Bytes &sram, const Rtc &rtc, const json &want) {
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            skipped++;
            return;
        }
        Plan p = discover(rom, sram, rtc);
        p.lang = want.value("lang", lang);
        n++;
        json got = Gen2Test::planToJson(p);
        bool ok = got["events"] == want["events"] && got["presses"] == want["presses"] && got["final_min"] == want["final_min"];
        if (!ok)
        {
            bad++;
            qWarning("%s plan differs: %s", lang.c_str(), got.dump().c_str());
        }
    };
    if (!victor || !enTest)
    {
        Gen2Test::countSkip("CelebiPlanTest::discoverPlans: no Victor's save or EN test save");
        QSKIP("needs the saves (POKEFINDERPLUS_GEN2_SAVES)");
    }
    const Rtc r0 = Gen2Test::rtcFromJson(g["rtcs"][0]), r1 = Gen2Test::rtcFromJson(g["rtcs"][1]);
    for (const auto &[lang, P] : g["plans"].items())
    {
        check(lang, *victor, r0, P);
    }
    check("fr", *victor, r1, g["plans"]["fr"]);
    check("en", *enTest, Gen2Test::rtcFromJson(g["en_test"]["rtc"]), g["en_test"]["plan"]);
    if (skipped)
    {
        Gen2Test::countSkip("CelebiPlanTest::discoverPlans: " + std::to_string(skipped) + " plan(s) without their ROM");
    }
    qInfo("discover: %d/%d plans identical to the prototype (%d skipped)", n - bad, n, skipped);
    QCOMPARE(bad, 0);
    if (n == 0)
    {
        QSKIP("needs the Crystal ROMs (POKEFINDERPLUS_GBC_ROMS)");
    }
}

void CelebiPlanTest::timerPhases()
{
    const json &g = Gen2Test::golden("gen2_plan.json");
    auto victor = Gen2Test::namedSave("victor");
    if (!victor)
    {
        Gen2Test::countSkip("CelebiPlanTest::timerPhases: no Victor's save");
        QSKIP("needs Victor's save (POKEFINDERPLUS_GEN2_SAVES)");
    }
    int n = 0, bad = 0, skipped = 0;
    for (const auto &t : g["timers"])
    {
        std::string lang = t["lang"].get<std::string>();
        auto rom = Gen2Test::langRom(lang);
        if (!rom)
        {
            skipped++;
            continue;
        }
        Plan p = Gen2Test::planFromJson(g["plans"][lang]);
        auto rows = timer(rom, *victor, p, t["d6"].get<int>(), t["dyes"].get<int>(), t["final"].get<int>());
        n++;
        bool ok = rows.size() == t["rows"].size();
        for (std::size_t i = 0; ok && i < rows.size(); i++)
        {
            const auto &w = t["rows"][i];
            ok = rows[i].press == w["press"].get<std::string>() && rows[i].vbc == w["vbc"].get<int>() && rows[i].gbFrame == w["gb_frame"].get<int>()
                && rows[i].beep == w["beep"].get<int>() && rows[i].phase == w["phase"].get<int>();
        }
        bad += !ok;
        if (lang == "fr" && t["plan"].get<int>() == 1)
        {
            std::vector<int> ph;
            for (const auto &r : rows)
            {
                ph.push_back(r.phase);
            }
            QCOMPARE(ph, (std::vector<int> { 395, 280, 527, 127, 628 }));
        }
    }
    if (skipped)
    {
        Gen2Test::countSkip("CelebiPlanTest::timerPhases: " + std::to_string(skipped) + " plan(s) without their ROM");
    }
    qInfo("timer: %d/%d phase lists identical to the prototype", n - bad, n);
    QCOMPARE(bad, 0);
}
